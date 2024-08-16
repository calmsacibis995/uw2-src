/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/olivetti/olivetti.h	1.1"
/*	Copyright (c) 1994 Ing C.Olivetti &c S.p.A. 	 	*/
/*	  All Rights Reserved                             	*/


#ifndef _SVC_OLIBUS_H	/* wrapper symbol for kernel use */
#define _SVC_OLIBUS_H	/* subject to change without notice */

#ifdef _KERNEL_HEADERS
#include <util/types.h>		/* REQUIRED */
#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/types.h>		/* REQUIRED */
#endif /* _KERNEL_HEADERS */

/*
 * OLI_DEBUG only turn on if compiled with DEBUG.
 * unifdef  OLI_DEBUG otherwise
 */
#ifdef	DEBUG
#define	OLI_DEBUG
#endif

#define	XINTR		13		/* irq no. of inter-processor intr. */

#define	CLOCKINTR	0		/* irq no. of clock interrupt. */

#define OLIBUS_MAXNUMCPU	4

int ltopcpu[OLIBUS_MAXNUMCPU];

#define	ENG1_CTL_PORT	0x0C93		/* P1 Processor Option Register	      */
#define	ENG2_CTL_PORT	0xCC93		/* P2 Processor Option Register       */
#define	ENG3_CTL_PORT	0xDC93		/* P3 Processor Option Register       */
#define	ENG4_CTL_PORT	0xEC93		/* P4 Processor Option Register       */

#define PHYS_ID0	0
#define PHYS_ID1	1
#define PHYS_ID2	2
#define PHYS_ID3	3

#define	RESET		0x01		/* take processor in and out of reset */
#define	IRQSEL		0x0C		/* IRQ selection mask                 */
#define	INTENA		0x80		/* enable interrupts on this cpu      */

#define	IRQ15		0x00		
#define	IRQ11		0x04	
#define	IRQ10		0x08
#define	IRQ13		0x0C

#define WHOAMI_PORT	0x0C70		/* who_am_i port (tells CPU # )	      */

#define OLIBUS_IAM0	0x00		/* who_am_i = cpu 0		      */
#define OLIBUS_IAM1	0xC0		/* who_am_i = cpu 1		      */
#define OLIBUS_IAM2	0xD0		/* who_am_i = cpu 2		      */
#define OLIBUS_IAM3	0xE0		/* who_am_i = cpu 3     	      */

#define OLIBUS_ENG1_SLOT	0x0
#define OLIBUS_ENG2_SLOT	0xC
#define OLIBUS_ENG3_SLOT	0xD
#define OLIBUS_ENG4_SLOT	0xE


/* Engine configuration */

#define OLIBUS_CONF	0x36		/* read CMOS configuration */
#define	CPU_DISPATCH	0x40

#define OLIBUS_ENG1_CONF		(0x11 << 0)
#define OLIBUS_ENG2_CONF		(0x11 << 1)
#define OLIBUS_ENG3_CONF		(0x11 << 2)
#define OLIBUS_ENG4_CONF		(0x11 << 3)

/*
** 5050 MB registers
*/
#define OLIMIMR		0x21		/* Master PIC IMR Port */
#define OLISIMR		0xA1		/* Slave PIC IMR Port  */

#define OLIMSPL5	0xFA		/* Leave Just IRQ0 on Master PIC */
#define OLISSPL5	0xFF		/* Leave Just IRQ0 on Slave PIC  */

#define OLICPURES	0x467 		/* cpu boot vector address */

#define OLIMTRIGGER	0x4D0		/* Master PIC Trigger Mode Selection  */
#define OLISTRIGGER	0x4D1		/* Slave  PIC Trigger Mode Selection  */
#define OLILMTRIGGER	0xC95		/* Ctr.1 APIC Trigger Mode Sel. Local */
#define OLILSTRIGGER	0xC96		/* Ctr.2 APIC Trigger Mode Sel. Local */

#define OLICACHECNTR	0xC94		/* cache control register */
#define CACHEFLUSHSYN	0x02		

/* ELC registers */

#define	ENG1_ELCR	0x04D0		/* MB ELC register */
#define	ENG2_ELCR	0xCC95		/* P2 ELC register */
#define	ENG3_ELCR	0xDC95		/* P3 ELC register */
#define	ENG4_ELCR	0xEC95		/* P4 ELC register */

#ifdef ECC 
/* ACER */
#define A15K_ECCFIXED	0xC10		/* latch reg. Recovered 1 bit ecc errs*/
#define A15K_MEMFATAL	0xC14		/* latch reg. fatal N bit ecc errs    */
#define A15K_MEMINFO	0xC18		/* memory config/event register	      */

#define	A15K_ECC1ENAB()	inb(0xc38)	/* enable single-bit err service*/
#define	A15K_ECC2ENAB()	inb(0xc39)	/* enable multi-bit err service	*/

#endif /* ECC */


#define	SXCALL_LED	(1 << 2)
#define	RXCALL_LED	(1 << 3)
#define	IVECT_LED	(1 << 4)

#define RESET_VECT	0x467	/* reset vector location */
#define APICINTS        96      /* start of apic interrupt in IDT */

#endif	/* _SVC_OLIBUS_H */
