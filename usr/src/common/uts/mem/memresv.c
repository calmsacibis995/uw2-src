/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/memresv.c	1.24"
#ident	"$Header: $"

/*
 * These routines handle memory resource reservations for both
 * anonymous memory ("anon") and locked real memory ("rmem").
 */

#define _MEM_RESV_C 1

#include <mem/anon.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/tuneable.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern void poolrefresh_outofmem(void);

fspin_t	vm_memlock;	/* This lock covers anoninfo, rmeminfo,
			   pages_pp_kernel, and pages_pp_locked */

event_t memwait_ev;	/* event for signalling memory waiters */

rmeminfo_t rmeminfo;
#ifndef NO_RDMA
dmainfo_t dmainfo;
#endif

#ifdef DEBUG

uint_t	pages_pp_kernel;
uint_t	pages_pp_locked;

#define	PAGES_PP_UP(pp, n) \
	(pp) += (n)

#define PAGES_PP_DOWN(pp, n) \
{ \
	ASSERT((pp) >= (n)); \
	(pp) -= (n); \
}

#else 	/* DEBUG */

#define PAGES_PP_UP(pp, n)
#define PAGES_PP_DOWN(pp, n)
	
#endif /* DEBUG */

#ifdef _MEM_RESV_STATS

/*
 * MEM_RESV STATISTICS
 *
 * 	mem_resv statistical information is organized into an open hash
 *	indexed by line, file, and reservation type. For each entry,
 *	the count of pages reserved is kept, as well as the number of
 *	calls.
 *
 *	The information is gathered by defining mem_resv, mem_unresv,
 *	and mem_resv_wait to the pre-processor macros (mem_instr_resv,
 *	mem_instr_unresv, and mem_instr_resv_wait respectively) - see
 *	memresv.h.
 */

/*
 * flag to dynamically turn off statistics collection at run time
 */
int mem_resv_stats_enabled = 1;

#define MEM_STATS_SIZE	227	/* # of entries in the hash table */

/*
 * mutex for all MEM_RESV STATISTICS
 */
STATIC fspin_t mem_instr_resv_lock;

/*
 * MEM_RESV STATISTICS table - organized as an open hash table
 */

struct mem_instr_resv {
	/*
	 *  information in the hash key
	 */
	mresvtyp_t		mir_typ;
	int			mir_line;
	char			*mir_file;

	/*
	 * information not in the hash key
	 */
	int			mir_pages;	/* [un]reservations attempted */
	int			mir_calls;	/* invocations */

	int			mir_fpages;	/* failed reservations */
	int			mir_fcalls;	/* invocations failed */
};

STATIC struct mem_instr_resv		mem_instr_resv_table[MEM_STATS_SIZE];
STATIC struct mem_instr_resv		*mem_sort_table[MEM_STATS_SIZE];
STATIC boolean_t			mem_instr_resv_overflow;
STATIC int				mem_instr_resv_used;

#endif /* _MEM_RESV_STATS */

/*
 * void rmem_init(void)
 *	Initialize locks and accounting for mem_resv().
 *
 * Calling/Exit State:
 *	None.
 */
void
rmem_init(void)
{
	FSPIN_INIT(&vm_memlock);
#ifdef _MEM_RESV_STATS
	FSPIN_INIT(&mem_instr_resv_lock);
#endif /* _MEM_RESV_STATS */
	EVENT_INIT(&memwait_ev);
	ASSERT(freemem > tune.t_minamem + tune.t_kmem_resv +
	       pages_dkma_maximum);
	ASSERT(freemem > tune.t_minamem + tune.t_kmem_resv + pages_pp_maximum);
	rmeminfo.rmi_kma_max = max_freemem() - tune.t_minamem;
	rmeminfo.rmi_max = rmeminfo.rmi_kma_max - tune.t_kmem_resv;
	rmeminfo.rmi_dkma_max = rmeminfo.rmi_max - pages_dkma_maximum;
	rmeminfo.rmi_user_max = rmeminfo.rmi_max - pages_pp_maximum;
#ifndef NO_RDMA
	dmainfo.dmi_max = maxfreemem[DMA_PAGE];
#endif /* NO_RDMA */
}

