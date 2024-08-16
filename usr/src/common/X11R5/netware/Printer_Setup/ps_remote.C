#ident	"@(#)prtsetup2:ps_remote.C	1.15"
/*----------------------------------------------------------------------------
 *	ps_remote.c
 */

#include <Xm/Xm.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/PushBG.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/List.h>

#include "ps_hdr.h"
#include "BasicComponent.h"
#include "ps_i18n.h"
#include "ps_xpmbutton.h"
#include "ps_mainwin.h"
#include "ps_win.h"
#include "ps_dialog.h"
#include "ps_remote.h"
#include "ps_msg.h"
#include "mnem.h"

//extern "C" {
#include "lpsys.h"
//}

char*							systemPixmap = { "/snowbird/ps/computer.xpm" };
//char*							systemPixmap = { "/usr/X/lib/pixmaps/ptr.system16" };

/*----------------------------------------------------------------------------
 *
 */
PSRemoteAcc::PSRemoteAcc (Widget		parent,
						  char*			name,
						  PSMainWin*	mainWin,
						  PSPrinter*	selPrinter,
						  short			ptype,
						  action*		abi,
						  short			buttonCnt) 
		   : PSDialog (parent, name, selPrinter, ptype, abi, buttonCnt)
{
	_selPrinter = selPrinter;
	_ctrlArea = GetCtrlArea ();
	_mainWin = mainWin;

    XtAddCallback (abi[0].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSRemoteAcc::OKCallback,
				   this);
    XtAddCallback (abi[1].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSRemoteAcc::resetCallback,
				   this);
    XtAddCallback (abi[2].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSRemoteAcc::cancelCallback,
				   this);
    XtAddCallback (abi[3].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSRemoteAcc::helpCallback,
				   this);
    XtAddCallback (panedWindow (),
				   XmNhelpCallback,
				   (XtCallbackProc)&PSRemoteAcc::helpCallback,
				   this);

	turnOffSashTraversal ();

	if (/*!di->IsAdmin()*/!is_user_admin ()) {
		XtVaSetValues (abi[0].w, XmNsensitive, False, 0);
	}

    CreateCtrlArea ();
	registerMnemInfoOnShell(_w);
}

/*----------------------------------------------------------------------------
 *
 */
PSRemoteAcc::~PSRemoteAcc()
{
}

