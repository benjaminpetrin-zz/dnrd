
/*

    File: dns.c
    
    Copyright (C) 1999 by Wolfgang Zekoll <wzk@quietsche-entchen.de>

    This source is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    This source is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dns.h"
#include "lib.h"
#include "common.h"

/*
static int get_objectname(unsigned char *msg, unsigned const char *limit, 
			  unsigned char **here, char *string, int strlen,
			  int k);
*/

/*
static int get_objname(unsigned char buf[], const int bufsize, int *here,
		      char name[], const int namelen) {
  if (*here > bufsize) return 0;
}
*/

int free_packet(dnsheader_t *x)
{
    free(x->packet);
    free(x);
    return (0);
}

static dnsheader_t *alloc_packet(void *packet, int len)
{
    dnsheader_t *x;

    x = allocate(sizeof(dnsheader_t));
    /*    memset(x, 0, sizeof(dnsheader_t));*/

    x->packet = allocate(len + 2);
    x->len    = len;
    memcpy(x->packet, packet, len);

    return (x);
}

static dnsheader_t *decode_header(void *packet, int len)
{
    unsigned short int *p;
    dnsheader_t *x;

    x = alloc_packet(packet, len);
    p = (unsigned short int *) x->packet;

    x->id      = ntohs(p[0]);
    x->u       = ntohs(p[1]);
    x->qdcount = ntohs(p[2]);
    x->ancount = ntohs(p[3]);
    x->nscount = ntohs(p[4]);
    x->arcount = ntohs(p[5]);

    x->here    = (char *) &x->packet[12];
    return (x);
}

static int raw_dump(dnsheader_t *x)
{
    unsigned int c;
    int	start, i, j;

    start = x->here - x->packet;
    for (i = 0; i < x->len; i += 16) {
	fprintf(stderr, "%03X -", i);

	for (j = i; j < x->len  &&  j < i+16; j++) {
	    fprintf(stderr, " %02X", ((unsigned int) x->packet[j]) & 0XFF);
	}
	for (; j < i+16; j++) {
	    fprintf(stderr, "   ");
	}

	fprintf(stderr, "  ");
	for (j = i; j < x->len  &&  j < i+16; j++) {
	    c = ((unsigned int) x->packet[j]) & 0XFF;
	    fprintf(stderr, "%c", (c >= 32  &&  c < 127) ? c : '.');
	}
	
	fprintf(stderr, "\n");
    }

    fprintf(stderr, "\n");
    return (0);
}

/*
static int get_objname(unsigned char buf[], const int bufsize, int *here,
		      char name[], const int namelen) {
  int i,p=*here, count=1000;
  unsigned int len, offs;
  if (p > bufsize) return 0;
  while (len = buf[p]) {
    
    while (len & 0x0c) {
      if (++p > bufsize) return 0;
      offs = lenbuf[p] 

}
*/


static int get_objectname(unsigned char *msg, unsigned const char *limit, 
			  unsigned char **here, char *string, int strlen,
			  int k)
{
    unsigned int len;
    int	i;

    if ((*here>=limit) || (k>=strlen)) return(-1);
    while ((len = **here) != 0) {

	*here += 1;
	if ( *here >= limit ) return(-1);
	/* If the name is compressed (see 4.1.4 in rfc1035)  */
	if (len & 0xC0) {
	    unsigned offset;
	    unsigned char *p;

	    offset = ((len & ~0xc0) << 8) + **here;
	    if ((p = &msg[offset]) >= limit) return(-1);
	    if (p == *here-1) {
	      log_msg(LOG_WARNING, "looping ptr");
	      return(-2);
	    }

	    if ((k = get_objectname(msg, limit, &p, string, RR_NAMESIZE, k))<0)
	      return(-1); /* if we cross the limit, bail out */
	    break;
	}
	else if (len < 64) {
	  /* check so we dont pass the limits */
	  if (((*here + len) > limit) || (len+k+2 > strlen)) return(-1);

	  for (i=0; i < len; i++) {
	    string[k++] = **here;
	    *here += 1;
	  }

	  string[k++] = '.';
	}
    }

    *here += 1;
    string[k] = 0;
    
    return (k);
}


static unsigned char *read_record(dnsheader_t *x, rr_t *y,
				  unsigned char *here, int question,
				  unsigned const char *limit)
{
    int	k, len;
    unsigned short int conv;

    /*
     * Read the name of the resource ...
     */

    k = get_objectname(x->packet, limit, &here, y->name, sizeof(y->name), 0);
    if (k < 0) return(NULL);
    y->name[k] = 0;

    /* safe to read TYPE and CLASS? */
    if ((here+4) > limit) return (NULL);

    /*
     * ... the type of data ...
     */

    memcpy(&conv, here, sizeof(unsigned short int));
    y->type = ntohs(conv);
    here += 2;

    /*
     * ... and the class type.
     */

    memcpy(&conv, here, sizeof(unsigned short int));
    y->class = ntohs(conv);
    here += 2;

    /*
     * Question blocks stop here ...
     */

    if (question != 0) return (here);


    /*
     * ... while answer blocks carry a TTL and the actual data.
     */

    /* safe to read TTL and RDLENGTH? */
    if ((here+6) > limit) return (NULL);
    memcpy(&y->ttl, here, sizeof(unsigned long int));
    y->ttl = ntohl(y->ttl);
    here += 4;

    /*
     * Fetch the resource data.
     */

    memcpy(&conv, here, sizeof(unsigned short int));
    len = y->len = ntohs(conv);
    here += 2;

    /* safe to read RDATA? */
    if ((here + y->len) > limit) return (NULL);
    
    if (y->len > sizeof(y->data) - 4) {
	y->len = sizeof(y->data) - 4;
    }

    memcpy(y->data, here, y->len);
    here += len;
    y->data[y->len] = 0;

    return (here);
}


