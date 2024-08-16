/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:svc/machdep.c	1.21"
#ident	"$Header: $"

/*
 * Highly Machine dependent routines.
 */

#include <fs/buf.h>
#include <io/SGSmem.h>
#include <io/SGSproc.h>
#include <io/bdp.h>
#include <io/cfg.h>
#include <io/clkarb.h>
#include <io/scan.h>
#include <io/slic.h>
#include <io/slicreg.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <svc/bic.h>
#include <svc/fp.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/kcontext.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#define	B1BUG21		/* 80386 B-1 Errata # 21 (see 9/1/87 errata) */

/*
 * void
 * hold_proc(int)
 * 	Assert FORCE HOLD on a processor.
 * 
 * Calling/Exit State:
 *	slicid - the SLIC ID of the SLIC of the processor.
 *	Returns: none
 *
 * 	No spin locks should be held by the caller.
 *
 * Description:
 *	Called to assert the FORCE HOLD bit of the External Control register
 *	of the SIC of the processor indicated by slicid.  The indicated 
 *	processor will not return until it has been "unheld()".
 *
 *	Assumes that the target processor is an SGS2 (i486).  The caller
 *	assures this.
 *	
 *	Caller must call with SPLHI.  No locks (or fspin locks) can be held
 *	by the target processor since it is about to go away (remember the
 *	target processor may be the caller).
 */

void
hold_proc(int slicid)
{
	unchar	regval;

	/*
	 * This mix of bits represents the state of the bits for "normal"
	 * operation, except that we have now set EXT_CTL1_FORCE_HOLD
	 * to cause ourselves to be held.
	 */
	regval = (EXT_CTL1_NO_486_PARITY_ERR | EXT_CTL1_NO_RESET_SLIC_ERR |
		  EXT_CTL1_NO_BUS_SCAN | EXT_CTL1_NORMAL_OSC | 
		  EXT_CTL1_NO_START_INVAL| EXT_CTL1_FORCE_HOLD);

	/*
	 * If the target processor is also the caller then let's
	 * be sure to use lwrslave/lrdslave instead of wrslave/rdslave.
	 * Otherwise, we would go away while holding the slic_mutex lock,
	 * causing all other processors to block.
	 */
	if (l.eng->e_slicaddr == slicid) {
		slic_lwrslave(slicid, EXT_CTL1, regval);
		while (slic_lrdslave(slicid, EXT_CTL0) & EXT_CTL0_NO_HOLD_ACK) 
			continue;
	} else {
		slic_wrslave(slicid, EXT_CTL1, regval);
		while (slic_rdslave(slicid, EXT_CTL0) & EXT_CTL0_NO_HOLD_ACK) 
			continue;
	}
}

/*
 * void
 * pause_self(void)
 *	Stop processor via pause self.
 *
 * Calling/Exit State:
 *	No parameters, returns: none.  Should be called at PLHI.
 *
 * Description:
 *	Typically called from NMI handler when a panic is in
 *	progress. 
 *	There is no return.
 */

