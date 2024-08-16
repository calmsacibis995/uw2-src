/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spdir3ns2.c	1.20"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spdir3ns2.c,v 2.59.2.9 1995/02/13 07:55:00 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE:    spdir3ns2.c
 *	ABSTRACT:    NetWare V3.x UnixNameSpace Directory access NEW NCP's 
 *
 *	Functions declared in this module:
 *	Public Functions:
 *		NCPsp3NOpenFile2
 *		NCPsp3NCreateFileOrSubdirectory2
 *		NCPsp3NDeleteFileOrDirectory2
 *		NCPsp3NRenameFileOrDirectory2
 *		NCPsp3NGetDirectoryEntries2
 *		NCPsp3NLinkFile2
 *		NCPsp3NGetNameSpaceInformation2
 *		NCPsp3NSetNameSpaceInformation2
 *
 *	Private Functions:
 *		NCPsp3NSearchForFileOrSubdirectorySet2
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
#include <net/nuc/slstruct.h>
#include <util/debug.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/requester.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_ncp

/*
 *	Macros
 */
#define NW_GET_UID( _c )	( ((nwcred_t*)((ncp_task_t*)((ncp_channel_t*) \
							(_c))->taskPtr)->credStructPtr)->userID )
#define NW_GET_GID( _c )	( ((nwcred_t*)((ncp_task_t*)((ncp_channel_t*) \
							(_c))->taskPtr)->credStructPtr)->groupID )

/*
 *	Forward function declarations
 */
ccode_t	NCPsp3NSearchForFileOrSubdirectorySet2_l (ncp_channel_t *,
													NUC_IOBUF_T *,
													Cookies3NS *,
													uint16 *,
													nwcred_t *);


/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NDeleteFileOrDirectory2.3k)
 * NAME
 *		NCPsp3NDeleteFileOrDirectory2 - Delete a directory on a V3.X NetWare
 *		server from a volume supporting the UNIX name space.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NDeleteFileOrDirectory2( channel, parentHandle, name ) 
 *		void_t	*channel;
 *		NCP_DIRHANDLE_T	*parentHandle;
 *		char	*name;
 *
 * INPUT
 *		void_t	*channel;
 *		NCP_DIRHANDLE_T	*parentHandle;
 *		char	*name;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Creates a directory on the server assocated with the channel
 *		structure passed by using a 3.1 NCP call to the UNIX name space.
 *
 * NCP
 *	95 20
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsp3NCreateDirectory2(3k)
 *		NCPsp3NOpenFile2
 *		NCPsp3NCreateFile2
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NDeleteFileOrDirectory2_l (	ncp_channel_t		*channel,
									ncp_volume_t		*volumeHandle,
									NCP_DIRHANDLE_T		*parentHandle,
									char				*name,
									int32				nodeType,
									int32				nodeUniqueID,
									nwcred_t			*credPtr )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
		uint32	uid;
		uint32	gid;
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
	} *replyPacket;
#pragma pack()

	int32	repLen = sizeof (struct repPack);
	uint32	pathNodeType;
	uint32	componentPathLength = 0, componentCount = 0;

	NVLT_ENTER (7);

	/*
	 * Assumption:
	 *	The calling function must make sure that we get at least one of the
	 *	following:
	 *		parentHandle, nodeUniqueID, or name.
	 */


	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket->SubFunction = SFNUCDeleteFileOrSubdirectory2;
	requestPacket->reserved = 0;
	requestPacket->ncpVersion = 0;
	requestPacket->uid = NW_GET_UID (channel);
	/*
	 * We need to use the gid that is passed down from the NUCFS file system,
	 * since it can change (newgrp, etc.).
	 */
	NWtlGetCredGroupID (credPtr, &requestPacket->gid);
	requestPacket->VolumeNumber = volumeHandle->number;
	requestPacket->HandleFlag = DIRECTORY_BASE;
	if (nodeUniqueID != -1) {
		/*
		 * Have the unique ID of the file/directory to be deleted.
		 */
		requestPacket->DirectoryBaseOrShortHandle = REVGETINT32 (nodeUniqueID);
		pathNodeType = nodeType;
	} else {
		if (parentHandle) {
			/*
			 * Have the parent handle.
			 */
			requestPacket->DirectoryBaseOrShortHandle =
					REVGETINT32 (parentHandle->DirectoryBase);
			pathNodeType = nodeType;
		} else {
			/*
			 * Do not have the parent handle but have the full path.
			 * Set DirectoryBaseOrShortHandle to 0 and HandleFlag to
			 * DIRETORY_BASE to indicate root of the file system.
			 */
			requestPacket->DirectoryBaseOrShortHandle = 0;
			pathNodeType = NS_ROOT;
		} 

		if (name)
			ccode = NCPspConvertPathToComponents (name,
					(char *)requestPacket->PComponent, 300, &componentCount,
					&componentPathLength, pathNodeType);
	}

	if (ccode == SUCCESS) {
		requestPacket->PathComponentCount = (uint8)componentCount;

		request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;
		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
			sizeof (struct reqPack) + componentPathLength - 1;
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;
	
		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + repLen;
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
						ccode = SUCCESS;
						break;

					case E_NO_DELETE_PRIV:
						ccode = SPI_ACCESS_DENIED;
						break;

					case E_ALL_FILES_INUSE:
						ccode = SPI_FILE_IN_USE;
						break;

					case E_DIRECTORY_NOT_EMPTY:
						ccode = SPI_DIRECTORY_NOT_EMPTY;
						break;

					case E_INVALID_PATH:
						ccode = SPI_INVALID_PATH;
						break;

					default:
						ccode = SPI_GENERAL_FAILURE;
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
 * BEGIN_MANUAL_ENTRY(NCPsp3NRenameFileOrDirectory2.3k)
 * NAME
 *		NCPsp3NRenameFileOrDirectory2 - Rename a directory on a V3.x NetWare
 *		server on a volume supporting the UNIX name space.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NRenameFileOrDirectory2 ( channel, oldParentHandle, oldName, 
 *			nameSpace, nameSpaceInfo, newParentHandle, newName )
 *		void_t	*channel;
 *		uint8	oldParentHandle;
 *		char	*oldName;
 *		int32	nameSpace;
 *		void_t	*nameSpaceInfo;
 *		uint8	newParentHandle;
 *		char	*newName;
 *
 * INPUT
 *		void_t	*channel;
 *		uint8	oldParentHandle;
 *		char	*oldName;
 *		int32	nameSpace;
 *		void_t	*nameSpaceInfo;
 *		uint8	newParentHandle;
 *		char	*newName;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Renames a directory on the specified volume on the server assocated 
 *		with the channel structure passed by using the NUC NCP.
 *
 * NCP
 *	95 19
 *
 * NOTES
 *		This routine assumes both directory names are in UNIX semantics.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NRenameFileOrDirectory2_l (	ncp_channel_t		*channel,
									ncp_volume_t		*volume,
									NCP_DIRHANDLE_T		*oldParentHandle,
									char				*oldName,
									int32				nameSpace,
									NWSI_NAME_SPACE_T	*nameSpaceInfo,
									NCP_DIRHANDLE_T		*newParentHandle,
									char				*newName,
									nwcred_t			*credPtr )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
		uint32	uid;
		uint32	gid;
		/*
		 *	The following 4 fields comprise SrcNWHandlePathS1
		 */
		uint8	VolumeNumberS1;
		uint32	DirectoryBaseOrShortHandleS1;
		uint8	HandleFlagS1;
		uint8	PathComponentCountS1;
		/*
		 *	The following 4 fields comprise SrcNWHandlePathS2
		 */
		uint8	VolumeNumberS2;
		uint32	DirectoryBaseOrShortHandleS2;
		uint8	HandleFlagS2;
		uint8	PathComponentCountS2;
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
	} *replyPacket;
