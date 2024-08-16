/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_AS_H	/* wrapper symbol for kernel use */
#define _MEM_AS_H	/* subject to change without notice */

#ident	"@(#)kern:mem/as.h	1.45"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifdef _KERNEL_HEADERS

#include <mem/faultcode.h> /* REQUIRED */
#include <mem/seg.h> /* REQUIRED */
#include <mem/vm_hat.h> /* REQUIRED */
#include <util/ksynch.h> /* REQUIRED */
#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/faultcode.h> /* REQUIRED */
#include <vm/seg.h> /* REQUIRED */
#include <vm/vm_hat.h> /* REQUIRED */
#include <sys/ksynch.h> /* REQUIRED */
#include <sys/types.h>	/* REQUIRED */

#else

#include <vm/vm_hat.h>	/* SVR4.0COMPAT */
#include <vm/faultcode.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */


/*
 * VM - Address spaces.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Structure for holding the address space working set information
 * for use in aging decisions.
 * Members of the structure are protected against concurrent updates
 * as explained below:
 *
 *	whenaged:	Set by the aging LWP at aging time after process 
 *			is seized. Therefore no protection is required.
 *	ageticks:	Protected by p_mutex when incremented
 *			per call to local clock handler. Also its setting
 *			to 0 after an aging step is protected effectively
 *			by process seizing.
 *	age_quantum:	Initialized at process creation or exec time; therefore
 *			effectively protected. Subsequent updates will need
 *			to be mutexed by p_mutex.
 *	maxrss,
 *	et_age_interval,
 *	init_agequantum,
 *	min_agequantum,
 *	max_agequantum: All these parameters can be modified by only two
 *			agents: process creation initialization which is
 *			effectively protected and by priocntl() which
 *			holds the p_mutex. The readers of these values
 *			do not neccessarily hold the p_mutex but any race
 *			conditions between read/write (even if these operations
 *			are not atomic), are benign.
 */
typedef	struct wkset {
	clock_t	 whenaged;	/* lbolt at the time of last aging step */
	ushort_t ageticks;	/* ticks collected since birth/exec/swapin */
				/* as_age */
	ushort_t agequantum;	/* interval between aging steps, measured */
				/* by ageticks */
	size_t	 prevrss;	/* used to compute the rate of RSS growth */
				/* over (virtual) time; in PAGESIZE units */
	size_t	 wss;		/* working set size */
	size_t	 maxrss;	/* maximum rss before trimming occurs */
	clock_t	 et_age_interval;	/* aging interval */
	clock_t  init_agequantum;
	clock_t  min_agequantum;
	clock_t  max_agequantum;
} wkset_t;


/*
 * Each address space consists of a list of sorted segments
 * and machine dependent address translation information.
 *
 * All the hard work is in the segment drivers and the
 * hardware address translation code.
 *
 * The a_isize field denotes the instantiated size of the address space.
 * This is used only by /proc to display the actual size of the process.
 * The need for this arose from the fact the stack segment is pre-mapped
 * and thus shows processes as having an artificially huge address space.
 * This size is maintained by the segment managers and in all segment
 * drivers except segdz, the value is the whole size of the segment.
 * segdz upates this field as pages in its segment gets instantiated.
 */
struct as {
	uint_t	a_paglck : 1;
	struct seg *a_segs;	/* segments in this address space */
	struct seg *a_seglast;	/* last segment hit on the address space */
	size_t	a_size;		/* size of address space */
	size_t	a_isize;	/* instantiated size of address space */
	size_t	a_rss;		/* # PAGESIZE pages currently loaded */
	size_t  a_lockedrss;    /* # PAGESIZE pages locked in memory */
	vaddr_t	a_trimnext;	/* next address at which to start RSS trimming*/
	wkset_t	a_wkset;	/* working set and aging information */
	struct hat a_hat;	/* hardware address translation */
	rwsleep_t a_rwslplck;	/* read-write sleep lock for AS */
};

