/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)abbrevstack:AbbrevStac.h	1.8"
#endif

#ifndef _Ol_AbbrevStac_h
#define _Ol_AbbrevStac_h

/*
 *************************************************************************
 *
 * Description:
 *		This is the "public" include file for the Abbreviated
 *	Button Stack Widget.
 *
 ******************************file*header********************************
 */

#include <Xol/AbbrevMenu.h>		/* include superclasses' header */

extern WidgetClass				abbrevStackWidgetClass;
typedef struct _AbbrevMenuButtonClassRec *	AbbrevStackWidgetClass;
typedef struct _AbbrevMenuButtonRec *		AbbrevStackWidget;

#endif /* _Ol_AbbrevStac_h */
