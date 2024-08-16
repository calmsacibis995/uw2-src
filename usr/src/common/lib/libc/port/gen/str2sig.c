/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/str2sig.c	1.4"

#include "synonyms.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#ifdef __STDC__
	#pragma weak str2sig = _str2sig
	#pragma weak sig2str = _sig2str
#endif

static const char tbl[] =
{
	/*0*/		'E','X','I','T', 0 , 0 , 0 , 0 ,
	/*SIGHUP*/	'H','U','P', 0 , 0 , 0 , 0 , 0 ,
	/*SIGINT*/	'I','N','T', 0 , 0 , 0 , 0 , 0 ,
	/*SIGQUIT*/	'Q','U','I','T', 0 , 0 , 0 , 0 ,
	/*SIGILL*/	'I','L','L', 0 , 0 , 0 , 0 , 0 ,
	/*SIGTRAP*/	'T','R','A','P', 0 , 0 , 0 , 0 ,
	/*SIGABRT*/	'A','B','R','T', 0 , 0 , 0 , 0 ,
	/*SIGEMT*/	'E','M','T', 0 , 0 , 0 , 0 , 0 ,
	/*SIGFPE*/	'F','P','E', 0 , 0 , 0 , 0 , 0 ,
	/*SIGKILL*/	'K','I','L','L', 0 , 0 , 0 , 0 ,
	/*SIGBUS*/	'B','U','S', 0 , 0 , 0 , 0 , 0 ,
	/*SIGSEGV*/	'S','E','G','V', 0 , 0 , 0 , 0 ,
	/*SIGSYS*/	'S','Y','S', 0 , 0 , 0 , 0 , 0 ,
	/*SIGPIPE*/	'P','I','P','E', 0 , 0 , 0 , 0 ,
	/*SIGALRM*/	'A','L','R','M', 0 , 0 , 0 , 0 ,
	/*SIGTERM*/	'T','E','R','M', 0 , 0 , 0 , 0 ,
	/*SIGUSR1*/	'U','S','R','1', 0 , 0 , 0 , 0 ,
	/*SIGUSR2*/	'U','S','R','2', 0 , 0 , 0 , 0 ,
	/*SIGCHLD*/	'C','H','L','D', 0 , 0 , 0 , 0 ,
	/*SIGPWR*/	'P','W','R', 0 , 0 , 0 , 0 , 0 ,
	/*SIGWINCH*/	'W','I','N','C','H', 0 , 0 , 0 ,
	/*SIGURG*/	'U','R','G', 0 , 0 , 0 , 0 , 0 ,
	/*SIGPOLL*/	'P','O','L','L', 0 , 0 , 0 , 0 ,
	/*SIGSTOP*/	'S','T','O','P', 0 , 0 , 0 , 0 ,
	/*SIGTSTP*/	'T','S','T','P', 0 , 0 , 0 , 0 ,
	/*SIGCONT*/	'C','O','N','T', 0 , 0 , 0 , 0 ,
	/*SIGTTIN*/	'T','T','I','N', 0 , 0 , 0 , 0 ,
	/*SIGTTOU*/	'T','T','O','U', 0 , 0 , 0 , 0 ,
	/*SIGVTALRM*/	'V','T','A','L','R','M', 0 , 0 ,
	/*SIGPROF*/	'P','R','O','F', 0 , 0 , 0 , 0 ,
	/*SIGXCPU*/	'X','C','P','U', 0 , 0 , 0 , 0 ,
	/*SIGXFSZ*/	'X','F','S','Z', 0 , 0 , 0 , 0 ,
	/*SIGWAITING*/	'W','A','I','T','I','N','G', 0 ,
	/*SIGLWP*/	'L','W','P', 0 , 0 , 0 , 0 , 0 ,
	/*SIGAIO*/	'A','I','O', 0 , 0 , 0 , 0 , 0 ,
/*aliases*/
	/*SIGIOT*/	'I','O','T', 0 , 0 , 0 , 0 , SIGIOT,
	/*SIGCLD*/	'C','L','D', 0 , 0 , 0 , 0 , SIGCLD,
	/*SIGIO*/	'I','O', 0 , 0 , 0 , 0 , 0 , SIGIO,
};

#define STEP	8
#define NALIAS	3
#define NMAIN	(sizeof(tbl) / STEP - NALIAS)

int
#ifdef __STDC__
str2sig(const char *s, int *ptr)
#else
str2sig(s, ptr)const char *s; int *ptr;
#endif
{
	const char *p;
	int n;

	if ('0' <= *s && *s <= '9')
	{
		if ((n = atoi(s)) >= NMAIN)
			return -1;
	}
	else
	{
		n = 0;
		for (p = &tbl[0];; p += STEP)
		{
			if (strcmp(p, s) == 0)
				break;
			if (++n >= NMAIN + NALIAS)
				return -1;
		}
		if (n >= NMAIN)
			n = p[STEP - 1];
	}
	*ptr = n;
	return 0;
}

int
#ifdef __STDC__
sig2str(int sig, char *s)
#else
sig2str(sig, s)int sig; char *s;
#endif
{
	if (sig < 0 || sig >= NMAIN)
		return -1;
	(void)strcpy(s, &tbl[sig * STEP]);
	return 0;
}
