/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:nwmpserv.c	1.5"
# /* ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/nwmpserv.c,v 1.10 1994/09/26 17:18:50 rebekah Exp $" */

static char SccsID[] = "nwmpserv.c	1.3 92/02/1416:38:50 92/02/1416:39:12";
/*
(C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

No part of this file may be duplicated, revised, translated, localized or modified
in any manner or compiled, linked or uploaded or downloaded to or from any
computer system without the prior written consent of Novell, Inc.
*/

#include <stdio.h>
#include <sys/nwctypes.h>
#include <nw/ntypes.h>
#include <sys/tiuser.h>
#include <sys/nwmp.h>
#include <errno.h>
#include <sys/nucerror.h>


NWMPInitSPI( int fp )
{
int32	junk;
int	ccode;

	ccode = ioctl(fp,NWMP_SPI_INIT,&junk);
#ifdef REQ_DEBUG
	if( ccode )
	{
		fprintf(stderr,"IOCTL Failure from NWMPInitSPI = %x\n",ccode);
	}
#endif

	return(ccode);
}

NWMPDownSPI( int fp )
{
int32	junk;
int	ccode;

	ccode = ioctl(fp,NWMP_SPI_DOWN,&junk);
#ifdef REQ_DEBUG
	if( ccode )
	{
		fprintf(stderr,"IOCTL Failure from NWMPDownSPI = %x\n",ccode);
	}
#endif

	return(ccode);
}

NWMPInitIPC( int fp )
{
int32	junk;
int	ccode;

	ccode = ioctl(fp,NWMP_IPC_INIT,&junk);
#ifdef REQ_DEBUG
	if( ccode )
	{
		fprintf(stderr,"IOCTL Failure from NWMPInitGIPC = %x\n",ccode);
	}
#endif

	return(ccode);
}

NWMPDownIPC( int fp )
{
int32	junk;
int	ccode;
	
	ccode = ioctl(fp,NWMP_IPC_DOWN,&junk);
#ifdef REQ_DEBUG
	if( ccode )
	{
		fprintf(stderr,"IOCTL Failure from NWMPDownIPC = %x\n",ccode);
	}
#endif

	return(ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWMPScanServices.3l)
 * NAME
 *		NWMPScanServices - Scan the SPI Service data structure
 *
 * DESCRIPTION
 *		Queries the SPIL driver to return information iteratively about
 *		the services that are currently available in SPIL.
 *
 */
NWMPScanServices(
	int 		fp,
	struct netbuf	*serviceAddress,
	uint32		*serviceProtocol,
	uint32		*transportProtocol
)
{
	struct scanServiceReq req;
	int ccode;

	req.address.maxlen = serviceAddress->maxlen;
	req.address.len = serviceAddress->len;
	req.address.buf = serviceAddress->buf;
	req.serviceFlags = 0;

	if (ccode = ioctl( fp, NWMP_SCAN_SERVICE, &req ))
	{
#ifdef REQ_DEBUG
		if (ccode != SPI_NO_MORE_SERVICE) {
			fprintf(stderr,"NWMPLib: ScanService failed with ccode=%X \n",
				ccode);
		}
#endif
        /* map nwmp errors into "requester" errors */
        ccode = map_nwmp_errors( ccode );
		return(ccode);
	}

	serviceAddress->len = req.address.len;
	*serviceProtocol = req.serviceProtocol;
	*transportProtocol = req.transportProtocol;

	return(ccode);
}

NWMPGetServerContext(
	int	fp,
	nuint	*majorVersion,
	nuint	*minorVersion
)
{
	struct getServerContextReq req;
	int ccode;

	if ((ccode = ioctl( fp, NWMP_GET_SERVER_CONTEXT, &req )) == SUCCESS)
	{
		*majorVersion = req.majorVersion;
		*minorVersion = req.minorVersion;
	} else {
        /* map nwmp errors into "requester" errors */
        ccode = map_nwmp_errors( ccode );
	}

	return(ccode);
}
