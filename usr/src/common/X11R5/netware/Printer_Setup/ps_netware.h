/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_netware.h	1.9"
/*----------------------------------------------------------------------------
 *	ps_netware.h
 */
#ifndef PSNETWARE_H
#define PSNETWARE_H

const short						NETWARE_BUTTON_CNT = 12;	

/*----------------------------------------------------------------------------
 *
 */
class PSMainWin;
class PSPrinter;
class PList;

class PSNetWare : public PSDialog {
public: 
								PSNetWare (Widget		parent,
										   char*		name,
										   PSMainWin*	mainWin, 
										   Boolean		newFlag,
										   PSPrinter*	selPrinter,
										   short		ptype, 
										   action*		abi,
										   short		buttonCnt);
								~PSNetWare ();

private:
	Widget						d_parent;
	Widget						d_printerList;
	PSPrinter*					_selPrinter;
	Boolean						_newFlag;
	PSMainWin*					_mainWin;
	XmString*					_printQueues;

	Widget			_ctrlArea;

	Widget			_printerNameLbl;
	Widget			_printerNameTextField;

	Widget			_netWareServerLbl;
	Widget			_netWarePrinterLbl;

	Widget			_netWareFileServerLbl;
	Widget			_netWareFileServerForm;
	PList			*_netWareFileServerList;
	char			**_nwFileServers;
	int				_nwFileServerPixmap;
	ClientInfo		_nwFileServerInfo;

	Widget			_netWarePrinterListLbl;
	Widget			_netWarePrinterForm;
	PList			*_netWarePrinterList;	
	int				_nwPrinterPixmap;
	ClientInfo		_nwPrinterInfo;

	ClientInfo		_cbData[NETWARE_BUTTON_CNT];

	Widget			_bannerPageForm;
	Widget			_bannerPageRC;
	Widget			_bannerPageLbl;
	Widget			_bannerPageYes;
	Widget			_bannerPageNo;

	Widget			_bannerOverrideForm;
	Widget			_bannerOverrideRC;
	Widget			_bannerOverrideLbl;
	Widget			_bannerOverrideYes;
	Widget			_bannerOverrideNo;

	Widget			_formFeedForm;
	Widget			_formFeedRC;
	Widget			_formFeedLbl;
	Widget			_formFeedYes;
	Widget			_formFeedNo;

	Widget			_ffOverrideForm;
	Widget			_ffOverrideRC;
	Widget			_ffOverrideLbl;
	Widget			_ffOverrideYes;
	Widget			_ffOverrideNo;

	Widget			_sep1;

	Widget			_sep2;

private: 
	void						CreateCtrlArea ();

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

private:
	void CreatePrinterNameInfo();
	void CreateNetWareInfo();
	void CreateToggleBoxes();
	void BuildYesNoToggleBox( Widget parent, Widget topWidget,
				Widget *form, 	const char *formName,
				Widget *rc, 	const char *rcName,
				Widget *lbl,	const char *lblName,const char *lblStr,
				Widget *yes,	XmString yesStr,
				Widget *no,		XmString noStr );

	static void SelectCB( Widget, XtPointer clientData, XtPointer callData );
	void Select( XmListCallbackStruct *, short type );

	void						InitValues();

	void						NetWareServerLbl (char* serverName);
	void						NetWarePrinterLbl (char* printerName);

	static void ToggleCB( Widget, XtPointer clientData, XtPointer callData );
	void Toggle( XmToggleButtonCallbackStruct *cbs, short type );
};

#endif		// PSNETWARE_H
