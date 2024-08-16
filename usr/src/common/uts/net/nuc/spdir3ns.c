/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spdir3ns.c	1.17"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spdir3ns.c,v 2.62.2.5 1995/01/18 03:30:44 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE:    spdir3ns.c
 *	ABSTRACT:    NetWare V3.x UnixNameSpace Directory access NCP's 
 *
 *	Functions declared in this module:
 *	Public Functions:
 *		NCPsp3NAllocTempDirectoryHandle
 *		NCPsp3NDeleteDirectoryHandle
 *		NCPsp3NGetDirPathFromHandle
 *
 *	Private Functions:
 *		NCPsp3NInitializeSearch
 *		NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct
 *		NCPsp3NPopulateNUCAttributesFromNameSpaceInfo
 */ 

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/spilcommon.h>
#include <util/cmn_err.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nwlist.h>
#include <util/debug.h>
#include <net/nuc/sistructs.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nuc_prototypes.h>
#include <util/param.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_ncp

/*
 *	NOBODY and NOGROUP values
 */
unsigned int svr4Nobody = 60001;
unsigned int svr4Nogroup = 60001;

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NGetDirPathFromHandle.3k)
 * NAME
 *    NCPsp3NGetDirPathFromHandle - Given a directory handle return a rooted
 *                                 path.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NGetDirPathFromHandle ( channel, dirHandle, path, nameSpace )
 *		void_t	*channel;
 *		uint8	*dirHandle;
 *		char	*path;
 *		uint32	nameSpace;
 *
 * INPUT
 *		void_t	*channel;
 *		uint8	*dirHandle;
 *		uint32	nameSpace;
 *
 * OUTPUT
 *		char	*path;
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *
 * DESCRIPTION
 *		Given a previously opened directory handle, will return a
 *		rooted path.  The NCP call returns the path with volume name,
 *		but it will be shifted off by this routine before passing
 *		it up to the caller.
 *		
 * NCP
 *	87 28
 *
 * NOTES
 *		Path argument should be at least 1024 characters long
 *
 * SEE ALSO
 *		NCPsp3NAllocTempDirHandle(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NGetDirPathFromHandle_l (	ncp_channel_t		*channel,
								ncp_volume_t		*volume,
								NCP_DIRHANDLE_T		*dirHandle,
								int32				objectID,
								boolean_t			wantParentPath,
								char				*path,
								int32				*objectType,
								uint32				nameSpace )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	SrcNameSpace;
		uint8	DstNameSpace;
		/*
		 *	The following 3 fields comprise the PathCookie structure
		 */
		uint8	Flags[2];
		uint8	Cookie1[4];
		uint8	Cookie2[4];
		/*
		 *	The following 5 fields comprise the NWHandlePathStruct
		 */
		uint8	VolumeNumber;
		uint32	DirectoryBaseOrShortHandle;
		uint8	HandleFlag;
		uint8	PathComponentCount;
		uint8	PComponent[1];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		/*
		 *	The following 3 fields comprise the PathCookie structure
		 */
		uint8	Flags[2];
		uint8	Cookie1[4];
		uint8	Cookie2[4];
	
		uint8	PathComponentSize[2];
		uint8	PathComponentCount[2];
		uint8	PathComponents[SPI_MAX_PATH_LENGTH];
	} *replyPacket;
