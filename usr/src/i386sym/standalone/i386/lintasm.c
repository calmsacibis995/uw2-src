/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386sym/standalone/i386/lintasm.c	1.1"

/*
 * lintasm.c
 *	Definitions and usages stubs for assembly language
 *	functions and variables from local *.s files.  This
 *	file is defined solely to allow lint to work.
 */

#include <sys/types.h>

/* These are defined by the loader, but not always used. */
char _edata;     	/* &_edata = last byte of nonzero-data plus 1 */
char _end;       	/* &_end = last byte of zero-data (BSS) plus 1 */
int end;

static void
stubs(void)
{
	extern void clearbss(void);
	extern void slic_init(void);
	extern int rdslave(unchar, unchar);
	extern int wrslave(unchar, unchar, unchar);


	clearbss();
	slic_init();
	(void) rdslave(0,0);
	(void) wrslave(0,0,0);
}

void 
/*ARGSUSED*/
bzero(void *loc, size_t len) 
{
	stubs();

}

void 
/*ARGSUSED*/
bcopy(void *src, void *dest, size_t len)
{
	stubs();
}

void 
/*ARGSUSED*/
gsp(caddr_t src, caddr_t dest, long len, caddr_t entry)
{
	stubs();
}
