/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/nonexclu.h	1.5"
#endif

#include "changebar.h"

#ifndef _NONEXCLU_H
#define _NONEXCLU_H

typedef struct NonexclusiveItem 
{
	XtArgVal		name;
	XtArgVal		addr;
	XtArgVal		is_default;
	XtArgVal		is_set;
}			NonexclusiveItem;

typedef struct {
	Boolean			caption;
	String			name;
	String			string;
	Modifiers		modifiers;
	NonexclusiveItem *	default_item;
	List *			items;
	void			(*f)();
	ADDR			addr;
	Widget *		w;
	String			current_label;
	Boolean			track_changes;
	ChangeBar *		ChangeBarDB;
	void			(*change) ( void );
}			Nonexclusive;

extern void		CreateNonexclusive(Widget, Nonexclusive *, Boolean );
extern void		UnsetAllNonexclusiveItems( Nonexclusive *);
extern void		SetNonexclusiveItem(Nonexclusive *, NonexclusiveItem *);
extern void		SetSavedItems(Nonexclusive *);
extern void		ReadSavedItems(Nonexclusive *);

#endif