#pragma pack()

	struct {
		NUC_IOBUF_T	names;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);
	uint8		*p;
	uint32		componentCountS1 = 0, componentPathLengthS1 = 0;
	uint32		componentCountS2 = 0, componentPathLengthS2 = 0;
	int32		oldNodeType, newNodeType;

	NVLT_ENTER (9);

	/*
	 * Assumption:
	 *	The calling function must make sure that we get at least one of the
	 *	following:
	 *		oldParentHandle, nameSpaceInfo->nodeNumber, or oldName.
	 *		Must also get newParentHandle and the newName.
	 */

	if (NCPdplGetFreePacket_l (channel, &request)) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket->SubFunction = SFNUCRenameFileOrSubdirectory2;
	requestPacket->reserved = 0;
	requestPacket->ncpVersion = 0;
	requestPacket->uid = NW_GET_UID (channel);
	/*
	 * We need to use the gid that is passed down from the NUCFS file system,
	 * since it can change (newgrp, etc.).
	 */
	NWtlGetCredGroupID (credPtr, &requestPacket->gid);

	KAlloc->names.buffer = kmem_zalloc (512, KM_SLEEP);
	KAlloc->names.memoryType = IOMEM_KERNEL;

	/*
	 *	Get a pointer into the path strings area for moving in of the path
	 *	components of source and destination.  Then move 'em in.
	 */
	p = (uint8 *)KAlloc->names.buffer;

	requestPacket->HandleFlagS1 = DIRECTORY_BASE;
	requestPacket->VolumeNumberS1 = volume->number;
	if (nameSpaceInfo->nodeNumber != -1) {
		/*
		 * Have the unique ID of the file/directory to be renamed.
		 */
		requestPacket->DirectoryBaseOrShortHandleS1 =
					REVPUTINT32 (nameSpaceInfo->nodeNumber);
		oldNodeType = nameSpaceInfo->nodeType;
	} else {
		if (oldParentHandle) {
			/*
			 * Have the parent handle.
			 */
			requestPacket->DirectoryBaseOrShortHandleS1 =
					REVPUTINT32 (oldParentHandle->DirectoryBase);
			oldNodeType = nameSpaceInfo->nodeType;
		} else {
			/*
			 * Do not have the parent handle but have the full path.
			 * Set DirectoryBaseOrShortHandle to 0 and HandleFlag to
			 * DIRETORY_BASE to indicate root of the file system.
			 */
			requestPacket->DirectoryBaseOrShortHandleS1 = 0;
			oldNodeType = NS_ROOT;
		} 

		if (oldName) {
			ccode = NCPspConvertPathToComponents (oldName, (char *)p, 256,
					&componentCountS1, &componentPathLengthS1, oldNodeType);

			if (ccode != SUCCESS)
				goto done;
		}
	}

	/*
	 * Always have the new parent handle.
	 */
	requestPacket->DirectoryBaseOrShortHandleS2 =
				REVPUTINT32 (newParentHandle->DirectoryBase);
	requestPacket->HandleFlagS2 = DIRECTORY_BASE;
	requestPacket->VolumeNumberS2 = volume->number;
	newNodeType = nameSpaceInfo->nodeType;
	requestPacket->PathComponentCountS1 = (uint8)componentCountS1;

	/*
	 * Must always have the newName.
	 */
	ccode = NCPspConvertPathToComponents (newName,
			(char *)p + componentPathLengthS1, 256, &componentCountS2,
			&componentPathLengthS2, newNodeType);

	if (ccode == SUCCESS) {
		requestPacket->PathComponentCountS2 = (uint8)componentCountS2;
		KAlloc->names.bufferLength =
				componentPathLengthS1 + componentPathLengthS2;

		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
				sizeof (struct reqPack);
		request->ncpDataBuffer = &KAlloc->names;

		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
				sizeof (struct repPack);
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
						ccode = SUCCESS;
						break;

					case E_NO_RENAME_PRIV:
					case E_NO_MODIFY_PRIV:
						ccode = SPI_ACCESS_DENIED;
						break;

					case E_INVALID_PATH:
						ccode = SPI_INVALID_PATH;
						break;

					case E_ALL_FILES_INUSE:
						ccode = SPI_FILE_IN_USE;
						break;

					case E_RENAME_DIR_INVALID:
						ccode = SPI_INVALID_MOVE;
						break;
				
					case E_DIRECTORY_NOT_EMPTY:
						ccode = SPI_DIRECTORY_NOT_EMPTY;
						break;

					default:
						ccode = SPI_GENERAL_FAILURE;
						break;
				}
			}
		}
	}

