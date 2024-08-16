/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/drvload/lddrv.c	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: lddrv.c,v 1.5 1994/05/05 15:38:05 vtag Exp $"
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
#include <net/nw/nwcommon.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <io/conf.h>
#include <util/mod/moddefs.h>
#else
#include "nwcommon.h"
#include <sys/conf.h>
#include <sys/moddefs.h>
#endif /* _KERNEL_HEADERS */

/*
 * Header, wrapper, and function declarations and definitions for
 * loadable driver.
 */
int ipxfini(void);
int ipxinit(void);

int ipx_load(void);
int ipx_unload(void);

#define DRVNAME "Novell's IPX Socket Mux/Lan Router (IPX)"

int ipxdevflag = D_NEW | D_MP;

MOD_DRV_WRAPPER(ipx, ipx_load, ipx_unload, NULL, DRVNAME);

/*
 * int ipx_load(void)
 *	Ipx load wrapper
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int
ipx_load(void)
{
	return(ipxinit());
}

/*
 * int ipx_unload(void)
 *	Ipx unload wrapper
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int
ipx_unload(void)
{
	return(ipxfini());
}
