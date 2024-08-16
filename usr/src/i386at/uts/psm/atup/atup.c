/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/atup/atup.c	1.4.1.9"
#ident	"$Header: $"

/*
 * AT PSM/mHAL hardware-specific routines and data.
 *
 * This file contains a set of subroutines and a set of data, all of which
 * are used elsewhere in the kernel but which vary for MP hardware from
 * different vendors. In general, all the routines with an "psm_" prefix
 * should be defined for whatever hardware you use, even if the routine does
 * nothing. The same applies for data (unfortunately the data names may not
 * have a distinctive prefix), even if it might not be used. Some routines
 * and data defined in this file are for specific platforms only. Vendors
 * do not need to support these items for their own hardware.
 *
 * The requirements for each item in this file are clearly stated in the
 * PSM/mHAL interface specification document.
 *
 * This PSM/mHAL file supports the uniprocessor system.
 */

#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/cram/cram.h>
#include <mem/immu.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/bootinfo.h>
#include <svc/errno.h>
#include <svc/intr.h>
#include <svc/pic.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#include <io/ddi.h>	/* Must come after other kernel headers */
#include <io/f_ddi.h>
#include <io/ddi_i386at.h>


#define SOFTINT 63              /* software interrupt */

extern void softint(void);
extern void devint0(void);
extern void devint1(void);
extern void devint2(void);
extern void devint3(void);
extern void devint4(void);
extern void devint5(void);
extern void devint6(void);
extern void devint7(void);
extern void devint8(void);
extern void devint9(void);
extern void devint10(void);
extern void devint11(void);
extern void devint12(void);
extern void devint13(void);
extern void devint14(void);
extern void devint15(void);
extern void devint16(void);
extern void devint17(void);
extern void devint18(void);
extern void devint19(void);
extern void devint20(void);
extern void devint21(void);
extern void devint22(void);
extern void devint23(void);
extern void devint24(void);
extern void devint25(void);
extern void devint26(void);
extern void devint27(void);
extern void devint28(void);
extern void devint29(void);
extern void devint30(void);
extern void devint31(void);
extern void devint32(void);
extern void devint33(void);
extern void devint34(void);
extern void devint35(void);
extern void devint36(void);
extern void devint37(void);
extern void devint38(void);
extern void devint39(void);
extern void devint40(void);
extern void devint41(void);
extern void devint42(void);
extern void devint43(void);
extern void devint44(void);
extern void devint45(void);
extern void devint46(void);
extern void devint47(void);
extern void devint48(void);
extern void devint49(void);
extern void devint50(void);
extern void devint51(void);
extern void devint52(void);
extern void devint53(void);
extern void devint54(void);
extern void devint55(void);
extern void devint56(void);
extern void devint57(void);
extern void devint58(void);
extern void devint59(void);
extern void devint60(void);
extern void devint61(void);
extern void devint62(void);
extern void devint63(void);
extern void devint64(void);
extern void devint65(void);
extern void devint66(void);
extern void devint67(void);
extern void devint68(void);
extern void devint69(void);
extern void devint70(void);
extern void devint71(void);

#define	IO_ROM_INIT	0xfff0
#define	IO_ROM_SEG	0xf000
#define	RESET_FLAG	0x1234
#define	SHUT_DOWN	0x8f
#define	SHUT5		5
#define	STAT_8042	0x64
#define I8042_RESETCPU	0xfe
#define FPU_BUSY_LATCH	0xf0

void	softreset(void);
int	atup_nmi(void);

STATIC void psm_clr_fpbusy(void);


extern boolean_t fpu_external;

/*
 * Interrupt distribution table (IDT).
 */
