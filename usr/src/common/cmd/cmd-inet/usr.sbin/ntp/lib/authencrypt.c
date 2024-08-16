/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/authencrypt.c	1.2"
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
 * authencrypt - compute and encrypt the mac field in an NTP packet
 */
#include <sys/types.h>

/*
 * For our purposes an NTP packet looks like:
 *
 *	a variable amount of encrypted data, multiple of 8 bytes, followed by:
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
 * Stat counters from the database module
 */
extern u_long authencryptions;
extern u_long authkeyuncached;
extern u_long authnokey;

void
authencrypt(keyno, pkt, length)
	u_long keyno;
	u_long *pkt;
	int length;	/* length of encrypted portion of packet */
{
	register u_long *pd;
	register int i;
	register u_char *keys;
	register int len;
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
	 * Do the encryption.  Work our way forward in the packet, eight
	 * bytes at a time, encrypting as we go.  Note that the byte order
	 * issues are handled by the DES routine itself
	 */
	pd = pkt;
	work[0] = work[1] = 0;
	len = length / sizeof(u_long);

	for (i = (len/2); i > 0; i--) {
		work[0] ^= *pd++;
		work[1] ^= *pd++;
		auth_des(work, keys);
	}

	if (len & 0x1) {
		work[0] ^= *pd++;
		auth_des(work, keys);
	}

	/*
	 * Space past the keyid and stick the result back in the mac field
	 */
	pd += NOCRYPT_LONGS;
	*pd++ = work[0];
	*pd = work[1];
}
