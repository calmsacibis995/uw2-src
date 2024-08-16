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
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlSetIcF.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "messages.h"

/* utilities used by this module */
extern char		*GetTxt();

/*
 * OlSetIcFocus()   : Set Input Context Focus.
 *                In: Pointer to the OL Input Context.
 *               Out: Nothing.
 *            Return: Nothing.
 *            Errors: None.
 *         Algorithm: Check input data is OK, then pass on to XIM.
 */

void
OlSetIcFocus(ol_ic_p)
    OlIc	*ol_ic_p;
{
    
    /* check client window is defined */
    if(ol_ic_p->cl_win == NULL && ol_ic_p->focus_win == NULL)
    {
         XtWarning(GetTxt(NoClientOrFocusWin));
         return;
    }


    /* call XIM */
    XSetICFocus((XIC) ol_ic_p->ictype);


}