void
pause_self(void)
{
	kcontext_t regs;
	extern void *saveregs(kcontext_t *);

	/*
	 * Save all registers.  Save "panic" sp in l.panicsp.
	 * Also save page-table in use by this processor when the
	 * system went down.
	 */

	l.panicsp = saveregs(&regs);
	l.panic_pt = READ_PTROOT();
	if (using_fpu)
		save_fpu();
 

	/*
	 * If fpa support is considered, need to save_fpa()
	 * here. See Dynix/ptx code.
	 */

	/*
	 * Turn off light show if enabled.
	 */

	if (light_show && fp_lights) {
		FP_LIGHTOFF(l.eng_num);
		*(int *) KVLED = 0;
	}

	/*
	 * Pause myself!
	 */

	l.eng->e_flags |= E_PAUSED;

	/*
	 * HOLD the processor, but don't reset it (also turn OFF led).
	 * Wait for processor to be HELD.  The while() will not actually
	 * ever finish. Call lwrslave and lrdslave so we don't go away
	 * with slic_mutex held.
	 *
	 * In this case, *don't* isolate the processor from the bus, since
	 * may want to take a dump.  Since pausing self, not clear we could
	 * isolate self from bus.
	 */

	if (l.eng->e_flags&E_SGS2) {
		hold_proc(l.eng->e_slicaddr);
	} else {
		slic_lwrslave(l.eng->e_slicaddr, PROC_CTL,
				PROC_CTL_LED_OFF | PROC_CTL_NO_NMI |
				PROC_CTL_NO_SSTEP | PROC_CTL_NO_RESET);
		while (slic_lrdslave(l.eng->e_slicaddr, PROC_STAT) &
			PROC_STAT_NO_HOLDA)
			continue;
	}
	slic_lwrslave(l.eng->e_slicaddr, PROC_CTL,
			PROC_CTL_LED_OFF | PROC_CTL_NO_NMI |
			PROC_CTL_NO_SSTEP | PROC_CTL_NO_RESET);

	while (slic_lrdslave(l.eng->e_slicaddr, PROC_STAT) & PROC_STAT_NO_HOLDA)
		continue;
}

/*
 * int
 * calc_delay(int)
 *	Returns a number that can be used in delay loops to wait
 *	for a small amount of time, such as 0.5 second.
 *
 * Calling/Exit State:
 *	Takes a single integer argument.
 *
 *	Returns: an integer whose value depends on the cpu speed.
 *
 * Description:
 *	Return a number to use in spin loops that takes into account
 *	both the cpu rate and the mip rating.
 *	If the machine is not yet up, then use cpurate otherwise
 *	use l.cpu_speed which will then be defined.
 */

int
calc_delay(int x)
{
	if (upyet == 0)
		return (x * cpurate);
	else
		return (x * l.cpu_speed);
}

/*
 * int buscheck(buf *bp)
 *	This platform does nothing with this. Other platforms
 *	may do something more useful.
 *
 * Calling/Exit State:
 *	Just return 0.
 */
/* ARGSUSED */
int
buscheck(struct buf *bp)
{
	return 0;
}


/*
 * void
 * splinit(void)
 *	Set the interrupt controller state such that all interrupts are
 *	disabled but the interrupt mask is set to PLBASE.
 *
 * Calling/Exit State:
 *	Called only at system intialization time.
 */

void
splinit(void)
{
	struct cpuslic *sl = (struct cpuslic *)KVSLIC;

	sl->sl_ictl = 0x00;
	sl->sl_procgrp = PROC_GROUP;
	slic_setgm(sl->sl_procid, SL_GM_ALLOFF);
	ASSERT(sl->sl_gmask == SL_GM_ALLOFF);

	(void) spl0();
}

/*
 * void
 * splstart(void)
 *	Set the interrupt controller state such that all interrupts are
 *	enabled and the interrupt mask is set to PLBASE.
 *
 * Calling/Exit State:
 *	Called only at per processor intialization time.
 */

void
splstart(void)
{
	struct cpuslic *sl = (struct cpuslic *)KVSLIC;

	ASSERT((sl->sl_ictl & (SL_HARDINT | SL_SOFTINT)) == 0);
	sl->sl_ictl = 0x00;
	sl->sl_procgrp = PROC_GROUP;
	slic_setgm(sl->sl_procid, SL_GM_ALLON);
	ASSERT(sl->sl_gmask == SL_GM_ALLON);

	(void) spl0();
}


/*
 * void access_error(int flags)
 *	Report an access error.
 *
 * Calling/Exit State:
 *	None.
 */
void
access_error(uint_t flags)
{
	cmn_err(CE_CONT, "access_error, flags=0x%x\n", flags);
}

