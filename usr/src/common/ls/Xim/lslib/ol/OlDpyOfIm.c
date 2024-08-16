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
 *
 */
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlDpyOfIm.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>


/*
 * OlDisplayofIm()   : Get display associated with OLIM.
 *                 In: Pointer to the OL Input Method.
 *                Out: Nothing.
 *             Return: Pointer to display of the OLIM on success, 
 *                     pointer to NULL if input data is garbage.
 *             Errors: see above.
 *          Algorithm: Call XIM interface.
 */

Display *
OlDisplayOfIm(ol_im_p)
    OlIm	*ol_im_p;
{
    if ( ol_im_p == NULL)
       return ((Display *)NULL);
    else
       return(XDisplayOfIM((XIM) ol_im_p->imtype));
}
