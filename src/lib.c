/*

    File: lib.c
    
    Copyright (C) 1999 by Wolfgang Zekoll  <wzk@quietsche-entchen.de>

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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h> 
#include <signal.h>


#include "lib.h"
#include "common.h"

/*
#define	DEBUG(x)		
*/

void *allocate(size_t size)
{
    void	*p;
    if ((p = malloc(size)) == NULL) {
	log_msg(LOG_ALERT, "memory allocation error");
	exit (1);
    }
    memset(p, 0, size);
    return (p);
}

void *reallocate(void *p, size_t size)
{
    if ((p = realloc(p, size)) == NULL) {
	log_msg(LOG_ALERT, "memory allocation error");
	exit (1);
    }

    return (p);
}


char *strnlwr(char *string, const int maxlen)
{
    int i;

    if (!string) return NULL;
    for (i=0; (i<=maxlen) && (string[i] != 0); i++) {
	string[i] = tolower(string[i]);
    }

    return (string);
}	

char *strnupr(char *string, const int maxlen)
{
    int i;

    if (!string) return NULL;
    for (i=0; (i<=maxlen) && string[i]; i++) {
	string[i] = toupper(string[i]);
    }

    return (string);
}	


char *skip_ws(char *string)
{
    unsigned int c;

    while ((c = *string) == ' '  ||  c == '\t') {
	string++;
    }

    return (string);
}

char *noctrln(char *buffer, const int maxlen)
{
    int	len, i;
    unsigned char *p;

    if ((p = buffer) == NULL) {
	return (NULL);
    }

    len = strnlen(p, maxlen);
    for (i=len-1; i>=0; i--) {
	if (p[i] <= 32) {
	    p[i] = '\0';
	}
	else {
	    break;
	}
    }

    return (p);
}

char *get_word(char **from, char *to, int maxlen)
{
    unsigned int c;
    unsigned char *p;
    int	k;

    maxlen -= 2;
    while ((c = **from) != 0  &&  c <= 32) {
	*from += 1;
    }

    *(p = to) = k = 0;
    while ((c = **from) != 0) {
	if (c == ' '  ||  c == '\t'  ||  c < 32) {
	    break;
	}

	*from += 1;
	if (k < maxlen) {
	    p[k++] = c;
	}
    }

    p[k] = 0;
    return (to);
}

char *get_quoted(char **from, int delim, char *to, int max)
{
    unsigned int c;
    int	k;

    to[0] = k = 0;
    max -= 2;
    
    while ((c = **from) != 0) {
	*from += 1;
	if (c == delim) {
	    break;
	}

	if (k < max) {
	    to[k++] = c;
	}
    }

    to[k] = 0;
    return (to);
}

char *copy_string(char *y, char *x, int len)
{
    char *z = skip_ws(x);
    noctrln(z, len - (x-z));

    len -= 2;
    if (strlen(z) >= len) {
	z[len] = 0;
    }

    if (y != z) {
	strcpy(y, z);
    }
	    
    return (y);
}


unsigned int get_stringcode(char *string)
{
    unsigned int c, code;
    int	i;

    code = 0;
    for (i=0; (c = (unsigned char) string[i]) != 0; i++) {
	if (isupper(c)) {
	    c = tolower(c);
	}

	code = code + c;
    }

    code = (code & 0xFF) + (strlen(string) << 8);
    return (code);
}

/* in case we dont have strnlen */
#ifndef HAVE_STRNLEN
size_t strnlen(const char *s, size_t maxlen) {
  size_t len=0;
  while (*s++ && len<maxlen) len++;
  return (len);
}
#endif

#ifndef HAVE_USLEEP
/*

  usleep -- support routine for 4.2BSD system call emulations
  last edit:  29-Oct-1984     D A Gwyn
*/

extern int        select();

int usleep( usec )              /* returns 0 if ok, else -1 */
     long       usec;           /* delay in microseconds */
{
  static struct                 /* `timeval' */
  {
    long        tv_sec;         /* seconds */
    long        tv_usec;        /* microsecs */
  }   delay;                    /* _select() timeout */
  
  delay.tv_sec = usec / 1000000L;
  delay.tv_usec = usec % 1000000L;
  
  return select( 0, (long *)0, (long *)0, (long *)0, &delay );
}
#endif

#ifndef HAVE_PSELECT
int pselect(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    const struct timespec *timeout, const sigset_t *sigmask) {
  int ret;
  struct timeval tv, *ptv;
  sigset_t oldsigs;

  if (timeout != NULL) {
    tv.tv_sec = timeout->tv_sec;
    tv.tv_usec = timeout->tv_nsec / 1000000;
    ptv = &tv;
  } else
    ptv = NULL;

  if (sigmask != NULL)
    sigprocmask(SIG_SETMASK, sigmask, &oldsigs);
  ret = select(n, readfds, writefds, exceptfds, ptv);
  if (sigmask != NULL)
    sigprocmask(SIG_SETMASK, &oldsigs, NULL);
  return (ret);
}
#endif