/*
 * void sgs2_access_error(int flags)
 *	Report a MODELD access error.
 *
 * Calling/Exit State:
 *	None.
 */
void
sgs2_access_error(uint_t flags)
{
	cmn_err(CE_CONT, "sgs2_access_error, flags=0x%x\n", flags);
}

/*
 * void
 * nmi(...)
 *	Handle an NMI interrupt.
 * 
 * Calling/Exit State:
 *
 *	The arguments are the saved registers which will be restored
 *	on return from this routine.
 *
 * Description:
 *
 *	Panic: For the panic handling case, there are two cases,
 *	depending on whether the trap occurred while in user mode
 *	or kernel mode. If we entered this code from user mode,
 *	the slic address of the master is extracted and a check is
 *	made to see if the cause was access error of I/O space.
 *	If true, the access error is cleared, NMI reenabled,
 *	and the lwp is sent a SIGBUS signal. If not access error,
 *	it could be due to Bug-21 if we are using the B1 version
 *	of 386. If true, a SIGFPE is sent to the lwp. If not,
 *	we fall through to the case of entry from kernel mode.
 *	A check is made here for Bug-21 (handled as above), if not,
 *	it continues with normal processing of the exception.
 *
 *	Next, report reason for NMI (decode errors, etc) and stop the machine.
 *
 *	This function is responsible for invoking the cmn_err()
 *	function with the appropriate arguments depending on
 *	what caused the fault. The processor executing this function
 *	could have received the NMI due to a variety of reasons.
 *
 *	Currently, NMIs are presented to the processor in these situations:
 *
 *		- SLIC NMI (sent by software)
 *		- Access error from access to reserved processor
 *			LOCAL address
 *		- Access error:
 *			- Bus Timeout
 *			- ECC Uncorrectable error
 *			- Processor fatal error (SGS only)
 *		- Cache Parity error (these hold the processor & freeze
 *					the bus)
 *
 *		In all cases, the processor pauses itself.
 */

