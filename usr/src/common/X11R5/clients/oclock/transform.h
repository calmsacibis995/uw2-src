/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5oclock:transform.h	1.1"
/*
 * header file for transformed coordinate system.  No rotations
 * supported, as elipses cannot be rotated in X.
 */

typedef struct _transform {
	double	mx, bx;
	double	my, by;
} Transform;

typedef struct _TPoint {
	double	x, y;
} TPoint;

typedef struct _TRectangle {
	double	x, y, width, height;
} TRectangle;

# define Xx(x,y,t)	((int)((t)->mx * (x) + (t)->bx + 0.5))
# define Xy(x,y,t)	((int)((t)->my * (y) + (t)->by + 0.5))
# define Xwidth(w,h,t)	((int)((t)->mx * (w) + 0.5))
# define Xheight(w,h,t)	((int)((t)->my * (h) + 0.5))
# define Tx(x,y,t)	((((double) (x)) - (t)->bx) / (t)->mx)
# define Ty(x,y,t)	((((double) (y)) - (t)->by) / (t)->my)
# define Twidth(w,h,t)	(((double) (w)) / (t)->mx)
# define Theight(w,h,t)	(((double) (h)) / (t)->my)
