/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwclient.h	1.6"
/*--------------------------------------------------------------------------
   Copyright (c) 1993 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWCLIENT_H
#define NWCLIENT_H

#ifndef NTYPES_H
#ifdef N_PLAT_UNIX
# include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
# include <ntypes.h>
#endif /* N_PLAT_UNIX */
#endif

#ifndef NWACCESS_H
#ifdef N_PLAT_UNIX
# include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
# include <nwaccess.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#ifndef _UNICODE_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/unicode.h>
#else
#include <unicode.h>
#endif /* N_PLAT_UNIX */
#endif /* _UNICODE_HEADER_ */


/* Maximum defines */
#define NWC_MAX_USER_NAME_LEN          48
#define NWC_MAX_SERVER_NAME_LEN        48
#define NWC_MAX_TREE_NAME_CHARS        32
#define NWC_MAX_TRAN_ADDR_LEN          30
#define NWC_MAX_SERVICE_TYPE_LEN       28
#define NWC_MAX_WORKGROUP_NAME_LEN     16
#define NWC_MAX_PNW_USER_NAME_LEN      16
#define NWC_MAX_NET_ADDR_LEN           128

#define MAX_JOBDESCR_LEN               50
#define MAX_FORM_NAME_LEN              13
#define MAX_BANNER_NAME_LEN            13
#define MAX_QUEUE_NAME_LEN             65

/* Name Format Type (nuint value) */
#define NWC_NAME_FORMAT_NDS            0x0001
#define NWC_NAME_FORMAT_BIND           0x0002
#define NWC_NAME_FORMAT_BDP            0x0004
#define NWC_NAME_FORMAT_NDS_TREE       0x0008
#define NWC_NAME_FORMAT_WILD           0x8000

/* String types - (nuint value) */
#define NWC_STRING_TYPE_ASCII          0x0001
#define NWC_STRING_TYPE_UNICODE        0x0002

/* Transport Type - (nuint value) */
#define NWC_TRAN_TYPE_IPX              0x0001
#define NWC_TRAN_TYPE_UDP              0x0002
#define NWC_TRAN_TYPE_DDP              0x0003
#define NWC_TRAN_TYPE_ASP              0x0004
#define NWC_TRAN_TYPE_WILD             0x8000

/* Open connection flags - (nuint value) */
#define NWC_OPEN_LICENSED              0x0001
#define NWC_OPEN_UNLICENSED            0x0002
#define NWC_OPEN_PRIVATE               0x0004
#define NWC_OPEN_PUBLIC                0x0008
#define NWC_OPEN_EXISTING_HANDLE       0x0010

/* Connection Info Levels (nuint value) */
#define NWC_CONN_INFO_INFO_VERSION     0x0001
#define NWC_CONN_INFO_AUTH_STATE       0x0002
#define NWC_CONN_INFO_BCAST_STATE      0x0003
#define NWC_CONN_INFO_CONN_REF         0x0004
#define NWC_CONN_INFO_TREE_NAME        0x0005
#define NWC_CONN_INFO_WORKGROUP_ID     0x0006
#define NWC_CONN_INFO_SECURITY_STATE   0x0007
#define NWC_CONN_INFO_CONN_NUMBER      0x0008
#define NWC_CONN_INFO_USER_ID          0x0009
#define NWC_CONN_INFO_SERVER_NAME      0x000A
#define NWC_CONN_INFO_TRAN_ADDR        0x000B
#define NWC_CONN_INFO_NDS_STATE        0x000C
#define NWC_CONN_INFO_MAX_PACKET_SIZE  0x000D
#define NWC_CONN_INFO_LICENSE_STATE    0x000E
#define NWC_CONN_INFO_PUBLIC_STATE     0x000F
#define NWC_CONN_INFO_SERVICE_TYPE     0x0010
#define NWC_CONN_INFO_DISTANCE         0x0011
#define NWC_CONN_INFO_RETURN_ALL       0xFFFF

/* Information verions (nuint value) */
#define NWC_INFO_VERSION_1             0x0001

