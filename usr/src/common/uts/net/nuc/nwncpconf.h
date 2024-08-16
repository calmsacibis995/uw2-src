/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwncpconf.h	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwncpconf.h,v 2.51.2.1 1994/12/12 01:27:59 stevbam Exp $"

#ifndef _NET_NUC_NCP_NWNCPCONF_H
#define _NET_NUC_NCP_NWNCPCONF_H


/*
 *  Netware Unix Client
 *
 *	  MODULE: nwncpconf.h
 *	ABSTRACT: Configuration header file used to define external variable
 *            structure used in tuning the package without re-compiling
 *            the driver itself.
 */

struct ncpConfStruct {
	int	minMemSize;		/* Configuration data */
	int	maxMemSize;
	int defMemSize;
	int memSize;
	int	maxServers;
	int maxTasksPerServer;
	int ioBuffersPerTask;
};

#ifndef _NWNCP_SPACE
/*
 *	If being included by the driver itself, extern in the structure as
 *	the parameters have been set by space.c
 */
extern struct ncpConfStruct ncpConf;

#endif

#endif /* _NET_NUC_NCP_NWNCPCONF_H */
