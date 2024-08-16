/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/telnet/main.c	1.1.1.3"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*      SCCS IDENTIFICATION        */

/*
 * Copyright (c) 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1988, 1990 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)main.c	5.3 (Berkeley) 3/1/91";
#endif /* not lint */

#include <sys/types.h>
#include <locale.h>
#include <pfmt.h>

#include "ring.h"
#include "externs.h"
#include "defines.h"

int international_settings = 0;
/*
 * Initialize variables.
 */
    void
tninit()
{
    init_terminal();

    init_network();
    
    init_telnet();

    init_sys();

#if defined(TN3270)
    init_3270();
#endif
}

	void
usage()
{
#ifdef	AUTHENTICATE
	pfmt(stderr, MM_ERROR, ":549:Usage: %s %s ", prompt,
	    "[[-7] | [-8] [-L]] [-E] [-K] [-X atype] [-a] [-d] [-e char] [-k realm]\n\t[-l user] [-n tracefile]");
#else
	pfmt(stderr, MM_ERROR, ":545:Usage: %s %s ", prompt,
	    "[[-7] | [-8] [-L]] [-E] [-a] [-d] [-e char] [-l user] [-n tracefile]\n\t");
#endif
#if defined(TN3270) && defined(unix)
# ifdef AUTHENTICATE
	pfmt(stderr, MM_NOSTD, ":550:%s",
	    "[-noasynch] [-noasynctty] [-noasyncnet]\n\t[-r] [-t transcom] ");
# else
	pfmt(stderr, MM_NOSTD, ":546:%s",
	    "[-noasynch] [-noasynctty] [-noasyncnet] [-r] [-t transcom]\n\t");
# endif
#else
	pfmt(stderr, MM_NOSTD, ":547:%s","[-r]");
#endif
#ifdef	ENCRYPT
	pfmt(stderr, MM_NOSTD, ":550:%s","[-x] [host-name [port]]");
#else
	pfmt(stderr, MM_NOSTD, ":548:%s","[host-name [port]]\n");
#endif
	exit(1);
}

/*
 * main.  Parse arguments, invoke the protocol or command parser.
 */


main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	int ch;
	char *user, *strrchr();
	struct termios temp_termios;
	int seven_bit_option = 0;
 
	if ((tcgetattr(0, &temp_termios) == 0) &&
	    (0 == (temp_termios.c_iflag & ISTRIP)) &&
	    ((temp_termios.c_cflag&CSIZE)==CS8)) {
		international_settings = 1;
	}

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxtelnet");
	(void)setlabel("UX:telnet");

	tninit();		/* Clear out things */
#if	defined(CRAY) && !defined(__STDC__)
	_setlist_init();	/* Work around compiler bug */
#endif

	TerminalSaveState();

	if (prompt = strrchr(argv[0], '/'))
		++prompt;
	else
		prompt = argv[0];

	user = NULL;

	rlogin = (strncmp(prompt, "rlog", 4) == 0) ? '~' : _POSIX_VDISABLE;
	autologin = -1;

	while ((ch = getopt(argc, argv, "78EKLS:X:ade:k:l:n:rt:x")) != EOF) {
		switch(ch) {
		case '7':
			if (eight) usage();
			international_settings = 0;
			seven_bit_option++;
			break;
  		case '8':
			if (seven_bit_option) usage();
			international_settings = 0;
			eight = 3;	/* binary output and input */
			break;
		case 'E':
			rlogin = escape = _POSIX_VDISABLE;
			break;
		case 'K':
#ifdef	AUTHENTICATE
			autologin = 0;
#endif
			break;
		case 'L':
			if (seven_bit_option) usage();
			international_settings = 0;
			eight |= 2;	/* binary output only */
			break;
		case 'S':
		    {
#ifdef	HAS_GETTOS
			extern int tos;

			if ((tos = parsetos(optarg, "tcp")) < 0)
				pfmt(stderr, MM_WARNING, ":2: Bad TOS argument '%s; will try to use default TOS\n", optarg);
#else
			pfmt(stderr, MM_WARNING, ":3: -S ignored, no parsetos() support.\n");
#endif
		    }
			break;
		case 'X':
#ifdef	AUTHENTICATE
			auth_disable_name(optarg);
#endif
			break;
		case 'a':
			autologin = 1;
			break;
		case 'c':
			skiprc = 1;
			break;
		case 'd':
			debug = 1;
			break;
		case 'e':
			set_escape_char(optarg);
			break;
		case 'k':
#if defined(AUTHENTICATE) && defined(KRB4)
		    {
			extern char *dest_realm, dst_realm_buf[], dst_realm_sz;
			dest_realm = dst_realm_buf;
			(void)strncpy(dest_realm, optarg, dst_realm_sz);
		    }
#else
			pfmt(stderr, MM_WARNING, ":4: -k ignored, no Kerberos V4 support.\n");
#endif
			break;
		case 'l':
			autologin = 1;
			user = optarg;
			break;
		case 'n':
#if defined(TN3270) && defined(unix)
			/* distinguish between "-n oasynch" and "-noasynch" */
			if (argv[optind - 1][0] == '-' && argv[optind - 1][1]
			    == 'n' && argv[optind - 1][2] == 'o') {
				if (!strcmp(optarg, "oasynch")) {
					noasynchtty = 1;
					noasynchnet = 1;
				} else if (!strcmp(optarg, "oasynchtty"))
					noasynchtty = 1;
				else if (!strcmp(optarg, "oasynchnet"))
					noasynchnet = 1;
			} else
#endif	/* defined(TN3270) && defined(unix) */
				SetNetTrace(optarg);
			break;
		case 'r':
			rlogin = '~';
			break;
		case 't':
#if defined(TN3270) && defined(unix)
			transcom = tline;
			(void)strcpy(transcom, optarg);
#else
			pfmt(stderr, MM_WARNING, ":5: -t ignored, no TN3270 support.\n");
#endif
			break;
		case 'x':
#ifdef	ENCRYPT
			encrypt_auto(1);
			decrypt_auto(1);
#else
			pfmt(stderr, MM_WARNING, ":6: -x ignored, no ENCRYPT support.\n");
#endif
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	if (autologin == -1)
		autologin = (rlogin == _POSIX_VDISABLE) ? 0 : 1;

	argc -= optind;
	argv += optind;

	if (argc) {
		char *args[7], **argp = args;

		if (argc > 2)
			usage();
		*argp++ = prompt;
		if (user) {
			*argp++ = "-l";
			*argp++ = user;
		}
		*argp++ = argv[0];		/* host */
		if (argc > 1)
			*argp++ = argv[1];	/* port */
		*argp = 0;

		if (setjmp(toplevel) != 0)
			Exit(0);
		if (tn(argp - args, args) == 1)
			return (0);
		else
			return (1);
	}
	(void)setjmp(toplevel);
	for (;;) {
#ifdef TN3270
		if (shell_active)
			shell_continue();
		else
#endif
			command(1, 0, 0);
	}
}
