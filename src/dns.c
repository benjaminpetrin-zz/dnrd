
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

#include <assert.h>

#include "dns.h"
#include "lib.h"
#include "common.h"
#include "check.h"


static unsigned char valid_char[256];

/* currently we accept everything... */
unsigned char tolerant_mode = 1;

/* init the table for valid chars */
void init_dns(void) {
	int i = 0;
	memset(valid_char, tolerant_mode, sizeof(valid_char));
	for (i = '0'; i<='9'; i++) valid_char[i] = 1;
	for (i = 'A'; i<='Z'; i++) valid_char[i] = 1;
	for (i = 'a'; i<='z'; i++) valid_char[i] = 1;
	valid_char['-'] = 1;
}

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
    int	i, j;

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


static int get_objname(const unsigned char *msg, const int msgsize, int index,
											 char *dest, const int destsize) {
	unsigned int i=index;
	int j = 0;
	int depth = 0;
	int compressed = 0;
	unsigned int c;

	/* we refuse to try to decode anything in the header or after the packet */
	if (index < PACKET_DATABEGIN || i >= msgsize) return(-1);

	while ((c = msg[i++]) != '\0') {
		if (i >= msgsize) return (-1);
		
		/* check if it is a pointer */
		if ((c & 0xc0) == 0xc0) {
			/* check that next byte is readable */
			if (i >= msgsize) return (-1); 

			/* Avoid circular pointers. */
			depth++;
			if ((depth >= msgsize) || (depth > UDP_MAXSIZE) ) return(-1);

			/* we store the first location */
			if (!compressed) compressed = i+1;

			/* check that the pointer points within limits */
			if ((i = ((c & 0x3f) << 8) + msg[i]) >= msgsize) return(-1);
			continue;
		} else if (c < 64) {
			/* check that label length don't cross any borders */
			if ( ((j + c + 1) >= destsize)	|| ((i + c) >= msgsize)) return(-1);

			while (c--) {
				if (valid_char[msg[i]])	
					dest[j++] = msg[i++];
				else return(-1);
			}
			dest[j++] = '.';
		} else return(-1); /* a label cannot be longer than 63 */
	}

	if (--j >= (destsize) ) return(-1); /* we need space for '\0' */
	dest[j] = '\0';
	/* if we used compression, return the location from the first ptr */
	return (compressed ? compressed : i);
}

int snprintf_cname(char *msg, const int msgsize, /* the dns packet */
										int index, /* where in the DNS packet the name is */
										char *dest, int destsize) { /* where to store the cname */
/*	unsigned char *p = &msg[index];*/ /* UNUSED */
	static const char malformatted[] = "(malformatted)";
	if (get_objname(msg, msgsize, index, dest, destsize) <0) {
		strncpy(dest, malformatted, destsize);
		return -1;
	}
	return 0;
}


static int read_record(dnsheader_t *x, rr_t *y,
											 int index, int question,
											 const int packetsize)
{
    int	i, len;
/*    unsigned short int conv;*/ /* UNUSED */

    /*
     * Read the name of the resource ...
     */

		i = get_objname(x->packet, packetsize, index, y->name, sizeof(y->name));


    /* safe to read TYPE and CLASS? */
    if ( (i < 0) || (i+4 > packetsize) ) return(-1);
		//    if ((here+4) > limit) return (NULL);

    /*
     * ... the type of data ...
     */

		y->type = ntohs(*(unsigned short *)(&x->packet[i]));
		//    memcpy(&conv, here, sizeof(unsigned short int));
		//    y->type = ntohs(conv);
		//    here += 2;
		i += 2;

    /*
     * ... and the class type.
     */

		y->class = ntohs( *(unsigned short *)(&x->packet[i]));
		//    memcpy(&conv, here, sizeof(unsigned short int));
		//    y->class = ntohs(conv);
		//    here += 2;
		i += 2;

    /*
     * Question blocks stop here ...
     */

    if (question != 0) return (i);


    /*
     * ... while answer blocks carry a TTL and the actual data.
     */

    /* safe to read TTL and RDLENGTH? */
		//    if ((here+6) > limit) return (NULL);
		if (i+6 > packetsize) return (-1);

		y->ttl = ntohl( *(unsigned long int *)(&x->packet[i]) );
		i += 4;
    //memcpy(&y->ttl, here, sizeof(unsigned long int));
    //y->ttl = ntohl(y->ttl);
    //here += 4;

    /*
     * Fetch the resource data.
     */
		len = y->len = ntohs( *(unsigned short int *)(&x->packet[i])  );
		i += 2;
		//    memcpy(&conv, here, sizeof(unsigned short int));
		//    len = y->len = ntohs(conv);
		//    here += 2;

    /* safe to read RDATA? */
		if (i + y->len > packetsize)  return (-1); 
		//    if ((here + y->len) > limit) return (NULL);
    
    if (y->len > sizeof(y->data) - 4) {
	/* we are actually losing data here. we should give up */
	/* return -1; */
	y->len = sizeof(y->data) - 4;
    }

		//memcpy(y->data, here, y->len);
    //here += len;
		memcpy(y->data, &x->packet[i], y->len);
		i += y->len;
    y->data[y->len] = 0;

    return (i);
}


