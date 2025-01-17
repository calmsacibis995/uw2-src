/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_AIO_AIO_HIER_H
#define _IO_AIO_AIO_HIER_H

#ident	"@(#)kern-i386at:io/async/aio_hier.h	1.3"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Like those of most other drivers, the aio driver locks start at low
 * hierarchy values.  By convention, the per-process p_aioprocp pointer is
 * covered by the address space lock.  The only data structures needing locks
 * are the per-process aio_proc structure (which is kmem_alloc-ed) and the
 * free list of per-job.  structures.  The first lock protects the internals of
 * the structure, including its list of aio structures.  The two locks could
 * probably do without an ordering relationship, because it appears unnecessary
 * to hold both concurrently.
 */
#define	AIO_HIER_BASE			5
#define AIO_HIER_PROC			(AIO_HIER_BASE + 5)
#define	AIO_HIER_AIO_FREE_LIST		(AIO_HIER_PROC + 5)

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_AIO_AIO_HIER_H */
