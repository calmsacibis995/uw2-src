/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:menuItem.h	1.3"
/*----------------------------------------------------------------------------
 *	menuItem.h
 */
#ifndef MENUITEM_H
#define MENUITEM_H

#include <Xm/Xm.h>

#include "action.h"

/*----------------------------------------------------------------------------
 *
 */
class MenuItemClass : public Action {
public:
								MenuItemClass (Widget		w,
											   char*		name,
											   action*		menuItem,
											   Arg*			arg,
											   int			argCnt);
								~MenuItemClass ();
};

#endif	// MENUITEM_H
