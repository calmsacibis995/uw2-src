/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)eventobj:EventObj.h	1.4"
#endif

#ifndef _OlEventObj_h
#define _OlEventObj_h

/*
 ************************************************************
 *
 *  Date:	July 19, 1989
 *
 *  Description:
 *	This file contains the source for the OPEN LOOK(tm)
 *	EventObj (Meta) Class.
 *
 ************************************************************
 */

#include <X11/Object.h>	/* RectObj.h doesn't include this but should */
#include <X11/RectObj.h>

#define _OlIsGadget(w)    XtIsSubclass(w, (WidgetClass) eventObjClass)

/*
 *  EventObjClass is defined in EventObj.c
 */
extern WidgetClass eventObjClass;

typedef struct _EventObjClassRec   *EventObjClass;
typedef struct _EventObjRec        *EventObj;

#endif /*  _OlEventObj_h  */
