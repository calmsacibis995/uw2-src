/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/cbus/cbusapic.c	1.8"
#ident  "$Header: $"

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
#include <util/mod/mod_hier.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <cbus.h>
#include <cbusapic.h>

/*
 * The priority scheme employed by the Intel APIC is as follows:
 *
 * there are 256 vectors, but since the APIC ignores the least
 * significant 4 bits, we really only have 16 distinct priority levels
 * to work with.
 *
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
#define CBUS1_FIRST_DEVICE_TASKPRI	0x60

/*
 * The next two vectors are private to the cbusapic PSM.
 */
#define CBUS1_SPURIOUS_TASKPRI		0xFF
#define CBUS1_IPI_TASKPRI		0xEF

#define NONAPIC_IPI_VECTOR		0x7f

#define EISA_IRQLINES			16

#define NO_PROCESSOR_BITS		0

/*
 * the list of CBUS APIC vector data:
 *
 * in_use is 0 if the entry is unused.
 *
 * Irqline is the irqline this vector maps to.  this is used on interrupt
 * receipt to translate the vector into an irqline the driver can understand.
 *
 * redirection_address is the redirection table entry address to poke to
 * enable/disable this interrupt.
 *
 * the pad field exists so assembly code can traverse each entry with a 4 bit
 * shift instead of the expensive imul.
 */
typedef struct _cbus1_vectors_t {
	ulong		irq_line;	/* irqline this vector is assigned to */
	ulong		redirection_address; /* redirection table entry addr */
	ulong		in_use;		/* set if this entry is being used */
	ulong		pad;
} CBUS1_VECTORS_T, *PCBUS1_VECTORS;

PAPIC_IOREGISTERS		cbus_io_apic;
PAPIC_REGISTERS			cbus_local_apic;
unsigned long			cbus_apics_everywhere;
unsigned long			cbus_apic_eoi_reg;
unsigned long			cbus_apic_clock_vector;
unsigned long *			cbus_apic_tpr[MAXACPUS];
CBUS1_VECTORS_T			cbus_apic_vector_data[256];
int				cbus_apic_irqline_to_vector[EISA_IRQLINES];
unsigned long			cbus_apic_irq_polarity;

extern int			cbus_bridge;
extern char			intpri[];
extern unsigned long		svcpri[];
extern void			(*ivect[])();
extern int			cbus_booted_processors;
extern int			cbus_booted_processors_mask;
extern unsigned long		corollary_spl_to_vector[];
extern int			intcpu[];
extern int			broadcast_csr;
extern int			plsti;
extern int			default_bindcpu;


extern void			ci_setpicmasks_base();
extern void			ci_setpicmasks_acpu();
extern void			ci_setpicmasks_apic();
extern void                     cbus_enable_int();
extern void                     cbus_disable_int();
extern void			ci_base_intr();
extern void			ci_acpu_intr();
extern void			ci_apic_intr();
extern void			softint_base();
extern void			softint_acpu();
extern void			softint_apic();
extern void			deferred_int_base();
extern void			deferred_int_acpu();
extern void			deferred_int_apic();
extern void			cbus_initialize_local_apic();
extern void			cbus_initialize_io_apic();
extern void			corollary_disable_8259s();
extern void			psm_intr();
extern void			ci_clock();
extern void			cbus_apic_cpuintr();
extern void			cbus_set_intr();
extern void                     cbus_apic_nenableint();
extern void                     cbus_apic_ndisableint();
extern void                     cbus_nenableint();
extern void                     cbus_ndisableint();

/*
 *
 * Routine Description:
 *
 *    Initialize this processor's local and I/O APIC units,
 *    and set up the spl-related function pointer entry for
 *    this processor.
 *
 *    Also bump the number of processors booted and the global mask.
 *
 * Arguments:
 *
 *    processor - Supplies a logical processor number, 0 based.
 *
 * Return Value:
 *
 *    None.
 *
 */
