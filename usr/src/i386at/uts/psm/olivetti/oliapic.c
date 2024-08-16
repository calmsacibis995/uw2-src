/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/olivetti/oliapic.c	1.7"
/*	Copyright (c) 1993 UNIX System Laboratories, Inc. 	*/
/*	  All Rights Reserved                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.   	            	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1994 Ing. C. Olivetti & C., Inc.		*/


#ifdef _KERNEL_HEADERS
#include <svc/bootinfo.h>
#include <svc/eisa.h>
#include <psm/olivetti/olivetti.h>
#include <psm/olivetti/oliapic.h>
#include <svc/pic.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

/* this is used in PSK environment */

#include <sys/bootinfo.h>
#include <sys/eisa.h>
#include "olivetti.h"
#include "oliapic.h"
#include <sys/pic.h>
#include <sys/systm.h>
#include <sys/cmn_err.h>
#include <sys/inline.h>
#include <sys/ipl.h>
#include <sys/plocal.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#endif /* _KERNEL_HEADERS */

extern int level_intr_mask;
extern k_pl_t ipl;

/* Programmable Interrupt Controllers */

/* defined in conf.c, which is generated by config: */
extern void (*ivect[])();	/* interrupt routines */
extern uchar_t intpri[];	/* priority levels for interrupts */
extern int nintr;		/* number of interrupts */

/* defined in space.c file: */
extern ushort_t cmdport[];	/* command port addrs for pics */
extern ushort_t imrport[];	/* intr mask port addrs for pics */
extern uchar_t masterpic[];	/* index of this pic's master */
extern uchar_t masterline[];	/* line this pic connected to */
extern uchar_t curmask[];	/* current masks for pics */
extern uchar_t picbuffered;	/* true if pic buffered */
extern int npic;		/* number of pics configured */
extern struct irqtab irqtab[];	/* per-IRQ info */
extern pl_t svcpri[];		/* interrupt service priority levels */
extern void clock();
extern void psm_intr();
extern int intcpu[];
extern int intmp[];

/*
 * iplmask[] contains the pic masks for each interrupt priority level.
 * It is effectively dimensioned iplmask[PLHI + 1][NPIC], and is initialized
 * from intpri[].
 *
 * Since code always runs at PLHI or below, we only need entries in iplmask
 * for PLBASE through PLHI, thus iplmask is dimensioned up to PLHI only.
 */
extern uchar_t iplmask[];

void intnull(void);		/* null interrupt routine */

/*
 * The deferred interrupt stack and stack index.  The size of the deferred
 * interrupt stack is based on the following considerations:
 *
 *  (1)	At most one interrupt per service level can be deferred.  This
 *	is guaranteed because whenever an interrupt occurs, the system
 *	blocks all interrupts at the interrupt's service level, and the
 *	interrupts remain blocked until the service of the interrupt
 *	is completed.
 *
 *  (2)	Only interrupts whose interrupt priority level is at or below PLHI
 *	will be deferred.  This is guaranteed because the system always
 *	runs at PLHI or below, thus interrupts whose level is above PLHI
 *	will never be deferred.
 *
 *  (3)	The implication of (1) & (2) is that the maximum number of interrupts
 *	which will be deferred at any one time is equal to the number of
 *	priority levels from PL1 to PLHI, inclusive.  Since this implementation
 *	uses ordinal numbers to represent priority levels, we can say that the
 *	maximum number of deferred interrupts is PLHI - PL1 + 1, or just PLHI.
 *
 *  (4)	There is always a dummy value at the bottom of the deferred interrupt
 *	stack; see the initialization of picdeferred below.  Thus, the total
 *	size of picdeferred is PLHI + 1: PLHI for the maximum number of
 *	deferred interrupts, plus 1 for the dummy value.
 */
int picdeferred[PLHI + 1];	/* deferred interrupt stack */
int picdeferndx;		/* deferred interrupt stack index */

static int slaves;		/* bitmask of slave lines into master PIC */

volatile uint_t iv_cookie;
STATIC void holdcpus(void *);
extern int picreload(void);

/*
 *	Disable the PIC on the motherboard.
 *	On exit, ipl and is set to PLBASE and a "sti" is done
 */

