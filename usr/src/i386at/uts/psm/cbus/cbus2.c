/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/cbus/cbus2.c	1.13"
#ident  "$Header: $"

#define CBUS2_OEM

#define CBUS_ENABLED_PW				0x1
#define CBUS_DISTRIBUTE_INTERRUPTS		0x2
#define CBUS2_DISABLE_LEVEL_TRIGGERED_INT_FIX	0x4

#define EARLY_PHYSMAP
/*
 *      Copyright (C) Corollary, Inc., 1986-1993.
 *      All Rights Reserved.
 *      This Module contains Proprietary Information of
 *      Corollary, Inc., and should be treated as Confidential.
 */
/*
 *
 *	CBUS-2 open issues (to verify/fix):
 *
 * verify that all cbus2 switch routines are being called at right time/place
 * flesh out pres routine
 * what happens for deferred interrupts (ie: when do we ACK them?)
 */

#define i486
#include <io/prf/prf.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <proc/seg.h>
#include <svc/corollary.h> 
#include <svc/errno.h>
#include <svc/pit.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/kdb/xdebug.h>
#include <util/map.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <io/f_ddi.h>

#include <cbus2.h>

#ifdef CBUS2_OEM
/*
 * definitions for the extended configuration override table.  these
 * really should be unions in corollary.h
 */
#define intrcontrolmask		ci_contend
#define faultcontrolmask	ci_contend_val
#define cbus2features		ci_setida
#define control8259mode		ci_setida_val
#define control8259mode_val	ci_cswi
#endif

/*
 * for CBUS-2, the CBC interrupt hardware supports all 256 interrupt
 * priorities, and unlike the APIC, doesn't disable receipt of interrupts
 * at granularities of 16-deep buckets.  instead the CBC uses the whole byte,
 * instead of 4 bits like the APIC, giving us a granularity of 1.  We
 * use the same priority bucketing as our CBUS APIC scheme (ie: each spl
 * level gets 0x10 possible interrupts.  an important difference to remember
 * is that the spl level to block a given interrupt must be at least as high
 * as the highest priority assigned to that level.  (the APIC ignores the
 * low 4 bits, so the blocking taskpri is just any vector with the high
 * 4 bits equal to or greater than the highest assigned priority at that level.
 */

/*
 * processor traps and various reserved vectors use up the first 0x30 vectors.
 * by ES/MP convention, i8259 vectors use 0x40 through 0x5F.  so start
 * our APIC/CBC vectors at 0x60.  see the picture below:
 *
 *	spl0 all interrupts enabled:	0x60
 *	spl1:				0x70
 *	spl2:				0x80
 *	spl3:				0x90
 *	spl4:				0xA0
 *	spl5:				0xB0
 *	spl6:				0xC0
 *	spl7:				0xD0
 *	spl8:				0xE0
 *	spl9:				0xE0
 *
 *	Spurious Vector:		0xFF
 *	IPI:				0xEF	(spl8-blockable)
 *
 * note that multiple devices can share an spl mask.
 */

/*
 * The IDT entries are not set up for TASKPRI's 0x50 through 0x5f
 * because there will never be an interrupt that is configured
 * for ipl 0.  The algorithm for allocating vectors adds 0x10 times
 * the ipl to FIRST_DEVICE_TASKPRI to calculate the base task priority
 * group.
 */
#define CBUS2_FIRST_DEVICE_TASKPRI      0x50

/*
 * The next two vectors are private to the CBUS-2 PSM.
 * intr_p.s has pointed the spurious interrupt at an iret, so
 * if we want to change the value from 0xFF, we must edit
 * that file as well.
 */
#define CBUS2_SPURIOUS_TASKPRI	0xFF
#define CBUS2_IPI_TASKPRI	0xEF
#define CBUS2_LOWEST_TASKPRI	0x00

#define EISA_IRQLINES		16

/*
 * A table converting software interrupt ipls to CBUS-2-specific offsets
 * within a given CSR space.  Note that all task priorities are shifted
 * by the CBUS-2 register width (64 bits) to create the correct hardware
 * offset to poke to cause the interrupt.  This table is declared here to
 * optimize the assembly software interrupt request lookup, and is filled
 * in as part of InitializePlatform.
 */

#define CBUS2_REGISTER_SHIFT 3


/*
 * the list of CBUS-2 CBC vector data:
 *
 * InUse is 0 if the entry is unused.
 *
 * irq_line is the irqline this vector maps to.  this is used on interrupt
 * receipt to translate the vector into an irqline the driver can understand.
 *
 * RedirectionAddress is the redirection table entry address to poke to
 * enable/disable this interrupt.
 *
 * BEWARE making size changes to this structure - it is padded to a 0x10 byte
 * entry so assembly code can traverse each entry with a 4 bit shift instead
 * of the expensive imul.  also, the assembly code relies on the internal
 * offsets of fields within the structure.
 */
typedef struct _cbus2_vectors_t {
	ulong		irq_line;	/* irqline this vector is assigned to */
	ulong		InUse;		/* set if this entry is being used */
	PHWINTRMAP	hardware_intr_map; /* hardware intr map entry addr */
	ulong		Pad;
} CBUS2_VECTORS_T, *PCBUS2_VECTORS;

