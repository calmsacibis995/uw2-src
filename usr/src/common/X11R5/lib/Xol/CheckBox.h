/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)checkbox:CheckBox.h	1.10"
#endif

#ifndef _OlCheckBox_h
#define _OlCheckBox_h

/*
 ************************************************************
 *
 * File: CheckBox.h - Public definitions for CheckBox widget
 * 
 ************************************************************
 */

#include <Xol/Manager.h>	/* include superclasses' header */
#include <Xol/Button.h>

extern WidgetClass     checkBoxWidgetClass;

typedef struct _CheckBoxClassRec   *CheckBoxWidgetClass;
typedef struct _CheckBoxRec        *CheckBoxWidget;

#endif /*  _OlCheckBox_h  */

/* DON'T ADD STUFF AFTER THIS */