void
pic_off(void)
{
	int cmd, imr, pic, bit;
	struct irqtab *ip;
	int i;

	/*
	 * Identify lines on master to which slaves are connected.
	 */
	slaves = 0;
	for (pic = 1; pic < npic; pic++)        /* for each slave */
		slaves |= 1 << masterline[pic];

	/*
	 * Initialize the irqtab, which contains per-vector information.
	 */
	ip = irqtab;
	for (pic = 0; pic < npic; pic++) {	/* loop thru PICs */
		for (bit = 1; bit <= 0x80; bit <<= 1, ip++) {
			ip->irq_cmdport = cmdport[pic];
			ip->irq_flags = 0;
			if (pic != 0)
				ip->irq_flags |= IRQ_ONSLAVE;
			if (bit == PIC_IRQSPUR)
				ip->irq_flags |= IRQ_CHKSPUR;
		}
	}

	/*
	 * Initialize the PIC hardware, starting with the master PIC.
	 */

	/* ICW1: Edge-triggered, Cascaded, need ICW4 */
	outb(cmdport[0], PIC_ICW1BASE|PIC_NEEDICW4);
	
	/* ICW2: start master vectors at PIC_VECTBASE */
	outb(imrport[0], PIC_VECTBASE);
	
	/* ICW3: define which lines are connected to slaves */
	outb(imrport[0], slaves);

	/* ICW4: buffered master (?), norm eoi, mcs 86 */
	outb(imrport[0],
	     picbuffered ? PIC_MASTERBUF|PIC_86MODE : PIC_86MODE);

	/* OCW1: Start the master with all interrupts off */
	outb(imrport[0], curmask[0] = MASKOFF);
	
	/* OCW3: set master into "read isr mode" */
	outb(cmdport[0], PIC_READISR);
	
	/*
	 * Initialize slave PICs
	 */
	for (pic = 1; pic < npic; pic++) {
		cmd = cmdport[pic];
		imr = imrport[pic];

		/* ICW1: Edge-triggered, Cascaded, need ICW4 */
		outb(cmd, PIC_ICW1BASE|PIC_NEEDICW4);

		/* ICW2: set base of vectors */
		outb(imr, PIC_VECTBASE + pic * 8);

		/* ICW3: specify ID for this slave */
		outb(imr, masterline[pic]);

		/* ICW4: buffered slave (?), norm eoi, mcs 86 */
		outb(imr,
		     picbuffered ? PIC_SLAVEBUF|PIC_86MODE : PIC_86MODE);

		/* OCW1: start the slave with all interrupts off */
		outb(imr, curmask[pic] = MASKOFF);

		/* OCW3: set pic into "read isr mode" */
		outb(cmd, PIC_READISR);
	}

	/*
	 * initialize ipl
	 */
	ipl = PLBASE;

	/*
	 * Initialize the deferred interrupt stack.  In order to simplify
	 * the testing of the deferred interrupt stack, the stack always
	 * contains a dummy interrupt number as the bottom-most element
	 * of the stack.  This dummy interrumpt number must be distinct
	 * from any valid interrupt (IRQ) number and also must have the
	 * property that its intpri entry is 0.  The value which is used
	 * is one more than the highest possible IRQ number; this is just
	 * npic * PIC_NIRQ.
	 *
	 * Set picdeferndx to 0 to indicate we're at the bottom of the
	 * stack, and set the bottom-most element to npic * PIC_NIRQ;
	 */
	picdeferndx = 0;
	picdeferred[picdeferndx] = npic * PIC_NIRQ;

	/* Initially set all masks to disable all interrupts */
	for (i = (PLHI + 1) * npic; i-- != 0;)
		iplmask[i] = MASKOFF;

	asm("sti");	/* ENABLE */
}

int apic_intpri[16] = {0,0,0,0, 0,0,1,2, 3,4,5,6, 7,8,8,8};
int apic_enable = 0;
int clock_intr_vector;

int apic_primask[] = {
	TPRI0, TPRI1, TPRI2, TPRI3,
	TPRI4, TPRI5, TPRI6, TPRI7,
	TPRIHI
};

void	(*apic_vecta[AP_EIRQS])();
int	apic_vect_xln[AP_EIRQS];
ulong	apic_redir[AP_IRQS];	/* Copy of the redirection table */
int	apic_cpu[AP_IRQS];	/* CPU where the IRQ is assigned to */


