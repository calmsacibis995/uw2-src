/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/asy/asyhp/asyhp.h	1.11"
#ident	"$Header: $"

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_ASY_ASYHP_ASYHP_H	/* wrapper symbol for kernel use */
#define _IO_ASY_ASYHP_ASYHP_H	/* subject to change without notice */


#if defined(__cplusplus)
extern "C" {
#endif

/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef MERGE386
#ifdef _KERNEL_HEADERS
#include <util/merge/merge386.h>
#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/merge386.h>
#endif
#endif


/*
 * Definitions for INS8250 / 16550  chips
 */

/*
 * defined as offsets from the data register 
 */
#define	DAT		0	/* receive/transmit data */
#define	ICR		1	/* interrupt control register */
#define ISR		2	/* interrupt status register */
#define	LCR		3	/* line control register */
#define	MCR		4	/* modem control register */
#define	LSR		5	/* line status register */
#define	MSR		6	/* modem status register */
#define	DLL		0	/* divisor latch (lsb) */
#define	DLH		1	/* divisor latch (msb) */

/*
 * INTEL 8210-A/B & 16450/16550 Registers Structure.
 */

/*
 * Line Control Register 
 */
#define	LCR_WLS0	0x01	/*word length select bit 0 */	
#define	LCR_WLS1	0x02	/*word length select bit 2 */	
#define	LCR_STB		0x04	/* number of stop bits */
#define	LCR_PEN		0x08	/* parity enable */
#define	LCR_EPS		0x10	/* even parity select */
#define	LCR_SETBREAK	0x40	/* break key */
#define	LCR_DLAB	0x80	/* divisor latch access bit */
#define	LCR_RXLEN	0x03    /* # of data bits per received/xmitted character */
#define	LCR_STOP1	0x00
#define	LCR_STOP2	0x04
#define	LCR_PAREN	0x08
#define	LCR_PAREVN	0x10
#define	LCR_PARMARK	0x20
#define	LCR_SNDBRK	0x40
#define	LCR_DLAB	0x80
#define	LCR_BITS5	0x00	/* 5 bits per char */
#define	LCR_BITS6	0x01	/* 6 bits per char */
#define	LCR_BITS7	0x02	/* 7 bits per char */
#define	LCR_BITS8	0x03	/* 8 bits per char */

/*
 * Line Status Register 
 */
#define	LSR_RCA		0x01	/* data ready */
#define	LSR_OVRRUN	0x02	/* overrun error */
#define	LSR_PARERR	0x04	/* parity error */
#define	LSR_FRMERR	0x08	/* framing error */
#define	LSR_BRKDET 	0x10	/* a break has arrived */
#define	LSR_XHRE	0x20	/* tx hold reg is now empty */
#define	LSR_XSRE	0x40	/* tx shift reg is now empty */
#define	LSR_RFBE	0x80	/* rx FIFO Buffer error */

/*
 * Interrupt Status Register 
 */
#define	ISR_MSTATUS	0x00	/* RS-232 line interrupt */
#define	ISR_NOINTRPEND	0x01	/* Interrupt pending bit */
#define	ISR_TxRDY	0x02	/* Transmitter interrupt */
#define	ISR_RxRDY	0x04	/* Receiver ready interrupt */
#define	ISR_ERROR_INTR	0x08	/* Error interrupt */
#define	ISR_FFTMOUT	0x0c	/* FIFO Timeout */
#define	ISR_RSTATUS	0x06	/* Receiver Line status */

/*
 * Interrupt Enable Register 
 */
#define	ICR_RIEN	0x01	/* Received Data Ready */
#define	ICR_TIEN	0x02	/* Tx Hold Register Empty */
#define	ICR_SIEN	0x04	/* Receiver Line Status */
#define	ICR_MIEN	0x08	/* Modem Status */

/*
 * Modem Control Register 
 */
#define	MCR_DTR		0x01	/* Data Terminal Ready */
#define	MCR_RTS		0x02	/* Request To Send */
#define	MCR_OUT1	0x04	/* Aux output - not used */
#define	MCR_OUT2	0x08	/* turns intr to 386 on/off */	
#define	MCR_ASY_LOOP	0x10	/* loopback for diagnostics */

/*
 * Modem Status Register 
 */
#define	MSR_DCTS	0x01	/* Delta Clear To Send */
#define	MSR_DDSR	0x02	/* Delta Data Set Ready */
#define	MSR_DRI		0x04	/* Trail Edge Ring Indicator */
#define	MSR_DDCD	0x08	/* Delta Data Carrier Detect */
#define	MSR_CTS		0x10	/* Clear To Send */
#define	MSR_DSR		0x20	/* Data Set Ready */
#define	MSR_RI		0x40	/* Ring Indicator */
#define	MSR_DCD		0x80	/* Data Carrier Detect */

#define DELTAS(x) 	((x)&(MSR_DCTS|MSR_DDSR|MSR_DRI|MSR_DDCD))
#define	STATES(x) 	((x)(MSR_CTS|MSR_DSR|MSR_RI|MSR_DCD))


#define	FIFOEN		0x87	/* fifo enabled, Rx fifo level at 14 */
#define	TRLVL1		0x07	/* 550 fifo enabled, Rx fifo level at 1	*/
#define	TRLVL2		0x47	/* 550 fifo enabled, Rx fifo level at 4	*/
#define	TRLVL3		0x87	/* 550 fifo enabled, Rx fifo level at 8	*/
#define	TRLVL4		0xc7	/* 550 fifo enabled, Rx fifo level at 14 */

/*
 *	Transmit FIFO size
 */
#define	TX_FIFOSIZE	16		/* Transmit fifo buffer size */

/*
 * asyhp_flags definitions 
 */
#define	XBRK		0x01	/* xmitting break in progress */
#define	HWDEV		0x02	/* Hardware device being used */
#define	HWFLWO		0x04	/* H/W Flow ON */
#define	HWFLWS		0x08	/* Start H/W after CSTOP */
/*
#define	CARRF		0x10		
*/ 
#define	ASY16550	0x20	/* 1 - 82510 , 0 - 16550/16551/8250 */
#define	ASY82510	0x40	/* 1 - 82510 , 0 - 16450/16550/8250 */
#define	ASYHERE		0x80	/* adapter is present */
#define ASYHP_SYSCON	0x100	/* device being used as system console */

#define	BRKTIME		HZ/4

/*
 * Serial in/out requests 
 */
#define	SO_DIVLLSB	1
#define	SO_DIVLMSB	2
#define	SO_LCR		3
#define	SO_MCR		4
#define	SI_MSR		1


/* 
 * Misc #defines
 */

#define	MAX_CONPORTS	4	/* Maximum Console ports supported by ASY */
				/* Put UART in loopback mode */
#define	SET_LBKMODE	(MCR_ASY_LOOP|MCR_RTS|MCR_OUT2|MCR_DTR|MCR_OUT1)	
				/* MSR expected value in loopback */
#define	LBK_EXPECTED	(MSR_CTS|MSR_DCD|MSR_RI|MSR_DSR)

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Asychronous configuration Structures 
 */
struct asyhp {
	int		asyhp_flags;
	unsigned	asyhp_dat;
	unsigned	asyhp_icr;
	unsigned	asyhp_isr;
	unsigned	asyhp_lcr;
	unsigned	asyhp_mcr;
	unsigned	asyhp_lsr;
	unsigned	asyhp_msr;
	unsigned	asyhp_vect;
	struct msgb	*asyhprbp;
	minor_t		asyhp_dev;
#ifdef MERGE386
	struct mrg_com_data mrg_data;
#endif /* MERGE386 */
	uchar_t		asyhp_save_icr;
};

#define         ASYHP_CONSOLE_INIT       1
#define         ASYHP_COM_INIT           2

typedef struct asyhp_base {
	ushort_t io_base;
	ushort_t int_vect;
} asyhp_base_t;

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
        }
#endif

#endif /* _IO_ASY_ASYHP_ASYHP_H */
