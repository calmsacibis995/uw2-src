/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/oscalls.h	1.1"
/****************************************************************************
 *
 * Program Name:  Storage Management Services (NWSMS Lib)
 *
 * Filename:      OSCALLS.H
 *
 * Date Created:  25 APRIL 1990
 *
 * Version:       3.11
 *
 * Files used:    
 *
 * Date Modified: 
 *
 * Modifications: 
 *
 ****************************************************************************/

#if !defined(_oscalls_)
#define _oscalls_

struct	ModifyStructure
{
	UINT8 *MModifyName;
	UINT32 MFileAttributes;
	UINT32 MFileAttributesMask;
	UINT16 MCreateDate;
	UINT16 MCreateTime;
	UINT32 MOwnerID;
	UINT16 MLastArchivedDate;
	UINT16 MLastArchivedTime;
	UINT32 MLastArchivedID;
	UINT16 MLastUpdatedDate;	/* also last modified date and time. */
	UINT16 MLastUpdatedTime;
	UINT32 MLastUpdatedID;
	UINT16 MLastAccessedDate;
	UINT16 MInheritanceGrantMask;
	UINT16 MInheritanceRevokeMask;
	UINT32 MMaximumSpace;
};

#define	MMaximumSpaceBit	0x2000

typedef struct
{
    int   subdirectory;
    long  fileAttributes; /* The DOS dir attributes are used for OS/2 */
    UINT8 uniqueID;
    UINT8 flags;
    UINT8 nameSpace;
    UINT8 fileNameLength;
    UINT8 fileName[80];
    long  extendedSpace;
    UINT8 extantsUsed;
    UINT8 lengthData;
    UINT8 reserved[18];
    long  deletedBlockSequenceNumber;
    long  primaryEntry;
    long  nameSpaceEntry; 
} NWDIR_STRUCT;


/*...used for log physical record...*/
#define NWNO_REPLY_BIT 0x80

typedef struct
{
   UINT32 attributes;
   UINT16 creationDate;
   UINT16 creationTime;
   UINT32 creatorID;
   UINT16 modifyDate;
   UINT16 modifyTime;
   UINT32 modifierID;
   UINT16 archiveDate;
   UINT16 archiveTime;
   UINT32 archiverID;
   UINT16 lastAccessDate;
   UINT16 grantMask;
   UINT16 revokeMask;
   UINT32 maximumSpace;
} NWMODIFY_INFO;

/****************************************************************************/
extern char *DataStreamName[NWNUM_DATA_STREAMS];
extern char *NameSpaceName[NWNUM_NAME_SPACES];
extern long  NumberOfDefinedNameSpaces[NWMAX_VOLUMES];
extern long  MapDataStreamToNameSpace[NWNUM_DATA_STREAMS];
extern long  MapPositionToNameSpace[NWMAX_VOLUMES][NWNUM_NAME_SPACES];

long CFindD(
    long  value,
    void *address,
    long  numberOfBytes);

long CloseEAHandle(
    long station,
    long task,
    long EAHandle);

long CloseFile(
    long station,
    long task,
    long handle);

long ConvertPathString(
    long  stationNumber,
    UINT8 dirHandle,
    char *modifierString,
    long *volNum,
    long *pathBase,
    char *path,
    long *pathCount);

long DestroyDirectoryHandle(
    long station,
    long base);

long DupEA(
    long station,
    long task,
    long srcTypeFlag,
    long srcVolume,
    long srcHandle,
    long dstTypeFlag,
    long dstVolume,
    long dstHandle,
    long *dupCount,
    long *dupData,
    long *DupKey);

long EnumEA(
    long    station,
    long    task,
    long    EAHandle,
    long    infoLevel,
    long    startPosition,
    long    inspectSize,
    long    keySize,
    char   *key,
    void   *enumBuf,
    long   *enumBufSize,
    UINT16 *nextPos,
    long   *totalEADataSize,
    long   *totalEAs,
    UINT16 *currentEAs,
    long   *totalEAsKeySize,
    long    maxDataSize);

long GenNSAddTrustees(
    long           station,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT16         searchAttributes,
    UINT16         trusteeRightsMask,
    UINT16         objectIDCount,
#if defined(NETWARE_V320) || defined(NETWARE_V312)
    NWTRUSTEES    *trustees);
#else
    NWTRUSTEES    *trusteeInfo[]);
#endif

