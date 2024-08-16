/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/smsutapi.h	1.2"

#if !defined(_SMSUTAPI_H_INCLUDED)
#define _SMSUTAPI_H_INCLUDED

#if !defined (_CLIB_HDRS_INCLUDED)
#define _CLIB_HDRS_INCLUDED
#if defined(__INLINE_FUNCTIONS__)
#undef __INLINE_FUNCTIONS__
#endif

#include <string.h>

#include <stdlib.h>
#if (defined(NLM) || defined(__WATCOMC__)) && (defined(NETWARE_V320) || defined(V2X_TSA_ON_40))
#include <nwlocale.h>
#endif
#endif

#if defined(DEBUG_CODE)
        void  EnterDebugger(void);
#else
#define ROUTINE
#endif

#include <smstypes.h>
#include <smsdefns.h>
#include <smsuterr.h>

/* CRC Flags */
#define CRC_NO    0
#define CRC_YES   1
#define CRC_LATER 2

/* Wild card patterns */
#define ASTERISK  '\x2A'
#define QUESTION  '\x3F'
#define SPERIOD   '\xAE' /*period with parity bit set*/
#define SASTERISK '\xAA' /*asterisk with parity bit set*/
#define SQUESTION '\xBF' /*question mark with parity set*/

#define NWSM_MATCH_SUCCESSFUL    0
#define NWSM_MATCH_TO_END        1
#define NWSM_MATCH_UNSUCCESSFUL -1




#define TARGET_SERVICE_AGENT            0x2E
#define GET_ADDRESS                        (void *)1
#define GET_ADDRESS_NEED_FILE_OFFSET    (void *)2

#define SetUINT64Value(a, b)    (*(UINT32 *)(a) = \
        (((b) is 1) ? (1) : (((b) is 2) ? (0x100) : \
        (((b) is 4) ? (0x10000L) : (0)))), \
        *((UINT32 *)(a) + 1) = ((*(UINT32 *)(a)) ? (0) : (1)))

#define SMSLIB_H

#define NWSM_BEGIN                        0x10000000L
#define NWSM_END                        0x20000000L
#define NWSM_SKIP_FIELD                    0x30000000L

/* nameSpaceType definitions, others are defined in OS, i.e. DOS, MAC etc */
#define NWSM_ALL_NAME_SPACES            0xFFFFFFFFL
#define NWSM_TSA_DEFINED_RESOURCE_TYPE  0xFFFFFFFEL
#define NWSM_CREATOR_NAME_SPACE            0xFFFFFFFDL
#define NWSM_DIRECTORY_NAME_SPACE        0xFFFFFFFCL

/* NetWare Name Space Defines */
#if defined(NLM) || defined(__WATCOMC__)
#define DOSNameSpace    0
#define MACNameSpace    1
#define NFSNameSpace    2
#define FTAMNameSpace   3
#define OS2NameSpace    4
#if defined(NCP)
#define MaxNameSpaces   1
#else
#define MaxNameSpaces   4
#endif
#else
#define DOSNameSpace    0L
#define MACNameSpace    1L
#define NFSNameSpace    2L
#define FTAMNameSpace   3L
#define OS2NameSpace    4L
#if defined(NCP)
#define MaxNameSpaces   1L
#else
#define MaxNameSpaces   4L
#endif
#endif

#define PRIMARY_DATA_STREAM 0
#define MAC_RESOURCE_FORK   1
#define FTAM_DATA_STREAM    2
#define MAX_DATA_STREAMS    3

#define NameSpaceName(n)   _GetMessage(((n) <= MaxNameSpaces) ?\
                ((n) + DOS_NAME_SPACE_NAME) : INVALID_NAME_SPACE_NAME)
#define DataStreamName(n)  _GetMessage(((n) <= MAX_DATA_STREAMS) ?\
                ((n) + PRIMARY_DATA_STREAM_NAME) : INVALID_DATA_STREAM_NAME)

/*  Miscellaneous defines */
#define STANDARD_INCREMENT 1024

