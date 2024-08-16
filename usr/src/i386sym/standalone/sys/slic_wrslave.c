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

#ident	"@(#)stand:i386sym/standalone/sys/slic_wrslave.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/slic.h>
#include <sys/saio.h>

extern int wrAddr(unchar, unchar);
extern int wrData(unchar, unchar);


/*
 * int 
 * wrslave(unchar, unchar, unchar)
 * 	Write the specified data to the designated SLIC slave data register.
 *
 * Calling/Exit State:
 *      "destination" identifies the SLIC i.d. of the intended
 *      recipient(s) of this command.
 *
 *	"reg" identifies the specific register of that recipient
 *	is going to be read.
 *
 *	"data" is the 8-bit value to be written to the specified slave register.
 *
 *      Returns zero if successfully written.  Otherwise returns a
 *	non-zero error indicator.
 *
 * Description:
 * 	calls wrAddr() to address the appropriate SLIC slave
 *	register, then calls wrData() to actually write data 
 *	to it.  It assumes this sequence will occur atomically
 *	with regard to the device being addressed.
 */
int
wrslave(unchar destination, unchar reg, unchar data)
{
	int error = 0;

	error = wrAddr(destination, reg);
	if (error)
		return(error);
	error = wrData(destination, data);
	return(error);
}
