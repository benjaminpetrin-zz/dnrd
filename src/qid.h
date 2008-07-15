/* qid.h
 *
 * Copyright (c) 2004 Natanael Copa
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 *
 */

#ifndef qid_h
#define qid_h

unsigned short int qid_get(void);
unsigned short int qid_return(unsigned short int qid);
void qid_init_pool(void);
int myrand(int max);

#endif
