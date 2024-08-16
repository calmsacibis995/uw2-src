/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/autoconf.c	1.18"
#ident	"$Header: $"

/*
 * autoconf.c
 *	Hardware specific routines to initialize various
 *	components and setup hardware effected data structures.
 */

#include <io/cfg.h>
#include <io/clkarb.h>
#include <io/slic.h>
#include <io/slicreg.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ipl.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

/* private function declarations */
static void conf_clkarb(void);
static void conf_proc(void);
void nullint(int);
void strayint(int);
void ivec_init(uint, uint, void (*)());
void ivec_free(uint, uint);
int ivec_alloc(uint);

/* global variables */
int	boothowto;			/* boot flags */
int	sys_clock_rate;			/* MHz system runs at */
int	fp_lights = 0;			/* control behavior of panel lights */
int	console_board_type = -1;	/* console board type */
int	Nengine;			/* number of physical processors */
struct	engine	*engine;		/* base of engine array */
struct	engine	*engine_Nengine;	/* end of engine array */
struct	bin_header int_bin_table[SLICBINS];	/* Interrupt Bin Table */
int	bin_alloc[SLICBINS];		/* for allocating vectors */
/* maps SLIC # to cfg info about that SLIC */
struct	ctlr_desc *slic_to_config[MAX_NUM_SLIC];
struct firmwaresw *firmwaresw;

/*
 * void
 * configure(void)
 *
 *	Scan the HW configuration information, do probes, etc,
 *	all in the name of determining what's out there.
 *
 * Calling/Exit State:
 *
 *	Assumes system is only running on the boot processor.
 *	No return value.
 */

void
configure(void)
{
	register struct ctlr_desc *cd;

	/*
	 * Determine boot flags and system clock rate.
	 */
	boothowto = KVCD_LOC->c_boot_flag;
	sys_clock_rate = KVCD_LOC->c_clock_rate;

	/*
	 * Build slic to config information map.
	 */
	for (cd = KVCD_LOC->c_ctlrs; cd < KVCD_LOC->c_end_ctlrs; cd++)
		slic_to_config[cd->cd_slic] = cd;

	/*
 	 * Configure clock-arbiter board, processors, and memory.
 	 */
	conf_clkarb();
	conf_proc();
	conf_mem();
}

/*
 * void
 * cfg_relocate(void)
 *
 *	Walks the firmware configuration table located at
 *	CD_LOC and relocates the physical addresses within
 *	it to their kernel virtual equivalent, KVCD_LOC, 
 *	which is the original value plus KVSBASE.
 *
 * Calling/Exit State:
 *	
 *	No return value.  Assumed to be called very early in the
 *	system initialization process. 
 *
 * Remarks:
 *
 *	The firmware configuration table has two virtual
 *	address mappings at this point:  
 *		CD_LOC is temporarily mapped virtual == physical 
 *		and is writable.
 *	
 *		KVCD_LOC is its permanent virtual address, which
 *		will be  mapped read-only on X486 procs.  
 *	
 *	Therefore a "CD_LOC" address must be used to write out
 *	adjustements to the table, but either is valid for reading
 *	values.  For this reason, the control table of contents and
 *	descriptor arrays must be relocated before pointers to those
 *	arrays are relocated.
 */

