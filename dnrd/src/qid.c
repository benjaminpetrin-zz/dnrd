#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "rand.h"
#include "common.h"
#include "qid.h"

#define QID_POOL_SIZE 65536

static unsigned short int qid_pool[QID_POOL_SIZE];
static int pool_ptr = QID_POOL_SIZE - 1;


static randctx isaac_ctx;

int myrand(int max) {
  return (int)  ((float)max * (unsigned int)rand(&isaac_ctx)/(0xffffffff+1.0));
}

void qid_init_pool(void) {
  int i;
  /*  FILE *f;
   */
  struct timeval tv;

  for (i=0; i<QID_POOL_SIZE; i++)
    qid_pool[i] = i;

  /* get random seed from time */
  for (i=0; i<RANDSIZ; i++) {
    gettimeofday(&tv,0);
    isaac_ctx.randrsl[i] = tv.tv_sec + tv.tv_usec;
  }
  /* init prng */
  randinit(&isaac_ctx, TRUE);
}

unsigned short int qid_get(void) {
  unsigned short int t;
  int i = myrand(pool_ptr) % QID_POOL_SIZE;
  /*
  if (pool_ptr == 0)
  log_debug("return_qid: qid pool is empty.");*/
  t = qid_pool[i];
  /* shrink the pool */
  qid_pool[i] = qid_pool[pool_ptr-- % QID_POOL_SIZE];
  return(t);
}

unsigned short int qid_return(unsigned short int qid) {
  /* 
 if ((pool_ptr+1) == QID_POOL_SIZE) 
 log_debug("return_qid: qid pool is already full."); */
  return (qid_pool[++pool_ptr % QID_POOL_SIZE] = qid);
}
