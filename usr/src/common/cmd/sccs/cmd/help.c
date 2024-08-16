/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:cmd/help.c	1.6"
#include	<stdio.h>
#include	<sys/types.h>
#include	<macros.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<sys/euc.h>
#include	<limits.h>

char	Ohelpcmd[]   =   "/usr/ccs/lib/help/lib/help2";
extern	int	errno;

main(argc,argv)
int argc;
char *argv[];
{
	(void)setlocale(LC_ALL,"");
	(void)setcat("uxepu");
	(void)setlabel("UX:help");

	execv(Ohelpcmd,argv);
	pfmt(stderr,MM_ERROR,
		":100:Could not exec: %s.  Errno=%d\n",Ohelpcmd,errno);
	exit(1);
}