#define MIN_BLANK_SPACE_SECTION_SIZE 14
/*  List structures */
    typedef struct NWSM_LIST_STRUCT
    {
/* 0x00 */ UINT8  marked;
/* 0x01 */ struct NWSM_LIST_STRUCT *prev;
/* 0x05 */ struct NWSM_LIST_STRUCT *next;
/* 0x09 */ void  *otherInfo;
/* 0x0D */ BUFFER text[1];
    } NWSM_LIST;

    typedef struct
    {
/* 0x00 */ NWSM_LIST *head;
/* 0x04 */ NWSM_LIST *tail;
/* 0x08 */ int (*sortProc)(char *,char *);
/* 0x0C */ void (*freeProcedure)(void *memoryPointer);
    } NWSM_LIST_PTR;

    typedef struct
    {
        UINT32  nameSpaceType;
        UINT32  selectionType;
        UINT16  count;
        UINT16  HUGE *namePositions;
        UINT16  HUGE *separatorPositions;
        UINT16  nameLength;
        STRING  name;
    } NWSM_DATA_SET_NAME;

//    Parser defines, etc.
/*    Bit Field defines */
#define SMDF_BIT_ONE    0xC1
#define SMDF_BIT_TWO    0xC2
#define SMDF_BIT_THREE    0xC4
#define SMDF_BIT_FOUR    0xC8
#define SMDF_BIT_FIVE    0xD0
#define SMDF_BIT_SIX    0xE0

/*    Misc. defines */
#define SMDF_MIN_PARSER_BUFFER    13    // The minimum parsr buffer size is 13,
                                        // this provides space for the maximum
                                        // size header, (13 bytes, 4 for the
                                        // Fid and 9 for the Size) is present.

/*    SMDF Compare Macros, these macros compare a UINT64 (a) with a UINT32 (b)
    and return TRUE if (a op b) where op is EQ (Equal), GE (Greater Than
    or Equal), GT (Greater Than), LE (Less Than or Equal), or LT (Less Than) */

#define SMDF_EQ(a, b)    (!((UINT64 *)a)->v[1] && ((UINT64 *)a)->v[0] == b)
#define SMDF_GE(a, b)    (((UINT64 *)a)->v[1] || ((UINT64 *)a)->v[0] >= b)
#define SMDF_GT(a, b)    (((UINT64 *)a)->v[1] || ((UINT64 *)a)->v[0] > b)
#define SMDF_LE(a, b)    (!((UINT64 *)a)->v[1] && ((UINT64 *)a)->v[0] <= b)
#define SMDF_LT(a, b)    (!((UINT64 *)a)->v[1] && ((UINT64 *)a)->v[0] < b)

/*    SMDF Misc. Macros */
#define SMDFFixedFid(fid)    ((longFid) ? (((fid) AND 0xF000) is 0xF000)\
                : ((fid) AND 0x40))

#define SMDFFixedSize(fid) (1L << ((longFid) ? (fid & 0x0F00)>>8 : fid & 0x0F))

#define SMDFPutUINT64(a, val) (((UINT64 *)a)->v[1] = 0, ((UINT64 *)a)->v[0] = (val))

#define SMDFBit1IsSet(v)    (((v) & SMDF_BIT_ONE) is SMDF_BIT_ONE)
#define SMDFBit2IsSet(v)    (((v) & SMDF_BIT_TWO) is SMDF_BIT_TWO)
#define SMDFBit3IsSet(v)    (((v) & SMDF_BIT_THREE) is SMDF_BIT_THREE)
#define SMDFBit4IsSet(v)    (((v) & SMDF_BIT_FOUR) is SMDF_BIT_FOUR)
#define SMDFBit5IsSet(v)    (((v) & SMDF_BIT_FIVE) is SMDF_BIT_FIVE)
#define SMDFBit6IsSet(v)    (((v) & SMDF_BIT_SIX) is SMDF_BIT_SIX)

#define SMDFSetBit1(v)    ((v) |= SMDF_BIT_ONE)
#define SMDFSetBit2(v)    ((v) |= SMDF_BIT_TWO)
#define SMDFSetBit3(v)    ((v) |= SMDF_BIT_THREE)
#define SMDFSetBit4(v)    ((v) |= SMDF_BIT_FOUR)
#define SMDFSetBit5(v)    ((v) |= SMDF_BIT_FIVE)
#define SMDFSetBit6(v)    ((v) |= SMDF_BIT_SIX)

