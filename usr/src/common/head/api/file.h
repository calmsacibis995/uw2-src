/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/file.h	1.1"
#ident	"$Header: $"

/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    include/api/file.h 1.3 (Novell) 7/30/91
 */

/*
**
**	TDR structure used for file system calls
**
*/

#define MAX_TRUSTEE_ENTRIES		20
#define SEARCH_FOR_DIR			0x80
#define	MIN( x, y )  ( ( x < y ) ? ( x ) : ( y ) )
#define	MOD4K( x )  ( x % 4096 )

typedef struct {
	uint32	objectID;
	uint16	trusteeRights;
} trustee_t;

typedef struct fileInfo {
	char	fileHndle[ NWMAX_FILE_HANDLE_SIZE ];
	char	destFileHndle[ NWMAX_FILE_HANDLE_SIZE ];
	char	fileNames[ NWMAX_DIR_PATH_LENGTH ];
	char	newFileNames[ NWMAX_DIR_PATH_LENGTH ];
	char	dirPath[ NWMAX_DIR_PATH_LENGTH ];
	char	volumeName[ NWMAX_VOLUME_NAME_LENGTH ];
	char	oldDirName[ NWMAX_DIR_PATH_LENGTH ];
	char	newDirName[ NWMAX_DIR_PATH_LENGTH ];
	char	*dataBuffer;
	uint8	oddOffset;
	uint8	bufferSize;
	uint8	dirHndle;
	uint8	newDirHndle;
	uint8	fileAttr;
	uint8	searchAttr;
	uint8	extendedAttr;
	uint8	accRights;
	uint8	volNameLength;
	uint8	fileNamesLength;
	uint8	newFileNamesLength;
	uint8	dirPathLength;
	uint8	oldPathFieldCount;
	uint8	newPathFieldCount;
	uint8	oldDirNameLength;
	uint8	newDirNameLength;
	uint8	newRightsMask;
	uint8	nameSpace;
	uint8	volNumber;
	uint8	sectorsInBlock;
	uint16	*maskArrayPtr;
	uint16	revokeRightsMask;
	uint16	grantRightsMask;
	uint16	dataStreamNumber;
	uint16	maxElements;
	uint16	rights;
	uint16	numOfBytes;
	int32	sequenceNum;
	uint32	attr;
	uint32	startingFileOffset;
	uint32	ownerID;
	uint32	srcFileOffset;
	uint32	destFileOffset;
	uint32	numOfBytesToCopy;
	uint32	numOfBytesCopied;	
	uint32	objID;
	uint32	*objIDArrayPtr;
	uint32	changeAttr;
	int32	diskSpaceInUse;
	int32	restriction;
	trustee_t			*trusteeArray;
	void				*entryData;
	NWNameSpaceInfo_t	*nsInfo;
	NWDirRestriction_t	*dirRestriction;
	NWUserRestriction_t	*userRestriction;
	NWDirEntryInfo_t	*dirEntry;
	NWFileEntryInfo_t	*fileEntry;
	NWVolUsage_t		*volInfo;
	NWSalvageableInfo_t	*salvageData;
	NWTrusteeRights_t	*trustee;
} fileInfo_t;





