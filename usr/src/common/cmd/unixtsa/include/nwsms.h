/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/nwsms.h	1.1"

#if !defined(_nwsms_)

/*...............................TYPES......................................*/
#if !defined(_nwtypes_) && !defined(_NWTYPES_H_INCLUDED_)
    typedef short          NWBOOLEAN;
    typedef unsigned long  CCODE;
    typedef char           INT8;
    typedef short          INT16;
    typedef long           INT32;
    typedef unsigned char  UINT8;
    typedef unsigned short UINT16;
    typedef unsigned long  UINT32;

    typedef char           int8;
    typedef short          int16;
    typedef long           int32;
    typedef unsigned char  uint8;
    typedef unsigned short uint16;
    typedef unsigned long  uint32;

    typedef unsigned char  BUFFER;
    typedef unsigned char *BUFFERPTR;
    typedef unsigned char  CHAR;
    typedef unsigned char *LSTRING;
    typedef unsigned char *PSTRING;
    typedef unsigned char *STRING;
#define _nwtypes_
#endif

#if defined(NETWARE_J)
#if !defined(DEFINES_NLS)
#define strchr  NWLstrchr
#define strpbrk NWLstrpbrk
#define strrchr NWLstrrchr
#define strstr  NWLstrstr
#define strupr  NWConvertToUpperCase
#define DEFINES_NLS

        char *NWLstrchr( 
                 const char *__s, 
                 int         __c );
        char *NWLstrpbrk( 
                 const char *__s1, 
                 const char *__s2 );
        char *NWLstrrchr( 
                 const char *__s, 
                 int         __c );
        char *NWLstrstr( 
                 const char *__s1, 
                 const char *__s2 );
        char *NWConvertToUpperCase( 
                 char  *__string );
#endif
#elif defined(NETWARE_V320)
#if !defined(DEFINES_NLS)
#define strchr  NWLstrchr
#define strpbrk NWLstrpbrk
#define strrchr NWLstrrchr
#define strstr  NWLstrstr
#define strupr  NWLstrupr
#define strxfrm NWLstrxfrm
#define printf  NWprintf
#define sprintf NWsprintf
#define fprintf NWfprintf
#define DEFINES_NLS
#endif
#elif defined(V2X_TSA_ON_40)
#if !defined(DEFINES_NLS)
#define strchr  NWLstrchr
#define strpbrk NWLstrpbrk
#define strrchr NWLstrrchr
#define strstr  NWLstrstr
#define strupr  NWLstrupr
#define strxfrm NWLstrxfrm
#define printf  NWprintf
#define sprintf NWsprintf
#define fprintf NWfprintf
#define DEFINES_NLS
#endif
#endif

    typedef UINT8  NWSMFILE_HANDLE[6];
    typedef UINT32 NWSMDIR_HANDLE;
    typedef UINT32 NWEA_HANDLE;

#if !defined(loop)
#define loop for (;;)
#define is   ==
#define isnt !=
#define and  &&
#define or   ||
#define AND  &
#define OR   |
#endif

/*...............................SIZES......................................*/
#define NWDEFAULT_ENUM_EA_SIZE 2048
#define NWMAX_EA_KEY_LENGTH    256
#define NWMAX_ENTRY_NAME       300
#define NWMAX_ENUM_EA_SIZE     266 * 512
#define NWMAX_NCP_BUFFER_SIZE  512
#define NWMAX_PATH_LEN         257
#define NW_COMPONENET_PATH_LEN 300
#define NWMAX_TRUSTEES         20
#define NWMAX_VOLUMES          64  /*...MaximumNumberOfVolumes (config.inc)...*/
#define NWNUM_DATA_STREAMS     3   /*...MaximumNumberOfDataStreams (config.inc)...*/
#if defined(NCP_SMS_H)
#define NWNUM_NAME_SPACES  2   /*...MaximumNumberOfNameSpaces (nspace.h)...*/
#else
#define NWNUM_NAME_SPACES 5   /*...MaximumNumberOfNameSpaces (nspace.h)...*/
#endif
#define NWMAX_EA_BUF_SIZE     0xFFFFFF80
#define NWPAGE_BOUNDARY       0XFFFFFF80
#define NWSTATE_INFO_SIZE     16
#define NWSERVER_PAGE_SIZE    128

/*   SMS    allocation signature...*/
#define NWSMS_SIGNATURE       0x00534D53
#define Ref(v) v = v
#define NWSMS_DEFAULT_INHERITED_RIGHTS 0x1FF


