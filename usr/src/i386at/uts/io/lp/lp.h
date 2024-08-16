/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_LP_LP_H	/* wrapper symbol for kernel use */
#define _IO_LP_LP_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/lp/lp.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define SPL()		splstr()/* protection from interrupts */

/* status port */
#define	UNBUSY		0x80
#define	READY		0x40	/* (~ready) prt ready for next char when 
				   this bit is 0 */
#define	NOPAPER		0x20
#define	ONLINE		0x10
#define	ERROR		0x08	/* (~error) error when bit 0 */

/* control port */
#define	INTR_ON		0x10	/* execute IRQ when ~NOT_READY */
#define	SEL		0x08	/* turn printer online (select input) */
#define	RESET		0x04	/* (~reset) initiate reset when this bit is 0 */
#define	AUTOLF		0x02
#define	STROBE		0x01	/* (~strobe) data transfer to data line D0 
				   thru D7 when this bit is 0 */

/* States: */
#define OPEN	0x01
#define LPPRES	0x10    /* set if parallel adapter present */


/*
 * The status register is checked during lpopen for the following conditions:
 *
 *   bit 7 = 1  Printer not busy
 *       6 = 1  Normally high; pulsed low by printer for ACK
 *       5 = 0  Paper out if 1
 *       4 = 1  Device selected
 *       3 = 0  No error present
 *       2 = x  pending irq (extended mode only)
 *       1 = x  Don't care
 *       0 = x  Don't care
 */
#define LP_OK           0xD8    /* Printer connected, powered up, & ready */
#define LP_STATUSMASK   0xF8    /* Eliminate unwanted status bits */

#define	LP_NOCONNECT	0x7f 		/* Printer not connected  */
#define	LP_NOPOWER	0x87 		/* Printer not powered on  */
#define	LP_NOTREADY	0x47 		/* Printer not ready  */


/* defines for mca bus */
#define PORT_ENAB	0x94
#define SYS_IO		0x102


/*
 * Structures for the LP 
 * ____________________________
 *
 *
 */

/* the 8 bits in data address port represent the data (D7 - D0) signal lines */

struct lpcfg{
	int		flag;		/* lp is configured in */
	unsigned	data;		/* data latch address */
	unsigned	status;		/* printer status address */
	unsigned	control;	/* printer controls address */
	unsigned	vect;		/* printer controls address */
};

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_LP_LP_H */