long GenNSAllocDirHandle(
    long           station,
    long           task,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT16         allocateMode,
    UINT8         *newDirHandle,
    UINT8         *volumeNumber);

long GenNSContinueFileSearch(
    long   station,
    UINT8  nameSpace,
    UINT8  dataStream,
    UINT16 searchAttributes,
    UINT32 returnMask,
    UINT8  volNum,
    long   dirNum,
    long   entryNum,
    char  *searchPattern,
    long  *newEntryNum,
    void  *netwareInfo,
    char  *fileName);

long GenNSDeleteTrustees(
    long           station,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT16         objectIDCount,
#if defined(NETWARE_V320) || defined(NETWARE_V312)
    NWTRUSTEES    *trustees);
#else
    NWTRUSTEES    *trusteeInfo[]);
#endif

long GenNSErase(
    long           station,
    long           task,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT16         searchAttributes);

long GenNSGetPathString(
    long   station,
    UINT8  nameSpace,
    UINT8  dirHandle,
    UINT8 *path);

long GenNSGetDirBase(
    long           station,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT16         searchAttributes,
    UINT32        *entryIndex,
    UINT32        *primaryEntryIndex,
    UINT8         *volNum);

long GenNSGetNameSpaceList(
    UINT16 *numberNSLoaded,
    void   *loadedList);

long GenNSGetSpecificInfo(
    long   station,
    UINT8  sourceNameSpace,
    UINT8  destNameSpace,
    UINT8  volNum,
    UINT32 entryIndex,
    UINT32 returnMask,
    void  *data);

long GenNSInitFileSearch(
    long           station,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT8         *volNum,
    long          *dirNum,
    long          *entryNum);

long GenNSModifyInfo(
    long           Station,
    long           task,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT16         searchAttributes,
    UINT32         setMask,
    NWMODIFY_INFO *modifyInfo);

long GenNSModifySpecificInfo(
    long   station,
    long   task,
    UINT8  sourceNameSpace,
    UINT8  destNameSpace,
    UINT8  volNum,
    UINT32 entryIndex,
    UINT32 modifyMask,
    UINT16 infoLength,
    void  *info);

long GenNSModifySpecificInfo(
    long   station,
    long   task,
    UINT8  sourceNameSpace,
    UINT8  destNameSpace,
    UINT8  volNum,
    UINT32 entryIndex,
    UINT32 modifyMask,
    UINT16 modifyLength,
    UINT8 *data);

long GenNSObtainInfo(
    long           station,
    NWHANDLE_PATH *pathInfo,
    UINT8          srcNameSpace,
    UINT8          destNameSpace,
    UINT16         searchAttributes,
    UINT32         returnMask,
    void          *netwareInfo,
    char          *name);
long GenNSOpenCreate(
    long           Station,
    long           task,
    NWHANDLE_PATH *NPathInfo,
    UINT8          NameSpace,
    UINT8          DataStream,
    UINT8          openCreateFlags,
    UINT16         searchAttributes,
    UINT16         accessRights,
    UINT16         createAttributes,
    UINT32         returnMask,
    UINT32        *fileHandle,
    UINT8         *openCreateAction,
    void          *netwareInfo,
    char          *fileName);

long GenNSOpenCreateFile(
    long           Station,
    long           task,
    NWHANDLE_PATH *NPathInfo,
    UINT8          NameSpace,
    UINT8          openCreateFlags,
    UINT16         searchAttributes,
    UINT16         accessRights,
    UINT16         createAttributes,
    UINT32         returnMask,
    UINT32        *fileHandle,
    UINT8         *openCreateAction,
    void          *netwareInfo,
    char          *fileName);

long GenNSPurgeSalvageableFile(
    long           station,
    NWHANDLE_PATH *pathInfo,
    long           nameSpace,
    long           sequence);

long GenNSQueryNameSpaceInfo(
    UINT8   nameSpace,
    UINT8   volNum,
    UINT32 *fixedFieldsMask,
    UINT32 *variableFieldsMask,
    UINT32 *hugeFieldsMask,
    UINT16 *fixedBitsDefined,
    UINT16 *variableBitsDefined,
    UINT16 *hugeBitsDefined,
    UINT32  fieldLengthsTable[32]);

long GenNSRecoverSalvageableFile(
    long           station,
    NWHANDLE_PATH *pathInfo,
    long           nameSpace,
    long           sequence,
    char          *newFileName);

long GenNSRename(
    long           station,
    long           task,
    NWHANDLE_PATH *srcPathInfo,
    NWHANDLE_PATH *destPathInfo,
    UINT8          nameSpace, 
    UINT16         searchAttributes);

