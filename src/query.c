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
#include "lib.h"
#include "common.h"
#include "query.h"
#include "qid.h"


query_t qlist; /* the active query list */
static query_t *qlist_tail = &qlist;

int max_sockets = 30;

int upstream_sockets = 0; /* number of upstream sockets */

/* init the query list */
void query_init() {
  qlist_tail = (qlist.next = &qlist);
}


/* create a new query, and open a socket to the server */
query_t *query_create(domnode_t *d, srvnode_t *s) {
  query_t *q;

  /* should never be called with no server */
  assert(s != NULL);

  if (upstream_sockets >= max_sockets) {
    log_msg(LOG_WARNING, "Socket limit reached. Dropping new queries");
    return NULL;
  }
  
  if ((q=(query_t *) allocate(sizeof(q))) == NULL)
    return NULL;

  q->next = q;
  /* we specify both domain and server */
  /* the dummy queries will use server but no domain is attatched */
  q->domain = d;
  q->srv = s;

  /* open a new socket */
  if ((q->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    log_msg(LOG_ERR, "query_create: Couldn't open socket");
    free(q);
    return NULL;
  } else upstream_sockets++;
  
  return q;
}

query_t *query_destroy(query_t *q) {
  /* close the socket and return mem */
  close(q->sock);
  upstream_sockets--;
  free(q);
  return NULL;
}

/* Get a new query */
query_t *query_get_new(domnode_t *dom, srvnode_t *srv) {
  query_t *q;

  /* if there are no prepared queries waiting for us, lets create one */
  if ((q=srv->newquery) == NULL)
    q = query_create(dom, srv);
  srv->newquery = NULL;
  return q;
}


/* get qid, rewrite and add to list */
query_t *query_add(query_t *q, const struct sockaddr_in* client, char* msg, 
		   unsigned len) {
  query_t *p;

  unsigned short client_qid = *((unsigned short *)msg);
  time_t now = time(NULL);

  /* 
     look if the query are in the list 
     if it is, don't add it again. 
  */
  for (p=&qlist; p->next != &qlist; p = p->next) {
    if (p->next->client_qid == client_qid) {
      /* we found the qid in the list */
      *((unsigned short *)msg) = p->my_qid;
      p->client_time = now;
      log_debug("Query %i from client already in list. Count=%i", client_qid,
		p->client_count++);
      return q;
    }
  }
  q->client_qid = client_qid;
  memcpy(&(q->client), client, sizeof(struct sockaddr_in));
  q->client_time = now;
  q->client_count = 1;

  /* set new qid from random generator */
  *((unsigned short *)msg) = htons(q->my_qid = qid_get());

  /* add the query to the list */
  q->next = qlist_tail->next;
  qlist_tail->next = q;

  /* new query is new tail */
  qlist_tail = q;

  log_debug("Query client qid=%i, my qid=%i added", 
	    q->client_qid,
	    q->my_qid);
  return q;

}


#ifdef DEBUG
void query_dump_list(void) {
  query_t *p;

  for (p=&qlist; p->next != &qlist; p=p->next) {
    log_debug("myqid=%i, client_qid=%i", p->my_qid, p->client_qid);
  }

}

#endif
