/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:menuBar.h	1.3"
/*----------------------------------------------------------------------------
 *	menuBar.h
 */
#ifndef MENUBAR_H
#define MENUBAR_H

#include "BasicComponent.h"
#include "action.h"

/*----------------------------------------------------------------------------
 *
 */
typedef struct _menubar_item {
	action						menu_bar;
	action*						action_items;	
} MenuBarItem;

/*----------------------------------------------------------------------------
 *
 */
class MenuPulldown;
class Action;

class MenuBar : public BasicComponent {
public:
								MenuBar (Widget			parent,
										 char*			name,
										 MenuBarItem	menubar[],
										 const short	cnt);
								~MenuBar ();
private:
	Action*						_menuBarList;
	Action*						_menuItemList;

private:
	MenuPulldown*				BuildPulldownMenu (MenuBarItem*	menuBarItem);

public:
	void						SetSensitivity (Boolean dfltFlg);
	void						SpecialSensitivity (short	special,
													Boolean	newSensitivity);
};

#endif	// MENUBAR_H