void
cfg_relocate(void)
{
	struct ctlr_desc *cd;

	/*
	 * There are no pointers embedded in the ctlr_toc 
	 * array addressed by the c_toc member.
	 *
	 * Now relocate all usable pointers embedded in each 
	 * record of the ctlr_desc array addressed by the
	 * c_ctlrs member.  This must be done *before* relocating
 	 * the c_ctlrs member of the main descriptor.
	 *
	 * NOTE: There are pointers within the ctlr_type_desc 
	 * records which we are not relocating because they
	 * are firmware specific and of no use to the kernel.  
	 * The firmware just flushed them out with everything else.
	 */
	for (cd = CD_LOC->c_ctlrs; cd < CD_LOC->c_end_ctlrs; cd++) {
		cd->cd_ct->ct_name = 
			(char *)(KVSBASE + (uint_t)cd->cd_ct->ct_name);
		cd->cd_ct = 
			(struct ctlr_type_desc *)(KVSBASE + (uint_t)cd->cd_ct);
		if (cd->cd_type == SLB_SCSIBOARD) {
			cd->cd_sc_init_queue = (char *)
				(KVSBASE + (uint_t)cd->cd_sc_init_queue);
		}
	}

	/*
	 * Finally, relocate the pointers in the 
	 * main configuration descriptor.
	 */
	CD_LOC->c_sys.sd_slotpri = 
		(unchar *)(KVSBASE + (uint_t)CD_LOC->c_sys.sd_slotpri);
	CD_LOC->c_boot_name = (char *)(KVSBASE + (uint_t)CD_LOC->c_boot_name);
	CD_LOC->c_pagetable = (int *)(KVSBASE + (uint_t)CD_LOC->c_pagetable);
	CD_LOC->c_mmap = (ulong *)(KVSBASE + (uint_t)CD_LOC->c_mmap);
	CD_LOC->c_toc = (struct ctlr_toc *)(KVSBASE + (uint_t)CD_LOC->c_toc);
	CD_LOC->c_ctlrs = 
		(struct ctlr_desc *)(KVSBASE + (uint_t)CD_LOC->c_ctlrs);
	CD_LOC->c_end_ctlrs = 
		(struct ctlr_desc *)(KVSBASE + (uint_t)CD_LOC->c_end_ctlrs);
	CD_LOC->c_cons = (struct ctlr_desc *)(KVSBASE + (uint_t)CD_LOC->c_cons);
}

/*
 * void
 * conf_clocks(void)
 *
 *	Set up vectors for local clock and tod clock.
 *	Must do local clock first, to insure it gets vector 0,
 *	and tod clock next to insure it gets vector 1.
 *
 * Calling/Exit State:
 *
 *	No arguments.
 */
void
conf_clocks(void)
{
	ivec_init(LCLKBIN, ivec_alloc(LCLKBIN), lclclock);
	ivec_init(TODCLKBIN, ivec_alloc(TODCLKBIN), todclock);
}

/*
 * void
 * conf_clkarb(void)
 *
 *	Determine if clock arbiter is present. If present,
 *	set flag for front panel lights.
 *
 * Calling/Exit State:
 *
 *	Sets the global "fp_lights" according to whether the light_show
 *	is turned on or off.
 */

static void
conf_clkarb(void)
{
	struct ctlr_toc *toc = &KVCD_LOC->c_toc[SLB_CLKARBBOARD];

	/*
	 * Do we have a clock arb board?
	 */

	if (toc->ct_count == 0) {
		return;
	}

	/*
	 * We must be an S81, set up to use front-panel LED's.
	 */

	if (light_show > 1) {
		fp_lights = -1;		/* use front-panel and processor LEDs */
	}
	else {
		fp_lights = 1;
	}
	FP_IO_ONLINE;			/* ASSUME all drives online */
}

/*
 * static void
 * conf_proc(void)
 *
 *	Configure processors.
 *
 * Calling/Exit State:
 *
 *	Allocate engine table for all possible processors, but only remember
 *	alive and configured processors.
 *
 *	We also set `Nengine' here to the # of processors.
 */