#define	a_ageticks		a_wkset.ageticks
#define a_agequantum		a_wkset.agequantum
#define a_prevrss		a_wkset.prevrss
#define a_whenaged		a_wkset.whenaged
#define a_wss			a_wkset.wss
#define a_maxrss		a_wkset.maxrss
#define a_et_age_interval	a_wkset.et_age_interval
#define a_init_agequantum 	a_wkset.init_agequantum
#define a_min_agequantum	a_wkset.min_agequantum
#define a_max_agequantum	a_wkset.max_agequantum

#define rm_assize( x )	( (x) == (struct as *)NULL ? 0 : (x)->a_size )

/* Opaque data type used with as_prmapin()/as_prmapout(). */
typedef struct page *as_prmapcookie_t;

extern int rss_trim_factor;

/*
 * boolean_t
 * IS_TRIM_NEEDED(asp)
 *      Return B_TRUE if the rss exceeds a threshold and memory availability
 *      is low, suggesting that address space trimming is needed.
 */
#define IS_TRIM_NEEDED(asp) ((asp)->a_maxrss < \
	(((asp)->a_rss - (asp)->a_lockedrss) * rss_trim_factor))

/*
 * Macro for resetting aging timers for all aging cases
 */
#define	RESET_AGING_INFO(asp, agequantum) { \
	(asp)->a_whenaged = lbolt; \
	(asp)->a_ageticks = 0; \
	(asp)->a_agequantum = (ushort_t)(agequantum); \
	(asp)->a_prevrss = (asp)->a_rss; \
}

extern	int	max_deficit;
extern	size_t	nonlocked_minpg;
extern	clock_t	init_agequantum;
extern	clock_t	min_agequantum;
extern	clock_t	max_agequantum;
extern	int	lo_grow_rate;
extern	int	hi_grow_rate;

#endif	/* _KERNEL || _KMEMUSER */

#ifdef _KERNEL


/*
 * AGE QUANTUM ADAPTATION:
 *
 *	The following description applies to how aging quanta are adjusted
 *	with variation of address space RSS. Growth/Decay of RSS is tracked
 *	over intervals of RSS_SAMPLE_TIME (typically 5 process ticks). The
 *	instant when an address space is evaluated for growth/contraction
 *	of its aging quantum may be called an adaptation point.
 *
 *	A high rate of RSS growth over the RSS_SAMPLE_TIME interval increases
 *	the aging quantum, in order to accomodate expansion of working set.
 *	A very low rate of RSS growth indicates stabilization of working set,
 *	and therefore results in a lower aging quantum. The changes in the
 *	aging quantum are subject to minimum and maximum limits for the quantum.
 *	
 *	In order to increase the likelihood of short running process exiting
 *	without undergoing an aging step, they are given an initial aging
 *	quantum that is large enough to avoid quantum expiration.
 *
 *	The quantum adaptation is done at the time of entering the local
 *	clock handler.
 *
 *	The unit by which quantum is reduced should be higher than that at
 *	which it is raised, in order to promote quicker aging of processes
 *	that stabilize, and in order to account for the fact that a process
 *	encounters more adaptation points with a larger quantum.
 *
 *	The actual window over which RSS growth is computed, in effect,
 *	varies from 1 to 2 RSS_SAMPLE_TIMEs. This is because of the following
 *	implementation choice: in order to provide some hysteresis following
 *	a period over which process RSS growth may have accelerated or
 *	decelerated, the previous RSS is set to an average of the previous
 *	and the current RSS at the time that RSS growth is sampled. This may
 *	be taken into consideration when selecting high and low thresholds
 *	for RSS growth.
 */

#define	RSS_SAMPLE_TIME		5	/* interval between successive age */
					/* quantum adaptation steps, when  */
					/* process RSS growth is sampled.  */
					/* in process ticks. */
#define QUANTUM_INCREMENT	(RSS_SAMPLE_TIME/2)
#define QUANTUM_DECREMENT	QUANTUM_INCREMENT+1


/*
 * The following defines are for the hitithard flag with which hat_agerange
 * is called. In all cases, hat_agerange will remove only those translations
 * that are not locked down. If the flag is AS_AGE, then only the unreferenced
 * translations will be removed. Both AS_SWAP and AS_TRIM, on the other hand,
 * will remove referenced translations as well. The difference between AS_SWAP
 * and AS_TRIM is that AS_TRIM causes removal of a limited number (less than
 * or equal to MAXTRIM) translations.
 */
