/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/cbus/cbus1.c	1.11"
#ident	"$Header: $"

#define i486
#include <io/prf/prf.h>
#include <proc/seg.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <svc/corollary.h> 
#include <svc/errno.h>
#include <svc/pit.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/map.h>
#include <util/kdb/xdebug.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <io/f_ddi.h>

#include <cbus.h>

extern int xintr_intr();
extern void default_crllry_registers();
extern void cbus_scsireversal();
extern void cbus_mappings();
extern void cbus_unisetup();
extern void cbus_refresh_off();
extern void cbus_go1();
extern void cbus_go2();
extern void cbus_go1end();
extern void cbus_clr_intr();
extern void cbus_ecc_init();
extern void cbus_ecc_disable();
extern volatile uint_t psm_eventflags[MAXNUMCPU];

int			cbus_io[MAXACPUS];

int			cbus_bridge;
extern int		corollary_cpuids[];

long			cbus_wait40ms = 100000;

int			cbus_cflush[MAXACPUS];

int			cbus_id0;
int			cbus_all;


int	cbus_memaddr[MAXRAMBOARDS] = {
	0x040000, 0x440000, 0x840000, 0xc40000, 
};

int	ex_cbus_memaddr[MAXRAMBOARDS] = {
	0x00000, 0x40000, 0x80000, 0xc0000, 
};

unsigned int *cbus_memory[MAXRAMBOARDS];

char *cbus_nmip[MAXACPUS];

int maxramboards = MAXRAMBOARDS;

extern unsigned cbus_apics_everywhere;

extern int xclock_pending;
extern int prf_pending;
extern uint_t engine_evtflags;

extern int corollary;

extern int      (*corollary_nmi_hook)();

int
cbus_pres()
{
	if ((corollary_global.ci_machine_type & 
		(MACHINE_CBUS1 | MACHINE_CBUS1_XM)) == 0)
	{
		return 0;
	}

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[cbus_pres]\n");
#endif
	if (cbuszpres())
	{
#ifdef COROLLARY_DEBUG
		cmn_err(CE_WARN, "Unsupported Corollary machine by this PSM\n");
#endif
		return 0;
	}
	
	if (cbusmitacpres() || cbuspres())
	{
		/*
		 * Set corollary to enable the early boot code in sysinit.
		 */
		corollary = 1;
#ifdef COROLLARY_DEBUG
		cmn_err(CE_CONT, "[cbus_pres - exit 1]\n");
#endif
		return 1;
	}

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[cbus_pres - exit 0]\n");
#endif

	return 0;
}

/*
 * detect the Corollary cbus 386/AT BRIDGE & 486/AT_BRIDGE
 * don't confuse this with C-bus+.  called before anything is set up.
 */
int
cbuspres()
{
	if (find_string("Corollary", 0xFFFE0000, 0xFFFF))
	{
		mpvendor = CBUS_OEM_COROLLARY;
		mpvendorclass = MP_CRLLRYISA;
		return 1;
	}
	return 0;
}


int
cbuszpres()
{
	if (find_string("MFM-1000", 0xF1000, 0xF000))
		return 1;

	return 0;
}

int
cbusmitacpres()
{
	if (find_string("MITAC TIGER", 0xF0000, 0xFFFF))
	{

		/*
		 * we are on a Mitac - Corollary licensee of the 2-board set
		 */
		mpvendor = CBUS_OEM_MITAC;
		mpvendorclass = MP_CRLLRYISA;
		return 1;
	}
	return 0;
}

#define	AB_MASK		0x1000000

cbuspeladdr(id, a)
{
	if (corollary_valid_ids)
		return corollary_ext_id_info[id].pel_start + (ulong)(a);
	return corollary_global.cbusio + AB_MASK + ( ((id)<<18) | (ulong)(a) );
}

cbuslocaliobase(id, paddr)
{
	return corollary_valid_ids ? corollary_ext_id_info[id].io_start : paddr;
}

