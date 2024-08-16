/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/cbus/corollary.c	1.11"
#ident	"$Header: $"

#include <fs/vnode.h>
#include <mem/kmem.h>
#include <mem/tuneable.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/corollary.h>
#include <svc/errno.h>
#include <util/ksynch.h>
#include <util/kdb/xdebug.h>
#include <util/ghier.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>

#include <io/f_ddi.h>
#include <cbus.h>

/*
 * move this to ../../svc/corollary.h when possible.
 */
#define CBUS2_OEM_IBM_MCA	8

#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif

#define NULL	0

extern k_pl_t ipl;		/* per-processor ipl */
extern pl_t picipl;		/* ipl to which pic is currently programmed */

unsigned long			corollary_spl_to_vector[PLMAX + 1];
int				broadcast_csr;
int				cbus_booted_processors;
int				cbus_booted_processors_mask;

int				corollary_proceed[MAXACPUS];

int				corollary_cpuids[MAXACPUS];

int				corollary_valid_ids = 0;
int				corollary_max_ids = SMP_MAX_IDS;
struct ext_id_info		corollary_ext_id_info[SMP_MAX_IDS] = { 0 };

extern void			corollary_read_ext_ids();

struct configuration		configuration = { 0 };

struct ext_cfg_override		corollary_global = { 0 };
struct corollary_hardware_info	corollary_hw_info = { 0 };

int mpvendor = 0;
int mpvendorclass = 0;

/*
 * Base processor counts as a processor.
 */
int corollary_num_cpus = 1;

static char *rrd_signature_p = NULL;
static char *corollary_copyright_p = NULL;

static unsigned int rrd_signature[] = { 0xdeadbeef, 0 };

static char crllry_owns[] = 
	"Copyright(C) Corollary, Inc. 1991. All Rights Reserved";

extern int num_sw_entries;

#define	CI_INTR_LENGTH		256

void (*ci_intsw[MAXACPUS])();
void (*ci_softint[MAXACPUS])();
void (*ci_deferred_int[MAXACPUS])();
void (*ci_cpu_intr[MAXACPUS])();
void (*ci_nenableint[MAXACPUS])();
void (*ci_ndisableint[MAXACPUS])();

extern void cbus_pic_nop();
static void handle_rrd_override();

void (*ci_setpicmasks[MAXACPUS])();

struct corollary_intr_table {
	void		(*intr)();
	unsigned	intr_value;
};

struct corollary_intr_info {
	unsigned	first;
	unsigned	last;
	lock_t		*lock;
	unsigned	overflow;
};

struct corollary_intr_table ci_intr_table[MAXACPUS][CI_INTR_LENGTH];
struct corollary_intr_info ci_intr_info[MAXACPUS];

#define ABS_VAL_DIFF(x,y)	(((x-y) > 0)?(x-y):(y-x))

extern void 	corollary_unmapnow();

extern int	(*corollary_nmi_hook)();
extern void	(*corollary_main_hook)();

lkinfo_t	ci_cpuintr_lkinfo[MAXACPUS];

void
corollary_setup()
{
	(*ciswp->ci_setup)();
}

void
corollary_initialize_cpu(processor)
unsigned processor;
{
	(void)(*ciswp->ci_initialize_cpu)(processor);
}

/*
 * be careful when modifying this routine - it is used by all
 * our licensees as well as C-bus2.  do not violate compatibility!
 */
