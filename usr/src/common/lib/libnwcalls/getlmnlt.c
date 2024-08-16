/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getlmnlt.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetLoadedMediaNumList*******************************************
SYNTAX:  NWCCODE N_API NWGetLoadedMediaNumList
         (
            NWCONN_HANDLE conn,
            NWFSE_LOADED_MEDIA_NUM_LIST NWPTR fseLoadedMediaNumList
         )

REMARKS:

ARGS: >  conn
      <  fseLoadedMediaNumList

INCLUDE: nwfse.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 47  Get Loaded Media Num List

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetLoadedMediaNumList
(
   NWCONN_HANDLE               conn,
   NWFSE_LOADED_MEDIA_NUM_LIST NWPTR fseLoadedMediaNumList
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s47GetLoadedMediaList(&access,
         (pNWNCPFSEVConsoleInfo)
         &fseLoadedMediaNumList->serverTimeAndVConsoleInfo,
         &fseLoadedMediaNumList->reserved,
         &fseLoadedMediaNumList->maxMediaTypes,
         &fseLoadedMediaNumList->mediaListCount,
         fseLoadedMediaNumList->mediaList));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getlmnlt.c,v 1.7 1994/09/26 17:46:12 rebekah Exp $
*/

