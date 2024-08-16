/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/xl.c	1.17"
#ident	"$Header: $"

/*
 * COMPAQ SYSTEMPRO/XL & PROLIANT HARDWARE INFO:
 *	- To do something to the CPU that you are on then just do the
 *	  outb, else write CPU # to INDEXCPU, LOW Addr to INDEXLOW, and 
 *	  HIGH Addr to INDEXHI that you wish to read/write to and then 
 *	  read or write from INDEXDATA. Note that several of the frequently
 *	  used commands are prepackaged into 32bit bundles that can be
 *	  ANDed with cpu, ie.
 *
 *		to PINT CPU 1 at IRQ 13 would be the following
 *
 *		INT13 addr 0x0CC8
 *		Command 0x05
 *
 *		outb(INDEXCPU,0x01)
 *		outb(INDEXLOW,0xC8)
 *		outb(INDEXHI,0x0C)
 *		outb(INDEXDATA,0x05)
 *
 *	  There for the command outl(INDEXCPU,(0x050CC800|(long)cpu))
 *
 *	  Keep in mind that all of the PICs are now local once you are in
 *	  SystemPro/XL mode.
 *
 *	  Set up iplmask so that each CPU can receive clock interrupts and
 *	  program both clock chips.
 *
 *	  See programmers guide for indexed and local io.
 *
 *	- All cpus can receive clock interrupts.
 *
 *	- All cpus access their control port at the same address.
 *
 *	- There can be empty cpu slots.
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

#include <syspro.h>
#include <xl.h>


/*
 * TODO:
 *	- Investigate to use xcall mechanism to turn interrupt on or off.
 */

#define	SPXL_ONLINE	0x01
#define SPXL_OFFLINE	0x02

#ifdef DEBUG
STATIC int spxl_debug = 0;
#define DEBUG1(a)	if (spxl_debug == 1) cmn_err(CE_CONT, a)
#define DEBUG2(a)	if (spxl_debug == 2) cmn_err(CE_CONT, a)
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG */

/*
 * Configurable objects -- defined in psm.cf/Space.c
 */
extern int sp_maxnumcpu;		/* max. cpus in SP or SP/XL */
extern int sp_systypes;			/* number of SP or SP/XL systems
					 * with different EISA ids.
					 */
extern char **sp_eisa_id[];		/* EISA ids for SP and SP/XL */
extern uchar_t sp_xl_slot[];		/* slot number for SP/XL */

extern int spxl_iomodemask;		/* mask the I/O mode */
extern int xl_maxnumcpu;		/* max. cpus (max_ACPUs) in an XL
					 * compatible system e.g. Proliant
					 */
extern struct idt_init cpq_idt0_init[];

extern void	cpq_noop(void);
extern boolean_t sp_ecc_intr(void);
extern boolean_t sp_dma_intr(void);

STATIC boolean_t spxl_findeng(uchar_t, int);

char		spxl_read_indexed(int, int);
void		spxl_write_indexed(int, int, int);

int		spxl_assignvec(int, int);
int		spxl_unassignvec(int);
void		spxl_intrdist(int, uint_t);
void		spxl_moveintr(int, int, int, int);

void		spxl_stopcpu(int);
struct idt_init *spxl_idt(int);
void		spxl_intr_init(void);
void		spxl_intr_start(void);
int		spxl_numeng(void);
void		spxl_configure(void);
void		spxl_bootcpu(int);
void		spxl_selfinit(void);
void		spxl_misc_init(void);
void		spxl_offline_self(void);
void		spxl_cpuclrintr(int);
void		spxl_cpuintr(int);
void		spxl_timer_init(void);
ulong_t		spxl_usec_time(void);
boolean_t	spxl_checkint13(void);


