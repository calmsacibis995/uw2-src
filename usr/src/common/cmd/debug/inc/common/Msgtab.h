/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Msgtab_h
#define _Msgtab_h
#ident	"@(#)debugger:inc/common/Msgtab.h	1.2"

#include "Msgtypes.h"
#include "Signature.h"
#include "Msgtable.h"

class Message_table
{
	struct Msgtab	*msgtab;
	int		cat_available;
public:
			Message_table();
			~Message_table(){}

	const char	*format(Msg_id);
	void		init();
#ifndef NOCHECKS
	Signature	signature(Msg_id);
	Msg_class	msg_class(Msg_id);
#else
	Signature	signature(Msg_id m)	{ return msgtab[m].signature; }
	Msg_class	msg_class(Msg_id m)	{ return msgtab[m].mclass; }
#endif	// NOCHECKS
};

extern Message_table	Mtable;

#endif // _Msgtab_h