/*...............................STRUCTURES.................................*/
typedef union    /* this is the inch-worm! */
{
    UINT8  *cp;
    UINT16 *wp;
    UINT32 *lp;
} PACKET_PTR_U;


typedef struct
{
/* 0x00 */ UINT32 signature;
/* 0x04 */ UINT32 clientConnID;
/* 0x08 */ UINT32 entryIndex;
/* 0x0C */ UINT32 NSEntryIndex;
/* 0x10 */ UINT32 fixedMask;
/* 0x14 */ UINT32 variableMask;
/* 0x18 */ UINT32 hugeMask;
/* 0x1C */ UINT32 checkBit;
/* 0x20 */ UINT32 lengthsTable[32];

/* 0xA0 */ UINT16 volNum;
/* 0xA2 */ UINT16 nameSpace;
/* 0xA4 */ UINT16 numFixed;
/* 0xA6 */ UINT16 numVariable;
/* 0xA8 */ UINT16 numHuge;
/* 0xAA */ UINT16 numTotal;
/* 0xAC */ UINT8  stateInfo[NWSTATE_INFO_SIZE];
/* 0xBC */ UINT8  newBufferFlag;
/* 0xBD */ UINT8  index;
} NWNS_ENTRY_INFO;


typedef enum
{
    NWNS_INFO_NONE,
    NWNS_INFO_FIXED,
    NWNS_INFO_VAR,
    NWNS_INFO_HUGE
} NWNS_INFO_TYPE;


typedef struct
{                                              /* NCP  */
/* 0x00 */ UINT32    clientConnID;             /* 0x00 */
/* 0x04 */ UINT32    nameSpace;                /* 0x04 */
/* 0x08 */ UINT32    signature;                /* 0x08 */
/* 0x0C */ UINT32    returnMask;               /* 0x0C */
#if defined(NCP_SMS_H)
           UINT32    AFPEntryID;               /* 0x10 */
           UINT32    dirHandle;                /* 0x14 */
           INT32     sequence;                 /* 0x18 */
           UINT32    sequenceNum;              /* 0x1C */
#else
/* 0x10 */ UINT32    dirNum;
#endif
/* 0x14 */ INT32     entryNum;                 /* 0x20 */
#if defined(NCP_SMS_H)
           NWBOOLEAN getMacInfo;               /* 0x24 */
           NWBOOLEAN deallocateDirHandle;      /* 0x26 */
#endif
/* 0x18 */ UINT16    searchAttributes;         /* 0x28 */
/* 0x1A */ char      search[NWMAX_ENTRY_NAME]; /* 0x2A */
/* 0x146*/ UINT8     volNum;                   /* 0x156*/
#if defined(NCP_SMS_H)
           UINT8     AFPFileName[33];          /* 0x157 */
           UINT8     DosFileName[16];          /* 0x178 */
           BUFFER    child[NWMAX_ENTRY_NAME];  /* 0x188 */
#endif
} NWSCAN_SEQUENCE;


typedef struct
{
/* 0x00 */ UINT32 objectID;
/* 0x04 */ UINT16 trusteeRights;
} NWTRUSTEES;


typedef struct
{
/* 0x00 */ UINT32      signature;
/* 0x04 */ INT32       entryIndex;
/* 0x08 */ UINT16      sequence;
/* 0x0A */ UINT16      count;
/* 0x0C */ NWTRUSTEES  trustees[NWMAX_TRUSTEES];
} NWSCAN_TRUSTEE_SEQUENCE;


typedef struct
{
/* 0x00 */ INT32       sequence;
/* 0x04 */ INT32       entryType;
/* 0x08 */ STRING      EAName;
/* 0x0C */ UINT32      signature;
/* 0x10 */ UINT32      totalKeySize;
/* 0x14 */ UINT32      totalEAs;
/* 0x18 */ UINT32      totalSize;
/* 0x1C */ UINT32      clientConnID;
/* 0x20 */ UINT32      volNum;
/* 0x24 */ UINT32      entryIndex;
/* 0x28 */ BUFFERPTR   enumBigBuffer;
/* 0x2C */ BUFFERPTR   enumBufCurrent;
/* 0x30 */ NWEA_HANDLE EAHandle;
/* 0x34 */ UINT16      nextSequence;
/* 0x36 */ UINT16      currentEAs;
/* 0x38 */ UINT8       enumBuf[NWDEFAULT_ENUM_EA_SIZE];
} NWEA_SCAN_SEQUENCE;