struct cpqpsmops xlmpfuncs = {
	spxl_stopcpu,				/* cpq_reboot */
	spxl_idt,				/* cpq_idt */
	spxl_intr_init,				/* cpq_intr_init */
	spxl_intr_start,			/* cpq_intr_start */
	spxl_numeng,				/* cpq_numeng */
	spxl_configure,				/* cpq_configure */
	spxl_bootcpu,				/* cpq_online_engine */
	spxl_selfinit,				/* cqp_selfinit */
	spxl_misc_init,				/* cpq_misc_init */
	spxl_offline_self,			/* cpq_offline_self */
	spxl_cpuclrintr,			/* cpq_clear_xintr */
	spxl_cpuintr,				/* cpq_send_xintr */
	spxl_timer_init,			/* cpq_timer_init */
	spxl_usec_time,				/* cpq_usec_time */
	spxl_checkint13,			/* cpq_isxcall */
	sp_ecc_intr,				/* cpq_ecc_intr */
	sp_dma_intr,				/* cpq_dma_intr */
	spxl_assignvec,				/* cpq_assignvec */
	spxl_unassignvec			/* cpq_unassignvec */
};

ulong_t	*spxl_nsec;		/* free-running nanosecond counter */
fspin_t	spxl_lock;		/* fast spin lock to protect the index reg. */

extern uchar_t	cpq_mptype;	/* type of Compaq multiprocessor platforms */
extern uchar_t	sp_iomode;


/*
 * char
 * spxl_read_indexed(int cpu_number, int port_addr)
 *
 * Calling/Exit State:
 *	It is protected by a fast spin lock, since index ports can
 *	be written by other CPUs simultaneously during indexed I/O
 *	transfer.
 */
char
spxl_read_indexed(int cpu_number, int port_addr)
{
	char	port_value;

	FSPIN_LOCK(&spxl_lock);
	outb(XL_INDEXCPU, (char)cpu_number);
	outb(XL_INDEXLOW, (char)port_addr);
	outb(XL_INDEXHI, (char)(port_addr >> 8));
	port_value = inb(XL_INDEXDATA);
	FSPIN_UNLOCK(&spxl_lock);

	return (port_value);
}


/*
 * void 
 * spxl_write_indexed(int cpu_number, int port_addr, int port_value)
 *
 * Calling/Exit State:
 *	It is protected by a fast spin lock, since index ports can
 *	be written by other CPUs simultaneously during indexed I/O
 *	transfer.
 */
void
spxl_write_indexed(int cpu_number, int port_addr, int port_value)
{
	FSPIN_LOCK(&spxl_lock);
	outb(XL_INDEXCPU, (char)cpu_number);
	outb(XL_INDEXLOW, (char)port_addr);
	outb(XL_INDEXHI, (char)(port_addr >> 8));
	outb(XL_INDEXDATA, (char)port_value);
	FSPIN_UNLOCK(&spxl_lock);
}


/*
 * void
 * spxl_bootcpu(int cpu)
 *	Boot additional CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
void
spxl_bootcpu(int cpu)
{
	ASSERT(cpu < xl_maxnumcpu);
	ASSERT(inb(XL_MODESELECT) & SP_XLMODE);
	ASSERT(spxl_read_indexed(cpu, XL_STATUSPORT) & XL_SLEEPING);

	outl(XL_INDEXCPU, 
		(long)(XL_START | XL_RESET | XL_CACHEON | XL_FLUSH) << 24 | 
		(long)XL_COMMANDPORT << 8 | (long)cpu);
}


/*
 * void
 * spxl_resetcpu(int cpu)
 *	Put the processor to a known state and put it to sleep.
 *
 * Calling/Exit State:
 *	None.
 */
void
spxl_resetcpu(int cpu)
{
	outl(XL_INDEXCPU, (long)(XL_RESET | XL_SLEEP) << 24 | 
		(long) XL_COMMANDPORT << 8 | (long)cpu);
}


/*
 * void
 * spxl_stopcpu(int cpu)
 *	Stop additonal CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
void
spxl_stopcpu(int cpu)
{
	outl(XL_INDEXCPU, (long)(XL_SLEEP | XL_FLUSH | XL_CACHEOFF) << 24 | 
			(long)XL_COMMANDPORT << 8 | (long)cpu);
}


/*
 * void
 * spxl_cpuintr(int cpu)
 *	Interrupt a CPU.
 *
 * Calling/Exit State:
 *	None.
 */
void
spxl_cpuintr(int cpu)
{
	/* Indexed I/O to another CPU. */
	outl(XL_INDEXCPU, XL_PINT13 | (long)cpu);
}


