/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:nwlogin.c	1.14"
/******************************************************************************
 ******************************************************************************
 *
 *	NWLOGIN.C
 *
 *	NetWare Login Handler (part 1 of 2)
 *
 ******************************************************************************
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/tiuser.h"
#include <sys/nwctypes.h>
#include <nw/ntypes.h>
#include <netdb.h>
#include <netdir.h>
#include "sys/nwmp.h"
#include <pfmt.h>
#include <locale.h>

#ifndef	FAILURE
#define	FAILURE	-1
#endif
#ifndef	SUCCESS
#define	SUCCESS	0
#endif

int 			NWMPfileDesc = -1;
int				retry_count = 0;	/* retry count */

extern	int		nuc_debug;			/* NUC debugging */
extern	int 	errno;
extern	FILE	*log_fd;

int 	getServiceAndCredentials( struct reqCreateTaskReq *request);

/******************************************************************************
 *
 *	nwlogin ( int arg )
 *
 *	Handle a login request from the kernel
 *
 *	Entry :
 *		arg		not used
 *
 *	Exit :
 *		1		something went wrong
 *
 *****************************************************************************/

int
nwlogin ( int arg )
{
	int 						ccode, ret;
	short 						connId;
	char						serverName [64];
	int							myUid, pid;
	struct	reqCreateTaskReq	request;
	char						abuf[12];
	struct	netconfig			*np;
	struct	nd_hostservlist		*hlist;
		
	myUid = geteuid();

	while (TRUE) {

		seteuid(myUid);
		request.address.buf = abuf;
		request.address.maxlen = 12;

		if (nuc_debug) {
			(void)pfmt (log_fd, MM_NOSTD,
				":106:nwlogin: awaiting a request\n");
		}

		if (getServiceAndCredentials(&request)) {
			/*
			 *	something went wrong sleeping in the kernel
			 */
			retry_count ++;
			sleep ( 5 );
			continue;
		}

		/*
		 *	have a valid request -- lookup address
		 */

		if ((np = getnetconfigent("ipx")) == NULL ) {
			/*
			 *	no IPX info available
			 */
			(void)pfmt (log_fd, MM_ERROR,
				":107:nwlogin: unable to locate IPX config entry\n");
			continue;
		}

		if ( (ret = netdir_getbyaddr ( np, &hlist, &request.address ) ) ) {
			/*
			 *	address not found
			 */
			(void)pfmt (log_fd, MM_NOSTD,
				":108:nwlogin: no name found 1 - ret = %d\n", ret);
			freenetconfigent ( np );
			continue;
		}

		if ( hlist -> h_cnt < 1 ) {
			/*
			 *	no matching name
			 */
			(void)pfmt (log_fd, MM_NOSTD,
				":115:nwlogin: no name found 2\n");
			netdir_free ((char *)hlist, ND_HOSTSERVLIST );
			freenetconfigent ( np );
			continue;
		}

		strcpy ( serverName, hlist->h_hostservs->h_host );
		netdir_free ((char *)hlist, ND_HOSTSERVLIST );
		freenetconfigent ( np );

		if ( nuc_debug ) 
			(void)pfmt (log_fd, MM_NOSTD,
				":109:nwlogin: authenticating to server '%s' [PID=%d]\n",
				serverName,request.pid );

		/*
		 *	have a valid name in serviceName -- authenticate him
		 */

		if (ccode = seteuid(request.uid)) {
			(void)pfmt(log_fd, MM_ERROR,
				":110:nwlogin: cannot setuid to uid %d\n", request.uid);
			continue;
		}


		if ( nwauthen ( serverName, request.uid, myUid, request.pid,
				&connId, request.xautoFlags ) == FAILURE ) {
			/*
		 	 *	something went wrong
		 	 */
			(void)pfmt (log_fd, MM_NOSTD,
					":111:nwlogin: something failed in authentication\n");
		}
	}
}
	
/******************************************************************************
 *
 *	getServiceAndCredentials ( struct reqCreateTaskReq *request )
 *
 *	Hang out in the kernel and wait for a task
 *
 *	Entry :
 *		*request	place to stash request
 *
 *	Exit :
 *		0			cool
 *		x			something went wrong
 *
 *****************************************************************************/

int
getServiceAndCredentials ( struct reqCreateTaskReq *request ) 
{
	int 	ccode;

	/*
	 *	make sure the management portal is open
	 */

	if (NWMPfileDesc == -1) {
		NWMPfileDesc = NWMPOpen();
		if (NWMPfileDesc < 0) {
			/*
			 *	bummer -- couldn't open it
		 	 */

			(void)pfmt (log_fd, MM_ERROR,
				":112:nwlogin: Unable to open NWMP\n");
			return ( 1 );
		}
	}

	/*
	 *	get a create task request from the NUC
	 */

	ccode = ioctl(NWMPfileDesc, NWMP_CREATE_TASK_REQ, request);

	if ( nuc_debug) 
		(void)pfmt (log_fd, MM_NOSTD,
			":113:nwlogin: ioctl ccode return = %d\n", ccode );

	if (ccode) {
		/*
		 *	something went wrong with create task
		 */

		(void)pfmt(log_fd, MM_ERROR,
			":114:nwlogin: error from IOCTL was %d\n", errno);
	}

	return(ccode);
}
