/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_FS_SFS_DOW_H
#define	_FS_SFS_DOW_H

#ident	"@(#)kern:fs/sfs/sfs_dow.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef	_KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <fs/sfs/sfs_inode.h>	/* REQUIRED */
#include <fs/fbuf.h>		/* REQUIRED */
#include <fs/dow.h>		/* REQUIRED */

#elif	defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/fs/sfs_inode.h>	/* REQUIRED */
#include <sys/fbuf.h>		/* REQUIRED */
#include <sys/dow.h>		/* REQUIRED */

#endif

#ifdef	_KERNEL

extern dowid_t sfs_dow_create_inode(inode_t *);
extern dowid_t sfs_dow_create_page(inode_t *, off_t , uint_t);
extern dowid_t sfs_dow_iupdat(inode_t *);
extern void sfs_dow_order(dowid_t, dowid_t);

/*
 * structure used to control hardening of different file system items.
 *
 *	sfs_dir specifies the maximum time, in clock ticks, from the
 *		point at which a directory is created and the time its
 *		directory entry (and inode) get written to disk.
 *
 *	sfs_file specifies the maximum time, in clock ticks, from the point 
 *		at which a file is created and the time its directory entry
 *		(and inode) get written to disk.
 *
 *	sfs_remove specifies the maximum time, in clock ticks, from the
 *		point at which a file or directory is removed and the time
 *		the structural updates for the remove are written to disk.
 */
extern struct sfs_hardening {
	clock_t sfs_dir;
	clock_t sfs_file;
	clock_t sfs_remove;
} sfs_hardening;

#endif

#if defined(__cplusplus)
	}
#endif

#endif
