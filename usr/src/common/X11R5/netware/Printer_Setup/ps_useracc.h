/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_useracc.h	1.6"
/*----------------------------------------------------------------------------
 *	ps_useracc.h
 */
#ifndef PSUSERACCESS_H
#define PSUSERACCESS_H

#include "printUser.h"
#include "PList.h"

const short ALLOW				= 0;
const short DENY				= 1;
const short ALLOWALL			= 2;
const short DENYALL				= 3;

/*----------------------------------------------------------------------------
 *
 */
class PSUserAccess : public PSDialog {
public: 
								PSUserAccess (Widget,
											  char*,
											  PSPrinter*,
											  short,
											  action*,
											  short);
								~PSUserAccess ();

private:
	void						CreateCtrlArea ();

	static void					applyCallback (Widget, XtPointer, XtPointer);
	void						Apply ();
	static void					resetCallback (Widget, XtPointer, XtPointer);
	void						Reset ();
	static void					cancelCallback (Widget, XtPointer, XtPointer);
	void						Cancel ();
	static void					helpCallback (Widget, XtPointer, XtPointer);
	void						Help ();
	static void					ChangeAllowedCB (Widget, XtPointer, XtPointer);
	void						ChangeAllowed (short type);

	static void					SelectCB (Widget, XtPointer, XtPointer);

private:
	Widget						_label;
	Widget						_denyListHdr;
	Widget						_allowListHdr;

	Widget						_rowCol;
	Widget						_allowBtn;
	ClientInfo					_allowInfo;
	Widget						_denyBtn;
	ClientInfo					_denyInfo;
	Widget						_allowAllBtn;
	ClientInfo					_allowAllInfo;
	Widget						_denyAllBtn;
	ClientInfo					_denyAllInfo;

	const short 				LOWEST_USER_UID;
	PSPrinter*					_selPrinter;

	Widget						_denyListForm;
	PList*						_denyList;
	Widget						_allowListForm;
	PList*						_allowList;

private:
	static void					ResizeCtrlAreaCB (Widget,
												  XtPointer,
												  XEvent*,
												  Boolean*);
	void						ResizeCtrlArea (XEvent*);

	Boolean						GetUserList ();

	void						AddPrintUser (struct passwd*);
	void						FreePrintUserList ();
	void						SetAllowedForList (Boolean);
	void						SetAllowedForUser (Boolean, char*, Boolean);
};

#endif
