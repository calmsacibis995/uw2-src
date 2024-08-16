/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_DEBUG_H	/* wrapper symbol for kernel use */
#define _UTIL_DEBUG_H	/* subject to change without notice */

#ident	"@(#)kern:util/debug.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

/*
 * Definitions to assist in debugging the kernel.
 */

#ifdef DEBUG

#define ASSERT(EX) ((void)((EX) || assfail(#EX, __FILE__, __LINE__)))

#else

#define ASSERT(x) ((void)0)

#endif	/* DEBUG */

#ifndef STATIC
#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif	/* DEBUG */
#endif	/* STATIC */

extern int assfail(const char *, const char *, int);

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * Data structure for help table for debug_help() debug tool function.
 */
struct dbgtool_info {
	/* These fields are initialized in the table: */
	const char * const dti_funcname; /* tool function name */
	const char * const dti_args;	/* function arguments, if any;
					   if none, set to NULL */
	const char * const dti_desc;	/* one-liner description of function */
	const char * const dti_details;	/* optional detailed description;
					   may be multi-line (use "\n");
					   if none, set to NULL */
	/* The remaining fields are used internally: */
	struct dbgtool_info *dti_next;
};

#endif /* DEBUG || DEBUG_TOOLS */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_DEBUG_H */
