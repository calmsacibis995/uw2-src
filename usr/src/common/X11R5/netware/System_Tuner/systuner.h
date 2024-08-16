/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systuner:systuner.h	1.4"
// systuner.h

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
///////////////////////////////////////////////////////////////////

#ifndef SYSTUNER_H
#define SYSTUNER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <Xm/Xm.h>
#include <locale.h>
#include "dtFuncs.h"

#define HELP_FILE	"System_Tuner/systuner.hlp"
#define HELP_SECTION	"10"

#define PARAM_FILE	"/etc/conf/cf.d/mtune_p"
#define DESC_FILE	"/etc/conf/cf.d/mtune_d"
#define DESC_SIZE	2000

#define DTMSG		"/usr/X/desktop/rft/dtmsg"

typedef struct parameter {
	char *name;
	Boolean modified, autoconfig, hex, display;
	int min, max, def_value, last, current;
	Widget form, label, frame, text, toggle;
	struct parameter *next;
} _parameter;

typedef struct category {
	char *name;
	Widget menu_item;
	_parameter *head;
	struct category *next;
} _category;

extern uid_t uid;
extern _category *c_list;
extern _category *last_list;
extern FILE *param_file, *desc_file;
extern XtAppContext context;
extern Widget toplevel, form, rowcolumn, text, scale, q_dialog1, q_dialog2, q_dialog3, m_dialog1, m_dialog2, m_dialog3, e_dialog1;
extern Widget no_button, no_button2;
extern Widget last_item;
extern Widget last_parameter;
extern _parameter *current_parameter;
extern Boolean mtune_modified, dialogs_created;
extern Display *dpy;
extern HelpText systunerHelp;

#endif
