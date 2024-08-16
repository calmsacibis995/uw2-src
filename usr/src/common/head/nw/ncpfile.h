/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpfile.h	1.5"
#if !defined( NCPFILE_H )
#define NCPFILE_H

#if !defined( NTYPES_H )
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include <ntypes.h>
#endif /* N_PLAT_UNIX */
#endif

#if !defined( NWACCESS_H )
#ifdef N_PLAT_UNIX
#include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
#include <nwaccess.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

/***************************************************************************
   Constant declarations
****************************************************************************/

#define NWNCP_NS_DOS     0
#define NWNCP_NS_MAC     1
#define NWNCP_NS_NFS     2
#define NWNCP_NS_FTAM    3
#define NWNCP_NS_OS2     4
#define NWNCP_NS_UNICODE 5

#define NWNCP_DS_DOS     0
#define NWNCP_DS_MAC     1
#define NWNCP_DS_FTAM    2


#define NWNCP_COMPPATH_USE_DIRHANDLE   0
#define NWNCP_COMPPATH_USE_DIRBASE     1
#define NWNCP_COMPPATH_NO_HANDLE       0xff


#define NWNCP_CHAR_COLON      58
#define NWNCP_CHAR_BACKSLASH  92
#define NWNCP_CHAR_SLASH      47
#define NWNCP_CHAR_STAR       42
#define NWNCP_CHAR_PERIOD     46
#define NWNCP_CHAR_QUESTION   63
#define NWNCP_CHAR_BREAK      255

/***************************************************************************
   Structure declarations
****************************************************************************/

typedef struct tagNWNCPEntryStruct
{
  nuint32   luSpaceAlloc;
  nuint32   luAttrs;
  nuint16   suFlags;
  nuint32   luDataStreamSize;
  nuint32   luTotalStreamSize;
  nuint16   suNumStreams;
  nuint16   suCreationTime;
  nuint16   suCreationDate;
  nuint32   luCreatorID;
  nuint16   suModifiedTime;
  nuint16   suModifiedDate;
  nuint32   luModifierID;
  nuint16   suAccessedDate;
  nuint16   suArchivedTime;
  nuint16   suArchivedDate;
  nuint32   luArchiverID;
  nuint16   suInheritedRightsMask;
  nuint32   luDirBase;
  nuint32   luDosDirBase;
  nuint32   luVolNum;
  nuint32   luEADataSize;
  nuint32   luEAKeyCount;
  nuint32   luEAKeySize;
  nuint32   luNamSpcCreator;
  nuint8    buNameLen;
  nuint8    abuName[256];
} NWNCPEntryStruct, N_FAR *pNWNCPEntryStruct;

typedef struct tagNWNCPCompPath
{
   nuint8   abuPacked[308];
   nuint8   buVolNum;
   nuint32  luDirBase;
   nuint8   buHandleFlag;
   nuint8   buCompCnt;
   nuint8   buNamSpc;
   nuint16  suPackedLen;
} NWNCPCompPath, N_FAR *pNWNCPCompPath;

typedef struct tagNWNCPCompPath2
{
   nuint8   abuPacked[534];
   nuint8   buSrcVolNum;
   nuint32  luSrcDirBase;
   nuint8   buSrcHandleFlag;
   nuint8   buSrcCompCnt;
   nuint8   buSrcNamSpc;
   nuint8   buDstVolNum;
   nuint32  luDstDirBase;
   nuint8   buDstHandleFlag;
   nuint8   buDstCompCnt;
   nuint8   buDstNamSpc;
   nuint16  suPackedLen;
} NWNCPCompPath2, N_FAR *pNWNCPCompPath2;

typedef struct tagNWNCPModifyDosInfo
{
   nuint16  suFileAttrs;
   nuint8   buFileMode;
   nuint8   buFileXAttrs;
   nuint16  suCreationDate;
   nuint16  suCreationTime;
   nuint32  luCreatorID;
   nuint16  suModifiedDate;
   nuint16  suModifiedTime;
   nuint32  luModifierID;
   nuint16  suArchivedDate;
   nuint16  suArchivedTime;
   nuint32  luArchiverID;
   nuint16  suLastAccessDate;
   nuint16  suInheritanceGrantMask;
   nuint16  suInheritanceRevokeMask;
   nuint32  luMaxSpace;
} NWNCPModifyDosInfo, N_FAR *pNWNCPModifyDosInfo;

typedef struct tagNWNCPFileInfo
{
   nuint16  suReserved;
   nuint8   abuFileName[14];
   nuint8   buAttrs;
   nuint8   buExeType;
   nuint32  luSize;
   nuint16  suCreationDate;
   nuint16  suAccessedDate;
   nuint16  suModifiedDate;
   nuint16  suModifiedTime;
} NWNCPFileInfo, N_FAR *pNWNCPFileInfo;

