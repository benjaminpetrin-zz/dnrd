/*

    File: cache.c -- add response caching
    
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "common.h"
#include "dns.h"
#include "cache.h"
#include "lib.h"
#include "dns.h"
#include "srvnode.h"

	/*
	 * Cache time calculations are done in seconds.  CACHE_TIMEUNIT
	 * defines a minute.  You might want to set it to 1 for
	 * debugging.
	 */

#define	CACHE_TIMEUNIT		60

	/*
	 * DNS queries that have been answered positively are stored for
	 * CACHE_TIME, errors for only CACHE_NEGTIME minutes.  Reusing
	 * a positive answer resets the entry's expire time.  After
	 * CACHE_MAXTIME the item is removed anyway.
	 */

#define	CACHE_NEGTIME		(     5 * CACHE_TIMEUNIT)
#define	CACHE_TIME		(    60 * CACHE_TIMEUNIT)
#define CACHE_MAXTIME		(6 * 60 * CACHE_TIMEUNIT)

	/*
	 * The expire function can be called as often as wanted.
	 * It will however wait CACHE_MINCYCLE between two
	 * expire runs.
	 */

#define	CACHE_MINCYCLE		(5 * CACHE_TIMEUNIT)

	/*
	 * If after an expire the cache holds more than CACHE_HIGHWATER
	 * items the oldest items are removed until there are only
	 * CACHE_LOWWATER left.
	 */

#define	CACHE_HIGHWATER		1000
#define	CACHE_LOWWATER		 800


typedef struct _cache {
    unsigned int  code;
    char	 *name;		/* Objectname */
    int		  type, class;	/* Query type and class. */

    int		  positive;	/* Positive or error response? */
    unsigned long created;
    unsigned long lastused;
    unsigned long expires;

    dnsheader_t	*p;		/* The DNS packet with decoded header */

  srvnode_t *server; /* the server that gave this answer */
    struct _cache *next, *prev;
} cache_t;


char	cache_param[256]	= "";
int	cache_onoff		= 1;
long	cache_highwater		= CACHE_HIGHWATER;
long	cache_lowwater		= CACHE_LOWWATER;

static cache_t *cachelist	= NULL;
static cache_t *lastcache	= NULL;

int cache_hits		  = 0;
int cache_misses		= 0;


static int free_cx(cache_t *cx)
{
    free_packet(cx->p);
    free(cx->name);
    free(cx);
    
    return (0);
}

static cache_t *create_cx(dnsheader_t *x, rr_t *query, srvnode_t *server)
{
    cache_t	*cx;

    cx = allocate(sizeof(cache_t));

    cx->name = strdup(query->name);
    cx->code = get_stringcode(cx->name);

    cx->positive = x->ancount;
    cx->type     = query->type;
    cx->class    = query->class;
    cx->p        = x;
    cx->lastused = time(NULL);
    cx->server = server;

    return (cx);
}

static cache_t *append_cx(cache_t *cx)
{
    if (lastcache == NULL) {
	cachelist = cx;
	lastcache = cx;
    }
    else {
	lastcache->next = cx;
	cx->prev = lastcache;
	lastcache = cx;
    }

    cx->created = time(NULL);
    log_debug(3, "cache: added %s, type= %d, class: %d, ans= %d\n",
	      cx->name, cx->type, cx->class, cx->p->ancount);

    return (cx);
}

static cache_t *remove_cx(cache_t *cx)
{
    if (cx->next != NULL) {
	cx->next->prev = cx->prev;
    }
    else {
	lastcache = cx->prev;
    }

    if (cx->prev != NULL) {
	cx->prev->next = cx->next;
    }
    else {
	cachelist = cx->next;
    }

    return (cx);
}



/*
 * cache_dnspacket()
 *
 * In:      packet - the response packet to cache.
 *          len    - length of the response packet.
 *
 * Returns: 0, all the time.
 *
 * Take the response packet and look if it meets some basic
 * conditions for caching.  If so put the entire response into
 * our cache.
 */
int cache_dnspacket(void *packet, int len, srvnode_t *server)
{
    dnsheader_t *x;
    rr_t	query;
    cache_t	*cx = NULL;
    unsigned long dns_ttl;

    if ((cache_onoff == 0) ||
	parse_query(&query, packet, len) ||
	(GET_QR(query.flags) == 0) ||
	(*query.name == 0)) {
	return (0);
    }

    x = parse_packet(packet, len);

    /*
     * Ok, the packet is interesting for us.  Let's put it into our
     * cache list.
     */
    sem_wait(&dnrd_sem);
    cx = create_cx(x, &query, server);
    append_cx(cx);

    /*
     * Set the expire time of the cached object.
     */
    cx->lastused = time(NULL);
    dns_ttl = cache_dns_ttl(packet, len, x);
    if(dns_ttl == 0) {
      cx->expires  = cx->lastused +
	           ((cx->p->ancount > 0) ? CACHE_TIME : CACHE_NEGTIME);
      log_debug(5, "Using static cache timeouts -> %lu seconds\n",
	           ((cx->p->ancount > 0) ? (unsigned long)CACHE_TIME : (unsigned long)CACHE_NEGTIME));
    }else{
      cx->expires = cx->lastused + dns_ttl;
      log_debug(5, "Using dynamic cache timeouts -> %lu seconds\n", dns_ttl);
    }
    sem_post(&dnrd_sem);
    return (0);
}

