/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:tstansi.c	1.1"

/* test if we have an ANSI compiler */
#if !defined(__STDC__)
	/* include a file that does not exist so that
	 * a non-zero value is returned from the compiler */
#include "TESTANSI"
#endif