done:
	kmem_free (KAlloc->names.buffer, 512);
	kmem_free (KAlloc, sizeof(*KAlloc));
	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NGetDirectoryEntries.3k)
 * NAME
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NGetDirectoryEntries( task, dirHandle, nameSpace, requestType,
 *			offset, buffer )
 *		void_t	*task;
 *		uint8	*dirHandle;
 *		uint32	nameSpace;
 *		uint8	requestType;
 *		uint32	*offset;
 *		NUC_IOBUF_T	*buffer;
 *
 * INPUT
 *		void_t	*task;
 *		uint8	*dirHandle;
 *		uint32	nameSpace;
 *		uint8	requestType;
 *		uint32	*offset;
 *
 * OUTPUT
 *		uint32	*offset;
 *		NUC_IOBUF_T	*buffer;
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		This function fills the caller-supplied buffer with NWSI_DIRECTORY_T
 *		structures describing file and subdirectory entries in the directory
 *		defined by the supplied directory handle.  It does this by 
 *		calling NCPsp3NSearchForFileOrSubdirectorySet with a struct
 *		searchStruct and a pointer to a data buffer.  The data buffer is
 *		preallocated because it becomes an extension to the packet.
 *		This routine attempts to guarantee that all information returned 
 *		from the server can fit in the caller-supplied NUC_IOBUF_T.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NGetDirectoryEntries2 (	ncp_channel_t	*channel,
								ncp_volume_t	*volume,
								NCP_DIRHANDLE_T	*dirHandle,
								uint32			nameSpace,
								uint8			requestType,
								uint32			*offset,
								NUC_IOBUF_T		*buffer,
								nwcred_t		*credPtr )
{
	ccode_t				ccode = SUCCESS;
	uint32				entryLength;
	NWSI_DIRECTORY_T	*dir;
	NUCAttributeStruct	*attr;
	uint8				*nameLength;
	uint32				FileNameLength, bufferUsed;
	Cookies3NS			myCookie;
	struct {
		NUC_IOBUF_T		myBuffer;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);
	uint16				entryCount;
	uint32				firstEntry = 0;
	uint8				*bp;	/* byte pointer */


	NVLT_ENTER (8);

	/*
	 *	Determine the state the search is currently in:  initialize, 
	 *	continue or terminate
	 */
	switch (requestType) {

		case GET_FIRST:
			/*
			 *	Initialize the search on the server
			 */
			ccode = NCPsp3NInitializeSearch (channel, volume, dirHandle,
					&myCookie);
			if (ccode != SUCCESS) {
				goto Finished;
			}

			/*
			 *	record fact that search has been initialized
			 */
			bp = (uint8 *) &firstEntry;
			bp[0] = myCookie.searchCookie[5];
			bp[1] = myCookie.searchCookie[6];
			bp[2] = myCookie.searchCookie[7];
			bp[3] = myCookie.searchCookie[8];
			break;

		case GET_MORE:
			bp = (uint8 *) &(dirHandle->DirectoryBase);
			myCookie.searchCookie[0] = volume->number;
			myCookie.searchCookie[1] = bp[0];
			myCookie.searchCookie[2] = bp[1];
			myCookie.searchCookie[3] = bp[2];
			myCookie.searchCookie[4] = bp[3];
			break;

		case GET_DONE:
			ccode = SUCCESS;
			goto Finished;

		default:
			/*
			 *	Return error since the request type is unknown
			 */
			ccode =  SPI_BAD_ARGS;
			goto Finished;
	}
	
	KAlloc->myBuffer.buffer = (NUC_IOBUF_T *)
			kmem_alloc (MAX_NEGOTIATED_BUFFER_SIZE, KM_SLEEP);

	/*
	 *	Ensure that all entries received from the server will fit in
	 *	the buffer provided by getting 17% fewer bytes from the server
	 *	than the size of the buffer.  This is done since NWSI_DIRECTORY_T
	 *	is currently slightly larger than the structure returned from the
	 *	server (by 8 bytes at last check).  Since a maximum of 17
	 *	entries can be retrieved from the server (each entry having a
	 *	one-byte name at packet size 1024), the max excess is 17*8==136 or 
	 *	about 14% of 1024.  In addition, if the packet size is smaller 
	 *	than the provided buffer size, we are more than safe since the
	 *	server will never exceed negotiated packet size no matter what
	 *	the request.
	 */
	KAlloc->myBuffer.bufferLength = (buffer->bufferLength *
		(sizeof (NUCAttributeStruct) + 2)) /
		(sizeof (NWSI_DIRECTORY_T) + 4);

	KAlloc->myBuffer.memoryType = IOMEM_KERNEL;

	/*
	 * offset 1 and 2 are handled in the NUCFS file system.  Either case
	 * we start the search at the beginning.
	 */

	if (*offset == 2 || *offset == 1)
		*offset = 0;

	if (*offset) {
		(*offset) >>= 2;
		bp = (uint8 *) offset;
		myCookie.searchCookie[5] = bp[0];
		myCookie.searchCookie[6] = bp[1];
		myCookie.searchCookie[7] = bp[2];
		myCookie.searchCookie[8] = bp[3];
	} else {
		/* Start at begining (indicated by -1) */
		myCookie.searchCookie[5] = 0xff;
		myCookie.searchCookie[6] = 0xff;
		myCookie.searchCookie[7] = 0xff;
		myCookie.searchCookie[8] = 0xff;
	}

	ccode = NCPsp3NSearchForFileOrSubdirectorySet2_l (channel,
				&KAlloc->myBuffer, &myCookie, &entryCount, credPtr);

	if (ccode != SUCCESS) {
		if ((ccode == SPI_SERVER_UNAVAILABLE) ||
				(ccode == SPI_NO_CONNECTIONS_AVAILABLE) ||
				(ccode == SPI_BAD_CONNECTION) || (ccode == SPI_SERVER_DOWN)) {
			/*
			 * There is no need to set the spil task mode to draining since
			 * NCPsp3NSearchForFileOrSubdirectorySet2_l has already done so.
			 */
			goto Finished1;
		} else {
			if ((KAlloc->myBuffer.bufferLength == 0) || (entryCount == 0)) {
				buffer->bufferLength = 0;
				ccode = SUCCESS;
				*offset = 0;
				goto Finished1;
			}

			goto Finished1;
		}
	}

	dir = (NWSI_DIRECTORY_T *)buffer->buffer;
	attr = (NUCAttributeStruct *)KAlloc->myBuffer.buffer;
	bufferUsed = 0;

	/*
	 *	Loop and add NWSI_DIRECTORY_T entries to the buffer until it is full
	 */
	while (entryCount--) {

		/*
		 *	Find the pointer to the length byte of the name.  The name 
		 *	immediately follows.
		 */
		nameLength = (uint8 *)attr + sizeof (NUCAttributeStruct); 

		/*	Calculate the length of the current entry rounded to the next word
		 *	including the NULL on the entry name.
		 */
		FileNameLength = *nameLength;
		if (FileNameLength == 0) {
			/*
			 * This is a bad entry.
			 */
			NVLT_PRINTF (
				"Bad entry for NCPsp3NSearchForFileOrSubdirectorySet2_l.\n",
				0, 0, 0);
			ccode = SPI_BAD_CONNECTION;
			*offset = 0;
			goto Finished1;
		}

		entryLength = sizeof (NWSI_DIRECTORY_T) + ((FileNameLength + 3) & ~3);
		if ((bufferUsed + entryLength) > buffer->bufferLength) {
			ccode = SUCCESS;
			break;
		}

		/*
		 *	Entry will fit:  consume it by moving it into the buffer:
		 *		Move the null-terminated name into the NWSI_DIRECTORY_T
		 *		Obtaining all applicable name space information
		 */
		bcopy (nameLength+1, dir->name, FileNameLength);
		dir->name[FileNameLength] = '\0';

		/*
		 *	Add the UNIX Name Space information from the NUCAttributeStruct
		 */
		NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct (&dir->nameSpaceInfo,
				attr);

		/*
		 *	Position to the next NWSI_DIRECTORY_T in the 
		 *	user buffer by adding the size of NWSI_DIRECTORY_T 
		 *	plus the length of the filename rounded up to 
		 *	the next multiple of four (to avoid word alignment
		 *	problems with some architectures)
		 */
		dir->structLength = entryLength;
		dir = (NWSI_DIRECTORY_T *) ((char *)dir + entryLength);

		/*
		 *	Position to the next NetWareInfoStruct in the
		 *	packet by adding the size of NetWareInfoStruct plus
		 *	the size of the NetWareFileNameStruct
		 */
		attr = (NUCAttributeStruct *)((char *)attr + sizeof(NUCAttributeStruct)
			+ FileNameLength + 1);
		/*
		 *	Record the number of bytes used by this entry
		 */
		bufferUsed += entryLength;
	}

	/*
	 *	Set number of buffer bytes used in the user-supplied bufferLength.
	 */
	buffer->bufferLength = bufferUsed;

	/*
	 *	If no errors, record the search cookie into the cookie table
	 *	entry at *offset +1.   
	 */

	if (ccode != SUCCESS) {
		*offset = 0;
	} else {

		/* save where we want search to continue */
		bp = (uint8 *) offset;
		bp[0] = myCookie.searchCookie[5];
		bp[1] = myCookie.searchCookie[6];
		bp[2] = myCookie.searchCookie[7];
		bp[3] = myCookie.searchCookie[8];
		*offset &= 0x00ffffff;
		(*offset) <<= 2;	/* 1 and 2 are reserved for "." and ".." */
	}

Finished1:
	/*
	 *	Free my packet buffer myBuffer and return any condition code.
	 */
	kmem_free (KAlloc->myBuffer.buffer, MAX_NEGOTIATED_BUFFER_SIZE);
Finished:
	kmem_free	(KAlloc, sizeof(*KAlloc));
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NSearchForFileOrSubdirectorySet2.3k)
 * NAME
 *		NCPsp3NSearchForFileOrSubdirectorySet2 - Get the next entry in the
 *			directory
 *
 * SYNOPSIS
 *		NCPsp3NSearchForFileOrSubdirectorySet2( s, buffer, cookie, entryCount )
 *		struct searchStruct *s;
 *		NUC_IOBUF_T			*buffer;
 *		Cookies3NS			*cookie;
 *		uint16				*entryCount;
 *
 * INPUT
 *		struct searchStruct *s;
 *		NUC_IOBUF_T			*buffer;
 *		Cookies3NS			*cookie;
 *
 * OUTPUT
 *		NUC_IOBUF_T			*buffer;
 *		Cookies3NS			*cookie;
 *		uint16				*entryCount;
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_GENERAL_FAILURE
 *		SPI_SERVER_UNAVAILABLE
 *
 * DESCRIPTION
 *		Performs a search of a directory returing the files and
 *		subdirectories based on the search pattern and the search attributes.
 *		Return to the caller with a packet containing as many directory
 *		entries as will fit in the packet or available in the subdirectory.
 *		If no more entries exist, return SPI_NO_MORE_ENTRIES.
 *
 *	NCP
 *	 95 25
 *
 * NOTES
 *		The caller provides a packet buffer in searchContext because the
 *		buffer he is filling may not be large enough to contain all the 
 *		entries returned from the server.  He buffers them in the receive
 *		packet and doesn't call this function again until more entries are
 *		needed.
 *
 * SEE ALSO
 *		NCPsp3NGetDirEntries(3k)
 *		NCPsp3NInitializeSearch(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NSearchForFileOrSubdirectorySet2_l (	ncp_channel_t	*channel,
											NUC_IOBUF_T		*buffer,
											Cookies3NS		*cookie,
											uint16			*entryCount,
											nwcred_t		*credPtr )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
		uint32	uid;
		uint32	gid;
		uint8	SearchSequence[9];
		uint8	Reserved;
		uint8	EntriesWanted[2];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint8	NextSearchSequence[9];
		uint8	MoreEntriesFlag;
		uint8	EntryCount[2];
	} *replyPacket;