void
corollary_getconfig()
{
	struct processor_configuration	*pc_ptr;
	struct oem_rom_information	*oem_ptr;
	struct ext_memory_board		*mem_ptr;
	struct ext_cfg_header		*ptr_header;
	int				override_len = 0;
	char				*ptr_source;
	int				i;

	default_crllry_registers();

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[corollary_getconfig]\n");
#endif
	/*
	 * current Corollary SMP standard system parameters;
	 * initialize them here so they can be overridden later
	 * by new machines as needed...
	 */
	corollary_global.useholes = 1;
	corollary_global.bootid = B_ID;
	corollary_global.cbusio = CBUSIO;

	/*
	 * baseram defines where the kernel is mapping
	 * the base of physical memory at.  for the
	 * multiprocessor kernel, all physical memory is
	 * mapped at 64Mb; for the uniprocessor (including
	 * crllry_uni) kernels, all physical memory used to
	 * be mapped relative to zero, but is now also at 64Mb.
	 */
	corollary_global.baseram = CBUSMEM;
	corollary_global.resetvec = corollary_global.baseram + MB(64);
	corollary_global.memory_ceiling = corollary_global.baseram + MB(64);

	/*
	 * read in the memory configuration structure
	 */
	ptr_source = rrd_signature_p;

	if (ptr_source == NULL)
		cmn_err(CE_PANIC, "no memory configuration table");

	bcopy(ptr_source, (char *)&configuration, sizeof(configuration));

	/*
	 * Read in the processor configuration structure.
	 * This information used to be held in the configuration
	 * structure, but now it is in the "extended configuration"
	 * structure which follows the configuration structure.
	 * Multiple structures are strung together with a "checkword",
	 * "length", and "data" structure.  The first null "checkword"
	 * entry marks the end of the extended configuration 
	 * structure.
	 */
	ptr_source += sizeof(configuration);
	ptr_header = (struct ext_cfg_header *)ptr_source;

	pc_ptr = corollary_hw_info.proc_config;
	mem_ptr = corollary_hw_info.ext_mem_board;

	/*
	 * default to 1.50 for compatibility 
	 */
	oem_ptr = &corollary_hw_info.oem_rom_info;
	oem_ptr->oem_number = CBUS_OEM_COROLLARY;
	oem_ptr->oem_rom_version = 1;
	oem_ptr->oem_rom_release = 50;
	oem_ptr->oem_rom_revision = 0;

	/* 
	 * RRD ROMs from 1.50 on use the extended configuration
	 * information structure, denoted by EXT_CHECKWORD.
	 */
	if (*(unsigned *)ptr_source == EXT_CHECKWORD) {
		do {
			ptr_source += sizeof(struct ext_cfg_header);

			switch (ptr_header->ext_cfg_checkword) {
			case EXT_CHECKWORD:
				bcopy(ptr_source, (char *)pc_ptr,
					ptr_header->ext_cfg_length);
				break;
			case EXT_VENDOR_INFO:
				bcopy(ptr_source, (char *)oem_ptr,
					ptr_header->ext_cfg_length);
				break;
			case EXT_MEM_BOARD:
				bcopy(ptr_source, (char *)mem_ptr,
					ptr_header->ext_cfg_length);
				break;
			case EXT_CFG_OVERRIDE:
				if (!corollary_copyright_p)
					break;
				/*
				 * we just copy the size of the structures
				 * we know about.  if an rrd tries to pass us
				 * more than we know about, we ignore the
				 * overflow.  this needs to be made more
				 * flexible when we have time.
				 */
				override_len = MIN(
					sizeof(struct ext_cfg_override), 
					ptr_header->ext_cfg_length);
				if (override_len)
					handle_rrd_override(ptr_source,
						override_len);
				break;
			case EXT_ID_INFO:
				if (!corollary_copyright_p)
					break;
				corollary_read_ext_ids((struct ext_id_info *)
					ptr_source);
				break;
			case EXT_CFG_END:
				break;
			default:
				/*
				 * skip unrecognized configuration entries
				 */
				break;
			}
			
			ptr_source += ptr_header->ext_cfg_length;
			ptr_header = (struct ext_cfg_header *)ptr_source;

		} while (ptr_header->ext_cfg_checkword != EXT_CFG_END);
	}
	else {
		/* 
		 * if there is no extended configuration structure,
		 * this must be an old rom.  set up the processor
		 * configuration structure to look the same.  this
		 * code is here to support RRD releases prior to 1.50.
		 */
		oem_ptr->oem_rom_release = 49;

		pc_ptr += LOWCPUID;

		for (i = LOWCPUID; i < HICPUID; i++, pc_ptr++) {
			switch(configuration.slot[i]) {
			case ATSIO386:
				pc_ptr->proc_type = PT_386;
				pc_ptr->io_function = IOF_SIO;
				break;
			case ATSCSI386:
				pc_ptr->proc_type = PT_386;
				pc_ptr->io_function = IOF_SCSI;
				break;
			case ATSIO486:
				pc_ptr->proc_type = PT_486;
				pc_ptr->proc_attr = PA_CACHE_OFF;
				pc_ptr->io_function = IOF_SIO;
				break;
			case ATSIO486C:
				pc_ptr->proc_type = PT_486;
				pc_ptr->proc_attr = PA_CACHE_ON;
				pc_ptr->io_function = IOF_SIO;
				break;
			case ATBASE386:
				pc_ptr->proc_type = PT_386;
				pc_ptr->io_function = IOF_ISA_BRIDGE;
				break;
			case ATBASE486:
				pc_ptr->proc_type = PT_486;
				pc_ptr->proc_attr = PA_CACHE_OFF;
				pc_ptr->io_function = IOF_ISA_BRIDGE;
				break;
			case ATBASE486C:
				pc_ptr->proc_type = PT_486;
				pc_ptr->proc_attr = PA_CACHE_ON;
				pc_ptr->io_function = IOF_ISA_BRIDGE;
				break;
			case ATP2486C:
				pc_ptr->proc_type = PT_486;
				pc_ptr->proc_attr = PA_CACHE_ON;
				pc_ptr->io_function = IOF_P2;
				break;
			default:
				break;
			}
		}
	}

	/*
	 * determine if Corollary 2-board set is EISA
	 */
	pc_ptr = &corollary_hw_info.proc_config[corollary_global.bootid];

	if (pc_ptr->io_function == IOF_EISA_BRIDGE)
		mpvendorclass = MP_CRLLRYEISA;
}


