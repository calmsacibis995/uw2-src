/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SIGINFO_H
#define _SIGINFO_H
#ident	"@(#)sgs-head:common/head/siginfo.h	1.6"

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/siginfo.h>

#ifdef __STDC__
extern void	psiginfo(const siginfo_t *, const char *);
extern void	psignal(int, const char *);
#else
extern void	psiginfo();
extern void	psignal();
#endif

union sigval {
	int	sival_int;	/* integer value */
	void	*sival_ptr;	/* pointer value */
};

union notifyinfo {
	int		nisigno;	/* signal number */
	void		(*nifunc)(union sigval);
};

struct sigevent {
	int		sigev_notify;	/* notification mode */
	union notifyinfo sigev_notifyinfo; 
	union sigval	sigev_value;	/* signal value */
};
#define	sigev_func	sigev_notifyinfo.nifunc
#define sigev_signo	sigev_notifyinfo.nisigno

/* values of sigev_notify */
#define	SIGEV_NONE	1		/* no notification required */
#define	SIGEV_SIGNAL	2		/* queued signal notification */
#define	SIGEV_CALLBACK	3		/* call back notification */

#ifdef __cplusplus
}
#endif

#endif /*_SIGINFO_H*/
