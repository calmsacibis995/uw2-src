#ident	"@(#)debugger:libol/common/Dialog_sh.C	1.31"

// GUI headers
#include "Help.h"
#include "UI.h"
#include "Component.h"
#include "Dialog_sh.h"
#include "Dialogs.h"
#include "Windows.h"
#include "gui_label.h"
#include "Label.h"

// Debug headers
#include "Machine.h"
#include "Message.h"
#include "Msgtab.h"
#include "str.h"

#include <string.h>
#include <stdio.h>
#include <X11/Xatom.h>

#include <Xol/PopupWindo.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include <Xol/RubberTile.h>
#include <Xol/OlCursors.h>
#include <DnD/OlDnDVCX.h>

#define TITLE_SIZE	80

struct DNDdata
{
	Drop_cb_action callback_action;
	Callback_ptr callback;
	char *drop_content;
	Atom atom;
};

// fields describes the Button_data structure for the flat buttons widget
// fields and Button_data MUST be in sync
static String fields[] =
{
	XtNlabel,
	XtNmnemonic,
	XtNselectProc,
	XtNuserData,
	XtNdefault,
	XtNsensitive,
};

struct Button_data
{
	XtArgVal label;		// button name
	XtArgVal mnemonic;	// keyboard shortcut
	XtArgVal callback;	// one of the callbacks defined in this file, not
				// a framework callback
	XtArgVal button_ptr;	// pointer into the Button array for the mnemonic and
				// framework callback
	XtArgVal default_action; // TRUE if this is the Apply button
	XtArgVal sensitive;	// TRUE if callback implemented and state ok
};

// the dialog has been dismissed (by the window manager, for example)
static void
dialog_dismissCB(Widget w, XtPointer, XtPointer)
{
	Dialog_shell	*ptr = 0;
	Callback_ptr	func;

	XtVaGetValues(w, XtNuserData, &ptr, 0);
	ptr->set_is_open(FALSE);

	func = ptr->get_dismiss_cb();
	Dialog_box	*creator = (Dialog_box *)ptr->get_creator();
	if (!creator)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	creator->dismiss();
	if (func)
		(creator->*func)(ptr, 0);
}

// pop down the dialog (Close button)
static void
dialog_popdownCB(Widget w, XtPointer, XtPointer)
{
	OlDefine	pin = 0;
	Dialog_shell	*dptr = 0;

	XtVaGetValues(w, XtNuserData, &dptr, 0);
	if (!dptr || !dptr->is_open())
		return;

	XtVaGetValues(dptr->get_popup_window(), XtNpushpin, &pin, 0);
	if (gui_mode != OL_OPENLOOK_GUI || pin == OL_OUT)
	{
		XtPopdown(dptr->get_popup_window());
		dptr->set_is_open(FALSE);
	}
}

// invoke the associated framework callback
// if the callback doesn't call wait_for_response() (incrementing cmds_sent) the window
// will be popped down when the callback returns
static void
dialog_execCB(Widget w, XtPointer cdata, OlFlatCallData *arg)
{
	Command_sender	*creator;
	Dialog_shell	*dptr = 0;
	char		mne = 0;
	Callback_ptr	func;

	XtVaGetValues(w, XtNuserData, &dptr, 0);
	if (!dptr || !dptr->is_open())
		return;

	if (arg)
	{
		Button	*button = 0;

		OlVaFlatGetValues(w, arg->item_index, 
			XtNmnemonic, &mne, 
			XtNuserData, &button,
			0);
		if (!button || !button->func)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		func = button->func;
	}
	else
		// called from DND selectionCB
		func = *(Callback_ptr *)cdata;

	dptr->set_ok_to_popdown(TRUE);
	dptr->set_busy(TRUE);
	dptr->clear_msg();
	creator = dptr->get_creator();
	(creator->*func)(dptr, (void *)mne);
	if (!dptr->get_cmds_sent())
	{
		dptr->set_busy(FALSE);
		dptr->popdown();
	}
}
	
