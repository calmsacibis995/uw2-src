/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_fs.h	2.51 26 Oct 1994 16:35:36 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_fs.h	1.21"

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

#ifndef	_FS_VXFS_VX_FS_H
#define	_FS_VXFS_VX_FS_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_KSYNCH_H
#include <util/ksynch.h> /* REQUIRED */
#endif

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#ifndef _UTIL_PARAM_H
#include <util/param.h> /* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h> /* REQUIRED */
#endif

#ifndef _FS_VFS_H
#include <fs/vfs.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h>	/* REQUIRED */
#include <sys/param.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */
#include <sys/vfs.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *  fs.h -- primary file system space management structures
 *
 * Each logical volume contains exactly one file system.
 * A file system consists of a number of allocation units.
 * Each allocation unit has data blocks and an extent map.
 * If this is a version 1 file system, then each allocation
 * unit also contains inodes.  In a version 2 file system,
 * the inodes are contained in file sets.
 *
 * A file system is described by its super-block, which in turn
 * describes the allocation units.
 */

/*
 * The OLT (Object Location Table) structure.  The OLT is used
 * at mount time to find out what file sets exist and where they
 * are stored in the file system.
 */

struct vx_olt {
	daddr_t		ol_ext[2];	/* 0x00 extents for this olt */
	long		ol_size;	/* 0x08 size of extents in bytes */
	long		ol_esize;	/* 0x0c size of extents in blocks */
	caddr_t		ol_oltdata;	/* 0x10 olt data from ol_ext[0] */
	caddr_t		ol_oltcopy;	/* 0x14 olt data from ol_ext[1] */
	struct vx_oltilist *ol_oip;	/* 0x18 entry for first ilist inodes */
	struct vx_oltfshead *ol_ofs;	/* 0x1c entry for fset header inodes */
	struct vx_oltcut *ol_ocut;	/* 0x20 entry for cut inode */
};

/*
 * Magic numbers for object location table extents
 * and inode allocation unit extents.
 */

#define	VX_OLTMAGIC		0xa504fcf5
#define	VX_IAUMAGIC		0xa505fcf5

/*
 * Valid OLT entry types.
 */

#define	VX_OLTFREE	1
#define	VX_OLTFSHEAD	2
#define	VX_OLTCUT	3
#define	VX_OLTILIST	4

/*
 * This structure is the header of every OLT extent.  The checksum
 * is computed based on all the bytes in the OLT extent beginning
 * with the timestamp in vx_olthead.  Each vx_oltent in the OLT is
 * aligned on an 8 byte boundary.  Each OLT extent is replicated so
 * none of the data will be lost if a block fails.
 */

struct vx_olthead {
	long		olt_magic;	/* 0x00 magic number */
	long		olt_size;	/* 0x04 length of this entry */
	long		olt_checksum;	/* 0x08 checksum of extent */
	long		olt_padding1;	/* 0x0c align to 8 byte boundary */
#ifdef	_KERNEL
	timestruc_t	olt_time;	/* 0x10 time of last olt modification */
#else
	time_t		olt_time;	/* 0x10 time of last olt modification */
	long		olt_etime;	/* 0x14 spare for eft */
#endif
	long		olt_totfree;	/* 0x18 free space in extent */
	daddr_t		olt_extents[2];	/* 0x1c addr of this extent, replica */
	long		olt_esize;	/* 0x24 size of this extent */
	daddr_t		olt_next[2];	/* 0x28 addr of next extent, replica */
	long		olt_nsize;	/* 0x30 size of next extent */
	long		olt_padding2;	/* 0x34 align to 8 byte boundary */
};

/*
 * Free space in the OLT
 */

struct vx_oltfree {
	long		olt_type;	/* 0x00 type number */
	long		olt_fsize;	/* 0x04 size of this free record */
};

/*
 * initial inode extents.
 */

struct vx_oltilist {
	long		olt_type;	/* 0x00 type number */
	long		olt_size;	/* 0x04 size of this record */
	daddr_t		olt_iext[2];	/* 0x08 first two inode extents */
};

/*
 * inodes containing file set header and replica
 */

struct vx_oltfshead {
	long		olt_type;	/* 0x00 type number */
	long		olt_size;	/* 0x04 size of this record */
	long		olt_fsino[2];	/* 0x08 inodes of file set headers */
};

/*
 * The inode containing the current usage table.  The current usage table
 * is used to store information about a file set that changes frequently.
 * This includes the current number of blocks used for checking against
 * the quota, and the file set version number that changes every time
 * any file in the file set is changed.
 */

struct vx_oltcut {
	long		olt_type;	/* 0x00 type number */
	long		olt_size;	/* 0x04 size of this record */
	long		olt_cutino;	/* 0x08 inode of current usage table */
	long		olt_padding;	/* 0x0c unused */
};

union vx_oltent {
	struct vx_oltcommon {
		u_long			type;
		long			size;
	} oltcommon;
	struct vx_olthead	olthead;
	struct vx_oltfree	oltfree;
	struct vx_oltfshead	oltfshead;
	struct vx_oltcut	oltcut;
	struct vx_oltilist	oltilist;
};

/*
 * The current usage table consists of an entry for each file set.
 * The file set version number is a 64 bit integer.  It is updated
 * each time a change is made to the file set (creating, removing,
 * or changing the attributes of a file), or when the modification
 * time is changed on an inode in the file set.  The cu_curused value
 * is a count of the total number of blocks allocated to files in the
 * file set.  It is used to check against the file set quota when
 * allocating blocks.
 */

struct vx_cuent {
	ulong		cu_fsindex;	/* 0x00 file set index */
	long		cu_curused;	/* 0x04 blocks in use by file set */
	struct vx_version cu_vversion;	/* 0x08 file set version number */
};

#define	cu_lversion	cu_vversion.vv_lversion
#define	cu_hversion	cu_vversion.vv_hversion

/*
 * This structure is stored in the header block of an Inode Allocation
 * Unit.  The IAU contains a header block, followed by the iau summary
 * block, then the free inode bitmap and inode extended operation bitmap.
 */

struct vx_iauhead {
	long		iau_magic;	/* 0x00 magic number */
	ulong		iau_fsindex;	/* 0x04 index of file set */
	long		iau_iaun;	/* 0x08 sequence number in file set */
};

/*
 * vxfs vfs private data.  This structure contains all the data associated
 * with a mounted file set.  It points to the file set structure which
 * contains all the structural data for the file set.
 */

struct vx_vfs {
	struct vnode	*vfs_root;	/* 0x00 root vnode */
	struct vx_fset	*vfs_fset;	/* 0x04 pointer to file set */
	struct vfs	*vfs_vfsp;	/* 0x08 point to vfs */
	struct vx_fs	*vfs_fs;	/* 0x0c pointer to file system */
};

#define	VX_VFS(vfsp)	((struct vx_vfs *)(void *)(vfsp->vfs_data))

/*
 * Known file set indexes.  The indexes below 1000 are reserved for
 * future use by VERITAS.
 */

#define	VX_FSET_ATTR_INDEX	1	/* index of attribute file set */
#define	VX_FSET_UNNAMED_INDEX	999	/* index of unnamed file set */

/*
 * Flags for file set headers
 */

#define	VX_FSET_UNNAMED		0x01	/* unnamed file set */
#define	VX_FSET_ATTRIBUTE	0x02	/* attribute file set */
#define	VX_FSET_UNMOUNTABLE	0x08	/* file set can't be mounted */

/*
 * File set extended operations.
 */

#define	VX_FSE_REMOVE	0x1		/* remove the file set */

#define	FSETNAMESZ	32

#define	VX_FSET_VERSION		1	/* current file set header version */

/*
 * This structure defines a file set on disk.  The incore copy
 * of vx_fsethead is the same as the copy on disk.  The _nau, _ninode,
 * _dflags, _quota fields are duplicated (but more current) in the
 * vx_fset structure.
 */

struct vx_fsethead {
	long		fsh_version;	/* 0x00 version number */
	ulong		fsh_fsindex;	/* 0x04 index of this file set */
#ifdef	_KERNEL
	timestruc_t	fsh_time;	/* 0x08 time of last olt modification */
#else
	time_t		fsh_time;	/* 0x08 time of last olt modification */
	long		fsh_etime;	/* 0x0c spare for eft */
#endif
	long		fsh_fsextop;	/* 0x10 file set extended op flags */
	ino_t		fsh_ninode;	/* 0x14 total allocated inodes */
	long		fsh_nau;	/* 0x18 number of iau's in file set */
	long		fsh_ilesize;	/* 0x1c ilist extent in blocks */
	long		fsh_dflags;	/* 0x20 flags */
	long		fsh_quota;	/* 0x24 quota limit */
	ino_t		fsh_maxinode;	/* 0x28 max inode number for fileset */
	ino_t		fsh_iauino;	/* 0x2c inode allocation unit inode */
	ino_t		fsh_ilistino[2];/* 0x30 inode list inodes */
	ino_t		fsh_lctino;	/* 0x38 link count table inode */
	char		fsh_name[FSETNAMESZ];	/* 0x3c file set name */
	long		fsh_checksum;	/* 0x5c checksum of structure */
};					/* 0x60 size of structure */

/*
 * The incore file set structure.  There is one of these structures for
 * every file set in a file system.
 */

struct vx_fsetlink {
	struct vx_fset	*fst_next;
	struct vx_fset	*fst_prev;
};

