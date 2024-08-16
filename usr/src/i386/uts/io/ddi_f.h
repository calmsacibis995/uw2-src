/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DDI_F_H	/* wrapper symbol for kernel use */
#define _IO_DDI_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/ddi_f.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

/*
 * Flag for ticks argument of itimeout() and dtimeout().
 */
#define TO_PERIODIC	0x80000000	/* periodic repeating timer */

/*
 * The following are not current interfaces; DO NOT USE THEM.
 * they are here to allow existing old drivers to compile.
 */
extern struct queue *backq();
extern void dma_pageio();
extern uint_t hat_getkpfnum();
extern uint_t hat_getppfnum();
#undef kvtophys
extern paddr_t kvtophys();
extern void rdma_filter();
extern void rminit();
extern void rmsetwant();
extern ulong_t rmwant();
extern int useracc();

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_DDI_F_H */
