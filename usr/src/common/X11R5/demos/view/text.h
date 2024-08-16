/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mdemos:view/text.h	1.1"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: text.h,v $ $Revision: 1.2.2.2 $ $Date: 1992/04/28 15:25:55 $ */

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
#include <Xm/Frame.h>
#include <Xm/CascadeB.h>
#include <Xm/MessageB.h>
#include <Xm/PanedW.h>
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>

#define text_h
#include "fileview.h"
#include "textE.h"
#include "mainE.h"
#include "fileE.h"

/*
 * Localized strings, loaded at run time from a localized resource file
 */

static XmString no_search_msg = NULL;
static XmString not_found_msg = NULL;
static XmString no_pattern_msg = NULL;
static XmString search_msg = NULL;

/*
 * UIL literal names for localized strings.
 */

static String no_search = "no_search" ;
static String not_found = "not_found" ;
static String no_pattern = "no_pattern" ;
static String search_prompt = "searchprompt";

#ifdef _NO_PROTO
static void CancelSearch();
static void SearchSubstring();
static void NoInsert();
static void ChangeCurrentPane();
#else
static void CancelSearch(Widget button, ViewPtr this,
			    XmPushButtonCallbackStruct *call_data);

static void SearchSubstring(Widget button, ViewPtr this,
			    XmPushButtonCallbackStruct *call_data);

static void NoInsert(Widget text, ViewPtr this, XmTextVerifyPtr verify);

static void ChangeCurrentPane(Widget text, ViewPtr this, 
			      XmAnyCallbackStruct verify);

#endif

#undef text_h
