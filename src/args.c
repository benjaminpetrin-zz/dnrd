/*
 * args.c - data and functions dealing with command-line argument processing.
 *
 * Copyright (C) 1998 Brad M. Garcia <garsh@home.com>
 * Copyright (C) 2008 Natanael Copa <natanael.copa@gmail.com>
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(__GNU_LIBRARY__)
#   include <getopt.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pwd.h>

#include "args.h"
#include "common.h"
#include "lib.h"
#include "cache.h"


/*
 * Definitions for both long and short forms of our options.
 * See man page for getopt for more details.
 */
#if defined(__GNU_LIBRARY__)
static struct option long_options[] =
{
    {"address",      1, 0, 'a'},
    {"load-balance", 0, 0, 'b'},
#ifndef EXCLUDE_MASTER
    {"blacklist",    1, 0, 'B'},
#endif
    {"cache",        1, 0, 'c'},
    {"debug",        1, 0, 'd'},
    {"foreground",   0, 0, 'f'},
    {"help",         0, 0, 'h'},
    {"ignore",       0, 0, 'i'},
#ifdef ENABLE_PIDFILE
    {"kill",         0, 0, 'k'},
#endif
    {"max-sock",     1, 0, 'M'},
#ifndef EXCLUDE_MASTER
    {"master",       1, 0, 'm'},
#endif
    {"retry",        1, 0, 'r'},
    {"server",       1, 0, 's'},
		{"stats",        1, 0, 'S'},
    {"timeout",      1, 0, 't'},
#ifndef __CYGWIN__
    {"uid",          1, 0, 'u'},
		/*
		{"gid",          1, 0, 'g'},
		*/
#endif
    {"version",      0, 0, 'v'},
    {"dnrd-root",    1, 0, 'R'},
    {0, 0, 0, 0}
};
#endif /* __GNU_LIBRARY__ */

#ifndef EXCLUDE_MASTER
#define MASTERPARM "B:m:"
#else
#define MASTERPARM
#endif

#ifdef ENABLE_PIDFILE
#define PIDPARM "k"
#else
#define PIDPARM 
#endif

#ifndef __CYGWIN__
#define UIDPARM "u:" /* "g:" */
#else
#define UIDPARM
#endif

#define file_exists(f) (access(f, R_OK) == 0)

const char short_options[] = 
    "a:bc:d:fhi" PIDPARM MASTERPARM "M:r:R:s:t:" UIDPARM "v";

/*
 * give_help()
 *
 * Abstract: Prints out the version number and a usage statement.
 */
