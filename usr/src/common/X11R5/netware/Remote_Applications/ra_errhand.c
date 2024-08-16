/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)remapps:ra_errhand.c	1.2"
#ident	"@(#)ra_errhand.c	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Remote_Applications/ra_errhand.c,v 1.1 1994/02/01 22:50:49 renu Exp $"

/*--------------------------------------------------------------------
** Filename : dl_errhand.c
**
** Description : These functions set up an error handler and reserve 
**               sufficient memory to print an out of memory error 
**               before exiting.
**
** Functions : SetupErrHndlr 
**             ErrHndlr
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                          I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include "ra_hdr.h"


/*--------------------------------------------------------------------
**                          D E F I N E S
**------------------------------------------------------------------*/
#define      BUF_SIZE        60000 



/*--------------------------------------------------------------------
**                  S T A T I C   B U F F E R 
**------------------------------------------------------------------*/
static char      *reserveBuf;
static char      *reserveErrStr;


/*--------------------------------------------------------------------
** Function : SetupErrHndlr
**
** Description : This functions installs a new error handler and
**               mallocs a buffer to allow for user feedback on
**               fatal error messages. ( curr. mallocs with
**               inadequate memory.  
**
** Parameters : XtAppContext context - passed to call to setup error
**                                     handler.
**              XtErrorHandler  handler - The error handler to install
**
** Return :
**------------------------------------------------------------------*/
void SetupErrHndlr( XtAppContext context, XtErrorHandler handler )
{
    XtAppSetErrorHandler( context, handler );
}


/*--------------------------------------------------------------------
** Function : ErrHndlr
**
** Description : This function is a new error handler.
**
** Parameters : Same as for XtAppErrorMsg
**
** Return : Does not return
**------------------------------------------------------------------*/
void ErrHndlr( XtAppContext context, String name, String type,
               String class, String dflt, String *params,
               Cardinal *num_params )
{
	static char *errProg = { "/usr/X/bin/.Xerrhand" };

    execl( errProg, errProg,  dflt, (char *) 0 );
}
