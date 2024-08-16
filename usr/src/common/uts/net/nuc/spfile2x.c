/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spfile2x.c	1.17"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spfile2x.c,v 2.53.2.6 1995/02/13 07:55:23 stevbam Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: spfile2x.c
 *	ABSTRACT: NetWare V2.1X File manipulation NCP's 
 *
 *	Functions declared in this module:
 *		NCPsp2XCloseFile
 *		NCPsp2XReadFile
 *		NCPsp2XWriteFile
 *		NCPsp2XLockFile
 *		NCPsp2XUnlockFile
 *		NCPsp2XGetCurrentSizeOfFile
 */ 

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <util/cmn_err.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/slstruct.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_ncp

/*
 *	Forward references
 */
ccode_t NCPsp2XCloseFile_l (ncp_channel_t *, uint8 *);
ccode_t NCPsp2XGetCurrentSizeOfFile_l (ncp_channel_t *, uint8 *, uint32 *);

/*
 * Define Constants
 */
#define UINT16_MAX		((1 << 16) - 1)
#define XFER_MAX		(UINT16_MAX - 8192)

/*
 * BEGIN_MANUAL_ENTRY(NCPsp2XCloseFile.3k)
 * NAME
 *		NCPsp2XCloseFile - NetWare V2.X file close operation
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp2XCloseFile( channel, fileHandle )
 *		ncp_channel_t	*channel;
 *		uint8	*fileHandle;
 *
 * INPUT
 *		ncp_channel_t	*channel;
 *		uint8	*fileHandle;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_LOCK_SHORTAGE
 *
 * DESCRIPTION
 *		Relinquish an open channel to a file on a V2.x compatibile
 *		file server.
 *
 * NCP
 *	66
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsp2XOpenFile(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp2XCloseFile_l (	ncp_channel_t	*channel,
						uint8			*fileHandle )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	function;
		uint8	reserved;
		uint8	fileHandle[NCP_FHANDLE_LENGTH];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
	} *replyPacket;
#pragma pack()

	NVLT_ENTER (2);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	/*
	 *	Initialize the packet header.
	 */
	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	/*
	 *	Cast the request structure into the data block of the
	 *	packet.
	 */
	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;

	/*
	 *	Set the function code for this request, and other
	 *	function specific fields
	 */
	requestPacket->function = FNCLOSE_FILE;
	bcopy (fileHandle, requestPacket->fileHandle, NCP_FHANDLE_LENGTH);

	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof (struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize =	NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
	reply->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	ccode = NCPdiTransaction_l (channel, request, reply);
	if (ccode == SUCCESS) {
		replyPacket = (struct repPack *)reply->ncpU.ncpHdr.data;

		/*
		 *	Ensure connection is sane
		 */
		if (replyPacket->connectionStatus & CS_NO_CONNECTIONS_AVAILABLE) {
			ccode = SPI_NO_CONNECTIONS_AVAILABLE;
		} else if (replyPacket->connectionStatus & CS_BAD_CONNECTION) {
			ccode = SPI_BAD_CONNECTION;
		} else if (replyPacket->connectionStatus & CS_SERVER_DOWN) {
			ccode = SPI_SERVER_DOWN;
		} else {

			/*
			 *	Check status of the request
			 */
			switch (replyPacket->completionCode) {
				case SUCCESS:
					/*
					 *	Free the data associated with the handle
					 */
					kmem_free (fileHandle, NCP_FHANDLE_LENGTH);
					ccode = SUCCESS;
					break;

				case E_ALL_FILES_INUSE:
					ccode = SPI_FILE_IN_USE;
					break;

				default:
					ccode = SPI_GENERAL_FAILURE;
					break;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp2XReadFile.3k)
 * NAME
 *		NCPsp2XReadFile - Netware V2.X file read operation
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp2XReadFile( channel, fileHandle, offset, buffer )
 *		ncp_channel_t		*channel;
 *		uint8		*fileHandle;
 *		uint32		offset;
 *		void_t		*buffer;
 *
 * INPUT
 *		ncp_channel_t		*channel;
 *		uint8		*fileHandle;
 *		uint32		offset;
 *
 * OUTPUT
 *		void_t	*buffer;
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_ACCESS_DENIED
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		NetWare 286 version of the file read operation.  The buffer
 *		is a NUC_IOBUF_T structure that contains bufferlength and
 *		buffer pointer which will be used by lower level IPC routines
 *		to copy the data from the IPC mechanism that communicates
 *		with the interface device to the use buffer passed down from
 *		the filesystem.
 *
 * NCP
 *	72
 *
 * NOTES
 *		This function will issue as many READ NCP requests as are 
 *		necessary to fill the callers NUC_IOBUF_T.
 *
 * SEE ALSO
 *		NCPsp2XOpenFile(3k), NCPsp2XWriteFile(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp2XReadFile_l (	ncp_channel_t	*channel,
					uint8			*fileHandle,
					uint32			offset,
					NUC_IOBUF_T		*buffer )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	function;
		uint8	reserved;
		uint8	fileHandle[NCP_FHANDLE_LENGTH];
		uint8	offset[4];
		uint8	buffSize[2];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint8	bytesRead[2];
	} *replyPacket;
#pragma pack()

	uint32			bufferSize;	
	int32			eof, bytesToRead, totalBytesRead, bytesRead;
	struct {
		NUC_IOBUF_T		tempBuffer;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NVLT_ENTER (4);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	/*
	 *	Make as many calls as are necessary to the server to fill
	 *	the callers buffer.
	 */
	eof = FALSE;
	KAlloc->tempBuffer.buffer = buffer->buffer;
	KAlloc->tempBuffer.bufferLength = buffer->bufferLength;
	KAlloc->tempBuffer.memoryType = buffer->memoryType;
	totalBytesRead = 0;
	bytesToRead = KAlloc->tempBuffer.bufferLength;

	while ((ccode == SUCCESS) && (!eof) && (bytesToRead > 0)) {

		NCPdplBuildNCPHeader_l (channel, request);

		request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

		/*
		 *	Cast the request structure into the data block of the
		 *	packet.
		 */
		requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;

		requestPacket->function = FNREAD_FILE;
		requestPacket->reserved = 0;
		bcopy (fileHandle, requestPacket->fileHandle, NCP_FHANDLE_LENGTH);
		MOVEBE_INT32 (requestPacket->offset, offset);
		bufferSize = bytesToRead;
		if (bufferSize > channel->negotiatedBufferSize)
			bufferSize = channel->negotiatedBufferSize;
		if (bufferSize > XFER_MAX)
			bufferSize = XFER_MAX;
		MOVEBE_INT16 (requestPacket->buffSize, bufferSize);

		request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE +
			sizeof (struct reqPack);
		request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
		reply->ncpDataBuffer = &KAlloc->tempBuffer;

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

		/*
		 *	Examine the return code from the request
		 */
		switch (replyPacket->completionCode) {
			case SUCCESS:
				bytesRead = GETINT16 (*(uint16 *)replyPacket->bytesRead);
				/*
				 *	At EOF the size of the data returned can be smaller
				 *	than the number of bytes requested.  This 
				 *	is our only indication of EOF.  If this happens
				 *	this is our last read.
				 */
				if (bytesRead < bufferSize)
					eof = TRUE;

				/*
				 *	Account for the data read into the NUC_IOBUF_T
				 */
				totalBytesRead += bytesRead;
				bytesToRead -= bytesRead;
				KAlloc->tempBuffer.bufferLength = bytesToRead;
				KAlloc->tempBuffer.buffer = ((char *)KAlloc->tempBuffer.buffer) + bytesRead;
				offset += bytesRead;
				ccode = SUCCESS;
				break;

			case E_NO_READ_PRIV:
				ccode = SPI_ACCESS_DENIED;
				break;

			case E_READ_LOCKED:
				ccode = SPI_LOCK_COLLISION;
				break;

			case E_ALL_FILES_INUSE:
				ccode = SPI_FILE_IN_USE;
				break;

			default:
				ccode = SPI_GENERAL_FAILURE;
				break;
		}
	}

	buffer->bufferLength = totalBytesRead;

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

done:
	kmem_free(KAlloc, sizeof(*KAlloc));
	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp2XWriteFile.3k)
 * NAME
 *		NCPsp2XWriteFile - Write buffer contents to the file
 *						   signified by fileHandle
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp2XWriteFile( channel, fileHandle, offset, buffer )
 *		ncp_channel_t		*channel;
 *		uint8		*fileHandle;
 *		uint32		offset;
 *		void_t		*buffer;
 *
 * INPUT
 *		ncp_channel_t		*channel;
 *		uint8		*fileHandle;
 *		uint32		offset;
 *		void_t		*buffer;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_ACCESS_DENIED
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		Writes the contents of the buffer signified by the buffer
 *		argument to the file signified by the fileHandle argument.
 *		The data is written to the file at the offset specfied.
 *
 * NCP
 *	73
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsp2XOpenFile(3k), NCPsp2XReadFile(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp2XWriteFile_l (	ncp_channel_t	*channel,
						uint8			*fileHandle,
						uint32			offset,
						NUC_IOBUF_T		*buffer )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	function;
		uint8	reserved;
		uint8	fileHandle[NCP_FHANDLE_LENGTH];
		uint8	offset[4];
		uint8	buffSize[2];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
	} *replyPacket;
#pragma pack()

	uint32			bufferSize;	
	int32			bytesToWrite;
	struct {
		NUC_IOBUF_T		tempBuffer;
	} *KAlloc = kmem_alloc(sizeof(*KAlloc), KM_SLEEP);

	NVLT_ENTER (4);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto done;
	}

	/*
	 *	Make as many calls as are necessary to the server to write
	 *	all data in the callers buffer.
	 */
	KAlloc->tempBuffer.buffer = buffer->buffer;
	KAlloc->tempBuffer.memoryType = buffer->memoryType;
	bytesToWrite = buffer->bufferLength;

	/*
	 *	Write all data bytes presented by the user.  Thi is a do-while
	 *	loop because NCPsp2XTruncateFile could call this function 
	 *	with zero bytes to write at a specific offset so I always
	 *	issue at least one NCP.
	 */
	do {

		NCPdplBuildNCPHeader_l (channel, request);

		request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

		requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;

		requestPacket->function = FNWRITE_FILE;
		requestPacket->reserved = 0;
		bcopy (fileHandle, requestPacket->fileHandle, NCP_FHANDLE_LENGTH);
		MOVEBE_INT32 (requestPacket->offset, offset);
		bufferSize = bytesToWrite;
		if (bufferSize > channel->negotiatedBufferSize)
			bufferSize = channel->negotiatedBufferSize;
		if (bufferSize > XFER_MAX)
			bufferSize = XFER_MAX;
		MOVEBE_INT16 (requestPacket->buffSize, bufferSize);

		KAlloc->tempBuffer.bufferLength = bufferSize;

		request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE +
			sizeof(struct reqPack);
		request->ncpDataBuffer = &KAlloc->tempBuffer;

		reply->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
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
				KAlloc->tempBuffer.buffer = ((char *)KAlloc->tempBuffer.buffer) + bufferSize;
				bytesToWrite -= bufferSize;
				offset += bufferSize;
				ccode = SUCCESS;
				break;

			case E_NO_WRITE_PRIV:
				ccode = SPI_ACCESS_DENIED;
				break;

			case E_ALL_FILES_INUSE:
				ccode = SPI_FILE_IN_USE;
				break;

			case E_BAD_FILE_HANDLE:
				ccode = SPI_BAD_HANDLE;
				break;

			case E_READ_LOCKED:
				ccode = SPI_LOCK_COLLISION;
				break;

			case E_OUT_OF_DISK_SPACE:
				ccode = SPI_OUT_OF_DISK_SPACE;
				break;

			default:
				ccode = SPI_GENERAL_FAILURE;
				break;
		}
	} while ((ccode == SUCCESS) && (bytesToWrite > 0)); 

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */
done:
	kmem_free(KAlloc, sizeof(*KAlloc));

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp2XLockFile.3k)
 * NAME
 *		NCPsp2XLockFile - Lock a file byte range
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp2XLockFile( channel, fileHandle, lockStruct )
 *		ncp_channel_t				*channel;		communication channel
 *		NCP_DIRHANDLE_T	*fileHandle;	handle to an opended file
 *		NWSI_LOCK_T			*lockStruct;	lock type, offset and length
 *
 * INPUT
 *		ncp_channel_t				*channel;		
 *		NCP_DIRHANDLE_T	*fileHandle;
 *		NWSI_LOCK_T			*lockStruct;
 *
 * OUTPUT
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_BAD_HANDLE
 *		SPI_LOCK_COLLISION
 *		SPI_LOCK_TIMEOUT
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		This function locks a range of bytes of a previously opened file.  The
 *		The range is locked exclusively if the mode is MODE_WRITE and 
 *		MODE_READ otherwise.  The NetWare lock timeout value is set to zero
 *		so that UNIX can decide on retry timing and amount.
 *
 * NCP
 *	26
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp2XLockFile_l (	ncp_channel_t	*channel,
					uint8			*fileHandle,
					NWSI_LOCK_T		*lockStruct )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	LockFlag;
		uint8	FileHandle[6];
		uint8	LockAreaStartOffset[4];
		uint8	LockAreaLength[4];
		uint8	LockTimeout[2];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
	} *replyPacket;
