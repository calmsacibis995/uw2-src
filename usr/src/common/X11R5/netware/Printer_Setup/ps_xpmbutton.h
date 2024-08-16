/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_xpmbutton.h	1.4"
/*----------------------------------------------------------------------------
 *	ps_xpmbutton.h
 */
#ifndef XPMBUTTON_H
#define XPMBUTTON_H

#include <Xm/Xm.h>

#include "action.h"

/*----------------------------------------------------------------------------
 *
 */
class XPMButton : public Action {
public:
								XPMButton (Widget		w,
										   action*		item,
										   Widget		promptWidget,
										   Arg*			arg,
										   int			argCnt);

private:
	unsigned int   				_width;
	unsigned int   				_height;
	char*						_prompt;
	Widget						_promptWidget;

	void						EnterLeaveEvent (int type);
	static void 				EnterLeaveEventCallback (Widget, 
														 XtPointer	client_data,
														 XEvent*	event, 
														 Boolean*);

public:
	long						GetBackground ();
	void						SetPositions (int x);

public:
	inline unsigned int			GetWidth ();
	inline unsigned int			GetHeight ();
};

/*----------------------------------------------------------------------------
 *
 */
unsigned int
XPMButton::GetWidth ()
{
	return (_width);
}

unsigned int
XPMButton::GetHeight ()
{
	return (_height);
} 

#endif	// XPMBUTTON_H