struct idt_init idt0_init[] = {
	{	SOFTINT,	GATE_386INT,	softint,	GATE_KACC },
	{	DEVINTS,	GATE_386INT,	devint0,	GATE_KACC },
	{	DEVINTS + 1,	GATE_386INT,	devint1,	GATE_KACC },
	{	DEVINTS + 2,	GATE_386INT,	devint2,	GATE_KACC },
	{	DEVINTS + 3,	GATE_386INT,	devint3,	GATE_KACC },
	{	DEVINTS + 4,	GATE_386INT,	devint4,	GATE_KACC },
	{	DEVINTS + 5,	GATE_386INT,	devint5,	GATE_KACC },
	{	DEVINTS + 6,	GATE_386INT,	devint6,	GATE_KACC },
	{	DEVINTS + 7,	GATE_386INT,	devint7,	GATE_KACC },
	{	DEVINTS + 8,	GATE_386INT,	devint8,	GATE_KACC },
	{	DEVINTS + 9,	GATE_386INT,	devint9,	GATE_KACC },
	{	DEVINTS + 10,	GATE_386INT,	devint10,	GATE_KACC },
	{	DEVINTS + 11,	GATE_386INT,	devint11,	GATE_KACC },
	{	DEVINTS + 12,	GATE_386INT,	devint12,	GATE_KACC },
	{	DEVINTS + 13,	GATE_386INT,	devint13,	GATE_KACC },
	{	DEVINTS + 14,	GATE_386INT,	devint14,	GATE_KACC },
	{	DEVINTS + 15,	GATE_386INT,	devint15,	GATE_KACC },
	{	DEVINTS + 16,	GATE_386INT,	devint16,	GATE_KACC },
	{	DEVINTS + 17,	GATE_386INT,	devint17,	GATE_KACC },
	{	DEVINTS + 18,	GATE_386INT,	devint18,	GATE_KACC },
	{	DEVINTS + 19,	GATE_386INT,	devint19,	GATE_KACC },
	{	DEVINTS + 20,	GATE_386INT,	devint20,	GATE_KACC },
	{	DEVINTS + 21,	GATE_386INT,	devint21,	GATE_KACC },
	{	DEVINTS + 22,	GATE_386INT,	devint22,	GATE_KACC },
	{	DEVINTS + 23,	GATE_386INT,	devint23,	GATE_KACC },
	{	DEVINTS + 24,	GATE_386INT,	devint24,	GATE_KACC },
	{	DEVINTS + 25,	GATE_386INT,	devint25,	GATE_KACC },
	{	DEVINTS + 26,	GATE_386INT,	devint26,	GATE_KACC },
	{	DEVINTS + 27,	GATE_386INT,	devint27,	GATE_KACC },
	{	DEVINTS + 28,	GATE_386INT,	devint28,	GATE_KACC },
	{	DEVINTS + 29,	GATE_386INT,	devint29,	GATE_KACC },
	{	DEVINTS + 30,	GATE_386INT,	devint30,	GATE_KACC },
	{	DEVINTS + 31,	GATE_386INT,	devint31,	GATE_KACC },
	{	DEVINTS + 32,	GATE_386INT,	devint32,	GATE_KACC },
	{	DEVINTS + 33,	GATE_386INT,	devint33,	GATE_KACC },
	{	DEVINTS + 34,	GATE_386INT,	devint34,	GATE_KACC },
	{	DEVINTS + 35,	GATE_386INT,	devint35,	GATE_KACC },
	{	DEVINTS + 36,	GATE_386INT,	devint36,	GATE_KACC },
	{	DEVINTS + 37,	GATE_386INT,	devint37,	GATE_KACC },
	{	DEVINTS + 38,	GATE_386INT,	devint38,	GATE_KACC },
	{	DEVINTS + 39,	GATE_386INT,	devint39,	GATE_KACC },
	{	DEVINTS + 40,	GATE_386INT,	devint40,	GATE_KACC },
	{	DEVINTS + 41,	GATE_386INT,	devint41,	GATE_KACC },
	{	DEVINTS + 42,	GATE_386INT,	devint42,	GATE_KACC },
	{	DEVINTS + 43,	GATE_386INT,	devint43,	GATE_KACC },
	{	DEVINTS + 44,	GATE_386INT,	devint44,	GATE_KACC },
	{	DEVINTS + 45,	GATE_386INT,	devint45,	GATE_KACC },
	{	DEVINTS + 46,	GATE_386INT,	devint46,	GATE_KACC },
	{	DEVINTS + 47,	GATE_386INT,	devint47,	GATE_KACC },
	{	DEVINTS + 48,	GATE_386INT,	devint48,	GATE_KACC },
	{	DEVINTS + 49,	GATE_386INT,	devint49,	GATE_KACC },
	{	DEVINTS + 50,	GATE_386INT,	devint50,	GATE_KACC },
	{	DEVINTS + 51,	GATE_386INT,	devint51,	GATE_KACC },
	{	DEVINTS + 52,	GATE_386INT,	devint52,	GATE_KACC },
	{	DEVINTS + 53,	GATE_386INT,	devint53,	GATE_KACC },
	{	DEVINTS + 54,	GATE_386INT,	devint54,	GATE_KACC },
	{	DEVINTS + 55,	GATE_386INT,	devint55,	GATE_KACC },
	{	DEVINTS + 56,	GATE_386INT,	devint56,	GATE_KACC },
	{	DEVINTS + 57,	GATE_386INT,	devint57,	GATE_KACC },
	{	DEVINTS + 58,	GATE_386INT,	devint58,	GATE_KACC },
	{	DEVINTS + 59,	GATE_386INT,	devint59,	GATE_KACC },
	{	DEVINTS + 60,	GATE_386INT,	devint60,	GATE_KACC },
	{	DEVINTS + 61,	GATE_386INT,	devint61,	GATE_KACC },
	{	DEVINTS + 62,	GATE_386INT,	devint62,	GATE_KACC },
	{	DEVINTS + 63,	GATE_386INT,	devint63,	GATE_KACC },
	{	DEVINTS + 64,	GATE_386INT,	devint64,	GATE_KACC },
	{	DEVINTS + 65,	GATE_386INT,	devint65,	GATE_KACC },
	{	DEVINTS + 66,	GATE_386INT,	devint66,	GATE_KACC },
	{	DEVINTS + 67,	GATE_386INT,	devint67,	GATE_KACC },
	{	DEVINTS + 68,	GATE_386INT,	devint68,	GATE_KACC },
	{	DEVINTS + 69,	GATE_386INT,	devint69,	GATE_KACC },
	{	DEVINTS + 70,	GATE_386INT,	devint70,	GATE_KACC },
	{	DEVINTS + 71,	GATE_386INT,	devint71,	GATE_KACC },
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
/* ARGSUSED */
int
psm_doarg(char *s)
{
	return -1;		/* unknown argument */
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
 * void
 * psmreboot(int)
 *
 * Calling/Exit State:
 *	Intended use for flag values:
 *		flag != 1	halt the system and wait for user interaction
 *		flag == 1	automatic reboot, no user interaction required
 */
void
psm_reboot(int flag)
{
	extern int i8042_write(uchar_t, uchar_t);
        uint_t  bustype;

	splhi();

	/*
	 * If sanity timer in use, turn off to allow clean soft reboot. 
	 */
        drv_gethardware(IOBUS_TYPE, &bustype);
        if (bustype & BUS_EISA)
		eisa_sanity_halt();

	reboot_prompt(flag);

	softreset();
	i8042_write(STAT_8042, I8042_RESETCPU);	/* trigger reboot */

	for (;;) {
		asm("   cli     ");
		asm("   hlt     ");
	}
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
	/* do soft reboot; only do memory check after power-on */


	*((ulong_t *)&kvpage0[0x467]) = ((ulong_t)IO_ROM_SEG << 16) |
					 (ulong_t)IO_ROM_INIT;
	*((ushort_t *)&kvpage0[0x472]) = RESET_FLAG;

	/* set shutdown flag to reset using int 19 */
	outb(CMOS_ADDR, SHUT_DOWN);
	outb(CMOS_DATA, SHUT5);
}


/*
 * struct idt_init *
 * psm_idt(uint_t engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 *
 * Remarks:
 *	This routine must be called before init_desc_tables() is called
 *	from selfinit().
 */
/* ARGSUSED */
struct idt_init *
psm_idt(int engnum)
{
	ASSERT(engnum == BOOTENG);

	return idt0_init;
}


/*
 * void
 * psm_intr_init(void)
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 */
void
psm_intr_init(void)
{
	picinit();
}

/*
 * MACRO
 * SET_CM_ARGS(struct cm_args *cma, rm_key_t key,
 *                              void *val, int vlen, int n)
 *	Set data fields of <cma>.
 *
 * Calling/Exit State:
 *	None.
 */
#define SET_CM_ARGS(cma, param, key, val, vlen, n) { \
	(cma)->cm_param = (param); \
	(cma)->cm_key = (key); \
	(cma)->cm_val = (val); \
	(cma)->cm_vallen = (vlen); \
	(cma)->cm_n = (n); \
}


/*
 * void
 * psm_intr_start(void)
 *
 * Calling/Exit State:
 *	Called when the processor is being onlined.
 *
 * Description:
 *	Register the resources for the floating-point interrupt handler.
 *	We also need to delete the resources because we may remove a
 *	floating-point chip or upgrade the motherboard to i486 before
 *	the next boot and may not find the fpu_external to be set. In
 *	such a scenario we need to remove the resources allocated for
 *	FPU interrupts because another device at a different IPL can
 *	claim the resources.
 *
 * Note:
 *	When deleteing the resources, do not reset the intpri table
 *	because there is no way to determine if irq 13 is not being
 *	shared by any other device. On the other hand, when we add the
 *	resources initialize the intpri table regardless of whether
 *	irq 13 is being shared or not.
 */
void
psm_intr_start(void)
{
	extern uchar_t intpri[];
	extern void (*ivect[])();
	extern void intnull();
	extern void psm_intr();

	
	if (fpu_external) {
		int	num;
		int	i;
		int	rv;
		cm_args_t cma;
		cm_num_t val;
		rm_key_t key;
		rm_key_t fkey;
		boolean_t flg = B_FALSE;	/* add/del irq 13 resources */

		num = cm_getnbrd("atup");
		for (i = 0; i < num; i++) {
			key = cm_getbrdkey("atup", i);
			SET_CM_ARGS(&cma, CM_IRQ, key, &val, sizeof(val), 0);
			if ((rv = cm_getval(&cma)) == 0) {
				if (val == 13) {
					flg = B_FALSE;
					break;
				}
			} else if (rv == ENOENT && flg == B_FALSE) {
				/* Save the first key without a CM_IRQ param. */
				fkey = cma.cm_key;
				flg = B_TRUE;
			}
		}

		if (flg == B_TRUE) {
			if ((rv = cm_AT_putconf(fkey, 13, 3, 0, 0, 0, 0, 0, 
					CM_SET_IRQ|CM_SET_ITYPE, 0)) != 0) {
				/*
				 *+ Failed to register irq 13 with the in-core
				 *+ resource manager (resmgr) database.
				 */
				cmn_err(CE_WARN,
					"psm_intr_start: cm_AT_putconf "
					"failed with rv = 0x%x", rv);
			}

			val = plhi + 1;
			SET_CM_ARGS(&cma, CM_IPL, fkey, &val, sizeof(val), 0);
			if ((rv = cm_addval(&cma)) != 0) {
				/*
				 *+ Failed to register irq 13 ipl with
				 *+ in-core resource manager (resmgr).
				 */
				cmn_err(CE_WARN,
					"psm_intr_start: cm_addval (ipl) "
					"failed with rv = 0x%x", rv);
			}

			/*
			 * KLUDGE: Adjust the interrupt data structures 
			 * (intpri and ivect table) so that interrupt 13
			 * can be attached/enabled via picstart. 
			 */
			intpri[13] = plhi + 1;
			ivect[13] = psm_intr;
		}
	} else {
		int	num;
		int	i;
		int	rv;
		cm_args_t cma;
		cm_num_t val;
		rm_key_t key;
		boolean_t flg = B_FALSE;

		num = cm_getnbrd("atup");
		for (i = 0; i < num; i++) {
			key = cm_getbrdkey("atup", i);
			SET_CM_ARGS(&cma, CM_IRQ, key, &val, sizeof(val), 0);
			if ((rv = cm_getval(&cma)) == 0) {
				if (val == 13) {
					flg = B_TRUE;
					break;
				}
			} 
		}

		if (flg == B_TRUE) {
			/*
			 * Remove CM_IRQ param. Note that the arguments are 
			 * already set.
			 */
			if ((rv = cm_delval(&cma)) != 0) {
				/*
				 *+ Failed to delete irq 13 from the 
				 *+ in-core resource manager (resmgr).
				 */
				cmn_err(CE_WARN,
					"psm_intr_start: cm_delval (irq) failed.");
			}

			SET_CM_ARGS(&cma, CM_ITYPE, key, &val, sizeof(val), 0);
			if ((rv = cm_delval(&cma)) != 0) {
				/*
				 *+ Failed to delete irq 13 itype from the 
				 *+ in-core resource manager (resmgr).
				 */
				cmn_err(CE_WARN,
					"psm_intr_start: cm_delval (itype) failed.");
			}

			SET_CM_ARGS(&cma, CM_IPL, key, &val, sizeof(val), 0);
			if ((rv = cm_delval(&cma)) != 0) {
				/*
				 *+ Failed to delete irq 13 ipl from the
				 *+ in-core resource manager (resmgr).
				 */
				cmn_err(CE_WARN,
					"psm_intr_start: cm_delval (ipl) failed.");
			}

			/*
			 * KLUDGE: Adjust the interrupt data structures 
			 * (intpri and ivect table) so that interrupt 13
			 * is NOT attached/enabled via picstart. 
			 */
			intpri[13] = 0;
			ivect[13] = intnull;
		}
	}

	picstart();
}


/*
 * int
 * psm_numeng(void)
 *
 * Calling/Exit State:
 *	Returns number of engines available in the system
 *	It is used by conf_proc() to calloc space for engine
 *	structures.
 */
int
psm_numeng(void)
{
	/* number of engines equal 1 */
	return 1;
}


/*
 * void
 * psm_configure(void)
 *	Gather miscellaneous hardware configuration information.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	No-op for UP.
 */
void
psm_configure(void)
{
	psm_clr_fpbusy();	/* TEMP; until sysinit reorg */
}


/*
 * void
 * psm_online_engine(int engnum, paddr_t startaddr, int flags)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Should never be called for UP.
 */
/* ARGSUSED */
void
psm_online_engine(int engnum, paddr_t startaddr, int flags)
{
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
        psm_clr_fpbusy();
}

#define	E8R1_PRODNUM	0x49e8	/* product id number for E8R1 EISA system */

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
	uint_t	bustype;
	int	eisa_brd_id;
	extern boolean_t soft_sysdump;

