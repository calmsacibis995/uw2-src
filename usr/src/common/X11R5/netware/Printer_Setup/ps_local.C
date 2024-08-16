#ident	"@(#)prtsetup2:ps_local.C	1.23"
/*----------------------------------------------------------------------------
 *	ps_local.c
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <stdlib.h>

#include <Xm/Xm.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleBG.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/SeparatoG.h>
#include <Xm/TextF.h>
#include <Xm/List.h>
#include <Xm/PushBG.h>
#include <Xm/PushB.h>

#include "ps_hdr.h"
#include "BasicComponent.h"
#include "ps_i18n.h"
#include "ps_mainwin.h"
#include "ps_win.h"
#include "ps_dialog.h"
#include "ps_local.h"
#include "mnem.h"

extern PrinterArray				s_supportedPrinters;

extern const char*				lpt1;
extern const char*				lpt2;
extern const char*				com1;
extern const char*				com2;

/*----------------------------------------------------------------------------
 *
 */
action							actions[] =
{
	{	TXT_OK,
		NULL,
		0,
		TXT_OKMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		NULL,
		False,
		NULL,
		NULL,
		BUTTON_OK
	},
	{	TXT_reset,
		NULL,
		0,
		TXT_resetMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		NULL,
		False,
		NULL,
		NULL,
		BUTTON_NONE
	},
	{	TXT_cancel,
		NULL,
		0,
		TXT_cancelMnem,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		NULL,
		False,
		NULL,
		NULL,
		BUTTON_CANCEL
	},
	{	TXT_help,
		NULL,
		0,
		TXT_MNEM_help,
		NULL,
		NULL,
		NULL,
		NULL,
		True,
		False,
		False,
		False,
		NULL,
		False,
		NULL,
		NULL,
		BUTTON_NONE
	},
	NULL
};

const short LOCAL_MAIL			= 0;
const short LOCAL_NO_MAIL		= 1;
const short LOCAL_BANNER		= 2;
const short LOCAL_NO_BANNER		= 3;
const short LOCAL_OVERRIDE		= 4;
const short LOCAL_NO_OVERRIDE	= 5;

const short LOCAL_300			= 6;
const short LOCAL_1200			= 7;
const short LOCAL_2400			= 8;
const short LOCAL_4800			= 9;
const short LOCAL_9600			= 10;
const short LOCAL_19200			= 11;
const short LOCAL_EVEN			= 12;
const short LOCAL_ODD			= 13;
const short LOCAL_NONE			= 14;
const short LOCAL_1				= 15;
const short LOCAL_2				= 16;
const short LOCAL_8				= 17;
const short LOCAL_7				= 18;

//--------------------------------------------------------------
// Parameters:	Widget parent - the parent (_panedWindow widget)
//		char *name - the widget name
//		char *printerName - the name of the printer
//		short ptype - the Printer_Setup dialog type	
//		ActionButtonItem *abi - list of button info
//		short buttonCnt - number of buttons in action area 
//--------------------------------------------------------------
PSLocal::PSLocal (Widget		parent,
				  char*			name,
				  PSMainWin*	mainWin,
				  Boolean		newFlag,
				  PSPrinter*	selPrinter,
				  short			ptype,
				  action*		abi,
				  short			buttonCnt) 
	   : HUNDRED_PERCENT (100),
		 TOGGLE_BUTTON_POS (50),
		 TWENTY_PERCENT (20),
		 SERIAL_TOGGLE_BUTTON_POS (30),
		 THIRTYFIVE_PERCENT (35),
		 SHCMD_MAIL ("mail %s"),
		 SHCMD_NONE ("none"),
		 PSDialog (parent, name, selPrinter, ptype, abi, buttonCnt)
{
#ifdef DEBUG
	cerr << "PSLocal (" << this << ") Constructor" << endl;
	cerr << "        (" << _selPrinter << ")" << endl;
#endif

	d_parent = parent;
	_selPrinter = selPrinter;
	_newFlag = newFlag;
	_serialInfoWin = 0;
	_showOptionsWin = 0;
	_mainWin = mainWin;
	_userName = 0;

	XtAddCallback (abi[0].w,
				   XmNactivateCallback, 
				   (XtCallbackProc)&PSLocal::addCallback,
				   this);
	XtAddCallback (abi[1].w,
				   XmNactivateCallback, 
				   (XtCallbackProc)&PSLocal::resetCallback,
				   this);
	XtAddCallback (abi[2].w,
				   XmNactivateCallback, 
				   (XtCallbackProc)&PSLocal::cancelCallback,
				   this);
	XtAddCallback (abi[3].w,
				   XmNactivateCallback, 
				   (XtCallbackProc)&PSLocal::helpCallback,
				   this);
	XtAddCallback (panedWindow (),
				   XmNhelpCallback, 
				   (XtCallbackProc)&PSLocal::helpCallback,
				   this);

	turnOffSashTraversal ();

	// Load up all the ClientInfo variables for Toggle Buttons 
	LoadClientInfo ();

    CreateCtrlArea ();

	if (s_supportedPrinters.cnt < 1) {
		if (s_supportedPrinters.warning != PT_OK) {
			new PSMsg (_w, "NoPrinters", TXT_noPrinters);
		}
		XtVaSetValues (abi[0].w, XmNsensitive, False, 0);
	}
	else {
		if (s_supportedPrinters.warning != PT_OK) {
			new PSMsg (_w, "PrintersError", TXT_warnPrinters);
		}
	}
	s_supportedPrinters.warning = PT_OK;

	_resetProto = (Protocol)_selPrinter->Proto ();
	if (!_newFlag) {
		_selPrinter->ResetPRINTER ((Protocol)_selPrinter->Proto ());
	}
	InitValues ();

	if (!_newFlag) {
		XtVaSetValues (_pNameTextField,
					   XmNeditable,
					   False,
					   XmNcursorPositionVisible,
					   False,
					   XmNnavigationType,
					   XmNONE,
					   0);	
		// Desensitize OK button if the user does not have Admin rights 
		if (/*!di->IsAdmin()*/!is_user_admin ()) {
			XtVaSetValues (abi[0].w, XmNsensitive, False, 0);
		}
		XmProcessTraversal (_printerList, XmTRAVERSE_CURRENT);
	}
	else {
		XmProcessTraversal (_pNameTextField, XmTRAVERSE_CURRENT);
	}
	registerMnemInfoOnShell(_w);
}

/*----------------------------------------------------------------------------
 *
 */
PSLocal::~PSLocal ()
{
#ifdef DEBUG1
	cerr << "PSLocal (" << this << ") Destructor" << endl;
#endif

	if (_serialInfoWin) {
		_serialInfoWin->unmanage ();
		delete (_serialInfoWin);
	}
	if (_showOptionsWin) {
		_showOptionsWin->unmanage ();
		delete (_showOptionsWin);
	}
#ifdef DIE
	if (_newFlag) {
		delete (_selPrinter);
	}
#endif
}

