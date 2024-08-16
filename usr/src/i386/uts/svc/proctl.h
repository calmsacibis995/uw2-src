/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_PROCTL_H	/* wrapper symbol for kernel use */
#define _SVC_PROCTL_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/proctl.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * XENIX proctl() requests. Both of these functions are currently
 * implemented as no-ops.
 */

#define PRHUGEX		1	/* allow process > swapper size to execute */
#define PRNORMEX 	2	/* remove PRHUGEX permission */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_PROCTL_H */