struct vx_fset {
	struct vx_fset	*fst_next;	/* 0x00 pointer to next file set */
	struct vx_fset	*fst_prev;	/* 0x04 pointer to previous file set */
	struct vx_mlink	*fst_cmlink;	/* 0x08 cut mlinks */
	struct vx_map	**fst_imaplist;	/* 0x0c list of inode map pointers */
	struct vx_map  **fst_iemaplist;	/* 0x10 list of extended op map ptrs */
	struct vx_ausum **fst_ausum;	/* 0x14 list of iau summary pointers */
	long		fst_curused;	/* 0x18 current blocks used */
	struct vx_mountinfo {
		char	fmi_rdonly;	/* 0x1c mounted readonly */
		char	fmi_blkclear;	/* 0x1d guarantee cleared storage */
		char	fmi_delaylog;	/* 0x1e workstation semantics */
		char	fmi_tmplog;	/* 0x1f tmp file system semantics */
		char	fmi_nolog;	/* 0x20 logging disabled */
		char	fmi_logwrites;	/* 0x21 log synchronous writes */
		char	fmi_mounted;	/* 0x22 file set mounted */
		char	fmi_loglevel;	/* 0x23 logging level */
		int	fmi_mincache;	/* 0x24 mount caching flags */
		int	fmi_convosync;	/* 0x28 mount convosync flags */
		caddr_t	fmi_mntpt;	/* 0x2c mount point of file system */
		int	fmi_mntlen;	/* 0x30 len of mntpt pathname */
	} fst_mountinfo;
	daddr_t		*fst_ilist;	/* 0x34 inode list extent map */
	long		fst_ilistsize;	/* 0x38 size of fst_ilist in extents */
	long		fst_inopile;	/* 0x3c inodes per ilist extent */
	long		fst_inopileshift; /* 0x40 shift for fst_inopile */
	long		fst_inopilemask;  /* 0x44 mask for fst_inopile */
	long		fst_ifree;	/* 0x48 free inodes */
	ino_t		fst_ninode;	/* 0x4c current number of ninode */
	int		fst_nau;	/* 0x50 current number of inode au */
	ulong		fst_index;	/* 0x54 file set index */
	struct vx_version fst_cversion;	/* 0x58 current file set version */
	struct vx_version fst_dversion;	/* 0x60 disk file set version */
	int		fst_dflags;	/* 0x68 current copy of dflags */
	int		fst_quota;	/* 0xcc current value of quota */
	struct vx_inode	*fst_lctip;	/* 0x70 lct inode */
	struct vx_inode	*fst_iauip;	/* 0x74 iau inode */
	struct vx_inode	*fst_ilistip[2]; /* 0x78 ilist inodes */
	struct vx_fsstruct *fst_fsstruct; /* 0x80 pointer to file system */
	struct vx_vfs	*fst_vxp;	/* 0x84 pointer to vfs private data */
	struct vx_vfs	*fst_pvxp;	/* 0x88 pointer to primary file set */
	long		fst_ilalloc;	/* 0x8c allocated ilist extents */
	struct vx_cut	*fst_cutp;	/* 0x90 pointer to cut table */
	int		fst_cuindex;	/* 0x94 index in cut table */
	long		fst_fsoffset;	/* 0x98 offset in fs header file */
	long		fst_ilesize;	/* 0x9c ilist extent size in blocks */
	char		fst_name[FSETNAMESZ]; /* 0xa0 cpy of name from header */
	struct vx_fsethead fst_fshead;	/* 0xc0 file set disk header */
	daddr_t		fst_cutbno;	/* 0x11c block containing cut entry */
	struct vx_lct	**fst_lctlist;	/* 0x120 list of lct structures */
	int		fst_nlct;	/* 0x124 number of lct structures */
	struct vx_version fst_iversion;	/* 0x128 next incr version number */
#ifdef	_KERNEL
	sleep_t		*fst_dirlock_slkp; /* 0x130 directory lock */
	fspin_t		fst_imaplist_flk; /* 0x134 inode map lists lock */
	fspin_t		fst_curused_flk; /* 0x138 curused and vversion lock */
	fspin_t		fst_lctlist_flk; /* 0x13c lct lists lock */
#endif	/* _KERNEL */
};					/* 0x140 size of structure */

#define	fst_fs		fst_fsstruct->fss_fsp

#define	fst_lversion	fst_cversion.vv_lversion
#define	fst_hversion	fst_cversion.vv_hversion
#define	fst_dlversion	fst_dversion.vv_lversion
#define	fst_dhversion	fst_dversion.vv_hversion
#define	fst_ilversion	fst_iversion.vv_lversion
#define	fst_ihversion	fst_iversion.vv_hversion

#define	fst_rdonly	fst_mountinfo.fmi_rdonly
#define	fst_blkclear	fst_mountinfo.fmi_blkclear
#define	fst_delaylog	fst_mountinfo.fmi_delaylog
#define	fst_tmplog	fst_mountinfo.fmi_tmplog
#define	fst_nolog	fst_mountinfo.fmi_nolog
#define	fst_logwrites	fst_mountinfo.fmi_logwrites
#define	fst_mounted	fst_mountinfo.fmi_mounted
#define	fst_mincache	fst_mountinfo.fmi_mincache
#define	fst_convosync	fst_mountinfo.fmi_convosync
#define	fst_mntpt	fst_mountinfo.fmi_mntpt
#define	fst_mntlen	fst_mountinfo.fmi_mntlen
#define	fst_loglevel	fst_mountinfo.fmi_loglevel

/*
 * Add/remove a file set to the fs_fileset list.
 */

#define	VX_FSFSETADD(fsethd, fset) { \
	(fset)->fst_next = (fsethd); \
	(fset)->fst_prev = (fsethd)->fst_prev; \
	(fsethd)->fst_prev = (fset); \
	(fset)->fst_prev->fst_next = (fset); \
}

#define	VX_FSFSETREM(fset) { \
	(fset)->fst_next->fst_prev = (fset)->fst_prev; \
	(fset)->fst_prev->fst_next = (fset)->fst_next; \
	(fset)->fst_prev = (fset); \
	(fset)->fst_next = (fset); \
}

/*
 * directory add or delete inhibit
 */

#define	VX_DIRLOCK(fset) {						\
	XTED_DIRLOCK1(fset);						\
	SLEEP_LOCK((fset)->fst_dirlock_slkp, PRIVFS);			\
	XTED_DIRLOCK2(fset);						\
}

#define	VX_DIRLOCK_TRY(fset)		vx_dirlock_try(fset)

#define	VX_DIRUNLOCK(fset) {						\
	XTED_DIRUNLOCK(fset);						\
	SLEEP_UNLOCK((fset)->fst_dirlock_slkp);				\
}

/*
 * The incore current usage table structure.
 */

struct vx_cut {
	struct vx_mlinkhd	cu_mlink;	/* 0x00 current mlinks */
	struct vx_mlinkhd	cu_flink;	/* 0x08 mlinks being flushed */
	struct vx_fsstruct	*cu_fsstruct;	/* 0x10 fs structure */
	struct vx_inode		*cu_cutip;	/* 0x14 inode containing cut */
	int			cu_size;	/* 0x18 size of cu_buffer */
	struct vx_cuent		*cu_cuent;	/* 0x1c memory copy of cut */
	struct vx_cuent		*cu_dskent;	/* 0x20 disk copy of cut */
	int			cu_bad;		/* 0x24 bad flag */
	int			cu_iocount;	/* 0x28 count of active I/Os */
	int			cu_error;	/* 0x2c error on I/O */
	int			cu_done;	/* 0x30 cut I/O done */
	int			cu_nmlink;	/* 0x34 num of mlinks on cut */
	daddr_t			cu_blkno;	/* 0x38 blkno of cut */
#ifdef	_KERNEL
	lock_t			*cu_cut_lkp;	/* 0x3c cut spin lock */
	sleep_t			*cu_lock_slkp;	/* 0x40 cut sleep lock */
	event_t			*cu_iodone_evp;	/* 0x44 iodone event variable */
#endif	/* _KERNEL */
};

/*
 * The incore link count table structure.
 */

struct vx_lct {
	struct vx_mlinkhd	lc_mlink;	/* 0x00 current mlinks */
	struct vx_mlinkhd	lc_flink;	/* 0x08 mlinks being flushed */
	struct vx_fset		*lc_fset;	/* 0x10 file set pointer */
	struct buf		*lc_bp;		/* 0x14 buffer pointer */
	union vx_lctdisk	*lc_logcopy;	/* 0x18 log copy of lct */
	daddr_t			lc_blkno;	/* 0x1c block number */
	int			lc_index;	/* 0x20 index number */
	int			lc_holdcnt;	/* 0x24 hold count */
	int			lc_bad;		/* 0x28 bad flag */
	int			lc_cloned;	/* 0x2c clone upd in progress */
	int			lc_error;	/* 0x30 error occurred */
	int			lc_done;	/* 0x34 clone done */
#ifdef	_KERNEL
	event_t			*lc_iodone_evp;	/* 0x38 iodone event variable */
	sleep_t			*lc_lock_slkp;	/* 0x3c lct lock */
#endif	/* _KERNEL */
};

#define	VX_NEFREE	32	/* number of entries in free extent array */

/*
 * root and lost+found inode numbers
 */

#define	VX_ROOTINO	((ino_t)2)	/* i number of all roots */
#define	LOSTFOUNDINO	(VX_ROOTINO + 1)

/*
 * Magic Number and Version
 */

#define	VX_MAGIC		0xa501fcf5
#define	VX_VERSION		1
#define	VX_AUMAGIC		0xa502fcf5
#define	VX_VERSION2		2

#define	VX_SNAPMAGIC		0xa501fcf6
#define	VX_SNAPVERSION		1
#define	VX_SNAPVERSION2		2

/*
 * The log format version number for this kernel.
 */