//--------------------------------------------------------------
//	This member function creates the control area portion for the User
//	Access dialog.
//--------------------------------------------------------------
void
PSLocal::CreateCtrlArea()
{
	XmString	str;
	Cardinal	argCnt;
	Arg			args[6];
	Widget		w = GetCtrlArea();

	// Add Printer Name Form, Label, and Text Field
	_pNameForm = XtVaCreateWidget ("PrinterNameForm", 
						xmFormWidgetClass, w,
						XmNtopAttachment, XmATTACH_FORM,
						XmNleftAttachment, XmATTACH_FORM,
						XmNrightAttachment, XmATTACH_FORM, NULL);

	str = XmStringCreateSimple (GetLocalStr (TXT_pName));
	_pName = XtVaCreateManagedWidget ("Printer Name", 
							xmLabelGadgetClass, _pNameForm, 
							XmNlabelString, str,
							XmNleftAttachment, XmATTACH_FORM, NULL);
	XmStringFree (str);

	_pNameTextField = XtVaCreateManagedWidget ("PrinterNameTextField", 
						xmTextFieldWidgetClass, _pNameForm,
						XmNtopAttachment, XmATTACH_FORM,
						XmNleftAttachment, XmATTACH_WIDGET,
						XmNleftWidget, _pName,
						XmNleftOffset, 10,
						XmNmarginHeight, 0, NULL);

	XtManageChild (_pNameForm);

	// Add Printer Form, Label, and List
	str = XmStringCreateSimple (GetLocalStr (TXT_printerModel));
	_printerLbl = XtVaCreateManagedWidget ("PrinterLabel",
						xmLabelGadgetClass, _pNameForm,
						XmNlabelString, str,
						XmNleftAttachment, XmATTACH_FORM,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, _pNameTextField, NULL);
	XmStringFree (str);

	argCnt = 0;	
	XtSetArg (args[argCnt], XmNselectionPolicy, XmSINGLE_SELECT); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNtopWidget, _printerLbl); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNrightAttachment, XmATTACH_FORM);argCnt++;
	XtSetArg (args[argCnt], XmNvisibleItemCount, 4); argCnt++;
	_printerList = XmCreateScrolledList (_pNameForm, "PrinterList",
										args, argCnt);

	for (int i = 0; i < s_supportedPrinters.cnt; i++) {
		str = XmStringCreateSimple (s_supportedPrinters.sPrinters[i].name);
		XmListAddItemUnselected (_printerList, str, 0);
		XmStringFree (str);
	}
	
	XtAddCallback (_printerList, 
				XmNsingleSelectionCallback, SelectPrinterCB, this);

	XtManageChild (_printerList);

	_sep1 = XtVaCreateManagedWidget ("Separator1",
							xmSeparatorGadgetClass, w, 
							XmNtopAttachment, XmATTACH_WIDGET,
							XmNtopWidget, _pNameForm,
							XmNtopOffset, 4,
							XmNleftAttachment, XmATTACH_FORM,
							XmNrightAttachment, XmATTACH_FORM, NULL);
						

	AddConnTypeAndPort (w);	

    // Now show the whole darn thing
    ShowDialog ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::addCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

	obj->add ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::add ()
{
	Boolean						state;
	char*						name;
	char*						other;
	int							error;

	if (_serialOtherFlag && _serialFlag) {
		if ((other = XmTextFieldGetString (_otherName)) && *other) {
			// Should "other" be checked?
			_selPrinter->Device (SERIAL, other);
		}
		else {
			_selPrinter->Device (COM1, NULL);		// SHOULD DISPLAY AN ERROR
		}
	}
	else {
		if (_parallelOtherFlag && _parallelFlag) {
			if ((other = XmTextFieldGetString (_otherName)) && *other) {
				_selPrinter->Device (PARALLEL, other);
			}
			else {
				_selPrinter->Device (LPT1, NULL);	// SHOULD DISPLAY AN ERROR
			}
		}
		else {
			if (_parallelFlag) {
				XtVaGetValues (_lpt1, XmNset, &state, 0);
				if (state) {
					_selPrinter->Device (LPT1, NULL);
				}
				else {
					_selPrinter->Device (LPT2, NULL);
				}
			}
			else {											// _serialFlag
				XtVaGetValues (_com1, XmNset, &state, 0);
				if (state) {
					_selPrinter->Device (COM1, NULL);
				}
				else {
					_selPrinter->Device (COM2, NULL);
				}
			}
		}
	}

	name = XmTextFieldGetString (_pNameTextField);
	if (!name || strlen (name) <= 0) {
		new PSError (_w, GetLocalStr (TXT_noPrinterName));
		return;
	}
	if (!legalName (name)) {
		new PSError (_w, GetLocalStr (TXT_badPrinterName));
		return;
	}
	if (_newFlag && _mainWin->findByName (name)) {
		char*					format;
		char*					tmp;

		format = GetLocalStr (TXT_printerExists);
		tmp = new char [strlen (name) + strlen (format) + 1];
		sprintf (tmp, format, name);
		new PSError (_w, tmp); 	
		delete (tmp);
		XtFree (name);
		return;
	}
	_selPrinter->Name (name);
	XtFree (name);

	if ((error = _selPrinter->AddPrinterToSys ()) != PRTADD_SUCCESS) {
		switch (error) {
		case PRTADD_NONAME:
		case PRTADD_BADNAME:
			new PSError (_w, GetLocalStr (TXT_noPrinterName));
			return;
		case PRTADD_NOTYPE:
			new PSError (_w, GetLocalStr (TXT_selectPrinterType));
			return;
		case PRTADD_NOTENABLEDACCEPT:
			new PSError (d_parent, GetLocalStr (TXT_cantEnableAccept));
			break;
		case PRTADD_NOTACCEPT:
			new PSError (d_parent, GetLocalStr (TXT_addCantAccept));
			break;
		case PRTADD_NOTENABLED:
			new PSError (d_parent, GetLocalStr (TXT_addCantEnable));
			break;
		case PRTADD_CANNOTADD:
			new PSError (d_parent, GetLocalStr (TXT_cantAddPrinter));
			unmanage ();
			d_delete = True;
			return;
		}
	}

	char*						buf;

	if (buf = new char[strlen (_selPrinter->Device ()) + 12]) {
		sprintf (buf, "chmod 600 %s", _selPrinter->Device ());
		system (buf);
		sprintf (buf, "chown lp %s", _selPrinter->Device ());
		system (buf);
		sprintf (buf, "chgrp lp %s", _selPrinter->Device ());
		system (buf);

		delete (buf);
	}

	if (_newFlag) {
		_mainWin->AddPrinter (_selPrinter);
	}

	if (_serialInfoWin) {
		_serialInfoWin->unmanage ();
	}
	if (_showOptionsWin) {
		_showOptionsWin->unmanage ();
	}

	unmanage ();
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::resetCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

	obj->reset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::reset ()
{
	XmListDeselectAllItems (_printerList);
	_selPrinter->ResetPRINTER ((Protocol)_selPrinter->Proto ());
	_selPrinter->Proto (_resetProto);
	InitValues ();
	optionReset ();
	serialReset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::cancelCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

	obj->cancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::cancel ()
{
	if (_serialInfoWin) {
		_serialInfoWin->unmanage ();
	}
	if (_showOptionsWin) {
		_showOptionsWin->unmanage ();
	}
	unmanage ();
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::helpCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

	obj->help ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::help ()
{
	if (_newFlag) {
		helpDisplay (DT_SECTION_HELP,
					 GetLocalStr (TXT_helpTitle),
					 TXT_localSect);
		return;
	}
	if (/*di->IsAdmin()*/is_user_admin ()) {
		helpDisplay (DT_SECTION_HELP,
					 GetLocalStr (TXT_helpTitle),
					 TXT_viewSect);
		return;
	}
	helpDisplay (DT_SECTION_HELP, GetLocalStr (TXT_helpTitle), TXT_changeSect);
}

//--------------------------------------------------------------
//	Member function that adds the Connection Type and Port toggle boxes.
//--------------------------------------------------------------
void
PSLocal::AddConnTypeAndPort (Widget w)
{
	XmString	str;

	BuildToggleBox (w, 					_sep1, 
					&_connTypeForm, 	"ConnTypeForm",
					&_connTypeRC, 		"ConnTypeRC", 
					&_connTypeLbl, 		TXT_connType, 
					&_serial, 			TXT_serial,
					&_parallel,			TXT_parallel,
					TOGGLE_BUTTON_POS, 	True);
	XtAddCallback (_parallel, XmNvalueChangedCallback, ParallelCB, this);

	// Add Port Toggle Box
	BuildToggleBox (w, 					_connTypeForm, 
					&_lptPortForm, 		"LPTPortForm",
					&_lptPortRC, 		"LPTPortRC", 
					&_lptPortLbl, 		TXT_port, 
					&_lpt1, 			TXT_lpt1,
					&_lpt2,				TXT_lpt2,
					TOGGLE_BUTTON_POS,  True);

	str = XmStringCreateSimple (GetLocalStr (TXT_other));
	_lptOther = XtVaCreateManagedWidget ("lpt2", 
						xmToggleButtonGadgetClass, _lptPortRC, 
						XmNlabelString, str, NULL);
	XtVaSetValues (_lpt1, XmNset, True, NULL);
	_parallelOtherFlag = FALSE;
	XmStringFree (str);

	// Now Add Callbacks for the Parallel Devices
	lpt1Info.ptr 	= this;
	lpt1Info.type 	= LPT1;
	XtAddCallback (_lpt1, XmNvalueChangedCallback, SelectDeviceCB, &lpt1Info);
	lpt2Info.ptr 	= this;
	lpt2Info.type 	= LPT2; 
	XtAddCallback (_lpt2, XmNvalueChangedCallback, SelectDeviceCB, &lpt2Info);
	parallelInfo.ptr 	= this;
	parallelInfo.type 	= PARALLEL; 
	XtAddCallback (_lptOther, XmNvalueChangedCallback, SelectDeviceCB, &parallelInfo);


	// Add Serial Port Toggle Box
	BuildToggleBox (w, 					_connTypeForm, 
					&_comPortForm, 		"ComPortForm",
					&_comPortRC, 		"ComPortRC", 
					&_comPortLbl, 		TXT_port, 
					&_com1, 			TXT_com1,
					&_com2,				TXT_com2,
					TOGGLE_BUTTON_POS, 	False);

	str = XmStringCreateSimple (GetLocalStr (TXT_other));
	_comOther = XtVaCreateManagedWidget ("comOther", 
						xmToggleButtonGadgetClass, _comPortRC, 
						XmNlabelString, str, NULL);
	XmStringFree (str);
	XtVaSetValues (_com1, XmNset, True, NULL);
	_serialOtherFlag = FALSE;
	XtManageChild (_comPortRC);


	// Add Callbacks to the Serial Device Toggle Buttons
	com1Info.ptr 	= this;
	com1Info.type 	= COM1;
	XtAddCallback (_com1, XmNvalueChangedCallback, SelectDeviceCB, &com1Info);
	com2Info.ptr 	= this;
	com2Info.type 	= COM2; 
	XtAddCallback (_com2, XmNvalueChangedCallback, SelectDeviceCB, &com2Info);
	serialInfo.ptr 	= this;
	serialInfo.type	= SERIAL; 
	XtAddCallback (_comOther, XmNvalueChangedCallback, SelectDeviceCB, &serialInfo);


	// Add Serial Configuration Radio Button
	str = XmStringCreateSimple (GetLocalStr (TXT_serialConf));
	_serialConf = XtVaCreateManagedWidget ("serialConfiguration",
						xmPushButtonWidgetClass, w,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, _lptPortForm,
						XmNleftAttachment, XmATTACH_FORM,
						XmNlabelString, str, NULL);
	XmStringFree (str);

	registerMnemInfo (_serialConf,
					  GetLocalStr (TXT_serialConfMnem),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);

	XtAddCallback (_serialConf, XmNactivateCallback, SerialConfCB, this);

	_otherName = XtVaCreateWidget ("otherName",
								   xmTextFieldWidgetClass, w,
								   XmNmarginHeight, 0,
								   XmNtopAttachment, XmATTACH_WIDGET,
								   XmNtopWidget, _lptPortForm,
								   XmNleftAttachment, XmATTACH_POSITION,
								   XmNleftPosition, TOGGLE_BUTTON_POS,
								   XmNrightAttachment, XmATTACH_FORM,
								   NULL);

	_sep2 = XtVaCreateManagedWidget ("Separator1",
						xmSeparatorGadgetClass, w, 
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, _serialConf,
						XmNleftAttachment, XmATTACH_FORM,
						XmNrightAttachment, XmATTACH_FORM,
						NULL);


	BuildToggleBox (w, 				_sep2, 
					&_sendMailForm, "SendMailForm",
					&_sendMailRC, 	"SendMailRC", 
					&_sendMailLbl, 	TXT_sendMail, 
					&_sendMailYes, 	TXT_yes,
					&_sendMailNo,	TXT_no,
					TOGGLE_BUTTON_POS, True);
	XtAddCallback (_sendMailYes, XmNvalueChangedCallback, ToggleCB, 
							&(_toggleButtonData[LOCAL_MAIL]));
	XtAddCallback (_sendMailNo, XmNvalueChangedCallback, ToggleCB, 
							&(_toggleButtonData[LOCAL_NO_MAIL]));

	BuildToggleBox (w, 					_sendMailForm, 
					&_printBannerForm, 	"PrintBannerForm",
					&_printBannerRC, 	"PrintBannerRC", 
					&_printBannerLbl, 	TXT_printBanner, 
					&_printBannerYes, 	TXT_yes,
					&_printBannerNo,	TXT_no,
					TOGGLE_BUTTON_POS,  True);
	XtAddCallback (_printBannerYes, XmNvalueChangedCallback, ToggleCB, 
							&(_toggleButtonData[LOCAL_BANNER]));
	XtAddCallback (_printBannerNo, XmNvalueChangedCallback, ToggleCB, 
							&(_toggleButtonData[LOCAL_NO_BANNER]));

	BuildToggleBox (w, 						_printBannerForm, 
					&_overrideBannerForm, 	"OverrideBannerForm",
					&_overrideBannerRC, 	"OverrideBannerRC", 
					&_overrideBannerLbl, 	TXT_overrideBanner, 
					&_overrideBannerYes, 	TXT_yes,
					&_overrideBannerNo,		TXT_no,
					TOGGLE_BUTTON_POS,		True);
	XtAddCallback (_overrideBannerYes, XmNvalueChangedCallback, ToggleCB, 
							&(_toggleButtonData[LOCAL_OVERRIDE]));
	XtAddCallback (_overrideBannerNo, XmNvalueChangedCallback, ToggleCB, 
							&(_toggleButtonData[LOCAL_NO_OVERRIDE]));


	// Add Show Other Options Radio Button
	str = XmStringCreateSimple (GetLocalStr (TXT_showOptions));
	_showOptions = XtVaCreateManagedWidget ("showOptions",
						xmPushButtonWidgetClass, w,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, _overrideBannerForm,
						XmNleftAttachment, XmATTACH_FORM,
						XmNlabelString, str, NULL);
	XmStringFree (str);
	XtAddCallback (_showOptions, XmNactivateCallback, ShowOptionsCB, this);
	registerMnemInfo (_showOptions,
					  GetLocalStr (TXT_showOptionsMnem),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);

	XtManageChild (_serialConf);
	_serialFlag = TRUE;
	_parallelFlag = FALSE;
}

//--------------------------------------------------------------
//	Member function to build a yes/no toggle box.
//--------------------------------------------------------------
void
PSLocal::BuildToggleBox (Widget parent, Widget topWidg, 
					Widget *form, 		char *formName,
					Widget *rc, 		char *rcName, 
					Widget *lbl, 		char *lblName, 
					Widget *one, 		char *oneName,
					Widget *two, 		char *twoName,
					const short pos,	Boolean	manageFlg)
{
	XmString	str;

	if (topWidg != NULL)
		*form = XtVaCreateWidget (formName, 
						xmFormWidgetClass, parent,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, topWidg,
						XmNfractionBase, HUNDRED_PERCENT,
						XmNleftAttachment, XmATTACH_FORM,
						XmNrightAttachment, XmATTACH_FORM, NULL);
	else
		*form = XtVaCreateWidget (formName, 
						xmFormWidgetClass, parent,
						XmNtopAttachment, XmATTACH_FORM,
						XmNfractionBase, HUNDRED_PERCENT,
						XmNleftAttachment, XmATTACH_FORM,
						XmNrightAttachment, XmATTACH_FORM, NULL);

	*rc = XtVaCreateWidget (rcName, 
						xmRowColumnWidgetClass, *form,
						XmNradioBehavior, True,
						XmNradioAlwaysOne, True,
						XmNorientation, XmHORIZONTAL,
						XmNtopAttachment, XmATTACH_FORM,
						XmNleftAttachment, XmATTACH_POSITION,
						XmNleftPosition, pos, NULL);

	str = XmStringCreateSimple (GetLocalStr (lblName));	
	*lbl = XtVaCreateManagedWidget (GetLocalStr (lblName), 
						xmLabelGadgetClass, *form,
						XmNlabelString, str, 
						XmNtopAttachment, XmATTACH_POSITION,
						XmNtopPosition, TWENTY_PERCENT,
						//XmNrightAttachment, XmATTACH_WIDGET,
						//XmNrightWidget, *rc, NULL);
						XmNleftAttachment, XmATTACH_POSITION,
						XmNleftPosition, 0, NULL);
	XmStringFree (str);

	str = XmStringCreateSimple (GetLocalStr (oneName));
	*one = XtVaCreateManagedWidget (GetLocalStr (oneName), 
						xmToggleButtonGadgetClass, *rc, 
						XmNlabelString, str, NULL);
	XmStringFree (str);

	str = XmStringCreateSimple (GetLocalStr (twoName));
	*two = XtVaCreateManagedWidget (GetLocalStr (twoName), 
						xmToggleButtonGadgetClass, *rc, 
						XmNlabelString, str, NULL);
	XmStringFree (str);

	if (manageFlg) {
		XtManageChild (*rc);
		XtManageChild (*form);
	}
}

//--------------------------------------------------------------
//	This function is called when the state of the _parallel button is changed.
//--------------------------------------------------------------
void
PSLocal::ParallelCB (Widget, XtPointer clientData, XtPointer callData)
{
	PSLocal*					obj = (PSLocal*)clientData;

	obj->Parallel (((XmToggleButtonCallbackStruct*)callData)->set);	
}

//--------------------------------------------------------------
//	Member function called from ParallelCB.
//--------------------------------------------------------------
void
PSLocal::Parallel (int set)
{
	if (set == True && _isParallel != True) {
		_isParallel = True;
		XtManageChild (_lptPortForm);
		XtUnmanageChild (_comPortForm); 
		XtUnmanageChild (_serialConf);
		if (_serialInfoWin != NULL)
			_serialInfoWin->unmanage();
		if (_parallelOtherFlag) {
			XtManageChild (_otherName);
		}
		else {
			XtUnmanageChild (_otherName);
		}
		_parallelFlag = TRUE;
		_serialFlag = FALSE;
		_selPrinter->Proto (0); // Parallel "Parallel"
	}
	else {
		if (set == False) {
			_isParallel = False;
			XtManageChild (_comPortForm);
			XtUnmanageChild (_lptPortForm); 
			XtManageChild (_serialConf);
			if (_serialOtherFlag) {
				XtManageChild (_otherName);
			}
			else {
				XtUnmanageChild (_otherName);
			}
			_parallelFlag = FALSE;
			_serialFlag = TRUE;
			_selPrinter->Proto (Serial); // Serial "Serial"
		}
	}
}

//--------------------------------------------------------------
//	This function is called when the state of the _serialConf button is changed.
//--------------------------------------------------------------
void
PSLocal::SerialConfCB (Widget, XtPointer clientData, XtPointer)
{
	PSLocal*					obj = (PSLocal*)clientData;

	obj->SerialConf ();
}

//--------------------------------------------------------------
//	Member function called from SerialConfCB.
//--------------------------------------------------------------
void
PSLocal::SerialConf ()
{
	Widget 						ctrlArea;
	XmString					str;

	if (_serialInfoWin) {		
//		XtManageChild (_serialInfoWin->GetDialog ());
		_serialInfoWin->ShowDialog ();
		_serialInfoWin->RaiseDialogWin ();
		return;
	}

	if (_newFlag) {
		_serialInfoWin = new PSDialog (_w,
									   GetLocalStr (TXT_serialConfTitle),
									   NULL,
									   PSWIN_NO_TYPE,
									   actions,
									   4); 
	}
	else {
		_serialInfoWin = new PSDialog (_w,
									   GetLocalStr (TXT_serialConfPropTitle),
									   NULL,
									   PSWIN_NO_TYPE,
									   actions,
									   4); 
	}

	XtAddCallback (actions[0].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSLocal::serialOKCallback,
				   this);
	XtAddCallback (actions[1].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSLocal::serialResetCallback,
				   this);
	XtAddCallback (actions[2].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSLocal::serialCancelCallback,
				   this);
	XtAddCallback (actions[3].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSLocal::helpCallback,
				   this);
	XtAddCallback (_serialInfoWin->panedWindow (),
				   XmNhelpCallback, 
				   (XtCallbackProc)&PSLocal::helpCallback,
				   this);

	_serialInfoWin->turnOffSashTraversal ();

	ctrlArea = _serialInfoWin->GetCtrlArea ();	
	BuildToggleBox (ctrlArea,				NULL, 
					&_baudRateForm, 			"baudRateForm",
					&_baudRateRC, 				"baudRateRC", 
					&_baudRateLbl, 				TXT_baudRate, 
					&_baudRate300, 				TXT_baudRate300,
					&_baudRate1200,				TXT_baudRate1200,
					SERIAL_TOGGLE_BUTTON_POS, 	True);
	str = XmStringCreateSimple (GetLocalStr (TXT_baudRate2400));
	_baudRate2400 = XtVaCreateManagedWidget ("baudRate2400", 
						xmToggleButtonGadgetClass, _baudRateRC, 
						XmNlabelString, str, NULL);
	XmStringFree (str);
	str = XmStringCreateSimple (GetLocalStr (TXT_baudRate4800));
	_baudRate4800 = XtVaCreateManagedWidget ("baudRate4800", 
						xmToggleButtonGadgetClass, _baudRateRC, 
						XmNlabelString, str, NULL);
	XmStringFree (str);
	str = XmStringCreateSimple (GetLocalStr (TXT_baudRate9600));
	_baudRate9600 = XtVaCreateManagedWidget ("baudRate9600", 
						xmToggleButtonGadgetClass, _baudRateRC, 
						XmNlabelString, str, NULL);
	XmStringFree (str);
	str = XmStringCreateSimple (GetLocalStr (TXT_baudRate19200));
	_baudRate19200 = XtVaCreateManagedWidget ("baudRate19200", 
						xmToggleButtonGadgetClass, _baudRateRC, 
						XmNlabelString, str, NULL);
	XmStringFree (str);

	XtAddCallback (_baudRate300, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_300]));
	XtAddCallback (_baudRate1200, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_1200]));
	XtAddCallback (_baudRate2400, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_2400]));
	XtAddCallback (_baudRate4800, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_4800]));
	XtAddCallback (_baudRate9600, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_9600]));
	XtAddCallback (_baudRate19200, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_19200]));

	BuildToggleBox (ctrlArea,				_baudRateForm, 
				&_parityForm, 				"parityForm",
				&_parityRC, 				"parityRC", 
				&_parityLbl, 				TXT_parity, 
				&_parityEven, 				TXT_even,
				&_parityOdd,				TXT_odd,
				SERIAL_TOGGLE_BUTTON_POS, 	True);
	str = XmStringCreateSimple (GetLocalStr (TXT_none));
	_parityNone = XtVaCreateManagedWidget ("parityNone", 
						xmToggleButtonGadgetClass, _parityRC, 
						XmNlabelString, str, NULL);
	XmStringFree (str);

	XtAddCallback (_parityEven, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_EVEN]));
	XtAddCallback (_parityOdd, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_ODD]));
	XtAddCallback (_parityNone, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_NONE]));


	BuildToggleBox (ctrlArea,				_parityForm, 
				&_stopBitsForm, 			"baudRateForm",
				&_stopBitsRC, 				"baudRateRC", 
				&_stopBitsLbl, 				TXT_stopBits, 
				&_stopBits1, 				TXT_1,
				&_stopBits2,				TXT_2,
				SERIAL_TOGGLE_BUTTON_POS,	True);
	XtAddCallback (_stopBits1, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_1]));
	XtAddCallback (_stopBits2, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_2]));

	BuildToggleBox (ctrlArea,				_stopBitsForm, 
				&_chrSizeForm, 				"charSizeForm",
				&_chrSizeRC, 				"charSizeRC", 
				&_chrSizeLbl, 				TXT_charSize, 
				&_chrSize8, 				TXT_8,
				&_chrSize7,					TXT_7,
				SERIAL_TOGGLE_BUTTON_POS,	True);
	XtAddCallback (_chrSize8, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_8]));
	XtAddCallback (_chrSize7, XmNvalueChangedCallback, ToggleCB, 
						&(_toggleButtonData[LOCAL_7]));

	InitSerialConf ();
	_serialInfoWin->ShowDialog ();
	registerMnemInfoOnShell(_serialInfoWin->GetDialog ());
}

