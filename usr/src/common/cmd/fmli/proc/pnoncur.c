/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:proc/pnoncur.c	1.5.3.3"

#include <stdio.h>
#include "inc.types.h"		/* abs s14 */
#include "wish.h"
#include "terror.h"
#include "proc.h"
#include "procdefs.h"

extern struct proc_ref PR_all;

int
proc_noncurrent(p, all)
proc_id p;
bool all;
{
	/* suspend process */
#ifdef _DEBUG
	_debug(stderr, "proc_noncurrent not yet implemented\n");
#endif
}
