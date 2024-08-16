/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:readprop.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwcaldef.h"
#include "nwbindry.h"

/*manpage*NWReadPropertyValue***********************************************
SYNTAX:  NWCCODE N_API NWReadPropertyValue
         (
            NWCONN_HANDLE  conn,
            pnstr8         objName,
            nuint16        objType,
            pnstr8         propertyName,
            nuint8         segmentNumber,
            pnuint8        segmentData,
            pnuint8        moreSegments,
            pnuint8        flags
         );

REMARKS: This call allows a client to retrieve the value associated with
         the specified property.  Property values are stored in a single
         128-byte segment.  The Segment Number must be set to 1 to read the
         first segment of a value and must be incremented by one for each
         subsequent call.  The 128-byte segment corresponding to Segment
         Number is returned to the client.

         The More Flag will be set to 0xFF if the requested Segment Number
         is not the last segment in the value and 0 if the Segment Number
         is the last segment.

         The Property Flags byte contains the property's status flags.  This
         byte can be tested to determine whether the property is a set property
         or an item property and whether the property is static or dynamic.

         The Object Type cannot be WILD (-1); the Object Name and Property Name
         cannot contain wildcard characters.

         This call can be used successfully by clients with read privileges to
         the specified property.


ARGS: >  conn
      >  objName
      >  objType
      >  propertyName
      >  segmentNumber
      <  segmentData
      <  moreSegments
      <  flags


INCLUDE: nwbindry.h

RETURN:  0x0000  Successful
         0x8988  Invalid File Handle
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x89EC  No Such Set
         0x89F0  Illegal Wildcard
         0x89F1  Bindery Security
         0x89F9  No Property Read
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure


SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 61  Read Property Value

CHANGES: 27 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWReadPropertyValue
(
   NWCONN_HANDLE conn,
   pnstr8  objName,
   nuint16 objType,
   pnstr8  propertyName,
   nuint8  segmentNumber,
   pnuint8 segmentData,
   pnuint8 moreSegments,
   pnuint8 flags
)
{
    NWCDeclareAccess(access);

    NWCSetConn(access, conn);

    return ((NWCCODE) NWNCP23s61ReadPropertyValue (&access, NSwap16(objType),
                     (nuint8) NWCStrLen(objName), objName,
                     segmentNumber, (nuint8) NWCStrLen(propertyName),
                     propertyName, segmentData, moreSegments,
                     flags));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/readprop.c,v 1.7 1994/09/26 17:48:49 rebekah Exp $
*/