#pragma pack()

	uint32	ticks;

	NVLT_ENTER (3);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	/*
	 *	Assemble the data items in the NCP packet
	 */
	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;

	requestPacket->FunctionCode = FNLOG_PHYS_RECORD;
	if (lockStruct->mode == READ_LOCK)
		requestPacket->LockFlag = LOCK_NONEXCLUSIVE;
	else
		requestPacket->LockFlag = LOCK_EXCLUSIVE;
	bcopy (fileHandle, requestPacket->FileHandle, NCP_FHANDLE_LENGTH);
	MOVEBE_INT32 (requestPacket->LockAreaStartOffset, lockStruct->offset);
	MOVEBE_INT32 (requestPacket->LockAreaLength, lockStruct->size);
	/*
	 * Convert UNIX ticks to NetWare ticks.
	 *
	 *	Seconds = UNIX ticks / 100  
	 *  DOS ticks = Seconds / 18
	 */
	ticks = lockStruct->timeout / 1800;
	MOVEBE_INT16 (requestPacket->LockTimeout, ticks);

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
			/*
			 *	Examine the completion code and set the appropriate SPIL return
			 *	code.
			 */
			switch (replyPacket->completionCode) {
				case SUCCESS:
					ccode = SUCCESS;
					break;

				case E_BAD_FILE_HANDLE:
					ccode = SPI_BAD_HANDLE;
					break;

				case E_LOCK_COLLISION:
					ccode = SPI_LOCK_COLLISION;
					break;

				case E_ALL_FILES_INUSE:
					ccode = SPI_FILE_IN_USE;
					break;

				case E_TIMEOUT:
					ccode = SPI_LOCK_TIMEOUT;
					break;

				default:
					ccode = SPI_GENERAL_FAILURE;
					break;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(NCPsp2XUnlockFile.3k)
 * NAME
 *		NCPsp2XUnlockFile - Unlock a file byte range
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp2XUnlockFile( channel, fileHandle, lockStruct )
 *		ncp_channel_t				*channel;		communication channel
 *		NCP_DIRHANDLE_T	*fileHandle;	handle to an opended file
 *		NWSI_LOCK_T			*lockStruct;	lock type, offset and length
 *
 * INPUT
 *		ncp_channel_t				*channel;		
 *		NCP_DIRHANDLE_T	*fileHandle;
 *		NWSI_LOCK_T			*lockStruct;
 *
 * OUTPUT
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_BAD_HANDLE
 *		SPI_LOCK_COLLISION
 *		SPI_LOCK_TIMEOUT
 *		SPI_GENERAL_FAILURE
 *
 * DESCRIPTION
 *		This function unlocks a range of bytes of a previously opened file.
 *
 * NCP
 *	30
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp2XUnlockFile_l (	ncp_channel_t	*channel,
						uint8			*fileHandle,
						NWSI_LOCK_T		*lockStruct )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	Reserved;
		uint8	FileHandle[NCP_FHANDLE_LENGTH];
		uint8	LockAreaStartOffset[4];
		uint8	LockAreaLength[4];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
	} *replyPacket;
