/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/siutils.c	1.4"

#include "simskbits.h"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */



/*
 * unsigned long
 * si_pfill(val)	-- replicate the value passed through the
 *			word as many time as is required by PSZ;
 *
 * Input:
 *	unsigned long	val	-- value to be replicated.
 */
unsigned long
si_pfill(val)
  register unsigned long val;
{
    unsigned long ret;
    extern unsigned long sipfillmask1[];
    extern unsigned long sipfillmask2[];
    extern unsigned long sipfillmask4[];

    if (PSZ == 32)
      return(val);

    val &= (1 << PSZ) - 1;

    if (PSZ == 1) {
	ret=sipfillmask1[val];
    } else if (PSZ == 2) {
	ret=sipfillmask2[val];
    } else if (PSZ <= 4) {
	ret=sipfillmask4[val];
    } else if (PSZ <= 8) {
	ret=val | val << 8 | val << 16 | val << 24;
    } else if (PSZ <= 16) {
	ret=val | val << 16;
    } else if (PSZ <= 32) {
	ret=val;
    }

    return(ret); /* should not get here */
}