// invoke the associated framework callback, without closing the window
static void
dialog_non_execCB(Widget w, XtPointer cdata, OlFlatCallData *arg)
{
	Command_sender	*creator;
	Dialog_shell	*dptr = 0;
	char		mne = 0;
	Callback_ptr	func;

	XtVaGetValues(w, XtNuserData, &dptr, 0);
	if (!dptr || !dptr->is_open())
		return;

	if (arg)
	{
		Button	*button = 0;

		OlVaFlatGetValues(w, arg->item_index, 
			XtNmnemonic, &mne, 
			XtNuserData, &button,
			0);
		if (!button || !button->func)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		func = button->func;
	}
	else
		// called from DND selectionCB
		func = *(Callback_ptr *)cdata;

	dptr->set_busy(TRUE);
	dptr->clear_msg();
	dptr->set_ok_to_popdown(FALSE);
	creator = dptr->get_creator();
	(creator->*func)(dptr, (void *)mne);
	if (!dptr->get_cmds_sent())
		dptr->set_busy(FALSE);
}

// invoke the help window
static void
dialog_help(Widget w, XtPointer, XtPointer)
{
	Dialog_shell	*dptr = 0;

	XtVaGetValues(w, XtNuserData, &dptr, 0);
	if (!dptr || !dptr->get_help_msg())
		return;

	display_help(dptr->get_widget(), HM_section, dptr->get_help_msg());
}

// get rid of old error messages
void
Dialog_shell::clear_msg()
{
	XtVaSetValues(msg_widget, XtNstring, " ", 0);
	delete error_string;
	error_string = 0;
}

// create the row of buttons at the bottom of the window
// if flat buttons are not available use a controlAreaWidget- the widget
// will have to be created first - before processing the buttons
void
Dialog_shell::make_buttons(const Button *b, int nb)
{
	if (!b)
		return;

	Callback_ptr	apply = 0;
	Button_data	*item;

	buttons = new Button_data[nb];
	item = buttons;
	for (int i = 0; i < nb; b++, i++)
	{
		XtCallbackProc	cb = 0;
		XtArgVal	is_default = (i == 0);
		LabelId 	name;

		switch (b->type)
		{
		// OK and Apply are the same for OpenLook,
		// since the window may be pinned
		case B_ok:
		case B_apply:
			if (apply)	// already saw B_ok or B_apply
				continue;
			name = b->label ? (LabelId)b->label : (LabelId)LAB_apply;
			apply = b->func;
			if (b->func)
				cb = (XtCallbackProc)dialog_execCB;
			break;

		// Close dismisses the popup, without changing any controls
		case B_close:
			name = b->label ? (LabelId)b->label : (LabelId)LAB_close;
			cb = dialog_popdownCB;
			break;

		// Cancel is equivalent to Reset followed by Close
		case B_cancel:
			name = b->label ? (LabelId)b->label : (LabelId)LAB_cancel;
			cb = (XtCallbackProc)dialog_execCB;
			break;

		// Reset the dialog to its state when last popped up
		case B_reset:
			name = b->label ? (LabelId)b->label : (LabelId)LAB_reset;
			if (b->func)
				cb = (XtCallbackProc)dialog_non_execCB;
			break;

		// execute the associated action then dismiss the window
		case B_exec:
			name = b->label ? (LabelId)b->label : (LabelId)LAB_apply;
			if (b->func)
				cb = (XtCallbackProc)dialog_execCB;
			break;

		// execute the associated action without dismissing the window
		case B_non_exec:
			name = b->label;
			if (b->func)
				cb = (XtCallbackProc)dialog_non_execCB;
			if (is_default)
				default_is_exec = FALSE;
			break;

		case B_help:
			if (!help_msg)
				continue;
			name = LAB_help;
			cb = (XtCallbackProc)dialog_help;
			break;

		default:
			break;
		}

		item->callback = (XtArgVal)cb;
		item->label = (XtArgVal)labeltab.get_label(name);
		if (b->mnemonic)
		{
			const char *mne = labeltab.get_label(b->mnemonic);
			item->mnemonic = (unsigned char)*mne;
		}
		else
			item->mnemonic = (unsigned char)*(const char *)item->label;
		item->button_ptr = (XtArgVal)b;
		item->default_action = is_default;
		item->sensitive = cb ? TRUE : FALSE;
		item++;
	}

	nbuttons = item-buttons;	// may be less than the value of i
	control_area = XtVaCreateManagedWidget("lower control",
		flatButtonsWidgetClass, widget,
		XtNitemFields, fields,
		XtNnumItemFields, XtNumber(fields),
		XtNnumItems, nbuttons,
		XtNitems, buttons,
		XtNweight, 0,
		XtNuserData, this,
		0);
}

