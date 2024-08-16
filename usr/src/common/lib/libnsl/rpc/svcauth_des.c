/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svcauth_des.c	1.2.10.5"
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
 * svcauth_des.c, server-side des authentication
 *
 * We insure for the service the following:
 * (1) The timestamp microseconds do not exceed 1 million.
 * (2) The timestamp plus the window is less than the current time.
 * (3) The timestamp is not less than the one previously
 *	seen in the current session.
 *
 * It is up to the server to determine if the window size is
 * too small .
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
struct cache_entry *_rpc_authdes_cache;	/* [AUTHDES_CACHESZ] */
static short *authdes_lru;		/* [AUTHDES_CACHESZ] */

static void cache_init();		/* initialize the cache */
static short cache_spot();		/* find an entry in the cache */
static void cache_ref(/*short sid*/);	/* note that sid was ref'd */
static void invalidate();		/* invalidate entry in cache */

/*
 * cache statistics
 */
struct {
	u_long ncachehits;	/* times cache hit, and is not replay */
	u_long ncachereplays;	/* times cache hit, and is replay */
	u_long ncachemisses;	/* times cache missed */
} svcauthdes_stats;

/*
 * Service side authenticator for AUTH_DES
 */
enum auth_stat
_svcauth_des(rqst, msg)
	register struct svc_req *rqst;
	register struct rpc_msg *msg;
{

	register long *ixdr;
	des_block cryptbuf[2];
	register struct authdes_cred *cred;
	struct authdes_verf verf;
	int status;
	register struct cache_entry *entry;
	short sid;
	des_block *sessionkey;
	des_block ivec;
	u_int window;
	struct timeval timestamp;
	u_long namelen;
	struct area {
		struct authdes_cred area_cred;
		char area_netname[MAXNETNAMELEN+1];
	} *area;
	int nick_in_use;

	trace1(TR___svcauth_des, 0);
	if (_rpc_authdes_cache == NULL) {
		MUTEX_LOCK(&__authdes_lock);
		if (_rpc_authdes_cache == NULL)
			cache_init();
		MUTEX_UNLOCK(&__authdes_lock);
	}

	area = (struct area *)rqst->rq_clntcred;
	cred = (struct authdes_cred *)&area->area_cred;

	/*
	 * Get the credential
	 */
	ixdr = (long *)msg->rm_call.cb_cred.oa_base;
	cred->adc_namekind = IXDR_GET_ENUM(ixdr, enum authdes_namekind);
	switch (cred->adc_namekind) {
	case ADN_FULLNAME:
		namelen = IXDR_GET_U_LONG(ixdr);
		if (namelen > MAXNETNAMELEN) {
			trace1(TR___svcauth_des, 1);
			return (AUTH_BADCRED);
		}
		cred->adc_fullname.name = area->area_netname;
		memcpy(cred->adc_fullname.name, (char *)ixdr, (u_int)namelen);
		cred->adc_fullname.name[namelen] = 0;
		ixdr += (RNDUP(namelen) / BYTES_PER_XDR_UNIT);
		cred->adc_fullname.key.key.high = (u_long)*ixdr++;
		cred->adc_fullname.key.key.low = (u_long)*ixdr++;
		cred->adc_fullname.window = (u_long)*ixdr++;
		nick_in_use = 0;
		break;
	case ADN_NICKNAME:
		cred->adc_nickname = (u_long)*ixdr++;
		nick_in_use = 1;
		break;
	default:
		trace1(TR___svcauth_des, 1);
		return (AUTH_BADCRED);
	}

	/*
	 * Get the verifier
	 */
	ixdr = (long *)msg->rm_call.cb_verf.oa_base;
	verf.adv_xtimestamp.key.high = (u_long)*ixdr++;
	verf.adv_xtimestamp.key.low = (u_long)*ixdr++;
	verf.adv_int_u = (u_long)*ixdr++;

	/*
	 * Get the conversation key
	 */
	if (!nick_in_use) {
		netobj	pkey;
		char	pkey_data[1024];

		sessionkey = &cred->adc_fullname.key;
		if (! getpublickey(cred->adc_fullname.name, pkey_data)) {
			(void) syslog(LOG_DEBUG,
			    gettxt("uxnsl:135",
				"_svcauth_des: no public key."));
			return (AUTH_BADCRED); /* no key */
		}
		pkey.n_bytes = pkey_data;
		pkey.n_len = strlen(pkey_data) + 1;
		if (key_decryptsession_pk(cred->adc_fullname.name, &pkey,
				sessionkey) < 0) {
			(void) syslog(LOG_DEBUG,
			    gettxt("uxnsl:136",
				"_svcauth_des: key_decryptsessionkey failed"));
			trace1(TR___svcauth_des, 1);
			return (AUTH_BADCRED);	/* key not found */
		}
	} else { /* ADN_NICKNAME */
		sid = cred->adc_nickname;
		if (sid >= AUTHDES_CACHESZ) {
			(void) syslog(LOG_DEBUG,
			    gettxt("uxnsl:137", "_svcauth_des: bad nickname"));
			trace1(TR___svcauth_des, 1);
			return (AUTH_BADCRED);	/* garbled credential */
		}
		MUTEX_LOCK(&__authdes_lock);
		sessionkey = &_rpc_authdes_cache[sid].key;
	}

	/*
	 * Decrypt the timestamp
	 */
	cryptbuf[0] = verf.adv_xtimestamp;
	if (cred->adc_namekind == ADN_FULLNAME) {
		cryptbuf[1].key.high = cred->adc_fullname.window;
		cryptbuf[1].key.low = verf.adv_winverf;
		ivec.key.high = ivec.key.low = 0;
		status = cbc_crypt((char *)sessionkey, (char *)cryptbuf,
			2 * sizeof (des_block), DES_DECRYPT | DES_HW,
			(char *)&ivec);
	} else {
		status = ecb_crypt((char *)sessionkey, (char *)cryptbuf,
			sizeof (des_block), DES_DECRYPT | DES_HW);
	}
	if (DES_FAILED(status)) {
		if (nick_in_use)
			MUTEX_UNLOCK(&__authdes_lock);
		(void) syslog(LOG_DEBUG,
		    gettxt("uxnsl:138", "_svcauth_des: decryption failure"));
		trace1(TR___svcauth_des, 1);
		return (AUTH_FAILED);	/* system error */
	}

	/*
	 * XDR the decrypted timestamp
	 */
	ixdr = (long *)cryptbuf;
	timestamp.tv_sec = IXDR_GET_LONG(ixdr);
	timestamp.tv_usec = IXDR_GET_LONG(ixdr);

	/*
	 * Check for valid credentials and verifiers.
	 * They could be invalid because the key was flushed
	 * out of the cache, and so a new session should begin.
	 * Be sure and send AUTH_REJECTED{CRED, VERF} if this is the case.
	 */
	{
		struct timeval current;
		int winverf;

		if (!nick_in_use) {
			window = IXDR_GET_U_LONG(ixdr);
			winverf = IXDR_GET_U_LONG(ixdr);
			if (winverf != window - 1) {
				(void) syslog(LOG_DEBUG,
				    gettxt("uxnsl:139",
				     "_svcauth_des: window verifier mismatch"));
				trace1(TR___svcauth_des, 1);
				/* garbled credential or invalid secret key */
				return (AUTH_BADCRED);
			}
			MUTEX_LOCK(&__authdes_lock);
			sid = cache_spot(sessionkey, cred->adc_fullname.name,
					&timestamp);
			if (sid < 0) {
				MUTEX_UNLOCK(&__authdes_lock);
				(void) syslog(LOG_DEBUG,
				    gettxt("uxnsl:140",
					"_svcauth_des: replayed credential"));
				trace1(TR___svcauth_des, 1);
				return (AUTH_REJECTEDCRED);	/* replay */
			}
		} else {	/* ADN_NICKNAME */
			window = _rpc_authdes_cache[sid].window;
		}

		if ((u_long)timestamp.tv_usec >= USEC_PER_SEC) {
			MUTEX_UNLOCK(&__authdes_lock);
			(void) syslog(LOG_DEBUG,
			    gettxt("uxnsl:141",
				"_svcauth_des: invalid time value (tv_usec)"));
			/* cached out (bad key), or garbled verifier */
			trace1(TR___svcauth_des, 1);
			return (nick_in_use ? AUTH_REJECTEDVERF : AUTH_BADVERF);
		}
		if (nick_in_use && BEFORE(&timestamp,
					&_rpc_authdes_cache[sid].laststamp)) {
			MUTEX_UNLOCK(&__authdes_lock);
			(void) syslog(LOG_DEBUG,
			    gettxt("uxnsl:142",
				"_svcauth_des: timestamp before last seen"));
			trace1(TR___svcauth_des, 1);
			return (AUTH_REJECTEDVERF);	/* replay */
		}
		(void) gettimeofday(&current, (struct timezone *) 0);
		current.tv_sec -= window;	/* allow for expiration */
		if (!BEFORE(&current, &timestamp)) {
			MUTEX_UNLOCK(&__authdes_lock);
			(void) syslog(LOG_DEBUG,
			    gettxt("uxnsl:143",
				"_svcauth_des: timestamp expired"));
			/* replay, or garbled credential */
			trace1(TR___svcauth_des, 1);
			return (nick_in_use ? AUTH_REJECTEDVERF : AUTH_BADCRED);
		}
	}

	/*
	 * Set up the reply verifier
	 */
	verf.adv_nickname = sid;

	/*
	 * xdr the timestamp before encrypting
	 */
	ixdr = (long *)cryptbuf;
	IXDR_PUT_LONG(ixdr, timestamp.tv_sec - 1);
	IXDR_PUT_LONG(ixdr, timestamp.tv_usec);

	/*
	 * encrypt the timestamp
	 */
	status = ecb_crypt((char *)sessionkey, (char *)cryptbuf,
				sizeof (des_block), DES_ENCRYPT | DES_HW);
	if (DES_FAILED(status)) {
		MUTEX_UNLOCK(&__authdes_lock);
		(void) syslog(LOG_DEBUG,
		    gettxt("uxnsl:144", "_svcauth_des: encryption failure"));
		trace1(TR___svcauth_des, 1);
		return (AUTH_FAILED);	/* system error */
	}
	verf.adv_xtimestamp = cryptbuf[0];

	/*
	 * Serialize the reply verifier, and update rqst
	 */
	ixdr = (long *)msg->rm_call.cb_verf.oa_base;
	*ixdr++ = (long)verf.adv_xtimestamp.key.high;
	*ixdr++ = (long)verf.adv_xtimestamp.key.low;
	*ixdr++ = (long)verf.adv_int_u;

	rqst->rq_xprt->xp_verf.oa_flavor = AUTH_DES;
	rqst->rq_xprt->xp_verf.oa_base = msg->rm_call.cb_verf.oa_base;
	rqst->rq_xprt->xp_verf.oa_length =
		(char *)ixdr - msg->rm_call.cb_verf.oa_base;

	/*
	 * We succeeded, commit the data to the cache now and
	 * finish cooking the credential.
	 */
	entry = &_rpc_authdes_cache[sid];
	entry->laststamp = timestamp;
	cache_ref(sid);
	if (!nick_in_use) {
		cred->adc_fullname.window = window;
		cred->adc_nickname = sid;	/* save nickname */
		if (entry->rname != NULL) {
			mem_free(entry->rname, strlen(entry->rname) + 1);
		}
		entry->rname =
			(char *)mem_alloc((u_int)strlen(cred->adc_fullname.name)
					+ 1);
		if (entry->rname != NULL) {
			(void) strcpy(entry->rname, cred->adc_fullname.name);
		} else {
			(void) syslog(LOG_DEBUG,
			    gettxt("uxnsl:32", "%s: out of memory"),
			    "_svcauth_des");
		}
		entry->key = *sessionkey;
		entry->window = window;
		/* mark any cached cred invalid */
		invalidate(entry->localcred);
	} else { /* ADN_NICKNAME */
		/*
		 * nicknames are cooked into fullnames
		 */
		cred->adc_namekind = ADN_FULLNAME;
		cred->adc_fullname.name = entry->rname;
		cred->adc_fullname.key = entry->key;
		cred->adc_fullname.window = entry->window;
	}
	MUTEX_UNLOCK(&__authdes_lock);
	trace1(TR___svcauth_des, 1);
	return (AUTH_OK);	/* we made it! */
}


