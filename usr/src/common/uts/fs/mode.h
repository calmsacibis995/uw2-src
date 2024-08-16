/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_MODE_H	/* wrapper symbol for kernel use */
#define _FS_MODE_H	/* subject to change without notice */

#ident	"@(#)kern:fs/mode.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Conversion between vnode types/modes and encoded type/mode as
 * seen by stat(2) and mknod(2).
 */
extern enum vtype	iftovt_tab[];
extern ushort		vttoif_tab[];
#define IFTOVT(M)	(iftovt_tab[((M) & S_IFMT) >> 12])
#define VTTOIF(T)	(vttoif_tab[(int)(T)])
#define MAKEIMODE(T, M)	(VTTOIF(T) | ((M) & ~S_IFMT))

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_MODE_H */