cbuslocaliosize(id, size)
{
	return corollary_valid_ids ? corollary_ext_id_info[id].io_size : size;
}

void
cbusreset()
{
#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[cbusreset]\n");
#endif
	COUTB(cbus_all, corollary_global.ci_sreset, 
		corollary_global.ci_sreset_val);
}

void
cbus_findcpus()
{
#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[cbus_findcpus]\n");
#endif
	cbus_mappings();

	cbus_ecc_init();

	corollary_num_cpus = cbus_arb();

	cbus_unisetup();

	cbus_refresh_off();
#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "found %d cpus\n", corollary_num_cpus);
#endif
}

int
cbus_arb()
{
	register int	i, n;
	int		numacpus;
	unsigned	size;

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[cbus_arb]\n");
#endif
	cbusreset();

	numacpus = 0;

	if (corollary_global.rrdarb)
	{
		struct processor_configuration *p;

		p = corollary_hw_info.proc_config;

		for (i = 0; i < MAXSLOTS; i++, p++)
		{
			if (p->proc_type == PT_NO_PROCESSOR || i == B_ID)
				continue;

			corollary_cpuids[numacpus++] = i;
		}

		cbus_scsireversal(corollary_cpuids, numacpus);
		return numacpus;
	}

	for (i = 0; i < HICPUID; i++)
	{
		COUTB(cbus_id0, corollary_global.ci_contend, 
			corollary_global.ci_contend_val);

		COUTB(cbus_all, corollary_global.ci_contend, 
			corollary_global.ci_contend_val);

		n = inb(ATB_STATREG) & BS_ARBVALUE;

		if (corollary_valid_ids)
			size = corollary_ext_id_info[n].pel_size;
		else
			size = ptob(1);

		cbus_io[numacpus] = physmap(cbuspeladdr(n,0), size, KM_NOSLEEP);

		COUTB(cbus_io[numacpus], corollary_global.ci_setida, 
			corollary_global.ci_setida_val);

		if (n >= LOWCPUID && n <= HICPUID)
			corollary_cpuids[numacpus++] = n;

		n = inb(ATB_STATREG) & BS_ARBVALUE;

		if (n == 0) 
		{
			cbus_scsireversal(corollary_cpuids, numacpus);
			return (numacpus + 1);
		}
	}
	cmn_err(CE_PANIC, "CPU arbitration failed");
	/* NOTREACHED */
}

void
cbus_setup()
{
}


/*
 * the order of cbus_go1, ciboot, and cbus_go2 is critical.
 * cbus_go1 and cbus_go2 must be separated by hwboot;
 * cbus_go1 must be defined before cbus_go2.
 * the size of all three must be less than 4K.
 *
 * WARNING!!!   WARNING!!!   WARNING!!!
 *
 * do not put any routines between cbus_go1 and cbus_go2.  there
 * are tricky games being played with the cbus caches so
 * that startvec[] does not get flushed.
 */
void
cbus_go1(processor, dest, code, length)
int	processor;
caddr_t	dest;
char	*code;
int	length;
{
	register int n;

	/*
	 * this timeout is hard-coded and must be increased
	 * as faster base processors are created.
	 */
	register long wait40ms = cbus_wait40ms;

	for (n = 0; n < length; n++) 
		dest[n] = code[n];
	COUTB(cbus_io[processor-1], corollary_global.ci_creset, corollary_global.ci_creset_val);
#pragma loop_opt(off)
	while (wait40ms--)
		;
#pragma loop_opt()
}

void
cbus_go1end()
{
}

