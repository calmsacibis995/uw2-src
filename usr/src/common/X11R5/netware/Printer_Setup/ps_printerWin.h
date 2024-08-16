/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_printerWin.h	1.5"
/*----------------------------------------------------------------------------
 *	ps_printerWin.h
 */
#ifndef PSPRINTERWIN_H
#define PSPRINTERWIN_H

#include "iconWin.h"

/*----------------------------------------------------------------------------
 *
 */
class PSPrinterWin : public IconWin {
public:
								PSPrinterWin (Widget		parent,
											  char*			name,
											  char**		pixmapFiles,
											  short			pixmapCnt,
											  Application*	ps,
											  void*			mainWin);
								~PSPrinterWin ();

private:
	char*						_dfltPrinter;
	void*						_mainWin;
	const char*					PRT_MGR_STR;
	Widget						_widg;

private:
	int							AppendPrinter (PRINTER* printer);

public:
	void 						GetPrinters ();
	void 						AddPrinter (char* name);
	void 						InsertPrinter (PRINTER* printer);
	void 						AddNewPrinter (PSPrinter* newPrinter);
	void			 			UpdatePixmap (PSPrinter* tmp);
	void						DeletePrinter ();
	void						SetDfltPrinter ();
	PSPrinter*					GetSelectedPrinter ();
	PSPrinter*					GetDefaultPrinter ();
	virtual void 				NotifyParentOfSel (IconObj* obj);
	virtual void			 	NotifyParentOfDblSel (IconObj* obj);
	PSPrinter*					AddNULLPrinter ();
	static void 				dtmResponseCB (Widget		w,
											   XtPointer	clientData, 
											   XEvent*		xEvent,
											   Boolean*);
	void						dtmResponse (Widget, XEvent* xEvent);
	virtual void				Transfer (Widget w, DmDnDDstInfoPtr dip);
	virtual void				DropProc (Widget	w,
										  XtPointer	clientData, 
										  XtPointer	callData);
};

#endif		// PSPRINTERWIN_H