/*
 * void
 * spxl_cpuclrintr(int cpu)
 *	To clear a CPU interrupt
 *
 * Calling/Exit State:
 *	None.
 */
void
spxl_cpuclrintr(int cpu)
{
	/* Do this locally. */
	outb(XL_INT13, XL_PINTCLR);
}


/*
 * boolean_t
 * spxl_checkint13(void)
 *	Check to see if IRQ13 is FP or PINT
 *
 * Calling/Exit State:
 *	None.
 */
boolean_t
spxl_checkint13(void)
{
	if ((inb(XL_STATUSPORT) & XL_NCPERR) || (sp_ecc_intr())) {
		/* This is a FP IRQ. */
		return (B_FALSE);
	} else {
		/* This is IPI IRQ */
		return (B_TRUE);
	}
}


/*
 * struct idt_init *
 * spxl_idt(int engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 */
/* ARGSUSED */
struct idt_init *
spxl_idt(int engnum)
{
	return (cpq_idt0_init);
}


/*
 * void
 * psm_intr_init(void)
 *	Initialize programmable interrupt controller.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 */
void
spxl_intr_init(void)
{
	/*
	 * Allocate memory for interrupt tables.
	 */
	intrtab_init(myengnum);

	/*
	 * Initialize the i8259 programmable interrupt controller.
	 */
	picinit();
}


/*
 * void
 * spxl_intr_start(void)
 *	Enable interrupts.
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is being onlined. 
 *
 * TODO:
 *	picstart() must be changed to take logical engnum as an argument.
 */
void
spxl_intr_start(void)
{
	picstart();
}


/*
 * void
 * spxl_configure(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
spxl_configure(void)
{
	FSPIN_INIT(&spxl_lock);
	/* Turn SysproXL mode on */
	outb(XL_MODESELECT, inb(XL_MODESELECT) | SP_XLMODE);
}


/*
 * void
 * spxl_selfinit(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	We use the logical CPU assignment port to assign a logical
 *	CPU number for a physical CPU. We specify the physical CPU
 *	to receive the assignment by writing to the CPU index port.
 */
void
spxl_selfinit(void)
{
	/* one-to-one mapping between logical CPU and physical CPU */
	outb(XL_INDEXPORT, myengnum);		/* write physical CPU no */
	outb(XL_ASSIGNPORT, myengnum);		/* write logical CPU no */
}


/*
 * void
 * spxl_offline_self(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Do any necessary work to redistribute interrupts.
 */
void
spxl_offline_self(void)
{
	pl_t	opl;

	opl = splhi();
	spxl_intrdist(myengnum, SPXL_OFFLINE);
	splx(opl);
}


/*
 * void
 * spxl_timer_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
spxl_timer_init(void)
{
	/*
	 * Initialize the i8254 programmable interrupt timer.
	 */
	clkstart();

	/*
	 * Map in the microsecond performance counter.
	 */
	if (myengnum == BOOTENG) {
		spxl_nsec = (ulong_t *)physmap(XL_NSEC_PADDR, 8, KM_NOSLEEP);
		if (spxl_nsec == NULL) {
			/*
			 *+ Not enough virtual memory available to map 
			 *+ in a free-running counter.
			 */
			cmn_err(CE_WARN,
				"!psm_timer_init: Unable to map in "
				" a free-flowing counter");
		}
	}
}

ulong_t spxl_lop;
ulong_t spxl_hop;
ulong_t spxl_usecs;
ulong_t spxl_latchcounter[2];

