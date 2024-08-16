/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/sistructs.h	1.12"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/sistructs.h,v 2.53.2.2 1995/01/05 17:54:48 hashem Exp $"

#ifndef _NET_NUC_NCP_SISTRUCTS_H
#define _NET_NUC_NCP_SISTRUCTS_H


/*
 *  Netware Unix Client
 *
 *	MODULE: silstructs.h
 *	ABSTRACT: Structures used by the NCP handler of the SPI.
 */

/*
 *	Dependent header files
 */

#ifdef _KERNEL_HEADERS
#include <net/nuc/ncpconst.h>
#else _KERNEL_HEADERS
#include <sys/ncpconst.h>
#endif _KERNEL_HEADERS

/*
 *	Typedef Structure: ncp_server_t
 *
 *	Field Abstract:
 *	
 *	Build in object validation tag if NUC_DEBUG is turned on.
 *		
 */
typedef struct {
	int32		majorVersion;		/* 2=286 3=386 */
	int32		minorVersion;		/* e.g. 15 for 2.15, 11 for 3.11 */
	void_t		*volumeListPtr;
	void_t		*diagnosticStruct;
	bmask_t		transportMask;
	bmask_t		diagnosticsMask;
	struct netbuf	*address;
	uint32		blockSize;	/* block size for all volumes */
	uint8		serverName[NCP_MAX_OBJECT_NAME_LENGTH];
#if ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	uint8		tag[2];
#endif ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
} ncp_server_t;


/*
 *	Typedef Structure: NW_AUTH
 *	Authentication state information.
 *
 *	Password may, or may not, be encrypted.  The password key
 *	negotiated with the server will be volatile, and thus, will
 *	not be maintained in any structure. 
 *
 *	Field Abstract:
 */
typedef struct {
	char	userID[NCP_MAX_OBJECT_NAME_LENGTH];
	uint8	authenticationKey[NCP_MAX_OBJECT_NAME_LENGTH];
	uint32  objectID;
} ncp_auth_t;

/*
 *	Task structure (one per OS authentication instance )
 *	for all users
 *
 *	This structure is created by NCPsiCreateTask which is invoked via NWMP
 *	function NWMP_OPEN_TASK.  It is destroyed by NCPsiCloseTask which is
 *	invoked via NWMP function NWMP_CLOSE_TASK.
 */
typedef struct {
	void_t	*serverPtr;
	void_t	*channelPtr;
	void_t	*authInfoPtr;
	void_t	*credStructPtr;
	void_t	*diagPtr;
	uint8	dirHandleTable[0xFF/sizeof(uint8)];	/* bit mask */
	void_t	*spilTaskPtr;
	/*
		isAuthenticated is set to TRUE by NCPsiAuthenticateTask and set back
		to FALSE by NCPsiUnauthenticateTask.  It is set to FALSE when the
		structure is created.
	*/
	uint32	isAuthenticated;
#if ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	uint8	tag[2];
#endif ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
} ncp_task_t;


/*
 *	Typedef struct: NCP_DIAG_NODE_T
 *	Management structure
 *
 */
typedef struct {
	int32	connCreateTime;
	int32	totalRequests;
	int32	currentServers;
	int32	currentTasks;
	int32	failedRequests;	
} ncp_diag_node_t;


/*
 *	Typedef struct: NCP_FHANDLE_T
 *	File handle structure
 */
typedef struct {
	int32	handleLength;
	uint8	handle[1];
} ncp_fhandle_t;


/*
 *	NetWareInfoStruct is a structure whose contents is determined by the
 *	value of RETURNINFOMASK which is set below.  If other fields are needed
 *	then add the appropriate bits to RETURNINFOMASK and be happy.
 */
typedef struct {
    /*
	 *	Data Stream Space Allocated Information Structure
	 */
    uint8   DiskSpaceAllocated[4];

    /*
	 *	Attributes Information Structure
	 */
    uint8   Attributes[4];
    uint8   Flags[2];

    /* 
	 *	Data Stream Size Information Structure
	 */
    uint8   DataStreamSize[4];

    /*
	 *	Total Space Allocated Structure
	 */
    uint8   TotalDataStreamSize[4];
    uint8   NumberOfDataStreams[2];

    /* 
	 *	Creation Information Structure
	 */
    uint8   CreationTime[2];
    uint8   CreationDate[2];
    uint8   CreatorsID[4];

    /*
	 *	Modify Information Structure
	 */
    uint8   ModifyTime[2];
    uint8   ModifyDate[2];
    uint8   ModifiersID[4];
    uint8   LastAccessDate[2];

    /*
	 *	Archival Information Structure
	 */
    uint8   ArchivedTime[2];
    uint8   ArchivedDate[2];
    uint8   ArchiversID[4];

    /*
	 *	Rights Information Structure
	 */
    uint8   InheritedRightsMask[2];

    /*
	 *	Directory Entry Information Structure
	 */
    uint8   DirectoryEntryNumber[4];    /* directory base */
    uint8   DOSDirectoryEntryNumber[4]; /* DOS entry directory base */
    uint8   VolumeNumber[4];

    /*
	 *	Extended Attributes Information Structure
	 */
    uint8   ExtendedAttributesValueSize[4];
    uint8   ExtendedAttributesCount[4];
    uint8   ExtendedAttributesKeySize[4];

    /*
	 *	Name Space Information Structure
	 */
    uint8   CreatorNameSpaceNumber[4];

    /*
	 *	NetWareFileNameStruct
	 */
    uint8   FileNameLength;
    uint8   FileName[1];

} NetWareInfoStruct ;

/*
 *  Cookies3NS is a structure of search cookie information used in
 *  UNIX Name Space directory searches.  A dynamically acquired array of
 *  cookies are obtained to save any cookies returned to the File System
 *  in support of telldir, seekdir, rewinddir and other UNIX functions
 *  since UNIX cannot tolerate a search cookie larger than 2**21.
 */
typedef struct {
    uint8   searchCookie[9];
} Cookies3NS;

#endif /* _NET_NUC_NCP_SISTRUCTS_H */
