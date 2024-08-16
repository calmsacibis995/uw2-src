#ident	"@(#)debugger:libol/common/Text_line.C	1.5"

#include "UI.h"
#include "Component.h"
#include "Text_line.h"
#include <Xol/TextField.h>

// getstring CB is called when the user hits return
static void
getstringCB(Widget, Text_line *ptr, OlTextFieldVerify *data)
{
	if (!ptr || !data)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Callback_ptr	cb = ptr->get_return_cb();
	Command_sender	*creator = ptr->get_creator();
	if (cb && creator)
		(creator->*cb)(ptr, data->string);
}

// A Text_line is simply an OpenLook TextField widget

Text_line::Text_line(Component *p, const char *s, const char *text, int width,
	Boolean edit, Callback_ptr ret, Command_sender *c, Help_id h)
	: COMPONENT(p, s, c, h)
{
	return_cb = ret;
	editable = edit;
	string = 0;
	if (!width)
		width = 10;

	widget = XtVaCreateManagedWidget(label, textFieldWidgetClass,
		parent->get_widget(), 
		XtNcharsVisible, width,
		XtNsensitive, edit,
		XtNstring, text,
		XtNuserData, this,
		0);

	if (return_cb)
	{
		if (!creator)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		XtAddCallback(widget, XtNverification, (XtCallbackProc)getstringCB,
			(XtPointer)this);
	}	

	if (!editable)
		XtSetSensitive(widget, FALSE);
	if (help_msg)
		register_help(widget, label, help_msg);
}

Text_line::~Text_line()
{
	delete string;
}

// blank out the display
void
Text_line::clear()
{
	XtVaSetValues(widget, XtNstring, "", 0);
}

// returns the current contents.  The toolkit makes a copy of the string.
// Text_line frees the space when through with it
char *
Text_line::get_text()
{
	delete string;
	XtVaGetValues(widget, XtNstring, &string, 0);
	return string;
}

void
Text_line::set_text(const char *s)
{
	if (!s)
		s = "";
	XtVaSetValues(widget, XtNstring, s, 0);
}

void
Text_line::set_cursor(int pos)
{
	XtVaSetValues(widget, XtNcursorPosition, pos, 0);
	OlMoveFocus(widget, OL_IMMEDIATE, CurrentTime);
}

void
Text_line::set_editable(Boolean e)
{
	if (e == editable)
		return;

	editable = e;
	XtSetSensitive(widget, e);
}
