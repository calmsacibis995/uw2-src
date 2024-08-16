/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)prtsetup2:ps_local.h	1.12"
/*----------------------------------------------------------------------------
 *	ps_local.h
 *
 *	This file defines the PSLocal class.
 */
#ifndef PSLOCAL_H
#define PSLOCAL_H

#include "PList.h"

#define TOGGLE_BUTTON_CNT 19

/*----------------------------------------------------------------------------
 *
 */
class PSLocal : public PSDialog {
public: 
								PSLocal (Widget		parent,
										 char*		name,
										 PSMainWin*	mainWin,
										 Boolean	newFlag,
										 PSPrinter*	selPrinter,
										 short		ptype,
										 action*	abi,
										 short		buttonCnt);
								~PSLocal ();

private:
	short						d_initBaudRate;
	short						d_initCharSize;
	short						d_initParity;
	short						d_initStopBits;
	Widget						d_parent;

public: 							// Public interface methods

private:							// Private methods
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

private:							// Private methods for the Options Dialog
	void						optionOK ();
	static void					optionOKCallback (Widget,
												  XtPointer data,
												  XtPointer);
	static void					optionResetCallback (Widget,
													 XtPointer data,
													 XtPointer);
	void						optionReset ();
	static void					optionCancelCallback (Widget,
													  XtPointer data,
													  XtPointer);
	void						optionCancel ();

private:							// Private methods for the Serial Dialog
	void						serialOK ();
	static void					serialOKCallback (Widget,
												  XtPointer data,
												  XtPointer);
	static void					serialResetCallback (Widget,
													 XtPointer data,
													 XtPointer);
	void						serialReset ();
	static void					serialCancelCallback (Widget,
													  XtPointer data,
													  XtPointer);
	void						serialCancel ();

private:
	const short		HUNDRED_PERCENT;
	const short		TOGGLE_BUTTON_POS;
	const short		TWENTY_PERCENT;
	const short		THIRTYFIVE_PERCENT;
	const short		SERIAL_TOGGLE_BUTTON_POS;
	const 	char	*SHCMD_MAIL;
	const 	char	*SHCMD_NONE;

	PSMainWin		*_mainWin;

	PSPrinter 		*_selPrinter;
	Protocol		_resetProto;
	Boolean			_newFlag;
	Boolean			_isParallel;
	Boolean			_parallelFlag;
	Boolean			_serialFlag;
	Boolean			_parallelOtherFlag;
	Boolean			_serialOtherFlag;
	ClientInfo		_toggleButtonData[TOGGLE_BUTTON_CNT];

	char			*_userName;

	PSDialog		*_serialInfoWin;
	PSDialog		*_showOptionsWin;

	Widget			_pNameForm;
	Widget			_pName;
	Widget			_pNameTextField;

	Widget			_printerLbl;
	Widget			_printerList;

	Widget			_sep1;
	Widget			_sep2;

	Widget			_connTypeForm;
	Widget			_connTypeRC;
	Widget			_connTypeLbl;
	Widget			_serial;
	Widget			_parallel;

	Widget			_lptPortForm;
	Widget			_lptPortRC;
	Widget			_lptPortLbl;
	Widget			_lpt1;
	Widget			_lpt2;
	Widget			_lptOther;
	ClientInfo		lpt1Info;
	ClientInfo		lpt2Info;
	ClientInfo		parallelInfo;

	Widget			_comPortForm;
	Widget			_comPortRC;
	Widget			_comPortLbl;
	Widget			_com1;
	Widget			_com2;
	Widget			_comOther;
	ClientInfo		com1Info;
	ClientInfo		com2Info;
	ClientInfo		serialInfo;

	Widget			_serialConf;
	Widget			_otherName;
	Widget			_showOptions;

	Widget			_sendMailForm;
	Widget			_sendMailRC;
	Widget			_sendMailLbl;
	Widget			_sendMailYes;
	Widget			_sendMailNo;

	Widget			_printBannerForm;
	Widget			_printBannerRC;
	Widget			_printBannerLbl;
	Widget			_printBannerYes;
	Widget			_printBannerNo;

	Widget			_overrideBannerForm;
	Widget			_overrideBannerRC;
	Widget			_overrideBannerLbl;
	Widget			_overrideBannerYes;
	Widget			_overrideBannerNo;

	Widget			_baudRateForm;
	Widget			_baudRateRC;
	Widget			_baudRateLbl;
	Widget			_baudRate300;
	Widget			_baudRate1200;
	Widget			_baudRate2400;
	Widget			_baudRate4800;
	Widget			_baudRate9600;
	Widget			_baudRate19200;

	Widget			_parityForm;
	Widget			_parityRC;
	Widget			_parityLbl;
	Widget			_parityEven;
	Widget			_parityOdd;
	Widget			_parityNone;

	Widget			_stopBitsForm;
	Widget			_stopBitsRC;
	Widget			_stopBitsLbl;
	Widget			_stopBits1;
	Widget			_stopBits2;

	Widget			_chrSizeForm;
	Widget			_chrSizeRC;
	Widget			_chrSizeLbl;
	Widget			_chrSize8;
	Widget			_chrSize7;

	Widget			_showOptionsRC;

	Widget			_pageLengthLbl;
	Widget			_pageLengthTxt;
	Widget			_pageLengthOptMenu;
	Widget			_pageLengthPulldown;
	Widget			_pageLengthOptions[3];

	Widget			_pageWidthLbl;
	Widget			_pageWidthTxt;
	Widget			_pageWidthOptMenu;
	Widget			_pageWidthPulldown;
	Widget			_pageWidthOptions[3];

	Widget			_charPitchLbl;
	Widget			_charPitchTxt;
	Widget			_charPitchOptMenu;
	Widget			_charPitchPulldown;
	Widget			_charPitchOptions[3];

	Widget			_linePitchLbl;
	Widget			_linePitchTxt;
	Widget			_linePitchOptMenu;
	Widget			_linePitchPulldown;
	Widget			_linePitchOptions[3];

private:
	static void ParallelCB (Widget, XtPointer clientData, XtPointer callData);
	void Parallel (int set);
	static void SerialConfCB (Widget, XtPointer clientData, XtPointer);
	static void ShowOptionsCB (Widget, XtPointer clientData, XtPointer);
	void						ShowOptions ();
	void						SerialConf ();

	void AddConnTypeAndPort (Widget w);
	void BuildToggleBox (Widget parent, Widget topWidg,
					Widget *form, 	char *formName,
					Widget *rc, 	char *rcName,
					Widget *lbl,	char *lblName,
					Widget *one,	char *oneName,
					Widget *two, 	char *twoName,
					const short pos, Boolean manageFlg);
	void BuildOtherOption (Widget parent, Widget *lbl, char *lblTxt,
							Widget *txt, Widget *optMenu, Widget *pulldown,
							Widget options[]);

	static void SelectPrinterCB (Widget, XtPointer clientData, XtPointer callData);
	void SelectPrinter (XmListCallbackStruct *cbs);

	static void SelectDeviceCB (Widget, XtPointer clientData, XtPointer callData);
	void SelectDevice (XmToggleButtonCallbackStruct *cbs, short type);

	static void ToggleCB (Widget, XtPointer clientData, XtPointer callData);
	void Toggle (XmToggleButtonCallbackStruct *cbs, short type);

	void LoadClientInfo();

	char *GetName();

	void InitValues();
	void InitSerialConf();
	int OptionIndex (char c);
	char OptionCL (Widget history, Widget options[]);
};

#endif