void
cbus_startcpu(processor, startaddr)
int	processor;
paddr_t startaddr;
{
	paddr_t		begin, end;
	int		dest;
	char		startvec[5];
	int		resetvec = corollary_global.resetvec - 16;

	dest = physmap(resetvec, ptob(1), KM_NOSLEEP);

	startvec[0] = 0xea;
	startvec[1] = ((int)startaddr & 0x0f);
	startvec[2] = 0;
	startvec[3] = ((int)startaddr >> 4) & 0xff;
	startvec[4] = ((int)startaddr >> 12) & 0xff;

	/*
	 * determine which go to use to avoid cache conflict with startvec
	 * NOTE: text is relocated and only 1K page-aligned.
	 */
	begin = (vtop(cbus_go1, NULL) & 
		(cbus_cflush[myengnum] - 1))>>CACHE_SHIFT;
	end = (vtop(cbus_go1end, NULL) & 
		(cbus_cflush[myengnum] - 1))>>CACHE_SHIFT;

	/*
	 * in order to startup ACPUs, we need to write its
	 * startup vector in the last 16 bytes of the memory
	 * address space.  this memory may or may not be
	 * present.  more specifically, a partially populated
	 * memory card which doesn't have memory at the end of
	 * the space, ie: a memory card at 48Mb->56Mb in a
	 * system with a 64Mb upper limit) will cause an ecc
	 * error if ecc is enabled.
	 *
	 * thus we must disable ecc to prevent such boards from
	 * generating ecc errors.  note that the errors are only
	 * generated on the read when we initially fill the
	 * startvector.  it's ok to enable ecc after startup
	 * even though the cacheline containing the startvec
	 * has not necessarily been flushed.  when it is flushed,
	 * it will go in the bit bucket and ecc errors are
	 * generated only on reads (not writes).
	 */
	cbus_ecc_disable();

	/*
	 * call cbus_go1 if there is no wrap...
	 * see the detailed comment at the top of cbus_go1 for more details.
	 */
	if (end != ((cbus_cflush[myengnum] - 1) >> CACHE_SHIFT) && end > begin)
		cbus_go1(processor, (caddr_t)dest, startvec, sizeof(startvec));
	else
		cbus_go2(processor, (caddr_t)dest, startvec, sizeof(startvec));

	cbus_ecc_enable();

	physmap_free(ptalign(dest), 1, 0);
}

void
cbus_go2(processor, dest, code, length)
int	processor;
caddr_t	dest;
char	*code;
int	length;
{
	register int n;

	register long wait40ms = cbus_wait40ms;

	for (n = 0; n < length; n++) 
		dest[n] = code[n];
	COUTB(cbus_io[processor-1], corollary_global.ci_creset, 
		corollary_global.ci_creset_val);
#pragma loop_opt(off)
	while (wait40ms--)
		;
#pragma loop_opt()
}

int
cbus_nmi_handle()
{
#ifdef COROLLARY_DEBUG
        calldebug();
        cbus_clr_nmi(myengnum);

        return NMI_BENIGN;
#else
        cbus_clr_nmi(myengnum);

        return NMI_UNKNOWN;
#endif
}


/* ARGSUSED */
void
cbus_intr_init(pl)
struct plocal *pl;
{
	cbus_clr_intr(myengnum);
	cbus_clr_nmi(myengnum);

        corollary_nmi_hook = cbus_nmi_handle;
}

void
cbus_set_intr(cpu)
int cpu;
{
	register int reg;

	if (cpu == 0)
		reg = cbus_bridge;
	else
		reg = cbus_io[cpu - 1];

	COUTB(reg, corollary_global.ci_sswi, corollary_global.ci_sswi_val);
}

void
cbus_clr_intr(cpu)
int cpu;
{
	COUTB(cpu == 0 ? cbus_bridge : cbus_io[cpu - 1], 
		corollary_global.ci_cswi, corollary_global.ci_cswi_val);
}

int
cbus_set_nmi(cpu)
{
	register int reg;

	/*
	 * the cbus base processor should be sent cpuintrs, not NMIs.
	 * let the world know by returning 0.
	 */
	if (cpu == 0)
		return 0;

	reg = cbus_io[cpu - 1];

	COUTB(reg, corollary_global.ci_snmi, corollary_global.ci_snmi_val);

	return 1;
}

#define	NMI_MASK		0x70	
#define IO_CHANNEL_CHECK	0x61

