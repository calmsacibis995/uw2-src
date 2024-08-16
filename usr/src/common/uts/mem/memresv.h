/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_MEMRESV_H	/* wrapper symbol for kernel use */
#define _MEM_MEMRESV_H	/* subject to change without notice */

#ident	"@(#)kern:mem/memresv.h	1.20"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Types of memory reservations, for mem_resv() and mem_unresv().
 */

typedef enum {
	M_REAL,		/* kernel reservation of locked real memory (rmem) */
	M_REAL_USER,	/* user reservation of locked real memory (rmem) */
	M_SWAP_KERNEL,	/* kenrel reservation for swap */
	M_SWAP,		/* user reservation of swap  */
	M_BOTH,		/* kernel reservation of both rmem and swap */
	M_BOTH_USER,	/* user reservation of both rmem and swap */
	M_KERNEL_ALLOC,	/* kma/kpg reservation of rmem and swap */
	M_DMA,		/* DMA_PAGE real memory reservation */
	M_NONE		/* no reservation */
} mresvtyp_t;

/*
 * boolean_t MEM_RESV_PROMOTE(uint_t npages, mresvtyp_t mtype)
 *	Promote npages from M_REAL/M_BOTH to M_REAL_USER/M_BOTH_USER.
 *	If mtype == M_REAL_USER then promotes from M_REAL to M_REAL_USER.
 *	If mtype == M_BOTH_USER then promotes from M_BOTH to M_BOTH_USER.
 * void MEM_RESV_DEMOTE(uint_t napges)
 *	Demote npages from M_REAL_USER/M_BOTH_USER to M_REAL/M_BOTH.
 */
#ifndef DEBUG

#define MEM_RESV_PROMOTE(npages, mtype)	mem_resv_promote(mtype)
#define MEM_RESV_DEMOTE(npages)		/**/

extern boolean_t mem_resv_promote(mresvtyp_t);

#else /* DEBUG */

#define MEM_RESV_PROMOTE(npages, mtype)	mem_resv_promote(npages, mtype)
#define MEM_RESV_DEMOTE(npages)		mem_resv_demote(npages)

extern boolean_t mem_resv_promote(uint_t, mresvtyp_t);
extern void mem_resv_demote(uint_t);

#endif /* DEBUG */

#ifdef DEBUG
/*
 * for sizing stats arrays; must be last
 */
#define M_TYPE_MAX	M_NONE
#endif

/*  
 * RMEMINFO: real memory accounting structure
 */

typedef struct rmeminfo {
	uint_t	rmi_user_max;	/* maximum real pages available for user */
				/* lockdown */
	uint_t	rmi_dkma_max;	/* maximum real pages available for */
				/* discretionary kma use */
	uint_t	rmi_max;	/* maximum real pages available for normal */
				/* use */
	uint_t	rmi_kma_max;	/* maximum real pages available for kma use */
	uint_t	rmi_resv;	/* number of real memory pages reserved */
} rmeminfo_t;

/*
 * DMAINFO: DMAable memory accounting structure
 *
 *	Reservations for the pool of DMA_PAGE pages in rdma_mode == RDMA_LARGE.
 */
typedef struct dmainfo {
	uint_t	dmi_max;	/* maximum reservations avail */
	uint_t	dmi_resv;	/* number of DMAable memory pages reserved */
} dmainfo_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern rmeminfo_t rmeminfo;
extern dmainfo_t dmainfo;
extern uint_t pages_pp_kernel;
extern uint_t pages_pp_locked;
extern fspin_t vm_memlock;	/* This lock covers anoninfo, rmeminfo,
				   pages_pp_kernel, and pages_pp_locked */

/*
 * instrumented mem_resv functions for statistics gathering
 */
#if defined(_MEM_RESV_STATS) && !defined(_MEM_RESV_C)

#define mem_resv(size, type)	\
	mem_instr_resv(size, type, __LINE__, __FILE__)
