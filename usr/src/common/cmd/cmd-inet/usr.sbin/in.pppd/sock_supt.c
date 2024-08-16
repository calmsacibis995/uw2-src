/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static char TCPID[] = "@(#)sock_supt.c	1.2 STREAMWare for Unixware 2.0 source";
#endif /* lint */
/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/sock_supt.c	1.3"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

#include "pppd.h"

#define LOCALHOST	"localhost"
#define PPP_SERV	"pppmsg"

int
pppd_sockinit(maxpend)
	int	maxpend;
{
	int	s;
	struct sockaddr_in sin_addr;
	struct hostent *hp;
	struct servent *serv;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                syslog(LOG_ERR,"socket create failed: %m");
		perror("socket AF_INET SOCK_STREAM");
		exit(1);
	}

	sin_addr.sin_family = AF_INET;
	sin_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	
	if (!(serv = getservbyname(PPP_SERV, "tcp"))) {
		syslog(LOG_INFO, "unknown service %s", PPP_SERV);
		fprintf(stderr, "unknown service %s\n", PPP_SERV);
		exit(1);
	}

	sin_addr.sin_port = serv->s_port;

	if (bind(s, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
                syslog(LOG_ERR,"bind failed: %m");
		perror("bind");
		exit(1);
	}
	syslog(LOG_INFO, "bound to host '%s' port %d.",
					LOCALHOST, ntohs(sin_addr.sin_port));

	listen(s, maxpend);
	return s;
}

int
ppp_sockinit()
{
	int	s;
	struct sockaddr_in sin_addr;
	struct hostent *hp, *gethostbyname();
	struct servent *serv;
	int port;

	port = IPPORT_RESERVED - 1;
	s = rresvport(&port);
	if (s < 0) {
		syslog(LOG_ERR, "socket call failed: %m");
		exit(1);
	}

	sin_addr.sin_family = AF_INET;
        sin_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (!(serv = getservbyname(PPP_SERV, "tcp"))) {
		syslog(LOG_INFO, "unknown service %s", PPP_SERV);
		exit(1);
	}
	sin_addr.sin_port = serv->s_port;

	if (connect(s, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
		syslog(LOG_INFO, "unable to connect to host %x port %d (%m)",
				ntohl(sin_addr.sin_addr.s_addr),
				ntohs(sin_addr.sin_port));
		exit(1);
	}
	return s;
}