/*
 * Initialize the cache
 */
static void
cache_init()
{
	register int i;

	trace1(TR_cache_init, 0);
	_rpc_authdes_cache = (struct cache_entry *)
		mem_alloc(sizeof (struct cache_entry) * AUTHDES_CACHESZ);
	memset((char *)_rpc_authdes_cache, 0,
		sizeof (struct cache_entry) * AUTHDES_CACHESZ);

	authdes_lru = (short *)mem_alloc(sizeof (short) * AUTHDES_CACHESZ);
	/*
	 * Initialize the lru list
	 */
	for (i = 0; i < AUTHDES_CACHESZ; i++) {
		authdes_lru[i] = i;
	}
	trace1(TR_cache_init, 1);
}

/*
 * Find the lru victim
 */
static short
cache_victim()
{
	trace1(TR_cache_victim, 0);
	trace1(TR_cache_victim, 1);
	return (authdes_lru[AUTHDES_CACHESZ-1]);
}

/*
 * Note that sid was referenced
 */
static void
cache_ref(sid)
	register short sid;
{
	register int i;
	register short curr;
	register short prev;

	trace1(TR_cache_ref, 0);
	prev = authdes_lru[0];
	authdes_lru[0] = sid;
	for (i = 1; prev != sid; i++) {
		curr = authdes_lru[i];
		authdes_lru[i] = prev;
		prev = curr;
	}
	trace1(TR_cache_ref, 1);
}

