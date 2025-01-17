/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/ntp_peer.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * ntp_peer.c - management of data maintained for peer associations
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp.h"

/*
 * These routines manage the allocation of memory to peer structures
 * and the maintenance of the peer hash table.  The two main entry
 * points are findpeer(), which looks for corresponding peer data
 * in the peer list, newpeer(), which allocates a new peer structure
 * and adds it to the list, and unpeer(), which demobilizes the association
 * and deallocates the structure.
 */

/*
 * The peer hash table (imported by the protocol module).
 */
struct peer *peer_hash[HASH_SIZE];
int peer_hash_count[HASH_SIZE];		/* count of peers in each bucket */

/*
 * The association ID hash table.  Used for lookups by association ID
 */
struct peer *assoc_hash[HASH_SIZE];
int assoc_hash_count[HASH_SIZE];

/*
 * The free list.  Clean structures only, please.
 */
struct peer *peer_free;
int peer_free_count;

/*
 * Association ID.  We initialize this value randomly, the assign a new
 * value every time the peer structure is incremented.
 */
u_short current_association_ID;

/*
 * Memory allocation watermarks.
 */
#define	INIT_PEER_ALLOC		15	/* initialize space for 15 peers */
#define	INC_PEER_ALLOC		5	/* when we run out, add 5 more */

/*
 * Miscellaneous statistic counters which may be queried.
 */
u_long peer_timereset;		/* time stat counters were zeroed */
u_long findpeer_calls;		/* number of calls to findpeer */
u_long assocpeer_calls;		/* number of calls to findpeerbyassoc */
u_long peer_allocations;	/* number of allocations from the free list */
u_long peer_demobilizations;	/* number of structs freed to free list */
int total_peer_structs;		/* number of peer structs in circulation */

/*
 * default interface.  Imported from the io module.
 */
extern struct interface *any_interface;

/*
 * Timer queue and current time.  Imported from the timer module.
 */
extern u_long current_time;
extern struct event timerqueue[];

/*
 * Our initial allocation of peer space
 */
static struct peer init_peer_alloc[INIT_PEER_ALLOC];

/*
 * Initialization data.  When configuring peers at initialization time,
 * we try to get their poll update timers initialized to different values
 * to prevent us from sending big clumps of data all at once.
 */
u_long init_peer_starttime;
extern int initializing;
extern int debug;


/*
 * init_peer - initialize peer data structures and counters
 *
 * N.B. We use the random number routine in here.  It had better be
 *      initialized prior to getting here.
 */
void
init_peer()
{
	register int i;
	extern u_long ranp2();

	/*
	 * Clear hash table and counters.
	 */
	for (i = 0; i < HASH_SIZE; i++) {
		peer_hash[i] = 0;
		peer_hash_count[i] = 0;
		assoc_hash[i] = 0;
		assoc_hash_count[i] = 0;
	}

	/*
	 * Clear stat counters
	 */
	findpeer_calls = peer_allocations = 0;
	assocpeer_calls = peer_demobilizations = 0;

	/*
	 * Initialization counter.
	 */
	init_peer_starttime = 0;

	/*
	 * Initialize peer memory.
	 */
	peer_free = 0;
	for (i = 0; i < INIT_PEER_ALLOC; i++) {
		init_peer_alloc[i].next = peer_free;
		peer_free = &init_peer_alloc[i];
	}
	total_peer_structs = INIT_PEER_ALLOC;
	peer_free_count = INIT_PEER_ALLOC;

	/*
	 * Initialize our first association ID
	 */
	current_association_ID = (u_short)ranp2(16);
	if (current_association_ID == 0)
		current_association_ID = 1;
}



/*
 * getmorepeermem - add more peer structures to the free list
 */
void
getmorepeermem()
{
	register int i;
	register struct peer *peer;
	extern char *emalloc();

	peer = (struct peer *)emalloc(INC_PEER_ALLOC*sizeof(struct peer));
	for (i = 0; i < INC_PEER_ALLOC; i++) {
		peer->next = peer_free;
		peer_free = peer;
		peer++;
	}

	total_peer_structs += INC_PEER_ALLOC;
	peer_free_count += INC_PEER_ALLOC;
}



/*
 * findexistingpeer - return a pointer to a peer in the hash table
 */