static void give_help()
{
    printf("dnrd version %s\n", version);
    printf("\nusage: %s [options]\n", progname);
    printf("  Valid options are\n");
    printf(
#ifdef __GNU_LIBRARY__
"    -a, --address=LOCALADDRESS\n"
"                            Only bind to the port on the given address,\n"
"                            rather than all local addresses.\n"
"    -b, --load-balance      Round-Robin load balance forwarding servers.\n"
#ifndef EXCLUDE_MASTER
"    -B, --blacklist=FILE    Blacklist all hosts in FILE. Path to FILE is\n"
"                            relative $DNRD_ROOT (--dnrd-root). Default is\n"
"                            \"blacklist\"\n"
#endif
"    -c, --cache=off|[LOW:]HIGH\n"
"                            Turn off cache or tune the low/high water marks\n"
"    -d, --debug=LEVEL       Set the debugging level. Level 0 (default)\n"
"                            means no debugging at all.\n"
"    -f, --foreground        Run program in foreground rather than fork.\n"
/*
#ifndef __CYGWIN__
"    -g, --gid=GID           Group name numeric gid to switch to.\n"
#endif
*/
"    -h, --help              Print this message, then exit.\n"
"    -i, --ignore            Ignore cache for disabled servers.\n"
#ifdef ENABLE_PIDFILE
"    -k, --kill              Kill a running daemon.\n"
#endif
#ifndef EXCLUDE_MASTER
"    -m, --master=FILE|off   Use FILE as master file or turn it off. Path to\n"
"                            FILE is relative $DNRD_ROOT (--dnrd-root).\n"
#endif
"    -M, --max-sock=N        Set maximum number of open sockets to N.\n"
"    -r, --retry=N           Set retry interval to N seconds.\n"
"    -s, --server=IPADDR[:domain]\n"
"                            Set the DNS server.  You can specify an\n"
"                            optional domain name, in which case a DNS\n"
"                            request will only be sent to that server for\n"
"                            names in that domain.\n"
"                            (Can be used more than once for multiple or\n"
"                            backup servers).\n"
"    -S, --stats=N[+]        Send cache/query stats to syslog (LOG_INFO)\n"
"                            every N seconds. Stats will not be resetted if\n"
"                            the '+' is added\n"
"    -t, --timeout=N         Set forward DNS server timeout to N.\n"
#ifndef __CYGWIN__
"    -u, --uid=UID           Username or numeric id to switch to.\n"
#endif
"    -R, --dnrd-root=DIR     The dnrd root directory. dnrd will chroot to\n"
"                            this dir.\n"
"    -v, --version           Print out the version number and exit.\n"

#else /* __GNU_LIBRARY__ */

"    -a IPADDR Only bind to the port on the given address, rather than all\n"
"              local addresses\n"
"    -b        Round-Robin load balance forwarding servers\n"
#ifndef EXCLUDE_MASTER
"    -B        Blacklist all hosts in FILE. FILE is relative\n"
"              $DNRD_ROOT (--dnrd-root). Default is \"blacklist\"\n"
#endif
"    -c off|[LOW:]HIGH\n"
"              Turn off caching or tune the low/high water marks"
"    -d LEVEL  Set the debugging level. Level 0 (default) means no debugging\n"
"              at all.\n"
"    -f        Run program in foreground rather than fork.\n"
/*
#ifndef __CYGWIN__
"    -g        Group name numeric gid to switch to.\n"
#endif
*/
"    -h        Print this message, then exit.\n"
"    -i        Ignore cache for disabled servers\n"
#ifdef ENABLE_PIDFILE
"    -k        Kill a running daemon.\n"
#endif
#ifndef EXCLUDE_MASTER
"    -m FILE|off\n"
"              Use FILE as master file or turn it off. Path to FILE is\n"
"              relative $DNRD_ROOT (--dnrd-root)\n"
#endif
"    -M N      Set maximum number of open sockets to N\n"
"    -r N      Set retry interval to N seconds\n"
"    -s IPADDR(:domain)\n"
"              Set the DNS server.  You can specify an optional domain name,\n"
"              in which case a DNS request will only be sent to that server\n"
"              for names in that domain. (Can be used more than once for\n"
"              multiple or backup servers)\n"
"    -S N[+]   Send cache/query stats to syslog (LOG_INFO) every N seconds.\n"
"              Stats will not be resetted if the '+' is added\n"
"    -t N      Set forward DNS server timeout to N\n"
#ifndef __CYGWIN__
"    -u UID    Username or numeric id to switch to\n"
#endif
"    -R DIR    The dnrd root directory. dnrd will chroot to this dir.\n"
"    -v        Print out the version number and exit.\n"

#endif /* __GNU_LIBRARY__ */


"\n");



}


/*
 * parse_args()
 *
 * In:      argc - number of command-line arguments.
 *          argv - string array containing command-line arguments.
 *
 * Returns: an index into argv where we stopped parsing arguments.
 *
 * Abstract: Parses command-line arguments.  In some cases, it will
 *           set the appropriate global variables and return.  Otherwise,
 *           it performs the appropriate action and exits.
 *
 * Assumptions: Currently, we check to make sure that there are no arguments
 *              other than the defined options, so the return value is
 *              pretty useless and should be ignored.
 */
