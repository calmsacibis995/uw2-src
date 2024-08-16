#ident	"@(#)debugger:libol/common/Button_bar.C	1.5"

// GUI headers
#include "UI.h"
#include "Component.h"
#include "Windows.h"
#include "Menu.h"		// for Set_cb, etc.
#include "Base_win.h"		
#include "Panes.h"		// for Pane specific callbacks
#include "Button_bar.h"
#include "Toolkit.h"
#include "gui_label.h"
#include "Label.h"

#include <Xol/FButtons.h>
#include <Xol/RubberTile.h>

// Using flat buttons, the button bar is a flat widget.

// Button bar descriptor - bar_fields and Bar_data must be in sync
static String bar_fields[] =
{
	XtNselectProc,
	XtNlabel,
	XtNmnemonic,
	XtNsensitive,
	XtNclientData,
};

struct Bar_data
{
	XtArgVal	select_cb;	// activate callback
	XtArgVal	label;		// button label
	XtArgVal	mnemonic;
	XtArgVal	sensitive;	// is the operation implemented and valid?
	XtArgVal	entry;		// table entry
};

// callback when button is selected - the widget's user data is the
// framework object, and func is its callback function

static void
buttonCB(Widget w, const Button_bar_table *tab, XtPointer)
{
	Button_bar	*ptr;
	Callback_ptr	func;
	Base_window	*win;
	Window_set	*ws;
	Pane		*pane;

	XtVaGetValues(w, XtNuserData, (XtPointer)&ptr, 0);
	func = tab->callback;

	ws = ptr->get_window_set();
	ws->clear_msgs();
	switch (tab->flags)
	{
	case Set_cb:
		(ws->*func)(ptr, ptr->get_window());
		break;
	case Window_cb:
		win = ptr->get_window();
		(win->*func)(ptr, (void *)tab->cdata);
		break;
	case Pane_cb:
		win = ptr->get_window();
		pane = win->get_pane((Pane_type)tab->pane_type);
		if (!pane)
		{
			display_msg(E_ERROR, GE_internal, __FILE__,
				__LINE__);
			break;
		}
		(pane->*func)(ptr, (void *)tab->cdata);
		break;
	case Set_data_cb:
		(ws->*func)(ptr, (void *)tab->cdata);
		break;
	}
}

Button_bar::Button_bar(Component *p, Base_window *w, 
	Window_set *ws, const Button_bar_table **t, int nb, Help_id h) : 
	COMPONENT(p, 0, 0, h)
{
	const Button_bar_table	**tab;
	int			i;

	window = w;
	window_set = ws;
	table = t;
	nbuttons = nb;

	widget = XtVaCreateManagedWidget("button_bar", rubberTileWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNorientation, OL_HORIZONTAL,
		XtNshadowThickness, 0,
		0);
	
	Bar_data	*bar_data;
	list = bar_data = new Bar_data[nbuttons];
	for (i = 0, tab = table; i < nbuttons; ++tab, ++i, ++bar_data)
	{
		const Button_bar_table	*btn = *tab;
		bar_data->select_cb = (XtArgVal)buttonCB;
		if (btn->label)
			bar_data->label = (XtArgVal)btn->label; // user supplied
		else
			bar_data->label = 
			(XtArgVal)labeltab.get_label(btn->labelid);
		bar_data->mnemonic = 0;
		bar_data->sensitive = (XtArgVal)w->is_sensitive(btn->sensitivity);
		bar_data->entry = (XtArgVal)btn;
	}

	buttons = XtVaCreateManagedWidget("buttons", flatButtonsWidgetClass,
		widget,
		XtNuserData, this,
		XtNitemFields, bar_fields,
		XtNnumItemFields, XtNumber(bar_fields),
		XtNitems, list,
		XtNnumItems, nbuttons,
		XtNgravity, WestGravity,
		0);

	for (i = 0, bar_data = list; i < nbuttons; bar_data++, ++i)
	{
		if (((Button_bar_table *)bar_data->entry)->help_msg)
		{
			register_button_help(buttons, i, 
				(const char *)bar_data->label, 
				((Button_bar_table *)bar_data->entry)->help_msg);
		}
	}
}

Button_bar::~Button_bar()
{
	delete list;
}

void
Button_bar::set_sensitivity(int button, Boolean sense)
{
	OlVaFlatSetValues(buttons, button, XtNsensitive, sense, 0);
}