int
cbus_clr_nmi(cpu)
{
	extern char *cbus_nmip[];
	register unsigned char  io_channel_check;

	/*
	 * AT/EISA bus, must clear these latches in order to get
	 * another NMI on the base.
	 */
	if (cpu == 0)
	{
		outb(NMI_MASK, 0);

		io_channel_check = inb(IO_CHANNEL_CHECK);
		outb(IO_CHANNEL_CHECK, (io_channel_check&0x0f)|0x08);

		io_channel_check = inb(IO_CHANNEL_CHECK);
		outb(IO_CHANNEL_CHECK, io_channel_check&0x07);
	}

	if (!cbus_nmip[cpu])
		return 0;
	
	COUTB(cbus_io[cpu-1], corollary_global.ci_cnmi, 
		corollary_global.ci_cnmi_val);

	*cbus_nmip[cpu] = 0;

	return 1;
}

/*
 * initialize multiprocessor control register
 * offsets to standard Corollary defaults - called
 * from the pres() routines as needed, when first
 * identifying a vendor's platform.
 */
void
default_crllry_registers()
{
	corollary_global.ci_creset = CI_CRESET;
	corollary_global.ci_sreset = CI_SRESET;
	corollary_global.ci_contend = CI_CONTEND;
	corollary_global.ci_setida = CI_SETIDA;
	corollary_global.ci_cswi = CI_CSWI;
	corollary_global.ci_sswi = CI_SSWI;
	corollary_global.ci_cnmi = CI_CNMI;
	corollary_global.ci_snmi = CI_SNMI;
	corollary_global.ci_sled = CI_SLED;
	corollary_global.ci_cled = CI_CLED;
	corollary_global.ci_machine_type = MACHINE_CBUS1;
}

/*
 * On the passive backplane, the internal SCSI
 * bus is connected to low id slots.  The
 * bootable drive is assumed to be connected
 * to the first SCSI processor arbitrated.
 * Therefore, we reverse the ids so that the
 * first processor is the processor in the
 * lowest id slot.
 */
void
cbus_scsireversal(cbus_ids, numacpus)
int *cbus_ids;
int numacpus;
{
	register int i, n;

	for (i = 0; i < (numacpus / 2); i++) 
	{
		n = cbus_ids[i];
		cbus_ids[i] = cbus_ids[numacpus - i - 1];
		cbus_ids[numacpus - i - 1] = n;
	}
}

/*
 * set up mappings for the base and additional processor control
 * registers.  remember that they may need to be different depending
 * on whether it is the base or an additional processor that is trying
 * to use it.
 */
void
cbus_mappings()
{
	register int	size;
	extern int	broadcast_csr;
	unsigned	broadcast_id;

	/*
	 * Map in I/O space for processor HICPUID
	 */
	if (corollary_valid_ids)
		size = corollary_ext_id_info[HICPUID].pel_size;
	else
		size = ptob(1);

	cbus_all = physmap(cbuspeladdr(HICPUID,0), size, KM_NOSLEEP);

	/*
	 * Map in I/O space for processor 0
	 */
	if (corollary_valid_ids)
		size = corollary_ext_id_info[0].pel_size;
	else
		size = ptob(1);

	cbus_id0 = physmap(cbuspeladdr(0,0), size, KM_NOSLEEP);

	broadcast_id = corollary_global.ci_broadcast_id;

	/*
	 * map the broadcast ID space
	 */
	if (corollary_valid_ids)
		size = corollary_ext_id_info[broadcast_id].pel_size;
	else
		size = ptob(1);

	if (broadcast_id == HICPUID)
		broadcast_csr = cbus_all;
	else
		broadcast_csr = physmap(cbuspeladdr(broadcast_id, 0), size, KM_NOSLEEP);
}


void
cbus_ecc_init()
{
	register int	i;
	unsigned	addr;

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[cbus_ecc_init]\n");
#endif

	for (i = 0; i < maxramboards; i++) 
	{
		if (corollary_global.baseram)
		{
			addr = corollary_global.cbusio + cbus_memaddr[i];
		}
		else
		{
			addr = corollary_global.cbusio + ex_cbus_memaddr[i];
		}


		cbus_memory[i] = (unsigned int *)physmap(addr, ptob(1), KM_NOSLEEP);
	}
}

