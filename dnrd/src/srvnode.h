/*
 * 
 *
 *
 */


#ifndef SRVNODE_H
#define SRVNODE_H

typedef struct _srvnode {
  int                 sock; /* the communication socket */
  int                 active; /* is this server active? */
  int                 nexttry; /* next time to retry */
  struct _srvnode     *next; /* ptr to next server */

} srvnode_t;


/* srvnode_t *alloc_srvnode();
 */

#endif




