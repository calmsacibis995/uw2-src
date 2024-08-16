/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_LOCKMGR_KLM_LK_H	/* wrapper symbol for kernel use */
#define _NET_LOCKMGR_KLM_LK_H	/* subject to change without notice */

#ident	"@(#)kern:net/lockmgr/klmlk.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	klmlk.h, all the lock hierarchy and minipl information
 *	pertaining to the kernel lock manager subsystem.
 */

#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <util/ipl.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/ipl.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define PRIKLM		PRIMED

#define KLM_HIER_BASE	150

#define KLM_HIERSYNC	KLM_HIER_BASE + 5
#define KLM_HIERCONFIG	KLM_HIER_BASE + 5

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* !_NET_LOCKMGR_KLM_LK_H */
