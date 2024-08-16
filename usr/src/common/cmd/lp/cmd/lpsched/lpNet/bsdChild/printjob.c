/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpsched/lpNet/bsdChild/printjob.c	1.1.5.3"
#ident	"$Header: $"

#include "lpNet.h"
#include "lpd.h"

/* 
 * This is basically a stub routine which just acknowledges 
 * the receipt of the command from the LPD system.
 */
void
#if defined (__STDC__)
printjob(void)
#else
printjob()
#endif
{
	ACK();
}
