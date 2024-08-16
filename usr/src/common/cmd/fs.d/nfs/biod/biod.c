/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:biod/biod.c	1.1.1.4"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	biod.c, nfs asynchronous block I/O daemon (nfsbiod).
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>

main(int argc, char *argv[])
{
	int	pid, ret;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxnfscmds");
	(void) setlabel("UX:biod");

	if (argc > 1) {
		usage();
		exit(1);
	}

	setsid();

	pid = fork();

	if (pid == 0) {
		if (async_daemon() == 0) {
			/*
			 * already running.
			 */
			exit(0);
		} else {
			pfmt(stderr, MM_ERROR, 
			     ":40:%s failed: %s\n",
			     "async_daemon", strerror(errno));
			exit(1);
		}
	}

	if (pid < 0) {
		pfmt(stderr, MM_ERROR, ":40:%s failed: %s\n",
		     "fork", strerror(errno));
		exit(1);
	}

	/*
	 * parent exits
	 */
	exit(0);

	/* NOTREACHED */
}

usage()
{
	pfmt(stderr, MM_ACTION, ":80:Usage: biod\n");
}