static void
conf_proc(void)
{
	register struct	ctlr_desc *cd;
	register struct engine *eng;
	register int i;
	struct ctlr_toc *tc;
	int	sgs_cnt;
	int	sgs2_cnt;
	int	flags = 0;

	tc = &KVCD_LOC->c_toc[SLB_SGSPROCBOARD];
	cd = &KVCD_LOC->c_ctlrs[tc->ct_start];
	sgs_cnt = KVCD_LOC->c_toc[SLB_SGSPROCBOARD].ct_count;
	sgs2_cnt = KVCD_LOC->c_toc[SLB_SGS2PROCBOARD].ct_count;

	engine = calloc((sgs_cnt + sgs2_cnt) * sizeof(struct engine));
	cmn_err(CE_CONT, "# %d processors; slic", sgs_cnt + sgs2_cnt);

	for (i = 0; i < (sgs_cnt + sgs2_cnt); i++, cd++) {

		/*
		 * Look at SGS2 boards after exhausting
		 * SGS boards (SGS2 boards have i486 processors).
		 */

		if (i == sgs_cnt) {
			tc = &KVCD_LOC->c_toc[SLB_SGS2PROCBOARD];
			cd = &KVCD_LOC->c_ctlrs[tc->ct_start];
			flags = (E_SGS2 | E_FPU387);
		}

		/*
		 * Print SLIC id number and skip failed
		 * or deconfigured processors.
		 */

		cmn_err(CE_CONT, " %d", cd->cd_slic);
		if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF))
			continue;

		eng = &engine[Nengine++];
		eng->e_diag_flag = cd->cd_diag_flag;
		eng->e_slicaddr = cd->cd_slic;
		eng->e_flags = flags;
		eng->e_cpu_speed = cd->cd_p_speed;
		eng->e_nsets = (int)cd->cd_p_nsets;
		eng->e_setsize = (int)(CDP_SETSIZE(cd->cd_p_setsize)) / 1024;

		/*
		 * Set the engine rate and find the fastest processor.
		 */

		if (eng->e_cpu_speed == 0) {
			eng->e_cpu_speed = cpurate;
		}
		else if (eng->e_cpu_speed > cpurate) {
			cpurate = eng->e_cpu_speed;
		}

		/*
		 * Notice if processor has a 387 FPU.
		 */
		
		if (cd->cd_p_fp & SLP_387) {
			eng->e_flags |= E_FPU387;
		}
	}

	cmn_err(CE_CONT, ".\n");

	if (Nengine > MAXNUMCPU) {
		cmn_err(CE_CONT,
		  "#	%d processors found, but only %d can be used\n",
		  Nengine, MAXNUMCPU);
		Nengine = MAXNUMCPU;
	}
	engine_Nengine = &engine[Nengine];

	if (Nengine < sgs_cnt + sgs2_cnt) {
		cmn_err(CE_CONT, "#	Not using processors: slic");
		tc = &KVCD_LOC->c_toc[SLB_SGSPROCBOARD];
		cd = &KVCD_LOC->c_ctlrs[tc->ct_start];
		for (i = 0; i < (sgs_cnt + sgs2_cnt); i++, cd++) {
			if (i == sgs_cnt) {
				tc = &KVCD_LOC->c_toc[SLB_SGS2PROCBOARD];
				cd = &KVCD_LOC->c_ctlrs[tc->ct_start];
			}
			if ((cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF))
			||  ((cd->cd_p_fp & SLP_387) &&
			     (cd->cd_diag_flag & CFG_SP_FPU))) {
				cmn_err(CE_CONT, " %d", cd->cd_slic);
			}
		}
		cmn_err(CE_CONT, ".\n");
	}
	/*
	 * compute cpurate for delay loops. The value should 
	 * correspond to the "mips" rating for the processor.
	 * cpurate: is an estimate of the actual boards running rate 
	 * (usually max)
	 * lcpuspeed: is the number of mips if sys_clock_rate = 100
	 * cpurate is only used for probe routines running on the
	 * "boot" processor.
	 * Note this gets replaced in selfinit() with l.cpu_speed.
	 * when the actual cpu rate is known.
	 */
	cpurate = (lcpuspeed * cpurate) / 100;
}

/*
 * void
 * conf_intr()
 *
 * Allocate int_bin_table, then init 
 * all entries to point at strayint().
 *
 * Calling/Exit State:
 *
 * No return value.  Don't allocate anything for bin[0]
 * (software doesn't use this table).  The remaining
 * bins are fully allocated and address strayint().
 */