        ASSERT(myengnum == BOOTENG);

	/*
	 * Initialize fail-safe timer or sanity clock.
	 */
        drv_gethardware(IOBUS_TYPE, &bustype);
        if (bustype & BUS_EISA)
		eisa_sanity_init();

	if (drv_gethardware(EISA_BRDID, &eisa_brd_id) == -1)
		eisa_brd_id = -1; 

	if (eisa_brd_id == E8R1_PRODNUM)
		/* Register the NMI handler */
		drv_callback(NMI_ATTACH, atup_nmi, 0);

        psm_clr_fpbusy();
	soft_sysdump = B_TRUE;
}


/*
 * void
 * psm_offline_self(void)
 *      Performs any necessay actions before offline itself.
 *
 * Calling/Exit State:
 *      Called in offline_self() right before getting into the offline
 *      while loop.
 *
 * Remarks:
 *      Noop for UP.
 *
 */
/* ARGSUSED */
void
psm_offline_self(void)
{
}


/*
 * void
 * psm_clear_xintr(int)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Should never be called for UP.
 */
/* ARGSUSED */
void
psm_clear_xintr(int engnum)
{
	ASSERT(engnum == myengnum);
}


/*
 * void
 * psm_send_xintr(int)
 *	Assert the cross-processor interrupt bit in the processor
 *	control port.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Should never be called for UP.
 */