#define	VX_LOGVERSION	1

/*
 *  values for fs_flags
 */

#define	VX_FLAGSMASK	0x03ff		/* mask for all values */
#define	VX_FULLFSCK	0x0001		/* full fsck required */
#define	VX_LOGBAD	0x0002		/* log is invalid, don't do replay */
#define	VX_NOLOG	0x0004		/* no logging, don't do replay */
#define	VX_RESIZE	0x0008		/* resize in progress */
#define	VX_LOGRESET	0x0010		/* log reset desired */
#define	VX_UPGRADING	0x0020		/* upgrade in progress */
#define	VX_METAIOERR	0x0100		/* file system meta-data i/o error */
#define	VX_DATAIOERR	0x0200		/* file data i/o error */

/*
 * values for fs_clean
 */

#define	VX_CLEAN	0x5a		/* file system is clean */
#define	VX_DIRTY	0x3c		/* file system is active */

/*
 * File system structure.  The first portion of this structure is
 * the super-block contents on disk. The tail portion is only
 * used in memory.
 */

/*
 * Read-only portion of super-block.  This information is set by
 * mkfs, and only changed for a resize or online upgrade.
 */

struct vx_fscommon {

	/*
	 * validation and dating info
	 */

	long	fsc_magic;		/* 0x00 magic number */
	long	fsc_version;		/* 0x04 version number */
	time_t	fsc_ctime;		/* 0x08 create time */
	time_t	fsc_ectime;		/* 0x0c spare for eft */

	/*
	 * fundamental sizes and offsets
	 */

	long	fsc_obsolete1;		/* 0x10 currently unused */
	long	fsc_obsolete2;		/* 0x14 currently unused */
	daddr_t	fsc_logstart;		/* 0x18 addr of first log block */
	daddr_t	fsc_logend;		/* 0x1c addr of last log block */
	long	fsc_bsize;		/* 0x20 size of blocks in fs */
	long	fsc_size;		/* 0x24 number of blocks in fs */
	long	fsc_dsize;		/* 0x28 number of data blocks in fs */
	ulong	fsc_ninode;		/* 0x2c number of inodes in fs */
	long	fsc_nau;		/* 0x30 number of au's in fs */
	long	fsc_obsolete3;		/* 0x34 currently unused */
	long	fsc_defiextsize;	/* 0x38 default indirect ext size */
	long	fsc_oilbsize;		/* 0x3c old ilist block size in bytes */
	long	fsc_immedlen;		/* 0x40 size of immediate data area */
	long	fsc_ndaddr;		/* 0x44 num of dir exts per inode */

	/*
	 * derived offsets
	 */

	daddr_t	fsc_aufirst;		/* 0x48 addr of first au */
	daddr_t	fsc_emap;		/* 0x4c offset of extent map in au */
	daddr_t	fsc_imap;		/* 0x50 offset of inode map in au */
	daddr_t	fsc_iextop;		/* 0x54 offset of extop map in au */
	daddr_t	fsc_istart;		/* 0x58 offset of inode list in au */
	daddr_t	fsc_bstart;		/* 0x5c offset of data block in au*/
	daddr_t	fsc_femap;		/* 0x60 fs_aufirst + fs_emap */
	daddr_t	fsc_fimap;		/* 0x64 fs_aufirst + fs_imap */
	daddr_t	fsc_fiextop;		/* 0x68 fs_aufirst + fs_iextop */
	daddr_t	fsc_fistart;		/* 0x6c fs_aufirst + fs_istart */
	daddr_t	fsc_fbstart;		/* 0x70 fs_aufirst + fs_bstart */

	/*
	 * derived lengths and sizes
	 */

	long	fsc_nindir;		/* 0x74 number of entries in indir */
	long	fsc_aulen;		/* 0x78 length of au in blocks */
	long	fsc_auimlen;		/* 0x7c length of au imap in blocks */
	long	fsc_auemlen;		/* 0x80 length of au emap in blocks */
	long	fsc_auilen;		/* 0x84 length of au ilist in blocks */
 	long	fsc_aupad;		/* 0x88 length of au pad in blocks */
 	long	fsc_aublocks;		/* 0x8c number data blocks in an au */
 	long	fsc_maxtier;		/* 0x90 log base 2 of fs_aublocks */
	long	fsc_inopb;		/* 0x94 number of inodes per block */
	long	fsc_inopau;		/* 0x98 number of inodes per au */
	long	fsc_obsolete4;		/* 0x9c currently unused */
	long	fsc_ndiripau;		/* 0xa0 num of directory ino per au */
	long	fsc_iaddrlen;		/* 0xa4 sz of ind addr ext in blks */

	/*
	 * derived shift values
	 */

	long	fsc_bshift;		/* 0xa8 log base 2 of fs_bsize */
	long	fsc_inoshift;		/* 0xac log base 2 of fs_inopb */

	/*
	 * derived masks
	 */

	ulong	fsc_bmask;		/* 0xb0 ~(fs_bsize - 1) */
	ulong	fsc_boffmask;		/* 0xb4 fs_bsize - 1 */
	long	fsc_obsolete5;		/* 0xb8 currently unused */

	/*
	 *  Checksum of all invariant fields (see FS_CHECKSUM)
	 */

	long	fsc_checksum;		/* 0xbc checksum */
};

/*
 * writable portion of super-block
 */

struct vx_fswrite {

	/*
	 * free resources
	 */

	long	fsw_free;		/* 0x00 number of free blocks */
	long	fsw_ifree;		/* 0x04 number of free inodes */
	long	fsw_efree[VX_NEFREE];	/* 0x08 num. of free extents by size */

	/*
	 * modification and state flags
	 */

	long	fsw_flags;		/* 0x88 flags */
	char	fsw_mod;		/* 0x8c file system has been changed */
	char	fsw_clean;		/* 0x8d file system is clean */
	short	fsw_reserved1;		/* 0x8e reserved space */
	ulong	fsw_firstlogid;		/* 0x90 mount time log id */

#ifdef	_KERNEL
	timestruc_t	fsw_time;	/* 0x94 time last written */
#else
	time_t	fsw_time;		/* 0x94 time last written */
	long	fsw_etime;		/* 0x98 spare for eft */
#endif

	/*
	 *  labels
	 */

	char	fsw_fname[6];		/* 0x9c file system name */
	char	fsw_fpack[6];		/* 0xa2 file system pack name */
	long	fsw_logversion;		/* 0xa8 log format version number */
	long	fsw_reserved2;		/* 0xac reserved space */
};					/* 0xb0 total */

/*
 * version 2 layout super-block fields
 */

struct vx_fscommon2 {
	daddr_t	fsd_oltext[2];		/* 0x00 olt extent and replica */
	long	fsd_oltsize;		/* 0x08 size of olt extent */
	long	fsd_iauimlen;		/* 0x0c size of inode maps */
	long	fsd_iausize;		/* 0x10 size of iau in blocks */
	long	fsd_dinosize;		/* 0x14 size of inode in bytes */
	long	fsd_reserved3;		/* 0x18 reserved for future expansion */
	long	fsd_checksum2;		/* 0x1c checksum of fscommon2 fields */
};					/* 0x20 total */

#ifdef	_KERNEL

/*
 *  Memory only super-block fields.  This structure is copied over
 *  during remount, resize, and upgrade.  It can't contain any pointers.
 */

struct vx_fsmem {
	long	fsm_ltop;		/* 0x00 logical to physical shift */
	int	fsm_iosize;		/* 0x04 device sector size */
	int	fsm_ioshift;		/* 0x08 log base 2 of fs_iosize */
	int	fsm_iomask;		/* 0x0c fs_iosize - 1 */
	int	fsm_sblkno;		/* 0x10 block offset of super-block */
	int	fsm_sblklen;		/* 0x14 len in blocks of super-block */
	int	fsm_sblkoff;		/* 0x18 offset in buf of super-block */
	int	fsm_blkpsg;		/* 0x1c blocks per segment */
	int	fsm_blkppg;		/* 0x20 blocks per page */
	ulong	fsm_logid;		/* 0x24 log id */
	short	fsm_logbusy;		/* 0x28 log is being written */
	short	fsm_logactive;		/* 0x2a serialize I/O with flogactive */
	short	fsm_flogactive;		/* 0x2c first log block active */
	short	fsm_logalloc;		/* 0x2e process allocating log buffer */
	daddr_t	fsm_logbegin;		/* 0x30 beginning of log */
	int	fsm_lbleft;		/* 0x34 space left in log buffer */
	long	fsm_logoff;		/* 0x38 absolute position of log */
	int	fsm_tranmax;		/* 0x3c max transaction reservation */
	int	fsm_tranleft;		/* 0x40 available reservation */
	int	fsm_ntran;		/* 0x44 number of transactions on fs */
	int	fsm_replaytran;		/* 0x48 trans for replay if crash */
	int	fsm_tranlow;		/* 0x4c flush point in log */
	int	fsm_tranmed;		/* 0x50 flush point in log */
	int	fsm_timeout;		/* 0x54 timeout id for freeze */
	long	fsm_dflags;		/* 0x58 flags confirmed on disk */
	int	fsm_ainopau;		/* 0x5c actual ino per au */
	int	fsm_logbad;		/* 0x60 log bad flag */
	int	fsm_allrdonly;		/* 0x64 all file sets readonly */
	int	fsm_busy;		/* 0x68 fs busy since last sync */
	int	fsm_disabled;		/* 0x6c file system disabled */
	int	fsm_upgrade;		/* 0x70 upgrade in progress */
	ulong	fsm_nextindex;		/* 0x74 next fset index to use */
	long	fsm_minfree;		/* 0x78 minimum desired free blocks */
	struct vx_version fsm_vversion;	/* 0x7c fset version for upgrade */
	long	fsm_aufree[5];		/* 0x84 low desired free per au */
	long	fsm_lastaufree[5];	/* 0x98 low desired free in last au */
	long	fsm_ilbsize;		/* 0xac size to read/write inode list */
	long	fsm_inosize;		/* 0xb0 size of an inode */
	long	fsm_inotobyte;		/* 0xb4 log base 2 of fs_inosize */
	long	fsm_inopilb;		/* 0xb8 fs_ilbsize / fs_inosize */
	ulong	fsm_inomask;		/* 0xbc mask of (fs_inopilb - 1) */
	ulong	fsm_lastid;		/* 0xc0 last tran id logged to disk */
	ulong	fsm_curid;		/* 0xc4 first tran id in fs_curlb */
					/* 0xc8 is length */
};