int
cbus_initialize_cpu(processor)
unsigned long processor;
{
	/*
	 * only flip this processor into APIC mode if all the processors
	 * are able to talk on the APIC bus.
	 */
	if (cbus_apics_everywhere)
	{
		cbus_initialize_local_apic(processor);

		cbus_initialize_io_apic(processor);

		/*
		 * XM APIC C-bus
		 */
		ci_intsw[processor] = ci_apic_intr;
		ci_setpicmasks[processor] = ci_setpicmasks_apic;
		ci_softint[processor] = softint_apic;
		ci_deferred_int[processor] = deferred_int_apic;
		ci_cpu_intr[processor] = cbus_apic_cpuintr;
		ci_nenableint[processor] = cbus_apic_nenableint;
		ci_ndisableint[processor] = cbus_apic_ndisableint;

		/*
		 * The apic additional processors can play the
		 * same deferred interrupt games as the base.
		 */
		plsti = PLMAX + 1;
	}
	else 
	{
		/*
		 * original C-bus, where only the base processor has
		 * 8259s, and the additional processors have no PIC (they
		 * rely on cli/sti only).
		 */
		if (processor)
		{
			ci_intsw[processor] = ci_acpu_intr;
			ci_setpicmasks[processor] = ci_setpicmasks_acpu;
			ci_softint[processor] = softint_acpu;
			/* 
			 * The CBUS-I additional processor does not defer
			 * interrupts, so there is no need for this
			 * entry.
			 */
			ci_deferred_int[processor] = NULL;
		}
		else
		{
			ci_intsw[processor] = ci_base_intr;
			ci_setpicmasks[processor] = ci_setpicmasks_base;
			ci_softint[processor] = softint_base;
			ci_deferred_int[processor] = deferred_int_base;
		}
		ci_cpu_intr[processor] = cbus_set_intr;
		ci_nenableint[processor] = cbus_nenableint;
		ci_ndisableint[processor] = cbus_ndisableint;
	
		plsti = PLBASE;
	}

	cbus_booted_processors++;
	cbus_booted_processors_mask |= (1 << processor);

	return 0;
}

/* 
 *
 * void
 * apic_arb_sync ( void )
 *
 * Routine Description:
 *
 *    Broadcast an ALL-INCLUDING-SELF interrupt with deassert, reset &
 *    physical mode set.  This routine is called after each APIC assigns
 *    itself a unique ID that can be used in APIC bus arbitration and
 *    priority arbitration.  This syncs up the picture that each APIC
 *    has with the new ID that has just been added.
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
apic_arb_sync()
{
	/*
	 * disable interrupts so that polling the register and
	 * poking it becomes an atomic operation (for this processor),
	 * as specified in the Intel 82489DX specification.
	 * this is needed since interrupt service routines must
	 * be allowed to send IPIs (for example, DPCs, etc).
	 */

	asm("pushfl");
	asm("cli");

	/*
	 * wait for the delivery status register to become idle, ie: APIC_WAIT
	 */
	while (cbus_local_apic->apic_icr.rb.dword1 & APIC_ICR_BUSY)
		;

	/*
	 * it is ILLEGAL to use the "destination shorthand" mode of the APIC
	 * for this command - we must set up the whole 64 bit register).
	 * both destination and vector are DONT_CARE for this request.
	 *
	 * no recipients (probably a don't care), but must be written
	 * _before_ the command is sent...
	 */

	cbus_local_apic->apic_icr.ra.destination = 0;

	/*
	 * now we can send the full deassert-reset command
	 */
	cbus_local_apic->apic_icr.rb.dword1 = APIC_FULL_DRESET;

	asm("popfl");
}

/*
 *
 *   unsigned long
 *   READ_IOAPIC_ULONG(Port)
 *
 *   Routine Description:
 *
 *       Read the specified offset of the calling processor's I/O APIC.
 *
 *
 *   Arguments:
 *       (esp+4) = Port
 *
 *   Returns:
 *       Value in Port.
 *
 */
unsigned long
READ_IOAPIC_ULONG(port)
long port;
{
	cbus_io_apic->register_select = port;
	return cbus_io_apic->window_register;
}

/*
 *
 *   void
 *   WRITE_IOAPIC_ULONG(port, value)
 *
 *   Routine Description:
 *
 *       Write the specified offset with the specified value into
 *       the calling processor's I/O APIC.
 *
*/
void
WRITE_IOAPIC_ULONG(port, value)
unsigned long port;
unsigned long value;
{
	cbus_io_apic->register_select = port;
	cbus_io_apic->window_register = value;
}

