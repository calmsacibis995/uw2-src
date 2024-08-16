/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libMDtI:fileclass.c	1.5"
#endif

#include <X11/Intrinsic.h>
#include "DesktopP.h"

/*
 * This file contains routines to maintain an array of file class structures.
 */


/*
 * Allocate an entry for a new class.
 */
DmFclassPtr
DmNewFileClass(key)
void *key;	/* ptr to FmodeKey or FnameKey */
{
	DmFclassPtr fcp;

	if (fcp = (DmFclassPtr)calloc(1, sizeof(DmFclassRec)))
		fcp->key  = key;
	return(fcp);
}