/* ARGSUSED */
void
nmi(volatile uint edi, volatile uint esi, volatile uint ebp,
    volatile uint unused, volatile uint ebx, volatile uint edx,
    volatile uint ecx, volatile uint eax, volatile uint es, volatile uint ds,
    volatile uint eip, volatile uint cs, volatile uint flags, volatile uint sp,
    volatile uint ss)
{
	int	sgs2 = l.eng->e_flags&E_SGS2;
	unchar	slicid;
	unchar	my_slic;
	unchar	bic_slic;
	unchar	accerr_reg;
	int 	regval;
	extern	struct	ctlr_desc *slic_to_config[];

	if (USERMODE(cs, flags)) {	/* NMI from user mode */
		/*
		 * Processor interrupts are OFF at entry, since entered
		 * thru interrupt-gate. NMI's are disabled at processor.
		 */
		/*
		 * If it was an access error I/O space, clear access error
		 * and signal user lwp rather than panicing.
		 */
		my_slic = l.eng->e_slicaddr;

		if (sgs2) {
			int flag = accerr_flag(my_slic);

			if (anyaccerr(flag) && !isio(flag)) {
				/*
				 * Toggle nmi accept in the processor
				 * control register so an NMI
				 * that arrived concurrently will be
				 * seen when NMI's are reenabled.
				 */
				reset_snmi(my_slic);
				enable_nmi();		/* proc NMI on */
#ifdef NOTYET
				nmi_sig(SIGBUS);
#endif
				return;
			}
		} else {
			bic_slic = BIC_SLIC(my_slic,
					slic_to_config[my_slic]->cd_flags);
			accerr_reg = (my_slic & 0x01) ? BIC_ACCERR1 : BIC_ACCERR0;
			regval = slic_rdSubslave(bic_slic, PROC_BIC, accerr_reg);
			if ((regval & (BICAE_OCC|BICAE_IO)) == 0) {
				slic_wrSubslave(bic_slic, PROC_BIC, accerr_reg,
					0xbb);
				/*
				 * Toggle nmi accept in the processor control
				 * register so an NMI that arrived concurrently
				 * will be seen when NMI's are reenabled.
				 */
				slic_wrslave(my_slic, PROC_CTL,
					PROC_CTL_NO_NMI | PROC_CTL_NO_SSTEP | 
					PROC_CTL_NO_HOLD | PROC_CTL_NO_RESET);
				slic_wrslave(my_slic, PROC_CTL,
					PROC_CTL_NO_SSTEP | PROC_CTL_NO_HOLD |
					PROC_CTL_NO_RESET);
				enable_nmi();		/* proc NMI on */
#ifdef NOTYET
				nmi_sig(SIGBUS);
#endif
				return;
			}
		}
#ifdef	B1BUG21
		/*
		 * If lwp using the FPU got a reserved-space access error,
		 * assume this is B-1 Errata # 21 (386 generated a 387 IO
		 * reference with address bit 31 off).  Recover from this in
		 * hardware, and give the lwp a SIGFPE.
		 */
		if (!sgs2 && using_fpu &&
		    (slic_rdslave(my_slic, PROC_FLT) & PROC_FLT_ACC_ERR) == 0) {
			/* invoke recover_bug_21(lwpp) here, where
			 * lwpp is the pointer to the faulting lwp.
			 */
#ifdef NOTYET
			nmi_sig(SIGFPE);
#endif
			return;
		}
#endif	/* B1BUG21  */
	}

#ifdef	B1BUG21
	else {
		/*
		 * Similar to user case, if processor isn't idle, was using the
		 * FPU, and got a reserved-space access error, assume this is
		 * B-1 Errata # 21 (386 generated a 387 IO reference with
		 * address bit 31 off).  Recover from this in hardware, and
		 * give the lwp a SIGFPE.
		 *
		 * Have test here in case lwp managed to enter kernel
		 * before taking NMI (eg, thru single-step trap).
		 */
		if (!sgs2 && !l.noproc && using_fpu &&
			(slic_rdslave(l.eng->e_slicaddr, PROC_FLT) &
			PROC_FLT_ACC_ERR) == 0) {
			/* invoke recover_bug_21(lwpp) here, where
			 * lwpp is the pointer to the faulting lwp.
			 */
			/* insert code here to send SIGFPE to the lwp */
			return;
		}
	}
#endif	/* B1BUG21  */

	/*
	 * Disable SLIC interrupts. Already off at
	 * proc and donmi() turns off NMI.
	 */
	(void) splblockall();

	/*
	 * Disable processor acceptence of NMI's. Other interrupts
	 * turned off by caller (currently only trap()), ie donmi
	 * must be called at splhi and cli.
	 */
	slicid = ((struct cpuslic *)KVSLIC)->sl_procid;
	if (sgs2)
		disable_snmi(slicid);
	else
		slic_wrslave(slicid, PROC_CTL,
			     PROC_CTL_NO_NMI | PROC_CTL_NO_SSTEP | 
			     PROC_CTL_NO_HOLD | PROC_CTL_NO_RESET);

	if (sgs2) {
		int flag = accerr_flag(slicid);

		/*
		 * Since there is no explicit indication of a slic-based
		 * NMI, we need to check the other sources of NMI and if
		 * none of them are set, we then assume it is a slic-based
		 * error.  If we're early in system initialization, the
		 * information to access these indicators may have not
		 * been initialized.  In this case, we just panic.
		 *
		 * Check local-board sources and bus-based sources.
		 *
		 * Assumes BIC access error register has identical bit
		 * assignments as 1st gen (032) processor board access error
		 * register, since access_error() is coded this way.
		 *
		 * Since system needs to be "up" enough to have filled out
		 * slic_to_config[]; just panic if not yet up far enough.
		 */

		if (slic_to_config[slicid] == NULL) {
			/*
			 *+ The processor has discovered a bus access error
			 *+ that occurred early in the boot sequence.
			 *+ The information needed to map the error
			 *+ information to the source of the access error 
			 *+ hasn't been initialized yet.
			 */
			cmn_err(CE_PANIC, "ACCESS ERROR");
			/*NOTREACHED*/
		}

		if (anyaccerr(flag)) {
			if (upyet) {
				/*
				 *+ A bus access error occurred, generating a
				 *+ Non-Maskable Interrupt.
			 	 */
				cmn_err(CE_WARN,
					"Processor %d (slic %d) Access Error\n",
					l.eng_num, slicid);
			} else {
				/*
				 *+ A bus access error occurred, generating a
				 *+ Non-Maskable Interrupt.
			 	 */
				cmn_err(CE_WARN,
					"Processor ?? (slic %d) Access Error\n",
					slicid);
			}
			sgs2_access_error(flag);
		}

		/*
		 * From no other source, assume this is a slic-based
		 * interrupt.  If the slic-register is set to "PAUSESELF",
		 * we assume this is a shut-down, otherwise, we just
		 * give-up.
		 */
		if (((struct cpuslic *)KVSLIC)->sl_nmiint == PAUSESELF)
			pause_self();

		/*
		 *+ A Non-Maskable Interrupt was received from the
		 *+ SLIC chip.
		 */
		cmn_err(CE_PANIC, "SLIC NMI");
		/*NOTREACHED*/
	} else {
		/*
		 * First check for SLIC NMI or local reserved location access.
		 */
	
		regval = slic_rdslave(slicid, PROC_FLT);
	
		if ((regval & PROC_FLT_SLIC_NMI) == 0) {	/* SLIC NMI */
			if (((struct cpuslic *)KVSLIC)->sl_nmiint == PAUSESELF)
				pause_self();
			/*
			 *+ A Non-Maskable Interrupt was received from the
			 *+ SLIC chip.
			 */
			cmn_err(CE_PANIC, "SLIC NMI");
			/*NOTREACHED*/
		}
	
		if ((regval & PROC_FLT_ACC_ERR) == 0) {	/* Reserved Space */
			/*
			 *+ The processor tried to access a reserved address
			 *+ that caused a Non-Maskable Interrupt to be
			 *+ generated.
			 */
			cmn_err(CE_PANIC, "RESERVED LOCATION ACCESS ERROR");
			/*NOTREACHED*/
		}
	
		/*
		 * Not a board-local source (cache parity errors freeze the bus,
		 * so we're not executing if that happened).  Thus, it should be
		 * a bus access error, stored in per-channel BIC access error register.
		 *
		 * Assumes BIC access error register has identical bit assignments
		 * as 1st gen (032) processor board access error register, since
		 * access_error() is coded this way.
		 *
		 * Since system needs to be "up" enough to have filled out
		 * slic_to_config[]; just panic if not yet up far enough.
		 */
	
		if (slic_to_config[slicid] == NULL) {
			/*
			 *+ The processor has discovered a bus access error
			 *+ that occurred early in the boot sequence.
			 *+ The information needed to map the error
			 *+ information to the source of the access error 
			 *+ hasn't been initialized yet.
			 */
			cmn_err(CE_PANIC, "ACCESS ERROR");
			/*NOTREACHED*/
		}
		bic_slic = BIC_SLIC(slicid, slic_to_config[slicid]->cd_flags);
		accerr_reg = (slicid & 0x01) ? BIC_ACCERR1 : BIC_ACCERR0;
	
		regval = slic_rdSubslave(bic_slic, PROC_BIC, accerr_reg);
	
		if ((regval & BICAE_OCC) == 0) {
			if (upyet) {
				/*
				 *+ A bus access error occurred, generating a
				 *+ Non-Maskable Interrupt.
			 	 */
				cmn_err(CE_WARN,
					"Processor %d (slic %d) Access Error\n",
					l.eng_num, slicid);
			} else {
				/*
				 *+ A bus access error occurred, generating a
				 *+ Non-Maskable Interrupt.
			 	 */
				cmn_err(CE_WARN,
					"Processor ?? (slic %d) Access Error\n",
					slicid);
			}
			access_error(regval);
		}
	}

	/*
	 * All else failed ==> just panic.
	 *
	 * When/if handle cache parity errors somehow (unfreeze bus and
	 * un-hold processors), should do something here.  For now, system
	 * is *gone* if these happen.
	 */

	/*
	 *+ A Non-Maskable Interrupt was generated by an unknown source.
	 */
	cmn_err(CE_PANIC, "Unexpected NMI.");
	/*NOTREACHED*/
}