/*
 * int mem_resv(uint_t npages, mresvtyp_t typ)
 *	Reserve locked real memory ("rmem") and/or anonymous memory.
 *
 * Calling/Exit State:
 *	Returns non-zero on success.
 *
 * Remarks:
 *	This function does NOT block which means there is no way to wait
 *	for a memory reservation at this level. This is by design to spare
 *	us the overhead of looking to see if anyone is blocked when we
 *	do mem_unresv etc. Since most of our callers can tolerate failure
 *	this works out to everybodies advantage. 
 *
 *	Callers who need to block because their callers cannot tolerate 
 *	failure (are prepared to block) can use the function
 *	mem_resv_wait() (see below).
 */
int
mem_resv(uint_t npages, mresvtyp_t typ)
{
	FSPIN_LOCK(&vm_memlock);
	switch (typ) {
	case M_REAL:
		if (rmeminfo.rmi_resv + npages > rmeminfo.rmi_max)
			goto no_mem;
		rmeminfo.rmi_resv += npages;
		PAGES_PP_UP(pages_pp_kernel, npages);
		break;
	case M_REAL_USER:
		if (rmeminfo.rmi_resv + npages > rmeminfo.rmi_user_max)
			goto no_mem;
		rmeminfo.rmi_resv += npages;
		PAGES_PP_UP(pages_pp_locked, npages);
		break;
	case M_SWAP_KERNEL:
		if (anoninfo.ani_resv + npages > anoninfo.ani_max)
			goto no_mem;
		anoninfo.ani_resv += npages;
		break;
	case M_SWAP:
		if (anoninfo.ani_resv + npages > anoninfo.ani_user_max)
			goto no_mem;
		anoninfo.ani_resv += npages;
		break;
	case M_BOTH:
		if (rmeminfo.rmi_resv + npages > rmeminfo.rmi_max ||
		    anoninfo.ani_resv + npages > anoninfo.ani_max)
			goto no_mem;
		anoninfo.ani_resv += npages;
		rmeminfo.rmi_resv += npages;
		PAGES_PP_UP(pages_pp_kernel, npages);
		break;
	case M_BOTH_USER:
		if (rmeminfo.rmi_resv + npages > rmeminfo.rmi_user_max ||
		    anoninfo.ani_resv + npages > anoninfo.ani_user_max)
			goto no_mem;
		anoninfo.ani_resv += npages;
		rmeminfo.rmi_resv += npages;
		PAGES_PP_UP(pages_pp_locked, npages);
		break;
	case M_KERNEL_ALLOC:
		if (rmeminfo.rmi_resv + npages > rmeminfo.rmi_kma_max ||
		    anoninfo.ani_resv + npages > anoninfo.ani_kma_max)
			goto no_mem;
		anoninfo.ani_resv += npages;
		rmeminfo.rmi_resv += npages;
		PAGES_PP_UP(pages_pp_kernel, npages);
		break;
#ifndef NO_RDMA
	case M_DMA:
		if (dmainfo.dmi_resv + npages > dmainfo.dmi_max)
			goto no_mem;
		dmainfo.dmi_resv += npages;
		break;
#endif /* NO_RDMA */
	}
	FSPIN_UNLOCK(&vm_memlock);
	return 1;
no_mem:
	FSPIN_UNLOCK(&vm_memlock);
	/*
	 * We failed to get a reservation; inform poolrefresh in the hopes of
	 * freeing some up.
	 */
	poolrefresh_outofmem();
	return 0;
}

/*
 * boolean_t
 * mem_resv_check(void)
 *	Check if ``discretionary'' kma reservations have been exhausted for
 *	for either real or virtual swap memory.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if reservations have not been exhuasted and B_FALSE
 *	otherwise.
 *
 * Description:
 *	This routine is typically called by subsystems which can kmem_alloc
 *	large amounts of memory. A B_FALSE return is an indication that
 *	kmem_alloc()s should be gated off.
 */
boolean_t
mem_resv_check(void)
{
	boolean_t ret;

	FSPIN_LOCK(&vm_memlock);
	ret = (rmeminfo.rmi_resv <= rmeminfo.rmi_dkma_max &&
	       anoninfo.ani_resv <= anoninfo.ani_dkma_max);
	FSPIN_UNLOCK(&vm_memlock);

	return ret;
}

