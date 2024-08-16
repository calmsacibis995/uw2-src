/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/lddrv.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: lddrv.c,v 1.3 1994/05/05 17:43:19 meb Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#ifdef _KERNEL_HEADERS
#include <net/nw/spx/spx2.h>
#include <util/mod/moddefs.h>
#else
#include "spx2.h"
#include <sys/moddefs.h>
#endif

/*
 * Header, wrapper, and function declarations and definitions for
 * loadable driver.
 */
int nspx_load();

extern	void	nspxinit();

#define DRVNAME "Novell's SPXII Loadable Netware Driver"

int nspxdevflag = D_NEW;

MOD_DRV_WRAPPER(nspx, nspx_load, NULL, NULL, DRVNAME);

/*
 * int spx_load(void)
 *	Spx load wrapper
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int
nspx_load(void)
{

	NTR_ENTER( 0, 0, 0, 0, 0, 0);
	nspxinit();
	return( NTR_LEAVE(0));
}
