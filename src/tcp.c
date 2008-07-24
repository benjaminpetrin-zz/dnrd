/*
 * tcprequest.c - handle TCP request by transparent proxying.
 *
 * Copyright (C) 1999 Wolfgang Zekoll <wzk@quietsche-entchen.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef ENABLE_TCP
#ifdef ENABLE_PTHREADS
#include <pthread.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <errno.h>

#include "common.h"
#include "relay.h"
#include "cache.h"
#include "lib.h"


#ifdef ENABLE_PTHREADS
#define TCP_EXIT(x) pthread_exit((x))
#else
#define TCP_EXIT(x) _exit((x))
#endif

typedef struct tcp_handle_info {
    int	               connect;
    struct sockaddr_in client;
    unsigned           len;
} tcp_handle_t;

/*
 * alarm_handler() - Timeout handler for the connect() function in ip_open().
 *		It's purpose is just to interrupt the connect() system call.
 */
static void alarm_handler(int dummy)
{
    return;
}


/*
 * ip_open() - Open a tcp socket to the given DNS server
 *
 * In:      server - The address to which to open a connection
 *          port   - The tcp port to which to open a connection
 *
 * Returns: the opened socket on success, -1 on failure.
 */
static int ip_open(struct sockaddr_in server, unsigned int port)
{
    int	sock;

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) return (-1);

    server.sin_family = AF_INET;
    server.sin_port   = htons(port);

    signal(SIGALRM, alarm_handler);
    alarm(10);
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
	return (-1);
    }

    alarm(0);
    signal(SIGALRM, SIG_DFL);

    return (sock);
}	

/*
 * tcp_handler() - This function accepts an incoming connection on
 *		   tcpsock and handles the client request by transparent
 *                 proxying.  After the request is served the thread
 *                 terminates.
 */