#pragma pack()

	NVLT_ENTER (5);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;

	requestPacket->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket->SubFunction = SFNUCGetDirectoryEntries2;
	requestPacket->reserved = 0;
	requestPacket->ncpVersion = 0;
	requestPacket->uid = NW_GET_UID (channel);
	/*
	 * We need to use the gid that is passed down from the NUCFS file system,
	 * since it can change (newgrp, etc.).
	 */
	NWtlGetCredGroupID (credPtr, &requestPacket->gid);

	/*
	 *	Request as many as possible, let the packet size and the length
	 *	of the names dictate the maximum number returned
	 */
	MOVELE_INT16 (requestPacket->EntriesWanted, ((uint16)buffer->bufferLength));
	bcopy (cookie->searchCookie, requestPacket->SearchSequence, 
			NCP_SEARCH_COOKIE_LENGTH);
	
	request->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
	reply->ncpDataBuffer = buffer;

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
					/*
					 *	Save the search context for the next call
					 */
					bcopy (replyPacket->NextSearchSequence, 
						cookie->searchCookie, NCP_SEARCH_COOKIE_LENGTH);
					*entryCount =
						REVGETINT16 (*(uint16 *) replyPacket->EntryCount);
					ccode = SUCCESS;
					break;

				case E_ALL_FILES_INUSE:
					buffer->bufferLength = 0;
					ccode = SPI_FILE_IN_USE;
					break;

				default:
					buffer->bufferLength = 0;
					ccode = SPI_NO_MORE_ENTRIES;
					break;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */
 
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NSetNameSpaceInformation2.3k)
 * NAME
 *		NCPsp3NSetNameSpaceInformation2 - Set UNIX information from a given
 *		NWSI_NAME_SPACE_T structure
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NSetNameSpaceInformation2( channel, volumeHandle, name,
 *			nameSpaceInfo )
 *		void_t			*channel;
 *		NCP_DIRHANDLE_T	*dirHandle;
 *		uint8			*name;
 *		void_t			*nameSpaceInfo;
 *
 * INPUT
 *		void_t			*channel;
 *		NCP_DIRHANDLE_T	*dirHandle;
 *		uint8			*name;
 *		void_t			*nameSpaceInfo;
 *
 * OUTPUT
 *		nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		Update the the DOS and UNIX name space information from data in the 
 *		NWSI_NAME_SPACE_T structure.
 *
 * NCP
 *	95 23
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NSetNameSpaceInformation2_l (	ncp_channel_t		*channel,
									ncp_volume_t		*volume,
									NCP_DIRHANDLE_T		*dirHandle,
									char				*name,
									NWSI_NAME_SPACE_T	*nameSpaceInfo,
									nwcred_t			*credPtr )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
		uint32	uid;
		uint32	gid;
		NUCAttributeStruct attr;
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
	} *replyPacket;
