/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mergeipx:mpip_usr.h	1.2"

/****************************************************************************

	Copyright (c) 1992 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

****************************************************************************/

#define MPIP_INIT	0x30	/* Ioctl to initialize the mpip device */
				/* It did not seem appropriate to choose 1 */
				/* as the actual numeric value since a lot of */
				/* other ioctls have the same value. */
				/* Hopefully, this reduces the probability of */
				/* conflict. */

typedef struct {
	long vm86pid;			/* The process id of the DOS task */
	unsigned short ioBasePort;	/* Starting port # */
	unsigned char irqNum;		/* IRQ number */
} mpipInitT;