/* Authentication states (nuint value) */
#define NWC_AUTH_STATE_NONE            0x0000
#define NWC_AUTH_STATE_BINDERY         0x0001
#define NWC_AUTH_STATE_NDS             0x0002
#define NWC_AUTH_STATE_PNW             0x0003

/* Broadcast states (nuint value) */
#define NWC_BCAST_PERMIT_ALL           0x0000
#define NWC_BCAST_PERMIT_SYSTEM        0x0001
#define NWC_BCAST_PERMIT_NONE          0x0002

/* Security states (nuint32 value) */
#define NWC_SECURITY_SIGNING_NOT_IN_USE   0x00000000
#define NWC_SECURITY_SIGNING_IN_USE       0x00000001
#define NWC_SECURITY_LEVEL_CHECKSUM       0x00000100
#define NWC_SECURITY_LEVEL_SIGN_HEADERS   0x00000200
#define NWC_SECURITY_LEVEL_SIGN_ALL       0x00000400
#define NWC_SECURITY_LEVEL_ENCRYPT        0x00000800

/* NDS states (nuint value) */
#define NWC_NDS_NOT_CAPABLE            0x0000
#define NWC_NDS_CAPABLE                0x0001

/* License states (nuint value) */
#define NWC_NOT_LICENSED               0x0000
#define NWC_CONNECTION_LICENSED        0x0001
#define NWC_HANDLE_LICENSED            0x0002

/* Public states (nuint value) */
#define NWC_CONN_PUBLIC                0x0000
#define NWC_CONN_PRIVATE               0x0001

/* Scan connection information flags (nuint value) */
#define NWC_MATCH_NOT_EQUALS           0x0000
#define NWC_MATCH_EQUALS               0x0001
#define NWC_RETURN_PUBLIC              0x0002
#define NWC_RETURN_PRIVATE             0x0004
#define NWC_RETURN_LICENSED            0x0008
#define NWC_RETURN_UNLICENSED          0x0010

/* Authentication types */
#define NWC_AUTHENT_BIND               0x0001
#define NWC_AUTHENT_NDS                0x0002
#define NWC_AUTHENT_PNW                0x0003

/* Tagged data store flags */
#define NWC_TDS_PRE_ZERO               0x0001
#define NWC_TDS_POST_ZERO              0x0002
#define NWC_TDS_ENCRYPT                0x0004

/* Drive status flags */
#define NWC_MAP_NOT_MAPPED             0x00000000
#define NWC_MAP_LOCAL                  0x00000001
#define NWC_MAP_NETWORK                0x00000002
#define NWC_MAP_NETWARE                0x00000004
#define NWC_MAP_PNW                    0x00000008

/* Access rights attributes */
#ifndef NWC_AR_READ_ONLY
#define NWC_AR_READ              0x0001
#define NWC_AR_WRITE             0x0002
#define NWC_AR_READ_ONLY         0x0001
#define NWC_AR_WRITE_ONLY        0x0002
#define NWC_AR_DENY_READ         0x0004
#define NWC_AR_DENY_WRITE        0x0008
#define NWC_AR_COMPATIBILITY     0x0010
#define NWC_AR_WRITE_THROUGH     0x0040
#define NWC_AR_OPEN_COMPRESSED   0x0100
#endif

#define NWC_CONN_HANDLE     nuint
#define NWC_FILE_HANDLE     nuint


typedef struct tagNWCTranAddr
{
   nuint    uType;
   nuint    uLen;
   pnuint8  pbuBuffer;
} NWCTranAddr, N_FAR *pNWCTranAddr;

typedef struct tagNWCFrag
{
   nptr  pAddr;
   nuint uLen;
} NWCFrag, N_FAR *pNWCFrag;

typedef struct tagNWCConnInfo
{
   nuint          uInfoVersion;
   nuint          uAuthenticationState;
   nuint          uBroadcastState;
   nuint32        luConnectionReference;
   pnstr          pstrTreeName;
   pnstr          pstrWorkGroupId;
   nuint32        luSecurityState;
   nuint          uConnectionNumber;
   nuint32        luUserId;
   pnstr          pstrServerName;
   pNWCTranAddr   pTranAddr;
   nuint          uNdsState;
   nuint          uMaxPacketSize;
   nuint          uLicenseState;
   nuint          uPublicState;
   pnstr          pstrServiceType;
   nuint          uDistance;
} NWCConnInfo, N_FAR *pNWCConnInfo;

