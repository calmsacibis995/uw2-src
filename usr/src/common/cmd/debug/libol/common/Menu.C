#ident	"@(#)debugger:libol/common/Menu.C	1.20"

// GUI headers
#include "UI.h"
#include "Component.h"
#include "Menu.h"
#include "Window_sh.h"
#include "Windows.h"
#include "Machine.h"
#include "Base_win.h"
#include "Panes.h"
#include "Toolkit.h"
#include "gui_label.h"
#include "Label.h"

#include <stdio.h>
#include <string.h>

#ifdef MULTIBYTE
#include <wchar.h>
#endif

#include <Xol/FButtons.h>
#include <Xol/MenuShell.h>
#include <Xol/RubberTile.h>

// Using flat buttons, the menu bar is a flat widget, and the descriptor
// for each button contains a slot for that button's menu pane.
// That menu pane, in turn, holds a flat widget for the menu items
// If flat buttons are not available, each button in the menu bar
// and in each menu is an individual widget

// Menu bar descriptor - bar_fields and Bar_data must be in sync
static String bar_fields[] =
{
	XtNlabel,
	XtNmnemonic,
	XtNpopupMenu,
};

struct Bar_data
{
	XtArgVal	label;		// button label
	XtArgVal	mnemonic;
	XtArgVal	popup;
};

// descriptor for each individual menu - menu_fields and Menu_data must be in sync
static String menu_fields[] =
{
	XtNlabel,
	XtNmnemonic,
	XtNclientData,
	XtNdefault,
	XtNselectProc,
	XtNsensitive,
	XtNpopupMenu,
};

struct Menu_data
{
	XtArgVal	label;		// button label
	XtArgVal	mnemonic;
	XtArgVal	entry;		// Menu_table entry
	XtArgVal	default_action;	// is this the default button?
	XtArgVal	callback;	// callback function
	XtArgVal	sensitive;	// is the operation implemented and valid?
	XtArgVal	popup;
};

// we handle menu button callbacks by first registering a
// Timer callback which fires "immediately". The callback (menu_buttonCB)
// then simply returns, allowing the real work to be done
// by the timer callback (menu_buttonCB2). The reason is some callbacks 
// that invoke the framework could take a long time before the callback 
// returns to the toolkit. Because the toolkit grabs the pointer and does not
// relinquishes it until the callback returns, user is essentially blocked
// from getting to other clients on the desktop!
// NOTE #1: we assume that the Timer callback with a timeout interval of 0
// always gets handled before any other user events (not necessarily
// events from other sources). Thus we cannot have nested callbacks
// clobbering menuCBdata, e.g. 
// 	ButtonPress->callback1->ButtonPress->callback1->callback2->callback2
// NOTE #2: A side-effect of this scheme is that setting the breakpoint in a
// framework callback no longer blocks the desktop. In a sense, this
// is really a toolkit problem whose fix could benefit other clients
// both in terms of performance and debugging.

// data to be passed between menu_buttonCB and menubuttonCB2
static struct MenuCBDataRec
{
	XtIntervalId		id;
	Widget			w;
	const Menu_table	*tab;
} menuCBdata;

static void menu_buttonCB2(MenuCBDataRec *, XtIntervalId *);

// first menu button callback
static void
menu_buttonCB(Widget w, const Menu_table *tab, XtPointer)
{
	menuCBdata.w = w;
	menuCBdata.tab = tab;
	menuCBdata.id = XtAddTimeOut(0, (XtTimerCallbackProc)menu_buttonCB2, 
				(XtPointer)&menuCBdata);
}

