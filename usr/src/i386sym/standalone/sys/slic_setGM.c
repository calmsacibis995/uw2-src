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

#ident	"@(#)stand:i386sym/standalone/sys/slic_setGM.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/slic.h>
#include <sys/SGSproc.h>
#include <sys/saio.h>

extern void slicerror(int, unchar, unchar, unchar, unchar);

/*
 * int 
 * setGM(unchar, unchar)
 * 	Set the group mask value of the specified SLIC controller.
 *
 * Calling/Exit State:
 *      PHYS_SLIC is the address of this processor's SLIC registers.
 *
 *      "destination" identifies the SLIC i.d. of the intended
 *      recipient(s) of this command.
 *
 *	"mask" is the value to set the destination SLICs group
 *	mask to.
 *
 *      Returns zero if successfully set.  Returns a non-zero error
 *	value upon failure.
 *
 * Description:
 *      Program the SLIC with a SET-GROUP-MASK command for the 
 *      specified slic destination and mask value, then await 
 * 	command completion. If a parity-error failure is detected, 
 *	then retry the command a limited number of times.  
 *	If a failure occurs because the intended recipient does 
 *	not exist, then return an error indicating that.  In either 
 *	case or if it fails for other reasons, log the failures.  
 */
int
setGM(unchar destination, unchar mask)
{
	volatile struct cpuslic *sl = (struct cpuslic *)PHYS_SLIC;
	int error = 0;
	int retry = 4;
	unsigned char stat;

	sl->sl_dest = destination;
	sl->sl_smessage = mask;
loop:
	sl->sl_cmd_stat = SL_SETGM;
	while (sl->sl_cmd_stat & SL_BUSY)
		continue;
	stat = sl->sl_cmd_stat;
	if ((stat & SL_PARITY) == 0) {
		slicerror(SL_PERROR, SL_SETGM, destination, mask, stat);
		if (--retry)
			goto loop;
		error = SL_BAD_PAR;
	}
	if ((stat & SL_EXISTS) == 0) {
		slicerror(SL_DERROR, SL_SETGM, destination, mask, stat);
		error += SL_BAD_DEST;
	}
	if ((stat & SL_OK) == 0) {
		slicerror(SL_NOTOK, SL_SETGM, destination, mask, stat);
		error += SL_NOT_OK;
	}
	return(error);
}