//--------------------------------------------------------------
// Member function to build one of the other options rows.
//--------------------------------------------------------------
void
PSLocal::BuildOtherOption (Widget parent,
						   Widget *lbl,
						   char *lblTxt,
						   Widget *txt,
						   Widget *optMenu,
						   Widget *pulldown,
						   Widget option[])
{
	XmString 	str;
	Arg			args[2];

	str = XmStringCreateSimple (GetLocalStr (lblTxt));
	*lbl = XtVaCreateManagedWidget (GetLocalStr (lblTxt), 
						xmLabelGadgetClass, parent,
						XmNlabelString, str, 
						NULL);
	XmStringFree (str);

	*txt = XtVaCreateManagedWidget ("textField",
						xmTextFieldWidgetClass, parent,
						XmNmaxLength, 8, 
						XmNwidth, 8,
						XmNmarginHeight, 0,
						XmNmarginWidth, 2,
						XmNcolumns, 8, NULL);

	XtSetArg (args[0], XmNmarginHeight, 0);
	*pulldown = XmCreatePulldownMenu (parent, "pulldownMenu", args, 1);

	str = XmStringCreateSimple (GetLocalStr (TXT_in));
	XtSetArg (args[0], XmNlabelString, str);
	XtSetArg (args[1], XmNmarginHeight, 0);
	option[0] = XmCreatePushButtonGadget (*pulldown, "inchOption", args, 2);
	XmStringFree (str);

	str = XmStringCreateSimple (GetLocalStr (TXT_cm));
	XtSetArg (args[0], XmNlabelString, str);
	XtSetArg (args[1], XmNmarginHeight, 0);
	option[1] = XmCreatePushButtonGadget (*pulldown, "cmOption", args, 2);
	XmStringFree (str);

	str = XmStringCreateSimple (GetLocalStr (TXT_chars));
	XtSetArg (args[0], XmNlabelString, str);
	XtSetArg (args[1], XmNmarginHeight, 0);
	option[2] = XmCreatePushButtonGadget (*pulldown, "charOption", args, 2);
	XmStringFree (str);

	XtManageChildren (option, 3);

	XtSetArg (args[0], XmNsubMenuId, *pulldown);
	XtSetArg (args[1], XmNmarginHeight, 0);
	*optMenu = XmCreateOptionMenu (parent, "optMenu" /*GetLocalStr (TXT_in)*/, args, 2);
	
	XtManageChild (*optMenu);
}