/*
 * Routine Description:
 *
 *    Each processor assigns an APIC ID to his I/O APIC so
 *    it can arbitrate for the APIC bus, etc.  Intel documentation
 *    says that every local and I/O APIC must have a unique id.
 *
 * Arguments:
 *
 *    processor - Supplies a logical processor number
 *
 * Return Value:
 *
 *    None.
 *
 **/
void
cbus_apic_brand_io_unit_id(processor)
unsigned long processor;
{
	WRITE_IOAPIC_ULONG(IO_APIC_ID_OFFSET, (2 * processor) <<APIC_BIT_TO_ID);

	apic_arb_sync();
}

/*
 *
 * Routine Description:
 *
 *    Note that all interrupts are assumed to be blocked on entry.
 *    Initialize this processor's local and I/O APIC units.
 *
 * Arguments:
 *
 *    processor - Supplies a logical processor number, 0 based.
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus_initialize_io_apic(processor)
unsigned long processor;
{
	/*
	 * since the boot processor has already taken care of
	 * all the global responsibilities, each additional
	 * processor only needs to give his I/O APIC an arbitration ID.
	 */
	if (processor)
	{
		cbus_apic_brand_io_unit_id(processor);
		return;
	}

	/*
	 * Provide portability in the event that the APIC might move.
	 */

#if (LOCAL_APIC_LOCATION == IO_APIC_LOCATION)
	cbus_io_apic = (PAPIC_IOREGISTERS)cbus_local_apic;
#else
	cbus_io_apic = (PAPIC_IOREGISTERS)physmap(IO_APIC_LOCATION, ptob(1), KM_NOSLEEP);
#endif

	cbus_apic_brand_io_unit_id(processor);

	/*
	 * Disable all 8259 inputs except the irq0 clock.
	 * remember the irq0 clock and the irq13 DMA
	 * chaining interrupts are internal to the Intel EISA
	 * chipset (specifically, the ISP chip), and if the PSM
	 * wants to enable them, it must be done here.
	 * This is done by enabling the 8259 ISP to send them
	 * to the processor(s) via the APIC.
	 *
	 * Note that all other EISA bus device interrupts only need to
	 * be enabled in the APIC for processors to see them.
	 */
	corollary_disable_8259s(0xFFFF);

	/*
	 * All redirection table entries are disabled by default when the
	 * processor emerges from reset.  Later, entries are enabled as
	 * part of picinit(), and later individual entries are
	 * enabled from their respective drivers via nenableint().
	 *
	 * Indicate the APIC (not the 8259s) will now handle provide
	 * the interrupt vectors to the processor during an INTA cycle.
	 * This is done by writing to the ap_mode port.  Note that at this
	 * time we will also sync the APIC polarity control registers with
	 * the ELCR.  Since irq0 has no polarity control, the hardware
	 * uses bit0 for the ap_mode enable, so make sure this bit is on too.
	 */

	cbus_apic_irq_polarity = corollary_query_interrupt_polarity();
	cbus_local_apic->ap_mode = (uchar_t)((cbus_apic_irq_polarity & 0xFF) | 0x1);
	cbus_local_apic->polarity_port_high = 
		(uchar_t)((cbus_apic_irq_polarity >> 8) & 0xFF);
}

