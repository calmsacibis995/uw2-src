/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/olivetti/oliapic.h	1.3"
/*	Copyright (c) 1993 UNIX System Laboratories, Inc. 	*/
/*	  All Rights Reserved                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.   	            	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*								*/
/*	Copyright 1994 Ing. C. Olivetti & C., S.p.A.		*/

#ifndef _SVC_APIC_H	/* wrapper symbol for kernel use */
#define _SVC_APIC_H	/* subject to change without notice */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif


/* Definitions for Intel Advanced Programmable Interrupt Controller */

#define APICADR		0xfee00000	/* physical adr of Local APIC */
#define APICIADR	APICADR		/* physical adr of I/O APIC */

volatile unsigned long 	*apic_vbase;	/* virtual adr of Local APIC */

/*
 * Note:
 * For 5050, APICADR == APICIADR, as Olivetti hardware does not map
 * I/O unit at physical 0xfec00000, as is recommended for standard pourpose
 */
/* offsets for registers */

#define	AP_IO_REG	apic_vbase[0]
#define AP_IO_DATA	apic_vbase[4]
#define AP_ID		apic_vbase[(4*2)]
#define AP_TASKPRI	apic_vbase[(4*8)]	/* == TASKPRI  */
#define AP_EOI		apic_vbase[(4*0xb)]	/* == APIC_EOI */
#define AP_LDEST	apic_vbase[(4*0xd)]
#define AP_DESTFMT	apic_vbase[(4*0xe)]
#define AP_SPUR		apic_vbase[(4*0xf)]
#define AP_ICMD		apic_vbase[(4*0x30)]
#define AP_ICMD2	apic_vbase[(4*0x31)]
#define AP_LVT_TIMER	apic_vbase[(4*0x32)]
#define AP_LVT_I0	apic_vbase[(4*0x35)]
#define AP_LVT_I1	apic_vbase[(4*0x36)]
#define AP_ICOUNT	apic_vbase[(4*0x38)] 	/* initial count register */
#define AP_CCOUNT	apic_vbase[(4*0x39)]	/* current count register */

/* select values for I/O registers */

#define AIR_ID		0
#define AIR_RDT		0x10
#define AIR_RDT2	0x11
#define	AP_RDEST(i)	(AIR_RDT2 + 2*(i)) /* redirection table entry */
#define	AP_RDIR(i)	(AIR_RDT + 2*(i)) /* redirection table destination */

/* various values for registers */

#define AV_MASK		0x10000
#define AV_TOALL	0x0fffffff
#define AV_IM_OFF	0x40000000
#define AV_FIXED	0
#define AV_LOPRI	0x100
#define AV_PENDING	0x1000
#define AV_PDEST	0
#define AV_LDEST	0x800
#define	AV_DESVER	1
#define AV_ASSERT       0x4000
#define AV_LEVEL        0x8000
#define AV_RESET        0x500
#define AV_DEASSERT     0


/* task priority for spl levels */
#define TPRI0   0x00
#define TPRI1   0x60
#define TPRI2   0x70
#define TPRI3   0x80
#define TPRI4   0x90
#define TPRI5   0xA0
#define TPRI6   0xB0
#define TPRI7   0xD0
#define TPRIHI  0xD0

#define PIC_LTIM        0x08
#define	MASKOFF		0xFF

/* define for timer on APIC */
#define AP_CLKNUM  (1000000/HZ)

/* LTOCPULU is used to convert a logical CPU number into the */
/* logical unit ID, used in AP_ICMD2 to send IPC to the desired */
/* CPU. CPU2LDEST is used to initialize the logical destination */
/* register of local unit, and to program the APIC destination  */
/* register of the redirection table, as all interrupts are programmed */
/* in logical destination mode */

#define	LTOCPULU(lcpu)	((2*(ltopcpu[lcpu]))<<24) /* LU ID for IPC */
#define	CPU2LDEST(lcpu)	((1<<(ltopcpu[lcpu]))<<24)/* log dest. reg ID */

#define	AP_DL_RESET	0x500
#define	AP_DST_PHYS	0

#define AP_LV_DEASSERT	0
#define AP_LV_ASSERT	0x4000

#define	AP_DSH_DEST	0
#define	AP_DSH_ALL	0x80000
#define	BINDPANIC	"intr: %d bound to illegal CPU (%d)!!"

#define	AP_TRM_0	0xc95	/* ctr.1 APIC trigger mode select Local */
#define	AP_TRM_1	0xc96	/* ctr.2 APIC trigger mode select Local */
#define AP_TR_EDGE	0
#define AP_TR_LEVEL	(1 << 15)

#define AP_UNITENABLE		0x100
#define	AP_SPURIOUSVECT	0xCF
#define	AP_TIMER	0

#define	AP_EIRQS	256
#define	AP_IRQS		16	/* 16 IRQs, 0 to 15 	   */
#define	SOFT_LVL	9	/* 9 logical spl (spl0-splhi) */
#define	LVL0_SHIFT	0x10	/* used to redirect intno, discarding */
				/* interrupt ids 0 through 15 */
#define	AP_IPCVECT	0xEE
#define APICINTS        96      /* start of apic interrupt in IDT */

#define SOFTINT 63              /* software interrupt */

#endif /* _SVC_AP_H */
