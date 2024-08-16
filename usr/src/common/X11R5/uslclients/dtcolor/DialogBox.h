/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:DialogBox.h	1.1"
/**---------------------------------------------------------------------
***	
***	file:		DialogBox.h
***
***	project:	MotifPlus Widgets
***
***	description:	Public include file for DtDialogBox class.
***	
***	
***			(c) Copyright 1990 by Hewlett-Packard Company.
***
***
***-------------------------------------------------------------------*/


#ifndef _DtDialogBox_h
#define _DtDialogBox_h

#include <Xm/Xm.h>
#include <DtStrDefs.h>

#ifndef DtIsDialogBox
#define DtIsDialogBox(w) XtIsSubclass(w, dtDialogBoxWidgetClass)
#endif /* XmIsDialogBox */

#ifdef _NO_PROTO

extern Widget DtCreateDialogBox() ;
extern Widget DtCreateDialogBoxDialog() ;
extern Widget DtDialogBoxGetButton() ;
extern Widget DtDialogBoxGetWorkArea() ;

#else

extern Widget DtCreateDialogBox( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;
extern Widget DtCreateDialogBoxDialog( 
                        Widget ds_p,
                        String name,
                        ArgList db_args,
                        Cardinal db_n) ;
extern Widget DtDialogBoxGetButton( 
                        Widget w,
                        Cardinal pos) ;
extern Widget DtDialogBoxGetWorkArea( 
                        Widget w) ;

#endif /* _NO_PROTO */

extern WidgetClass dtDialogBoxWidgetClass;

typedef struct _DtDialogBoxClassRec * DtDialogBoxWidgetClass;
typedef struct _DtDialogBoxRec      * DtDialogBoxWidget;


#define XmBUTTON	11


#define XmCR_DIALOG_BUTTON	100

typedef struct
{
	int		reason;
	XEvent *	event;
	Cardinal	button_position;
	Widget		button;
} DtDialogBoxCallbackStruct;


#endif /* _DtDialogBox_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