void
cbus_ecc_disable()
{
	register int i;

	for (i = 0; i < maxramboards; i++)
	{
		if (corollary_hw_info.ext_mem_board[i].mem_attr) 
		{
			COUTB((unsigned char *)cbus_memory[i], 0,
				corollary_hw_info.ext_mem_board[i].mem_attr & 
				(~EDAC_EN));
		}
	}
}

cbus_ecc_enable()
{
	register int i;

	for (i = 0; i < maxramboards; i++)
	{
		if (corollary_hw_info.ext_mem_board[i].mem_attr)
			COUTB((unsigned char *)cbus_memory[i], 0,
			corollary_hw_info.ext_mem_board[i].mem_attr);
	}

	return 1;
}

/* RST - move me to a space file */
#ifdef NEVER
extern int corollary_no_refresh;
#else
int corollary_no_refresh;
#endif

void
cbus_refresh_off()
{
	if (corollary_no_refresh)
	{
		outb(PITCTL_PORT, 0x50);
		outb(PITCTR1_PORT, 0x00);
	}
}

/* RST - fix me - CFLUSH is wrong */
/*
 * crllry_unisetup will:
 * 	- turn on base 486 internal cache
 *	- map in the memory control registers
 *	- map in the bridge control register allowing
 *	  base interrupts and base LED activity
 *
 * note these are independent of multiprocessing.
 */
void
cbus_unisetup()
{
	register struct processor_configuration	*pc;
	register struct ext_id_info		*einfo;
	register int				size;

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "[cbus_unisetup]\n");
#endif
	pc = &corollary_hw_info.proc_config[corollary_global.bootid];

	cbus_cflush[myengnum] = FLUSH_ALL;

	if (pc->proc_type == PT_486) 
	{
		if (pc->proc_attr == PA_CACHE_ON)
		{
			corollary_i486cacheon();
		}
		else
		{
			corollary_i486cacheoff();
		}
	}

	einfo = &corollary_ext_id_info[corollary_global.bootid];

	size = corollary_valid_ids ? einfo->pel_size : ptob(1);

	cbus_bridge = physmap(cbuspeladdr(corollary_global.bootid, 0), size, KM_NOSLEEP);

	if (cbus_bridge == 0) {
		cmn_err(CE_PANIC, "no memory to map bridge card");
		/* NOTREACHED */
	}
}

cbus_get_cpu_type(processor)
int processor;
{
	register int id = (processor ? corollary_cpuids[processor - 1] : B_ID);
	return corollary_hw_info.proc_config[id].proc_type;
}

cbus_get_proc_attr(processor)
int processor;
{
	register int id = (processor ? corollary_cpuids[processor - 1] : B_ID);
	return corollary_hw_info.proc_config[id].proc_attr;
}

cbus_get_io_type(processor)
int processor;
{
	register int id = (processor ? corollary_cpuids[processor - 1] : B_ID);
	return corollary_hw_info.proc_config[id].io_function;
}

cbus_get_io_attr(processor)
int processor;
{
	register int id = (processor ? corollary_cpuids[processor - 1] : B_ID);
	return corollary_hw_info.proc_config[id].io_attr;
}

#ifdef LATER
/*
 * this routine is called (and run) by each processor.
 */