typedef struct
{
/* 0x00 */ INT32         entryType;
/* 0x04 */ STRING        EAName;
/* 0x08 */ UINT32        clientConnID;
/* 0x0C */ UINT32        entryIndex;
/* 0x10 */ NWEA_HANDLE   EAHandle;
/* 0x14 */ UINT8         overflowBuffer[NWSERVER_PAGE_SIZE];
/* 0x94 */ NWSMFILE_HANDLE fileHandle;
/* 0x9A */ UINT16        volNum;
/* 0x9C */ UINT8         overflow;
} NWEA_ENTRY_SPEC;


typedef struct
{
/* 0x00 */ UINT32       clientConnID;
/* 0x04 */ UINT32       nameSpace;
/* 0x08 */ UINT32       entryIndex;
/* 0x0C */ STRING       pathName;
/* 0x10 */ NWSMDIR_HANDLE dirHandle;
#if defined(NCP_SMS_H)
/* 0x14 */ NWBOOLEAN    isDirectory;
/* 0x16 */ NWBOOLEAN    AFPIsValid;
/* 0x18 */ UINT8        volNum;
#endif
} NWENTRY_SPEC;


typedef struct
{
/* 0x00 */ UINT32 entryIndex;
/* 0x04 */ UINT32 primaryEntryIndex;
/* 0x08 */ UINT32 attributes;
/* 0x0C */ UINT32 creatorID;
/* 0x10 */ UINT32 archiverID;
/* 0x14 */ UINT32 maximumSpace;
/* 0x18 */ UINT32 inheritedRightsMask;
/* 0x1C */ UINT32 extendedAttributesSize;
/* 0x20 */ UINT32 extendedAttributesCount;
/* 0x24 */ UINT32 extendedAttributesKeySize;
/* 0x28 */ UINT32 creatorNameSpaceNumber;
/* 0x2C */ UINT16 volNum;
/* 0x2E */ UINT16 flags;
/* 0x30 */ UINT16 creationDate;
/* 0x32 */ UINT16 creationTime;
/* 0x34 */ UINT16 archiveDate;
/* 0x36 */ UINT16 archiveTime;
/* 0x38 */ char   dirName[NWMAX_PATH_LEN];
} NWDIR_ENTRY_INFO;


typedef struct
{
/* 0x00 */ UINT32 dataLength;
/* 0x04 */ UINT32 accessFlags;
/* 0x08 */ char   EAName[NWMAX_EA_KEY_LENGTH];
} NWEA_INFO;


typedef struct
{
/* 0x00 */ UINT32 entryIndex;
/* 0x04 */ UINT32 primaryEntryIndex;
/* 0x08 */ UINT32 volNum;
/* 0x0C */ UINT32 attributes;
/* 0x10 */ UINT32 creatorID;
/* 0x14 */ UINT32 archiverID;
/* 0x18 */ UINT32 modifierID;
/* 0x1C */ UINT32 primaryDataStreamSize;
/* 0x20 */ UINT32 totalStreamsDataSize;
/* 0x24 */ UINT32 diskSpaceAllocated;
/* 0x28 */ UINT32 inheritedRightsMask;
/* 0x2C */ UINT32 deletorID;
/* 0x30 */ UINT32 extendedAttributesSize;
/* 0x34 */ UINT32 extendedAttributesCount;
/* 0x38 */ UINT32 extendedAttributesKeySize;
/* 0x3C */ UINT32 creatorNameSpaceNumber;
/* 0x40 */ UINT16 flags;
/* 0x42 */ UINT16 creationDate;
/* 0x44 */ UINT16 creationTime;
/* 0x46 */ UINT16 archiveDate;
/* 0x48 */ UINT16 archiveTime;
/* 0x4A */ UINT16 modifyDate;
/* 0x4C */ UINT16 modifyTime;
/* 0x4E */ UINT16 lastAccessDate;
/* 0x50 */ UINT16 numberOfDataStreams;
/* 0x52 */ UINT16 deletedDate;
/* 0x54 */ UINT16 deletedTime;
/* 0x56 */ char   fileName[NWMAX_PATH_LEN];
} NWFILE_ENTRY_INFO;
typedef NWFILE_ENTRY_INFO NWDOS_NAME_SPACE_ENTRY;


typedef struct /* Passed to OS, can't align! */
{
/* 0x00 */ UINT8  volume;
/* 0x01 */ UINT32 dirBaseOrShortDirHandle;
/* 0x05 */ UINT8  handleFlag;
/* 0x06 */ UINT8  pathLength;
/* 0x07 */ UINT8  path[NW_COMPONENET_PATH_LEN];
} NWHANDLE_PATH;


typedef NWFILE_ENTRY_INFO    NWOS2_NAME_SPACE_ENTRY;