int
corollary_findcpus()
{
	int i = 0;

	if (corollary_main_hook)
		(*corollary_main_hook)();

#ifdef COROLLARY_DEBUG
	("[corollary_findcpus]\n");
#endif

	(*ciswp->ci_findcpus)();

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "corollary_valid_ids = 0x%x\n", corollary_valid_ids);
#endif

	for ( ; i <= corollary_num_cpus ; i++)
	{
		if (corollary_ext_id_info[i].id == corollary_global.bootid)
			continue;
#ifdef COROLLARY_DEBUG
		cmn_err(CE_CONT, "found processor 0x%x slot 0x%x\n",
			i, corollary_cpuids[i-1]);
#endif
	}

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[exit - corollary_findcpus]\n");
#endif
	return corollary_num_cpus;
}

/*
 * read in the extended id information table.  filled in
 * by some of our C-bus licensees, and _all_ of the C-bus2
 * machines.
 */
void
corollary_read_ext_ids(p)
struct ext_id_info *p;
{
	register int i;
	extern int corollary_max_ids;

	for (i = 0; i < corollary_max_ids && p->id != 0x7f; i++, p++)
		bcopy(p, &corollary_ext_id_info[i], sizeof(struct ext_id_info));

	corollary_valid_ids = i;
}

static void
handle_rrd_override(p, len)
struct ext_cfg_override *p;
int len;
{
	/*
	 * it is possible that the RRD is older (ie: pre-XM/NT),
	 * and thus did not fill in any fields after ci_cled_val.
	 * meaning that we must assign defaults for the following:
	 *		ci_machine_type
	 *		ci_supported_environments
	 *		ci_broadcast_id
	 * note that XM/NT and CBUS-2 will have an RRD which fills in
	 * these 3 fields, and thus we set our defaults to a CBUS classic.
	 */

	corollary_global.ci_machine_type = MACHINE_CBUS1;
	corollary_global.ci_supported_environments = 0;
	corollary_global.ci_broadcast_id = ALL_CPUID;

	bcopy(p, &corollary_global, len);
}

