/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olmisc:OlCursors.h	1.5"
#endif

/*
 * OlCursors.h
 *
 */

#ifndef _OlCursors_h
#define _OlCursors_h

  /* MORE: SCOTTN: Need better solution to this than this!  Wait for */
  /* code, then apply. */
#define OlGetBusyCursor(x)	GetOlBusyCursor(XtScreenOfObject(x))
#define OlGetDuplicateCursor(x)	GetOlDuplicateCursor(XtScreenOfObject(x))
#define OlGetMoveCursor(x)	GetOlMoveCursor(XtScreenOfObject(x))
#define OlGetPanCursor(x)	GetOlPanCursor(XtScreenOfObject(x))
#define OlGetQuestionCursor(x)	GetOlQuestionCursor(XtScreenOfObject(x))
#define OlGetStandardCursor(x)	GetOlStandardCursor(XtScreenOfObject(x))
#define OlGetTargetCursor(x)	GetOlTargetCursor(XtScreenOfObject(x))

	/* OLBegin/OLEndFunctionPrototypeBlock are defined in OpenLook.h */
OLBeginFunctionPrototypeBlock

extern void   _GetBandWXColors OL_ARGS((
	Screen *	screen,
	XColor *	black,
	XColor *	white
));
extern Cursor _CreateCursorFromFiles OL_ARGS((
	Screen *	screen,
	char *		sourcefile,
	char *		maskfile
));
extern Cursor _CreateCursorFromData OL_ARGS((
	Screen *	screen,
	unsigned char *	sbits,
	unsigned char *	mbits,
	int		w,
	int		h,
	int 		xhot,
	int 		yhot
));
extern Cursor GetOlMoveCursor OL_ARGS((
	Screen *	screen
));
extern Cursor GetOlDuplicateCursor OL_ARGS((
	Screen *	screen
));
extern Cursor GetOlBusyCursor OL_ARGS((
	Screen *	screen
));
extern Cursor GetOlPanCursor OL_ARGS((
	Screen *	screen
));
extern Cursor GetOlQuestionCursor OL_ARGS((
	Screen *	screen
));
extern Cursor GetOlTargetCursor OL_ARGS((
	Screen *	screen
));
extern Cursor GetOlStandardCursor OL_ARGS((
	Screen *	screen
));
extern Pixmap OlGet50PercentGrey OL_ARGS((
	Screen *	screen
));
extern Pixmap OlGet75PercentGrey OL_ARGS((
	Screen *	screen
));

extern Cursor OlGetNoCursor OL_ARGS((Screen *));

OLEndFunctionPrototypeBlock

#endif