int dump_dnspacket(char *type, unsigned char *packet, int len)
{
  int	i, index;
  rr_t	y;
  dnsheader_t *x;
	//  unsigned char *limit;

  if (opt_debug < 4) return 0;

  if ((x = decode_header(packet, len)) == NULL ) {
    return (-1);
  }
	//  limit = x->packet + len;

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
  
	if ((index = read_record(x, &y, PACKET_DATABEGIN, 1, len)) <0) {
	//  if ((x->here = read_record(x, &y, x->here, 1, limit)) == NULL) {
    free_packet(x);
    return(-1);
  }

  fprintf(stderr, "  name= %s, type= %d, class= %d\n",
	  y.name, y.type, y.class);
  
  fprintf(stderr, "ans= %u\n", x->ancount);
  for (i = 0; i < x->ancount; i++) {
		if ((index = read_record(x, &y, index, 0, len)) <0) {
			//if ((x->here = read_record(x, &y, x->here, 0, limit)) == NULL) {
      free_packet(x);
      return(-1);
    }
    fprintf(stderr, "  name= %s, type= %d, class= %d, ttl= %lu\n",
	    y.name, y.type, y.class, y.ttl);
  }
	    
  fprintf(stderr, "ns= %u\n", x->nscount);
  for (i = 0; i < x->nscount; i++) {
		if ((index = read_record(x, &y, index, 0, len)) <0) {
			//if ((x->here = read_record(x, &y, x->here, 0, limit))==NULL) {
      free_packet(x);
      return(-1);
    }
    fprintf(stderr, "  name= %s, type= %d, class= %d, ttl= %lu\n",
	    y.name, y.type, y.class, y.ttl);
  }
    
  fprintf(stderr, "ar= %u\n", x->arcount);
  for (i = 0; i < x->arcount; i++) {
		if ((index = read_record(x, &y, index, 0, len)) <0) {
    //if ((x->here = read_record(x, &y, x->here, 0, limit))==NULL) {
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
 * parse_query()
 *
 * The function get_dnsquery() returns us the query part of an
 * DNS packet.  For this we must already have a dnsheader_t
 * packet which is additional work.  To speed things a little
 * bit up (we use it often) parse_query() gets the query direct.
 *
 * returns non-zero on error and zero on success.
 */
int parse_query(rr_t *y, unsigned char *msg, int len)
{
    int	k;
		//    unsigned char *here;
		int i;
		//    unsigned short int conv;
		//    unsigned const char *limit = msg+len; 

    /* If QDCOUNT, the number of entries in the question section,
     * is zero, we just give up */

    if (ntohs(((dnsheader_t *)msg)->qdcount) == 0 ) {
			/*      QDCOUNT will always be zero on responses.
							log_debug(1, "QDCOUNT was zero"); */
      return(-1);
    }

    /*
      if (ntohs(((dnsheader_t *)msg)->u) & MASK_QR ) {
      return(0);
      }
    */
    
    y->flags = ntohs(((short int *) msg)[1]);

		//    here = &msg[PACKET_DATABEGIN];
    if ((i = get_objname(msg, len, PACKET_DATABEGIN, y->name, 
														 sizeof(y->name)))< 0) 
      return(-1);
		//    y->name[k] = 0;

    /* check that there is type and class */
		if (i + 4 > len) return(-1);
		//    if (here + 4 > limit) return(0);

		y->type = ntohs(*(unsigned short *)(&msg[i]));
		i += 2;

		y->class = ntohs( *(unsigned short *)(&msg[i]));
		i += 2;
		
		//    memcpy(&conv, here, sizeof(unsigned short int));
		//    y->type = ntohs(conv);
		//    here += 2;

		//    memcpy(&conv, here, sizeof(unsigned short int));
		//    y->class = ntohs(conv);
		//    here += 2;

    /* those strings should have been checked in get_objectname */
    if ((k = strlen(y->name))) {
			if (y->name[--k] == '.') y->name[k] = '\0';
		}
		//    if (k > 0  &&  y->name[k-1] == '.') {
		//	y->name[k-1] = '\0';
		//    }

    /* should we really convert the name to lowercase?  
     * rfc1035 2.3.3
     */
    strnlwr(y->name, sizeof(y->name));
	    
    return (0);
}