/* Where to put vectors for different prioroity levels */
int apic_frees[SOFT_LVL][LVL0_SHIFT] = {
	{0},					/* spl0	     */
	{0,0,0,0, 0,0,0xffff},                  /* spl1 0x60 */
	{0,0,0,0, 0,0,0,0xffff},                /* spl2 0x70 */
	{0,0,0,0, 0,0,0,0, 0xffff},             /* spl3 0x80 */
	{0,0,0,0, 0,0,0,0, 0,0xffff},           /* spl4 0x90 */
	{0,0,0,0, 0,0,0,0, 0,0,0xffff},         /* spl5 0xA0 */
	{0,0,0,0, 0,0,0,0, 0,0,0,0xffff},       /* spl6 0xB0 */
	{0,0,0,0, 0,0,0,0, 0,0,0,0, 0xffff},    /* spl7 0xC0 */
 	{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0xffff},  /* spl8 0xD0 */
};

#ifndef CPUANY
#define CPUANY	(-1)
#endif

void
apic_stray(vec)
{
	cmn_err(CE_WARN, "weird interrupt at %x\n", vec);
}

/*
 * void picstart(void)
 *	Enable normal interrupt masks and allow device interrupts.
 *
 * Calling/Exit State:
 *	Called from selfinit() during processor initialization.
 *
 * Remarks:
 *	Now we can enable interrupts.  This requires both an ENABLE
 *	to enable interrupts at the processor and an spl0 to set the
 *	priority level to PLBASE.
 */
void
apicstart(void)
{
	ipl = PLHI;	/* Force the spl0 to change PIC masks */
	spl0();
}

/*
 * void ndisableint(int iv, pl_t level, int engnum, int itype)
 *	Disable interrupts of specified interrupt number.
 *
 * Calling/Exit State:
 *	Called and exit with mod_iv_lock held at PLHI.
 */
void
ndisableint(int iv, pl_t level, int intcpu, int itype)
{
        struct emask iv_emask;
        int     group, slot;

        if (nonline > 1)
                xcall_all(&iv_emask, B_FALSE, holdcpus,
                          (void *)iv_cookie);

        for(slot=0; slot<LVL0_SHIFT; slot++) {
                for (group=0; group<LVL0_SHIFT; group++) {
                        if (apic_vect_xln[group*LVL0_SHIFT+slot] == iv)
                                break;;
		}
		if (group < LVL0_SHIFT)
			break;
	}
	if (slot == LVL0_SHIFT)
		cmn_err(CE_PANIC, "cannot find active apic slot for %d\n",
									iv);
        apic_frees[level][group] |= (1<<slot);
        apic_vecta[group*LVL0_SHIFT+slot] = apic_stray;
        apic_vect_xln[group*LVL0_SHIFT+slot] = 0;

	AP_IO_REG = AP_RDEST(iv);
        AP_IO_DATA = AV_MASK;
	apic_redir[iv] = AV_MASK;

	/*
	 * Reload pic masks so that mask modifications take
	 * effect immediately.
	 */
	picreload();

	++iv_cookie;
}

/*
 * void nenableint(int iv, pl_t level, int intcpu, int intmp, int itype)
 *	Enable interrupts of specified interrupt number.
 *	Typically, called when an interrupt driven driver is
 *	loaded via "modadmin".
 *
 * Calling/Exit State:
 *	Called and exit with mod_iv_lock held at PLHI.
 */