#define SMDFSizeOfFID(d) ((d & 0xFF000000) ? 4 : ((d & 0xFF0000) ? 3 : \
            ((d & 0xFF00) ? 2 : 1))) 

#define SMDFSizeOfUINT32Data(d)    ((d & 0xFFFF0000) ? 4 : ((d & 0xFF00) ? 2 : 1))

#define SMDFSizeOfUINT32Data0(d)    ((d & 0xFF000000) ? 4 : \
                ((d & 0xFF0000) ? 3 : ((d & 0xFF00) ? 2 : 1))) 

#define SMDFSizeOfUINT64Data(d)    ((d.v[1]) ? 8 :\
                ((d.v[0] & 0xFFFF0000) ? 4 :\
                ((d.v[0] & 0xFF00) ? 2 :\
                ((d.v[0] & 0xFF) ? 1 : 0))))

#define SMDFSizeOfFieldData(d, m)    (m = 0, ((d.v[1]) ? (m = 0x83, 8) :\
                ((d.v[0] & 0xFFFF0000) ? (m = 0x82, 4) :\
                ((d.v[0] & 0xFF00) ? (m = 0x81, 2) :\
                ((d.v[0] & 0x80) ? (m = 0x80, 1) : 0)))))

#define SMDFZeroUINT64(a)    (((UINT64 *)a)->v[0] = ((UINT64 *)a)->v[1] = 0)

    typedef struct
    {
/* 0x00 */ UINT32    fid;
/* 0x04 */ UINT64    dataSize;
/* 0x0C */ void   *data;
/* 0x10 */ UINT32    bytesTransfered;
/* 0x14 */ UINT64    dataOverflow;
    } SMDF_FIELD_DATA;

    typedef struct
    {
/* 0x00 */ SMDF_FIELD_DATA field;
/* 0x1C */ UINT32    sizeOfData;
/* 0x20 */ void   *addressOfData;
/* 0x24 */ UINT8    dataSizeMap;
/* 0x25 */ UINT8    reserved[3];
    } NWSM_FIELD_TABLE_DATA;

    typedef struct
    {
/* 0x00 */ UINT32    fid;
/* 0x04 */ void   *data;
/* 0x08 */ UINT32    dataSize; /* The size of the data variable passed in */
/* 0x0C */ NWBOOLEAN found;
    } NWSM_GET_FIELDS_TABLE;

typedef struct
{
    UINT32                      mediaDateAndTime;        // Time of indivitual media
    UINT32                      mediaSetDateAndTime;    // Time of entire media set
    BUFFER                      mediaSetLabel[NWSM_MAX_MEDIA_LABEL_LEN];
    UINT32                      mediaNumber;
} NWSM_MEDIA_INFO;

typedef struct
{
    UINT32                      sessionDateAndTime;
    BUFFER                      sessionDescription[NWSM_MAX_DESCRIPTION_LEN];
    BUFFER                      softwareName[NWSM_MAX_SOFTWARE_NAME_LEN];
    BUFFER                      softwareType[NWSM_MAX_SOFTWARE_TYPE_LEN];
    BUFFER                      softwareVersion[NWSM_MAX_SOFTWARE_VER_LEN];
    BUFFER                      sourceName[NWSM_MAX_TARGET_SRVC_NAME_LEN];
    BUFFER                      sourceType[NWSM_MAX_TARGET_SRVC_TYPE_LEN];
    BUFFER                      sourceVersion[NWSM_MAX_TARGET_SRVC_VER_LEN];
} NWSM_SESSION_INFO;

typedef struct
{
    NWBOOLEAN                  isSubRecord;
    NWSM_DATA_SET_NAME_LIST     *dataSetName;
    NWSM_SCAN_INFORMATION     *scanInformation;
    UINT32                      headerSize;
    UINT32                      recordSize;
    UINT32                     *addressOfRecordSize;
    UINT32                     *addressForCRC;
    BUFFERPTR                  crcBegin;
    UINT32                      crcLength;
    UINT32                      archiveDateAndTime;
} NWSM_RECORD_HEADER_INFO;

/* vstring */
STRING_BUFFER *NWSMAllocString(
                    STRING_BUFFER    **path,
                    INT16              size);