// timer callback for calling the framework callback
static void
menu_buttonCB2(MenuCBDataRec *cd, XtIntervalId *id)
{
	if (cd->id != *id)
	{
		// see NOTE #1 above
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	Widget w = cd->w;
	const Menu_table *tab = cd->tab;

	XFlush(XtDisplay(w));	// make sure the ungrab and unmap requests
				// gets to the server.

	// do the real work here - the widget's user data is the
	// framework object, and func is its callback function
	Menu		*ptr;
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
		pane = win->get_pane(tab->pane_type);
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

#define MAX_TITLE_LEN	32

Menu::Menu(Component *p, LabelId l, Boolean has_title, Widget pwidget,
	Base_window *w, Window_set *ws, const Menu_table *ptr, int nb, Help_id h)
	: COMPONENT(p, 0, 0, h)
{
	window = w;
	window_set = ws;
	nbuttons = nb;
	table = ptr;
	mlabel = l;
	
	Menu_data	*menu_data;
	Arg		args[3];
	int		i, n = 0;

	delete_table = FALSE;

	if (has_title)
	{
		const char *lab = labeltab.get_label(mlabel);
		char	*title;

		if (ws->get_id() > 1)
		{
			title = new char[strlen(lab)+1+MAX_INT_DIGITS+1];
			sprintf(title, "%s %d", lab, ws->get_id());
		}
		else
			title = (char *)lab;
		XtSetArg(args[n], XtNpushpin, OL_OUT); n++;
		XtSetArg(args[n], XtNtitle, title); n++;
		label = title;
	}
	XtSetArg(args[n], XtNuserData, this); n++;
	widget = XtCreatePopupShell("menu", popupMenuShellWidgetClass, pwidget,
		args, n);

	list = menu_data = new Menu_data[nbuttons];
	for (i = 0; i < nbuttons; ++ptr, ++i, ++menu_data)
	{
		if (ptr->label == LAB_none)
			menu_data->label = (XtArgVal)ptr->name;
		else
			menu_data->label = 
				(XtArgVal)labeltab.get_label(ptr->label);
		if (ptr->mnemonic == LAB_none)
		{
#ifdef MULTIBYTE
			// cannot handle multibyte chars
			if (wcwidth(ptr->mnemonic_name) == 1)
				menu_data->mnemonic = ptr->mnemonic_name;
			else
				menu_data->mnemonic = 0;
#else
			menu_data->mnemonic = ptr->mnemonic_name;
#endif
		}
		else
			menu_data->mnemonic = (XtArgVal)*(labeltab.get_label(ptr->mnemonic));
		menu_data->entry = (XtArgVal)ptr;
		menu_data->default_action = (i == 0)
			? (XtArgVal)TRUE : (XtArgVal)FALSE;

		if (ptr->flags == Menu_button)
		{
			// cascading menu - should not have pushpin or title
			Menu *child = new Menu(this, ptr->label, FALSE, pwidget, w, ws,
				ptr->sub_table, ptr->cdata, ptr->help_msg);
			children.add(child);
			menu_data->callback = NULL;
			menu_data->sensitive = FALSE;
			menu_data->popup = (XtArgVal)child->get_widget();
		}
		else
		{
			menu_data->callback = (XtArgVal)menu_buttonCB;
			menu_data->sensitive = FALSE;
			menu_data->popup = NULL;
		}
	}

	menu = XtVaCreateManagedWidget("menu", flatButtonsWidgetClass, widget,
		XtNlayoutType, OL_FIXEDCOLS,
		XtNmeasure, 1,
		XtNitemFields, menu_fields,
		XtNnumItemFields, XtNumber(menu_fields),
		XtNitems, list,
		XtNnumItems, nbuttons,
		XtNuserData, this,
		0);

	for (i = 0, menu_data = list; i < nbuttons; i++, menu_data++)
	{
		if (((Menu_table *)menu_data->entry)->help_msg)
		{
			register_button_help(menu, i, 
				(const char *)menu_data->label, 
				((Menu_table *)menu_data->entry)->help_msg);
		}
	}
}

Menu *
Menu::find_item(LabelId label)
{
	for(Menu *mp = (Menu *)children.first(); 
				mp != NULL; 
				mp = (Menu *)children.next())
	{
		if (label == mp->mlabel)
			return mp;
	}
	return NULL;
}

Menu *
Menu::find_item(const char *name)
{
	for(Menu *mp = (Menu *)children.first(); 
				mp != NULL; 
				mp = (Menu *)children.next())
	{
		if (strcmp(name, mp->label) == 0)
			return mp;
	}
	return NULL;
}
// add an item to end of Menu_data list
void
Menu::add_item(const Menu_table *item)
{

	// contiguous Menu_table is needed for check_sensitivity
	const char *blabel;
	Menu_table *new_table = new Menu_table[nbuttons+1];
	memcpy(new_table, table, sizeof(Menu_table) * nbuttons);
	memcpy(&new_table[nbuttons], item, sizeof(Menu_table));
	if (delete_table)
		delete (Menu_table *)table;
	delete_table = TRUE;
	table = new_table;

	Menu_data *new_list = new Menu_data[nbuttons+1];
	memcpy(new_list, list, sizeof(Menu_data)*nbuttons);
	if (item->label == LAB_none)
		new_list[nbuttons].label = (XtArgVal)item->name;
	else
		new_list[nbuttons].label = 
			(XtArgVal)labeltab.get_label(item->label);
	blabel = (const char *)new_list[nbuttons].label;
	if (item->mnemonic == LAB_none)
	{
#ifdef MULTIBYTE
		// cannot handle multibyte chars
		if (wcwidth(item->mnemonic_name) == 1)
			new_list[nbuttons].mnemonic = item->mnemonic_name;
		else
			new_list[nbuttons].mnemonic = 0;
#else
		new_list[nbuttons].mnemonic = item->mnemonic_name;
#endif
	}
	else
		new_list[nbuttons].mnemonic = (XtArgVal)*(labeltab.get_label(item->mnemonic));
	new_list[nbuttons].entry = (XtArgVal)&table[nbuttons];
	// assume there is always at least one item on the list
	// by the time add_item is called, so, the new item is
	// never the default.
	new_list[nbuttons].default_action = (XtArgVal)FALSE;
	new_list[nbuttons].callback = (XtArgVal)menu_buttonCB;
	// assume no cascading menus can be added (yet), so there 
	// should always be a callback
	new_list[nbuttons].sensitive = item->callback ? 
			(XtArgVal)window->is_sensitive(item->sensitivity) : 
			(XtArgVal)FALSE;
	new_list[nbuttons].popup = NULL;
	++nbuttons;

	for (int i = 0; i < nbuttons; i++)
		new_list[i].entry = (XtArgVal)&table[i];
	delete list;
	list = new_list;

	XtVaSetValues(menu, 
		XtNitems, new_list, 
		XtNnumItems, nbuttons, 
		NULL, 0);

	if (item->help_msg)
		register_button_help(menu, nbuttons-1, blabel, item->help_msg);
}

void
Menu::delete_item(const	char * label)
{
	Menu_data *new_list, *old_list;
	int i;

	old_list = list;
	for(i = 0; i < nbuttons; ++i, ++old_list)
	{
		if(strcmp((char *)old_list->label, label) == 0)
			break;
	}
	if(i >= nbuttons)
		return;
	new_list = new Menu_data[nbuttons-1];
	if(i > 0)
		memcpy(new_list, list, sizeof(Menu_data)*i);
	if(i < nbuttons-1)
		memcpy(new_list+i, old_list+1, sizeof(Menu_data)*(nbuttons-i-1));
	--nbuttons;
	delete list;
	list = new_list;
	XtVaSetValues(menu, 
		XtNitems, new_list, 
		XtNnumItems, nbuttons, 
		NULL, 0);
}

Menu::~Menu()
{
	for (Menu *ptr = (Menu *)children.first(); ptr; ptr = (Menu *)children.next())
		delete ptr;
	if (delete_table)
		delete (Menu_table *)table;
}

void
Menu::set_sensitive(int button, Boolean value)
{
	OlVaFlatSetValues(menu, button, XtNsensitive, value, 0);
}

Menu_bar::Menu_bar(Component *p, Base_window *w, Window_set *ws,
	const Menu_bar_table *table, int nb, Help_id h) : COMPONENT(p, 0, 0, h)
{
	const Menu_bar_table	*tab;
	int			i;
	Widget			menubar;

	bar_table = table;
	nbuttons = nb;
	Bar_data	*bar_data;

	// allocate space for the menu bar's descriptor
	list = bar_data = new Bar_data[nbuttons];
	children = new Menu *[nbuttons];
	widget = XtVaCreateManagedWidget("menu", rubberTileWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNorientation, OL_HORIZONTAL,
		XtNshadowThickness, 0,
		0);
	
	for (i = 0, tab = table; i < nbuttons; ++tab, ++i, ++bar_data)
	{
		if (!tab->table)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}
		children[i] = new Menu(this, tab->label, TRUE, widget, w, ws, tab->table,
			tab->nbuttons, tab->help_msg);
		bar_data->label = (XtArgVal)labeltab.get_label(tab->label);
		bar_data->mnemonic = (XtArgVal)*(labeltab.get_label(tab->mnemonic));
		bar_data->popup = (XtArgVal)children[i]->get_widget();
	}

	menubar = XtVaCreateManagedWidget("menu", flatButtonsWidgetClass,
		widget,
		XtNuserData, this,
		XtNitemFields, bar_fields,
		XtNnumItemFields, XtNumber(bar_fields),
		XtNitems, list,
		XtNnumItems, nbuttons,
		XtNmenubarBehavior, TRUE,
		XtNgravity, WestGravity,
		0);

	for (i = 0, bar_data = list, tab = table;
		i < nbuttons-1; ++tab, ++i, bar_data++)
	{
		if (tab->help_msg)
		{
			register_button_help(menubar, i, 
				(const char *)bar_data->label, tab->help_msg);
		}
	}
}

Menu *
Menu_bar::find_item(LabelId lab)
{
	Menu	**mp = children;
	for(int i = 0; i < nbuttons; ++i, ++mp)
	{
		if ((*mp)->get_label() == lab)
			return *mp;
	}
	return NULL;
}

Menu_bar::~Menu_bar()
{
	for (int i = 0; i < nbuttons; i++)
	{
		delete children[i];
	}
	delete children;
	delete list;
}
