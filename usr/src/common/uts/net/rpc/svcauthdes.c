/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/svcauthdes.c	1.15"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	svcauthdes.c, server-side des authentication
 *
 *	We insure for the service the following:
 *
 *	1) The timestamp microseconds do not exceed 1 million.
 *	2) The timestamp plus the window is less than the current
 *	   time.
 *	3) The timestamp is not less than the one previously 
 *	   seen in the current session.
 *
 *	It is up to the server to determine if the window size is
 *	too small .
 *
 */

#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <svc/time.h>
#include <proc/user.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/pid.h>
#include <net/inet/in.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/rpclk.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_des.h>
#include <net/rpc/svc_auth.h>
#include <net/rpc/des_crypt.h>
#include <net/xti.h>
#include <net/tihdr.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <net/rpc/rpc_msg.h>
#include <net/rpc/clnt.h>

extern	enum clnt_stat	netname2user(char *, uid_t *, gid_t *, int *, int *);
extern	void		bcopy(void *, void *, size_t);
extern	int		bcmp(char *, char *, size_t);
extern	void		bzero(void *, size_t);
extern	enum clnt_stat	key_decryptsession(char *, des_block *);
extern	int		strlen(char *);
extern	char		*strcpy();

extern timestruc_t	hrestime;

#define BEFORE(t1, t2) 	timercmp(t1, t2, <)
#define AUTHDES_CACHESZ 64

/*
 * LRU cache of conversation keys and some other useful items.
 */

struct cache_entry {
	des_block key;			/* conversation key */
	char *rname;			/* client's name */
	u_int window;			/* credential lifetime window */
	struct timeval laststamp;	/* detect replays of creds */
	char *localcred;		/* generic local credential */
};

static struct cache_entry *authdes_cache;
static short *authdes_lru;

/*
 * lock for lru cache. We may want to use spin
 * lock instead, but this is held for a long time
 */
extern sleep_t	authdes_lock; 

static void cache_init();	/* initialize the cache */
static short cache_spot();	/* find an entry in the cache */
static void cache_ref(short sid);	/* note that sid was ref'd */
static void invalidate();	/* invalidate entry in cache */

/*
 * cache statistics struct and its mp lock.
 */
struct {
	u_long ncachehits;	/* times cache hit, and is not replay */
	u_long ncachereplays;	/* times cache hit, and is replay */
	u_long ncachemisses;	/* times cache missed */
} svcauthdes_stats;
extern fspin_t	svathd_lock;

/*
 * these macros help in portability
 */
#define	ATOMIC_SVATHD_REPLAYS() {			\
	FSPIN_LOCK(&(svathd_lock));			\
	(svcauthdes_stats.ncachereplays)++;		\
	FSPIN_UNLOCK(&(svathd_lock));			\
}

#define	ATOMIC_SVATHD_HITS() {				\
	FSPIN_LOCK(&(svathd_lock));			\
	(svcauthdes_stats.ncachehits)++;		\
	FSPIN_UNLOCK(&(svathd_lock));			\
}

#define	ATOMIC_SVATHD_MISSES() {			\
	FSPIN_LOCK(&(svathd_lock));			\
	(svcauthdes_stats.ncachemisses)++;		\
	FSPIN_UNLOCK(&(svathd_lock));			\
}

/*
 * _svcauth_des(rqst, msg)
 *	DES authenticator for a rpc request.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns auth status in a auth_stat.
 *
 * Description:
 *	DES authenticator for a rpc request.
 *
 * Parameters:
 *
 *	rqst			# service request
 *	msg			# rpc message
 */
