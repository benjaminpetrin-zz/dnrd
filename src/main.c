/*
 * main.c - contains the main() function
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <dirent.h>
#include <limits.h>

#include "relay.h"
#include "cache.h"
#include "common.h"
#include "args.h"
#include "master.h"
#include "domnode.h"
#include "lib.h"
#include "qid.h"
#include "query.h"
#include "dns.h"

static int is_writeable (const struct stat* st);
static int user_groups_contain (gid_t file_gid);



#ifndef __CYGWIN__
static void init_dnrd_uid(void) {
	char *ep;

	/* find the uid */
	daemonuid = (uid_t)strtoul(dnrd_user, &ep, 10);
	if (dnrd_user[0] == '\0') /* should never happen */
		log_err_exit(-1, "Please specify a valid user with -u");

	if (*ep) { /* dnrd_user is not numeric */
		struct passwd *pwent;
		if ((pwent = getpwnam(dnrd_user))) {
			if ((daemonuid = pwent->pw_uid) == 0)
				log_err_exit(-1, "Please specify a non-root user (uid != 0) with -u");
			if ((daemongid = pwent->pw_gid) == 0)
				log_err_exit(-1, "Please specify a non-root user group (gid != 0)");
		} else {
			perror("getpwnam");
			log_err_exit(-1, "Could not become \"%s\" user. Please create the user "
									 "account or specify a valid user with  the -u option.", 
									 dnrd_user);
		}
	}

}
#endif /* __CYGWIN__ */



/***************************************************************************/
static void dnrd_root_sanity_check(void) {
	DIR               *dirp;
	int                rslt;
	struct dirent     *direntry;
	struct stat        st;
	
	if (chdir(dnrd_root))
		log_err_exit(-1, "Could not chdir to %s, %s", dnrd_root, strerror(errno));
	
	dirp = opendir(dnrd_root);
	if (!dirp) 
		log_err_exit(-1, "The directory %s must be created before "
								 "dnrd will run", dnrd_root);
	
	rslt = stat(dnrd_root, &st);
#ifdef __CYGWIN__
	if (is_writeable (&st))
		log_err_exit(-1, "The %s directory must not be writeable", dnrd_root);
#else
	if (st.st_uid != 0)
		log_err_exit(-1, "The %s directory must be owned by root", dnrd_root);
	if ((st.st_mode & (S_IWGRP | S_IWOTH)) != 0)
		log_err_exit(-1, "The %s directory should only be user writable",
								 dnrd_root);
#endif
	
	while ((direntry = readdir(dirp)) != NULL) {
		if (!strcmp(direntry->d_name, ".") ||
				!strcmp(direntry->d_name, "..")) {
	    continue;
		}
		
		rslt = stat(direntry->d_name, &st);
		
		if (rslt) continue;
		if (S_ISDIR(st.st_mode)) 
	    log_err_exit(-1, "The %s directory must not contain subdirectories", 
									 dnrd_root);
		if ((st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH|S_IWGRP|S_IWOTH)) != 0)
	    log_err_exit(-1, "A file in %s has either execute "
									 "permissions or non-user write permission.  Please do a "
									 "\"chmod a-x,go-w\" on all files in this directory",
									 dnrd_root);
#ifdef __CYGWIN__
		if (is_writeable (&st))
	    log_err_exit(-1, "No files in %s may be writeable", dnrd_root);
#else
		if (st.st_uid != 0) 
	    log_err_exit(-1, "All files in %s must be owned by root", dnrd_root);
#endif
	}
	closedir(dirp);
}



