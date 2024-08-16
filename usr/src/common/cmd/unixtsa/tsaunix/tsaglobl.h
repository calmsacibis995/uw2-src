/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tsaglobl.h	1.8"

/*

$Abstract: 
Global header file for tsaunix. Contains all the data structure 
definitions and prototypes.
$

*/

#ifndef _TSAGLOBL_H_INCLUDED
#define _TSAGLOBL_H_INCLUDED
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>

#ifdef SYSV
#include	<netdb.h>
#endif

#include <smsdefns.h>
#include <smsutapi.h>
#include <smsuterr.h>
#include <smstserr.h>
#include "tsamsgs.h"
#include <tsalib.h>
#include <i18n.h>


#define MAX_SELECTION_TYPES   3
 
#define FINAL_SEQUENCE  0xFFFFFFFFL
 
#define INCLUDE_EXCLUDE_DIRS  0
#define INCLUDE_EXCLUDE_FILES 1
#define INCLUDE_EXCLUDE_PATHS 2
 
 
#define MAX_SCAN_TYPES               32  
/* Following 13 scan types are reserved, some of which are not
   supported in unix */
#define EXCLUDE_SUBDIRECTORIES       0   
#define EXCLUDE_ARCHIVED_FILES       1   /*not in unixtsa*/
#define EXCLUDE_HIDDEN_FILES         2   /*not in unixtsa*/
#define EXCLUDE_HIDDEN_DIRS          3   /*not in unixtsa*/
#define EXCLUDE_SYSTEM_FILES         4   /*not in unixtsa*/
#define EXCLUDE_SYSTEM_DIRS          5   /*not in unixtsa*/
#define EXCLUDE_FILE_TRUSTES         6   /*not in unixtsa*/
#define EXCLUDE_DIRECTORY_TRUSTEES   7   /*not in unixtsa*/
#define EXCLUDE_BINDERY              8   /*not in unixtsa*/
#define EXCLUDE_VOLUME_RESTRICTIONS  9   /*not in unixtsa*/
#define EXCLUDE_SPACE_RESTRICTIONS   10   /*not in unixtsa*/
#define EXCLUDE_EXTENDED_ATTRIBUTES  11   /*not in unixtsa*/
#define EXCLUDE_NO_DATA_STREAMS      12  

/* unixtsa specific Scan Type definitions */
#define NWSM_EXCLUDE_SPECIAL_FILES	0x80000000	/* Bit 31 */
#define NWSM_INCLUDE_REMOVABLE_FS	0x40000000	/* Bit 30 */
 
#define VALID_BACKUP_DATA_SET          0x425F4856      /* VH-B */
#define VALID_CONNECTION               0x435F4856      /* VH-C */
#define VALID_SEQUENCE                 0x535F4856      /* VH-S */
#define VALID_SMFILE                   0x465F4856      /* VH-F */
#define VALID_RESTORE_DATA_SET_ALMOST  0x725F4856      /* VH-r */
#define VALID_RESTORE_DATA_SET         0x525F4856      /* VH-R */
#define VALID_SELECTION                0x4C5F4856      /* VH-L */
#define VALID_TEMPFILE                 0x545F4856      /* VH-T */
 
#define DATA_UNDERFLOW_SIZE           SMDF_MIN_PARSER_BUFFER * 2

 
/*      Backup/Restore state values */
#define STATE_HEADER                             1
#define STATE_FULL_PATHS                         2
#define STATE_CHARACTERISTICS                    3
#define STATE_NFS_CHARACTERISTICS                5
#define STATE_DATA_STREAMS                       6
#define STATE_TRAILER                           10
#define STATE_DONE                              11
#define STATE_GET_ENTIRE_SUBRECORD              12 /* Restore only */
 
/*      Special Sequences */
#define ERROR_LOG_SEQUENCE                      0x00000001
#define SKIPPED_DATA_SETS_SEQUENCE              0x00000002

/*      Defines for ExcludedBySelectionList */
#define DATA_SET_INCLUDED               0
#define DATA_SET_NOT_INCLUDED           1
#define DATA_SET_EXCLUDED               2
#define DATA_SET_NOT_IN_LIST            3
 
