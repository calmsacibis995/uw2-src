/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/acer/acer.cf/Space.c	1.2"
#ident	"$Header: $"

#include <sys/ipl.h>
#include <sys/param.h>
#include <sys/pic.h>
#include <sys/types.h>
#include <config.h>


#define ACER_MAXNUMCPU		4

int acer_maxnumcpu = ACER_MAXNUMCPU;


/*
 * Interrupt command ports and mask ports to be accessed
 * by other engines.
 */

#define ENG0_MCMD_PORT	0xC020		/* rev C CPU0 master command port */
#define ENG0_MIMR_PORT	0xC021		/* rev C CPU0 master intr mask port */
#define ENG0_SCMD_PORT	0xC0A0		/* rev C CPU0 slave command port */
#define	ENG0_SIMR_PORT	0xC0A1		/* rev C CPU0 slave intr mask port */

#define ENG2_MCMD_PORT	0xC024		/* rev C CPU2 master command port */
#define ENG2_MIMR_PORT	0xC025		/* rev C CPU2 master intr mask port */
#define ENG2_SCMD_PORT	0xC0A4		/* rev C CPU2 slave command port */
#define ENG2_SIMR_PORT	0xC0A5		/* rev C CPU2 slave intr mask port */

#define ENG3_MCMD_PORT	0xC028		/* rev C CPU3 master command port */
#define ENG3_MIMR_PORT	0xC029		/* rev C CPU3 master intr mask port */
#define ENG3_SCMD_PORT	0xC0A8		/* rev C CPU3 slave command port */
#define ENG3_SIMR_PORT	0xC0A9		/* rev C CPU3 slave intr mask port */

#define ENG4_MCMD_PORT	0xC02C		/* rev C CPU4 master command port */
#define ENG4_MIMR_PORT	0xC02D		/* rev C CPU4 master intr mask port */
#define ENG4_SCMD_PORT	0xC0AC		/* rev C CPU4 slave command port */
#define ENG4_SIMR_PORT	0xC0AD		/* rev C CPU4 slave intr mask port */

#define	ENG1_CTL_PORT	0x0C6A		/* P1 Processor Option Register	      */
#define	ENG2_CTL_PORT	0xFC6A		/* P2 Processor Option Register       */
#define	ENG3_CTL_PORT	0xCC6A		/* P3 Processor Option Register       */
#define	ENG4_CTL_PORT	0xDC6A		/* P4 Processor Option Register       */

#define	ENG2_IVEC_PORT	0xFC68		/* P2 Interrupt Vector Control Port   */
#define	ENG3_IVEC_PORT	0xCD8		/* P3 Interrupt Vector Control Port   */
#define	ENG4_IVEC_PORT	0xCE0		/* P4 Interrupt Vector Control Port   */
#define	ENG3_IVECALT_PORT	0xC028	/* P3 Interrupt ??? Control Port      */
#define	ENG4_IVECALT_PORT	0xC02C	/* P4 Interrupt ??? Control Port      */

#define ACER_ENG1_SLOT	0x0
#define ACER_ENG2_SLOT	0xF
#define ACER_ENG3_SLOT	0xC
#define ACER_ENG4_SLOT	0xD

#define ACER_IAM0	0x00		/* who_am_i = cpu 0		      */
#define ACER_IAM1	0xF0		/* who_am_i = cpu 1		      */
#define ACER_IAM2	0x0F		/* who_am_i = cpu 2		      */
#define ACER_IAM3	0xFF		/* who_am_i = cpu 3 (beware !!)	      */

/*
 * Interrupt distribution mask to prevent reassignment of an 
 * interrupt because of asymmetric nature of the hardware.
 * A non-zero entry indicates that the interrupt is a
 * non-distributable and a zero indicates that the interrupt
 * can be assigned to any cpu. 
 *
 * The array is indexed by interrupt request number (irq).
 */
int intrdistmask[NPIC * PIC_NIRQ] = {
	{ 1 },	/* 0 - clock */
	{ 0 },	/* 1 - kdintr */
	{ 0 },	/* 2 - intnull */
	{ 0 },	/* 3 - asycintr */
	{ 0 },	/* 4 - asycintr */
	{ 0 },	/* 5 - dcd_intr */
	{ 0 },	/* 6 - fdintr */
	{ 0 },	/* 7 - lpintr */
	{ 0 },	/* 8 - rtcintr */
	{ 0 },	/* 9 - intnull */
	{ 0 },	/* 10 - intnull */
	{ 0 },	/* 11 - adscintr */
	{ 0 },	/* 12 - intnull */
 	{ 1 },	/* 13 - psm_intr */
 	{ 1 },	/* 14 - dcd_intr or shrint14 */ 
	{ 1 }	/* 15 - shrint15 */
};