//--------------------------------------------------------------
// Function called when a printer type is selected.
//--------------------------------------------------------------
void
PSLocal::SelectPrinterCB (Widget, XtPointer clientData, XtPointer callData) 
{
	PSLocal *obj = (PSLocal *) clientData;
	obj->SelectPrinter ((XmListCallbackStruct *)callData);
	
}

//--------------------------------------------------------------
// Member function called from SelectPrinterCB.
//--------------------------------------------------------------
void
PSLocal::SelectPrinter (XmListCallbackStruct *cbs)
{
	char *item;

	XmStringGetLtoR (cbs->item, XmSTRING_DEFAULT_CHARSET, &item);
	_selPrinter->PrinterType (item);
	XtFree (item);
}

//--------------------------------------------------------------
// Function called when a toggle button is selected
//				that determines the device.
//--------------------------------------------------------------
void
PSLocal::SelectDeviceCB (Widget, XtPointer clientData, XtPointer callData) 
{
	PSLocal *obj = (PSLocal*)((ClientInfo*)clientData)->ptr;

	obj->SelectDevice ((XmToggleButtonCallbackStruct *)callData,
						 ((ClientInfo *)clientData)->type);
}

//--------------------------------------------------------------
// Member function called from SelectPrinterCB.
//--------------------------------------------------------------
void
PSLocal::SelectDevice (XmToggleButtonCallbackStruct *cbs, short type)
{
	if (cbs->set == True) {
		switch (type) {
		case LPT1:
		case LPT2:
			_parallelOtherFlag = FALSE;
			XtUnmanageChild (_otherName);
			_selPrinter->Device (type, NULL);
			break;
		case COM1:
		case COM2:
			_serialOtherFlag = FALSE;
			XtUnmanageChild (_otherName);
			_selPrinter->Device (type, NULL);
			break;

		case PARALLEL:
			XtManageChild (_otherName);
			_parallelOtherFlag = TRUE;
			_selPrinter->Device (type, NULL);
			break; 
		case SERIAL:
			XtManageChild (_otherName);
			_serialOtherFlag = TRUE;
			_selPrinter->Device (type, NULL);
			break; 

		default:
			break;
		}
	}
}