/*
 * ulong_t
 * spxl_usec_time(void)
 *
 * Calling/Exit State:
 *	- Return the current time (free-running counter value)
 *	  in microseconds.
 *
 * Remarks:
 *	The documentation for the 48-bit free-running counter 
 *	available on the Compaq SysproXL and ProLiant machines 
 *	does not adequately describe the timebase for the counter.
 *	The documentation describes the counter as having nanosecond
 *	resolution, when infact the counter appears to be driven by a
 *	33MHz clock and thus would have a 30ns resolution. Consequently
 *	the values previously returned by the compaq spxl_usec_time() 
 *	routine (which converts this counter into a microsecond value)
 *	are off by factor of 33.  This is fixed by dividing the low
 *	order 32 bits (nanosecond counter) by 33 instead of 1000. 
 *
 *	There appears to be a hardware bug, where sometimes the value
 *	returned in the low 32 bits of the counter is 0x45f8. Reading
 *	the counter again in this case gives the correct value. This
 *	results in a more reliable microsecond timer because bad values
 *	are now sampled out.
 *
 *	In our previous scheme we read the nanosecond counter and 
 *	divided it by 1000 to return the microsecond counter value.
 *	This scheme only used lower 32 bits of the 48 bit counter.
 *	Converting that to microsecond lost 5 bits of resolution.
 *	The counter then wrapped in 2^27 bits or 134 seconds. Since 
 *	there are 16 more bits of counter available in the hardware, 
 *	we can read this counter also and shift in the low 5 bits 
 *	of the upper 16 bits, and not loose any resolution. This
 *	results in a higher resolution timer because we now do not
 *	discard the high 16 bits (32-48) of the 48 bit counter.
 */
ulong_t
spxl_usec_time(void)
{
	uint_t	ntries;


	if (spxl_nsec == NULL)
		return (0);

	/*
	 * Read all 64-bits of the performance counter, so that the
	 * memory-mapped counter is latched/updated and read later.
	 */
	for (ntries = 0; ntries < 4; ntries++) {
		spxl_latchcounter[0] = spxl_nsec[0];
		spxl_latchcounter[1] = spxl_nsec[1];
		spxl_lop = spxl_nsec[0];
		spxl_hop = spxl_nsec[1];
		/* we somtimes get back bad values */
		if (spxl_hop >= spxl_latchcounter[1] && 
		    spxl_lop >= spxl_latchcounter[0] &&
		    spxl_lop != 0x45f8)
			break;
	}

	/*
	 * The input clock seems to be a 33Mhz clock. We divide
	 * divide this by 33 to get a microsecond clock. We shift
	 * in some of the high order bits to keep the full range
	 * of the counter.  We accomplish both (without using a
	 * divide) by using the formula:
	 *	x/33 = x/32 - x/32^2 + x/32^3 - x/32^4
	 */

	/* convert to microseconds */
	spxl_usecs = ((spxl_hop << 27) | (spxl_lop >> 5));
	/* x/33 = x/32 - x/32^2 + x/32^3 - x/32^4 */
	spxl_usecs = spxl_usecs - (spxl_usecs >> 5) + 
			(spxl_usecs >> 10) - (spxl_usecs >> 15);
	/* return the microsec. value */
	return (spxl_usecs);
}

extern uchar_t intpri[];
extern int npic;

/*
 * void 
 * spxl_moveintr(int iv, int seng, int teng, int itype)
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
spxl_moveintr(int iv, int seng, int teng, int itype)
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
 * spxl_assignvec(int vec, int itype)
 *
 * Calling/Exit State:
 *	<vec> is the interrupt vector number that is to be redistributed.
 *	<itype> represents the sensitivity of interrupt -- level/edge/shared.
 *
 *	Returns the engine number to which the interrupt is assigned.
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
 *	When spxl_assignvec() is called during interrupt redistribution
 *	thru spxl_intrdist() --> psm_intron()/psm_introff, then <engnum> 
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
spxl_assignvec(int vec, int itype)
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
	 */
	if (intrdistmask[vec] == 0) {
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
		return BOOTENG;
	}

	ASSERT(rval == B_TRUE);
	return e;
}


/*
 * int 
 * spxl_unassignvec(int vec)
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
spxl_unassignvec(int vec)
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
			"engine. Attempting to disable the interrupt from "
			"boot engine.", e, vec);
		return BOOTENG;
	}

	ASSERT(rval == B_TRUE);
	return e;
}


/*
 * void
 * spxl_intrdist(int engnum, uint_t eflag)
 *	Assign/Unassign interrupts to the engnum engine.
 *
 * Calling/Exit State:
 *	- <engnum> is the engine number that is being onlined/offlined.
 *	- <eflag> is the state that determines whether the engine is
 *	  being onlined/offlined. The possible flag bits are:
 *		SPXL_OFFLINE
 *		SPXL_ONLINE
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
spxl_intrdist(int engnum, uint_t eflag)
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
	if (!(sp_iomode & SYMINTR))
		return;

	ASSERT(Nengine <= xl_maxnumcpu);

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
			
			if (eflag == SPXL_ONLINE) {
				seng = intrdist_table[iv].idi_seng;
				teng = intrdist_table[iv].idi_teng;
			} else if (eflag == SPXL_OFFLINE) {
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
				spxl_moveintr(iv, seng, teng, 
						intrdist_table[iv].idi_itype);
		}
	}
}


/*
 * void
 * psm_misc_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
spxl_misc_init(void)
{
	/*
	 * Assign interrupts allocated to the engine.
	 */
	spxl_intrdist(myengnum, SPXL_ONLINE);
}


