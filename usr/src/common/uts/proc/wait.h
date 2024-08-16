/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_WAIT_H	/* wrapper symbol for kernel use */
#define _PROC_WAIT_H	/* subject to change without notice */

#ident	"@(#)kern:proc/wait.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif
#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <proc/siginfo.h> /* SVR4.0COMPAT */
#include <proc/procset.h> /* SVR4.0COMPAT */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/siginfo.h> /* SVR4.0COMPAT */
#include <sys/procset.h> /* SVR4.0COMPAT */

#else

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)

#include <sys/types.h> /* SVR4.0COMPAT */
#include <sys/siginfo.h> /* SVR4.0COMPAT */
#include <sys/procset.h> /* SVR4.0COMPAT */

#else

#ifndef _PID_T
#define _PID_T
typedef long	pid_t;			/* process id type	*/
#endif

#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */

#endif /* _KERNEL_HEADERS */


/*
 * arguments to wait functions
 */

#ifndef WUNTRACED
#define WUNTRACED	0004	/* for POSIX */
#define WNOHANG		0100	/* non blocking form of wait	*/

/*
 * macros for stat return from wait functions
 */

#define WIFEXITED(stat)		(((int)((stat)&0377))==0)
#define WIFSIGNALED(stat)	(((int)((stat)&0377))>0&&((int)(((stat)>>8)&0377))==0)
#define WIFSTOPPED(stat)	(((int)((stat)&0377))==0177&&((int)(((stat)>>8)&0377))!=0)

#define WEXITSTATUS(stat)	((int)(((stat)>>8)&0377))
#define WTERMSIG(stat)		(((int)((stat)&0377))&0177)
#define WSTOPSIG(stat)		((int)(((stat)>>8)&0377))
#endif


/* non-POSIX definitions */

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)

#define WEXITED		0001	/* wait for processes that have exited	*/
#define WTRAPPED	0002	/* wait for processes stopped while tracing */
#define WSTOPPED	0004	/* wait for processes stopped by signals */
#define WCONTINUED	0010	/* wait for processes continued */
#define WNOWAIT		0200	/* non destructive form of wait */

#define WOPTMASK	(WEXITED|WTRAPPED|WSTOPPED|WCONTINUED|WNOHANG|WNOWAIT)

#define WSTOPFLG		0177
#define WSIGMASK		0177
#define WLOBYTE(stat)		((int)((stat)&0377))
#define WHIBYTE(stat)		((int)(((stat)>>8)&0377))
/*
 * macros for stat return from wait functions
 */

#define WCONTFLG		0177777
#define WCOREFLG		0200

#define WWORD(stat)		((int)((stat))&0177777)
#define WIFCONTINUED(stat)	(WWORD(stat)==WCONTFLG)
#define WCOREDUMP(stat)		((stat)&WCOREFLG)
#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */


#if !defined(_KERNEL)
#if defined(__STDC__)

extern pid_t wait(int *);
extern pid_t waitpid(pid_t, int *, int);

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
extern int waitid(idtype_t, id_t, siginfo_t *, int);
#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */

#else

extern pid_t wait();
extern pid_t waitpid();

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE)
extern int waitid();
#endif /* !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) */

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined(__cplusplus)
        }
#endif
#endif /* _PROC_WAIT_H */
