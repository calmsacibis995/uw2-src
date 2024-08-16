#ident	"@(#)prtsetup2:ps_unix.C	1.22"
/*----------------------------------------------------------------------------
 *	ps_unix.c
 */

#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/List.h>

#include "ps_i18n.h"
#include "ps_mainwin.h"
#include "ps_win.h"
#include "hostBrowse.h"
#include "ps_unix.h"
#include "mnem.h"

extern PrinterArray				s_supportedPrinters;

/*----------------------------------------------------------------------------
 *
 */
PSUnix::PSUnix (Widget		parent,
				char*		name,
				PSMainWin*	mainWin, 
				Boolean		newFlag,
				PSPrinter*	selPrinter,
				short		ptype, 
				action*		abi,
				short		buttonCnt) 
	  : PSDialog (parent, name, selPrinter, ptype, abi, buttonCnt)
{
	d_parent = parent;
//	_di = di;

	_selPrinter = selPrinter;
	_newFlag = newFlag;
	_mainWin = mainWin;
	_cascadeButtons = NULL;

	XtAddCallback (abi[0].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSUnix::addCallback,
				   this);
	XtAddCallback (abi[1].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSUnix::resetCallback,
				   this);
	XtAddCallback (abi[2].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSUnix::cancelCallback,
				   this);
	XtAddCallback (abi[3].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSUnix::helpCallback,
				   this);
	XtAddCallback (panedWindow (),
				   XmNhelpCallback,
				   (XtCallbackProc)&PSUnix::helpCallback,
				   this);

	turnOffSashTraversal ();

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

	if (!_newFlag) {
		_selPrinter->ResetPRINTER ((Protocol)_selPrinter->Proto ());
	}
	InitValues ();

	if (!_newFlag) {
		XtVaSetValues (d_printerName,
					   XmNeditable,
					   False,
					   XmNcursorPositionVisible,
					   False,
					   XmNnavigationType,
					   XmNONE,
					   0);
		if (/*!di->IsAdmin()*/!is_user_admin ()) {
			XtVaSetValues (abi[0].w, XmNsensitive, False, 0);
		}
	}
	else {
		XmProcessTraversal (d_printerName, XmTRAVERSE_CURRENT);
	}
	registerMnemInfoOnShell(_w);
}

/*----------------------------------------------------------------------------
 *
 */
PSUnix::~PSUnix ()
{
	XtFree ((char*)_cascadeButtons);
}

/*----------------------------------------------------------------------------
 *	creates the control area portion for the PSUnix dialog.
 */
void
PSUnix::CreateCtrlArea ()
{
	_ctrlArea = GetCtrlArea ();

	CreatePrinterNameInfo ();

	_sep1 = XtVaCreateManagedWidget ("Separator1",
									 xmSeparatorWidgetClass, _ctrlArea,
									 XmNtopAttachment, XmATTACH_WIDGET,
									 XmNtopWidget, d_printerList,
									 XmNleftAttachment, XmATTACH_FORM,
									 XmNrightAttachment, XmATTACH_FORM,
									 NULL);

	CreateUnixInfo ();

	ShowDialog ();
}

//--------------------------------------------------------------
//	This member function creates the widgets to allow the 
//	entry of a Printer Name and selection of a printer type.
//--------------------------------------------------------------
void
PSUnix::CreatePrinterNameInfo ()
{
	XmString					str;
	Widget						nameLabel;
	Widget						printerLabel;
	Arg							args[6];
	Cardinal					argCnt;

	str = XmStringCreateSimple (GetLocalStr (TXT_pName));
	nameLabel =  XtVaCreateManagedWidget ("printerNameLbl",
										  xmLabelWidgetClass, _ctrlArea,
										  XmNlabelString, str,
										  XmNtopAttachment, XmATTACH_FORM,
										  XmNleftAttachment, XmATTACH_FORM,
										  NULL); 
	XmStringFree (str);

	d_printerName = XtVaCreateManagedWidget ("printerNameTextField",
											 xmTextFieldWidgetClass, _ctrlArea,
											 XmNcolumns, 16,
											 XmNmarginHeight, 0,
											 XmNtopAttachment, XmATTACH_FORM,
											 XmNleftAttachment, XmATTACH_WIDGET,
											 XmNleftWidget, nameLabel,
										     XmNleftOffset, 10,
											 0);

	// Add Printer Form, Label, and List
	str = XmStringCreateSimple (GetLocalStr (TXT_printerModel));
	printerLabel = XtVaCreateManagedWidget ("PrinterLabel",
						xmLabelGadgetClass, _ctrlArea,
						XmNlabelString, str,
						XmNleftAttachment, XmATTACH_FORM,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, d_printerName, NULL);
	XmStringFree (str);

	argCnt = 0;	
	XtSetArg (args[argCnt], XmNselectionPolicy, XmSINGLE_SELECT); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNtopWidget, printerLabel); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNrightAttachment, XmATTACH_FORM);argCnt++;
	XtSetArg (args[argCnt], XmNvisibleItemCount, 4); argCnt++;
	d_printerList = XmCreateScrolledList (_ctrlArea, "PrinterList",
										args, argCnt);

	for (int i = 0; i < s_supportedPrinters.cnt; i++) {
		str = XmStringCreateSimple (s_supportedPrinters.sPrinters[i].name);
		XmListAddItemUnselected (d_printerList, str, 0);
		XmStringFree (str);
	}
	
	XtAddCallback (d_printerList, XmNsingleSelectionCallback, ModelCB, this);

	XtManageChild (d_printerList);
}

