/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwmpdev.h	1.10"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwmpdev.h,v 2.51.2.1 1994/12/12 01:27:26 stevbam Exp $"

#ifndef _NET_NUC_NWMP_NWMPDEV_H
#define _NET_NUC_NWMP_NWMPDEV_H

/*
 *  Netware Unix Client
 *
 *	  MODULE: nwmpdev.h
 *	ABSTRACT: Define the netware management portal device structure
 */

#ifdef _KERNEL_HEADERS
#include <util/param.h>
#include <util/types.h>
#include <proc/cred.h>
#include <net/nuc/nwctypes.h>
#include <net/xti.h>
#elif defined(_KERNEL)
#include <sys/param.h>
#include <sys/types.h>
#include <sys/cred.h>
#include <sys/nwctypes.h>
#include <sys/xti.h>
#define MAX_ADDRESS_SIZE 30
#else
#include <sys/param.h>
#include <sys/types.h>
#include <sys/cred.h>
#include <sys/nwctypes.h>
#include <sys/xti.h>
#endif	/* _KERNEL_HEADERS */

#define NWMP_SERVICE_NAME_LENGTH 64

struct nwmpRawHandle {
	struct netbuf address;
	char buffer[MAX_ADDRESS_SIZE];
	uint32 connectionReference;
	unsigned char	token;
};

struct NWMPDevice {
	long	state;			/* state of this minor number */
	void	*diagnosticBlock;	/* pointer to diagnostic information */
	struct	nwmpRawHandle raw;	/*  raw Sproto Request*/
	cred_t	openCred;		/* credentials of the opening task */
	uint32	openPid;
	int	serviceFlags;		/* services provided by this task */
};

/*
	These bits define the various services provided by tasks opening 
	the Management Portal.  These bits are set by nwmpioctl and reset
	by nwmpclose.
*/
#define NWMPserviceCreateTaskRequest		0x00000002
#define NWMPserviceGetMessage				0x00000004

#ifdef NUC_DEBUG
#define NWMP_CMN_ERR cmn_err
#endif

#endif	/* _NET_NUC_NWMP_NWMPDEV_H */