PCSR				crllry_cbus2base[MAXACPUS];
unsigned long			*cbus2_send_ipi[MAXACPUS];
unsigned long			cbus2_clock_vector;
CBUS2_VECTORS_T			cbus2_vector_data[256];
int				cbus2_irqline_to_vector[EISA_IRQLINES];
unsigned long			cbus2_irq_polarity;
unsigned long *			cbus2_tpr[MAXACPUS];
unsigned                        chip_rev;
int				cbus2_fix_level_interrupts = 1;
int				cbus2_eoitohwmap;
unsigned char			cbus2_eoi_array[MAXACPUS][EISA_IRQLINES];
unsigned char *			cbus2_eoi_needed[MAXACPUS];
#define	EOI_NONE	0
#define EOI_EDGE	1
#define EOI_LEVEL	2
unsigned			cbus2_irqtohwmap[EISA_IRQLINES];

extern char			intpri[];
extern unsigned long		svcpri[];
extern void			(*ivect[])();
extern int			intcpu[];
extern int			cbus_booted_processors;
extern int			cbus_booted_processors_mask;
extern int			corollary_cpuids[];
extern volatile uint_t		psm_eventflags[];
extern unsigned long		corollary_spl_to_vector[];
extern int			broadcast_csr;
extern int			xclock_pending;
extern int			prf_pending;
extern uint_t			engine_evtflags;
extern int			plsti;
extern unsigned			corollary_early_boot;
extern int			corollary;

extern void			psm_intr();
extern void			corollary_disable_8259s();
extern void			cbus2_disable_my_interrupts();
extern void			ci_clock();
extern void			ci_setpicmasks_cbus2();
extern void			ci_cbus2_intr();
extern void			softint_cbus2();
extern void			deferred_int_cbus2();
extern void                     cbus2_enable_interrupt();

#ifdef COROLLARY_DEBUG
unsigned			cbus2_last_vector[256];
#endif

extern int      (*corollary_nmi_hook)();

#define CLOCK_IRQ               0

cbus2_pres()
{
	if (find_string("Corollary", 0xFFFE0000, 0xFFFF))
	{
		if ((corollary_global.ci_machine_type & (MACHINE_CBUS2)) == 0)
			return 0;

		/*
		 * reset corollary to disable early boot code in sysinit.
		 */
		corollary = 0;

#ifdef COROLLARY_DEBUG
		cmn_err(CE_CONT, "it's a CBUS2 machine!\n");
#endif
		mpvendor = CBUS2_OEM_COROLLARY;

		mpvendorclass = MP_CBUS2EISA;

		corollary_early_boot = 0;

		return 1;
	}
	return 0;
}

void
cbus2_startcpu(processor, startaddr)
int	processor;
paddr_t startaddr;
{
	long	*io_rom = (long *)physmap(0x467, ptob(1), KM_NOSLEEP);

	*io_rom = (long)startaddr;

	WRITECBUS2((unsigned)crllry_cbus2base[processor] + 
		corollary_global.ci_creset, corollary_global.ci_creset_val);

	physmap_free(io_rom, ptob(1), 0);
}

void
cbus2_set_intr(processor)
{
	(*cbus2_send_ipi[processor]) = 1;
}

/*
 * processor is zero based.  Map its CSR space.
 */
void
cbus2_mappings(processor, idp)
register int processor;
register struct ext_id_info *idp;
{
	corollary_cpuids[processor] = idp->id;

	crllry_cbus2base[processor] = 
		(PCSR)physmap(idp->pel_start, idp->pel_size, KM_NOSLEEP);
}

void
cbus2reset(processor)
int	processor;
{
	WRITECBUS2((unsigned)crllry_cbus2base[processor] + 
		corollary_global.ci_sreset, corollary_global.ci_sreset_val);
}

/*
 * establish CSR mappings for all CBUS-2 processors
 */
void
cbus2_setup()
{
	register int			i, index;
	register struct ext_id_info	*idp = corollary_ext_id_info;

	for (i = 0; i < corollary_valid_ids; i++, idp++)
	{
		if (idp->id == corollary_global.bootid)
		{
			cbus2_mappings(0, idp);
			break;
		}
	}

	corollary_num_cpus = 1;
	idp = corollary_ext_id_info;
	index = 1;
	for (i = index ; i < corollary_valid_ids ; i++, idp++) 
	{
		if (idp->id == corollary_global.ci_broadcast_id)
		{
			/*
			 * map the broadcast ID, each backend must set this
			 * ID value.
			 */
			broadcast_csr = physmap(idp->pel_start, idp->pel_size, KM_NOSLEEP);
			continue;
		}

		if (idp->proc_type == PT_NO_PROCESSOR || idp->pm == 0)
			continue;

		if (idp->id == corollary_global.bootid)
			continue;

		cbus2_mappings(index, idp);

		cbus2reset(index);

		corollary_num_cpus++;

		index++;
	}

#ifdef NOTYET
	cbus_remap_phys();
#endif
}

void
cbus2_findcpus() 
{
#ifdef EARLY_PHYSMAP
	/*
	 * this is really a kludge to pull out the number of processors.
	 * cbus2_setup() is really the right way to get the number.  fix.
	 */
	int i;

#define LOWCPUID 1
#define HICPUID 0xF

	for (i = LOWCPUID; i < HICPUID; i++)
	{
		if (configuration.slot[i])
		{
			corollary_num_cpus++;
		}
	}

#else /* EARLY_PHYSMAP */
	error - code needs to be redone when EARLY_PHYSMAP is ripped out
#endif
}

void
cbus2_ledon(processor)
register int processor;
{
	if (crllry_cbus2base[processor])
		WRITECBUS2((unsigned)crllry_cbus2base[processor] + 
			corollary_global.ci_sled, corollary_global.ci_sled_val);
}

