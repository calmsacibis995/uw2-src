#ident	"@(#)debugger:libol/common/Boxes.C	1.9"

#include "UI.h"
#include "Component.h"
#include "Boxes.h"
#include <Xol/Form.h>
#include <Xol/RubberTile.h>
#include <Xol/Panes.h>
#include <Xol/Handles.h>

// Boxes do not need to save the creator since they have no callbacks

Box::Box(Component *p, const char *s, Orientation o, Box_type t,
	Help_id h)
	: COMPONENT(p, s, 0, h)
{
	orientation = o;
	type = t;
}

Box::~Box()
{
	Component *ptr;

	ptr = (Component *)children.first();
	for (; ptr; ptr = (Component *)children.next())
		delete ptr;
}

// pure virtual function
void
Box::add_component(Component *, Boolean)
{
}

Packed_box::Packed_box(Component *p, const char *s, Orientation o, Help_id h)
	: BOX(p, s, o, Box_packed, h)
{
	widget = XtVaCreateManagedWidget(label, formWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNshadowThickness, 0,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

void
Packed_box::add_component(Component *p, Boolean)
{
	Component	*sib = (Component *)children.last();
	Widget		w = p->get_widget();
	Arg		args[6];
	int		n = 3;

	if (orientation == OR_horizontal)
	{
		// make resizable in the vertical dimension
		XtSetArg(args[0], XtNyResizable, TRUE);
		XtSetArg(args[1], XtNyAttachBottom, TRUE);
		XtSetArg(args[2], XtNxResizable, FALSE);
		if (sib)
		{
			Widget sw = sib->get_widget();

			// position to the right of the previous widget
			XtSetArg(args[3], XtNxRefWidget, sw);
			XtSetArg(args[4], XtNxAddWidth, TRUE);
			XtSetArg(args[5], XtNxOffset, get_widget_pad(w,sw));
			n = 6;
		}
	}
	else
	{
		// make resizable horizontally
		XtSetArg(args[0], XtNxResizable, TRUE);
		XtSetArg(args[1], XtNxAttachRight, TRUE);
		XtSetArg(args[2], XtNyResizable, FALSE);
		if (sib)
		{
			Widget sw = sib->get_widget();

			// position below the previous widget
			XtSetArg(args[3], XtNyRefWidget, sw);
			XtSetArg(args[4], XtNyAddHeight, TRUE);
			XtSetArg(args[5], XtNyOffset, get_widget_pad(w,sw));
			n = 6;
		}
	}
	XtSetValues(w, args, n);

	children.add(p);
}

// Expansion box is simple, if Rubber Tiles are available.  Trying to do
// it with a form is more difficult...
Expansion_box::Expansion_box(Component *p, const char *s, Orientation o,
	Help_id h) : BOX(p, s, o, Box_expansion, h)
{
	widget = XtVaCreateManagedWidget(label, rubberTileWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNshadowThickness, 0,
		XtNorientation, o == OR_horizontal ? OL_HORIZONTAL : OL_VERTICAL,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

void
Expansion_box::add_component(Component *p, Boolean elastic)
{
	Component 	*sib = (Component *)children.last();
	Widget		w = p->get_widget();
	Arg		args[3];
	int		n = 0;

	if (sib)
	{
		Widget sw = sib->get_widget();

		XtSetArg(args[n], XtNrefWidget, sw); n++;
		XtSetArg(args[n], XtNrefSpace, get_widget_pad(w,sw)); n++;
	}

	if (elastic)
	{
		// weight of 1 will give this component benefits of resizing
		XtSetArg(args[n], XtNweight, 1); n++;
	}
	else
	{
		// weight of zero means it won't be resized
		XtSetArg(args[n], XtNweight, 0); n++;
	}
	XtSetValues(w, args, n);
	children.add(p);
}

// If panes are not available, this can be implemented with a form widget,
// but that would not allow the children to be resized relative to one another
Divided_box::Divided_box(Component *p, const char *s, Boolean handles, Help_id h)
	: BOX(p, s, OR_vertical, Box_divided, h)
{
	// make a panes widget with handles
	widget = XtVaCreateManagedWidget(label, panesWidgetClass,
		parent->get_widget(),
		// workaround for toolkit bug:
		//   when layoutWidth is also constrained, in the
		//   presence of the scrolledWindow widget, the panes
		//   will shrink, eventually leaving the display area
		//   too small for any line to be visible.
		// layoutWidth's default is OL_MINIMIZE
		//XtNlayoutWidth, OL_IGNORE,
		XtNlayoutHeight, OL_IGNORE,
		XtNshadowThickness, 0,
		0);
	if (handles)
		(void)XtCreateManagedWidget("handles", handlesWidgetClass,
			widget, 0, 0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

void
Divided_box::add_component(Component *p, Boolean)
{
	children.add(p);
}
