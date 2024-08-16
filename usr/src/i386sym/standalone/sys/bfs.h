/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_BFS_H	/* wrapper symbol for kernel use */
#define _SYS_BFS_H	/* subject to change without notice */

#ident	"@(#)stand:i386sym/standalone/sys/bfs.h	1.1"

/*
 * bfs.h - structure of Boot Independent Filesystem (BFS).
 */
#ifndef _FS_VNODE_H
#include <sys/vnode.h>
#endif /* _FS_VNODE_H */


#define BFS_MAXFNLEN 14			/* Maximum file length */
#define BFS_MAXFNLENN (BFS_MAXFNLEN+1)  /* Used for NULL terminated copies */


struct bfsvattr {
	vtype_t		va_type;	/* vnode type (for create) */
	mode_t	va_mode;	/* file access mode */
	uid_t	va_uid;		/* owner user id */
	uid_t	va_gid;		/* owner group id */
	nlink_t	va_nlink;	/* number of references to file */
	time_t		va_atime;	/* time of last access */
	time_t		va_mtime;	/* time of last modification */
	time_t		va_ctime;	/* time file ``created'' */
	long		va_filler[4];	/* padding */
};

/*
 * The bfs_dirent is the "inode" of BFS.  Always on disk, it is pointed
 * to (by disk offset) by the vnode and is referenced every time an
 * operation is done on the vnode.  It must be referenced every time,
 * as things can move around very quickly
 */
struct bfs_dirent
{
	ushort  d_ino;				/* inode */
	daddr_t d_sblock;			/* Start block */
	daddr_t d_eblock;			/* End block */
	daddr_t d_eoffset;			/* EOF disk offset (absolute) */
	struct  bfsvattr d_fattr;		/* File attributes */
};


struct bfs_ldirs {
	ushort l_ino;
	char   l_name[BFS_MAXFNLEN];
};

/*
 * Incore BFS information for bookkeeping on a given BFS filesystem.
 */
struct bfs_info {
	struct bfs_info *bsup_next;	/* Linked list of these. */
	dev_t bsup_dev;			/* Device where BFS located */
        daddr_t bsup_boff;         	/* starting block offset on device */
	off_t bsup_start;		/* The filesystem data start offset */
	off_t bsup_end;			/* The filesystem data end offset */
	uid_t bsup_uid;		/* owner user id */
	uid_t bsup_gid;		/* owner group id */
	long bsup_freeblocks;		/* # of freeblocks (for statfs) */
	long bsup_freedrents;		/* # of free dir entries remaining. */
	int  bsup_eldirs;		/* Byte offset for end of ldir array,
					 * which is also kept in root inode. */
	ino_t bsup_lastfile;		/* Last allocated file's inode # */
	ino_t bsup_lasteblk;		/* Last allocated data block */
	daddr_t bsup_dirsblk;		/* BFS block starting the ldirs-array */
	int bsup_ndirs;			/* #entries in the BFS ldirs-array */
};

/* The header of the disk superbuff */
struct bfs_bdsuphead
{
	long 	bh_bfsmagic;		/* Magic number */
	off_t	bh_start;		/* Filesystem data start offset */
	off_t	bh_end;			/* Filesystem data end offset */
};

/*
 * The sanity structure is used to promote sanity in compaction.  Used
 * correctly, a crash at any point during compaction is recoverable.
 */
struct bfs_sanity
{
	daddr_t fromblock;		/* "From" block of current transfer */
	daddr_t toblock;		/* "To" block of current transfer */
	daddr_t bfromblock;		/* Backup of "from" block */
	daddr_t btoblock;		/* Backup of "to" block */
};

/* The on-disk superbuff */
struct bdsuper
{
	struct bfs_bdsuphead bdsup_head;/* Header info */
	struct bfs_sanity bdsup_sane;	/* Crash recovery during compacting */
	char    bdsup_fsname[6];	/* file system name */
	char    bdsup_volume[6];	/* file system volume name */
	long    bdsup_filler[118];	/* Padding */
};

#define	bdsup_bfsmagic	bdsup_head.bh_bfsmagic
#define	bdsup_start	bdsup_head.bh_start
#define	bdsup_end	bdsup_head.bh_end
#define	bdcp_fromblock	bdsup_sane.fromblock
#define	bdcp_toblock	bdsup_sane.toblock
#define	bdcpb_fromblock	bdsup_sane.bfromblock
#define	bdcpb_toblock	bdsup_sane.btoblock

#define BFS_MAGIC	0x1BADFACE
#define BFS_SUPEROFF	0
#define BFS_DIRSTART	(BFS_SUPEROFF + sizeof(struct bdsuper))
#define BFS_BSIZE	512
#define BIGFILE		500
#define SMALLFILE	10
#define BFSROOTINO	2
#define DIRBUFSIZE	1024

#define BYTE_TO_BLK(x)  ((x) / BFS_BSIZE)
#define EXC_BLK(x)      ((x) & (BFS_BSIZE - 1))

#define BFS_OFF2INO(offset) \
	((offset - BFS_DIRSTART) / sizeof(struct bfs_dirent)) + BFSROOTINO

#define BFS_INO2OFF(inode) \
	((inode - BFSROOTINO) * sizeof(struct bfs_dirent)) + BFS_DIRSTART

#endif	/* _SYS_BFS_H */