/*...............................MACROS and DEFINES.........................*/
#define NWEndScanTrustee(seq)  (seq.signature is NWSMS_SIGNATURE ? (seq.signature = 0) : NWERR_FS_INVALID_SEQUENCE)
#define NWInitScanTrustee(seq) { seq.sequence = 0; seq.entryIndex = 0 ; seq.signature = NWSMS_SIGNATURE; }
#define NWSwap16(x) (  (UINT16)(((x) AND 0x00ff)     << 8 ) \
                    OR (UINT16)(((x) AND 0xff00)     >> 8 ) )

#define NWSwap32(x) (  (UINT32)(((x) AND 0x000000ffL) << 24) \
                    OR (UINT32)(((x) AND 0x0000ff00L) << 8 ) \
                    OR (UINT32)(((x) AND 0x00ff0000L) >> 8 ) \
                    OR (UINT32)(((x) AND 0xff000000L) >> 24) )

#define _NWFillHandleStruct(e, p)  if ((ccode = _NWFillHandlePathStruct(e, p))\
                                           isnt 0) return (ccode)

/* The following macro is valid only for English ASCII (but it's fast) */
#define TOUPPER(ch)    (((ch) >= 'a' and (ch) <= 'z') ? (ch) - ' ' : (ch))

#if !defined(TRUE)
#define TRUE  1
#define FALSE 0
#endif

#if !defined(NULL)
#define NULL 0
#endif

#define NWEA_SPEC_ENTRY_INDEX 0
#define NWEA_SPEC_FILE_HANDLE 1
#define NWEA_SPEC_EA_HANDLE   2

#if defined(NCP_SMS_H)
    /* ... nameSpaceTypes */
#define MyDOSNameSpace 0x00
#define MyMACNameSpace 0x01
#endif

/*...access rights...*/
#define NWFILE_READ                    0x0001
#define NWFILE_WRITE                   0x0002
#define NWFILE_DENY_READ               0x0004
#define NWFILE_DENY_WRITE              0x0008
#define NWFILE_READ_DENY_WRITE         0x0009
#define NWFILE_SINGLE_USER             0x0010
#define NWFILE_READ_WRITE_DENY_ALL     0x000F
#define NWENABLE_IO_ON_COMPRESSED_DATA	0x00000100
#define NWLEAVE_FILE_COMPRESSED	    0x00000200

/*...directory handle allocation flags...*/
#define NWSMDIR_HANDLE_PERMANENT    0x00L
#define NWSMDIR_HANDLE_TEMPORARY    0x01L
#define NWSMDIR_HANDLE_SPECIAL_TEMP 0x02L

/*...entry modification flags...*/
#define NWMODIFY_NAME                 0x00000001L
#define NWMODIFY_ATTRIBUTES           0x00000002L
#define NWMODIFY_CREATION_DATE        0x00000004L
#define NWMODIFY_CREATION_TIME        0x00000008L
#define NWMODIFY_CREATOR_ID           0x00000010L
#define NWMODIFY_ARCHIVE_DATE         0x00000020L
#define NWMODIFY_ARCHIVE_TIME         0x00000040L
#define NWMODIFY_ARCHIVER_ID          0x00000080L
#define NWMODIFY_MODIFY_DATE          0x00000100L
#define NWMODIFY_MODIFY_TIME          0x00000200L
#define NWMODIFY_MODIFIER_ID          0x00000400L
#define NWMODIFY_ACCESSED_DATE        0x00000800L
#define NWMODIFY_RIGHTS_MASK          0x00001000L
#define NWMODIFY_MAXIMUM_SPACE        0x00002000L
#define NWMODIFY_ALL                  0x00003FFFL
#define NWMODIFY_ALL_EXCEPT_NAME      0x00003FFEL
#define NWMOD_ALL_EXCEPT_NAME_AND_IDS 0x00003B6EL

/*...return info flags....*/
#define NWRETURN_NAME                   0x00000001
#define NWRETURN_DATA_STREAM_ALLOC      0x00000002
#define NWRETURN_ATTRIBUTES             0x00000004
#define NWRETURN_DATA_STREAM_SIZE       0x00000008
#define NWRETURN_TOTAL_DATA_STREAM_SIZE 0x00000010
#define NWRETURN_EA_INFO                0x00000020
#define NWRETURN_ARCHIVE_INFO           0x00000040
#define NWRETURN_MODIFY_INFO            0x00000080
#define NWRETURN_CREATION_INFO          0x00000100
#define NWRETRUN_NAME_SPACE_INFO        0x00000200
#define NWRETURN_DIRECTORY_INFO         0x00000400
#define NWRETURN_RIGHTS_INFO            0x00000800
#define NWRETURN_MAX_SPACE_INFO         0x00002000
#define NWRETURN_ALL_INFO               0x00002FFF

