/*

    File: srvnode.h
    
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



#ifndef SRVNODE_H
#define SRVNODE_H

#include <netinet/in.h>

typedef struct _srvnode {
  /*  int                 sock;*/ /* the communication socket */
  struct sockaddr_in  addr;      /* IP address of server */
  time_t              inactive; /* is this server active? */
  unsigned int        send_count;
  int                 send_time;
  int                 tcp;
  struct _query   *newquery; /* new opened socket, prepared for a new query */
  struct _srvnode     *next; /* ptr to next server */
} srvnode_t;


srvnode_t *alloc_srvnode(void);
srvnode_t *init_srvlist(void);
srvnode_t *ins_srvnode (srvnode_t *list, srvnode_t *p);
srvnode_t *del_srvnode_after(srvnode_t *list);
srvnode_t *destroy_srvnode(srvnode_t *p);
srvnode_t *clear_srvlist(srvnode_t *head);
srvnode_t *destroy_srvlist(srvnode_t *head);
srvnode_t *add_srv(srvnode_t *head, const char *ipaddr);
srvnode_t *last_srvnode(srvnode_t *head);
int no_srvlist(srvnode_t *head);


#endif




