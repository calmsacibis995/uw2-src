/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfiname.h	1.4"
#ident  "@(#)nwfiname.h	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfiname.h,v 2.1.2.3 1995/01/24 18:15:18 mdash Exp $"

#ifndef _NWFINAME_H	/* wrapper symbol for kernel use */
#define _NWFINAME_H	/* subject to change without notice */

/*
 * Name maintenence in nucfs.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <util/sysmacros.h>	/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/sysmacros.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

/*
 * Names table lock.
 */
extern fspin_t	NWfiNameTableLock;

/*
 * Macros to lock/unlock the names table.
 */
#define NWFI_NAME_LOCK()	FSPIN_LOCK(&NWfiNameTableLock)
#define NWFI_NAME_UNLOCK()	FSPIN_UNLOCK(&NWfiNameTableLock)

#if defined(__cplusplus)
	}
#endif

#endif /* _NWFINAME_H */
