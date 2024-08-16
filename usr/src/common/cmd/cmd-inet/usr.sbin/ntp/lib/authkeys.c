/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/authkeys.c	1.2"
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
 * authkeys.c - routines to manage the storage of authentication keys
 */
#include <stdio.h>
#include <sys/types.h>

/*
 * Structure to store keys in in the hash table.
 */
struct savekey {
	struct savekey *next;
	u_long key[2];
	u_long keyid;
	u_short flags;
};

#define	KEY_TRUSTED	0x1	/* this key is trusted */
#define	KEY_KNOWN	0x2	/* this key is known */


/*
 * The hash table.  This is indexed by the low order bits of the
 * keyid.  We make this fairly big for potentially busy servers.
 */
#define	HASHSIZE	64
#define	HASHMASK	((HASHSIZE)-1)
#define	KEYHASH(keyid)	((keyid) & HASHMASK)

struct savekey *key_hash[HASHSIZE];
int authnumkeys;		/* number of keys in here, info only */

/*
 * Storage for free key structures.  We malloc() such things but
 * never free them.
 */
struct savekey *authfreekeys;
int authnumfreekeys;		/* number of free ones */

#define	MEMINC	12		/* number of new free ones to get at once */

/*
 * Size of the key schedule
 */
#define	KEY_SCHED_SIZE	128	/* number of octets to store key schedule */

/*
 * The zero key, which we always have.  Store the permutted key
 * zero in here.
 */
#define	ZEROKEY_L	0x01010101	/* odd parity zero key */
#define	ZEROKEY_R	0x01010101	/* right half of same */
u_char zeroekeys[KEY_SCHED_SIZE];
u_char zerodkeys[KEY_SCHED_SIZE];

/*
 * The key cache.  We cache the last key we looked at here.
 */
u_long cache_keyid;
u_short cache_flags;
u_char cache_ekeys[KEY_SCHED_SIZE];
u_char cache_dkeys[KEY_SCHED_SIZE];


/*
 * Some statistic counters, for posterity
 */
u_long authkeylookups;		/* calls to authhavekey() */
u_long authkeynotfound;		/* requested key unknown */
u_long authencryptions;		/* number of encryptions */
u_long authdecryptions;		/* number of decryptions */
u_long authdecryptok;		/* number of successful decryptions */
u_long authkeyuncached;		/* calls to encrypt/decrypt with uncached key */
u_long authnokey;		/* calls to encrypt with no key */

extern void auth_subkeys();


/*
 * init_auth - initialize internal data
 */
void
init_auth()
{
	u_long zerokey[2];

	/*
	 * Initialize hash table and free list
	 */
	memset((char *)key_hash, '\0', sizeof key_hash);
	authnumkeys = 0;
	authfreekeys = 0;
	authnumfreekeys = 0;
	cache_keyid = 0;

	/*
	 * Initialize the zero key
	 */
	zerokey[0] = ZEROKEY_L;
	zerokey[1] = ZEROKEY_R;
	auth_subkeys(zerokey, zeroekeys, zerodkeys); /* could just zero all */

	/*
	 * Zero stat counters
	 */
	authkeylookups = authkeynotfound = 0;
	authencryptions = authdecryptions = 0;
	authdecryptok = authkeyuncached = 0;
	authnokey = 0;
}


/*
 * auth_findkey - find a key in the hash table
 */
struct savekey *
auth_findkey(keyno)
	u_long keyno;
{
	register struct savekey *sk;

	sk = key_hash[KEYHASH(keyno)];
	while (sk != 0) {
		if (keyno == sk->keyid)
			return sk;
		sk = sk->next;
	}
	return 0;
}


/*
 * auth_havekey - return whether a key is known
 */
int
auth_havekey(keyno)
	u_long keyno;
{
	register struct savekey *sk;

	if (keyno == 0 || keyno == cache_keyid)
		return 1;

	sk = key_hash[KEYHASH(keyno)];
	while (sk != 0) {
		if (keyno == sk->keyid) {
			if (sk->flags & KEY_KNOWN)
				return 1;
			else {
				authkeynotfound++;
				return 0;
			}
		}
		sk = sk->next;
	}
	authkeynotfound++;
	return 0;
}


/*
 * authhavekey - return whether a key is known.  Permute and cache
 *		 the key as a side effect.
 */
int
authhavekey(keyno)
	u_long keyno;
{
	register struct savekey *sk;

	authkeylookups++;
	if (keyno == 0 || keyno == cache_keyid)
		return 1;

	sk = key_hash[KEYHASH(keyno)];
	while (sk != 0) {
		if (keyno == sk->keyid)
			break;
		sk = sk->next;
	}

	if (sk == 0 || !(sk->flags & KEY_KNOWN)) {
		authkeynotfound++;
		return 0;
	}
	
	cache_keyid = sk->keyid;
	cache_flags = sk->flags;
	auth_subkeys(sk->key, cache_ekeys, cache_dkeys);
	return 1;
}