typedef struct tagNWNCPFileInfo2
{
   nuint8   buAttrs;
   nuint8   buExtAttrs;
   nuint32  luSize;
   nuint16  suCreationDate;
   nuint16  suAccessedDate;
   nuint16  suModifiedDate;
   nuint16  suModifiedTime;
   nuint32  luOwnerID;
   nuint16  suArchiveDate;
   nuint16  suArchiveTime;
} NWNCPFileInfo2, N_FAR *pNWNCPFileInfo2;

typedef union tagNWNCPSrchInfo
{
   struct
   {
      nuint16  suReserved;
      nuint8   abstrFileName[14];
      nuint8   buAttrs;
      nuint8   buExeType;
      nuint32  luSize;
      nuint16  suCreationDate;
      nuint16  suAccessedDate;
      nuint16  suModifiedDate;
      nuint16  suModifiedTime;
   } f;
   struct
   {
      nuint16 suReserved1;
      nstr8   abstrDirName[14];
      nuint8  buDirAttributes;
      nuint8  buDirAccessRights;
      nuint16 suCreateDate;
      nuint16 suCreateTime;
      nuint32 luOwningObjectID;
      nuint16 suReserved2;
      nuint16 suDirStamp;
   } d;
} NWNCPSrchInfo, N_FAR *pNWNCPSrchInfo;

typedef struct tagNWNCPPathCookie
{
   nuint16  suFlags;
   nuint32  luCookie1;
   nuint32  luCookie2;
} NWNCPPathCookie, N_FAR *pNWNCPPathCookie;

typedef struct tagNWNCPDiskLvlRest
{
  nuint8    buLevel;
  nuint32   luMax;
  nuint32   luCurrent;
} NWNCPDiskLvlRest, N_FAR *pNWNCPDiskLvlRest;

typedef struct tagNWNCPEntryUnion
{
   nuint32  luSubdir;
   nuint32  luAttrs;
   nuint8   buUniqueID;
   nuint8   buFlags;
   nuint8   buNamSpc;
   nuint8   buNameLen;

   union
   {
      struct
      {
         nuint8   abuName[12];
         nuint32  luCreationDateAndTime;
         nuint32  luOwnerID;
         nuint32  luArchivedDateAndTime;
         nuint32  luArchiverID;
         nuint32  luModifiedDateAndTime;
         nuint32  luModifierID;
         nuint32  luFileSize;
         nuint8   abuReserved1[44];
         nuint16  suInheritedRightsMask;
         nuint16  suLastAccessedDate;
         nuint8   abuReserved2[28];
      } file;

      struct
      {
         nuint8   abuName[12];
         nuint32  luCreationDateAndTime;
         nuint32  luOwnerID;
         nuint32  luArchivedDateAndTime;
         nuint32  luArchiverID;
         nuint32  luModifiedDateAndTime;
         nuint32  luNextTrusteeEntry;
         nuint8   abuReserved1[48];
         nuint32  luMaxSpace;
         nuint16  suInheritedRightsMask;
         nuint8   abuReserved2[14];
         nuint32  luVolObjID;
         nuint8   abuReserved3[8];
      } dir;

      struct
      {
         nuint8   abuFileName[32];
         nuint32  luResourceFork;
         nuint32  luResourceForkSize;
         nuint8   abuFinderInfo[32];
         nuint8   abuProDosInfo[6];
         nuint8   abuReserved[38];
      } mac;

      struct
      {
         nuint8   abuName[12];
         nuint32  luCreationDateAndTime;
         nuint32  luOwnerID;
         nuint32  luArchivedDateAndTime;
         nuint32  luArchiverID;
         nuint32  luModifiedDateAndTime;
         nuint32  luModifierID;
         nuint32  luDataForkSize;
         nuint32  luDataForkFirstFAT;
         nuint32  luNextTrusteeEntry;
         nuint8   abuReserved1[36];
         nuint16  suInheritedRightsMask;
         nuint16  suLastAccessedDate;
         nuint32  luDeletedFileTime;
         nuint32  luDeletedDateAndTime;
         nuint32  luDeletorID;
         nuint8   abuReserved2[8];
         nuint32  luPrimaryEntry;
         nuint32  luNameList;
      } entry;
   } info;
} NWNCPEntryUnion, N_FAR *pNWNCPEntryUnion;

typedef struct tagNWNCPDelEntryInfo
{
   nuint32 luSubdir;
   nuint32 luAttrs;
   nuint8  buUniqueID;
   nuint8  buFlags;
   nuint8  buNamSpc;
   nuint8  buFileNameLen;
   nstr8   pbstrFileNameB256[256];
   nuint32 luCreationDateTime;
   nuint32 luOwnerID;
   nuint32 luArchivedDateTime;
   nuint32 luArchiverID;
   nuint32 luUpdatedDateTime;
   nuint32 luUpdatorID;
   nuint32 luFileSize;
   nuint8  abuReservedB44[44];
   nuint16 suRightsMask;
   nuint16 suAccessDate;
   nuint32 luDelFileTime;
   nuint32 luDelDateTime;
   nuint32 luDelID;
   nuint8  abuReservedB16[16];
} NWNCPDelEntryInfo, N_FAR *pNWNCPDelEntryInfo;