/*
 *
 * Routine Description:
 *
 *    Called by each processor to initialize his local APIC.
 *    The first processor to run this routine will map the
 *    local APICs for all processors.
 *
 *    It is assumed that all interrupts are blocked on entry.
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
void
cbus_initialize_local_apic(processor)
unsigned long processor;
{
	unsigned long		processor_bit;
	unsigned long		apic_id_bit;
	REDIRECTION_T		redirection_entry;

	/*
	 * If the APIC mapping has not been set up yet,
	 * do it now.  Given the ES/MP startup architecture,
	 * this will always be done by the boot processor.
	 *
	 * We can map in the APIC into global space
	 * because all processors see it at the
	 * same _physical_ address.  Note the page is mapped PWT.
	 */

	/*
	 * A single physmap() for the APIC is enough for all
	 * processors to be able to see their APICs.
	 */
	if (!cbus_local_apic)
	{
		cbus_local_apic = (PAPIC_REGISTERS)
			physmap(LOCAL_APIC_LOCATION, ptob(1), KM_NOSLEEP);

		/*
		 * Initialize our EOI address for interrupt dismissal.
		 */
		cbus_apic_eoi_reg = (unsigned long)&cbus_local_apic->apic_eoi;
	}
	
	cbus_apic_tpr[processor] = 
		(unsigned long *)&cbus_local_apic->apic_task_priority;

	/*
	 * Here we initialize our destination format and
	 * logical destination registers so that we can get IPIs
	 * from other processors.
	 *
	 * Specify full decode mode in the destination format register -
	 * ie: each processor sets only his own bit, and a "match" requires
	 * that at least one bit match.  The alternative is encoded mode,
	 * in which _ALL_ encoded bits must match the sender's target for
	 * this processor to see the sent IPI.
	 */
	cbus_local_apic->apic_destination_format = APIC_ALL_PROCESSORS;

	/*
	 * the logical destination register is what the redirection destination
	 * entry compares against.  only the high 8 bits will be supported
	 * in Intel's future APICs, although this isn't documented anywhere!
	 */
        processor_bit = (1 << processor);

	apic_id_bit = (processor_bit << APIC_BIT_TO_ID);

	cbus_local_apic->apic_logical_destination = apic_id_bit;

	/*
	 * designate the spurious interrupt vector we want to see,
	 * and inform this processor's APIC to enable interrupt
	 * acceptance.  note that the idt[] entry has already been
	 * set up to just iret (see intr_p.s).
	 */
	cbus_local_apic->apic_spurious_vector =
				CBUS1_SPURIOUS_TASKPRI|LOCAL_APIC_ENABLE;

	/*
	 * as each processor comes online here, we must have ALL
	 * processors resync their arbitration IDs to take into
	 * account the new processor.  note that we will set:
	 * arb id == APIC id == processor number.
	 *
	 * the strange ID setting is to satisfy Intel's need for
	 * uniqueness amongst I/O and local unit ID numbering.
	 */

	cbus_local_apic->local_unit_id = ((2 * processor + 1) << APIC_BIT_TO_ID);

	/*
	 * sync up our new ID with everyone else
	 */

	apic_arb_sync();

	/*
	 * Create the NMI routing linkage for this processor
	 * It is set as edge sensitive, enabled and generating NMI trap 2.
	 */

	bzero(&redirection_entry, sizeof(REDIRECTION_T));
	redirection_entry.ra.trigger = APIC_EDGE;
	redirection_entry.ra.mask = APIC_INTR_UNMASKED;
	redirection_entry.ra.delivery_mode = APIC_INTR_NMI;
	cbus_local_apic->apic_local_int1 = redirection_entry;
}

