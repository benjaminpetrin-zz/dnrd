/*
 * sig.c - signal handling.
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

#include "sig.h"
#include "common.h"

/*
 * sig_handler()
 *
 * In:       signo - the type of signal that has been recieved.
 *
 * Abstract: If we receive SIGUSR1, we toggle debugging mode.
 *           Otherwise, we assume that we should die.
 */
void sig_handler(int signo)
{
  switch(signo) {
  case SIGUSR1:
    opt_debug = opt_debug ? 0 : 3;
    break;
#ifndef EXCLUDE_MASTER
  case SIGHUP:
    master_reload = 1;
    break;
#endif
  default:
    cleanexit(0);
  }
  signal(signo, sig_handler);
}


/*
 * Setup signal handlers.  
 * See: select_tut manual page 
 *
 * basicly, we mask out the signals that is not supposed to be
 * recieved except within the pselect call.
 */

void init_sig_handler(sigset_t *orig_sigmask) {
  sigset_t sigmask;

  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGQUIT);
  sigaddset(&sigmask, SIGTERM);
  sigaddset(&sigmask, SIGUSR1);
#ifndef EXCLUDE_MASTER
  sigaddset(&sigmask, SIGHUP);
#endif
  sigaddset(&sigmask, SIGCHLD);

  sigprocmask(SIG_BLOCK, &sigmask, orig_sigmask);

  signal(SIGINT,  sig_handler);
  signal(SIGQUIT, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGUSR1, sig_handler);
#ifndef EXCLUDE_MASTER
  signal(SIGHUP, sig_handler);
#endif
  /*
   * Handling TCP requests is done by forking a child.  When they terminate
   * they send SIGCHLDs to the parent.  This will eventually interrupt
   * some system calls.  Because I don't know if this is handled it's better
   * to ignore them -- 14OCT99wzk
   */
  signal(SIGCHLD, SIG_IGN);

}
