/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/psm.c	1.3"
#ident	"$Header: $"

#include <sys/bootinfo.h>
#include <sys/cram.h>
#include <sys/immu.h>
#include <mem/vm_mdep.h>
#include <sys/vmparam.h>
#include <svc/intr.h>
#include <psm/intr_p.h>
#include <sys/nf_pic.h>
#include <sys/nf_pit.h>
#include <sys/psm.h>
#include <sys/uadmin.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/engine.h>
#include <sys/inline.h>
#include <sys/param.h>
#include <sys/plocal.h>
#include <sys/processor.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/nf_apic.h>

extern caddr_t physmap(paddr_t, ulong_t, uint_t);

void softreset(void);

#define	IO_ROM_INIT	0xfff0
#define	IO_ROM_SEG	0xf000
#define	RESET_FLAG	0x1234
#define	SHUT_DOWN	0x8f
#define	SHUT5		5
#define	STAT_8042	0x64

#define START_ID_LOC	0	/* location containing the ID of the */
				/* processor to be started */
#define	START_ADDR_LOC	4	/* location containing the starting address */
#define	SLAVEID_LOC	8	/* location containing the mask for the */
				/* slave processors */
#define	MASTERID_LOC	12	/* location containing the mask for the */
				/* master processor */

/*
 * ARRAY Of eventflags, one per engine, used in implementation of sendsoft
 */
STATIC volatile uint_t psm_eventflags[MAXNUMCPU];

extern struct engine  *engine;                /* base of engine array */
extern struct engine  *engine_Nengine;        /* end of engine array */
extern int Nengine;
extern int cpurate;
int is_gemstone;

/*
 * common IDT structure for all NetFRAME engines. 
 */
