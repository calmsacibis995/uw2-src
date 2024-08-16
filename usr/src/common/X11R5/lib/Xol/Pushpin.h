/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)pushpin:Pushpin.h	1.8"
#endif

#ifndef _Pushpin_h
#define _Pushpin_h

/*
 *************************************************************************
 *
 * Description:
 *		"Public" include file for the Pushpin Widget.
 *
 ******************************file*header********************************
 */

#include <Xol/Primitive.h>		/* include superclasses' header */

extern WidgetClass			pushpinWidgetClass;
typedef struct _PushpinClassRec *	PushpinWidgetClass;
typedef struct _PushpinRec *		PushpinWidget;

#endif /* _Pushpin_h */