/*
 *
 * Routine Description:
 *
 *    Remember only the boot processor can add/remove processors from
 *    the I/O APIC's redirection entries.
 *
 *    This operation is run only by the boot processor.  
 *
 * Arguments:
 *
 *    vector - Supplies a vector number to enable
 *    engnum - The processor that this interrupt is to be delivered to.
 *             -1 means all processors.
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus_apic_enable_interrupt(vector, engnum)
unsigned long vector;
int engnum;
{
	unsigned long			processor_bit;
	unsigned long			irq_line;
	unsigned long			spllevel;
	unsigned long			participating_processors;
	REDIRECTION_T			redirection_entry;
	unsigned long			redirection_address;

	/* 
	 * Only the Base processor can run this code.  The irq lines
	 * are hooked up to the APIC on the APIC I/O board, so the
	 * base's APIC will do the distribution.
	 */
	if (myengnum != 0)
		return;

	/*
	 * No redirection table editing is needed for
	 * APIC timer interrupts or IPIs, so check for
	 * those interrupts here.  we only need to edit
	 * the redirection table for legitimate EISA interrupts.
	 * all of these interrupts are automatically accepted
	 * as long as the task priority register is low enough.
	 *
	 * note that their idt entries are set up as follows:
	 *
	 * - intr_p.s points the spurious interrupt at an iret.
	 * - cbus_apic_clock_vector has been assigned the ivect[0] entry
	 *   by the kernel build, and its vector has been assigned
	 *   by us.  we map the vector back down to 0 for the common
	 *   interrupt dispatcher.  the idt entry is initialized in
	 *   intr_p.s.
	 * - IPI has an idt entry set up in intr_p.s.  however, the
	 *   corresponding ivect[] entry was carved out by us.
	 */

	if ((vector == cbus_apic_clock_vector) ||
	    (vector == CBUS1_SPURIOUS_TASKPRI) ||
	    (vector == CBUS1_IPI_TASKPRI))
			return;

	irq_line = cbus_apic_vector_data[vector].irq_line;

	if (irq_line >= EISA_IRQLINES)
	{
		cmn_err(CE_PANIC, 
			"cbus_apic_enable_interrupt: bogus interrupt line %d\n",
			irq_line);
	}

	spllevel = intpri[irq_line];

	redirection_address = cbus_apic_vector_data[vector].redirection_address;

	/*
	 * Let the I/O APIC know that the calling processor wishes to
	 * participate in receipt of the interrupt.  This must be done
	 * regardless of whether or not any other processors have already
	 * enabled the interrupt.
	 */

	bzero(&redirection_entry, sizeof(REDIRECTION_T));

	redirection_entry.ra.vector = vector;

	/*
	 * Note that non-EISA interrupts (ie: APIC clocks or IPI) are
	 * correctly handled in the EnableNonDeviceInterrupt case, and
	 * hence calls to enable those will never enter this routine.
	 * If this changes, the code below needs to be modified because
	 * these interrupts do NOT have an ELCR entry (there are only
	 * 16 of those).
	 */

	/*
	 * Mark the caller's interrupt as level or edge triggered,
	 * based on the ELCR register we read earlier.
	 */
	cbus_apic_irq_polarity = corollary_query_interrupt_polarity();
	if (((cbus_apic_irq_polarity >> irq_line)) & 0x1)
	{
		redirection_entry.ra.trigger = APIC_LEVEL;
	}
	else
	{
		redirection_entry.ra.trigger = APIC_EDGE;
	}

	redirection_entry.ra.mask = APIC_INTR_UNMASKED;
	redirection_entry.ra.dest_mode = APIC_LOGICAL_MODE;
	redirection_entry.ra.delivery_mode = APIC_INTR_FIXED;

	/*
	 * Add this processor's bit field number to the
	 * I/O APIC in order to be considered for receipt of
	 * the interrupt.
	 */
	processor_bit = (1 << myengnum);
	participating_processors = (processor_bit << APIC_BIT_TO_ID);

#ifdef NEVER
	/*
	 * Only enable APIC LIG arbitration delay (at least 4 cycles on
	 * our 10Mhz APIC bus, which is 0.4 microseconds) if there is
	 * more than one processor in the machine.
	 */
	if (corollary_num_cpus > 1)
	{
		/* 
		 * If the interrupt is not fully distributed then keep 
		 * this APIC in "FIXED" mode.
		 */
		if (engnum == -1)
		{
			/* 
			 * utilize APIC "Lowest-In-Group" mode 
			 */
			redirection_entry.ra.delivery_mode = APIC_INTR_LIG;
			participating_processors = 
				(cbus_booted_processors_mask << APIC_BIT_TO_ID);
		}
		else
		{
			/*
			 * Add processor's bit field number to the
			 * I/O APIC in order to be considered for receipt of
			 * the interrupt.  Remember, each processor can only
			 * access his own I/O APIC.
			 */
			processor_bit = (1 << engnum);

			participating_processors = 
				(processor_bit << APIC_BIT_TO_ID);
		}
	}
#endif

	redirection_entry.ra.destination = participating_processors;

	WRITE_IOAPIC_ULONG(redirection_address + 1,
				redirection_entry.ra.destination);
	WRITE_IOAPIC_ULONG(redirection_address,
				redirection_entry.rb.dword1);
}

