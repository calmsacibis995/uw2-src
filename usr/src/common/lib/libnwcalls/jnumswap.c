/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:jnumswap.c	1.4"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwcaldef.h"
#include "nwqms.h"
#include "nwintern.h"



/*manpage*__NWSwapJobStructIDs**********************************************
SYNTAX:  void N_API __NWSwapJobStructIDs
         (
            QueueJobStruct NWPTR pJob
         );

REMARKS: This internal function NSwap's all of the bindery object ID's in
         the job struct; we need to do this because NWCalls is broken and
         expects all object ID's to be anti-native

ARGS: <> pJob

INCLUDE: nwintern.h

CLIENT:  DOS WIN OS2 NT UNIX

CHANGES: 12 Feb 1994 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
void N_API __NWSwapJobStructIDs
(
   QueueJobStruct NWPTR pJob
)
{
   if (!pJob)
      return;

   pJob->clientID           = NSwap32(pJob->clientID);
   pJob->targetServerID     = NSwap32(pJob->targetServerID);
   pJob->servicingServerID  = NSwap32(pJob->servicingServerID);
}




/*manpage*__NWSwapNWJobStructIDs********************************************
SYNTAX:  void N_API __NWSwapNWJobStructIDs
         (
            NWQueueJobStruct NWPTR pJob
         );

REMARKS: This internal function NSwap's all of the bindery object ID's in
         the job struct; we need to do this because NWCalls is broken and
         expects all object ID's to be anti-native

ARGS: <> pJob

INCLUDE: nwintern.h

CLIENT:  DOS WIN OS2 NT UNIX

CHANGES: 1/28/94 - Create seperate file for this call it was contained in
                   crteqjo2.c before but that file is not used by the UNIX
                   platform.  Bree
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
void N_API __NWSwapNWJobStructIDs
(
   NWQueueJobStruct NWPTR pJob
)
{
   if (!pJob)
      return;

   pJob->clientID           = NSwap32(pJob->clientID);
   pJob->targetServerID     = NSwap32(pJob->targetServerID);
   pJob->servicingServerID  = NSwap32(pJob->servicingServerID);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/jnumswap.c,v 1.5 1994/06/08 23:11:16 rebekah Exp $
*/
