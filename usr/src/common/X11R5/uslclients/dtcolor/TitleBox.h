/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtcolor:TitleBox.h	1.1"
/**---------------------------------------------------------------------
***	
***	file:		TitleBox.h
***
***	project:	MotifPlus Widgets
***
***	description:	Public include file for DtTitleBox class.
***	
***	
***			(c) Copyright 1990 by Hewlett-Packard Company.
***
***
***-------------------------------------------------------------------*/


#ifndef _DtTitleBox_h
#define _DtTitleBox_h

#include <Xm/Xm.h>
#include <DtStrDefs.h>

#ifndef DtIsTitleBox
#define DtIsTitleBox(w) XtIsSubclass(w, DtTitleBoxClass)
#endif /* DtIsTitleBox */


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget DtCreateTitleBox() ;
extern Widget DtTitleBoxGetTitleArea() ;
extern Widget DtTitleBoxGetWorkArea() ;

#else

extern Widget DtCreateTitleBox( 
                        Widget parent,
                        char *name,
                        ArgList arglist,
                        Cardinal argcount) ;
extern Widget DtTitleBoxGetTitleArea( 
                        Widget w) ;
extern Widget DtTitleBoxGetWorkArea( 
                        Widget w) ;

#endif /* _NO_PROTO */

extern WidgetClass	dtTitleBoxWidgetClass;

typedef struct _DtTitleBoxClassRec * DtTitleBoxWidgetClass;
typedef struct _DtTitleBoxRec      * DtTitleBoxWidget;


#define XmTITLE_TOP	0
#define XmTITLE_BOTTOM	1

#define XmTITLE_AREA	1


#endif /* _DtTitleBox_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
