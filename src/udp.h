/*
 * udp.h - handle upd connections
 *
 * Copyright (C) 1999 Brad M. Garcia <garsh@home.com>
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

#ifndef _DNRD_UDP_H_
#define _DNRD_UDP_H_
#include "srvnode.h"
#include "query.h"

/* Function to call when a message is available on isock */
/* returns the new query */
query_t *udp_handle_request();

/* Call this to handle upd DNS replies */
void udp_handle_reply(query_t *q);

/* send a reactivation packet */
int udp_send_dummy(srvnode_t *s);


#endif /* _DNRD_UDP_H_ */
