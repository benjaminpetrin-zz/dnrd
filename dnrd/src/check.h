/*

    File: check.h
    
    Copyright (C) 2004 by Natanael Copa <n@tanael.org>

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

#ifndef _DNRD_CHECK_H_
#define _DNRD_CHECK_H_

#include "srvnode.h"

 /* According to RFC 1035 (2.3.4) */
#define UDP_MAXSIZE 512

int check_query(void *msg, int len);
int check_reply(srvnode_t *s, void *msg, int len);



#endif /* _DNRD_CHECK_H_ */