//--------------------------------------------------------------
// Function called when a printer type is selected.
//--------------------------------------------------------------
void
PSUnix::ModelCB (Widget, XtPointer clientData, XtPointer callData) 
{
	PSUnix*						obj = (PSUnix*)clientData;

	obj->Model ((XmListCallbackStruct*)callData);
}

//--------------------------------------------------------------
// Member function called from SelectPrinterCB.
//--------------------------------------------------------------
void
PSUnix::Model (XmListCallbackStruct *cbs)
{
	char*						item;

	XmStringGetLtoR (cbs->item, XmSTRING_DEFAULT_CHARSET, &item);
	_selPrinter->PrinterType (item, TRUE);
	XtFree (item);
}

//--------------------------------------------------------------
//	This member function creates the widgets to display
//	and allow the user to select a Unix box and
//	enter the name of a printer.
//--------------------------------------------------------------
void
PSUnix::CreateUnixInfo ()
{
	XmString					str;
	XmString					str2;
	Application*				ps;
	XtAppContext				context;
	Widget						osTypeForm;
	Widget						osTypeRC;
	Widget						osTypeLbl;
	Widget						systemForm;
	Widget						remPrinterLbl;

	str = XmStringCreateSimple (GetLocalStr (TXT_systemV));
	str2 = XmStringCreateSimple (GetLocalStr (TXT_bsd));

	BuildYesNoToggleBox (_ctrlArea,		_sep1, 
						 &osTypeForm, 	"osTypeForm",
						 &osTypeRC, 	"osTypeRC",
						 &osTypeLbl,	"osTypeLbl", 	TXT_sysType,
						 &_osTypeSysV,	str,
						 &_osTypeBSD,	str2);
	XtAddCallback (_osTypeSysV, XmNvalueChangedCallback, SystemVCB, this);

	XmStringFree (str);
	XmStringFree (str2);

	Widget						lbl;

	str = XmStringCreateSimple (GetLocalStr (TXT_remoteSystem));
	lbl =  XtVaCreateManagedWidget ("systemLbl",
								xmLabelWidgetClass, _ctrlArea,
								XmNlabelString, str,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, osTypeForm,
								XmNleftAttachment, XmATTACH_FORM, NULL); 
	_systemLbl = XtVaCreateManagedWidget ("pathText",
								xmTextFieldWidgetClass,
								_ctrlArea,
								XmNcolumns, 32,
								XmNmarginHeight, 0,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, osTypeForm,
								XmNleftAttachment, XmATTACH_WIDGET,
								XmNleftWidget, lbl,
								NULL); 

	XmStringFree (str);

	systemForm = XtVaCreateWidget ("systemForm",
								   xmFormWidgetClass, _ctrlArea,
								   XmNtopAttachment, XmATTACH_WIDGET,
								   XmNtopWidget, _systemLbl,
								   XmNleftAttachment, XmATTACH_POSITION, 
								   XmNleftPosition, 0, 
								   XmNrightAttachment, XmATTACH_FORM,
								   NULL);

	ps = _mainWin->GetApplication ();
	context = ps->appContext ();
	_systemList = new hostBrowse (context, systemForm, 2);
	_systemList->setSelectCallback (SelectCB, this);
	_systemList->setUnselectCallback (UnselectCB, this);

	XtManageChild (systemForm);

	str = XmStringCreateSimple (GetLocalStr (TXT_remotePrinter));
	remPrinterLbl =  XtVaCreateManagedWidget ("remotePrinterNameLbl",
								xmLabelWidgetClass, _ctrlArea,
								XmNlabelString, str,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, systemForm,
								XmNleftAttachment, XmATTACH_FORM,
								NULL); 
	XmStringFree (str);

	_remPrinterTxtField = XtVaCreateManagedWidget ("remPrinterTxtField",
								xmTextFieldWidgetClass, _ctrlArea,
								XmNcolumns, 16,
								XmNmarginHeight, 0,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, systemForm,
								XmNleftAttachment, XmATTACH_WIDGET,
								XmNleftWidget, remPrinterLbl, NULL);
}

