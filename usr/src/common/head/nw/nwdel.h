/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdel.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWDEL_INC
#define NWDEL_INC

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
# include <nw/nwcaldef.h>
#else /* !N_PLAT_UNIX */
# include <nwcaldef.h>
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
   nuint32  sequence;
   nuint32  parent;
   nuint32  attributes;
   nuint8   uniqueID;
   nuint8   flags;
   nuint8   nameSpace;
   nuint8   nameLength;
   nuint8   name [256];
   nuint32  creationDateAndTime;
   nuint32  ownerID;
   nuint32  lastArchiveDateAndTime;
   nuint32  lastArchiverID;
   nuint32  updateDateAndTime;
   nuint32  updatorID;
   nuint32  fileSize;
   nuint8   reserved[44];
   nuint16  inheritedRightsMask;
   nuint16  lastAccessDate;
   nuint32  deletedTime;
   nuint32  deletedDateAndTime;
   nuint32  deletorID;
   nuint8   reserved3 [16];
} NWDELETED_INFO;

NWCCODE N_API NWPurgeDeletedFile
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint32        iterHandle,
   nuint32        volNum,
   nuint32        dirBase,
   pnstr8         fileName
);

NWCCODE N_API NWRecoverDeletedFile
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   nuint32        iterHandle,
   nuint32        volNum,
   nuint32        dirBase,
   pnstr8         delFileName,
   pnstr8         rcvrFileName
);

NWCCODE N_API NWScanForDeletedFiles
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnuint32       iterHandle,
   pnuint32       volNum,
   pnuint32       dirBase,
   NWDELETED_INFO N_FAR * entryInfo
);

NWCCODE N_API NWPurgeErasedFiles
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWRestoreErasedFile
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath,
   pnstr8         oldName,
   pnstr8         newName
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdel.h,v 1.6 1994/06/08 23:32:34 rebekah Exp $
*/
