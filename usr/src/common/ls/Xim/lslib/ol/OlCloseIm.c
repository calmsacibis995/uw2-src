/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *                 European Language Supplement 4.2 
 *                    Open Look Input Method
 *
 * Copyright (c) 1992 UNIX System Laboratories, Inc.
 * All Rights Reserved 
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF 
 * UNIX System Laboratories, Inc.
 * The copyright notice above does not evidence any
 * actual or intended publication of such source code.
 *
 * 
 */
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlCloseIm.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>

/*
 * OlCloseIm()   : Close Input method.
 *             In: Pointer to the OL Input Method.
 *            Out: Nothing.
 *         Return: Nothing.
 *         Errors: None.
 *      Algorithm: Free pointers inside the OlIm structure,
 *                 then the structure itself.
 */
 

void
OlCloseIm(ol_im_p)
    OlIm	*ol_im_p;
{
	OlIc	*p, *q; /* tmp pointers when freeing the IC list */

	if ( ol_im_p == NULL) 
           return;

	p = q = ol_im_p->iclist;

       /* free any Input Contexts in the IC list of this IM */
	while( p != NULL )
        {
           q = p->nextic;
           if ( p->ictype ) 
              XtFree((char *)p->ictype);
           XtFree((char *)p);
           p = q;
	}

        /* free the underlying X11 IM */
	if ( ol_im_p->imtype ) 
           XtFree((char *)ol_im_p->imtype);

        /* free the application name and class */
	if (ol_im_p->appl_name) 
           XtFree((char *)ol_im_p->appl_name);

	if ( ol_im_p->appl_class ) 
           XtFree((char *)ol_im_p->appl_class);

        /* finally, free the OlIm structure itself */
	
	XtFree((char *)ol_im_p);
}
