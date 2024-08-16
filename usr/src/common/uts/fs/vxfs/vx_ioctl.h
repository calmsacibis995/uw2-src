/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_ioctl.h	2.9 28 Mar 1994 01:38:30 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_ioctl.h	1.8"

/*
 * Copyright (c) 1994 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 *
 *	       RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *	       VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SANTA CLARA, CA 95054
 */

#ifndef	_FS_VXFS_VX_IOCTL_H
#define	_FS_VXFS_VX_IOCTL_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * ioctl arguments for VXFS ioctls
 */

#define	VX_IOCTL   (('V' << 24) | ('X' << 16) | ('F' << 8))

#define	VX_SETCACHE	(VX_IOCTL | 1)	/* set caching advisories */
#define	VX_GETCACHE	(VX_IOCTL | 2)	/* get caching advisories */
#define	VX_SETEXT	(VX_IOCTL | 3)	/* set extent allocation policy	*/
#define	VX_GETEXT	(VX_IOCTL | 4)	/* get extent allocation policy	*/
#define	VX_GETFSOPT	(VX_IOCTL | 5)	/* get file system options */
#define	VX_EXPAND	(VX_IOCTL | 6)	/* expand a file system */
#define	VX_EXTREORG	(VX_IOCTL | 7)	/* reorganize extents in file */
#define	VX_DIRSORT	(VX_IOCTL | 8)	/* sort directory entries */
#define	VX_SNAPREAD	(VX_IOCTL | 9)	/* read a snapshot file system */
#define	VX_SHRINK	(VX_IOCTL | 10)	/* shrink a file system */
#define	VX_SETROOTFSOPT	(VX_IOCTL | 12)	/* set mountflags for root */
#define	VX_UPGRADE	(VX_IOCTL | 13)	/* upgrade layout */
#define	VX_MAKEFSET	(VX_IOCTL | 14)	/* make a new file set */
#define	VX_REMFSET	(VX_IOCTL | 15)	/* remove a file set */
#define	VX_NAMEFSET	(VX_IOCTL | 16)	/* name the unnamed file set */
#define	VX_CHGQUOTA	(VX_IOCTL | 17)	/* change the quota on a file set */
#define VX_ATTRLIST	(VX_IOCTL | 18) /* list extended attributes */
#define VX_ATTRGET	(VX_IOCTL | 19) /* get an extended attribute */
#define VX_ATTRSET	(VX_IOCTL | 20) /* set extended attributes */

/*
 * Values for freeze and thaw ioctls.  These must match the volume manager
 * VOL_FREEZE and VOL_THAW ioctl values.
 */

#ifndef	VIOC
#define	VIOC	(('V' << 24) | ('O' << 16) | ('L' << 8))
#endif	/* VIOC */

#define	VX_FREEZE	(VIOC | 100)	/* freeze the file system */
#define	VX_THAW		(VIOC | 101)	/* unfreeze the file system */

/*
 * vx_setcache flags
 */
#define	VX_ADVFLAGS	0x0001f		/* valid advisory flags */
#define	VX_RANDOM	0x00001		/* file is accessed randomly */
#define	VX_SEQ		0x00002		/* file is accessed sequentially */
#define	VX_DIRECT	0x00004		/* perform direct (un-buffered) i/o */
#define	VX_NOREUSE	0x00008		/* do not cache file data */
#define	VX_DSYNC	0x00010		/* synchronous data i/o (not mtime) */

/*
 * Extra caching flags for internal advisories only.
 */

#define	VX_TMPCACHE	0x10000		/* use tmp caching semantics */

/*
 * vx_setext structure and extent allocation flags
 */

struct vx_ext {
	off_t	ext_size;		/* extent size */
	daddr_t	reserve;		/* space reservation */
	int	a_flags;		/* allocation flags */
};

#define	VX_AFLAGS	0x3f	/* valid flags for a_flags */
#define	VX_NOEXTEND	0x01	/* file is not to be extended */
#define	VX_TRIM		0x02	/* trim reservation to i_size on close */
#define	VX_CONTIGUOUS	0x04	/* file must be contiguously allocated */
#define	VX_ALIGN	0x08	/* extents allocated on extent boundaries */
#define	VX_NORESERVE	0x10	/* don't change i_reserve */
#define	VX_CHGSIZE	0x20	/* change i_size to match reservation */

/*
 * Flags for VX_GETFSOPT
 */