/*      Selection Types */
#define DEFINED_RESOURCES_TO_EXCLUDE    0x02
#define DEFINED_RESOURCES_TO_INCLUDE    0x03
#define DIR_TO_BE_EXCLUDED              0x04
#define DIR_TO_BE_INCLUDED              0x05
#define FILE_TO_BE_EXCLUDED             0x08
#define FILE_TO_BE_INCLUDED             0x09
#define FILE_WITH_PATH_TO_BE_EXCLUDED   0x10
#define FILE_WITH_PATH_TO_BE_INCLUDED   0x11
 
/*      DIRECTORY_STACK scanningModes */
#define SCANNING_FILES                  1
#define SCANNING_DIRS                   2
#define SCANNING_SINGLE_FILE            3
#define SCAN_COMPLETE                   4
 
/*      dataSetTypes */
#define TYPE_FILE                       1
#define TYPE_DIRECTORY                  2
#define TYPE_FILESYSTEM                 3
 
/* Buffer State's for PutFields */
#define BUFFER_STATE                    1
#define STATE_DATA_BUFFER_STATE         2
#define STATE_DATA_FILE_STATE         3
#define STATE_DATA_SYMLINK_STATE         4
 
/*      Miscellaneous Defines */
#define STATE_DATA_SIZE                 4096
#define TEMP_DATA_BUFFER_SIZE   0x10000
#define NWSM_BUFFER_OVERFLOW            0xFFDDEEAAL
 
 
#define ref(a)                                  a = a
#define PutDateAndTimeInBuffer(b, d, t)   (*((UINT16 *)(b) + 1) = (d), *((UINT16 *)(b)) = (t), *(b) = *(b))
#define TrashValidFlag(v)                       (v) = 0
 
#define NotValidOrEvenClose(v) (((v) & VALID_RESTORE_DATA_SET) != VALID_RESTORE_DATA_SET)
#define InitBufferState(dataSetHandle) (dataSetHandle->bufferState = BUFFER_STATE, \
	dataSetHandle->begin = dataSetHandle->addressOfDataOffsetField = NULL,\
	dataSetHandle->stateDataOffset = dataSetHandle->stateDataSize = \
	dataSetHandle->bufferUnderflowSize = \
	dataSetHandle->sizeOfDataOffsetField = 0)


#define	MAX_TARGET_SERVICE_NAME_LENGTH	MAXHOSTNAMELEN
#define	MAX_TARGET_SERVICE_TYPE_LENGTH	50
#define	MAX_TARGET_SERVICE_VER_LENGTH	50
#define	MAX_FULL_PATH_LENGTH		MAXPATHLEN
#define	MAX_RESOURCE_NAME_LENGTH	MAX_FULL_PATH_LENGTH
#define MAX_PASSWORD_LENGTH			50
#define MODULE_FILE_NAME_LEN		256
#define MODULE_OS_LEN	64
#define SI_SYSNAME_LEN	32
#define SI_RELEASE_LEN	32

#define	FIRST_SEPARATOR_SIZE		1
#define	SECOND_SEPARATOR_SIZE		1

#ifdef SYSV
#define S_ISSPECIAL(mode)	(S_ISCHR(mode) || S_ISBLK(mode) \
				|| S_ISFIFO(mode))
#else
#define S_ISSPECIAL(mode)	(S_ISCHR(mode) || S_ISBLK(mode) \
				|| S_ISSOCK(mode) || S_ISFIFO(mode))
#endif

/* Set the default access mode for directory to 
   owner - rwx, group - r-x other - ---
*/
#define DEFAULT_MODE_FOR_DIRECTORY	0x1E8 | S_IFDIR

/* Set the default access mode for file to 
   owner - rwx, group - r-x other - ---
*/
#define DEFAULT_MODE_FOR_FILE	0x1E8 | S_IFREG

//****************************************************************************
// Structures											
//****************************************************************************

