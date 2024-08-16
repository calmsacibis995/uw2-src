/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:menuPulldown.h	1.3"
/*----------------------------------------------------------------------------
 *	menuPulldown.h
 */
#ifndef MENUPULLDOWN_H
#define MENUPULLDOWN_H

#include <Xm/Xm.h>

#include "action.h"

/*----------------------------------------------------------------------------
 *
 */
class MenuPulldown : public Action {
public:
								MenuPulldown (Widget	w,
											  char*		name,
											  action*	menuItem);
								~MenuPulldown ();

private:
	Widget						_pulldown;

public:
	inline Widget				PulldownWidg ();
};

/*----------------------------------------------------------------------------
 *
 */
Widget
MenuPulldown::PulldownWidg ()
{
	return (_pulldown);
}

#endif	// MENUPULLDOWN_H