typedef struct tagNWCConnString
{
   nptr  pString;
   nuint uStringType;
   nuint uNameFormatType;
} NWCConnString, N_FAR *pNWCConnString;


typedef struct tagNWCAuthenInfo
{
   struct
   {
      nuint32  luObjectType;
   } BinderyInfo;

   struct
   {
      nstr     pstrTreeName[NWC_MAX_TREE_NAME_CHARS];
   } DSInfo;

   struct
   {
      nuint32 luObjectType;
   } PNWInfo;
} NWCAuthenInfo, N_FAR *pNWCAuthenInfo;

typedef  struct tagNWCCaptureRWFlag
{
   nuint8   ubJobDescr[MAX_JOBDESCR_LEN];
   nuint8   ubTabSize;
   nuint16  uiNumCopies;
   nuint16  ulPrintFlags;
   nuint16  uiMaxLines;
   nuint16  uiMaxChars;
   nuint8   ubFormName[MAX_FORM_NAME_LEN];
   nuint8   ubReserved[9];
   nuint16  uiFormType;
   nuint8   ubBanner[MAX_BANNER_NAME_LEN];
   nuint8   ubreserved2;
   nuint16  uiFlushCaptureTimeout;
   nuint8   ubFlushCaptureOnClose;
} NWCCaptureRWFlag, N_FAR *pNWCCaptureRWFlag;

typedef struct tagNWCCaptureROFlag
{
   nuint32  ulConnRef;
   nuint32  ulQueueID;
   nuint16  uiSetupStringMaxLength;
   nuint16  uiResetStringMaxLength;
   nuint8   ubLPTCaptureFlag;
   nuint8   ubFileCaptureFlag;
   nuint8   ubTimingOutFlag;
   nuint8   ubInProgress;
   nuint8   ubPrintQueueFlag;
   nuint8   ubPrintJobValid;
   nuint8   ubQueueName[MAX_QUEUE_NAME_LEN];
} NWCCaptureROFlag, N_FAR *pNWCCaptureROFlag;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

#define NWC_SPECIALCHAR1   0x10
#define NWC_SPECIALCHAR2   0x11
#define NWC_SPECIALCHAR3   0x12
#define NWC_SPECIALCHAR4   0x13


/* init/term functions */
N_EXTERN_LIBRARY( NWRCODE )
NWClientInit
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWClientTerm
(
   pNWAccess pAccess
);

/* standard 'C' model-independent memory and string functions */
#if defined (N_USE_CRT)

#if (N_USE_CRT == 2)
#define NWCMemSet        _fmemset
#define NWCMemMove       _fmemmove
#define NWCMemCmp        _fmemcmp
#define NWCStrCpy        _fstrcpy
#define NWCStrLen        _fstrlen
#define NWCStrCat        _fstrcat
#define NWCMalloc		 _fmalloc
#define NWCFree			 _ffree
#define NWCStrCmp		 _fstrcmp
#else
#define NWCMemSet        memset
#define NWCMemMove       memmove
#define NWCMemCmp        memcmp
#define NWCMemCpy        memcpy
#define NWCStrCpy        strcpy
#define NWCStrLen        strlen
#define NWCStrCat        strcat
#define NWCMalloc		 malloc
#define NWCFree			 free
#define NWCStrCmp		 strcmp
#endif

#else /* N_USE_CRT */

#if (defined N_PLAT_MSW && defined N_ARCH_16 && !defined N_PLAT_WNT) || defined(N_PLAT_DOS) || defined(N_PLAT_OS2)
#define NWCMalloc			 _fmalloc
#define NWCFree			 _ffree
#else
#define NWCMalloc			 malloc
#define NWCFree			 free
#endif


N_EXTERN_LIBRARY( nptr )
NWCMemSet
(
   nptr  pDst,
   nint  iChar,
   nuint uCount
);

