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

#ident	"@(#)stand:i386sym/standalone/sys/clearbss.c	1.1"

extern void bzero(void *, unsigned int);
extern char _edata;     /* &_edata = last byte of nonzero-data plus 1 */
extern char _end;       /* &_end = last byte of zero-data (BSS) plus 1 */


/*
 * void
 * clearbss(void) 
 *	zero out the area that will be a standalone program's bss data segment.
 *
 * Calling/Exit State:
 * 	The loader generates symbols (variables) _end and _edata 
 *	automatically; they are not defined by the source.  The
 *	area between these two variables is the BSS, which is to
 *	be zeroed by this function.
 *
 *	No return value.
 */
void
clearbss(void)
{
	bzero(&_edata, &_end - &_edata);
}
