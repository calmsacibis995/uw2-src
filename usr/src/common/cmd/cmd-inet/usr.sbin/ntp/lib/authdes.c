/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/authdes.c	1.2"
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
 * authdes.c - an implementation of the DES cipher algorithm for NTP
 *
 *				NOT!
 *
 *	See the release notes for more information.
 */
#include <sys/types.h>

/*
 * There are two entries in here.  auth_subkeys() called to
 * compute the encryption and decryption key schedules, while
 * auth_des() is called to do the actual encryption/decryption
 */

/*
 * Permute the key to give us our key schedule.
 */
void
auth_subkeys(key, encryptkeys, decryptkeys)
	u_long *key;
	u_char *encryptkeys;
	u_char *decryptkeys;
{
	return;
}

/*
 * auth_des - perform an in place DES encryption on 64 bits
 *
 * Note that the `data' argument is always in big-end-first
 * byte order, i.e. *(char *)data is the high order byte of
 * the 8 byte data word.  We modify the initial and final
 * permutation computations for little-end-first machines to
 * swap bytes into the natural host order at the beginning and
 * back to big-end order at the end.  This is unclean but avoids
 * a byte swapping performance penalty on Vaxes (which are slow already).
 */
void
auth_des(data, subkeys)
	u_long *data;
	u_char *subkeys;
{
	return;
}
