/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_INTR_H	/* wrapper symbol for kernel use */
#define _SVC_INTR_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:svc/intr.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL

/*
 * SLIC interrupt vectors.
 */

#define	BIN0INT		32		/* Bin 0 interrupt (SW interrupt) */
#define	BIN1INT		33		/* Bin 1 interrupt */
#define	BIN2INT		34		/* Bin 2 interrupt */
#define	BIN3INT		35		/* Bin 3 interrupt */
#define	BIN4INT		36		/* Bin 4 interrupt */
#define	BIN5INT		37		/* Bin 5 interrupt */
#define	BIN6INT		38		/* Bin 6 interrupt */
#define	BIN7INT		39		/* Bin 7 interrupt */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_INTR_H */