enum age_type {
	AS_AGE,		/* aging through time quantum */
	AS_SWAP,	/* aging for swapping */
	AS_TRIM		/* aging based on resident set size */
};

/*
 * The following define is used in hat_agerange() to forcibly free up
 * MAXTRIM virtual translations.
 */
#define MAXTRIM 5

/*
 * Flags for as_gap.
 */
#define AH_DIR		0x1	/* direction flag mask */
#define AH_LO		0x0	/* find lowest hole */
#define AH_HI		0x1	/* find highest hole */
#define AH_CONTAIN	0x2	/* hole must contain `addr' */
#define	AH_KERNEL	0x4	/* a kernel address space is argument */
 
struct page;
struct vnode;

struct	seg *as_segat(struct as *as , vaddr_t addr);
struct	as *as_alloc(void);
void	as_free(struct as *as);
struct	as *as_dup(struct as *as);
void	as_childload(struct as *pas, struct as *cas);
int	as_addseg(struct as *as, struct seg *seg);
faultcode_t as_fault(struct as *as, vaddr_t addr, u_int size, 
		     enum fault_type, enum seg_rw rw);
int	as_setprot(struct as *as, vaddr_t addr, u_int size, u_int prot);
int	as_checkprot(struct as *as, vaddr_t addr, u_int size, u_int prot);
int	as_unmap(struct as *as, vaddr_t addr, u_int size);
int	as_map(struct as *as, vaddr_t addr, u_int size, 
	       int (*crfp)(), void *argsp);
int	as_gap(struct as *as, u_int minlen, vaddr_t *basep, u_int *lenp,
	       int flags, vaddr_t addr);
int	as_memory(struct as *as, vaddr_t *basep, u_int *lenp);
u_int	as_swapout(struct as *as);
int	as_incore(struct as *as, vaddr_t addr, u_int size, char *vec, 
	          u_int *sizep);
int	as_ctl(struct as *as, vaddr_t addr, u_int size, int func, int attr,
               void *arg);
u_int	as_getprot(struct as *as, vaddr_t addr, vaddr_t *naddr);
int	as_exec(struct as *oas, vaddr_t ostka, u_int stksz, struct as *nas,
		vaddr_t nstka, u_int hatflag);
faultcode_t as_prmapin(struct as *as, vaddr_t uvaddr, enum seg_rw rw,
		       vaddr_t *kvaddrp, as_prmapcookie_t *cookiep);
void	as_prmapout(struct as *as, vaddr_t uvaddr, vaddr_t kvaddr,
		    as_prmapcookie_t cookie);

void	as_age(void);
void	as_ageswap(struct as *as, int type);
struct	proc;
void	as_age_externally_l(struct proc *procp);
boolean_t as_getwchan(vaddr_t addr, uint_t *typep, void **key1, void **key2);
boolean_t as_aio_able(struct as *asp, vaddr_t addr, u_int len);

/*
 * Macros for manipulating the AS lock
 */

#ifndef NO_AS_INLINE

#define as_rdlock(as) \
		RWSLEEP_RDLOCK(&(as)->a_rwslplck, PRIMEM - 1)

#define as_wrlock(as) \
		RWSLEEP_WRLOCK(&(as)->a_rwslplck, PRIMEM - 1)

#define as_unlock(as) \
		RWSLEEP_UNLOCK(&(as)->a_rwslplck)

#define as_tryrdlock(as) \
		RWSLEEP_TRYRDLOCK(&(as)->a_rwslplck)

#define as_trywrlock(as) \
                RWSLEEP_TRYWRLOCK(&(as)->a_rwslplck)

extern struct as kas;

#else /* NO_AS_INLINE */

extern void as_rdlock_func(struct as *);
extern void as_wrlock_func(struct as *);
extern void as_unlock_func(struct as *);

#define as_rdlock	as_rdlock_func
#define as_wrlock	as_wrlock_func
#define as_unlock	as_unlock_func

#endif /* NO_AS_INLINE */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_AS_H */