void
conf_intr(void)
{
	int i, vec;

	for (i = 1; i < SLICBINS; i++) {
		int_bin_table[i].bh_size = MSGSPERBIN;
		int_bin_table[i].bh_hdlrtab = (void(**)())
			kmem_alloc(MSGSPERBIN*sizeof(void (*)()), KM_SLEEP);
		ASSERT(int_bin_table[i].bh_hdlrtab);

		for (vec = 0; vec < int_bin_table[i].bh_size; vec++)
			ivec_init(i, vec, strayint);
	}
}

/* void
 * ivec_init(uint, uint, void (*)())
 *
 *      Install a SLIC interrupt handler.
 *
 * Calling/Exit State:
 *
 *	No return value. Install the handler function
 *	provided as the handler for the SLIC maskable
 *	interrupt specified by bin and vector.
 *
 *	Description:
 *		bin		SLIC bin number
 *		vec		vector number within bin
 *		handler		Interrupt service handler
 */

void
ivec_init(uint bin, uint vector, void (*handler)())
{
	ASSERT(bin >= 1 && bin < SLICBINS);
	ASSERT(vector < int_bin_table[bin].bh_size);

	int_bin_table[bin].bh_hdlrtab[vector] = handler;
}

/*
 * int
 * ivec_alloc(uint)
 *
 *	Allocate the next available vector from a given bin.
 *
 * Calling/Exit State:
 *
 *	Returns X, where 0 <= X < MSGPERBIN if a usable vector is
 *	located, searching in ascending sequence from vector number 
 *	stored in bin_alloc[bin], possibly wrapping around the bin.  
 *	After finding usable vector, set bin_alloc[bin] to the next 
 *	vector location, vector + 1 (adjusted for wrap).  Also, set
 *	the allocated vector's handler to "nullint" so we don't try
 *	to allocate it again until it is explicitly released.
 *
 * 	Returns -1 if no vectors remain for allocation.
 */

int
ivec_alloc(uint bin)
{
	uint vec;
	void (**bh)();
	int count, limit;

	ASSERT(bin >= 1 && bin < SLICBINS);

	limit = int_bin_table[bin].bh_size;	/* Max # of tries */
	bh = &int_bin_table[bin].bh_hdlrtab[0];	/* Addr of handler array */

	for (count = 0, vec = bin_alloc[bin]; count < limit; count++) { 
		if (bh[vec] == strayint) {	
			/*
			 * Return success...
			 */
			bin_alloc[bin] = (vec + 1) % limit;
			ivec_init(bin, vec, nullint);
			return(vec);
		}

		vec = (vec + 1) % limit;	/* Try next possible vector */
	}

	return(-1);		/* Failed - no vectors left in the pool */
}

/*
 * void
 * ivec_free(uint, uint)
 *
 *	Return the specified SLIC interrupt to the free list.
 *
 * Calling/Exit State:
 *
 *	Merely replaces the handler for the specified bin and
 *	vector combination with strayint().
 */

void
ivec_free(uint bin, uint vec)
{
	ASSERT(bin >= 1 && bin < SLICBINS);
	ASSERT(vec < int_bin_table[bin].bh_size);

	ivec_init(bin, vec, strayint);
}

/*
 * int
 * ivec_alloc_group(uint, uint)
 *
 *	Allocate a sequential group of available vectors from a given bin.
 *
 * Calling/Exit State:
 *
 *	Returns X, where 0 <= X <= MSGPERBIN-num if a usable group
 *	of sequential vectors is located, searching in ascending 
 *	sequence from the vector after the one most recently 
 *	allocated, saved in bin_alloc[bin]. After finding a usable 
 *	group, set bin_alloc[bin] to the next vector location, vector 
 *	+ num (adjusted for wrap).  Also, set the allocated vectors' 
 *	handler to "nullint" so we don't try to allocate them again 
 *	until they are explicitly released.
 *
 * 	Returns -1 if a sequential group of num vectors could not
 *	be found for allocation.
 */

