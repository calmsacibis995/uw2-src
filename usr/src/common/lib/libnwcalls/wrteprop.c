/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:wrteprop.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwbindry.h"
#include "nwcaldef.h"

#if defined (N_PLAT_UNIX)
#endif /* N_PLAT_UNIX */

/*manpage*NWWritePropertyValue**********************************************
SYNTAX:  NWCCODE N_API NWWritePropertyValue
         (
            NWCONN_HANDLE  conn,
            pnstr8         objectName,
            nuint16        objectType,
            pnstr8         propertyName,
            nuint8         segmentNumber,
            pnuint8        segmentData,
            nuint8         moreSegments
         );

REMARKS:
         Writes the property data of a bindery object on the file server
         associated with the given connection ID.

         The segmentNumber parameter indicates which segment of data is being
         written and should be assigned a value of 1 for the first segment.
         In addition, the moreSegments parameter should contain a value of
         0xFF.  When the function call returns, the segmentNumber will now
         point to the next segment (Netware updates it automatically).  To
         signal Netware that the last segment is being written, assign the
         moreSegments parameter to 0x00.

         Create property value segments sequentially.  In other words, before
         segment N can be created, all segments from 1 to N-1 must have been
         created.  However, once all segments of a property value have been
         established, segments can then be written at random.  If the segment
         data is longer than 128 bytes it is truncated and the 128th byte
         will contain a NULL.

         The objName, objType, and propertyName parameters must
         uniquely identify the property and cannot contain wildcard
         specifiers.

         We recommend that property values be kept to a single segment (128
         bytes) to improve bindery efficiency.

         The bindery makes no attempt to coordinate activities among multiple
         workstations concurrently reading or writing data to a single
         property.  This means that one workstation might read a partially
         updated property and get inconsistent data if the property's data
         extends across multiple segments.  If this presents a problem,
         coordination on reads and writes must be handled by application
         programs.  Logical record locks can be used to coordinate activities
         among applications.

         A client must have write access to the property to call this
         function.

ARGS: >  conn
      >  objectName
      >  objectType
      >  propertyName
      >  segmentNumber
      >  segmentData
      >  moreSegments

INCLUDE: nwbindry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 62  Write Property Value

CHANGES: 23 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWWritePropertyValue
(
   NWCONN_HANDLE  conn,
   pnstr8         objectName,
   nuint16        objectType,
   pnstr8         propertyName,
   nuint8         segmentNumber,
   pnuint8        segmentData,
   nuint8         moreSegments
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE)NWNCP23s62WritePropertyValue(&access, NSwap16(objectType),
                  (nuint8)NWCStrLen(objectName), objectName, segmentNumber,
                  moreSegments, (nuint8)NWCStrLen(propertyName),
                  propertyName, segmentData));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/wrteprop.c,v 1.7 1994/09/26 17:50:37 rebekah Exp $
*/

