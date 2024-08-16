/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/authdecrypt.c	1.2"
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
 * authdecrypt - routine to decrypt a packet to see if this guy knows our key.
 */
#include <sys/types.h>

/*
 * For our purposes an NTP packet looks like:
 *
 *	a variable amount of unencrypted data, multiple of 8 bytes, followed by:
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
extern u_char cache_dkeys[];	/* cached decryption keys */
extern u_char zerodkeys[];	/* zero key decryption keys */

extern int authhavekey();

/*
 * Stat counters, imported from data base module
 */
extern u_long authdecryptions;
extern u_long authkeyuncached;
extern u_long authdecryptok;

int
authdecrypt(keyno, pkt, length)
	u_long keyno;
	u_long *pkt;
	int length;	/* length of variable data in octets */
{
	register u_long *pd;
	register int i;
	register u_char *keys;
	register int longlen;
	u_long work[2];

	authdecryptions++;
	
	if (keyno == 0)
		keys = zerodkeys;
	else {
		if (keyno != cache_keyid) {
			authkeyuncached++;
			if (!authhavekey(keyno))
				return 0;
		}
		keys = cache_dkeys;
	}

	/*
	 * Get encryption block data in host byte order and decrypt it.
	 */
	longlen = length / sizeof(u_long);
	pd = pkt + longlen;		/* points at NOCRYPT area */
	work[0] = *(pd + NOCRYPT_LONGS);
	work[1] = *(pd + NOCRYPT_LONGS + 1);

	if (longlen & 0x1) {
		auth_des(work, keys);
		work[0] ^= *(--pd);
	}

	for (i = longlen/2; i > 0; i--) {
		auth_des(work, keys);
		work[1] ^= *(--pd);
		work[0] ^= *(--pd);
	}

	/*
	 * Success if the encryption data is zero
	 */
	if ((work[0] == 0) && (work[1] == 0)) {
		authdecryptok++;
		return 1;
	}
	return 0;
}