#pragma pack()

	uint32	componentCount = 0, componentPathLength = 0;
	int32	nodeType;

	NVLT_ENTER (6);

    /*
	 * Assumption:
	 *  The calling function must make sure that we get at least one of the
	 *  following:
	 *      dirHandle, nameSpaceInfo->nodeNumber, or name.
	 */

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket->SubFunction = SFNUCSetAttributes2;
	requestPacket->reserved = 0;
	requestPacket->ncpVersion = 0;
	requestPacket->uid = NW_GET_UID (channel);
	/*
	 * We need to use the gid that is passed down from the NUCFS file system,
	 * since it can change (newgrp, etc.).
	 */
	NWtlGetCredGroupID (credPtr, &requestPacket->gid);

	requestPacket->HandleFlag = DIRECTORY_BASE;
	requestPacket->VolumeNumber = volume->number;
	if (nameSpaceInfo->nodeNumber != -1) {
		/*
		 * Have the unique ID of the file/directory to be changed.
		 */
		requestPacket->DirectoryBaseOrShortHandle =
				REVPUTINT32 (nameSpaceInfo->nodeNumber);
		nodeType = nameSpaceInfo->nodeType;
	} else {
		if (dirHandle) {
			/*
			 * Have the parent handle.
			 */
			requestPacket->DirectoryBaseOrShortHandle =
					REVGETINT32 (dirHandle->DirectoryBase);
			nodeType = nameSpaceInfo->nodeType;
		} else {
			/*
			 * Do not have the parent handle but have the full path.
			 * Set DirectoryBaseOrShortHandle to 0 and HandleFlag to
			 * DIRETORY_BASE to indicate root of the file system.
			 */
			requestPacket->DirectoryBaseOrShortHandle = 0;
			nodeType = NS_ROOT;
		} 

		if (name)
			ccode = NCPspConvertPathToComponents (name,
					(char *)requestPacket->PComponent, 300, &componentCount,
					&componentPathLength, nodeType);
	}

	if (ccode == SUCCESS) {

		requestPacket->PathComponentCount = (uint8)componentCount;
		NCPsp3NPopulateNUCAttributesFromNameSpaceInfo (&requestPacket->attr, 
				nameSpaceInfo);

		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
			sizeof (struct reqPack) + componentPathLength -1;
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

				/*
				 *	Verify the completion code
				 */
				switch (replyPacket->completionCode) {
					case SUCCESS:
						ccode = SUCCESS;
						break;

					case E_ALL_FILES_INUSE:
						ccode = SPI_FILE_IN_USE;
						break;

					case E_NO_MODIFY_PRIV:
						ccode = SPI_ACCESS_DENIED;
						break;

					case E_INVALID_PATH:
						ccode = SPI_INVALID_PATH;
						break;

					default:
						ccode = SPI_SET_NAME_SPACE_DENIED;
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
 * BEGIN_MANUAL_ENTRY(NCPsp3NGetNameSpaceInformation2.3k)
 * NAME
 *		NCPsp3NGetNameSpaceInformation2 - Get UNIX information for a given
 *		UNIX directory entry.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NGetNameSpaceInformation2( channel, nameSpaceInfo )
 *		void_t			*channel;
 *		NCP_DIRHANDLE_T	*dirHandle;
 *		char			*name;
 *		void_t			*nameSpaceInfo;
 *
 * INPUT
 *		void_t			*channel;
 *		NCP_DIRHANDLE_T	*dirHandle;
 *		char			*name;
 *
 * OUTPUT
 *		void_t	*nameSpaceInfo;
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		Acquire information about a file or directory.  A passed 
 *		NWSI_NAME_SPACE_T structure is populated with name space information
 *		in UNIX semantics.
 *
 * NCP
 *	95 22
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NGetNameSpaceInformation2_l (	ncp_channel_t		*channel,
									ncp_volume_t		*volume,
									NCP_DIRHANDLE_T		*dirHandle,
									char				*name,
									NWSI_NAME_SPACE_T	*nameSpaceInfo,
									nwcred_t			*credPtr )
{

	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
		uint32	uid;
		uint32	gid;
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
		NUCAttributeStruct attr;
		uint8	name[256];
	} *replyPacket;
#pragma pack()

	int32		nodeType;
	uint32		componentCount = 0, componentPathLength = 0;
	uint32		tempSize;
	void_t		*fileHandle;
	boolean_t	freePacketFlag = TRUE;

	NVLT_ENTER (6);

	/*
	 * Assumption:
	 *	The calling function must make sure that we get at least one of the
	 *	following:
	 *		dirHandle, nameSpaceInfo->nodeNumber, or name.
	 */

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket->SubFunction = SFNUCGetAttributes2;
	requestPacket->reserved = 0;
	requestPacket->ncpVersion = 0;
	requestPacket->uid = NW_GET_UID (channel);
	/*
	 * We need to use the gid that is passed down from the NUCFS file system,
	 * since it can change (newgrp, etc.).
	 */
	NWtlGetCredGroupID (credPtr, &requestPacket->gid);

	requestPacket->HandleFlag = DIRECTORY_BASE;
	requestPacket->VolumeNumber = volume->number;
	if (nameSpaceInfo->nodeNumber != -1) {
		/*
		 * Have the unique ID of the file/directory.
		 */
		requestPacket->DirectoryBaseOrShortHandle =
				REVPUTINT32 (nameSpaceInfo->nodeNumber);
		nodeType = nameSpaceInfo->nodeType;
	} else {
		if (dirHandle) {
			/*
			 * Have the parent handle.
			 */
			requestPacket->DirectoryBaseOrShortHandle =
					REVPUTINT32 (dirHandle->DirectoryBase);
			nodeType = nameSpaceInfo->nodeType;
		} else {
			/*
			 * Do not have the parent handle but have the full path.
			 * Set DirectoryBaseOrShortHandle to 0 and HandleFlag to
			 * DIRETORY_BASE to indicate root of the file system.
			 */
			requestPacket->DirectoryBaseOrShortHandle = 0;
			nodeType = NS_ROOT;
		} 

		if (name)
			ccode = NCPspConvertPathToComponents (name,
					(char *)requestPacket->PComponent, 300, &componentCount,
					&componentPathLength, nodeType);
	}

	if (ccode == SUCCESS) {
		requestPacket->PathComponentCount = (uint8)componentCount;
		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
			sizeof(struct reqPack) + componentPathLength -1;
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
		reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		ccode = NCPdiTransaction_l (channel, request, reply);
		if (ccode == SUCCESS) {
			/*
			 *	Overlay the replyPacket template into the header data
			 */
			replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

			/*
			 *	check connection status on every request to make sure
			 *	a window wasn't hit
			 */
			if (replyPacket->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
				ccode = SPI_NO_CONNECTIONS_AVAILABLE;
			} else if (replyPacket->connectionStatus & CS_BAD_CONNECTION) {
				ccode = SPI_BAD_CONNECTION;
			} else if (replyPacket->connectionStatus & CS_SERVER_DOWN) {
				ccode = SPI_SERVER_DOWN;
			} else {

				/*
				 *	Check the completion code and act accordingly
				 */
				switch (replyPacket->completionCode) {
					case SUCCESS:
						/*
						 *	Convert the NUCAttributeStruct data to the
						 *	NWSI_NAME_SPACE_T format wanted by the file system
						 */
						NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct (
							nameSpaceInfo, &replyPacket->attr);

						/*
						 * If file system knows that the file is open by us,
						 * we need to get the real node size.
						 */
						if (nameSpaceInfo->openFileHandle != NULL) {
							NCPdplFreePacket (request);
							NCPdplFreePacket (reply);
							freePacketFlag = FALSE;
							NWslGetHandleSProtoResHandle (
								nameSpaceInfo->openFileHandle,
								&fileHandle);
							if (!NCPsp2XGetCurrentSizeOfFile_l (channel,
								fileHandle, &tempSize))
								nameSpaceInfo->nodeSize = tempSize;
							else {
								ccode = SPI_NO_ACTUAL_SIZE;
								break;
							}
						}
						ccode = SUCCESS;
						break;

					case E_NO_SEARCH_PRIV:
						ccode = SPI_ACCESS_DENIED;
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
	}

	if (freePacketFlag) {
		NCPdplFreePacket (request);		/* done with it, give it back to pool */
		NCPdplFreePacket (reply);		/* done with it, give it back to pool */
	}

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NLinkFile2.3k)
 * NAME
 *		NCPsp3NLinkFile2 - Set UNIX information from a given
 *		NWSI_NAME_SPACE_T structure
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NLinkFile2( taskPtr, curDirHandle, curName, curNameSpaceInfo,
 *			newPath, newNameSpaceInfo )
 *		ncp_task_t		*taskPtr;
 *		NCP_DIRHANDLE_T	curDirHandle;
 *		char			*curName;
 *		NWSI_NAME_SPACE_T *curNameSpaceInfo;
 *		uint8			*newPath;
 *		NWSI_NAME_SPACE_T *newNameSpaceInfo;
 *		
 *
 * INPUT
 *		ncp_task_t		*taskPtr;
 *		NCP_DIRHANDLE_T	curDirHandle;
 *		char			*curName;
 *		NWSI_NAME_SPACE_T *curNameSpaceInfo;
 *		uint8			*newPath;
 *		NWSI_NAME_SPACE_T *newNameSpaceInfo;
 *
 * OUTPUT
 *		nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		Link the newPath with curName
 *
 * NCP
 *	95 21
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
#define MAX_HUGE_DATA_LENGTH 480
ccode_t
NCPsp3NLinkFile2_l (	ncp_channel_t		*channel,
						ncp_volume_t		*volume,
						NCP_DIRHANDLE_T		*curDirHandle,
						char				*curName,
						int32				curUniqueID,
						NCP_DIRHANDLE_T		*newDirHandle,
						char				*newName,
						nwcred_t			*credPtr )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
		uint32	uid;
		uint32	gid;
		/*
		 *	The following 4 fields comprise the modified NUCHandlePathStruct 
		 *	for the current entry
		 */
		uint8	CurVolumeNumber;
		uint32	CurDirectoryBase;
		uint8	CurHandleFlag;
		uint8	CurPathComponentCount;
		/*
		 *	The following 4 fields comprise the modified NUCHandlePathStruct 
		 *	for the entry to be created
		 */
		uint8	NewVolumeNumber;
		uint32	NewDirectoryBase;
		uint8	NewHandleFlag;
		uint8	NewPathComponentCount;
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
	} *replyPacket;
#pragma pack()

	struct {
		NUC_IOBUF_T	names;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);
	uint32		curComponentCount = 0, curComponentPathLength = 0;
	uint32		newComponentCount = 0, newComponentPathLength = 0;
	uint8		*p = NULL;
	uint32		curNodeType, newNodeType;

	NVLT_ENTER (8);

	/*
	 * Assumption:
	 *	The calling function must make sure that we get at least one of the
	 *	following:
	 *		curDirHandle, currUniqueID, or curName.
	 *	Also must get newDirHandle and newName.
	 */

	if (NCPdplGetFreePacket_l (channel, &request)) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	NCPdplBuildNCPHeader_l (channel, request);

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket->SubFunction = SFNUCLinkFile2;
	requestPacket->reserved = 0;
	requestPacket->ncpVersion = 0;
	requestPacket->uid = NW_GET_UID (channel);
	/*
	 * We need to use the gid that is passed down from the NUCFS file system,
	 * since it can change (newgrp, etc.).
	 */
	NWtlGetCredGroupID (credPtr, &requestPacket->gid);

	requestPacket->CurHandleFlag = DIRECTORY_BASE;
	requestPacket->CurVolumeNumber = volume->number;
	if (curUniqueID != -1) {
		/*
		 * Using the unique ID of the file to link to.
		 */
		requestPacket->CurDirectoryBase = REVPUTINT32 (curUniqueID);
		curNodeType = NS_FILE;
	} else {
		if (curDirHandle) {
			/*
			 * Have the parent handle of the old link.
			 */
			requestPacket->CurDirectoryBase =
					REVPUTINT32 (curDirHandle->DirectoryBase);
			curNodeType = NS_FILE;
		} else {
			/*
			 * Have the path of the old link.
			 */
			requestPacket->CurDirectoryBase = 0;
			curNodeType = NS_ROOT;
		}

		if (curName) {
			ccode = NCPspConvertPathToComponents (curName, (char *)p, 256,
				&curComponentCount, &curComponentPathLength, curNodeType);
		
			if (ccode != SUCCESS)
				goto done;
		}
	}

	/*
	 * Have the parent handle of the parent new directory.
	 */
	requestPacket->NewDirectoryBase = REVPUTINT32 (newDirHandle->DirectoryBase);
	requestPacket->NewHandleFlag = DIRECTORY_BASE;
	requestPacket->NewVolumeNumber = volume->number;
	newNodeType = NS_FILE;

	requestPacket->CurPathComponentCount = (uint8)curComponentCount;

	KAlloc->names.memoryType = IOMEM_KERNEL;
	KAlloc->names.buffer = kmem_zalloc (512, KM_SLEEP);
	p = (uint8 *)KAlloc->names.buffer;

	/*
	 * The newName is always passed in.
	 */
	ccode = NCPspConvertPathToComponents (newName,
				(char *)p+curComponentPathLength,
				256, &newComponentCount, &newComponentPathLength, newNodeType);

	if (ccode == SUCCESS) {
		requestPacket->NewPathComponentCount = (uint8)newComponentCount;
		KAlloc->names.bufferLength = curComponentPathLength +
			newComponentPathLength;
 
		request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;
		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
				sizeof (struct reqPack);
		request->ncpDataBuffer = &KAlloc->names;

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
						ccode = SUCCESS;
						break;

					case E_NO_CREATE_PRIV:
					case E_NO_SEARCH_PRIV:
						ccode = SPI_ACCESS_DENIED;
						break;

					case E_INVALID_PATH:
						ccode = SPI_INVALID_PATH;
						break;

					case E_ALL_FILES_INUSE:
						ccode = SPI_FILE_IN_USE;
						break;

					default:
						ccode = SPI_TOO_MANY_LINKS;
						break;
				}
			}
		}
	}