void
cbus2_ledoff(processor)
register int processor;
{
	if (crllry_cbus2base[processor])
		WRITECBUS2((unsigned)crllry_cbus2base[processor] + 
			corollary_global.ci_cled, corollary_global.ci_cled_val);
}

#ifdef COROLLARY_DEBUG
volatile unsigned cbus2_in_debugger;
#endif

dump_regs()
{
        asm("outw       $0x92");        /* EAX */

        asm("movl       %ebx, %eax");   /* EBX */
        asm("outw       $0x92");

        asm("movl       %ecx, %eax");   /* ECX */
        asm("outw       $0x92");

        asm("movl       %edx, %eax");   /* EDX */
        asm("outw       $0x92");

        asm("movl       %esp, %eax");   /* ESP */
        asm("outw       $0x92");

        asm("movl       %ebp, %eax");   /* EBP */
        asm("outw       $0x92");

        asm("pushfl");                  /* FLAGS */
        asm("pop        %eax");
        asm("outw       $0x92");
}


int
cbus2_nmi_handle()
{
#ifdef COROLLARY_DEBUG
        /* dump_regs(); */

        if (myengnum == 0)
        {
                cbus2_in_debugger = 1;

                (*cdebugger)(DR_OTHER, NO_FRAME);

                cbus2_in_debugger = 0;
        }
        else
        {
                while (cbus2_in_debugger == 0)
                        continue;

                (*cdebugger)(DR_SLAVE, NO_FRAME);
        }

        return NMI_BENIGN;
#else
        return NMI_UNKNOWN;
#endif
}


void 
cbus2_intr_init() 
{
	/*
	 * no interrupt latches need to be cleared as part of initialization,
	 * so this routine is no-op.
	 */
	corollary_nmi_hook = cbus2_nmi_handle;
}

void 
cbus2_clr_intr()
{
	/*
	 * CBUS-2 interprocessor interrupts do not need to
	 * be cleared, so this is a no-op.
	 */
}

int 
cbus2_set_nmi() 
{ 
	return 0;		/* no NMI debugging hooks for CBUS-2 just yet */
}

int 
cbus2_clr_nmi() 
{ 
	return 0;		/* no NMI debugging hooks for CBUS-2 just yet */
}

/*
 * Special asm macro to enable interrupts before calling xcall_intr
 */
asm void sti(void)
{
	sti;
}

#pragma	asm partial_optimization sti

/* ARGSUSED */
void
cbus2_intr(oldpl, eax, eip, cs)
uint oldpl, *eax;
uint eip, cs;
{
	if (psm_eventflags[myengnum] != 0)
		engine_evtflags |= atomic_fnc(&psm_eventflags[myengnum]);

	sti();

	xcall_intr();

	if (prf_pending > 0) 
	{
		prf_pending--;
	}

	if (xclock_pending)
	{
#ifdef COROLLARY_DEBUG
		if (atomic_fnc((uint_t *)&xclock_pending) > 1)
		{
#ifdef NEVER
			cmn_err(CE_CONT, "Missed clock interrupt\n");
#endif
		}
#else
		xclock_pending = 0;
#endif
		lclclock(eax);
	}
}


unsigned long			cbus2_bridges_found;
PCSR_REG			cbus2_bridge_eoi[CBUS_MAX_BRIDGES];
PCSR				cbus2_bridge_csr[CBUS_MAX_BRIDGES];

/*
 *
 *		CBUS-2 switch table entry routines begin here
 *
 *
 */

int
cbus2_initialize_vector(irq_line, spllevel)
int irq_line;
int spllevel;
{
	int newvector;
	PCSR bridgecsr = (PCSR)cbus2_bridge_csr[0];

	/*
         * This routine initializes the bridge's hardware interrupt
         * map for the given irq_line.  This routine only needs to
         * be called once for a given irq_line, because the first
         * caller to this routine will setup the bridge's csr.  If
         * the bridge has been set up then cbus2_irqline_to_vector
         * will represent the vector that was chosen for that irq
         * line.
         */
	if (cbus2_irqline_to_vector[irq_line])
                return cbus2_irqline_to_vector[irq_line];

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "cbus2_initialize_vector 0x%x 0x%x\n", irq_line, spllevel);
#endif

	newvector = spllevel * 0x10 + CBUS2_FIRST_DEVICE_TASKPRI;

	while (cbus2_vector_data[newvector].InUse)
		newvector++;

	ASSERT(newvector < (spllevel+1) * 0x10 + CBUS2_FIRST_DEVICE_TASKPRI);

	cbus2_irqline_to_vector[irq_line] = newvector;

	/*
	 * each irqline is assigned an entry based
	 * on its configured spl level.
	 */

	cbus2_vector_data[newvector].InUse = 1;
	cbus2_vector_data[newvector].irq_line = irq_line;
	/*
	 * For each EISA interrupt, point the caller at the
	 * corresponding bridge entry.
	 */
	cbus2_vector_data[newvector].hardware_intr_map =
		(&bridgecsr->hwintrmap[irq_line]);

	/*
	 * irq0 is always built in as the clock vector in the ES/MP
	 * build.  we always want every processor to get these so
	 * we don't have to rebroadcast them in software.  so save the
	 * vector so each processor can initialize his ICR when he
	 * come up to accept these interrupts.
	 */
	if (irq_line == CLOCK_IRQ)
	{
		cbus2_clock_vector = newvector;
	}

	return newvector;
}

