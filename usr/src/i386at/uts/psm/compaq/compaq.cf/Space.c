/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/compaq.cf/Space.c	1.3"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/ipl.h>
#include <sys/pic.h>
#include <sys/param.h>


#define	SP_SYSTYPES	11
#define	SP_MAXNUMCPU	2
#define	XL_MAXNUMCPU	4

#define ENG1_CTL_PORT	0x0C6A		/* Processor 1 (P1) Control Port */
#define ENG2_CTL_PORT	0xFC6A		/* Processor 2 (P2) Control Port */
#define ENG2_IVEC_PORT	0xFC68		/* P2 Interrupt Vector Control Port */


int sp_systypes = SP_SYSTYPES;
int sp_maxnumcpu = SP_MAXNUMCPU;
int xl_maxnumcpu = XL_MAXNUMCPU;

/*
 * processor control port
 */
ushort_t engine_ctl_port[SP_MAXNUMCPU] = {
	ENG1_CTL_PORT,
	ENG2_CTL_PORT,
};

/*
 * processor interrupt vector base port
 */
ushort_t engine_ivec_port[SP_MAXNUMCPU] = {
	NULL,
	ENG2_IVEC_PORT,
};

static char *eisa_id0[SP_MAXNUMCPU] = {
	"CPQ9990", "CPQ5000"		/* Compaq Systempro 386/33 */
};
static char *eisa_id1[SP_MAXNUMCPU] = {
	"CPQ9014", "CPQ5900"		/* Compaq Systempro 486/33 */
};
static char *eisa_id2[SP_MAXNUMCPU] = {
	"CPQ9014", "CPQ5A00"		/* Compaq Systempro 486/33 */
};
static char *eisa_id3[SP_MAXNUMCPU] = {
	"CPQ9014", "CPQ5C00"		/* Compaq Systempro 486/33 */
};
static char *eisa_id4[SP_MAXNUMCPU] = {
	"CPQ9999", "CPQ5900"            /* Compaq Systempro 486/33 */
};
static char *eisa_id5[SP_MAXNUMCPU] = {
	"CPQ9999", "CPQ5A00"		/* Compaq Systempro 486/33 */
};
static char *eisa_id6[SP_MAXNUMCPU] = {
	"CPQ9999", "CPQ5C00"		/* Compaq Systempro 486/33 */
};
static char *eisa_id7[SP_MAXNUMCPU] = {
	"CPQ9013", "CPQ5B00"		/* Compaq Systempro 486DX2/66 */
};
static char *eisa_id8[SP_MAXNUMCPU] = {
	"CPQ1501", "CPQ5281"		/* Compaq Systempro/XL 486/50MHz */
};
static char *eisa_id9[SP_MAXNUMCPU] = {
	"CPQ1501", "CPQ5287"		/* Compaq Systempro/XL Pentium/66MHz */
};
static char *eisa_id10[SP_MAXNUMCPU] = {
	"CPQ1501", "CPQ5287"		/* Compaq Systempro/XL Pentium/66MHz */
};
char **sp_eisa_id[SP_SYSTYPES] = {
        eisa_id0, eisa_id1, eisa_id2, eisa_id3,
        eisa_id4, eisa_id5, eisa_id6, eisa_id7,
        eisa_id8, eisa_id9, eisa_id10
};

#define SP_XL_1		11		/* CPU Slot 1                   */
#define SP_XL_2		15		/* CPU Slot 2                   */

uchar_t sp_xl_slot[SP_MAXNUMCPU] = {
	SP_XL_1,
	SP_XL_2
};

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
 * I/O mode mask allows the Systempro/XL to overwrite the default
 * mode of operation -- symmetric. The XL can be configured in either 
 * a systempro compatible or symmetric mode. By setting the I/O mode
 * mask to either SYMINTR or ASYMINTR, the PSM will not program the
 * system to that mode. On a systempro compatible this value is
 * ignored since there is only one I/O mode to which the system
 * can be programmed. 
 *
 * If the I/O mode mask value is other than SYMINTR or ASYMINTR,
 * then the system is set to the default I/O mode. IOW, the
 * I/O mode mask value is ignored.
 */
int spxl_iomodemask = 0; 

#ifdef NOTYET
int	sp_xlresetonce[XL_MAXNUMCPU];
#endif /* NOTYET */

long	proliant_id[] = {
		0x0915110E,
		0x1915110E,
		0x2915110E,
		0x3915110E,
		0x6915110E,
		0x7915110E
};

int proliant_ids = sizeof(proliant_id)/4;

char sp_xlmode = 0;

char    sp_xl_eisabuf[24 * 1024];

/*
 * Initialized data for Programmable Interupt Controllers (i8259)
 */

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
	0, MASTERLINE,
};

/*
 * current pic masks
 */
uchar_t curmask[XL_MAXNUMCPU][NPIC];

/*
 * pic masks for intr priority levels
 */
uchar_t iplmask[XL_MAXNUMCPU][(PLHI + 1) * NPIC];

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
uchar_t *curmaskp[XL_MAXNUMCPU] = {
	(uchar_t *)&curmask[0],
	(uchar_t *)&curmask[1],
	(uchar_t *)&curmask[2],
	(uchar_t *)&curmask[3],
};

/*
 * pointers to pic masks for intr priority levels for each engine. 
 */
uchar_t *iplmaskp[XL_MAXNUMCPU] = {
	(uchar_t *)&iplmask[0],
	(uchar_t *)&iplmask[1],
	(uchar_t *)&iplmask[2],
	(uchar_t *)&iplmask[3],
};