#pragma pack()

	uint8		*buffer, *bufferEnd, *p;
	uint8		size;
	uint32		Cookie1, Cookie2;
	uint16		Flags;
	uint32		i, totalSize = 0;
	boolean_t	pathComplete = FALSE;
	uint16		componentCount, componentSize;

	NVLT_ENTER (8);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	/*
	 *	Preset the cookies to indicate "START AT BEGINNING" and set the
	 *	flag to indicate I want the filename in addition to the directories
	 */
	Cookie1 = (uint32)-1L;
	Cookie2 = (uint32)-1L;
	Flags = 0;
	buffer = (uint8 *)kmem_zalloc (SPI_MAX_PATH_LENGTH, KM_SLEEP);

	while ((!pathComplete) && (ccode == SUCCESS)) {

		NCPdplBuildNCPHeader_l (channel, request);

		request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

		requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
		requestPacket->FunctionCode = FNGENERIC_ENHANCED;
		requestPacket->SubFunction = SFENHANCED_GET_FULL_PATH_STRING;
		requestPacket->SrcNameSpace = (uint8)nameSpace;
		requestPacket->DstNameSpace = (uint8)nameSpace;
		MOVELE_INT32 (requestPacket->Cookie1, Cookie1);
		MOVELE_INT32 (requestPacket->Cookie2, Cookie2);
		MOVELE_INT16 (requestPacket->Flags, Flags);

		requestPacket->VolumeNumber = volume->number;
		if (dirHandle)
			requestPacket->DirectoryBaseOrShortHandle = 
				REVPUTINT32 (dirHandle->DirectoryBase);
		else
			requestPacket->DirectoryBaseOrShortHandle = REVGETINT32 (objectID);
		requestPacket->PathComponentCount = 0;
		requestPacket->HandleFlag = DIRECTORY_BASE;

		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct reqPack);
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
		reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		ccode = NCPdiTransaction_l (channel, request, reply);
		if (ccode != SUCCESS)
			break;

		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;
		if (replyPacket->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
			ccode = SPI_NO_CONNECTIONS_AVAILABLE;
			break;
		} else if (replyPacket->connectionStatus & CS_BAD_CONNECTION) {
			ccode = SPI_BAD_CONNECTION;
			break;
		} else if (replyPacket->connectionStatus & CS_SERVER_DOWN) {
			ccode = SPI_SERVER_DOWN;
			break;
		}

		switch (replyPacket->completionCode) {

			case SUCCESS:
				/*
				 *	Save the cookies for the next call
				 */
				Cookie1 = REVGETINT32 (*(uint32 *) replyPacket->Cookie1);
				Cookie2 = REVGETINT32 (*(uint32 *) replyPacket->Cookie2);
				Flags = REVGETINT16 (*(uint16 *) replyPacket->Flags);

				/*
				 *	If the path components won't fit in the buffer, set
				 *	an error condition to exit the loop
				 *
				 * NOTE:
				 *	Echo component is returned as follow:
				 *		byte	pathComponentLength
				 *		byte	pathComponent
				 */
				componentCount = REVGETINT16 (*(uint16 *)
						replyPacket->PathComponentCount);
				componentSize = REVGETINT16 (*(uint16 *)
						replyPacket->PathComponentSize);
				/*
				 * Add the totalSize, the componetSize and also the
				 * componentCount (this is added to accommodate the 
				 * slashes added for each entry) to make sure we have
				 * enough space.
				 */
				if  ((totalSize + componentSize + componentCount) > 
						(SPI_MAX_PATH_LENGTH - 1)) {
					ccode = SPI_NAME_TOO_LONG;
					break;
				}

				/*
				 *	For each path component, move it into the buffer 
				 *	in reverse order, so the last component will be first.
				 *	We subtract one from the componentCount, so the volume
				 *	name will not be part of the path.
				 */
				p = replyPacket->PathComponents;
				NVLT_PRINTF ("Path=0x%x, Size = %d , Count = %d\n", 
						p, componentSize, componentCount);
				bufferEnd = buffer + SPI_MAX_PATH_LENGTH;
				NVLT_PRINTF ("bufferEnd = 0x%x\n", bufferEnd, 0, 0);
				*(--bufferEnd) = '\0';
				totalSize++;
				for (i = 0; i < componentCount - 1; i++) {
					size = *p + 1;
					NVLT_PRINTF ("size[%d]=%d, total=%d\n", i, size, totalSize);
					if (wantParentPath && i == 0) {
						/*
						 *	We want the path of the parent, so ignore the 
						 *	first component which is the last element of
						 *	the path.
						 */ 
						p += (size);	
						continue;
					}

					bufferEnd -= (size);
					*bufferEnd = '/';
					NVLT_PRINTF ("firstBcopy p+1=0x%x, bufferEnd+1=0x%x\n", 
							p+1, bufferEnd+1, 0);
					bcopy (p+1, bufferEnd+1, size-1);
					p += (size);
					totalSize += (size);
				}
				ccode = SUCCESS;

				/*
				 *	If Cookie2 == -1, the entire path has been received
				 *	so break out of this loop
				 */
				if (Cookie2 == (uint32)-1)
					pathComplete = TRUE;
				break;

			case E_INVALID_PATH:
				ccode = SPI_INVALID_PATH;
				break;

			case E_ALL_FILES_INUSE:
				ccode = SPI_FILE_IN_USE;
				break;

			default:
				ccode = SPI_NODE_NOT_DIRECTORY;
				break;
		}
	}

	/*
	 *	If the entire path was retrieved from the server, copy it to the
	 *	callers buffer and NULL-terminate it
	 */
	if (ccode == SUCCESS) {
		NVLT_PRINTF ("Second bcopy bufferEnd = 0x%x, totalSize = %d\n", 
				bufferEnd+1, totalSize, 0);
		if (totalSize == 1) {
			/*
			 * We are returning the path to the root node.
			 */
			path[0] = '/'; 
			path[1] = '\0';
			*objectType = NS_ROOT;
		} else {
			bcopy (bufferEnd+1, path, totalSize-1); 
			path[totalSize] = '\0';
			*objectType = NS_DIRECTORY;
		}
	}

	kmem_free (buffer, SPI_MAX_PATH_LENGTH);

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NInitializeSearch.p3k)
 * NAME
 *		NCPsp3NInitializeSearch - Initialize a directory search 
 *
 * SYNOPSIS
 *		NCPsp3NInitializeSearch ( s, cookie )
 *		struct searchStruct	*s;
 *		Cookies3NS			*cookie;
 *
 * INPUT
 *		struct searchStruct *s;
 *
 * OUTPUT
 *		Cookies3NS			*cookie;
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		Initializes a directory search to a NetWare V3.11 file server
 *
 * NCP
 *	87 2
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsp3NSearchForFileOrSubdirectorySet(3k)
 *		NCPsp3NGetDirectoryEntries(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NInitializeSearch (	ncp_channel_t	*channel,
							ncp_volume_t	*volume,
							NCP_DIRHANDLE_T	*dirHandle,
							Cookies3NS		*cookie )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	NameSpace;
		uint8	Reserved;
		/*
		 *	The following 5 fields comprise the NWHandlePathStruct
		 */
		uint8	VolumeNumber;
		uint32	DirectoryBaseOrShortHandle;
		uint8	HandleFlag;
		uint8	PathComponentCount;
		uint8	PComponent[1];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint8	SearchSequence[NCP_SEARCH_COOKIE_LENGTH];
	} *replyPacket;
