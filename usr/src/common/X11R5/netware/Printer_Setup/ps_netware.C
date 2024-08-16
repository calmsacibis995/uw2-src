#ident	"@(#)prtsetup2:ps_netware.C	1.22"
/*----------------------------------------------------------------------------
 *	ps_netware.c
 */

#include <iostream.h>

#include <nw/nwcalls.h>						// KLUDGE

#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/List.h>

#include "ps_hdr.h"
#include "BasicComponent.h"
#include "ps_i18n.h"
#include "ps_msg.h"
#include "ps_mainwin.h"
#include "ps_win.h"
#include "ps_dialog.h"
#include "ps_netware.h"
#include "bindery.h"
#include "PList.h"
#include "mnem.h"

const short						NETWARE_FILESERVER = 0;
const short						NETWARE_PRINTER = 1;

const short						NETWARE_BANNER = 4;
const short						NETWARE_NO_BANNER = 5;
const short						NETWARE_OVERRIDE = 6;
const short						NETWARE_NO_OVERRIDE = 7;

const short						NETWARE_FORMFEED = 8;
const short						NETWARE_NO_FORMFEED = 9;
const short						NETWARE_FF_OVERRIDE = 10;
const short						NETWARE_NO_FF_OVERRIDE = 11;

extern PrinterArray				s_supportedPrinters;
extern char*					fileServerPixmap;
extern char*					printerPixmap;

