/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef EV_Notify_h
#define EV_Notify_h

#ident	"@(#)debugger:inc/common/Ev_Notify.h	1.3"

#include "Link.h"

class ProcObj;

// Process level primitive events are connected to 
// the higher level EventManager events through Event Notifiers.
// An NotifyEvent object contains a pointer to an interface function,
// a pointer to a ProcObj, and a "this" pointer.
// The interface function calls a class member function, that in turn 
// provides notification actions.
//
// typedef used to register callback functions with event handlers

typedef int (*Notifier)(void *, ...);

// priority of notifiers 
enum ev_priority {
	ev_none,
	ev_low,
	ev_high,
};

struct NotifyEvent : public Link {
	Notifier	func;
	void		*thisptr;
	ProcObj		*object;
	ev_priority	priority;
			NotifyEvent(Notifier f, void *t, ProcObj *o,
				ev_priority ep = ev_none)
				{ func = f; thisptr = t; object = o;
					priority = ep; }
			~NotifyEvent() {}
	NotifyEvent	*next() { return (NotifyEvent *)Link::next(); }
};

extern int notify_sig_e_trigger(void *thisptr);
extern int notify_sys_e_trigger(void *thisptr);
extern int notify_onstop_e_trigger(void *thisptr);
extern int notify_stoploc_trigger(void *thisptr);
extern int notify_stop_e_clean_foreign(void *thisptr);
extern int notify_returnpt_trigger(void *thisptr);
extern int notify_watchframe_start(void *thisptr);
extern int notify_watchframe_watch(void *thisptr);
extern int notify_watchframe_end(void *thisptr);

#endif
