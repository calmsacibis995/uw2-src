/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef EventTable_h
#define EventTable_h
#ident	"@(#)debugger:inc/common/EventTable.h	1.2"

// EventTable manages the low-level events associated with a given
// process.  When a process dies or exits, its event table is saved with
// a proto program so its events can be re-instantiated.

#include "Breaklist.h"
#include "Siglist.h"
#include "TSClist.h"
#include "Ev_Notify.h"

class Event;
class Object;
class ProcObj;

struct EventTable {
	Breaklist	breaklist;
	Siglist		siglist;
	TSClist		tsclist;
	NotifyEvent	*watchlist;
	NotifyEvent	*onstoplist;
	Object 		*object;
	Event		*firstevent; 
			EventTable();
			~EventTable();
	void		set_watchpoint(Notifier, void *, ProcObj *);
	int		remove_watchpoint(Notifier, void *, ProcObj *);
	void		remove_event(Event *);
	void		set_onstop(Notifier, void *, ProcObj *);
	int		remove_onstop(Notifier, void *, ProcObj *);
};

EventTable		*find_et( int, char *&path );
EventTable		*dispose_et( EventTable * );

#endif

// end of EventTable.h

