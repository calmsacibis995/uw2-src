#ident	"@(#)debugger:libol/common/Text_disp.C	1.27"

// GUI headers
#include "UI.h"
#include "Component.h"
#include "Text_area.h"
#include "Text_disp.h"
#include "DbgTextEdit.h"
#include "Base_win.h"
#include "Window_sh.h"

// Debug headers
#include "str.h"
#include "Machine.h"

#ifdef OLD_REGEXP
#include <regexpr.h>
#else
#include <regex.h>
#endif

#include <values.h>
#include <stdio.h>

// arrow_bits and stop_bits are compact, machine-generated descriptions
// of the bits that must be turned on to create the current position
// arrow and the stop sign picture.  x_height and x_width give the
// dimensions, in pixels, of the bitmap

// arrow definition
#define arrow_width 18
#define arrow_height 9
static unsigned char arrow_bits[] = {
   0x00, 0x10, 0x00, 0x00, 0x30, 0x00, 0x00, 0x70, 0x00, 0xfe, 0xff, 0x00,
   0xfe, 0xff, 0x01, 0xfe, 0xff, 0x00, 0x00, 0x70, 0x00, 0x00, 0x30, 0x00,
   0x00, 0x10, 0x00};

// stop sign definition
#define stop_width 11
#define stop_height 11
static unsigned char stop_bits[] = {
   0xf8, 0x00, 0xfc, 0x01, 0x06, 0x03, 0xf7, 0x07, 0xf7, 0x07, 0x07, 0x07,
   0x7f, 0x07, 0x7f, 0x07, 0x06, 0x03, 0xfc, 0x01, 0xf8, 0x00};

static Pixmap stop_sign;
static Pixmap arrow;
static Text_display *searched_pane;

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

// search_forward and search_backward are registered as the scanning functions
// in finish_setup(), and called by ForwardScanTextBuffer or BackwardScanTextBuffer
// from search().  They are called once for each line in the file, working
// forwards or backwards, respectively, until the function returns non-zero.
// These are being used instead of the built-in search functions because the
// built-ins don't handle regular expressions
//
// step, loc1, and loc2 are declared in regexp.h,
// exp is the regular expression being searched for.

// start past the current selection and search forward
// curp is a pointer to the first character after the current selection
#ifdef OLD_REGEXP
static CONST char *
search_forward(CONST char *, CONST char *curp, CONST char *exp)
{
	if (step(curp, exp))
	{
		searched_pane->set_selection_size(loc2 - loc1);
		return loc1;
	}
	return 0;
}
#else
static CONST char *
search_forward(CONST char *, CONST char *curp, CONST char *exp)
{
	regex_t		*re = (regex_t *)exp;
	regmatch_t	rm[1];
	if (regexec(re, curp, 1, rm, 0) == 0)
	{
		searched_pane->set_selection_size(rm[0].rm_eo - 
			rm[0].rm_so);
		return curp + rm[0].rm_so;
	}
	return 0;
}
#endif

// string is the entire line.  Unlike, search_forward, here curp is the first
// character of the current selection.  search_backward scans forward in the
// string until it finds the last match before the current selection.
#ifdef OLD_REGEXP
static CONST char *
search_backward(CONST char *string, CONST char *curp, CONST char *exp)
{
	char	*last_match = 0;
	while (step(string, exp))
	{
		if (!curp || strstr(loc1, curp))
		{
			last_match = loc1;
			searched_pane->set_selection_size(loc2 - loc1);
		}
		string = locs = loc2;
	}
	return last_match;
}
#else
static CONST char *
search_backward(CONST char *string, CONST char *curp, CONST char *exp)
{
	char		*last_match = 0;
	regex_t		*re = (regex_t *)exp;
	regmatch_t	rm[1];

	while(regexec(re, string, 1, rm, 0) == 0)
	{
		if (!curp || strstr(string + rm[0].rm_so, curp))
		{
			last_match = (char *)string + rm[0].rm_so;
			searched_pane->set_selection_size(rm[0].rm_eo -
				rm[0].rm_so);
		}
		string += rm[0].rm_eo;
	}
	return last_match;
}
#endif

static void
marginCB(Widget, Text_display *pane, OlTextMarginCallData *ptr)
{
	if (!pane || !ptr)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	pane->fix_margin(ptr->rect);
}