#define NW_RNSAttribute                 0x00002000
#define NW_RDataStreamSizes             0x00004000
#define NW_RNewStyle                    0x80000000	

/*...search attributes...*/
#define NWSA_NORMAL             0x0000
#define NWSA_ALL_FILES_AND_DIRS 0x0001
#define NWSA_HIDDEN             0x0002
#define NWSA_SYSTEM             0x0004
#define NWSA_ALL_FILES          0x0006
#define NWSA_DIRECTORIES        0x0010
#define NWSA_ALL_DIRS           0x0016

/*...lock directives...*/
#define NWLD_LOG_RECORD     0x0000
#define NWLD_LOCK_EXCLUSIVE 0x0001
#define NWLD_LOCK_SHAREABLE 0x0011

#define NWLD_WHOLE_FILE     0xFFFFFFFF

/*...entry attributes...*/
#define NWFA_NORMAL                    0x00000000
#define NWFA_READ_ONLY                 0x00000001
#define NWFA_HIDDEN                    0x00000002
#define NWFA_SYSTEM                    0x00000004
#define NWFA_EXECUTE_ONLY              0x00000008
#define NWFA_DIRECTORY                 0x00000010
#define NWFA_ARCHIVE                   0x00000020
#define NWFA_SHARABLE                  0x00000080
#define NWFA_LOW_SEARCH                0x00000100
#define NWFA_MEDIUM_SEARCH             0x00000200
#define NWFA_HIGH_SEARCH               0x00000400
#define NWFA_TRANSACTION               0x00001000
#define NWFA_INDEXED                   0x00002000
#define NWFA_READ_AUDIT                0x00004000
#define NWFA_WRITE_AUDIT               0x00008000
#define NWFA_PURGE                     0x00010000
#define NWFA_RENAME_INHIBIT            0x00020000
#define NWFA_DELETE_INHIBIT            0x00040000
#define NWFA_COPY_INHIBIT              0x00080000
#define NWFA_FILE_AUDITING 	        0x00100000
#define NWFA_REMOTE_DATA_ACCESS        0x00400000
#define NWFA_REMOTE_DATA_INHIBIT       0x00800000
#define NWFA_REMOTE_DATA_SAVE_KEY_BIT	0x01000000	/* ie. Data Migration (file only)*/
#define NWFA_COMPRESS_FILE_IMMEDIATE   0x02000000
#define NWFA_DATA_STREAM_IS_COMPRESSED	0x04000000
#define NWFA_DO_NOT_COMPRESS_FILE      0x08000000
#define NWFA_CANT_COMPRESS_DATA        0x20000000
#define NWFA_MASK_TO_NORMAL			(NWFA_READ_ONLY | NWFA_TRANSACTION | NWFA_RENAME_INHIBIT | NWFA_DELETE_INHIBIT | NWFA_SYSTEM | NWFA_HIDDEN)

/*...trustee rights...*/
#define NWTR_NONE       0x00000000L
#define NWTR_READ       0x00000001L
#define NWTR_WRITE      0x00000002L
#define NWTR_CREATE     0x00000008L
#define NWTR_ERASE      0x00000010L
#define NWTR_ACCESS     0x00000020L
#define NWTR_FILE_SCAN  0x00000040L
#define NWTR_MODIFY     0x00000080L
#define NWTR_SUPERVISOR 0x00000100L
#define NWTR_NORMAL     0x000000FBL
#define NWTR_ALL        0x000001FBL

