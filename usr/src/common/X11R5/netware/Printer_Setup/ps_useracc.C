#ident	"@(#)prtsetup2:ps_useracc.C	1.12"
//--------------------------------------------------------------
// ps_useracc.c
//--------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#ifndef UID_MAX
#define UID_MAX 60002
#endif

#include <Xm/Xm.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>

#include "ps_hdr.h"
#include "BasicComponent.h"
#include "ps_i18n.h"
#include "ps_xpmbutton.h"
#include "ps_mainwin.h"
#include "ps_win.h"
#include "ps_dialog.h"
#include "ps_useracc.h"
#include "ps_msg.h"
#include "mnem.h"

//extern "C" {
#include "lpsys.h"
//}

char*				userPixmap = { "/usr/X/lib/pixmaps/ptr.user16" };

/*----------------------------------------------------------------------------
 *
 */
PSUserAccess::PSUserAccess (Widget		parent,
							char*		name,
							PSPrinter*	selPrinter,
							short		ptype,
							action*		abi,
							short		buttonCnt) 
			: LOWEST_USER_UID (100),
			  PSDialog (parent, name, selPrinter, ptype, abi, buttonCnt)
{
	_selPrinter = selPrinter;

    XtAddCallback (abi[0].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSUserAccess::applyCallback,
				   this);
    XtAddCallback (abi[1].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSUserAccess::resetCallback,
				   this);
    XtAddCallback (abi[2].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSUserAccess::cancelCallback,
				   this);
    XtAddCallback (abi[3].w,
				   XmNactivateCallback,
				   (XtCallbackProc)&PSUserAccess::helpCallback,
				   this);
    XtAddCallback (panedWindow (),
				   XmNhelpCallback,
				   (XtCallbackProc)&PSUserAccess::helpCallback,
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
PSUserAccess::~PSUserAccess ()
{
}

//--------------------------------------------------------------
// creates the control area portion for the User Access dialog.
//--------------------------------------------------------------
void
PSUserAccess::CreateCtrlArea ()
{
	PrintUser*					tmp;
	Arg							args[10];
	Cardinal					argCnt;
	XmString					str;
	char*						tmpStr;

	_allowInfo.ptr = this;
	_denyInfo.ptr = this;
	_allowAllInfo.ptr = this;
	_denyAllInfo.ptr = this;

	argCnt = 0;
	tmpStr = new char[strlen (GetLocalStr (TXT_useracc_label))
					  + strlen (GetPrinterName ()) + 1];
	sprintf (tmpStr, GetLocalStr (TXT_useracc_label), GetPrinterName ());
	str = XmStringCreateSimple (tmpStr);	
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_FORM); argCnt++;
	_label = XmCreateLabelGadget (GetCtrlArea (),
								  "UserAccessLabel",
								  args,
								  argCnt);
	XmStringFree (str);
	delete (tmpStr);
	XtManageChild (_label);

	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_denyList));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNtopWidget, _label); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_FORM); argCnt++;
	_denyListHdr = XmCreateLabelGadget (GetCtrlArea(), "Deny List Hdr", args, argCnt);
	XmStringFree (str);
	XtManageChild (_denyListHdr);

	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_allowList));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNtopWidget, _label); argCnt++;
	_allowListHdr = XmCreateLabelGadget (GetCtrlArea(), "Allow List Hdr", args, argCnt);
	XmStringFree (str);
	XtManageChild (_allowListHdr);

	argCnt = 0;
	XtSetArg (args[argCnt], XmNbottomAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNtopWidget, _denyListHdr); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_FORM); argCnt++;
	_denyListForm 	= XmCreateForm (GetCtrlArea(), "DenyListForm", args, argCnt);
	_denyList 		= new PList (_denyListForm, "DenyList");

	argCnt = 0;
	XtSetArg (args[argCnt], XmNbottomAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNtopWidget, _denyListHdr); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNleftWidget, _denyListForm); argCnt++;
	XtSetArg (args[argCnt], XmNorientation, XmVERTICAL); argCnt++;
	_rowCol	= XmCreateRowColumn (GetCtrlArea(), "DenyAllowRC", args, argCnt);

	XtVaSetValues (_allowListHdr, XmNleftAttachment, XmATTACH_WIDGET,
						XmNleftWidget, _rowCol, NULL);

	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_ALLOW_ALL));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	_allowAllBtn	= XmCreatePushButton (_rowCol,
						"AllowAllBtn", args, argCnt);
	_allowAllInfo.type = ALLOWALL;
	XtAddCallback (_allowAllBtn, XmNactivateCallback, ChangeAllowedCB, &_allowAllInfo);
	XmStringFree (str);
	registerMnemInfo (_allowAllBtn,
					  GetLocalStr (TXT_MNEM_ALLOW_ALL),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (_allowAllBtn);


	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_DENY_ALL));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	_denyAllBtn		= XmCreatePushButton (_rowCol,
						"DenyAllBtn", args, argCnt);
	_denyAllInfo.type = DENYALL;
	XtAddCallback (_denyAllBtn, XmNactivateCallback, ChangeAllowedCB, &_denyAllInfo);
	XmStringFree (str);
	registerMnemInfo (_denyAllBtn,
					  GetLocalStr (TXT_MNEM_DENY_ALL),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (_denyAllBtn);

	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_ALLOW));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	_allowBtn		= XmCreatePushButton (_rowCol,
						"AllowBtn", args, argCnt);
	_allowInfo.type = ALLOW;
	XtAddCallback (_allowBtn, XmNactivateCallback, ChangeAllowedCB, &_allowInfo);
	XmStringFree (str);
	registerMnemInfo (_allowBtn,
					  GetLocalStr (TXT_MNEM_ALLOW),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (_allowBtn);

	argCnt = 0;
	str = XmStringCreateSimple (GetLocalStr (TXT_DENY));
	XtSetArg (args[argCnt], XmNlabelString, str); argCnt++;
	_denyBtn		= XmCreatePushButton (_rowCol,
						"DenyBtn", args, argCnt);
	_denyInfo.type = DENY;
	XtAddCallback (_denyBtn, XmNactivateCallback, ChangeAllowedCB, &_denyInfo);
	XmStringFree (str);
	registerMnemInfo (_denyBtn,
					  GetLocalStr (TXT_MNEM_DENY),
					  MNE_ACTIVATE_BTN | MNE_GET_FOCUS);
	XtManageChild (_denyBtn);

	XtManageChild (_rowCol);
	
	argCnt = 0;
	XtSetArg (args[argCnt], XmNbottomAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNtopAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNtopWidget, _denyListHdr); argCnt++;
	XtSetArg (args[argCnt], XmNrightAttachment, XmATTACH_FORM); argCnt++;
	XtSetArg (args[argCnt], XmNleftAttachment, XmATTACH_WIDGET); argCnt++;
	XtSetArg (args[argCnt], XmNleftWidget, _rowCol); argCnt++;
	_allowListForm 	= XmCreateForm (GetCtrlArea(), "AllowListForm", args, argCnt);
	_allowList 	= new PList (_allowListForm, "AllowList"); 

	// Now give the 2 PLists a pixmap and add the users to the correct list.
	_denyList->UW_AddPixmap (userPixmap);
	_allowList->UW_AddPixmap (userPixmap);

	tmp = _selPrinter->GetFirstUser ();
	while (tmp) {
		str = XmStringCreateSimple (tmp->Name ());
		if (tmp->Allow_State () == S_ALLOW) {
			_allowList->UW_ListAddItemUnselected (str, 0, 0);
		}
		else {
			_denyList->UW_ListAddItemUnselected (str, 0, 0);
		}
		tmp = _selPrinter->GetNextUser ();
		XmStringFree (str);
	}

	// Set up Single and Double Select callbacks
	_denyList->UW_RegisterDblCallback (SelectCB, this);
	_denyList->UW_RegisterSingleCallback (SelectCB, this);
	XtVaSetValues (_denyList->UW_GetListWidget (),
				   XmNselectionPolicy,
				   XmEXTENDED_SELECT,
				   0);

	_allowList->UW_RegisterDblCallback (SelectCB, this);
	_allowList->UW_RegisterSingleCallback (SelectCB, this);
	XtVaSetValues (_allowList->UW_GetListWidget (),
				   XmNselectionPolicy,
				   XmEXTENDED_SELECT,
				   0);

	XtManageChild (_denyListForm);
	XtManageChild (_allowListForm); 
    ShowDialog();
	
	XtAddEventHandler (GetCtrlArea (),
					   StructureNotifyMask,
					   False,
					   &ResizeCtrlAreaCB,
					   (XtPointer)this);
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUserAccess::applyCallback (Widget, XtPointer clientData, XtPointer)
{
    PSUserAccess*				obj = (PSUserAccess*)clientData;

    obj->Apply ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUserAccess::Apply ()
{
	_selPrinter->UpdateUserArray ();
	_selPrinter->UpdateUserAccessLists ();
	unmanage ();
	d_delete = True;
} 

/*----------------------------------------------------------------------------
 *
 */
void
PSUserAccess::resetCallback (Widget, XtPointer clientData, XtPointer)
{
    PSUserAccess*				obj = (PSUserAccess*)clientData;

    obj->Reset ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUserAccess::Reset ()
{
	PrintUser*					tmp;
	XmString					str;

	_allowList->UW_ListDeleteAllItems ();
	_denyList->UW_ListDeleteAllItems ();	
	_selPrinter->ResetUserList ();
	tmp = _selPrinter->GetFirstUser ();
	while (tmp) {
		str = XmStringCreateSimple (tmp->Name ());
		if (tmp->Allow_State () == S_ALLOW) {
			_allowList->UW_ListAddItemUnselected (str, 0, 0);
		}
		else {
			_denyList->UW_ListAddItemUnselected (str, 0, 0);
		}
		tmp = _selPrinter->GetNextUser ();
		XmStringFree (str);
	}
} 

/*----------------------------------------------------------------------------
 *
 */
void
PSUserAccess::cancelCallback (Widget, XtPointer clientData, XtPointer)
{
    PSUserAccess*				obj = (PSUserAccess*)clientData;

    obj->Cancel ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUserAccess::Cancel ()
{
	_selPrinter->ResetUserList ();
	unmanage ();
	d_delete = True;
} 

/*----------------------------------------------------------------------------
 *
 */
void
PSUserAccess::helpCallback (Widget, XtPointer clientData, XtPointer)
{
    PSUserAccess*				obj = (PSUserAccess*)clientData;

    obj->Help ();
}

/*----------------------------------------------------------------------------
 *
 */
void
PSUserAccess::Help ()
{
	helpDisplay (DT_SECTION_HELP, GetLocalStr (TXT_helpTitle), TXT_usersSect);
} 

//--------------------------------------------------------------
// Static member function called when the CtrlArea is resized.
//--------------------------------------------------------------
void
PSUserAccess::ResizeCtrlAreaCB (Widget		w,
								XtPointer	client_data,
								XEvent*		event,
								Boolean*	continue_to_dispach_return)
{
	PSUserAccess*				obj = (PSUserAccess*)client_data;

	obj->ResizeCtrlArea (event);
}

//--------------------------------------------------------------
// Member function called from 
//	PSUserAccess::ResizeCtrlAreaCB when the CtrlArea is resized.
//--------------------------------------------------------------
void
PSUserAccess::ResizeCtrlArea (XEvent* event)
{
	static Boolean				first = True; 
	short 						width;
	static Dimension			rcWidth;
	Dimension					offset;

	if (first) {
		XtVaGetValues (_rowCol, XmNwidth, &rcWidth, 0);		
		first = False;
	}

	if (event->type == ConfigureNotify) {
		width = event->xconfigure.width;
		XtVaSetValues (GetCtrlArea (), XmNfractionBase, width, 0);	
		offset = (width - rcWidth) / 2;
		XtVaSetValues (_denyListForm, XmNleftAttachment, XmATTACH_POSITION,  
					   XmNleftPosition, 0,
					   XmNrightAttachment, XmATTACH_POSITION, 
					   XmNrightPosition, offset,
					   0);
		XtVaSetValues (_rowCol, XmNleftAttachment, XmATTACH_POSITION, 
					   XmNleftPosition, offset,
					   XmNrightAttachment, XmATTACH_POSITION, 
					   XmNrightPosition, width - offset,
					   0);
		XtVaSetValues (_allowListForm, XmNleftAttachment, XmATTACH_POSITION, 
					   XmNleftPosition, width - offset,
					   XmNrightAttachment, XmATTACH_POSITION, 
					   XmNrightPosition, width,
					   0);
	}
}

//--------------------------------------------------------------
// Function called when an item is selected from 
//				either the _allowList or _denyList.
//--------------------------------------------------------------
void
PSUserAccess::SelectCB (Widget, XtPointer, XtPointer)
{
}

//--------------------------------------------------------------
// Function called when "Allow All", "Deny All",
//				"Allow" or "Deny" buttons are selected.
//--------------------------------------------------------------
void
PSUserAccess::ChangeAllowedCB (Widget, XtPointer clientData, XtPointer callData)
{
    PSUserAccess*				obj;

    obj = (PSUserAccess*)((ClientInfo*)clientData)->ptr;
    obj->ChangeAllowed (((ClientInfo*)clientData)->type);
}

//--------------------------------------------------------------
// Member function called from ChangeAllowedCB.
//--------------------------------------------------------------
void
PSUserAccess::ChangeAllowed (short type)
{
	int							i;
	int 						cnt = 0;
	XmString*					selList;
	char*						tmp;

	switch (type) {
	case ALLOW:
		XtVaGetValues (_denyList->UW_GetListWidget (),
					   XmNselectedItems,
					   &selList,
					   XmNselectedItemCount,
					   &cnt,
					   0);
		for (i = cnt - 1; i >= 0; i--) {
			_allowList->UW_ListAddItemUnselected (selList[i], 0, 0);
			XmStringGetLtoR (selList[i], XmSTRING_DEFAULT_CHARSET, &tmp); 
			_selPrinter->ChangeUserAllowState (tmp, S_ALLOW);
			XtFree (tmp);
		}
		for(;;) {
			XtVaGetValues (_denyList->UW_GetListWidget (),
						   XmNselectedItems,
						   &selList,
						   XmNselectedItemCount,
						   &cnt,
						   0);
			if (!cnt) {
				break;
			}
			_denyList->UW_ListDeleteItem (selList[0]);
		}
		XtManageChild (_denyListForm);
		XtManageChild (_allowListForm);
		break;

	case DENY:
		XtVaGetValues (_allowList->UW_GetListWidget (),
					   XmNselectedItems,
					   &selList,
					   XmNselectedItemCount,
					   &cnt,
					   0);
		for (i = cnt - 1; i >= 0; i--) {
			_denyList->UW_ListAddItemUnselected (selList[i], 0, 0);
			XmStringGetLtoR (selList[i], XmSTRING_DEFAULT_CHARSET, &tmp); 
			_selPrinter->ChangeUserAllowState (tmp, S_DENY);
			XtFree (tmp);
		}
		for(;;) {
			XtVaGetValues (_allowList->UW_GetListWidget (),
						   XmNselectedItems,
						   &selList,
						   XmNselectedItemCount,
						   &cnt,
						   0);
			if (!cnt) {
				break;
			}
			_allowList->UW_ListDeleteItem (selList[0]);
		}
		XtManageChild (_denyListForm);
		XtManageChild (_allowListForm);
		break;

	case ALLOWALL:
		XtVaGetValues (_denyList->UW_GetListWidget (),
					   XmNitems,
					   &selList,
					   XmNitemCount,
					   &cnt,
					   0);
		for (i = cnt - 1; i >= 0; i--) {
			_allowList->UW_ListAddItemUnselected (selList[i], 0, 0);
			XmStringGetLtoR (selList[i], XmSTRING_DEFAULT_CHARSET, &tmp); 
			_selPrinter->ChangeUserAllowState (tmp, S_ALLOW);
			XtFree (tmp);
		}
		for(;;) {
			XtVaGetValues (_denyList->UW_GetListWidget (),
						   XmNitems,
						   &selList,
						   XmNitemCount,
						   &cnt,
						   0);
			if (!cnt) {
				break;
			}
			_denyList->UW_ListDeleteItem (selList[0]);
		}
		XtManageChild (_denyListForm);
		XtManageChild (_allowListForm);
		break;

	case DENYALL:
		XtVaGetValues (_allowList->UW_GetListWidget (),
					   XmNitems,
					   &selList,
					   XmNitemCount,
					   &cnt,
					   0);
		for (i = cnt - 1; i >= 0; i--) {
			_denyList->UW_ListAddItemUnselected (selList[i], 0, 0);
			XmStringGetLtoR (selList[i], XmSTRING_DEFAULT_CHARSET, &tmp); 
			_selPrinter->ChangeUserAllowState (tmp, S_DENY);
			XtFree (tmp);
		}
		for(;;) {
			XtVaGetValues (_allowList->UW_GetListWidget (),
						   XmNitems,
						   &selList,
						   XmNitemCount,
						   &cnt,
						   0);
			if (!cnt) {
				break;
			}
			_allowList->UW_ListDeleteItem (selList[0]);
		}
		XtManageChild (_denyListForm);
		XtManageChild (_allowListForm);
		break;

	default:
		break;
	}
} 