/*
 * Super-block fields related to file system structure.
 */

struct vx_fsstruct {
	struct vx_fs	*fss_fsp;	/* 0x00 pointer to fs structure */
	int		fss_nfileset;	/* 0x04 number of file sets */
	struct vx_fsetlink fss_fileset;	/* 0x08 list of file sets */
	struct vx_map	**fss_emaplist;	/* 0x10 list of extent map pointers */
	struct vx_ausum	**fss_ausum;	/* 0x14 list of au summary pointers */
	struct vx_cut	*fss_cutp;	/* 0x18 current usage table pointer */
	struct vx_fset	*fss_attrfset;	/* 0x1c pointer to attribute file set */
	struct vx_inode	*fss_fsetip[2];	/* 0x20 files containing fset headers */
					/* 0x28 is length */
};

/*
 * This is the stable portion of the super-block.  When a remount,
 * upgrade, or resize changes the file system structure, this portion
 * doesn't move or change.  The inodes and vx_vfs structures can point
 * at this to reference the file system they are associated with.  All
 * the structural information can be swapped underneath this structure.
 */

struct vx_fsnocopy {
	int	fsn_freezelvl;		/* 0x00 level freeze has locked */
	int	fsn_freeze;		/* 0x04 freeze lock flags */
	struct vx_cpuinfo *fsn_cpuinfo; /* 0x08 cpu specific data */
	ulong	fsn_logbuftime;		/* 0x0c time last log buffer alloced */
	struct vx_fs *fsn_agnext;	/* 0x10 next aggregate pointer */
	struct vx_fs *fsn_agprev;	/* 0x14 previous aggregate pointer */
	struct vnode *fsn_devvp;	/* 0x18 block device vnode for I/O */
	dev_t	fsn_dev;		/* 0x1c device number fs on */
	caddr_t	fsn_aggrname;		/* 0x20 aggregate name */
	int	fsn_agnmlen;		/* 0x24 length of aggregate name */
	caddr_t	fsn_devname;		/* 0x28 device name */
	int	fsn_devnmlen;		/* 0x2c length of device name */
	int	fsn_nmounts;		/* 0x30 number of mounted file sets */
	caddr_t	fsn_flogbufp;		/* 0x34 buffer for first log block */ 
	struct buf *fsn_superbuf;	/* 0x38 buffer for super-block */
	struct vx_fs *fsn_snapfs;	/* 0x3c list of snapshots */
	struct vx_snap *fsn_snapp;	/* 0x40 pointer to snap structure */
	struct buf *fsn_copybp;		/* 0x44 snapshot copy buffer */
	struct vx_tran	*fsn_tdoneq;	/* 0x48 completed transaction queue */
	struct vx_tran	*fsn_tundoq;	/* 0x4c trans waiting for undo */
	struct vx_tran	*fsn_acttranq;	/* 0x50 active trans queue head */
	struct vx_logbuf *fsn_actlb;	/* 0x54 active log buffers */
	struct vx_logbuf *fsn_curlb;	/* 0x58 current log buffer */
	struct vx_logbuf *fsn_lactlb;	/* 0x5c tail of active log buffers */
	struct vx_fset	*fsn_dummyfset;	/* 0x60 dummy file set for mounting */
	struct vx_mlink	fsn_mlink;	/* 0x64 head of async update queue */
	sleep_t	*fsn_ausum_slkp;	/* 0x94 au summary lock */
	lock_t	*fsn_tranq_lkp;		/* 0x98 transaction queue lock */
	lock_t	*fsn_fsq_lkp;		/* 0x9c file system queue lock */
	fspin_t	fsn_active_flk;		/* 0xa0 active level fast lock */
	lock_t	*fsn_curlb_lkp;		/* 0xa4 current log buffer lock */
	sv_t	*fsn_curlb_svp;		/* 0xa8 logbuf i/o signal variable */
	sv_t	*fsn_curlbmem_svp;	/* 0xac logbuf alloc signal variable */
	sv_t	*fsn_tranlog_svp;	/* 0xb0 tran logged signal var */
	sleep_t *fsn_logclean_slkp;	/* 0xb4 prevent copies for umount */
	sleep_t	*fsn_sblock_slkp;	/* 0xb8 super-block lock */
	rwsleep_t fsn_copylock;		/* 0xbc prevent copies for umount */
	char	fsn_dataioerr;		/* 0x?? set on file data i/o error */
	char	fsn_metaioerr;		/* 0x?? set on meta-data i/o error */
};					/* 0x??? total */

/*
 * disk based portion of super-block
 */

struct vx_fsdisk {
	struct vx_fscommon	fs_c;	/* 0x000 fixed portion */
	struct vx_fswrite	fs_w;	/* 0x0c0 writable portion */
	struct vx_fscommon2	fs_c2;	/* 0x170 version 2 portion */
					/* 0x190 is length */
};

/*
 * super-block structure
 */

struct vx_fs {
	struct vx_fsstruct	*fs_struct;	/* 0x000 structural portion */
	struct vx_fsdisk	fs_dsk;		/* 0x004 disk portion */
	struct vx_fsmem		fs_mem;		/* 0x194 memory only portion */
	struct vx_fsnocopy	fs_ncpy;	/* 0x25c non copyable */
					 	/* 0x??? is length */
};

#define	fs_magic	fs_dsk.fs_c.fsc_magic
#define	fs_version	fs_dsk.fs_c.fsc_version
#define	fs_ctime	fs_dsk.fs_c.fsc_ctime
#define	fs_ectime	fs_dsk.fs_c.fsc_ectime
#define	fs_obsolete1	fs_dsk.fs_c.fsc_obsolete1
#define	fs_obsolete2	fs_dsk.fs_c.fsc_obsolete2
#define	fs_logstart	fs_dsk.fs_c.fsc_logstart
#define	fs_logend	fs_dsk.fs_c.fsc_logend
#define	fs_bsize	fs_dsk.fs_c.fsc_bsize
#define	fs_size		fs_dsk.fs_c.fsc_size
#define	fs_dsize	fs_dsk.fs_c.fsc_dsize
#define	fs_ninode	fs_dsk.fs_c.fsc_ninode
#define	fs_nau		fs_dsk.fs_c.fsc_nau
#define	fs_obsolete3	fs_dsk.fs_c.fsc_obsolete3
#define	fs_defiextsize	fs_dsk.fs_c.fsc_defiextsize
#define	fs_oilbsize	fs_dsk.fs_c.fsc_oilbsize
#define	fs_immedlen	fs_dsk.fs_c.fsc_immedlen
#define	fs_ndaddr	fs_dsk.fs_c.fsc_ndaddr
#define	fs_aufirst	fs_dsk.fs_c.fsc_aufirst
#define	fs_emap		fs_dsk.fs_c.fsc_emap
#define	fs_imap		fs_dsk.fs_c.fsc_imap
#define	fs_iextop	fs_dsk.fs_c.fsc_iextop
#define	fs_istart	fs_dsk.fs_c.fsc_istart
#define	fs_bstart	fs_dsk.fs_c.fsc_bstart
#define	fs_femap	fs_dsk.fs_c.fsc_femap
#define	fs_fimap	fs_dsk.fs_c.fsc_fimap
#define	fs_fiextop	fs_dsk.fs_c.fsc_fiextop
#define	fs_fistart	fs_dsk.fs_c.fsc_fistart
#define	fs_fbstart	fs_dsk.fs_c.fsc_fbstart
#define	fs_nindir	fs_dsk.fs_c.fsc_nindir
#define	fs_aulen	fs_dsk.fs_c.fsc_aulen
#define	fs_auimlen	fs_dsk.fs_c.fsc_auimlen
#define	fs_auemlen	fs_dsk.fs_c.fsc_auemlen
#define	fs_auilen	fs_dsk.fs_c.fsc_auilen
#define	fs_aupad	fs_dsk.fs_c.fsc_aupad
#define	fs_aublocks	fs_dsk.fs_c.fsc_aublocks
#define	fs_maxtier	fs_dsk.fs_c.fsc_maxtier
#define	fs_inopb	fs_dsk.fs_c.fsc_inopb
#define	fs_inopau	fs_dsk.fs_c.fsc_inopau
#define	fs_obsolete4	fs_dsk.fs_c.fsc_obsolete4
#define	fs_ndiripau	fs_dsk.fs_c.fsc_ndiripau
#define	fs_iaddrlen	fs_dsk.fs_c.fsc_iaddrlen
#define	fs_bshift	fs_dsk.fs_c.fsc_bshift
#define	fs_inoshift	fs_dsk.fs_c.fsc_inoshift
#define	fs_bmask	fs_dsk.fs_c.fsc_bmask
#define	fs_boffmask	fs_dsk.fs_c.fsc_boffmask
#define	fs_obsolete5	fs_dsk.fs_c.fsc_obsolete5
#define	fs_checksum	fs_dsk.fs_c.fsc_checksum