//--------------------------------------------------------------
// This function loads up all the client info fields
//				for the toggle buttons
//--------------------------------------------------------------
void
PSLocal::LoadClientInfo()
{
	for (int i = 0; i < TOGGLE_BUTTON_CNT; i++) {
		_toggleButtonData[i].ptr = this;
		_toggleButtonData[i].type = i;
	}
}

//--------------------------------------------------------------
// Function called when a toggle button is selected
//--------------------------------------------------------------
void
PSLocal::ToggleCB (Widget, XtPointer clientData, XtPointer callData)
{
	PSLocal *obj = (PSLocal *)((ClientInfo *)clientData)->ptr;

	obj->Toggle ((XmToggleButtonCallbackStruct *)callData,
						 ((ClientInfo *)clientData)->type);
}

//--------------------------------------------------------------
// Member function called from ToggleCB
//--------------------------------------------------------------
void
PSLocal::Toggle (XmToggleButtonCallbackStruct *cbs, short type)
{
	char		*tmp;
	Boolean 	state;


	if (cbs->set == False)
		return;

	switch (type) {
	case LOCAL_MAIL:
	case LOCAL_NO_MAIL:
		if (type == LOCAL_MAIL) {
			tmp = XtMalloc (strlen (SHCMD_MAIL) + strlen(GetName()) + 1);
			sprintf (tmp, SHCMD_MAIL, GetName());
		}
		else
			tmp = strdup (SHCMD_NONE); 
		_selPrinter->ShCmd (tmp);
		XtFree (tmp);	
		break;

	case LOCAL_BANNER:
	case LOCAL_NO_BANNER:
		if (type == LOCAL_BANNER)
			state = True;
		else
			state = False;
		_selPrinter->BannerPage (state);
		break;

	case LOCAL_OVERRIDE:
	case LOCAL_NO_OVERRIDE:
		if (type == LOCAL_OVERRIDE)
			state = True;
		else
			state = False;
		_selPrinter->BannerOverride (state);
		break;

	case LOCAL_300:
		_selPrinter->BaudRate (baud300);
		break;
	case LOCAL_1200:
		_selPrinter->BaudRate (baud1200);
		break;
	case LOCAL_2400:
		_selPrinter->BaudRate (baud2400);
		break;
	case LOCAL_4800:
		_selPrinter->BaudRate (baud4800);
		break;
	case LOCAL_9600:
		_selPrinter->BaudRate (baud9600);
		break;
	case LOCAL_19200:
		_selPrinter->BaudRate (baud19200);
		break;

	case LOCAL_EVEN:
		_selPrinter->Parity (parityEven);
		break;
	case LOCAL_ODD:
		_selPrinter->Parity (parityOdd);
		break;
	case LOCAL_NONE:
		_selPrinter->Parity (parityNone);
		break;

	case LOCAL_1:
		_selPrinter->StopBits (stopBits1);
		break;
	case LOCAL_2:
		_selPrinter->StopBits (stopBits2);
		break;

	case LOCAL_8:
		_selPrinter->CharSize (charSize8);
		break;
	case LOCAL_7:
		_selPrinter->CharSize (charSize7);
		break;

	default:
		break;
	}
}

