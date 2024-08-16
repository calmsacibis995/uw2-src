#ident	"@(#)debugger:libol/common/Sel_list.C	1.12"

#include "UI.h"
#include "Component.h"
#include "Sel_list.h"
#include "Vector.h"
#include "str.h"
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

// fields describes Item_data for the flat list widget.  The two must be kept in sync
static String fields[] =
{
	XtNformatData,
};

struct Item_data
{
	const char **strings;
};

static void
select_CB(Widget, Selection_list *list, OlFlatCallData *ptr)
{
	if (!ptr || !list)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Callback_ptr	cb = list->get_select_cb();
	Command_sender	*creator = list->get_creator();
	(creator->*cb)(list, (void *)ptr->item_index);
}

static void
deselect_CB(Widget, Selection_list *list, OlFlatCallData *ptr)
{
	if (!ptr || !list)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Callback_ptr	cb = list->get_deselect_cb();
	Command_sender	*creator = list->get_creator();
	(creator->*cb)(list, (void *)ptr->item_index);
}

static void
default_CB(Widget, Selection_list *list, OlFlatCallData *ptr)
{
	if (!ptr || !list)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	Command_sender	*creator = list->get_creator();
	Callback_ptr	cb = list->get_default_cb();
	(creator->*cb)(list, (void *)ptr->item_index);
}

static void
drop_CB(Widget w, Selection_list *list, OlFlatDropCallData *ptr)
{
	Widget		destination;
	Component	*component = 0;

	// Look for the component the selection was dropped on.
	// Since several widgets may have been created for one component,
	// this may mean walking up the tree a few levels until one is
	// found that points back to a component
	// Note: there are really 3 cases here for the destination window:
	// 1) the root window, which is a child widget of a shell
	//    initialized at XtInitialize time. we assume that none of these
	//    widgets have XtNuserData set.
	// 2) a "foreign" window, i.e. one that belongs to another
	//    application. we assume that XtWindowToWidget() will
	//    return NULL for it
	// 3) one of debugger's windows. we assume that XtNuserData is
	//    set for the component.
	destination = XtWindowToWidget(XtDisplay(w), ptr->dst_info->window);
	while (destination)
	{
		XtVaGetValues(destination, XtNuserData, &component, 0);
		if (component)
			break;
		destination = XtParent(destination);
	}
	Callback_ptr	cb = list->get_drop_cb();
	Command_sender	*creator = list->get_creator();
	(creator->*cb)(list, component);
}

// If setting or resetting the list makes it overflow, the toolkit will call
// this function, which in turn calls set_overflow in the component to set
// a flag.  After the call to XtVaSetValues (that caused the overflow) returns,
// the component will check the flag, and call handle_overflow to set things
// back to a sane state
static void
overflow_CB(Widget, Selection_list *list, OlFListItemsLimitExceededCD *cdp)
{
	display_msg(E_ERROR, GE_list_overflow);
	list->set_overflow(cdp->preferred);
	cdp->ok = TRUE;
}

Selection_list::Selection_list(Component *p, const char *s, int rows, int cols,
	const char *format, int total, const char **ival, Select_mode m, 
	Command_sender *c, Callback_ptr sel_cb, Callback_ptr desel_cb,
	Callback_ptr def_cb, Callback_ptr cb, Help_id h) : COMPONENT(p, s, c, h)
{
	if (!ival)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	drop_cb = cb;
	select_cb = sel_cb;
	deselect_cb = desel_cb;
	default_cb = def_cb;

	columns = cols;
	total_items = total;
	pointers = 0;
	item_data = 0;
	overflow = FALSE;
	new_size = 0;
	allocate_pointers(total_items);

	for (int i = 0; i < total_items; i++)
	{
		char **ptr = item_data[i];
		for (int j = 0; j < columns; j++, ival++)
		{
			ptr[j] = makestr(*ival);
		}	
	}

	widget = XtVaCreateManagedWidget(label,
		scrolledWindowWidgetClass, parent->get_widget(),
		XtNuserData, this,
		0);

	// create the list with a small number of items to start with,
	// then reset later, after adding callback function to catch overflow
	list = XtVaCreateManagedWidget(label, flatListWidgetClass, widget,
		XtNformat, format,
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNitems, item_data,
		XtNnumItems, rows >= total_items ? total_items : rows,
		XtNnoneSet, TRUE,
		XtNexclusives, (m == SM_single) ? TRUE : FALSE,
		XtNclientData, this,
		XtNuserData, this,
		XtNviewHeight, rows,
		0);

	XtAddCallback(list, XtNitemsLimitExceeded, 
		(XtCallbackProc)overflow_CB,
		this);

	if (drop_cb)
		XtVaSetValues(list, XtNdropProc, (XtCallbackProc)drop_CB, 0);
	if (select_cb)
		XtVaSetValues(list, XtNselectProc, (XtCallbackProc)select_CB, 0);
	if (deselect_cb)
		XtVaSetValues(list, XtNunselectProc, (XtCallbackProc)deselect_CB, 0);
	if (default_cb)
		XtVaSetValues(list, XtNdblSelectProc, (XtCallbackProc)default_CB, 0);

	if (help_msg)
		register_help(widget, label, help_msg);

	if (rows < total_items)
	{
		XtVaSetValues(list, XtNitems, item_data, XtNnumItems, total_items, 0);
		if (overflow)
			handle_overflow();
	}
}

