/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)category:Category.h	2.2"
#endif

#if	!defined(_CATEGORY_H)
#define _CATEGORY_H

#include "Xol/Manager.h"

/*
 * Define this if you want to support XtNavailableWhenUnmanaged.
 */
#define AVAILABLE_WHEN_UNMANAGED

extern char		XtNshowFooter             [];
extern char		XtCShowFooter             [];
#if	defined(AVAILABLE_WHEN_UNMANAGED)
extern char		XtNavailableWhenUnmanaged [];
extern char		XtCAvailableWhenUnmanaged [];
#endif

extern WidgetClass			categoryWidgetClass;

typedef struct _CategoryClassRec *	CategoryWidgetClass;
typedef struct _CategoryRec *		CategoryWidget;
typedef struct _CategoryConstraintRec *	CategoryConstraints;

/*
 * Public types:
 */

typedef struct _OlCategoryNewPage {
	Widget			new_page;
	Widget			old_page;
	Boolean			apply_all;
}			OlCategoryNewPage;

OLBeginFunctionPrototypeBlock

/*
 * Used to change the property sheet. Returns True if the
 * Apply All button should be displayed.
 */
extern Boolean
OlCategorySetPage OL_ARGS((
	Widget			w,
	Widget			child
));

OLEndFunctionPrototypeBlock

#endif /* _CATEGORY_H */
