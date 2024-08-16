/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NAMEFS_NAMEHIER_H	/* wrapper symbol for kernel use */
#define _FS_NAMEFS_NAMEHIER_H	/* subject to change without notice */

#ident	"@(#)kern:fs/namefs/namehier.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>	/* REQUIRED */
#include <util/ghier.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the namefs file system. 
 */

#ifdef _KERNEL

#define NM_HIER_BASE	STR_HIER_BASE - 7

/*
 * The hierarchy values will be checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 *
 */
#define NM_HIER		NM_HIER_BASE

#define	PLNM		PLSTR	

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_NAMEFS_NAMEHIER_H */
