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


#include "domnode.h"
#include "lib.h"

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
  p->next = list->next;
  list->next = p;
  return p;
} 

/* removes a node from the list.
 * returns the deleted node 
 */
domnode_t *del_domnode(domnode_t *list) {
  domnode_t *p = list->next;
  list->next = p->next;
  return p;
}

/* destroys the server list and frees the mem */
domnode_t *destroy_domnode(domnode_t *p) {
  if (p->srvlist) destroy_srvlist(p->srvlist);
  free(p->domain);
  free(p);
  return NULL;
}

/* empties a linked server list. returns the head */
domnode_t *empty_domlist(domnode_t *head) {
  domnode_t *p;
  while (p->next != head) {
    destroy_domnode(del_domnode(p));
  }
}

/* destroys the domain list, including the head */
domnode_t *destroy_domlist(domnode_t *head) {
  empty_domlist(head);
  free(head);
  return NULL;
}


/* add a domain */
domnode_t *add_domain(domnode_t *list, const char *name, const int maxlen) {
  domnode_t *p;

  p = alloc_domnode();

  /* in case the domain is NULL */ 
  if (name) {
    /* allocate strnlen +1 incase there is no ending \0 */
    p->domain = allocate(strnlen(name, maxlen)+1);
    strncpy(p->domain, name, maxlen);
  } else {
    p->domain = NULL;
  }

  ins_domnode(list, p);
  return p;
}

/* search for a domain. returns the node if found or then head if not */
domnode_t *search_domnode(domnode_t *head, const char *name) {
  domnode_t *d=head;
  /* the list head is pointing to the default domain */
  if ((name == NULL) || (d == NULL)) return head;
  while ((d=d->next) != head) {
    if (strcmp(d->domain, name) == 0) return d;
  }
  return head;
}

/* search for the domnode that has the domain. Returns domnode if
   domain is a subdomain under domnode */

domnode_t *search_subdomnode(domnode_t *head, const char *name, 
			     const int maxlen) {
  domnode_t *d=head;
  int h,n;
  const char *p;
  /* the list head is pointing to the default domain */
  if ((name == NULL) || (d == NULL)) return head;
  while ((d=d->next) != head) {
    if ( (n = strnlen(name, maxlen)) > (h = strnlen(d->domain, maxlen))) {
      p = name + n - h;
    } else p=name;
    if (strncmp(d->domain, p, maxlen - (p - name)) == 0) return d;
  }
  return head;
}