done:
	kmem_free (KAlloc->names.buffer, 512);
	kmem_free(KAlloc, sizeof(*KAlloc));

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NOpenFile2.3k)
 * NAME
 *		NCPsp3NOpenFile
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NOpenFile2( channel, volumeHandle, parentDirHandle, 
 *			name, desiredAccessRights, nameSpaceInfo, fileHandle )
 *		ncp_channel_t	*channel;
 *		ncp_volume_t	*volumeHandle;
 *		NCP_DIRHANDLE_T	*parentDirHandle;
 *		char			*name;
 *		uint32			desiredAccessRights;
 *		NWSI_NAME_SPACE_T *nameSpaceInfo;
 *		uint8			**fileHandle;
 *
 * INPUT
 *		ncp_channel_t	*channel;
 *		ncp_volume_t	*volumeHandle;
 *		NCP_DIRHANDLE_T	*parentDirHandle;
 *		char			*name;
 *		uint32			desiredAccessRights;
 *		NWSI_NAME_SPACE_T *nameSpaceInfo;
 *		uint8			**fileHandle;
 *
 * OUTPUT
 *		uint8			**fileHandle;
 *
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		This function opens a file in the UNIX name space and returns a 
 *		pointer to a file handle to be used for read and write requests.
 * 
 * NCP
 *	95 17
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsp3NCloseFile2(3k)
 *		NCPsp2XReadFile(3k)
 *		NCPsp2XWriteFile(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NOpenFile2_l (	ncp_channel_t		*channel,
						ncp_volume_t		*volumeHandle,
						NCP_DIRHANDLE_T		*parentDirHandle,
						char				*name,
						uint32				desiredAccessRights,
						NWSI_NAME_SPACE_T	*nameSpaceInfo,
						uint8				**fileHandle,
						nwcred_t			*credPtr )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
		uint32	uid;
		uint32	gid;
		uint8	DesiredAccessRights[2];
		/*
		 *	The following 5 fields comprise the NUCHandlePathStruct
		 */
		uint8	VolumeNumber;
		uint32	DirectoryBase;
		uint8	HandleFlag;
		uint8	PathComponentCount;
		uint8	PComponent[1];
	} *requestPacket;
	
	struct repPack {
		uint8	CompletionCode;
		uint8	ConnectionStatus;
		uint8   FileHandle[4];
		NUCAttributeStruct attr;
	} *replyPacket;
