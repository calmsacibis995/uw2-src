/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/ncpsms.h	1.1"

#if !defined(_ncpsms_)
#define NCP_SMS_H

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
#define _NWTYPES_H_INCLUDED_
#endif

/* From "ncpe.h" */
#define    MAXFRAGMENTS 4

    struct FragmentStruct
    {
        UINT8 *fptr;
        UINT32 fsize;
    };

    typedef struct FragmentStruct NCP_FRAGMENT;
#define fragAddress    fptr
#define fragSize    fsize

    struct ECBStruct
    {
        struct ECBStruct *ECBLink;
        struct ECBStruct *ECBPrev;
        signed short Status;
        void       (*ECBESR)();
        UINT16       LogicalId;
        UINT8        ProtocolId[6];
        UINT32       BoardNum;
        UINT8        ImmAddress[6];
        UINT32       DriverWork;
        UINT8       *ESRSave;
        UINT16       Socket;
        UINT16       PWork;
        UINT32       PacketLen;
        UINT32       FragmentCount;
        struct FragmentStruct fragment[MAXFRAGMENTS];
    };

    struct PacketHeader
    {
        UINT16 checksum;
        UINT16 packetlen;
        UINT8  trancontrol;
        UINT8  hpackettype;
        UINT8  destnet[4];
        UINT8  desthost[6];
        UINT16 destsocket;
        UINT8  srcnet[4];
        UINT8  srchost[6];
        UINT16 srcsocket;
        UINT16 packettype;
        UINT8  seqnum;
        UINT8  lowslot;
        UINT8  task;
        UINT8  highslot;
        UINT8  code;
        UINT8  code2;
    };

    struct NCPJConnection
    {
        struct NCPJConnection *link;
        struct SessionStruct  *ConSession;
        UINT16   unique;
        UINT8    NetNumber[4];
        UINT8    NodeNumber[6];
        UINT16   SocketNumber;
        UINT16   RecvTimeOut;
        UINT8    ImmediateNode[6];
        UINT16   MaxTimeOut;
        UINT16   Slot;
        UINT32   Semaphore;
        struct   FileHandle *ConHandles;
        struct   SearchHandle *Searches;
        UINT16   MaxIOSize;
        UINT8    SeqNumber;
        UINT8    GlobalFlag;
        UINT16   NCPSocket;
        UINT16   QSocket;
        UINT16   BSocket;
        UINT32   MessageFlag;
        UINT32 (*CriticalError)();
        UINT8    ServerName[48];
        struct ECBStruct    SendECB;
        struct ECBStruct    ReplyECB;
        struct PacketHeader NCPOut;
        struct PacketHeader NCPIn;
        struct ECBStruct    WatchECB;
        struct PacketHeader Watch;
    };

    typedef struct NCPJConnection NCP_CONNECTION;

#include <nwsms.h>

/*...............................PROTOTYPES.................................*/
CCODE GetIDFromDirHandle(
    UINT32  clientConnID,
    UINT8   dirHandle,
    UINT32 *AFPEntryID,
    char   *child);

CCODE GetIDFromPathAndDirHandle(
    UINT32  clientConnID,
    UINT8   volNum,
    UINT8   dirHandle,
    UINT32  nameSpace,
    STRING  pathName,
    UINT32 *AFPEntryID,
    STRING  child);

CCODE GetPathFromDirHandle(
    NWENTRY_SPEC *entry,
    UINT8        *parent,
    UINT8        *child,
    NWSMDIR_HANDLE *newDirHandle);

CCODE SetTempDirEntry(
    NWENTRY_SPEC *entry,
    NWENTRY_SPEC *newEntry,
    STRING        child);

UINT32 DirPathRequest(
    UINT32 connectionID,
    void  *requestBuff,
    void  *returnBuff);

UINT32 NWNCPRequest(
    UINT32        connectionID,
    UINT16        requestCode,
    UINT16        requestFragCount,
    NCP_FRAGMENT *requestFragList,
    UINT16        replyFragCount,
    NCP_FRAGMENT *replyFragList);

UINT32 NWNCPLongSwap(
    UINT32 value);

UINT16 NWNCPWordSwap(
    UINT16 value);

UINT32 NWNCPIs386Supported(
    UINT32 connectionID);

UINT32 NWNCPIsAFPSupported(
    UINT32 currentConnectionID,
    UINT8  volumeNumber,
    UINT8 *macNSIsSupported,
    UINT8 *macDSIsSupported);

UINT32 NWNCPIsMacNSAndDSSupported386(
    UINT32 connectionID,
    UINT8  volumeNumber,
    UINT8 *macNSSupported,
    UINT8 *macDSSupported);

UINT32 NWNCPAFPSupported(
    UINT32 currentConnectionID);

UINT32 NWNCPGetNetWareVersion(
    UINT32 currentConnectionID,
    UINT8 *majorVersion,
    UINT8 *minorVersion,
    UINT8 *revision);

UINT32 NWNCPGetFileServerInformation(
    UINT32  currentConnectionID,
    UINT8  *serverName,
    UINT8  *majorVersion,
    UINT8  *minorVersion,
    UINT8  *revision,
    UINT16 *maxConnectionsSupported,
    UINT16 *peakConnectionsUsed,
    UINT16 *connectionsInUse,
    UINT16 *maxVolumesSupported,
    UINT8  *SFTLevel,
    UINT8  *TTSLevel);

UINT32 NWNCPGetVolumeDiskRestrictions(
    UINT32 connectionID,
    UINT8  volumeNumber,
    INT32  sequence,
    UINT8 *returnBuff);

UINT32 NCPRequest(
    struct  NCPJConnection *conn,
    UINT16  RequestCode,
    UINT16  sendfragments,
    void   *sendlist,
    UINT16  receivefragments,
    void   *receivelist,
    UINT16 *reclenret);

UINT32 NCPGetBinderyAccessLevel(
    UINT32  connection,
    UINT8  *accessLevel,
    UINT32 *objectID);

UINT32 InvertLong(
    UINT32 value);

UINT16 InvertShort(
    UINT16 value);

/* ncpe.h */ UINT32 NCPConnect(
    struct SessionStruct *sess,
    void    *servername,
    void    *user,
    int      nametype,
    void    *password,
    UINT32 (*CritErr)(),
    NCP_CONNECTION **connectionret);

/* ncpe.h */ void NCPSessionClose(
    struct SessionStruct *sess);

/* ncpe.h */ UINT32 NCPSessionInit(
    UINT32 ModuleHandle,
    struct SessionStruct **SessionRet);

#if !defined(_SMSUTAPI_H_INCLUDED)
/* smslib.h */ STRING NWSMStripPathChild(
    UINT32 nameSpaceType,
    STRING path,
    STRING child,
    size_t maxChildLength);
#endif
#define _ncpsms_
#endif

