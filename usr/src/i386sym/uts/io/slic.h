/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_SLIC_H	/* wrapper symbol for kernel use */
#define	_IO_SLIC_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/slic.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	NUMGATES	64		/* number of slic gates */

/*
 * MAX_NUM_SLIC is the maximum number of different slic addresses possible.
 * Slic addresses are 0 thru MAX_NUM_SLIC-1.
 */
#define	MAX_NUM_SLIC	64

/* Commands: */
#define	SL_MINTR	0x10	/* transmit maskable interrupt */
#define	SL_INTRACK	0x20	/* interrupt acknowledge */
#define	SL_SETGM	0x30	/* set group interrupt mask */
#define	SL_REQG		0x40	/* request Gate */
#define	SL_RELG		0x50	/* release Gate */
#define	SL_NMINTR	0x60	/* transmit non-maskable interrupt */
#define	SL_RDDATA	0x70	/* read slave data */
#define	SL_WRDATA	0x80	/* write slave data */
#define	SL_WRADDR	0x90	/* write slave I/O address */

/* Returned command status: */
#define	SL_BUSY		0x80	/* SLIC busy */
#define	SL_GATEFREE	0x10	/* Gate[send_message_data] free */
#define	SL_WRBE		0x08	/* Processor write buffer empty */
#define	SL_PARITY	0x04	/* parity error during SLIC message */
#define	SL_EXISTS	0x02	/* destination SLIC's exist */
#define	SL_OK		0x01	/* command completed ok */

/* Destination id's */
#define	SL_GROUP	0x40
#define	SL_ALL		0x3F

/* Interrupt control */
#define	SL_HARDINT	0x80	/* hardware interrupts accepted */
#define	SL_SOFTINT	0x40	/* software interrupts accepted */
#define	SL_MODEACK	0x01	/* interrupt acknowledge mode */

#define SL_GM_ALLOFF	0x0	/* Group Mask all disabled */
#define	SL_GM_ALLON	0xFF	/* Group Mask all enabled */

/* Timer interrupts */
#define	SL_TIMERINT	0x80	/* enable timer interrupts */
#define	SL_TIM5MHZ	0x08	/* decrement timer at 5 MHz */
#define	SL_TIMERBIN	0x07	/* interrupt bin mask of timer */
#define	SL_TIMERFREQ	10000	/* counts per second */
#define	SL_TIMERDIV	1000	/* system clock divisor for one clock count */

/* Processor ID */
#define	SL_TESTM	0x80	/* enable test mode */
#define	SL_PROCID	0x3F	/* processor ID mask */

/* Chip version stuff */
#define	SL_VENDOR	0xE0	/* vendor number */
#define	SL_RELEASE	0x1C	/* release number */
#define	SL_STEPPING	0x03	/* step number */

/*
 * This structure represents the SLIC hardware.
 */

struct	cpuslic {
	unchar	sl_cmd_stat,	d0[3];	/* RW W: command, R: status */
	unchar	sl_dest,	d1[3];	/* W */
	unchar	sl_smessage,	d2[3];	/* W   send message data */
	unchar	sl_b0int,	d3[3];	/* R   bin 0 interrupt */
	unchar	sl_binint,	d4[3];	/* RW  bin 1-7 interrupt */
	unchar	sl_nmiint,	d5[3];	/* R   NMI interrupt */
	unchar	sl_lmask,	d6[3];	/* RW  local interrupt mask */
	unchar	sl_gmask,	d7[3];	/* R   group interrupt mask */
	unchar	sl_ipl,		d8[3];	/* RW  interrupt priority level */
	unchar	sl_ictl,	d9[3];	/* RW  interrupt control */
	unchar	sl_tcont,	d10[3];	/* RW  timer contents */
	unchar	sl_trv,		d11[3];	/* RW  timer reload value */
	unchar	sl_tctl,	d12[3];	/* W   timer control */
	unchar	sl_sdr,		d13[3];	/* R   slave data register */
	unchar	sl_procgrp,	d14[3];	/* RW  processor group */
	unchar	sl_procid,	d15[3];	/* RW  processor id */
	unchar	sl_crl,		d16[3];	/* R   chip revision level */
};

#if defined(_KERNEL)

/* public function declarations */
extern void slic_init(void);
extern int slic_rdslave(int, int);
extern int slic_lrdslave(int, int);
extern void slic_wrslave(int, int, int);
extern void slic_lwrslave(int, int, int);
extern void slic_wrAddr(int, int);
extern void slic_wrSubslave(int, int, int, int);
extern int slic_rdSubslave(int, int, int);
extern void slic_sendsoft(int, int);
extern void slic_nmIntr(int, int);
extern void slic_mIntr(int, int, int);
extern void slic_setgm(int, int);
extern void slic_priinit(int);
extern void startrtclock(void);
extern void slic_flush_intr(void);

extern int slic_prishift;		/* used by priority mapping */

/*
 * Takes a global priority and turns it into the equivalent slic priority.
 */
#define	SLICPRI(p)		((struct cpuslic *)KVSLIC)->sl_ipl = \
				 (31-((p) >> slic_prishift) << 2)

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_SLIC_H */
