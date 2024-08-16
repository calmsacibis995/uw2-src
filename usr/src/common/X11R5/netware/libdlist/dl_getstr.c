/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libdlist:dl_getstr.c	1.2"
#ident	"@(#)dl_getstr.c	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/libdlist/dl_getstr.c,v 1.1 1994/02/01 22:52:12 renu Exp $"

/*--------------------------------------------------------------------
** Filename : dl_getstr.c
**
** Description : This file contains a function to get an 
**               internationalized string. 
**
** Functions : GetInternationalStr
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                        I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <unistd.h>

#include "dl_fsdef.h"


/*--------------------------------------------------------------------
** Function : GetInternationalStr
**
** Parameters : unsigned char *stringID - The string id. the string id 
**                                        has the format
**                                       "filename:id" FS_CHR "default_str"
**
** Return : The internationalized string
**------------------------------------------------------------------*/
unsigned char *GetInternationalStr( unsigned char *stringID )
{
    unsigned char         *sep;
    unsigned char         *str;

    sep = ( unsigned char * )strchr( ( char *)stringID, FS_CHR );
    *sep = 0;
    str = ( unsigned char * )gettxt( ( char * )stringID, ( char * )sep + 1 );
    *sep = FS_CHR; 
    return( str );
}
