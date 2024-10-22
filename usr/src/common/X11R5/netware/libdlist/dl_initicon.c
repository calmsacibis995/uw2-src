/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libdlist:dl_initicon.c	1.2"
#ident	"@(#)dl_initicon.c	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/libdlist/dl_initicon.c,v 1.1 1994/02/01 22:52:15 renu Exp $"

/*--------------------------------------------------------------------
** Filename : initicon.c
**
** Description : This file contains a function that sets up an icon for
**               the application, and connects it to the top level
**               widget.
**
** Functions : InitIcon
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                       I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <X11/Shell.h>


/*--------------------------------------------------------------------
** Function : InitIcon
**
** Description : This function sets up the icon for the application and
**               makes the connection to the top level widget.
**
** Parameters : Widget w       - toplevel widget to attach icon to 
**              int attachFlag - If TRUE attach the icon to the
**                               application window
**              The rest of the parameters deal with the physical
**              characteristics of the icon.
**
** Return : A pointer to the created icon
**------------------------------------------------------------------*/
Pixmap *InitIcon ( Widget *w, int width, int height, int ncolors,
                int charsPerPixel, char *colors, char *pixels,
                unsigned char *iconName, Boolean attachFlag )
{
    static Pixmap   *icon;
    Arg             args[2];
    Screen          *screen;

    screen = XtScreen( *w );

    icon = ( Pixmap * ) XtMalloc( sizeof ( Pixmap ) );
    *icon = XCreatePixmapFromData( XtDisplay ( *w ),
                                  RootWindowOfScreen( screen ),
                                  DefaultColormapOfScreen( screen ),
                                  width, 
                                  height,
                                  DefaultDepthOfScreen( screen ),
                                  ncolors,
                                  charsPerPixel,
                                  colors,
                                  pixels );
    if ( attachFlag == TRUE )
    {
        XtSetArg( args[0], XtNiconPixmap, ( XtArgVal ) *icon );
        XtSetArg( args[1], XtNiconName, ( XtArgVal ) iconName );
        XtSetValues( *w, args, 2 );
    }
    return( icon );
}