void
nenableint(int iv, pl_t level, int intcpu, int intmp, int itype)
{
        struct emask iv_emask;
        int group, slot, trlevel;
	unsigned int triggermode;

        if (nonline > 1)
                xcall_all(&iv_emask, B_FALSE, holdcpus,
                          (void *)iv_cookie);

	if (bootinfo.machflags & EISA_IO_BUS) {
                if (itype == 4) {
                        eisa_set_elt(iv, LEVEL_TRIG);
                }
        }

	triggermode = (unsigned int) inb(ELCR_PORT0) | 
		      (unsigned int) inb(ELCR_PORT1) << 8;
        /*
         * allocate one of the (AP_EIRQS) 256 interrupt vector number.
         */
        if (level >= 7)
                level = 8;
        for(slot=0; slot<LVL0_SHIFT; slot++) {
		for (group=0; group<LVL0_SHIFT; group++) {
			if ((1<<slot) & apic_frees[level][group])
				break;
		}
		if (group < LVL0_SHIFT)
			break;
	}

	if (group == LVL0_SHIFT)
		cmn_err(CE_PANIC, "cannot allocate apic slot for %d\n", iv);

	apic_frees[level][group] &= ~(1<<slot);
	apic_vect_xln[group*LVL0_SHIFT+slot] = iv;
	apic_vecta[group*LVL0_SHIFT+slot] = ivect[iv];

	trlevel = (triggermode & (1<<iv)); /* 1: LEVEL, 0: EDGE */

	if (intcpu == -1) {
		if (intmp == 1) {
			apic_redir[iv] = (LVL0_SHIFT*group+slot) | AV_LOPRI
			    | AV_LDEST | (trlevel ? AP_TR_LEVEL : AP_TR_EDGE);
			apic_cpu[iv] = -1;

			AP_IO_REG = AP_RDEST(iv);
			AP_IO_DATA = AV_TOALL;

			AP_IO_REG = AP_RDIR(iv);
			AP_IO_DATA = apic_redir[iv];
		} else {
			apic_redir[iv] = (LVL0_SHIFT*group+slot) | AV_FIXED
			    | AV_LDEST | (trlevel ? AP_TR_LEVEL : AP_TR_EDGE);
			apic_cpu[iv] = BOOTENG;

			AP_IO_REG = AP_RDEST(iv);
			AP_IO_DATA = CPU2LDEST(BOOTENG); /* bind to BOOT engine */

			AP_IO_REG = AP_RDIR(iv);
			AP_IO_DATA = apic_redir[iv];
		}
	} else {	/* will be bound to intcpu */
			apic_redir[iv] = (LVL0_SHIFT*group+slot) | AV_FIXED 
			    | AV_LDEST | (trlevel ? AP_TR_LEVEL : AP_TR_EDGE);
			apic_cpu[iv] = intcpu;

			AP_IO_REG = AP_RDEST(iv);
			AP_IO_DATA = CPU2LDEST(intcpu); /* bind to intcpu eng */

			AP_IO_REG = AP_RDIR(iv);
			AP_IO_DATA = apic_redir[iv];
        }

	/*
	 * Reload pic masks so that mask modifications take
	 * effect immediately.
	 */
	picreload();

	/* Done with pic mask change, release all the cpus */

	++iv_cookie;
}

/*
 * STATIC void holdcpus(void *arg)
 *	Hold the cpu executing this routine until the value
 *	of the iv_cookie changed.
 *
 * Calling/Exit State:
 *	Called through xcall, the cpu is already in PLHI.
 */
STATIC void
holdcpus(void *arg)
{
	while (iv_cookie == (uint_t)arg)
		;
	picreload();
}


/*
 * void
 * intnull(void)
 *	Null interrupt routine.  Used to fill in empty slots in the
 *	interrupt handler table.
 *
 * Calling/Exit State:
 *	None.
 */
void
intnull(void)
{
}

/*
 * Assumptions for this version of picinit (for APIC):
 *
 * 1) There is only one i/o APIC
 * 2) The i/o APIC will be initialized by cpu 0 for everybody
 * 3) Interrupts go to a single processor or any processor
 * 4) priority is the same for all processors
 */

apic_susp()
{
	AP_LDEST = AV_IM_OFF;
}

apic_resu()
{
	AP_LDEST = CPU2LDEST(myengnum);
}

/*
 * apicinit()
 *
 * Initialize the apic.
 *
 * Called by picinit() on machines with APIC instead of PIC.
 */

