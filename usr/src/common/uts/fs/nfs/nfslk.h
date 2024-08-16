/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_NFS_NFSLK_H	/* wrapper symbol for kernel use */
#define _FS_NFS_NFSLK_H	/* subject to change without notice */

#ident	"@(#)kern:fs/nfs/nfslk.h	1.13"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	nfslk.h, all the lock hierarchy and minipl information
 *	pertaining to nfs
 */

#ifdef	_KERNEL

/*
 * various hierarchy definitions for nfs
 */
#define		NFS_HIERSTATE		170
#define		NFS_HIERRW		150

#define		NFS_HIERASYNC		150
#define		NFS_HIERMMAP		150
#define		NFS_HIERMIASYNC		160
#define		NFS_HIERMNT		150
#define		NFS_HIERJUNK		150
#define		NFS_HIERCHTAB		150
#define		NFS_HIERRPFREEL		150
#define		NFS_HIERNFSRROK		150
#define		NFS_HIERMI		150
#define		NFS_HIERSP		150

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* !_FS_NFS_NFSLK_H */