struct peer *
findexistingpeer(addr, start_peer)
	struct sockaddr_in *addr;
	struct peer *start_peer;
{
	register struct peer *peer;

	/*
	 * start_peer is included so we can locate instances of the
	 * same peer through different interfaces in the hash table.
	 */
	if (start_peer == 0)
		peer = peer_hash[HASH_ADDR(addr)];
	else
		peer = start_peer->next;
	
	while (peer != 0) {
		if (NSRCADR(addr) == NSRCADR(&peer->srcadr)
		    && NSRCPORT(addr) == NSRCPORT(&peer->srcadr))
			return peer;
		peer = peer->next;
	}

	return (struct peer *)0;
}


/*
 * findpeer - find and return a peer in the hash table.
 */
struct peer *
findpeer(srcadr, dstadr)
	struct sockaddr_in *srcadr;
	struct interface *dstadr;
{
	register struct peer *any_inter_peer;
	register struct peer *peer;
	int hash;
	extern char *ntoa();

	findpeer_calls++;

	any_inter_peer = 0;
	hash = HASH_ADDR(srcadr);
	for (peer = peer_hash[hash]; peer != 0; peer = peer->next) {
		if (NSRCADR(srcadr) == NSRCADR(&peer->srcadr)
		    && NSRCPORT(srcadr) == NSRCPORT(&peer->srcadr)) {
			if (peer->dstadr == dstadr)
				return peer;	/* got it! */
			if (peer->dstadr == any_interface) {
				/*
				 * We shouldn't have more than one
				 * instance of the peer in the table,
				 * but I don't trust this.  Save this
				 * one for later and continue search.
				 */
				if (any_inter_peer == 0)
					any_inter_peer = peer;
				else
					syslog(LOG_ERR,
		"two instances of default interface for %s in hash table",
					    ntoa(srcadr));
			}
		}
	}

	/*
	 * If we didn't find the specific peer but found a wild card,
	 * modify the interface and return him.
	 */
	if (any_inter_peer != 0) {
		any_inter_peer->dstadr = dstadr;
		return any_inter_peer;
	}

	/*
	 * Out of luck.  Return 0.
	 */
	return (struct peer *)0;
}

/*
 * findpeerbyassocid - find and return a peer using his association ID
 */
struct peer *
findpeerbyassoc(assoc)
	int assoc;
{
	register struct peer *peer;
	int hash;
	extern char *ntoa();

	assocpeer_calls++;

	hash = assoc & HASH_MASK;
	for (peer = assoc_hash[hash]; peer != 0; peer = peer->ass_next) {
		if ((u_short)assoc == peer->associd)
			return peer;	/* got it! */
	}

	/*
	 * Out of luck.  Return 0.
	 */
	return (struct peer *)0;
}

/*
 * unpeer - remove peer structure from hash table and free structure
 */
void
unpeer(peer_to_remove)
	struct peer *peer_to_remove;
{
	int hash;
	char *ntoa();
#ifdef REFCLOCK
	extern void refclock_unpeer();
#endif

	hash = HASH_ADDR(&peer_to_remove->srcadr);
	peer_hash_count[hash]--;
	peer_demobilizations++;

#ifdef REFCLOCK
	/*
	 * If this peer is actually a clock, shut it down first
	 */
	if (peer_to_remove->flags & FLAG_REFCLOCK)
		refclock_unpeer(peer_to_remove);
#endif

	if (peer_hash[hash] == peer_to_remove)
		peer_hash[hash] = peer_to_remove->next;
	else {
		register struct peer *peer;

		peer = peer_hash[hash];
		while (peer != 0 && peer->next != peer_to_remove)
			peer = peer->next;
		
		if (peer == 0) {
			peer_hash_count[hash]++;
			syslog(LOG_ERR, "peer struct for %s not in table!",
			    ntoa(&peer->srcadr));
		} else {
			peer->next = peer_to_remove->next;
		}
	}

	/*
	 * Remove him from the association hash as well.
	 */
	hash = peer_to_remove->associd & HASH_MASK;
	assoc_hash_count[hash]--;
	if (assoc_hash[hash] == peer_to_remove)
		assoc_hash[hash] = peer_to_remove->ass_next;
	else {
		register struct peer *peer;

		peer = assoc_hash[hash];
		while (peer != 0 && peer->ass_next != peer_to_remove)
			peer = peer->ass_next;
		
		if (peer == 0) {
			assoc_hash_count[hash]++;
			syslog(LOG_ERR,
			    "peer struct for %s not in association table!",
			    ntoa(&peer->srcadr));
		} else {
			peer->ass_next = peer_to_remove->ass_next;
		}
	}