/*
 * void mem_unresv(uint_t npages, mresvtyp_t typ)
 *	Give back a memory reservation.
 *
 * Calling/Exit State:
 *	None.
 */
void
mem_unresv(uint_t npages, mresvtyp_t typ)
{
	FSPIN_LOCK(&vm_memlock);
	switch (typ) {
	case M_REAL:
		rmeminfo.rmi_resv -= npages;
		PAGES_PP_DOWN(pages_pp_kernel, npages);
		break;
	case M_REAL_USER:
		rmeminfo.rmi_resv -= npages;
		PAGES_PP_DOWN(pages_pp_locked, npages);
		break;
	case M_SWAP_KERNEL:
	case M_SWAP:
		anoninfo.ani_resv -= npages;
		break;
	case M_BOTH:
	case M_KERNEL_ALLOC:
		anoninfo.ani_resv -= npages;
		rmeminfo.rmi_resv -= npages;
		PAGES_PP_DOWN(pages_pp_kernel, npages);
		break;
	case M_BOTH_USER:
		anoninfo.ani_resv -= npages;
		rmeminfo.rmi_resv -= npages;
		PAGES_PP_DOWN(pages_pp_locked, npages);
		break;
#ifndef NO_RDMA
	case M_DMA:
		dmainfo.dmi_resv -= npages;
		break;
#endif /* NO_RDMA */
	}
	FSPIN_UNLOCK(&vm_memlock);
}

#ifndef DEBUG
/*
 * boolean_t
 * mem_resv_promote(mresvtyp_t mtype)
 *	Promote reservations from kernel to user level.
 *
 * Calling/Exit State:
 *	If mtype == M_REAL_USER then promotes from M_REAL to M_REAL_USER.
 *	If mtype == M_BOTH_USER then promotes from M_BOTH to M_BOTH_USER.
 *	
 *	Returns B_TURE if the promotion succeeds and B_FALSE otherwise.
 */
boolean_t
mem_resv_promote(mresvtyp_t mtype)
{
	ASSERT(mtype == M_REAL_USER || mtype == M_BOTH_USER);

	FSPIN_LOCK(&vm_memlock);

	if (rmeminfo.rmi_resv > rmeminfo.rmi_user_max ||
	    (mtype == M_BOTH_USER &&
	     anoninfo.ani_resv > anoninfo.ani_user_max)) {
		FSPIN_UNLOCK(&vm_memlock);
		return B_FALSE;
	}

	FSPIN_UNLOCK(&vm_memlock);
	return B_TRUE;
}

#else /* DEBUG */

/*
 * boolean_t
 * mem_resv_promote(uint_t npages, mresvtyp_t mtype)
 *	Promote reservations from kernel to user level.
 *
 * Calling/Exit State:
 *	``npages'' gives the number of pages to promote.
 *	If mtype == M_REAL_USER then promotes from M_REAL to M_REAL_USER.
 *	If mtype == M_BOTH_USER then promotes from M_BOTH to M_BOTH_USER.
 *	
 *	Returns B_TURE if the promotion succeeds and B_FALSE otherwise.
 */
boolean_t
mem_resv_promote(uint_t npages, mresvtyp_t mtype)
{
	ASSERT(mtype == M_REAL_USER || mtype == M_BOTH_USER);

	FSPIN_LOCK(&vm_memlock);

	if (rmeminfo.rmi_resv > rmeminfo.rmi_user_max ||
	    (mtype == M_BOTH_USER &&
	     anoninfo.ani_resv > anoninfo.ani_user_max)) {
		FSPIN_UNLOCK(&vm_memlock);
		return B_FALSE;
	}

	/*
	 * `promote' from M_REAL to M_REAL_USER
	 */
	PAGES_PP_DOWN(pages_pp_kernel, npages);
	PAGES_PP_UP(pages_pp_locked, npages);

	FSPIN_UNLOCK(&vm_memlock);
	return B_TRUE;
}