#pragma pack()

	NVLT_ENTER (4);

	if (NCPdplGetFreePacket_l(channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	
	requestPacket->FunctionCode = FNGENERIC_ENHANCED;
	requestPacket->SubFunction = SFENHANCED_INITIALIZE_SEARCH;
	requestPacket->NameSpace = NCP_UNIX_NAME_SPACE;

	requestPacket->VolumeNumber = volume->number;
	requestPacket->DirectoryBaseOrShortHandle =
			REVPUTINT32 (dirHandle->DirectoryBase);
	requestPacket->HandleFlag = DIRECTORY_BASE;		
	requestPacket->PathComponentCount = 0;

	request->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;
		if (replyPacket->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
			ccode = SPI_NO_CONNECTIONS_AVAILABLE;
		} else if (replyPacket->connectionStatus & CS_BAD_CONNECTION) {
			ccode = SPI_BAD_CONNECTION;
		} else if (replyPacket->connectionStatus & CS_SERVER_DOWN) {
			ccode = SPI_SERVER_DOWN;
		} else {
			switch (replyPacket->completionCode) {
				case SUCCESS:
					bcopy (replyPacket->SearchSequence, 
							cookie->searchCookie, NCP_SEARCH_COOKIE_LENGTH);
					ccode = SUCCESS;
					break;
				
				case E_INVALID_PATH:
					ccode = SPI_INVALID_PATH;
					break;

				case E_ALL_FILES_INUSE:
					ccode = SPI_FILE_IN_USE;
					break;

				default:
					ccode = SPI_BAD_ARGS_TO_SERVER;
					break;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct.3k)
 * NAME
 *		NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct - Get UNIX 
 *			information from a NUCAttributeStruct and place it into a 
 *			NWSI_NAME_SPACE structure
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct ( nameSpaceInfo,
 *			attr )
 *		NWSI_NAME_SPACE_T	*nameSpaceInfo;
 *		NUCAttributeStruct	*attr;
 *
 * INPUT
 *		NUCAttributeStruct	*attr;
 *
 * OUTPUT
 *		NWSI_NAME_SPACE_T	*nameSpaceInfo;
 *
 * RETURN VALUES
 *		SUCCESS
 *
 * DESCRIPTION
 *		Copy relevant data from a NUCAttributeStruct and place it into a
 *		supplied NWSI_NAME_SPACE_T structure.
 *
 *		The NUCAttributeStruct is returned by the NUC NCP.
 *
 * NOTES
 *		Fields in NWSI_NAME_SPACE_T that are updated are:
 *
 *			nodeType		nodeNumber		nodePermissions	nodeSize
 *			majorNumber		minorNumber		userID			groupID
 *			accessTime		modifyTime		changeTime
 *			
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
void_t
NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct ( NWSI_NAME_SPACE_T	*nameSpaceInfo,
													NUCAttributeStruct	*attr )
{

	/*
	 *	Populate the NWSI_NAME_SPACE_T name space structure
	 *	with values returned in a NUCAttributeStruct
	 *
	 * NOTE:
	 *	nodeNumber is set to realUNIXnodeNumber incase it is a linked file.
	 *  Otherwise UNIXnodeNumber and realUNIXnodeNumber will be the same.
	 */
	nameSpaceInfo->nodeNumber = REVGETINT32 (*(uint32 *)attr->UNIXnodeNumber);
	nameSpaceInfo->linkNodeNumber =
			REVGETINT32 (*(uint32 *)attr->realUNIXnodeNumber);

	/*
	 * We need to make sure that the searchSequence is set to the next directory
	 * entry search for each entry incase of a diretory.
	 */
	nameSpaceInfo->searchSequence=(nameSpaceInfo->linkNodeNumber & 0x00ffffff)
			<< 2;
	nameSpaceInfo->nodeType = REVGETINT32 (*(uint32 *)attr->nodeType);
	nameSpaceInfo->nodeNumberOfLinks =
			REVGETINT32 (*(uint32 *)attr->numberOfLinks);
	nameSpaceInfo->nodeSize =  REVGETINT32 (*(uint32 *)attr->nodeSize);
	nameSpaceInfo->userID = REVGETINT32 (*(uint32 *)attr->userID);
	if (nameSpaceInfo->userID == 0xfffffffe)
		nameSpaceInfo->userID = svr4Nobody;
	nameSpaceInfo->groupID = REVGETINT32 (*(uint32 *)attr->groupID);
	if (nameSpaceInfo->groupID == 0xfffffffe)
		nameSpaceInfo->groupID = svr4Nogroup;
	nameSpaceInfo->accessTime = REVGETINT32 (*(uint32 *)attr->accessTime);
	nameSpaceInfo->modifyTime = REVGETINT32 (*(uint32 *)attr->modifyTime);
	nameSpaceInfo->changeTime = REVGETINT32 (*(uint32 *)attr->changeTime);
	nameSpaceInfo->nodePermissions =
			REVGETINT32 (*(uint32 *)attr->nodePermissions);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NAllocTempDirectoryHandle.3k)
 * NAME
 *		NCPsp3NAllocTempDirectoryHandle
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NAllocTempDirectoryHandle ( channel, volumeHandle, parentHandle, 
 *		path, handleName )
 *		void_t	*channel;
 *		ncp_volume_t volumeHandle;
 *		NCP_DIRHANDLE_T	*parentHandle;
 *		char	*path;
 *		NCP_HANDLE_T	**handleName;
 *
 * INPUT
 *		void_t	*channel;
 *		ncp_volume_t volumeHandle;
 *		NCP_DIRHANDLE_T	*parentHandle;
 *		char	*path;
 *
 * OUTPUT
 *		NCP_HANDLE_T	**handleName;
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_SERVER_RESOURCE_SHORTAGE
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *
 * DESCRIPTION
 *		Allocate a temporary directory handle on the server for marking
 *		places in the directory tree for subsequent reference.  
 *
 *
 * NCP
 *	87 22
 *
 * NOTES
 *		This function generates a Directory Base from the directory base or
 *		fully qualified path presented.  
 *
 * SEE ALSO
 *		NCPsp3NDeleteDirectoryHandle(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NAllocTempDirectoryHandle_l (	ncp_channel_t		*channel,
									ncp_volume_t		*volumeHandle,
									NCP_DIRHANDLE_T		*parentHandle,
									char				*directoryPath,
									int32				objectID,
									int32				objectType,
									uint32				nameSpace,
									NCP_DIRHANDLE_T		**dirHandle )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	NameSpace;
		uint8	Reserved[3];
		/*
		 *	The following 5 fields comprise the NWHandlePathStruct
		 */
		uint8	VolumeNumber;
		uint32	DirectoryBaseOrShortHandle;
		uint8	HandleFlag;
		uint8	PathComponentCount;
		uint8	PComponent[1];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint32	NSDirectoryBase;
		uint32	DOSDirectoryBase;
		uint8	VolumeNumber;
	} *replyPacket;
#pragma pack()

	uint32	componentCount = 0, componentPathLength = 0;
	int32	nodeType;

	NVLT_ENTER (8);

	/*
	 *	Allocate a NCP_DIRHANDLE_T:  return if failure
	 */
	*dirHandle = (NCP_DIRHANDLE_T *) kmem_alloc (sizeof (NCP_DIRHANDLE_T), 
			KM_SLEEP);
	/*
	 *	Build a GenerateDirectoryBaseAndVolumeNumber NCP 87/22
	 */
	if (NCPdplGetFreePacket_l (channel, &request)) {
		kmem_free (*dirHandle, sizeof (NCP_DIRHANDLE_T));
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		kmem_free (*dirHandle, sizeof (NCP_DIRHANDLE_T));
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->FunctionCode = FNGENERIC_ENHANCED;
	requestPacket->SubFunction = SFENHANCED_GENERATE_DIRECTORY_BASE;
	requestPacket->NameSpace = (uint8)nameSpace;
	bzero (requestPacket->Reserved, 3);
	requestPacket->VolumeNumber = 0;
	nodeType = objectType;
	requestPacket->HandleFlag = DIRECTORY_BASE;
	requestPacket->VolumeNumber = volumeHandle->number;
	if (objectID != -1) {
		/*
		 * Have the object ID of the node.
		 */
		requestPacket->DirectoryBaseOrShortHandle = REVPUTINT32 (objectID);
	} else {
		if (parentHandle) {
			/*
			 * Have the parent handle.
			 */
			requestPacket->DirectoryBaseOrShortHandle =
					REVPUTINT32 (parentHandle->DirectoryBase);
		} else {
			/*
			 * Do not have the parent handle but have the full path.
			 * Set DirectoryBaseOrShortHandle to 0 and HandleFlag to
			 * DIRETORY_BASE to indicate root of the file system.
			 */
			requestPacket->DirectoryBaseOrShortHandle = 0;
		} 

		if (directoryPath) 
			ccode = NCPspConvertPathToComponents (directoryPath, 
					(char *)requestPacket->PComponent, 300, &componentCount,
					&componentPathLength, nodeType);
	}

	if (ccode == SUCCESS) {

		requestPacket->PathComponentCount = (uint8)componentCount;
		/*
		 *	Determine the size of the packet header and data
		 */
		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
			sizeof (struct reqPack) + componentPathLength - 1;
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
		reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		ccode = NCPdiTransaction_l (channel, request, reply);
		if (ccode == SUCCESS) {
			replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;
			if (replyPacket->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
				ccode = SPI_NO_CONNECTIONS_AVAILABLE;
			} else if (replyPacket->connectionStatus & CS_BAD_CONNECTION) {
				ccode = SPI_BAD_CONNECTION;
			} else if (replyPacket->connectionStatus & CS_SERVER_DOWN) {
				ccode = SPI_SERVER_DOWN;
			} else {
				switch (replyPacket->completionCode) {
					case SUCCESS:
						(*dirHandle)->DirectoryBase =
							REVGETINT32 (replyPacket->NSDirectoryBase);
						ccode = SUCCESS;
						NVLT_PRINTF ("NCPsp3NAllocTempDirectoryHandle: 0x%x\n",
								*dirHandle, 0, 0);
						NVLT_STRING (directoryPath);
						break;

					case E_NO_SEARCH_PRIV:
						ccode = SPI_ACCESS_DENIED;
						break;

					case E_INVALID_PATH:
						ccode = SPI_INVALID_PATH;
						break;

					default:
						ccode = SPI_SERVER_RESOURCE_SHORTAGE;
						break;
				}
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NDeleteDirectoryHandle.3k)
 * NAME
 *		NCPsp3NDeleteDirectoryHandle
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NDeleteDirectoryHandle ( channel, handleName )
 *		void_t			*channel;
 *		NCP_DIRHANDLE_T	*handleName;
 *
 * INPUT
 *		void_t			*channel;
 *		NCP_DIRHANDLE_T	*handleName;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		This function previously freed a previously allocated directory handle. 
 *		Any struct searchStructs chained to this directory handle will be 
 *		removed from the list and freed prior to deallocating the directory
 *		handle.
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsp3NAllocTempDirectoryHandle(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NDeleteDirectoryHandle_l (	ncp_channel_t	*channel,
									NCP_DIRHANDLE_T	*handleName )
{
	NVLT_ENTER (2);

	/*
	 *	Free the NCP_DIRHANDLE_T memory
	 */
	kmem_free (handleName, sizeof (NCP_DIRHANDLE_T));

	return (NVLT_LEAVE (SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NPopulateNUCAttributesFromNameSpaceInfo.3k)
 * NAME
 *		NCPsp3NPopulateNUCAttributesFromNameSpaceInfo
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NPopulateNUCAttributesFromNameSpaceInfo ( attr, ns )
 *		NWSI_NAME_SPACE_T *ns;
 *		NUCAttributeStruct *attr;
 *
 * INPUT
 *		NWSI_NAME_SPACE_T *ns;
 *
 * OUTPUT
 *		NUCAttributeStruct *attr;
 *
 *
 * RETURN VALUES
 *		NOTHING
 *
 * DESCRIPTION
 *		This function builds a NUCAttributeStruct from a NWSI_NAME_SPACE_T. 
 *		NUCAttributeStruct is a structure required by various subfunctions 
 *		of the NetWare UNIX Client NCP.  NWSI_NAME_SPACE_T is a structure
 *		passed between the NUC File System and the SPIL layer.
 * 
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
void_t
NCPsp3NPopulateNUCAttributesFromNameSpaceInfo(	NUCAttributeStruct	*attr,
												NWSI_NAME_SPACE_T 	*ns )
{

	MOVELE_INT32 (attr->UNIXnodeNumber, ns->nodeNumber);
	MOVELE_INT32 (attr->nodeType, ns->nodeType);
	MOVELE_INT32 (attr->numberOfLinks, ns->nodeNumberOfLinks);
	MOVELE_INT32 (attr->nodeSize, ns->nodeSize);
	attr->majorNumber[0] = 0;
	attr->majorNumber[1] = 0;
	attr->minorNumber[0] = 0;
	attr->minorNumber[1] = 0;
	MOVELE_INT32 (attr->userID, ns->userID);
	if (ns->userID == svr4Nobody)
		MOVELE_INT32 (attr->userID, 0xfffffffe);
	MOVELE_INT32 (attr->groupID, ns->groupID);
	if (ns->groupID == svr4Nogroup)
		MOVELE_INT32 (attr->groupID, 0xfffffffe);
	MOVELE_INT32 (attr->accessTime, ns->accessTime);
	MOVELE_INT32 (attr->modifyTime, ns->modifyTime);
	MOVELE_INT32 (attr->changeTime, ns->changeTime);
	MOVELE_INT32 (attr->nodePermissions, ns->nodePermissions);
}