//--------------------------------------------------------------
// This member function creates a single yes/no toggle box.
//--------------------------------------------------------------
void
PSUnix::BuildYesNoToggleBox (Widget parent,	Widget topWidget,
					Widget *form, 	const char *formName,
					Widget *rc,		const char *rcName,
					Widget *lbl, 	const char *lblName, const char *lblStr,
					Widget *yes,	XmString yesStr,
					Widget *no, 	XmString noStr)
{
	XmString	str;

	*form = XtVaCreateWidget (formName,
						xmFormWidgetClass, parent,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, topWidget,
						XmNfractionBase, 100,
						XmNleftAttachment, XmATTACH_FORM, 
						XmNrightAttachment, XmATTACH_FORM, NULL);

	*rc = XtVaCreateWidget (rcName,
						xmRowColumnWidgetClass, *form,
						XmNradioBehavior, True,
						XmNradioAlwaysOne, True,
						XmNorientation, XmHORIZONTAL,
						XmNtopAttachment, XmATTACH_FORM,
						XmNleftAttachment, XmATTACH_POSITION,
						XmNleftPosition, 60, NULL);

	str = XmStringCreateSimple (GetLocalStr ( (char *)lblStr));
	*lbl = XtVaCreateManagedWidget (lblName,
						xmLabelWidgetClass, *form,
						XmNlabelString, str,
						XmNtopAttachment, XmATTACH_POSITION,
						XmNtopPosition, 20,
						XmNleftAttachment, XmATTACH_POSITION,
						XmNleftPosition, 0, NULL);
	XmStringFree (str);

	*yes = XtVaCreateManagedWidget ("yes",
						xmToggleButtonWidgetClass, *rc,
						XmNset, True,
						XmNlabelString, yesStr, NULL); 

	*no = XtVaCreateManagedWidget ("no",
						xmToggleButtonWidgetClass, *rc,
						XmNlabelString, noStr, NULL); 
						
	XtManageChild (*rc);
	XtManageChild (*form);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::addCallback (Widget, XtPointer data, XtPointer)
{
	PSUnix*						obj = (PSUnix*)data;

	obj->add ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::add ()
{
	char*						name;
	char*						system;
	char*						remPrinterName;
	int							error;

	name = XmTextFieldGetString (d_printerName);
	if (!name || strlen (name) <= 0) {
		new PSError (_w, GetLocalStr (TXT_noPrinterName));
		return;
	}
	if (!legalName (name)) {
		new PSError (_w, GetLocalStr (TXT_badPrinterName));
		return;
	}
	if (_newFlag && _mainWin->findByName (name)) {
		char*				format;
		char*				tmp;

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

	system = XmTextFieldGetString (_systemLbl);
	if (!system || strlen (system) <= 0) {
		new PSError (_w, GetLocalStr (TXT_noRemoteSys));
		return;
	}
	if (!_systemList->checkAddr (system)) {
		new PSError (_w, GetLocalStr (TXT_badRemoteSys));
		return;
	}
	_selPrinter->RemoteSystem (system);

	remPrinterName = XmTextFieldGetString (_remPrinterTxtField);
	if (remPrinterName && strlen (remPrinterName) != 0) {
		_selPrinter->RemoteQueue (remPrinterName);
		XtFree (remPrinterName);
	}

	if ((error = _selPrinter->AddPrinterToSys ()) != PRTADD_SUCCESS) {
		switch (error) {
		case PRTADD_NONAME:
		case PRTADD_BADNAME:
			new PSError (_w, GetLocalStr (TXT_noPrinterName));
			return;
		case PRTADD_NOTYPE:
			new PSError (_w, GetLocalStr (TXT_selectPrinterType));
			return;
		case PRTADD_NOREMOTESYSTEM:
			new PSError (_w, GetLocalStr (TXT_noRemoteSys));
			return;
		case PRTADD_NOREMOTEPRINTER:
			new PSError (_w, GetLocalStr (TXT_noUnixPrinter));
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
			XtUnmanageChild (_w);
			d_delete = True;
			return;
		}
	}

	if (_newFlag) {
		_mainWin->AddPrinter (_selPrinter);
	}

	XtUnmanageChild (_w);
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::resetCallback (Widget, XtPointer data, XtPointer)
{
	PSUnix*						obj = (PSUnix*)data;

	obj->reset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::reset ()
{
	_systemList->unselect (0);
	XmListDeselectAllItems (d_printerList);
	_selPrinter->ResetPRINTER ((Protocol)_initialProto);
	InitValues ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::cancelCallback (Widget, XtPointer data, XtPointer)
{
	PSUnix*						obj = (PSUnix*)data;

	obj->cancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::cancel ()
{
	if (_newFlag) {
		delete (_selPrinter);
	}
	XtUnmanageChild (_w);
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::helpCallback (Widget, XtPointer data, XtPointer)
{
	PSUnix*						obj = (PSUnix*)data;

	obj->help ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::help ()
{
	if (_newFlag) {
		helpDisplay (DT_SECTION_HELP,
					 GetLocalStr (TXT_helpTitle),
					 TXT_unixSect);
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

/*----------------------------------------------------------------------------
 *	Function called when an entry in the _systemList is selected
 */
void
PSUnix::SelectCB (XtPointer clientData)
{
	PSUnix*						obj = (PSUnix*)clientData;

	obj->Select ();
}

/*----------------------------------------------------------------------------
 *	Member function called when an entry in the _systemList is selected.
 */
void
PSUnix::Select ()
{
	XtVaSetValues (_systemLbl, XmNvalue, (char*)_systemList->getPathName (), 0);
}

/*----------------------------------------------------------------------------
 *	Function called when an entry in the _systemList is selected
 */
void
PSUnix::UnselectCB (XtPointer clientData)
{
	PSUnix*						obj = (PSUnix*)clientData;

	obj->Unselect ();
}

/*----------------------------------------------------------------------------
 *	Member function called when an entry in the _systemList is selected.
 */
void
PSUnix::Unselect ()
{
	XtVaSetValues (_systemLbl, XmNvalue, 0, 0);
}

/*----------------------------------------------------------------------------
 *	reads the current values from _selPrinter and updates the display
 */
void
PSUnix::InitValues ()
{
	char*						tmp;
	short						sel;

	tmp = _selPrinter->Name ();
	XtVaSetValues (d_printerName, XmNvalue, tmp, NULL);

	if ((sel = _selPrinter->Description ()) != 0) {
		XmListSelectPos (d_printerList, sel, True); 
		XmListSetPos (d_printerList, sel);
	}

	if (!_newFlag) {
		_initialProto = _selPrinter->Proto ();
	}
	else {
		_initialProto = SysV;
	}
	_selPrinter->Proto (_initialProto);

	if (_initialProto == SysV) {
		XtVaSetValues (_osTypeSysV, XmNset, True, NULL);
		XtVaSetValues (_osTypeBSD, XmNset, False, NULL);
	}	
	else {
		XtVaSetValues (_osTypeSysV, XmNset, False, NULL);
		XtVaSetValues (_osTypeBSD, XmNset, True, NULL);
	}

	XtVaSetValues (_systemLbl, XmNvalue, _selPrinter->RemoteSystem (), 0);

	XtVaSetValues (_remPrinterTxtField,
				   XmNvalue,
				   _selPrinter->RemoteQueue (),
				   NULL);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::SystemVCB (Widget, XtPointer clientData, XtPointer callData)
{
	PSUnix*						obj = (PSUnix*)clientData;

	obj->SystemV ((XmToggleButtonCallbackStruct*)callData);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUnix::SystemV (XmToggleButtonCallbackStruct* state)
{
	if (state->set) {
		_selPrinter->Proto (SysV);			// Save and set in Add
	}										// or do a Get in Add
	else {
		_selPrinter->Proto (BSD);
	}
}  

