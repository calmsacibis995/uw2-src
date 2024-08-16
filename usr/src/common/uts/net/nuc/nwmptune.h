/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwmptune.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwmptune.h,v 2.51.2.1 1994/12/12 01:27:52 stevbam Exp $"

#ifndef _NET_NUC_NWMP_NWMPTUNE_H
#define _NET_NUC_NWMP_NWMPTUNE_H

/*
 *  Netware Unix Client
 *
 *	  MODULE:   nwmptune.h
 *	ABSTRACT:   Tuneable parameters for the Netware Management Portal
 *              psudo-device driver.
 */

#ifdef _KERNEL_HEADERS
#ifndef _NET_NUC_NWMP_NWMPDEV_H
#include <net/nuc/nwmpdev.h>
#endif
#elif defined(_KERNEL)
#include <sys/nwmpdev.h>
#else
#include <sys/nwmpdev.h>
#endif	/* _KERNEL_HEADERS */

/*
 *	Currently, number of opens is the same as number of device
 *	minors.  Allowed Opens is the parameter that should be modified
 *
 */
#define NWMP_ALLOWED_OPENS	NUCNWMPOPENS
#define NWMP_MINOR_DEVICES	NWMP_ALLOWED_OPENS

/*
 *	Allocate the device structure that will track the state of
 *	the devices.
 */
struct NWMPDevice *nwmpDevices[NWMP_MINOR_DEVICES];

#endif	/* _NET_NUC_NWMP_NWMPTUNE_H */