struct idt_init nf_idt_init[] = {
	{       31,             GATE_386INT,    ivctSTRAY,      GATE_KACC },
        {       SOFTINT,        GATE_386INT,    softint,        GATE_KACC },
        {       DEVINTS,        GATE_386INT,    devint0,        GATE_KACC },
        {       DEVINTS + 1,    GATE_386INT,    devint1,        GATE_KACC },
        {       DEVINTS + 2,    GATE_386INT,    devint2,        GATE_KACC },
        {       DEVINTS + 3,    GATE_386INT,    devint3,        GATE_KACC },
        {       DEVINTS + 4,    GATE_386INT,    devint4,        GATE_KACC },
        {       DEVINTS + 5,    GATE_386INT,    devint5,        GATE_KACC },
        {       DEVINTS + 6,    GATE_386INT,    devint6,        GATE_KACC },
        {       DEVINTS + 7,    GATE_386INT,    devint7,        GATE_KACC },
        {       DEVINTS + 8,    GATE_386INT,    devint8,        GATE_KACC },
        {       DEVINTS + 9,    GATE_386INT,    devint9,        GATE_KACC },
        {       DEVINTS + 10,   GATE_386INT,    devint10,       GATE_KACC },
        {       DEVINTS + 11,   GATE_386INT,    devint11,       GATE_KACC },
        {       DEVINTS + 12,   GATE_386INT,    devint12,       GATE_KACC },
        {       DEVINTS + 13,   GATE_386INT,    devint13,       GATE_KACC },
        {       DEVINTS + 14,   GATE_386INT,    devint14,       GATE_KACC },
        {       DEVINTS + 15,   GATE_386INT,    devint15,       GATE_KACC },
        {       DEVINTS + 16,   GATE_386INT,    devint16,       GATE_KACC },
        {       DEVINTS + 17,   GATE_386INT,    devint17,       GATE_KACC },
        {       DEVINTS + 18,   GATE_386INT,    devint18,       GATE_KACC },
        {       DEVINTS + 19,   GATE_386INT,    devint19,       GATE_KACC },
        {       DEVINTS + 20,   GATE_386INT,    devint20,       GATE_KACC },
        {       DEVINTS + 21,   GATE_386INT,    devint21,       GATE_KACC },
        {       DEVINTS + 22,   GATE_386INT,    devint22,       GATE_KACC },
        {       DEVINTS + 23,   GATE_386INT,    devint23,       GATE_KACC },
        {       DEVINTS + 24,   GATE_386INT,    devint24,       GATE_KACC },
        {       DEVINTS + 25,   GATE_386INT,    devint25,       GATE_KACC },
        {       DEVINTS + 26,   GATE_386INT,    devint26,       GATE_KACC },
        {       DEVINTS + 27,   GATE_386INT,    devint27,       GATE_KACC },
        {       DEVINTS + 28,   GATE_386INT,    devint28,       GATE_KACC },
        {       DEVINTS + 29,   GATE_386INT,    devint29,       GATE_KACC },
        {       DEVINTS + 30,   GATE_386INT,    devint30,       GATE_KACC },
        {       DEVINTS + 31,   GATE_386INT,    devint31,       GATE_KACC },
        {       APICINTS,       GATE_386INT,    ivct60,         GATE_KACC },
        {       APICINTS + 1,   GATE_386INT,    ivct61,         GATE_KACC },
        {       APICINTS + 2,   GATE_386INT,    ivct62,         GATE_KACC },
        {       APICINTS + 3,   GATE_386INT,    ivct63,         GATE_KACC },
        {       APICINTS + 4,   GATE_386INT,    ivct64,         GATE_KACC },
        {       APICINTS + 5,   GATE_386INT,    ivct65,         GATE_KACC },
        {       APICINTS + 6,   GATE_386INT,    ivct66,         GATE_KACC },
        {       APICINTS + 7,   GATE_386INT,    ivct67,         GATE_KACC },
        {       APICINTS + 8,   GATE_386INT,    ivct68,         GATE_KACC },
        {       APICINTS + 9,   GATE_386INT,    ivct69,         GATE_KACC },
        {       APICINTS + 10,  GATE_386INT,    ivct6A,         GATE_KACC },
        {       APICINTS + 11,  GATE_386INT,    ivct6B,         GATE_KACC },
        {       APICINTS + 12,  GATE_386INT,    ivct6C,         GATE_KACC },
        {       APICINTS + 13,  GATE_386INT,    ivct6D,         GATE_KACC },
        {       APICINTS + 14,  GATE_386INT,    ivct6E,         GATE_KACC },
        {       APICINTS + 15,  GATE_386INT,    ivct6F,         GATE_KACC },
        {       APICINTS + 16,  GATE_386INT,    ivct70,         GATE_KACC },
        {       APICINTS + 17,  GATE_386INT,    ivct71,         GATE_KACC },
        {       APICINTS + 18,  GATE_386INT,    ivct72,         GATE_KACC },
        {       APICINTS + 19,  GATE_386INT,    ivct73,         GATE_KACC },
        {       APICINTS + 20,  GATE_386INT,    ivct74,         GATE_KACC },
        {       APICINTS + 21,  GATE_386INT,    ivct75,         GATE_KACC },
        {       APICINTS + 22,  GATE_386INT,    ivct76,         GATE_KACC },
        {       APICINTS + 23,  GATE_386INT,    ivct77,         GATE_KACC },
        {       APICINTS + 24,  GATE_386INT,    ivct78,         GATE_KACC },
        {       APICINTS + 25,  GATE_386INT,    ivct79,         GATE_KACC },
        {       APICINTS + 26,  GATE_386INT,    ivct7A,         GATE_KACC },
        {       APICINTS + 27,  GATE_386INT,    ivct7B,         GATE_KACC },
        {       APICINTS + 28,  GATE_386INT,    ivct7C,         GATE_KACC },
        {       APICINTS + 29,  GATE_386INT,    ivct7D,         GATE_KACC },
        {       APICINTS + 30,  GATE_386INT,    ivct7E,         GATE_KACC },
        {       APICINTS + 31,  GATE_386INT,    ivct7F,         GATE_KACC },
        {       APICINTS + 32,  GATE_386INT,    ivct80,         GATE_KACC },
        {       APICINTS + 33,  GATE_386INT,    ivct81,         GATE_KACC },
        {       APICINTS + 34,  GATE_386INT,    ivct82,         GATE_KACC },
        {       APICINTS + 35,  GATE_386INT,    ivct83,         GATE_KACC },
        {       APICINTS + 36,  GATE_386INT,    ivct84,         GATE_KACC },
        {       APICINTS + 37,  GATE_386INT,    ivct85,         GATE_KACC },
        {       APICINTS + 38,  GATE_386INT,    ivct86,         GATE_KACC },
        {       APICINTS + 39,  GATE_386INT,    ivct87,         GATE_KACC },
        {       APICINTS + 40,  GATE_386INT,    ivct88,         GATE_KACC },
        {       APICINTS + 41,  GATE_386INT,    ivct89,         GATE_KACC },
        {       APICINTS + 42,  GATE_386INT,    ivct8A,         GATE_KACC },
        {       APICINTS + 43,  GATE_386INT,    ivct8B,         GATE_KACC },
        {       APICINTS + 44,  GATE_386INT,    ivct8C,         GATE_KACC },
        {       APICINTS + 45,  GATE_386INT,    ivct8D,         GATE_KACC },
        {       APICINTS + 46,  GATE_386INT,    ivct8E,         GATE_KACC },
        {       APICINTS + 47,  GATE_386INT,    ivct8F,         GATE_KACC },
        {       APICINTS + 48,  GATE_386INT,    ivct90,         GATE_KACC },
        {       APICINTS + 49,  GATE_386INT,    ivct91,         GATE_KACC },
        {       APICINTS + 50,  GATE_386INT,    ivct92,         GATE_KACC },
        {       APICINTS + 51,  GATE_386INT,    ivct93,         GATE_KACC },
        {       APICINTS + 52,  GATE_386INT,    ivct94,         GATE_KACC },
        {       APICINTS + 53,  GATE_386INT,    ivct95,         GATE_KACC },
        {       APICINTS + 54,  GATE_386INT,    ivct96,         GATE_KACC },
        {       APICINTS + 55,  GATE_386INT,    ivct97,         GATE_KACC },
        {       APICINTS + 56,  GATE_386INT,    ivct98,         GATE_KACC },
        {       APICINTS + 57,  GATE_386INT,    ivct99,         GATE_KACC },
        {       APICINTS + 58,  GATE_386INT,    ivct9A,         GATE_KACC },
        {       APICINTS + 59,  GATE_386INT,    ivct9B,         GATE_KACC },
        {       APICINTS + 60,  GATE_386INT,    ivct9C,         GATE_KACC },
        {       APICINTS + 61,  GATE_386INT,    ivct9D,         GATE_KACC },
        {       APICINTS + 62,  GATE_386INT,    ivct9E,         GATE_KACC },
        {       APICINTS + 63,  GATE_386INT,    ivct9F,         GATE_KACC },
        {       APICINTS + 64,  GATE_386INT,    ivctA0,         GATE_KACC },
        {       APICINTS + 65,  GATE_386INT,    ivctA1,         GATE_KACC },
        {       APICINTS + 66,  GATE_386INT,    ivctA2,         GATE_KACC },
        {       APICINTS + 67,  GATE_386INT,    ivctA3,         GATE_KACC },
        {       APICINTS + 68,  GATE_386INT,    ivctA4,         GATE_KACC },
        {       APICINTS + 69,  GATE_386INT,    ivctA5,         GATE_KACC },
        {       APICINTS + 70,  GATE_386INT,    ivctA6,         GATE_KACC },
        {       APICINTS + 71,  GATE_386INT,    ivctA7,         GATE_KACC },
        {       APICINTS + 72,  GATE_386INT,    ivctA8,         GATE_KACC },
        {       APICINTS + 73,  GATE_386INT,    ivctA9,         GATE_KACC },
        {       APICINTS + 74,  GATE_386INT,    ivctAA,         GATE_KACC },
        {       APICINTS + 75,  GATE_386INT,    ivctAB,         GATE_KACC },
        {       APICINTS + 76,  GATE_386INT,    ivctAC,         GATE_KACC },
        {       APICINTS + 77,  GATE_386INT,    ivctAD,         GATE_KACC },
        {       APICINTS + 78,  GATE_386INT,    ivctAE,         GATE_KACC },
        {       APICINTS + 79,  GATE_386INT,    ivctAF,         GATE_KACC },
        {       APICINTS + 80,  GATE_386INT,    ivctB0,         GATE_KACC },
        {       APICINTS + 81,  GATE_386INT,    ivctB1,         GATE_KACC },
        {       APICINTS + 82,  GATE_386INT,    ivctB2,         GATE_KACC },
        {       APICINTS + 83,  GATE_386INT,    ivctB3,         GATE_KACC },
        {       APICINTS + 84,  GATE_386INT,    ivctB4,         GATE_KACC },
        {       APICINTS + 85,  GATE_386INT,    ivctB5,         GATE_KACC },
        {       APICINTS + 86,  GATE_386INT,    ivctB6,         GATE_KACC },
        {       APICINTS + 87,  GATE_386INT,    ivctB7,         GATE_KACC },
        {       APICINTS + 88,  GATE_386INT,    ivctB8,         GATE_KACC },
        {       APICINTS + 89,  GATE_386INT,    ivctB9,         GATE_KACC },
        {       APICINTS + 90,  GATE_386INT,    ivctBA,         GATE_KACC },
        {       APICINTS + 91,  GATE_386INT,    ivctBB,         GATE_KACC },
        {       APICINTS + 92,  GATE_386INT,    ivctBC,         GATE_KACC },
        {       APICINTS + 93,  GATE_386INT,    ivctBD,         GATE_KACC },
        {       APICINTS + 94,  GATE_386INT,    ivctBE,         GATE_KACC },
        {       APICINTS + 95,  GATE_386INT,    ivctBF,         GATE_KACC },
        {       APICINTS + 96,  GATE_386INT,    ivctC0,         GATE_KACC },
        {       APICINTS + 97,  GATE_386INT,    ivctC1,         GATE_KACC },
        {       APICINTS + 98,  GATE_386INT,    ivctC2,         GATE_KACC },
        {       APICINTS + 99,  GATE_386INT,    ivctC3,         GATE_KACC },
        {       APICINTS + 100, GATE_386INT,    ivctC4,         GATE_KACC },
        {       APICINTS + 101, GATE_386INT,    ivctC5,         GATE_KACC },
        {       APICINTS + 102, GATE_386INT,    ivctC6,         GATE_KACC },
        {       APICINTS + 103, GATE_386INT,    ivctC7,         GATE_KACC },
        {       APICINTS + 104, GATE_386INT,    ivctC8,         GATE_KACC },
        {       APICINTS + 105, GATE_386INT,    ivctC9,         GATE_KACC },
        {       APICINTS + 106, GATE_386INT,    ivctCA,         GATE_KACC },
        {       APICINTS + 107, GATE_386INT,    ivctCB,         GATE_KACC },
        {       APICINTS + 108, GATE_386INT,    ivctCC,         GATE_KACC },
        {       APICINTS + 109, GATE_386INT,    ivctCD,         GATE_KACC },
        {       APICINTS + 110, GATE_386INT,    ivctCE,         GATE_KACC },
        {       APICINTS + 111, GATE_386INT,    ivctCF,         GATE_KACC },
        {       APICINTS + 112, GATE_386INT,    ivctD0,         GATE_KACC },
        {       APICINTS + 113, GATE_386INT,    ivctD1,         GATE_KACC },
        {       APICINTS + 114, GATE_386INT,    ivctD2,         GATE_KACC },
        {       APICINTS + 115, GATE_386INT,    ivctD3,         GATE_KACC },
        {       APICINTS + 116, GATE_386INT,    ivctD4,         GATE_KACC },
        {       APICINTS + 117, GATE_386INT,    ivctD5,         GATE_KACC },
        {       APICINTS + 118, GATE_386INT,    ivctD6,         GATE_KACC },
        {       APICINTS + 119, GATE_386INT,    ivctD7,         GATE_KACC },
        {       APICINTS + 120, GATE_386INT,    ivctD8,         GATE_KACC },
        {       APICINTS + 121, GATE_386INT,    ivctD9,         GATE_KACC },
        {       APICINTS + 122, GATE_386INT,    ivctDA,         GATE_KACC },
        {       APICINTS + 123, GATE_386INT,    ivctDB,         GATE_KACC },
        {       APICINTS + 124, GATE_386INT,    ivctDC,         GATE_KACC },
        {       APICINTS + 125, GATE_386INT,    ivctDD,         GATE_KACC },
        {       APICINTS + 126, GATE_386INT,    ivctDE,         GATE_KACC },
        {       APICINTS + 127, GATE_386INT,    ivctDF,         GATE_KACC },
        {       APICINTS + 128, GATE_386INT,    ivctE0,         GATE_KACC },
        {       APICINTS + 129, GATE_386INT,    ivctE1,         GATE_KACC },
        {       APICINTS + 130, GATE_386INT,    ivctE2,         GATE_KACC },
        {       APICINTS + 131, GATE_386INT,    ivctE3,         GATE_KACC },
        {       APICINTS + 132, GATE_386INT,    ivctE4,         GATE_KACC },
        {       APICINTS + 133, GATE_386INT,    ivctE5,         GATE_KACC },
        {       APICINTS + 134, GATE_386INT,    ivctE6,         GATE_KACC },
        {       APICINTS + 135, GATE_386INT,    ivctE7,         GATE_KACC },
        {       APICINTS + 136, GATE_386INT,    ivctE8,         GATE_KACC },
        {       APICINTS + 137, GATE_386INT,    ivctE9,         GATE_KACC },
        {       APICINTS + 138, GATE_386INT,    ivctEA,         GATE_KACC },
        {       APICINTS + 139, GATE_386INT,    ivctEB,         GATE_KACC },
        {       APICINTS + 140, GATE_386INT,    ivctEC,         GATE_KACC },
        {       APICINTS + 141, GATE_386INT,    ivctED,         GATE_KACC },
        {       APICINTS + 142, GATE_386INT,    ivctEE,         GATE_KACC },
        {       APICINTS + 143, GATE_386INT,    ivctEF,         GATE_KACC },
        {       240,            GATE_386INT,    ivctF0,         GATE_KACC },
        {       241,            GATE_386INT,    ivctF1,         GATE_KACC },
        {       242,            GATE_386INT,    ivctF2,         GATE_KACC },
        {       243,            GATE_386INT,    ivctF3,         GATE_KACC },
        {       255,            GATE_386INT,    apic_ivctSTRAY, GATE_KACC },
        { 0 },
};
#define str(s)  #s
#define xstr(s) str(s)