/*
 * auth_moremem - get some more free key structures
 */
int
auth_moremem()
{
	register struct savekey *sk;
	register int i;
	extern char *malloc();

	sk = (struct savekey *)malloc(MEMINC * sizeof(struct savekey));
	if (sk == 0)
		return 0;
	
	for (i = MEMINC; i > 0; i--) {
		sk->next = authfreekeys;
		authfreekeys = sk++;
	}
	authnumfreekeys += MEMINC;
	return authnumfreekeys;
}


/*
 * authtrust - declare a key to be trusted/untrusted
 */
void
authtrust(keyno, trust)
	u_long keyno;
	int trust;
{
	register struct savekey *sk;

	sk = key_hash[KEYHASH(keyno)];
	while (sk != 0) {
		if (keyno == sk->keyid)
			break;
		sk = sk->next;
	}

	if (sk == 0 && !trust)
		return;
	
	if (sk != 0) {
		if (cache_keyid == keyno)
			cache_keyid = 0;

		if (trust) {
			sk->flags |= KEY_TRUSTED;
			return;
		}

		sk->flags &= ~KEY_TRUSTED;
		if (!(sk->flags & KEY_KNOWN)) {
			register struct savekey *skp;

			skp = key_hash[KEYHASH(keyno)];
			if (skp == sk) {
				key_hash[KEYHASH(keyno)] = sk->next;
			} else {
				while (skp->next != sk)
					skp = skp->next;
				skp->next = sk->next;
			}
			authnumkeys--;

			sk->next = authfreekeys;
			authfreekeys = sk;
			authnumfreekeys++;
		}
		return;
	}

	if (authnumfreekeys == 0)
		if (auth_moremem() == 0)
			return;

	sk = authfreekeys;
	authfreekeys = sk->next;
	authnumfreekeys--;

	sk->keyid = keyno;
	sk->flags = KEY_TRUSTED;
	sk->next = key_hash[KEYHASH(keyno)];
	key_hash[KEYHASH(keyno)] = sk;
	authnumkeys++;
	return;
}


/*
 * authistrusted - determine whether a key is trusted
 */
int
authistrusted(keyno)
	u_long keyno;
{
	register struct savekey *sk;

	if (keyno == cache_keyid)
		return ((cache_flags & KEY_TRUSTED) != 0);
	authkeyuncached++;

	sk = key_hash[KEYHASH(keyno)];
	while (sk != 0) {
		if (keyno == sk->keyid)
			break;
		sk = sk->next;
	}

	if (sk == 0 || !(sk->flags & KEY_TRUSTED))
		return 0;
	return 1;
}



/*
 * auth_setkey - set a key into the key array
 */
void
auth_setkey(keyno, key)
	u_long keyno;
	u_long *key;
{
	register struct savekey *sk;

	/*
	 * See if we already have the key.  If so just stick in the
	 * new value.
	 */
	sk = key_hash[KEYHASH(keyno)];
	while (sk != 0) {
		if (keyno == sk->keyid) {
			sk->key[0] = key[0];
			sk->key[1] = key[1];
			sk->flags |= KEY_KNOWN;
			if (cache_keyid == keyno)
				cache_keyid = 0;
			return;
		}
		sk = sk->next;
	}

	/*
	 * Need to allocate new structure.  Do it.
	 */
	if (authnumfreekeys == 0) {
		if (auth_moremem() == 0)
			return;
	}

	sk = authfreekeys;
	authfreekeys = sk->next;
	authnumfreekeys--;

	sk->key[0] = key[0];
	sk->key[1] = key[1];
	sk->keyid = keyno;
	sk->flags = KEY_KNOWN;
	sk->next = key_hash[KEYHASH(keyno)];
	key_hash[KEYHASH(keyno)] = sk;
	authnumkeys++;
	return;
}


/*
 * auth_delkeys - delete all known keys, in preparation for rereading
 *		  the keys file (presumably)
 */
void
auth_delkeys()
{
	register struct savekey *sk;
	register struct savekey **skp;
	register int i;

	for (i = 0; i < HASHSIZE; i++) {
		skp = &(key_hash[i]);
		sk = key_hash[i];
		while (sk != 0) {
			sk->flags &= ~KEY_KNOWN;
			if (sk->flags == 0) {
				*skp = sk->next;
				authnumkeys--;
				sk->next = authfreekeys;
				authfreekeys = sk;
				authnumfreekeys++;
				sk = *skp;
			} else {
				skp = &(sk->next);
				sk = sk->next;
			}
		}
	}
}