void
cbus2_initialize_vectors()
{
	int irq_line, spllevel;

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "cbus2_initialize_vectors\n");
#endif

	/*
	 * first carve out the IPI vector essentials so that IPIs will
	 * be handled properly by the various interrupt dispatcher pieces.
	 */
	cbus2_vector_data[CBUS2_IPI_TASKPRI].InUse = 1;
	cbus2_vector_data[CBUS2_IPI_TASKPRI].irq_line = CBUS2_IPI_TASKPRI;
	ivect[CBUS2_IPI_TASKPRI] = psm_intr;
	intpri[CBUS2_IPI_TASKPRI] = PLMAX;
	svcpri[CBUS2_IPI_TASKPRI] = PLMAX;

	/*
	 * for each irq line, assign a unique vector
	 * which is what a processor will see when
	 * that line generates an interrupt.
	 */
	for (irq_line = 0; irq_line < EISA_IRQLINES; irq_line++)
	{
		spllevel = intpri[irq_line];

		if (spllevel == 0)
			continue;

		ASSERT(spllevel <= PLMAX);

		cbus2_initialize_vector(irq_line, spllevel);
	}
}


void
cbus2_nenableint(irq_line, spl_level, engnum)
int irq_line;
pl_t spl_level;
int engnum;
{
	int 	vector;

	/*
	 * An engnum of -1 indicates that this interrupt is to be fully
	 * distributed.  If the engnum is not -1 then we need to make
	 * sure that the interrupt is to be enabled on this processor.
	 * If the interrupt is not being bound to this processor, then
	 * return.
	 */
	if (engnum != -1)
        {
                if (engnum != myengnum)
                        return;
        }
	vector = cbus2_initialize_vector(irq_line, spl_level);

	cbus2_enable_interrupt(vector, engnum);
}


/*
 *
 * Routine Description:
 *
 *    Disable the specified interrupt so it can not occur on the calling
 *    processor upon return from this routine.
 *
 *    This operation is called only by the boot processor.
 *
 * Arguments:
 *
 *    vector - Supplies a vector number to disable
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus2_ndisableint(irq_line)
int irq_line;
{
	PHWINTRMAP	hwentry;	/* CBC entry generating the intr */
	PCSR		csr;
	unsigned long	processor = myengnum;
	unsigned long	vector;

	/*
         * We will let all of the processors run this code
         * because even if the interrupt was never enabled,
         * it can't hurt to disable it again.
         */
	vector = cbus2_irqline_to_vector[irq_line];

	ASSERT (irq_line < EISA_IRQLINES);

	/*
	 * If the vector has already been cleared by another processor
	 * then there is nothing for us to do.  So just bail, we are
	 * done.
	 */
	if (!vector)
		return;

	/*
	 * Again, check if some other processor has invalidated this
	 * entry.  If so, then there is no work to perform, so just
	 * bail out of here.
	 */
	if (cbus2_vector_data[vector].InUse == 0)
		return;

	/*
	 * point at the hardware interrupt map entry address on
	 * the CBC of the bridge whose vector is specified
	 */
	hwentry = cbus2_vector_data[vector].hardware_intr_map;

	/*
	 * If the hwentry is not filled in, then this processor never
	 * had this interrupt enabled, so we should just leave now.
	 */
	if (!hwentry)
		return;

	/*
	 * Reaching out to the specific I/O CBC will disable the
	 * interrupt at the source, and now NO processors will see it.
	 */
	hwentry->csr_register = HW_MODE_DISABLED;

	/*
	 * tell the world that _this processor_ is no longer
	 * participating in receipt of this interrupt.  this code
	 * really is optional since we have already killed the
	 * interrrupt at the source.  but it's a useful template
	 * if we only wish for this particular processor to no
	 * longer participate in the interrupt arbitration.
	 */
	csr = crllry_cbus2base[processor];
	csr->intrconfig[vector].csr_register = HW_IMODE_DISABLED;

	ASSERT(irq_line < EISA_IRQLINES);

	cbus2_irqline_to_vector[irq_line] = 0;

	cbus2_vector_data[vector].InUse = 0;
}


/*
 *
 * Routine Description:
 *
 *    Determine if the supplied vector belongs to an EISA device - if so,
 *    then the corresponding bridge's CBC hardware interrupt map entry
 *    will need to be modified to enable the line, so return FALSE here.
 *
 *    Otherwise, just enable this processor's interrupt configuration register
 *    for the supplied vector and return TRUE immediately.
 *
 *    Note: this routine must be executed by every processor wishing to
 *    participate in the interrupt receipt.  all the EISA interrupts
 *    configured into the system are enabled by all the processors - it's
 *    only the dynamically loaded drivers (which ES/MP doesn't tell us
 *    whether or not they are multithreaded) that will have their interrupt
 *    routines run only on the boot cpu (because he's the only one who
 *    currently runs this routine).
 *
 * Arguments:
 *
 *    vector - Supplies a vector number to enable
 *
 * Return Value:
 *
 *    TRUE if the vector was enabled, FALSE if not.
 *
 */

void
cbus2_enable_interrupt(vector, engnum)
unsigned long vector;
int engnum;
{
	PHWINTRMAP	hwentry;	/* CBC entry generating the intr */
	PCSR		csr;
	unsigned long	csrval;
	unsigned long	processor = myengnum;
	unsigned long	irq_line;

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT,"myengnum 0x%x, vector 0x%x, engnum 0x%x\n", 
		myengnum, vector, engnum);