//--------------------------------------------------------------
// Parameters:	Widget parent - the parent  (_panedWindow widget)
//		char *name - the widget name
//		char *printerName - the name of the printer
//		short ptype - the Printer_Setup dialog type	
//		ActionButtonItem *abi - list of button info
//		short buttonCnt - number of buttons in action area 
//--------------------------------------------------------------
PSNetWare::PSNetWare (Widget		parent,
					  char*			name,
					  PSMainWin*	mainWin,
					  Boolean		newFlag,
					  PSPrinter*	selPrinter,
					  short			ptype, 
					  action*		abi,
					  short			buttonCnt) 
		 : PSDialog (parent, name, selPrinter, ptype, abi, buttonCnt)
{
	d_parent = parent;
	_selPrinter = selPrinter;
	_newFlag = newFlag;
	_mainWin = mainWin;
	_printQueues = NULL;

	XtAddCallback (abi[0].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSNetWare::addCallback,
				   this);
	XtAddCallback (abi[1].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSNetWare::resetCallback,
				   this);
	XtAddCallback (abi[2].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSNetWare::cancelCallback,
				   this);
	XtAddCallback (abi[3].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSNetWare::helpCallback,
				   this);
	XtAddCallback (panedWindow (),
				   XmNhelpCallback,
				   (XtCallbackProc)&PSNetWare::helpCallback,
				   this);

	for (int i = 0; i < NETWARE_BUTTON_CNT; i++) {
		_cbData[i].ptr = this;
		_cbData[i].type = i;
	}

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
		_selPrinter->ResetPRINTER (NUC);
	}
	InitValues ();

	if (_newFlag) {
		_selPrinter->Proto (NUC);
		XmProcessTraversal (_printerNameTextField, XmTRAVERSE_CURRENT);
	}
	else {
		XtVaSetValues (_printerNameTextField,
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
	registerMnemInfoOnShell(_w);
}

/*----------------------------------------------------------------------------
 *
 */
PSNetWare::~PSNetWare ()
{
	if (_printQueues) {
		FreeArrayOfXmStrings (_printQueues);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::CreateCtrlArea ()
{
	_ctrlArea = GetCtrlArea ();

	XtVaSetValues (_ctrlArea, XmNfractionBase, 2, NULL);

	CreatePrinterNameInfo ();

	_sep1 = XtVaCreateManagedWidget ("Separator1",
									 xmSeparatorWidgetClass, _ctrlArea,
									 XmNtopAttachment, XmATTACH_WIDGET,
									 XmNtopWidget, d_printerList,
									 XmNleftAttachment, XmATTACH_FORM,
									 XmNrightAttachment, XmATTACH_FORM,
									 0);

	CreateNetWareInfo  ();

	_sep2 = XtVaCreateManagedWidget ("Separator2",
									 xmSeparatorWidgetClass, _ctrlArea,
									 XmNtopAttachment, XmATTACH_WIDGET,
									 XmNtopWidget, _netWareFileServerForm,
									 XmNleftAttachment, XmATTACH_FORM,
									 XmNrightAttachment, XmATTACH_FORM,
									 0);
	XtVaSetValues (_netWarePrinterForm, 
				   XmNbottomAttachment, XmATTACH_WIDGET,
				   XmNbottomWidget, _sep2,
				   0);

	CreateToggleBoxes ();

	ShowDialog ();
}

//--------------------------------------------------------------
// creates the widgets to allow the 
//entry of a Printer Name and selection of a printer type.
//--------------------------------------------------------------
void
PSNetWare::CreatePrinterNameInfo ()
{
	XmString					str;
	Widget						printerLabel;
	Arg							args[6];
	Cardinal					argCnt;

	str = XmStringCreateSimple (GetLocalStr (TXT_pName));
	_printerNameLbl =  XtVaCreateManagedWidget ("printerNameLbl",
								xmLabelWidgetClass, _ctrlArea,
								XmNlabelString, str,
								XmNtopAttachment, XmATTACH_FORM,
								XmNleftAttachment, XmATTACH_FORM, NULL); 
	XmStringFree (str);

	_printerNameTextField = XtVaCreateManagedWidget ("printerNameTextField",
								xmTextFieldWidgetClass, _ctrlArea,
								XmNcolumns, 16,
								XmNmarginHeight, 0,
								XmNtopAttachment, XmATTACH_FORM,
								XmNleftAttachment, XmATTACH_WIDGET,
								XmNleftWidget, _printerNameLbl,
								XmNleftOffset, 10,
								NULL);

	str = XmStringCreateSimple (GetLocalStr (TXT_printerModel));
	printerLabel = XtVaCreateManagedWidget ("PrinterLabel",
						xmLabelGadgetClass, _ctrlArea,
						XmNlabelString, str,
						XmNleftAttachment, XmATTACH_FORM,
						XmNtopAttachment, XmATTACH_WIDGET,
						XmNtopWidget, _printerNameTextField, NULL);
	XmStringFree (str);

	argCnt = 0;	
	XtSetArg (args[argCnt], XmNselectionPolicy, XmSINGLE_SELECT); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNtopWidget, printerLabel); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNrightAttachment, XmATTACH_FORM);argCnt++;
	XtSetArg (args[argCnt], XmNvisibleItemCount, 4); argCnt++;
	d_printerList = XmCreateScrolledList (_ctrlArea,
										  "PrinterList",
										  args,
										  argCnt);

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
PSNetWare::ModelCB (Widget, XtPointer clientData, XtPointer callData) 
{
	PSNetWare*					obj = (PSNetWare*)clientData;

	obj->Model ((XmListCallbackStruct*)callData);
}

//--------------------------------------------------------------
// Member function called from SelectPrinterCB.
//--------------------------------------------------------------
void
PSNetWare::Model (XmListCallbackStruct *cbs)
{
	char*						item;

	XmStringGetLtoR (cbs->item, XmSTRING_DEFAULT_CHARSET, &item);
	_selPrinter->PrinterType (item, TRUE);
	XtFree (item);
}

//--------------------------------------------------------------
// This member function creates the widgets to display
//	and allow the user to select a NetWare server and printer.
//--------------------------------------------------------------
void
PSNetWare::CreateNetWareInfo ()
{
	XmString	str;
	char		**tmp;

	str = XmStringCreateSimple (GetLocalStr (TXT_netwareServer));
	_netWareServerLbl =  XtVaCreateManagedWidget ("netWareServerLbl",
								xmLabelWidgetClass, _ctrlArea,
								XmNlabelString, str,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, _sep1,
								XmNleftAttachment, XmATTACH_FORM, NULL); 
	XmStringFree (str);

	str = XmStringCreateSimple (GetLocalStr (TXT_netwarePrinter));
	_netWarePrinterLbl =  XtVaCreateManagedWidget ("netWareServerLbl",
								xmLabelWidgetClass, _ctrlArea,
								XmNlabelString, str,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, _netWareServerLbl,
								XmNleftAttachment, XmATTACH_FORM, NULL); 
	XmStringFree (str);

	str = XmStringCreateSimple (GetLocalStr (TXT_fileServers));
	_netWareFileServerLbl =  XtVaCreateManagedWidget ("netWareFileServerLbl",
									xmLabelWidgetClass, _ctrlArea,
									XmNlabelString, str,
									XmNtopAttachment, XmATTACH_WIDGET,
									XmNtopWidget, _netWarePrinterLbl,
									XmNleftAttachment, XmATTACH_POSITION, 
									XmNleftPosition, 0, NULL); 
	XmStringFree (str);

	_netWareFileServerForm = XtVaCreateWidget ("netWareFileServerForm",
								xmFormWidgetClass, _ctrlArea,
								XmNwidth, 180,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, _netWareFileServerLbl,
								XmNleftAttachment, XmATTACH_POSITION, 
								XmNleftPosition, 0, NULL);

	_netWareFileServerList = new PList (_netWareFileServerForm, 
										"netWareFileServerList");
	_nwFileServerPixmap = _netWareFileServerList->UW_AddPixmap (fileServerPixmap);

	XtVaSetValues (_netWareFileServerList->UW_GetListWidget (),
				   XmNvisibleItemCount, 7,
				   0);

	if (!(_nwFileServers = GetNetWareFileServers())) {
		new PSError (d_parent, GetLocalStr (TXT_openConn));
	}
	else {
		for (tmp = _nwFileServers; *tmp != NULL; tmp++) {
			str = XmStringCreateSimple (*tmp);
			_netWareFileServerList->UW_ListAddItemUnselected (str, 0, 
							_nwFileServerPixmap);
			XmStringFree (str);
		}
	}

	_nwFileServerInfo.ptr = this;
	_nwFileServerInfo.type = NETWARE_FILESERVER;
//	_netWareFileServerList->UW_RegisterDblCallback (SelectCB, &_nwFileServerInfo);
	_netWareFileServerList->UW_RegisterSingleCallback (SelectCB, &_nwFileServerInfo);


	XtManageChild (_netWareFileServerForm);

	str = XmStringCreateSimple (GetLocalStr (TXT_printers));
	_netWarePrinterListLbl =  XtVaCreateManagedWidget ("netWarePrinterListLbl",
									xmLabelWidgetClass, _ctrlArea,
									XmNlabelString, str,
									XmNtopAttachment, XmATTACH_WIDGET,
									XmNtopWidget, _netWarePrinterLbl,
									XmNleftAttachment, XmATTACH_POSITION, 
									XmNleftPosition, 1, 
									NULL); 
	XmStringFree (str);

	_netWarePrinterForm = XtVaCreateWidget ("netWarePrinterForm",
								xmFormWidgetClass, _ctrlArea,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, _netWarePrinterListLbl,
								XmNleftAttachment, XmATTACH_POSITION,
								XmNleftPosition, 1, 
								XmNrightAttachment, XmATTACH_FORM, NULL);

	_netWarePrinterList = new PList (_netWarePrinterForm, 
										"netWarePrinterList");
	_nwPrinterPixmap = _netWarePrinterList->UW_AddPixmap (printerPixmap);

	XtVaSetValues (_netWarePrinterList->UW_GetListWidget (),
				   XmNvisibleItemCount, 7,
				   0);

	XtVaSetValues (_netWareFileServerForm, 
						XmNrightAttachment, XmATTACH_POSITION,
						XmNrightPosition, 1, NULL);
	_nwPrinterInfo.ptr 		= this;
	_nwPrinterInfo.type 	= NETWARE_PRINTER;
//	_netWarePrinterList->UW_RegisterDblCallback (SelectCB, &_nwPrinterInfo);
	_netWarePrinterList->UW_RegisterSingleCallback (SelectCB, &_nwPrinterInfo);
	XtManageChild (_netWareFileServerForm);
	XtManageChild (_netWarePrinterForm);
}

//--------------------------------------------------------------
// This member function creates the widgets for the 
//	Yes/No Toggle Boxes near the bottom of the dialog.
//--------------------------------------------------------------
void
PSNetWare::CreateToggleBoxes()
{
	XmString 	yesStr;
	XmString	noStr;

	yesStr 	= XmStringCreateSimple (GetLocalStr (TXT_yes));
	noStr 	= XmStringCreateSimple (GetLocalStr (TXT_no));

	BuildYesNoToggleBox (_ctrlArea, _sep2, 
				&_bannerPageForm, 		"printBannerPageForm",
				&_bannerPageRC, 		"printBannerPageRC",
				&_bannerPageLbl,		"printBannerPageLbl", TXT_printBanner,
				&_bannerPageYes,		yesStr,
				&_bannerPageNo,			noStr);
	XtAddCallback (_bannerPageYes, XmNvalueChangedCallback, ToggleCB,
					& (_cbData[NETWARE_BANNER]));
	XtAddCallback (_bannerPageNo, XmNvalueChangedCallback, ToggleCB,
					& (_cbData[NETWARE_NO_BANNER]));

	BuildYesNoToggleBox (_ctrlArea, _bannerPageForm,
				&_bannerOverrideForm,	"bannerOverrideForm",
				&_bannerOverrideRC,		"bannerOverrideRC",
				&_bannerOverrideLbl,	"bannerOverrideLbl", TXT_overrideBanner,
				&_bannerOverrideYes,	yesStr,
				&_bannerOverrideNo, 	noStr);
	XtAddCallback (_bannerOverrideYes, XmNvalueChangedCallback, ToggleCB,
					& (_cbData[NETWARE_OVERRIDE]));
	XtAddCallback (_bannerOverrideNo, XmNvalueChangedCallback, ToggleCB,
					& (_cbData[NETWARE_NO_OVERRIDE]));


	BuildYesNoToggleBox (_ctrlArea, _bannerOverrideForm,
				&_formFeedForm, 		"formFeedForm",
				&_formFeedRC,			"formFeedRC",
				&_formFeedLbl,			"formFeedLbl", TXT_sendFF,
				&_formFeedYes,			yesStr,
				&_formFeedNo,			noStr);
	XtAddCallback (_formFeedYes, XmNvalueChangedCallback, ToggleCB,
					& (_cbData[NETWARE_FORMFEED]));
	XtAddCallback (_formFeedNo, XmNvalueChangedCallback, ToggleCB,
					& (_cbData[NETWARE_NO_FORMFEED]));


	BuildYesNoToggleBox (_ctrlArea, _formFeedForm,
				&_ffOverrideForm,		"formFeedOverrideForm",
				&_ffOverrideRC,			"formFeedOverrideRC",
				&_ffOverrideLbl,		"formFeedLbl", TXT_overrideFF,
				&_ffOverrideYes,		yesStr,
				&_ffOverrideNo,			noStr);
	XtAddCallback (_ffOverrideYes, XmNvalueChangedCallback, ToggleCB,
					& (_cbData[NETWARE_FF_OVERRIDE]));
	XtAddCallback (_ffOverrideNo, XmNvalueChangedCallback, ToggleCB,
					& (_cbData[NETWARE_NO_FF_OVERRIDE]));

	XmStringFree (yesStr);
	XmStringFree (noStr);
}

//--------------------------------------------------------------
// This member function creates a single yes/no toggle box.
//--------------------------------------------------------------
void
PSNetWare::BuildYesNoToggleBox (Widget parent,	Widget topWidget,
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

	str = XmStringCreateSimple (GetLocalStr ((char *)lblStr));
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
PSNetWare::addCallback (Widget, XtPointer data, XtPointer)
{
	PSNetWare*					obj = (PSNetWare*)data;

	obj->add ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::add ()
{
	char*						name;
	int							error;

	name = XmTextFieldGetString (_printerNameTextField);
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
			new PSError (_w, GetLocalStr (TXT_noFileServer));
			return;
		case PRTADD_NOREMOTEPRINTER:
			new PSError (_w, GetLocalStr (TXT_noRemotePrinter));
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
PSNetWare::resetCallback (Widget, XtPointer data, XtPointer)
{
	PSNetWare*					obj = (PSNetWare*)data;

	obj->reset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::reset ()
{
	XmListDeselectAllItems (d_printerList);
	_selPrinter->ResetPRINTER (NUC);

	InitValues ();

	NetWareServerLbl (_selPrinter->RemoteSystem ());
	NetWarePrinterLbl (_selPrinter->RemoteQueue ());
	_netWarePrinterList->UW_ListDeleteAllItems ();
	XtVaSetValues (_netWareFileServerList->UW_GetListWidget (),
				   XmNselectedItemCount,
				   0,
				   XmNselectedItems,
				   NULL,
				   NULL);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::cancelCallback (Widget, XtPointer data, XtPointer)
{
	PSNetWare*					obj = (PSNetWare*)data;

	obj->cancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::cancel ()
{
	if (_newFlag == True) {
		delete (_selPrinter);
	}
	XtUnmanageChild (_w);
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::helpCallback (Widget, XtPointer data, XtPointer)
{
	PSNetWare*					obj = (PSNetWare*)data;

	obj->help ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::help ()
{
	if (_newFlag) {
		helpDisplay (DT_SECTION_HELP,
					 GetLocalStr (TXT_helpTitle),
					 TXT_netwareSect);
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
// Function called when a member of the FileServer
//	List is selected with either a single or double click.
//--------------------------------------------------------------
void
PSNetWare::SelectCB (Widget, XtPointer clientData, XtPointer callData)
{
	ClientInfo*					info = (ClientInfo*)clientData;
	PSNetWare*					obj = (PSNetWare*)info->ptr;

	obj->Select ((XmListCallbackStruct*)callData, info->type);
}

//--------------------------------------------------------------
//	XmListCallbackStruct *cbs - list callback struct
//				short type - whether an item was selected from 
//							the file server or printer list.
//--------------------------------------------------------------
void
PSNetWare::Select (XmListCallbackStruct* cbs, short type)
{
	extern void					setWatchCursor (Window window, Boolean flush);
	extern void					resetCursor (Window window, Boolean flush);
	char*						item;
	short						itemAddCnt;

	setWatchCursor (XtWindow (_w), True);

	if (type == NETWARE_FILESERVER) {
		if (XmStringGetLtoR (cbs->item, XmSTRING_DEFAULT_CHARSET, &item)) {
			if (_printQueues) {
				FreeArrayOfXmStrings (_printQueues);
				_printQueues = 0;
			}
			_netWarePrinterList->UW_ListDeleteAllItems ();
			NetWarePrinterLbl (0);
			_selPrinter->RemoteQueue (0);

			if (_selPrinter->RemoteSystem () &&
								!strcmp (item, _selPrinter->RemoteSystem ())) {
				_selPrinter->RemoteSystem (0);
				NetWareServerLbl (0);
			}
			else {
				_selPrinter->RemoteSystem (0);
				NetWareServerLbl (0);
			  	if (IsAuthenticated (item)) {
					if (!(_printQueues = GetNetWarePrintQueues (item,
																&itemAddCnt))) {
						new PSError (_w, GetLocalStr (TXT_openConn));
					}
					else {
						if (itemAddCnt > 0) {
							_netWarePrinterList->UW_ListAddItems (_printQueues,
																  itemAddCnt,
																  0,
																  0); 
						}
						_selPrinter->RemoteSystem (item);
						NetWareServerLbl (item);
					}
				}
				else {
					// ERROR ???????????????????????
				}
			}
			XtFree (item);
		}
		else {
			// ERROR ???????????????????????
		}
	}
	else {	// type == NETWARE_PRINTER
		if (XmStringGetLtoR (cbs->item, XmSTRING_DEFAULT_CHARSET, &item)) {
			if (_selPrinter->RemoteQueue () &&
								!strcmp (item, _selPrinter->RemoteQueue ())) {
				_selPrinter->RemoteQueue (0);
				NetWarePrinterLbl (0);
			}
			else {
				_selPrinter->RemoteQueue (item);
				NetWarePrinterLbl (item);
			}
			XtFree (item);
		}
	}
	resetCursor (XtWindow (_w), True);
}

/*----------------------------------------------------------------------------
 *	Reads the current values from _selPrinter and updates the display
 */
void
PSNetWare::InitValues ()
{
	char*						tmp;
	int							sel;

	tmp = _selPrinter->Name ();
	XtVaSetValues (_printerNameTextField, XmNvalue, tmp, 0);

	if ((sel = _selPrinter->Description ()) != 0) {
		XmListSelectPos (d_printerList, sel, True); 
		XmListSetPos (d_printerList, sel);
	}

	if (tmp = _selPrinter->RemoteSystem ()) {
		NetWareServerLbl (tmp);
	}
	if (tmp = _selPrinter->RemoteQueue ()) {
		NetWarePrinterLbl (tmp);
	}

	if (!_selPrinter->BannerPage ()) {
		XtVaSetValues (_bannerPageNo, XmNset, True, NULL);
		XtVaSetValues (_bannerPageYes, XmNset, False, NULL);
	}
	else {
		XtVaSetValues (_bannerPageNo, XmNset, False, NULL);
		XtVaSetValues (_bannerPageYes, XmNset, True, NULL);
	}

	if (!_selPrinter->BannerOverride ()) {
		XtVaSetValues (_bannerOverrideNo, XmNset, True, NULL);
		XtVaSetValues (_bannerOverrideYes, XmNset, False, NULL);
	}
	else {
		XtVaSetValues (_bannerOverrideNo, XmNset, False, NULL);
		XtVaSetValues (_bannerOverrideYes, XmNset, True, NULL);
	}

	if (!_selPrinter->FormFeed ()) {
		XtVaSetValues (_formFeedNo, XmNset, True, NULL);
		XtVaSetValues (_formFeedYes, XmNset, False, NULL);
	}
	else {
		XtVaSetValues (_formFeedNo, XmNset, False, NULL);
		XtVaSetValues (_formFeedYes, XmNset, True, NULL);
	}

	if (!_selPrinter->FormFeedOverride ()) {
		XtVaSetValues (_ffOverrideNo, XmNset, True, NULL);
		XtVaSetValues (_ffOverrideYes, XmNset, False, NULL);
	}
	else {
		XtVaSetValues (_ffOverrideNo, XmNset, False, NULL);
		XtVaSetValues (_ffOverrideYes, XmNset, True, NULL);
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::NetWareServerLbl (char* serverName)
{
	char*						tmp;
	XmString 					str;

	if (!serverName) {
		tmp = XtMalloc (strlen (GetLocalStr (TXT_netwareServer)) + 2);
		sprintf (tmp, "%s", GetLocalStr (TXT_netwareServer));
	}
	else {
		tmp = XtMalloc (strlen (GetLocalStr (TXT_netwareServer)) + 
						strlen (serverName) + 1 + 1);
		sprintf (tmp, "%s %s", GetLocalStr (TXT_netwareServer), serverName);
	}
	str = XmStringCreateSimple (tmp);
	XtVaSetValues (_netWareServerLbl, XmNlabelString, str, NULL);  
	XtFree (tmp);
	XmStringFree (str);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::NetWarePrinterLbl (char* printerName)
{
	char*						tmp;
	XmString					str;

	if (!printerName) {
		tmp = XtMalloc (strlen (GetLocalStr (TXT_netwarePrinter)) + 1 + 1);
		sprintf (tmp, "%s", GetLocalStr (TXT_netwarePrinter));
	}
	else {
		tmp = XtMalloc (strlen (GetLocalStr (TXT_netwarePrinter)) +
						strlen (printerName) + 1 + 1);
		sprintf (tmp, "%s %s", GetLocalStr (TXT_netwarePrinter), printerName);
	}
	str = XmStringCreateSimple (tmp);
	XtVaSetValues (_netWarePrinterLbl, XmNlabelString, str, NULL);
	XtFree (tmp);
	XmStringFree (str); 
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::ToggleCB (Widget, XtPointer clientData, XtPointer callData)
{
	PSNetWare*					obj;

	obj = (PSNetWare*)((ClientInfo*)clientData)->ptr;
	obj->Toggle ((XmToggleButtonCallbackStruct*)callData,
				 ((ClientInfo*)clientData)->type);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSNetWare::Toggle (XmToggleButtonCallbackStruct* cbs, short type)
{
	Boolean						state = False;

	if (cbs->set == False) {
		return;
	}

	switch (type) {
	case NETWARE_BANNER:
		state = True;	
	case NETWARE_NO_BANNER:
		_selPrinter->BannerPage (state);
		break;

	case NETWARE_OVERRIDE:
		state = True;
	case NETWARE_NO_OVERRIDE:
		_selPrinter->BannerOverride (state);
		break;

	case NETWARE_FORMFEED:
		state = True;
	case NETWARE_NO_FORMFEED:
		_selPrinter->FormFeed (state);
		break;

	case NETWARE_FF_OVERRIDE:
		state = True;
	case NETWARE_NO_FF_OVERRIDE:
		_selPrinter->FormFeedOverride (state);
		break;

	default:
		break;
	} 
}

