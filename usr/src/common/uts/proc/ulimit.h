/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_ULIMIT_H	/* wrapper symbol for kernel use */
#define _PROC_ULIMIT_H	/* subject to change without notice */

#ident	"@(#)kern:proc/ulimit.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 * THIS FILE CONTAINS CODE WHICH IS DESIGNED TO BE
 * PORTABLE BETWEEN DIFFERENT MACHINE ARCHITECTURES
 * AND CONFIGURATIONS. IT SHOULD NOT REQUIRE ANY
 * MODIFICATIONS WHEN ADAPTING XENIX TO NEW HARDWARE.
 */

/*
 * The following are codes which can be
 * passed to the ulimit system call. (Xenix compatible.)
 */

#if !defined(_XOPEN_SOURCE)
#define UL_GFILLIM	1	/* get file limit */
#define UL_SFILLIM	2	/* set file limit */
#define UL_GMEMLIM	3	/* get process size limit */
#define UL_GDESLIM	4	/* get file descriptor limit */
#define UL_GTXTOFF	64	/* get text offset */
#endif /* !defined(_XOPEN_SOURCE) */

/*
 * The following are symbolic constants required for
 * X/Open Conformance.   They are the equivalents of
 * the constants above.
 */

#define UL_GETFSIZE	1	/* get file limit */
#define UL_SETFSIZE	2	/* set file limit */


#if defined(__cplusplus)
	}
#endif

#endif /* _PROC_ULIMIT_H */
