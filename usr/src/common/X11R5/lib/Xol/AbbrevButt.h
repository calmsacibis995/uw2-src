/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)abbrevstack:AbbrevButt.h	1.1"
#endif

#ifndef _Ol_AbbrevButt_h
#define _Ol_AbbrevButt_h

/*
 *************************************************************************
 *
 * Description:
 *		This is the "public" include file for the
 *	AbbreviatedButton Widget.
 *
 ******************************file*header********************************
 */

#include <Xol/Primitive.h>		/* include superclasses' header */

extern WidgetClass				abbreviatedButtonWidgetClass;

typedef struct _AbbreviatedButtonClassRec *	AbbreviatedButtonWidgetClass;
typedef struct _AbbreviatedButtonRec *		AbbreviatedButtonWidget;

#endif /* _Ol_AbbrevButt_h */