//--------------------------------------------------------------
// This function returns the name of the user running Printer_Setup.
//--------------------------------------------------------------
char*
PSLocal::GetName ()
{
	if (_userName == NULL) {
		uid_t			id;
		struct passwd 	*pwd;

		id = getuid();
		pwd = getpwuid (id);	
		_userName = strdup (pwd->pw_name);
		return (_userName);
	}
	return (_userName);
}

//--------------------------------------------------------------
//
//--------------------------------------------------------------
void
PSLocal::InitValues ()
{
	char*						tmp;
	short						sel;

	tmp = _selPrinter->Name ();
	XtVaSetValues (_pNameTextField, XmNvalue, tmp, NULL);

	if ((sel = _selPrinter->Description ()) != 0) {
		XmListSelectPos (_printerList, sel, True); 
		XmListSetPos (_printerList, sel);
	}

	tmp = _selPrinter->Device ();
	if (_selPrinter->GetProtocol () == Serial) {
		Parallel (False);
		XtVaSetValues (_serial, XmNset, True, NULL);
		XtVaSetValues (_parallel, XmNset, False, NULL);
		if (strcmp (tmp, com1) == 0 || _newFlag == True) {
			XtVaSetValues (_com1, XmNset, True, NULL);
			_selPrinter->Device (COM1, NULL);
			XtVaSetValues (_com2, XmNset, False, NULL);
			XtVaSetValues (_comOther, XmNset, False, NULL);
		}
		else if (strcmp (tmp, com2) == 0)
		{
			XtVaSetValues (_com2, XmNset, True, NULL);
			_selPrinter->Device (COM2, NULL);
			XtVaSetValues (_com1, XmNset, False, NULL);
			XtVaSetValues (_comOther, XmNset, False, NULL);
		}
		else     // Must be other
		{
			XtVaSetValues (_comOther, XmNset, True, NULL);
			XtVaSetValues (_com1, XmNset, False, NULL);
			XtVaSetValues (_com2, XmNset, False, NULL);
			_selPrinter->Device (SERIAL, tmp);
			XtVaSetValues (_otherName, XmNvalue, tmp, 0);
			XtManageChild (_otherName);
			_serialOtherFlag = TRUE;
		}
	}
	else {
		_isParallel = False;
		Parallel (True);
		XtVaSetValues (_parallel, XmNset, True, NULL);
		XtVaSetValues (_serial, XmNset, False, NULL);
		if (strcmp (tmp, lpt1) == 0 || _newFlag == True)
		{
			XtVaSetValues (_lpt1, XmNset, True, NULL);
			_selPrinter->Device (LPT1, NULL);
			XtVaSetValues (_lpt2, XmNset, False, NULL);
			XtVaSetValues (_lptOther, XmNset, False, NULL);
		}
		else if (strcmp (tmp, lpt2) == 0)
		{
			XtVaSetValues (_lpt2, XmNset, True, NULL);
			XtVaSetValues (_lpt1, XmNset, False, NULL);
			_selPrinter->Device (LPT2, NULL);
			XtVaSetValues (_lptOther, XmNset, False, NULL);
		}
		else     // Must be other
		{
			XtVaSetValues (_lptOther, XmNset, True, NULL);
			XtVaSetValues (_lpt1, XmNset, False, NULL);
			XtVaSetValues (_lpt2, XmNset, False, NULL);
			_selPrinter->Device (PARALLEL, tmp);
			XtVaSetValues (_otherName, XmNvalue, tmp, 0);
			XtManageChild (_otherName);
			_parallelOtherFlag = TRUE;
		}
	}

	if (_selPrinter->ShCmd() == True) {
		XtVaSetValues (_sendMailYes, XmNset, True, NULL);
		XtVaSetValues (_sendMailNo, XmNset, False, NULL);
	}
	else {
		XtVaSetValues (_sendMailYes, XmNset, False, NULL);
		XtVaSetValues (_sendMailNo, XmNset, True, NULL);
	}

	if (_selPrinter->BannerPage() == False) {
		XtVaSetValues (_printBannerNo, XmNset, True, NULL);
		XtVaSetValues (_printBannerYes, XmNset, False, NULL);	
	}
	else {
		XtVaSetValues (_printBannerYes, XmNset, True, NULL);	
		XtVaSetValues (_printBannerNo, XmNset, False, NULL);
	}

	if (_selPrinter->BannerOverride() == False) {
		XtVaSetValues (_overrideBannerNo, XmNset, True, NULL);
		XtVaSetValues (_overrideBannerYes, XmNset, False, NULL);	
	}
	else {
		XtVaSetValues (_overrideBannerYes, XmNset, True, NULL);	
		XtVaSetValues (_overrideBannerNo, XmNset, False, NULL);
	}
}