char *
find_string(ptr, addr, length)
char *ptr;
int length;
paddr_t addr;
{
	register char *buf = (char *)physmap(addr, length, KM_NOSLEEP);
	int i, j = 0;

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "buf = 0x%x - ", buf);
#endif

	while (ptr[j]) 
	{
#ifdef COROLLARY_DEBUG
		cmn_err(CE_CONT, "[%x]", ptr[j]);
#endif
		j++;
	}

	if (j-- == 0) 
		return buf;

	length -= j;

	for ( ; length ; length--, buf++, addr++)
	{
		if (*buf != *ptr) 
			continue;

		if (*(buf+j) != *(ptr+j))
			continue;

		for (i = 1; i < j; i++)
			if (*(buf+i) != *(ptr+i))
				break;

		if (i >= j)
		{
#ifdef COROLLARY_DEBUG
			cmn_err(CE_CONT, "found @ 0x%x\n", buf);
#endif
			return buf;
		}
	}

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "not found\n");
#endif
	return NULL;
}

void
corollary_i486cacheon()
{
	asm(".set    CR0_CE, 0xbfffffff");
	asm(".set    CR0_WT, 0xdfffffff");
	asm("movl    %cr0, %eax");

	/* flush internal 486 cache */
	asm(".byte	0x0f");		
	asm(".byte	0x09");

	asm("andl	$CR0_CE, %eax");
	asm("andl	$CR0_WT, %eax");

	/* flush queues */
	asm("jmp	i486flush1");
	asm("i486flush1:");
	asm("movl    %eax, %cr0");
}

void
corollary_i486cacheoff()
{
	asm(".set    CR0_CD, 0x20000000");
	asm(".set    CR0_NW, 0x40000000");
	asm("movl    %cr0, %eax");

	/* flush internal 486 cache */
	asm(".byte	0x0f");		
	asm(".byte	0x09");

	asm("orl	$CR0_CD, %eax");
	asm("orl	$CR0_NW, %eax");

	/* flush queues */
	asm("jmp	i486flush2");
	asm("i486flush2:");
	asm("movl    %eax, %cr0");
}

int
corollary_pres()
{
	int i;

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[corollary_pres]\n");
#endif
	rrd_signature_p = find_string(rrd_signature, 
		RRD_RAM, 0x8000);

	corollary_copyright_p = find_string(crllry_owns, 
		RRD_RAM, 0x8000);

	corollary_getconfig();

	for (i = 0 ; i < num_sw_entries ; i++)
	{
		if ((*cisw[i]->ci_pres)())
		{
			ciswp = cisw[i];

			/*
			 * set up flags for whether to use APICs, etc for
			 * CBUS.  also initialize spl translation table.
			 */
			(*ciswp->ci_parse_rrd)(corollary_ext_id_info,
				corollary_valid_ids);

			return 1;
		}
	}
		
	return 0;
}

void
corollary_intr_init(pl)
struct plocal *pl;
{
	(*ciswp->ci_intr_init)(pl);
}

void
corollary_set_intr(cpu)
{
	(*ci_cpu_intr[cpu])(cpu);
}

void
corollary_clr_intr(cpu)
{
	(*ciswp->ci_clr_intr)(cpu);
}


void
corollary_online_engine(engno, startaddr)
int engno;
paddr_t startaddr;
{
	ci_intr_info[engno].lock = LOCK_ALLOC(XCALL_HIER, 5, 
		&ci_cpuintr_lkinfo[engno], KM_NOSLEEP);

	if (!ci_intr_info[engno].lock)
		cmn_err(CE_PANIC, "cannot allocate lock\n");

	(*ciswp->ci_startcpu)(engno, startaddr);
}

void
corollary_xcall_init()
{
	ci_intr_info[myengnum].lock = LOCK_ALLOC(XCALL_HIER, 5, 
		&ci_cpuintr_lkinfo[myengnum], KM_NOSLEEP);

	ipl = PLBASE;
	picipl = PLBASE;
}

corollary_picinit()
{
	if (myengnum == 0)
		return 0;

	return 1;
}

