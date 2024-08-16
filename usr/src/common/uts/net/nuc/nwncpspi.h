/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwncpspi.h	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwncpspi.h,v 2.51.2.1 1994/12/12 01:28:06 stevbam Exp $"

#ifndef _NET_NUC_NCP_NWNCPSPI_H
#define _NET_NUC_NCP_NWNCPSPI_H

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncpspi.h
 *	ABSTRACT: NCP functions that will be installed in the SPI switch table
 *            to handle the specific service and task operations allowed
 *            in NCP
 */

/* #include "nwspiswitch.h" */

#define NCP_MODULE

extern ccode_t	NCPsiInitNCP(),
		NCPsiFreeNCP(),
		NCPsiCreateService(),
		NCPsiCreateTask(),
		NCPsiDestroyTask(),
		NCPsiAuthenticateTask(),
		NCPsiLicenseTask(),
		NCPsiRawNCP(),
		NCPsiRegisterRawNCP(),
		NCPsiRelinquishRawToken(),
		NCPsilGetUserID(),
#ifndef FS_ONLY
		NCPsiGetBroadcastMessage(),
		NCPsiDestroyService();
#else
		NCPsiGetBroadcastMessage();
#endif /* FS_ONLY */

SPI_OPS_T ncp_spiops = {
			NCPsiInitNCP,
			NCPsiFreeNCP,
			NCPsiCreateService,
			NCPsiCreateTask,
			NCPsiDestroyTask,
			NCPsiAuthenticateTask,
			NCPsiLicenseTask,

			/*
			 *	Raw request operations
			 */
			NCPsiRawNCP,				/* Send Raw NCP */
			NCPsiRegisterRawNCP,		/* register for raw NCP (get task #) */
			NCPsiRelinquishRawToken,
			NCPsilGetUserID,			/* Scan services by userid (SSBU) */
#ifndef FS_ONLY
			NCPsiGetBroadcastMessage,	/* Get outstanding message */
			NCPsiDestroyService
#else
			NCPsiGetBroadcastMessage	/* Get outstanding message */
#endif /* FS_ONLY */
};

#endif /* _NET_NUC_NCP_NWNCPSPI_H */
