/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5FS_S5MACROS_H	/* wrapper symbol for kernel use */
#define _FS_S5FS_S5MACROS_H	/* subject to change without notice */

#ident	"@(#)kern:fs/s5fs/s5macros.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#define FsLTOP(fs, b)	((b) << (fs)->fs_ltop)
#define FsPTOL(fs, b)	((b) >> (fs)->fs_ltop)
#define FsITOD(fs, x)	(daddr_t)(((unsigned)(x)+(2*(fs)->fs_inopb-1)) >> (fs)->fs_inoshift)
#define FsITOO(fs, x)	(daddr_t)(((unsigned)(x)+(2*(fs)->fs_inopb-1)) & ((fs)->fs_inopb-1))
#define FsINOS(fs, x)	((((x) >> (fs)->fs_inoshift) << (fs)->fs_inoshift) + 1)

#define LTOPBLK(blkno, bsize)	(blkno * ((bsize>>SCTRSHFT)))
#define NINDIR(fs)		((fs)->fs_nindir)
#define lblkno(fs, loc)	((loc) >> (fs)->fs_bshift)
#define blkoff(fs, loc)	((loc) & ~(fs)->fs_bmask)
#define blkrounddown(fs, loc)	((loc) & (fs)->fs_bmask)
#define blkroundup(fs, size)	(((size) + (fs)->fs_bsize - 1) & (fs)->fs_bmask)

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_S5FS_S5MACROS_H */
