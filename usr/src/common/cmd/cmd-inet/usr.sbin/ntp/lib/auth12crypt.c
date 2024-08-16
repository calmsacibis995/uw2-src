/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/auth12crypt.c	1.2"
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
 * auth12crypt.c - routines to support two stage NTP encryption
 */
#include <sys/types.h>

/*
 * For our purposes an NTP packet looks like:
 *
 *	a variable amount of encrypted data, multiple of 8 bytes, which
 *		is encrypted in pass 1, followed by:
 *	an 8 byte chunk of data which is encrypted in pass 2
 *	NOCRYPT_OCTETS worth of unencrypted data, followed by:
 *	BLOCK_OCTETS worth of ciphered checksum.
 */ 
#define	NOCRYPT_OCTETS	4
#define	BLOCK_OCTETS	8

#define	NOCRYPT_LONGS	((NOCRYPT_OCTETS)/sizeof(u_long))
#define	BLOCK_LONGS	((BLOCK_OCTETS)/sizeof(u_long))

/*
 * Imported from the key data base module
 */
extern u_long cache_keyid;	/* cached key ID */
extern u_char cache_ekeys[];	/* cached decryption keys */
extern u_char zeroekeys[];	/* zero key decryption keys */

extern int authhavekey();

/*
 * Stat counters, from the database module
 */
extern u_long authencryptions;
extern u_long authkeyuncached;
extern u_long authnokey;


/*
 * auth1crypt - do the first stage of a two stage encryption
 */
void
auth1crypt(keyno, pkt, length)
	u_long keyno;
	u_long *pkt;
	int length;	/* length of all encrypted data */
{
	register u_long *pd;
	register int i;
	register u_char *keys;
	u_long work[2];
	void auth_des();


	authencryptions++;

	if (keyno == 0) {
		keys = zeroekeys;
	} else {
		if (keyno != cache_keyid) {
			authkeyuncached++;
			if (!authhavekey(keyno)) {
				authnokey++;
				return;
			}
		}
		keys = cache_ekeys;
	}

	/*
	 * Do the first five encryptions.  Stick the intermediate result
	 * in the mac field.  The sixth encryption must wait until the
	 * caller freezes a transmit time stamp, and will be done in stage 2.
	 */
	pd = pkt;
	work[0] = work[1] = 0;

	for (i = (length/BLOCK_OCTETS - 1); i > 0; i--) {
		work[0] ^= *pd++;
		work[1] ^= *pd++;
		auth_des(work, keys);
	}

	/*
	 * Space to the end of the packet and stick the intermediate
	 * result in the mac field.
	 */
	pd += BLOCK_LONGS + NOCRYPT_LONGS;
	*pd++ = work[0];
	*pd = work[1];
}


/*
 * auth2crypt - do the second stage of a two stage encryption
 */
void
auth2crypt(keyno, pkt, length)
	u_long keyno;
	u_long *pkt;
	int length;	/* total length of encrypted area */
{
	register u_long *pd;
	register u_char *keys;
	void auth_des();

	/*
	 * Skip the key check.  The call to the first stage should
	 * have got it.
	 */
	if (keyno == 0)
		keys = zeroekeys;
	else
		keys = cache_ekeys;

	/*
	 * The mac currently should hold the results of the first `n'
	 * encryptions.  We xor in the last block in data section and
	 * do the final encryption in place.
	 *
	 * Get a pointer to the MAC block.  XOR in the last two words of
	 * the data area. Call the encryption routine.
	 */
	pd = pkt + (length/sizeof(u_long)) + NOCRYPT_LONGS;

	*pd ^= *(pd - NOCRYPT_LONGS - 2);
	*(pd + 1) ^= *(pd - NOCRYPT_LONGS - 1);
	auth_des(pd, keys);
}
