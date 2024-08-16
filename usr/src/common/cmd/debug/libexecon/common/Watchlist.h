/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Watchlist_h
#define Watchlist_h
#ident	"@(#)debugger:libexecon/common/Watchlist.h	1.3"

#include "Iaddr.h"

class HW_Wdata;	// opaque to clients
class ProcObj;
class Proclive;
class Rvalue;

class HW_Watch {
	HW_Wdata	*hwdata;
public:
			HW_Watch();
			~HW_Watch();
	int		add(Iaddr, Rvalue *, ProcObj *, void *);
	int		remove(Iaddr, ProcObj *, void *);
	int		hw_fired(ProcObj *); // did a hw watchpoint fire?
	int		clear_state(Proclive *);
	int		set_state(Proclive *);
};


#endif // end of Watchlist.h

