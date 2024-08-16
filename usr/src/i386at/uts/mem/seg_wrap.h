/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_WRAP	/* wrapper symbol for kernel use */
#define _MEM_SEG_WRAP	/* subject to change without notice */

#ident	"@(#)kern-i386at:mem/seg_wrap.h	1.1"
#ident	"$Header: $"

/***************************************************************************

       "@(-)seg_wrap.h	1.2   LCC";  Mod Time 13:39:49 6/7/93

       Copyright (c) 1989 Locus Computing Corporation.
       All rights reserved.
       This is an unpublished work containing CONFIDENTIAL INFORMATION
       that is the property of Locus Computing Corporation.
       Any unauthorized use, duplication or disclosure is prohibited.

***************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
**	internal data needed by the wrap segment driver
*/
struct segwrap_data {
	vaddr_t base;		/* the address of the other segment which */
				/* mirrors the base of our segment */
};

/*
** 	A pointer to this structure is passed to segmap_create() when the kernel
**	wishes to set up a wrapped mapping.
*/
struct segwrap_crargs {
	vaddr_t base;		/* the address of the other segment which */
				/* mirrors the base of our segment */
};

extern caddr_t	segwrap_args;
extern caddr_t	ksegwrap_args;
extern int segwrap_create();

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_SEG_WRAP */
