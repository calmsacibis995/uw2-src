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

#ident	"@(#)kern-i386at:io/fnt/fntwrap.c	1.1"
#ident	"$Header: $"

/*
 * fntwrap.c, Memory Resident Font Module
 */
#include <io/fnt/fnt.h>
#include <io/gsd/gsd.h>
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/mod/moddefs.h>

#define	DRVNAME	"fnt - Memory Resident Font Module"

int	fnt_load(void);
int	fnt_unload(void);

MOD_MISC_WRAPPER(fnt, fnt_load, fnt_unload, DRVNAME);

/*
 * fnt_load(void)
 * Load and register the driver on demand.
 *
 * Calling/Exit status:
 *
 */
int
fnt_load(void)
{
	fntinit();

	return(0);
}

/*
 * fnt_unload(void)
 *	Unload and unregister the driver.
 *
 * Calling/Exit status:
 *
 */
int
fnt_unload(void)
{
	if (channel_ref_count == 0) {
		fnt_init_flg = 0;
		return (0);
	}
	return (EBUSY);
}