/*
 *
 * Routine Description:
 *
 *    Disable the specified interrupt so it can not occur on the calling
 *    processor upon return from this routine.  Remember only the boot processor
 *    can add/remove processors from his I/O APIC's redirection entries.
 *
 *    This operation is called only by the boot processor.
 *
 * Arguments:
 *
 *    Vector - Supplies a vector number to disable
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus_apic_disable_interrupt(irq_line)
unsigned long irq_line;
{
	unsigned long		processor, processor_bit;
	unsigned long		vector;
	REDIRECTION_T		redirection_entry;
	unsigned long		redirection_address;

	/* 
	 * Only the Base processor can run this code.  The irq lines
	 * are hooked up to the APIC on the APIC I/O board, so the
	 * base's APIC will do the distribution.
	 */
	if (myengnum != 0)
		return;

	vector = cbus_apic_irqline_to_vector[irq_line];

	redirection_address = cbus_apic_vector_data[vector].redirection_address;

	/*
	 * We can not remove any of the following vectors.
	 */
	if ((vector == cbus_apic_clock_vector) ||
	    (vector == CBUS1_SPURIOUS_TASKPRI) ||
	    (vector == CBUS1_IPI_TASKPRI))
			return;

	ASSERT (irq_line < EISA_IRQLINES);

	/*
	 * Let the I/O APIC know that this CPU is no longer participating in
	 * receipt of the interrupt.
	 */
	redirection_entry.rb.dword1 = READ_IOAPIC_ULONG(redirection_address);
	redirection_entry.rb.dword2 = READ_IOAPIC_ULONG(redirection_address+1);

	/*
	 * Remove all processors from the interrupt bit field
	 * participation list.  Mask off the interrupt at the source
	 * as well, so no one will need to arbitrate for it should
	 * it get asserted later.
	 */
	redirection_entry.ra.destination = NO_PROCESSOR_BITS;

	redirection_entry.ra.mask |= APIC_INTR_MASKED;

	WRITE_IOAPIC_ULONG(redirection_address, redirection_entry.rb.dword1);
	WRITE_IOAPIC_ULONG(redirection_address + 1, 
		redirection_entry.rb.dword2);
}

int
cbus_apic_initialize_vector(irq_line, spllevel)
int irq_line;
int spllevel;
{
	int newvector;

	newvector = spllevel * 0x10 + CBUS1_FIRST_DEVICE_TASKPRI;

	while (cbus_apic_vector_data[newvector].in_use)
		newvector++;

	ASSERT(newvector < ((spllevel+1) * 0x10) + CBUS1_FIRST_DEVICE_TASKPRI);

	cbus_apic_irqline_to_vector[irq_line] = newvector;

	/*
	 * each irqline is assigned an entry based
	 * on its configured spl level.
	 */

	cbus_apic_vector_data[newvector].in_use = 1;
	cbus_apic_vector_data[newvector].irq_line = irq_line;
	cbus_apic_vector_data[newvector].redirection_address =
		IO_APIC_REDIRLO + 2 * irq_line;

	/*
	 * irq0 is always built in as the clock vector in the ES/MP
	 * build.  we will use each processor's local APIC to send
	 * these clocks rather than polluting the EISA bus with the
	 * 8254.  so record the vector now so we can enable it later.
	 */
	if (irq_line == 0)
		cbus_apic_clock_vector = newvector;

	return newvector;
}

/*
 * this routine deallocates an interrupt vector for a driver that is
 * unloading.  we are only passed the irqline, not the vector.
 */
void
cbus_apic_deallocate_int_vector(irq_line)
{
	int vector;

	ASSERT(irq_line < EISA_IRQLINES);

	if (cbus_apics_everywhere == 0)
		return;

	vector = cbus_apic_irqline_to_vector[irq_line];

	ASSERT(cbus_apic_vector_data[vector].in_use);

	cbus_apic_irqline_to_vector[irq_line] = 0;
	cbus_apic_vector_data[vector].in_use = 0;	/* this is enough */
}

