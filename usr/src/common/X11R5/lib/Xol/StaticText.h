/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)statictext:StaticText.h	1.6"
#endif

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        StaticText.h
 **
 **   Project:     X Widgets
 **
 **   Description: Public include file for StaticText class widgets
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **   
 *****************************************************************************
 *************************************<+>*************************************/

#ifndef _XtStaticText_h
#define _XtStaticText_h

#include <Xol/Primitive.h>		/* include superclasses' header */

#define	StaticTextSelect	0
#define	StaticTextAdjust	1
#define	StaticTextEnd		2

/***********************************************************************
 *
 * StaticText Widget
 *
 ***********************************************************************/

extern WidgetClass staticTextWidgetClass;

typedef struct _StaticTextClassRec	*StaticTextWidgetClass;
typedef struct _StaticTextRec     	*StaticTextWidget;

#endif
/* DON'T ADD STUFF AFTER THIS */
