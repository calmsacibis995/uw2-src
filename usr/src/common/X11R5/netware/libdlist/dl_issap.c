/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libdlist:dl_issap.c	1.2"
#ident	"@(#)dl_issap.c	2.1 %Q"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/libdlist/dl_issap.c,v 1.1 1994/02/01 22:52:16 renu Exp $"

/*--------------------------------------------------------------------
** Filename : dl_issap.c
**
** Description : This file contains a function to check to see if 
**               a machine is sapping a a particular sap type.
**
** Functions : IsSap
**------------------------------------------------------------------*/


/*--------------------------------------------------------------------
**                        I N C L U D E S
**------------------------------------------------------------------*/
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <stdlib.h>
#include <dirent.h>

#include "dl_common.h"
#include "dl_protos.h"


/*--------------------------------------------------------------------
**                        D E F I N E S
**------------------------------------------------------------------*/
#define     SAP_OUT_DIR       "/var/spool/sap/out"


/*--------------------------------------------------------------------
** Function : IsSap
**
** Parameters : long  sapFile - The number of the sap file.
**
** Return : TRUE if the file exists
**          FALSE if the file doesn't exist
**------------------------------------------------------------------*/
int IsSap( long sapFile )
{
    int                retCode = FAILURE;
    DIR               *Dp;
    struct dirent     *dirPtr;

    Dp = opendir( SAP_OUT_DIR );
    for ( dirPtr = readdir( Dp ); dirPtr != NULL; dirPtr = readdir( Dp ) )
        if ( ( strtol( dirPtr->d_name, NULL, 0 ) ) == sapFile )
            retCode = SUCCESS;
    return( retCode );
}
