/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $RCSfile: usersub.c,v $$Revision: 1.1.1.1 $$Date: 1993/10/11 20:26:14 $
 *
 * $Log: usersub.c,v $
 * Revision 1.1.1.1  1993/10/11  20:26:14  ram
 * NUC code from 1.1d release
 *
 * Revision 4.0.1.1  92/06/08  11:54:52  lwall
 * Initial revision
 * 
 * Revision 4.0.1.1  91/11/05  19:07:24  lwall
 * patch11: there are now subroutines for calling back from C into Perl
 * 
 * Revision 4.0  91/03/20  01:56:34  lwall
 * 4.0 baseline.
 * 
 * Revision 3.0.1.1  90/08/09  04:06:10  lwall
 * patch19: Initial revision
 * 
 */

#include "EXTERN.h"
#include "perl.h"

int
userinit()
{
    install_null();	/* install device /dev/null or NUL: */
    init_curses();
    return 0;
}
