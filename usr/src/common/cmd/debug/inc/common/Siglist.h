/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Siglist_h
#define Siglist_h
#ident	"@(#)debugger:inc/common/Siglist.h	1.2"

#include "Ev_Notify.h"
#include "Proctypes.h"
#include <signal.h>

class ProcObj;

// _sigctl keeps track of the global mask actually set for the process.
// _sigset keeps track of the process level settings to be used
// when a new thread is created; this set is changed on signal or signal -i;
// it is not effected by a signal event.

class Siglist {
	sig_ctl		_sigctl;
	sigset_t	_sigset;
	NotifyEvent	*_events[ NSIG-1 ];
public:
			Siglist();
			~Siglist();
	void		copy(Siglist &);
	int		add( sigset_t *, Notifier, void *, ProcObj *);
	int		remove( sigset_t *, Notifier, void *, ProcObj *);
	int		catch_sigs(sigset_t *, int level);
	int		ignore_sigs(sigset_t *);
	NotifyEvent 	*events( int sig );
	sig_ctl		*sigctl() { return &_sigctl; }
	sigset_t	*sigset() { return &_sigset; }
};

#endif

// end of Siglist.h

