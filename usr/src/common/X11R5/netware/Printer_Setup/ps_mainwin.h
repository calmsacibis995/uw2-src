/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_mainwin.h	1.8"
/*----------------------------------------------------------------------------
 *	ps_mainwin.h
 */
#ifndef PSMAINWIN_H
#define PSMAINWIN_H

#include "ps_xpmbutton.h"
#include "dispInfo.h"
#include "ps_hdr.h"
#include "ps_application.h"
#include "ps_actions.h"
#include "ps_win.h"
#include "ps_printerWin.h"
#include "ps_question.h"
#include "ps_msg.h"

enum {
	LOCAL_PTR,
	LOCAL_DFLT_PTR,
	UNIX_PTR,
	UNIX_DFLT_PTR,
	NETWARE_PTR,
	NETWARE_DFLT_PTR,
	PIXMAP_CNT
};	

/*----------------------------------------------------------------------------
 *
 */
class PSMainWin : public BasicComponent {
public:
								PSMainWin (Widget, char*, Application* ps);
								~PSMainWin ();

public:
	void						DeleteDialogs ();

private:
	void						RemovePrinterDialogs (char*);
	void						PurgePrinter ();	
	void 						CleanupPrinter ();

	static void					helpCallback (Widget,
											  XtPointer data,
											  XtPointer);
	void						help ();

private:
	Application*				_ps;
	PSPrinterWin*				_printerWin;
	PSActions*					_actions;
	Widget						_printerSW;
	Widget						_printerInfoSW;
	PSPrinter*					_selPrinter;
	PSWin*						_winList;

	ClientInfo					_deletePrinterInfo;
	PSQuestion*					_pq;
	PSMsg*						_pm;

public:
	Boolean						RaiseDialogWin (short ptype);
	void						AddDialogWin (PSWin* newWin);
	void						UpdateSelectedPrinter (IconObj* obj);
	void						NotifyParentOfDefault ();
	PSPrinter*					AddNULLPrinter ();
	void						DeletePrinter ();
	void						AddPrinter (PSPrinter*);
	const IconObj*				findByName (const char* name);
	void						ResponseCallback (short type, Boolean response);
	inline PSPrinter*			SelectedPrinter ();
	inline Application*			GetApplication ();
};

/*----------------------------------------------------------------------------
 *
 */
PSPrinter*
PSMainWin::SelectedPrinter ()
{
	return (_selPrinter);
}

Application*
PSMainWin::GetApplication ()
{
	return (_ps);
}

#endif		// PSMAINWIN_H
