/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwundoc.h	1.3"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef UNDOC_INC
#define UNDOC_INC

#ifndef NWCALDEF_INC
# include <nwcaldef.h>
#endif

#include <npackon.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(USE_CRTL)
# include <string.h>
#endif

#define EMPTY_LIST   1
#define NOT_IN_LIST  2

typedef struct
{
  nuint32 dataStreamSpaceAlloc;
  nuint8  attributes[4];
  nuint16 flags;
  nuint32 dataStreamSize;
  nuint32 ttlDSDskSpaceAlloc;
  nuint16 numberOfDataStreams;
  nuint16 creationTime;
  nuint16 creationDate;
  nuint32 creatorID;
  nuint16 modifiedTime;
  nuint16 modifiedDate;
  nuint32 modifierID;
  nuint16 lastAccessDate;
  nuint16 archivedTime;
  nuint16 archivedDate;
  nuint32 archiverID;
  nuint16 inheritedRightsMask;
  nuint32 directoryEntryNumber;
  nuint32 dosDirectoryEntryNumber;
  nuint32 VolumeNumber;
  nuint32 extendedAttributeSize;
  nuint32 extendedAttributeCount;
  nuint32 extendedAttributeKeySize;
  nuint32 creatorNameSpaceNumber;
} NIS;

typedef struct
{
  nuint16 fileAttributes;
  nuint8  fileMode;
  nuint8  extendedAttributes;
  nuint16 creationDate;
  nuint16 creationTime;
  nuint32 creatorID;
  nuint16 modifiedDate;
  nuint16 modifiedTime;
  nuint32 modifierID;
  nuint16 archiveDate;
  nuint16 archiveTime;
  nuint32 archiverID;
  nuint16 lastAccessDate;
  nuint16 grantMask;
  nuint16 revokeMask;
  nuint32 allocatedSpace;
} MDIS;

typedef struct
{
  nuint8  volumeNumber;
  nuint32 dirBase;
  nuint8  handleFlag;
  nuint8  componentCount;
  nuint8  componentPath[300];
} CompPath, COMPPATH;

typedef struct
{
  nuint8  srcVolNum;
  nuint32 srcDirBase;
  nuint8  srcHandleFlag;
  nuint8  srcCompCount;
  nuint8  destVolNum;
  nuint32 destDirBase;
  nuint8  destHandleFlag;
  nuint8  destCompCount;
  nuint8  componentPath[520];  /* What's left of MAX packet size */
} PHSTRUCT;

N_EXTERN_LIBRARY( NWCCODE )
NWGetRequesterStats
(
   pnuint8 statsBuffer
);

N_EXTERN_LIBRARY( NWCCODE )
NWResetRequesterStats
(
   void
);

N_EXTERN_LIBRARY( NWCCODE )
NWGetFileServerType
(
   NWCONN_HANDLE conn,
   pnuint16 suServerType
);

N_EXTERN_LIBRARY( NWCCODE )
CheckPathAtRoot
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   pnstr8 pbstrPath
);

/* the following calls are not officially documented */
N_EXTERN_LIBRARY( nuint16 )
__NWGetNumberDrives
(
   void
);

N_EXTERN_LIBRARY( nuint16 )
__NWGetDriveType
(
   nuint16 suDriveNum
);

N_EXTERN_LIBRARY( nuint16 )
__NWGetCurrentDrive
(
   void
);

N_EXTERN_LIBRARY( nuint16 )
__NWGetCurrentDirectory
(
   nuint16 suDriveNum,
   pnstr8 pbstrDrivePath
);

#ifndef WIN32
N_EXTERN_LIBRARY( NWCCODE )
__NWGetNDriveStatus
(
   nuint16  suDriveNum,
   pnuint16 psuStatus,
   pnstr8   pbstrDrivePath,
   pnstr8   pbstrRelPath);
#endif

#if defined(WIN32) || defined(USE_CRTL) || defined(N_USE_CRT)

#define _LMemCpy(dest, src, len)   \
        memmove(dest, src, (size_t) len)