Selection_list::~Selection_list()
{
	for (int i = 0; i < total_items * columns; i++)
		delete pointers[i];
	delete pointers;
	delete item_data;
}

void
Selection_list::allocate_pointers(int rows)
{
	char	***olddata;
	char	**oldptr;
	char	**ptr;
	int	i;

	olddata = item_data;
	oldptr = pointers;
	if (rows)
	{
		pointers = new char *[rows * columns];
		item_data = new char **[rows];
		ptr = pointers;
		for (i = 0; i < rows; i++)
		{
			item_data[i] = ptr;
			ptr += columns;
		}
	}
	else
	{
		pointers = 0;
		item_data = 0;
	}

	if (oldptr)
	{
		for (i = 0; i < total_items * columns; i++)
			delete oldptr[i];
		delete oldptr;
	}
	delete olddata;
}

// flatList overflow.  new_size was set in set_overflow.
// truncate the list in 'item_data' to the new size.
void
Selection_list::handle_overflow()
{
	int	i;
	char	**ptr = pointers;

	pointers = new char *[new_size*columns];
	memcpy(pointers, ptr, new_size*columns*sizeof(char*));
	for (i = new_size; i < total_items; i++)
	{
		for (int j = 0; j < columns; j++)
			delete item_data[i][j];
	}
	delete item_data;
	delete ptr;

	item_data = new char **[new_size];
	ptr = pointers;
	for (i = 0; i < new_size; ++i)
	{
		item_data[i] = ptr;
		ptr += columns;
	}
	total_items = new_size;
	XtVaSetValues(list, XtNitems, item_data, 0);
	overflow = FALSE;
}
        
void
Selection_list::set_list(int new_total, const char **values)
{
	if (new_total != total_items)
	{
		allocate_pointers(new_total);
		total_items = new_total;
	}
	for (int i = 0; i < total_items; i++)
	{
		char **ptr = item_data[i];
		for (int j = 0; j < columns; j++, values++)
		{
			ptr[j] = makestr(*values);
		}	
	}

	XtVaSetValues(list,
		XtNitemsTouched, TRUE,
		XtNnumItems, total_items,
		XtNitems, item_data,
		0);
	if (overflow)
		handle_overflow();
}

int
Selection_list::get_selections(Vector *vector)
{
	int nsels = 0;

	vector->clear();
	for (int i = 0; i < total_items; i++)
	{
		Boolean is_set = FALSE;
		OlVaFlatGetValues(list, i, XtNset, &is_set, 0);
		if (is_set)
		{
			vector->add(&i, sizeof(int));
			nsels++;
		}
	}
	return nsels;
}

const char *
Selection_list::get_item(int row, int column)
{
	if (row >= total_items || column >= columns)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;
	}

	return item_data[row][column];
}

void
Selection_list::select(int row)
{
	OlVaFlatSetValues(list, row, XtNset, TRUE, 0);
}

void
Selection_list::deselect(int row)
{
	if (row >= 0)
	{
		OlVaFlatSetValues(list, row, XtNset, FALSE, 0);
	}
	else
	{
		for (int i = 0; i < total_items * columns; i++)
			OlVaFlatSetValues(list, i, XtNset, FALSE, 0);
	}
}

void
Selection_list::set_sensitive(Boolean sense)
{
	XtSetSensitive (list, sense);
}
