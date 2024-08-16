/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_LOCKING_H	/* wrapper symbol for kernel use */
#define _SVC_LOCKING_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/locking.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 *   Flag values for XENIX locking() system call 
 */

#define LK_UNLCK  0	/* unlock request */
#define LK_LOCK   1	/* lock request */
#define LK_NBLCK  20	/* non-blocking lock request */
#define LK_RLCK   3	/* read permitted only lock request */
#define LK_NBRLCK 4	/* non-blocking read only lock request */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_LOCKING_H */
