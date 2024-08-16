/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_remote.h	1.6"
//--------------------------------------------------------------
// ps_remote.h
//--------------------------------------------------------------
#ifndef PSREMOTEACC_H
#define PSREMOTEACC_H

#include "PList.h"
#include "hostBrowse.h"

const short						ADD = 0;
const short						REMOVE = 1;
const short						REMOVEALL = 2;

/*----------------------------------------------------------------------------
 *
 */
class PSRemoteAcc : public PSDialog {
public: 
								PSRemoteAcc (Widget		parent,
											 char*		name,
											 PSMainWin*	mainWin,
											 PSPrinter*	selPrinter,
											 short		ptype,
											 action*	abi,
											 short		buttonCnt);
								~PSRemoteAcc ();

private:
	PSMainWin*					_mainWin;

public: 
	void						CreateCtrlArea ();
	void						ReadHosts ();

private:
	static void					OKCallback (Widget,
											XtPointer clientData,
											XtPointer);
	void						OK ();
	static void					resetCallback (Widget,
											   XtPointer clientData,
											   XtPointer);
	void						reset ();
	static void					cancelCallback (Widget,
												XtPointer clientData,
												XtPointer);
	void						cancel ();
	static void					helpCallback (Widget,
											  XtPointer clientData,
											  XtPointer);
	void						help ();

	void						BuildSysOrDNSForm ();
	void						BuildSysActionRC ();
	void						BuildSelSysForm ();

private:
	Widget						_ctrlArea;

	Widget						_label;

	Widget						_rowCol;
	Widget						_addBtn;
	ClientInfo					_addInfo;
	Widget						_removeBtn;
	ClientInfo					_removeInfo;
	Widget						_removeAllBtn;
	ClientInfo					_removeAllInfo;

	Widget						_sysOrDNSForm;		// Form for System/DNS Lists
	hostBrowse*					_hostsList;

	Widget						_denyAllowSysToggleBox;
	Widget						_denyToggle;
	Widget						_allowToggle;
	Widget						_selSysForm;
	PList*						_selSysList;

	PSPrinter*					_selPrinter;

private:
	static void					ChangeAllowedCB (Widget,
												 XtPointer clientData,
												 XtPointer callData);
	void						ChangeAllowed (short type);
	
	static void					DenyCB (Widget,
										XtPointer clientData,
										XtPointer callData);
	void						Deny (XmToggleButtonCallbackStruct* cbs);

	static void					AllowCB (Widget,
										 XtPointer clientData,
										 XtPointer callData);
	void						Allow (XmToggleButtonCallbackStruct* cbs);
};

#endif