N_EXTERN_LIBRARY( nptr )
NWCMemMove
(
   nptr  pDst,
   const void N_FAR *pSrc,
   nuint uCount
);

N_EXTERN_LIBRARY( nint )
NWCMemCmp
(
   const void N_FAR *pBuf,
   const void N_FAR *pBuf2,
   nuint uCount
);

N_GLOBAL_LIBRARY( nptr )
NWCMemCpy
(
   nptr  pDst,
   const void N_FAR *pSrc,
   nuint uCount
);

N_EXTERN_LIBRARY( nchar8 N_FAR * )
NWCStrCpy
(
   nchar8 N_FAR *pDst,
   const nchar8 N_FAR *pSrc
);

N_EXTERN_LIBRARY( nint )
NWCStrLen
(
   const nchar8 N_FAR *pStr
);

N_EXTERN_LIBRARY( nchar8 N_FAR * )
NWCStrCat
(
   nchar8 N_FAR *pStr1,
   const nchar8 N_FAR *pStr2
);

N_EXTERN_LIBRARY( nint )
NWCStrCmp
(
   const nchar8 N_FAR *pStr1,
   const nchar8 N_FAR *pStr2
);

#endif /* N_USE_CRT */

/* the following section deals with the requests */

N_EXTERN_LIBRARY( NWRCODE )
NWCRequest
(
   pNWAccess pAccess,
   nuint    uFunction,
   nuint    uNumReqFrags,
   pNWCFrag pReqFrags,
   nuint    uNumReplyFrags,
   pNWCFrag pReplyFrags,
   pnuint   puActualReplyLen
);

N_EXTERN_LIBRARY( NWRCODE )
NWCRequestSingle                 /* NCPs which don't need fragments */
(
   pNWAccess pAccess,
   nuint    uFunction,
   nptr     pReq,
   nuint    uReqLen,
   nptr     pReply,
   nuint    uReplyLen,
   pnuint   puActualReplyLen
);

N_EXTERN_LIBRARY( NWRCODE )
NWCRequestSimple                 /* NCPs with only subfunction request */
(
   pNWAccess pAccess,
   nuint    uFunction,
   nuint16  suNCPLen,
   nuint8   buSubfunction
);

N_EXTERN_LIBRARY( NWRCODE )
NWCFragmentRequest
(
   pNWAccess pAccess,
   nuint    uFunction,
   nuint32  luVerb,
   nuint    uNumReqFrags,
   pNWCFrag pReqFrags,
   nuint    uNumReplyFrags,
   pNWCFrag pReplyFrags,
   pnuint   puActualReplyLen
);


/* The following section is the connection interface calls */

N_EXTERN_LIBRARY( NWRCODE )
NWCResolveName
(
   pNWAccess          pAccess,
   nuint32           luConnHandle,
   pNWCConnString    pName,
   pnstr16           pstrServiceType,
   nuint             uTranType,
   pNWCConnString    pRepName,
   pnstr16           pstrRepServiceType,
   pNWCTranAddr      pRepTranAddr
);

N_EXTERN_LIBRARY( NWRCODE )
NWCOpenConnByAddr
(
   pNWAccess       pAccess,
   pnstr          pstrServiceType,
   nuint          uConnFlags,
   pNWCTranAddr   pTranAddr
);

N_EXTERN_LIBRARY( NWRCODE )
NWCOpenConnByName
(
   pNWAccess       pAccess,
   nuint32        luConnHandle,
   pNWCConnString pName,
   pnstr          pstrServiceType,
   nuint          uConnFlags,
   nuint          uTranType
);

N_EXTERN_LIBRARY( NWRCODE )
NWCOpenConnByReference
(
   pNWAccess    pAccess,
   nuint32     luConnectionReference,
   nuint       uConnFlags
);


