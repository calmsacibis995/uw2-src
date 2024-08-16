#ident	"@(#)xpr:devices/terminfo/text.h	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

/*
 * The following are lifted from the Blit software, with
 * minor changes.
 */

typedef struct _Fontchar {
	short			x;
	unsigned char		top,
				bottom;
	unsigned char		left;
	unsigned char		width;
}			_Fontchar;

typedef struct _Font {
	short			n;
	char			height,
				ascent;
	XImage			*image;
	_Fontchar		info[1];
}			_Font;

typedef struct _Fontref {
	_Font			*font_p;
	char			*name;
}			_Fontref;

extern _Fontref		fontref[];
