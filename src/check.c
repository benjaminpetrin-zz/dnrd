/*
 * check.c - validation of DNS packets
 *
 * Copyright (C) 2004 Natanael Copa <n@tanael.org>
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

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "check.h"
#include "dns.h"

/*
 * check_query()
 *
 * This functions does some test to verify that the message is sane
 * returns:
 *          -1 if the query is insane and should be ignored
 *          0 if the query is ok and should be processed
 *          1 if the query is not ok and a format error should be sent
 */

int check_query(int len, void *msg) {
  short int flags = ntohs(((short int *)msg)[1]); /* flags */

  /* first check the size */
  if (len <12) {
    log_debug("Query packet is to small. Ignoring");
    return -1;
  }

  if (len > UDP_MAXSIZE) {
    log_debug("Query packet is too big. Ignoring");
    return -1;
  }

  /* check if Z is set. It should never been set */
  if (flags & MASK_Z) {
    log_debug("Z was set. Ignoring query");
    return -1;
  }

  /* Check if it is a query, if QR is set */
  if (flags & MASK_QR) {
    log_debug("QR was set. Ignoring query");
    return -1;
  }

  /* ok this is valid */
  return 0;
}

/*
 * check_reply()
 *
 * This functions does some test to verify that the message is sane
 * returns:
 *          -1 if the reply is insane and should be ignored
 *          0 if the query is ok and should be processed
 */

int check_reply(int len, void *msg) {
  short int flags = ntohs(((short int *)msg)[1]); /* flags */

  if (len <12) {
    log_debug("Reply packet was to small. Ignoring");
    return -1;
  }

  /* check if Z is set. It should never been set */
  if (flags & MASK_Z) {
    log_debug("Z was set. Ignoring reply");
    return -1;
  }

  /* Check if it is a query, if QR is set */
  if (! (flags & MASK_QR)) {
    log_debug("QR was not set. Ignoring reply");
    return -1;
  }

  /* check for possible cache poisoning attempts etc here.... */

  /* ok this packet is ok */
  return 0;
}
