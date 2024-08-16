/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/acer/aceridb.c	1.4"
#ident	"$Header: $"

/*
 * ACER Interrupt Distribution Board (IDB) Support in Symmetric mode.
 */

#include <io/cram/cram.h>
#include <io/prf/prf.h>
#include <mem/immu.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/regset.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <svc/bootinfo.h>
#include <svc/eisa.h>
#include <svc/intr.h>
#include <svc/pic.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/processor.h>
#include <util/types.h>

#include <acer.h>


/*
 * TODO:
 *	- Investigate to use xcall mechanism to turn interrupt on or off.
 */

#define	ACER_ONLINE	0x01
#define ACER_OFFLINE	0x02

#ifdef DEBUG
STATIC int spxl_debug = 0;
#define DEBUG1(a)	if (aidb_debug == 1) printf a
#define DEBUG2(a)	if (aidb_debug == 2) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG */

/*
 * Configurable objects -- defined in psm.cf/Space.c
 */
extern int acer_maxnumcpu;		/* max. cpus */
extern ushort_t engine_ctl_port[];
extern ushort_t engine_ivec_port[];

extern struct idt_init acer_idt0_init[];
extern uchar_t	acer_iomode;

extern void	acer_noop(void);

int		aidb_assignvec(int, int);
int		aidb_unassignvec(int);
void		aidb_intrdist(int, uint_t);
void		aidb_moveintr(int, int, int, int);

struct idt_init *aidb_idt(int);
void		aidb_intr_init(void);
void		aidb_intr_start(void);
void		aidb_misc_init(void);
void		aidb_offline_self(void);
void		aidb_timer_init(void);


struct acerpsmops aidbmpfuncs = {
	(void (*)())acer_noop,			/* acer_reboot */
	aidb_idt,				/* acer_idt */
	aidb_intr_init,				/* acer_intr_init */
	aidb_intr_start,			/* acer_intr_start */
	(int (*)())acer_noop,			/* acer_numeng */
	(void (*)())acer_noop,			/* acer_configure */
	(void (*)())acer_noop,			/* acer_online_engine */
	(void (*)())acer_noop,			/* acer_selfinit */
	aidb_misc_init,				/* acer_misc_init */
	aidb_offline_self,			/* acer_offline_self */
	(void (*)())acer_noop,			/* acer_clear_xintr */
	(void (*)())acer_noop,			/* acer_send_xintr */
	aidb_timer_init,			/* acer_timer_init */
	(ulong_t (*)())acer_noop,		/* acer_usec_time */
	(boolean_t (*)())acer_noop,		/* acer_isxcall */
	aidb_assignvec,				/* acer_assignvec */
	aidb_unassignvec			/* acer_unassignvec */
};


/*
 * struct idt_init *
 * aidb_idt(int engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 */
/* ARGSUSED */
struct idt_init *
aidb_idt(int engnum)
{
	return (acer_idt0_init);
}


/*
 * void
 * aidb_intr_init(void)
 *	Initialize programmable interrupt controller.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 */
void
aidb_intr_init(void)
{
	/*
	 * Allocate memory for interrupt tables.
	 */
	intrtab_init(myengnum);

	/*
	 * Initialize the i8259 programmable interrupt controller.
	 */
	picinit();

        /* Clear all interrupts. */
	outb(engine_ctl_port[myengnum],
		inb(engine_ctl_port[myengnum]) & ~(PINT | INTDIS));
}


/*
 * void
 * aidb_intr_start(void)
 *	Enable interrupts.
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is being onlined. 
 */
void
aidb_intr_start(void)
{
	picstart();
}


/*
 * void
 * acer_offline_self(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Do any necessary work to redistribute interrupts.
 */
void
aidb_offline_self(void)
{
	pl_t	opl;

	opl = splhi();
	aidb_intrdist(myengnum, ACER_OFFLINE);
	splx(opl);
}


/*
 * void
 * aidb_timer_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
aidb_timer_init(void)
{
	/*
	 * Initialize the i8254 programmable interrupt timer.
	 */
	if (myengnum == BOOTENG)
		clkstart();
}