/*
 * void
 * mem_resv_demote(uint_t npages)
 *	Demote npages from M_REAL_USER/M_BOTH_USER to M_REAL/M_BOTH.
 *
 * Calling/Exit State:
 *	None.
 */
void
mem_resv_demote(uint_t npages)
{
	FSPIN_LOCK(&vm_memlock);
	PAGES_PP_UP(pages_pp_kernel, npages);
	PAGES_PP_DOWN(pages_pp_locked, npages);
	FSPIN_UNLOCK(&vm_memlock);
}

#endif /* DEBUG */

/*
 * int
 * mem_resv_wait(uint_t npages, mresvtyp_t typ, boolean_t cansig)
 *	Like mem_resv, but caller is prepared to block for reservation.
 *
 * Calling/Exit State:
 *	Caller is prepared to block and hold no LOCKs on entry.
 *
 *	If the caller has not specified a signalable sleep (!cansig), then
 *	they will only return when memory has been reserved.
 *
 *	If the caller has specified a signalable sleep (cansig), then this
 *	function can fail.
 *
 *	Returns non-zero on success.
 *
 * Remarks:
 *
 *	###PERF: The current implementation of memory reservation waiting
 *	is far from ideal, though it has the benefit of being simple and
 *	cheap. A more `ideal' solution would involve the the following:
 *
 *		- About to block LWPs would register the size and type of
 *		  the reservation they desired.
 *
 *		- The service routine would service the blocked LWPs by
 *		  calling mem_resv on the behalf of each. 
 *
 *		- LWPs would only be resumed when their memory reservation
 *		  had been made, unless they are signallable and signaled
 *		  before then.
 *
 *	This approach alleviates the `thundering herd' problem of the
 *	present solution. However, given the current choice and implementation 
 *	of blocking primitives, this approach would be very messy, involving
 *	locks from the process subsystem. If a true counting semaphore were to 
 *	become available this code could be reworked to take advantage of it.
 *
 *	Note that other approachs are possible. However, any approach which 
 *	attempts to gauge the instantaneous availability of memory reservations
 *	but does not actually make the reservations must handle a number of 
 *	pathologic conditions. For example, if the service routine only runs 
 *	in-between peak periods of memory usage, memory will seem abundant 
 *	when it decides to resume waiters but will be scarce when they finally 
 *	run. Conversely, if the service routine runs only at periods of peak 
 *	demand, too few or no waiters may be resumed. Good luck.
 */
int
mem_resv_wait(uint_t npages, mresvtyp_t typ, boolean_t cansig)
{

	ASSERT(KS_HOLD0LOCKS());

	MRW_STAT_CALLS(typ);

	if (!mem_resv(npages, typ)) {

		MRW_STAT_CALLS(typ);

		do {

			if (cansig) {
		
				/*
				 * If signalled out of the wait, simply
				 * return failure.
				 */

				if (!EVENT_WAIT_SIG(&memwait_ev, PRIMEM))
					return (0);

			}
			else  
				EVENT_WAIT(&memwait_ev, PRIMEM);

			MRW_STAT_SPINS(typ);


		} while (!mem_resv(npages, typ));
	}

	return (1);

}

/*
 * void
 * mem_resv_service(void)
 *	Resume LWPs blocked waiting for memory reservations.
 *
 * Calling/Exit State:
 *	Called once a second from poolrefresh().
 */
void
mem_resv_service(void)
{

	/*
	 * Only signal if someone blocked.
	 */

	if (EVENT_BLKD(&memwait_ev))
		EVENT_BROADCAST(&memwait_ev, 0);
	return;
}

#if defined(DEBUG) && !defined(_MEM_RESV_STATS)

/*
 * void
 * print_mem_resv_stats(void) 
 *	Dump stats collected on mem_resv waiting.
 * 
 * Calling/Exit State:
 *	DEBUG-function to be called from a kernel debugger.
 */
void
print_mem_resv_stats(void) 
{
	int i;

	debug_printf("Memory reservation waiting statistics\n\n");

	for (i = 0; i < M_TYPE_MAX; i++) {
		debug_printf("reservation type = %d\n", i);
		debug_printf(
		    "\ttotal calls = %d times blocked = %d times spun = %d\n",
		    mem_resv_wait_stats[i].mrw_calls,
		    mem_resv_wait_stats[i].mrw_blocked,
		    mem_resv_wait_stats[i].mrw_spins);
		debug_printf("\n");
	}
}
 