#pragma pack()

	NVLT_ENTER (3);

	if (NCPdplGetFreePacket_l (channel, &request)) {
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	if (NCPdplGetFreePacket_l (channel, &reply)) {
		NCPdplFreePacket (request);
		return (NVLT_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}

	NCPdplBuildNCPHeader_l (channel, request);

	request->ncpU.ncpHdr.type = FILE_SERVICE_REQUEST;

	/*
	 *	Assemble the data items in the NCP packet
	 */
	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;

	requestPacket->FunctionCode = FNCLEAR_PHYSICAL_RECORD;
	requestPacket->Reserved = 0;
	bcopy (fileHandle, requestPacket->FileHandle, NCP_FHANDLE_LENGTH);
	MOVEBE_INT32 (requestPacket->LockAreaStartOffset, lockStruct->offset);
	MOVEBE_INT32 (requestPacket->LockAreaLength, lockStruct->size);

	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqPack);
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize =	NCP_HEADER_CONST_SIZE + sizeof(struct repPack);
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
			 *	Examine the completion code and set the appropriate SPIL return
			 *	code.
			 */
			switch (replyPacket->completionCode) {
				case SUCCESS:
					ccode = SUCCESS;
					break;

				case E_ALL_FILES_INUSE:
					ccode = SPI_FILE_IN_USE;
					break;

				default:
					ccode = SPI_ACCESS_DENIED;
					break;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsp2XGetCurrentSizeOfFile.3k)
 * NAME
 *		NCPsp2XGetCurrentSizeOfFile - get size of an open file
 *			
 * SYNOPSIS
 *		ccode_t
 *		NCPsp2XGetCurrentSizeOfFile( channel, fileHandle, fileSize )
 *		void_t	*channel;
 *		uint8	*fileHandle;
 *		uint32	*fileSize;
 *
 * INPUT
 *		void_t	*channel;
 *		uint8	*fileHandle;
 *
 * OUTPUT
 *		uint32	*fileSize;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_FAILURE
 *
 * DESCRIPTION
 *		This function issues NCP 71 to obtain the true size of an open file.
 *
 * NCP
 *	71
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp2XGetCurrentSizeOfFile_l (	ncp_channel_t	*channel,
								uint8			*fileHandle,
								uint32			*fileSize )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	Function;
		uint8	Reserved;
		uint8	FileHandle[NCP_FHANDLE_LENGTH];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint8	FileSize[4];
	} *replyPacket;
#pragma pack()

	NVLT_ENTER (3);

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
	requestPacket->Function = FNGET_FILE_SIZE;
	bcopy (fileHandle, requestPacket->FileHandle, NCP_FHANDLE_LENGTH);

	/*
	 *	Determine the size of the packet header and data
	 */
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
					*fileSize = GETINT32 (* ( uint32 *)replyPacket->FileSize);
					break;

				case E_ALL_FILES_INUSE:
					ccode = SPI_FILE_IN_USE;
					break;

				default:
					ccode = SPI_BAD_HANDLE;
					break;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}