/*
 * void
 * aidb_misc_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
aidb_misc_init(void)
{
	/*
	 * Assign interrupts allocated to the engine.
	 */
	aidb_intrdist(myengnum, ACER_ONLINE);
}


extern uchar_t intpri[];
extern int npic;

/*
 * void 
 * aidb_moveintr(int iv, int seng, int teng, int itype)
 *
 * Calling/Exit State:
 *	<iv> is the interrupt vector (interrupt request no.) that is being
 *	reassigned.
 *	<seng> is the engine number from which the interrupt is being moved.
 *	<teng> is the engine number to which the interrupt is being moved.
 *	<itype> is the interrupt type state -- level/edge/shared
 *
 * Remarks:
 *	Unconditionally move an interrupt <iv> from the source engine
 *	<seng> to target engine <teng>.
 */
void
aidb_moveintr(int iv, int seng, int teng, int itype)
{
	pl_t	opl;
	extern lock_t mod_iv_lock;


	ASSERT(Nengine > 1);

	/*
	 * if the current owner and the new owner of the interrupt 
	 * are the same, return immediately with a successful 
	 * completion code.
	 */
	if (seng == teng)
		return;

	opl = LOCK(&mod_iv_lock, PLHI);

/*
	if (seng == myengnum) {
		psm_introff(iv);
	} else {
		emask_t	targets, responders;

		EMASK_SET1(&targets, seng);
		xcall(&targets, &responders, psm_introff, iv);
	}

	if (teng == myengnum) {
		psm_intron(iv);
	} else {
		emask_t	targets, responders;

		EMASK_SET1(&targets, teng);
		xcall(&targets, &responders, psm_intron, iv);
	}
*/

	/*
	 * Mask/clear the interrupt on the source engine.
	 */
	psm_introff(iv, (pl_t)intpri[iv], seng, itype);

	/*
	 * Unmask/allow/set interrupt on the target engine.
	 */
	psm_intron(iv, (pl_t)intpri[iv], teng, 1, itype);

	UNLOCK(&mod_iv_lock, opl);
}


/*
 * Defined in psm.cf/Space.c file.
 */
extern int intrdistmask[];

/*
 * Interrupt distribution structure indicates which
 * interrupts are moved from the source engine to the
 * target engine. This information is necessary to move
 * them back to their source engine if an engine is going
 * offline. This facility is available to balance the 
 * interrupt load across engines.
 *
 * The structure is indexed by interrupt vector.
 */
struct intrdistinfo {
	int	idi_seng;		/* source engine */
	int	idi_teng;		/* target engine */
	int	idi_flag;		/* bound or distributable */
#define	INTR_BOUND		0x01
#define	INTR_UNBOUND		0x02
	int	idi_itype;		/* intr. type -- level/edge/shared */
} intrdist_table[NPIC * PIC_NIRQ];

/*
 * Last engine number to which a distribuable interrupt was assigned.
 */
int intrdist_engnum = BOOTENG;

/*
 * int
 * aidb_assignvec(int vec, int itype)
 *
 * Calling/Exit State:
 *	<vec> is the interrupt vector number that is to be redistributed.
 *	<itype> represents the sensitivity of interrupt -- level/edge/shared.
 *
 *	Returns the engine to which the interrupt is assigned.
 *
 * Remarks:
 *	Assign interrupt vector of the multithreaded drivers
 *	to the processors based on a round-robin scheme.
 *
 *	You can devise any other scheme to assign these interrupt
 *	vectors across multiple engines. Here is an example of
 *	another possible interrupt distribution scheme based on
 *	static/fixed interrupt allocation per processor.
 *
 *	No. of cpus = 2, then
 *
 *      cpu no.                 interrupt distribution
 *      ------                  ----------------------
 *
 *      1                       clock(todclock, lclclock), kd, serial port
 *      2                       clock(lclclcok), disk, network
 *
 *	No of cpus = 3, then
 *
 *      cpu no.                 interrupt distribution
 *      ------                  --------------------
 *
 *      1                       clock(todclock, lclclock), kd, serial port
 *      2                       clock(lclclcok), disk
 *      3                       clock(lclclock), network
 *
 *	No of cpus = 4, then
 *
 *      cpu no.                 interrupt distribution
 *      ------                  --------------------
 *
 *      1                       clock(todclock, lclclock), kd
 *      2                       clock(lclclcok), disk
 *      3                       clock(lclclock), network
 *      4                       clock(lclclock), serial port
 *
 *	The assumption is that all drivers are multithreaded.
 *
 * Note:
 *	The interrupt distribution state must be set only once during
 *	driver initialization or load time.
 *
 *	When aidb_assignvec() is called during interrupt redistribution
 *	thru aidb_intrdist() --> psm_intron()/psm_introff, then <engnum> 
 *	would map to a valid engine instead of to an unknown (-1) engine.
 *	In contrast, when a driver is loaded the <engnum> would map to
 *	-1 and thus would cause the distribution state to be set.
 *
 *	The interrupt will only be reassigned once, regardless of number
 *	of multithreaded drivers that share the interrupt. This is 
 *	prevented by the DLM. 
 *
 *	If there are two drivers, one of which is multithreaded, but the
 *	other is bound, then the interrupt should be assigned to the cpu 
 *	to which the driver is bound. This is true regardless of which 
 *	driver is loaded first and is assured by the DLM.
 */