void
corollary_intr(oldpl, eax, eip, cs)
uint oldpl, *eax;
uint eip, cs;
{
	(*ciswp->ci_intr)(oldpl, eax, eip, cs);
}

int
corollary_nmi()
{
	/* future ecc driver */
	if (corollary_nmi_hook)
		return (*corollary_nmi_hook)();

	return NMI_UNKNOWN;
}

int
corollary_nmi_debug()
{
	calldebug();
	return 1;
}

void
corollary_ledon(cpu)
int cpu;
{
	(*ciswp->ci_ledon)(cpu);
}

void
corollary_ledoff(cpu)
int cpu;
{
	(*ciswp->ci_ledoff)(cpu);
}

void
corollary_execute()
{
	register struct corollary_intr_table *itp;
	register struct corollary_intr_info *infop;
	unsigned first_entry, last_entry;
	void (*routine)();
	unsigned value;
	int s;

	infop = &ci_intr_info[myengnum];

	s = LOCK(infop->lock, PLHI);

	first_entry = infop->first;
	last_entry = infop->last;

	while (last_entry != first_entry)
	{
		infop->first++;

		if (infop->first == CI_INTR_LENGTH)
			infop->first = 0;

		itp = &ci_intr_table[myengnum][first_entry];

		routine = itp->intr;
		value = itp->intr_value;

		UNLOCK(infop->lock, s);

		(*routine)(value);

		s = LOCK(infop->lock, PLHI);

		first_entry = infop->first;
		last_entry = infop->last;
	}

	UNLOCK(infop->lock, s);
}


void
corollary_cpuintr(target_cpu_num, routine, value)
int target_cpu_num;
void (*routine)();
unsigned value;
{
	register struct corollary_intr_table *itp;
	register struct corollary_intr_info *infop;
	unsigned first_entry, last_entry;
	int s;

	infop = &ci_intr_info[target_cpu_num];

	s = LOCK(infop->lock, PLHI);

	first_entry = infop->first;
	last_entry = infop->last;

	infop->last++;

	if (infop->last == CI_INTR_LENGTH)
		infop->last = 0;

	if (first_entry == infop->last)
	{
		infop->overflow = 1;
		cmn_err(CE_PANIC,"cpuintr: too many queued CPU %d interrupts\n",
			target_cpu_num);
	}

	itp = &ci_intr_table[target_cpu_num][last_entry];

	itp->intr = routine;
	itp->intr_value = value;

	UNLOCK(infop->lock, s);

	corollary_set_intr(target_cpu_num);
}

void
corollary_broadcast(routine, value)
void (*routine)();
unsigned value;
{
	register int i;
	register struct	engine	*eng = engine;

	for (i=0 ; i < Nengine ; i++, eng++)
	{
		if (myengnum == i)
			continue;		/* skip the caller */
                if ((eng->e_flags & E_NOWAY) == 0)
			corollary_cpuintr(i, routine, value);
	}
}

#define	STAT_8042	0x64

void
corollary_reboot()
{
	(*ciswp->ci_reset_all_others)();

	outb( STAT_8042, 0xfe );	/* trigger reboot */
}

/*
 * Mask for valid bits of edge/level control register (ELCR) in 82357 ISP:
 * ie: ensure irqlines 0, 1, 2, 8 and 13 are always marked edge, as the
 * I/O register will not have them set correctly.  All other bits in the
 * I/O register will be valid without us having to poke them.
 */
#define ELCR_MASK	0xDEF8

#define PIC1_ELCR_PORT	0x4D0	/* ISP edge/level control regs*/
#define PIC2_ELCR_PORT	0x4D1

/*
 *
 * Routine Description:
 *
 *    Called once to read the EISA interrupt configuration registers.
 *    This will tell us which interrupt lines are level-triggered and
 *    which are edge-triggered.  Note that irqlines 0, 1, 2, 8 and 13
 *    are not valid in the 4D0/4D1 registers and are defaulted to edge.
 *
 * Arguments:
 *
 *    None.
 *
 * Return Value:
 *
 *    The interrupt line polarity of all the EISA irqlines in the system.
 *
 */