/*...............................ERRORS.....................................*/
#define NWERR_FS_INSUFFICIENT_SPACE   0x00000001
#define NWERR_FS_HARD_LINK_COLLISION  0x00000008
#define NWERR_FS_NO_MORE_DATA         0x00000013
#define NWERR_FS_INVALID_SEQUENCE     0x00000014
#define NWERR_FS_INVALID_SET_DATA     0x00000015
#define NWERR_FS_PATH_TOO_LONG        0x00000016
#define NWERR_FS_NO_FILE_TRUSTEES     0x00000017
#define NWERR_FS_EAS_NOT_APPLICABLE   0X00000018
#define NWERR_FS_FILE_IN_USE          0x00000080
#define NWERR_FS_FILE_IN_USE_NOT_AFF  0x0000008E
#define NWERR_FS_NO_SEARCH_PRIVILEGES 0x00000089
#define NWERR_FS_NO_MODIFY_PRIVILEGES 0x0000008C
#define NWERR_FS_SERVER_OUT_OF_MEMORY 0x00000096
#define NWERR_FS_VOL_DOES_NOT_EXIST   0x00000098
#define NWERR_FS_DIR_FULL             0x00000099
#define NWERR_FS_BAD_DIR_HANDLE       0x0000009B
#define NWERR_FS_INVALID_PATH         0x0000009C
#define NWERR_FS_TRUSTEE_NOT_FOUND    0x0000009C
#define NWERR_FS_DIRECTORY_NOT_EMPTY  0x000000A0
#define NWERR_FS_INVALID_DATA_STREAM  0x000000BE
#define NWERR_FS_INVALID_NAME_SPACE   0x000000BF
#define NWERR_FS_EA_INTERNAL_FAILURE  0x000000CC
#define NWERR_FS_SMALL_RECEIVE_SIZE   0x000000D5
#define NWERR_FS_INVALID_FILE_SYSTEM  0x000000FB
#define NWERR_FS_NO_SUCH_BINDERY_OBJ  0x000000FC
#define NWERR_FS_NO_SUCH_RESTRICTION  0x000000FE
#define NWERR_FS_ENTRY_NOT_FOUND      0x000000FF
#define NWERR_FS_NO_MORE_INFO         0x000000FF

/*...............................NAME_SPACES................................*/
#define NWNAME_SPACE_PRIMARY 0x00
#define NWNAME_SPACE_DOS     0x00
#define NWNAME_SPACE_MAC     0x01
#define NWNAME_SPACE_UNIX    0x02
#define NWNAME_SPACE_FTAM    0x03
#define NWNAME_SPACE_OS2     0x04

/*...file action flags...*/
#define NWFILE_ACTION_OPEN    0x0001
#define NWFILE_ACTION_REPLACE 0x0002
#define NWFILE_ACTION_CREATE  0x0008

/*...............................PROTOTYPES.................................*/
CCODE NWAddTrustee(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        trusteeID,
    UINT16        trusteeRights);

CCODE NWAllocDirHandle(
    NWENTRY_SPEC *dirEntry,
    UINT16        allocFlags,
    NWSMDIR_HANDLE *dirHandle,
    UINT16       *volNum);

CCODE NWCloseEA(
    UINT32      clientConnID,
    NWEA_HANDLE EAHandle);

CCODE NWCloseFile(
    UINT32        clientConnID,
    NWSMFILE_HANDLE fileHandle);

CCODE NWCopyEAs(
    NWEA_ENTRY_SPEC *sourceEAPath,
    NWEA_ENTRY_SPEC *destEAPath);

CCODE NWDeallocDirHandle(
    UINT32       clientConnID,
    NWSMDIR_HANDLE dirHandle);

CCODE NWCreateDirEntry(
    NWENTRY_SPEC *dirEntry,
    UINT32        createAttributes);

CCODE NWDeleteDirEntry(
    NWENTRY_SPEC *dirEntry,
    UINT16        searchAttributes);

CCODE NWDeleteEA(
    NWEA_ENTRY_SPEC *EAEntry);

CCODE NWDeleteExistingEAs(
    NWENTRY_SPEC *entry);

CCODE NWDeleteFileEntry(
    NWENTRY_SPEC *fileEntry,
    UINT16        searchAttributes);

CCODE NWDeleteTrustee(
    NWENTRY_SPEC *entry,
    UINT32        trusteeID);

CCODE NWEndDirScan(
    NWSCAN_SEQUENCE *sequence);

CCODE NWEndEAScan(
    NWEA_SCAN_SEQUENCE *sequence);

CCODE NWEndFileScan(
    NWSCAN_SEQUENCE *sequence);

CCODE NWEndNameSpaceInfoScan(
    NWNS_ENTRY_INFO *sequence);

CCODE NWGetDataStreamAttributes(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
	 UINT32        requestedNameSpaceType,
    UINT32        *dataStreamAttribute);

CCODE NWGetDataStreamName(
	 int	volNum, 
    UINT32 dataStream,
    STRING dataStreamName);

CCODE NWGetDataStreamNumberAndSizes(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        *retInfo);

CCODE NWGetDirPath(
    UINT32       clientConnID,
    UINT32       nameSpace,
    NWSMDIR_HANDLE dirHandle,
    STRING       dirPath);

CCODE NWGetEntryIndex(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32       *primaryEntryIndex,
    UINT32       *nameSpaceEntryIndex,
    UINT16       *volNum);

CCODE NWGetFileSize(
    UINT32        clientConnID,
    NWSMFILE_HANDLE fileHandle,
    UINT32       *fileSize);

