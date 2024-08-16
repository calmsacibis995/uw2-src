/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systuner:caw.h	1.4"
// caw.h

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
///////////////////////////////////////////////////////////////////

#ifndef CAW_H
#define CAW_H

#include <Xm/Xm.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/MessageB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/Scale.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <Dt/Desktop.h>
#include "systuner.h"
#include "systuner_txt.h"
#include "mnem.h"

#define OK		1
#define RESET		2
#define RESET_FACTORY	3
#define CANCEL		4
#define HELP		5

#define XPM_FILE	"systuner48"

extern mnemInfoPtr	mPtr;      //pointer to the mnemonic record

extern void create_all_widgets (int *argc, char **argv);
extern void get_categories ();
extern void dialog_CB (Widget, int, XmAnyCallbackStruct *);
extern void text_focus_CB (Widget, _parameter *, XmAnyCallbackStruct *);
extern void text_losing_focus_CB (Widget, _parameter *, XmAnyCallbackStruct *);
extern void text_value_changed_CB (Widget, _parameter *, XmAnyCallbackStruct *);
extern void toggleCB (Widget, XtPointer, XmAnyCallbackStruct *);
extern void menuCB (Widget, char *, XmAnyCallbackStruct *);
extern void buttonCB (Widget, int, XmAnyCallbackStruct *);
extern void system_time (XtPointer, XtIntervalId);
extern void scaleCB (Widget, XtPointer, XmAnyCallbackStruct *);
extern void get_list (_category *);
extern void get_para_value (_parameter *);
extern char * get_description (char *);
extern void set_text (_category *);
extern void create_dialogs ();
extern void item_selected (_category *);

extern XmStringCharSet char_set;

#endif