// Dialog_shell is implemented as transient shell with box, control area
// (for buttons) and footer children, instead of using the OpenLook
// popup window widget, because, unfortunately, the upper control area
// in a popup window widget doesn't allow for resizing.

Dialog_shell::Dialog_shell(Component *p, LabelId tid, Callback_ptr d, Dialog_box *c,
	const Button *bptr, int num_buttons, Help_id h, Callback_ptr dcb, 
	Drop_cb_action dcb_act) : COMPONENT(p, 0, c, h, DIALOG_SHELL)
{
	Dimension	shadow = 0;
	const char	*dlabel = labeltab.get_label(LAB_debug);

	label = labeltab.get_label(tid);

	if (!creator)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	char *title = new char[strlen(dlabel) + 3 + MAX_INT_DIGITS + strlen(label) + 1];

	int	id = ((Dialog_box *)creator)->get_window_set()->get_id();
	if (id > 1)
		sprintf(title, "%s %d: %s", dlabel, id, label);
	else
		sprintf(title, "%s: %s", dlabel, label);

	dismiss = d;
	ok_to_popdown = FALSE;
	cmds_sent = 0;
	default_is_exec = TRUE;
	child = 0;
	error_string = 0;
	_is_open = FALSE;
	drop_cb = dcb;
	drop_cb_act = dcb_act;
	drop_data = 0;
	is_busy = FALSE;
	busy_buttons = 0;
	popup_focus_widget = 0;

	popup_window = XtVaCreatePopupShell(label, transientShellWidgetClass,
		parent->get_widget(),
		XtNtransientFor, parent->get_widget(),
		XtNtitle, title,
		XtNpushpin, OL_OUT,
		XtNmenuButton, FALSE,
		XtNuserData, this,
		XtNresizeCorners, TRUE,
		0);

	XtAddCallback(popup_window, XtNpopdownCallback,
		(XtCallbackProc)dialog_dismissCB, (XtPointer)&dismiss);
	OlAddCallback(popup_window, XtNconsumeEvent, (XtCallbackProc)eventCB, 0);

	// if the rubberTile widget is not available, use a form widget,
	// although you may have to restrict the resizing capabilities
	widget = XtVaCreateManagedWidget(label, rubberTileWidgetClass,
		popup_window,
		XtNuserData, this,
		XtNorientation, OL_VERTICAL,
		0);

	XtVaGetValues(widget, XtNshadowThickness, &shadow, 0);
	XtVaSetValues(widget,
		XtNleftMargin, shadow + 2,
		XtNrightMargin, shadow + 2,
		XtNtopMargin, shadow + 2,
		XtNbottomMargin, shadow + 2,
		0);

	make_buttons(bptr, num_buttons);

	msg_widget = XtVaCreateManagedWidget("error widget",
		staticTextWidgetClass, widget,
		XtNuserData, this,
		XtNstring, " ",
		XtNgravity, WestGravity,
		XtNrefWidget, control_area,
		XtNrefSpace, shadow + 2,
		XtNweight, 0,
		0);

	if (help_msg)
		register_help(popup_window, label, help_msg);

	delete title;
}