/*
 * void
 * psm_do_softint(void)
 *
 * Calling/Exit State:
 *	Called if user preemption flag is set.
 *
 * Remarks:
 *	User preemption flag is set if either of the following events
 *	are pending:
 *		- streams service procedures
 *		- local callouts
 *		- global callouts
 *		- runrun flag is set
 */
void
psm_do_softint(void)
{
        asm("int  $" xstr(SOFTINT));
}

/*
 * void
 * psm_configure(void)
 *
 *      Gather platform specific configuration info. for kernel.
 *
 * Calling/Exit State:
 *
 *      Assumes system is only running on the boot processor.
 *      No return value.
 */
void
psm_configure(void)
{
}

/*
 * array to keep track of logical engine number and physical engine.
 */
int 	ltop_mapping[MAXNUMCPU];

/*
 * int
 * psm_numeng(void)
 *
 * Calling/Exit State:
 *      Returns number of engines available in the system
 *      It is used by conf_proc() to calloc space for engine
 *      structures.
 */
int
psm_numeng(void)
{
        unsigned long master, slaves;
	int engcnt, i;

#ifndef	UNIPROC	
	if (is_gemstone) {
 		slaves = *(unsigned long *)(KVPAGE0 + SLAVEID_LOC);
 		master = *(unsigned long *)(KVPAGE0 + MASTERID_LOC);
		engcnt = 0;
		ltop_mapping[engcnt++] = master;
		for (i = 1 ; i ; i <<= 1)
			if ((i & slaves) && (i != master))
				ltop_mapping[engcnt++] = i;
	} else
#endif	/* UNIPROC */
		engcnt = 1;
	return(engcnt);
}

