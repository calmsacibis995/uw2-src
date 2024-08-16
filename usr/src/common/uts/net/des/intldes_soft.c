/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/des/intldes_soft.c	1.5"
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
 *	intldes_soft.c, international version of software
 *	implementation of DES
 *
 *	Warning!  Things are arranged very carefully in this file to
 *	allow read-only data to be moved to the text segment.  The
 *	various DES tables must appear before any function definitions
 *	(this is arranged by including them immediately below) and partab
 *	must also appear before and function definitions
 *	This arrangement allows all data up through the first text to
 *	be moved to text.
 *
 *	This Won't work without 8 bit chars and 32 bit longs
 */

#include <util/types.h>
#include <net/des/des.h>
#include <net/des/softdes.h>
#include <util/debug.h>

#define	btst(k, b)	(k[b >> 3] & (0x80 >> (b & 07)))
#define	BIT28		(1<<28)

/*
 * __des_Crypt(buf, len, desp)
 *	Encrypt or decrypt a block of data.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 1.
 *
 * Description:
 *	Software encrypt or decrypt a block of data
 *	(multiple of 8 bytes). Do the CBC ourselves if needed.
 *
 *	This international version does nothing.
 *
 * Parameters:
 *
 *	buf			# data to process
 *	len			# length of data
 *	desp			# des specific parameters
 *	
 */
/* ARGSUSED */
int
__des_crypt(char *buf, unsigned len, struct desparams *desp)
{
	return(0);
}

/*
 * des_setkey(userkey, kd, dir)
 *	Set the key and direction for an encryption operation.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	Set the key and direction for an encryption operation.
 *	We build the 16 key entries here.
 *
 *	This international version does nothing.
 *
 * Parameters:
 *
 *	userkey			# user supplied key
 *	kd			# data for the key
 *	dir			# encrypt or decrypt
 *	
 */
/* ARGSUSED */
void
des_setkey(u_char userkey, struct deskeydata *kd, unsigned dir)
{
}

/*
 * des_encrypt(data, kd)
 *	Do an encryption operation.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine does an encryption operation.
 *	Much pain is taken (with preprocessor) to
 *	avoid loops so the compiler can do address
 *	arithmetic instead of doing it at runtime.
 *	Note that the byte-to-chunk conversion is
 *	necessary to guarantee processor byte-order
 *	independence.
 *
 *	This international version does nothing.
 *
 * Parameters:
 *
 *	None.
 *	
 */
/* ARGSUSED */
void
des_encrypt(u_char *data, struct deskeydata *kd)
{
}
