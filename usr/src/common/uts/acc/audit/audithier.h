/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_AUDIT_HIER_H    /* wrapper symbol for kernel use */
#define _ACC_AUDIT_HIER_H    /* subject to change without notice */

#ident	"@(#)kern:acc/audit/audithier.h	1.4"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ghier.h>		/* REQUIRED */
#include <util/ipl.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the audit subsystem. 
 *
 */


/*
 * Hierarchy values used by the file system subsystem:
 *
 *	PLMIN:	ADT_HIER_BASE  to ADT_HIER_BASE  + 2
 */

/*
 * The hierarchy values will be checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 *
 */

#define ADT_BUFHIER	ADT_HIER_BASE
#define ADT_LVLHIER	ADT_HIER_BASE
#define	PLAUDIT		PLMIN	

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _ACC_AUDIT_HIER_H */
