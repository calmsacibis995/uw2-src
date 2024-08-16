/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)tty:tty.c	1.5.2.1"

/*
** Type tty name
*/

#include	<stdio.h>
#include	<stdlib.h>	/* for ttyname, isatty, optind?  */
#include	<sys/stermio.h>
#include	<locale.h>
#include	<pfmt.h>

int		lflg;
int		sflg;

main(argc, argv)
char **argv;
{
	register char *p;
	register int	i;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue.abi");
	(void)setlabel("UX:tty");

	while((i = getopt(argc, argv, "ls")) != EOF)
		switch(i) {
		case 'l':
			lflg = 1;
			break;
		case 's':
			sflg = 1;
			break;
		case '?':
			pfmt(stderr, MM_ACTION, 
					":6:Usage:\n\ttty [-l]\n\ttty [-s]\n") ;
			exit(2);
		}
	p = ttyname(0);
	if(!sflg){
		if (p)
			puts(p);
		else
			pfmt(stdout, MM_NOSTD, ":2:not a tty\n");
	}
	if(lflg) {
		if((i = ioctl(0, STWLINE, 0)) == -1)
			pfmt(stdout, MM_NOSTD, 
				":3:not on an active synchronous line\n");
		else
			pfmt(stdout, MM_NOSTD, ":4:synchronous line %d\n", i);
	}
	exit(p? 0: 1);
}
