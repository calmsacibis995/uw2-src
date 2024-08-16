/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_unix.h	1.10"
/*----------------------------------------------------------------------------
 *	ps_unix.h
 */
#ifndef PSUNIX_H
#define PSUNIX_H

#include <Xm/Xm.h>

#include "ps_dialog.h"
#include "ps_hdr.h"
#include "action.h"

/*----------------------------------------------------------------------------
 *
 */
class PSMainWin;
class PSPrinter;
class hostBrowse;

class PSUnix : public PSDialog {
public: 
								PSUnix (Widget		parent,
										char*		name,
										PSMainWin*	mainWin, 
										Boolean		newFlag,
										PSPrinter*	selPrinter,
										short		ptype, 
										action*		abi,
										short		buttonCnt);
								~PSUnix ();

private:
	Widget						d_parent;
	Widget						d_printerName;
	Widget						d_printerList;

	PSPrinter*					_selPrinter;
	Boolean						_newFlag;
	PSMainWin*					_mainWin;

	Widget*						_cascadeButtons;
	Widget						_ctrlArea;
	Widget						_printerTypeOptionMenu;

	Widget						_osTypeSysV;
	Widget						_osTypeBSD;
	Widget						_systemLbl;
	Widget						_remPrinterTxtField;
	Widget						_sep1;

	hostBrowse*					_systemList;
	int							_systemPixmap;

	short						_initialProto;

private:
	void						InitValues ();

	void						CreateCtrlArea ();
	void						CreatePrinterNameInfo ();
	void						CreateUnixInfo ();
	void						BuildYesNoToggleBox (Widget			parent,
													 Widget			topWidget,
													 Widget*		form,
													 const char*	formName,
													 Widget*		rc,
													 const char*	rcName,
													 Widget*		lbl,
													 const char*	lblName,
													 const char*	lblStr,
													 Widget*		yes,
													 XmString		yesStr,
													 Widget*		no,
													 XmString		noStr);

private:
	static void					addCallback (Widget, XtPointer data, XtPointer);
	void						add ();
	static void					resetCallback (Widget,
											   XtPointer data,
											   XtPointer);
	void						reset ();
	static void					cancelCallback (Widget,
												XtPointer data,
												XtPointer);
	void						cancel ();
	static void					helpCallback (Widget,
											  XtPointer data,
											  XtPointer);
	void						help ();

	static void					ModelCB (Widget,
										 XtPointer clientData,
										 XtPointer callData);
	void						Model (XmListCallbackStruct *cbs);

	static void					SelectCB (XtPointer clientData);
	void						Select ();
	static void					UnselectCB (XtPointer clientData);
	void						Unselect ();

	static void					SystemVCB (Widget,
										   XtPointer clientData,
										   XtPointer callData);
	void						SystemV (XmToggleButtonCallbackStruct* state);
};

#endif
