#ident	"@(#)debugger:libol/common/Caption.C	1.7"

#include "UI.h"
#include "Component.h"
#include "Caption.h"
#include "gui_label.h"
#include "Label.h"
#include <Xol/Form.h>
#include <Xol/StaticText.h>

// Caption is implemented with a form widget instead of the OpenLook caption
// widget because the caption widget doesn't resize its children
Caption::Caption(Component *p, LabelId lid, Caption_position pos, Help_id h)
	: COMPONENT(p, 0, 0, h)
{
	label = labeltab.get_label(lid);
	if (!label || *label == 0)
	// at least one blank or we can't expand label later
		label = " ";
	child = 0;
	position = pos;
	widget = XtVaCreateManagedWidget(label, formWidgetClass,
		parent->get_widget(),
		XtNuserData, this,
		XtNshadowThickness, 0,
		0);

	if (position == CAP_TOP_CENTER)
		// when the Component is resized, the caption should still
		// be centered
		caption = XtVaCreateManagedWidget(label, staticTextWidgetClass,
			widget,
			XtNxResizable, TRUE,
			XtNxAttachRight, TRUE,
			XtNxAttachOffset, PADDING,
			XtNgravity, CenterGravity,
			XtNalignment, OL_CENTER,
			XtNstring, label,
			XtNuserData, this,
			XtNrecomputeSize, FALSE,
			0);
	else
		caption = XtVaCreateManagedWidget(label, staticTextWidgetClass,
			widget,
			XtNstring, label,
			XtNuserData, this,
			0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Caption::~Caption()
{
	delete child;
}

void
Caption::add_component(Component *c, Boolean resizable)
{
	Arg	args[7];
	int	n = 0;

	if (child)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	child = c;

	Widget child_widget = child->get_widget();
	Dimension shadow = 0;

	XtVaGetValues(child_widget, XtNshadowThickness, &shadow, 0);

	if (position == CAP_TOP_CENTER)
	{
		XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
		XtSetArg(args[n], XtNyRefWidget, caption); n++;
		XtSetArg(args[n], XtNyOffset, shadow+2); n++;
		XtSetArg(args[n], XtNxResizable, TRUE); n++;
		XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
		if (resizable)
		{
			XtSetArg(args[n], XtNyResizable, TRUE); n++;
			XtSetArg(args[n], XtNyAttachBottom, TRUE); n++;
		}
	}
	else
	{
		if (position == CAP_TOP_LEFT)
		{
			XtSetArg(args[n], XtNyAddHeight, TRUE); n++;
			XtSetArg(args[n], XtNyRefWidget, caption); n++;
			XtSetArg(args[n], XtNyOffset, shadow+2); n++;
		}
		else
		{
			// position == CAP_LEFT
			XtSetArg(args[n], XtNxAddWidth, TRUE); n++;
			XtSetArg(args[n], XtNxRefWidget, caption); n++;
			XtSetArg(args[n], XtNxOffset, shadow+2); n++;
		}

		if (resizable)
		{
			XtSetArg(args[n], XtNxResizable, TRUE); n++;
			XtSetArg(args[n], XtNxAttachRight, TRUE); n++;
			XtSetArg(args[n], XtNyResizable, TRUE); n++;
			XtSetArg(args[n], XtNyAttachBottom, TRUE); n++;
		}
	}
	XtSetValues(child_widget, args, n);
}

// change the caption's text
void
Caption::set_label(const char *s)
{
	XtVaSetValues(caption, XtNstring, s, 0);
}
