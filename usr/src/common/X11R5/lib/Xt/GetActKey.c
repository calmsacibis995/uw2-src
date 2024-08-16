/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:GetActKey.c	1.2"
/* $XConsortium: GetActKey.c,v 1.4 91/01/10 14:10:30 converse Exp $ */

/*LINTLIBRARY*/

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

#include "IntrinsicI.h"

KeySym XtGetActionKeysym(event, modifiers_return)
    XEvent *event;
    Modifiers *modifiers_return;
{
    TMKeyContext tm_context= _XtGetPerDisplay(event->xany.display)->tm_context;
    Modifiers modifiers;
    KeySym keysym;

    if (event->xany.type != KeyPress && event->xany.type != KeyRelease)
	return NoSymbol;

    if (tm_context != NULL
	&& event == tm_context->event
	&& event->xany.serial == tm_context->serial ) {

	if (modifiers_return != NULL)
	    *modifiers_return = tm_context->modifiers;
	return tm_context->keysym;
    }

    XtTranslateKeycode( event->xany.display, (KeyCode)event->xkey.keycode,
		        event->xkey.state, &modifiers, &keysym );

    if (modifiers_return != NULL)
	*modifiers_return = event->xkey.state & modifiers;

    return keysym;
}