#pragma pack()

	uint32	componentPathLength = 0, componentCount = 0;
	int32	nodeType;

	NVLT_ENTER (8);

	/*
	 * Assumption:
	 *	The calling function must make sure that we get at least one of the
	 *	following:
	 *		parentDirHandle, nameSpaceInfo->nodeNubmer, or name.
	 */

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	/*
	 *	set the connection number, type and function/subfunction
	 */
	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket->SubFunction = SFNUCOpenFile2;
	requestPacket->reserved = 0;
	requestPacket->ncpVersion = 0;
	requestPacket->uid = NW_GET_UID (channel);
	/*
	 * We need to use the gid that is passed down from the NUCFS file system,
	 * since it can change (newgrp, etc.).
	 */
	NWtlGetCredGroupID (credPtr, &requestPacket->gid);

	MOVELE_INT16 (requestPacket->DesiredAccessRights, desiredAccessRights);

	requestPacket->VolumeNumber = volumeHandle->number;
	requestPacket->HandleFlag = DIRECTORY_BASE;
	if (nameSpaceInfo->nodeNumber != -1) {
		/*
		 * Have the unique ID of the file/directory to be opened.
		 */
		requestPacket->DirectoryBase = REVPUTINT32 (nameSpaceInfo->nodeNumber);
		nodeType = nameSpaceInfo->nodeType;
	} else {
		if (parentDirHandle) {
			/*
			 * Have the parent handle.
			 */
			requestPacket->DirectoryBase =
					REVPUTINT32 (parentDirHandle->DirectoryBase);
			nodeType = nameSpaceInfo->nodeType;
		} else {
			/*
			 * Do not have the parent handle but have the full path.
			 * Set DirectoryBaseOrShortHandle to 0 and HandleFlag to
			 * DIRETORY_BASE to indicate root of the file system.
			 */
			requestPacket->DirectoryBase = 0;
			nodeType = NS_ROOT;
		} 

		if (name)
			ccode = NCPspConvertPathToComponents (name,
					(char *)requestPacket->PComponent, 300, &componentCount,
					&componentPathLength, nodeType);
	}

	if (ccode == SUCCESS) {
		requestPacket->PathComponentCount = (uint8)componentCount;

		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
			sizeof (struct reqPack) + componentPathLength - 1;
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
		reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		ccode = NCPdiTransaction_l (channel, request, reply);
		if (ccode == SUCCESS) {
			/*
			 *	Overlay the array with the packet template
			 *
			 */
			replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;
			if (replyPacket->ConnectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
				ccode = SPI_NO_CONNECTIONS_AVAILABLE;
			} else if (replyPacket->ConnectionStatus & CS_BAD_CONNECTION) {
				ccode = SPI_BAD_CONNECTION;
			} else if (replyPacket->ConnectionStatus & CS_SERVER_DOWN) {
				ccode = SPI_SERVER_DOWN;
			} else {
				switch (replyPacket->CompletionCode) {
					case SUCCESS:
						/*
						 *	Give the returned file handle to the caller
						 */
						*fileHandle = (uint8 *)kmem_alloc (
							sizeof(NCP_FILEHANDLE_T), KM_SLEEP);
						(*fileHandle)[2] = replyPacket->FileHandle[0];
						(*fileHandle)[3] = replyPacket->FileHandle[1];
						(*fileHandle)[4] = replyPacket->FileHandle[2];
						(*fileHandle)[5] = replyPacket->FileHandle[3];
						ccode = SUCCESS;

						/*
						 *	Get the name space information goodies 
						 */
						NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct (
							nameSpaceInfo, &replyPacket->attr);
						break;

					case E_NO_OPEN_PRIV:
					case E_NO_READ_PRIV:
					case E_NO_WRITE_PRIV:
					case E_ALL_READ_ONLY:
						ccode = SPI_ACCESS_DENIED;
						break;

					case E_DIRECTORY_FULL:
						ccode = SPI_DIRECTORY_FULL;
						break;

					case E_LOCK_FAIL:
					case E_ALL_FILES_INUSE:
						ccode = SPI_FILE_IN_USE;
						break;

					case E_SERVER_NO_MEMORY:
						ccode = SPI_SERVER_RESOURCE_SHORTAGE;
						break;

					default:
						ccode = SPI_NODE_NOT_FOUND;
						break;
				}
			}
		}
	}

	NCPdplFreePacket (request);	/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp3NCreateFileOrSubdirectory2.3k)
 * NAME
 *		NCPsp3NCreateFileOrSubdirectory2
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp3NCreateFileOrSubdirectory2( channel, parentDirHandle, name, 
 *			nameSpaceInfo )
 *		ncp_channel_t	*channel;
 *		NCP_DIRHANDLE_T	*parentDirHandle;
 *		char			*name;
 *		int32			openFlags;
 *		void_t			**objectHandle;
 *		NWSI_NAME_SPACE_T *nameSpaceInfo;
 *
 * INPUT
 *		ncp_channel_t	*channel;
 *		NCP_DIRHANDLE_T	*parentDirHandle;
 *		char			*name;
 *		int32			openFlags;
 *		NWSI_NAME_SPACE_T *nameSpaceInfo;
 *
 * OUTPUT
 *		void_t			**objectHandle;
 *		NWSI_NAME_SPACE_T *nameSpaceInfo;
 *
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		This function creates a file in the UNIX name space.
 * 
 * NCP
 *	95 18
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsp3NCloseFile2(3k)
 *		NCPsp2XReadFile(3k)
 *		NCPsp2XWriteFile(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp3NCreateFileOrSubdirectory2_l (	ncp_channel_t		*channel,
										ncp_volume_t		*volume,
										NCP_DIRHANDLE_T		*parentDirHandle,
										char				*name,
										int32				openFlags,
										uint8				**objectHandle,
										NWSI_NAME_SPACE_T	*nameSpaceInfo,
										nwcred_t			*credPtr )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t	*request;
	iopacket_t	*reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
		uint32	uid;
		uint32	gid;
		uint8	DesiredAccessRights[2];
		NUCAttributeStruct attr;
		/*
		 *	The following 5 fields comprise the NUCHandlePathStruct
		 */
		uint8	VolumeNumber;
		uint32	DirectoryBase;
		uint8	HandleFlag;
		uint8	PathComponentCount;
		uint8	PComponent[1];
	} *requestPacket;

	struct repPack {
		uint8	CompletionCode;
		uint8	ConnectionStatus;
		uint8	FileHandle[4];
		NUCAttributeStruct attr;
	} *replyPacket;