#define _LMemSet(dest, value, len) \
        memset(dest, (int) value, (size_t) len)

#define _LSetMem(dest, len, value) \
        memset(dest, (int) value, (size_t) len)

#define _LMovMem(src, dest, len) \
        memmove(dest, src, (size_t) len)

#define _LStrCpy(dest, src) \
        strcpy((char *)dest, (const char *)src)

#define _LMemCmp(s1, s2, len) \
        memcmp(s1, s2, (size_t) len)

#define _LStrLen(string) \
        strlen((const char *)string)

#define _LStrCat(dest, src) \
        strcat((char *)dest, (const char *)src)

#else
#define _LMemCpy(dest, source, len)   \
              _LMovMem((nptr )(source), (nptr )(dest), len)

#define _LMemSet(dest, value, len) \
              _LSetMem((nptr )(dest), len, value)

nint16 N_API _LMemCmp(const nptr s1, const nptr s2, nuint16 len);
void N_API _LMovMem(const nptr src, nptr dest, nuint16 len);
void N_API _LSetMem(nptr dest, nuint16 len, nuint16 value);
nuint16 N_API _LStrLen(const nptr string);
void N_API _LStrCat(nptr dest, const nptr src);
void N_API _LStrCpy(nptr dest, const nptr src);
#endif

N_EXTERN_LIBRARY( nuint16 )
_NWGetCompathStructLength
(
   pnuint8 pCompPath
);

N_EXTERN_LIBRARY( nuint16 )
_NWGetComPathLen2
(
   PHSTRUCT N_FAR * pLen
);

#define NW_MAX_OS2_NAME_LENGTH 260 + 16
#define NW_MAX_BUFFER_LENGTH   512
#define NW_HANDLE_PATH_OFFSET  307 /* size of NWHandlePath struct*/

N_EXTERN_LIBRARY( void )
_NWFillComponentPath2
(
   PHSTRUCT N_FAR * cPath,
   pnuint8  pbuSrcPath,
   pnuint8  pbuDstPath
);

N_EXTERN_LIBRARY( void )
__NWFillComponentPath2
(
   PHSTRUCT N_FAR * cPath,
   pnuint8  pbuSrcPath,
   pnuint8  pbuDstPath,
   nuint16  suAugmentFlag
);

N_EXTERN_LIBRARY( void )
_NWFillHandlePathStruct
(
   pnuint8  pbuPath,
   NWDIR_HANDLE dirHandle,
   pnstr8   pbstrDirPath
);

N_EXTERN_LIBRARY( void )
__NWFillHandlePathStruct
(
   COMPPATH N_FAR * cPath,
   NWDIR_HANDLE dirHandle,
   pnstr8   pbstrDirPath,
   nuint16  suAugmentFlag
);

N_EXTERN_LIBRARY( void )
_NWFillHandlePathStruct2
(
   PHSTRUCT N_FAR * cPath,
   NWDIR_HANDLE srcDirHandle,
   pnuint8  pbuSrcPath,
   NWDIR_HANDLE dstDirHandle,
   pnuint8  pbuDstPath
);

N_EXTERN_LIBRARY( void )
__NWFillHandlePathStruct2
(
   PHSTRUCT N_FAR * cPath,
   NWDIR_HANDLE sourceDirHandle,
   pnuint8  pbuSrcPath,
   NWDIR_HANDLE destDirHandle,
   pnuint8  pbuDstPath,
   nuint16  suAugmentFlag
);

N_EXTERN_LIBRARY( void )
_NWFillWildPath
(
   pnuint8  pbuPath,
   pnstr8   pbstrDirPath
);

N_EXTERN_LIBRARY( void )
__NWFillWildPath
(
   pnuint8  pbuPath,
   pnstr8   pbstrDirPath,
   nuint16  suAugmentFlag
);

N_EXTERN_LIBRARY( NWCCODE )
WildCardCheck
(
   pnstr8  pbstrDirPath
);

N_EXTERN_LIBRARY( void )
_NWFillComponentPath
(
   pnuint8  pbuPacketPtr,
   pnstr8   pbstrDirPath
);