int 
aidb_assignvec(int vec, int itype)
{
	int e = BOOTENG;
	boolean_t rval = B_FALSE;
	

	ASSERT(vec < (npic * PIC_NIRQ));

	/*
	 * Do not redistribute inter-processor interrupt vector,
	 * since it is always enabled on each processor.
	 *
	 * XXX: remove it since intrdistmask[] prevents the
	 * inter-processor interrupt redistribution.
	 */
	if (vec == XINTR)
		return myengnum;

	/*
	 * The interrupt is distributable if the following conditons are true.
	 *	1. The driver is multithreaded.
	 *	2. The driver is not bound to any cpu.
	 *	3. The interrupt is allowed to be distributable by the PSM.
	 *	   Note: The level trigger interrupts are not distributable.
	 */
	if (intrdistmask[vec] == 0 && itype != 4) {
		intrdist_table[vec].idi_seng = BOOTENG;
		intrdist_table[vec].idi_teng = (++intrdist_engnum % Nengine);
		intrdist_table[vec].idi_flag = INTR_UNBOUND;
		intrdist_table[vec].idi_itype = itype;
	}

	e = intrdist_table[vec].idi_teng;
	if ((engine_state(e, ENGINE_ONLINE, &rval) != 0) || (rval == B_FALSE)) {
		/*
		 *+ The engine on which the interrupt is to be enabled
		 *+ is either not yet online or cannot be onlined because
		 *+ of some hardware defect. Enabling the interrupt on
		 *+ the boot engine.
		 */
		cmn_err(CE_NOTE,
			"!Engine 0x%x is currently unoperational. Assigning "
			"the interrupt vector 0x%x to boot engine.", e, vec);
		return BOOTENG;;
	}

	ASSERT(rval == B_TRUE);
	return e;
}


/*
 * int 
 * aidb_unassignvec(int vec)
 *
 * Calling/Exit State:
 *	<vec> is the interrupt vector number that is to be redistributed.
 *
 *	Returns the engine number from which the interrupt is to be unassigned.
 *
 * Note:
 *	The interrupt distribution state must be reset only once during
 *	driver halt or unload time.
 *
 *	When spxl_unassignvec() is called during interrupt redistribution
 *	thru spxl_intrdist() --> psm_intron()/psm_introff, then <engnum> 
 *	would map to a valid engine instead of to an unknown (-1) engine.
 *	In contrast, when a driver is unloaded the <engnum> would map to
 *	-1 and thus would cause the distribution state to be reset.
 */
