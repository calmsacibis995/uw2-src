#ident	"@(#)debugger:libol/common/Stext.C	1.2"

#include "Component.h"
#include "Stext.h"

#include <Xol/StaticText.h>

Simple_text::Simple_text(Component *p, const char *s, Boolean resize, Help_id h)
	: COMPONENT(p, s, 0, h)
{
	widget = XtVaCreateManagedWidget("simple text", staticTextWidgetClass,
		parent->get_widget(),
		XtNstring, label,
		XtNgravity, WestGravity,
		XtNrecomputeSize, resize,
		XtNuserData, this,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Simple_text::~Simple_text()
{
}


// change the text displayed
void
Simple_text::set_text(const char *s)
{
	if (!s)
		s = "";
	XtVaSetValues(widget, XtNstring, s, 0);
}