	TIMER_DEQUEUE(&peer_to_remove->event_timer);

	peer_to_remove->next = peer_free;
	peer_free = peer_to_remove;
	peer_free_count++;
}


/*
 * peer_config - configure a new peer
 */
struct peer *
peer_config(srcadr, dstadr, hmode, version, key, flags)
	struct sockaddr_in *srcadr;
	struct interface *dstadr;
	int hmode;
	int version;
	u_long key;
	int flags;
{
	register struct peer *peer;
	struct peer *findexistingpeer();
	struct peer *newpeer();
	extern char *ntoa();

#ifdef DEBUG
	if (debug)
		printf("peer_config: addr %s mode %d version %d key %u\n",
		    ntoa(srcadr), hmode, version, key);
#endif
	/*
	 * See if we have this guy in the tables already.  If
	 * so just mark him configured.
	 */
	peer = findexistingpeer(srcadr, (struct peer *)0);
	if (dstadr != 0) {
		while (peer != 0) {
			if (peer->dstadr == dstadr)
				break;
			peer = findexistingpeer(srcadr, peer);
		}
	}

	/*
	 * Torque the flags to make sure they're valid
	 */
	flags &= (FLAG_AUTHENABLE|FLAG_MINPOLL);

	/*
	 * If we found one, just change his mode and mark him configured.
	 */
	if (peer != 0) {
		peer->hmode = hmode;
		peer->flags = ((u_char)(flags|FLAG_CONFIG))
		    |(peer->flags & (FLAG_REFCLOCK|FLAG_DEFBDELAY));
		peer->keyid = key;
		return peer;
	}

	/*
	 * If we're here this guy is unknown to us.  Make a new peer
	 * structure for him.
	 */
	peer = newpeer(srcadr, dstadr, hmode, version, key);
	if (peer != 0)
		peer->flags |= (u_char)(flags|FLAG_CONFIG);
	return peer;
}


/*
 * newpeer - initialize a new peer association
 */
struct peer *
newpeer(srcadr, dstadr, hmode, version, key)
	struct sockaddr_in *srcadr;
	struct interface *dstadr;
	int hmode;
	int version;
	u_long key;
{
	register struct peer *peer;
	register int i;
	extern struct interface *findbcastinter();
	extern void transmit();
#ifdef REFCLOCK
	extern int refclock_newpeer();
#endif

	/*
	 * Some dirt here.  Some of the initialization requires
	 * knowlege of our system state.
	 */
	extern u_long sys_bdelay;
	
	if (peer_free_count == 0)
		getmorepeermem();

	peer = peer_free;
	peer_free = peer->next;
	peer_free_count--;

	/*
	 * Initialize the structure.  This stuff is sort of part of
	 * the receive procedure and part of the clear procedure rolled
	 * into one.
	 *
	 * Zero the whole thing for now.  We might be pickier later.
	 */
	bzero((char *)peer, sizeof(struct peer));

	peer->srcadr = *srcadr;
	if (dstadr != 0)
		peer->dstadr = dstadr;
	else if (hmode == MODE_BROADCAST)
		peer->dstadr = findbcastinter(srcadr);
	else
		peer->dstadr = any_interface;
	peer->hmode = (u_char)hmode;
	peer->keyid = key;

	if (hmode == MODE_BCLIENT) {
		peer->estbdelay = sys_bdelay;
		peer->flags |= FLAG_DEFBDELAY;
	}

	peer->leap = LEAP_NOTINSYNC;
	peer->hpoll = NTP_MINPOLL;
	peer->precision = DEFPRECISION;
	peer->version = (u_char)version;
	peer->estdisp = PEER_MAXDISP;
	peer->stratum = STRATUM_UNSPEC;

	for (i = 0; i < PEER_SHIFT; i++)
		peer->filter_order[i] = i;
	
	/*
	 * Assign him an association ID and increment the system variable
	 */
	peer->associd = current_association_ID;
	if (++current_association_ID == 0)
		++current_association_ID;

	/*
	 * Note time on statistics timers.
	 */
	peer->timereset = current_time;
	peer->timereachable = current_time;
	peer->timereceived = current_time;

#ifdef REFCLOCK
	if (ISREFCLOCKADR(srcadr)) {
		/*
		 * We let the reference clock support do clock
		 * dependent initialization.  This includes setting
		 * the peer timer, since the clock may have requirements
		 * for this.
		 */
		if (!refclock_newpeer(peer)) {
			/*
			 * Dump it, something screwed up
			 */
			peer->next = peer_free;
			peer_free = peer;
			peer_free_count++;
			return (struct peer *)0;
		}
	} else {
#endif
	/*
	 * Set up timer.  If initializing, just make sure we start polling
	 * in different 4 second intervals.
	 */
	peer->event_timer.peer = peer;
	peer->event_timer.event_handler = transmit;

	if (initializing) {
		init_peer_starttime += (1<<EVENT_TIMEOUT);
		if (init_peer_starttime >= (1<<NTP_MINPOLL))
			init_peer_starttime = (1<<EVENT_TIMEOUT);
		peer->event_timer.event_time = init_peer_starttime;
	} else {
		/*
		 * First expiry is set to 32 seconds from now.
		 */
		peer->event_timer.event_time
		    = (1 << (NTP_MINPOLL-1)) + current_time;
	}
	TIMER_ENQUEUE(timerqueue, &peer->event_timer);
#ifdef REFCLOCK
	}
#endif

	/*
	 * Put him in the hash tables.
	 */
	i = HASH_ADDR(srcadr);
	peer->next = peer_hash[i];
	peer_hash[i] = peer;
	peer_hash_count[i]++;

	i = peer->associd & HASH_MASK;
	peer->ass_next = assoc_hash[i];
	assoc_hash[i] = peer;
	assoc_hash_count[i]++;

	return peer;
}


