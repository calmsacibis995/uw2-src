/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getopt.c	1.34"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "pfmtm.h"

#ifdef __STDC__
	#pragma weak getopt = _getopt
#endif

static void
#ifdef __STDC__
err(char *cmd, int ch, const char *msg)
#else
err(cmd, ch, msg)char *cmd; int ch; const char *msg;
#endif
{
	STDLOCK(&_pfmt_lock_setlabel);
	if (_pfmt_label == 0)
		_pfmt_label = cmd;
	pfmt(stderr, MM_ERROR, msg, ch);
	if (_pfmt_label == cmd)
		_pfmt_label = 0;
	STDUNLOCK(&_pfmt_lock_setlabel);
}

int
#ifdef __STDC__
getopt(int argc, char *const *argv, const char *opts)
#else
getopt(argc, argv, opts)int argc; char *const *argv; const char *opts;
#endif
{
	static const char badopt[] =
		"uxlibc:1:Illegal option -- %c\n";
	static const char argreq[] =
		"uxlibc:2:Option requires an argument -- %c\n";
	extern int _sp;
	char *p, *s;
	int ch;

	if (optind >= argc || (p = argv[optind]) == 0)
		return -1;
	if (_sp == 1) /* start of a new list member */
	{
		if (p[0] != '-' || p[1] == '\0') /* not an option or just "-" */
			return -1;
		if (p[1] == '-' && p[2] == '\0') /* was "--" */
		{
			optind++;
			return -1;
		}
	}
	p += _sp;
	optopt = ch = *(unsigned char *)p; /* current (presumed) option */
	p++;
	if (ch == ':' || (s = strchr(opts, ch)) == 0) /* isn't an option */
	{
		if (opterr && *opts != ':')
			err(argv[0], ch, badopt);
		ch = '?';
	}
	else if (*++s == ':') /* option needs an argument */
	{
		_sp = 1;
		if (*p != '\0') /* argument is rest of current member */
			optarg = p;
		else if (++optind < argc) /* next member exists */
			optarg = argv[optind];
		else
		{
			optarg = 0;
			if (*opts == ':')
				return ':';
			if (opterr)
				err(argv[0], ch, argreq);
			return '?';
		}
		optind++;
		return ch;
	}
	optarg = 0;
	if (*p == '\0')
	{
		optind++;
		_sp = 0;
	}
	_sp++;
	return ch;
}