typedef	struct _TEMP_FILE_HANDLE
{
	UINT32			valid;
	UINT32				position;
	UINT32				size;
	int				fileHandle;
	STRING				filePath;
} TEMP_FILE_HANDLE;

 
typedef struct _SELECTION
{
	UINT32         valid;
	NWSM_LIST_PTR  definedResourcesToExclude; // text will have the 
	NWSM_LIST_PTR  definedResourcesToInclude; // resource to include/exclude
	NWSM_LIST_PTR  subdirectoriesToInclude;   // otherInfo will contain the
	NWSM_LIST_PTR  subdirectoriesToExclude;   // nameSpaceType. Don't need 
	NWSM_LIST_PTR  filesToInclude;            // to worry about freeing, 
	NWSM_LIST_PTR  filesToExclude;            // NWSMDestroyList will ignore
	NWSM_LIST_PTR  filesWithPathsToInclude;   // otherInfo if the free
	NWSM_LIST_PTR  filesWithPathsToExclude;   // procedure is NULL.
} SELECTION;
 
typedef	struct RESOURCE_LIST
{
	NWBOOLEAN	validResource;
	char		resourceName[MAX_RESOURCE_NAME_LENGTH+1];
	UINT32		nameSpaceType;
	int 		resourceFlags ;
	int		resourceNameLength ;
	dev_t		resourceDevice ;
	struct RESOURCE_LIST *next ;
} RESOURCE_LIST;

#define UNIX_WORKSTATION	0 /* the first node in the resource list */
typedef struct RESOURCE_INFO_STRUCTURE 
{
	UINT16 blockSize;
	UINT32 totalBlocks;
	UINT32 freeBlocks;
	NWBOOLEAN resourceIsRemovable;
	UINT32 purgeableBlocks;
	UINT32 notYetPurgeableBlocks;
	UINT32 migratedSectors;
	UINT32 preCompressedSectors;
	UINT32 compressedSectors;
} RESOURCE_INFO_STRUCTURE ;

/* Resource flag definitions */
#define FS_READONLY	0x00000001
#define FS_REMOVABLE	0x00000002
#define FS_READWRITE	0x00000000

typedef	struct _TARGET_INFO
{
	char		targetName[MAX_TARGET_SERVICE_NAME_LENGTH];
	char		targetType[MAX_TARGET_SERVICE_TYPE_LENGTH];
	char		targetVersion[MAX_TARGET_SERVICE_VER_LENGTH];
	RESOURCE_LIST	*resource;
} TARGET_INFO;


typedef struct _BACKED_UP_HARDLINKS {
	ino_t	inodeNo ;
	UINT32	modifyTime ;
	char	*path ;
	struct  _BACKED_UP_HARDLINKS *next ;
} BACKED_UP_HARDLINKS ;

typedef struct _BACKED_UP_DEVS {
	dev_t	devNo ;
	BACKED_UP_HARDLINKS *links ;
	struct _BACKED_UP_DEVS *next ;
} BACKED_UP_DEVS ;


typedef	struct _CONNECTION
{
	UINT32			valid;
	uid_t			loginUserID ;
	gid_t			loginGroupID ;
	NWBOOLEAN		dontCheckSelectionList;
	SELECTION		*selectionLists;
	TEMP_FILE_HANDLE	ErrorLogFileHandle;
	TEMP_FILE_HANDLE	SkippedDataSetsFileHandle;
	TARGET_INFO		*targetInfo ;
	BACKED_UP_DEVS		*disks ;
	UINT32			numberOfActiveScans ;
	NWSM_LIST_PTR   defaultIgnoreList; 
} CONNECTION;

typedef	struct _DIR_STRUCT
{
	struct stat statb ;
	int	namelen ;
	char	name[MAXNAMLEN+1] ;
	DIR *dirHandle ;
	char	parentPath[MAXPATHLEN + 1] ;
} DIRECTORY_STRUCT;

typedef struct _DIRECTORY_STACK {
	struct           _DIRECTORY_STACK *next;
	NWBOOLEAN        firstScan;
	DIRECTORY_STRUCT dirEntry ;
	UINT16           scanningMode;   /* SCANNING_FILES, SCANNING_DIRS, */
} DIRECTORY_STACK;
 