void
cbus_apic_initialize_vectors()
{
	int irq_line, spllevel;

	/*
	 * first carve out the IPI vector essentials so that IPIs will
	 * be handled properly by the various interrupt dispatcher pieces.
	 */
	cbus_apic_vector_data[CBUS1_IPI_TASKPRI].in_use = 1;
	cbus_apic_vector_data[CBUS1_IPI_TASKPRI].irq_line = CBUS1_IPI_TASKPRI;
	ivect[CBUS1_IPI_TASKPRI] = psm_intr;
	intpri[CBUS1_IPI_TASKPRI] = PLHI;
	svcpri[CBUS1_IPI_TASKPRI] = PLHI;

	/*
	 * hardcode the ivect[0] clock entry so that all the processors
	 * run the same clock interrupt routine.
	 */
	ivect[0] = ci_clock;

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

		cbus_apic_initialize_vector(irq_line, spllevel);
	}
}

void
cbus_apic_nenableint(irq_line, spl_level, engnum)
int irq_line;
pl_t spl_level;
int engnum;
{
	int 	vector;

	vector = cbus_apic_initialize_vector(irq_line, spl_level);

	cbus_apic_enable_interrupt(vector, engnum);
}

void
cbus_apic_ndisableint(irq_line)
int irq_line;
{
	cbus_apic_deallocate_int_vector(irq_line);

	cbus_apic_disable_interrupt(irq_line);
}

/*
 *
 * Routine Description:
 *
 *    Check for CBUS system board configuration.  We must handle both
 *    all symmetric XM boards as well as a mixture of XM boards with and
 *    without APICs.
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
cbus_apic_parse_rrd(table, count)
struct ext_id_info *table;
unsigned long count;
{
	unsigned long			i;
	unsigned long			index;
	unsigned long			apics_everywhere = 0;
	struct ext_id_info		*p;

	/*
	 * Old versions of RRD don't have an EXT_ID_INFO entry
	 * in the extended configuration structure.  The old
	 * machines will not have apics and will enter this routine
	 * with a count of 0.
	 */
	if (count != 0)
	{
		apics_everywhere = 1;

		p = table;

		/*
		 * if every processor board in the system has an APIC,
		 * then we'll use APIC mode for everything.  if any processor
		 * board does not have an APIC, we'll resort to using CBUS I/O
		 * space for all the processor IPIs, and 8259s on the base and
		 * cli/sti on all the additional processors.
		 */
		for (index = 0; index < count; index++, p++)
		{
			if (index == 0)
			{
				/*
				 * this is the special CBUS global arbitration
				 * ID, which doesn't correspond to any processor
				 * element. skip it.
				 */
				continue;
			}

			if (p->pm == 0)
				continue;	/* not a processor board */

			if ((p->pel_features & ELEMENT_HAS_APIC) == 0)
			{
				apics_everywhere = 0;
				break;
			}
		}
	}

	if (apics_everywhere) 
	{
		/*
		 * set global flag so that all remaining CBUS code flips
		 * into distributed interrupt APIC mode for this platform.
		 */
		cbus_apics_everywhere = 1;

		/*
		 * Initialize the vectors each spl level represents in hardware
		 */
		for (i = 0; i <= PLMAX; i++)
		{
			corollary_spl_to_vector[i] = 0x10 * i
				+ CBUS1_FIRST_DEVICE_TASKPRI;
		}

		/*
		 * Assign a vector to each irq line (based on the spl level
		 * it's configured at.  This is the vector we will see when
		 * the line interrupts us.
		 */
		cbus_apic_initialize_vectors();
	}
	else
	{
		default_bindcpu = 0;
		/*
		 * here we carve out the ivect[] entry for the IPI vector.
		 * this is for the case of additional processors without APICs
		 * or the case where all CPUs don't have APICs.
		 */

		ivect[NONAPIC_IPI_VECTOR] = psm_intr;
		intpri[NONAPIC_IPI_VECTOR] = PLHI;
		svcpri[NONAPIC_IPI_VECTOR] = PLHI;
	}

	cbus_apic_irq_polarity = corollary_query_interrupt_polarity();
}

