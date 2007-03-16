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
#include <time.h>
#include <signal.h>

#include "query.h"
#include "relay.h"
#include "cache.h"
#include "common.h"
#include "tcp.h"
#include "udp.h"
#include "dns.h"
#include "domnode.h"
#include "sig.h"

#ifndef EXCLUDE_MASTER
#include "master.h"
#endif


/* prepare the dns packet for a not found reply */
/* not used anymore
char *set_notfound(char *msg, const int len) {
  if (len < 4) return NULL;
  msg[2] |= 0x84;
  msg[3] = 0x83;
  return msg;
}
*/

/* prepare the dns packet for a Server Failure reply */
char *set_srvfail(char *msg, const int len) {
  if (len < 4) return NULL;
  /* FIXME: host to network should be called here */
  /* Set flags QR and AA */
  msg[2] |= 0x84;
  /* Set flags RA and RCODE=3 */
  msg[3] = 0x82;
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
    domnode_t *d;

    if (opt_debug) {
	char      cname_buf[256];

	snprintf_cname(msg, *len, 12, cname_buf, sizeof(cname_buf));
	log_debug(3, "Received DNS query for \"%s\"", cname_buf);
	if (dump_dnspacket("query", msg, *len) < 0)
	  log_debug(3, "Format error");
    }
   
#ifndef EXCLUDE_MASTER
    /* First, check to see if we are master server */
    if ((replylen = master_lookup(msg, *len)) > 0) {
	log_debug(2, "Replying to query as master");
	*len = replylen;
	return 0;
    } else if (replylen < 0) return -1;
#endif

    /* Next, see if we have the answer cached */
    if ((replylen = cache_lookup(msg, *len)) > 0) {
	log_debug(3, "Replying to query with cached answer.");
	*len = replylen;
	return 0;
    }  else if (replylen < 0) return -1;

    /* get the server list for this domain */
    d=search_subdomnode(domain_list, &msg[12], *len);

    if (no_srvlist(d->srvlist)) {
      /* there is no servers for this domain, reply with "Server failure" */
	log_debug(2, "Replying to query with \"Server failure\"");
	if (!set_srvfail(msg, *len)) return -1;
	return 0;
    }

    if (d->roundrobin) {
      set_current(d, next_active(d));
    } else {
      /* find the first active server */
      d->current=NULL;
      d->current=next_active(d);
    }

    /* Send to a server until it "times out". */
    if (d->current) {
      time_t now = time(NULL);
      if ((d->current->send_time != 0) 
	  && (forward_timeout != 0)
	  && (reactivate_interval != 0)
	  && (now - d->current->send_time > forward_timeout)) {
	deactivate_current(d);
      }
    }

    if (d->current) {
	log_debug(3, "Forwarding the query to DNS server %s",
		  inet_ntoa(d->current->addr.sin_addr));
    } else {
      log_debug(3, "All servers deactivated. Replying with \"Server failure\"");
      if (!set_srvfail(msg, *len)) return -1;
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
  if ( (now - last_try < interval))
    return;
 
  last_try = now;
  do {
    if (!no_srvlist(d->srvlist))
      retry_srvlist(d, interval);
  } while ((d = d->next) != domain_list);  
}
/* Check if any server are timing out and should be deactivated */
static void deactivate_servers(int interval) {
  time_t now=time(NULL);
  static int last_try = 0;
  domnode_t *d = domain_list;
  srvnode_t *s;

  if (!last_try) last_try = now;
  if (now - last_try < interval)
    return;
  last_try = now;

  do {
    if ((s=d->srvlist)) 
      while ((s=s->next) != d->srvlist) {
        int current_disabled=0;
	if (s->inactive) continue;
	if (s->send_time
	    && (difftime(now, s->send_time) > forward_timeout)) {
	  s->inactive = now;
	  if (s == d->current) /* we need to update d->current */
	    current_disabled=1; 
	}
	if (current_disabled) deactivate_current(d);
      }
  } while ((d = d->next) != domain_list);  
}

void srv_stats(time_t interval) {
  srvnode_t *s;
  domnode_t *d=domain_list;
  time_t now = time(NULL);
  static time_t last=0;
  
  if (last + interval > now) {
    last = now;
    do {
      if ((s=d->srvlist)) 
	while ((s=s->next) != d->srvlist)
	  log_debug(1, "stats for %s: send count=%i",
		    inet_ntoa(s->addr.sin_addr), s->send_count);
    } while ((d=d->next) != domain_list);
  }
}


/* print statics about the query list and open sockets */
void query_stats(time_t interval) {
  time_t now = time(NULL);
/*  int count; */ /* UNUSED */
  static time_t last=0;
	if (interval == 0) return;
  if (last + interval < now) {
    last = now;
    log_msg(LOG_INFO, "Hits: %i, Misses: %i, Total: %i, Timeouts: %i", 
						cache_hits, cache_misses, cache_hits + cache_misses, 
						total_timeouts);
		if (stats_reset)
			cache_hits = cache_misses = total_timeouts = 0;
  }  
}



/*
 * run()
 *
 * Abstract: This function runs continuously, waiting for packets to arrive
 *           and processing them accordingly.
 */
void run()
{
  struct timespec    tout;
  fd_set             fdread;
  int                retn;
  sigset_t          orig_sigmask; 

  FD_ZERO(&fdmaster);
  FD_SET(isock,   &fdmaster);
#ifdef ENABLE_TCP
  FD_SET(tcpsock, &fdmaster);
  maxsock = (tcpsock > isock) ? tcpsock : isock;
#else
  maxsock = isock;
#endif

  init_sig_handler(&orig_sigmask);

  while(1) {
    query_t *q;
    tout.tv_sec  = select_timeout;
    tout.tv_nsec = 0;
    fdread = fdmaster;
    
    /* Wait for input or timeout */
    retn = pselect(maxsock+1, &fdread, 0, 0, &tout, &orig_sigmask);
    
    /* reactivate servers */
    if (reactivate_interval != 0) {
      reactivate_servers(reactivate_interval);
      /* check if any server should be timed out */
      if (forward_timeout != 0)
	deactivate_servers(1);
    }

    /* Handle errors */
    if (retn < 0) {
	    if (errno == EINTR) {
#ifndef EXCLUDE_MASTER
    /* Reload the master database if neccessary */
    master_reinit();
#endif
	    } else {
      log_msg(LOG_ERR, "select returned %s", strerror(errno));
	    }
      continue;
    }
    else if (retn != 0) {
      for (q = &qlist; q->next != &qlist; q = q->next) {
	if (FD_ISSET(q->next->sock, &fdread)) {
	  udp_handle_reply(q);
	}
      }

#ifdef ENABLE_TCP
      /* Check for incoming TCP requests */
      if (FD_ISSET(tcpsock, &fdread)) tcp_handle_request();
#endif
      /* Check for new DNS queries */
      if (FD_ISSET(isock, &fdread)) {
	q = udp_handle_request();
	if (q != NULL) {
	}
      }
    } else {
      /* idle */
    }
    
    /* ok, we are done with replies and queries, lets do some
	   maintenance work */
    
    /* Expire lookups from the cache */
    cache_expire();
    /* Remove old unanswered queries */
    query_timeout(20);
    
    /* create new query/socket for next incomming request */
    /* this did not make the program run any faster
    d=domain_list;
    do {
      if ((s=d->srvlist)) 
	while ((s=s->next) != d->srvlist)
	  if (s->newquery == NULL) 
	    s->newquery = query_get_new(d, s);
    } while ((d=d->next) != domain_list);
    */

    /* print som query statestics */
    query_stats(stats_interval);
    srv_stats(10);
  }
}
