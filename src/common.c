/*
 * common.c - includes global variables and functions.
 *
 * Copyright (C) 1998 Brad M. Garcia <garsh@home.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include "common.h"
#include "lib.h"
#include "dns.h"

#ifdef DEBUG
#define OPT_DEBUG 1
#else
#define OPT_DEBUG -1
#endif /* DEBUG */


/*
 * These are all the global variables.
 */
int                 opt_debug = OPT_DEBUG;
int                 opt_serv = 0;
const char*         progname = 0;

#ifdef ENABLE_PIDFILE
#if defined(__sun__)
const char*         pid_file = "/var/tmp/dnrd.pid";
#else
const char*         pid_file = "/var/run/dnrd.pid";
#endif
#endif

int                 isock = -1;
#ifdef ENABLE_TCP
int                 tcpsock = -1;
#endif
int                 select_timeout = SELECT_TIMEOUT;
int                 forward_timeout = FORWARD_TIMEOUT;
//int                 load_balance = 0;
#ifndef __CYGWIN__
uid_t               daemonuid = 0;
gid_t               daemongid = 0;
char                dnrd_user[256] = "dnrd";
char                dnrd_group[256] = "dnrd";
#endif
const char*         version = PACKAGE_VERSION;
int                 foreground = 0; /* 1 if attached to a terminal */
sem_t               dnrd_sem;  /* Used for all thread synchronization */

int                 reactivate_interval = REACTIVATE_INTERVAL;
int                 stats_interval = 0;
int                 stats_reset = 1;

/* The path where we chroot. All config files are relative this path */
char                dnrd_root[512] = DNRD_ROOT;

char                config_file[512] = DNRD_ROOT "/" CONFIG_FILE;

domnode_t           *domain_list;
/* turn this on to skip cache hits from responses of inactive dns servers */
int                 ignore_inactive_cache_hits = 0; 

/* highest socket number */
int                 maxsock;

/* maximum number of open sockets. If we have this amount of
   concurrent queries, we start dropping new ones */
int max_sockets = 200;

/* the fd set. query modifies this so we make it global */
fd_set              fdmaster;



/*
 * This is the address we listen on.  It gets initialized to INADDR_ANY,
 * which means we listen on all local addresses.  Both the address and
 * the port can be changed through command-line options.
 */
/*
#if defined(__sun__)
struct sockaddr_in recv_addr = { AF_INET, 53, { { {0, 0, 0, 0} } } };
#else
struct sockaddr_in recv_addr = { AF_INET, 53, { INADDR_ANY } };
#endif
*/

/* init recv_addr in main.c instead of here */ 
struct sockaddr_in recv_addr;

#ifdef ENABLE_PIDFILE
/* check if a pid is running 
 * from the unix faq
 * http://www.erlenstar.demon.co.uk/unix/faq_2.html#SEC18
 */

int isrunning(int pid) {
  if (kill(pid, 0) ) {
    if (errno==EPERM) { 
      return 1;
    } else return 0;
  } else {
    return 1;
  }
}

/* wait_for_exit()
 *
 * In: pid     - the process id to wait for
 *     timeout - maximum time to wait in 1/100 secs
 *
 * Returns: 1 if it died in before timeout
 *
 * Abstract: Check if a process is running and wait til it does not
 */
int wait_for_exit(int pid, int timeout) {
  while (timeout--) {
    if (! isrunning(pid)) return 1;
    usleep(10000);
  }
  /* ouch... we timed out */
  return 0;
}

/*
 * kill_current()
 *
 * Returns: 1 if a currently running dnrd was found & killed, 0 otherwise.
 *
 * Abstract: This function sees if pid_file already exists and, if it does,
 *           will kill the current dnrd process and remove the file.
 */
int kill_current()
{
    int         pid;
    int         retn;
    struct stat finfo;
    FILE*       filep;

    if (stat(pid_file, &finfo) != 0) return 0;

    filep = fopen(pid_file, "r");
    if (!filep) {
	log_msg(LOG_ERR, "%s: Can't open %s\n", progname, pid_file);
	exit(-1);
    }
    if ((retn = (fscanf(filep, "%i%*s", &pid) == 1))) {
        kill(pid, SIGTERM);
	/* dnrd gets 4 seconds to die or we give up */
	if (!wait_for_exit(pid, 400)) {
	  log_msg(LOG_ERR, "The dnrd process didn't die within 4 seconds");
	}
    }
    fclose(filep);
    unlink(pid_file);
    return retn;
}
#endif /* ENABLE_PIDFILE*/

