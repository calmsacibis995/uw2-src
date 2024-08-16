#ident	"@(#)debugger:libol/common/Toggle.C	1.6"

#include "UI.h"
#include "Component.h"
#include "Toggle.h"
#include "gui_label.h"
#include "Label.h"
#include <Xol/FButtons.h>

// fields specifies the contents of Toggle_data
// there is one Toggle_descriptor for each toggle
// fields and Toggle_descriptor must always be in sync
static String fields[] =
{
	XtNlabel,	// name
	XtNset,		// initial setting
};

struct Toggle_descriptor
{
	XtArgVal	label;
	XtArgVal	state;
};

// Invoke the framework callback if there is one
static void
toggleCB(Widget w, int, OlFlatCallData *data)
{
	Boolean			state;
	Toggle_button		*ptr;
	Callback_ptr		cb;
	Command_sender		*creator;

	int index = data->item_index;
	XtVaGetValues(w, XtNuserData, &ptr, 0);
	if (!ptr)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __FILE__);
		return;
	}

	state = (Boolean) (ptr->get_toggles())[index].state;

	if (index < 0 || index >= ptr->get_nbuttons())
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __FILE__);
		return;
	}

	cb = ptr->get_buttons()[index].callback;
	creator = ptr->get_creator();
	if (cb && creator)
		(creator->*cb)(ptr, (void *)state);
}

// if flat button widgets are not available use a control area widget and a
// series of checkbox widgets
Toggle_button::Toggle_button(Component *p, const char *s, const Toggle_data *b,
	int n, Orientation o, Command_sender *c, Help_id h) : COMPONENT(p, s, c, h)
{
	Toggle_descriptor	*toggle;

	nbuttons = n;
	buttons = b;
	toggle = new Toggle_descriptor[nbuttons];
	toggles = toggle;
	
	for (int i = 0; i < n; i++, b++, toggle++)
	{
		toggle->label = (XtArgVal)labeltab.get_label(b->label);
		toggle->state = (XtArgVal)b->state;
	}

	widget = XtVaCreateManagedWidget(label,
		flatButtonsWidgetClass,
		parent->get_widget(),
		XtNnumItems, n,
		XtNitems, toggles,
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNselectProc, toggleCB,
		XtNunselectProc, toggleCB,
		XtNbuttonType, OL_CHECKBOX,
		XtNlayoutType, (o == OR_vertical) ? OL_FIXEDCOLS : OL_FIXEDROWS,
		XtNclientData, this,
		XtNuserData, this,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Toggle_button::~Toggle_button()
{
	delete toggles;
}

void
Toggle_button::set(int button, Boolean setting)
{
	OlVaFlatSetValues(widget, button, XtNset, setting, 0);
}

Boolean
Toggle_button::is_set(int button)
{
	return toggles[button].state;
}