long GenNSScanSalvageableFiles(
    long           station,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT8          dataStream,
    long           returnMask,
    long           sequence,
    long          *nextSequence,
    long          *deleteDateAndTime,
    long          *deletorID,
    void          *netwareInfo,
    char          *fileName);

long GenNSScanForTrustees(
    long           station,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT16         searchAttributes,
    long           sequence,
    long          *nextSequence,
    UINT16        *count,
#if defined(NETWARE_V320) || defined(NETWARE_V312)
    NWTRUSTEES    *trustees);
#else
    NWTRUSTEES    *trustees[NWMAX_TRUSTEES]);
#endif

long GenNSSetDirHandle(
    long           station,
    NWHANDLE_PATH *pathInfo,
    UINT8          nameSpace,
    UINT8          dataStream,
    UINT8          dirHandle);

long GetEntryFromPathStringBase(
    long   station,
    long   volume,
    long   pathBase,
    char  *pathString,
    long   pathCount,
    long   sourceNameSpace,
    long   destNameSpace,
    void **dirInfo,
    long  *dirNum);

long GetFileHoles(
    long   station,
    long   fileHandle,
    long   startingOffset,
    long   numberOfBlocks,
    UINT8 *bitMap,
    long  *blockSize);

long GetFileSize(
    long   station,
    UINT32 fileHandle,
    long  *fileSize);

long GetHugeInformation(
    long   station,
    long   task,
    UINT8  nameSpace,
    UINT8  volNum,
    UINT32 entryIndex,
    UINT32 returnMask,
    UINT8 *stateInfo,
    UINT8 *data,
    long  *dataLength,
    UINT8 *nextStateInfo);

long LogPRec(
    UINT16 station,
    UINT32 fileHandle,
    long   start,
    long   length,
    long   flags,
    long   waitTime,
    void  *reply);

long ModifyDirectoryEntry(
		UINT32 Station,
		UINT32 Task,
		UINT32 Volume,
		UINT32 PathBase,
		UINT8 *PathString,
		UINT32 PathCount,
		UINT32 NameSpace,
		UINT32 MatchBits,
		UINT32 TargetNameSpace,
		struct ModifyStructure *ModifyVector,
		UINT32 ModifyBits,
		UINT32 AllowWildCardsFlag);

long OpenEAHandle(
    long  station,
    long  task,
    long  openFlags,
    long  flags,
    long  volume,
    long  handle,
    long *EAHandle);

long OpenFile(
    long    station,
    long    task,
    long    volume,
    long    pathBase,
    char   *path,
    long    pathCount,
    long    nameSpace,
    long    MatchBits,
    long    accessRights,
    UINT8   dataStream,
    UINT32 *handle,
    UINT32 *entryIndex,
    void  **directoryEntry);

long ReleasePRec(
    UINT16 station,
    long   fileHandle,
    long   start,
    long   length,
    long   clrFlag);

long ReadEAData(
    long    station,
    long    task,
    long    eaHandle,
    long    startPosition,
    long    inspectSize,
    long    keySize,
    char   *key,
    void   *dataBuffer,
    long   *EASize,
    UINT16 *dataSize,
    long   *accessFlags,
    long    maximumDataSize);

long ReadFile(
    long  station,
    long  handle,
    long  startingOffset,
    long  bytesToRead,
    long *bytesRead,
    void *buffer);

long ReturnDirectorySpaceRestrictions(
    long   station,
    long   volume,
    long   entryIndex,
    UINT8 *rInfo,
    long   rInfoSize,
    long  *answerSize);

long SetHugeInformation(
    long   station,
    long   task,
    UINT8  nameSpace,
    UINT8  volNum,
    UINT32 entryIndex,
    UINT32 modifyMask,
    UINT8 *stateInfo,
    long   dataLength,
    UINT8 *data,
    UINT8 *nextStateInfo,
    long  *hugeDataUsed);

long WriteEAData(
    long  station,
    long  task,
    long  eaHandle,
    long  totalDataLength,
    long  start,
    long  accessFlag,
    long  keySize,
    char *keyBuf,
    long  dataSize,
    void *dataBuffer,
    long *bytesWritten);

long WriteFile(
    long   station,
    UINT32 handle,
    long   start,
    long   bytesToWrite,
    void  *buffer);

#endif

/****************************************************************************/
/****************************************************************************/

