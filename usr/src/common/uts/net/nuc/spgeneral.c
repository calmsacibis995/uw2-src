/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spgeneral.c	1.16"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spgeneral.c,v 2.52.2.4 1995/01/16 22:53:02 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncpspgen.c
 *	ABSTRACT: General non-version specific NetWare Core protocol requests
 *
 *	Functions declared in this module:
 *	Public functions:
 *		NCPspIsUNIXNameSpaceSupported
 *		NCPsp2XGetVolumeNumber
 *		NCPspConvertPathToComponents
 *
 */ 

#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <util/cmn_err.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nwspiswitch.h>
#include <net/nuc/nwspi.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

#define NVLT_ModMask	NVLTM_ncp

#ifdef NUC_DEBUG
uint32 ncpDebug = TRUE;
#endif

/*
 * BEGIN_MANUAL_ENTRY(NCPspIsUnixNameSpaceSupported.3k)
 * NAME
 *		NCPspIsUnixNameSpaceSupported()
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPspIsUNIXNameSpaceSupported( channel, volumeHandle, nucNlmMode )
 *		void_t			*channel;
 *		ncp_volume_t	*volumeHandle;
 *		int32			*nucNlmMode;
 *
 * INPUT
 *		void_t			*channel;
 *		ncp_volume_t	*volumeHandle;
 *
 * OUTPUT
 *		int32			*nucNlmMode;
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		Queries the target file server to determine whether the volume
 *		in question supports the UNIX name space and has running NUC.NLM.
 *
 *		Returns supportFlag=FALSE if server cannot support the UNIX namespace
 *		or if the UNIX name space has not been added to the volume.
 *
 * NCP
 *	95 16
 *
 * NOTES
 *		The NCP issued by this function is only applicable to server versions 
 *		V3.11 and higher and the caller should verify this before calling 
 *		this function.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspIsUNIXNameSpaceSupported_l (	ncp_channel_t	*channel,
									ncp_volume_t	*volumeHandle,
									uint8			*nucNlmMode )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;

#pragma pack(1)
	struct reqPack {
		uint8	FunctionCode;
		uint8	SubFunction;
	} *requestPacket;

	struct reqPack2 {
		uint8	FunctionCode;
		uint8	SubFunction;
		uint8	reserved;
		uint16	ncpVersion;
	} *requestPacket2;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint8	Reserved[64];
		uint8	volumeSupportTable[256];
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

	requestPacket2 = (struct reqPack2 *)request->ncpU.ncpHdr.data;
	
	requestPacket2->FunctionCode = FNNETWARE_UNIX_CLIENT_FUNCTION;
	requestPacket2->SubFunction = SFUNCIsUNIXNameSpaceSupported;
	requestPacket2->reserved = 0;
	requestPacket2->ncpVersion = 0;

	request->ncpHeaderSize = NCP_HEADER_CONST_SIZE + sizeof (struct reqPack2);
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
	
			if (replyPacket->completionCode == SUCCESS) {
				if (replyPacket->volumeSupportTable[volumeHandle->number] &
						NFS_MODE){
					*nucNlmMode = NUC_NLM_UNIX_MODE;
					ccode = SUCCESS;
				} else if (replyPacket->volumeSupportTable[volumeHandle->number]
							!= 0 ) {
					*nucNlmMode = NUC_NLM_NETWARE_MODE;
					ccode = SUCCESS;
				}
			} else {
				*nucNlmMode = NUC_NLM_NONE;
				ccode = SPI_BAD_ARGS_TO_SERVER;
			}
		}
	}

	NCPdplFreePacket (request);		/* done with it, give it back to pool */
	NCPdplFreePacket (reply);		/* done with it, give it back to pool */

	return (NVLT_LEAVE (ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(NCPsp2XGetVolumeNumber.3k)
 * NAME
 *		NCPsp2XGetVolumeNumber - Get volume number on a NetWare
 *							   V2.1X server given volume name.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsp2XGetVolumeInfo( channel, volumeName, volumeNumber )
 *		void_t	*channel;
 *		char	*volumeName;
 *		int32	*volumeNumber;
 *
 * INPUT
 *		void_t	*channel;
 *		char	*volumeName;
 *
 * OUTPUT
 *		int32	*volumeNumber
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_SERVER_UNAVAILABLE
 *		SPI_ACCESS_DENIED
 *
 * DESCRIPTION
 *		Query a NetWare V2.X server for the number of for the
 *		specified volume name.
 *
 * NCP
 *	22 05
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsp2XGetVolumeNumber_l (	ncp_channel_t	*channel,
							char			*volumeName,
							int32			*volumeNumber )
{
	ccode_t		ccode = SUCCESS;
	iopacket_t*	request;
	iopacket_t*	reply;
	uint16		tmp;

#pragma pack(1)
	struct reqPack {
		uint8	function;
		uint8	subFunctionLength[2];
		uint8	subFunctionCode;
		uint8	volNameLength;
		char	volName[1];
	} *requestPacket;

	struct repPack {
		uint8	completionCode;
		uint8	connectionStatus;
		uint8	volumeNumber;
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
	 *	Cast the request structure into the data block of the
	 *	packet.
	 */
	requestPacket = (struct reqPack *)request->ncpU.ncpHdr.data;

	requestPacket->function = FNDIRECTORY_SERVICES;
	requestPacket->volNameLength = (uint8)strlen (volumeName);
	tmp = 4 + requestPacket->volNameLength -1;
	*(uint16 *)requestPacket->subFunctionLength = GETINT16 (tmp);
	requestPacket->subFunctionCode = SFGET_VOLUME_NUMBER;
	bcopy (volumeName, requestPacket->volName, requestPacket->volNameLength);

	request->ncpHeaderSize =  NCP_HEADER_CONST_SIZE + sizeof(struct reqPack) +
								requestPacket->volNameLength; 
	request->ncpDataBuffer = (NUC_IOBUF_T *)NULL;

	reply->ncpHeaderSize =	NCP_HEADER_CONST_SIZE + sizeof (struct repPack);
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
					*volumeNumber = (int32)replyPacket->volumeNumber;
					ccode = SUCCESS;
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
 * BEGIN_MANUAL_ENTRY(NCPspConvertPathToComponents.3k)
 * NAME
 *		NCPspConvertPathToComponents - Convert a path name into components of
 *			pascal-type strings, each preceeded by their length.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPspConvertPathToComponents( path, componentPath, 
 *		componentPathLengthMax, componentCount, componentPathLength )
 *		char	*path;						null-terminated input path
 *		char	*componentPath;				output component path
 *		int32	componentPathLengthMax;		max size of componentPath
 *		int32	*componentCount;			how many components found
 *		int32	*componentPathLength;		length of componentPath
 *
 * INPUT
 *		char	*path;
 *		int32	componentPathLengthMax;
 *
 * OUTPUT
 *		char	*componentPath;
 *		int32	*componentCount;
 *		int32	*componentPathLength;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_FAILURE 						if componentPath is too small 
 *
 * DESCRIPTION
 *		This function converts a UNIX-style path name into a component path
 *		format used by NetWare 3.x NCPs.  Each component of the path is 
 *		converted into a length:string pair and the count of components is
 *		returned along with the actual length of the output component path.  
 *		A general error is returned if the path isn't a valid UNIX
 *		path or the receiving field is too small.
 *
 * NOTES
 *		The input path is assumed to be a null-terminated ascii path name in
 *		UNIX semantics.  Each output component is preceeded by a one-byte
 *		length and contains the component without a null-terminator.  The
 *		resultant output is not null-terminated; the length is returned in
 *		componentPathLength.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPspConvertPathToComponents (	char	*path,
								char	*componentPath,
								int32	componentPathLengthMax,
								uint32	*componentCount,
								uint32	*componentPathLength,
								int32	nodeType )

{
	char	*p, *cP, *cPend, *lengthPointer;
	
	/*
	 *	Initialize our pointers and counters and skip over any leading 
	 *	component separators
	 */
	p = path;
	while (( *p == '/' ))
		p++;
	cP = componentPath;
	*componentCount = 0;
	*componentPathLength = 0;

	/* 
	 * Do we have to worry about "./" or "../" ?
	 */
	if ((strcmp (p, ".") == SUCCESS) || (strcmp (p, "..") == SUCCESS)) {
		/*
		 * We are adjusting so the size put into the request packet
		 * is accurate.
		 */
		*componentPathLength = 1;
		return (SUCCESS);
	}

	/*
	 *	Calculate the end of the componentPath string to be sure we dont
	 *	exceed its length
	 */
	cPend = cP + componentPathLengthMax - 1;

	/*
	 *	For each component found, place it in componentPath and preceed it
	 *	with its length in a one-byte field.
	 */
	while (*p) {
		/*
		 *	Get a pointer to the length field for this component and set
		 *	that length to zero
		 */
		lengthPointer = cP++;
		(*componentPathLength)++;
		*lengthPointer = 0;

		/*
		 *	While a component separator has not been found, copy the 
		 *	component to componentPath and keep track of the component
		 *	length and the total output length.
		 *
		 *	If we run out of room in componentPath, return an error.
		 */
		while ((*p != '/') && (*p != '\0')) {
			if (cP > cPend)
				return (FAILURE);
			*cP++ = *p++;
			(*lengthPointer)++;
			(*componentPathLength)++;
		}

		/*
		 *	If this is the first component and it ends with a ':' and
		 *	this is an entry of type NS_ROOT then remove the ':'
		 */
		if ((*componentCount == 0) && (*(cP -1) == ':') &&
				(nodeType == NS_ROOT)) {
			(*lengthPointer)--;
			(*componentPathLength)--;
			if (*p == '\0') {
				/*
				 * Only dealing with the root node.  No need to pass any
				 * name. The componentPathLenght is set to 1 because all
				 * of the routines calling this function subtract 1 from it
				 * before sending the packet.  That way we get 0 for the
				 * componentPathLength.
				 */
				*componentPathLength = 1;
				break;
			}
			cP--;
		}

		/*
		 *	The component is now complete, count it.  If the
		 *	terminator was a '/' advance the input pointer over the 
		 *	component terminator and continue looking for the next one.
		 */
		(*componentCount)++;
		while (*p == '/')
			p++;
	}

	return (SUCCESS);
}