apicinit()
{
	int intno;

	/*
	 * The following has only to be done by BOOTENG.
	 */
	if (myengnum == BOOTENG) {
		/*
		 * map physical 0xfee00000 to apic_vbase. Olivetti 5050
		 * hardware maps I/O units AND local units at this address
		 */

		apic_vbase = (unsigned long *)physmap0(APICADR, MMU_PAGESIZE);

		AP_IO_REG = AV_DESVER;  /* select I/O version reg */
		if ((AP_IO_REG == 1) && ((AP_IO_DATA & 0xff0000) == 0x0f0000))
			apic_enable = 1;
		else
			cmn_err(CE_PANIC, "Failed to enable APIC\n");
		pic_off();
		apic_setiplmask();
		apic_setcpumask();
	} else {
		ipl = PLBASE;
	}

	AP_TASKPRI = TPRIHI;

	/* mask un-used slots */
	for (intno=1; intno < AP_IRQS; intno++) {
		AP_IO_REG = AP_RDIR(intno);
		AP_IO_DATA = AV_MASK;
	}
	asm("sti");
	AP_IO_REG = AP_RDIR(0);
	AP_IO_DATA = AV_MASK;	/* now mask old pic */
	asm("	cli");

	/*
	 * Set the Redirection Table Registers
	 */
	for (intno=0; intno < AP_IRQS; intno++) {
		unsigned long luid, redir;

		if (ivect[intno] == psm_intr)
			continue;
		if (intpri[intno] == 0)
			continue;

		redir = apic_redir[intno];

		if (!(redir & AV_MASK)) {
			if ((apic_cpu[intno] != -1) \
			     && (apic_cpu[intno] != myengnum))
				continue;

			/* if interrupt is in lowest priority, then let */
			/* it be sent on BOOTENG and masked on others. */
			/* BOOTENG's APIC  will send request to others */

			if (redir & AV_LOPRI) {
				if (myengnum != BOOTENG)
					continue;
				luid = AV_TOALL;
			} else
				luid = CPU2LDEST(apic_cpu[intno]);
		}

		/*
		 * Timer interrupt is managed by a specific register,
		 * so it does not go through the redirection table.
		 */
		if (ivect[intno] == clock)
			continue;

		AP_IO_REG = AP_RDEST(intno);
		AP_IO_DATA = luid;

		AP_IO_REG = AP_RDIR(intno);
		AP_IO_DATA = redir;
	}

	/* synchronize all arbitration ids together */

	while (AP_ICMD & AV_PENDING)	/* waits for IPC delivery */
		;

	AP_ICMD = AP_DL_RESET|AP_DST_PHYS
		   | AP_LV_DEASSERT|LEVEL_TRIG|AP_DSH_ALL;

	/*
	 * Keep timer interrupt disabled. It will be enabled by apic_clkstart().
	 * 
	 * For compatibility, set the timer interrupt on IRQ0, 
	 * as on standard PCs.
	 */

	AP_LVT_TIMER = AV_MASK;

	/*
	 * Disable local interrupts 0 and 1
	 *
	 * LINT 0 is connected to the motherboard PIC for the compatibility
	 * mode.
	 *
	 * LINT 1 is for an external coprocessor.	More infos are needed.
	 */

	AP_LVT_I0 = AV_MASK;
	AP_LVT_I1 = AV_MASK;

	/*
	 * Finally, enable the local unit
	 */
	AP_DESTFMT = -1;		/* Enable addressing scheme */
	AP_LDEST = CPU2LDEST(myengnum); /* Set Logical Dest Reg */
	AP_SPUR = AP_UNITENABLE | AP_SPURIOUSVECT;
}

/************************************************************************/

/*
 * apic_setiplmask()
 * This routine builds an array (apic_redir), initializing all
 * interrupts to be FIXED priority, with their correct level (level/edge).
 * It' s the purpouse of apic_setcpumask to recognize which interrupts
 * are LOW priority (non hardware bound AND multi-threaded), which ones
 * are bound to BOOTENG (should make this configurable) for beeing
 * non hardware bound but also NOT multithreaded and finally which ones
 * are bound to the cpu specified in intcpu.
 */