/*
 * int
 * clearnmi(void)
 *	Clear the nmi error.
 *
 * Calling/Exit State:
 *	Returns:
 *		1 : if the NMI was the result of bus timeout,
 *		0 : otherwise.
 *
 * Description:
 *	Clear the nmi error, returning true if the nmi was the result of
 *	a bus timeout.
 *	
 *	This function allows bus probers to clear NMI's in an SGS vs. SGS2
 *	independent manner.
 */
 
int
clearnmi(void)
{
	int	my_slic = ((struct cpuslic *)KVSLIC)->sl_procid;
	int	timeout = 0;
	int	sgs2 = l.eng->e_flags&E_SGS2;

	if (sgs2) {
		int flag = accerr_flag(my_slic);

		clraccerr(my_slic);	/* Takes it out of the BIC */
		reset_snmi(my_slic);	/* ... and the SIC */
		enable_snmi(my_slic);
		timeout = istimeout(flag);
	} else {
		int bic_slic = BIC_SLIC(my_slic, slic_to_config[my_slic]->cd_flags);
		int accerr_reg = (my_slic & 1) ? BIC_ACCERR1 : BIC_ACCERR0;
		int regval = slic_rdSubslave(bic_slic, PROC_BIC, accerr_reg);

		/*
		 * Writing any value to the BIC's
		 * access-error register clears it.
		 */
		slic_wrSubslave(bic_slic, PROC_BIC, accerr_reg, 0xbb);
		timeout = ((~regval)&SLB_ATMSK) == SLB_AETIMOUT;
	}
	return timeout;
}

