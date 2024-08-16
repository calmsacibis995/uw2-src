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

#ident	"@(#)stand:i386sym/standalone/sys/putchar.c	1.1"

/*
 * Control the putting of chars
 */

#include <sys/types.h>
#include <sys/cfg.h>
#include <sys/slic.h>
#include <sys/slicreg.h>

static void init_putchar(int);
static void (*char_out)(int) = init_putchar;
int _slscsi = -1;


/*
 * int
 * putchar(int)
 *      Write 1 character to the system console.
 *
 * Calling/Exit State:
 *	Do not assume the hook to the console device
 *	has been initialized, but will be handled by
 *	the indirect call made to its character output
 * 	function.  "char_out" is initially set to 
 *	init_putchar() which will reset it to the device
 * 	specific function.
 *
 *	Always returns zero.
 *
 * Remarks:
 *      This function insulates the utilities from the specific
 *      console controller for the system, in the event multiple
 *      such controllers are supported simultaneously.
 */
int
putchar(int c)
{
	(*char_out)(c);
	return (0);
}

/*
 * static void
 * init_putchar(int)
 *	Initialize the console, then display the specified character upon it.
 *
 * Calling/Exit State:
 *	The console device has not yet been determined or initialized
 *	to perform output, which this function installs.
 *
 *	No return value.
 *
 * Description:
 * 	A one-shot function invoked the first time a console 
 *	character is to be printed.  Determine the location 
 *	of the console output, save its slic address for
 *	future use and install its device dependent putchar 
 *	function into the indirect function pointer used to
 *	call this function.  Then invoke the device dependent 
 *	to output the value passed to this function.
 *
 *	This technique is easily expandable to support multiple
 *	possible console controllers simultaneously.
 */
static void
init_putchar(int c)
{
	struct config_desc *cd = CD_LOC;
	extern void ssm_putchar(int);

	char_out = ssm_putchar;
	_slscsi = cd->c_cons->cd_slic;

	(void)putchar(c);
}