N_EXTERN_LIBRARY( void )
_NWMakeComponentPath
(
   pnuint8 pbuCompCnt,
   pnuint8 pbuCompPath,
   pnstr8  pbstrPath,
   nuint16 suAugmentFlag
);

#define StripServerOffPath(p, s) NWStripServerOffPath(p, s)

#ifndef NW_FRAGMENT_DEFINED
#define NW_FRAGMENT_DEFINED
typedef struct
{
  nptr fragAddress;
#if defined(NWNLM) || defined(WIN32)
  nuint32 fragSize;
#else
   nuint16 fragSize;
#endif
} NW_FRAGMENT;
#endif

N_EXTERN_LIBRARY( NWCCODE )
NWOrderedRequestToAll
(
   nuint16 suLockReqCode,
   nuint16 suReqCode,
   nuint16 suReqFragCount,
   NW_FRAGMENT N_FAR * reqFragList,
   nuint16 suErrorReqCode,
   nuint16 suErrorFragCount,
   NW_FRAGMENT N_FAR * errorFragList
);

N_EXTERN_LIBRARY( NWCCODE )
NWRequestToAll
(
   nuint16 suReqCode,
   nuint16 suReqFragCount,
   NW_FRAGMENT N_FAR * reqFragList
);

#if defined (N_PLAT_OS2) || defined (NWOS2)

N_EXTERN_LIBRARY( NWCCODE )
NWCall
(
   nuint16 suReqCode,
   nptr    pParamBuf
);

N_EXTERN_LIBRARY( NWCCODE )
NWLockRequest
(
   NWCONN_HANDLE conn,
   nuint16 suReqCode,
   nuint16 suReqFragCount,
   NW_FRAGMENT N_FAR * reqFrags,
   nuint16 suReplyFragCount,
   NW_FRAGMENT N_FAR * replyFrags
);

N_EXTERN_LIBRARY( NWCCODE )
NWToggleLNS
(
   NWCONN_HANDLE conn,
   nuint16 suOnOffFlagForConn
);

/* undocumented exported routines in the CONNECT module */

typedef struct _LOCK_SEM_NODE
{
  nint32   ramSem;
  nuint16  pid;
  struct _LOCK_SEM_NODE N_FAR * prev;
  struct _LOCK_SEM_NODE N_FAR * next;
} LOCK_SEM_NODE;


/* This is the our cds structure used in the FS in version 1.2 of OS/2 -- rfw.*/

typedef struct _cds_info
{
  NWCONN_HANDLE connID;
  nint8   directoryHandle;
  nint8   serverType;
  nint16  connectionSegment;
  nint16  reserved;
  pnstr8  currentDirectory;
} CDD_INFO;

typedef struct _sfd_info
{
  char netwareFileHandle[6];
  NWCONN_HANDLE connID;
  nint16  directoryNumber;
  nint16  connectionSegment;
  nint16  fileAttributes;
  nint16  taskID;
  nint16  redirDeviceType;
  nint8   reserved[12];
} SFD_INFO;

typedef struct ATTACH_PARMS
{
  nuint16 initDriveFlag;
  NWCONN_HANDLE  connID;
  nuint8  dHandle;
  nint8   mapPath[260];
} FS_PARMS;

typedef struct _dv_type
{
   NWCONN_HANDLE connID;
   nint8  directoryHandle;
} DRIVE_TYPE;

typedef struct tagCONNECTION_TABLE
{
  nuint8  serverUsed;
  nuint8  orderNumber;
  nuint8  serverNetwork[4];
  nuint8  serverNode[6];
  nuint8  serverSocket[2];
  nuint16 receiveTimeout;
  nuint8  ImmediateNode[6];
  nuint8  sequenceNumber;
  nuint8  connNumber;
  nuint8  connectionOK;
  nuint16 maximumTimeout;
} CONNECTION_TABLE;

N_EXTERN_LIBRARY( NWCCODE )
NWSetConnectionStatus
(
   NWCONN_HANDLE conn,
   nuint16 suStatusANDmask,
   nuint16 suStatusORmask
);

