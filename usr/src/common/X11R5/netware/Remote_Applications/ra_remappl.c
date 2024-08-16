/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_remappl.c	1.3"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_remappl.c,v 1.3 1994/06/23 16:14:52 plc Exp $"

/*--------------------------------------------------------------------
** Filename : remappl.c
**
** Description : This file contains the main function for the 
**               Remote Application Launcher.
**
** Functions : main
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                            I N C L U D E S 
**-------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h> 
#include <locale.h>

#include "ra_saptypes.h"
#include "ra_hdr.h"

/*--------------------------------------------------------------------
**            C A L L B A C K     P R O T O T Y P E S
**------------------------------------------------------------------*/
void   ServerSelCB( Widget, XtPointer, XtPointer );

/*--------------------------------------------------------------------
** Function : main
**
** Description : This is the main function for the Remote Application
**               Launcher program.
**
** Parameters : argc and argv ( I don't know why yet ! )
**
** Return : 0 on success
**          Error code otherwise
**------------------------------------------------------------------*/
char *arg;
int main ( int argc, char **argv )
{

    arg = argv[0];
    GenerateScreen( sapType, ServerSelCB, &argc, argv );

EXIT_MAIN:
    return ( 0 );
}