enum auth_stat
_svcauth_des(struct svc_req *rqst, struct rpc_msg *msg)
{
	long			*ixdr;
	des_block		cryptbuf[2];
	struct	authdes_cred	*cred;
	struct	authdes_verf	verf;
	int			status;
	struct	cache_entry	*entry;
	short			sid;
	des_block		*sessionkey;
	des_block		ivec;
	u_int			window;
	struct	timeval		timestamp;
	u_long			namelen;
	struct area {
		struct authdes_cred area_cred;
		char area_netname[MAXNETNAMELEN+1];
	}			*area;

	if (authdes_cache == NULL) {
		SLEEP_LOCK(&authdes_lock, PRIRPC);
		/*
		 * check again as someone else
		 * may have raced us
		 */
		if (authdes_cache == NULL) {
			cache_init();
		}
		SLEEP_UNLOCK(&authdes_lock);
	}

	/* LINTED pointer alignment */
	area = (struct area *)rqst->rq_clntcred;
	cred = (struct authdes_cred *)&area->area_cred;

	/*
	 * Get the credential
	 */
	/* LINTED pointer alignment */
	ixdr = (long *)msg->rm_call.cb_cred.oa_base;
	cred->adc_namekind = IXDR_GET_ENUM(ixdr, enum authdes_namekind);
	RPCLOG(0x10, "_svcauth_des: cred->adc_namekind = %d\n", cred->adc_namekind);

	switch (cred->adc_namekind) {
		case ADN_FULLNAME:
			namelen = IXDR_GET_U_LONG(ixdr);
			if (namelen > MAXNETNAMELEN) {
				return (AUTH_BADCRED);
			}
			cred->adc_fullname.name = area->area_netname;
			bcopy((char *)ixdr, cred->adc_fullname.name, 
				(u_int)namelen);
			cred->adc_fullname.name[namelen] = 0;
			ixdr += (RNDUP(namelen) / BYTES_PER_XDR_UNIT);
			cred->adc_fullname.key.key.high = (u_long)*ixdr++;
			cred->adc_fullname.key.key.low = (u_long)*ixdr++;
			cred->adc_fullname.window = (u_long)*ixdr++;
			break;
		case ADN_NICKNAME:
			cred->adc_nickname = IXDR_GET_U_LONG(ixdr);
			RPCLOG(0x10, "_svcauth_des: cred->adc_nickname = %d\n",
			       cred->adc_nickname);
			break;
		default:
			return (AUTH_BADCRED);	
	}

	/*
	 * Get the verifier
	 */
	/* LINTED pointer alignment */
	ixdr = (long *)msg->rm_call.cb_verf.oa_base;
	verf.adv_xtimestamp.key.high = (u_long)*ixdr++;
	verf.adv_xtimestamp.key.low = (u_long)*ixdr++;
	verf.adv_int_u = (u_long)*ixdr++;

	SLEEP_LOCK(&authdes_lock, PRIRPC);

	/*
	 * Get the conversation key
 	 */
	if (cred->adc_namekind == ADN_FULLNAME) {
		sessionkey = &cred->adc_fullname.key;
		if (key_decryptsession((char *)cred->adc_fullname.name, 
					sessionkey) != RPC_SUCCESS) {

			RPCLOG(0x10,
		"_svcauth_des: key_decryptsessionkey failed\n", 0);

			SLEEP_UNLOCK(&authdes_lock);
			/*
			 * key not found
			 */
			return (AUTH_BADCRED);
		}
	} else {
		/*
		 * nickname
		 */
		sid = cred->adc_nickname;
		if (sid >= AUTHDES_CACHESZ) {

			RPCLOG(0x10, "_svcauth_des: bad nickname %d\n", sid);

			SLEEP_UNLOCK(&authdes_lock);
			/*
			 * garbled credentials
			 */
			return (AUTH_BADCRED);
		}
		sessionkey = &authdes_cache[sid].key;
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
			2*sizeof(des_block), DES_DECRYPT, 
			(char *)&ivec);
	} else {
		status = ecb_crypt((char *)sessionkey, (char *)cryptbuf,
			sizeof(des_block), DES_DECRYPT);
	}
	if (DES_FAILED(status)) {

		RPCLOG(0x10, "_svcauth_des: decryption failure\n", 0);

		SLEEP_UNLOCK(&authdes_lock);
		/*
		 * system error
		 */
		return (AUTH_FAILED);
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
	 * Be sure and send AUTH_REJECTED{CRED, VERF} if this
	 * is the case.
	 */
	{
		struct timeval current;
		int nick;
		int winverf;

		if (cred->adc_namekind == ADN_FULLNAME) {
			window = IXDR_GET_U_LONG(ixdr);
			winverf = IXDR_GET_U_LONG(ixdr);

			RPCLOG(0x10, "_svcauth_des: winverf=%d ", winverf);
			RPCLOG(0x10, "(window=%d)\n", window);

			if (winverf != window - 1) {

				RPCLOG(0x10,
		"_svcauth_des: window verifier mismatch %d\n", winverf);

				SLEEP_UNLOCK(&authdes_lock);
				/*
				 * garbled credentials
				 */
				return (AUTH_BADCRED);
			}
			sid = cache_spot(sessionkey, cred->adc_fullname.name, 
				&timestamp);
			if (sid < 0) {

				RPCLOG(0x10,
		"_svcauth_des: replayed credential sid %d\n", sid);

				SLEEP_UNLOCK(&authdes_lock);
				/*
				 * replayed credentials
				 */
				return (AUTH_REJECTEDCRED);
			}
			nick = 0;
		} else {
			/*
			 * nickname
			 */
			window = authdes_cache[sid].window;
			nick = 1;
		}

		if ((u_long)timestamp.tv_usec >= MILLION) {

			RPCLOG(0x10,
		"_svcauth_des: invalid usecs %d\n", timestamp.tv_usec);

			SLEEP_UNLOCK(&authdes_lock);
			/*
			 * cached out (bad key), or garbled verifier
			 */
			return (nick ? AUTH_REJECTEDVERF : AUTH_BADVERF);
		}
		if (nick && BEFORE(&timestamp, 
					&authdes_cache[sid].laststamp)) {

			RPCLOG(0x10,
			"_svcauth_des: timestamp before last seen\n", 0);

			SLEEP_UNLOCK(&authdes_lock);
			/*
			 * replayed credentials
			 */
			return (AUTH_REJECTEDVERF);
		}
		current.tv_sec = hrestime.tv_sec;
		current.tv_usec = hrestime.tv_nsec/1000;

		/*
		 * allow for expiration
		 */
		current.tv_sec -= window;

		if (!BEFORE(&current, &timestamp)) {

			RPCLOG(0x10, "_svcauth_des: timestamp expired\n", 0);

			SLEEP_UNLOCK(&authdes_lock);
			/*
			 * replay, or garbled credential
			 */
			return (nick ? AUTH_REJECTEDVERF : AUTH_BADCRED);
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
			sizeof(des_block), DES_ENCRYPT);
	if (DES_FAILED(status)) {

		RPCLOG(0x10, "_svcauth_des: encryption failure\n", 0);

		SLEEP_UNLOCK(&authdes_lock);
		/*
		 * system error
		 */
		return (AUTH_FAILED);
	}
	verf.adv_xtimestamp = cryptbuf[0];

	/*
	 * Serialize the reply verifier, and update rqst
	 */
	/* LINTED pointer alignment */
	ixdr = (long *)msg->rm_call.cb_verf.oa_base;
	*ixdr++ = (long)verf.adv_xtimestamp.key.high;
	*ixdr++ = (long)verf.adv_xtimestamp.key.low;
	IXDR_PUT_LONG(ixdr, verf.adv_int_u);	/* nickname */

	rqst->rq_xprt->xp_verf.oa_flavor = AUTH_DES;
	rqst->rq_xprt->xp_verf.oa_base = msg->rm_call.cb_verf.oa_base;
	rqst->rq_xprt->xp_verf.oa_length = 
		(char *)ixdr - msg->rm_call.cb_verf.oa_base;

	/*
	 * We succeeded, commit the data to the cache now and
	 * finish cooking the credential.
	 */
	entry = &authdes_cache[sid];
	entry->laststamp = timestamp;
	cache_ref(sid);
	if (cred->adc_namekind == ADN_FULLNAME) {
		cred->adc_fullname.window = window;
		/*
		 * save nickname
		 */
		cred->adc_nickname = sid;
		if (entry->rname != NULL) {
			kmem_free(entry->rname, strlen(entry->rname) + 1);
		}
		entry->rname = (char *)
		  kmem_alloc((strlen(cred->adc_fullname.name) + 1), KM_SLEEP);

		if (entry->rname != NULL) {
			(void) strcpy(entry->rname, cred->adc_fullname.name);
		} else {
			RPCLOG(0x10, "_svcauth_des: out of memory\n", 0);

		}
		entry->key = *sessionkey;
		entry->window = window;
		invalidate(entry->localcred); /* mark any cached cred invalid */
	} else {
		/*
		 * nicknames are cooked into fullnames
		 */	
		cred->adc_namekind = ADN_FULLNAME;
		cred->adc_fullname.name = entry->rname;
		cred->adc_fullname.key = entry->key;
		cred->adc_fullname.window = entry->window;
	}
	SLEEP_UNLOCK(&authdes_lock);

	/*
	 * whew!
	 */
	return (AUTH_OK);
}


/*
 * cache_init()
 *	Initialize the LRU cache of conversation keys and
 *	some other useful items.
 *
 * Calling/Exit State:
 *	The sleep lock protecting the cache (authdes_lock) must
 *	be held on entry. It is still held on exit.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine simply initializes the LRU cache.
 *
 * Parameters:
 *
 *	None.
 *
 */
static void
cache_init()
{
	short	i;

	ASSERT(SLEEP_LOCKOWNED(&authdes_lock));

	authdes_cache = (struct cache_entry *)
	  kmem_alloc((sizeof(struct cache_entry) * AUTHDES_CACHESZ), KM_SLEEP);	
	bzero((char *)authdes_cache, 
		sizeof(struct cache_entry) * AUTHDES_CACHESZ);

	authdes_lru = (short *)
		kmem_alloc((sizeof(short) * AUTHDES_CACHESZ), KM_SLEEP);

	/*
	 * Initialize the lru list
	 */
	for (i = 0; i < AUTHDES_CACHESZ; i++) {
		authdes_lru[i] = i;
	}
}


/*
 * cache_victim()
 *	Find the lru victim in the lru cache.
 *
 * Calling/Exit State:
 *	The sleep lock protecting the cache (authdes_lock) must
 *	be held on entry. It is still held on exit.
 *
 *	Returns a short indicating the victim's position.
 *
 * Description:
 *	This routine simply finds the lru victim.
 *
 * Parameters:
 *
 *	None.
 *
 */
static short
cache_victim()
{
	ASSERT(SLEEP_LOCKOWNED(&authdes_lock));

	return (authdes_lru[AUTHDES_CACHESZ-1]);
}

/*
 * cache_ref(sid)
 *	Note that sid was referenced.
 *
 * Calling/Exit State:
 *	The sleep lock protecting the cache (authdes_lock) must
 *	be held on entry. It is still held on exit.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine inserts sid into the cache.
 *
 * Parameters:
 *
 *	sid			# index
 *
 */
static void
cache_ref(short sid)
{
	short	curr;
	short	prev;
	int	i;

	ASSERT(SLEEP_LOCKOWNED(&authdes_lock));

	prev = authdes_lru[0];
	authdes_lru[0] = sid;
	for (i = 1; prev != sid; i++) {
		curr = authdes_lru[i];
		authdes_lru[i] = prev;
		prev = curr;
	}
}


/*
 * cache_spot(key, name, timestamp)
 *	Find a spot in the cache.
 *	
 *
 * Calling/Exit State:
 *	The sleep lock protecting the cache (authdes_lock) must
 *	be held on entry. It is still held on exit.
 *
 *	Return -1 if a replay is detected, otherwise return the
 *	spot in the cache.
 *
 * Description:
 *	Find a spot in the cache for a credential containing
 *	the key, name and timestamp.Return -1 if a replay is
 *	detected, otherwise return the spot in the cache.
 *
 * Parameters:
 *
 *	key			# des key
 *	name			# full name 
 *	timestamp 		# timestamp from client
 *
 */
static short
cache_spot(des_block *key, char *name, struct timeval *timestamp)
{
	struct	cache_entry	*cp;
	u_long			hi;
	int			i;

	ASSERT(SLEEP_LOCKOWNED(&authdes_lock));

	hi = key->key.high;
	for (cp = authdes_cache, i = 0; i < AUTHDES_CACHESZ; i++, cp++) {
		if (cp->key.key.high == hi && 
			cp->key.key.low == key->key.low &&
			cp->rname != NULL &&
			bcmp(cp->rname, name, strlen(name) + 1) == 0) {
			if (BEFORE(timestamp, &cp->laststamp)) {
				/*
				 * replayed credentials
				 */
				ATOMIC_SVATHD_REPLAYS();

				return (-1);
			}

			ATOMIC_SVATHD_HITS();

			/*
			 * should refresh now
			 */
			return (i);
		}
	}

	ATOMIC_SVATHD_MISSES();

	/*
	 * this is a new credential
	 */
	return (cache_victim());
}


/*
 * Local credential handling stuff.
 * NOTE: bsd unix dependent.
 * Other operating systems should put something else here.
 */
#define UNKNOWN 	-2	/* grouplen, if cached cred is unknown user */
#define INVALID		-1 	/* grouplen, if cache entry is invalid */

struct bsdcred {
	uid_t uid;			/* cached uid */
	gid_t gid;			/* cached gid */
	short grouplen;			/* length of cached groups */
	short groups[NGROUPS_UMAX];	/* cached groups */
};

/*
 * authdes_getucred(adc, uid, gid, grouplen, groups)
 *	Map a des credential into a unix cred.
 *	
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 1 on success, 0 on failure.
 *
 * Description:
 *	Map a des credential into a unix cred.
 *	We cache the credential here so the application does
 *	not have to make an rpc call every time to interpret
 *	the credential.
 *
 * Parameters:
 *
 *	adc			# des credential
 *	uid			# user id
 *	gid			# group id
 *	grouplen		# number of groups
 *	groups			# all the groups
 *
 */
int
authdes_getucred(struct authdes_cred *adc, uid_t *uid, gid_t *gid,
		 short *grouplen, int *groups)
{
	struct	bsdcred	*cred;
	unsigned	sid;
	uid_t		i_uid;	
	gid_t		i_gid;
	int		i_grouplen;
	int		i;

	sid = adc->adc_nickname;
	if (sid >= AUTHDES_CACHESZ) {
		/*
		 * invalid sid (nickname/index)
		 */
		RPCLOG(0x10, "authdes_getucred: invalid nickname\n", 0);

		return (0);
	}

	SLEEP_LOCK(&authdes_lock, PRIRPC);
	/* LINTED pointer alignment */
	cred = (struct bsdcred *)authdes_cache[sid].localcred;
	if (cred == NULL) {
		cred = (struct bsdcred *)
			kmem_alloc(sizeof(struct bsdcred), KM_SLEEP);
		authdes_cache[sid].localcred = (char *)cred;
		cred->grouplen = INVALID;
	}

	if (cred->grouplen == INVALID) {
		/*
		 * not in cache: lookup
		 */
		if (netname2user(adc->adc_fullname.name, &i_uid, &i_gid, 
			&i_grouplen, groups) != RPC_SUCCESS)
		{
			/*
			 * mark as lookup up, but not found
			 */
			RPCLOG(0x10, "authdes_getucred: unknown netname\n", 0);

			cred->grouplen = UNKNOWN;
			SLEEP_UNLOCK(&authdes_lock);
			return (0);
		}

		/*
		 * missed unix credential cache
		 */
		RPCLOG(0x10, "authdes_getucred: missed ucred cache\n", 0);

		*uid = cred->uid = i_uid;
		*gid = cred->gid = i_gid;
		*grouplen = cred->grouplen = (short)i_grouplen;
		for (i = i_grouplen - 1; i >= 0; i--) {
			cred->groups[i] = groups[i]; /* int to short */
		}
		SLEEP_UNLOCK(&authdes_lock);
		return (1);
	} else if (cred->grouplen == UNKNOWN) {
		/*
		 * Already lookup up, but no match found
		 */	
		SLEEP_UNLOCK(&authdes_lock);
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
	SLEEP_UNLOCK(&authdes_lock);

	return (1);
}

/*
 * invalidate(cred)
 *	Invalidate entry in cache.
 *	
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Invalidate entry in cache.
 *
 * Parameters:
 *
 *	cred			# unix credential
 *
 */
static void
invalidate(char *cred)
{
	if (cred == NULL) {
		return;
	}

	/* LINTED pointer alignment */
	((struct bsdcred *)cred)->grouplen = INVALID;
}
