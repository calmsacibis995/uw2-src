/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/authparity.c	1.2"
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
 * auth_parity - set parity on a key/check for odd parity
 */
#include <sys/types.h>

int
auth_parity(key)
	u_long *key;
{
	u_long mask;
	int parity_err;
	int bitcount;
	int half;
	int byte;
	int i;

	/*
	 * Go through counting bits in each byte.  Check to see if
	 * each parity bit was set correctly.  If not, note the error
	 * and set it right.
	 */
	parity_err = 0;
	for (half = 0; half < 2; half++) {		/* two halves of key */
		mask = 0x80000000;
		for (byte = 0; byte < 4; byte++) {	/* 4 bytes per half */
			bitcount = 0;
			for (i = 0; i < 7; i++) {	/* 7 data bits / byte */
				if (key[half] & mask)
					bitcount++;
				mask >>= 1;
			}

			/*
			 * If bitcount is even, parity must be set.  If
			 * bitcount is odd, parity must be clear.
			 */
			if ((bitcount & 0x1) == 0) {
				if (!(key[half] & mask)) {
					parity_err++;
					key[half] |= mask;
				}
			} else {
				if (key[half] & mask) {
					parity_err++;
					key[half] &= ~mask;
				}
			}
			mask >>= 1;
		}
	}

	/*
	 * Return the result of the parity check.
	 */
	return (parity_err == 0);
}


