#ident	"@(#)systuner:systuner.C	1.5"
// systuner.c

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
///////////////////////////////////////////////////////////////////

#include "caw.h"

uid_t uid;
_category *c_list = NULL;
_category *last_list = NULL;
FILE *param_file, *desc_file;
XtAppContext context;
Widget toplevel, form, rowcolumn, text, scale, q_dialog1, q_dialog2, q_dialog3, m_dialog1, m_dialog2, m_dialog3, e_dialog1;
Widget no_button, no_button2;
Widget last_item = NULL;
Widget last_parameter = NULL;
_parameter *current_parameter = NULL;
Boolean mtune_modified = False, dialogs_created = False;
Display *dpy;
HelpText systunerHelp = {TXT_title, HELP_FILE, HELP_SECTION};

void main (int argc, char *argv[])
{
	setlocale (LC_ALL, "");
	uid = getuid ();
	get_categories ();
	create_all_widgets (&argc, argv);
}