/*
 * skip_labels()
 *
 * In: packet - pointer to the current position within the packet
 *     term   - beyond the end of the packet. Passing this is bad
 *
 * Returns:
 *     a pointer to the byte immediately beyond the name
 *     OR null, if we walk off the end of the packet
 */
unsigned char *skip_labels(unsigned char *packet, unsigned char *term) {
  while(1) {
    if(*packet > 63) return packet+2;
    if(*packet == 0) return packet+1;
    packet += *packet + 1;
    if(packet >= term) return NULL;
  }
}

/*
 * cache_dns_ttl()
 *
 * In: packet   - a pointer to the packet data
 *     len      - the size of the packet
 *     hdr      - Preprocessed header data
 *
 * Returns: the minimum TTL within the packet
 *    OR 0 if we can't work it out
 */
unsigned long cache_dns_ttl(void *packet, int len, dnsheader_t *hdr) {
  unsigned char *p = packet;
  unsigned char *term = p + len;
  unsigned long ttl = -1;
  unsigned long t;
  unsigned short i;
  unsigned long buf[1];
  unsigned short sbuf[1];

  // skip header
  p += PACKET_DATABEGIN;

  // skip questions
  i = hdr->qdcount;
  while(i) {
    p = skip_labels(p, term);
    if(p==NULL) return 0;
    p += 4;
    i--;
    if(p>=term) return 0; //walked off the end
    log_debug(8, "Walking Questions. %hu to go.\n", i);
  }

  i = hdr->ancount + hdr->nscount + hdr->arcount;
  while(i) {
    if(p>=term || p<(unsigned char *)packet) return 0; //walked off the end
    p = skip_labels(p, term);
    if(p==NULL) return 0;
    p += 4;
    if(p>=term) return 0; //walked off the end

    memcpy(buf, p, sizeof(buf));
    t = ntohl(buf[0]);
    log_debug(8, "ttl = %lu", t);
    if(t < ttl) ttl = t;
    p += 4;
    if(p>=term) return 0; //walked off the end

    memcpy(sbuf, p, sizeof(sbuf));
    log_debug(8, "Jumping rdata of %hu bytes", ntohs(sbuf[0]));
    p += ntohs(sbuf[0]);
    p += 2;

    log_debug(8, "Walking Answers/Nameservers/Additional Information. %hu to go from %p.\n", i, p);
    i--;
  }

  if(ttl == -1) return 0;
  return ttl;
}

/*
 * cache_lookup()
 *
 * In/Out:  packet - the query packet on input, response on output.
 *          len    - length of the query packet.
 *
 * Returns: length of the response packet "cached",
 *          0 if no cached response is available.
 *
 * Look into the query packet too see if it is a lookup request that
 * might already have been cached.  If so, copy the data to the
 * cached pointer returning the length of the cached packet
 * as return value.
 *
 * The function assumes that the area cached points to is
 * large enough.
 */
int cache_lookup(void *packet, int len)
{
    unsigned int code;
    /*    dnsheader_t *x;*/
    rr_t	query;
    cache_t	*cx = NULL;

    if ((cache_onoff == 0) ||
	parse_query(&query, packet, len) ||
	(GET_QR(query.flags) == 1) ||
	(*query.name == 0) ||
	(query.class != DNS_CLASS_INET)) {
	return (0);
    }

    /*
     * The query could be in the cache.  Let's take the packet ...
     * ... and search our cache for this request.
     */
    code = get_stringcode(query.name);
    for (cx = cachelist; cx != NULL; cx = cx->next) {
      if (cx->code == code  &&
	  cx->type == query.type  &&
	  cx->class == query.class  &&
	  strcasecmp(cx->name, query.name) == 0) {
	
	log_debug(3, "cache: found %s, type= %d, class: %d, ans= %d, ttl= %d\n",
		  cx->name, cx->type, cx->class, cx->p->ancount, cx->expires-time(NULL)); //XXX remove expiry

	if (cx->positive > 0) {
	  cx->lastused = time(NULL);
	  //cx->expires  = cx->lastused + CACHE_TIME;
	}

        if(cx->expires <= cx->lastused) {
          // this packet has expired, get rid of it!
	  remove_cx(cx);
	  free_cx(cx);
          log_debug(3, "found cached entry is expired, deleting");
          return (0);
        }

	memcpy(packet + 2, cx->p->packet + 2, cx->p->len - 2);
	cache_hits++;

	/* lets check if the server is active */
	if (ignore_inactive_cache_hits && cx->server->inactive ) {
	  log_debug(2, "server is inactive. Skipping cache entry");
	  return (0);
	}

	return (cx->p->len);
      }
    }

    cache_misses++;
    return (0);
}


