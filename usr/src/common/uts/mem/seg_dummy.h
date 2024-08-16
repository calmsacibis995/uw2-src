/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_DUMMY_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_DUMMY_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg_dummy.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

struct seg;
extern int segdummy_create(struct seg *, void *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SEG_DUMMY_H */
