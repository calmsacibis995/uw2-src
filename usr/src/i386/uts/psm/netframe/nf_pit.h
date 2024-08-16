/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/nf_pit.h	1.1"
#ident	"$Header: $"

#ifndef _SVC_PIT_H	/* wrapper symbol for kernel use */
#define _SVC_PIT_H	/* subject to change without notice */

/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */

/* Definitions for 8254 Programmable Interrupt Timer ports on AT 386 */
#define	PITCTR0_PORT	0x1024		/* counter 0 port */	
#define	PITCTR1_PORT	0x1024		/* counter 1 port NOT USED */	
#define	PITCTR2_PORT	0x1028		/* counter 2 port NOT USED */	
#define	PITCTL_PORT	0x102C		/* PIT control port */
#define	PITAUX_PORT	0x61		/* PIT auxiliary port */
#define SANITY_CTR0	0x48		/* sanity timer counter */
#define SANITY_CTL	0x4B		/* sanity control word */
#define SANITY_CHECK	0x461		/* bit 7 set if sanity timer went off*/
#define FAILSAFE_NMI	0x80		/* to test if sanity timer went off */
#define ENABLE_SANITY	0x04		/* Enables sanity clock NMI ints */
#define RESET_SANITY	0x00		/* resets sanity NMI interrupt */

/* Definitions for 8254 commands */

/* Following are used for Timer 0 */
#define PIT_C0          0x00            /* select counter 0 */
#define	PIT_LOADMODE	0x30		/* load least significant byte followed
					 * by most significant byte */
#define PIT_NDIVMODE	0x04		/*divide by N counter */
#define	PIT_SQUAREMODE	0x06		/* square-wave mode */
#define	PIT_ENDSIGMODE	0x00		/* assert OUT at end-of-count mode*/

/* Used for Timer 1. Used for delay calculations in countdown mode */
#define PIT_C1          0x40            /* select counter 1 */
#define	PIT_READMODE	0x30		/* read or load least significant byte
					 * followed by most significant byte */
#define	PIT_RATEMODE	0x06		/* square-wave mode for USART */

#define	CLKNUM	(1000000/HZ)		/* NF has a 1Mhz crystal	*/
					/* divided by HZ gives number	*/
					/* of ticks for the timer to	*/
					/* count for 10 milliseconds	*/
					/* ie. set timer to interrupt	*/
					/* every 10 mS			*/
#define SANITY_NUM	0xFFFF		/* Sanity timer goes off every .2 secs*/
/* bits used in auxiliary control port for timer 2 */
#define	PITAUX_GATE2	0x01		/* aux port, PIT gate 2 input */
#define	PITAUX_OUT2	0x02		/* aux port, PIT clock out 2 enable */

#endif /* _SVC_PIT_H */
