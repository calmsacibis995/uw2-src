/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_CPU_H	/* wrapper symbol for kernel use */
#define _SVC_CPU_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/cpu.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Legal values for cpu_id */

#define CPU_UNK	0
#define	CPU_386	3
#define	CPU_486	4
#define	CPU_P5	5

/* Legal values for cpu_stepping */

#define STEP_UNK	0

/* For CPU_386: */
#define STEP_386B1	1

/* Feature bits */

#ifndef NCPUFEATWORD
#define NCPUFEATWORD	27	/* # (32-bit) words in feature bit array */
#endif

/*   Feature bits in word 0: */
/*   (Details of features are confidential.) */
#define CPUFEAT_DE	0x00000004
#define CPUFEAT_PSE	0x00000008
#define CPUFEAT_TSC	0x00000010
#define CPUFEAT_MCE	0x00000080
#define CPUFEAT_APIC	0x00000200

#ifdef _KERNEL

/* true if one or more online CPU is CPU_386 */
extern volatile int n_i386_online;

#ifdef BUG386B1
extern int do386b1;
extern int do386b1_x87;
extern int do387cr3;
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_CPU_H */