//--------------------------------------------------------------
//
//--------------------------------------------------------------
void
PSLocal::InitSerialConf ()
{
	// Now, get the current Serial values
	switch (_selPrinter->BaudRate()) {
	case baud300:
		XtVaSetValues (_baudRate300, XmNset, True, NULL);
		break;
	case baud1200:
		XtVaSetValues (_baudRate1200, XmNset, True, NULL);
		break;
	case baud2400:
		XtVaSetValues (_baudRate2400, XmNset, True, NULL);
		break;
	case baud4800:
		XtVaSetValues (_baudRate4800, XmNset, True, NULL);
		break;
	case baud9600:
	default:
		XtVaSetValues (_baudRate9600, XmNset, True, NULL);
		break;
	case baud19200:
		XtVaSetValues (_baudRate19200, XmNset, True, NULL);
		break;
	}
	d_initBaudRate = _selPrinter->BaudRate ();

	if (_selPrinter->CharSize() == charSize7)
		XtVaSetValues (_chrSize7, XmNset, True, NULL);
	else
		XtVaSetValues (_chrSize8, XmNset, True, NULL);
	d_initCharSize = _selPrinter->CharSize ();

	if (_selPrinter->StopBits() == stopBits1)
		XtVaSetValues (_stopBits1, XmNset, True, NULL);
	else
		XtVaSetValues (_stopBits2, XmNset, True, NULL);
	d_initStopBits = _selPrinter->StopBits ();

	switch (_selPrinter->Parity()) {
	case parityNone:
	default:
		XtVaSetValues (_parityNone, XmNset, True, NULL);
		break;

	case parityOdd:
		XtVaSetValues (_parityOdd, XmNset, True, NULL);
		break;

	case parityEven:
		XtVaSetValues (_parityEven, XmNset, True, NULL);
		break;
	}
	d_initParity = _selPrinter->Parity ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::serialOKCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

    obj->serialOK ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::serialOK ()
{
	delete (_serialInfoWin);
	_serialInfoWin = NULL;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::serialResetCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

    obj->serialReset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::serialReset ()
{
	if (!_serialInfoWin) {
		return;
	}

	XtVaSetValues (_baudRate300, XmNset, False, NULL);
	XtVaSetValues (_baudRate1200, XmNset, False, NULL);
	XtVaSetValues (_baudRate2400, XmNset, False, NULL);
	XtVaSetValues (_baudRate4800, XmNset, False, NULL);
	XtVaSetValues (_baudRate9600, XmNset, False, NULL);
	XtVaSetValues (_baudRate19200, XmNset, False, NULL);

	switch (d_initBaudRate) {
	case baud300:
		XtVaSetValues (_baudRate300, XmNset, True, NULL);
		break;
	case baud1200:
		XtVaSetValues (_baudRate1200, XmNset, True, NULL);
		break;
	case baud2400:
		XtVaSetValues (_baudRate2400, XmNset, True, NULL);
		break;
	case baud4800:
		XtVaSetValues (_baudRate4800, XmNset, True, NULL);
		break;
	case baud9600:
	default:
		XtVaSetValues (_baudRate9600, XmNset, True, NULL);
		break;
	case baud19200:
		XtVaSetValues (_baudRate19200, XmNset, True, NULL);
		break;
	}
	_selPrinter->BaudRate (d_initBaudRate);

	XtVaSetValues (_chrSize7, XmNset, False, NULL);
	XtVaSetValues (_chrSize8, XmNset, False, NULL);
	if (d_initCharSize == charSize7) {
		XtVaSetValues (_chrSize7, XmNset, True, NULL);
	}
	else {
		XtVaSetValues (_chrSize8, XmNset, True, NULL);
	}
	_selPrinter->CharSize (d_initCharSize);

	XtVaSetValues (_stopBits1, XmNset, False, NULL);
	XtVaSetValues (_stopBits2, XmNset, False, NULL);
	if (d_initStopBits == stopBits1) {
		XtVaSetValues (_stopBits1, XmNset, True, NULL);
	}
	else {
		XtVaSetValues (_stopBits2, XmNset, True, NULL);
	}
	_selPrinter->StopBits (d_initStopBits);

	XtVaSetValues (_parityNone, XmNset, False, NULL);
	XtVaSetValues (_parityOdd, XmNset, False, NULL);
	XtVaSetValues (_parityEven, XmNset, False, NULL);
	switch (d_initParity) {
	case parityNone:
	default:
		XtVaSetValues (_parityNone, XmNset, True, NULL);
		break;
	case parityOdd:
		XtVaSetValues (_parityOdd, XmNset, True, NULL);
		break;
	case parityEven:
		XtVaSetValues (_parityEven, XmNset, True, NULL);
		break;
	}
	_selPrinter->Parity (d_initParity);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::serialCancelCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

    obj->serialCancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::serialCancel ()
{
	serialReset ();
	delete (_serialInfoWin);
	_serialInfoWin = NULL;
}

//--------------------------------------------------------------
//--------------------------------------------------------------
int
PSLocal::OptionIndex (char c)
{
	if (c == 'i') {
		return (0);
	}
	if (c == 'c') {
		return (1);
	}
	return (2);
}

//--------------------------------------------------------------
//--------------------------------------------------------------
char
PSLocal::OptionCL (Widget history, Widget options[])
{
	if (history == options[0]) {
		return ('i');
	}
	if (history == options[1]) {
		return ('c');
	}
	return (' ');
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::ShowOptionsCB (Widget, XtPointer clientData, XtPointer)
{
	PSLocal*					obj = (PSLocal*)clientData;

	obj->ShowOptions ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::ShowOptions ()
{
	if (_showOptionsWin) {
		_showOptionsWin->ShowDialog ();
		_showOptionsWin->RaiseDialogWin ();
		return;
	}

	if (_newFlag) {
		_showOptionsWin = new PSDialog (_w,
										GetLocalStr (TXT_otherOptions),
										NULL,
										PSWIN_NO_TYPE,
										actions,
										4); 
	}
	else {
		_showOptionsWin = new PSDialog (_w,
										GetLocalStr (TXT_otherPropOptions),
										NULL,
										PSWIN_NO_TYPE,
										actions,
										4); 
	}

	XtAddCallback (actions[0].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSLocal::optionOKCallback,
				   this);
	XtAddCallback (actions[1].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSLocal::optionResetCallback,
				   this);
	XtAddCallback (actions[2].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSLocal::optionCancelCallback,
				   this);
	XtAddCallback (actions[3].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSLocal::helpCallback,
				   this);
	XtAddCallback (_showOptionsWin->panedWindow (),
				   XmNhelpCallback, 
				   (XtCallbackProc)&PSLocal::helpCallback,
				   this);

	_showOptionsWin->turnOffSashTraversal ();


	_showOptionsRC = XtVaCreateWidget ("showOptionsRC",
									   xmRowColumnWidgetClass,
									   _showOptionsWin->GetCtrlArea (),
									   XmNnumColumns,
									   4,
									   XmNorientation,
									   XmHORIZONTAL,
									   XmNisAligned,
									   True,
									   XmNpacking,
									   XmPACK_COLUMN,
									   XmNentryAlignment,
									   XmALIGNMENT_BEGINNING, 
									   0);

	BuildOtherOption (_showOptionsRC,
					  &_pageLengthLbl,
					  TXT_pageLength,
					  &_pageLengthTxt,
					  &_pageLengthOptMenu,
					  &_pageLengthPulldown,
					  _pageLengthOptions);
	BuildOtherOption (_showOptionsRC,
					  &_pageWidthLbl,
					  TXT_pageWidth,
					  &_pageWidthTxt,
					  &_pageWidthOptMenu,
					  &_pageWidthPulldown,
					  _pageWidthOptions);
	BuildOtherOption (_showOptionsRC,
					  &_charPitchLbl,
					  TXT_charPitch,
					  &_charPitchTxt,
					  &_charPitchOptMenu,
					  &_charPitchPulldown,
					  _charPitchOptions);
	BuildOtherOption (_showOptionsRC,
					  &_linePitchLbl,
					  TXT_linePitch,
					  &_linePitchTxt,
					  &_linePitchOptMenu,
					  &_linePitchPulldown,
					  _linePitchOptions);
	XtManageChild (_showOptionsRC);
	_showOptionsWin->ShowDialog ();

	optionReset ();
	registerMnemInfoOnShell(_showOptionsWin->GetDialog ());
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::optionOKCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

    obj->optionOK ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::optionOK ()
{
	SCALED						pl;
	Widget						history;
	Arg							args[16];
	char*						text;

	XtSetArg (args[0], XmNvalue, &text);
	XtGetValues (_pageLengthTxt, args, 1);
	pl.val = (double)atoi (text);
	XtSetArg (args[0], XmNmenuHistory, &history);
	XtGetValues (_pageLengthOptMenu, args, 1);
	pl.sc = OptionCL (history, _pageLengthOptions);
	_selPrinter->PageLength (pl);

	XtSetArg (args[0], XmNvalue, &text);
	XtGetValues (_pageWidthTxt, args, 1);
	pl.val = (double)atoi (text);
	XtSetArg (args[0], XmNmenuHistory, &history);
	XtGetValues (_pageWidthOptMenu, args, 1);
	pl.sc = OptionCL (history, _pageWidthOptions);
	_selPrinter->PageWidth (pl);

	XtSetArg (args[0], XmNvalue, &text);
	XtGetValues (_charPitchTxt, args, 1);
	pl.val = (double)atoi (text);
	XtSetArg (args[0], XmNmenuHistory, &history);
	XtGetValues (_charPitchOptMenu, args, 1);
	pl.sc = OptionCL (history, _charPitchOptions);
	_selPrinter->CharPitch (pl);

	XtSetArg (args[0], XmNvalue, &text);
	XtGetValues (_linePitchTxt, args, 1);
	pl.val = (double)atoi (text);
	XtSetArg (args[0], XmNmenuHistory, &history);
	XtGetValues (_linePitchOptMenu, args, 1);
	pl.sc = OptionCL (history, _linePitchOptions);
	_selPrinter->LinePitch (pl);

	delete (_showOptionsWin);
	_showOptionsWin = 0;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::optionResetCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

    obj->optionReset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::optionReset ()
{
	SCALED						pl;
	Arg							args[16];
	char						buf[16];

	if (!_showOptionsWin) {
		return;
	}
	pl = _selPrinter->PageLength ();
	sprintf (buf, "%.0f", pl.val);
	XtSetArg (args[0], XmNvalue, buf);
	XtSetValues (_pageLengthTxt, args, 1);
	XtSetArg (args[0], XmNmenuHistory, _pageLengthOptions[OptionIndex (pl.sc)]);
	XtSetValues (_pageLengthOptMenu, args, 1);

	pl = _selPrinter->PageWidth ();
	sprintf (buf, "%.0f", pl.val);
	XtSetArg (args[0], XmNvalue, buf);
	XtSetValues (_pageWidthTxt, args, 1);
	XtSetArg (args[0], XmNmenuHistory, _pageWidthOptions[OptionIndex (pl.sc)]);
	XtSetValues (_pageWidthOptMenu, args, 1);

	pl = _selPrinter->CharPitch ();
	sprintf (buf, "%.0f", pl.val);
	XtSetArg (args[0], XmNvalue, buf);
	XtSetValues (_charPitchTxt, args, 1);
	XtSetArg (args[0], XmNmenuHistory, _charPitchOptions[OptionIndex (pl.sc)]);
	XtSetValues (_charPitchOptMenu, args, 1);

	pl = _selPrinter->LinePitch ();
	sprintf (buf, "%.0f", pl.val);
	XtSetArg (args[0], XmNvalue, buf);
	XtSetValues (_linePitchTxt, args, 1);
	XtSetArg (args[0], XmNmenuHistory, _linePitchOptions[OptionIndex (pl.sc)]);
	XtSetValues (_linePitchOptMenu, args, 1);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::optionCancelCallback (Widget, XtPointer data, XtPointer)
{
    PSLocal*					obj = (PSLocal*)data;

    obj->optionCancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSLocal::optionCancel ()
{
	optionReset ();
	delete (_showOptionsWin);
	_showOptionsWin = 0;
}

