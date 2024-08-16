/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SYS_CLKARB_H	/* wrapper symbol for kernel use */
#define	_SYS_CLKARB_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/clkarb.h	1.8"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Clock Arbiter board definitions
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <io/slic.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/slic.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#define	CLKARB_SLIC	0		/* always SLIC id 0 */
#define	CA_FOL		32		/* 1st slot optionally low priority */
#define	CA_LOL		41		/* last slot optionally low priority */

#define	FP_NLIGHTS	48
#define	FP_LIGHTOFF(i)	(slic_wrAddr(CLKARB_SLIC, (fp_lightmap[(i)])))
#define	FP_LIGHTON(i)	(slic_wrAddr(CLKARB_SLIC, (fp_lightmap[(i)] ^ 1)))

#define	FP_IO_INACTIVE	(slic_wrAddr(CLKARB_SLIC, SL_C_IO_ACTIVE - 1))
#define	FP_IO_ACTIVE	(slic_wrAddr(CLKARB_SLIC, SL_C_IO_ACTIVE))

#define	FP_IO_OFFLINE	(slic_wrAddr(CLKARB_SLIC, SL_C_IO_ONLINE - 1))
#define	FP_IO_ONLINE	(slic_wrAddr(CLKARB_SLIC, SL_C_IO_ONLINE))

#define	FP_IO_NOERROR	(slic_wrAddr(CLKARB_SLIC, SL_C_IO_ERROR - 1))
#define	FP_IO_ERROR	(slic_wrAddr(CLKARB_SLIC, SL_C_IO_ERROR))

#ifdef _KERNEL

extern	int	fp_lights;
extern	unchar	fp_lightmap[FP_NLIGHTS];
extern	int	light_show;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SYS_CLKARB_H */