Dialog_shell::~Dialog_shell()
{
	XtDestroyWidget(popup_window);
	delete child;
	delete error_string;
	delete buttons;
	delete busy_buttons;
}

void
Dialog_shell::add_component(Component *box)
{
	if (!box || !box->get_widget() || child)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	child = box;

	Dimension	shadow = 0;
	XtVaGetValues(widget, XtNshadowThickness, &shadow, 0);

	// weight of 1 allows the child component to be resized
	XtVaSetValues(box->get_widget(),
		XtNweight, 1,
		XtNrefSpace, shadow + 2,
		0);

	// buttons go at the bottom, even though created before box
	XtVaSetValues(control_area,
		XtNrefWidget, box->get_widget(),
		XtNrefSpace, shadow + 2,
		0);
}

// called when selection value (file name) has been obtained
// selectionCB is called indirectly from triggerNotify
static void
selectionCB(Widget w, XtPointer client_data, Atom * selection, Atom *type, 
	XtPointer value, unsigned long *, int *)
{
	DNDdata *dp = (DNDdata *)client_data;
	Display *dpy = XtDisplay(w);

	// we are not interested in the other kinds of objects (like selected text)
	// that may be dropped.
	if (*type == OL_XA_FILE_NAME(dpy))
	{
		char *fname = (char *)value;

		dp->drop_content = makestr(fname);
		if (dp->callback_action == Drop_cb_popdown)
			dialog_execCB(w, &(dp->callback), NULL);
		else
			dialog_non_execCB(w, &(dp->callback), NULL);
		XtFree(fname);
		OlDnDDragNDropDone(w, *selection, 
			XtLastTimestampProcessed(dpy), NULL, NULL);
	}
}

// Called when a drop is made from the desktop
// Even though the drop has been made, the dropped on object (the destination)
// doesn't know what the dropped object (the source) is.  Calling
// XtGetSelectionValue gets the name of the dropped file, and calls
// selectionCB to hand off that information.
static Boolean
triggerNotify(Widget w, Window, Position, Position, Atom selection, 
	Time timestamp, OlDnDDropSiteID, OlDnDTriggerOperation, 
	Boolean, Boolean, XtPointer closure)
{
	DNDdata *dp = (DNDdata *)closure;

	if (!dp)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return False;
	}

	if (dp->drop_content)
	{
		delete dp->drop_content;
		dp->drop_content = NULL;
	}
	XtGetSelectionValue(w, selection, dp->atom, 
		selectionCB, (XtPointer)dp, timestamp);
	return True;
}

// return the item dropped from the desktop
char *
Dialog_shell::get_drop_item()
{
	if (drop_cb && drop_data)
		return drop_data->drop_content;
	return NULL;
}

void
Dialog_shell::set_popup_focus(Component *cp)
{
	popup_focus_widget = cp->get_widget();
}

void
Dialog_shell::popup()
{
	Display *dpy = XtDisplay(popup_window);

	clear_msg();
	if (_is_open)
	{
		XRaiseWindow(dpy, XtWindow(popup_window));
		Widget focusw = popup_focus_widget ? 
			popup_focus_widget : popup_window;
		OlMoveFocus(focusw, OL_IMMEDIATE, CurrentTime);
		recover_notice();
		return;
	}

	_is_open = TRUE;
	ok_to_popdown = FALSE;
	XtPopup(popup_window, XtGrabNone);
	if (popup_focus_widget)
		OlMoveFocus(popup_focus_widget, OL_IMMEDIATE, CurrentTime);
	if (drop_cb)
	{
		// register the drop site for desktop
		// this has to be done every time the dialog is popped up,
		// it is unregistered automatically when the dialog is popped down
		OlDnDSiteRect rect;
		Dimension w,h;

		XtVaGetValues (popup_window,
			XtNwidth, &w,
			XtNheight, &h,
			0);
		rect.x  = 0;
		rect.y = 0;
		rect.width = w;
		rect.height = h;
		drop_data = new DNDdata;
		drop_data->atom = OL_XA_FILE_NAME(dpy);
		drop_data->callback_action = drop_cb_act;
		drop_data->callback = drop_cb;
		drop_data->drop_content = NULL;
		OlDnDRegisterWidgetDropSite(popup_window,
			OlDnDSitePreviewNone,
			&rect, 1,
			(OlDnDTMNotifyProc)triggerNotify,
			(OlDnDPMNotifyProc)NULL,
			True,	/* interest */
			(XtPointer)drop_data
		);
	}
	recover_notice();
}

