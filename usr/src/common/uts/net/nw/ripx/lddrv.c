/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/lddrv.c	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: lddrv.c,v 1.8 1994/05/02 17:58:43 vtag Exp $"
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
#include <net/nw/ripx/rip.h>
#include <io/conf.h>
#include <util/mod/moddefs.h>
#include <net/nw/ipx_app.h>
#include <net/nw/ripx_app.h>
#include <net/nw/ripx/ripx_streams.h>
#else
#include "rip.h"
#include <sys/conf.h>
#include <sys/moddefs.h>
#include "sys/ipx_app.h"
#include "sys/ripx_app.h"
#include "ripx_streams.h"
#endif /* _KERNEL_HEADERS */

/*
 * Header, wrapper, and function declarations and definitions for
 * loadable driver.
 */
int ripx_load();

#define DRVNAME "Novell's IPX RIP Router (RIPX)"

int ripxdevflag = D_NEW | D_MP;

MOD_DRV_WRAPPER(ripx, ripx_load, NULL, NULL, DRVNAME);

int
ripx_load(void)
{

	NTR_ENTER( 0, 0, 0, 0, 0, 0);
	ripxinit();
	return( NTR_LEAVE(0));
}
