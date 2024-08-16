/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)stand:i386sym/standalone/sys/slic_mIntr.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/slic.h>
#include <sys/SGSproc.h>
#include <sys/saio.h>

extern void slicerror(int, unchar, unchar, unchar, unchar);

/*
 * int
 * mIntr(unchar, unchar, unchar)
 * 	transmit a maskable interrupt over the SLIC to the
 *	specified SLIC destination.
 *
 * Calling/Exit State:
 *	PHYS_SLIC is the address of this processor's SLIC registers.
 *
 *	"destination" identifies the SLIC i.d. of the intended
 *	recipient(s) of this command.
 *
 *	"intr" is the interrupt priority/task level for the command.
 *
 *	"msg" is an 8-bit data value which will be transmitted to
 *	the recipient.  Usually its and interrupt vector or sub-task
 *	identifier.
 *
 *	Returns zero if the message was successfully sent; non-zero
 *	if it failed.
 *
 * Description:
 *	Arbitrate for the SLIC bus for the right to transmit
 *	the message described by the arguments.
 *	Then program the SLIC with an maskable intr comand
 *	to the appropriate interrupt level and destination.
 *	If a parity-error failure is detected, then retry the
 *	command a limited number of times.  If a failure occurs
 *	because the intended recipient does not exist, then
 *	return an error indicating that.  In either case, log
 * 	these failures.
 *
 *	Otherwise, reset the command status value and poll the 
 *	slic status register again to determine if the transmission 
 *	has been accepted; in essense, a retry.  Sometimes we loose
 *	the arbitration when another device sends a SLIC message
 *	at about the same time, in which case the command is retried.
 */
int
mIntr(unchar destination, unchar intr, unchar message)
{
	volatile struct cpuslic *sl = (struct cpuslic *)PHYS_SLIC;
	int error = 0;
	int retry = 4;
	unsigned char stat;

	sl->sl_dest = destination;
	sl->sl_smessage = message;
	do {
		sl->sl_cmd_stat = SL_MINTR | intr;
		while ((stat = sl->sl_cmd_stat) & SL_BUSY)
			continue;
		if ((stat & SL_GOOD) == SL_GOOD) {
			return(0);
		}
		if ((stat & SL_PARITY) == 0) {
			slicerror(SL_PERROR, SL_MINTR|intr, 
					destination, message, stat);
			if (--retry)
				continue;
			error = SL_BAD_PAR;
		}
		if ((stat & SL_EXISTS) == 0) {
			slicerror(SL_DERROR, SL_MINTR|intr, 
					destination, message, stat);
			error += SL_BAD_DEST;
		}
	} while ((stat & SL_OK) == 0);
	return(error);
}