static void *tcp_handler(tcp_handle_t *dummy)
{
    tcp_handle_t      *arg = (tcp_handle_t *)dummy;
    int	               rc, bytes;
    int                maxsock;
    //    int                server[MAX_SERV];
    char	       buffer[4096]; /* FIXME: do we want this on the stack?*/
    struct timeval     tov;
    fd_set	       connection, available;
    unsigned short     tcpsize;
    int	               connect = arg->connect;
    struct sockaddr_in client  = arg->client;
    domnode_t *d =domain_list;
    srvnode_t *s;

    /*    free(arg);*/

    do {
      if ((s=d->srvlist))
	while ((s=s->next) != d->srvlist) s->tcp = -1;
    } while ((d=d->next) != domain_list);
    /*
    for(i = 0; i < MAX_SERV; i++) {
	server[i] = -1;
    }
    */

    maxsock = connect + 1;
    FD_ZERO(&connection);
    FD_SET(connect, &connection);

    while (1) {
	int child_die = 0;

	memcpy(&available, &connection, sizeof(fd_set));
	tov.tv_sec  = 120;	/* Give the DNS 2min to answer. */
	tov.tv_usec = 0;

	rc = select(maxsock, &available, NULL, NULL, &tov);
	if (rc < 0) {
	    log_msg(LOG_ERR, "[%d] select() error: %m\n", getpid());
	    TCP_EXIT(0);
	}
	else if (rc == 0) {
	    log_msg(LOG_NOTICE, "[%d] connection timed out", getpid());
	    TCP_EXIT(0);
	}

	/* Forward replies from DNS server to client */
	/*  
	for(i = 0; i < serv_cnt; i++) {
	    if (server[i] == -1) continue;
	    if (FD_ISSET(server[i], &available)) {
		log_debug("[%d] Received tcp reply.  Forwarding...", getpid());
		if ((bytes = read(server[i], buffer, sizeof(buffer))) <= 0) {
		    child_die = 1;
		    break;
		}
		dump_dnspacket("reply", buffer + 2, bytes - 2);
		cache_dnspacket(buffer + 2, bytes - 2);
		if (write(connect, buffer, bytes) != bytes) {
		    child_die = 1;
		    break;
		}
	    }
	}
*/
	d=domain_list;
	do {
	  if ((s=d->srvlist)) 
	    while ((s=s->next) != d->srvlist) {
	      if (s->tcp == -1) continue;
	      if (FD_ISSET(s->tcp, &available)) {
		log_debug(1, "[%d] Received tcp reply.  Forwarding...", getpid());
		if ((bytes = read(s->tcp, buffer, sizeof(buffer))) <= 0) {
		  child_die = 1;
		  break;
		}
		dump_dnspacket("reply", buffer + 2, bytes - 2);
		cache_dnspacket(buffer + 2, bytes - 2, s);
		if (write(connect, buffer, bytes) != bytes) {
		  child_die = 1;
		  break;
		}
	      }
	    }
	  if (child_die) break;
	} while ((d=d->next) != domain_list);
	  
	if (child_die) break;

	/* Forward requests from client to DNS server */
	if (FD_ISSET(connect, &available)) {
	    int retn;
	    domnode_t *dptr;
	    

	    bytes = read(connect, &tcpsize, sizeof(tcpsize));
	    /* check for connection close */
	    if (bytes == 0) break;
	    log_debug(1, "[%d] Received tcp DNS query...", getpid());

	    if (bytes < 0) {
		log_debug(1, "[%d] tcp read error %s", getpid(), strerror(errno));
		break;
	    }
	    memcpy(buffer, &tcpsize, sizeof(tcpsize));
	    tcpsize = ntohs(tcpsize);
	    if (tcpsize > (sizeof(buffer) + 2)) {
		log_msg(LOG_WARNING,
			"[%d] Received tcp message is too big to process",
			getpid());
		break;
	    }
	    bytes = read(connect, buffer + 2, (size_t)tcpsize);
	    if (bytes == 0) {
		log_msg(LOG_ERR, "[%d] tcp DNS query is mangled", getpid());
		break;
	    }
	    if (bytes < 0) {
		log_debug(1, "[%d] tcp read error %s", getpid(), strerror(errno));
		break;
	    }
	    if ((retn = handle_query(&client, buffer + 2, &bytes, &dptr))<0) {
	      /* bogus query */
	      break;
	    }
	    /* If we can reply locally (master, cache, no servers), do so. */
	    else if (retn == 0) {
		unsigned short len = htons((unsigned short)bytes);
		memcpy(buffer, &len, sizeof(len));
		if (write(connect, buffer, bytes + 2) != (bytes + 2)) break;
	    }

	    /*
	     * Forward DNS request to the appropriate server.
	     * Open a socket if one doesn't already exist.
	     */
	    else if (retn == 1) {
	      s = dptr->current;
	      if (s->tcp == -1) {
		s->tcp = ip_open(s->addr, 53);
		if (s->tcp < 0) {
		  log_msg(LOG_ERR, "[%d] Can't connect to server",
			  getpid());
		  break;
		}
		if (s->tcp > (maxsock - 1)) {
		  maxsock = s->tcp + 1;
		}
		FD_SET(s->tcp, &connection);
	      }
	      if (write(s->tcp, buffer, bytes + 2) != (bytes + 2)) {
		break;
	      }
	    }
	}
	
    }
    /* The child process is done.  It can now die */
    log_debug(1, "[%d] Closing tcp connection", getpid());
    /*    for(i = 0; i < MAX_SERV; i++) {
	if (server[i] != -1) close(server[i]);
    }
    */
    d=domain_list;
    do {
      if ((s=d->srvlist))
	while ((s=s->next) != d->srvlist) 
	  if (s->tcp != -1) close(s->tcp);
    } while ((d=d->next) != domain_list);

    close(connect);
    log_debug(1, "[%d] Exiting child", getpid());
    TCP_EXIT(0);
    /* NOTREACHED */
    return (void *)0;
}

/*
 * handle_tcprequest()
 *
 * This function spawns a thread to actually handle the tcp request.
 * A thread is used instead of a process so that any DNS replies can
 * be placed in cache of the parent process.
 */


void tcp_handle_request()
{
    tcp_handle_t *arg;
#ifdef ENABLE_PTHREADS
    pthread_t t;
    pthread_attr_t  attr;
#endif
    pid_t pid;

    arg = (tcp_handle_t *)allocate(sizeof(tcp_handle_t));
    arg->len = sizeof(arg->client);

    arg->connect = accept(tcpsock, (struct sockaddr *) &(arg->client), 
			  &(arg->len));
    if (arg->connect < 0) {
	log_msg(LOG_ERR, "accept error for TCP connection");
	free(arg);
	return;
    }

#ifdef ENABLE_PTHREADS
    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    log_debug("creating a thread (parent=%d)", getpid());
    if (pthread_create(&t, &attr, (void *)&tcp_handler, (void *)arg)) {
	log_msg(LOG_ERR, "Couldn't spawn thread to handle tcp connection");
	free(arg);
    }
#else
    if ((pid=fork())==0) {
      /* we are the child */
      tcp_handler(arg);
    } else if (pid < 0) {
      log_msg(LOG_ERR, "fork for TCP connection failed");
    }
    close(arg->connect);
    free(arg);
#endif
    
}
#endif
