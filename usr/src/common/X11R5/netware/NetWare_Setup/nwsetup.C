#ident	"@(#)nwsetup:nwsetup.C	1.13"
// nwsetup.c

//////////////////////////////////////////////////////////////////
//
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
//////////////////////////////////////////////////////////////////

#include "caw.h"

uid_t uid;
int ph, pw, lan_number, nic_count, nic_number, last_nic;
nic_t nic_menu[9];
nwsetup_t config, config_last;
Boolean changes_saved = False;
Boolean changes_made = False;
Boolean plist_created = False;
Boolean dialog5_created = False;
Boolean dialog6_created = False;
Boolean dialog7_created = False;
Boolean dialog8_created = False;
Boolean can_dialogs_created = False;
Boolean nuc_pkg_loaded = True;
Boolean netmgt_pkg_loaded = True;

Widget toplevel, form, scrolled_win, scs_item, show_item, hide_item, rb1_on, rb1_off, text2, rb2a_on, rb2a_off, text3, text4, toggle5, rb6_on, rb6_off, rb7_on, rb7_off, rb9_on, rb9_off, rb10_on, rb10_off, rb11_on, rb11_off, error_dialog, decision_dialog, still_exit_dialog, info_dialog, help_text;
Widget plist_dialog, no_button;
Widget dialog5, dialog5_label1b, dialog5_option_menu1, dialog5_text2, dialog5_text3, dialog5_option_menu2, e_II, e_8022, e_8023, e_SNAP, t_8022, t_SNAP, dialog5_help_text;
Widget dialog6, dialog6_rb_on, dialog6_rb_off, dialog6_text2, dialog6_text3, dialog6_help_text;
Widget dialog7, dialog7_text1, dialog7_help_text;
Widget dialog8, dialog8_rb1_on, dialog8_rb1_off, dialog8_rb2_on, dialog8_rb2_off, dialog8_rb3_on, dialog8_rb3_off, dialog8_text, dialog8_help_text;

char *pixmap_files[] = {"/usr/X/lib/pixmaps/noconnect.xpm", "/usr/X/lib/pixmaps/connect.xpm"};
char *plist_strings[] = {"Logical LAN 1", "Logical LAN 2", "Logical LAN 3", "Logical LAN 4", "Logical LAN 5", "Logical LAN 6", "Logical LAN 7", "Logical LAN 8"};
XmString table[8];
Display *dpy;
Window root;
PList *plist;
HelpText nwsetupHelp = {TXT_title, HELP_FILE, HELP_SECTION};
HelpText tocHelp = {TXT_title, HELP_FILE, NULL};
HelpText lan1Help = {TXT_title, HELP_FILE, LAN1_HELP_SECTION};
HelpText lan2Help = {TXT_title, HELP_FILE, LAN2_HELP_SECTION};
HelpText spxHelp = {TXT_title, HELP_FILE, SPX_HELP_SECTION};
HelpText sapHelp = {TXT_title, HELP_FILE, SAP_HELP_SECTION};
HelpText networkHelp = {TXT_title, HELP_FILE, NETWORK_HELP_SECTION};

void main (int argc, char *argv[])
{
//	setlocale (LC_ALL, "");
	uid = getuid ();
	init_structs ();
	create_all_widgets (&argc, argv);
}