/*
 * Find a spot in the cache for a credential containing
 * the items given. Return -1 if a replay is detected, otherwise
 * return the spot in the cache.
 */
static short
cache_spot(key, name, timestamp)
	register des_block *key;
	char *name;
	struct timeval *timestamp;
{
	register struct cache_entry *cp;
	register int i;
	register u_long hi;
	short dummy;

	trace1(TR_cache_spot, 0);
	hi = key->key.high;
	for (cp = _rpc_authdes_cache, i = 0; i < AUTHDES_CACHESZ; i++, cp++) {
		if (cp->key.key.high == hi &&
		    cp->key.key.low == key->key.low &&
		    cp->rname != NULL &&
		    memcmp(cp->rname, name, strlen(name) + 1) == 0) {
			if (BEFORE(timestamp, &cp->laststamp)) {
				svcauthdes_stats.ncachereplays++;
				trace1(TR_cache_spot, 1);
				return (-1);	/* replay */
			}
			svcauthdes_stats.ncachehits++;
			trace1(TR_cache_spot, 1);
			return (i);
			/* refresh */
		}
	}
	svcauthdes_stats.ncachemisses++;
	dummy = cache_victim();
	trace1(TR_cache_spot, 1);
	return (dummy);	/* new credential */
}


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
	short grouplen;	/* length of cached groups */
	short groups[NGROUPS];	/* cached groups */
};

static void
invalidate(cred)
	char *cred;
{
	trace1(TR_invalidate, 0);
	if (cred == NULL) {
		trace1(TR_invalidate, 1);
		return;
	}
	((struct bsdcred *)cred)->grouplen = INVALID;
	trace1(TR_invalidate, 1);
}
