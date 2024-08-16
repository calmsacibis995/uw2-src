/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/compat/v86.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This header file is obsolete.  It should not be included by any program
 * or module.
 *
 * NOTICE: This file will be removed in a subsequent release.
 *
 */

#ifdef _KERNEL

#ifndef _OBSOLETE_IN_UW2_0
#define _OBSOLETE_IN_UW2_0

static int _Obsolete_in_UW2_0()
{
	extern int _Header_File_Obsolete_in_UW2_0;

	return _Header_File_Obsolete_in_UW2_0;
}

#endif /* _OBSOLETE_IN_UW2_0 */

/*
 * Some old drivers include v86_t pointers.
 * This lets them compile.
 */
typedef void v86_t;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif
