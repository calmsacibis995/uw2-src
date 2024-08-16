#ident	"@(#)debugger:libol/common/Radio.C	1.4"

#include "UI.h"
#include "Component.h"
#include "Radio.h"
#include "gui_label.h"
#include "Label.h"
#include <Xol/FButtons.h>

// fields specifies the contents of Button_data for XtCreateManagedWidget
// there will be one Button_data struct for each button
// fields and Button_data must always be in sync
static String fields[] =
{
	XtNlabel,	// button name
	XtNset,		// initial_setting for this button
};

struct Button_data
{
	XtArgVal	label;
	XtArgVal	state;
};

// radioCB is called by the toolkit when the user pushes a button
// the toolkit sets the state field in the Button_data array
// radioCB sets current to the number of the button pushed, and invokes
// the framework callback, if there is one
static void
radioCB(Widget, Radio_list *ptr, XtPointer data)
{
	int item = ((OlFlatCallData *)data)->item_index;

	if (!ptr  || item < 0 || item >= ptr->get_nbuttons())
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	ptr->set_current(item);

	Command_sender	*creator = ptr->get_creator();
	Callback_ptr	cb = ptr->get_callback();
	if (cb && creator)
		(creator->*cb)(ptr, (void *)item);
}

// Using flat widgets instead of the normal exclusives -
// flat widgets take up less space, but you have to know how
// many buttons there will be and create them all at once

Radio_list::Radio_list(Component *p, const char *s, Orientation orient,
	const LabelId *names, int cnt, int initial_button, Callback_ptr f, Command_sender *c,
	Help_id h) : COMPONENT(p, s, c, h)
{
	Button_data	*button;

	callback = f;
	if ((initial_button >= cnt) || (callback && !creator))
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	current = initial_button;
	nbuttons = cnt;
	buttons = button = new Button_data[cnt];

	for (int i = 0; i < cnt; i++, button++, names++)
	{
		button->label = (XtArgVal)labeltab.get_label(*names);
		button->state = (i == current) ? TRUE : FALSE;
	}

	widget = XtVaCreateManagedWidget((char *)label,
		flatButtonsWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNnumItems, nbuttons,
		XtNitems, buttons,
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNclientData, this,
		XtNselectProc, radioCB,
		XtNlayoutType, (orient == OR_vertical) ? OL_FIXEDCOLS : OL_FIXEDROWS,
		XtNbuttonType, OL_RECT_BTN,
		XtNexclusives, TRUE,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Radio_list::~Radio_list()
{
	delete buttons;
}

// set_button is called from the framework, when it needs to update the
// displayed value, for example if the user sets %lang in a script,
// the Set_language dialog will call set_button to display the new
// current language
void
Radio_list::set_button(int which)
{
	Button_data	*button;

	if (which < 0 || which >= nbuttons)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	current = which;
	button = buttons;
	for (int i = 0; i < nbuttons; i++)
	{
		if (i == which)
			button->state = (XtArgVal)TRUE;
		else
			button->state = (XtArgVal)FALSE;
		button++;
	}

	XtVaSetValues(widget, XtNitemsTouched, TRUE, 0);
}

// this can't be inlined because it is a toolkit-specific implementation of
// a framework-independent interface
int
Radio_list::which_button()
{
	return current;
}