typedef struct tagNWNCPRestrictions
{
   nuint32      luObjID;
   nuint32  luRestriction;
} NWNCPRestrictions, N_FAR *pNWNCPRestrictions;

typedef struct tagNWNCPTrustees
{
   nuint32      luObjID;
   nuint16  suRights;
} NWNCPTrustees, N_FAR *pNWNCPTrustees;

typedef struct tagNWNCPNSNames
{
   nuint8   buNameLen;
   nuint8   abuName[32];
} NWNCPNSNames, N_FAR *pNWNCPNSNames;

typedef struct tagNWNCPDSNames
{
   nuint8   buAssociatedNS;
   nuint8   buNameLen;
   nuint8   abuName[32];
} NWNCPDSNames, N_FAR *pNWNCPDSNames;

typedef struct tagNWNCPConnUsingFile
{
   nuint16  suConnNum;
   nuint8   buTaskNum;
   nuint8   buLockType;
   nuint8   buAccessControl;
   nuint8   buLockFlag;
} NWNCPConnUsingFile, N_FAR *pNWNCPConnUsingFile;

typedef struct tagNWNCPFileUsed2x
{
   nuint16  suUseCount;
   nuint16  suOpenCount;
   nuint16  suOpenForReadCount;
   nuint16  suOpenForWriteCount;
   nuint16  suDenyReadCount;
   nuint16  suDenyWriteCount;
   nuint16  suNextRequest;
   nuint8   buLocked;
   nuint8   buConnCount;
   NWNCPConnUsingFile aConnInfo[70];
} NWNCPFileUsed2x, N_FAR *pNWNCPFileUsed2x;

typedef struct tagNWNCPFileUsed  /* as per NWCALLS struct 'CONNS_USING_FILE' */
{
   nuint16  suNextRequest;
   nuint16  suUseCount;
   nuint16  suOpenCount;
   nuint16  suOpenForReadCount;
   nuint16  suOpenForWriteCount;
   nuint16  suDenyReadCount;
   nuint16  suDenyWriteCount;
   nuint8   buLocked;
   nuint8   buForkCount;
   nuint16  suConnCount;
   NWNCPConnUsingFile aConnInfo[70];
} NWNCPFileUsed, N_FAR *pNWNCPFileUsed;

typedef struct tagNWNCPLimb
{
   nuint32  luLimbFlags;
   nuint32  luLimbVolNumber;
   nuint32  luLimbDirBase;
   nuint32  luLimbScanNumber;
   nuint32  luLimbNamSpc;
} NWNCPLimbNam, N_FAR *pNWNCPLimbNam;

typedef struct tagNWNCPSearchSeq
{
   nuint8   buVolNum;
   nuint32  luDirNum;
   nuint32  luEntryNum;
} NWNCPSearchSeq, N_FAR *pNWNCPSearchSeq;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( void )
NWNCPMakeACompPath
(
   nint     iPathlen,
   nptr     pInPath,
   nint     iNamSpc,
   pnuint8  pbuCompCnt,
   pnuint8  pabuCompPath,
   pnuint16 psuLen,
   nflag32  flOptions
);

N_EXTERN_LIBRARY( nuint16 )
NWNCPPackCompPath
(
   nint           iPathLen,
   pnstr          pstrPath,
   nint           iNamSpc,
   pNWNCPCompPath pCompPath,
   nflag32        flOptions

);

N_EXTERN_LIBRARY( nuint16 )
NWNCPPackCompPath2
(
   nint              iSrcPathLen,
   pnstr             pstrSrcPath,
   nint              iSrcNamSpc,
   nint              iDstPathLen,
   pnstr             pstrDstPath,
   nint              iDstNamSpc,
   pNWNCPCompPath2   pCompPath2,
   nflag32           flOptions
);

