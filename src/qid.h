/* qid.h

   Copyright (c) Natanael Copa
   This is distributed under GPL2
*/

#ifndef qid_h
#define qid_h

unsigned short int qid_get(void);
unsigned short int qid_return(unsigned short int qid);
void qid_init_pool(void);
int myrand(int max);

#endif
