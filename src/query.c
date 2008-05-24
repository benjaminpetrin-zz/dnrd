/*
 * query.c
 *
 * This is a complete rewrite of Brad garcias query.c
 *
 * This file contains the data definitions, function definitions, and
 * variables used to implement our DNS query list.
 *
 * Assumptions: No multithreading.
 *
 * Copyright (C) Natanael Copa <ncopa@users.sourceforge.net>
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

#include <syslog.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lib.h"
#include "common.h"
#include "query.h"
#include "qid.h"


query_t qlist; /* the active query list */
static query_t *qlist_tail;
unsigned long total_queries=0;
unsigned long total_timeouts=0;

int upstream_sockets = 0; /* number of upstream sockets */

static int dropping = 0; /* dropping new packets */

/* init the query list */
void query_init() {
  qlist_tail = (qlist.next = &qlist);
}


/* create a new query, and open a socket to the server */
query_t *query_create(domnode_t *d, srvnode_t *s) {
  query_t *q;
#ifdef RANDOM_SRC
  struct sockaddr_in my_addr;
#endif

  /* should never be called with no server */
  assert(s != NULL);

  /* check if we have reached maximum of sockets */
  if (upstream_sockets >= max_sockets) {
    if (!dropping)
      log_msg(LOG_WARNING, "Socket limit reached. Dropping new queries");
    return NULL;
  }

  dropping=0;
  /* allocate */
  if ((q=(query_t *) allocate(sizeof(query_t))) == NULL)
    return NULL;

  /* return an emtpy circular list */
  q->next = (struct _query *)q;

  /* we specify both domain and server */
  /* the dummy queries will use server but no domain is attatched */
  q->domain = d;
  q->srv = s;

  /* set the default time to live value */
  q->ttl = forward_timeout;
  
  /* open a new socket */
  if ((q->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    log_msg(LOG_ERR, "query_create: Couldn't open socket");
    free(q);
    return NULL;
  } else upstream_sockets++;

  /* bind to random source port */
#ifdef RANDOM_SRC
  memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = INADDR_ANY;

  my_addr.sin_port = htons(myrand(65536-1026) + 1025);
  while ((bind(q->sock, (struct sockaddr *)&my_addr, 
      sizeof(struct sockaddr))) == -1) {

    if (errno != EADDRINUSE) {
      log_msg(LOG_WARNING, "bind: %s", strerror(errno));
      break;
    }

    /* try another port */
    my_addr.sin_port = htons(myrand(65535-1026) + 1025);
  }
#endif

  /* add the socket to the master FD set */
  FD_SET(q->sock, &fdmaster);
  if (q->sock > maxsock) maxsock = q->sock;

  /* get an unused QID */
  q->my_qid = qid_get();
  return q;
}

query_t *query_destroy(query_t *q) {
  /* close the socket and return mem */
  qid_return(q->my_qid);

  /* unset the socket */
  FD_CLR(q->sock, &fdmaster);
  close(q->sock);

  upstream_sockets--;
  total_queries++;
  free(q);
  return NULL;
}

/* Get a new query */
query_t *query_get_new(domnode_t *dom, srvnode_t *srv) {
  query_t *q;
  assert(srv != NULL);
  /* if there are no prepared queries waiting for us, lets create one */
  if ((q=srv->newquery) == NULL) {
    if ((q=query_create(dom, srv)) == NULL) return NULL;
  }
  srv->newquery = NULL;
  q->domain=dom;
  q->srv = srv;
  return q;
}


/* get qid, rewrite and add to list. Retruns the query before the added  */
query_t *query_add(domnode_t *dom, srvnode_t *srv, 
		   const struct sockaddr_in* client, char* msg, 
		   unsigned len) {

  query_t *q, *p, *oldtail;
  unsigned short client_qid = *((unsigned short *)msg);
  time_t now = time(NULL);

  /* 
     look if the query are in the list 
     if it is, don't add it again. 
  */
  for (p=&qlist; p->next != &qlist; p = p->next) {
    if ((p->next->client_qid == client_qid) &&
        ((p->next->client.sin_addr.s_addr == client->sin_addr.s_addr) &&
         (p->next->client.sin_port == client->sin_port))) {

      char client_ip[INET_ADDRSTRLEN];

      inet_ntop(AF_INET, &(client->sin_addr), client_ip, sizeof(*client));

      log_debug(2, "Query ID %hu from client %s:%hu already in list. "
        "Count=%hu", 
               ntohs(client_qid), client_ip, ntohs(client->sin_port),
        p->next->client_count++);

      dump_dnspacket("duplicate query", (unsigned char *)msg, len);

      /* we found the qid in the list */
      *((unsigned short *)msg) = p->next->my_qid;
      p->next->client_time = now;
      return p;
    }
  }

  if ((q=query_get_new(dom, srv))==NULL) {
    /* if we could not allocate any new query, return with NULL */
    return NULL;
  }

  q->client_qid = client_qid;
  memcpy(&(q->client), client, sizeof(struct sockaddr_in));
  q->client_time = now;
  q->client_count = 1;

  /* set new qid from random generator */
  *((unsigned short *)msg) = htons(q->my_qid);

  /* add the query to the list */
  q->next = qlist_tail->next;
  qlist_tail->next = q;

  /* new query is new tail */
  oldtail = qlist_tail;
  qlist_tail = q;
  return oldtail;

}

/* remove query after */
query_t *query_delete_next(query_t *q) {
  query_t *tmp = q->next;

  /* unlink tmp */
  q->next = q->next->next;
 
  /* if this was the last query in the list, we need to update the tail */
  if (qlist_tail == tmp) {
    qlist_tail = q;
  }

  /* destroy query */
  query_destroy(tmp);
  return q;
}


/* remove old unanswered queries */
void query_timeout(time_t age) {
  int count=0;
  time_t now = time(NULL);
  query_t *q;
  
  for (q=&qlist; q->next != &qlist; q = q->next) {
    if (q->next->client_time < (now - q->next->ttl)) {
      count++;
      query_delete_next(q);
    }
  }
  if (count) log_debug(1, "query_timeout: removed %d entries", count);
  total_timeouts += count;
}

int query_count(void) {
  int count=0;
  query_t *q;
  
  for (q=&qlist; q->next != &qlist; q = q->next) {
    count++;
  }
  return count;
}



void query_dump_list(void) {
  query_t *p;
  for (p=&qlist; p->next != &qlist; p=p->next) {
    log_debug(2, "srv=%s, myqid=%i, client_qid=%i", 
	      inet_ntoa(p->next->srv->addr.sin_addr), p->next->my_qid, 
	      p->next->client_qid);
  }
}