CCODE NWGetNameSpaceName(
	 int	volNum,
    UINT32 nameSpace,
    STRING nameSpaceName);

CCODE NWGetNameSpaceEntryName(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        nameSpace,
    STRING        entryName);

CCODE NWGetNameSpaceEntryInfo(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        nameSpace,
    UINT32        returnMask,
    void         *nameSpaceInfo);

CCODE NWGetVolSupportedNameSpaces(
    UINT16  volNum,
    UINT32 *supportedNameSpaces,
    void   *nameSpaceList);

CCODE NWGetVolSupportedDataStreams(
    UINT16  volNum,
    UINT32 *supportedDataStreams,
    void   *dataStreamList);

CCODE NWInitDirScan(
    NWENTRY_SPEC    *dirEntry,
    UINT16           searchAttributes,
    NWSCAN_SEQUENCE *sequence,
    UINT32           returnMask);

CCODE NWInitFileScan(
    NWENTRY_SPEC    *fileEntry,
    UINT16           searchAttributes,
    NWSCAN_SEQUENCE *sequence,
    UINT32           returnMask);

CCODE NWInitEAScan(
    NWEA_ENTRY_SPEC    *EAEntry,
    NWEA_SCAN_SEQUENCE *sequence);

CCODE NWInitNameSpaceInfoScan(
    NWENTRY_SPEC    *entry,
    UINT16           searchAttributes,
    UINT32           nameSpace,
    NWNS_ENTRY_INFO *sequence,
    UINT32          *infoSize);

CCODE NWLogPhysicalRecord(
    UINT32        clientConnID,
    NWSMFILE_HANDLE fileHandle,
    UINT16        lockDirective,
    UINT32        recordStartOffset,
    UINT32        recordLength);

CCODE NWMapDirHandleToVolName(
    UINT32       clientConnID,
    UINT32       nameSpace,
    NWSMDIR_HANDLE dirHandle,
    STRING       volName);

CCODE NWMoveDirEntry(
    NWENTRY_SPEC *sourceDirEntry,
    UINT16        searchAttributes,
    NWENTRY_SPEC *destDirEntry);

CCODE NWMoveFileEntry(
    NWENTRY_SPEC *sourceFileEntry,
    UINT16        searchAttributes,
    NWENTRY_SPEC *destFileEntry);

CCODE NWOpenEA(
    NWEA_ENTRY_SPEC *EAEntry,
    NWEA_HANDLE     *EAHandle);

CCODE NWOpenFile(
    NWENTRY_SPEC *fileEntry,
    UINT16        fileActionFlags,
    UINT16        attributes,
    UINT16        accessRights,
    NWSMFILE_HANDLE fileHandle);

CCODE NWOpenDataStream(
    NWENTRY_SPEC *fileEntry,
    UINT16        searchAttributes,
    UINT32        dataStream,
    UINT16        accessRights,
    NWSMFILE_HANDLE fileHandle);

CCODE NWPurgeSalvageableFile(
    NWENTRY_SPEC *dirEntry,
    INT32         sequence);

CCODE NWReadEA(
    NWEA_ENTRY_SPEC *EAEntry,
    UINT32          *readPos,
    UINT32           receiveBufferSize,
    void            *dataBuffer,
    UINT32          *EASize,
    UINT32          *accessFlags,
    UINT32          *bytesRead);

CCODE NWReadFile(
    UINT32        clientConnID,
    NWSMFILE_HANDLE fileHandle,
    UINT32        *readPosition,
    UINT32        count,
    void         *dataBuffer,
    UINT32        *bytesRead);

CCODE NWRecoverSalvageableFile(
    NWENTRY_SPEC *dirEntry,
    INT32         sequence,
    STRING        newFileName);

CCODE NWScanEntryForTrustees(
    NWENTRY_SPEC            *entry,
    UINT16                   searchAttributes,
    NWSCAN_TRUSTEE_SEQUENCE *sequence,
    UINT32                  *trusteeID,
    UINT16                  *trusteeRights);

CCODE NWScanNextDirEntry(
    NWSCAN_SEQUENCE  *sequence,
    NWDIR_ENTRY_INFO *dirInfo);

CCODE NWScanNextEAInfo(
    NWEA_SCAN_SEQUENCE *sequence,
    UINT32             *numOfEAs,
    UINT32             *totalEAsSize,
    UINT32             *totalEAsNameSize,
    NWEA_INFO          *EAInfo);

CCODE NWScanNextFileEntry(
    NWSCAN_SEQUENCE   *sequence,
    NWFILE_ENTRY_INFO *fileInfo);

