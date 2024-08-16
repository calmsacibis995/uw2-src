#ident	"@(#)debugger:libol/common/Alert_sh.C	1.8"

// GUI headers
#include "Alert_sh.h"
#include "UI.h"
#include "gui_label.h"
#include "Label.h"

// Debug headers
#include "List.h"

#include <Xol/Notice.h>
#include <Xol/FButtons.h>

static List notice_list;

Alert_shell	*old_alert_shell;
Alert_shell	*current_notice;

// Note - buttons are implemented using the Flat Button widget.
// If that is not available, use the Oblong Button widget

// Flat button callback
static void
alert_CB(Widget, Alert_shell *ptr, OlFlatCallData *fcd)
{
	Callback_ptr	hdlr = ptr->get_handler();
	Command_sender	*obj = ptr->get_object();

	if (!hdlr || !obj)
		return;
	(obj->*hdlr)(ptr, (void *)fcd->item_user_data);
}

static void
alert_popdown_CB(Widget, Alert_shell *ptr, XtPointer)
{
	old_alert_shell = ptr;
	current_notice = (Alert_shell *)notice_list.first();
	if (current_notice)
	{
		notice_list.remove(current_notice);
		XtPopup(current_notice->get_widget(), XtGrabExclusive);
	}
}

// fields and Button_data MUST be in sync
static String fields[] =
{
	XtNlabel,	// button label
	XtNselectProc,	// button callback
	XtNuserData,	// callback parameter
};

struct Button_data
{
	XtArgVal	label;
	XtArgVal	callback;
	XtArgVal	value;
};

Alert_shell::Alert_shell(const char *string, LabelId action, LabelId no_action,
	Callback_ptr h, void *obj) : COMPONENT(0, "notice", 0, HELP_none)
{
	Widget		text;
	Widget		control;
	int		nbuttons = (no_action != LAB_none) ? 2 : 1;
	const char	*lab = labeltab.get_label(LAB_notice);

	button_data = new Button_data[nbuttons];
	button_data[0].label = (XtArgVal)labeltab.get_label(action);
	button_data[0].callback = (XtArgVal)alert_CB;
	button_data[0].value = (XtArgVal)1;
	if (no_action != LAB_none)
	{
		button_data[1].label = (XtArgVal)labeltab.get_label(no_action);
		button_data[1].callback = (XtArgVal)alert_CB;
		button_data[1].value = (XtArgVal)0;
	}

	handler = h;
	object = (Command_sender *)obj;
	widget = XtVaCreatePopupShell(lab, noticeShellWidgetClass,
		base_widget, 0);
	XtAddCallback(widget, XtNpopdownCallback, (XtCallbackProc)alert_popdown_CB,
		(XtPointer)this);

	XtVaGetValues(widget, XtNtextArea, &text, XtNcontrolArea, &control, 0);
	XtVaSetValues(text, XtNstring, string, 0);

	XtVaCreateManagedWidget("buttons",
		flatButtonsWidgetClass, control,
		XtNnumItems, nbuttons,
		XtNitems, button_data,
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNclientData, this,
		0);
}

Alert_shell::~Alert_shell()
{
	XtDestroyWidget(widget);
	delete button_data;
}

void
Alert_shell::popup()
{
	if (current_notice)
	{
		// append to list
		notice_list.add(this);
	}
	else
	{
		current_notice = this;
		XtPopup(widget, XtGrabExclusive);
	}
}

void
Alert_shell::popdown()
{
	XtPopdown(widget);
}

void
Alert_shell::raise()
{
	Display *dpy = XtDisplay(widget);
	Window win = XtWindow(widget);

	XRaiseWindow(dpy, win);
	OlSetInputFocus(widget, RevertToParent, CurrentTime);
}

void
recover_notice()
{
	if (current_notice)
	{
		current_notice->raise();
	}
}