#define	fs_free		fs_dsk.fs_w.fsw_free
#define	fs_ifree	fs_dsk.fs_w.fsw_ifree
#define	fs_efree	fs_dsk.fs_w.fsw_efree
#define	fs_flags	fs_dsk.fs_w.fsw_flags
#define	fs_mod		fs_dsk.fs_w.fsw_mod
#define	fs_clean	fs_dsk.fs_w.fsw_clean
#define	fs_firstlogid	fs_dsk.fs_w.fsw_firstlogid
#define	fs_time		fs_dsk.fs_w.fsw_time
#define	fs_fpack	fs_dsk.fs_w.fsw_fpack
#define	fs_fname	fs_dsk.fs_w.fsw_fname
#define	fs_logversion	fs_dsk.fs_w.fsw_logversion

#define	fs_oltext	fs_dsk.fs_c2.fsd_oltext
#define	fs_oltsize	fs_dsk.fs_c2.fsd_oltsize
#define	fs_iauimlen	fs_dsk.fs_c2.fsd_iauimlen
#define	fs_iausize	fs_dsk.fs_c2.fsd_iausize
#define	fs_dinosize	fs_dsk.fs_c2.fsd_dinosize
#define	fs_reserved3	fs_dsk.fs_c2.fsd_reserved3
#define	fs_checksum2	fs_dsk.fs_c2.fsd_checksum2

#define fs_ltop		fs_mem.fsm_ltop
#define fs_iosize	fs_mem.fsm_iosize
#define fs_ioshift	fs_mem.fsm_ioshift
#define fs_iomask	fs_mem.fsm_iomask
#define fs_sblkno	fs_mem.fsm_sblkno
#define fs_sblklen	fs_mem.fsm_sblklen
#define fs_sblkoff	fs_mem.fsm_sblkoff
#define fs_blkpsg	fs_mem.fsm_blkpsg
#define fs_blkppg	fs_mem.fsm_blkppg
#define fs_logid	fs_mem.fsm_logid
#define fs_logbusy	fs_mem.fsm_logbusy
#define fs_logactive	fs_mem.fsm_logactive
#define fs_flogactive	fs_mem.fsm_flogactive
#define fs_logalloc	fs_mem.fsm_logalloc
#define fs_logbegin	fs_mem.fsm_logbegin
#define fs_lbleft	fs_mem.fsm_lbleft
#define fs_logoff	fs_mem.fsm_logoff
#define fs_tranmax	fs_mem.fsm_tranmax
#define fs_tranleft	fs_mem.fsm_tranleft
#define fs_ntran	fs_mem.fsm_ntran
#define fs_replaytran	fs_mem.fsm_replaytran
#define fs_tranlow	fs_mem.fsm_tranlow
#define fs_tranmed	fs_mem.fsm_tranmed
#define fs_timeout	fs_mem.fsm_timeout
#define fs_dflags	fs_mem.fsm_dflags
#define fs_ainopau	fs_mem.fsm_ainopau
#define fs_logbad	fs_mem.fsm_logbad
#define fs_allrdonly	fs_mem.fsm_allrdonly
#define fs_busy		fs_mem.fsm_busy
#define fs_disabled	fs_mem.fsm_disabled
#define fs_upgrade	fs_mem.fsm_upgrade
#define fs_nextindex	fs_mem.fsm_nextindex
#define fs_minfree	fs_mem.fsm_minfree
#define fs_vversion	fs_mem.fsm_vversion
#define	fs_hversion	fs_mem.fsm_vversion.vv_hversion
#define	fs_lversion	fs_mem.fsm_vversion.vv_lversion
#define fs_aufree	fs_mem.fsm_aufree
#define fs_lastaufree	fs_mem.fsm_lastaufree
#define fs_ilbsize	fs_mem.fsm_ilbsize
#define fs_inosize	fs_mem.fsm_inosize
#define fs_inotobyte	fs_mem.fsm_inotobyte
#define fs_inopilb	fs_mem.fsm_inopilb
#define fs_inomask	fs_mem.fsm_inomask
#define fs_lastid	fs_mem.fsm_lastid
#define fs_curid	fs_mem.fsm_curid

#define fs_fsp		fs_struct->fss_fsp
#define fs_nfileset	fs_struct->fss_nfileset
#define fs_fileset	fs_struct->fss_fileset
#define fs_emaplist	fs_struct->fss_emaplist
#define fs_ausum	fs_struct->fss_ausum
#define fs_cutp		fs_struct->fss_cutp
#define fs_attrfset	fs_struct->fss_attrfset
#define fs_fsetip	fs_struct->fss_fsetip

#define fs_freezelvl	fs_ncpy.fsn_freezelvl
#define fs_freeze   	fs_ncpy.fsn_freeze
#define fs_cpuinfo	fs_ncpy.fsn_cpuinfo
#define fs_agnext	fs_ncpy.fsn_agnext
#define fs_agprev	fs_ncpy.fsn_agprev
#define fs_aggrname	fs_ncpy.fsn_aggrname
#define fs_agnmlen	fs_ncpy.fsn_agnmlen
#define fs_devname	fs_ncpy.fsn_devname
#define fs_devnmlen	fs_ncpy.fsn_devnmlen
#define fs_nmounts	fs_ncpy.fsn_nmounts
#define fs_flogbufp	fs_ncpy.fsn_flogbufp
#define fs_superbuf	fs_ncpy.fsn_superbuf
#define fs_snapfs	fs_ncpy.fsn_snapfs
#define fs_snapp	fs_ncpy.fsn_snapp
#define fs_copybp	fs_ncpy.fsn_copybp
#define fs_tdoneq	fs_ncpy.fsn_tdoneq
#define fs_tundoq	fs_ncpy.fsn_tundoq
#define fs_acttranq	fs_ncpy.fsn_acttranq
#define fs_actlb	fs_ncpy.fsn_actlb
#define fs_curlb	fs_ncpy.fsn_curlb
#define fs_lactlb	fs_ncpy.fsn_lactlb
#define fs_logbuftime	fs_ncpy.fsn_logbuftime
#define fs_devvp	fs_ncpy.fsn_devvp
#define fs_dev		fs_ncpy.fsn_dev
#define fs_dummyfset	fs_ncpy.fsn_dummyfset
#define fs_mlink	fs_ncpy.fsn_mlink
#define	fs_copylock	fs_ncpy.fsn_copylock
#define	fs_curlb_lkp	fs_ncpy.fsn_curlb_lkp
#define	fs_curlb_svp	fs_ncpy.fsn_curlb_svp
#define	fs_curlbmem_svp	fs_ncpy.fsn_curlbmem_svp
#define	fs_logclean_slkp fs_ncpy.fsn_logclean_slkp 
#define	fs_ausum_slkp	fs_ncpy.fsn_ausum_slkp
#define	fs_tranq_lkp	fs_ncpy.fsn_tranq_lkp
#define	fs_fsq_lkp	fs_ncpy.fsn_fsq_lkp
#define	fs_active_flk	fs_ncpy.fsn_active_flk
#define	fs_logbuf_lkp	fs_ncpy.fsn_logbuf_lkp
#define	fs_logbuf_svp	fs_ncpy.fsn_logbuf_svp
#define	fs_tranlog_svp	fs_ncpy.fsn_tranlog_svp
#define	fs_sblock_slkp	fs_ncpy.fsn_sblock_slkp
#define	fs_freezewait_svp fs_ncpy.fsn_freezewait_svp
#define	fs_metaioerr	fs_ncpy.fsn_metaioerr
#define	fs_dataioerr	fs_ncpy.fsn_dataioerr

#else	/* not _KERNEL */

/*
 * For utilities, don't include the memory only fields.
 */

struct vx_fs {
	struct vx_fscommon	fs_c;	/* 0x000 fixed portion */
	struct vx_fswrite	fs_w;	/* 0x0c0 writable portion */
	struct vx_fscommon2	fs_c2;	/* 0x170 version 2 portion */
					/* 0x190 is length */
};

#define	fs_magic	fs_c.fsc_magic
#define	fs_version	fs_c.fsc_version
#define	fs_ctime	fs_c.fsc_ctime
#define	fs_ectime	fs_c.fsc_ectime
#define	fs_obsolete1	fs_c.fsc_obsolete1
#define	fs_obsolete2	fs_c.fsc_obsolete2
#define	fs_logstart	fs_c.fsc_logstart
#define	fs_logend	fs_c.fsc_logend
#define	fs_bsize	fs_c.fsc_bsize
#define	fs_size		fs_c.fsc_size
#define	fs_dsize	fs_c.fsc_dsize
#define	fs_ninode	fs_c.fsc_ninode
#define	fs_nau		fs_c.fsc_nau
#define	fs_obsolete3	fs_c.fsc_obsolete3
#define	fs_defiextsize	fs_c.fsc_defiextsize
#define	fs_oilbsize	fs_c.fsc_oilbsize
#define	fs_immedlen	fs_c.fsc_immedlen
#define	fs_ndaddr	fs_c.fsc_ndaddr
#define	fs_aufirst	fs_c.fsc_aufirst
#define	fs_emap		fs_c.fsc_emap
#define	fs_imap		fs_c.fsc_imap
#define	fs_iextop	fs_c.fsc_iextop
#define	fs_istart	fs_c.fsc_istart
#define	fs_bstart	fs_c.fsc_bstart
#define	fs_femap	fs_c.fsc_femap
#define	fs_fimap	fs_c.fsc_fimap
#define	fs_fiextop	fs_c.fsc_fiextop
#define	fs_fistart	fs_c.fsc_fistart
#define	fs_fbstart	fs_c.fsc_fbstart
#define	fs_nindir	fs_c.fsc_nindir
#define	fs_aulen	fs_c.fsc_aulen
#define	fs_auimlen	fs_c.fsc_auimlen
#define	fs_auemlen	fs_c.fsc_auemlen
#define	fs_auilen	fs_c.fsc_auilen
#define	fs_aupad	fs_c.fsc_aupad
#define	fs_aublocks	fs_c.fsc_aublocks
#define	fs_maxtier	fs_c.fsc_maxtier
#define	fs_inopb	fs_c.fsc_inopb
#define	fs_inopau	fs_c.fsc_inopau
#define	fs_obsolete4	fs_c.fsc_obsolete4
#define	fs_ndiripau	fs_c.fsc_ndiripau
#define	fs_iaddrlen	fs_c.fsc_iaddrlen
#define	fs_bshift	fs_c.fsc_bshift
#define	fs_inoshift	fs_c.fsc_inoshift
#define	fs_bmask	fs_c.fsc_bmask
#define	fs_boffmask	fs_c.fsc_boffmask
#define	fs_obsolete5	fs_c.fsc_obsolete5
#define	fs_checksum	fs_c.fsc_checksum

