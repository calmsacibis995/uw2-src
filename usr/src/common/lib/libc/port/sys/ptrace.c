/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/ptrace.c	1.7"

#ifdef __STDC__
	#pragma weak ptrace = _ptrace
#endif
#include "synonyms.h"
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/procfs.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <ptrace.h>

/*
 * BRAINDEAD - standards and standards conformance test suites are
 * written by fools.  One result of this is the fact that it is not
 * possible to increase the capabilities of a feature by removing a
 * limitation -- someone will have written a test case to make sure
 * that the limitation is present.  This BRAINDEAD macro enables
 * compatibility with a few such limitations of the old ptrace -
 * including that the target process must have executed ptrace request
 * 0 and that memory addresses must be even.
 */
#define BRAINDEAD

static int prcmd(int ctlfd, int cmd, void *argp, size_t arglen)
{
	int rc;

	if (arglen > 0) {
		struct iovec vec[2];
		vec[0].iov_base = (void *)&cmd;
		vec[0].iov_len = sizeof cmd;
		vec[1].iov_base = argp;
		vec[1].iov_len = arglen;
		rc = (writev(ctlfd, vec, 2) == sizeof cmd + arglen) ? 0 : -1;
	} else
		rc = (write(ctlfd, &cmd, sizeof cmd) == sizeof cmd) ? 0 : -1;
	errno = rc ? errno == ENOENT ? ESRCH : EIO : 0;
	return rc;
}

#define CLOSED -1
/*
 * ptrace - old-fashioned way to control a child process.
 */
