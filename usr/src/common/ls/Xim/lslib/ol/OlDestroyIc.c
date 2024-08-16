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
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlDestroyIc.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>


/* external OlIm functions used by this module */
extern OlIm	*OlImOfIc();

/*
 * OlDestroyIc()   :  Destroy Input Context of the OLIM.
 *                In: Pointer to the IC to be destroyed.
 *               Out: Nothing
 *            Return: Nothing
 *            Errors: None
 *         Algorithm: Free pointers inside the OlIc structure,
 *                    destroy the XIM IC, then free OlIc structure 
 *                    itself.
 */

void
OlDestroyIc(ol_ic_p)
    OlIc	*ol_ic_p;
{
    OlIm	*ol_im_p = OlImOfIc(ol_ic_p);
    OlIc	*p, *q;

    if ( ol_ic_p == NULL ) 
       return;

    p = q = ol_im_p->iclist;

    if ( p == ol_ic_p ) 
    {
       /* if IC to destroy is first in the list, relinking is easy */
       ol_im_p->iclist = ol_ic_p->nextic;
    } 
    else 
    {
       while(TRUE) 
       {
          if ( p == NULL) 
          {
             /* reached end of the list and IC not found */
             return;
          }
          /* have we found our IC? */
          if ( (q = p->nextic) == ol_ic_p) 
          {
             /* yes - relink */
             p->nextic = ol_ic_p->nextic;
             break;
          }
          /* no - go on traversing */
          p = q;
       }
    }

    /* IC found and list relinked - close the XIM IC */
    if (ol_ic_p->ictype != NULL) 
	XDestroyIC((XIC) ol_ic_p->ictype);
    
    if (ol_ic_p->ictype) 
       XtFree((char *) ol_ic_p->ictype);

    XtFree((char *) ol_ic_p);
}