/* list */
NWSM_LIST       *NWSMAppendToList(
                    NWSM_LIST_PTR     *list,
                    BUFFERPTR          text,
                    void             *otherInfo);

/* vstring */
STRING            NWSMCatString(
                    STRING_BUFFER    **dest,
                    void             *source,
                    INT16              srcLen);

/* datetime */
CCODE            NWSMCheckDateAndTimeRange(
                    UINT32              firstDateAndTime,
                    UINT32              lastDateAndTime,
                    UINT32              compareDateAndTime);

/* name */
CCODE            NWSMCloseName(
                    UINT32 HUGE         *handle);

/* vstring */
STRING            NWSMCopyString(
                    STRING_BUFFER    **dest,
                    void             *src,
                    INT16              srcLen);

/* list */
void            NWSMDestroyList(
                    NWSM_LIST_PTR     *list);

/* strip */
STRING            NWSMFixDOSPath(
                    STRING              path,
                    STRING              newPath);

/* free */
CCODE            NWSMFreeNameList(
                    NWSM_NAME_LIST    **list);

/* vstring */
void            NWSMFreeString(
                    STRING_BUFFER    **path);

/* gencrc */
UINT32            NWSMGenerateCRC(
                    UINT32            size,
                    UINT32            crc,
                    BUFFERPTR        ptr);

/* datetime */
UINT32            NWSMGetCurrentDateAndTime(void);

/* name */
CCODE            NWSMGetDataSetName(
                    void HUGE         *buffer,
                    UINT32              nameSpaceType,
                    NWSM_DATA_SET_NAME HUGE *name);

/* name */
CCODE            NWSMGetFirstName(
                    void HUGE         *buffer,
                    NWSM_DATA_SET_NAME HUGE *name,
                    UINT32 HUGE         *handle);

/* list */
NWSM_LIST       *NWSMGetListHead(
                    NWSM_LIST_PTR     *listPtr);

/* headers */
CCODE            NWSMGetMediaHeaderInfo(
        BUFFERPTR                      headerBuffer,
        UINT32                           headerBufferSize, 
        NWSM_MEDIA_INFO                 *mediaInfo);

/* name */
CCODE            NWSMGetNextName(
                    UINT32 HUGE         *handle,
                    NWSM_DATA_SET_NAME HUGE *name);

/* name */
CCODE            NWSMGetOneName(
                    void HUGE         *buffer,
                    NWSM_DATA_SET_NAME HUGE *name);

/* headers */
void            NWSMPadBlankSpace(
                    BUFFERPTR bufferPtr, 
                    UINT32 unusedSize);

/* strip */
STRING            NWSMGetPathChild(
                    UINT32              nameSpaceType,
                    STRING              path,
                    STRING             *child);

/* headers */
CCODE            NWSMGetRecordHeader(
        BUFFERPTR                     *buffer, 
        UINT32                         *bufferSize,
        NWSM_RECORD_HEADER_INFO         *recordHeaderInfo);

/* headers */
CCODE            NWSMGetSessionHeaderInfo(
        BUFFERPTR                       headerBuffer,
        UINT32                          headerBufferSize, 
        NWSM_SESSION_INFO             *sessionInfo);

/* strip */
STRING            NWSMGetVolume(
                    STRING             *ptr,
                    UINT32              nameSpaceType,
                    STRING              volume);

/* str */
void            NWSMGetTargetName(
                    STRING              source,
                    STRING              name,
                    STRING              type,
                    STRING              version);

/* list */
void            NWSMInitList(
                    NWSM_LIST_PTR     *listPtr,
                    void (*freeRoutine)(void *memoryPointer));

/* match (should use NWSMMatchName()) */
NWBOOLEAN        NWSMIsWild(
                    STRING              string);

/* strip */
NWBOOLEAN        NWSMLegalDOSName(
                    STRING              name);

/* pattern match */
int                NWSMMatchName(
                    UINT32              nameSpaceType, 
                    char                 *pattern, 
                    char                 *string);

/* datetime */
UINT16            NWSMPackDate(
                    UINT16              year,
                    UINT16              month,
                    UINT16              day);