int 
aidb_unassignvec(int vec)
{
	int e = BOOTENG;
	boolean_t rval = B_FALSE;


	ASSERT(vec < (npic * PIC_NIRQ));

	/*
	 * Do not redistribute inter-processor interrupt vector,
	 * since it is always enabled on each processor.
	 *
	 * XXX: remove it since intrdistmask[] prevents the
	 * inter-processor interrupt redistribution.
	 */
	if (vec == XINTR)
		return myengnum;

	e = intrdist_table[vec].idi_teng;

	/*
	 * Reset the interrupt distribution state if the following conditions
	 * hold true:
	 *	1. The driver is multithreaded.
	 *	2. The interrupt is reassigned/redistributed/unbound.
	 */
	if (intrdist_table[vec].idi_flag & INTR_UNBOUND) {
		intrdist_table[vec].idi_seng = 0;
		intrdist_table[vec].idi_teng = 0;
		intrdist_table[vec].idi_flag = 0;
		intrdist_table[vec].idi_itype = 0;
	}

	if ((engine_state(e, ENGINE_ONLINE, &rval) != 0) || (rval == B_FALSE)) {
		/*
		 *+ The engine on which the interrupt is to be disabled
		 *+ is either not yet online or cannot be onlined because
		 *+ of some hardware defect. Disableing the interrupt on
		 *+ the boot engine.
		 */
		cmn_err(CE_NOTE,
			"!Engine 0x%x is currently unoperational. Discarding "
			"the request to disable interrupt vector 0x%x from "
			"engine. Attempting to disable the interrupt on "
			"boot engine.", e, vec);
		return BOOTENG;
	}

	ASSERT(rval == B_TRUE);
	return e;
}


/*
* void
* aidb_intrdist(int engnum, uint_t eflag)
*	Assign/Unassign interrupts to the engnum engine.
*
* Calling/Exit State:
*	- <engnum> is the engine number that is being onlined/offlined.
 *	- <eflag> is the state that determines whether the engine is
 *	  being onlined/offlined. The possible flag bits are:
 *		ACER_OFFLINE
 *		ACER_ONLINE
 *
 * Remarks;
 *	Uninitialize/re-route/redistribute the interrupts assigned to
 *	the boot engine when the engnum is being onlined or offlined.
 *	It is responsible for balancing the interrupt load across the
 *	processors.
 *
 *	The interrupt distribution information is stored in intrdistinfo 
 *	structure.
 *
 *	Must be called when the non-boot engine is being onlined/offlined
 *	and no drivers are bound to it.
 *
 *	eflag		seng		teng	
 *	-----		----		----
 *
 *	online:		BOOTENG		engnum
 *	offline:	engnum		BOOTENG	
 */
void
aidb_intrdist(int engnum, uint_t eflag)
{
	int	seng;		/* source engine */
	int 	teng;		/* target engine */
        int	iv;		/* interrupt vector */


	/*
	 * Return immediately if running on the boot engine.
	 */
	if (engnum == BOOTENG)
		return;

	/*
	 * Return immediately if in an asymmetric mode.
	 */
	if (!(acer_iomode & SYMINTR))
		return;

	ASSERT(Nengine <= acer_maxnumcpu);

	/*
	 * The device drivers that are bound or siglethreaded 
	 * must NOT have their interrupts reassigned because 
	 * its possible that there could be pending timeouts
	 * on the source processor for bound drivers. Only
	 * multithreaded drivers must be allowed to have
	 * their interrutpts reassigned.
	 */

	for (iv = 0; (iv < (npic * PIC_NIRQ)); iv++) {

		/*
		 * <myengnum> must be equal to the target engine number for
		 * the <iv> entry in the interrupt distribution table since 
		 * the engine going offline can again reassign the interrupt
		 * to another engine. Similarly when the engine is coming
		 * online the interrupt can be assigned to the <teng>.
		 */
		if (intrdist_table[iv].idi_flag & INTR_UNBOUND &&
		    intrdist_table[iv].idi_teng == myengnum) {
			
			if (eflag == ACER_ONLINE) {
				seng = intrdist_table[iv].idi_seng;
				teng = intrdist_table[iv].idi_teng;
			} else if (eflag == ACER_OFFLINE) {
				seng = intrdist_table[iv].idi_teng;
				teng = intrdist_table[iv].idi_seng;
			} else
				return;

			ASSERT(BOOTENG <= seng && seng < Nengine);
			ASSERT(BOOTENG <= teng && teng < Nengine);

			/*
			 * Assign interrupt from the source engine to the
			 * target engine.
			 */
			if (seng != teng)
				aidb_moveintr(iv, seng, teng, 
						intrdist_table[iv].idi_itype);
		}
	}
}
