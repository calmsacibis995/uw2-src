/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:util/mod/mod_intr.c	1.1"
#ident "$Header: $"

#include	<util/mod/moddrv.h>
#include	<util/types.h>

/*
 * int mod_drvattach(struct mod_drvintr *aip)
 * 	Install and enable all interrupts required by a
 * 	given device driver.
 *
 * Calling/Exit State:
 *	Called from the _load routine of loadable modules.
 *	No locks held upon calling and exit.
 *
 *	A dummy routine for i386sym platform, until we have
 *	fully DLM suport.
 */
/* ARGSUSED */
void
mod_drvattach(struct mod_drvintr *aip)
{
	return;
}

/*
 * int mod_drvdetach(struct mod_drvintr *aip)
 *	 Remove and disable all interrupts used by a given device driver.
 *
 * Calling/Exit State:
 *	Called from the _unload routine of loadable modules.
 *	No locks held upon calling and exit.
 *
 *	A dummy routine for i386sym platform, until we have
 *	fully DLM suport.
 */
/* ARGSUSED */
void
mod_drvdetach(struct mod_drvintr *aip)
{
	return;
}