typedef struct _DATA_SET_SEQUENCE {
	UINT32 valid;
	CONNECTION *connection;
	UINT16 dataSetType;  /* TYPE_FILE, TYPE_DIRECTORY, TYPE_FILESYSTEM */
	NWBOOLEAN scanComplete; /* Next Scan should return */ 
	                        /* NWSM_NO_MORE_DATA_SETS if true */
	STRING_BUFFER *fullPath;
	NWSM_LIST_PTR  definedResourcesToExclude; // text will have the 
	NWSM_LIST_PTR  definedResourcesToInclude; // resource to include/exclude
	NWSM_LIST_PTR  subdirectoriesToInclude;   // otherInfo will contain the
	NWSM_LIST_PTR  subdirectoriesToExclude;   // nameSpaceType. Don't need 
	NWSM_LIST_PTR  filesToInclude;            // to worry about freeing, 
	NWSM_LIST_PTR  filesToExclude;            // NWSMDestroyList will ignore
	NWSM_LIST_PTR  filesWithPathsToInclude;   // otherInfo if the free
	NWSM_LIST_PTR  filesWithPathsToExclude;   // procedure is NULL.
	DIRECTORY_STACK           *directoryStack;
	NWSM_SCAN_INFORMATION     *scanInformation;
	NWSM_DATA_SET_NAME_LIST   *dataSetNames;
	NWSM_SCAN_CONTROL         *scanControl;
	CHAR                       singleFileName[MAXNAMLEN];
	NWBOOLEAN    dataSetIsOpen;  /* Set to TRUE while the dataSet is open */
	                             /* for backup  */
	NWBOOLEAN	WorkstationResource ;
	RESOURCE_LIST		*resourcePtr ;
	UINT16	resourceNumber ;
	NWBOOLEAN	validScanInformation ;
} DATA_SET_SEQUENCE;
 
/*      SM File Handle Structure */
 
typedef struct _SMFILE_HANDLE {
	UINT32    valid;
	INT32     FileHandle;
	UINT32    position;
	UINT32    size;
	NWBOOLEAN outputDataStreamHeader;
	NWBOOLEAN dataStreamIsCorrupt;
	UINT32    CreationDateAndTime ;
	UINT32    LastAccessDateAndTime ;
	UINT32    LastModifyDateAndTime ;
} SMFILE_HANDLE;

typedef struct _UNIX_CHARS {
	dev_t     devNo;      /* device file resides on */
	ino_t     inodeNo;      /* the file serial number */
	mode_t    mode;     /* file mode */
	nlink_t   nlinks;    /* number of hard links to the file */
	uid_t     userID;      /* user ID of owner */
	gid_t     groupID;      /* group ID of owner */
	dev_t     rdevice;     /* the device identifier (special files only)*/
} UNIX_CHARACTERISTICS ;
 
typedef struct _LINK_INFO {
	char	linkPath[MAXPATHLEN + 1] ;
	uid_t	userID ;
	gid_t	groupID ;
	mode_t	mode ;
} LINK_INFORMATION ;

/****************************************************************************/
/*      Backup Data Set Handle */
 
typedef struct _BACKUP_DATA_SET_HANDLE
{
	UINT32               valid;
	CONNECTION          *connection;
	DATA_SET_SEQUENCE   *dataSetSequence;
	SMFILE_HANDLE        smFileHandle;
	UINT16               state;
	UINT16               mode;
	TEMP_FILE_HANDLE    *tempFileHandle;      /* Used to backup the
					error log and skipped data set files */
	/*      State Data Info */
	UINT16               stateDataSize;       /* Amount of data in buffer */
	UINT16               stateDataOffset;     /* Read position */
	BUFFERPTR            stateData;
	 
	/*      Buffer State Data for PutFields */
	UINT16               bufferState;
	BUFFERPTR            begin;       /*begin of data for crc's on backup */
	BUFFERPTR            addressOfDataOffsetField;
	UINT16               sizeOfDataOffsetField;
	UINT16               bufferUnderflowSize;
	NWBOOLEAN            dataSetIsOpen;
} BACKUP_DATA_SET_HANDLE;
 
/*      Restore Data Set Handle */
 