#endif /* DEBUG && !_MEM_RESV_STATS */

#ifdef _MEM_RESV_STATS

/*
 * STATIC void
 * mem_stats_instr(int npages, mresvtyp_t type, int ret, int line, char *file)
 *	Gather mem_resv/mem_unresv statistics.
 *
 *	npages, type	arguments to mem_resv(), mem_resv_wait(), or
 *			mem_unresv()
 *
 *	ret		non-zero indicates success, 0 failure
 *
 *	line, file	identifies the client for accounting purposes
 *
 * Calling/Exit State:
 *	The caller holds no FSPIN_LOCK.
 */

STATIC void
mem_stats_instr(int npages, mresvtyp_t type, int ret, int line, char *file)
{
	struct mem_instr_resv *org, *rp;

	FSPIN_LOCK(&mem_instr_resv_lock);

	org = rp = &mem_instr_resv_table[(type + line + (uint_t)file) %
					 MEM_STATS_SIZE];
	while(type != rp->mir_typ || line != rp->mir_line ||
	      file != rp->mir_file) {
		if (rp->mir_calls == 0) {
			/*
			 * not found - so allocate a new entry
			 */
			rp->mir_typ = type;
			rp->mir_line = line;
			rp->mir_file = file;
			mem_sort_table[mem_instr_resv_used] = rp;
			++mem_instr_resv_used;
			break;
		}
		if (++rp == &mem_instr_resv_table[MEM_STATS_SIZE]) {
			rp = &mem_instr_resv_table[0];
		}
		if (rp == org) {
			mem_instr_resv_overflow = B_TRUE;
			FSPIN_UNLOCK(&mem_instr_resv_lock);
			return;
		}
	}

	/*
	 * update stats
	 */
	++rp->mir_calls;
	rp->mir_pages += npages;
	if (!ret) {
		rp->mir_fpages += npages;
		++rp->mir_fcalls;
	}

	FSPIN_UNLOCK(&mem_instr_resv_lock);
}

/*
 * void
 * print_mem_resv_stats(void)
 *	Routine to print out memory reservation statistics.
 *
 * Calling/Exit State:
 *	No parameters.
 *
 * Remarks:
 *	The mem_instr_resv_lock is held during the sort, but not during the
 *	printout.  Consequently, the printout might print stale information.
 *	However, since information is never deleted from the tables,
 *	the pointers should always lead to valid addresses.
 *
 *	This function is intended for use from a kernel debugger.
 */