/*
 * processor slot info table. 
 */
int acer_eng_slot[ACER_MAXNUMCPU] = {
        ACER_ENG1_SLOT,
        ACER_ENG2_SLOT,
        ACER_ENG3_SLOT,
        ACER_ENG4_SLOT
};


/*
 * processor control port
 */
ushort_t engine_ctl_port[ACER_MAXNUMCPU] = {
	ENG1_CTL_PORT,
	ENG2_CTL_PORT,
	ENG3_CTL_PORT,
	ENG4_CTL_PORT
};


/*
 * processor interrupt vector base port
 */
ushort_t engine_ivec_port[ACER_MAXNUMCPU] = {
	NULL,
	ENG2_IVEC_PORT,
	ENG3_IVEC_PORT,
	ENG4_IVEC_PORT
};


/* Initialized data for Programmable Interupt Controllers */

/*
 * command port addrs for pics accessed by other cpus
 */
ushort_t dist_cmdport[ACER_MAXNUMCPU][NPIC] = {
	ENG0_MCMD_PORT, ENG0_SCMD_PORT,	/* CPU0 master/slave command ports */
	ENG2_MCMD_PORT, ENG2_SCMD_PORT,	/* CPU2 master/slave command ports */
	ENG3_MCMD_PORT, ENG3_SCMD_PORT,	/* CPU3 master/slave command ports */
	ENG4_MCMD_PORT, ENG4_SCMD_PORT	/* CPU4 master/slave command ports */
};

/*
 * interrupt mask port addrs for pics accessed by other cpus.
 */
ushort_t dist_imrport[ACER_MAXNUMCPU][NPIC] = {
	ENG0_MIMR_PORT, ENG0_SIMR_PORT,	/* CPU0 master/slave interrupt mask */
	ENG2_MIMR_PORT, ENG2_SIMR_PORT,	/* CPU2 master/slave interrupt mask */
	ENG3_MIMR_PORT, ENG3_SIMR_PORT,	/* CPU3 master/slave interrupt mask */
	ENG4_MIMR_PORT, ENG4_SIMR_PORT	/* CPU1 master/slave interrupt mask */
};

/*
 * command port addrs of pics for self access
 */
ushort_t cmdport[NPIC] = {
	MCMD_PORT, SCMD_PORT
};

/*
 * interrupt mask port addrs of pics for self access
 */
ushort_t imrport[NPIC] = {
	MIMR_PORT, SIMR_PORT
};

uchar_t masterpic[NPIC]		/* index of this pic's master (for 82380) */
	= { 0, 0 };

/*
 * line on master this slave connected to
 */
uchar_t masterline[NPIC] = {
	0, MASTERLINE
};

/*
 * current pic masks
 */
uchar_t curmask[ACER_MAXNUMCPU][NPIC];

/*
 * pic masks for intr priority levels
 */
uchar_t iplmask[ACER_MAXNUMCPU][(PLHI + 1) * NPIC];

uchar_t picbuffered = PICBUFFERED;	/* PICs in buffered mode */

int npic = NPIC;		/* number of pics configured */

/*
 * service priority level for each interrupt
 */
pl_t svcpri[NPIC * PIC_NIRQ];

/*
 * table of per IRQ information
 */ 
struct irqtab irqtab[NPIC * PIC_NIRQ];

/*
 * pointers to current masks of pics for each engine. 
 */
uchar_t *curmaskp[ACER_MAXNUMCPU] = {
	(uchar_t *)&curmask[0],
	(uchar_t *)&curmask[1],
	(uchar_t *)&curmask[2],
	(uchar_t *)&curmask[3]
};

/*
 * pointers to pic masks for intr priority levels for each engine. 
 */
uchar_t *iplmaskp[ACER_MAXNUMCPU] = {
	(uchar_t *)&iplmask[0],
	(uchar_t *)&iplmask[1],
	(uchar_t *)&iplmask[2],
	(uchar_t *)&iplmask[3]
};
