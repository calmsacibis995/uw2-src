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
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlResetIc.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>

/*
 * OlResetIc()   : Reset the OL Input Context.
 *             In: Pointer to the IC to be reset.
 *            Out: Nothing.
 *         Return: Any string processed at reset time.
 *         Errors: None.
 *      Algorithm: Call XIM's multi-byte reset.
 */
char *
OlResetIc(ol_ic_p)
    OlIc	*ol_ic_p;
{
    char *ret;

    if ( ol_ic_p->ictype != NULL)
       ret = XmbResetIC((XIC)ol_ic_p->ictype);

    if (ret != NULL)
       XtFree (ret);
    
    return ((char *)NULL);
}