/*
 * void
 * psm_intr_init(void)
 *
 * Calling/Exit State:
 *      Called when the system is being initialized.
 */
void
psm_intr_init(void)
{
	if (is_gemstone)
		apicinit();
	else
		picinit();
}

/*
 * void
 * psm_intr_start(void)
 *
 * Calling/Exit State:
 *      Called when the processor is being onlined.
 */
void
psm_intr_start(void)
{
	if (is_gemstone)
		apicstart();
	else
		picstart();
}

/*
 * void
 * psm_timer_init(void)
 *
 * Calling/Exit State:
 *      None.
 */
void
psm_timer_init(void)
{
	clkstart();
}


/*
 * int
 * psm_doarg(char *s) 
 *	psm specific boot argument parsing
 *
 * Calling/Exit State:
 *	This routine is called after first checking an argument
 *	against the standard set of supported arguments.  Unknown
 *	(non-standard) arguments will be passed to this routine.
 *
 *	Return Values:	 0 if argument handled
 *			-1 if argument unknown
 */	 
psm_doarg(char *s)
{
	return(-1);		/* unknown argument */
}

/*
 * void
 * psm_reboot(int)
 *
 * Calling/Exit State:
 *	Intended use for flag values:
 * 		flag == 0	halt the system and wait for user interaction
 * 		flag != 0	automatic reboot, no user interaction required
 */