void
print_mem_resv_stats(void)
{
	struct mem_instr_resv *rp;
	struct mem_instr_resv **sp0, **sp1, **endp;
	char *type, *usage;
	boolean_t did_flip;
	int sign, diff;
#ifdef DEBUG
	int i;

	debug_printf("Memory reservation waiting statistics\n\n");

	for (i = 0; i < M_TYPE_MAX; i++) {
		debug_printf("reservation type = %d\n", i);
		debug_printf(
		    "\ttotal calls = %d times blocked = %d times spun = %d\n",
		    mem_resv_wait_stats[i].mrw_calls,
		    mem_resv_wait_stats[i].mrw_blocked,
		    mem_resv_wait_stats[i].mrw_spins);
		debug_printf("\n");
	}
#endif /* DEBUG */

	/*
	 * first sort the table in order of <file name, line number>
	 */
	FSPIN_LOCK(&mem_instr_resv_lock);

	endp = &mem_sort_table[mem_instr_resv_used];
	do {
		did_flip = B_FALSE;
		sp0 = &mem_sort_table[0];
		sp1 = &mem_sort_table[1];
		while (sp1 < endp) {
			diff = strcmp((*sp0)->mir_file, (*sp1)->mir_file);
			if (diff > 0 || (diff == 0 &&
			    (*sp0)->mir_line > (*sp1)->mir_line)) {
				rp = *sp0;
				*sp0 = *sp1;
				*sp1 = rp;
				did_flip = B_TRUE;
			}
			++sp0;
			++sp1;
		}
		--endp;
	} while (did_flip);

	FSPIN_UNLOCK(&mem_instr_resv_lock);

	debug_printf("%d of %d entries in use\n", mem_instr_resv_used,
		     MEM_STATS_SIZE);
	if (mem_instr_resv_overflow)
		debug_printf("WARNING: stats table has overflowed\n");
	sp0 = &mem_sort_table[0];
	endp = &mem_sort_table[mem_instr_resv_used];
	while (sp0 < endp) {
		if (debug_output_aborted())
			break;
		rp = *sp0;
		if (rp->mir_calls != 0) {
			switch(rp->mir_typ) {
			case M_REAL:
				type = "REAL";
				break;
			case M_REAL_USER:
				type = "REAL_USER";
				break;
			case M_SWAP_KERNEL:
				type = "SWAP_KERNEL";
				break;
			case M_SWAP:
				type = "SWAP";
				break;
			case M_BOTH:
				type = "BOTH";
				break;
			case M_BOTH_USER:
				type = "BOTH_USER";
				break;
			case M_KERNEL_ALLOC:
				type = "KERNEL_ALLOC";
				break;
			case M_DMA:
				type = "DMA";
				break;
			default:
				type = "UNKNOWN";
				break;
			}
			if (rp->mir_pages >= 0) {
				usage = "reservations";
				sign = 1;
			} else {
				usage = "un-reservations";
				sign = -1;
			}
			debug_printf("%d M_%s %s, %d calls, "
				     "line %d of file %s\n",
				     (rp->mir_pages - rp->mir_fpages) * sign,
				     type, usage,
				     rp->mir_calls - rp->mir_fcalls,
				     rp->mir_line, rp->mir_file);
			if (rp->mir_fcalls > 0) {
				debug_printf("    %d FAILURES in %d calls\n",
					     rp->mir_fpages, rp->mir_fcalls);
			}
		}
		++sp0;
	}
}

/*
 * int
 * mem_instr_resv(uint_t npages, mresvtyp_t type, int line, char *file)
 *	Intercept a call to mem_resv() in order to gather statistics.
 *
 *	npages, type	arguments to mem_resv()
 *
 *	line, file	identifies the client for accounting purposes
 *
 * Calling/Exit State:
 *	The caller holds no FSPIN_LOCK.
 */

int
mem_instr_resv(uint_t npages, mresvtyp_t type, int line, char *file)
{
	int ret;

	ret = mem_resv(npages, type);
	if (mem_resv_stats_enabled) {
		mem_stats_instr(npages, type, ret, line, file);
	}
	return ret;
}

/*
 * int
 * mem_instr_unresv(uint_t npages, mresvtyp_t type, int line, char *file)
 *	Intercept a call to mem_unresv() in order to gather statistics.
 *
 *	npages, type	arguments to mem_unresv()
 *
 *	line, file	identifies the client for accounting purposes
 *
 * Calling/Exit State:
 *	The caller holds no FSPIN_LOCK.
 */

void
mem_instr_unresv(uint_t npages, mresvtyp_t type, int line, char *file)
{
	if (mem_resv_stats_enabled) {
		mem_stats_instr(-npages, type, 1, line, file);
	}
	mem_unresv(npages, type);
}

/*
 * int
 * mem_instr_resv_wait(uint_t npages, mresvtyp_t type, boolean_t cansig,
 * 		    int line, char *file)
 *	Intercept a call to mem_resv_wait() in order to gather statistics.
 *
 *	npages, type, cansig 	arguments to mem_resv()
 *
 *	line, file		identifies the client for accounting purposes
 *
 * Calling/Exit State:
 *	The caller holds no FSPIN_LOCK.
 */

int
mem_instr_resv_wait(uint_t npages, mresvtyp_t type, boolean_t cansig,
		    int line, char *file)
{
	int ret;

	ret = mem_resv_wait(npages, type, cansig);
	if (mem_resv_stats_enabled) {
		mem_stats_instr(npages, type, ret, line, file);
	}
	return ret;
}

#endif /* _MEM_RESV_STATS */
