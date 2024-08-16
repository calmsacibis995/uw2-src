/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_FIFOFS_FIFOHIER_H	/* wrapper symbol for kernel use */
#define _FS_FIFOFS_FIFOHIER_H	/* subject to change without notice */

#ident	"@(#)kern:fs/fifofs/fifohier.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>	/* REQUIRED */
#include <util/ghier.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/*
 * This header file has all the hierarchy and minipl information 
 * pertaining to the fifofs file system. Note that all lock hierarchies in 
 * this file will be expressed as an offset from a base hierarchy value that
 * will be associated with the fifofs file system. 
 */

#define FIFO_HIER_BASE	STR_HIER_BASE - 4

/*
 * The hierarchy values will be checked amongst locks that have identical
 * minipl and hence the hierarchy namespace can be shared among locks that
 * have different minipls.
 *
 */
#define FIFO_HIER	FIFO_HIER_BASE

#define	PLFIFO		PLSTR	

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_FIFOFS_FIFOHIER_H */
