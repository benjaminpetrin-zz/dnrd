/*
 * 
 *
 *
 */


#ifndef SRVNODE_H
#define SRVNODE_H

#include <netinet/in.h>

typedef struct _srvnode {
  int                 sock; /* the communication socket */
  struct sockaddr_in     addr;      /* IP address of server */
  int                 active; /* is this server active? */
  int                 nexttry; /* next time to retry */
  struct _srvnode     *next; /* ptr to next server */

} srvnode_t;


srvnode_t *alloc_srvnode(void);
srvnode_t *init_srvlist(void);
srvnode_t *ins_srvnode (srvnode_t *list, srvnode_t *p);
srvnode_t *del_srvnode(srvnode_t *list);
void destroy_srvnode(srvnode_t *p);
srvnode_t *empty_srvlist(srvnode_t *head);
srvnode_t * destroy_srvlist(srvnode_t *head);
srvnode_t *add_srv(srvnode_t *head, char *ipaddr);






#endif




