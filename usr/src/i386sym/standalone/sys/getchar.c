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

#ident	"@(#)stand:i386sym/standalone/sys/getchar.c	1.1"

extern int ssm_getchar(int);

/*
 * int
 * getchar(void)
 *	Read 1 character from the system console.
 *
 * Calling/Exit State:
 *	Simply passes the return value from ssm_getchar()
 *	back to the caller.  
 *
 * Remarks:
 *	This function insulates the utilities from the specific 
 *	console controller for the system, in the event multiple
 *	such controllers are supported simultaneously.
 */
int
getchar(void)
{
	return(ssm_getchar(1));
}
