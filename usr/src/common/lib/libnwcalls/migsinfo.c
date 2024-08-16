/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:migsinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwmigrat.h"

/*manpage*NWGetSupportModuleInfo********************************************
SYNTAX:  NWCCODE N_API NWGetSupportModuleInfo
         (
           NWCONN_HANDLE conn,
           nuint32 infoLevel,
           nuint32 moduleID,
           pnuint8 returnInfo,
           pnuint32 returnInfoLen
         )

REMARKS: Gets information about the Data Migrator NLM support modules or
         a list of all loaded support module ID's.

ARGS:
       > infoLevel
         If infomationLevel = 0, then return information about the DM nlm
         support module, else, if infomationLevel = 1, then return a list of
         all loaded support module ID's.

       > moduleID
         The assigned ID number of the support module that will migrate the
         data.

       < returnInfo
         Return information buffer
           if(infomationLevel == 0)
             typdef struct
             {
               nuint32 IOStatus;     Read and write access
               nuint32 infoBlockSize Information Block Size
               nuint32 availSpace;   Amount of space free on support module
               nuint32 usedSpace;    Amount of used space on support module
               nuint8 SModuleName;   Support module name (max size is 128)
               nuint8 SMinfo[128];   Info about a selected DM suport module
             } SUPPORT_MODULE_INFO;
           else if (infomationLevel == 1)
             typdef struct
             {
               nuint32 numberOfSMs;  Number of valid support module ID's
               nuint32 SMIDs[32];    List of Support Module ID's
             } SUPPORT_MODULE_IDS;
           else if (infomationLevel == 2)
             typdef struct
             {
               nuint8 nameLen;       The length of the support module name
               nuint8 Name[MAX_SIZE_OF_SUPPORT_MODULE_NAMES];
             } SUPPORT_MODULE_IDS;

       < returnInfoLen
         The size of the data area that the user allocated to return the
         information in.

INCLUDE: nwmigrat.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     90 132 DM Support Module Information

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetSupportModuleInfo
(
   NWCONN_HANDLE  conn,
   nuint32        infoLevel,
   nuint32        moduleID,
   pnuint8        returnInfo,
   pnuint32       returnInfoLen
)
{
    NWCDeclareAccess(access);

    NWCSetConn(access, conn);

    return ((NWCCODE) NWNCP90s132GetDMSupportModInfo( &access,
                        infoLevel, moduleID, returnInfo, returnInfoLen) );
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/migsinfo.c,v 1.7 1994/09/26 17:48:10 rebekah Exp $
*/
