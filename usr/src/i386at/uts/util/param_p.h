/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_PARAM_P_H	/* wrapper symbol for kernel use */
#define _UTIL_PARAM_P_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/param_p.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define	CANBSIZ	256		/* max size of typewriter line	*/
#define	HZ	100		/* 100 ticks/second of the clock */
#define TICK    10000000	/* nanoseconds per tick */

#ifndef UNIPROC
#define	MAXNUMCPU	32	/* max supported # processors;
				 * most of kernel is independent of this */
#else
#define	MAXNUMCPU	1
#endif /* UNIPROC */

#ifdef _KERNEL
/*
 * Special support for 4.2 binary HBA drivers:
 *
 * 	Because these drivers statically allocate command blocks, they must be
 * 	loaded into non-DMAable memory.
 */
#ifndef NO_RDMA
extern boolean_t phystokvmem;
#define PHYSMEM_DRV_DMA()		phystokvmem
#else /* !NO_RDMA */
#define PHYSMEM_DRV_DMA()		0
#endif /* NO_RDMA */
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_PARAM_P_H */