apic_setiplmask()
{
	int intno, trlevel, group, slot;
	unsigned int triggermode;
	unsigned char pri;
	void (*vec)();

	/* (1<<n) & triggermode ==> intr n is level triggered else edge */
	triggermode = (unsigned int) inb(ELCR_PORT0) | 
		      (unsigned int) inb(ELCR_PORT1) << 8;

	/*
	 * Set up apic_redirp[] from ivect[] (intrv[]) and intpri[].
	 * If an interrupt number is not configured, set its level to
	 * 0 (level 0 is always DISABLED); otherwise set its vector as
	 * level * 16 + intno (use apic priority level to emulate spl)
	 */
	for (intno = 0; intno < AP_EIRQS; intno++)
		apic_vecta[intno] = apic_stray;

	apic_vecta[AP_IPCVECT] = psm_intr;

	for (intno=0; intno < AP_IRQS; intno++) {
		pri = intpri[intno];
		if (pri == 0) {
			/* For interrupts that we never expect, */
			/* give them a valid priority anyway.   */
			/* Priority 0 (it will never be called) */
			/* and masked = true.			*/

			apic_redir[intno] = AV_MASK;
			continue;
		} 
		vec = ivect[intno];
		if (vec == psm_intr)
			continue;
		if (pri >= 7)
			pri = 8;
		for(slot=0; slot < LVL0_SHIFT; slot++) {
			for (group=0; group < LVL0_SHIFT; group++) {
				if ((1<<slot) & apic_frees[pri][group])
					break;
			}
			if (group < LVL0_SHIFT)
				break;
		}

		if (slot == LVL0_SHIFT)
			cmn_err(CE_PANIC, "apic_init: couldn' t allocate slot");

		apic_frees[pri][group] &= ~(1<<slot);
		apic_vect_xln[group*LVL0_SHIFT+slot] = intno;
		apic_vecta[group*LVL0_SHIFT+slot] = vec;

		if (vec == clock) {
			AP_LVT_TIMER = (LVL0_SHIFT*group+slot) | AV_MASK;
			clock_intr_vector = LVL0_SHIFT*group+slot;
			/* will be un-masked in clkstart() */
			continue;
		} else {
			/* 1: LEVEL, 0: EDGE */
			trlevel = (triggermode & (1<<intno));
			apic_redir[intno] = (LVL0_SHIFT*group+slot) | AV_LDEST
				| (trlevel ? AP_TR_LEVEL : AP_TR_EDGE);
		}
	}
}

/*
 * See comments for apic_setiplmask
 */
apic_setcpumask()
{
	int cpu, pic, bit, intno;
	unsigned int redir;

	/*
	 *  Check for bound interrupts and drivers
	 * 
	 */
	for (intno = 0; intno < AP_IRQS; intno++) {
		/*
		 * Look at the driver state structure to find out which
		 * CPU a driver is bound to.
		 */
		if (intcpu[intno] == -1) { /* random */
			if (intmp[intno] == 1) { /* multi-threaded */
				apic_cpu[intno] = -1; /* will be set ALLCPUS */
				/* so, declare this intr as low-priority */
				apic_redir[intno] |= AV_LOPRI;
				
			} else { /* fixed priority, bound to BOOTENG */
				apic_redir[intno] |= AV_FIXED;
				apic_cpu[intno] = BOOTENG; 
			}
		} else {
			apic_redir[intno] |= AV_FIXED;
			apic_cpu[intno] = intcpu[intno]; /* to intcpu[intno] */
		}
	}
}
/*
 * waits for IPC delivery and then writes 'x' on local unit's
 * interrupt command register
 */
void
apic_lcmdwr(x)
{
	pl_t pl;

	pl = splhi();
	while (AP_ICMD & AV_PENDING)
		;
	AP_ICMD = x;
	while (AP_ICMD & AV_PENDING)
		;
	splx(pl);
}

/* called by psm_online(cpu) to start a cpu */

void
apic_reset_cpu(cpu)
{

	while (AP_ICMD & AV_PENDING)	/* waits for IPC delivery */
		;

	AP_ICMD2 = cpu;

	while (AP_ICMD & AV_PENDING)	/* waits for IPC delivery */
		;

	AP_ICMD = AP_DL_RESET|AP_DST_PHYS          /* == 0x8500 */
	   |AP_LV_DEASSERT|LEVEL_TRIG|AP_DSH_DEST;

	while (AP_ICMD & AV_PENDING)
		;

	AP_ICMD = AP_DL_RESET|AP_DST_PHYS	   /* ==0xc500 */
	   | AP_LV_ASSERT|LEVEL_TRIG|AP_DSH_DEST;

	while (AP_ICMD & AV_PENDING)
		;

	AP_ICMD = AP_DL_RESET|AP_DST_PHYS          /* == 0x8500 */
	   |AP_LV_DEASSERT|LEVEL_TRIG|AP_DSH_DEST;
}

/* send an inter-processor interrupt using the IPC APIC mechanism */

apic_sendxintr(cpu)
{
	pl_t pl;

	pl = splhi();

	while (AP_ICMD & AV_PENDING)	/* waits for IPC delivery */
		;
	AP_ICMD2 = cpu;
	while (AP_ICMD & AV_PENDING)	/* waits for IPC delivery */
		;
	AP_ICMD = AP_IPCVECT;
	while (AP_ICMD & AV_PENDING)	/* waits for IPC delivery */
		;
	splx(pl);
}