void
psm_reboot(int flag)
{
	if (flag)	
		cmn_err(CE_CONT, "\nAutomatic Boot Procedure\n");
	else
		cmn_err(CE_CONT, "Reboot the system now.\n");
	nf_reboot(flag);
}

/*
 * void
 * psm_intron(int iv, pl_t level, int engnum, int intmp, int itype)
 *	Unmask the interrupt vector from the iplmask.
 *
 * Calling/Exit State:
 *	iv is the interrupt request no. that needs to be enabled.
 *	engnum is the engine on which the interrupt must be unmasked.
 *	level is the interrupt priority level of the iv.
 *	itype is the type of interrupt.
 */
void
psm_intron(int iv, pl_t level, int engnum, int intmp, int itype)
{
	if (is_gemstone)
		apic_nenableint(iv, level, engnum, intmp, itype);
	else
		pic_nenableint(iv, level, engnum, intmp, itype);
}


/*
 * void
 * psm_introff(int iv, pl_t level, int engnum, int itype)
 *	Mask the interrupt vector in the iplmask of the engine currently
 *	running on.
 *
 * Calling/Exit State:
 *	iv is the interrupt that needs to be disabled.
 *	engnum is the engine on which the interrupt must be masked.
 *	level is the interrupt priority level of the iv.
 *	itype is the type of interrupt.
 */
