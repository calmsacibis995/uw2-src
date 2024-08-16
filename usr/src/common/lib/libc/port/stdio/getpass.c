/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/getpass.c	1.20.3.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <signal.h>
#include <termio.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "stdiom.h"
#include "stdlock.h"

#ifdef __STDC__
	#pragma weak getpass = _getpass
	#pragma weak getpass_r = _getpass_r
#endif

static volatile sig_atomic_t intr;

static void
#ifdef __STDC__
catch(int sig)
#else
catch(sig)int sig;
#endif
{
	intr = 1;
}

char *
#ifdef __STDC__
getpass_r(const char *prompt, char *ans, size_t len)
#else
getpass_r(prompt, ans, len)const char *prompt;char *ans;size_t len;
#endif
{
	struct termio ttymode;
	unsigned long oldflags;
	struct sigaction sa, sigint;
	int fd;
	size_t cnt;
	char byte, *p;

	if (len == 0 || (fd = open(_str_devtty, O_RDONLY)) < 0)
		return 0;
	intr = 0;
	sa.sa_handler = SIG_IGN;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, &sigint);
	if (sigint.sa_handler != SIG_IGN)
	{
		sa.sa_handler = catch;
		sigaction(SIGINT, &sa, (struct sigaction *)0);
	}
	ioctl(fd, TCGETA, &ttymode);
	oldflags = ttymode.c_lflag;
	ttymode.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	ioctl(fd, TCSETAF, &ttymode);
	fputs(prompt, stderr);
	p = &ans[0];
	cnt = len - 1;	/* leave room for \0 */
	while (!intr)
	{
		if (read(fd, (void *)&byte, 1) != 1
			|| byte == '\n' || byte == '\r')
		{
			break;
		}
		if (cnt != 0)
		{
			*p++ = byte;
			cnt--;
		}
	}
	*p = '\0';
	ttymode.c_lflag = oldflags;
	ioctl(fd, TCSETAW, &ttymode);
	putc('\n', stderr);
	close(fd);
	sigaction(SIGINT, &sigint, (struct sigaction *)0);
	if (intr)
		kill(getpid(), SIGINT);
	return ans;
}

#ifndef PASS_MAX
#   define PASS_MAX	8	/* maximum significant chars in password */
#endif

char *
#ifdef __STDC__
getpass(const char *prompt)
#else
getpass(prompt)const char *prompt;
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static char ans[PASS_MAX + 1];
	char *p;

	STDLOCK(&lock);
	p = getpass_r(prompt, ans, sizeof(ans));
	STDUNLOCK(&lock);
	return p;
}