typedef struct _RESTORE_DATA_SET_HANDLE 
{
	UINT32              valid;
	CONNECTION         *connection;
 
	NWBOOLEAN           excluded;      /* data set was excluded */
	UINT16              state;
 
	struct _RESTORE_DATA_SET_HANDLE *parentDataSetHandle;/* If set,
				currentDirHandle is in the parent, not here */
 
	STRING_BUFFER      *fullPath;
 
	NWBOOLEAN           inDataStream;
	NWBOOLEAN           skippingDataStream;
	UINT32              bytesLeftInDataStream;
	 
	STRING_BUFFER      *dataSetName;
	UINT32              nameSpaceType;
	NWBOOLEAN           dataSetWasRenamed;
	UINT16              mode;      /* NWSM_OVERWRITE_DATA_SET,        */
	                               /* NWSM_DO_NOT_OVERWRITE_DATA_SET, */
	                               /* NWSM_CREATE_PARENT_HANDLE       */
		 
	/*      State Data Info */
	UINT16              stateDataSize; /* Amount of data in buffer */
	UINT16              stateDataOffset;       /* Write position */
	BUFFERPTR           stateData;
 
	/*      Buffer State Data for WriteField */
	UINT16   dataSetType;   /* TYPE_FILE, TYPE_DIRECTORY, TYPE_FILESYSTEM */
	UINT16   recordType;    
	UINT16  bufferState;
	NWBOOLEAN inBuffer;   /* restoring from the buffer, not stateData or 
	                         stateDataFile */
	UINT32 recordFID;     /* recordFID is used to validate the end fields 
	                         after offsetToEnd is used */
	BUFFERPTR begin;      /* Beginning of record to restore */
	BUFFERPTR bufferPtr;  /* Position restoring from */
	UINT32    bufferSize; /* the amount of data at bufferPtr */
	BUFFERPTR buffer;     /* Beginning of buffer for 
	                         STATE_DATA_BUFFER_STATE and 
	                         STATE_DATA_FILE_STATE */
	UINT32    bufferBytesToWrite;
	UINT64    offsetToEnd;   /* Number of bytes needed for a full record */
	UINT16    dataUnderflowSize;
	UINT16    usedDataUnderflowSize;
	BUFFERPTR tempData;
	UINT64    dataOverflow; /* number of bytes of data before next field */
	BUFFER    dataUnderflow[DATA_UNDERFLOW_SIZE];
	SMFILE_HANDLE smFileHandle;  /* points at either fileHandle or into 
	                                dataStreams */
	UNIX_CHARACTERISTICS	unixChars ;
	SMFILE_HANDLE stateDataHandle;
	NWBOOLEAN restoreFlag;
	NWBOOLEAN dataSetExists;
	UINT32 currentDataStreamNumber ;
	UINT32 dataStreamType ;
	struct stat dataSetInfo ;
	UINT8	backupOptions ;
	RESOURCE_LIST		*resourcePtr ;
} RESTORE_DATA_SET_HANDLE;
 
typedef union _DATA_SET_HANDLE
{
	BACKUP_DATA_SET_HANDLE  back;
	RESTORE_DATA_SET_HANDLE rest;
} DATA_SET_HANDLE;

/* restoreFlag defines */
#define RESTORE_DATA_SET_NOT_YET_CREATED	1
#define RESTORE_DATA_SET_JUST_CREATED	2
#define RESTORED_CHARACTERISTICS	3
#define RESTORE_DATA_SET_LINKED		4
#define RESTORE_DATA_SET_SKIPPED	5
#define RESTORE_DATA_SET_DONE	6
#define RESTORE_NO_DATA_STREAMS	7

CCODE ExcludeDataSet(DATA_SET_SEQUENCE *dataSetSequence,
			struct stat *dataSetInfo) ;

CCODE ExcludedByScanControl(DATA_SET_SEQUENCE *dataSetSequence,
			struct stat *dataSetInfo) ;

CCODE ExcludedByBackupSelectionList(DATA_SET_SEQUENCE *dataSetSequence) ;

CCODE ExcludedByRestoreSelectionList(RESTORE_DATA_SET_HANDLE *dataSetHandle);

CCODE ExcludedByRestoreSelectionList1( RESTORE_DATA_SET_HANDLE *dataSetHandle,
				UINT32 dataSetType, STRING path) ;

CCODE IsInDefaultIgnoreList(CONNECTION *connection, STRING path);

int match(char *s, char *p) ;

CCODE IncludeOrExcludeDefinedResources(char *fullPathPtr,
SELECTION *selectionLists) ;

CCODE IncludeOrExcludeSubDirectories(char *fileSystem,
char *fullPathPtr,
SELECTION *selectionLists,
UINT32  nameSpaceType) ;

CCODE IncludeOrExcludeFilesAndFilesWithPaths(char *fileSystem,
char *fullPathPtr,
char *terminalNode,
SELECTION *selectionLists,
UINT32  nameSpaceType) ;