unsigned long
corollary_query_interrupt_polarity()
{
	unsigned long	InterruptLines = 0;

	/*
	 * Read the edge-level control register (ELCR) so we'll know how
	 * to mark each driver's interrupt line (ie: edge or level triggered).
	 * in the APIC I/O unit redirection table entry.
	 */

	InterruptLines = ( ((unsigned long)inb(PIC2_ELCR_PORT) << 8) |
			   ((unsigned long)inb(PIC1_ELCR_PORT)) );

	/*
	 * Explicitly mark irqlines 0, 1, 2, 8 and 13 as edge.  Leave all
	 * other irqlines at their current register values.  If the system
	 * has a Microchannel Bus, then use the ELCR register contents as is.
	 */
	if (corollary_hw_info.oem_rom_info.oem_number != CBUS2_OEM_IBM_MCA)
		InterruptLines &= ELCR_MASK;

	return InterruptLines;
}

#ifdef LATER
extern unsigned	hr_lbolt;
extern unsigned	Hz;

unsigned delay_micro[MAXACPUS];

#define MSEC_PER_TICK		(1000/Hz)
#define uSEC_PER_MSEC		(1000)

void
microdelay(delay)
int delay;
{
	/*
	 * delay is in microseconds and delay_micro is in 
	 * count per milliseconds.
	 */
	register unsigned x = (delay_micro[myengnum] * delay) / uSEC_PER_MSEC;
	int y = 0;

	while(x != y)
		x--;
}

int ci_speed_unstable = 0;

#ifdef COROLLARY_DEBUG
int ci_xcount[30];
int ci_ycount[30];
#endif
int
corollary_speed_calibrate()
{
	int val1, val2;
	int count = 0;

cal_again:
	count++;

	val1 = corollary_getcount();
	val2 = corollary_getcount();

	if (count < 20)
	{
		/*
		 * If the count is not within 12%, recalculate.
		 */
		if (ABS_VAL_DIFF(val1,val2) > (val1>>3))
		{
#ifdef COROLLARY_DEBUG
			ci_xcount[count] = val1;
			ci_ycount[count] = val2;
#endif
			goto cal_again;
		}
	}
	else
	{
		ci_speed_unstable = 1;
		cmn_err(CE_WARN, "clock unstable\n");
	}

	/*
	 * Take the bigger of the two.
	 */
	val1 = (val1 > val2) ? val1 : val2;

	/*
	 * delay_micro will be in units of count / millisecond
	 */
	delay_micro[myengnum] = val1 / MSEC_PER_TICK;
}


corollary_getcount()
{
	register int x = 0;
	register unsigned start = hr_lbolt;

	/*
	 * Find the first edge of the clock
	 */
	while(hr_lbolt == start)
		continue;

	/*
	 * Start counting until the next edge.
	 */
	start = hr_lbolt;

	while(hr_lbolt == start)
		x++;

	return x;
}
#endif /* LATER */

void
corollary_enable_all_interrupts()
{
	(*ciswp->ci_enable_all_ints)();
}

void
corollary_timer_init()
{
	(*ciswp->ci_timer_init)();
}

corollary_nenableint(irq_line, spl_level, engnum)
int irq_line;
pl_t spl_level;
int engnum;
{
        (*ci_nenableint[myengnum])(irq_line, spl_level, engnum);
}

corollary_ndisableint(irq_line)
unsigned irq_line;
{
        (*ci_ndisableint[myengnum])(irq_line);
}