/* ARGSUSED */
void
psm_send_xintr(int engnum)
{
}


/*
 * STATIC void
 * psm_clr_fpbusy(void)
 *	Clear FPU BUSY latch.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
psm_clr_fpbusy(void)
{
	/*
	 * The coprocessor's busy line is only held active while actually
	 * executing, but hardware that sits between the main processor
	 * and the coprocessor latches the coprocessor error output and
	 * uses this to feed the coprocessor busy input to the CPU.
	 * (This is an 8086/8087 PC compatibility configuration.)
	 *
	 * Since this busy line prevents normal coprocessor communication,
	 * we must clear the latch before attempting to examine the FPU
	 * status word.  Otherwise, the CPU would hang on any FPU access.
	 */
	outb(FPU_BUSY_LATCH, 0);	/* Clear FPU BUSY latch */
}


/*
 * void
 * psm_intr(uint vec, uint oldpl, uint edx, uint ecx,
 *               uint eax, uint es, uint ds, uint eip, uint cs)
 *	Interrupt handler for IRQ 13 (FPU error).
 *
 * Calling/Exit State:
 *	Called from H/W interrupt.
 */
/* ARGSUSED */
void
psm_intr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs)
{
	/*
	 * Process floating-point interrupts by first verifying that
	 * there is an external FPU chip present, since FPU chips
	 * embedded in the processor or emulated do not generate FPU 
	 * interrupts.
	 */
	if (!(fpu_external))
		return;

	/*
	 * Handle an FPU error interrupt:
	 *
	 * First, clear any exception.
	 */
	psm_clr_fpbusy();

	fpu_error();
}


