/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StdSel.h	1.3"
/* $XConsortium: StdSel.h,v 1.3 91/07/22 23:46:07 converse Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided 
 * that the above copyright notice appear in all copies and that both that 
 * copyright notice and this permission notice appear in supporting 
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific, 
 * written prior permission. M.I.T. makes no representations about the 
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * The X Window System is a Trademark of MIT.
 *
 * The interfaces described by this header file are for miscellaneous utilities
 * and are not part of the Xlib standard.
 */

#ifndef _XMU_SELECTION_H_
#define _XMU_SELECTION_H_

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern Boolean XmuConvertStandardSelection(
#if NeedFunctionPrototypes
    Widget		/* w */,
    Time		/* time */,
    Atom*		/* selection */,
    Atom*		/* target */,
    Atom*		/* type_return */,
    caddr_t *		/* value_return */,
    unsigned long *	/* length_return */,
    int *		/* format_return */
#endif
);

_XFUNCPROTOEND

#endif /* _XMU_SELECTION_H_ */