int ptrace(int request, pid_t pid, int addr, int data)
{
	char name[50];
	static pid_t lastpid = CLOSED;
	static int ctlfd = CLOSED, statusfd = CLOSED, asfd = CLOSED;
	boolean_t needctl=B_FALSE, needstatus=B_FALSE, needas=B_FALSE;
	pstatus_t pstatus;

	if (request == 0)
		pid = getpid();

	if (lastpid != pid) {
		if (ctlfd != CLOSED) {
			(void)close(ctlfd);
			ctlfd = CLOSED;
		}
		if (statusfd != CLOSED) {
			(void)close(statusfd);
			statusfd = CLOSED;
		}
		if (asfd != CLOSED) {
			(void)close(asfd);
			asfd = CLOSED;
		}
	}
	lastpid = pid;

	switch (request) {
	case 6:				/* Write u-area */
	case 7:				/* Continue */
	case 9:				/* Single Step */
		needstatus = B_TRUE;
		/* FALL THROUGH */
	case 0:				/* I want to be traced. */
	case 8:				/* Kill */
		needctl = B_TRUE;
		break;
	case 1:				/* Read I space */
	case 2:				/* Read D space */
	case 4:				/* Write I space */
	case 5:				/* Write D space */
#ifdef BRAINDEAD
		if (addr & 1) {
			errno = EIO;
			return -1;
		}
#endif
		needas = B_TRUE;
		break;
	case 3:				/* Read u-area */
		needstatus = B_TRUE;
		break;
	default:
		errno = EIO;
		return -1;
	}

#ifdef BRAINDEAD
	if (request != 0)
		needstatus = B_TRUE;
#endif

	if (needctl && ctlfd == CLOSED) {
		(void)sprintf(name, "/proc/%d/ctl", pid);
		ctlfd = open(name, O_WRONLY);
		if (ctlfd == CLOSED) {
			if (errno == ENOENT)
				errno = ESRCH;
			return -1;
		}
		fcntl(ctlfd, F_SETFD, 1);
	}
	if (needstatus && statusfd == CLOSED) {
		sprintf(name, "/proc/%d/status", pid);
		statusfd = open(name, O_RDONLY);
		if (statusfd == CLOSED) {
			if (errno == ENOENT)
				errno = ESRCH;
			return -1;
		}
		fcntl(statusfd, F_SETFD, 1);
	}
	if (needas && asfd == CLOSED) {
		sprintf(name, "/proc/%d/as", pid);
		asfd = open(name, O_RDWR);
		if (asfd == CLOSED) {
			if (errno == ENOENT)
				errno = ESRCH;
			return -1;
		}
		fcntl(asfd, F_SETFD, 1);
	}

	if (needstatus &&
	    (lseek(statusfd, 0L, 0) == -1L ||
	     read(statusfd, &pstatus, sizeof pstatus) != sizeof pstatus)) {
		errno = errno == ENOENT ? ESRCH : EIO;
		return -1;
	}

#ifdef BRAINDEAD
	if (request != 0 && !(pstatus.pr_flags & PR_PTRACE)) {
		errno = ESRCH;
		return -1;
	}
#endif

	switch (request) {
	case 0:				/* I want to be traced. */
	{
		long flags = PR_PTRACE;
		sigset_t sigs;
		sysset_t syss;
		if (prcmd(ctlfd, PCSET, &flags, sizeof flags))
			return -1;
		prfillset(&sigs);
		if (prcmd(ctlfd, PCSTRACE, &sigs, sizeof sigs))
			return -1;
		premptyset(&syss);
		praddset(&syss, SYS_exec);
		praddset(&syss, SYS_execve);
		if (prcmd(ctlfd, PCSEXIT, &syss, sizeof syss))
			return -1;
		return fcntl(ctlfd, F_SETFD, 0);
	}

	case 8:				/* Kill */
	{
		int sig = SIGKILL;
		return prcmd(ctlfd, PCKILL, &sig, sizeof sig);
	}

	case 7:				/* Continue */
	case 9:				/* Single Step */
	{
		int i, flags = 0;

		if (addr != 1 || data < 0 || data >= NSIG) {
			errno = EIO;
			return -1;
		}
		if (request == 9)
			flags |= PRSTEP;

		/* Clear any pending signals. */
		for (i=1; i<NSIG; ++i)
			if (sigismember(&pstatus.pr_sigpend, i))
				prcmd(ctlfd, PCUNKILL, &i, sizeof i);

		/* Set current signal if requested. */
		if (data == 0)
			flags |= PRCSIG;
		else
			if (data != pstatus.pr_lwp.pr_cursig) {
				siginfo_t siginfo;
				memset(&siginfo, 0, sizeof siginfo);
				siginfo.si_signo = data;
				if (prcmd(ctlfd, PCSSIG, &siginfo, sizeof siginfo))
					return -1;
			}

		if (prcmd(ctlfd, PCRUN, &flags, sizeof flags)) {
			errno = EIO;
			return -1;
		}
		return data;
	}

	case 1:				/* Read I space */
	case 2:				/* Read D space */
	{
		int val;
		if (lseek(asfd, (off_t)addr, 0) == -1L ||
		    read(asfd, &val, sizeof val) != sizeof val) {
			errno = EIO;
			return -1;
		}
		errno = 0;
		return val;
	}

	case 4:				/* Write I space */
	case 5:				/* Write D space */
		if (lseek(asfd, (off_t)addr, 0) == -1L ||
		    write(asfd, &data, sizeof data) != sizeof data) {
			errno = EIO;
			return -1;
		}
		errno = 0;
		return data;

	case 3:				/* Read u-area */
		if (addr == offsetof(struct user, u_ar0))
			return offsetof(struct user, u_context.uc_mcontext.gregs);
		if (addr < offsetof(struct user, u_context) ||
		    addr >= sizeof (struct user)) {
			errno = EIO;
			return -1;
		}
		addr -= offsetof(struct user, u_context);
		errno = 0;
		return *(int *)(addr + (char *)&pstatus.pr_lwp.pr_context);

	case 6:				/* Write u-area */
		if (addr < offsetof(struct user, u_context) ||
		    addr >= sizeof (struct user)) {
			errno = EIO;
			return -1;
		}
		addr -= offsetof(struct user, u_context);
		*(int *)(addr + (char *)&pstatus.pr_lwp.pr_context) = data;
		if (addr >= offsetof(struct user, u_context.uc_mcontext.gregs) &&
		    addr < (offsetof(struct user, u_context.uc_mcontext.gregs) +
			    sizeof (gregset_t)))
			if (prcmd(ctlfd, PCSREG,
				  &pstatus.pr_lwp.pr_context.uc_mcontext.gregs,
				  sizeof pstatus.pr_lwp.pr_context.uc_mcontext.gregs)) {
				errno = EIO;
				return -1;
			}
		if (addr >= offsetof(struct user, u_context.uc_mcontext.fpregs) &&
		    addr < (offsetof(struct user, u_context.uc_mcontext.fpregs) +
			    sizeof (fpregset_t)))
			if (prcmd(ctlfd, PCSFPREG,
				  &pstatus.pr_lwp.pr_context.uc_mcontext.fpregs,
				  sizeof pstatus.pr_lwp.pr_context.uc_mcontext.fpregs)) {
				errno = EIO;
				return -1;
			}

		return data;
	}
	/* NOTREACHED */
}