void FreeDirectoryStack(DATA_SET_SEQUENCE *dataSetSequence) ;

void FreeSequenceStructure(DATA_SET_SEQUENCE **dataSetSequence) ;

void PopDirectory(DATA_SET_SEQUENCE *dataSetSequence) ;

CCODE PushDirectory(DATA_SET_SEQUENCE *dataSetSequence, 
			UINT16 scanningMode, struct stat *stb) ;

STRING FixDirectoryPath(STRING path) ;

CCODE AppendDataSetNamesAndPaths(STRING FileName, 
			DATA_SET_SEQUENCE *dataSetSequence) ;

void  FreeResourceList(RESOURCE_LIST *resPtr);

CCODE BuildDataSetNamesAndPaths(STRING path, 
			DATA_SET_SEQUENCE *dataSetSequence);

CCODE BuildFileDataSetNames(STRING FileName, DATA_SET_SEQUENCE *dataSetSequence,
	NWSM_DATA_SET_NAME_LIST **dataSetNames,
	UINT8 returnChildTerminalNodeNameOnly) ;

CCODE DelDirFromFullPath(STRING_BUFFER **fullPath) ;

CCODE FixPath(STRING_BUFFER **properPath, STRING improperPath) ;

CCODE AddSlashToPath(STRING path) ;

#define TSAGetMessage(messageID)	tsaMessages[messageID]

CCODE ScanDataSet(CONNECTION *connection, DATA_SET_SEQUENCE *dataSetSequence) ;

CCODE ScanFiles(CONNECTION  *connection, DATA_SET_SEQUENCE *dataSetSequence);

CCODE ScanDirectories(CONNECTION *connection,
		 DATA_SET_SEQUENCE *dataSetSequence) ;

CCODE ScanASingleFile(DATA_SET_SEQUENCE *dataSetSequence) ;

CCODE IncrementCurrentDataSet(CONNECTION *connection,
			DATA_SET_SEQUENCE *dataSetSequence) ;

CCODE InitDataSetSequence( CONNECTION *connection,
			NWSM_SCAN_CONTROL *scanControl,
			NWSM_SELECTION_LIST *selectionList,
			DATA_SET_SEQUENCE **dataSetSequence) ;

void SetScanInformation(DATA_SET_SEQUENCE *dataSetSequence, 
struct stat *dataSetInfo) ;

CCODE BuildSelectionLists(DATA_SET_SEQUENCE *dataSetSequence,
			NWSM_SELECTION_LIST *selectionList) ;

CCODE BuildDefaultIgnoreList(CONNECTION  *connection) ;

int IsInResourceList(char *targetResourceName,
			RESOURCE_LIST **resourcePtr, TARGET_INFO *targetInfo) ;

int FindNextFS(int *resourceNumber, RESOURCE_LIST **resourcePtr,
			TARGET_INFO *targetInfo) ;

int GetResourceNumber(char *dataSetName, RESOURCE_LIST **resourcePtr,
			TARGET_INFO *targetInfo) ;

int IsLocalFS(dev_t devNo, TARGET_INFO *targetInfo) ;

int IsInHardLinksList(BACKED_UP_DEVS *disks, dev_t devNo, 
			ino_t inodeNo, UINT32 modifyTime, char *path,
			BACKED_UP_HARDLINKS **linkNodePtr) ;

int AddToHardLinksList(BACKED_UP_DEVS **disks, dev_t devNo, 
				ino_t inodeNo, UINT32 modifyTime, char *path) ;

void ReleaseHardLinksList(BACKED_UP_DEVS **disks) ;

#include "filesys.h"

void dump_buf(char *buf, int len, char *dumpBuf);

CCODE NWSMFreeName(NWSM_DATA_SET_NAME HUGE *name);

extern char dumpBuffer[] ;

CCODE CloseAndDeleteTempFile(TEMP_FILE_HANDLE *tempFileHandle) ;

CCODE CreateTempFile(TEMP_FILE_HANDLE *tempFileHandle, char *prefix) ;

CCODE ReadTempFile(TEMP_FILE_HANDLE  *tempFileHandle, NWBOOLEAN adjustBuffer, 
		BUFFERPTR *buffer, UINT32 *bytesToRead, UINT32 *bytesRead) ;