N_EXTERN_LIBRARY( NWCCODE )
NWGetProcSessionID
(
   pnuint16 psuCurrProcessSessionID
);

N_EXTERN_LIBRARY( NWCCODE )
NWGetSFNSessionID
(
   nuint16,
   pnuint16
);

N_EXTERN_LIBRARY( NWCCODE )
NWClearLockSemList
(
   nuint8
);

N_EXTERN_LIBRARY( NWCCODE )
NWGetHead
(
   nuint8,
   LOCK_SEM_NODE N_FAR * N_FAR *
);

N_EXTERN_LIBRARY( NWCCODE )
NWGetTail
(
   nuint8 buLockType,
   LOCK_SEM_NODE N_FAR * N_FAR * pTail,
   pnuint32 N_FAR * pluLockSem
);

N_EXTERN_LIBRARY( NWCCODE )
NWGetConnectionTable
(
   NWCONN_HANDLE conn,
   nuint16 suConnTableLen,
   CONNECTION_TABLE N_FAR * connTable
);

N_EXTERN_LIBRARY( NWCCODE )
NWAddLockRequestToList
(
   nuint8,
   LOCK_SEM_NODE N_FAR *  N_FAR *
);

N_EXTERN_LIBRARY( NWCCODE )
NWRemoveLockRequestFromList
(
   nuint8,
   LOCK_SEM_NODE N_FAR *
);

#else

#ifndef WIN32

typedef struct CONNECTIONADDRESS
{
  nuint8 inUse;             /* app fills in before attach */
  nuint8 serverOrder;       /* app fills in before attach */
  nuint8 netAddress[12];    /* app fills in before attach */
  nuint8 receiveTimeOut[2];
  nuint8 routerNode[6];
  nuint8 packetNumber;
  nuint8 connNumber;
  nuint8 connectionOK;
  nuint8 maximumTimeOut[2];
  NWCONN_NUM wordconnNumber;
  nuint8 serverFlag;
  nuint8 filler[2];
} CONNECTIONADDRESS;

typedef struct CONNECTIONNAME
{
  char name[48];
} CONNECTIONNAME;

#ifndef _REGISTERS_DEF
#define _REGISTERS_DEF

typedef struct
{
  nuint16 si;
  nuint16 ds;
  nuint16 di;
  nuint16 es;
  nuint8  al, ah;
  nuint8  bl, bh;
  nuint8  cl, ch;
  nuint8  dl, dh;
} BYTE_REGISTERS;

typedef struct
{
  nuint16 si;
  nuint16 ds;
  nuint16 di;
  nuint16 es;
  nuint16 ax;
  nuint16 bx;
  nuint16 cx;
  nuint16 dx;
  nuint16 bp;
  nuint16 flags;
} WORD_REGISTERS;

typedef struct
{
  nptr requestBuffer;
  nptr replyBuffer;
} PTR_REGISTERS;

typedef struct
{
  nptr ds_si;
  nptr es_di;
} SEG_OFF_REGISTERS;

typedef union
{
  WORD_REGISTERS w;
  BYTE_REGISTERS b;
  PTR_REGISTERS  p;
  SEG_OFF_REGISTERS s;
} REGISTERS;
#endif

#ifndef USE_DS
#define USE_DS  1
#define USE_ES  2
#define USE_DOS 0x80
#endif

/* these are for internal use only. DO NOT USE THEM. */
extern nuint16 _NWShellIsLoaded;
extern nuint16 _NWVLMIsLoaded;

N_EXTERN_LIBRARY( nint )
NWShellRequest
(
   REGISTERS N_FAR *,
   nuint16
);

N_EXTERN_LIBRARY( nuint16 )
_NWInitVLM
(
   void
);

N_EXTERN_LIBRARY( nuint16 )
NWVLMRequest
(
   nuint16 suCallerID,
   nuint16 suDestID,
   nuint16 suDestFunc,
   REGISTERS N_FAR * regs,
   nuint16 suMask
);

#endif
#endif

#ifdef __cplusplus
}
#endif

#include <npackoff.h>

#endif