#define	fs_free		fs_w.fsw_free
#define	fs_ifree	fs_w.fsw_ifree
#define	fs_efree	fs_w.fsw_efree
#define	fs_flags	fs_w.fsw_flags
#define	fs_mod		fs_w.fsw_mod
#define	fs_clean	fs_w.fsw_clean
#define	fs_firstlogid	fs_w.fsw_firstlogid
#define	fs_time		fs_w.fsw_time
#define	fs_etime	fs_w.fsw_etime
#define	fs_fpack	fs_w.fsw_fpack
#define	fs_fname	fs_w.fsw_fname
#define	fs_logversion	fs_w.fsw_logversion

#define	fs_oltext	fs_c2.fsd_oltext
#define	fs_oltsize	fs_c2.fsd_oltsize
#define	fs_iauimlen	fs_c2.fsd_iauimlen
#define	fs_iausize	fs_c2.fsd_iausize
#define	fs_dinosize	fs_c2.fsd_dinosize
#define	fs_reserved3	fs_c2.fsd_reserved3
#define	fs_checksum2	fs_c2.fsd_checksum2

#endif	/* _KERNEL */

/*
 * allocation unit header
 */

struct vx_auheader {
	long	au_magic;		/* 0x00 magic number */
	long	au_aun;			/* 0x04 nth au */
	struct vx_fscommon au_auxsb;	/* 0x08 copy of super-block common */
	struct vx_fscommon2 au_auxsb2;	/* 0xc8 copy of super-block common2 */
};

/*
 * allocation unit summaries
 */

struct vx_iemapsum {		/* iemap summary */
	long	ausd_iextop;	/* count of iextops enabled */
};

struct vx_imapsum {		/* imap summary */
	long	ausd_difree;	/* free directory inodes */
	long	ausd_dibfree;	/* free directory inode blocks */
	long	ausd_rifree;	/* free regular file inodes */
	long	ausd_ribfree;	/* free regular file inode blocks */
};

struct vx_emapsum {		/* emap summary */
	long	ausd_efree[VX_NEFREE];	/* free extents by size */
};

/*
 * disk format of ausum info
 */

struct vx_ausumd {
	struct vx_iemapsum	ausd_iesum;
	struct vx_imapsum	ausd_isum;
	struct vx_emapsum	ausd_esum;
};

/*
 * The in-memory au summary.
 */

struct vx_ausum {
	struct vx_ausumd aus_cur;	/* 0x00 current summaries */
	int		aus_flag;	/* 0x94 flags */
	long		aus_free;	/* 0x98 free blocks in au */
	long		aus_lastextop;	/* 0x9c aus_extop at last write */
	ino_t		aus_lastino;	/* 0xa0 last inode allocated */
	daddr_t		aus_blkno;	/* 0xa4 block number of summary */
	int		aus_aun;	/* 0xa8 au number */
	struct vx_fset	*aus_fset;	/* 0xac file set pointer */
	struct vx_map	*aus_emap;	/* 0xb0 free extent map pointer */
	struct vx_map	*aus_imap;	/* 0xb4 free inode map pointer */
	struct vx_map	*aus_iemap;	/* 0xb8 inode extop map pointer */
};

#define	VX_AUBADSUM	0x1	/* summary is bad */
#define	VX_AUDIRTYSUM	0x2	/* summary is dirty */
#define	VX_AUCLEANABLE	0x4	/* summary is idle, it can be cleaned */

#define	aus_iextop	aus_cur.ausd_iesum.ausd_iextop
#define	aus_difree	aus_cur.ausd_isum.ausd_difree
#define	aus_dibfree	aus_cur.ausd_isum.ausd_dibfree
#define	aus_rifree	aus_cur.ausd_isum.ausd_rifree
#define	aus_ribfree	aus_cur.ausd_isum.ausd_ribfree
#define	aus_efree	aus_cur.ausd_esum.ausd_efree

#define	ausumd_iextop	ausd_iesum.ausd_iextop
#define	ausumd_difree	ausd_isum.ausd_difree
#define	ausumd_dibfree	ausd_isum.ausd_dibfree
#define	ausumd_rifree	ausd_isum.ausd_rifree
#define	ausumd_ribfree	ausd_isum.ausd_ribfree
#define	ausumd_efree	ausd_esum.ausd_efree

/*
 * calculate checksum of invariant super-block fields in vx_fscommon
 */

#define	VX_FSCHECKSUM(fs)	( \
	(fs)->fs_magic + \
	(fs)->fs_version + \
	(fs)->fs_ctime + \
	(fs)->fs_ectime + \
	(fs)->fs_obsolete1 + \
	(fs)->fs_obsolete2 + \
	(fs)->fs_logstart + \
	(fs)->fs_logend + \
	(fs)->fs_bsize + \
	(fs)->fs_size + \
	(fs)->fs_dsize + \
	(fs)->fs_ninode + \
	(fs)->fs_nau + \
	(fs)->fs_obsolete3 + \
	(fs)->fs_defiextsize + \
	(fs)->fs_oilbsize + \
	(fs)->fs_immedlen + \
	(fs)->fs_ndaddr + \
	(fs)->fs_aufirst + \
	(fs)->fs_emap + \
	(fs)->fs_imap + \
	(fs)->fs_iextop + \
	(fs)->fs_istart + \
	(fs)->fs_bstart + \
	(fs)->fs_femap + \
	(fs)->fs_fimap + \
	(fs)->fs_fiextop + \
	(fs)->fs_fistart + \
	(fs)->fs_fbstart + \
	(fs)->fs_nindir + \
	(fs)->fs_aulen + \
	(fs)->fs_auimlen + \
	(fs)->fs_auemlen + \
	(fs)->fs_auilen + \
	(fs)->fs_aupad + \
	(fs)->fs_aublocks + \
	(fs)->fs_maxtier + \
	(fs)->fs_inopb + \
	(fs)->fs_inopau + \
	(fs)->fs_obsolete4 + \
	(fs)->fs_ndiripau + \
	(fs)->fs_iaddrlen + \
	(fs)->fs_bshift + \
	(fs)->fs_inoshift + \
	(fs)->fs_bmask + \
	(fs)->fs_boffmask + \
	(fs)->fs_obsolete5)

/*
 * calculate checksum of invariant super-block fields in vx_fscommon2
 */

#define	VX_FSCHECKSUM2(fs)	( \
	(fs)->fs_oltext[0] + \
	(fs)->fs_oltext[1] + \
	(fs)->fs_oltsize + \
	(fs)->fs_iausize + \
	(fs)->fs_iauimlen + \
	(fs)->fs_dinosize + \
	(fs)->fs_reserved3)

/*
 * Maximum levels in the freeze locking hierarchy (plus 1 for zero base)
 */

#define	VX_ACTIVE_LEVELS	(7 + 1)

/*
 * File system specific per cpu data.  We pad the structure out to 128
 * bytes to put each instance in a different cache line.
 */

struct vx_cpuinfo {
	int	vcpu_active[VX_ACTIVE_LEVELS];		/* 0x00 active counts */
	int	vcpu_active_check[VX_ACTIVE_LEVELS];	/* 0x20 active counts */
	int	padding[16];				/* 0x40 padding */
							/* 0x80 is length */
};

/*
 * Snapshot structure used to control snapshot file systems.
 */

struct vx_snap {
	struct vx_fs	*sh_snapfs;	/* 0x00 snapshot file system */
	struct vx_fs	*sh_primaryfs;	/* 0x04 primary file system */
	char		*sh_bitmap;	/* 0x08 bitmap of copied blocks */
	daddr_t		sh_blkmap_bno;	/* 0x0c start of block map */
	int		sh_blkmap_bad;	/* 0x10 set if i/o error on block map */
	struct buf	*sh_resbp;	/* 0x14 reserve buffer for block map */
	struct buf	*sh_curbp;	/* 0c18 current, locked, block map bp */
	daddr_t		sh_nextblk;	/* 0x1c next block to be allocated */
	daddr_t		sh_maxblk;	/* 0x20 max blocks in snapshot area */
#ifdef	_KERNEL
	sleep_t		sh_snapmap_lk;	/* 0x24 sleep lock for block map */
	lock_t		sh_snapbit_lk;	/* 0x?? spin lock for bitmap */
	sv_t		sh_snapbit_sig; /* 0x?? signal variable for bitmap */
#endif	/* _KERNEL */
};

