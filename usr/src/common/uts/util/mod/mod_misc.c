/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/mod_misc.c	1.6"
#ident	"$Header: $"
#include <util/mod/mod.h>
#include <util/mod/mod_k.h>
#include <util/mod/moddefs.h>

STATIC int mod_miscdonothing();
STATIC int mod_miscinfo(struct mod_type_data *p, int *p0, int *p1, int *type);
STATIC int mod_miscbind(struct mod_type_data *, int *);

struct	mod_operations	mod_misc_ops	= {
	mod_miscdonothing,
	mod_miscdonothing,
	mod_miscinfo,
	mod_miscbind
};

/*
 * int mod_misc_reg(void * arg)
 *	Register loadable module for misc module type
 *
 * Calling/Exit State:
 *	Always return 0.
 *	Provided for consistency with other module types.
 */
/* ARGSUSED */
int
mod_misc_reg(void * arg)
{
	return(0); /* nothing to register */
}


/*
 * STATIC int mod_miscdonothing()
 *	Used as the install and remove mod_operations routines
 *	for MISC type.
 *
 * Calling/Exit State:
 *	Always return 0.
 */
STATIC int
mod_miscdonothing()
{
	return(0);
}

/*
 * STATIC int mod_miscinfo(struct mod_type_data *p, int *p0, int *p1, int *type)
 *	Get the module type specific information.
 *
 * Calling/Exit State:
 *	Set the module type for MISC type, always return 0.
 */
/* ARGSUSED */
STATIC int 
mod_miscinfo(struct mod_type_data *p, int *p0, int *p1, int *type)
{
	*type = MOD_TY_MISC;
	return(0);
}

/*
 * STATIC int mod_miscbind(struct mod_type_data *miscdata, int *cpup)
 *	Routine to handle cpu binding for non-MP modules.
 *
 * Calling/Exit State:
 *	Always return 0 and doesn't change the value of cpu.
 */
/* ARGSUSED */
STATIC int
mod_miscbind(struct mod_type_data *miscdata, int *cpup)
{
	if (*cpup == -1)
		*cpup = 0;
	return (0);
}
