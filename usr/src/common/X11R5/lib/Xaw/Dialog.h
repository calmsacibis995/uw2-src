/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Dialog.h	1.3"
/* $XConsortium: Dialog.h,v 1.26 91/07/22 19:05:10 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef _XawDialog_h
#define _XawDialog_h

#include <X11/Xaw/Form.h>
#include <X11/Xfuncproto.h>

/***********************************************************************
 *
 * Dialog Widget
 *
 ***********************************************************************/

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 background	     Background		Pixel		XtDefaultBackground
 border		     BorderColor	Pixel		XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	1
 destroyCallback     Callback		Pointer		NULL
 height		     Height		Dimension	computed at create
 icon		     Icon		Pixmap		0
 label		     Label		String		NULL
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 sensitive	     Sensitive		Boolean		True
 value		     Value		String		NULL
 width		     Width		Dimension	computed at create
 x		     Position		Position	0
 y		     Position		Position	0

*/

#define XtCIcon "Icon"
#define XtNicon "icon"

typedef struct _DialogClassRec	*DialogWidgetClass;
typedef struct _DialogRec	*DialogWidget;

extern WidgetClass dialogWidgetClass;

_XFUNCPROTOBEGIN

extern void XawDialogAddButton(
#if NeedFunctionPrototypes
    Widget		/* dialog */,
    _Xconst char*	/* name */,
    XtCallbackProc	/* function */,
    XtPointer		/* client_data */
#endif
);

extern char *XawDialogGetValueString(
#if NeedFunctionPrototypes
    Widget		/* w */
#endif
);

_XFUNCPROTOEND

#endif /* _XawDialog_h */
/* DON'T ADD STUFF AFTER THIS #endif */