#endif

	csr = crllry_cbus2base[processor];

	/*
	 * if just IPI or spurious, setting the calling processor's
	 * CBC entry to accept the interrupt is sufficient.
	 */

	if (vector == CBUS2_SPURIOUS_TASKPRI || vector == CBUS2_IPI_TASKPRI)
	{
		csr->intrconfig[vector].csr_register =
			 HW_IMODE_ALLINGROUP;
		return;
	}

	irq_line = cbus2_vector_data[vector].irq_line;
	cbus2_irq_polarity = corollary_query_interrupt_polarity();

	hwentry = cbus2_vector_data[vector].hardware_intr_map;

	if (irq_line < EISA_IRQLINES)
	{
		if (myengnum == 0)
		{
			/*
			 * all interrupts that occur on the base
			 * will require an EOI.
			 */
			if (((cbus2_irq_polarity >> irq_line)) & 0x1)
			    cbus2_eoi_needed[myengnum][irq_line] = EOI_LEVEL;
			else
			    cbus2_eoi_needed[myengnum][irq_line] = EOI_EDGE;
		}
		else {
			/*
			 * for additional CPUs, only interrupts marked LIG
			 * will require an EOI.  this is needed since
			 * C-bus II only requires EOI's for bridge interrupts
			 * (ie. not IPI or spurious).  it only requires
			 * 1 EOI in the case where HW_IMODE_ALLINGROUP
			 * is set.  The base is selected to perform the EOI
			 * in this case.
			 */
			if ((corollary_global.cbus2features &
			    CBUS_DISTRIBUTE_INTERRUPTS) && intcpu[irq_line]==-1)
				if (((cbus2_irq_polarity >> irq_line)) & 0x1)
				    cbus2_eoi_needed[myengnum][irq_line] = EOI_LEVEL;
				else
				    cbus2_eoi_needed[myengnum][irq_line] = EOI_EDGE;
		}
	}

	/*
	 * since this is the _first_ processor to actually enable this
	 * interrupt, notify the generating CBC in a single dword access...
	 */

	/*
	 * if we need to fix level triggered interrupts,
	 * determine the difference from the hwintrmapeoi
	 * CSR entries to the hwintrmap entries.  this will
	 * make the assembly code fix both quicker and easier.
	 */
	if (cbus2_fix_level_interrupts)
	{
	    cbus2_eoitohwmap = (int)cbus2_bridge_csr[0]->hwintrmapeoi -
			(int)cbus2_bridge_csr[0]->hwintrmap;
	}

	/*
	 * Mark the caller's interrupt as level or edge triggered,
	 * based on the ELCR register we read earlier.
	 */
	if (((cbus2_irq_polarity >> irq_line)) & 0x1)
		csrval = HW_LEVEL_LOW;
	else
		csrval = HW_EDGE_RISING;

	hwentry->csr_register = (csrval | vector);
	cbus2_irqtohwmap[irq_line] = (csrval | vector);

	/*
	 * Now that the I/O side of the interrupt initialization is
	 * finished, set up the processor side as well.
	 * this needs to be done for ALL interrupts (ie: software,
	 * IPI, etc, as well as for real hardware devices).
	 */

	/*
	 * set intcpu[] for the clock vector so that the interrupt will
	 * go to all requesting processors, not just the lowest-in-group.
	 *
	 * hardcode the ivect[0] clock entry so that all the processors
	 * run the same clock interrupt routine.
	 */
	if (vector == cbus2_clock_vector)
	{
		intcpu[irq_line] = 0;
		ivect[0] = ci_clock;
		/*
                 * make sure the clock vector goes to all processors,
                 * and is not fought over as lowest-in-group.
                 */
                csr->intrconfig[vector].csr_register =
                        HW_IMODE_ALLINGROUP;
                return;
	}

	switch (engnum)
	{
	case -1:	/* indicates fully distributed */

		/* utilize CBC "Lowest-In-Group" mode */

		/*
		 * RRD will pass a flag as to whether to distribute
		 * interrupts or not.  If interrupts are to be fully
		 * distributed, then use Lowest In Group distribution,
		 * otherwisze, use all in group and set the group to
		 * be the first processor.  This feature is for CBC-REV1.
		 */
		if (corollary_global.cbus2features & CBUS_DISTRIBUTE_INTERRUPTS)
		{
			/* 
			 * utilize CBC "Lowest-In-Group" mode 
			 */
			csr->intrconfig[vector].csr_register = HW_IMODE_LIG;
		}
		else
		{
			/*
			 * This checks to see if we are on the boot
			 * processor.  If we are not on the boot processor,
			 * then do not set the All In Group interrupt
			 * distribution mode.  This means that only the
			 * boot processor will get these interrupts.
			 */
			if (myengnum)
				return;

			csr->intrconfig[vector].csr_register =
				HW_IMODE_ALLINGROUP;
		}

		break;

	default:

		/* Keep this in CBC "FIXED" mode */

		csr->intrconfig[vector].csr_register =
				HW_IMODE_ALLINGROUP;

		break;
	}
}


