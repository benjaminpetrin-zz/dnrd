/*

    File: srvnode.c
    
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




#include <sys/types.h>
#include "srvnode.h"
#include "lib.h"

srvnode_t *alloc_srvnode(void) {
  srvnode_t *p = allocate(sizeof(srvnode_t));
  /* actually we return a new emty list... */
  return p->next=p;
}


/* init the linked server list
 * returns ptr to the head/tail dummy node in an empty list
 */

srvnode_t *init_srvlist(void) {
  srvnode_t *p = alloc_srvnode();
  p->sock=0;
  p->next = p;
  return p;
}

/* insert srvnode in the list 
 * returns the new node
 */
srvnode_t *ins_srvnode (srvnode_t *list, srvnode_t *p) {
  p->next = list->next;
  list->next = p;
  return p;
} 

/* removes a node from the list.
 * returns the deleted node 
 */
srvnode_t *del_srvnode(srvnode_t *list) {
  srvnode_t *p = list->next;
  list->next = p->next;
  return p;
}

/* closes the server socket and frees the mem */
srvnode_t *destroy_srvnode(srvnode_t *p) {
  /* close socket */
  if (p->sock) close(p->sock);
  free(p);
  return NULL;
}

/* emties a linked server list. returns the head */
srvnode_t *empty_srvlist(srvnode_t *head) {
  srvnode_t *p;
  while (p->next != head) {
    destroy_srvnode(del_srvnode(p));
  }
}

/* destroys the server list, including the head */
srvnod_t *destroy_srvlist(srvnode_t *head) {
  empty_srvlist(head);
  free(head);
  return NULL;
}


/* add a server */
srvnode_t *add_srv(srvnode_t *head, char *ipaddr) {
  srvnode_t *p;
  struct in_inaddr inp;

  if (!inet_aton(ipaddr, &inp)) {
    return NULL;
  }
  p = alloc_srvnode();
  memcpy(&p->addr.sin_addr, &inp, sizeof(p->addr.sin_addr));
  p->active = 1;
  ins_srvnode(head, p);
  return p;
}
