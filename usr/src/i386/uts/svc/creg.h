/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_CREG_H	/* wrapper symbol for kernel use */
#define _SVC_CREG_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/creg.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * i386 family Control Register bits
 */

#define	CR0_PE		0x00000001	/* Protection Enable		*/
#define	CR0_MP		0x00000002	/* Math coprocessor Present	*/
#define	CR0_EM		0x00000004	/* use math EMulation		*/
#define	CR0_TS		0x00000008	/* Task Switched		*/
#define	CR0_NE		0x00000020	/* Numeric Error mode		*/
#define	CR0_WP		0x00010000	/* Write Protect		*/
#define	CR0_AM		0x00040000	/* Alignment Mask		*/
#define	CR0_NW		0x20000000	/* No Write through		*/
#define	CR0_CD		0x40000000	/* Cache Disable		*/
#define	CR0_PG		0x80000000	/* enable PaGing		*/

#define	CR4_VME		0x00000001	/* Virtual-8086 Mode Extensions	*/
#define	CR4_PVI		0x00000002	/* Protect-Mode Virtual Int	*/
#define	CR4_TSD		0x00000004	/* Time Stamp counter Disable	*/
#define	CR4_DE		0x00000008	/* Debugging Extensions		*/
#define	CR4_PSE		0x00000010	/* Page Size Extension 		*/
#define	CR4_MCE		0x00000040	/* Machine Check Exceptions	*/

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_CREG_H */
