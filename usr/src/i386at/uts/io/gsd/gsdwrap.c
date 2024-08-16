/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/gsd/gsdwrap.c	1.1"
#ident	"$Header: $"

/*
 * gsdwrap.c, Graphics System Driver Module wrappers.
 */

#include <io/ws/ws.h>
#include <io/gsd/gsd.h>
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/mod/moddefs.h>

#define	DRVNAME	"gsd - Graphics System Driver Module"

int	gsd_load(void);
int	gsd_unload(void);

MOD_MISC_WRAPPER(gsd, gsd_load, gsd_unload, DRVNAME);

/*
 * gsd_load(void)
 * Load and register the driver on demand.
 *
 * Calling/Exit status:
 *
 */
int
gsd_load(void)
{
	gsdinit();

	return(0);
}

/*
 * gsd_unload(void)
 *	Unload and unregister the driver.
 *
 * Calling/Exit status:
 *
 */
int
gsd_unload(void)
{
	if (channel_ref_count == 0) {
		gs_init_flg = 0;
		return (0);
	}

	return (EBUSY);
}

