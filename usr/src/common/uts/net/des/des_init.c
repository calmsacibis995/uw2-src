/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/des/des_init.c	1.2"
#ident	"$Header: $"

/*
 *	des_init.c, des module initialization.
 */

#include <util/mod/moddefs.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/cmn_err.h>

int	des_load(void);
int	des_unload(void);

MOD_MISC_WRAPPER(des, des_load, des_unload, "Data Encryption Standard");

/*
 * des_load(void)
 *      Dynamically load DES.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description:
 *      Dynamically load DES.
 *
 * Parameters:
 *
 */
int
des_load(void)
{
        /*
         * nothing to do.
         */
        return(0);
}

/*
 * des_unload(void)
 *      Dynamically unload DES.
 *
 * Calling/Exit State:
 *      No locking assumptions.
 *
 * Description:
 *      Dynamically unload DES.
 *
 * Parameters:
 *
 */
int
des_unload(void)
{
        /*
         * nothing to do.
         */
        return(0);
}