/*
 * Definitions that apply only to Proliant machines.
 */

int	xl_check_evs(void);
int	xl_get_ev(char *, char *, int *);

extern char	sp_xl_eisabuf[];
extern int	proliant_ids;		/* number of Proliant system ids */
extern long	proliant_id[];		/* list of Proliant system ids */

struct {
	char	slot;
	char	step;
	char	type;
} cqhcpu[MAXNUMCPU];			/* cpus present (type/step) */

int	xl_num_OK_CPUs = 0;		/* no. of operation cpus */

char	cqhcpf;				/* bitmask of cpus that failed POST */


/*
 * STATIC boolean_t
 * spxl_findeng(uchar_t slot, int engnum)
 *
 * Calling/Exit State:
 *	- <slot> is the eisa socket no. of the cpu board id.
 *	- <engnum> is the processor index in the <sp_eisa_id> table to find 
 *	  the list of EISA id strings.
 */
STATIC boolean_t
spxl_findeng(uchar_t slot, int engnum)
{
	char	eid[4];		/* compressed EISA id */
	char	*cbuf;		/* character buffer */
	int	systype;


	eisa_boardid(slot, eid);
	cbuf = eisa_uncompress(eid);

	DEBUG1(("spxl_findeng: eisa uncompressed id = %s\n", cbuf));

        for (systype = 0; systype < sp_systypes; systype++) {
		if (strncmp(cbuf, sp_eisa_id[systype][engnum], 7) == 0) {
			cmn_err(CE_CONT, 
				"!spxl_findeng: Found processor %d in "
				"eisa slot %x\n", engnum, slot);
			return(B_TRUE);
		}
        }

	return(B_FALSE);
}


/*
 * int 
 * spxl_numeng(void)
 *	Find number of processor boards in the system.
 *
 * Calling/Exit State:
 *	Must be called after spxl_presense().
 */
int
spxl_numeng(void)
{
	int	neng = 0;
	int	i;
	int	error = 0;

	if (cpq_mptype & CPQ_SYSTEMPROXL) {
		int	slot;

		/*
		 * Check only the slots listed in sp_xl_slot for the
		 * "CPU" type/subtype string.
		 */
		for (i = 0; i < sp_maxnumcpu; i++) {
			if (eisa_read_nvm(sp_xl_slot[i], (uchar_t *)sp_xl_eisabuf, &error))
				if (eisa_parse_devconfig(
				    (void *)sp_xl_eisabuf, (void *)"CPU", EISA_SLOT_DATA, 0))
					neng++;
		}

		if (neng == sp_maxnumcpu)
			return neng;

		/*
		 * Check all the slots for the "CPU" type/subtype string.
		 * This is necessary because the slot number listed in 
		 * sp_xl_slot could be different from the actual slot no.
		 * of the processor board.
		 */
		if (neng < sp_maxnumcpu) {
			neng = 0;
			for (slot = 0; slot < EISA_MAX_SLOTS; slot++) {
				if (eisa_read_nvm(slot, (uchar_t *)sp_xl_eisabuf, &error))
					if (eisa_parse_devconfig(
					    (void *)sp_xl_eisabuf, (void *)"CPU", EISA_SLOT_DATA, 0))
						neng++;
			}

			if (neng == sp_maxnumcpu)
				return neng;
		}

		/*
		 * If we cannot find "CPU" string in EISA NVRAM, check the
		 * slots for the boardid listed in sp_eisa_id.
		 */
		if (neng == 0) {
			neng = BASE_PROCESSOR + 1;
			for (i = 1; i < sp_maxnumcpu; i++) {
				for (slot = 1; slot < EISA_MAX_SLOTS; slot++) {
					if (spxl_findeng(slot, neng)) {
						neng++;
						break;
					}
				}
			}
		}

		for (i = BASE_PROCESSOR + 1; i < neng; i++) {
			spxl_resetcpu(i);
		}

		ASSERT(neng <= sp_maxnumcpu);
		return (neng);

	} else if (cpq_mptype & CPQ_PROLIANT) {
		int	cpu;

		/* Do this if it is a proliant */

		xl_check_evs();

		for (cpu = 1; cpu < xl_num_OK_CPUs; cpu++) {
			/*
			 * Is it a i486 or Pentium?
			 */
			cmn_err(CE_CONT, "!CPU %d type = %s\n", cpu,
				(cqhcpu[cpu].type == 4) ? "486" : "Pentium");
		}

		neng = xl_num_OK_CPUs;

		for (i = BASE_PROCESSOR + 1; i < neng; i++) {
			spxl_resetcpu(i);
		}

		return (neng);
	}
}