/*
 * sendsoft support --
 *
 * This table vectors each of the different types of soft interrupts
 * (low priority inter-processor interrupts) to the appropriate handler.
 */

extern void nudge_int(void);
extern void kpnudge_int(void);
extern void globalsoftint(void);
extern void localsoftint(void);
extern void runqueues(void);
static void unused_softint(void);

void (*softvec[8])(void) = {
	unused_softint,		/* 0 */
	unused_softint,		/* 1 */
	unused_softint,		/* 2 */
	runqueues,		/* 3 */
	globalsoftint,		/* 4 */
	localsoftint,		/* 5 */
	kpnudge_int,		/* 6 */
	nudge_int,		/* 7 */
};

/*
 * static void
 * unused_softint(void)
 *	Handle a soft interrupt of an unused type.
 *
 * Calling/Exit State:
 *	Called at interrupt level.  Must not block.
 */
static void
unused_softint(void)
{
	/*
	 *+ A bin 0 software interrupt was received, but its type was
	 *+ not one of the types which are being used by the kernel.
	 */
	cmn_err(CE_NOTE, "Received software interrupt of unused type.");
}

/*
 * boolean_t
 * mainstore_memory(paddr_t)
 *	Determines if the given paddr is part of mainstore or device memory.
 *
 * Calling/Exit State:
 *	For symmetry platform, there is no device memory.
 */
boolean_t
mainstore_memory(paddr_t paddr)
{
	return B_TRUE;
}

/*
 * This function is a no-op in symmetry platform.
 */
void
sysdump_init(void)
{
}
