/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/exclusive.h	1.12"
#endif

#include "changebar.h"

#ifndef _EXCLUSIVE_H
#define _EXCLUSIVE_H

#include <list.h>

typedef struct ExclusiveItem {
	XtArgVal		name;
	XtArgVal		addr;
	XtArgVal		is_default;
	XtArgVal		is_set;
}			ExclusiveItem;

typedef struct Exclusive {
	Boolean			caption;
	String			name;
	String			string;
	ExclusiveItem *		current_item;
	ExclusiveItem *		default_item;
	List *			items;
	void			(*f) (struct Exclusive *);
	ADDR			addr;
	Widget *		w;
	Boolean			track_changes;
	ChangeBar *		ChangeBarDB;
	void			(*change) ();
}Exclusive;

extern Widget		CreateExclusive ( Widget, Exclusive *, Boolean );
extern void		SetExclusive( Exclusive *, ExclusiveItem *, int ); 
extern int 		CheckExclusiveChangeBar( Exclusive * );


#define EXCLUSIVE(LAB,STR,ITEMS,CHANGE) \
    {									\
	True,								\
	(LAB),								\
	(STR),								\
	(ExclusiveItem *)0,						\
	(ExclusiveItem *)0,						\
	(ITEMS),							\
	(void (*)(struct Exclusive * ))0,				\
	(ADDR)0,							\
	(Widget *)0,							\
	True,								\
	NULL,								\
	CHANGE,								\
    }

extern Widget	CreateToggle(Widget, Exclusive *, Exclusive *);
#endif