/*
 *
 * VOID
 * corollary_disable_8259s (IN USHORT MASK)
 *
 * Routine Description:
 *
 *	Called to disable 8259 input lines as we switch into full
 *	distributed interrupt chip mode.  our distributed interrupt
 *	chip logic in the CBC will handle all interrupts from this
 *	point on.  the only reason we have to leave irq0 enabled is
 *	because Intel's EISA chipset doesn't leave an external irq0
 *	clock line for us to wire into the CBC.  hence, we have wired
 *	it from the 8259 into the CBC, and must leave it enabled in
 *	the 8259 IMR.  note that we will never allow the now passive
 *	8259 to respond to a CPU INTA cycle, but we do need to see the
 *	interrupt ourselves in the CBC so we can drive an appropriate
 *	vector during the INTA.
 *
 *	For the CBUS architecture, we mask off ALL the interrupts coming
 *	from the EISA chipset, since timers are generated by each local APIC.
 *
 *	This is the ONLY place in the Corollary HAL (and all of NT!)
 *	where the 8259s are accessed.
 *
 *
 * Arguments:
 *
 *    Mask to put on the master and slave 8259.
 *
 * Return Value:
 *
 *    None.
 *
 */
void
corollary_disable_8259s(mask)
unsigned short mask;
{
	extern short imrport[];

	outb(imrport[0], (char)mask);
	outb(imrport[1], (char)(mask>>8));
}

void
cbus_pic_nop()
{
}


/*
 * void corollary_holdcpus_enable()
 *	Hold the cpu executing this routine until the value
 *	of the iv_cookie changed.
 *
 * Calling/Exit State:
 *	Called through xcall, the cpu is already in PLHI.
 */
void
corollary_holdcpus_enable(ci_interrupt)
volatile struct corollary_interrupt *ci_interrupt;
{
	int		irq_line;
	pl_t		spl_level;
	int		engnum;
	unsigned	cookie_value;

	irq_line = ci_interrupt->irq_line;
	spl_level = ci_interrupt->spl_level;
	engnum = ci_interrupt->engnum;
	cookie_value = *ci_interrupt->cookie_addr;

	while (*ci_interrupt->cookie_addr == cookie_value)
		continue;

	corollary_nenableint(irq_line, spl_level, engnum);

	picreload();
}

/*
 * void corollary_holdcpus_enable()
 *	Hold the cpu executing this routine until the value
 *	of the iv_cookie changed.
 *
 * Calling/Exit State:
 *	Called through xcall, the cpu is already in PLHI.
 */
void
corollary_holdcpus_disable(ci_interrupt)
volatile struct corollary_interrupt *ci_interrupt;
{
	int		irq_line;
	unsigned	cookie_value;

	irq_line = ci_interrupt->irq_line;
	cookie_value = *ci_interrupt->cookie_addr;

	while (*ci_interrupt->cookie_addr == cookie_value)
		continue;

	corollary_ndisableint(irq_line);

	picreload();
}

int	ci_null_pres() { return 0 ; }
void	ci_null_setup() {}
void	ci_null_startcpu() {}
void	ci_null_findcpus() {}
void	ci_null_intr_init() {}
void	ci_null_clr_intr() {}
int	ci_null_set_nmi() { return 0 ; }
int	ci_null_clr_nmi() { return 0 ; }
void	ci_null_intr() {}
void	ci_null_ledon() {}
void	ci_null_ledoff() {}
void	ci_null_initializeCPU() {}
int	ci_null_parserrd() {}
void	ci_null_resetallothers() {}
void	ci_null_enableintrs() {}
void	ci_null_timer_init() {}

struct corollarysw ci_null = {
	ci_null_pres,
	ci_null_setup,
	ci_null_findcpus,
	ci_null_startcpu,
	ci_null_intr_init,
	ci_null_clr_intr,
	ci_null_set_nmi,
	ci_null_clr_nmi,
	ci_null_intr,
	ci_null_ledon,
	ci_null_ledoff,
	ci_null_initializeCPU,
	ci_null_parserrd,
	ci_null_resetallothers,
	ci_null_enableintrs,
	ci_null_timer_init,
};

extern struct corollarysw cbus_sw;
extern struct corollarysw cbus2_sw;

struct corollarysw *cisw[] = {
	&ci_null,
	&cbus_sw,
	&cbus2_sw,
};

struct corollarysw *ciswp = NULL;

int num_sw_entries = sizeof(cisw)/sizeof(struct corollarysw *);
