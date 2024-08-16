/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_MAC_HIER_H    /* wrapper symbol for kernel use */
#define _ACC_MAC_HIER_H    /* subject to change without notice */

#ident	"@(#)kern:acc/mac/mac_hier.h	1.2"
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
 * pertaining to spin locks controlling the mac levels cache.
 *
 */

#define MAC_LIST_HIER	MAC_HIER_BASE
#define MAC_ENTRY_HIER	MAC_HIER_BASE+1
#define	PL_LVLS_CACHE	PLHI

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _ACC_MAC_HIER_H */