/*
 *
 * Routine Description:
 *
 *    Called to put all the other processors in reset prior to reboot.
 *    Only the CBUS boot processor can reset all the others with a
 *    global broadcast.  Although each additional processor has the
 *    capability to reset any other additional processor, no additional
 *    processor can reset the boot processor.  So the calling processor
 *    sends an interrupt to the boot processor to have him take care of
 *    it all.
 *
 *    The boot processor must return to poke the 8042 into oblivion.
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
cbus_apic_reset_all_other_processors()
{
	ASSERT (myengnum == 0);

	/*
	 * issue a global reset to the broadcast address
	 */

	COUTB(broadcast_csr, corollary_global.ci_sreset, 
		corollary_global.ci_sreset_val);

	/*
	 * Disable BIOS shadowing until RRD recopies the BIOS ROM
	 * into shadow RAM again.  This is because we "reclaim" the
	 * BIOS hole up at 256MB + 640K, to use it as general purpose
	 * (shadowed) RAM for ES/MP.  Referring to the ROM at 640K (NOT
	 * 256MB + 640K) you will see a pristine BIOS copy, but
	 * accessing it at the high address is like using any other
	 * area of general-purpose DRAM.
	 *
	 * thus, during ES/MP, the shadowed copy will be overwritten with
	 * random data as the pages are used, and we must warn the BIOS
	 * to recopy the ROM into the shadowed RAM on bootup.  Here's
	 * the magic to instruct the BIOS accordingly:
	 *
	 * Note that once RRD regains control, he will again
	 * shadow the BIOS as long as the EISA Config CMOS bits
	 * instruct him to.  The shadowing is only being disabled
	 * here because the BIOS will initially get control _before_
	 * the Corollary RRD ROM does.
	 *
	 * if this code wasn't here, the user would have to cycle
	 * power to boot up successfully.
	 */

	COUTB(cbus_bridge, CBUS1_SHADOW_REGISTER, DISABLE_BIOS_SHADOWING);
}

/*
 *
 * void
 * cbus_apic_initialize_clock()
 *
 * Routine Description:
 *
 *    This routine initializes the system time clock for each calling
 *    processor to generate an interrupt every 10 milliseconds, using
 *    this processor's local APIC timer.
 *
 * Arguments:
 *
 *    None
 *
 * Return Value:
 *
 *    None.
 *
 */
void
cbus_apic_initialize_clock()
{
	unsigned	rate = CLOCK_INTERRUPT_RATE * APIC_TIMER_MICROSECOND;

	asm("pushfl");
	asm("cli");

	/*
	 * as a frame of reference, the count register decrements
	 * approximately 0x30 per asm instruction on a 486/33.
	 * initialize the local timer vector table entry with
	 * appropriate Vector, Timer Base, Timer Mode and Mask.
	 */

	cbus_local_apic->timer_vector.rb.dword1 =
			(cbus_apic_clock_vector|CLKIN_ENABLE_PERIODIC);

	/*
	 * poke initial count reg to start the periodic clock timer interrupts.
	 * the IDT entry is valid & enabled on entry to this routine.
	 */

	cbus_local_apic->initial_count.rb.dword1 = rate;

	asm("popfl");
}

void
cbus_apic_enable_all_interrupts()
{
	unsigned irq_line, vector;	

	if (cbus_apics_everywhere == 0)
	{
		return;
	}

	if (myengnum != 0)
		return;

	/*
	 * for APIC-based systems, we need to go pound the
	 * redirection table entries to enable all the interrupts.
	 */
	for (irq_line = 0; irq_line < EISA_IRQLINES; irq_line++)
        {
		vector = cbus_apic_irqline_to_vector[irq_line];

		if (vector)
			cbus_apic_enable_interrupt(vector, intcpu[irq_line]);
	}
}

void
cbus_apic_cpuintr(index)
int index;
{
	asm("pushfl");
	asm("cli");

	while (cbus_local_apic->apic_icr.rb.dword1 & APIC_ICR_BUSY)
		continue;

	/*
	 * defaulting the rest of the interrupt command
	 * register bits to zero is fine.  destination must
	 * be written before the low 32 bits of the ICR.
	 * remember only the high byte of the logical
	 * destination register is being compared, so we must
	 * poke the same portion of the destination here.
	 */
	cbus_local_apic->apic_icr.ra.destination =
		(1 <<(index + APIC_BIT_TO_ID));

	cbus_local_apic->apic_icr.rb.dword1 = 
		(APIC_LOGICAL_MODE_IN_ULONG | CBUS1_IPI_TASKPRI);

	asm("popfl");
}
