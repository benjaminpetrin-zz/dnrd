/*

    File: domnode.c
    
    Copyright (C) 2004 by Natanael Copa <n@tanael.org>

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <assert.h>

#include "lib.h"
#include "common.h"
#include "udp.h"
#include "domnode.h"

/* Allocate a domain node */
domnode_t *alloc_domnode(void) {
  domnode_t *p = allocate(sizeof(domnode_t));
  p->domain=NULL;
  p->srvlist=alloc_srvnode();
  /* actually we return a new emty list... */
  return p->next=p;
}


/* init the linked domain list
 * returns ptr to the head/tail dummy node in an empty list
 */

domnode_t *init_domainlist(void) {
  domnode_t *p = alloc_domnode();
  
  return p;
}


/* insert domnode in the list 
 * returns the new node
 */
domnode_t *ins_domnode (domnode_t *list, domnode_t *p) {
  assert((list != NULL) && (p != NULL));
  p->next = list->next;
  list->next = p;
  return p;
} 

/* removes a node from the list.
 * returns the deleted node 
 */
domnode_t *del_domnode(domnode_t *list) {
  domnode_t *p = list->next;
  assert((list != NULL));
  list->next = p->next;
  return p;
}

/* destroys the server list and frees the mem */
domnode_t *destroy_domnode(domnode_t *p) {
  if (p==NULL) {
    log_debug(1, "tried to destroy a NULL domnode"); 
    return NULL;
  }
  assert((p != NULL));
  if (p->srvlist) destroy_srvlist(p->srvlist);
  if (p->domain) free(p->domain);
  free(p);
  return NULL;
}

/* empties a linked server list. returns the head */
domnode_t *empty_domlist(domnode_t *head) {
  domnode_t *p=head;
  assert(p!=NULL);
  while (p->next != head) {
    destroy_domnode(del_domnode(p));
  }
  return (head);
}

/* destroys the domain list, including the head */
domnode_t *destroy_domlist(domnode_t *head) {
  assert(head!=NULL);
  empty_domlist(head);
  destroy_domnode(head);
  return NULL;
}


/* add a domain */
/* note: cname must be allocated! */
domnode_t *add_domain(domnode_t *list, const int load_balance, 
		      char *cname, const int maxlen) {
  domnode_t *p;
  assert(list != NULL);
  p = alloc_domnode();
  p->domain = cname;
  p->roundrobin = load_balance;
  ins_domnode(list, p);
  return p;
}

/* search for a domain. returns the node if found or NULL if not */
domnode_t *search_domnode(domnode_t *head, const char *name) {
  domnode_t *d=head;
  assert((head != NULL) && (name != NULL));
  /* the list head is pointing to the default domain */
  //  if ((name == NULL) || (d == NULL)) return head;
  while ((d=d->next) != head) {
    if (strcmp(d->domain, name) == 0) return d;
  }
  return NULL;
}

/* search for the domnode that has the domain. Returns domnode if
   domain is a subdomain under domnode */

domnode_t *search_subdomnode(domnode_t *head, const char *name, 
			     const int maxlen) {
  domnode_t *d=head, *curr=head;
  int h,n, maxfound=0;
  const char *p;
  assert( (head != NULL) && (name != NULL));
  /* the list head is pointing to the default domain */
  if ((name == NULL) || (d == NULL)) return head;
  while ((d=d->next) != head) {
    if ( (n = strnlen(name, maxlen)) > (h = strnlen(d->domain, maxlen))) {
      p = name + n - h;
    } else p=name;

    /* this works because the domain names are in cname format so
       hayes.org will nor appear as a subdomain under yes.org yes.org
       will be encoded as "\3yes\3org" while hayes.org will be encoded
       as "\5hayes\3org"
    */
    if ((strncmp(d->domain, p, maxlen - (p - name)) == 0) && (h > maxfound)) {
      maxfound = h; /* max length found */
      curr = d;
    }
  }
  return curr;
}

/* wrapper for setting current server */
srvnode_t *set_current(domnode_t *d, srvnode_t *s) {
  assert(d!=NULL);
  //  if (d == NULL) return NULL;
  if (s) {
    if (d->roundrobin) {
      log_debug(3, "Setting server %s for domain %s",
		inet_ntoa(s->addr.sin_addr), cname2asc(d->domain));
    } else {
      log_msg(LOG_NOTICE, "Setting server %s for domain %s",
		inet_ntoa(s->addr.sin_addr), cname2asc(d->domain));
    }
  } else 
    log_msg(LOG_WARNING, "No active servers for domain %s", 
	    cname2asc(d->domain));
  return (d->current = s);
}


/* next active server
   Returns: NULL - if there are no active servers in list
            the next active server otherwise
*/
srvnode_t *next_active(domnode_t *d) {
  srvnode_t *s, *start;
  assert(d!=NULL);
  if (d->current) {
    start=d->current;
  } else { /* previously deactivated everything, lets check from start */
    start=d->srvlist;
  }
  for (s=start->next; s->inactive && s != start; s = s->next);
  if (s->inactive) s=NULL;
  return (s);
}



/* deactivate current server
   Returns: next active server, NULL if there are none 
*/
srvnode_t *deactivate_current(domnode_t *d) {
  assert(d!=NULL);
  if (d->current) {
    d->current->inactive = time(NULL);
    log_msg(LOG_NOTICE, "Deactivating DNS server %s",
	      inet_ntoa(d->current->addr.sin_addr));
  }
  return set_current(d, next_active(d));
}


/* reactivate all dns servers */
void reactivate_srvlist(domnode_t *d) {
  srvnode_t *s;
  assert(d!=NULL); /* should never be called with NULL */ 
  s = d->srvlist;
  while ((s = s->next) && (s != d->srvlist)) {
    s->inactive = 0;
    s->send_time = 0;
  }
}

/* reactivate servers that have been inactive for delay seconds */
void retry_srvlist(domnode_t *d, const int delay) {
  time_t now = time(NULL);
  srvnode_t *s;
  assert(d!=NULL); /* should never happen */
  s = d->srvlist;
  while ((s = s->next) && (s != d->srvlist))
    if (s->inactive && (now - s->inactive) >= delay ) {
      s->inactive=now;
      udp_send_dummy(s);
    }
}

