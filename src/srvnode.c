/* 
 *
 */


#include <sys/types.h>
#include "srvnode.h"
#include "lib.h"

srvnode_t *alloc_srvnode() {
  srvnode_t *p = allocate(sizeof(srvnode_t));
  return p;
}


/* init the linked server list
 * returns ptr to the head/tail dummy node in an empty list
 */

srvnode_t *init_srvlist() {
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
void destroy_srvnode(srvnode_t *p) {
  /* close socket */
  if (p->sock) close(p->sock);
  free(p);
}

/* destroys the server list */
void destroy_srvlist(srvnode_t *head) {
  srvnode_t *p;
  while (p->next != head) {
    destroy_srvnode(del_srvnode(p));
  }
  free(head);
}


void printlist(srvnode_t *head) {
  srvnode_t *p = head->next;
  while (p != head) {
    printf("%i\n",p->sock);
    p = p->next;
  }
}

srvnode_t *add_srv(srvnode_t *head, int i) {
  srvnode_t *p = alloc_srvnode();
  p->sock = i;
  ins_srvnode(head, p);
  return p;
}

int main(int argc, char**argv) {
  int i;
  srvnode_t *list = init_srvlist();

  for (i=1; i<10; i++) {
    add_srv(list, i);
  }
  printlist(list);
  destroy_srvlist(list);
  return 0;
}
