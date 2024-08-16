/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_VN_F_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_VN_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/seg_vn_f.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Tuning options and constants for seg_vn.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Allow segs to expand in place. Really an optimization for stack growth.
 * See segvn_concat for more comment.
 */
#define ANON_SLOP	(16 * PAGESIZE)

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SEG_VN_F_H */
