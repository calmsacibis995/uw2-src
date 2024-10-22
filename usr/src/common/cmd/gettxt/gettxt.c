/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)gettxt:gettxt.c	1.4.7.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/gettxt/gettxt.c,v 1.1 91/02/28 17:32:24 ccs Exp $"

#include <stdio.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

main(argc, argv)
int	argc;
char	*argv[];
{
	char	*dfltp;
	char	*locp;

	locp = setlocale(LC_ALL, "");
	if (locp == (char *)NULL) {
		(void)setlocale(LC_CTYPE, "");
		(void)setlocale(LC_MESSAGES, "");
	}

	(void)setlabel("UX:gettxt");
	(void)setcat("uxcore.abi");

	if (argc != 2 && argc != 3) {
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, ":316:Usage: gettxt msgid [ dflt_msg ]\n");
		exit(1);
	}


	if (argc == 2) {
		(void)setcat("");
		fputs(gettxt(argv[1], ""), stdout);
		exit(0);
	}

	if ( ( dfltp = malloc(strlen(argv[2]) + 1) )
					== (char *)NULL ) {
		pfmt(stderr, MM_ERROR, 
			":312:Out of memory: %s\n", strerror(errno));
		exit(1);
	}

	(void)setcat("");

	strccpy(dfltp, argv[2]);

	fputs(gettxt(argv[1], dfltp), stdout);

	(void)free(dfltp);

	exit(0);
}