/*
 * Item expiration
 */

/*
 * cmp_cachemru() - Compare the "ages" of two cache entries.
 *
 * In:      cx - pointer to first cache entry. 
 *          cy - pointer to second cache entry.
 *
 * Returns: >0 if cx is older than cy
 *          <0 if cx is younger than cy
 *           0 if cx is equal to cy
 */
static int cmp_cachemru(const void *x, const void * y)
{
    const cache_t **cx = (const cache_t **)x;
    const cache_t **cy = (const cache_t **)y;
    return ((*cy)->lastused - (*cx)->lastused);
}

/*
 * expire_oldest() - expire cache entries until lowwater is reached
 *
 * In:      itemcount - number of entries in cache.
 *
 * Returns: 0, always.
 *
 * Sorts all the entries in order of age, oldest first.
 * Then removes the first (itemcount - cache_lowwater) entries.
 */
static int expire_oldest(int itemcount)
{
    int	     i, n;
    cache_t *cx, **cv;
    
    if ((n = itemcount - cache_lowwater) < 0) return (0);

    cv = allocate(itemcount * sizeof(cache_t *));
    for (i = 0, cx = cachelist; i < itemcount; i++, cx = cx->next) {
	cv[i] = cx;
    }

    qsort(cv, itemcount, sizeof(cache_t *), cmp_cachemru);
    for (i=0; i<n; i++) {
	cx = cv[i];
	remove_cx(cx);
	free_cx(cx);
    }

    log_msg(LOG_NOTICE, "cache: %d of %d mru expired, %d remaining\n",
	   n, itemcount, itemcount - n);

    return (0);
}

/*
 * cache_expire() - Expire old entries from the cache.
 *
 * Returns: 0, always.
 *
 * When called, this function walks the entire cachelist, looking for entries
 * that should be expired.
 */
int cache_expire(void)
{
    int	           total, expired;
    time_t         now;
    cache_t	  *cx, *next;
    static time_t  lastexpire = 0;

    if (cache_onoff == 0) return (0);

    /* Initialize lastexpire to the current time */
    if (lastexpire == 0) lastexpire = time(NULL);

    now = time(NULL);
    if (now - lastexpire < CACHE_MINCYCLE) return (0);

    total = 0;
    expired = 0;

    cx = cachelist;
    while (cx != NULL) {
	total += 1;
	next = cx->next;

	if (cx->expires < now  ||  (now - cx->created) > CACHE_MAXTIME) {
	    remove_cx(cx);
	    free_cx(cx);
	    expired++;
	}
		
	cx = next;
    }


    log_debug(2, "cache stats: %d entries", total);

    if (expired > 0) {
	log_debug(2, "cache: %d of %d expired, %d remaining\n",
		  expired, total, total - expired);
    }

    total = total - expired;
    if (total > cache_highwater) {
	expire_oldest(total);
    }

    lastexpire = now;
    return (0);
}


/*
 * cache_init()
 *
 * Returns: 0, always.
 *
 * Called once from main() to initialize our cache.
 * Caching is turned on by default.
 */
int cache_init(void)
{
    if (*cache_param != 0) {
	if (strcmp(cache_param, "off") == 0) {
	    cache_onoff = 0;
	}
	else if (isdigit((int)(*cache_param))) {
	    char *p = strchr(cache_param, ':');

	    if (p == NULL) {
		cache_highwater = atoi(cache_param);
		cache_lowwater  = (cache_highwater * 75) / 100;
	    }
	    else {
		*p++ = 0;
		cache_lowwater  = atoi(cache_param);
		cache_highwater = atoi(p);
	    }
	}
	else {
	    log_msg(LOG_ERR, "invalid cache parameter: %s", cache_param);
	}
    }

    if (cache_onoff == 0) {
	log_msg(LOG_NOTICE, "caching turned off");
    }
    else {
	if (cache_highwater < 0) {
	    cache_highwater = CACHE_HIGHWATER;
	    cache_lowwater  = CACHE_LOWWATER;
	}
	else if (cache_lowwater > cache_highwater) {
	    cache_lowwater  = (cache_highwater * 75) / 100;
	}

	log_debug(1, "cache low/high: %d/%d", cache_lowwater, cache_highwater);
    }

    return (0);
}