int parse_args(int argc, char **argv)
{
  static int load_balance = 0;
    int c;
    /*    int gotdomain = 0;*/

    progname = strrchr(argv[0], '/');
    if (!progname) progname = argv[0];

    while(1) {
#if defined(__GNU_LIBRARY__)
	c = getopt_long(argc, argv, short_options, long_options, 0);
#else
	c = getopt(argc, argv, short_options);
#endif
	if (c == -1) break;
	switch(c) {
	  case 'a': {
	      if (!inet_aton(optarg, &recv_addr.sin_addr)) {
		  log_msg(LOG_ERR, "%s: Bad ip address \"%s\"\n",
			  progname, optarg);
		  exit(-1);
	      }
	      break;
	  }
	case 'b': {
	  load_balance = 1;
	  break;
	}
	  case 'c': {
	      copy_string(cache_param, optarg, sizeof(cache_param));
	      break;
	  }
	  case 'd': {
	    opt_debug = atoi(optarg);
	    break;
	  }
	  case 'f': {
	      foreground = 1;
	      break;
	  }
	  case 'h': {
	      give_help();
	      exit(0);
	      break;
	  }
	  case 'i' : {
	    ignore_inactive_cache_hits = 1; 
	    break;
	  }
#ifdef ENABLE_PIDFILE
	  case 'k': {
	      if (!kill_current()) {
		  printf("No %s daemon found.  Exiting.\n", progname);
	      }
	      exit(0);
	      break;
	  }
#endif
#ifndef EXCLUDE_MASTER
	  case 'B': {
		  strncpy(blacklist, optarg, sizeof(blacklist));
		  break;
	  }
		  
	  case 'm': {
		  if (strcmp(optarg, "off") == 0) master_onoff = 0;
		  else strncpy(master_config, optarg, sizeof(master_config));
		  break;
	  }
#endif
	  case 'M': {
	    max_sockets = atoi(optarg);
	    log_debug(1, "Setting maximum number of open sockets to %i", 
		      max_sockets);
	    break;
	  }
	  case 'r': {
	    if ((reactivate_interval = atoi(optarg)))
	      log_debug(1, "Setting retry interval to %i seconds.", 
			reactivate_interval);
	    else 
	      log_debug(1, "Retry=0. Will never deactivate servers.");
	    break;
	  }
	  case 's': {
	    domnode_t *p;
	    char *s,*sep = strchr(optarg, (int)':');
	    
	    if (sep) { /* is a domain specified? */
	      s = make_cname(strnlwr(sep+1,200),200);
	      *sep = 0;
	      if ( (p=search_domnode(domain_list, s)) == NULL) {
		p=add_domain(domain_list, load_balance, s, 200);
		log_debug(1, "Added domain %s %s load balancing", sep+1, 
			  load_balance ? "with" : "without");
	      } else {
		free(s);
	      }
	    } else p=domain_list;
	    if (!add_srv(last_srvnode(p->srvlist), optarg)) {
	      log_msg(LOG_ERR, "%s: Bad ip address \"%s\"\n",
		      progname, optarg);
	      exit(-1);
	    } else {
	      log_debug(1, "Server %s added to domain %s", optarg, 
			sep ? sep+1:"(default)");
	    }
	    if (p->roundrobin != load_balance) {
	      p->roundrobin =load_balance;
	      log_debug(1, "Turned on load balancing for domain %s",
			cname2asc(p->domain));
	    }
	    if (sep) *sep = ':';
	    break;
	  }
	case 'S': {
		char *p = strrchr(optarg, '+');
		if (p) {
			stats_reset = 0;
			*p = '\0';
		}
		stats_interval = atoi(optarg);
		break;
	}
	  case 't': {
	    if ((forward_timeout = atoi(optarg)))
	      log_debug(1, "Setting timeout value to %i seconds.", 
			forward_timeout);
	    else 
	      log_debug(1, "Timeout=0. Servers will never timeout.");
	    break;
	  }
#ifndef __CYGWIN__ /** { **/
	  case 'u': {
		  strncpy(dnrd_user, optarg, sizeof(dnrd_user));
			break;
	  }
			/*
	  case 'g': {
		  strncpy(dnrd_group, optarg, sizeof(dnrd_group));
			break;
			} */
#endif /** } __CYGWIN__ **/
	  case 'v': {
	      printf("dnrd version %s\n\n", version);
	      exit(0);
	      break;
	  }

	  case 'R': {
	    strncpy(dnrd_root, optarg, sizeof(dnrd_root));
	    log_debug(1, "Using %s as chroot", dnrd_root);
	    break;
	  }
	  case ':': {
	      log_msg(LOG_ERR, "%s: Missing parameter for \"%s\"\n",
		      progname, argv[optind]);
	      exit(-1);
	      break;
	  }
	  case '?':
	  default: {
	      /* getopt_long will print "unrecognized option" for us */
	      give_help();
	      exit(-1);
	      break;
	  }
	}
    }

    if (optind != argc) {
	log_msg(LOG_ERR, "%s: Unknown parameter \"%s\"\n",
		progname, argv[optind]);
	exit(-1);
    }
    return optind;
}
