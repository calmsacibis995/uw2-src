#ident	"@(#)debugger:libol/common/Slider.C	1.2"

#include "UI.h"
#include "Component.h"
#include "Slider.h"
#include "Machine.h"
#include <Xol/Slider.h>
#include <stdio.h>

Slider::Slider(Component *p, const char *s, Orientation orient,
	int min, int max, int initial, int granularity,
	Help_id h) : COMPONENT(p, s, 0, h)
{
	char	*minlabel = new char[MAX_INT_DIGITS+1];
	char	*maxlabel = new char[MAX_INT_DIGITS+1];

	sprintf(minlabel, "%d", min);
	sprintf(maxlabel, "%d", max);

	widget = XtVaCreateManagedWidget((char *)label,
		sliderWidgetClass,
		parent->get_widget(),
		XtNsliderMin, min,
		XtNsliderMax, max,
		XtNsliderValue, initial,
		XtNgranularity, granularity,
		XtNorientation, (orient == OR_vertical) ? OL_VERTICAL : OL_HORIZONTAL,
		XtNendBoxes, TRUE,
		XtNminLabel, minlabel,
		XtNmaxLabel, maxlabel,
		0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

int
Slider::get_value()
{
	int	val = 0;

	XtVaGetValues(widget, XtNsliderValue, &val, 0);
	return val;
}

void
Slider::set_value(int val)
{
	XtVaSetValues(widget, XtNsliderValue, val, 0);
}