/* datetime */
UINT32            NWSMPackDateTime(
                    UINT16              year,
                    UINT16              month,
                    UINT16              day,
                    UINT16              hours,
                    UINT16              minutes,
                    UINT16              seconds);

/* datetime */
UINT16            NWSMPackTime(
                    UINT16              hours,
                    UINT16              minutes,
                    UINT16              seconds);

/* strip */
NWBOOLEAN        NWSMPathIsNotFullyQualified(
                    UINT32              nameSpaceType,
                    STRING              path);

/* name */
CCODE            NWSMPutFirstName(
                    void HUGE        **buffer,
                    UINT32              nameSpaceType,
                    UINT32              selectionType,
                    NWBOOLEAN          reverseOrder,
                    STRING              sep1,
                    STRING              sep2,
                    STRING              name,
                    UINT32 HUGE         *handle);

/* name */
CCODE            NWSMPutNextName(
                    void HUGE        **buffer,
                    UINT32 HUGE         *handle,
                    UINT32              nameSpaceType,
                    UINT32              selectionType,
                    NWBOOLEAN          reverseOrder,
                    STRING              sep1,
                    STRING              sep2,
                    STRING              name);

/* name */
CCODE            NWSMPutOneName(
                    void HUGE        **buffer,
                    UINT32              nameSpaceType,
                    UINT32              selectionType,
                    NWBOOLEAN          reverseOrder,
                    STRING              sep1,
                    STRING              sep2,
                    STRING              name);

/* headers */
CCODE            NWSMSetMediaHeaderInfo(
        NWSM_MEDIA_INFO                 *mediaInfo, 
        BUFFERPTR                      buffer,
        UINT32                          bufferSize, 
        UINT32                         *headerSize);

/* headers */
CCODE            NWSMSetRecordHeader(
        BUFFERPTR                      *buffer, 
        UINT32                          *bufferSize,
        UINT32                          *bufferData,
        NWBOOLEAN                       setCRC, 
        NWSM_RECORD_HEADER_INFO         *recordHeaderInfo);

/* headers */
CCODE            NWSMSetSessionHeaderInfo(
        NWSM_SESSION_INFO             *sessionInfo, 
        BUFFERPTR                      buffer,
        UINT32                           bufferSize, 
        UINT32                         *headerSize);

/* str */
STRING            NWSMStr(
                    UINT8              n,
                    void             *dest,
                    void             *src1,
                    void             *src2,
                    ...);

/* strip */
CHAR            NWSMStripEndSeparator(
                    UINT32              nameSpaceType,
                    STRING              path,
                    CHAR            **separatorPos);

/* strip */
STRING            NWSMStripPathChild(
                    UINT32              nameSpaceType,
                    STRING              path,
                    STRING              child,
                    size_t              maxChildLength);

/* vstring */
STRING            NWSMCatStrings(
                    UINT8              numStrings,
                    STRING_BUFFER    **dest,
                    void             *src1,
                    void             *src2,
                    ...);

/* datetime */
void            NWSMUnPackDate(
                    UINT16              date,
                    UINT16             *year,
                    UINT16             *month,
                    UINT16             *day);

/* datetime */
void            NWSMUnPackDateTime(
                    UINT32              dateTime,
                    UINT16             *year,
                    UINT16             *month,
                    UINT16             *day,
                    UINT16             *hours,
                    UINT16             *minutes,
                    UINT16             *seconds);

/* datetime */
void            NWSMUnPackTime(
                    UINT16              time,
                    UINT16             *hours,
                    UINT16             *minutes,
                    UINT16             *seconds);

/* headers */
CCODE            NWSMUpdateRecordHeader(
        NWSM_RECORD_HEADER_INFO         *recordHeaderInfo);

/* match (should use NWSMMatchName()) */
CCODE            NWSMWildMatch(
                    STRING              pattern,
                    STRING              string);

/* parser */
CCODE            SMDFAddUINT64(                                    // sum = a + b
                    UINT64             *a, 
                    UINT64             *b, 
                    UINT64             *sum);

/* parser */
CCODE            SMDFDecrementUINT64(                            // a -= b
                    UINT64             *a, 
                    UINT32              b);

/* parser */
CCODE            SMDFGetNextField(
                    BUFFERPTR          buffer, 
                    UINT32              bufferSize, 
                    SMDF_FIELD_DATA     *field);

