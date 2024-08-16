#ident	"@(#)debugger:libol/common/olutil.C	1.19"

// GUI headers
#include "Alert_sh.h"
#include "UI.h"
#include "Dispatcher.h"
#include "Message.h"
#include "Transport.h"
#include "Resources.h"
#include "gui_label.h"
#include "Label.h"

// Debug headers
#include "str.h"
#include "Vector.h"

#include <stdio.h>
#include <locale.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <Xol/OlCursors.h>
#include <Xol/Flat.h>
#include <Dt/Desktop.h>
#include <sys/types.h>
#include <sys/stat.h>

Widget                  base_widget;
extern Transport        transport;
char			*Help_path;
OlDefine		gui_mode;

static Vector		argv_new;
static void		parse_args(int *, char **&);
static char 		*parse_one(char *&);

static void
get_msg(XtPointer, int *, XtInputId)
{
	do
	{
		dispatcher.process_msg();
	}
	while(!transport.inqempty());
}

void
toolkit_main_loop()
{

	// using infinite loop here instead of XtMainLoop to avoid getting
	// backlog of messages in the queue
	// this can happen because Xt gives higher priority to X events
	// than other input events.
        for (;;)
	{
		XEvent  event;

		XtNextEvent(&event);
		XtDispatchEvent(&event);

		// clear queue
		while (!transport.inqempty())
			dispatcher.process_msg();

		// clean up alert shell space after shell is popped down
		if (old_alert_shell)
		{
			delete old_alert_shell;
			old_alert_shell = 0;
		}
	}
}

static Boolean
help_path_check(String dir)
{
	struct stat sbuf;

	// make sure it's a readable directory
	if (access(dir, R_OK|X_OK) == 0 && 
	    stat(dir, &sbuf) == 0 && 
	    (sbuf.st_mode & S_IFDIR))
		return True;
	return False;
}

Widget
init_gui(const char *name, const char *wclass, int *argc, char **argv)
{
	XrmOptionDescRec	*options;
	int			noptions;

	parse_args(argc, argv);
	options = resources.get_options(noptions);
	argv[0] = (char *)wclass;
	OlToolkitInitialize(argc, argv, NULL);
	base_widget = XtInitialize(name, wclass, options, noptions, 
		argc, argv);
	resources.initialize();
	DtInitialize(base_widget);
	gui_mode = OlGetGui();
	XtAddInput(fileno(stdin), (XtPointer) XtInputReadMask,
		(XtInputCallbackProc) get_msg, NULL);

	XtVaSetValues(base_widget, XtNhelpDirectory, "/help/debug", 0);
	Help_path = XtResolvePathname(XtDisplay(base_widget), 
		"help",		// %T 
		"debug",	// %N 
		"",		// %S
		NULL,		// path 
		NULL, 		// subst
		0, 		// nsubst
		(XtFilePredicate)help_path_check);
	if (!Help_path)
	{
		char *locale = setlocale(LC_MESSAGES, NULL);
		Help_path = makesf("/usr/X/lib/locale/%s/help/debug", locale);
	}

	return base_widget;
}

// the -X options from the command line are passed in verbatim,
// e.g. if -X "-xrm 'debug*background: blue'" was specified, we would get a
// single argument "-xrm 'debug*background: blue'" here. in order for 
// XtInitialize to understand it, we must break it up into separate args.
// in this particular case, the result should be 2 args: "-xrm" and
// "debug*background: blue".
static void
parse_args(int *argc, char **&argv)
{
	int argc_old, argc_new;
	char *input;
	char *arg1;

	argc_old = *argc;
	if(argc_old <= 1)
		return;
	// skip argv[0], it's never interesting
	argv_new.add(&argv[0], sizeof(char *));
	argc_new = 1;
	while(--argc_old > 0)
	{
		input = *++argv;
		while(arg1 = parse_one(input))
		{
			argv_new.add(&arg1, sizeof(char *));
			++argc_new;
		}
	}

	argv = (char **)argv_new.ptr();
	*argc = argc_new;
}

