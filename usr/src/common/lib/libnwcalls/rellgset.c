/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rellgset.c	1.5"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY    /* only use the locks part of nwfile.h */
#include "nwfile.h"

/*manpage*NWReleaseLogicalRecordSet*****************************************
SYNTAX:  NWCCODE N_API NWReleaseLogicalRecordSet(
            void)

REMARKS: Unlocks all logical records that are currently locked in the log
         table of the requesting workstation, but does not remove them from
         the log table.

         In order to avoid deadlock, a workstation is required to request
         those records it will need to lock, and it does so by making an
         entry into a log table at the file server.  Once the log table is
         complete, the application can then lock those records.  The locking
         will only work if all records in the table are available.

         A logical record is simply a name (a string) registered with the
         file server.  The name (as with a semaphore) can then be locked or
         unlocked by applications, and can be used as a "good-guy"
         inter-application locking mechanism.  Note that locking or unlocking
         a logical record does not physically lock or unlock those resources
         associated with the logical record since only the applications using
         the record know about such an association.

ARGS:

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     13 --  Release Logical Record Set

CHANGES: 30 Aug 1993 - Purposefully NOT NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReleaseLogicalRecordSet(
  void)
{
  nuint8        reqBuf[1];
  NWCCODE ccode;
  NW_FRAGMENT reqFrag[1];

  reqBuf[0] = 0;  /* Unlock all sets */

  reqFrag[0].fragAddress = reqBuf;
  reqFrag[0].fragSize    = 1;

  ccode = NWRequestToAll(13, 1, reqFrag);
#ifdef N_PLAT_OS2
  if(ccode)
    return ccode;
  return(NWClearLockSemList(1));
#else
  return ccode;
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rellgset.c,v 1.7 1994/09/26 17:48:58 rebekah Exp $
*/
