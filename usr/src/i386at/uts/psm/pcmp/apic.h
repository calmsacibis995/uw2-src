/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/pcmp/apic.h	1.4"
#ident	"$Header: $"

/* Definitions for Intel Advanced Programmable Interrupt Controller */

#define APICADR		0xfee00000		/* physical adr of Local APIC */
						/* virtual adr of Local APIC */
#define APICIADR	0xfec00000		/* physical adr of I/O APIC */
						/* virtual adr of I/O APIC */

/* Misc. */
#define APICINTS        0x50      /* start of apic interrupt in IDT */

/* offsets for registers */

#define	AP_IO_REG	0
#define AP_IO_DATA	4
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
#define AIR_RDT		0x10
#define AIR_RDT2	0x11

/* various values for registers */

#define AV_MASK		0x10000
#define AV_TOALL	0x7fffffff
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
#define AV_POLOW	0x2000
#define AV_POHIGH	0
#define AV_ASSERT	0x4000
#define AV_DEASSERT	0
#define AV_EDGE		0
#define AV_LEVEL	0x8000
#define AV_XTOSELF	0x40000
#define AV_XTOALL	0x80000

/* task priority for spl levels */
#define TPRI0   (16 * 1)			/* 0x10 */
#define TPRI1   APICINTS+(16 * 1)		/* 0x60 */
#define TPRI2   APICINTS+(16 * 2)		/* 0x70 */
#define TPRI3   APICINTS+(16 * 3)		/* 0x80 */
#define TPRI4   APICINTS+(16 * 4)		/* 0x90 */
#define TPRI5   APICINTS+(16 * 5)		/* 0xA0 */
#define TPRI6   APICINTS+(16 * 6)		/* 0xB0 */
/*
 *   --- ipl7 & ipl8 wll lock all interrupts not including IPI and
 *       system bus error conditions and NMI.
 */
#define TPRI7   APICINTS+(16 * 8)		/* 0xD0 */
#define TPRI8   APICINTS+(16 * 8)		/* 0xD0 */
#define TPRIHI  TPRI8

/* define for timer on APIC */
#define AP_CLKNUM       (14318000/HZ)

#define MAXIOAPIC	4	/* max number of I/O apics */