// scan & return the next token. input starts from 'input'.
// the character following the last character in the token gets
// overwritten with '\0'. 'input' is updated to point to the
// beginning of next token.
// the following (shell-like) quoting mechanisms are supported 
// currently:
// 1) \c => c, except inside '...' and sometimes inside "..."
// 2) inside "...", all whitespaces are preserved and \ 
//    quotes only " and \, that is \" => ", and \\ => \;
//    and ' loses its special meaning.
// 3) inside '...', everything is preserved verbatim, including
//    \ and ".
// note that the resulting string may be shorter than the original
// since all quoting characters are thrown out. to handle this,
// we make use of a trailing pointer 'ss' as we scan and overwrite
// the existing data space.
static char *
parse_one(char *&input)
{
	char	*tok;		// beginning of token
	char	*s;		// scanning pointer
	char	*ss;		// trailing pointer
	Boolean in_sq;		// True if inside '...'
	Boolean in_dq;		// True if inside "..."

	if(!input)
		return NULL;
	// assume that on entry, input points to 
	// the first char of a token
	s = ss = tok = input;
	// find end of token
	in_sq = in_dq = False;
	for(; *s && !(isspace(*s) && !in_sq && !in_dq); ++s)
	{
		switch(*s)
		{
		case '"':
			if (!in_sq)
			{
				in_dq = in_dq ? False : True;
				continue;
			}
			break;
		case '\'':
			if (!in_dq)
			{
				in_sq = in_sq ? False : True;
				continue;
			}
			break;
		case '\\':
			if (in_sq)
				break;
			if (in_dq)
				switch(s[1])
				{
				default:
					if (ss < s)
						*ss = '\\';
					break;
				case '"':
				case '\\':
					break;
				}
			++s;
			break;
		}
		if (ss < s)
			*ss = *s;
		++ss;
	}
	if (*s)
	{
		// turn first char past the end of token to '\0'
		*ss = '\0';
		// s is pointing at a whitespace following the token
		// by now, so skip to first nonwhite space (i.e. beginning
		// of next token)
		for(++s; isspace(*s); ++s)
			;
	}
	// else we've reached end of this arg, so make sure
	// the token's terminated properly
	else if (ss < s)
		*ss = '\0';
	// advance input to next token
	input = *s ? s : NULL;
	return tok;
			
}

void
display_msg(Message *m)
{
	Alert_shell *shell = new Alert_shell(m->format(), LAB_ok);
	shell->popup();
}

void
display_msg(Severity, Gui_msg_id id ...)
{
	va_list	ap;
	char	*msg;

	va_start(ap, id);
	msg = do_vsprintf(gm_format(id), ap);
	va_end(ap);

	Alert_shell *shell = new Alert_shell(msg, LAB_ok);
	shell->popup();
}

void
display_msg(Callback_ptr handler, void *object, LabelId action,
	LabelId no_action, Gui_msg_id id ...)
{
	va_list	ap;
	char	*msg;

	va_start(ap, id);
	msg = do_vsprintf(gm_format(id), ap);
	va_end(ap);

	Alert_shell *shell = new Alert_shell(msg, action, no_action, handler, object);
	shell->popup();
}

void
beep()
{
	XBell(XtDisplay(base_widget), -1);
}

void
busy_window(Widget w, Boolean busy)
{
	Display		*dpy = XtDisplay(w);
	Window		win = XtWindow(w);
	static Cursor	busy_cur;

	XtVaSetValues(w, XtNbusy, busy, 0);
	if (busy)
	{
		if(!busy_cur)
			busy_cur = OlGetBusyCursor(w);
		XDefineCursor(dpy, win, busy_cur);
		XSync(dpy, False);
	}
	else
	{
		XUndefineCursor(dpy, win);
	}
}

