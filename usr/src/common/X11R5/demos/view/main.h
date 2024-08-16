/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:view/main.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: main.h,v $ $Revision: 1.3.2.3 $ $Date: 1992/04/30 19:06:29 $ */

#include <stdio.h>
#include <stdlib.h>

#include <Xm/Xm.h>
#include <Mrm/MrmPublic.h>
#include <Xm/MainW.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>
#include <Xm/Form.h>
#include <Xm/CascadeB.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>
#include <Xm/Text.h>
/*
#include <Xm/TextF.h>
*/

#define main_h

#define DECLAREGLOBAL
#include "fileview.h"
#undef DECLAREGLOBAL

#include "mainE.h"
#include "textE.h"

/*
 * Local variables
 */
#define UIL_FILE_COUNT 1

static char *uid_files[UIL_FILE_COUNT] = {"fileview.uid"};

static MrmHierarchy theUIDdatabase;  /* MRM database hierarchy id */
/*
static MRMRegisterArg regvec[] = {
        {"exit_proc", (caddr_t) ExitCallback},
};

static MrmCount regnum = sizeof(regvec) / sizeof(MRMRegisterArg);
*/

#ifdef _NO_PROTO
static String MyLanguageProc();
static ViewPtr NewFileShell();
static Widget CreateMenuBarEntry();
static void ExitCallback();
static void CloseCallback();
static void HelpCallback();
static void OpenNewShellCallback();
static void OpenFileCallback();
static Widget CreateFileSelectionBox();
static void FileCancelCallback();
#else
static String MyLanguageProc(Display * dpy, String xnl,
			     XtAppContext theContext);

static ViewPtr NewFileShell(Widget parent, Bool primary,
			   int argc, char *argv[]);

static Widget CreateMenuBarEntry(Widget menubar, String entry, String names[],
		XtCallbackProc procs[], XtPointer private[], int count);

static void ExitCallback(Widget button, Widget root,
			 XmPushButtonCallbackStruct *call_data);

static void CloseCallback(Widget button, ViewPtr this,
			 XmPushButtonCallbackStruct *call_data);

static void HelpCallback(Widget	widget, ViewPtr this,
			 XmPushButtonCallbackStruct *call_data);

static void OpenNewShellCallback(Widget widget, ViewPtr this,
			 XmPushButtonCallbackStruct *call_data);

static void OpenFileCallback(Widget widget, ViewPtr this,
			     XmPushButtonCallbackStruct *call_data);

static Widget CreateFileSelectionBox(ViewPtr this);

static void FileCancelCallback(Widget fsb, ViewPtr this,
			XmFileSelectionBoxCallbackStruct *call_data);

#endif

#undef main_h