#define mem_unresv(size, type) \
	mem_instr_unresv(size, type, __LINE__, __FILE__)
#define mem_resv_wait(size, type, cansig) \
	mem_instr_resv_wait(size, type, cansig, __LINE__, __FILE__)

extern int mem_instr_resv(uint_t, mresvtyp_t, int, char *);
extern void mem_instr_unresv(uint_t, mresvtyp_t, int , char *);
extern int mem_instr_resv_wait(uint_t, mresvtyp_t, boolean_t, int, char *);

#else /* !_MEM_RESV_STATS || _MEM_RESV_C */

extern int mem_resv(uint_t npages, mresvtyp_t typ);
extern void mem_unresv(uint_t npages, mresvtyp_t typ);
extern int mem_resv_wait(uint_t npages, mresvtyp_t typ, boolean_t cansig);

#endif /* _MEM_RESV_STATS && !_MEM_RESV_C */

extern void mem_resv_service(void);
extern boolean_t mem_resv_check(void);

/*
 * boolean_t
 * mem_check(int npages)
 *	 Provide a hint of whether enough rmem and swap is likely to be
 *	 available to satisfy a given M_KERNEL_ALLOC request. This
 *	 information will potentially be stale.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if reservations are likely to be available and
 *	B_FALSE otherwise.
 */
#define mem_check(npages) \
	(rmeminfo.rmi_resv + (npages) <= rmeminfo.rmi_kma_max && \
	 anoninfo.ani_resv + (npages) <= anoninfo.ani_kma_max)

/*
 * boolean_t
 * mem_dma_check(int npages)
 *	 Provide a hint of whether enough DMAable memory is likely
 *	 available to satisfy a given M_DMA request. This
 *	 information will potentially be stale.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if reservations are likely to be available and
 *	B_FALSE otherwise.
 */
#define mem_dma_check(npages)	\
	(dmainfo.dmi_resv + (npages) <= dmainfo.dmi_max)

/*
 * boolean_t
 * mem_disc_check(void)
 *	Check if ``discretionary'' kma reservations have been exhausted for
 *	for either real or virtual swap memory.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if reservations have not been exhuasted and B_FALSE
 *	otherwise.
 *
 * Description:
 *	This is typically used by subsystems which can kmem_alloc
 *	large amounts of memory. A B_FALSE return is an indication that
 *	kmem_alloc()s should be gated off.
 *
 * Note:
 *	Functionality is identical to mem_resv_check(), except that the
 *	vm_memlock is not acquired.
 */
#define mem_disc_check() \
	(rmeminfo.rmi_resv <= rmeminfo.rmi_dkma_max &&	\
	anoninfo.ani_resv <= anoninfo.ani_dkma_max)

#ifdef DEBUG

/*
 * Structure to gather stats on effectivness of memory reservation
 * waiting policy.
 *
 * The array mem_resv_wait_stats is sized using M_TYPE_MAX from
 * the mresvtyp_t enum defined above.
 */

struct mem_resv_wait_stat {
	int	mrw_calls;	/* total number of calls for this type */
	int	mrw_blocked;	/* total number of times blocked */
	int	mrw_spins;	/* total number of times woke up and */
				/* couldn't get memory */
} mem_resv_wait_stats[M_TYPE_MAX];
 
#define MRW_STAT_CALLS(typ) \
		{ mem_resv_wait_stats[(typ)].mrw_calls++; }

#define MRW_STAT_BLOCKED(typ) \
		 { mem_resv_wait_stats[(typ)].mrw_blocked++; }

#define MRW_STAT_SPINS(typ) \
		{ mem_resv_wait_stats[(typ)].mrw_spins++; }

#else	/* DEBUG */

#define MRW_STAT_CALLS(typ) 
#define MRW_STAT_BLOCKED(typ)
#define MRW_STAT_SPINS(typ)

#endif /* DEBUG */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_MEMRESV_H */
