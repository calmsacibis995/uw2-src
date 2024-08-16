/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdentry.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWDENTRY_INC
#define NWDENTRY_INC

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
# include <nw/nwcaldef.h>
#else /* !N_PLAT_UNIX */
# include <nwcaldef.h>
#endif /* N_PLAT_UNIX */
#endif

#ifndef NWDIRECT_INC
#ifdef N_PLAT_UNIX
# include <nw/nwdirect.h>
#else /* !N_PLAT_UNIX */
# include <nwdirect.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   nstr8   entryName[16];
   nuint32 creationDateAndTime;
   nuint32 ownerID;
   nuint32 sequenceNumber;
   TRUSTEE_INFO trusteeList[20];
} NWET_INFO;

typedef struct
{
   nuint32  updateDateAndTime;
   nuint32  updatorID;
   nuint32  fileSize;
   nuint8   reserved[44];
   nuint16  inheritedRightsMask;
   nuint16  lastAccessDate;
   nuint8   reserved2[28];
} NWFILE_INFO;
typedef struct
{
   nuint32  lastModifyDateAndTime;
   nuint32  nextTrusteeEntry;
   nuint8   reserved[48];
   nuint32  maximumSpace;
   nuint16  inheritedRightsMask;
   nuint8   reserved2[14];
   nuint32  volObjectID;
   nuint8   reserved3[8];
} NWDIR_INFO;

typedef struct
{
   nuint32  sequence;
   nuint32  parent;
   nuint32  attributes;
   nuint8   uniqueID;
   nuint8   flags;
   nuint8   nameSpace;
   nuint8   nameLength;
   nuint8   name[12];
   nuint32  creationDateAndTime;
   nuint32  ownerID;
   nuint32  lastArchiveDateAndTime;
   nuint32  lastArchiverID;

   union
   {
      NWFILE_INFO file;
      NWDIR_INFO   dir;
   }info;

} NWENTRY_INFO;

typedef struct
{
   nuint32 sequence;
   nuint32 parent;
   nuint32 attributes;
   nuint8  uniqueID;
   nuint8  flags;
   nuint8  nameSpace;
   nuint8  nameLength;
   nuint8  name [12];
   nuint32 creationDateAndTime;
   nuint32 ownerID;
   nuint32 lastArchiveDateAndTime;
   nuint32 lastArchiverID;
   nuint32 updateDateAndTime;
   nuint32 lastUpdatorID;
   nuint32 dataForkSize;         /* file size */
   nuint32 dataForkFirstFAT;
   nuint32 nextTrusteeEntry;
   nuint8  reserved[36];
   nuint16 inheritedRightsMask;
   nuint16 lastAccessDate;
   nuint32 deletedFileTime;
   nuint32 deletedDateAndTime;
   nuint32 deletorID;
   nuint8  reserved2 [16];
   nuint32 otherForkSize[2];
} NW_EXT_FILE_INFO;

#define TR_NONE         0x0000
#define TR_READ         0x0001
#define TR_WRITE        0x0002
#define TR_OPEN         0x0004
#define TR_DIRECTORY    0x0004
#define TR_CREATE       0x0008
#define TR_DELETE       0x0010
#define TR_ERASE        0x0010
#define TR_OWNERSHIP    0x0020
#define TR_ACCESS_CTRL  0x0020
#define TR_FILE_SCAN    0x0040
#define TR_SEARCH       0x0040
#define TR_FILE_ACCESS  0x0040
#define TR_MODIFY       0x0080
#define TR_ALL          0x01FB
#define TR_SUPERVISOR   0x0100
#define TR_NORMAL       0x00FB

#ifndef MModifyNameBit
#define MModifyNameBit           0x0001L
#define MFileAttributesBit       0x0002L
#define MCreateDateBit           0x0004L
#define MCreateTimeBit           0x0008L
#define MOwnerIDBit              0x0010L
#define MLastArchivedDateBit     0x0020L
#define MLastArchivedTimeBit     0x0040L
#define MLastArchivedIDBit       0x0080L
#define MLastUpdatedDateBit      0x0100L
#define MLastUpdatedTimeBit      0x0200L
#define MLastUpdatedIDBit        0x0400L
#define MLastAccessedDateBit     0x0800L
#define MInheritedRightsMaskBit  0x1000L
#define MMaximumSpaceBit         0x2000L
#endif

NWCCODE N_API NWDeleteTrustee
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath,
   nuint32        objID
);

NWCCODE N_API NWAddTrustee
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint32        objID,
   nuint16        rightsMask
);

#define NWScanDirEntryInfo(a, b, c, d, e, f) \
        NWIntScanDirEntryInfo(a, b, c, d, e, f, 0)

NWCCODE N_API NWIntScanDirEntryInfo
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint16        attrs,
   pnuint32       iterHandle,
   pnuint8        searchPattern,
   NWENTRY_INFO N_FAR * entryInfo,
   nuint16        augmentFlag
);

#define NWScanForTrustees(a, b, c, d, e, f) \
        NWIntScanForTrustees(a, b, c, d, e, f, 0)

NWCCODE N_API NWIntScanForTrustees
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint32       iterHandle,
   pnuint16       numOfEntries,
   NWET_INFO N_FAR * entryTrusteeInfo,
   nuint16        augmentFlag
);

#define NWMoveDirEntry(a, b, c, d, e, f) \
        NWIntMoveDirEntry(a, b, c, d, e, f, 0)

NWCCODE N_API NWIntMoveDirEntry
(
   NWCONN_HANDLE  conn,
   nuint8         searchAttrs,
   NWDIR_HANDLE   srcDirHandle,
   pnstr8         srcPath,
   NWDIR_HANDLE   dstDirHandle,
   pnstr8         dstPath,
   nuint16        augmentFlag
);

NWCCODE N_API NWSetDirEntryInfo
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint8         searchAttrs,
   nuint32        iterHandle,
   nuint32        changeBits,
   NWENTRY_INFO N_FAR * newEntryInfo
);

#define NWScanExtendedInfo(a, b, c, d, e, f) \
        NWIntScanExtendedInfo(a, b, c, d, e, f, 0)

NWCCODE N_API NWIntScanExtendedInfo
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint8         attrs,
   pnuint32       iterHandle,
   pnstr8         searchPattern,
   NW_EXT_FILE_INFO N_FAR * entryInfo,
   nuint16        augmentFlag
);

NWCCODE N_API NWGetEffectiveRights
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint16       effectiveRights
);

#ifdef __cplusplus
}
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdentry.h,v 1.6 1994/06/08 23:32:35 rebekah Exp $
*/
