/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwbindry.h	1.5"
/* *************************************************************************

   Copyright (c) 1991-1994 Novell, Inc.  All Rights Reserved.                      
                                                                            
   THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND TREATIES.
   NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED COPIED, DISTRIBUTED,
   REVISED, MODIFIED, TRANSLATED, ABRIDGED, CONDENSED,  EXPANDED,  COLLECTED,
   COMPILED, LINKED,  RECAST, TRANSFORMED  OR  ADAPTED  WITHOUT   THE   PRIOR
   WRITTEN  CONSENT  OF  NOVELL, INC.  ANY USE  OR EXPLOITATION OF THIS WORK
   WITHOUT AUTHORIZATION   COULD  SUBJECT  THE  PERPETRATOR  TO CRIMINAL AND 
   CIVIL LIABILITY.              

   **************************************************************************/

#ifndef NWBINDRY_INC
#define NWBINDRY_INC

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
# include <nw/nwcaldef.h>
#else /* !N_PLAT_UNIX */
# include <nwcaldef.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Bindery object types (in HIGH-LOW order) */
#define OT_WILD                 0xFFFF
#define OT_UNKNOWN              0x0000
#define OT_USER                 0x0100
#define OT_USER_GROUP           0x0200
#define OT_PRINT_QUEUE          0x0300
#define OT_FILE_SERVER          0x0400
#define OT_JOB_SERVER           0x0500
#define OT_GATEWAY              0x0600
#define OT_PRINT_SERVER         0x0700
#define OT_ARCHIVE_QUEUE        0x0800
#define OT_ARCHIVE_SERVER       0x0900
#define OT_JOB_QUEUE            0x0A00
#define OT_ADMINISTRATION       0x0B00
#define OT_NAS_SNA_GATEWAY      0x2100
#define OT_REMOTE_BRIDGE_SERVER 0x2600
#define OT_TCPIP_GATEWAY        0x2700

/* Extended bindery object types */
#define OT_TIME_SYNCHRONIZATION_SERVER 0x2D00
#define OT_ARCHIVE_SERVER_DYNAMIC_SAP  0x2E00
#define OT_ADVERTISING_PRINT_SERVER    0x4700
#define OT_BTRIEVE_VAP                 0x5000
#define OT_PRINT_QUEUE_USER            0x5300


/* Bindery object and property flags */
#define BF_STATIC   0x00
#define BF_DYNAMIC  0x01
#define BF_ITEM     0x00
#define BF_SET      0x02

/*********  Bindery object and property security access levels  **********/
#define BS_ANY_READ      0x00   /* Readable by anyone                */
#define BS_LOGGED_READ   0x01   /* Must be logged in to read         */
#define BS_OBJECT_READ   0x02   /* Readable by same object or super  */
#define BS_SUPER_READ    0x03   /* Readable by supervisor only       */
#define BS_BINDERY_READ  0x04   /* Readable only by the bindery      */
#define BS_ANY_WRITE     0x00   /* Writeable by anyone               */
#define BS_LOGGED_WRITE  0x10   /* Must be logged in to write        */
#define BS_OBJECT_WRITE  0x20   /* Writeable by same object or super */
#define BS_SUPER_WRITE   0x30   /* Writeable only by the supervisor  */
#define BS_BINDERY_WRITE 0x40   /* Writeable by the bindery only     */

NWCCODE N_API NWVerifyObjectPassword
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         password
);

NWCCODE N_API NWDisallowObjectPassword
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         disallowedPassword
);

NWCCODE N_API NWChangeObjectPassword
(
  NWCONN_HANDLE   conn,
  pnstr8          objName,
  nuint16         objType,
  pnstr8          oldPassword,
  pnstr8          newPassword
);

NWCCODE N_API NWReadPropertyValue
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   nuint8         segmentNum,
   pnuint8        segmentData,
   pnuint8        moreSegments,
   pnuint8        flags
);

NWCCODE N_API NWWritePropertyValue
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   nuint8         segmentNum,
   pnuint8        segmentData,
   nuint8         moreSegments
);

NWCCODE N_API NWAddObjectToSet
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   pnstr8         memberName,
   nuint16        memberType
);

NWCCODE N_API NWDeleteObjectFromSet
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   pnstr8         memberName,
   nuint16        memberType
);

NWCCODE N_API NWIsObjectInSet
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   pnstr8         memberName,
   nuint16        memberType
);

NWCCODE N_API NWScanProperty
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         searchPropertyName,
   pnuint32       iterHandle,
   pnstr8         propertyName,
   pnuint8        propertyFlags,
   pnuint8        propertySecurity,
   pnuint8        valueAvailable,
   pnuint8        moreFlag
);

NWCCODE N_API NWGetObjectID
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnuint32       objID
);

NWCCODE N_API NWGetObjectDiskSpaceLeft
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   pnuint32       systemElapsedTime,
   pnuint32       unusedDiskBlocks,
   pnuint8        restrictionEnforced
);

NWCCODE N_API NWGetObjectName
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   pnstr8         objName,
   pnuint16       objType
);

NWCCODE N_API NWScanObject
(
   NWCONN_HANDLE  conn,
   pnstr8         searchName,
   nuint16        searchType,
   pnuint32       objID,
   pnstr8         objName,
   pnuint16       objType,
   pnuint8        hasPropertiesFlag,
   pnuint8        objFlags,
   pnuint8        objSecurity
);

NWCCODE N_API NWGetBinderyAccessLevel
(
   NWCONN_HANDLE  conn,
   pnuint8        accessLevel,
   pnuint32       objID
);

NWCCODE N_API NWCreateProperty
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   nuint8         propertyFlags,
   nuint8         propertySecurity
);

NWCCODE N_API NWDeleteProperty
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName
);

NWCCODE N_API NWChangePropertySecurity
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   pnstr8         propertyName,
   nuint8         newPropertySecurity
);

NWCCODE N_API NWCreateObject
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   nuint8         objFlags,
   nuint8         objSecurity
);

NWCCODE N_API NWDeleteObject
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType
);

NWCCODE N_API NWRenameObject
(
   NWCONN_HANDLE  conn,
   pnstr8         oldObjName,
   pnstr8         newObjName,
   nuint16        objType
);

NWCCODE N_API NWChangeObjectSecurity
(
   NWCONN_HANDLE  conn,
   pnstr8         objName,
   nuint16        objType,
   nuint8         newObjSecurity
);

NWCCODE N_API NWOpenBindery
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWCloseBindery
(
   NWCONN_HANDLE conn
);

NWCCODE N_API NWScanObjectTrusteePaths
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   nuint16        volNum,
   pnuint16       iterHandle,
   pnuint8        accessRights,
   pnstr8         dirPath
);

NWCCODE N_API NWGetObjectEffectiveRights
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint16       rightsMask
);

#ifdef __cplusplus
}
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwbindry.h,v 1.7 1994/09/26 17:11:49 rebekah Exp $
*/
