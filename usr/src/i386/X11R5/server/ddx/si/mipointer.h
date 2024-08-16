/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/si/mipointer.h	1.3"

/*
 *	Copyright (c) 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */


/*
 * mipointer.h
 *
 */

/* $XConsortium: mipointer.h,v 5.4 91/07/19 23:20:09 keith Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.  M.I.T. makes no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.
*/

typedef struct {
    Bool	(*RealizeCursor)();	/* pScreen, pCursor */
    Bool	(*UnrealizeCursor)();	/* pScreen, pCursor */
    void	(*SetCursor)();		/* pScreen, pCursor, x, y */
    void	(*MoveCursor)();	/* pScreen, x, y */
} miPointerSpriteFuncRec, *miPointerSpriteFuncPtr;

typedef struct {
    Bool	(*CursorOffScreen)();	/* ppScreen, px, py */
    void	(*CrossScreen)();	/* pScreen, entering */
    void	(*WarpCursor)();	/* pScreen, x, y */
    void	(*EnqueueEvent)();	/* xEvent */
    void	(*NewEventScreen)();	/* pScreen */
} miPointerScreenFuncRec, *miPointerScreenFuncPtr;

extern Bool miPointerInitialize ();
extern void miPointerWarpCursor ();
extern void miPointerUpdate ();
extern void miPointerDeltaCursor ();
extern void miPointerAbsoluteCursor();
extern void miPointerPosition ();
extern void miRegisterPointerDevice();
extern int  miPointerGetMotionEvents();
extern int  miPointerGetMotionBufferSize();