void
psm_introff(int iv, pl_t level, int engnum, int itype)
{
	if (is_gemstone)
		apic_ndisableint(iv, level, engnum, itype);
	else
		pic_ndisableint(iv, level, engnum, itype);
}

/*
 * void
 * softreset(void)
 *	Indicate that the subsequent reboot is a "soft" reboot.
 *
 * Calling/Exit State:
 *	None.
 */
void
softreset(void)
{
}

/*
 * struct idt_init *
 * psm_idt(int engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 *	It also detects the machine type.
 *
 * Remarks:
 *      This routine must be called before loading the IDTR from selfinit().
 *      This routine must also be called before is_gemstone is referenced.
 */
struct idt_init *
psm_idt(int engnum)
{
	extern struct idt_init *apic_idt(), *pic_idt();

	if (gem_detect())
		is_gemstone = 1;
	return(nf_idt_init);
}

/*
 * void
 * psm_online_engine(int engnum, paddr_t startaddr, int flags)
 *
 * Calling/Exit State:
 *	onoff mutex lock is held on entry/exit.
 *
 * Remarks:
 *	online_kl1pt is used by online_engine() to communicate to reset_code()
 *	the physical address of the newly onlined engine's level 1 page table.
 *
 *	online_engno is used by online_engine() to communicate to reset_code()
 *	the logical engine number (engine[] index) for the newly onlined engine.
 *
 *	They are mutexed by onoff_mutex.
 *
 *	They are statically initialized merely to force them into kernel static
 *	data rather than BSS; this forces the bootstrap loader to load them into
 *	physical memory such that reset_code() can calculate their physical
 *	addresses by subtracting KVSBASE from their virtual addresses.
 */
void
psm_online_engine(int engnum, paddr_t startaddr, int flags)
{
	if (flags != WARM_ONLINE) {
 		*(paddr_t *)(KVPAGE0 + START_ADDR_LOC) = startaddr;
 		*(int *)(KVPAGE0 + START_ID_LOC) = ltop_mapping[engnum];
	}
}

/*
 * void
 * psm_selfinit(void)
 *	Performs any necessay per-engine initialization.
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is brought online
 *	for the very first time.
 *
 * Note:
 *      The internal on-chip cache is already enabled for the boot engine.
 */
void
psm_selfinit(void)
{
}

