/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SIGNAL_H
#define _SIGNAL_H
#ident	"@(#)sgs-head:common/head/signal.h	1.5.6.6"

#ifdef __cplusplus
extern "C" {
#endif

typedef int 	sig_atomic_t;

extern const char	*const _sys_siglist[];
extern const int	_sys_nsig;

#include <sys/signal.h>
#include <sys/types.h>

#ifdef __STDC__

extern void	(*signal(int, void (*)(int)))(int);
extern int	raise(int);

#if __STDC__ == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)

#ifndef _PID_T
#   define _PID_T
	typedef long	pid_t;
#endif

extern int	kill(pid_t, int);
extern int	sigaction(int, const struct sigaction *, struct sigaction *);
extern int	sigaddset(sigset_t *, int);
extern int	sigdelset(sigset_t *, int);
extern int	sigemptyset(sigset_t *);
extern int	sigfillset(sigset_t *);
extern int	sigismember(const sigset_t *, int);
extern int	sigpending(sigset_t *);
extern int	sigprocmask(int, const sigset_t *, sigset_t *);
extern int	sigsuspend(const sigset_t *);

#endif

#if __STDC__ == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#include <sys/procset.h>

extern int	gsignal(int);
extern void	(*sigset(int, void (*)(int)))(int);
extern int	sighold(int);
extern int	sigrelse(int);
extern int	sigignore(int);
extern int	sigpause(int);
extern int	(*ssignal(int, int (*)(int)))(int);
extern int	sigaltstack(const stack_t *, stack_t *);
extern int	sigsend(idtype_t, id_t, int);
extern int	sigsendset(const procset_t *, int);
extern int	sigwait(sigset_t *);

#endif

#else /*!__STDC__*/

extern void	(*signal())();
extern void	(*sigset())();

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*_SIGNAL_H*/
