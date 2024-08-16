/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)abbrevstack:AbbrevMenu.h	1.2"
#endif

#ifndef _Ol_AbbrevMenu_h
#define _Ol_AbbrevMenu_h

/*
 *************************************************************************
 *
 * Description:
 *		This is the "public" include file for the
 *	AbbreviatedMenuButton Widget.
 *
 ******************************file*header********************************
 */

#include <Xol/Primitive.h>		/* include superclasses' header */
#include <X11/Shell.h>			/* need this for XtNtitle	*/

extern WidgetClass				abbrevMenuButtonWidgetClass;
typedef struct _AbbrevMenuButtonClassRec *	AbbrevMenuButtonWidgetClass;
typedef struct _AbbrevMenuButtonRec *		AbbrevMenuButtonWidget;

#endif /* _Ol_AbbrevMenu_h */