/*
 * void
 * psm_misc_init(void)
 *      Performs any necessay per-engine initialization.
 *
 * Calling/Exit State:
 *      Called at the end of selfinit() when the engine is brought online
 *      for the very first time.
 */
/* ARGSUSED */
void
psm_misc_init(void)
{
}

/* ARGSUSED */
void
psm_offline_self(void)
{
}

/*
 * void
 * psm_send_xintr(int)
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_send_xintr(int engnum)
{
	if (is_gemstone)
		apic_xmsg2(0xF0, engnum);
}

/*
 * void
 * psm_intr(uint vec, uint oldpl, uint edx, uint ecx,
 *               uint eax, uint es, uint ds, uint eip, uint cs)
 *	Interrupt handler for cross-processor interrupt.
 *
 * Calling/Exit State:
 *	this routine is called at PLHI.
 *
 * Description:
 *	The cross-processor interrupt is used to signal soft interrupts
 *	as well as xcall interrupts.  Thus, once psm_intr determines that
 *	the interrupt was actually a cross-processor interrupt,
 *	it does several steps:
 *		(1) Call xcall_intr to handle a pending xcall if present.
 *		(2) Copy new soft interrupt flags (if any) to l.eventflags.
 *
 * Remarks:
 *	Note that the same interrupt is used for both soft interrupts and
 *	xcall interrupts.  There are a number of races possible, but these
 *	are all handled correctly because both the soft interrupt handler
 *	and the xcall handler are implemented such that they effectively
 *	do nothing if there is nothing to do.
 */
void
psm_aintr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs)
{
#ifndef	UNIPROC	
	extern void xcall_intr(void);

	/*
	 * xcall_intr() is the handler for tlb flush.
	 */
	xcall_intr();  
	/*
	 * we use the same interrupt for software interrupt for now.
	 */
	if (psm_eventflags[l.eng_num] != 0)
		l.eventflags |= atomic_fnc(&psm_eventflags[l.eng_num]);
#endif	/* UNIPROC */
}

psm_intr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs)
{
}

/*
 * void
 * psm_sendsoft(engine_t *engp, int arg)
 *	Post SW interrupt to (typically) another processor.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Post a software interrupt to a processor with the specified flags.
 *	The implementation uses the static array psm_eventflags; each entry
 *	in psm_eventflags corresponds to an engine and acts as a mailbox for
 *	soft interrupts to that engine.  A soft interrupt is sent by:
 *		(1) Atomically or'ing the specified flags into the
 *			psm_eventflags entry for the engine (using
 *			a bus lock prefix with an OR instruction)
 *		(2) Sending a cross-processor interrupt (directly through
 *			psm_send_xintr, not via xcall)
 *
 * Remarks:
 *	Since sendsoft is sometimes called with an fspinlock held, we
 *	can't acquire locks while doing a sendsoft.  Thus, sendsoft does
 *	not use the xcall mechanism, and it uses bus locking rather than
 *	a software lock to update psm_eventflags atomically.
 */

void
psm_sendsoft(engine_t *engp, uint_t arg)
{
        if (engp == l.eng)
                l.eventflags |= arg;
        else {
                int engnum = engp - engine;

                atomic_or(&psm_eventflags[engnum], arg);
                psm_send_xintr(engnum);
        }
}

/*
 * Currently not Supported.
 */
ulong_t
psm_usec_time(void)
{
	extern ulong_t pit_usec_time(void);

	return pit_usec_time();
}

/*
 * void
 * psm_ledctl(led_request_t, uint_t)
 *
 * Calling/Exit State:
 *      None.
 *
 * Remarks:
 *      Noop for UP.
 */
/* ARGSUSED */
void
psm_ledctl(led_request_t req, uint_t led_bits)
{
}

/*
 * void
 * psm_lwp_resume(lwp_t, lwp_t, user_t)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Perform platform specific function when resumming a suspended
 *	lwp.
 */
/* ARGSUSED */
void
psm_lwp_resume(lwp_t *new_lwp, lwp_t *old_lwp, user_t *new_userp)
{
}

/*
 * void
 * psm_panic()
 *      Dump platform specific information to memory upon system panicing.
 *
 * Calling/Exit State:
 *      None.
 */
void
psm_panic()
{
}
