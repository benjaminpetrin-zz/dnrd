/*

    File: lib.h
    
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

#ifndef	_LIB_INCLUDED
#define	_LIB_INCLUDED

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>


extern char *program;
extern int verbose;


void *allocate(size_t size);
void *reallocate(void *p, size_t size);

char *strnlwr(char *string, const int maxlen);
char *strnupr(char *string, const int maxlen);

char *skip_ws(char *string);
char *noctrln(char *buffer, const int maxlen);
char *get_word(char **from, char *to, int maxlen);
char *get_quoted(char **from, int delim, char *to, int max);
char *copy_string(char *y, char *x, int len);

unsigned int get_stringcode(char *string);

#ifndef HAVE_STRNLEN
size_t strnlen(const char *s, size_t maxlen);
#endif

#ifndef HAVE_USLEEP
int usleep(long usec);
#endif

#ifndef HAVE_PSELECT
int pselect(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    const struct timespec *timeout, const sigset_t *sigmask);
#endif

#endif

