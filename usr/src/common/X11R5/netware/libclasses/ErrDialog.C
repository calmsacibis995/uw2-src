#ident	"@(#)libclass:ErrDialog.C	1.4"
////////////////////////////////////////////////////////////////////
// ErrDialog.C: A dialog for error messages 
/////////////////////////////////////////////////////////////////////
#include "ErrDialog.h"
#include <Xm/MessageB.h>
#include <iostream.h>
#include "msgs.h"
#include "i18n.h"

ErrDialog *theErrDialogMgr = new ErrDialog ("Error");

ErrDialog::ErrDialog (char *name) : Dialog (name)
{
	_type = ERROR;
}

void ErrDialog::postDialog (Widget parent, char *title, char *msg) 
{
	Dialog::postDialog (parent, title, msg);
	registerCancelCallback (&ErrDialog::cancelCB, this);
	unmanageOk ();
	unmanageHelp ();
	setCancelString (I18n::GetStr (TXT_cancel), I18n::GetStr (MNEM_cancel));
	manage ();
}

void ErrDialog::cancelCB (Widget w, XtPointer  client_data, XtPointer)
{
	ErrDialog *obj = (ErrDialog *) client_data;
	
	obj->cancel (w);
}

void ErrDialog::cancel (Widget w)
{
	XtDestroyWidget (w);
}

void ErrDialog::setModal()
{
	XtVaSetValues (_w, XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
			0);
}
