/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)mesg:mesg.c	1.5.1.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/mesg/mesg.c,v 1.1 91/02/28 18:32:20 ccs Exp $"
/*
 * mesg -- set current tty to accept or
 *	forbid write permission.
 *
 *	mesg [-y] [-n]
 *		y allow messages
 *		n forbid messages
 *	return codes
 *		0 if messages are ON or turned ON
 *		1 if messages are OFF or turned OFF
 *		2 if usage error
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>

struct stat sbuf;

char *tty;
char *ttyname();

main(argc, argv)
char *argv[];
{
	int i, c, r=0, errflag=0;
	extern int optind;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:mesg");

	for(i = 0; i <= 2; i++) {
		if ((tty = ttyname(i)) != NULL)
			break;
	}
	if (stat(tty, &sbuf) < 0)
		error(":21:Cannot access %s: %s\n", tty, strerror(errno));
	if (argc < 2) {
		if (sbuf.st_mode & (S_IWGRP|S_IWOTH))
			pfmt(stdout, MM_NOSTD, ":334:is y\n");
		else  {
			r = 1;
			pfmt(stdout, MM_NOSTD, ":335:is n\n");
		}
	}
	while ((c = getopt(argc, argv, "yn")) != EOF) {
		switch (c){
		case 'y':
			newmode(S_IRUSR|S_IWUSR|S_IWGRP);
			break;
		case 'n':
			newmode(S_IRUSR|S_IWUSR);
			r = 1;
			break;
		case '?':
			errflag++;
		}
	}

	if (errflag /*  || (argc > optind) */ )
		usage(0);

/* added for temporary compat. */
	if(argc > optind) switch(*argv[optind]) {
		case 'y':
			newmode(S_IRUSR|S_IWUSR|S_IWGRP);
			break;
		case 'n':
			newmode(S_IRUSR|S_IWUSR);
			r = 1;
			break;
		default:
			errflag++;
		}

	if (errflag)
		usage(1);
/* added to here */
	exit(r);
}

error(s, a1, a2, a3)
char *s, *a1, *a2, *a3;
{
	pfmt(stderr, MM_ERROR, s, a1, a2, a3);
	exit(2);
}

newmode(m)
mode_t m;
{
	if (chmod(tty, m) < 0)
		error(":336:Cannot change mode of %s: %s\n", tty,
			strerror(errno));
}

usage(complain)
int complain;
{
	if (complain)
		pfmt(stderr, MM_ERROR, ":1:Incorrect usage\n");
	pfmt(stderr, MM_ACTION, ":337:Usage: mesg [-y] [-n]\n");
	exit(2);
}

