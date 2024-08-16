/* kconf.c */

/*****************************************************************

 Copyright (c) 1993 Univel
 All Rights Reserved

 THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF UNIVEL

 The copyright notice above does not evidence any
 actual or intended publication of such source code.

 *****************************************************************/

#include "kconf.h"

extern void get_categories ();
extern void create_all_widgets (int *argc, char **argv);

uid_t uid;
_category *c_list = NULL;
_category *last_list = NULL;
FILE *param_file, *desc_file;
XtAppContext context;
XmFontList tfont_list;
Widget toplevel, form, rowcolumn, text, scale, q_dialog1, q_dialog2, q_dialog3, m_dialog1, m_dialog2, m_dialog3, e_dialog1;
Widget last_item = NULL;
Widget last_parameter = NULL;
_parameter *current_parameter = NULL;
Boolean mtune_modified = False;
HelpText kconfHelp = {"System Tuner", "10", HELP_FILE};
HelpText TocHelp = {"Table of Contents", NULL, HELP_FILE};

main (int argc, char *argv[])
{
	setlocale (LC_ALL, "");
	uid = getuid ();
	get_categories ();
	create_all_widgets (&argc, argv);
}