/*
 * void
 * psm_sendsoft(int engnum, uint_t arg)
 *	UP version of sendsoft; just post event locally.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
psm_sendsoft(int engnum, uint_t arg)
{
        ASSERT(engnum == myengnum);

	engine_evtflags |= arg;
}


/*
 * ulong_t
 * psm_time_get(psmtime_t *)
 *
 * Calling/Exit State:
 *	- save the current time stamp that is opaque to the base kernel
 *	  in psmtime_t.	  
 */

void
psm_time_get(psmtime_t *ptime)
{
	extern ulong_t pit_usec_time(void);

        ptime->pt_lo = pit_usec_time();
        ptime->pt_hi = 0;
}


/*
 * subtracts the time stamp `src' from `dst', and stores the result in `dst'.
 */
void
psm_time_sub(psmtime_t *dst, psmtime_t *src)
{
        dst->pt_lo -= src->pt_lo;
}


/*
 * add the time stamp `src' to `dst', and stores the result in `dst'.
 */
void
psm_time_add(psmtime_t *dst, psmtime_t *src)
{
        dst->pt_lo += src->pt_lo;
}

/*
 * Convert the opaque time stamp to micro second.
 */
void
psm_time_cvt(dl_t *dst, psmtime_t *src)
{
        dst->dl_lop = src->pt_lo;
        dst->dl_hop = (long)src->pt_hi;
}


