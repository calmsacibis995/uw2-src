#ident	"@(#)libclass:WarnDialog.C	1.3"
////////////////////////////////////////////////////////////////////
// WarnDialog.C: A dialog for warning messages 
/////////////////////////////////////////////////////////////////////
#include "WarnDialog.h"
#include <Xm/MessageB.h>
#include <iostream.h>
#include "msgs.h"
#include "i18n.h"

WarnDialog *theWarnDialogMgr = new WarnDialog ("Warning");

WarnDialog::WarnDialog (char *name) : Dialog (name)
{
	_type = WARNING;
}

void WarnDialog::postDialog (Widget parent, char *title, char *msg) 
{
	Dialog::postDialog (parent, title, msg);
	registerOkCallback (&WarnDialog::okCB, this);
	unmanageCancel ();
	unmanageHelp ();
	setOkString (I18n::GetStr (TXT_ok), I18n::GetStr (MNEM_ok));
	manage ();
}

void WarnDialog::okCB (Widget w, XtPointer  client_data, XtPointer)
{
	WarnDialog *obj = (WarnDialog *) client_data;
	
	obj->ok (w);
}

void WarnDialog::ok(Widget w)
{
	XtDestroyWidget (w);
}