/*
 * conversion from logical to physical blocks
 */

#define	FsLTOP(fs, b)	((b) << (fs)->fs_ltop)
#define	FsPTOL(fs, b)	((b) >> (fs)->fs_ltop)

/*
 * conversion from logical or physical to I/O size blocks
 */

#define	FsLTOIO(fs, b)	((b) << ((fs)->fs_bshift - (fs)->fs_ioshift))
#define	FsIOTOL(fs, b)	((b) >> ((fs)->fs_bshift - (fs)->fs_ioshift))

#define	FsPTOIO(b)	((b) >> ((fs)->fs_ioshift - DEV_BSHIFT))
#define	FsIOTOP(b)	((b) << ((fs)->fs_ioshift - DEV_BSHIFT))

/*
 * Superblock size and location
 */

#define	VX_SUPERBOFF	1024
#define	VX_SBDISKSIZE	(sizeof (struct vx_fscommon) + \
			 sizeof (struct vx_fswrite) + \
			 sizeof (struct vx_fscommon2))

/*
 * conversion to au number from block number
 */

#define	VX_DTOAU(fs, b) (((b) - (fs)->fs_aufirst) / (fs)->fs_aulen)

/*
 * Maximum allowed file offset and block offset in a file system
 */

#define	VX_MAXOFF	INT_MAX
#define	VX_MAXBLK(fs)	(((unsigned)VX_MAXOFF + (fs)->fs_boffmask) >> \
			  (fs)->fs_bshift)

/*
 * flag values for freeze locks
 */

#define	VX_FREEZING		0x1
#define	VX_FROZEN		0x2

#ifdef	_KERNEL

/*
 * Locks to support the active/inactive macros and freeze/thaw code.
 */

#define	VX_FACTIVE_LOCK(fs) {						\
	XTED_FACTIVE_LOCK1(fs);						\
	FSPIN_LOCK(&(fs)->fs_active_flk); 				\
	XTED_FACTIVE_LOCK2(fs);						\
}

#define	VX_FACTIVE_UNLOCK(fs) {						\
	XTED_FACTIVE_UNLOCK(fs);					\
	FSPIN_UNLOCK(&(fs)->fs_active_flk); 				\
}

/*
 * active/inactive interface crossing macros
 */

#define VX_TRY_ACTIVE_COMMON(fs, level)	vx_try_active_common((fs), (level))
#define VX_ACTIVE_COMMON(fs, level)	vx_active_common((fs), (level))
#define VX_INACTIVE_COMMON(fs, level)	vx_inactive_common((fs), (level))

#define VX_ACTIVE1(fs)		VX_ACTIVE_COMMON(fs, 1)
#define VX_ACTIVE2(fs)		VX_ACTIVE_COMMON(fs, 2)
#define VX_ACTIVE3(fs)		VX_ACTIVE_COMMON(fs, 3)
#define VX_ACTIVE4(fs)		VX_ACTIVE_COMMON(fs, 4)
#define VX_ACTIVE5(fs)		VX_ACTIVE_COMMON(fs, 5)
#define VX_ACTIVE6(fs)		VX_ACTIVE_COMMON(fs, 6)
#define VX_ACTIVE7(fs)		VX_ACTIVE_COMMON(fs, 7)

#define VX_TRY_ACTIVE1(fs)	VX_TRY_ACTIVE_COMMON(fs, 1)
#define VX_TRY_ACTIVE2(fs)	VX_TRY_ACTIVE_COMMON(fs, 2)
#define VX_TRY_ACTIVE3(fs)	VX_TRY_ACTIVE_COMMON(fs, 3)
#define VX_TRY_ACTIVE4(fs)	VX_TRY_ACTIVE_COMMON(fs, 4)
#define VX_TRY_ACTIVE5(fs)	VX_TRY_ACTIVE_COMMON(fs, 5)
#define VX_TRY_ACTIVE6(fs)	VX_TRY_ACTIVE_COMMON(fs, 6)
#define VX_TRY_ACTIVE7(fs)	VX_TRY_ACTIVE_COMMON(fs, 7)

#define VX_INACTIVE1(fs)	VX_INACTIVE_COMMON(fs, 1)
#define VX_INACTIVE2(fs)	VX_INACTIVE_COMMON(fs, 2)
#define VX_INACTIVE3(fs)	VX_INACTIVE_COMMON(fs, 3)
#define VX_INACTIVE4(fs)	VX_INACTIVE_COMMON(fs, 4)
#define VX_INACTIVE5(fs)	VX_INACTIVE_COMMON(fs, 5)
#define VX_INACTIVE6(fs)	VX_INACTIVE_COMMON(fs, 6)
#define VX_INACTIVE7(fs)	VX_INACTIVE_COMMON(fs, 7)

/*
 * lock super-block
 */

#define	VX_SBLOCK(fs) { \
	XTED_SBLOCK1(fs); \
	SLEEP_LOCK((fs)->fs_sblock_slkp, PRINOD); \
	XTED_SBLOCK2(fs); \
}

#define	VX_SBUNLOCK(fs) { \
	XTED_SBUNLOCK(fs); \
	SLEEP_UNLOCK((fs)->fs_sblock_slkp); \
}

/*
 * AU summary lock (currently a single global lock)
 */

#define	VX_SUM_LOCK(fs, ausp) { \
	XTED_SUM_LOCK(fs, ausp); \
	SLEEP_LOCK((fs)->fs_ausum_slkp, PRINOD); \
}

#define	VX_SUM_UNLOCK(fs, ausp) { \
	XTED_SUM_UNLOCK(fs, ausp); \
	SLEEP_UNLOCK((fs)->fs_ausum_slkp); \
}

#ifndef	TED_
#define	VX_SUM_TRYLOCK(fs, ausp) \
	(SLEEP_TRYLOCK((fs)->fs_ausum_slkp) == B_TRUE)
#else									/*TED_*/
#define	VX_SUM_TRYLOCK(fs, ausp)	xted_sum_trylock(fs, ausp)	/*TED_*/
#endif									/*TED_*/

/*
 * The inode map lists lock.  This locks the arrays of pointers to
 * the inode extents, the inode allocation maps, the inode extended
 * operation maps, and au summaries for file sets on version 2 file
 * systems.  This lock required for lookups through fst_imaplist,
 * fst_iemaplist, fst_ilist, and fst_ausum on a version 2 file system.
 */

#ifndef	TED_
#define VX_IMAPLIST_LOCK(fset) \
	(FSPIN_LOCK(&(fset)->fst_imaplist_flk), INVPL)
#else									/*TED_*/
#define	VX_IMAPLIST_LOCK(fset)		xted_imaplist_lock(fset)	/*TED_*/
#endif									/*TED_*/
#define	VX_IMAPLIST_UNLOCK(fset, ipl) {					\
	XTED_IMAPLIST_UNLOCK(fset, ipl);				\
	FSPIN_UNLOCK(&(fset)->fst_imaplist_flk);			\
}

/*
 * Link count table locks.
 */

#define VX_LCT_LOCK(lct) {				\
	XTED_LCT_LOCK(lct);				\
	SLEEP_LOCK((lct)->lc_lock_slkp, PRINOD);	\
}

#define	VX_LCT_UNLOCK(lct) { \
	XTED_LCT_UNLOCK(lct); \
	SLEEP_UNLOCK((lct)->lc_lock_slkp); \
}

#ifndef	TED_
#define	VX_LCT_TRYLOCK(lct) \
	(SLEEP_TRYLOCK((lct)->lc_lock_slkp) == B_TRUE)
#else									/*TED_*/
#define	VX_LCT_TRYLOCK(lct)	xted_lct_trylock(lct)			/*TED_*/
#endif									/*TED_*/


#ifndef	TED_
#define VX_LCT_LIST_LOCK(fset) \
	(FSPIN_LOCK(&(fset)->fst_lctlist_flk), INVPL)
#else									/*TED_*/
#define	VX_LCT_LIST_LOCK(fset)		xted_lct_list_lock(fset)	/*TED_*/
#endif									/*TED_*/
#define	VX_LCT_LIST_UNLOCK(fset, ipl) {					\
	XTED_LCT_LIST_UNLOCK(fset, ipl);				\
	FSPIN_UNLOCK(&(fset)->fst_lctlist_flk);				\
}

/*
 * The curused lock.  This lock is required for changes to the curused
 * fields of vx_cuent and vx_fset structures.  It also protects updates
 * to cu_vversion fields, fst_dversion, and fst_cversion.
 */

#ifndef	TED_
#define VX_CURUSED_LOCK(fset) \
	(FSPIN_LOCK(&(fset)->fst_curused_flk), INVPL)
#else									/*TED_*/
#define	VX_CURUSED_LOCK(fset)		xted_curused_lock(fset)		/*TED_*/
#endif									/*TED_*/
#define	VX_CURUSED_UNLOCK(fset, ipl) {					\
	XTED_CURUSED_UNLOCK(fset, ipl);					\
	FSPIN_UNLOCK(&(fset)->fst_curused_flk);				\
}

/*
 * Locks for the transaction queues.
 */

#ifndef	TED_
#define	VX_FSQ_LOCK(fs, ipl)	LOCK((fs)->fs_fsq_lkp, PLHI)
#else									/*TED_*/
#define	VX_FSQ_LOCK(fs, ipl)	xted_fsq_lock(fs, ipl)			/*TED_*/
#endif									/*TED_*/
#define	VX_FSQ_UNLOCK(fs, ipl) {					\
	XTED_FSQ_UNLOCK(fs, ipl);					\
	UNLOCK((fs)->fs_fsq_lkp, ipl);					\
}

