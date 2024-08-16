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

#ident	"@(#)stand:i386sym/standalone/sys/slic_rdData.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/slic.h>
#include <sys/SGSproc.h>
#include <sys/saio.h>

extern void slicerror(int, unchar, unchar, unchar, unchar);

/*
 * int 
 * rdData(unchar)
 * 	Read data from the previously addressesd SLIC slave data register.
 *
 * Calling/Exit State:
 *	The caller has previously invoked wrAddr() to set the 
 *	address of the SLIC slave register to be read this time.
 *
 *      PHYS_SLIC is the address of this processor's SLIC registers.
 *
 *      "destination" identifies the SLIC i.d. of the intended
 *      recipient(s) of this command.
 *
 *      Returns an 8-bit data value if successfully read.  Returns
 * 	a larger error value if it fails.
 *
 * Description:
 *      Program the SLIC with a READ-DATA command from the 
 *      specified slic destination, then await command completion. 
 *      If a parity-error failure is detected, then retry the
 *      command a limited number of times.  If a failure occurs
 *      because the intended recipient does not exist, then
 *      return an error indicating that.  In either case or if
 * 	it fails for other reasons, log the failures.  Otherwise
 * 	return the lower 8-bits of the data value read.
 */
int
rdData(unchar destination)
{
	volatile struct cpuslic *sl = (struct cpuslic *)PHYS_SLIC;
	int error = 0;
	int retry = 4;
	unsigned char stat;

	sl->sl_dest = destination;
loop:
	sl->sl_cmd_stat = SL_RDDATA;
	while ((stat = sl->sl_cmd_stat) & SL_BUSY)
		continue;
	if ((stat & SL_PARITY) == 0) {
		slicerror(SL_PERROR, SL_RDDATA, destination, 0, stat);
		if (--retry)
			goto loop;
		error = SL_BAD_PAR;
	}
	if ((stat & SL_EXISTS) == 0) {
		slicerror(SL_DERROR, SL_RDDATA, destination, 0, stat);
		error += SL_BAD_DEST;
	}
	if ((stat & SL_OK) == 0) {
		slicerror(SL_NOTOK, SL_RDDATA, destination, 0, stat);
		error += SL_NOT_OK;
	}
	if(error)
		return(error);
	
	return(sl->sl_sdr & 0xff);
}