/***************************************************************************
   File function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP84OpenCreateFile
(
   pNWAccess         pAccess,
   nuint8           buDirHandle,
   nuint8           buFileAttrs,
   nuint8           buAccessFlags,
   nuint8           buActionCode,
   nuint8           buFileNameLen,
   pnstr8           pbstrFileName,
   pnuint8          pbuNWHandleB6,
   pNWNCPFileInfo   pFileInfo
);


N_EXTERN_LIBRARY( NWRCODE )
NWNCP59CommitFile
(
   pNWAccess         pAccess,
   pnuint8          pbuReservedB3,
   nuint32          luFileHandle
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP63ScanNext
(
   pNWAccess         pAccess,
   nuint8           buVolNum,
   nuint16          suDirID,
   nuint8           buSrchAttrs,
   nuint8           buSrchPathLen,
   pnstr8           pbstrSrchPath,
   pnuint16         psuIterHnd,
   pNWNCPSrchInfo   pSrchInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP62ScanFirst
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuVolNum,
   pnuint16 psuDirID,
   pnuint16 psuIterHnd,
   pnuint8  pbuAccessRights
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s2ScanFirst
(
   pNWAccess        pAccess,
   nuint8          buNamSpc,
   nuint8          buReserved,
   pNWNCPCompPath  pCompPath,
   pNWNCPSearchSeq pIterHnd
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s6GetEntryInfo
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDstNamSpc,
   nuint16           suSrchAttrs,
   nuint32           luRetMask,
   pNWNCPCompPath    pCompPath,
   pNWNCPEntryStruct pEntryStruct
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s1EntryOpenCreate
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buOpenCreateMode,
   nuint16           suSrchAttrs,
   nuint32           luRetMask,
   nuint32           luCreateAttrs,
   nuint16           suAccessRights,
   pNWNCPCompPath    pCompPath,
   pnuint8           pbuNWHandleB4,
   pnuint8           pbuOpenCreateAction,
   pnuint8           pbuReserved,
   pNWNCPEntryStruct pEntryStruct
);

/* Is not implemented by NWCalls */
N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s46RenameMoveEntry
(
   pNWAccess pAccess,
   nuint8   buSrchAttr,
   nuint8   buSrcDirHandle,
   nuint8   buSrcPathCompCnt,
   pnuint8  pabuSrcCompPath,
   nuint16  suSrcCompPathLen,
   nuint8   buDstDirHandle,
   nuint8   buDstPathCompCnt,
   pnuint8  pabuDstCompPath,
   nuint16  suDstCompPathLen
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s4RenameMoveEntry
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buRenameFlag,
   nuint16           suSrchAttrs,
   pNWNCPCompPath2   pCompPath2
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s3ScanNext
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDataStream,
   nuint16           suSrchAttrs,
   nuint32           luRetMask,
   pNWNCPSearchSeq   pIterHnd,
   nuint8            buSrchPatternLen,
   pnuint8           pbuSrchPattern,
   pnuint8           pbuReserved,
   pNWNCPEntryStruct pEntryStruct
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s20ScanNextSet
(
   pNWAccess  pAccess,
   nuint8    buNamSpc,
   nuint8    buDataStream,
   nuint16   suSrchAttrs,
   nuint32   luRetMask,
   nuint16   suRetInfoCount,
   pNWNCPSearchSeq pIterHnd,
   nuint8    buSrchPatternLen,
   pnuint8   pbuSrchPattern,
   pnuint8   pbuMoreEntriesFlag,
   pnuint16  psuInfoCount,
   pNWNCPEntryStruct pEntryStruct
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s8DelEntry
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved,
   nuint16        suSrchAttrs,
   pNWNCPCompPath pCompPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s31GetEntryInfo
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   pNWNCPEntryUnion  pEntryUnion
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s42GetEffRights
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint16 pRights
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s48GetEntryInfo
(
   pNWAccess          pAccess,
   nuint8            buVolNum,
   pnuint32          pluIterHnd,
   nuint8            buNamSpc,
   pNWNCPEntryUnion  pEntryUnion
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s49OpenDataStream
(
   pNWAccess pAccess,
   nuint8   buDataStream,
   nuint8   buDirHandle,
   nuint8   buFileAttrs,
   nuint8   buOpenRights,
   nuint8   buFileNameLen,
   pnstr8   pbuFileName,
   pnuint8  pbuNWHandleB4
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s30Scan
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   nuint8            buSrchPatternLen,
   pnuint8           pbuSrchPattern,
   pnuint32          pluIterHnd,
   pNWNCPEntryUnion  pEntryUnion
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s37SetEntryInfo
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   nuint32           luIterHnd,
   nuint32           luChangeBits,
   pNWNCPEntryUnion  pEntryUnion
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s7EntrySetDOSInfo
(
   pNWAccess             pAccess,
   nuint8               buNamSpc,
   nuint8               buReserved,
   nuint16              suSrchAttrs,
   nuint32              luModifyDOSMask,
   pNWNCPModifyDosInfo  pInfo,
   pNWNCPCompPath       pCompPath
);

/* File Functions */

N_EXTERN_LIBRARY( NWRCODE )
NWNCP78FileAllowTaskAccess
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuSrcNWHandleB6,
   pnuint8  pbuDstNWHandleB6
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP66FileClose
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP61FileCommit
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP74FileCopy
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuSrcNWHandleB6,
   pnuint8  pbuDstNWHandleB6,
   nuint32  luSrcOffset,
   nuint32  luDstOffset,
   nuint32  luBytesToCopy,
   pnuint32 pluActualBytesCopied
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP67FileCreate
(
   pNWAccess       pAccess,
   nuint8         buDirHandle,
   nuint8         buFileAttrs,
   nuint8         buFileNameLen,
   pnstr8         pbstrFileName,
   pnuint8        pbuNWHandleB6,
   pNWNCPFileInfo pFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP77FileCreate
(
   pNWAccess       pAccess,
   nuint8         buDirHandle,
   nuint8         buFileAttrs,
   nuint8         buFileNameLen,
   pnstr8         pbstrFileName,
   pnuint8        pbuNWHandleB6,
   pNWNCPFileInfo pFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP68FileErase
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buSrchAttrs,
   nuint8   buFileNameLen,
   pnstr8   pbstrInFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP71FileGetSize
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6,
   pnuint32 pluFileSize
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP85FileGetSparseBitMap
(
   pNWAccess pAccess,
   pnuint8  pbuNWHandleB6,
   nuint32  luStartingOffset,
   pnuint32 pluAllocationBlockSize,
   pnuint8  pbuReservedB4,
   pnuint8  pbuBitMapB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP65FileOpen
(
   pNWAccess       pAccess,
   nuint8         buDirHandle,
   nuint8         buSrchAttrs,
   nuint8         buFileNameLen,
   pnstr8         pbstrFileName,
   pnuint8        paNWHandleB6,
   pNWNCPFileInfo pFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP76FileOpen
(
   pNWAccess       pAccess,
   nuint8         buDirHandle,
   nuint8         buSrchAttrs,
   nuint8         buAccessRights,
   nuint8         buFileNameLen,
   pnstr8         pbstrFileName,
   pnuint8        pbuNWHandleB6,
   pNWNCPFileInfo pFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP72FileRead
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6,
   nuint32  luStartingOffset,
   nuint16  suBytesToRead,
   pnuint16 psuBytesActuallyRead,
   pnuint8  pbuDataB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP69FileRename
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buSrchAttrs,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName,
   nuint8   buDstDirHandle,
   nuint8   buNewFileNameLen,
   pnstr8   pbstrNewFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s15ScanFiles
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   pnuint16          psuIterHnd,
   nuint8            buFileNameLen,
   pnstr8            pbstrInFileName,
   pnstr8            pbstrOutFileNameB14,
   pNWNCPFileInfo2   pFileInfo2,
   pnuint8           pbuReservedB56
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP64ScanFiles
(
   pNWAccess       pAccess,
   nuint8         buDirHandle,
   nuint8         buSrchAttrs,
   nuint8         buFileNameLen,
   pnstr8         pbstrFileName,
   pnuint16       psuIterHnd,
   pNWNCPFileInfo pFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP70FileSetAttrs
(
   pNWAccess pAccess,
   nuint8   buNewFileAttr,
   nuint8   buDirHandle,
   nuint8   buSrchAttr,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP79FileSetExtAttr
(
   pNWAccess pAccess,
   nuint8   buNewExtAttrs,
   nuint8   buDirHandle,
   nuint8   buSrchAttrs,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s16FileSetInfo
(
   pNWAccess          pAccess,
   pNWNCPFileInfo2   pFileInfo2,
   pnuint8           pbuReservedB56,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   nuint8            buFileNameLen,
   pnstr8            pbstrFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP75FileSetTimeDate
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuFileHandleB6,
   nuint16  suForgedTime,
   nuint16  suForgedDate
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP73FileWrite
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuFileHandleB6,
   nuint32  luStartingByteOffset,
   nuint16  suBytesToWrite,
   pnuint8  pbuData
);

/* directory functions */

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s18AllocPermDirHandle
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buRequestedDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNewDirHandle,
   pnuint8  pbuRightsMask
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s12AllocDirHandle
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved1,
   nuint16        suAllocMode,
   pNWNCPCompPath pCompPath,
   pnuint8        pbuDirHandle,
   pnuint8        pbuVolNum,
   pnuint8        pbuReservedB4
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s22AllocSpecTDirHandle
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buRequestedDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNewDirHandle,
   pnuint8  pbuRightsMask
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s19AllocTempDirHandle
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buRequestedDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNewDirHandle,
   pnuint8  pbuRightsMask
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s244ConvertPathToEntry
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuVolNum,
   pnuint32 pluDirBase
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s10DirCreate
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buAccessMask,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s20FreeDirHandle
(
   pNWAccess pAccess,
   nuint8   buDirHandle
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s11DirDel
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buReserved,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s22GenDirBaseVolNum
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   pnuint8         pbuReservedB3,
   pNWNCPCompPath pCompPath,
   pnuint32       pluNSDirBase,
   pnuint32       pluDOSDirBase,
   pnuint8        pbuVolNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s35GetDirSpcRest
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   pnuint8           pbuNumEntries,
   pNWNCPDiskLvlRest pDiskRest
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s45GetDirInfo
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   pnuint32 pluTotalBlocks,
   pnuint32 pluAvailableBlocks,
   pnuint32 pluTotalDirEntries,
   pnuint32 pluAvailDirEntries,
   pnuint8  pbuReservedB4,
   pnuint8  pbuSectorsPerBlock,
   pnuint8  pbuVolNameLen,
   pnstr8   pbstrVolNameB16
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s1GetPath
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   pnuint8  pbuPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s3GetDirEffRights
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuEffRightsMask
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s29GetDirEffRights
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDstNamSpc,
   nuint16           suSrchAttrs,
   nuint32           luReturnInfoMask,
   pNWNCPCompPath    pCompPath,
   pnuint16          psuEffRights,
   pNWNCPEntryStruct pEntryStruct
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s28GetFullPath
(
   pNWAccess         pAccess,
   nuint8           buSrcNamSpc,
   nuint8           buDstNamSpc,
   pNWNCPCompPath   pCompPath,
   pNWNCPPathCookie pPathCookie,
   pnuint16         psuPathCompSize,
   pnuint16         psuPathCompCnt,
   pnuint8          pbuPathComponentsB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s26GetPathName
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint16  suDirBase,
   pnuint8  pbuPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s21GetDirHandlePath
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buDirHandle,
   pnuint8  pbuPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s243GetPathFromDirBase
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint8   buNamSpc,
   pnuint8  pbuPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s4DirModMaxRightsMask
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buRightsGrantMask,
   nuint8   buRightsRevokeMask,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s15RenameDir
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   nuint8   buNewPathLen,
   pnstr8   pbstrNewPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s40ScanDirDiskSpace
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   nuint8            buSrchPatternLen,
   pnuint8           pbuSrchPattern,
   pnuint32          pluIterHnd,
   pNWNCPEntryUnion  pEntryInfo,
   pnuint32          pluReservedB2
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s2ScanDirInfo
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint16 psuIterHnd,
   pnstr8   pbstrDirNameB16,
   pnuint16 psuCreationDate,
   pnuint16 psuCreationTime,
   pnuint32 pluOwnerTrusteeID,
   pnuint8  pbuAccessRightsMask,
   pnuint8  pbuReserved
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s36SetDirDiskSpcRest
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luDiskSpaceLimit
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s0SetDirHandle
(
   pNWAccess pAccess,
   nuint8   buTargetDirHandle,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNewDirHandle,
   pnuint8  pbuRightsMask,
   pnuint8  pbuReserved
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s25SetDirInfo
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint16  suCreationDate,
   nuint16  suCreationTime,
   nuint32  luOwnerID,
   nuint8   buMaxRightsMask,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s9SetDirHandle
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buDataStream,
   nuint8         buTargetDirHandle,
   nuint8         buReserved,
   pNWNCPCompPath pCompPath
);

/* delete/undelete functions */

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s206DelPurge
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s16DelPurge
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s29DelPurge
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luIterHnd
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s18DelPurge
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buReserved,
   nuint32  luIterHnd,
   nuint32  luVolNum,
   nuint32  luDirBase
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s17DelRecover
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathNameLen,
   pnstr8   pbstrPathName,
   pnstr8   pbstrOldFileNameB15,
   pnstr8   pbstrNewFileNameB15
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s28DelRecover
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luIterHnd,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s17DelRecover
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buReserved,
   nuint32  luIterHnd,
   nuint32  luVolNum,
   nuint32  luDirBase,
   nuint8   buNewFileNameLen,
   pnstr8   pbstrNewFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s27DelScan
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   pnuint32          pluIterHnd,
   pNWNCPDelEntryInfo pInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s16DelScan
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDataStream,
   nuint32           luRetMask,
   pNWNCPCompPath    pCompPath,
   pnuint32          pluIterHnd,
   pnuint16          psuDeleteTime,
   pnuint16          psuDeleteDate,
   pnuint32          pluDeletorID,
   pnuint32          pluVolNum,
   pnuint32          pluDirBase,
   pNWNCPEntryStruct   pEntryStruct
);

/* Volume Functions */

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s33VolAddRestrict
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32      luObjID,
   nuint32  luSpaceLimit
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s41VolGetRestrict
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32      luObjID,
   pnuint32 pluRestriction,
   pnuint32 pluInUse
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s21VolGetInfo
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   pnuint16 psuSectorsPerCluster,
   pnuint16 psuTotalVolClusters,
   pnuint16 psuAvailClusters,
   pnuint16 psuTotalDirSlots,
   pnuint16 psuAvailDirSlots,
   pnstr8   pbstrVolNameB16,
   pnuint16 psuRemovableFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP18VolGetInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint16 psuSectorsPerCluster,
   pnuint16 psuTotalVolClusters,
   pnuint16 psuAvailClusters,
   pnuint16 psuTotalDirSlots,
   pnuint16 psuAvailDirSlots,
   pnstr8   pbstrVolNameB16,
   pnuint16 psuRemovableFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s44VolGetPurgeInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint32 pluTotalBlocks,
   pnuint32 pluFreeBlocks,
   pnuint32 pluPurgeableBlocks,
   pnuint32 pluNotYetPurgeableBlocks,
   pnuint32 pluTotalDirEntries,
   pnuint32 pluAvailDirEntries,
   pnuint8  pbuReservedB4,
   pnuint8  pbuSectorsPerBlock,
   pnuint8  pbuVolNameLen,
   pnstr8   pbstrVolName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s6VolGetName
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint8  pbuVolNameLen,
   pnstr8   pbstrVolName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s5VolGetNum
(
   pNWAccess pAccess,
   nuint8   buVolNameLen,
   pnstr8   pbstrVolName,
   pnuint8  pbuVolNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s34VolRemoveRestrict
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32      luObjID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s32VolScanRestrict
(
   pNWAccess             pAccess,
   nuint8               buVolNum,
   nuint32              luIterHnd,
   pnuint8              pbuNumEntries,
   pNWNCPRestrictions   pRestrictionsB12
);

/* trustees functions */

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s39TrusteeAddExt
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luObjectID,
   nuint16  suTrusteeRights,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s13TrusteeAddToDir
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luTrusteeID,
   nuint8   buTrusteeAccessMask,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s10TrusteeAddSet
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved,
   nuint16        suSrchAttrs,
   nuint16        suTrusteeRightsMask,
   nuint16        suTrusteeCnt,
   pNWNCPCompPath pCompPath,
   pNWNCPTrustees pTrustees
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s14TrusteeDelDir
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luTrusteeID,
   nuint8   buReserved,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s11TrusteeDelSet
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved,
   nuint16        suObjIDCnt,
   pNWNCPCompPath pCompPath,
   pNWNCPTrustees pTrustees
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s43TrusteeRemoveExt
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32      luObjID,
   nuint8   buReserved,
   nuint8   buPathLen,
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s12TrusteesScanDir
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buSetNum,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnstr8   pbstrDirNameB16,
   pnuint16 psuCreationDate,
   pnuint16 psuCreationTime,
   pnuint32 pluOwnerID,
   pnuint32 pluTrusteeIDSetB5,
   pnuint8  pbuTrusteeAccessMasksB5
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s38TrusteesScanExt
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buSetNum,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNumEntries,
   pnuint32 pluObjIDB20,
   pnuint16 psuTrusteeRightsB20
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s5TrusteesScan
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved,
   nuint16        suSrchAttrs,
   pNWNCPCompPath pCompPath,
   pnuint32       pluIterHnd,
   pnuint16       psuObjIDCnt,
   pNWNCPTrustees pTrusteesB20
);

/* name space functions */

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s47NSGetInfo
(
   pNWAccess       pAccess,
   nuint8         buVolNum,
   pnuint8        pbuNumDefinedNS,
   pNWNCPNSNames  pNSNamesBX,
   pnuint8        pbuNumDefinedDS,
   pNWNCPDSNames  pDSNamesBX,
   pnuint8        pbuNumLoadedNS,
   pnuint8        pbuLoadedIndexNums,
   pnuint8        pbuVolsNS,
   pnuint8        pbuVolNSIndexNums,
   pnuint8        pbuVolsDS,
   pnuint8        pbuVolDSIndexNums
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s19NSGetInfo
(
   pNWAccess pAccess,
   nuint8   buSrcNamSpc,
   nuint8   buDstNamSpc,
   nuint8   buReserved,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint32  luNSInfoBitMask,
   pnuint8  pabuNSSpecificInfoB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s25NSSetInfo
(
   pNWAccess pAccess,
   nuint8   buSrcNamSpc,
   nuint8   buDstNamSpc,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint32  luNSInfoBitMask,
   pnuint8  pNSInfoB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s24NSGetLoadedList
(
   pNWAccess pAccess,
   nuint16  suReserved,
   nuint8   buVolNum,
   pnuint16 psuNumNSLoaded,
   pnuint8  pbuNSLoadedList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s23NSQueryInfoFormat
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buVolNum,
   pnuint32 pluFixedBitMask,
   pnuint32 pluVariableBitMask,
   pnuint32 pluHugeBitMask,
   pnuint16 psuFixedBitsDefined,
   pnuint16 psuVariableBitsDefined,
   pnuint16 psuHugeBitsDefined,
   pnuint32 pluFieldsLenTableB32
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s26NSGetHugeInfo
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint32  luHugeMask,
   pnuint8  pbuHugeStateInfoB16,
   pnuint8  pbuNxtHugeStateInfoB16,
   pnuint32 pluHugeDataLen,
   pnuint8  pbuHugeDataB
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s27NSSetHugeInfo
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint32  luHugeMask,
   pnuint8  pbuHugeStateInfoB16,
   nuint32  luHugeDataLen,
   pnuint8  pbuHugeDataB512,
   pnuint8  pbuNextHugeStateInfoB16,
   pnuint32 pHugeDataUsed
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP87s30OpenCreateFileOrDir
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buDataStream,
   nuint8   buOpenCreateMode,
   nuint8   buReserved,
   nuint16  suSrchAttr,
   nuint16  suReserved2,
   nuint32  luRetInfoMask,
   nuint32  luCreateAttr,
   nuint16  suDesiredRights,
   pNWNCPCompPath pCompPath,
   pnuint8  pbuNWFileHandleB4,
   pnuint8  pbuOpenCreateAction,
   pnuint8  pbuReserved3,
   pNWNCPEntryStruct pEntryStruct
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s23SaveBaseHandle
(
   pNWAccess    pAccess,
   nuint8      buDirHandle,
   pnuint8     pbuSaveBufferB14
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP22s24RestoreBaseHandle
(
   pNWAccess pAccess,
   pnuint8  pbuSaveBuffer14,
   pnuint8  pbuNewDirHandle,
   pnuint8  pbuRightsMask
);

N_EXTERN_LIBRARY( void )
NWNCPUnpackEntryStruct
(
   pNWNCPEntryStruct pEntryStruct,
   pnuint8           pbuDataB77,
   nuint32           luRetMask
);

N_EXTERN_LIBRARY( void )
NWNCPUnpackEntryUnion
(
   pNWNCPEntryUnion  pEntryUnion,
   pnuint8           pbuDataB128,
   nint              iNcpNum
);

N_EXTERN_LIBRARY( void )
NWNCPPackEntryUnion
(
   pnuint8           pbuDataB128,
   pNWNCPEntryUnion  pEntryUnion,
   nint              iNcpNum
);

typedef struct tagNWNCPSMInfo0
{
   nuint32 luIOStatus;
   nuint32 luInfoBlockSize;
   nuint32 luAvailSpace;
   nuint32 luUsedSpace;
   nuint8  abuSMInfo[128]; /* 128 length limit, length preceded, Info block follows string */
} NWNCPSMInfo0, N_FAR * pNWNCPSMInfo0;

typedef struct tagNWNCPSMInfo1
{
   nuint32  luNumberofSMs;
   nuint32  abuSMIDs[32];
} NWNCPSMInfo1, N_FAR * pNWNCPSMInfo1;

typedef struct tagNWNCPSMInfo2
{
   nuint8   buNameLength; /* sm name*/
   nuint8   abuName[128];
} NWNCPSMInfo2, N_FAR * pNWNCPSMInfo2;

typedef struct tagNWNCPDMInfo
{
   nuint32  luDataStrmNum;
   nuint32  luDataSize;
} NWNCPDMInfo, N_FAR *pNWNCPDMInfo;

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s128MovDataToDM
(
   pNWAccess pAccess,
   nuint32  luVol,
   nuint32  luDirEntry,
   nuint32  luNameSpace,
   nuint32  luSupportModuleID,
   nuint32  luDMFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s133MovDataFromDM
(
   pNWAccess pAccess,
   nuint32  luVol,
   nuint32  luDirEntry,
   nuint32  luNameSpace
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s129DMFileInfo
(
   pNWAccess pAccess,
   nuint32  luVolNum,
   nuint32  luDirBase,
   nuint32  luNamSpc,
   pnuint32 pluModuleID,
   pnuint32 pluRestoreTime,
   pnuint32 pluDataStreams
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s130GetVolDMStatus
(
   pNWAccess pAccess,
   nuint32  luVol,
   nuint32  luSupportModuleID,
   pnuint32 pluFilesMigrated,
   pnuint32 pluTotalMigratedSize,
   pnuint32 pluSpaceUsedOnDM,
   pnuint32 pluLimboSpaceUsedOnDM,
   pnuint32 pluSpaceMigrated,
   pnuint32 pluFilesInLimbo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s131MigratStatusInfo
(
   pNWAccess pAccess,
   pnuint32 pluDMPresent,
   pnuint32 pluDMMajorVer,
   pnuint32 pluDMMinorVer,
   pnuint32 pluDMSMRegistered
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s134GetSetVolDMStatus
(
   pNWAccess pAccess,
   nuint32  luGetSetFlag,
   pnuint32 pluSupportModuleID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s132GetDMSupportModInfo
(
   pNWAccess pAccess,
   nuint32    luInfoLevel,
    nuint32    luSupportModuleID,
    pnuint8    pbuInfoBuf,
    pnuint32   pluInfoBufSize
);

N_EXTERN_LIBRARY( void )
NWNCPUnpackFileInfo
(
   pNWNCPFileInfo  pFileInfo,
   pnuint8         pbuDataB30
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s0ParseTree
(
   pNWAccess    pAccess,
   nuint32     luInfoMask,
   nuint32     luInfoMask2,
   nuint32     luReserved,
   nuint32     luLimbCount
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s10GetRefCntFromDirNum
(
   pNWAccess    pAccess,
   nuint32     luVolume,
   nuint32     luDirBase,
   nuint32     luNamSpc,
   pnuint32    pluRefCount
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s11GetRefCntFromDirHnd
(
   pNWAccess    pAccess,
   nuint32     luDirHandle,
   pnuint32    pluRefCount
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s12SetCompFileSize
(
   pNWAccess    pAccess,
   pnuint8     pbuHandleB6,
   nuint32     luFileSize,
   pnuint32    pluOldFileSize,
   pnuint32    pluNewFileSize
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP90s150FileMigrationReq
(
   pNWAccess    pAccess,
   nuint32     luVol,
   nuint32     luDOSDirEntry,
   nuint32     luFileMigrState
);

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
}
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpfile.h,v 1.8 1994/09/26 17:11:19 rebekah Exp $
*/