#ifndef	TED_
#define	VX_TRANQ_LOCK(fs, ipl)	LOCK((fs)->fs_tranq_lkp, PLHI)
#else									/*TED_*/
#define	VX_TRANQ_LOCK(fs, ipl)	xted_tranq_lock(fs, ipl)		/*TED_*/
#endif									/*TED_*/
#define	VX_TRANQ_UNLOCK(fs, ipl) {					\
	XTED_TRANQ_UNLOCK(fs, ipl);					\
	UNLOCK((fs)->fs_tranq_lkp, ipl);				\
}

#define	VX_FSQ_TRANQ_SWITCH(fs, oipl, nipl) \
	vx_fsq_tranq_switch(fs, oipl, nipl)

#define	VX_TRANQ_FSQ_SWITCH(fs, oipl, nipl) \
	vx_tranq_fsq_switch(fs, oipl, nipl)

/*
 * The cut lock.  This locks the cu_error, cu_iocount, cu_done, and
 * cut mlink chains.
 */

#ifndef	TED_
#define VX_CUT_LOCK(cutp, ipl)	LOCK((cutp)->cu_cut_lkp, PLHI)
#else									/*TED_*/
#define	VX_CUT_LOCK(cutp, ipl)	xted_cut_lock(cutp, ipl)		/*TED_*/
#endif									/*TED_*/
#define	VX_CUT_UNLOCK(cutp, ipl) {					\
	XTED_CUT_UNLOCK(cutp, ipl);					\
	UNLOCK((cutp)->cu_cut_lkp, ipl);				\
}

#define VX_CUTSLP_LOCK(cutp) {				\
	XTED_CUTSLP_LOCK(cutp);				\
	SLEEP_LOCK((cutp)->cu_lock_slkp, PRINOD);	\
}

#define	VX_CUTSLP_UNLOCK(cutp) { \
	XTED_CUTSLP_UNLOCK(cutp); \
	SLEEP_UNLOCK((cutp)->cu_lock_slkp); \
}

#ifndef	TED_
#define	VX_CUTSLP_TRYLOCK(cutp) \
	(SLEEP_TRYLOCK((cutp)->cu_lock_slkp) == B_TRUE)
#else									/*TED_*/
#define	VX_CUTSLP_TRYLOCK(cutp)	xted_cutslp_trylock(cutp)		/*TED_*/
#endif									/*TED_*/
/*
 * true if primary file system with active snapshots
 */

#define	VX_SNAPPED(fs)	((fs)->fs_snapfs && !(fs)->fs_snapp)

/*
 * true if snapshot file system
 */

#define	VX_SNAPSHOT(fs)	((fs)->fs_snapp)

/*
 * Strategy routine for almost all i/o
 */

#define	VX_STRATEGY(fs, bp)	vx_snap_strategy((fs), (bp))

/*
 * Stand-alone locks and their lockinfo's.
 */

#define	VX_COPYLOCK_INIT(fs, flag) \
	RWSLEEP_INIT(&(fs)->fs_copylock, 0, &vx_copylock_lkinfo, flag)

#define VX_COPYLOCK(fs, flag) {					\
	if (flag == VX_LOCK_SHARED) {  				\
		RWSLEEP_RDLOCK(&(fs)->fs_copylock, PRINOD);	\
	} else {  						\
		RWSLEEP_WRLOCK(&(fs)->fs_copylock, PRINOD);	\
	}  							\
	XTED_COPYLOCK(fs, flag);				\
}

#define VX_COPYUNLOCK(fs) {					\
	XTED_COPYUNLOCK(fs);					\
	RWSLEEP_UNLOCK(&(fs)->fs_copylock);			\
}


#define	VX_SNAPMAP_LOCK_INIT(shp) \
	SLEEP_INIT(&(shp)->sh_snapmap_lk, 0, &vx_snapmap_lkinfo, KM_NOSLEEP)

#define VX_SNAPMAP_LOCK(shp) {					\
 	SLEEP_LOCK(&(shp)->sh_snapmap_lk, PRINOD);		\
	XTED_SNAPMAP_LOCK(shp);					\
}

#define VX_SNAPMAP_UNLOCK(shp) {				\
	XTED_SNAPMAP_UNLOCK(shp);				\
	vx_snapunlock(shp);					\
	SLEEP_UNLOCK(&(shp)->sh_snapmap_lk);			\
}

#define	VX_SNAPBIT_LOCK_INIT(shp)  {				\
	SV_INIT(&(shp)->sh_snapbit_sig);			\
	LOCK_INIT(&(shp)->sh_snapbit_lk, 0, PLHI, 		\
		  &vx_snapbit_lkinfo, KM_NOSLEEP);		\
}

#ifndef	TED_
#define VX_SNAPBIT_LOCK(shp)  	LOCK(&(shp)->sh_snapbit_lk, PLHI)
#else									/*TED_*/
#define	VX_SNAPBIT_LOCK(shp)	xted_snapbit_lock(shp)			/*TED_*/
#endif									/*TED_*/

#define VX_SNAPBIT_UNLOCK(shp, ipl)  {					\
	XTED_SNAPBIT_UNLOCK(shp, ipl);					\
	UNLOCK(&(shp)->sh_snapbit_lk, ipl);				\
	SV_BROADCAST(&(shp)->sh_snapbit_sig, KM_NOSLEEP);		\
}

#define VX_SNAPBIT_SLEEP(shp)  \
	SV_WAIT(&(shp)->sh_snapbit_sig, PLHI, &(shp)->sh_snapbit_lk)

/*
 * The lock is actually external to VxFS, but i'm tired
 * of writing it out when i want to use it.
 */

#define	VX_VFSLIST_LOCK()	SLEEP_LOCK(&vfslist_lock, PRIVFS)

#define	VX_VFSLIST_UNLOCK()	SLEEP_UNLOCK(&vfslist_lock)

/*
 * Sync processor caches.
 */

#define	VX_CACHE_WRITE_SYNC()	WRITE_SYNC()
#define	VX_CACHE_READ_SYNC()	READ_SYNC()

/*
 * Flags for various locks.  We aquire some locks in shared
 * or exclusive mode.
 */

#define	VX_LOCK_SHARED	1
#define	VX_LOCK_EXCL	2

/*
 * round x upto the the next boundary (bnd) where bnd is a power of 2.
 */

#define	VX_ROUNDUP(x, bnd)	(((x) + (bnd) - 1) & ~((bnd) - 1))

extern int	vx_noprmpt;	/* KS_NOPRMPT flag */
extern int	vx_lite;	/* lite file system flag */
extern int	vx_prefio;	/* preferred I/O size */
extern int	vx_maxbufspace;	/* max kbytes allowed to be held by maps */
extern int	vx_maxtran;	/* max number of transactions desired */
extern int	vx_replay_tran;	/* num of trans when flushing started on fs */

extern int	vx_fstype;		/* file system type; set at boot time */
extern whymountroot_t	vx_rootstate;	/* mount state of root fs */
extern int		vx_rootmflags;	/* mount flags for root fs */

extern atomic_int_t	vx_bufspace;	/* total Kbytes held in map buffers */
extern atomic_int_t	vx_ntran;	/* current number of trans in system */

extern lock_t		*vx_ilist_lkp;	/* inode list lock */

extern lock_t		*vx_sched_lkp;			/* thread sched lock */
extern sv_t		*vx_inactive_thread_svp;	/* thread sched var */
extern sv_t		*vx_logflush_thread_svp;	/* thread sched var */
extern sv_t		*vx_delxwri_thread_svp;		/* thread sched var */
extern sv_t		*vx_attr_thread_svp;		/* thread sched var */

/*
 * Lockinfo's for per-structure locks to share.
 */

extern lkinfo_t		vx_attrop_lkinfo;
extern lkinfo_t		vx_ausum_lkinfo;
extern lkinfo_t		vx_copylock_lkinfo;
extern lkinfo_t		vx_curlb_lkinfo;
extern lkinfo_t		vx_cutlock_lkinfo;
extern lkinfo_t		vx_cutspin_lkinfo;
extern lkinfo_t		vx_dirlock_lkinfo;
extern lkinfo_t		vx_fsq_lkinfo;
extern lkinfo_t		vx_iglock_lkinfo;
extern lkinfo_t		vx_iintrlock_lkinfo;
extern lkinfo_t		vx_ilist_lkinfo;
extern lkinfo_t		vx_ilock_lkinfo;
extern lkinfo_t		vx_iplock_lkinfo;
extern lkinfo_t		vx_iptrs_lkinfo;
extern lkinfo_t		vx_irwlock_lkinfo;
extern lkinfo_t		vx_ispinlock_lkinfo;
extern lkinfo_t		vx_lctlock_lkinfo;
extern lkinfo_t		vx_logclean_lkinfo;
extern lkinfo_t		vx_sblock_lkinfo;
extern lkinfo_t		vx_sched_lkinfo;
extern lkinfo_t		vx_snapbit_lkinfo;
extern lkinfo_t		vx_snapmap_lkinfo;
extern lkinfo_t		vx_tranq_lkinfo;
extern lkinfo_t		vx_upd_lkinfo;

extern int		vx_noversion2;	/* prevent version 2 mounts */

extern struct vx_fs	vx_aglist;	/* list of attached aggregates */

#endif	/*_KERNEL*/

#if defined(__cplusplus)
	}
#endif

/*
 * Defines for various swap map formats.
 */

#define	VX_SWAPMAP_MAGIC	0xa505fcf9	/* Magic number for swap maps */
#define	VX_SWAPMAP_BLOCKMAP	1		/* Block map format */

#endif	/*_FS_VXFS_VX_FS_H */
