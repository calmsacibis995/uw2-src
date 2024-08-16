/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/mem.cf/Space.c	1.13"

#include <config.h>
#include <sys/tuneable.h>
#include <sys/param.h>
#include <sys/types.h>

/*
 * Kernel segment driver aging control parameters.
 */
clock_t segmap_age_time = SEGMAP_AGE_TIME * HZ;
clock_t segkvn_age_time = SEGKVN_AGE_TIME * HZ;
ulong_t segmap_agings = SEGMAP_AGINGS;


struct tune tune = {
	GPGSLO,
	FDFLUSHR,
	MINAMEM,
	KMEM_RESV,
	FLCKREC,
	MAXDMAPAGE,
	0,	/* t_dma_base */
	0,	/* t_devnondma */
	LGDMA_RATIO,
	DMA_PERCENT,
};

uint_t pages_pp_maximum = PAGES_UNLOCK;
uint_t pages_dkma_maximum = PAGES_NODISKMA;
int scale_maxpgio = SCALE_MAXPGIO;
u_int deficit_age = DEFICIT_AGE;
int io_weight = IO_WEIGHT;
int cpu_weight = CPU_WEIGHT;
int swap_weight = SWAP_WEIGHT;
int sleep_weight = SLEEP_WEIGHT;
int maxslp = MAXSLP;
int swap_maxdev = SWAP_MAXDEV;

/*
 * Miscellaneous Aging Parameters
 */

/* Elapsed time aging: interval under memory stress */
clock_t	et_age_interval_tune = ET_AGE_INTERVAL * HZ;

/* Maximum permitted value for short term deficit due to swapins */
int	max_deficit = MAX_DEFICIT;

/* Minimum number of nonlocked pages a process must have, for getting aged */
size_t	nonlocked_minpg = NONLOCKED_MINPG;

size_t	maxrss_tune = MAXRSS;	/* Maximum RSS */

/* The aging quanta defined below are in units of clock ticks */
clock_t	init_agequantum_tune = INIT_AGEQUANTUM;	/* Initial Aging Quantum */
clock_t	min_agequantum_tune = MIN_AGEQUANTUM;	/* Minimum Aging Quantum */
clock_t	max_agequantum_tune = MAX_AGEQUANTUM;	/* Maximum Aging Quantum */

boolean_t aging_tune_priv = AGING_TUNE_PRIV;

/*
 * Threshold RSS growth rates (in units of pages over RSS sampling period)
 * for performing growth rate based short term aging quantum adjustment.
 */
int	lo_grow_rate = LO_GROW_RATE;
int	hi_grow_rate = HI_GROW_RATE;

/*
 * The following are kernel configuration parameters to request
 * the size of the kernel virtual space managed by each of the
 * kernel segment managers.
 *
 * See carve_kvspace() for a discussion of how these are used.
 */

ulong_t segkmem_bytes = SEGKMEM_BYTES;
ulong_t segkmem_percent = SEGKMEM_PERCENT;
ulong_t segmap_bytes = SEGMAP_BYTES;
ulong_t segmap_percent = SEGMAP_PERCENT;
ulong_t segkvn_bytes = SEGKVN_BYTES;
ulong_t segkvn_percent = SEGKVN_PERCENT;