// If there were no errors, dismiss the window.  If there were
// errors, the window is left up so the user has time to read the error
// message (of course, that means that the user has to do something to
// get rid of the popup)
void
Dialog_shell::popdown()
{
	if (cmds_sent)
		return;

	OlDefine	pin = OL_OUT;

	XtVaGetValues(popup_window, XtNpushpin, &pin, 0);
	// OpenLook bug -- sometimes the pushpin disappears unless menuButton
	// is explicitly reset
	XtVaSetValues(popup_window, XtNmenuButton, FALSE, 0);
	if (ok_to_popdown && !error_string)
	{
		if (gui_mode != OL_OPENLOOK_GUI || pin == OL_OUT)
		{
			XtPopdown(popup_window);
			_is_open = FALSE;
			if (drop_cb && drop_data)
			{
				if (drop_data->drop_content)
					delete drop_data->drop_content;
				delete drop_data;
				drop_data = 0;
			}
		}
	}
}

Boolean
Dialog_shell::is_pinned()
{
	if (gui_mode == OL_OPENLOOK_GUI)
	{
		OlDefine	pin = OL_OUT;
		XtVaGetValues(popup_window, XtNpushpin, &pin, 0);
		// OpenLook bug -- sometimes the pushpin disappears unless
		// menuButton is explicitly reset
		XtVaSetValues(popup_window, XtNmenuButton, FALSE, 0);
		if (pin == OL_IN)
			return TRUE;
	}
	return FALSE;
}
	
void
Dialog_shell::cmd_complete()
{
	if (cmds_sent)
	{
		cmds_sent = FALSE;
		set_busy(FALSE);
		if (ok_to_popdown)
		{
			popdown();
		}
	}
}

// This function isn't inlined, even though it is an obvious candidate, because
// there doesn't seem to be any way to do it, since the interface is in the common
// header but the implementation is toolkit specific
void
Dialog_shell::wait_for_response()
{
	cmds_sent = TRUE;
}

// display an error or warning message in the dialog's footer
void
Dialog_shell::error(Severity sev, const char *message)
{
	if (!error_string)
	{
		const char *wlabel;
		size_t	len = strlen(message);
		if (sev == E_WARNING)
		{
			wlabel = labeltab.get_label(LAB_warning_line);
			len += strlen(wlabel) + 1;
		}
		error_string = new char[len + 1];
		if (sev == E_WARNING)
		{
			sprintf(error_string, "%s %s", wlabel, message);
		}
		else
			strcpy(error_string, message);
		// chop off ending new line
		if (error_string[len-1] == '\n')
			len--;
		error_string[len] = '\0';

		// reset the string
		XtVaSetValues(msg_widget, XtNstring, error_string, 0);
	}
}

void
Dialog_shell::error(Severity severity, Gui_msg_id mid, ...)
{
	va_list		ap;

	va_start(ap, mid);
	const char *message = do_vsprintf(gm_format(mid), ap);
	va_end(ap);

	error(severity, message);
}

void
Dialog_shell::error(Message *msg)
{
	if (Mtable.msg_class(msg->get_msg_id()) == MSGCL_error)
		error(msg->get_severity(), msg->format());
	else
		error(E_NONE, msg->format());
}

