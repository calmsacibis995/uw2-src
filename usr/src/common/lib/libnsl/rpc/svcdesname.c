/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svcdesname.c	1.2.10.2"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * svcdesname.c
 *
 * server-side des authentication
 * netname conversion dependent stuff
 *
 */

#include <string.h>
#include <unistd.h>
#include <rpc/des_crypt.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include "trace.h"
#include <sys/syslog.h>
#include "rpc_mt.h"

#ifndef NGROUPS
#define	NGROUPS 16
#endif

#define	USEC_PER_SEC ((u_long) 1000000L)
#define	BEFORE(t1, t2) timercmp(t1, t2, <)

/*
 * LRU cache of conversation keys and some other useful items.
 */
#define	AUTHDES_CACHESZ 64
struct cache_entry {
	des_block key;			/* conversation key */
	char *rname;			/* client's name */
	u_int window;			/* credential lifetime window */
	struct timeval laststamp;	/* detect replays of creds */
	char *localcred;		/* generic local credential */
};
extern struct cache_entry *_rpc_authdes_cache; /* [AUTHDES_CACHESZ] */

/*
 * Local credential handling stuff.
 * NOTE: bsd unix dependent.
 * Other operating systems should put something else here.
 */
#define	UNKNOWN 	-2	/* grouplen, if cached cred is unknown user */
#define	INVALID		-1 	/* grouplen, if cache entry is invalid */

struct bsdcred {
	uid_t uid;		/* cached uid */
	gid_t gid;		/* cached gid */
	short grouplen;		/* length of cached groups */
	short groups[NGROUPS];	/* cached groups */
};

/*
 * Map a des credential into a unix cred.
 * We cache the credential here so the application does
 * not have to make an rpc call every time to interpret
 * the credential.
 */

authdes_getucred(adc, uid, gid, grouplen, groups)
	struct authdes_cred *adc;
	uid_t *uid;
	gid_t *gid;
	short *grouplen;
	gid_t *groups;
{
	unsigned sid;
	register int i;
	uid_t i_uid;
	gid_t i_gid;
	int i_grouplen;
	struct bsdcred *cred;

	trace1(TR_authdes_getucred, 0);
	sid = adc->adc_nickname;
	if (sid >= AUTHDES_CACHESZ) {
		(void) syslog(LOG_DEBUG,
		    gettxt("uxnsl:145",
			"authdes_getucred: invalid nickname"));
		trace1(TR_authdes_getucred, 1);
		return (0);
	}
	MUTEX_LOCK(&__authdes_lock);
	cred = (struct bsdcred *)_rpc_authdes_cache[sid].localcred;
	if (cred == NULL) {
		cred = (struct bsdcred *)mem_alloc(sizeof (struct bsdcred));
		_rpc_authdes_cache[sid].localcred = (char *)cred;
		cred->grouplen = INVALID;
	}
	if (cred->grouplen == INVALID) {
		/*
		 * not in cache: lookup
		 */
		if (!netname2user(adc->adc_fullname.name, (uid_t *) &i_uid,
			(gid_t *)&i_gid, &i_grouplen, (gid_t *) groups)){
			MUTEX_UNLOCK(&__authdes_lock);
			(void) syslog(LOG_DEBUG,
			    gettxt("uxnsl:146",
				"authdes_getucred: unknown netname"));
			/* mark as lookup up, but not found */
			cred->grouplen = UNKNOWN;
			trace1(TR_authdes_getucred, 1);
			return (0);
		}
		(void) syslog(LOG_DEBUG,
		    gettxt("uxnsl:147",
			"authdes_getucred: missed credentials cache"));
		*uid = cred->uid = i_uid;
		*gid = cred->gid = i_gid;
		*grouplen = cred->grouplen = i_grouplen;
		for (i = i_grouplen - 1; i >= 0; i--) {
			cred->groups[i] = groups[i];	/* int to short */
		}
		trace1(TR_authdes_getucred, 1);
		MUTEX_UNLOCK(&__authdes_lock);
		return (1);
	} else if (cred->grouplen == UNKNOWN) {
		/*
		 * Already lookup up, but no match found
		 */
		MUTEX_UNLOCK(&__authdes_lock);
		trace1(TR_authdes_getucred, 1);
		return (0);
	}

	/*
	 * cached credentials
	 */
	*uid = cred->uid;
	*gid = cred->gid;
	*grouplen = cred->grouplen;
	for (i = cred->grouplen - 1; i >= 0; i--) {
		groups[i] = cred->groups[i];	/* short to int */
	}
	MUTEX_UNLOCK(&__authdes_lock);
	trace1(TR_authdes_getucred, 1);
	return (1);
}