const char *get_typestr(int type) {
	static const char *EMERG = "EMERG: "; 
	static const char *ALERT = "ALERT: "; 
	static const char *CRIT = "CRIT:  "; 
	static const char *ERR = "ERROR: "; 
	static const char *WARNING = "Warning: "; 
	static const char *NOTICE = "Notice: "; 
	static const char *INFO = "Info:  "; 
	static const char *DEBUG = "Debug: "; 
	static const char *EMPTY = "";

	switch (type) {
	  case LOG_EMERG:   return EMERG;
	  case LOG_ALERT:   return ALERT;
	  case LOG_CRIT:    return CRIT;
	  case LOG_ERR:     return ERR;
	  case LOG_WARNING: return WARNING;
	  case LOG_NOTICE:  return NOTICE;
	  case LOG_INFO:    return INFO;
	  case LOG_DEBUG:   return DEBUG;
	default:          return EMPTY;
	}
}


/*
 * log_msg()
 *
 * In:      type - a syslog priority
 *          fmt  - a formatting string, ala printf.
 *          ...  - other printf-style arguments.
 *
 * Sends a message to stdout or stderr if attached to a terminal, otherwise
 * it sends a message to syslog.
 */
void log_msg(int type, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    if (foreground) {
			fprintf(stderr, get_typestr(type));
			vfprintf(stderr, fmt, ap);
			if (fmt[strlen(fmt) - 1] != '\n') fprintf(stderr, "\n");
    }
    else {
			vsyslog(type, fmt, ap);
    }
    va_end(ap);
}

/*
 * log_debug()
 *
 * In:      fmt - a formatting string, ala printf.
 *          ... - other printf-style arguments.
 *
 * Abstract: If debugging is turned on, this will send the message
 *           to syslog with LOG_DEBUG priority.
 */
void log_debug(int level, const char *fmt, ...)
{
    va_list ap;
    
    if (opt_debug < level) return;

    va_start(ap, fmt);
    if (foreground) {
	fprintf(stderr, "Debug: ");
	vfprintf(stderr, fmt, ap);
	if (fmt[strlen(fmt) - 1] != '\n') fprintf(stderr, "\n");
    }
    else {
	vsyslog(LOG_DEBUG, fmt, ap);
    }
    va_end(ap);
}

/*
 * cleanexit()
 *
 * In:      status - the exit code.
 *
 * Abstract: This closes our sockets, removes /var/run/dnrd.pid,
 *           and then calls exit.
 */
void cleanexit(int status)
{
  /*    int i;*/

    /* Only let one process run this code) */
    sem_wait(&dnrd_sem);

    log_debug(1, "Shutting down...\n");
    if (isock >= 0) close(isock);
#ifdef ENABLE_TCP
    if (tcpsock >= 0) close(tcpsock);
#endif
    /*
    for (i = 0; i < serv_cnt; i++) {
	close(dns_srv[i].sock);
    }
    */
    destroy_domlist(domain_list);
    exit(status);
}


/*
 * log_err_exit()
 *
 * In:      exitcode - the exitcode returned
 *          fmt  - a formatting string, ala printf.
 *          ...  - other printf-style arguments.
 *
 * Sends a message to log_msg, LOG_ERR and exit clean
 */
void log_err_exit(int exitcode, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (foreground) {
			fprintf(stderr, get_typestr(LOG_ERR));
			vfprintf(stderr, fmt, ap);
			if (fmt[strlen(fmt) - 1] != '\n') fprintf(stderr, "\n");
    }
    else {
			vsyslog(LOG_ERR, fmt, ap);
    }
    va_end(ap);
		cleanexit(exitcode);
}



/*
 * make_cname()
 *
 * In:       text - human readable domain name string
 *
 * Returns:  Pointer to the allocated, filled in character string on success,
 *           NULL on failure.
 *
 * Abstract: converts the human-readable domain name to the DNS CNAME
 *           form, where each node has a length byte followed by the
 *           text characters, and ends in a null byte.  The space for
 *           this new representation is allocated by this function.
 */
char* make_cname(const char *text, const int maxlen)
{
  /* this kind of code can easily contain buffer overflow. 
     I have checked it and double checked it so I believe it does not.
     Natanael */
    const char *tptr = text;
    const char *end = text;
    char *cname = (char*)allocate(strnlen(text, maxlen) + 2);
    char *cptr = cname;

    while (*end != 0) {
	size_t diff;
	end = strchr(tptr, '.');
	if (end == NULL) end = text + strnlen(text, maxlen);
	if (end <= tptr) {
	    free(cname);
	    return NULL;
	}
	diff = end - tptr;
	*cptr++ = diff;
	memcpy(cptr, tptr, diff);
	cptr += diff;
	tptr = end + 1;
    }
    *cptr = 0;
    assert((unsigned)(cptr - cname) == strnlen(text, maxlen) + 1);
    return cname;
}

/* convert cname to ascii and return a static buffer */
/* this func must *never* be called with an incomming DNS
   packet as input. Never. */
char *cname2asc(const char *cname) {
  static char buf[256];
  /* Note: we don't really check the size of the incomming cname. but
     according to RFC 1035 a name must not be bigger than 255 octets.
   */
  if (cname) 
    snprintf_cname((char *)cname, strlen(cname), 0, buf, sizeof(buf));
  else
    strncpy(buf, "(default)", sizeof(buf));
  return buf;
}