CCODE NWGetNameSpaceFixedInfo(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        nameSpace,
    UINT32        returnMask,
    UINT32        bufferSize,
    void         *info);

CCODE NWGetNameSpaceHugeInfo(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        nameSpace,
    UINT32        returnMask,
    UINT32        bufferSize,
    UINT32       *infoSize,
    void         *info);

CCODE NWScanNextNameSpaceInfo(
    NWNS_ENTRY_INFO  *sequence,
    UINT32            bufferSize,
    UINT32           *infoSize,
    void             *info);

CCODE NWScanSalvageableFiles(
    NWENTRY_SPEC      *dirEntry,
    INT32             *sequence,
    UINT32             returnMask,
    NWFILE_ENTRY_INFO *fileInfo);

CCODE NWScanSparseFileBitMap(
    UINT32        clientConnID,
    NWSMFILE_HANDLE fileHandle,
    UINT32       *startingOffset,
    UINT32        bufferSize,
    UINT32       *blockSize,
    UINT8        *bitMap);

CCODE NWSetDirHandle(
    NWENTRY_SPEC *dirEntry,
    NWSMDIR_HANDLE  dirHandle);

CCODE NWSetDirEntryInfo(
    NWENTRY_SPEC     *dirEntry,
    UINT16            searchAttributes,
    UINT32            setMask,
    NWDIR_ENTRY_INFO *dirInfo);

CCODE NWSetFileEntryInfo(
    NWENTRY_SPEC      *fileEntry,
    UINT16             searchAttributes,
    UINT32             setMask,
    NWFILE_ENTRY_INFO *fileInfo);

CCODE NWSetNameSpaceEntryName(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        nameSpace,
    STRING        entryName);

CCODE NWSetNameSpaceFixedInfo(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        nameSpace,
    UINT32          modifyMask,
    UINT32        infoSize,
    void         *info);

CCODE NWSetNameSpaceHugeInfo(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        nameSpace,
    UINT32          modifyMask,
    UINT32        infoSize,
    void         *info);

CCODE NWSetNameSpaceSpecificInfo(
    NWENTRY_SPEC *entry,
    UINT16        searchAttributes,
    UINT32        nameSpace,
    INT32        *sequence,
    UINT32        infoSize,
    void         *info);

CCODE NWWriteEA(
    NWEA_ENTRY_SPEC *EAEntry,
    UINT32          *writePosition,
    UINT32           accessFlags,
    UINT32           totalEASize,
    UINT32           dataSize,
    void            *dataBuffer);

CCODE NWWriteFile(
    UINT32        clientConnID,
    NWSMFILE_HANDLE fileHandle,
    UINT32       *writePosition,
    UINT32        count,
    void         *dataBuffer,
    UINT32       *bytesWritten);

CCODE NWUnlockPhysicalRecord(
    UINT32        clientConnID,
    NWSMFILE_HANDLE fileHandle,
    UINT32        recordStartOffset,
    UINT32        recordLength);

void _NWFillEAStruct(
    PACKET_PTR_U    *packetPtr,
    NWEA_ENTRY_SPEC *EAEntry,
    UINT16           flags);

void _NWFillNWDIR_ENTRY_INFO(
    PACKET_PTR_U      packetPtr,
    NWDIR_ENTRY_INFO *fileInfo,
    UINT32            returnMask);

void _NWFillNWFILE_ENTRY_INFO(
    PACKET_PTR_U       packetPtr,
    NWFILE_ENTRY_INFO *fileInfo,
    UINT32             returnMask);

CCODE _NWFillHandlePathStruct(
    NWENTRY_SPEC  *entry,
    NWHANDLE_PATH *pathInfo);

STRING _NWFindDelimiter(
    STRING path,
    UINT32 nameSpace);

CCODE _NWStoreAsComponentPath(
    STRING componentPath,
    UINT8  nameSpace,
    STRING path);

void _NWStoreAsWildPath(
    STRING wildPath,
    UINT8  nameSpace,
    STRING path);

CCODE NWGetVolumeDirRestrictions(
/* START BLOCK COMMENT
**		UINT32 clientConnID,
END BLOCK COMMENT */
	NWENTRY_SPEC *entry,
	UINT32 volNum,
	UINT32 *dirRestrictions);

CCODE NWSetVolumeDirRestrictions(
/* START BLOCK COMMENT
**		UINT32 clientConnID,
END BLOCK COMMENT */
	NWENTRY_SPEC *entry,
	UINT32 volNum,
	UINT32 dirRestrictions);

#define _nwsms_
#define _NWTYPES_H_INCLUDED_
#endif

