/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:toolbar.h	1.3"
/*----------------------------------------------------------------------------
 *	toolbar.h
 */
#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <Xm/Xm.h>

#include "BasicComponent.h"
#include "menuBar.h"

/*----------------------------------------------------------------------------
 *
 */
class Action;

class Toolbar : public BasicComponent {
public:
								Toolbar (Widget			parent,
										 char*			name,
										 MenuBarItem	buttons[],
										 const short	cnt);

private:
	Action*						_buttonList;	
	Widget						_buttonForm;
	Widget						_promptLbl;
	const short					SIDE_BORDER;
	const short					GROUP_PAD;
	short						_buttonCnt;

public:
	static void					ResizeButtonFormCB (Widget		w,
													XtPointer	client_data,
													XEvent*		event,
													Boolean*);
	void						ResizeButtonForm (XEvent *event);
	void						SetSensitivity (Boolean dfltFlg);
	void						SpecialSensitivity (short	special,
													Boolean	newSensitivity);
	void						UnmanageButtonForm ();
	void						ManageButtonForm ();
};

#endif	// TOOLBAR_H