cbus2_dump_interrupts()
{
	PCSR		csr;
	PCSR bridgecsr = (PCSR)cbus2_bridge_csr[0];
	int		irq_line;
	PHWINTRMAP	intr_map;
	unsigned	vector;
	unsigned	mode;
	unsigned	i;

	for (irq_line = 0; irq_line < EISA_IRQLINES; irq_line++)
	{
		intr_map = (&bridgecsr->hwintrmap[irq_line]);

		vector = intr_map->ra.vector;

		printf("IRQ 0x%x - vector 0x%x - ", irq_line, vector);

		for (i = 0 ; i < corollary_num_cpus ; i++) 
		{
			csr = crllry_cbus2base[i];

			mode = csr->intrconfig[vector].ra.imode;

			switch(mode)
			{
			case HW_IMODE_DISABLED:
				printf("%d[D]", i);
				break;
			case HW_IMODE_ALLINGROUP:
				printf("%d[A]", i);
				break;
			case HW_IMODE_LIG:
				printf("%d[L]", i);
				break;
			default:
				break;
			}
		}
		printf("\n");
	}
}


/*
 *
 * Routine Description:
 *
 *    Overlay the spl-to-vector mappings with the CBUS-2 vector maps.
 *
 * Arguments:
 *
 *    None.
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus2_initialize_platform(void)
{
	unsigned long i;

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "cbus2_initialize_platform\n");
#endif

	/*
	 * Initialize the vectors each spl level represents in hardware
	 * Add the 0xF here so that we mask out all interrupts at this
	 * level.  Unlike the APIC, the CBC doesn't group all interrupts
	 * into 16 entries per groups.  We are mimicing the way the APIC
	 * handles interrupt "levels" by adding the 0xF to the "picmask".
	 */
	for (i = 0; i < PLMAX; i++)
	{
		corollary_spl_to_vector[i] = 
			0x10 * i + CBUS2_FIRST_DEVICE_TASKPRI + 0xF;
	}

	/*
         * The last priority level (PLMAX) can not be masked because
         * it is used for IPI's.  Set the value of corollary_spl_to_vector
         * such that it is less than the TASKPRI of IPI.
         */
	corollary_spl_to_vector[PLMAX] =
                        0x10 * (PLMAX - 1) + CBUS2_FIRST_DEVICE_TASKPRI + 0xF;

	cbus2_irq_polarity = corollary_query_interrupt_polarity();

	/*
	 * Assign a vector to each irq line (based on the spl level
	 * it's configured at.  This is the vector we will see when
	 * the line interrupts us.
	 */
	cbus2_initialize_vectors();
}

/*
 *
 * Routine Description:
 *
 *    Initialize this processor's CSR, interrupts, spurious interrupts & IPI
 *    vector.
 *
 * Arguments:
 *
 *    processor - Supplies a logical processor number
 *
 * Return Value:
 *
 *    None.
 *
 */

int
cbus2_initialize_cpu(processor)
unsigned long processor;
{
	PCSR			csr;
	ulong			cbc_config;

	csr = crllry_cbus2base[processor];

	/*
	 * save the interrupt address for this processor to
	 * make the interrupt code simple
	 */
	cbus2_send_ipi[processor] =
		(unsigned long *)&(csr->intrreq[CBUS2_IPI_TASKPRI]);

#ifdef CBUS2_OEM
	/*
	 * generate NMIs (trap 2) when we get error interrupts.
	 */
	csr->errorvector.low_dword = 2;
	csr->interruptcontrol.low_dword = corollary_global.intrcontrolmask;
	csr->faultcontrol.low_dword = corollary_global.faultcontrolmask;
#endif

	/*
	 * initialize the spurious vector for the CBC
	 * to generate when it detects inconsistencies.
	 */
	csr->spurious_vector.csr_register = CBUS2_SPURIOUS_TASKPRI;

	cbus2_tpr[processor] = (unsigned long *)&csr->task_priority;

	if (processor == 0)
	{
#ifdef CBUS2_OEM
		if (corollary_global.cbus2features & CBUS_ENABLED_PW)
		{
			/*
			 * setida is a misleading name - if the
			 * posted-writes bit is enabled, then allow EISA
			 * I/O cycles to use posted writes.
			 *
			 * call a function here so the compiler won't use byte
			 * enables here - we must force a dword access.
			 */
			cbc_config = cbusreadcsr(
				&csr->CbcConfiguration.low_dword);

			cbuswritecsr(&csr->CbcConfiguration.low_dword,
				cbc_config & ~CBC_DISABLE_PW);
		}
#endif
		if (corollary_global.cbus2features &
		    CBUS2_DISABLE_LEVEL_TRIGGERED_INT_FIX)
			cbus2_fix_level_interrupts = 0;
	}

	/*
	 * Disable all of this processor's incoming interrupts _AND_
	 * any generated by his local CBC (otherwise they could go to
	 * any processor).
	 */
	cbus2_disable_my_interrupts(processor);

	ci_intsw[processor] = ci_cbus2_intr;
	ci_setpicmasks[processor] = ci_setpicmasks_cbus2;
	ci_softint[processor] = softint_cbus2;
	ci_deferred_int[processor] = deferred_int_cbus2;
	ci_cpu_intr[processor] = cbus2_set_intr;
	ci_nenableint[processor] = cbus2_nenableint;
	ci_ndisableint[processor] = cbus2_ndisableint;

	/*
	 * The CBUS-2 additional processors can play the
	 * same deferred interrupt games as the base.
	 */
        plsti = PLMAX + 1;

	cbus_booted_processors++;
	cbus_booted_processors_mask |= (1 << processor);
}


/*
 *
 *		internal CBUS-2 support routines begin here
 *
 *
 */