int
ivec_alloc_group(uint bin, uint num)
{
	uint start, vec;
	void (**bh)();
	int i, limit;

	ASSERT(bin >= 1 && bin < SLICBINS);
	ASSERT(num <= int_bin_table[bin].bh_size);

	if (num == 0) return(-1);	/* Invalid argument */

	limit = int_bin_table[bin].bh_size;	/* Max # of tries */
	bh = &int_bin_table[bin].bh_hdlrtab[0];	/* Addr of handler array */
	start = vec = bin_alloc[bin];

	/*
	 * need to search for a contiguous group of vectors in an array.
	 * the index 'i' is used only to stop the search.  We have to iterate
	 * limit + num - 1 times because there are 'limit' possibilities for
	 * the start of the group and once we examine the starting vector we
	 * have to look at 'num' more elements to see if they are all free.
	 */
	for (i = limit + num - 1; i > 0; i--)  {
		if (bh[vec] == strayint) {	
			/*
			 * Found an available one; enough?
			 */
			if (vec - start + 1 == num) {
				/*
				 * Enough; Return success...
				 */
				bin_alloc[bin] = (vec + 1) % limit;
				/*LINTED*/
				for ( ; num > 0; num--, vec--) 
					ivec_init(bin, vec, nullint);
				return(start);
			}
			vec++;		/* Keep checking next location */
			if (vec == limit) {
				start = vec = 0;  /* Table wrap-around */
			}
		} else {
			/*
			 * Not available; retry from next possible 
			 * vector, adjusting for wrap-around.
			 */
			start = vec = (vec + 1) % limit;
		}
	}

	return(-1);		/* Failed - not enough vecs left in sequence */
}

/*
 * void
 * ivec_free_group(uint, uint, uint)
 *
 *	Return the specified group of SLIC interrupts to the free list.
 *
 * Calling/Exit State:
 *
 *	Merely replaces the handler for the specified bin and
 *	vector combinations with strayint().
 */

void
ivec_free_group(uint bin, uint vec, uint num)
{
	/* FIX THESE - arunk has notes */
	ASSERT(bin >= 1 && bin < SLICBINS);
	ASSERT(vec < int_bin_table[bin].bh_size);
	ASSERT(num <= int_bin_table[bin].bh_size);
	ASSERT(vec + num <= int_bin_table[bin].bh_size);

	/*LINTED*/
	for ( ; num > 0; num--, vec++) 
		ivec_init(bin, vec, strayint);
}

/* void
 * nullint(int)
 * 
 *	Stray SLIC interrupt catcher for allocated vectors.
 *
 * Calling/Exit State:
 *
 *	No return value. Reports the occurrence of an SLIC maskable 
 *	interrupt which had been allocated, but not initialized with
 *	its owner's handler routine.  Invokes cmn_error() to report 
 *	vector and the current value of SLIC local mask, which allows 
 *	inference of interrupting bin.
 */

void
nullint(int vec)
{
	struct  cpuslic *sl = (struct cpuslic *)KVSLIC;

	/*
	 *+ A null interrupt was caught.  This is caused by a SLIC maskable
	 *+ interupt that has been allocated but not initialized.
	 */
	cmn_err(CE_WARN, "No handler for intr, vector %d ipl 0x%x.", 
		vec, sl->sl_lmask);
}

/*
 * void
 * strayint(int)
 *
 *	Stray SLIC interrupt catcher.
 *
 * Calling/Exit State:
 *
 *	No return value.  Reports the occurance of an unexpected,
 *	unallocates SLIC maskable interrupt.  Invokes cmn_error()
 *	to report vector and the current calue of SLIC local mask,
 *	which allows inference of interrupting bin.
 */

void
strayint(int vec)
{
	register struct cpuslic *sl = (struct cpuslic *)KVSLIC;

	/*
	 *+ A stray interrupt was caught.  
	 *+ The interrupt occurred at a level not expected by the system.
	 */
	cmn_err(CE_WARN,
		"Stray intr, vector %d ipl 0x%x.", vec, sl->sl_lmask);
}

/*
 * void
 * io_kmadv(void)
 *	Call kmem_advise for data structures present in IO.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */

void
io_kmadv(void)
{
	extern void str_kmadv(void);

	str_kmadv();
}
