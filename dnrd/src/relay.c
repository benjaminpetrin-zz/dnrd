/*
 * relay.c - the guts of the program.
 *
 * Copyright (C) 1998 Brad M. Garcia <garsh@home.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "query.h"
#include "relay.h"
#include "cache.h"
#include "common.h"
#include "tcp.h"
#include "udp.h"
#include "dns.h"
#include "domnode.h"

#ifndef EXCLUDE_MASTER
#include "master.h"
#endif

/* time interval to retry a deactivated server */
#define SINGLE_RETRY 10


/* prepare the dns packet for a not found reply */
char *set_notfound(char *msg, const int len) {
  if (len < 3) return NULL;
  /* FIXME: host to network should be called here */
  /* Set flags QR and AA */
  msg[2] |= 0x84;
  /* Set flags RA and RCODE=3 */
  msg[3] = 0x83;
  return msg;
}



/*
 * handle_query()
 *
 * In:      fromaddrp - address of the sender of the query.
 *
 * In/Out:  msg       - the query on input, the reply on output.
 *          len       - length of the query/reply
 *
 * Out:     dptr      - dptr->current contains the server to which to forward the query
 *
 * Returns:  -1 if the query is bogus
 *           1  if the query should be forwarded to the srvidx server
 *           0  if msg now contains the reply
 *
 * Takes a single DNS query and determines what to do with it.
 * This is common code used for both TCP and UDP.
 *
 * Assumptions: There is only one request per message.
 */
int handle_query(const struct sockaddr_in *fromaddrp, char *msg, int *len,
		 domnode_t **dptr)

{
    int       replylen;
    short int * flagp = &((short int *)msg)[1]; /* pointer to flags */
    domnode_t *d;

    if (opt_debug) {
	char      cname_buf[256];
	sprintf_cname(&msg[12], *len-12, cname_buf, 256);
	log_debug("Received DNS query for \"%s\"", cname_buf);
	if (dump_dnspacket("query", msg, *len) < 0)
	  log_debug("Format error");
    }

    /* First flags check. If Z flag, QR or RCODE is set, just ignore
     * the request. According to rfc1035 4.1.1 Z flag must be zero in
     * all queries and responses. We should also not have any RCODE
     */
   
    if ( ntohs(*flagp) & (MASK_Z + MASK_QR + MASK_RCODE) ) {
      log_debug("QR, Z or RCODE was set. Ignoring query");
      return(-1);
    }

   
#ifndef EXCLUDE_MASTER
    /* First, check to see if we are master server */
    if ((replylen = master_lookup(msg, *len)) > 0) {
	log_debug("Replying to query as master");
	*len = replylen;
	return 0;
    }
#endif

    /* Next, see if we have the answer cached */
    if ((replylen = cache_lookup(msg, *len)) > 0) {
	log_debug("Replying to query with cached answer.");
	*len = replylen;
	return 0;
    }

    /* get the server list for this domain */
    d=search_subdomnode(domain_list, &msg[12], *len);

    if (no_srvlist(d->srvlist)) {
      /* there is no servers for this domain, reply with "entry not found" */
	log_debug("Replying to query with \"entry not found\"");
	if (!set_notfound(msg, *len)) return -1;
	return 0;
    }

    if (d->roundrobin) set_current(d, next_active(d));
    /* Send to a server until it "times out". */
    if (d->current) {
      time_t now = time(NULL);
      if ((d->current->send_time == 0)) {
	d->current->send_time = now;
      } 
      else if (now - d->current->send_time > forward_timeout) {
	deactivate_current(d);
      }
      if (d->current) {
	log_debug("Forwarding the query to DNS server %s",
		  inet_ntoa(d->current->addr.sin_addr));
      } else {
	log_debug("All servers deactivated. Replying with \"entry not found\"");
	if (!set_notfound(msg, *len)) return -1;
	return 0;
      }

    } else {
	log_debug("All servers deactivated. Replying with \"entry not found\"");
	if (!set_notfound(msg, *len)) return -1;
	return 0;
    }
    *dptr = d;
    return 1;
}

/* Check if any deactivated server are back online again */

static void reactivate_servers(int interval) {
  time_t now=time(NULL);
  static int last_try = 0;
  domnode_t *d = domain_list;
  /*  srvnode_t *s;*/
  
  if (!last_try) last_try = now;
  /* check for reactivate servers */
  if ( (now - last_try < interval) || no_srvlist(d->srvlist)  ) 
    return;
 
  last_try = now;
  do {
    retry_srvlist(d, SINGLE_RETRY);
    if (!d->roundrobin) {
      /* find the first active server in serverlist */
      d->current=NULL;
      d->current=next_active(d);
    }
  } while ((d = d->next) != domain_list);  
}

/*
 * run()
 *
 * Abstract: This function runs continuously, waiting for packets to arrive
 *           and processing them accordingly.
 */
void run()
{
    int                maxsock;
    struct timeval     tout;
    fd_set             fdmask;
    fd_set             fds;
    int                retn;
    /*    int                i, j;*/
    domnode_t *d = domain_list;
    srvnode_t *s;

    FD_ZERO(&fdmask);
    FD_SET(isock,   &fdmask);
#ifdef ENABLE_TCP
    FD_SET(tcpsock, &fdmask);
    maxsock = (tcpsock > isock) ? tcpsock : isock;
#else
    maxsock = isock;
#endif
    do {
      if ((s=d->srvlist)) {
	while ((s=s->next) != d->srvlist) {
	  if (maxsock < s->sock) maxsock = s->sock;
	  FD_SET(s->sock, &fdmask);
	  s->send_time = 0;
	  s->send_count = 0;
	}
      }
    } while ((d=d->next) != domain_list);
    maxsock++;

    while(1) {
	tout.tv_sec  = select_timeout;
	tout.tv_usec = 0;

	fds = fdmask;

	/* Wait for input or timeout */
	retn = select(maxsock, &fds, 0, 0, &tout);
	
	/* Expire lookups from the cache */
	cache_expire();

#ifndef EXCLUDE_MASTER
	/* Reload the master database if neccessary */
	master_reinit();
#endif

	/* Remove old unanswered queries */
	dnsquery_timeout(60);

	/* reactivate servers */
	reactivate_servers(10);

	/* Handle errors */
	if (retn < 0) {
	    log_msg(LOG_ERR, "select returned %s", strerror(errno));
	    continue;
	}
	else if (retn == 0) {
	  continue;  /* nothing to do */
	}

	/* Check for replies to DNS queries */
	d=domain_list;
	do {
	  if ((s=d->srvlist)) {
	    while ((s=s->next) != d->srvlist)
	      if (FD_ISSET(s->sock, &fds)) handle_udpreply(s);
	  }
	} while ((d=d->next) != domain_list);

#ifdef ENABLE_TCP
	/* Check for incoming TCP requests */
	if (FD_ISSET(tcpsock, &fds)) handle_tcprequest();
#endif

	/* Check for new DNS queries */
	if (FD_ISSET(isock, &fds)) handle_udprequest();

    }
}
