/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nuclocks.h	1.8"
#ifndef _NET_NUC_NUCLOCKS_H
#define _NET_NUC_NUCLOCKS_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nuclocks.h,v 2.51 1994/12/01 07:56:02 hashem Exp $"

/*
 *    Copyright Novell Inc. 1994 - 1995
 *    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *    No part of this file may be duplicated, revised, translated, localized
 *    or modified in any manner or compiled, linked or uploaded or
 *    downloaded to or from any computer system without the prior written
 *    consent of Novell, Inc.
 *
 *
 *  Netware Unix Client 
 *
 */

#ifdef _KERNEL_HEADERS
#include <util/ksynch.h>
#else
#include <sys/ksynch.h>
#endif	/* _KERNEL_HEADERS */

LKINFO_DECL(nucLockInfo, "NETNUC:NUC:nucLock", 0);
LKINFO_DECL(nucUpperLockInfo, "NETNUC:NUC:nucUpperLock", 0);
LKINFO_DECL(criticalSectLockInfo, "NETNUC:NUC:criticalSectLock", 0);
LKINFO_DECL(nucIPCLockInfo, "NETNUC:NUC:nucIPCLock", 0);
LKINFO_DECL(nucTSLockInfo, "NETNUC:NUC:nucTSLock", 0);
LKINFO_DECL(serviceListSleepLockInfo, "NETNUC:NUC:serviceListSleepLockInfo",0);
LKINFO_DECL(spiTaskListSleepLockInfo, "NETNUC:NUC:spiTaskListSleepLockInfo",0);

#endif _NET_NUC_NUCLOCKS_H