#define	VX_FSO_NOLOG		0x00001	/* mounted with VX_MS_NOLOG */
#define	VX_FSO_BLKCLEAR		0x00002	/* mounted with VX_MS_BLKCLEAR */
#define	VX_FSO_NODATAINLOG	0x00004	/* mounted with VX_MS_NODATAINLOG */
#define	VX_FSO_SNAPSHOT		0x00008	/* is a snapshot */
#define	VX_FSO_SNAPPED		0x00010	/* is being snapped */
#define	VX_FSO_VJFS		0x00020	/* the kernel is VJFS */
#define	VX_FSO_DELAYLOG		0x00040	/* mounted with VX_MS_DELAYLOG */
#define	VX_FSO_TMPLOG		0x00080	/* mounted with VX_MS_TMPLOG */
#define	VX_FSO_CACHE_DIRECT	0x00100	/* mounted with VX_MS_CACHE_DIRECT */
#define	VX_FSO_CACHE_DSYNC	0x00200	/* mounted with VX_MS_CACHE_DSYNC */
#define	VX_FSO_CACHE_CLOSESYNC	0x00400	/* mounted with VX_MS_CACHE_CLOSESYNC */
#define	VX_FSO_OSYNC_DIRECT	0x01000	/* mounted with VX_MS_OSYNC_DIRECT */
#define	VX_FSO_OSYNC_DSYNC	0x02000	/* mounted with VX_MS_OSYNC_DSYNC */
#define	VX_FSO_OSYNC_CLOSESYNC	0x04000	/* mounted with VX_MS_OSYNC_CLOSESYNC */
#define	VX_FSO_FILESET		0x10000	/* mounted as a file set */
#define	VX_FSO_CACHE_TMPCACHE	0x20000	/* mounted with VX_MS_CACHE_TMPCACHE */
#define	VX_FSO_OSYNC_DELAY	0x40000	/* mounted with VX_MS_OSYNC_DELAY */


/*
 * Structure for VX_DIRSORT ioctl
 */

struct vx_dirsort {
	long	dir_fsetidx;	/* file set index */
	ino_t	dir_inode;	/* inode number */
	off_t	dir_off;	/* offset within directory in blocks */
	daddr_t	dir_bno[2];	/* original block numbers */
	caddr_t	dir_ibuf[2];	/* original content of blocks */
	caddr_t	dir_obuf[2];	/* new contents of blocks */
	size_t	dir_olen[2];	/* new lengths of blocks */
};

/*
 * Structure for VX_SNAPREAD ioctl
 */

struct vx_snapread {
	caddr_t	sr_buf;		/* address of read buffer */
	int	sr_off;		/* offset in file system blocks */
	int	sr_len;		/* number of blocks to read */
};

/*
 * Data structures passed to the mount system call.  The first is
 * used for a normal mount, the second is used for a file set mount or
 * a snapshot mount.
 */

struct vx_mountargs1 {
	int	mflags;		/* mount flags--see below */
};

struct vx_mountargs3 {
	int	mflags;		/* mount flags--see below */
	char	*primaryspec;	/* block special file of primary file system */
	long	snapsize;	/* size in 512-byte blocks of snapshot */
};

struct vx_mountargs5 {
	int	mflags;		/* mount flags--see below */
	char	*primaryspec;	/* block special file of primary file system */
	long	snapsize;	/* size in 512-byte blocks of snapshot */
	char	*aggrname;	/* name of aggregate */
	char	*fsetname;	/* name of file set */
};

/*
 * mount options for vxfs only
 *
 * The VX_MS_CACHE_DIRECT and VX_MS_CACHE_DSYNC options also imply an
 * fsync for all files on last close.
 */

#define	VX_MS_MASK		0xff7f	/* mask of valid flags */
#define	VX_MS_NOLOG		0x0001	/* no logging */
#define	VX_MS_BLKCLEAR		0x0002	/* guarantee cleared storage */
#define	VX_MS_SNAPSHOT		0x0004	/* shadow mount */
#define	VX_MS_NODATAINLOG	0x0008	/* disable logged writes */
#define	VX_MS_DELAYLOG		0x0010	/* unix semantics */
#define	VX_MS_TMPLOG		0x0020	/* temporary file system semantics */
#define	VX_MS_FILESET		0x0040	/* this is a file set mount */

#define	VX_MS_CACHE_MASK	0x0f00	/* mask of valid mincache flags */
#define	VX_MS_CACHE_DIRECT	0x0100	/* async writes handled as VX_DIRECT */
#define	VX_MS_CACHE_DSYNC	0x0200	/* async writes handled as VX_DSYNC */
#define	VX_MS_CACHE_CLOSESYNC	0x0400	/* all files fsynced on last close */
#define	VX_MS_CACHE_TMPCACHE	0x0800	/* fast caching semantics */

#define	VX_MS_OSYNC_MASK	0xf000	/* mask of valid convosync flags */
#define	VX_MS_OSYNC_DIRECT	0x1000	/* O_SYNC writes handled as VX_DIRECT */
#define	VX_MS_OSYNC_DSYNC	0x2000	/* O_SYNC writes handled as VX_DSYNC */
#define	VX_MS_OSYNC_CLOSESYNC	0x4000	/* O_SYNC writes handled as async and
					   all files fsynced on last close */
#define	VX_MS_OSYNC_DELAY	0x8000	/* O_SYNC writes handled as delayed */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_VXFS_VX_IOCTL_H */
