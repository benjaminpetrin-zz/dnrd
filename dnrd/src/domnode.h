/*
 * Copyright (c) 2004 Natanael Copa
 * This is distributed under GPL
 *
 */


#ifndef DOMNODE_H
#define DOMNODE_H

#include "srvnode.h"

typedef struct _domnode {
  char            *domain;  /* the domain */
  srvnode_t       *srvlist; /* linked list of servers */
  struct _domnode *next;    /* ptr to next server */
} domnode_t;


domnode_t *alloc_domnode(void);
domnode_t *ins_domnode (domnode_t *list, domnode_t *p);
domnode_t *del_domnode(domnode_t *list);
domnode_t *destroy_domnode(domnode_t *p);
domnode_t *empty_domlist(domnode_t *head);
domnode_t *destroy_domlist(domnode_t *head);
domnode_t *add_domain(domnode_t *list, char *name);
domnode_t *search_domain(domnode_t *head, char *name);





#endif