#pragma pack()

	uint32	componentPathLength = 0, componentCount = 0;
	int32	nodeType;

	NVLT_ENTER (8);

	/*
	 * Assumption:
	 *	The calling function must make sure that we get the following:
	 *		parentDirHandle, and the name.
	 */

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	/*
	 *	set the connection number, type and function/subfunction
	 */
	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;
	requestPacket->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket->SubFunction = SFNUCCreateFileOrSubdirectory2;
	requestPacket->reserved = 0;
	requestPacket->ncpVersion = 0;
	requestPacket->uid = NW_GET_UID (channel);
	/*
	 * We need to use the gid that is passed down from the NUCFS file system,
	 * since it can change (newgrp, etc.).
	 */
	NWtlGetCredGroupID (credPtr, &requestPacket->gid);

	MOVELE_INT16 (requestPacket->DesiredAccessRights, openFlags);
	NCPsp3NPopulateNUCAttributesFromNameSpaceInfo (&requestPacket->attr,
			nameSpaceInfo);
	requestPacket->DirectoryBase = REVPUTINT32 (parentDirHandle->DirectoryBase);
	requestPacket->HandleFlag = DIRECTORY_BASE;		
	requestPacket->VolumeNumber = volume->number;
	nodeType = nameSpaceInfo->nodeType;

	/*
	 * The name is always passed in.
	 */
	ccode = NCPspConvertPathToComponents (name,
				(char *)requestPacket->PComponent,
				300, &componentCount, &componentPathLength, nodeType);

	if (ccode == SUCCESS) {
		requestPacket->PathComponentCount = (uint8)componentCount;

		request->ncpHeaderSize = NCP_HEADER_CONST_SIZE +
			sizeof (struct reqPack) + componentPathLength - 1;
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
		reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		ccode = NCPdiTransaction_l (channel, request, reply);
		if (ccode == SUCCESS) {
			/*
			 *	Overlay the array with the packet template
			 *
			 */
			replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;
			if (replyPacket->ConnectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
				ccode = SPI_NO_CONNECTIONS_AVAILABLE;
			} else if (replyPacket->ConnectionStatus & CS_BAD_CONNECTION) {
				ccode = SPI_BAD_CONNECTION;
			} else if (replyPacket->ConnectionStatus & CS_SERVER_DOWN) {
				ccode = SPI_SERVER_DOWN;
			} else {
				switch (replyPacket->CompletionCode) {
					case SUCCESS:
						/*
						 *	Save off the new open file handle if creating a file
						 */
						if ((nameSpaceInfo->nodeType == NS_FILE) ||
								(nameSpaceInfo->nodeType == NS_SYMBOLIC_LINK)) {
							*objectHandle = (uint8 *)kmem_alloc (
								sizeof(NCP_FILEHANDLE_T), KM_SLEEP);
							(*objectHandle)[2] = replyPacket->FileHandle[0];
							(*objectHandle)[3] = replyPacket->FileHandle[1];
							(*objectHandle)[4] = replyPacket->FileHandle[2];
							(*objectHandle)[5] = replyPacket->FileHandle[3];
						}

						/*
						 *	Update the Name Space Information
						 */
						NCPsp3NPopulateNameSpaceInfoFromNUCAttributeStruct(
							nameSpaceInfo, &replyPacket->attr);
						ccode = SUCCESS;
						break;

					case E_NO_CREATE_PRIV:
					case E_ALL_READ_ONLY:
						ccode = SPI_ACCESS_DENIED;
						break;

					case E_ALL_FILES_INUSE:
						ccode = SPI_FILE_IN_USE;
						break;

					case E_DIRECTORY_FULL:
						ccode = SPI_DIRECTORY_FULL;
						break;

					case E_NODE_EXISTS:
						ccode = SPI_FILE_ALREADY_EXISTS;
						break;

					case E_SERVER_NO_MEMORY:
						ccode = SPI_SERVER_RESOURCE_SHORTAGE;
						break;

					case E_INVALID_PATH:
						ccode = SPI_INVALID_PATH;
						break;

					default:
						ccode = SPI_GENERAL_FAILURE;
						break;
				}
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}
