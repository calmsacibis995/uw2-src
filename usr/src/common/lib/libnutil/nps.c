/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nps.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nps.c,v 1.9 1994/06/20 15:59:57 mark Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stropts.h>
#include <fcntl.h>
#include <signal.h>
#include "nwmsg.h"
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/nwportable.h>
#include <sys/nwtdr.h>
#include <sys/ipx_app.h>
#include "nwconfig.h"

#include "util_proto.h"
#include "nps.h"
#include "npsmsgtable.h"

int
SetSocket
	(int 	fdNetwork,
	uint16 	*socketNumber,
	int 	timeOut)
{
	struct strioctl ioc;

	/* build an ioctl to set the socket */
	ioc.ic_dp = (char *)socketNumber;
	ioc.ic_len = sizeof(uint16);
	ioc.ic_cmd = IPX_SET_SOCKET;
	ioc.ic_timout = timeOut;  
	if (ioctl(fdNetwork, I_STR, &ioc) == -1) {
#ifdef DEBUG
		fprintf(stderr, "\nIoctl IPX_SET_SOCKET failed setting value %d\n",
					(uint32)socketNumber );
#endif
		return(FAILURE);
	}
	*socketNumber = GETINT16(*socketNumber);
	return(SUCCESS);
}

int
BindSocket
	(int 	fdNetwork,
	uint16 	socketNumber,
	int 	timeOut)
{
	struct strioctl ioc;   

	if( socketNumber == 0)
		return(FAILURE);
	/* build an ioctl to set the socket */
	ioc.ic_dp = (char *)&socketNumber;
	ioc.ic_len = sizeof(socketNumber);
	ioc.ic_cmd = IPX_BIND_SOCKET;
	ioc.ic_timout = timeOut;  
	if (ioctl(fdNetwork, I_STR, &ioc) == -1) {
#ifdef DEBUG
		fprintf(stderr, "\nIoctl IPX_BIND_SOCKET failed setting value %d\n",
					(uint32)socketNumber );
#endif
		return(FAILURE);
	}
	return(SUCCESS);
}

int
SetHiLoWater
	(int 	fd,
	uint32 	hiWater,
	uint32 	loWater,
	int 	timeOut)
{
	struct strioctl ioc;   
	IpxSetWater_t water;   

	water.hiWater = hiWater;
	water.loWater = loWater;
	/* build an ioctl to set the socket */
	ioc.ic_dp = (char *)&water;
	ioc.ic_len = sizeof(IpxSetWater_t);
	ioc.ic_cmd = IPX_SET_WATER;
	ioc.ic_timout = timeOut;  
	if (ioctl(fd, I_STR, &ioc) == -1) {
#ifdef DEBUG
		fprintf(stderr, "\nIoctl IPX_SET_WATER failed setting value %d\n",
				hiWater );
#endif
		return(FAILURE);
	}
	return(SUCCESS);
}

PrivateIoctl
	(int fd,
	int ioctlName,
	char *buffer,
	int size)
{
	struct strioctl privateIoctl;

	privateIoctl.ic_dp = buffer;
	privateIoctl.ic_len = size;
	privateIoctl.ic_cmd = ioctlName;
	privateIoctl.ic_timout = 5;
	if (ioctl(fd, I_STR, &privateIoctl) == -1) {
#ifdef DEBUG
		fprintf(stderr, "\nPrivate Ioctl 0x%X failed\n", ioctlName);
#endif
		return(FAILURE);
	}
	return (SUCCESS);
}

int
killNPSD()
{
	const char *PidLogDir;

	if( (PidLogDir = NWCMGetConfigDirPath()) == NULL) {
		return(FAILURE);
	}
	if( LogPidKill((char *)PidLogDir, "npsd", SIGTERM) != 0) {
		return(FAILURE);
	}
	return(SUCCESS);
}
