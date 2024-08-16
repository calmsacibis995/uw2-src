/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/nf_apic.h	1.1"
#ident	"$Header: $"
#ifndef _SVC_APIC_H	/* wrapper symbol for kernel use */
#define _SVC_APIC_H	/* subject to change without notice */


#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Definitions for Intel Advanced Programmable Interrupt Controller */

#define APICADR		0xfee00000		/* physical adr of Local APIC */

#define APICINTS        0x60      /* start of apic interrupt in IDT */

#define	APICVECS	16

/* offsets for registers */

#define	AP_IO_REG	0
#define AP_IO_DATA	0x10
#define AP_ID		(4*2)
#define AP_VERS		(4*3)
#define AP_TASKPRI      (4*8)
#define AP_LDEST	(4*0xd)
#define AP_DESTFMT	(4*0xe)
#define AP_SPUR		(4*0xf)
#define AP_ICMD		(4*0x30)
#define AP_ICMD2	(4*0x31)
#define AP_LVT_TIMER	(4*0x32)
#define AP_LVT_I0	(4*0x35)
#define AP_LVT_I1	(4*0x36)
#define AP_ICOUNT	(4*0x38) 	/* initial count register */
#define AP_CCOUNT	(4*0x39)	/* current count register */
/* select values for I/O registers */

#define AIR_ID		0
#define AIR_VERS	1
#define AIR_RDT		0x10
#define AIR_RDT2	0x11

/* various values for registers */

#define AV_MASK		0x10000
#define AV_TOALL	0x7fffffff
#define AV_PALL		0xff000000
#define APIC_LOGDEST(c)	(0x40000000>>(c))	/* log dest. if ON */
#define AV_IM_OFF	0x80000000		/* log dest. if OFF */
#define AV_FIXED	0
#define AV_LOPRI	0x100
#define AV_NMI		0x400
#define AV_RESET	0x500
#define AV_STARTUP	0x600
#define AV_EXTINT	0x700
#define AV_PENDING	0x1000
#define AV_PDEST	0
#define AV_LDEST	0x800
#define AV_ASSERT	0x4000
#define AV_DEASSERT	0
#define AV_EDGE		0
#define AV_LEVEL	0x8000
#define AV_XTOSELF	0x40000
#define AV_XTOALL	0x80000

/* Mapping of PIC IRQ to APIC IRQ */
struct apic_int {
	int	a_pic; 		/* pic vector */
	int	a_type;		/* type of APIC */
	int	a_addr;		/* apic address */
	int	a_irq;		/* intr source */
};
#define	IO_APIC		1
#define	PROC_APIC	2

/* define for timer on APIC */
#define AP_CLKNUM       (20000000/HZ)

#endif /* _SVC_APIC_H */
