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

#ident	"@(#)stand:i386sym/standalone/sys/slic_init.c	1.1"

#include <sys/types.h>
#include <sys/SGSproc.h>
#include <sys/slic.h>

extern int setGM(unchar, unchar);

/*
 * void
 * slic_init(void)
 * 	Initialize the executing processor's SLIC.
 *
 * Calling/Exit State:
 *	PHYS_SLIC is the address of the SLIC registers themselves.
 *
 *	Set up SLIC to everyone is in GROUP1, using BIN1 for 
 *	console I/O and set GROUP MASK to 0xFF (clear group mask).
 *
 * 	No return value.
 */
void
slic_init(void)
{
	struct cpuslic *sl = (struct cpuslic *) PHYS_SLIC;

	sl->sl_procgrp = 1;
	sl->sl_lmask = 2;
	(void)setGM(sl->sl_procid & SL_PROCID, 0xff);
}
