/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:irq.h	1.2"
/*******************************************************************************
 *******************************************************************************
 *
 *	IRQ.H
 *
 *	Interrupt Trap Handler Header File
 *
 *	History :
 *		08/08/92 Kurt Mahan	Started
 *		07/27/93 Kurt Mahan	Cleaned up
 *
 *******************************************************************************
 ******************************************************************************/

#ifndef	_IO_IRQ_H
#define	_IO_IRQ_H

/*
 *	Minor Numbers 
 */

#define	IRQ_IOC		('I' << 8)		/* ioctl base */
#define	IRQ_IOC_IN	(IRQ_IOC | 0x01)	/* in instruction */
#define	IRQ_IOC_OUT	(IRQ_IOC | 0x02)	/* out instruction */
#define	IRQ_IOC_LIST	(IRQ_IOC | 0x03)	/* out list instruction */
#define	IRQ_IOC_ENABLE	(IRQ_IOC | 0x04)	/* enable a list of irqs */
#define	IRQ_IOC_WRRAW	(IRQ_IOC | 0x05)	/* write to a raw mem addr */
#define	IRQ_IOC_RDRAW	(IRQ_IOC | 0x06)	/* read from a raw mem addr */

/*
 *	Interrupt Flags
 */

#define	IRQ_TRAP	0x00		/* trap and send a message */
#define	IRQ_OPEN 	0x01		/* this irq is open somewhere */
#define	IRQ_SIG 	0x02		/* send a signal on interrupt */

/*
 *	Size defines
 */

#define	IO_BYTE		0x00		/* byte move */
#define	IO_WORD		0x01		/* word move */
#define	IO_LONG		0x02		/* long move */

/*******************************************************************************
 *
 *	IN/OUT IOCTL STRUCTS
 *
 ******************************************************************************/

typedef struct irqiolist {
	ushort_t	addr;		/* address to out to */
	ushort_t	data;		/* data to out */
} irqiolist_t;

typedef struct irqioinfo {
	ushort_t	size;		/* size of move */
	ushort_t	addr;		/* address to set */
	ulong_t		rawaddr;	/* raw addr to r/w */
	ulong_t		data;		/* data (in/out) */
	irqiolist_t	*list;		/* list ptr */
	int		items;		/* items in the list */
} irqioinfo_t;

/*
 *	IRQ Information struct
 */

typedef struct irqconfig {

	/*
	 *	filled in by space.c/config.h
	 */

	int		vect;		/* irq vector */
	int		flags;		/* flags */

	/*
	 *	signal support stuff
	 */
	
	proc_t		*proc;		/* process ptr to send signal to */
} irqconfig_t;

/*******************************************************************************
 *
 *	RANDOM DEFS
 *
 ******************************************************************************/

#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif

#endif	/* _IO_IRQ_H */
