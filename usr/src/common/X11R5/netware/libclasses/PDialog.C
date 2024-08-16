#ident	"@(#)libclass:PDialog.C	1.3"
////////////////////////////////////////////////////////////////////
// PDialog.C: A dialog for prompts 
/////////////////////////////////////////////////////////////////////
#include "PDialog.h"
#include "main.h"
#include "msgs.h"
#include "i18n.h"

PDialog::PDialog (char *name) : Dialog (name)
{
	_type = PROMPT;
}

void PDialog::postPDialog (Widget parent, char *title, char *selection, 
				char *label) 
{
	/* post the dialog
	 */
	Dialog::postDialog (parent, title, NULL);
	XtVaSetValues (_w, 
			XmNselectionLabelString,XmStringCreateLtoR(selection,
						charset),
			XmNtextString, 		XmStringCreateLtoR(label,
						charset),
			0);

	/* register the callbacks
	 */
	registerCancelCallback (&PDialog::cancelCB, this);
	registerOkCallback (&PDialog::okCB, this);
	registerHelpCallback (&PDialog::helpCB, this);

	setOkString (I18n::GetStr (TXT_ok), I18n::GetStr (MNEM_ok));
	setCancelString (I18n::GetStr (TXT_cancel), I18n::GetStr (MNEM_cancel));
	setHelpString (I18n::GetStr (TXT_help), I18n::GetStr (MNEM_help));

	/* manage the widgets 
	 */
	manage ();
}

void PDialog::cancelCB (Widget, XtPointer  client_data, XtPointer)
{
	PDialog *obj = (PDialog *) client_data;
	
	obj->cancel ();
}

void PDialog::okCB (Widget, XtPointer  client_data, XtPointer call_data)
{
	XmSelectionBoxCallbackStruct 	*cb = (XmSelectionBoxCallbackStruct *)
					call_data;
	
	PDialog *obj = (PDialog *) client_data;
	
	obj->ok (cb);
}

void PDialog::helpCB (Widget w, XtPointer  client_data, XtPointer)
{
	PDialog *obj = (PDialog *) client_data;
	
	obj->help(obj->_w);
}