void
register_help(Widget widget, const char *title, Help_id help)
{
        if(!help)
	{
                return;
	}

        Help_info *help_entry = &Help_files[help];
        OlDtHelpInfo *help_data = (OlDtHelpInfo *)help_entry->data;

	// there are 2 cases here:
	// 1) this help entry is not registered with any widget, so
	//    create one
	// 2) this help entry is already registered with a widget.
	//    this is ok since we need to allow different widgets to 
	//    register with the same help entry (e.g. in case of
	//    multiple window sets). 
	//    note that we don't check if different help entries are
	//    registered with the same widget. we assume that the
	//    desktop will do something reasonable.
        if(!help_data)
        {
                help_entry->data = help_data = new OlDtHelpInfo;

                help_data->title = (String)title;
                help_data->path = (String)Help_path;
                help_data->filename = (String)help_entry->filename;
                help_data->section = (String)help_entry->section;
                help_data->app_title = NULL;
        }

        OlRegisterHelp(OL_WIDGET_HELP, widget, NULL,
                OL_DESKTOP_SOURCE, (XtPointer)help_data);
}

void
display_help(Widget w, Help_mode mode, Help_id help)
{
	DtRequest request;
	Display *dpy = XtDisplay(w);

	if(!help)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// extract help entry data
	Help_info *help_entry = &Help_files[help];
	if(!help_entry->data)
	{
		// not registered?
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (DtGetAppId(dpy, _HELP_QUEUE(dpy)) == None)
	{
		// dtm not running?
		display_msg(E_NONE, GM_no_help);
		return;
	}

	// issue request to DTM 
	memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	request.display_help.source_type = 
		mode == HM_section ? DT_SECTION_HELP : DT_TOC_HELP;
	request.display_help.app_name = (String)labeltab.get_label(LAB_debug);
	request.display_help.help_dir = (String)Help_path;
	request.display_help.file_name = (String)help_entry->filename;
	request.display_help.sect_tag = (String)help_entry->section;
	DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(dpy),
		_DT_QUEUE(dpy), XtWindow(w), &request);
}

// open Help Desk
void
helpdesk_help(Widget w)
{
	DtRequest request;
	Display	*dpy = XtDisplay(w);

	if (DtGetAppId(dpy, _HELP_QUEUE(dpy)) == None)
	{
		// dtm not running?
		display_msg(E_NONE, GM_no_help);
		return;
	}

	// issue request to DTM 
	memset(&request, 0, sizeof(request));
	request.display_help.rqtype = DT_DISPLAY_HELP;
	request.display_help.source_type =  DT_OPEN_HELPDESK;
	request.display_help.app_name = (String)labeltab.get_label(LAB_debug);
	DtEnqueueRequest(XtScreen(w), _HELP_QUEUE(dpy),
		_DT_QUEUE(dpy), XtWindow(w), &request);
}

// Calculate padding as a function of the shadow thicknesses of
// adjacent widgets. This also makes the padding independent of
// monitor resolution, and avoids the overlapping problem when
// adjacent widgets' thicknesses add up to > PADDING
int
get_widget_pad(Widget w1, Widget w2)
{
	Dimension shadow_thickness;
	int pad = 2;

	shadow_thickness = 0;
	XtVaGetValues(w1, XtNshadowThickness, &shadow_thickness, 0);
	pad += shadow_thickness;
	shadow_thickness = 0;
	XtVaGetValues(w2, XtNshadowThickness, &shadow_thickness, 0);
	pad += shadow_thickness;
	return pad;
}

// When a window is raised to the top, check to see if a notice needs
// to be raised back to the top.  Checking for FocusIn may result in
// recover_notice being called extra times, but it seems to be the
// only thing we can count on
void
eventCB(Widget, XtPointer, OlVirtualEventRec *e)
{
	if (e->xevent->type == FocusIn)
	{
		recover_notice();
	}
}

void
register_button_help(Widget widget, int index, const char *title, Help_id help_id)
{
	OlFlatHelpId	id;
	Help_info	*help_entry = &Help_files[help_id];
	OlDtHelpInfo	*help_data = (OlDtHelpInfo *)help_entry->data;

	id.widget = widget;
	id.item_index = index;

	if(!help_data)
	{
		help_entry->data = help_data = new OlDtHelpInfo;
		help_data->title = (String)title;
		help_data->path = (String)Help_path;
		help_data->filename = (String)help_entry->filename;
		help_data->section = (String)help_entry->section;
		help_data->app_title = NULL;
	}
	OlRegisterHelp(OL_FLAT_HELP, &id, NULL,
		OL_DESKTOP_SOURCE, (XtPointer)help_data);
}
