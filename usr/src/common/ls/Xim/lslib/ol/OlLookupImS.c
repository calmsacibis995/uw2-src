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
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/OlLookupImS.c	1.2"
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>

/*
 * OlLookupImString()   : Look up string of the OLIM.
 *                    In: Event to process.
 *                        Pointer to Input Context where event occurred.
 *                        Buffer for (multibyte) string returned.
 *                        Number of bytes in the buffer.
 *                        Pointer to Keysym of the event.
 *                        Pointer to Status of the lookup.
 *                   Out: Lookup string in the buffer.
 *                        Keysym of the event.
 *                        Status of the lookup.
 *                Return: Number of characters in the buffer.
 *                Errors: None.
 *             Algorithm: Call XmbLookupString() to do the work,
 *                        behave sensibly if that gives us no joy.
 */
int
OlLookupImString(event, ol_ic_p, buffer_return, bytes_buffer, 
                 keysym_return, status_return)
    XKeyEvent	*event;
    OlIc	*ol_ic_p;
    char	*buffer_return;
    int		 bytes_buffer;
    KeySym	*keysym_return;
    OlImStatus	*status_return;
{
    int	ret;
    OlIcValues icvalues[2];
    XPoint *point;

    if ( ol_ic_p->ictype != NULL )
    {
       ret = (XmbLookupString((XIC) ol_ic_p->ictype, 
                              event, buffer_return, bytes_buffer,
                              keysym_return, status_return));

       if ( ret > 0 )
       {
           /* update preedit attributes if working in that style */
           if( ol_ic_p->style & OlImPreEditPosition )
           {
               
/*
               not sure this is needed, so comment out for now
               OlSetIcValues(ol_ic_p, icvalues);                 
*/
           }

           return ret;
       }
    }
    else
        /* call "normal" lookup, if Xmb... returned 0 */
        return(XLookupString(event, buffer_return, bytes_buffer, 
                        keysym_return, 
                        (XComposeStatus *)status_return));
}