int
cbus_main()
{
	register percpu_t	*cp;


	if (myengnum == 0)
	{
#ifdef COROLLARY_DEBUG
		cmn_err(CE_CONT, "processor %d: online - ", myengnum);

		if (mpvendorclass == MP_CRLLRYEISA)
			cmn_err(CE_CONT, "EISA processor\n");
		else
			cmn_err(CE_CONT, "ISA  processor\n");
#endif
		return; 
	} 

#ifdef COROLLARY_DEBUG
	cmn_err(CE_CONT, "processor %d: online - ", myengnum);
#endif

	if (cbus_get_cpu_type(myengnum) == PT_486)
	{
		cbus_cflush[myengnum] = FLUSH486;

		if (cbus_get_proc_attr(myengnum) == PA_CACHE_ON)
			corollary_i486cacheon();
		else
			corollary_i486cacheoff();
	}
	else
		cbus_cflush[myengnum] = FLUSH386;

	switch (cbus_get_io_type(myengnum)) 
	{
		case IOF_SIO:	
#ifdef COROLLARY_DEBUG
			cmn_err(CE_CONT, "SIO  processor\n");
#endif
			cbus_nmip[myengnum] = physmap(0xc0000, ptob(1), KM_NOSLEEP) + 0x11;
			break;
		case IOF_SCSI:	
#ifdef COROLLARY_DEBUG
			cmn_err(CE_CONT, "SCSI processor\n");
#endif
			cbus_nmip[myengnum] = physmap(0x84000, ptob(1), KM_NOSLEEP);
			break;
		default:
#ifdef COROLLARY_DEBUG
			cmn_err(CE_CONT, "UNKNOWN processor\n");
#endif
			break;
	}

	return 0;
}

void
cbus_init()
{
	int io_type = cbus_get_io_type(myengnum);
	int io_attr = cbus_get_io_attr(myengnum);

	if (cbus_init_table[io_type] == NULL)
		return;

	(*cbus_init_table[io_type])(io_attr, corollary_cpuids[myengnum-1]);
}
#endif

void
cbus_ledon(cpu)
register int cpu;
{
	COUTB(cpu == 0 ? cbus_bridge : cbus_io[cpu - 1], 
		corollary_global.ci_sled, corollary_global.ci_sled_val);
}

void
cbus_ledoff(cpu)
register int cpu;
{
	COUTB(cpu == 0 ? cbus_bridge : cbus_io[cpu - 1], 
		corollary_global.ci_cled, corollary_global.ci_cled_val);
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
cbus_intr(oldpl, eax, eip, cs)
uint oldpl, *eax;
uint eip, cs;
{
	cbus_clr_intr(myengnum);

	if (psm_eventflags[myengnum] != 0)
		engine_evtflags |= atomic_fnc(&psm_eventflags[myengnum]);

	sti();

	xcall_intr();

	if (prf_pending > 0) 
	{
		prf_pending--;

/* hpw - comment out for USERMODE() argument mis-match.
		if (prfstat)
			prfintr(eip, USERMODE(cs));
*/
	}

	corollary_execute();

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

void
cbus_timer_init()
{
	if (cbus_apics_everywhere)
	{
		/*
		 * Initialize the apic clock.
		 */
		cbus_apic_initialize_clock();
	}
	else
	{
		if (IS_BOOT_ENG(myengnum))
		{
			/*
			 * Initialize the i8254 programmable interrupt timer.
			 */
			clkstart();
		}
	}
}

void
cbus_nenableint(irq_line, spl_level, engnum)
int irq_line;
pl_t spl_level;
int engnum;
{
}

void
cbus_ndisableint(irq_line)
int irq_line;
{
}

extern void cbus_apic_parse_rrd();

extern int cbus_initialize_cpu();

extern void cbus_apic_reset_all_other_processors();

extern void cbus_apic_enable_interrupt();

extern void cbus_apic_enable_all_interrupts();

struct corollarysw cbus_sw = {
	cbus_pres,
	cbus_setup,
	cbus_findcpus,
	cbus_startcpu,
	cbus_intr_init,
	cbus_clr_intr,
	cbus_set_nmi,
	cbus_clr_nmi,
	cbus_intr,
	cbus_ledon,
	cbus_ledoff,
	cbus_apic_parse_rrd,
	cbus_initialize_cpu,
	cbus_apic_reset_all_other_processors,
	cbus_apic_enable_all_interrupts,
	cbus_timer_init,
};
