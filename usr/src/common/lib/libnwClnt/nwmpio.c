/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:nwmpio.c	1.4"
# /* ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/nwmpio.c,v 1.11 1994/09/26 17:18:49 rebekah Exp $" */

static char SccsID[] = "nwmpio.c	1.3 92/02/1416:38:49 92/02/1416:39:12";
/*
(C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

No part of this file may be duplicated, revised, translated, localized or modified
in any manner or compiled, linked or uploaded or downloaded to or from any
computer system without the prior written consent of Novell, Inc.
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/nwctypes.h>
#include <nw/ntypes.h>
#include <sys/tiuser.h>
#include <sys/nwmp.h>
#include <sys/nwmpccode.h>
#include <nwmptune.h>
#include <sys/nucerror.h>

int
NWMPOpen()
{
	return(open("/dev/nuc00", O_RDWR));
}

int
NWMPClose( int fp )
{
	return(close(fp));
}

int
NWMPRawPacket(
	int	fp,
	char	*protoHeader,
	int	sendHeaderLen,
	int	*recvHeaderLen,
	char	*protoData,
	int	sendDataLen,
	int	*recvDataLen
)
{
	int ccode;
	struct rawReq raw;

	raw.header = protoHeader;
	raw.sendHdrLen = sendHeaderLen;
	raw.recvHdrLen = recvHeaderLen ? *recvHeaderLen : 0;
	raw.data = protoData;
	raw.sendDataLen = sendDataLen;
	raw.recvDataLen = recvDataLen ? *recvDataLen : 0;

	if (ccode = ioctl(fp, NWMP_RAW, &raw))
	{
#ifdef REQ_DEBUG
		printf("NWMPLib: RAW ioctl failed with errno=%d \n",ccode);
#endif
	    /* map nwmp errors into "requester" errors */
       	ccode = map_nwmp_errors( ccode );
	}
	if (recvHeaderLen)
	{
		*recvHeaderLen = raw.recvHdrLen;
	}
	if (recvDataLen)
	{
		*recvDataLen = raw.recvDataLen;
	}

	return(ccode);
}

int
NWMPRegisterRaw(
	int fp,
	struct netbuf *serviceAddress,
	nuint32 flags
)
{
	struct regRawReq rawReq;
	int	ccode;
#ifdef REQ_DEBUG
	extern int 	errno;
#endif

	rawReq.address.maxlen = serviceAddress->maxlen;
	rawReq.address.len = serviceAddress->len;
	rawReq.address.buf = serviceAddress->buf;
	rawReq.flags = flags;

	if (ccode = ioctl(fp, NWMP_REGISTER_RAW, &rawReq))
	{
	    /* map nwmp errors into "requester" errors */
       	ccode = map_nwmp_errors( ccode );
#ifdef REQ_DEBUG
		printf("NWMPLib: Register raw failed with ccode=%d and errno=%d\n",ccode, errno);
#endif
	}

	return(ccode);
}

int
NWMPSetPrimaryService( int fd )
{
	int ccode;
#ifdef REQ_DEBUG
	extern int errno;
#endif
	
	if (ccode = ioctl(fd, NWMP_SET_PRIMARY_SERVICE, 0))
	{
		/* map nwmp errors into "requester" errors */
		ccode = map_nwmp_errors(ccode);
#ifdef REQ_DEBUG
		printf ( "NWMPLib: Set Primary Service failed with ccode=%d and errno=%d\n",
			ccode, errno);
#endif
	}
	return(ccode);
}

int
NWMPGetPrimaryService( int fd, nuint32 *reference )
{
	int ccode;

	if (ccode = ioctl(fd, NWMP_GET_PRIMARY_SERVICE, reference))
	{
		/* map nwmp errors into "requester" errors */
		ccode = map_nwmp_errors(ccode);
	}
	return(ccode);
}