int dump_dnspacket(char *type, unsigned char *packet, int len)
{
  int	i;
  rr_t	y;
  dnsheader_t *x;
  unsigned char *limit;

  if (opt_debug < 4) return 0;

  if ((x = decode_header(packet, len)) == NULL ) {
    return (-1);
  }
  limit = x->packet + len;

  if (x->u & MASK_Z) log_debug(2, "Z is set");

  fprintf(stderr, "\n");
  fprintf(stderr, "- -- %s\n", type);
  raw_dump(x);
  
  fprintf(stderr, "\n");
  fprintf(stderr, "id= %u, q= %d, opc= %d, aa= %d, wr/ra= %d/%d, "
	  "trunc= %d, rcode= %d [%04X]\n",
	  x->id, GET_QR(x->u), GET_OPCODE(x->u), GET_AA(x->u),
	  GET_RD(x->u), GET_RA(x->u), GET_TC(x->u), GET_RCODE(x->u), x->u);

  fprintf(stderr, "qd= %u\n", x->qdcount);
  
  if ((x->here = read_record(x, &y, x->here, 1, limit)) == NULL) {
    free_packet(x);
    return(-1);
  }

  fprintf(stderr, "  name= %s, type= %d, class= %d\n",
	  y.name, y.type, y.class);
  
  fprintf(stderr, "ans= %u\n", x->ancount);
  for (i = 0; i < x->ancount; i++) {
    if ((x->here = read_record(x, &y, x->here, 0, limit)) == NULL) {
      free_packet(x);
      return(-1);
    }
    fprintf(stderr, "  name= %s, type= %d, class= %d, ttl= %lu\n",
	    y.name, y.type, y.class, y.ttl);
  }
	    
  fprintf(stderr, "ns= %u\n", x->nscount);
  for (i = 0; i < x->nscount; i++) {
    if ((x->here = read_record(x, &y, x->here, 0, limit))==NULL) {
      free_packet(x);
      return(-1);
    }
    fprintf(stderr, "  name= %s, type= %d, class= %d, ttl= %lu\n",
	    y.name, y.type, y.class, y.ttl);
  }
    
  fprintf(stderr, "ar= %u\n", x->arcount);
  for (i = 0; i < x->arcount; i++) {
    if ((x->here = read_record(x, &y, x->here, 0, limit))==NULL) {
      free_packet(x);
      return(-1);
    }
    fprintf(stderr, "  name= %s, type= %d, class= %d, ttl= %lu\n",
	    y.name, y.type, y.class, y.ttl);
  }
	    
  fprintf(stderr, "\n");
  free_packet(x);

  return (0);
}



dnsheader_t *parse_packet(unsigned char *packet, int len)
{
    dnsheader_t *x;

    x = decode_header(packet, len);
    return (x);
}

/*
int get_dnsquery(dnsheader_t *x, rr_t *query)
{
    char	*here;

    if (x->qdcount == 0) {
	return (1);
    }

    here = &x->packet[12];
    read_record(x, query, here, 1);

    return (0);
}
*/

/*
 * parse_query()
 *
 * The function get_dnsquery() returns us the query part of an
 * DNS packet.  For this we must already have a dnsheader_t
 * packet which is additional work.  To speed things a little
 * bit up (we use it often) parse_query() gets the query direct.
 */
unsigned char *parse_query(rr_t *y, unsigned char *msg, int len)
{
    int	k;
    unsigned char *here;
    unsigned short int conv;
    unsigned const char *limit = msg+len; 

    /* If QDCOUNT, the number of entries in the question section,
     * is zero, we just give up */

    if (ntohs(((dnsheader_t *)msg)->qdcount) == 0 ) {
      log_debug(1, "QDCOUNT was zero");
      return(0);
    }

    /*
      if (ntohs(((dnsheader_t *)msg)->u) & MASK_QR ) {
      return(0);
      }
    */
    
    y->flags = ntohs(((short int *) msg)[1]);

    here = &msg[PACKET_DATABEGIN];
    if ((k=get_objectname(msg, limit, &here, y->name, sizeof(y->name),0))< 0) 
      return(0);
    y->name[k] = 0;

    /* check that there is type and class */
    if (here + 4 > limit) return(0);
    memcpy(&conv, here, sizeof(unsigned short int));
    y->type = ntohs(conv);
    here += 2;

    memcpy(&conv, here, sizeof(unsigned short int));
    y->class = ntohs(conv);
    here += 2;

    /* those strings should have been checked in get_objectname */
    k = strlen(y->name);
    if (k > 0  &&  y->name[k-1] == '.') {
	y->name[k-1] = '\0';
    }

    /* should we really convert the name to lowercase?  
     * rfc1035 2.3.3
     */
    strnlwr(y->name, sizeof(y->name));
	    
    return (here);
}