/*
 * void
 * psm_ledctl(led_request_t, uint_t)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Noop for UP.
 */
/* ARGSUSED */
void
psm_ledctl(led_request_t req, uint_t led_bits)
{
}


extern void nenableint(int, pl_t, int, int, int);
extern void ndisableint(int, pl_t, int, int);

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
	nenableint(iv, level, engnum, intmp, itype);
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
	ndisableint(iv, level, engnum, itype);
}


#define PORT_B		0x61	/* System Port B */
#define IOCHK_DISABLE	0x08	/* Disable I/O CH CK */
#define PCHK_DISABLE	0x04	/* Disable motherboard parity check */
#define IOCHK		0x40	/* I/O CH CK */
#define PCHK		0x80	/* Motherboard parity check */

#define CMOS_PORT	0x70	/* CMOS Port */
#define NMI_ENABLE	0x0F	/* Enable NMI interrupt */
#define NMI_DISABLE	0x8F	/* Disable NMI interrupt */
#define	NMI_MASK	0x0F	/* Mask NMI bits in status/control port */
#define	FAR_HIGH	0x844	/* high bits of Fault Address Register */
#define	FAR_LOW		0x840	/* low bits of Fault Address Register */


/*
 * int 
 * atup_nmi(void)
 *	Check to see if accessing past equipped memory caused the nmi.
 *
 * Calling/Exit State:
 *	None.
 */
int
atup_nmi(void)
{
	int	i;
	ulong_t	far_addr = 0;
	uint_t	valid_flag = 0;
	uchar_t	byte;
	int	eisa_brd_id;


	/*
	 * First check to see if we are executing on a E8R1 system. 
	 */

	if (drv_gethardware(EISA_BRDID, &eisa_brd_id) == -1)
		eisa_brd_id = -1; 

	if (eisa_brd_id == E8R1_PRODNUM) {
		/*
		 * Obtain the address causing the NMI 
		 * from the Fault Address Register (FAR).
		 */
		far_addr = inw(FAR_HIGH);
		far_addr <<= 16;
		far_addr += inw(FAR_LOW);

		/*
		 * Search through memory to find if a valid address. 
		 */
		for (i = 0; i < bootinfo.memavailcnt; i++) {
			if ((far_addr >= bootinfo.memavail[i].base) &&
			    (far_addr < bootinfo.memavail[i].base
					+ bootinfo.memavail[i].extent)) {
				valid_flag = 1;
			}
		}

		if (valid_flag == 0) {
			/*
			 * Read the NMI status and control port.
			 */

			/* mask status bits */
			byte = (inb(PORT_B) & NMI_MASK);
			/* clear parity error */
			outb(PORT_B, byte | PCHK_DISABLE);
			/* re-enable parity */
			outb(PORT_B, byte & ~PCHK_DISABLE);
			cmn_err(CE_NOTE, 
				"!Accessing past equipped memory") ;
			return (NMI_BUS_TIMEOUT);
		}

		return (NMI_BENIGN);
	}

	return (NMI_UNKNOWN);
}

/*
 * boolean_t
 * psm_intrpend(pl_t pl)
 *	Check for pending interrupts above specified priority level.
 *
 * Calling/Exit State:
 *	None.
 * 
 * Description:
 *	Returns B_TRUE (B_FALSE) if there are (are not) any pending
 *	interrupts above the specified priority level.
 *
 * Remarks:
 *	picipl is generally set to the ipl of the highest priority
 *	pending interrupt on this processor; if picipl == PLBASE,
 *	then there are no pending interrupts on this processor.
 */
boolean_t
psm_intrpend(pl_t pl)
{
	extern pl_t picipl;

	return (picipl > pl);
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