/*
 *
 * Routine Description:
 *
 *    by default, disable all of the calling processor's
 *    interrupt configuration registers(ICR) so he will take no interrupts.
 *    also disable all interrupts originating from his CBC, so
 *    no other processor will get interrupts from any devices
 *    attached to this CBC.
 *
 *    as each interrupt is enabled, it will need to be enabled
 *    at this CBC, and also in each receiving processors' ICR.
 *
 *    all EISA bridges have had their interrupts disabled already.
 *    as each interrupt is enabled, it will need to be enabled
 *    at the bridge, and also on each processor participating
 *    in the reception.
 *
 * Arguments:
 *
 *    processor - Supplies the caller's logical processor number whose
 *		interrupts will be disabled
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus2_disable_my_interrupts(processor)
unsigned long processor;
{
	unsigned long 		vector;
        PCSR			csr;

	csr = crllry_cbus2base[processor];

	for (vector = 0; vector < INTR_CONFIG_ENTRIES; vector++)
	{
		csr->intrconfig[vector].csr_register =
			HW_IMODE_DISABLED;
	}

	/*
	 * In the midst of setting up the EISA element CBCs or
	 * processor CBCs (for those with half-card devices),
	 * a device interrupt that was pending in a bridge's
	 * 8259 ISRs may be lost.  None should be fatal, even an
	 * 8042 keystroke, since the keyboard driver should do a flush
	 * on open, and thus recover in the same way the standard
	 * uniprocessor ES/MP does when it initializes 8259s.
	 */
	
#if 0
	for (vector = 0; vector < HWINTR_MAP_ENTRIES; vector++)
	{
		csr->hardware_interrupt_map[vector].csr_register =
			HW_MODE_DISABLED;
	}
#endif
}

/*
 *
 * Routine Description:
 *
 *    Check for CBUS-2 I/O bridges and disable their incoming interrupts
 *    here.  This cannot be done in InitializeCPU() because it
 *    is permissible for the I/O bridge to be a halfcard without a CPU
 *    attached.
 *
 * Arguments:
 *
 *    table - Supplies a pointer to the RRD extended ID information table
 *
 *    count - Supplies a pointer to the number of valid entries in the
 *	    RRD extended ID information table
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus2_parse_rrd(table, count)
struct ext_id_info *table;
unsigned long count;
{
	unsigned long		index;
	struct ext_id_info	*idp;
	unsigned long 		vector, IntrControl;
        PCSR			csr;

	idp = table;
#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "cbus2_parse_rrd - count = 0x%x\n", count);
#endif

	for (index = 0; index < count; index++, idp++)
	{

		if ((idp->pel_features & ELEMENT_BRIDGE) == 0)
		{
			continue;
		}

		/* 
		 * fix: we can't call physmap yet because kvminit() hasn't
		 * run.  but we call cbus2_initialize_platform() at the end of
		 * this routine, who assumes that all the element CSR spaces
		 * have been mapped because cbus2_initialize_vectors() uses
		 * them to associate interrupt lines with the correct EISA CBC
		 * bridge hardware interrupt map entries.
		 */
		csr = (PCSR)physmap(idp->pel_start, idp->pel_size, KM_NOSLEEP);

		/*
		 * to go from 8259 to CBC mode for interrupt handling,
		 *
		 *	a) disable PC compatible interrupts, ie: stop each
		 *	   bridge CBC from asking its 8259 to satisfy INTA
		 *	   pulses to the CPU.
		 *	b) mask off ALL 8259 interrupt input lines EXCEPT
		 *	   for irq0.  since clock interrupts are not external
		 *	   in the EISA chipset, the bridge 8259 must enable
		 *	   them even when the CBC is enabled.  putting the
		 *	   8259 in passthrough mode (ie: the 8259 irq0 input
		 *	   will just be wired straight through) WILL NOT
		 *	   allow the 8259 to actually talk to the CPU; it
		 *	   just allows the interrupt to be seen by the CBC.
		 *	   the CBC is responsible for all the CPU interrupt
		 *	   handshaking.
		 *	c) initialize the hardware interrupt map for the irq0
		 *	   entry.
		 *	d) enable each participating element's (ie: CPUs only)
		 *	   interrupt configuration register for the vector
		 *	   the HAL has programmed irq0 to actually generate.
		 *
		 *	IT IS CRITICAL THAT THE ABOVE STEPS HAPPEN IN THE
		 *	ORDER OUTLINED, OTHERWISE YOU MAY SEE SPURIOUS
		 *	INTERRUPTS.
		 *

		 *
		 * now process this I/O bridge:
		 *
		 * currently assumes that all bridges will be of the same
		 * flavor. if this element is a bridge, map it systemwide
		 * and disable all incoming interrupts on this bridge.
		 * any extra bridges beyond our configuration maximum
		 * are just disabled, and not used by ES/MP.
		 */

		if (cbus2_bridges_found < CBUS_MAX_BRIDGES)
		{

			cbus2_bridge_csr[cbus2_bridges_found] = csr;
			cbus2_bridge_eoi[cbus2_bridges_found++] =
				csr->hwintrmapeoi;
		}
	
		if (idp->pel_features & ELEMENT_HAS_8259)
		{
#ifdef CBUS2_OEM
			IntrControl = inb(corollary_global.control8259mode);
			outb(corollary_global.control8259mode,
				(unsigned char)(IntrControl | corollary_global.control8259mode_val));
#else
			IntrControl = inb(EISA_CONTROL_PORT);
			outb(EISA_CONTROL_PORT,
				(unsigned char)(IntrControl | DISABLE_8259));
#endif

			/*
			 * disable all inputs in the 8259 IMRs except for the
			 * irq0. and explicitly force these masks onto the
			 * 8259s.
			 *
			 * if profiling is disabled, we will disable it in
			 * the interrupt configuration registers, but still
			 * we must leave the 8259 irq0 enabled.  not to worry,
			 * the processor will not see irq0 interrupts.
			 * this way, if profiling is re-enabled later, we
			 * only need to change the interrupt configuration
			 * registers, and bingo, we provide the desired effect.
			 */

			if (corollary_hw_info.oem_rom_info.oem_number ==
			    CBUS2_OEM_IBM_MCA)
				corollary_disable_8259s(0xFFFF);
			else
				corollary_disable_8259s(0xFFFE);
		}
	
		/*
		 * In the midst of setting up the EISA element CBCs or
		 * processor CBCs (for those with half-card devices), a
		 * device interrupt that was pending in a bridge's 8259 ISRs
		 * may be lost.  None should be fatal, even an
		 * 8042 keystroke, since the keyboard driver does a flush
		 * on open, and will, thus recover in the same way the standard
		 * uniprocessor ES/MP does when it initializes 8259s.
		 */
		