//--------------------------------------------------------------
// creates the control area portion for the Remote Access dialog.
//--------------------------------------------------------------
void
PSRemoteAcc::CreateCtrlArea ()
{
	Arg							args[10];
	Cardinal					argCnt;
	XmString					str;
	char*						tmpStr;

	_addInfo.ptr = this;
	_removeInfo.ptr = this;
	_removeAllInfo.ptr = this;

	argCnt = 0;
	tmpStr = new char[strlen (GetLocalStr (TXT_remoteacc_label)) +
					  strlen (GetPrinterName ()) + 1];
	sprintf (tmpStr, GetLocalStr (TXT_remoteacc_label), GetPrinterName ());
	str = XmStringCreateSimple (tmpStr);	
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_FORM); argCnt++;
	_label = XmCreateLabelGadget (_ctrlArea, "remoteAccessLabel", args, argCnt);
	XmStringFree (str);
	delete (tmpStr);
	XtManageChild (_label);

	BuildSysOrDNSForm ();
	BuildSysActionRC ();
	BuildSelSysForm ();
	XtVaSetValues (_sysOrDNSForm, XmNrightAttachment, XmATTACH_WIDGET,
				   XmNtopAttachment, XmATTACH_WIDGET,
				   XmNtopWidget, _denyAllowSysToggleBox,
				   XmNrightWidget, _rowCol,
				   0);
	XtVaSetValues (_rowCol, XmNrightAttachment, XmATTACH_WIDGET,
				   XmNtopAttachment, XmATTACH_WIDGET,
				   XmNtopWidget, _denyAllowSysToggleBox,
				   XmNrightWidget, _denyAllowSysToggleBox,
				   0);

    ShowDialog ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSRemoteAcc::OKCallback (Widget, XtPointer clientData, XtPointer)
{
	PSRemoteAcc*				obj = (PSRemoteAcc*)clientData;

	obj->OK ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSRemoteAcc::OK ()
{
	PrintUser*					tmp;

	_selPrinter->ResetState (_selPrinter->CurrentState ());
	_selPrinter->UpdateSysArray ();
	_selPrinter->UpdateSystemAccessLists ();
	tmp = _selPrinter->GetFirstSys ();
	while (tmp != NULL) {
		tmp->InitState (tmp->ResetState ());
		tmp = _selPrinter->GetNextSys ();
	}
	unmanage ();
//	XtUnmanageChild (_w);
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSRemoteAcc::resetCallback (Widget, XtPointer clientData, XtPointer)
{
	PSRemoteAcc*				obj = (PSRemoteAcc*)clientData;

	obj->reset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSRemoteAcc::reset ()
{
	PrintUser*					tmp;
	XmString					str;

	_selPrinter->CurrentState (_selPrinter->ResetState());
	if (_selPrinter->ResetState() == S_DENY) {
		XtVaSetValues (_allowToggle, XmNset, True, NULL);
		XtVaSetValues (_denyToggle, XmNset, False, NULL);
		_selPrinter->CurrentState (S_DENY);
		_selSysList->UW_ListDeleteAllItems();
		tmp = _selPrinter->GetFirstSys();
		while (tmp) {
			tmp->InitState (tmp->ResetState ());
			if (tmp->DenyState () == S_DENY) {
				str = XmStringCreateLocalized (tmp->Name ());
				_selSysList->UW_ListAddItemUnselected (str, 0, 0);
				XmStringFree (str);
			}
			tmp = _selPrinter->GetNextSys ();
		}	
	}
	else {
		XtVaSetValues (_denyToggle, XmNset, True, NULL);
		XtVaSetValues (_allowToggle, XmNset, False, NULL);
		_selPrinter->CurrentState (S_ALLOW);
		_selSysList->UW_ListDeleteAllItems ();
		tmp = _selPrinter->GetFirstSys ();
		while (tmp) {
			tmp->InitState (tmp->ResetState ());
			if (tmp->ResetState () == S_ALLOW) {
				str = XmStringCreateLocalized (tmp->Name ());
				_selSysList->UW_ListAddItemUnselected (str, 0, 0);
				XmStringFree (str);
			}
			tmp = _selPrinter->GetNextSys ();
		}	
	}
}

/*----------------------------------------------------------------------------
 *
 */
void
PSRemoteAcc::cancelCallback (Widget, XtPointer clientData, XtPointer)
{
	PSRemoteAcc*				obj = (PSRemoteAcc*)clientData;

	obj->cancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSRemoteAcc::cancel ()
{
	PrintUser*					tmp;
	AllowState					state;

	_selPrinter->CurrentState (_selPrinter->ResetState ());
	tmp = _selPrinter->GetFirstSys ();
	while (tmp) {
		if ((state = tmp->ResetState ()) == S_NO_STATE) {
			_selPrinter->DelSys (tmp->Name ());
		}
		else {
			tmp->InitState (state);
		}
		tmp = _selPrinter->GetNextSys ();
	}
	if (_selPrinter->ResetState () == S_DENY) {
		XtVaSetValues (_allowToggle, XmNset, True, 0);
		XtVaSetValues (_denyToggle, XmNset, False, 0);
	}
	else {
		XtVaSetValues (_denyToggle, XmNset, True, 0);
		XtVaSetValues (_allowToggle, XmNset, False, 0);
	}
//	XtUnmanageChild (_w);
	unmanage ();
	d_delete = True;
}

/*----------------------------------------------------------------------------
 *
 */
void
PSRemoteAcc::helpCallback (Widget, XtPointer clientData, XtPointer)
{
	PSRemoteAcc*				obj = (PSRemoteAcc*)clientData;

	obj->help ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSRemoteAcc::help ()
{
	helpDisplay (DT_SECTION_HELP, GetLocalStr (TXT_helpTitle), TXT_remoteSect);
}

//--------------------------------------------------------------
// This function builds the DNS and Systems Form, and
//				Lists.
//--------------------------------------------------------------
void
PSRemoteAcc::BuildSysOrDNSForm ()
{
	Application*				ps;
	XtAppContext				context;

	_sysOrDNSForm = XtVaCreateWidget ("sysOrDNSForm", 
									  xmFormWidgetClass, _ctrlArea,
									  XmNbottomAttachment, XmATTACH_FORM,
									  XmNleftAttachment, XmATTACH_FORM,
									  0);

	ps = _mainWin->GetApplication ();
	context = ps->appContext ();
	_hostsList = new hostBrowse (context, _sysOrDNSForm, 2);

	XtManageChild (_sysOrDNSForm);
}

//--------------------------------------------------------------
// builds a RowColumn and buttons to manipulate system entries.
//--------------------------------------------------------------
void
PSRemoteAcc::BuildSysActionRC ()
{
	Arg							args[10];
	Cardinal					argCnt;
	XmString					str;

	argCnt = 0;
	XtSetArg (args[argCnt], XmNbottomAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNorientation, XmVERTICAL); argCnt++;
	_rowCol = XmCreateRowColumn (_ctrlArea, "sysActionRC", args, argCnt);

	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_addsys));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	_addBtn = XmCreatePushButton (_rowCol, "addSysBtn", args, argCnt);
	_addInfo.type = ADD;
	XtAddCallback (_addBtn, XmNactivateCallback, ChangeAllowedCB, &_addInfo);
	XmStringFree (str);
	registerMnemInfo (_addBtn,
					  GetLocalStr (TXT_MNEM_addsys),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (_addBtn);

	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_remove));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	_removeBtn = XmCreatePushButton (_rowCol, "removeSysBtn", args, argCnt);
	_removeInfo.type = REMOVE;
	XtAddCallback (_removeBtn, XmNactivateCallback, ChangeAllowedCB, &_removeInfo);
	XmStringFree (str);
	registerMnemInfo (_removeBtn,
					  GetLocalStr (TXT_MNEM_remove),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (_removeBtn);

	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_removeall));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	_removeAllBtn = XmCreatePushButton (_rowCol, "removeSysBtn", args, argCnt);
	_removeAllInfo.type = REMOVEALL;
	XtAddCallback (_removeAllBtn, XmNactivateCallback, ChangeAllowedCB, &_removeAllInfo);
	XmStringFree (str);
	registerMnemInfo (_removeAllBtn,
					  GetLocalStr (TXT_MNEM_removeall),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (_removeAllBtn);

	XtManageChild (_rowCol);
}

//--------------------------------------------------------------
// This function builds the Selected Systems form, list and toggle buttons.
//--------------------------------------------------------------
void
PSRemoteAcc::BuildSelSysForm ()
{
	PrintUser*					tmp;
	XmString					str;

	_denyAllowSysToggleBox = XtVaCreateWidget ("denyAllowSysToggleBox",
								xmRowColumnWidgetClass, _ctrlArea,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, _label,
								XmNradioBehavior, True,
								XmNradioAlwaysOne, True,
								XmNorientation, XmVERTICAL,
								XmNrightAttachment, XmATTACH_FORM, NULL);

	str = XmStringCreateSimple (GetLocalStr (TXT_allowSysLabel));
	_allowToggle = XtVaCreateManagedWidget ("allowToggle",
							xmToggleButtonWidgetClass, _denyAllowSysToggleBox,
							XmNlabelString, str, NULL);
	XtAddCallback (_allowToggle, XmNvalueChangedCallback, AllowCB, this);
	XmStringFree (str);
	XtManageChild (_denyAllowSysToggleBox);


	str = XmStringCreateSimple (GetLocalStr (TXT_denySysLabel));
	_denyToggle = XtVaCreateManagedWidget ("denyToggle",
							xmToggleButtonWidgetClass, _denyAllowSysToggleBox,
							XmNlabelString, str, NULL);
	XtAddCallback (_denyToggle, XmNvalueChangedCallback, DenyCB, this);
	XmStringFree (str);
								

	_selSysForm = XtVaCreateWidget ("selectedSystemsForm", 
								xmFormWidgetClass, _ctrlArea,
								XmNtopAttachment, XmATTACH_WIDGET,
								XmNtopWidget, _denyAllowSysToggleBox,
								XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
								XmNleftWidget, _denyAllowSysToggleBox,
								XmNrightAttachment, XmATTACH_FORM,
								XmNbottomAttachment, XmATTACH_FORM,
								NULL);

	_selSysList = new PList (_selSysForm, "seletedSystemsList");
	_selSysList->UW_AddPixmap (systemPixmap);	
	XtVaSetValues (_selSysList->UW_GetListWidget(), XmNselectionPolicy,
					XmEXTENDED_SELECT, NULL);

	if (_selPrinter->CurrentState() == S_DENY)
	{
#ifdef DEBUG
		printf("DENY FLAG IS SET\n");
#endif
		XtVaSetValues (_allowToggle, XmNset, True, NULL);
		tmp = _selPrinter->GetFirstSys ();
		while (tmp) {
			if (tmp->ResetState () == S_DENY)
			{
				str = XmStringCreateLocalized (tmp->Name ());
				_selSysList->UW_ListAddItemUnselected (str, 0, 0);
				XmStringFree (str);
			}
			tmp = _selPrinter->GetNextSys ();
		}	
	}
	else
	{
#ifdef DEBUG
		printf("ALLOW FLAG IS SET\n");
#endif
		XtVaSetValues (_denyToggle, XmNset, True, NULL);
		tmp = _selPrinter->GetFirstSys ();
		while (tmp) {
			if (tmp->ResetState () == S_ALLOW) {
				str = XmStringCreateLocalized (tmp->Name ());
				_selSysList->UW_ListAddItemUnselected (str, 0, 0);
				XmStringFree (str);
			}
			tmp = _selPrinter->GetNextSys ();
		}	
	}

	XtManageChild (_selSysForm);
}

//--------------------------------------------------------------
// Function called when "Add", "Remove", or "Remove All" buttons are selected.
//--------------------------------------------------------------
void
PSRemoteAcc::ChangeAllowedCB (Widget, XtPointer clientData, XtPointer callData)
{
	PSRemoteAcc*				obj = (PSRemoteAcc*)((ClientInfo*)clientData)->ptr;

	obj->ChangeAllowed (((ClientInfo*)clientData)->type);
}

//--------------------------------------------------------------
// Member function called from ChangeAllowedCB
//--------------------------------------------------------------
void
PSRemoteAcc::ChangeAllowed (short type)
{
	int 		cnt	= 0;
	XmString	*selList;
	const char*	item;
	char		*str;
	XmString	tmp;
	
	switch (type) {
		case ADD:
			if (item = _hostsList->getLeafName ()) {
				str = new char[strlen (item) + 5];
				sprintf (str, "%s!all", item);
				tmp = XmStringCreateSimple (str);
				if (_selSysList->UW_ListItemExists (tmp)) {
					_selSysList->UW_ListReplaceItems (&tmp, 1, &tmp);
				}
				else {
					_selSysList->UW_ListAddItemUnselected (tmp, 0, 0);
				}
				_selPrinter->AddSysAllowState ((char*)str, 
				      _selPrinter->CurrentState () == S_DENY ? S_DENY: S_ALLOW);
				delete (str);
				XmStringFree (tmp);
			}
		break;

		case REMOVE:
			for(;;) {	
				XtVaGetValues (_selSysList->UW_GetListWidget(), 
					XmNselectedItems, &selList, XmNselectedItemCount, 
					&cnt, NULL);
				if (!cnt)
					break;
				XmStringGetLtoR (selList[0], XmSTRING_DEFAULT_CHARSET, &str);
				_selPrinter->ClearSysAllowState (str);
				_selSysList->UW_ListDeleteItem (selList[0]);
			}
		break;

		case REMOVEALL:
			for(;;) {	
				XtVaGetValues (_selSysList->UW_GetListWidget(), 
					XmNitems, &selList, XmNitemCount, &cnt, NULL);
				if (!cnt)
					break;
				XmStringGetLtoR (selList[0], XmSTRING_DEFAULT_CHARSET, &str);
				_selPrinter->ClearSysAllowState (str);
				_selSysList->UW_ListDeleteItem (selList[0]);
			}
		break;

		default:
		break;
	}	
}

//--------------------------------------------------------------
// Called when the _denyToggle's value is changed 
//--------------------------------------------------------------
void
PSRemoteAcc::DenyCB (Widget, XtPointer clientData, XtPointer callData)
{
	PSRemoteAcc*				obj = (PSRemoteAcc*)clientData;

	obj->Deny ((XmToggleButtonCallbackStruct*)callData);
}

//--------------------------------------------------------------
// Member function called from DenyCB
//--------------------------------------------------------------
void
PSRemoteAcc::Deny (XmToggleButtonCallbackStruct* cbs)
{
	PrintUser*					tmp;
	XmString					str;

	if (!cbs->set) {
		return;
	}
	if (_selPrinter->CurrentState () != S_ALLOW) {
		_selPrinter->CurrentState (S_ALLOW);
		_selSysList->UW_ListDeleteAllItems ();
		tmp = _selPrinter->GetFirstSys ();
		while (tmp) {
			if (tmp->Allow_State () == S_ALLOW) {
				str = XmStringCreateLocalized (tmp->Name ());
				_selSysList->UW_ListAddItemUnselected (str, 0, 0);
				XmStringFree (str);
			}
			tmp = _selPrinter->GetNextSys ();
		}	
	}
}

//--------------------------------------------------------------
// Called when the _allowToggle's value is changed
//--------------------------------------------------------------
void
PSRemoteAcc::AllowCB (Widget, XtPointer clientData, XtPointer)
{
	PSRemoteAcc*				obj = (PSRemoteAcc*)clientData;

	obj->Allow ((XmToggleButtonCallbackStruct*)clientData);
}

//--------------------------------------------------------------
// Member function called from AllowCB.
//--------------------------------------------------------------
void
PSRemoteAcc::Allow (XmToggleButtonCallbackStruct *cbs)
{
	PrintUser*					tmp;
	XmString 					str;

	if (!cbs->set) {
		return;
	}

	if (_selPrinter->CurrentState () != S_DENY) {
		_selPrinter->CurrentState (S_DENY);
		_selSysList->UW_ListDeleteAllItems ();
		tmp = _selPrinter->GetFirstSys ();
		while (tmp) {
			if (tmp->DenyState () == S_DENY) {
				str = XmStringCreateLocalized (tmp->Name ());
				_selSysList->UW_ListAddItemUnselected (str, 0, 0);
				XmStringFree (str);
			}
			tmp = _selPrinter->GetNextSys ();
		}	
	}
}

