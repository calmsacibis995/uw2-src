/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _ACC_PRIV_HIER_H	/* wrapper symbol for kernel use */
#define _ACC_PRIV_HIER_H	/* subject to change without notice */

#ident	"@(#)kern:acc/priv/priv_hier.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/ipl.h>	/* REQUIRED */
#include <util/ghier.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/* Lock hierarchies for file privilege locks. */

#define FPRIV_HIER	KERNEL_HIER_BASE

#define PLFPRIV		PL1

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _ACC_PRIV_HIER_H */
