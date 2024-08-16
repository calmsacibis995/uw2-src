/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_ctrlprinter.h	1.3"
/*----------------------------------------------------------------------------
 *	ps_ctrlprinter.h
 */
#ifndef PSCTRLPRINTER_H
#define PSCTRLPRINTER_H

/*----------------------------------------------------------------------------
 *
 */
class PSCtrlPrinter : public PSDialog {
public: 
								PSCtrlPrinter (Widget		parent,
											   char*		name,
											   PSPrinter*	selPrinter,
											   short		ptype,
											   action*		abi,
											   short		buttonCnt);
								~PSCtrlPrinter ();

private:
	Widget 						_status;
	Widget 						_newReqLbl;
	Widget 						_printerLbl;
	Widget 						_newReqsRC;
	Widget 						_enabledRC;
	Widget 						_enabled;
	Widget 						_disabled;
	Widget 						_accept;
	Widget 						_reject;
	Boolean 					_enabledState;
	Boolean 					_newReqsState;

private:
	static void					AcceptCallback (Widget,
												XtPointer data,
												XtPointer callData);
	void						Accept (XmToggleButtonCallbackStruct* state);
	static void					EnabledCallback (Widget,
												 XtPointer data,
												 XtPointer callData);
	void						Enabled (XmToggleButtonCallbackStruct* state);

	static void					ApplyCallback (Widget,
											   XtPointer data,
											   XtPointer);
	void						Apply ();
	static void					ResetCallback (Widget,
											   XtPointer data,
											   XtPointer);
	void						Reset ();
	static void					CancelCallback (Widget,
												XtPointer data,
												XtPointer);
	void						Cancel ();
	static void					HelpCallback (Widget,
											  XtPointer data,
											  XtPointer);
	void						Help ();

	void						CreateCtrlArea ();
	void						SetCtrls ();
	void						SetButtons ();
};

#endif
