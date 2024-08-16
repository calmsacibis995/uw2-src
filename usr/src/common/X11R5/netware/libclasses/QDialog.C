#ident	"@(#)libclass:QDialog.C	1.3"
////////////////////////////////////////////////////////////////////
// QDialog.C: A dialog for questions
/////////////////////////////////////////////////////////////////////
#include "QDialog.h"
#include "main.h"
#include "msgs.h"
#include "i18n.h"
#include <Xm/MessageB.h>

QDialog::QDialog (char *name) : Dialog (name)
{
	_type = QUESTION;
}

void QDialog::postDialog (Widget parent, char *title, char *msg) 
{
	/* post the dialog
	 */
	Dialog::postDialog (parent, title, msg);

	/* register the callbacks
	 */
	registerCancelCallback (&QDialog::cancelCB, this);
	registerOkCallback (&QDialog::okCB, this);

	/* unmanage the help
	 * button
	 */
	unmanageHelp ();

	/* set the strings to yes / no instead
	 */
	setOkString (I18n::GetStr (TXT_yes),I18n::GetStr (MNEM_yes));
	setCancelString (I18n::GetStr (TXT_no),I18n::GetStr (MNEM_no));

	/* manage the widgets 
	 */
	manage ();
}

void QDialog::cancelCB (Widget, XtPointer  client_data, XtPointer)
{
	QDialog *obj = (QDialog *) client_data;
	
	obj->cancel ();
}

void QDialog::okCB (Widget, XtPointer  client_data, XtPointer)
{
	QDialog *obj = (QDialog *) client_data;
	
	obj->ok ();
}
