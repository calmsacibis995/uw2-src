/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)changebar:ChangeBar.h	1.9"
#endif

#ifndef _CHANGEBAR_H
#define _CHANGEBAR_H

#include "X11/Intrinsic.h"
#include "Xol/OpenLook.h"

/*
 * Size/spacing of a change bar, in points
 */
#define CHANGE_BAR_WIDTH	3
#define CHANGE_BAR_HEIGHT	18
#define CHANGE_BAR_PAD		7

/*
 * Types:
 */

typedef struct ChangeBar {
	GC			normal_GC; /* GC for regular change bar	*/
	GC			dim_GC;	   /* GC for dim change bar	*/
	Dimension		width;	   /* width of change bar	*/
	Dimension		height;	   /* height of change bar	*/
	Dimension		pad;	   /* padding next to change bar*/
}			ChangeBar;

/*
 * Macros:
 */

#define OL_PROPAGATE_TO_CONTROL_AREA	0x0001
#define OL_PROPAGATE_TO_CATEGORY	0x0002
#define OL_PROPAGATE \
		(OL_PROPAGATE_TO_CONTROL_AREA|OL_PROPAGATE_TO_CATEGORY)

#define OlChangeBarWidth(CB)	((CB)->width)
#define OlChangeBarHeight(CB)	((CB)->height)
#define OlChangeBarSpan(CB)	((CB)->width + (CB)->pad)

/*
 * External functions:
 */

OLBeginFunctionPrototypeBlock

extern ChangeBar *
OlCreateChangeBar OL_ARGS((
	Widget			w,
	Pixel			color
));
extern void
OlDestroyChangeBar OL_ARGS((
	Widget			w,
	ChangeBar *		cb
));
extern void
OlSetChangeBarState OL_ARGS((
	Widget			w,
	OlDefine		state,
	unsigned int		propagate
));
extern void
OlFlatSetChangeBarState OL_ARGS((
	Widget			w,
	Cardinal		indx,
	OlDefine		state,
	unsigned int		propagate
));
extern void
OlDrawChangeBar OL_ARGS((
	Widget			w,
	ChangeBar *		cb,
	OlDefine		state,
	Boolean			expose,
	Position		x,
	Position		y,
	Region			region
));
extern void
OlChangeBarSetValues OL_ARGS((
	Widget			w,
	Pixel			color,
	ChangeBar *		cb
));

OLEndFunctionPrototypeBlock

#endif
