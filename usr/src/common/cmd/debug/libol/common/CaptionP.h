/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_CAPTIONP_H
#define	_CAPTIONP_H
#ident	"@(#)debugger:libol/common/CaptionP.h	1.2"

// toolkit specific members of the Caption class
// included by ../../gui.d/common/Caption.h

// caption is a static text widget; the top level widget is a form
// The predefined caption widget is not being used because it doesn't
// allow for resizing the child widget

#define CAPTION_TOOLKIT_SPECIFICS	\
private:				\
	Widget		caption;	\
					\
	Caption_position position;

#endif	// _CAPTIONP_H
