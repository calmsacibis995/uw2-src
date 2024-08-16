/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_UTIL_IPL_H	/* wrapper symbol for kernel use */
#define	_UTIL_IPL_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:util/ipl.h	1.24"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Interrupt specific information.
 */

#define	PROC_GROUP	1		/* SLIC group used by processors */
#define	SLICBINS	8		/* 8 bins in current SLIC */
#define	MSGSPERBIN	256		/* Interrupt vectors per bin */

/*
 * Define for collecting interrupt stats. MAX_INTR_LEVELS must be set to the 
 * number of bins supported.
 */
#define MAX_INTR_LEVELS	SLICBINS

/*
 * Since we drop the ipl and schedule timeouts in the clock interrupt handler
 * we can nest interrupts more than the number of distinct levels
 * that are supported.
 */
#define MAX_INTR_NESTING	50

/*
 * PL masks for SLIC.
 */
#define PL0	0xFF			/* all interrupts enabled */
#define	PL1	0xFE			/* bins 1-7 enabled, 0 disabled */
#define	PL2	0xFC			/* bins 2-7 enabled, 0-1 disabled */
#define	PL3	0xF8			/* bins 3-7 enabled, 0-2 disabled */
#define	PL4	0xF0			/* bins 4-7 enabled, 0-3 disabled */
#define	PL5	0xE0			/* bins 5-7 enabled, 0-4 disabled */
#define	PL6	0xC0			/* bins 6-7 enabled, 0-5 disabled */
#define	PL7	0x80			/* bin    7 enabled, 0-6 disabled */
#define	PL8	0x00			/* all interrupts disabled */
#define	INVPL	((pl_t)-1)		/* impossible PL value return value */

#ifdef UNIPROC
#define PLMIN	PL0			/* minimum ipl for acquiring locks */
#else
#define PLMIN	PL1			/* minimum ipl for acquiring locks */
#endif

#define PLMAX	PL8			/* all interrupts disabled */

/*
 * Misc defines for compatibility
 */

#define	PLVM	PL6			/* 4.0 specific */
#define	PLTTY	PL6

#define	PLBASE	PL0			/* Block no interrupts */
#define	PLTIMEOUT PL1			/* Block functions scheduled by
					   itimeout and dtimeout */
#define	PLDISK	PL6			/* Block disk device interrupts */
#define	PLSTR	PLTTY			/* Block STREAMS interrupts */
#define	PLHI	PL7			/* Block all device interrupts */
#define PLXCALL	PL8			/* Block all interrupts */


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Minimum sized cell necessary to hold one of the PL? types above
 * for this machine.  Used in synchronization statistics buffers to
 * minimize space.
 */
typedef char mpl_t;

/*
 * Bin_header structure used for bin 1-6 interrupts.
 * We allocate one for bin0 for convenience, although it isn't used.
 *
 * locore.s assumes this data-structure is 8-bytes big.  If this
 * changes, *MUST* change locore.s (dev_common handler).
 */

struct bin_header {
	int	bh_size;		/* # entries */
	void	(**bh_hdlrtab)();	/* real interrupt handlers */
};

/*
 * Typedefs for collecting information on interrupt handlers.
 */

typedef struct {
	unsigned long	intr_count;	/* # of interrupts for this bin/ipl */
	unsigned long	intr_start;	/* Start time for the handler */
	unsigned long	intr_mx;	/* max time for which the handler ran */
	void	*intr_handler;	/* handler address */
	void	*intr_retpc;	/* pc to which the handler will return */
	void	*intr_mxhandler;	/* handler that ran for max time */
	void	*intr_mxretpc;	/* pc to which the handler (mx) returns */
} intr_stats_t;

typedef struct {
	int	intr_top;	/* the current top of stack */
	int	intr_stack[MAX_INTR_NESTING]; /* stack to maintain bin # */
} intr_stack_t;

#endif /* _KERNEL || _KMEMUSER */


#if defined(_KERNEL)

/*
 * Definitions for DDI/DKI complaint drivers.
 */
extern pl_t pl0;
extern pl_t pl1;
extern pl_t pl2;
extern pl_t pl3;
extern pl_t pl4;
extern pl_t pl5;
extern pl_t pl6;
extern pl_t pl7;

extern pl_t plbase;
extern pl_t pltimeout;
extern pl_t pldisk;
extern pl_t plstr;
extern pl_t plhi;

/*
 * The following interfaces (ivecres, ivecpeek, and ivecinit) may
 * be used by custom hardware configuration software to ease setting
 * up interrupt handling.
 * 
 * ivecres:	reserve interrupt vector slots.
 * ivecpeek:	peek at the next interrupt vector to be allocated
 * iveninit:	assign an interrupt handler to a vector
 */

#define ivecpeek(bin)		bin_alloc[(bin)]
#define ivecinit(bin, vector, handler) \
	int_bin_table[(bin)].bh_hdlrtab[(vector)] = (handler)

/*
 * NMI Interrupt messages
 */

#define PAUSESELF	0x01

/*
 * Software (Bin 0) Interrupt messages
 */
#define STRSCHED	0x08		/* streams scheduler */
#define	GLOBALSOFTINT	0x10		/* global software interrupt */
#define	LOCALSOFTINT	0x20		/* local software interrupt */
#define	KPNUDGE		0x40		/* kernel-mode reschedule */
#define	NUDGE		0x80		/* user-mode reschedule */

extern	struct	bin_header int_bin_table[];
extern	int	bin_alloc[];

/*
 * Send cross-processor interrupt to nudge another engine, or
 * to ourself, to handle low-priority events.  Must be called with all
 * interrupts disabled (via DISABLE or FSPIN_LOCK).
 */ 
#define	sendsoft(eng, val)	slic_sendsoft((eng)->e_slicaddr, (val))

extern void globalsoftint(void);
extern void localsoftint(void);

extern void slic_sendsoft(int, int);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_IPL_H */
