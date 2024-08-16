/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:DialogBoxP.h	1.1"
/**---------------------------------------------------------------------
***	
***	file:		DialogBoxP.h
***
***	project:	Motif Widgets
***
***	description:	Private include file for DtDialogBox class.
***	
***	
***			(c) Copyright 1990 by Hewlett-Packard Company.
***
***
***-------------------------------------------------------------------*/


#ifndef _DtDialogBoxP_h
#define _DtDialogBoxP_h

#include <Xm/XmP.h>
#include <Xm/BulletinBP.h>
#include <Xm/FormP.h>
#include <DialogBox.h>

typedef void (*DlgGetSizeProc)(
#ifndef _NO_PROTO
	Widget,
	Dimension,
	Dimension,
	Dimension,
	Dimension,
	Dimension *,
	Dimension *
#endif
);
/*-------------------------------------------------------------
**	Class Structure
*/

/*	Class Part
*/
typedef struct _DtDialogBoxClassPart
{
#ifdef _NO_PROTO
	XtWidgetProc		create_children;
	XtWidgetProc		configure_children;
	XtWidgetProc		get_size;
#else
	XtWidgetProc		create_children;
	XtWidgetProc		configure_children;
	DlgGetSizeProc		get_size;
#endif
	XtCallbackProc		button_callback;
	caddr_t			extension;
} DtDialogBoxClassPart;

/*	Full Class Record
*/
typedef struct _DtDialogBoxClassRec
{
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	XmManagerClassPart	manager_class;
	XmBulletinBoardClassPart	bulletin_board_class;
	XmFormClassPart		form_class;
	DtDialogBoxClassPart	dialog_box_class;
} DtDialogBoxClassRec;

/*	Actual Class
*/
extern DtDialogBoxClassRec dtDialogBoxClassRec;


/*-------------------------------------------------------------
**	Instance Structure
*/

/*	Instance Part
*/
typedef struct _DtDialogBoxPart
{
	XtCallbackList	callback;
	Widget		work_area;
	Widget		separator;
	WidgetList	button;
	Cardinal	button_count;	
	Boolean		minimize_buttons;
	XmStringTable	button_label_strings;
} DtDialogBoxPart;

/*	Full Instance Record
*/
typedef struct _DtDialogBoxRec
{
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	XmManagerPart		manager;
	XmBulletinBoardPart	bulletin_board;
	XmFormPart		form;
	DtDialogBoxPart		dialog_box;
} DtDialogBoxRec;


/*-------------------------------------------------------------
**	Constraint Structure
*/

/*	Constraint Part
*/
typedef struct _DtDialogBoxConstraintPart
{
	unsigned char		child_type;
} DtDialogBoxConstraintPart, * DtDialogBoxConstraint;

/*	Full Constraint Record
*/
typedef struct _DtDialogBoxConstraintRec
{
	XmManagerConstraintPart		manager_constraint;
	XmFormConstraintPart		form_constraint;
 	DtDialogBoxConstraintPart	dialog_box_constraint;
} DtDialogBoxConstraintRec, * DtDialogBoxConstraintPtr;



/*-------------------------------------------------------------
**	Class and Instance Macros
*/

/*	DtDialogBox Class Macros
*/	
#define C_CreateChildren(mc)	(mc -> dialog_box_class.create_children)
#define C_ConfigureChildren(mc)	(mc -> dialog_box_class.configure_children)
#define C_GetSize(mc)		(mc -> dialog_box_class.get_size)
#define C_ButtonCallback(mc)	(mc -> dialog_box_class.button_callback)

/*	DtDialogBox Instance Macros
*/
#define M_DialogBoxConstraint(w) \
 	(&((DtDialogBoxConstraintPtr) (w)->core.constraints) \
		-> dialog_box_constraint)
#define M_CreateChildren(m) \
	(((DtDialogBoxWidgetClass) m -> core.widget_class) \
		-> dialog_box_class.create_children) (m)
#define M_ConfigureChildren(m) \
	(((DtDialogBoxWidgetClass) m -> core.widget_class) \
		-> dialog_box_class.configure_children) (m)
#define M_GetSize(m,w1,h1,w2,h2,w,h) \
	(((DtDialogBoxWidgetClass) m -> core.widget_class) \
		-> dialog_box_class.get_size) (m,w1,h1,w2,h2,w,h)
#define M_ButtonCallback(w) \
	(w -> core.widget_class->dialog_box_class.button_callback)
#define M_WorkArea(m)		(m -> dialog_box.work_area)
#define M_Separator(m)		(m -> dialog_box.separator)
#define M_MinimizeButtons(m)	(m -> dialog_box.minimize_buttons)
#define M_Button(m)		(m -> dialog_box.button)
#define M_ButtonCount(m)	(m -> dialog_box.button_count)
#define M_ButtonLabelStrings(m)	(m -> dialog_box.button_label_strings)
#define M_Callback(m)		(m -> dialog_box.callback)

#endif /* _DtDialogBoxP_h */

/* DON'T ADD ANYTHING AFTER THIS #endif */
