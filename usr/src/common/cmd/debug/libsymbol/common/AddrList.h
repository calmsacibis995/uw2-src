/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/AddrList.h	1.1"
#ifndef AddrList_h
#define AddrList_h

#include	"Attribute.h"
#include	"Rbtree.h"
#include	"Iaddr.h"

class AddrEntry : public Rbnode {
	Iaddr		loaddr,hiaddr;
	Attr_form	form;
	Attr_value	value;
	friend class	AddrList;
	friend class	Evaluator;
public:
			AddrEntry();
			~AddrEntry() {}
			AddrEntry( AddrEntry & );
	int		cmp( Rbnode & );
	Rbnode *	makenode();
};

class AddrList : public Rbtree {
public:
			AddrList() {}
			~AddrList() {}
	void		add( Iaddr lo, Iaddr hi, long offset, Attr_form );
	void		complete();
};

#endif /* AddrList_h */
