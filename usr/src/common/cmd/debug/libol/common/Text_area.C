#ident	"@(#)debugger:libol/common/Text_area.C	1.7"

#include "UI.h"
#include "Component.h"
#include "Text_area.h"
#include <string.h>
#include <stdlib.h>
#include <Xol/TextEdit.h>
#include <Xol/textbuff.h>
#include <Xol/ScrolledWi.h>

static void
get_textCB(Widget, Text_area *text, OlTextMotionCallData *ptr)
{
	if (!ptr || !text || !text->get_creator())
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	text->set_selection(ptr->select_start, ptr->select_end);
}

// This constructor is called from the framework code directly to
// create a Text_area object.  A Text_area is implemented with a
// TextEdit widget inside a scrolled window
Text_area::Text_area(Component *p, const char *name, int rows, int columns,
	Boolean editable, Callback_ptr scb, Command_sender *c, Help_id h, 
	int scroll) : COMPONENT(p, name, c, h)
{
	nrows = rows;
	npages = scroll / rows ;
	string = 0;
	select_cb = scb;
	selection_start = selection_size = 0;

	widget = XtVaCreateManagedWidget("text_widget", scrolledWindowWidgetClass,
		parent->get_widget(), XtNuserData, this, 0);

	// Create a TextEdit widget using an initial empty string source,
	// and retrieve the associated TextBuffer for later manipulation.
	text_area = XtVaCreateManagedWidget(label, textEditWidgetClass,
		widget,
		XtNsourceType, OL_STRING_SOURCE,
		XtNsource, "",
		XtNcharsVisible, columns,
		XtNlinesVisible, rows,
		XtNwrapMode, OL_WRAP_OFF,
		XtNuserData, this,
		XtNeditType, editable ? OL_TEXT_EDIT : OL_TEXT_READ,
		0);
	textbuf = OlTextEditTextBuffer(text_area);

	if (select_cb)
		XtAddCallback(text_area, XtNmotionVerification,
			(XtCallbackProc)get_textCB, (XtPointer)this);
	setup_tab_table();

	if (help_msg)
		register_help(widget, label, help_msg);
}

// This constructor is called by the Text_display constructor,
// which then creates the text_area widget
Text_area::Text_area(Component *p, const char *name, Command_sender *c,
	Callback_ptr scb, Help_id h) : COMPONENT(p, name, c, h)
{
	string = 0;
	select_cb = scb;
	selection_start = selection_size = 0;
	nrows = npages = 0;

	widget = XtVaCreateManagedWidget("text_widget", scrolledWindowWidgetClass,
		parent->get_widget(), XtNuserData, this, 0);

	if (help_msg)
		register_help(widget, label, help_msg);
}

Text_area::~Text_area()
{
	delete string;
	delete tab_table;
}

// setup_tab_table sets the widget's tab stops.  The defaults are based on
// the width of the widest character, rather than an average, making the
// built-in tab stops too wide to be useful.
void
Text_area::setup_tab_table()
{
	int		n_width;

	XtVaGetValues(text_area, XtNfont, &font, 0);
	n_width = XTextWidth(font, "n", 1);
	tab_table = new Position[21];
	for (int i = 0; i < 20; i++)
		tab_table[i] = (i + 1) * n_width * 8;
	tab_table[20] = 0;
	XtVaSetValues(text_area, XtNtabTable, tab_table, 0);
}

// append the string to the end of the displayed text
// note that we can't call OlTextEditInsert() here,
// at least not in the case of a TextEdit widget with an
// editType of OL_TEXT_READ.
void
Text_area::add_text(const char *s)
{
	if (!s || !*s)
		return;

	TextLocation	last_loc = LastTextBufferLocation(textbuf);
	ReplaceBlockInTextBuffer(textbuf, &last_loc, &last_loc, (char *)s, 0, 0);
	if (npages && (LinesInTextBuffer(textbuf) > (npages * nrows)))
	{
		TextLocation	tstart, tend;

		tstart = LocationOfPosition(textbuf, 
			PositionOfLine(textbuf, 0)),
		tend = LocationOfPosition(textbuf, 
			PositionOfLine(textbuf, nrows)),
		ReplaceBlockInTextBuffer(textbuf, &tstart, &tend,
			0, 0, 0);
	}
	TextPosition	last_pos = LastTextBufferPosition(textbuf);
	if (!XtIsRealized(text_area))
	{
		// we can't call OlTextEditSetCursorPosition here
		// since it seems to assume the existence of a window.
		// so, we explicitly adjust the display & cursor positions.
		// note that we could've used this code in both "realized"
		// and "unrealized" cases, however, OlTextEditSetCursorPosition
		// seem to cause less seemingly needless screen redraws.
		TextPosition	top = 0, newtop;
		int 		ntextlines;
		Arg		args[2];
		int		n = 0;

		ntextlines = LinesInTextBuffer(textbuf);
		XtVaGetValues(text_area, 
			XtNdisplayPosition, &top, 
			0);
		if (ntextlines > nrows)
		{
			newtop = PositionOfLine(textbuf, ntextlines-nrows+1);
			if (top < newtop)
			{
				XtSetArg(args[n], XtNdisplayPosition, newtop); 
				++n;
			}
		}
		XtSetArg(args[n], XtNcursorPosition, last_pos);
		++n;
		XtSetValues(text_area, args, n);
	}
	else
	{
		(void) OlTextEditSetCursorPosition(text_area, last_pos, 
				last_pos, last_pos);
	}
}

// Return entire contents.  OlTextEditCopyBuffer allocates the space for
// the string; Text_area is responsible for freeing the space later
char *
Text_area::get_text()
{
	delete string;
	string = 0;
	(void) OlTextEditCopyBuffer(text_area, &string);
	return string;
}

char *
Text_area::get_selection()
{
	Boolean ret;

	delete string;
	string = 0;

	if (!selection_size)
		return 0;

	// OlTextEditReadSubString allocates space for a copy of the
	// selected substring
	ret = OlTextEditReadSubString(text_area, &string, selection_start,
		selection_start + selection_size - 1);
	if (!ret || !string)
		return 0;
	return string;
}

// blank out text area
void
Text_area::clear()
{
	TextLocation	first_loc = LocationOfPosition(textbuf, PositionOfLine(textbuf, 0));
	TextLocation	last_loc = LastTextBufferLocation(textbuf);
	ReplaceBlockInTextBuffer(textbuf, &first_loc, &last_loc, "", 0, 0);
}

// Copy the selection to the clipboard
void
Text_area::copy_selection()
{
	(void) OlTextEditCopySelection(text_area, 0);
}

// Call the framework callback.  1 indicates that there is a selection,
// 0 indicates that the previous selection has been deselected.  That is
// indicated by start == end (that is, the start and the end are at the
// same position)
void
Text_area::set_selection(int start, int end)
{
	selection_start = start;
	selection_size = end - start;

	if (!select_cb)
		return;

	(creator->*select_cb)(this, selection_size ? (void *)1 : (void *)0);
}

// Lines are numbered 1-to-n to make implementing Source Window easy.
// They are numbered 0-to-n-1 in the underlying widget.
void
Text_area::position(int line)
{
	TextPosition start = PositionOfLine(textbuf, line-1);
	XtVaSetValues(text_area, XtNdisplayPosition, start, 0);
}

int
Text_area::get_current_position()
{
	int	start, end, pos;

	OlTextEditGetCursorPosition(text_area, &start, &end, &pos);
	return LineOfPosition(textbuf, start) + 1;
}
