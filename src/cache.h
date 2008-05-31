/*

    File: cache.h
    
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

#ifndef _DNRD_CACHE_H_
#define	_DNRD_CACHE_H_

#include "dns.h"

extern char cache_param[256];
extern int cache_hits;
extern int cache_misses;

/* Interface for DNS cache */
int cache_dnspacket(void *packet, int len, srvnode_t *server);
int cache_lookup(void *packet, int len);
int cache_expire(void);
int cache_init(void);

unsigned char *skip_labels(unsigned char *packet, unsigned char *term);
unsigned long cache_dns_ttl(void *packet, int len, dnsheader_t *hdr);
#endif /* _DNRD_CACHE_H_ */

