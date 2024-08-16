/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_MAP_U_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_MAP_U_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg_map_u.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/param.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/param.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Per-LWP segmap data, stored in the user structure.
 */

#ifndef PGPERSMAP
#define PGPERSMAP	((MAXBSIZE + PAGEOFFSET) / PAGESIZE)
#endif

struct segmap_u_data {
	struct page *sud_pl[PGPERSMAP + 1];	/* I/O page list */
	ushort_t sud_off;		/* offset of xfer w/in chunk */
	ushort_t sud_end;		/* last xfer byte plus one */
#ifdef DEBUG
	struct smap *sud_smp;		/* active smap chunk pointer */
#endif /* DEBUG */
};

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_SEG_MAP_U_H */