N_EXTERN_LIBRARY( NWRCODE )
NWCCloseConn
(
   pNWAccess    pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSysCloseConn
(
   pNWAccess    pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWCMakeConnPermanent
(
   pNWAccess    pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWCLicenseConn
(
   pNWAccess    pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWCUnlicenseConn
(
   pNWAccess    pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetConnInfo
(
   pNWAccess       pAccess,
   nuint          uInfoLevel,
   nuint          uInfoLen,
   nptr           pConnInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWCScanConnInfo
(
   pNWAccess       pAccess,
   pnuint32       pluScanIndex,
   nuint          uScanInfoLevel,
   nuint          uScanInfoLen,
   nptr           pScanConnInfo,
   nuint          uScanFlags,
   nuint          uReturnInfoLevel,
   nuint          uReturnInfoLen,
   pnuint32       pluConnectionReference,
   nptr           pReturnConnInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetConnInfo
(
   pNWAccess       pAccess,
   nuint          uInfoLevel,
   nuint          uInfoLength,
   nptr           pConnInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetPrefServer
(
   pNWAccess pAccess,
   pnuint   puPrefServerLen,
   pnstr    pstrPrefServer
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetPrefServer
(
   pNWAccess pAccess,
   nuint    uPrefServerLen,
   pnstr    pstrPrefServer
);

/* Authentication calls */

N_EXTERN_LIBRARY( NWRCODE )
NWCAuthenticate
(
   pNWAccess       pAccess,
   nuint          uAuthenticationType,
   pnstr8         pstrUserName,
   pnstr8         pstrPassword,
   pNWCAuthenInfo pAuthenInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWCUnauthenticate
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWCUnauthenticateConnection
(
   pNWAccess       pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWCCreateAuthenticationID
(
   pNWAccess       pAccess,
   nuint          uAuthenticationType,
   punicode       pstrUserName,
   pnstr8         pstrPassword,
   pNWCAuthenInfo pAuthenInfo,
   pnuint32       pluAuthenticationID
);

N_EXTERN_LIBRARY( NWRCODE )
NWCAuthenticateWithID
(
   pNWAccess       pAccess,
   nuint32        luAuthenticationID
);

N_EXTERN_LIBRARY( NWRCODE )
NWCScanAuthenticationIDs
(
   pNWAccess       pAccess,
   pnuint32       pluScanIndex,
   pnuint32       pluAuthenticationID,
   pnuint         puAuthenticationType,
   punicode       pstrUserName,
   pNWCAuthenInfo pAuthenInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWCFreeAuthenticationID
(
   pNWAccess       pAccess,
   nuint32        luAuthenticationID
);

/* Security and signing calls */

N_EXTERN_LIBRARY( NWRCODE )
NWCCreateSessionKey
(
   pNWAccess pAccess,
   pnuint8  pbuSessionKey
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetSessionKey
(
   pNWAccess pAccess,
   pnuint8  pbuSessionKey
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetSecurityFlags
(
   pNWAccess pAccess,
   pnuint32 pluSecurityFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetSecurityFlags
(
   pNWAccess pAccess,
   nuint32  luSecurityFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWCRenegotiateSecurityLevel
(
   pNWAccess pAccess
);


/* Tagged data store calls */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetTdsInfo
(
   pNWAccess pAccess,
   nuint    uTag,
   pnuint   puMaxSize,
   pnuint   puDataSize,
   pnuint   puTdsFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWCAllocTds
(
   pNWAccess pAccess,
   nuint    uTag,
   nuint    uTdsSize,
   nuint    uTdsFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWCFreeTds
(
   pNWAccess pAccess,
   nuint    uTag
);

N_EXTERN_LIBRARY( NWRCODE )
NWCReadTds
(
   pNWAccess pAccess,
   nuint    uTag,
   nuint    uBytesToRead,
   nuint    uReadOffset,
   pnchar   pchBuffer,
   pnuint   puBytesRead
);

N_EXTERN_LIBRARY( NWRCODE )
NWCWriteTds
(
   pNWAccess pAccess,
   nuint    uTag,
   nuint    uBytesToWrite,
   nuint    uWriteOffset,
   pnchar   pchBuffer,
   pnuint   puBytesWritten
);

/* NDS support calls */

N_EXTERN_LIBRARY( NWRCODE )
NWCGetDefNameContext
(
   pNWAccess pAccess,
   pnuint   puNameContextLen,
   pnstr16  pstrNameContext
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetDefNameContext
(
   pNWAccess pAccess,
   nuint    uNameContextLen,
   pnstr16  pstrNameContext
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetPrefDsTree
(
   pNWAccess pAccess,
   pnuint   puPrefDsTreeLen,
   pnstr    pstrPrefDsTree
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetPrefDsTree
(
   pNWAccess pAccess,
   nuint    uPrefDsTreeLen,
   pnstr    pstrPrefDsTree
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetMonitoredConnReference
(
   pNWAccess pAccesss,
   pnuint32 pLuConnReference
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetMonitoredConn
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY(NWRCODE)
NWCGetTreeName
(
  pNWAccess	 pAccess,
  pnstr8     pbstrTreeName
);

/* File handle conversion calls go here. */

N_EXTERN_LIBRARY( NWRCODE )
NWCConvertLocalFileHandle
(
   pNWAccess          pAccess,
   NWC_FILE_HANDLE   localFileHandle,
   pnuint32          pluConnRef,
   pnuint8           pbuNWHandleB6
);

N_EXTERN_LIBRARY( NWRCODE )
NWCConvertNetWareFileHandle
(
   pNWAccess          pAccess,
   nuint8            buAccessMode,
   pnuint8           pbuNWHandleB6,
   nuint32           luFileSize,
   NWC_FILE_HANDLE  N_FAR *pLocalFileHandle
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetConnReference                                /* NWREQSUP\CGTCNREF.C  */
(
   pNWAccess pAccess,
   pnuint32 pluConnectionReference
);

/* Directory redirection calls go here. (Drive mapping) */

N_EXTERN_LIBRARY( NWRCODE )
NWCMapDrive
(
   pNWAccess pAccess,
   nuint    uDriveNum,
   nuint8   buDirHandle,
   pnstr8   pbstrDirPath
);

N_EXTERN_LIBRARY( NWRCODE )
NWCUnmapDrive
(
   pNWAccess pAccess,
   nuint    uDriveNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetMapInformation
(
   pNWAccess pAccess,
   nuint    uDriveNum,
   pnuint32 pluMapStatus,
   pnuint32 pluConnReference,
   pnuint8  pbuDirHandle,
   pnstr8   pbstrDirPath
);


/* Print object redirection calls go here. (Capture) */

N_EXTERN_LIBRARY( NWRCODE )
NWCGetMaxPrinters
(
   pNWAccess pAccess,
   pnuint   puMaxPrinters
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetPrinterStrings
(
   pNWAccess pAccess,
   nuint    uLPTDevice,
   pnuint   puSetupStringLen,
   pnstr8   pbstrSetupString,
   pnuint   puResetStringLen,
   pnstr8   pbstrResetString
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetPrinterStrings
(
   pNWAccess pAccess,
   nuint    uLPTDevice,
   nuint    uSetupStringLen,
   pnstr8   pbstrSetupString,
   nuint    uResetStringLen,
   pnstr8   pbstrResetString
);


N_EXTERN_LIBRARY( NWRCODE )
NWCGetCaptureFlags
(
   pNWAccess          pAccess,
   nuint             uLPTDevice,
   pNWCCaptureRWFlag pCaptureFlagsRW,
   pNWCCaptureROFlag pCaptureFlagsRO
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetCaptureFlags
(
   pNWAccess          pAccess,
   nuint             uLPTDevice,
   pNWCCaptureRWFlag pCaptureFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetBannerUserName
(
   pNWAccess pAccess,
   pnuint   uBannerUserNameLen,
   pnstr8    pbstrBannerUserName
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetBannerUserName
(
   pNWAccess pAccess,
   nuint   uBannerUserNameLen,
   pnstr8   pbstrBannerUserName
);

N_EXTERN_LIBRARY( NWRCODE )
NWCStartCapture
(
   pNWAccess pAccess,
   nuint    uLPTDevice,
   nuint32  luQueueId,
   pnstr8   pbstrQueueName
);

N_EXTERN_LIBRARY( NWRCODE )
NWCEndCapture
(
   pNWAccess pAccess,
   nuint    uLPTDevice
);

N_EXTERN_LIBRARY( NWRCODE )
NWCCancelCapture
(
   pNWAccess pAccess,
   nuint    uLPTDevice
);

N_EXTERN_LIBRARY( NWRCODE )
NWCFlushCapture
(
   pNWAccess pAccess,
   nuint    uLPTDevice
);


/* Misc. information calls */

N_EXTERN_LIBRARY( NWRCODE )
NWCGetNumConns
(
   pNWAccess       pAccess,
   pnuint         puMaxConns,
   pnuint         puPublicConns,
   pnuint         puMyPrivateConns,
   pnuint         puOtherPrivateConns
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetRequesterVersion
(
   pNWAccess       pAccess,
   pnuint         puMajorVersion,
   pnuint         puMinorVersion,
   pnuint         puRevision
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetPrimConnRef
(
   pNWAccess       pAccess,
   pnuint32       pluConnRef
);

N_EXTERN_LIBRARY( NWRCODE )
NWCSetPrimConn
(
   pNWAccess       pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWCGetServerVersion
(
   pNWAccess       pAccess,
   pnuint         puMajorVersion,
   pnuint         puMinorVersion
);

/* miscellaneous helper functions */

N_EXTERN_LIBRARY (void)
NWCConvertToSpecialChar
(
   pnstr8   pbustrPath
);

/* platform specific things */

#if defined N_PLAT_WNT && defined N_ARCH_32

typedef struct tagNWCNTNcpReq
{
   nuint32        luConnHandle;
   nuint32        luFunction;
   nuint32        luNumReqFrags;
   nuint32        luNumReplyFrags;
   struct
   {
      nuint32  luLen;
      nptr     pAddr;
   } NT_Frags[5];
} NWCNTNcpReq;

N_EXTERN_LIBRARY( NWRCODE )
NWCNTGetBcastMessage
(
   pNWAccess       pAccess,
   nflag32        flMessageFlags,
   pnuint32       pluConnRef,
   nflag32        flOptions
);

#elif defined(N_PLAT_UNIX) || defined( N_PLAT_NLM )
typedef struct UNIX_NWFILE_STRUCT
{
   nuint32            conn;
   nuint8             pbuNWHandle[6];
   nuint16            suHandleType;
   nuint32            luOffset;
} UNIX_NWFILE_STRUCT;

#elif defined(N_PLAT_DOS) || (defined N_PLAT_MSW && defined N_ARCH_16 && !defined N_PLAT_WNT)

#ifndef _NWC_REGISTERS_DEF
#define _NWC_REGISTERS_DEF

typedef union
{
   struct
   {
      nuint16 si;
      nuint16 ds;
      nuint16 di;
      nuint16 es;
      nuint8  al, ah;
      nuint8  bl, bh;
      nuint8  cl, ch;
      nuint8  dl, dh;
   } b;

   struct
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
   } s;

   struct
   {
      nptr  ds_si;
      nptr  es_di;
   } p;
} NWCRegs, N_FAR *pNWCRegs;
#endif /* _NWC_REGISTERS_DEF */

#ifndef NWC_USE_DS
#define NWC_USE_DS  1
#define NWC_USE_ES  2
#define NWC_USE_DOS 0x80
#endif  /* NWC_USE_DS */

N_EXTERN_LIBRARY( nuint16 )
NWCShellReq
(
   pNWCRegs pRegs,
   nuint16  suMask
);

N_EXTERN_LIBRARY( nuint16 )
NWCVlmReq
(
  nuint16        callerID,
  nuint16        vlmID,
  nuint16        function,
  NWCRegs N_FAR *pRegs,
  nuint16        mask
);

N_EXTERN_LIBRARY( nbool )
_NWCShellIsLoaded
(
   void
);

N_EXTERN_LIBRARY( NWRCODE )
_NWCVlmInit
(
  void
);

#endif /* platform specific sections */

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
}
#endif

#endif /* NWCLIENT_H */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwclient.h,v 1.9 1994/09/30 23:56:26 rebekah Exp $
*/