static void
propertyChangeCB(Text_display *pane)
{
	if (!pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	pane->redisplay(TRUE);
}

Text_display::~Text_display()
{
	if (gc_base)
		XFreeGC(XtDisplay(text_area), gc_base);
	if (gc_stop)
		XFreeGC(XtDisplay(text_area), gc_stop);
	OlUnregisterDynamicCallback( (OlDynamicCallbackProc)propertyChangeCB,
		(XtPointer)this);
}

// Both the Source and Disassembly windows call the constructor and finish_setup.
// In between they call either setup_source_file or setup_disassembly.

Text_display::Text_display(Component *p, const char *name,
	Base_window *b,
	Callback_ptr scb, Callback_ptr tbcb, Command_sender *c, Help_id h) : 
	TEXT_AREA(p, name, c, scb, h)
{
	bw = b;
	current_line = 0;
	empty = TRUE;
	search_expr = 0;
	compiled_expr = 0;
	selection_size = 0;
	breaks = 0;
	last_line = 0;
	gc_base = gc_stop = 0;
	selection_start = 0;
	toggle_break_cb = tbcb;
	user_selection = TRUE;	// any selection made while this is true is
				// a user selection
}

void
dblSelectCB(Widget, Text_display *text, OlTextDblSelectCallDataPointer cdata)
{
	XEvent *xev = cdata->xevent;

	if (!text->toggle_bkpt(xev->xbutton.x, xev->xbutton.y))
		return;
	cdata->consumed = TRUE;
}

int
Text_display::toggle_bkpt(int x, int y)
{
	// handle double clicks only inside margin
	if (x > left_margin)
		return 0;
	int	top, top_line;
	int	line;

	XtVaGetValues(text_area,
		XtNdisplayPosition, &top,
		0);

	// translate the coordinates into line numbers
	top_line = LineOfPosition(textbuf, top) + 1;
	if (!last_line)
		last_line = LineOfPosition(textbuf,
			LastTextBufferPosition(textbuf)) + 1;
	line = top_line + (y/font_height);
	if (line > last_line)
		return 0;
	(creator->*toggle_break_cb)(this, (void *)line);
	return 1;
}

void
Text_display::setup_source_file(const char *path, int rows, int columns)
{
	Arg		args[7];

	XtSetArg(args[0], XtNcharsVisible, columns);
	XtSetArg(args[1], XtNlinesVisible, rows);
	XtSetArg(args[2], XtNwrapMode, OL_WRAP_OFF);
	XtSetArg(args[3], XtNuserData, this);
	XtSetArg(args[4], XtNeditType, OL_TEXT_READ);

	if (path)
	{
		empty = TRUE;
		XtSetArg(args[5], XtNsourceType, OL_DISK_SOURCE);
		XtSetArg(args[6], XtNsource, path);
	}
	else
	{
		empty = FALSE;
		XtSetArg(args[5], XtNsourceType, OL_STRING_SOURCE);
		XtSetArg(args[6], XtNsource, "");
	}
	text_area = XtCreateManagedWidget(label, dbgTextEditWidgetClass,
		widget, args, 7);

	is_source = TRUE;
	finish_setup();
}

void
Text_display::setup_disassembly(int rows, int columns)
{
	text_area = XtVaCreateManagedWidget(label, dbgTextEditWidgetClass, widget,
		XtNsourceType, OL_STRING_SOURCE,
		XtNsource, "",
		XtNlinesVisible, rows,
		XtNcharsVisible, columns,
		XtNwrapMode, OL_WRAP_OFF,
		XtNuserData, this,
		XtNeditType, OL_TEXT_READ,
		0);

	is_source = FALSE;
	finish_setup();
}

void
Text_display::finish_setup()
{
	if (select_cb)
		XtAddCallback(text_area, XtNmotionVerification,
			(XtCallbackProc)get_textCB, (XtPointer)this);
	if (toggle_break_cb)
		XtAddCallback(text_area, XtNdblSelectProc,
			(XtCallbackProc)dblSelectCB, (XtPointer)this);

	// get the pointer to the widget's underlying TextBuffer
	textbuf = OlTextEditTextBuffer(text_area);
	setup_tab_table();

	// initialize the values needed to put line numbers, arrows, and stop signs
	// in the right places in the margins
	if (is_source)
	{
		last_line = LineOfPosition(textbuf,
			LastTextBufferPosition(textbuf)) + 1;
		x_stop = XTextWidth(font, "000000 ", sizeof("000000 "));
	}
	else
	{
		last_line = 0;
		x_stop = XTextWidth(font, " ", 1);
	}
	x_arrow = x_stop + stop_width + 2;
	left_margin = x_arrow + arrow_width + 2;
	XtVaSetValues(text_area, XtNleftMargin, left_margin, 0);
	font_height = OlFontHeight(font, (OlFontList *)0);
	XtVaGetValues(text_area, XtNtopMargin, &top_margin, 0);

	RegisterTextBufferScanFunctions(search_forward, search_backward);
	XtAddCallback(text_area, XtNmargin, (XtCallbackProc)marginCB, (XtPointer)this);
	OlRegisterDynamicCallback((OlDynamicCallbackProc)propertyChangeCB,
		(XtPointer)this);
}

// set_file is called from the Source window to change the file displayed
void
Text_display::set_file(const char *path)
{
	delete breaks;
	breaks = 0;
	empty = FALSE;
	current_line = 0;

	XtVaSetValues(text_area,
		XtNsourceType, OL_DISK_SOURCE,
		XtNsource, path,
		0);
	textbuf = OlTextEditTextBuffer(text_area);
	last_line = LineOfPosition(textbuf, LastTextBufferPosition(textbuf)) + 1;
	redisplay(FALSE);
}

// set_buf is called from the Disassembly window to display a disassembled function
void
Text_display::set_buf(const char *buf)
{
	delete breaks;
	breaks = 0;
	empty = FALSE;
	current_line = 0;

	XtVaSetValues(text_area,
		XtNsourceType, OL_STRING_SOURCE,
		XtNsource, buf,
		0);
	textbuf = OlTextEditTextBuffer(text_area);
	last_line = LineOfPosition(textbuf, LastTextBufferPosition(textbuf)) + 1;
}

// set_breaklist initializes the breaks array.  It allocates enough space for
// one bit per line, then turns on the bits corresponding to lines with
// breakpoints set on them
void
Text_display::set_breaklist(int *breaklist)
{
	if (!last_line)
		last_line = LineOfPosition(textbuf,
			LastTextBufferPosition(textbuf)) + 1;

	int break_size = (last_line + BITSPERBYTE)/BITSPERBYTE;
	breaks = new char[break_size];
	memset(breaks, 0, break_size);
	if (breaklist)
	{
		for (int i = 0; breaklist[i]; i++)
			set_stop(breaklist[i]);
	}
}

// Lines are numbered from 1 to n, so 0 indicates no selection (or previous
// selection has been deselected).  That is when start and end both point
// to the same position
void
Text_display::set_selection(int start, int end)
{
	int line;
	if (user_selection)
	{
		selection_start = start;
		selection_size = end - start;
		if (start < end)
			line = LineOfPosition(textbuf, start) + 1;
		else
			line = 0;
	}
	else
	{
		// this is a selection made by the debugger to
		// highlight the current line.  The selected text is not to
		// be copied or used for popup window initialization
		selection_start = selection_size = line = 0;
	}

	if (!select_cb)
		return;
	(creator->*select_cb)(this, (void *)line);
}

Search_return
Text_display::search(const char *s, int forwards)
{
	if (empty)
		return SR_notfound;

	TextLocation	loc;
	ScanResult	ret;
	int		pos;

	// if searching for the same string as the previous search,
	// use the previously compiled regular expression string
#ifdef OLD_REGEXP
	if (!search_expr || strcmp(s, search_expr) != 0)
	{
		delete (char *)search_expr;
		delete compiled_expr;
		search_expr = makestr(s);
		compiled_expr = compile(s, 0, 0);
		if (!compiled_expr)
			return SR_bad_expression;
	}
#else
	if (!search_expr || strcmp(s, search_expr) != 0)
	{
		int	flags;
#ifdef REG_ONESUB
	// Novell extension
		flags = REG_ONESUB;
#else
		flags = 0;
#endif
		delete (char *)search_expr;
		if (compiled_expr)
		{
			regfree((regex_t *)compiled_expr);
			delete((regex_t *)compiled_expr);
		}
		search_expr = makestr(s);
		compiled_expr = new regex_t;
		if (regcomp((regex_t *)compiled_expr, s, flags) != 0)
		{
			delete((regex_t *)compiled_expr);
			delete (char *)search_expr;
			search_expr = 0;
			compiled_expr = 0;
			return SR_bad_expression;
		}
	}
#endif

	// ForwardScanTextBuffer and BackwardScanTextBuffer call search_forward
	// and search_backward, respectively, to scan individual lines
	searched_pane = this;
	if (forwards)
	{
		// search forward from the end of the current selection
		loc = LocationOfPosition(textbuf,
			selection_start + selection_size - 1);
		ret = ForwardScanTextBuffer(textbuf, 
			(CONST char *)compiled_expr, &loc);
	}
	else
	{
		// search backwards from the beginning of the current selection
		loc = LocationOfPosition(textbuf, selection_start);
		ret = BackwardScanTextBuffer(textbuf, 
			(CONST char *)compiled_expr, &loc);
	}

	switch(ret)
	{
	case SCAN_WRAPPED:
	case SCAN_FOUND:
		pos = PositionOfLocation(textbuf, loc);
		OlTextEditSetCursorPosition(text_area, pos, pos + selection_size, pos);
		return SR_found;

	case SCAN_NOTFOUND:	return SR_notfound;
	case SCAN_INVALID:
	default:		return SR_bad_expression;
	}
}

Boolean
Text_display::has_breakpoint(int line)
{
	if (line < 0 || line > last_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;
	}

	if (!breaks)
		return 0;
	int	bit = 1 << (line % BITSPERBYTE);
	return breaks[line/BITSPERBYTE] & bit;
}

// framework lines are numbered from 1 to n
// however, set_line can be called with 0 to clear current_line
void
Text_display::set_line(int line)
{
	if (line == current_line)
		return;
	int	upper, lower;
	pane_bounds(lower, upper);
	if (current_line && current_line >= lower && current_line <= upper)
		clear_arrow(current_line, lower);
	current_line = line;
	if (line == 0)
		// done with clear
		return;

	// avoid multiple panes trying to set the highlight
	// at the same time;  the Window_set change_current notifier
	// sets the count for each Base_window to 1 before
	// calling notify;  we allow only 1 call per notification
	// event.  If the count is 1, we set it to 2.  If 2 or
	// above, we just return.  The Window_set change_current
	// notifier clears the count before returning.
	switch(bw->get_set_line_count())
	{
	case 0:	
		break;
	case 1:
		bw->set_set_line_count(2);
		break;
	default:
		return;
	}
	if (line >= lower && line <= upper)
		draw_arrow(line, lower);

	// don't try to set the highlight unless we are part of the
	// Window_shell that has focus;

	if (bw->get_window_shell() != focus_window)
		return;

	// lines are numbered from 0 to n-1 internal to TextBuffer
	// we highlight text from the first position of the current
	// line to the first position of the next line
	TextPosition start = PositionOfLine(textbuf, line-1);
	TextPosition end = PositionOfLine(textbuf, line);
	if (end == EOF)
		// past end of buffer
		end = LastTextBufferPosition(textbuf);

	// set user_selection to FALSE so that when the set_selection callback
	// is invoked, it will know to treat the selection differently from
	// user-selected text.
	user_selection = FALSE;
	OlTextEditSetCursorPosition(text_area, start, end, start);
	user_selection = TRUE;
}

void
Text_display::position(int line)
{
	int	lower;
	int	upper;

	if (empty)
		return;

	if (line < 0 || line > last_line)
	{
		display_msg(E_ERROR, GE_src_line, line, last_line);
		return;
	}

	TextPosition start = PositionOfLine(textbuf, line-1);

	pane_bounds(lower, upper);
	if (line < lower || line > upper)
		XtVaSetValues(text_area, XtNdisplayPosition, start, 0);
}

void
Text_display::draw_number(int line, int top_line)
{
	char	buf[MAX_INT_DIGITS+2];

	sprintf(buf, "%d ", line);
	int width = XTextWidth(font, buf, strlen(buf));
	int x = x_stop - width;
	int y = top_margin + font->ascent + font_height * (line - top_line);

	if (!gc_base)
		set_gc();
	XDrawString(XtDisplay(text_area), XtWindow(text_area), gc_base,
		x, y, buf, strlen(buf));
}

// clear_arrow assumes calling function has checked that the arrow is visible,
// lines_from_top is (old-current-line - lower-bound-in-pane)
void
Text_display::clear_arrow(int line, int lower_bound)
{
	int y = top_margin + font_height * (line - lower_bound)
		+ (font_height - arrow_height)/2;
	XClearArea(XtDisplay(text_area), XtWindow(text_area),
		x_arrow, y, arrow_width, arrow_height, FALSE);
}

void
Text_display::draw_arrow(int line, int lower_bound)
{
	if (!line)
		return;

	int	y = top_margin + font_height * (line - lower_bound)
		+ (font_height - arrow_height)/2;

	if (!gc_base)
		set_gc();
	if (!arrow)
	{
		if ((arrow = XCreateBitmapFromData(XtDisplay(text_area), XtWindow(text_area), 
			(char *)arrow_bits, arrow_width, arrow_height)) == 0)
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}

	XCopyPlane(XtDisplay(text_area), arrow, XtWindow(text_area), gc_base, 0, 0,
			arrow_width, arrow_height, x_arrow, y, 1);
}

void
Text_display::draw_stop(int line, int lower_bound)
{
	int y = top_margin + font_height * (line - lower_bound)
		+ (font_height - stop_height)/2;

	if (!gc_base)
		set_gc();
	if (!stop_sign)
	{
		if ((stop_sign = XCreateBitmapFromData(XtDisplay(text_area), XtWindow(text_area),
			(char *)stop_bits, stop_width, stop_height)) == 0)
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}

	XCopyPlane(XtDisplay(text_area), stop_sign, XtWindow(text_area), gc_stop,
			0, 0, stop_width, stop_height, x_stop, y, 1);
}

// clear_stop and set_stop have to do bounds checking, since they are
// called from the Source and Disassembly windows, instead of from other
// Text_display functions
void
Text_display::clear_stop(int line)
{
	int	lower, upper;
	int	bit = 1 << (line % BITSPERBYTE);

	breaks[line/BITSPERBYTE] &= ~bit;

	pane_bounds(lower, upper);
	if (line < lower || line > upper)
		return;

	int y = top_margin + font_height * (line - lower)
		+ (font_height - stop_height)/2;
	XClearArea(XtDisplay(text_area), XtWindow(text_area),
		x_stop, y, stop_width, stop_height, FALSE);
}

void
Text_display::set_stop(int line)
{
	int lower, upper;
	int bit = 1 << (line % BITSPERBYTE);

	breaks[line/BITSPERBYTE] |= bit;

	pane_bounds(lower, upper);
	if (line < lower || line > upper)
		return;

	draw_stop(line, lower);
}

void
Text_display::pane_bounds(int &lower, int &upper)
{
	int	top;
	int	lines_visible;

	if (empty)
	{
		lower = upper = 0;
		return;
	}

	XtVaGetValues(text_area,
		XtNdisplayPosition, &top,
		XtNlinesVisible, &lines_visible, 0);

	lower = LineOfPosition(textbuf, top) + 1;
	upper = lower + lines_visible - 1;
}

void
Text_display::set_gc()
{
	XGCValues	gc_values;
	unsigned long	gc_mask;
	unsigned long	foreground;
	unsigned long	background;
	Colormap	cmap;
	XColor		red;

	XtVaGetValues(text_area,
		XtNfontColor, &foreground, 
		XtNbackground, &background,
		0);

	gc_values.foreground = foreground;
	gc_values.background = background;
	gc_values.font = font->fid;
	gc_mask = GCFont | GCForeground | GCBackground;
	gc_base = XCreateGC(XtDisplay(text_area), XtWindow(text_area), gc_mask, &gc_values);

	cmap = DefaultColormapOfScreen(XtScreen(text_area));
	if (DefaultDepthOfScreen(XtScreen(text_area)) > 1 &&
	    XParseColor(XtDisplay(text_area), cmap, "Red", &red) &&
	    XAllocColor(XtDisplay(text_area), cmap, &red))
		gc_values.foreground = red.pixel;
	gc_stop = XCreateGC(XtDisplay(text_area), XtWindow(text_area), gc_mask, &gc_values);
}

// fix_margin draws line numbers, arrows, and stop signs whenever additional
// lines are displayed (the file is scrolled, the window resized, etc.)
void
Text_display::fix_margin(XRectangle *rect)
{
	if (empty)
	{
		XClearArea(XtDisplay(text_area), XtWindow(text_area), rect->x, rect->y,
			left_margin, rect->height, FALSE);
		return;
	}

	int	top, top_line;
	int	line;
	int	lines_visible;
	int	max_lines;
	int	stop;

	XtVaGetValues(text_area,
		XtNdisplayPosition, &top,
		XtNlinesVisible, &lines_visible, 0);

	// translate the coordinates and size of the exposed rectangle into
	// line numbers
	top_line = LineOfPosition(textbuf, top) + 1;
	max_lines = ((int)rect->height+font_height-1)/font_height;
	if (!last_line)
		last_line = LineOfPosition(textbuf,
			LastTextBufferPosition(textbuf)) + 1;

	// start drawing line numbers, etc. at the first newly exposed line
	// stop at the first already-exposed line (if scrolling up) or the
	// last visible line in the window
	line = top_line + (rect->y/font_height);
	stop = top_line + lines_visible;
	if (line + max_lines < stop)
		stop = line + max_lines;

	for (; line < stop; line++)
	{
		// if the text is too short to completely fill the window,
		// make sure any remaining space is blanked out
		if (line > last_line)
		{
			int y = top_margin + font->ascent
				+ font_height * (line - top_line);
			XClearArea(XtDisplay(text_area), XtWindow(text_area), 0, y,
				left_margin, rect->height - (y - rect->y), FALSE);
			break;
		}
		else
		{
			if (is_source)
				draw_number(line, top_line);
			if (line == current_line)
				draw_arrow(line, top_line);
			if (has_breakpoint(line))
				draw_stop(line, top_line);
		}
	}
}

// update the margin whenever 
// 1) a new file is displayed, or
// 2) some property, like background color, is changed
void
Text_display::redisplay(Boolean update_gcs)
{
	if (empty)
		return;

	if (update_gcs)
	{
		// reset font & font_height
		XtVaGetValues(text_area, XtNfont, &font, 0);
		font_height = OlFontHeight(font, (OlFontList *)0);
		// free old GCs
		if (gc_base)
		{
			XFreeGC(XtDisplay(text_area), gc_base);
			gc_base = 0;
		}
		if (gc_stop)
		{
			XFreeGC(XtDisplay(text_area), gc_stop);
			gc_stop = 0;
		}
	}

	// update line numbers, arrow, and stop signs
	int	top, top_line;
	int	line;
	int	lines_visible;
	int	stop;

	XtVaGetValues(text_area, XtNdisplayPosition, &top,
		XtNlinesVisible, &lines_visible, 0);

	top_line = LineOfPosition(textbuf, top) + 1;
	line = top_line;
	stop = top_line + lines_visible;
	if (last_line < stop)
		stop = last_line;

	for (; line < stop; line++)
	{
		if (is_source)
			draw_number(line, top_line);
		if (line == current_line)
			draw_arrow(line, top_line);
		if (has_breakpoint(line))
			draw_stop(line, top_line);
	}
}

// blank out text area
void
Text_display::clear()
{
	empty = TRUE;
	if (is_source)
	{
		XtVaSetValues(text_area,
			XtNsourceType, OL_STRING_SOURCE,
			XtNsource, "",
			XtNcursorPosition, 0,
			XtNselectStart, 0,
			XtNselectEnd, 0,
			XtNdisplayPosition, 0,
			0);
	}
	else
		(void) OlTextEditClearBuffer(text_area);
}

int
Text_display::get_last_line()
{
	return last_line;
}

int
Text_display::get_position()
{
	if (empty)
		return 0;

	int top;
	XtVaGetValues(text_area, XtNdisplayPosition, &top, 0);
	return LineOfPosition(textbuf, top) + 1;
}