#if 0
		for (vector = 0; vector < HWINTR_MAP_ENTRIES; vector++)
		{
			csr->hardware_interrupt_map[vector].csr_register =
				HW_MODE_DISABLED;
		}
#endif
	}

	cbus2_initialize_platform();

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "cbus2_parse_rrd - done\n");
#endif
}

/*
 *
 * Routine Description:
 *
 *    Called to put all the other processors in reset prior to reboot.
 *    Only the CBUS-2 boot processor is reset by the 8042 reset.  So if
 *    the calling processor is not the boot processor, he sends an
 *    interrupt to the boot processor to have him take care of it all.
 *
 *    The boot processor will return (either on his own behalf or on
 *    another processor's behalf) to poke the 8042 into oblivion.
 *
 * Arguments:
 *
 *    None.
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus2_reset_all_other_processors()
{
	unsigned long		index;
#ifdef CBUS2_OEM
	unsigned long 		IntrControl;
#endif

	for (index = 1; index < corollary_num_cpus; index++)
	{
		*(unsigned long *)(void *)
		  ((unsigned char *)crllry_cbus2base[index] +
			corollary_global.ci_sreset) =
			corollary_global.ci_sreset_val;
	}

#ifdef CBUS2_OEM
	/*
	 * since there will be no more OS interrupts after this point, we need
	 * to switch back from CBC to 8259 mode so the BIOS can get interrupts
	 * for the soft reboot to work.
	 */
	IntrControl = inb(corollary_global.control8259mode);
	outb(corollary_global.control8259mode,
		(unsigned char)(IntrControl & ~corollary_global.control8259mode_val));
#endif
}

void
cbus2_enable_all_interrupts()
{
	unsigned irq_line, vector;	

	/*
	 * for APIC-based systems, we need to go pound the
	 * redirection table entries to enable all the interrupts.
	 */
	for (irq_line = 0; irq_line < EISA_IRQLINES; irq_line++)
	{
		cbus2_eoi_needed[myengnum] = cbus2_eoi_array[myengnum];

		/*
                 * If the intcpu for this irq_line is not -1, then
                 * the irq_line is bound to a processor.
                 * The clock goes to all processors, whether it
                 * is marked to go to the base or not.
                 */
		if ((intcpu[irq_line] != -1) && (irq_line != CLOCK_IRQ))
                {
                        if (intcpu[irq_line] != myengnum)
                                continue;
			/*
			 * only 1 EOI is needed for the clock,
			 * so mark the base as the EOIer.
			 */
/*DJO1?*/
			cbus2_irq_polarity = corollary_query_interrupt_polarity();
			if (myengnum == 0)
				if (((cbus2_irq_polarity >> irq_line)) & 0x1)
				    cbus2_eoi_needed[myengnum][irq_line] = EOI_LEVEL;
				else
				    cbus2_eoi_needed[myengnum][irq_line] = EOI_EDGE;
                }

		vector = cbus2_irqline_to_vector[irq_line];
		if (vector)
			cbus2_enable_interrupt(vector, intcpu[irq_line]);
	}

	/*
	 * since each processor must enable his own IPI vector and
	 * spurious vector, do it now.
	 */
	cbus2_enable_interrupt(CBUS2_IPI_TASKPRI, 0);
	cbus2_enable_interrupt(CBUS2_SPURIOUS_TASKPRI, 0);
	*cbus2_tpr[myengnum] = (long)CBUS2_LOWEST_TASKPRI;
}

void
cbus2_timer_init()
{
	if (IS_BOOT_ENG(myengnum))
	{
		/*
		 * Initialize the i8254 programmable interrupt timer.
		 */
		clkstart();
	}
}

struct corollarysw cbus2_sw = {
	cbus2_pres,
	cbus2_setup,
	cbus2_findcpus,
	cbus2_startcpu,
	cbus2_intr_init,
	cbus2_clr_intr,
	cbus2_set_nmi,
	cbus2_clr_nmi,
	cbus2_intr,
	cbus2_ledon,
	cbus2_ledoff,
	cbus2_parse_rrd,
	cbus2_initialize_cpu,
	cbus2_reset_all_other_processors,
	cbus2_enable_all_interrupts,
	cbus2_timer_init,
};