/***************************************************************************/
void init_socket(void) {
    struct servent    *servent;   /* Let's be good and find the port numbers
				     the right way */

    /*
     * Pretend we don't know that we want port 53
     */
    servent = getservbyname("domain", "udp");
    if (servent != getservbyname("domain", "tcp"))
			log_err_exit(-1, "domain ports for udp & tcp differ. "
									 "Check /etc/services");
    recv_addr.sin_port = servent ? servent->s_port : htons(53);

    /*
     * Setup our DNS query reception socket.
     */
    if ((isock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			log_err_exit(-1, "isock: Couldn't open socket");
		}
    else {
			int opt = 1;
			setsockopt(isock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }

    if (bind(isock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0)
			log_err_exit(-1, "isock: Couldn't bind local address");

    /*
     * Setup our DNS tcp proxy socket.
     */
#ifdef ENABLE_TCP
    if ((tcpsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			log_err_exit(-1, "tcpsock: Couldn't open socket");
    }
    else {
	int opt = 1;
	setsockopt(tcpsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
    if (bind(tcpsock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0)
			log_err_exit(-1, "tcpsock: Couldn't bind local address");
    if (listen(tcpsock, 5) != 0)
			log_err_exit(-1, "tcpsock: Can't listen");
#endif
}


/***************************************************************************
 * main() - startup the program.
 *
 * In:      argc - number of command-line arguments.
 *          argv - string array containing command-line arguments.
 *
 * Returns: 0 on exit, -1 on error.
 *
 * Abstract: We set up the signal handler, parse arguments,
 *           turn into a daemon, write our pid to /var/run/dnrd.pid,
 *           setup our sockets, and then parse packets until we die.
 ****************************************************************************/
int main(int argc, char *argv[])
{
  /*    int                i;*/
#ifdef ENABLE_PIDFILE
	FILE              *filep;
#endif
	domnode_t *p;
	srvnode_t *s;
	char *tmpstr;

	/*
	 * Initialization in common.h of recv_addr is broken, causing at
	 * least the '-a' switch not to work.  Instead of assuming
	 * positions of fields in the struct across platforms I thought it
	 * safer to do a standard initialization in main().
	 */
	memset(&recv_addr, 0, sizeof(recv_addr));
	recv_addr.sin_family = AF_INET;
	
	openlog(progname, LOG_PID, LOG_DAEMON);
	
	/* create the domain list */
	domain_list = alloc_domnode();
	
	/* get the dnrd_root from environment */
	if ((tmpstr = getenv("DNRD_ROOT")))
		strncpy(dnrd_root, tmpstr, sizeof(dnrd_root));
	
	/*
	 * Parse the command line.
     */
	parse_args(argc, argv);
	
	/* we change to the dnrd-root dir */
	chdir(dnrd_root);
	
#ifdef ENABLE_PIDFILE
	/* Kill any currently running copies of the program. */
	kill_current();
	/*
	 * Write our pid to the appropriate file.
	 * Just open the file here.  We'll write to it after we fork.
	 */
	if ( !(filep = fopen(pid_file, "w")) ) 
		log_err_exit(-1, "can't write to %s.  "
								 "Check that dnrd was started by root.", pid_file);
#endif
	
	/*
	 * Setup the thread synchronization semaphore
	 */
	if (sem_init(&dnrd_sem, 0, 1) == -1)
		log_err_exit(-1, "Couldn't initialize semaphore");

	init_socket();
	
	/* Initialise our cache */
	cache_init();
	
	/* init the qid pool */
	qid_init_pool();
	
#ifndef EXCLUDE_MASTER
	/* Initialise out master DNS */
	master_init();
#endif
	
	/* init query list */
	query_init();

	/* init dns validation table */
	init_dns();

#ifndef __CYGWIN__	
	/* we need to find the uid and gid from /etc/passwd before we chroot. */
	init_dnrd_uid();
#endif /* __CYGWIN__ */
	
	/*
	 * Change our root and current working directories to
	 * /usr/local/etc/dnrd.  Also, so some sanity checking on that
	 * directory first.
	 */
	dnrd_root_sanity_check();
	if (chroot(dnrd_root)) {
		log_err_exit(-1, "couldn't chroot to %s, %s",	dnrd_root, 
								 strerror(errno));
	} else log_debug(1, "chrooting to %s", dnrd_root);
	
	/*
	 * Change uid/gid to something other than root.
	 */
	
	/* drop supplementary groups */
	if (setgroups(0, NULL) < 0)
		log_err_exit(-1, "can't drop supplementary groups");
	
#ifndef __CYGWIN__ /** { **/
	/*
	 * Switch uid/gid to something safer than root.
	 */
	log_debug(1, "setting uid to %i", daemonuid);
	if (setuid(daemonuid) < 0) 
		log_err_exit(-1, "Could not switch to uid %i", daemonuid);  
#endif /** } __CYGWIN__ **/
	
	/*
	 * Setup our DNS query forwarding socket.
	 */
	p=domain_list;
	do {
		s=p->srvlist;
		while ((s=s->next) != p->srvlist) {
			s->addr.sin_family = AF_INET;
			s->addr.sin_port   = htons(53);
		}
		/* set the first server as current */
		p->current = p->srvlist->next;
	} while ((p=p->next) != domain_list);
	
#ifndef __CYGWIN__ /** { **/
	/*
	 * Now it's time to become a daemon.
	 */
	if (!foreground) {
		pid_t pid = fork();
		if (pid < 0) log_err_exit(-1, "%s: Couldn't fork\n", progname);
		if (pid != 0) exit(0);
		setsid();
		chdir("/");
		umask(077);
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
	}
#endif /** } __CYGWIN__ */

#ifdef ENABLE_PIDFILE
	/*
	 * Write our pid to the appropriate file.
	 * Now we actually write to it and close it.
	 */
	fprintf(filep, "%i\n", (int)getpid());
	fclose(filep);
#endif
	
	/*
	 * Run forever.
	 */
	run();
	exit(0); /* to make compiler happy */
}

/***************************************************************************/
static int is_writeable (const struct stat* st)
{
    if (st->st_uid == getuid ())
        return 1;

    if ((user_groups_contain (st->st_gid)) &&
        ((st->st_mode & S_IWGRP) != 0))
	return 1;

    if ((st->st_mode & S_IWOTH) != 0)
	return 1;

    return 0;
}

/***************************************************************************/
static int user_groups_contain (gid_t file_gid)
{
    int igroup;
    gid_t user_gids[NGROUPS_MAX];
    int ngroups = getgroups (NGROUPS_MAX, &user_gids[0]);

    if (ngroups < 0)
			log_err_exit(-1, "Couldn't get user's group list");

    for (igroup = 0 ; igroup < ngroups ; ++igroup)
    {
    	if (user_gids[igroup] == file_gid)
	    return 1;
    }

    return 0;
}