/*
 * STATIC int
 * spxl_ckidstr(char *, char *, int)
 *
 * Calling/Exit State:
 *      Return 1, if the string <strp> is found within 
 *	(addrp) to ((addrp) + (cnt)) range, otherwise return 0.
 */
STATIC int
spxl_ckidstr(char *addrp, char *strp, int cnt)
{
        int     tcnt;


	for (tcnt = 0; tcnt < cnt; tcnt++) {
		if (*addrp != strp[0]) {
			addrp++;
			continue;
		}

		if (strncmp(addrp, strp, strlen(strp)) == 0) {
			cmn_err(CE_CONT, "!Found COMPAQ string...\n");
			return (1);
		}
		addrp++;
        }

        return (0);
}


/*
 * int
 * spxl_presense(void)
 *
 * Calling/Exit State:
 *	Sets <cpq_mptype> to CPQ_SYSTEMPROXL if it detects a Systempro/XL or
 *	CPQ_PROLIANT if it detects a Proliant system.
 *
 *	Called from psm_configure().
 */
int
spxl_presense(void)
{
	int	rv = 0;
	int	id;
	long	system_id;
	caddr_t	addr;

	
	system_id = inl(ID_PORT);

        /*
         * Determine if its an COMPAQ machine (search
         * 0xfe000-0xfe100 for "COMPAQ" string).
         */

        addr = (caddr_t)physmap((paddr_t)0xFE000, 0x100, KM_SLEEP);
        if (spxl_ckidstr((char *)addr, "COMPAQ", 0x100)) {

		if (system_id == SP_XL_ID) {
			rv = 1;
			cpq_mptype = CPQ_SYSTEMPROXL;
			cmn_err(CE_CONT, "!Found Systempro/XL...\n");
		} else {
			for (id = 0; id < proliant_ids; id++) {
				if (proliant_id[id] == system_id) {
					cpq_mptype = CPQ_PROLIANT;
					cmn_err(CE_CONT, "!Found PROLIANT...\n");
					rv = 1;
				}
			}
		}
	}
	physmap_free(addr, 0x100, 0);

	return rv;
}

#define OK			0
#define NOT_OK 			(-1)
#define ROM_CALL_ERROR 		0x01
#define CALL_NOT_SUPPORTED 	0x86
#define EV_NOT_FOUND 		0x88
#define GET_EV 			0xD8A4

/*
 * int 
 * xl_check_evs(void)
 *	Check environment variables found in EISA NVM.
 *
 * Calling/Exit State:
 *	Returns OK if ASR functionality is in an EV, else return NOT_OK
 *
 *	Called from spxl_numeng().
 *
 * Remarks:
 *	"CQHCPU" EV returns the number of cpus present in the system.
 *	"CQHCPF" EV returns the state of cpus that are present, but have
 *		 failed the POST (Power On Self Test). A "1" indicates
 *		 that a cpu has failed post and a "0" indicates that a cpu
 *		 has passed post.
 */