ipl0panic2()
{
	cmn_err(CE_PANIC,
  "ret_user: interrupt priority level not zero at return to user mode\n");
}

splpanic2(s)
{
	cmn_err(CE_PANIC,
		"splx(%d): logic error in misc.s\n", s);
}

#ifdef DEBUG
#ifdef SLOWSPL
spl7panic2()
{
	cmn_err(CE_PANIC,
		"spl7: logic error in misc.s\n");
}
#endif /* SLOWSPL */

splintpanic2()
{
	cmn_err(CE_PANIC,
		"splint: logic error in misc.s\n");
}

splxintpanic2()
{
	cmn_err(CE_PANIC,
		"splxint: logic error in misc.s\n");
}

setpicmaskspanic2()
{
	cmn_err(CE_PANIC,
		"setpicmasks: logic error in misc.s\n");
}
#endif /* DEBUG */


apic_xmsg(x)
{

        asm ("pushfl");
        asm ("cli");

        while (AP_ICMD & AV_PENDING)
                ;
        AP_ICMD = x;
        while (AP_ICMD & AV_PENDING)
                ;

        asm ("popfl");
}


apic_xmsg2(x,x2)
{

        asm ("pushfl");
        asm ("cli");

	AP_ICMD2 = x2;
	while (AP_ICMD & AV_PENDING)
		;
	AP_ICMD = x;
	while (AP_ICMD & AV_PENDING)
		;

        asm ("popfl");
}

apic_llook()
{
	long i, j, k, x;

	cmn_err(CE_CONT, "APIC registers\n");
	for (i=0; i<=0x3f; i++) {
		if ((i&3) == 0)
			cmn_err(CE_CONT, "%x: ", i);
		x = apic_vbase[i*4];
		cmn_err(CE_CONT, "%x ", x);
		if ((i&3) == 3)
			cmn_err(CE_CONT, "\n");
	}
}

apic_rlook()
{
	volatile long *p;
	long i, j, k, x;
	int xcpuid;

		xcpuid = 1;
	cmn_err(CE_CONT, "Remote APIC registers\n");
	for (i=0; i<=0x3f; i++) {
		if ((i&3) == 0)
			cmn_err(CE_CONT, "%x: ", i);
		apic_xmsg2(i | 0x300, (2*xcpuid+1)<<24);
		while ((apic_vbase[4*0x30] & 0x30000) == 0x10000)
			;
		if ((apic_vbase[4*0x30] & 0x30000) == 0x20000)
			x = apic_vbase[0xc*4];
		else
			x = 0xabcdef01;
		cmn_err(CE_CONT, "%x ", x);
		if ((i&3) == 3)
			cmn_err(CE_CONT, "\n");
	}
}

apic_ilook()
{
	long i, j, k, x;

	cmn_err(CE_CONT, "I/O APIC registers\n");
	for (i=0; i<=0x2f; i++) {
		if ((i&3) == 0)
			cmn_err(CE_CONT, "%x: ", i);
		*apic_vbase = i;
		x = apic_vbase[4];
		cmn_err(CE_CONT, "%x ", x);
		if ((i&3) == 3)
			cmn_err(CE_CONT, "\n");
	}
}

/*
 * assumming there is only one IO apic for olivetti machine.
 */
#define	 MAXIOAPIC	1

static void
apic_reset_io(void)
{
        int i, n;

        for (n=0; n<MAXIOAPIC; n++) {
                /* mask unused slots */
		for (i=0; i<AP_IRQS; i++) {
                	AP_IO_REG = AP_RDIR(i);
                	AP_IO_DATA = AV_MASK;
                }
        }
}

#define AV_XTOOTHERS    0x000C0000

apic_reset(void)
{
        int tmp;

	apic_reset_io();
	AP_LDEST = AV_IM_OFF;
	AP_LVT_I0 = AV_MASK;
	AP_LVT_I1 = AV_MASK;
	/*
	 * disable APIC timer interrupts on processor
	 */
	AP_LVT_TIMER = AV_MASK;
	AP_SPUR = 0x0cf;
	tmp = AP_SPUR;
}