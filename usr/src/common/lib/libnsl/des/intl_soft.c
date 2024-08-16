/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/des/intl_soft.c	1.1.8.2"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * Warning!  Things are arranged very carefully in this file to
 * allow read-only data to be moved to the text segment.  The
 * various DES tables must appear before any function definitions
 * (this is arranged by including them immediately below) and partab
 * must also appear before any function definitions
 * This arrangement allows all data up through the first text to
 * be moved to text.
 */

#include <sys/types.h>
#include <des/softdes.h>

/* remove des/desdata.h in international version to conserve space */
/* #include "des/desdata.h" */

#include <des/des.h>

void
des_setparity(p)
	char *p;
{
}

__des_crypt(buf, len, desp)
	register char *buf;
	register unsigned len;
	struct desparams *desp;
{
	return (1);
}


static
des_setkey(userkey, kd, dir)
	u_char userkey[8];	
	register struct deskeydata *kd;
	unsigned dir;
{
}


static
des_encrypt(data, kd)
	register u_char *data;
	register struct deskeydata *kd;
{
}