int
xl_check_evs(void)
{
	int length;
	int itmp;
	int err_code = 1;


	length = 24;

	if (xl_get_ev("CQHCPU", (char *)cqhcpu, &length) != OK)
		err_code--;
	
	xl_num_OK_CPUs = length / 3;	/* length / sizeof(struct cqhcpu) */

	DEBUG1(("xl_check_evs: length=0x%x, cpus=0x%x\n", length, xl_num_OK_CPUs));
	
	length = 1;

	if (xl_get_ev("CQHCPF", &cqhcpf, &length) != OK)
		err_code--;

	DEBUG1(("xl_check_evs: length=%d, cqhcpf=%x\n", length, cqhcpf));

	for (itmp = 0; itmp < xl_maxnumcpu; itmp++) {
		if ((cqhcpf >> itmp) & 0x01) {
			xl_num_OK_CPUs--;
			/*
			 *+ The processor is unoperational. It failed a
			 *+ Power On Self Test (POST).
			 */
			cmn_err(CE_WARN, 
				"!Processor slot %d failed", itmp);
		}
	}

	DEBUG1(("xl_check_evs: err_code=%d, cpus=%d\n", err_code, xl_num_OK_CPUs));

	return(err_code);
}


/*
 * int
 * xl_get_ev(char *ev_name_string, char *rbuf, int *length)
 *	Get an environment variable (EV) from EISA NV MEM.
 *
 * Calling/Exit State:
 *	Intput:
 *		ev_name_string	- points to the NULL terminated EV name string
 *
 *	Output:
 *		rbuf		- points to target buffer for EV value
 *		length		- length of EV string
 *
 *	Return:
 *		OK if EV completed successfully, else return NOT_OK
 */
int
xl_get_ev(char *ev_name_string, char *rbuf, int *length)
{
        regs reg;
        int status = OK;


        bzero(&reg, sizeof(regs));
        reg.eax.eax = GET_EV;
        reg.ebx.ebx = 0x0;
	reg.ecx.ecx = *length;
	reg.edx.edx = 0x0;
	reg.esi.esi = (unsigned long) ev_name_string;
	reg.edi.edi = (unsigned long) rbuf;

	/* make INT 15 ROM call */
	status = eisa_rom_call(&reg);

	/* save the count of data in the return buffer */
	*length = reg.ecx.ecx;

	/* check for error conditions */
	if ((reg.eflags & ROM_CALL_ERROR) == 1) {

		if (reg.eax.byte.ah == CALL_NOT_SUPPORTED) {
			/*
			 *+ Call not supported.
			 */
			cmn_err(CE_WARN, 
				"!xl_get_ev: get EV call not supported");
			status = NOT_OK;

		} else if (reg.eax.byte.ah == EV_NOT_FOUND) {
			/*
			 *+ Unsupported event.
			 */
			cmn_err(CE_WARN, 
				"!xl_get_ev: EV %s not found", ev_name_string);
			status = NOT_OK;
		}

	} else if (reg.eax.byte.ah != 0x0) {
		/*
		 *+ ROM call error.
		 */
		cmn_err(CE_WARN, 
			"!xl_get_ev: error with get EV ROM call");
		status = NOT_OK;
	}

        return(status);
}


/*
 * void
 * spxl_eisa_set_elt(int engnum, int irq, int mode)
 *	Sets mode (edge- or level-triggered) for a given interrupt line
 *	on the engnum.
 *
 * Calling/Exit State:
 *	On entry, irq is the interrupt line whose mode is being set
 *	and mode is the desired mode (either EDGE_TRIG or LEVEL_TRIG).
 *
 * Remarks:
 *	IRQ's 0, 1, 2, 8 and 13 are always set to EDGE_TRIG regardless of
 *	the mode argument passed in.
 *
 * Note:
 *	<engnum> must not be equal to <myengnum>
 */
void
spxl_eisa_set_elt(int engnum, int irq, int mode)
{
	int i, port;


	ASSERT(engnum != myengnum);

	switch (irq) {
	case 0:
	case 1:
	case 2:
	case 8:
	case 13:
		return;
	}

	if (irq > 7) {
		irq -= 8;
		port = ELCR_PORT1;
	} else {
		port = ELCR_PORT0;
	}

	i = spxl_read_indexed(engnum, port);
	i &= ~(1 << irq);
	i |= (mode << irq);
	spxl_write_indexed(engnum, port, i);

	return;
}
