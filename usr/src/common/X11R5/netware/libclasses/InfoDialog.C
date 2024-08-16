#ident	"@(#)libclass:InfoDialog.C	1.3"
////////////////////////////////////////////////////////////////////
// InfoDialog.C: A dialog for information messages 
/////////////////////////////////////////////////////////////////////
#include "InfoDialog.h"
#include <Xm/MessageB.h>
#include <iostream.h>
#include "msgs.h"
#include "i18n.h"

InfoDialog *theInfoDialogMgr = new InfoDialog ("Information");

InfoDialog::InfoDialog (char *name) : Dialog (name)
{
	_type = INFORMATION;
}

void InfoDialog::postDialog (Widget parent, char *title, char *msg) 
{
	Dialog::postDialog (parent, title, msg);
	registerOkCallback (&InfoDialog::OkCB, this);
	unmanageCancel ();
	unmanageHelp ();
	setOkString (I18n::GetStr (TXT_ok), I18n::GetStr (MNEM_ok));
	manage ();
}

void InfoDialog::OkCB (Widget w, XtPointer  client_data, XtPointer)
{
	InfoDialog *obj = (InfoDialog *) client_data;
	
	obj->ok (w);
}

void InfoDialog::ok (Widget w)
{
	XtDestroyWidget (w);
	cout << "destroyed the widget " << endl;
}