/* parser */
CCODE            SMDFGetFields(
                    UINT32              headFID, 
                    NWSM_GET_FIELDS_TABLE table[], 
                    BUFFERPTR         *buffer, 
                    UINT32             *bufferSize);

/* parser */
CCODE            SMDFGetUINT64(
                    UINT64             *a, 
                    UINT32             *v);

/* parser */
CCODE            SMDFIncrementUINT64(                            // a += b
                    UINT64             *a, 
                    UINT32              b);

/* parser */
CCODE            SMDFPutFields(
                    NWSM_FIELD_TABLE_DATA table[], 
                    BUFFERPTR         *buffer, 
                    UINT32             *bufferSize, 
                    UINT32              crcFlag);

#if defined(DEBUG_CODE)
/* parser */
void            SMDFPrintUINT64(
                    BUFFERPTR          buffer, 
                    UINT64             *data, 
                    UINT16              pad);
#endif

/* parser */
CCODE            SMDFPutNextField(
                    BUFFERPTR          buffer, 
                    UINT32              bufferSize, 
                    SMDF_FIELD_DATA     *field, 
                    UINT8              dataSizeMap, 
                    UINT32              sizeOfData);

/* parser */
CCODE            SMDFSetUINT32Data(
                    UINT64             *dataSize, 
                    BUFFERPTR          buffer, 
                    UINT32             *data);

/* parser */
CCODE            SMDFSetUINT64(
                    UINT64             *a, 
                    void             *buffer, 
                    UINT16              n);

/* parser */
CCODE            SMDFSubUINT64(                                    // dif = a - b
                    UINT64             *a, 
                    UINT64             *b, 
                    UINT64             *dif);
/* str */
int            StrNIEqu(                            // for Enabling
                     const char *__s1,
                     const char *__s2,
                     size_t      __n );
/* str */
int            StrIEqu(
                     const char *__s1, 
                     const char *__s2 );

#define ECMA_TIME_ZONE_UNKNOWN  -2047           // from the ECMA 167 spec

CCODE NWSMUnixTimeToECMA(
               UINT32        unixTime,
               ECMATime     *ECMATime,
               NWBOOLEAN32   local);

CCODE NWSMECMAToUnixTime(
               ECMATime     *ECMATime,
               UINT32       *unixTime,
               INT32        *tzOffset );

CCODE NWSMDOSTimeToECMA(
               UINT32        dosTime,
               ECMATime     *ECMATime );

CCODE NWSMECMAToDOSTime(
               ECMATime     *ECMATime,
               UINT32       *dosTime );

int   NWSMECMATimeCompare(
               ECMATime     *ECMATime1,
               ECMATime     *ECMATime2);


#ifdef UNIX
#ifndef SYSV
#include <strings.h>
extern void *memmove(void *, void *, size_t) ;
#endif
extern char *strrev(char *) ;
extern char *strupr(char *) ;
extern char *strlwr(char *) ;
#define stricmp       strcasecmp
#define strnicmp      strncasecmp
#endif

/* portlib */
UINT32 SwapUINT32(const UINT32 in) ;

/* portlib */
UINT16 SwapUINT16(const UINT16 in) ;
 
/* portlib */
UINT32 SwapUINT32p(const char *in) ;
 
/* portlib */
UINT16 SwapUINT16p(const char *in) ;

/* portlib */
char *SwapUINT32buf(char *in) ;
 
/* portlib */
char *SwapUINT16buf(char *in) ;

/* portlib */
UINT64 *SwapUINT64buf(UINT64 *in) ;

/* portlib */
UINT64 *SwapUINT64(const UINT64 *in) ;

/* portlib */
UINT64 *SwapUINT64p(const char *in) ;

/* portlib */
void *AllocateMemory(const unsigned int size) ;

/* portlib */
int FreeMemory(void *buffer) ;

#define MapECMATime(ecmaTime) \
	ecmaTime.typeAndTimeZone = SwapUINT16(ecmaTime.typeAndTimeZone); \
	ecmaTime.year = SwapUINT16(ecmaTime.year);

#ifdef SYSV
int strcasecmp(char *s1, char *s2) ;
#endif

#endif