/*
 * peer_unconfig - remove the configuration bit from a peer
 */
int
peer_unconfig(srcadr, dstadr)
	struct sockaddr_in *srcadr;
	struct interface *dstadr;
{
	register struct peer *peer;
	int num_found;
	struct peer *findexistingpeer();

	num_found = 0;
	peer = findexistingpeer(srcadr, (struct peer *)0);
	while (peer != 0) {
		if (peer->flags & FLAG_CONFIG
		    && (dstadr == 0 || peer->dstadr == dstadr)) {
			num_found++;
			/*
			 * Tricky stuff here.  If the peer is polling us
			 * in active mode, turn off the configuration bit
			 * and make the mode passive.  This allows us to
			 * avoid dumping a lot of history for peers we
			 * might choose to keep track of in passive mode.
			 * The protocol will eventually terminate undesirables
			 * on its own.
			 */
			if (peer->hmode == MODE_ACTIVE
			    && peer->pmode == MODE_ACTIVE) {
				peer->hmode = MODE_PASSIVE;
				peer->flags &= ~FLAG_CONFIG;
			} else {
				unpeer(peer);
				peer = 0;
			}
		}
		peer = findexistingpeer(srcadr, peer);
	}
	return num_found;
}


/*
 * peer_clr_stats - clear peer module stat counters
 */
void
peer_clr_stats()
{
	findpeer_calls = 0;
	assocpeer_calls = 0;
	peer_allocations = 0;
	peer_demobilizations = 0;
	peer_timereset = current_time;
}

/*
 * peer_reset - reset stat counters in a peer structure
 */
void
peer_reset(peer)
	struct peer *peer;
{
	if (peer == 0)
		return;
	peer->sent = 0;
	peer->received = 0;
	peer->badlength = 0;
	peer->processed = 0;
	peer->badauth = 0;
	peer->bogusorg = 0;
	peer->oldpkt = 0;
	peer->baddelay = 0;
	peer->seldelaytoolarge = 0;
	peer->seldisptoolarge = 0;
	peer->selbroken = 0;
	peer->seltooold = 0;
	peer->untrustable = 0;
	peer->timereset = current_time;
}


/*
 * peer_all_reset - reset all peer stat counters
 */
void
peer_all_reset()
{
	struct peer *peer;
	int hash;

	for (hash = 0; hash < HASH_SIZE; hash++)
		for (peer = peer_hash[hash]; peer != 0; peer = peer->next)
			peer_reset(peer);
}