// set_busy sets the busy indicator until the command is completed
// set_busy(FALSE) unsets the busy indicator
void
Dialog_shell::set_busy(Boolean busy)
{
	busy_window(popup_window, busy);
	if (busy)
	{
		if (is_busy)
			// already busied
			return;
		is_busy = TRUE;
		if (!busy_buttons)
			busy_buttons = new Boolean[nbuttons];
		Button_data	*bp = buttons;
		for (int i = 0; i < nbuttons; ++i, ++bp)
		{
			// save old sensitive state
			busy_buttons[i] = (Boolean)bp->sensitive;
			// set button insensitive, but careful not to
			// steal the focus (OlVaFlatSetValues does!)
			// stealing the focus results in loss of 
			// selection in the Source pane in the case
			// of a search from the search dialog
			bp->sensitive = (XtArgVal)FALSE;
			OlFlatRefreshItem(control_area, i, TRUE); 
		}
	}
	else
	{
		if (!is_busy)
			// already unbusied
			return;
		is_busy = FALSE;
		for (int i = 0; i < nbuttons; ++i)
			// if button was sensitive and still is, restore its
			// sensitivity
			if (busy_buttons[i])
				OlVaFlatSetValues(control_area, i, 
					XtNsensitive, TRUE, 0);
	}
}

void
Dialog_shell::set_label(LabelId new_label, LabelId mnemonic)
{
	const char	*dlabel = labeltab.get_label(LAB_debug);
	const char	*nlabel = labeltab.get_label(new_label);
	const char	*mne = labeltab.get_label(mnemonic);

	char *title = new char[strlen(dlabel) + 3 + MAX_INT_DIGITS + strlen(nlabel) + 1];
	int	id = ((Dialog_box *)creator)->get_window_set()->get_id();

	if (id > 1)
		sprintf(title, "%s %d: %s", dlabel, id, nlabel);
	else
		sprintf(title, "%s: %s", dlabel, nlabel);
	XtVaSetValues(popup_window, XtNtitle, title, 0);

	// reset the label on the default button (the zero-th entry)
	OlVaFlatSetValues(control_area, 0,
		XtNlabel, nlabel,
		XtNmnemonic, *mne,
		0);
	delete title;
}

void
Dialog_shell::set_focus(Component *p)
{
	OlMoveFocus(p->get_widget(), OL_IMMEDIATE, CurrentTime);
}

void
Dialog_shell::set_sensitive(LabelId lab, Boolean s)
{
	Button_data	*bp = buttons;
	int		i;

	for (i = 0; i < nbuttons; i++, bp++)
	{
		if (((Button *)bp->button_ptr)->label == lab &&
		    (Boolean)bp->sensitive != s)
		{
			if (is_busy)
				busy_buttons[i] = s;
			else
				OlVaFlatSetValues(control_area, i, 
					XtNsensitive, s, 0);
			break;
		}
	}
}

void
Dialog_shell::set_sensitive(Boolean s)
{
	Button_data	*bp = buttons;
	int	i;

	for (i = 0; i < nbuttons; i++, bp++)
	{
		// close is always sensitive
		switch (((Button *)bp->button_ptr)->type)
		{
		case B_close:
		case B_help:
		case B_cancel:
			continue;
		}
		// assume that set_sensitive(Boolean) is never called
		// in busy mode
		if ((Boolean)bp->sensitive != s)
			OlVaFlatSetValues(control_area, i, XtNsensitive, s, 0);
	}
}

void
Dialog_shell::set_help(Help_id help)
{
	if (!help)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	help_msg = help;
	register_help(popup_window, label, help_msg);
}

Boolean
Dialog_shell::is_open()
{
	return _is_open;
}

// called by framework at the start of default activation cb
void
Dialog_shell::default_start()
{
	set_busy(TRUE);
	clear_msg();
	ok_to_popdown = default_is_exec ? TRUE : FALSE;
}

// called by framework at the end of default activation cb
void
Dialog_shell::default_done()
{
	if (!cmds_sent)
	{
		set_busy(FALSE);
		if (default_is_exec)
			popdown();
	}
}
