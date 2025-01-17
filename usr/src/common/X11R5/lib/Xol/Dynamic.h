/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olmisc:Dynamic.h	1.17"
#endif

#ifndef __Dynamic_h__
#define __Dynamic_h__

typedef int		OlInputEvent;

typedef struct {
	Boolean		consumed;
	XEvent *	event;
	KeySym *	keysym;
	char *		buffer;
	int *		length;
	OlInputEvent	ol_event;
} OlInputCallData, *OlInputCallDataPointer;

/*
 * function prototype section
 */

OLBeginFunctionPrototypeBlock

extern OlInputEvent	
LookupOlInputEvent OL_ARGS((
	Widget			w,
	XEvent *		event,
	KeySym *		keysym,
	char **			buffer,
	int *			length
));
extern void
OlReplayBtnEvent OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XEvent *		event
));

OLEndFunctionPrototypeBlock

#endif /* __Dynamic_h__ */