CCODE WriteTempFile(TEMP_FILE_HANDLE *tempHandle, BUFFERPTR buffer,
                UINT32 bytesToWrite, UINT32 *bytesWritten);
 
void  LogSkippedDataSets(DATA_SET_SEQUENCE *dataSetSequence, CCODE ccode);

void  LogError(CONNECTION *connection, UINT16 messageNumber, ...);

CCODE BackupDataStreams(BACKUP_DATA_SET_HANDLE *dataSetHandle, 
		UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer) ;
  
CCODE CreateHeaderOrTrailer(BACKUP_DATA_SET_HANDLE *dataSetHandle, 
		UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer, 
		NWBOOLEAN isHeader) ;
 
CCODE BackupNFSCharacteristics(BACKUP_DATA_SET_HANDLE *dataSetHandle,
		UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer) ;

CCODE PutFields(BUFFERPTR *buffer, UINT32 *bytesToRead,
		NWSM_FIELD_TABLE_DATA table[], UINT32 *bytesRead,
		NWBOOLEAN processEndField, UINT32 tailFID,
		BACKUP_DATA_SET_HANDLE *dataSetHandle,
		UINT32 *fileOffsetPtr) ;

CCODE BackupFullPaths(BACKUP_DATA_SET_HANDLE *dataSetHandle,
		UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer) ;

CCODE BackupCharacteristics(BACKUP_DATA_SET_HANDLE *dataSetHandle,
		UINT32 *bytesToRead, UINT32 *bytesRead, BUFFERPTR *buffer) ;

STRING ReturnDataSetName(void *dsHandle) ;

void _ConvertTimeToDOS( time_t calendarTime, UINT16 *filDatP, UINT16 *filTimP);

time_t _ConvertDOSTimeToCalendar( UINT32 dateTime ) ;

void ReInitializeStateInfo(RESTORE_DATA_SET_HANDLE *dataSetHandle) ;

CCODE GetEntireSection(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
				UINT32 *bytesToWrite, BUFFERPTR *buffer) ;

CCODE GetEntireTrailer(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
				UINT32 *bytesToWrite, BUFFERPTR *buffer) ;

CCODE GetFieldToGetSection(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
			BUFFERPTR *buffer, UINT32 *bytesToWrite, UINT32 *fid, 
			UINT64 *dataSize, BUFFERPTR *data) ;

CCODE GetFieldToProcessSection(RESTORE_DATA_SET_HANDLE *dataSetHandle,
			UINT32 *fid, UINT64 *dataSize, BUFFERPTR *data, 
			UINT32 *dataOverflowSize) ;

CCODE CopyToStateData(RESTORE_DATA_SET_HANDLE *dataSetHandle, BUFFERPTR buffer);

CCODE RestoreFullPaths(RESTORE_DATA_SET_HANDLE *dataSetHandle) ;

#define CREATE_PARENT_DIRECTORY	1
CCODE CreateDirectory(CHAR *fullPath, NWBOOLEAN mode, mode_t accessMode) ;

int HandleUnixError(int errorNum) ;

int GetUIDByNWname(char *UserName) ;

int GetUIDByNWuid(UINT32 OwnerID) ;

char *GetNWNameByuid(int uid) ;

CCODE SetAttributes(RESTORE_DATA_SET_HANDLE *dataSetHandle) ;

CCODE CreateDataSet(RESTORE_DATA_SET_HANDLE *dataSetHandle) ;

CCODE RestoreDataStreams(RESTORE_DATA_SET_HANDLE *dataSetHandle, 
				UINT32 *bytesToWrite, BUFFERPTR *buffer) ;

CCODE RestoreCharacteristics(RESTORE_DATA_SET_HANDLE *dataSetHandle) ;

CCODE RestoreNFSCharacteristics(RESTORE_DATA_SET_HANDLE *dataSetHandle) ;

void DecryptPassword(UINT8  *key, UINT8  *authentication, UINT8  *password) ;
extern char scratchBuffer[] ;

extern char tmpPath[] ;

extern int convertCodeset ;

extern NWSM_MODULE_VERSION_INFO unixtsaModule ;

extern	CONNECTION	ConnectionHandle ;

extern	TARGET_INFO	TargetInfo;

#endif
