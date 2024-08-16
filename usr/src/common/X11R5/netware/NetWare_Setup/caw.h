/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwsetup:caw.h	1.5"
// caw.h

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//////////////////////////////////////////////////////////////////

#ifndef CAW_H
#define CAW_H

#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ScrolledW.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <Dt/Desktop.h>
#include "nwsetup.h"
#include "nwsetup_txt.h"

#define XPM_FILE	"nwsetup.icon"

extern void create_all_widgets (int *argc, char **argv);
extern Widget make_option_menu_item (char *, int, Widget);
extern void toggleCB (Widget, int, XmAnyCallbackStruct *);
extern void menuCB (Widget, int, XmAnyCallbackStruct *);
extern void option_menuCB (Widget, int, XmAnyCallbackStruct *);
extern void textCB (Widget, int, XmAnyCallbackStruct *);
extern void buttonCB (Widget, int, XmAnyCallbackStruct *);
extern void dialogCB (Widget, int, XmAnyCallbackStruct *);
extern void help_textCB (Widget, int, XmAnyCallbackStruct *);
extern void dialog5_help_textCB (Widget, int, XmAnyCallbackStruct *);
extern void dialog6_help_textCB (Widget, int, XmAnyCallbackStruct *);
extern void dialog7_help_textCB (Widget, XtPointer, XmAnyCallbackStruct *);
extern void dialog8_help_textCB (Widget, int, XmAnyCallbackStruct *);
extern void plistCB (Widget, XtPointer, XtPointer);
extern int get_nic_count ();
extern int get_active_lan ();
extern void set_values (Boolean);
extern void collect_data ();
extern void collect_default_data ();
extern void create_plist ();
extern void create_dialog5 ();
extern void create_dialog6 ();
extern void create_dialog7 ();
extern void create_dialog8 ();
extern void create_can_dialogs ();
extern void set_dialog5_values ();
extern void set_dialog6_values ();
extern void set_dialog7_values ();
extern void set_dialog8_values ();
extern void add_item (int, XmString, int);
extern void replace_pix_at_pos (int, int);
extern void reset_chosen_nic ();
extern int valid (const uint32, const char *, const char *);
extern void set_pixmaps ();
extern void init_structs ();
extern void write_nwcm ();

extern XmStringCharSet char_set;

#endif
