/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwcloseconns:nwcloseconns.c	1.3"
#include <stdio.h>
#include <string.h>
#include <netdir.h>
#include <sys/types.h>
#include <sys/tiuser.h>
#include <nw/nwclient.h>
#include "nwapidef.h"
#include <sys/nwportable.h>
#include <sys/nwmp.h>
#include "nucrequester.h"

#define DEBUG

/*
 *  Open all public (shared) connections in the kernel's connection table
 *  and obliterate each connection.  This program runs with privilege
 *  and wild women.
 *
 *  Private connections disappear when the process goes away.
 */
main()
{
	struct	scanTaskReq	request;
	struct	netbuf		serverAddress;
	char				buffer[NWMAX_INTERNET_ADDRESS_LENGTH];
	NWCTranAddr			tranAddr;
	nuint32				connHandle;
	nint				fd, r, j1, j2, j, k;
	uid_t				saved_euid;


	if ((fd = NWMPOpen()) < 0) {
		exit(1);
	}

	serverAddress.len = 0;
	serverAddress.maxlen = MAX_ADDRESS_SIZE;
	serverAddress.buf = buffer;

	saved_euid = geteuid();

	while (NWMPScanServices(fd, &serverAddress, &j1, &j2) == SUCCESS) {
		/* get a count to use for later */
		j = 0;
		request.address = serverAddress;
		request.userID = -1;
		while (ioctl(fd, NWMP_SCAN_TASK, &request) == SUCCESS) {
			j++;
		}

		for( k = 0; k < j; k++ ) {
			request.userID = -1;
			ioctl(fd, NWMP_SCAN_TASK, &request);
			tranAddr.uType = NWC_TRAN_TYPE_IPX;
			tranAddr.uLen = serverAddress.len;
			tranAddr.pbuBuffer = (uint8 *) serverAddress.buf;
			if( !seteuid( request.userID ) ) {
				open_conn_by_addr(&tranAddr, &connHandle, NWC_OPEN_PUBLIC);
				sys_close_conn(connHandle);
				seteuid(saved_euid);
			} else {
				perror("seteuid");
			}
		}
	}
	exit(0);
}
