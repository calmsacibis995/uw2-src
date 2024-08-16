/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/i386/uts/fs/vxfs/vx_inode.h	2.65 16 Sep 1994 17:36:38 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern-i386:fs/vxfs/vx_inode.h	1.17"

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

#ifndef	_FS_VXFS_VX_INODE_H
#define	_FS_VXFS_VX_INODE_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h>	/* REQUIRED */
#endif

#ifndef _UTIL_KSYNCH_H
#include <util/ksynch.h>	/* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h>	/* REQUIRED */
#endif

#ifndef _FS_VNODE_H
#include <fs/vnode.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */
#include <sys/vnode.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *  setup endian type
 */
#define _VXFS_BIG_ENDIAN	1234
#define _VXFS_LITTLE_ENDIAN	4321
#define _VXFS_BYTE_ORDER	_VXFS_LITTLE_ENDIAN

struct vx_version {
	u_long	vv_version[2];
};

#if _VXFS_BYTE_ORDER == _VXFS_LITTLE_ENDIAN
#define	vv_lversion	vv_version[0]
#define	vv_hversion	vv_version[1]
#endif
#if _VXFS_BYTE_ORDER == _VXFS_BIG_ENDIAN
#define	vv_lversion	vv_version[1]
#define	vv_hversion	vv_version[0]
#endif

/*
 * The disk structure of the link count table.
 */

union vx_lctdisk {
	ulong		lcd_entry;		/* link count table entry */
	struct vx_lctval {
#if _VXFS_BYTE_ORDER == _VXFS_LITTLE_ENDIAN
		uint	lcf_count : 31;		/* count of links to inode */
		uint	lcf_free : 1;		/* flag for inode removal */
#endif
#if _VXFS_BYTE_ORDER == _VXFS_BIG_ENDIAN
		uint	lcf_free : 1;		/* flag for inode removal */
		uint	lcf_count : 31;		/* count of links to inode */
#endif
	} lcd_fields;
};

#define	lcd_free	lcd_fields.lcf_free
#define	lcd_count	lcd_fields.lcf_count

/*
 * The mlink structure links asynchronous updates to transactions.
 * The mlinkhd structure is used as a list header, it must match
 * the start of the mlink structure.
 */

struct vx_mlinkhd {
	struct vx_mlink	*ml_forw;	/* forw link of inode/map list */
	struct vx_mlink	*ml_back;	/* back link of inode/map list */
};

struct vx_mlink {
	struct vx_mlink	*ml_forw;	/* forw link of inode/map list */
	struct vx_mlink	*ml_back;	/* back link of inode/map list */
	struct vx_mlink *ml_fsforw;	/* forw link on fs_mlink queue */
	struct vx_mlink *ml_fsback;	/* back link on fs_mlink queue */
	struct vx_tran	*ml_tranp;	/* pointer to tran for mlink */
	int		ml_flag;	/* flag */
	struct vx_fs	*ml_fs;		/* file system structure */
	union vx_mlspec {
		struct vx_mli {		/* used for VX_MLINODE mlinks */
			struct vx_inode	*mi_ip;	    /* pointer to inode */
		} ml_mli;
		struct vx_mlm {		/* used for VX_MLMAP mlinks */
			struct vx_map	*mm_map;    /* pointer to map */
			struct vx_ctran	*mm_tcp;    /* pointer to ctran */
			struct vx_emtran *mm_emp;   /* pointer to emtran */
		} ml_mlm;
		struct vx_mld {		/* used for VX_MLDATA mlinks */
			struct vx_inode	*md_ip;     /* pointer to inode */
			off_t		md_off;     /* offset of logged write */
			u_int		md_len;     /* length of logged write */
		} ml_mld;
		struct vx_mlbh {	/* used for VX_MLBUFHEAD mlinks */
			struct vx_mlinkhd mh_link;  /* list of mlinks for buf */
			struct vx_inode	*mh_ip;     /* pointer to inode */
			daddr_t		mh_bno;     /* blkno of buffer */
			u_int		mh_len;     /* length of buffer */
		} ml_mlbh;
		struct vx_mlc {		/* used for VX_MLCUT mlinks */
			struct vx_fset	*mc_fset;    /* pointer to fset */
			struct vx_mlink	*mc_link;    /* mlinks for fset */
			struct vx_cut	*mc_cutp;    /* pointer to cut */
			struct vx_version mc_vers;   /* version number logged */
		} ml_mlc;
		struct vx_mll {		/* used for VX_MLLCT mlinks */
			struct vx_lct	*ml_lctp;    /* pointer to lct */
		} ml_mll;
	} ml_ml; 	
};

#define	mli_ip		ml_ml.ml_mli.mi_ip
#define	mlm_map		ml_ml.ml_mlm.mm_map
#define	mlm_tcp		ml_ml.ml_mlm.mm_tcp
#define	mlm_emp		ml_ml.ml_mlm.mm_emp
#define	mld_ip		ml_ml.ml_mld.md_ip
#define	mld_off		ml_ml.ml_mld.md_off
#define	mld_len		ml_ml.ml_mld.md_len
#define	mlbh_link	ml_ml.ml_mlbh.mh_link
#define	mlbh_ip		ml_ml.ml_mlbh.mh_ip
#define	mlbh_bno	ml_ml.ml_mlbh.mh_bno
#define	mlbh_len	ml_ml.ml_mlbh.mh_len
#define	mlc_fset	ml_ml.ml_mlc.mc_fset
#define	mlc_link	ml_ml.ml_mlc.mc_link
#define	mlc_cutp	ml_ml.ml_mlc.mc_cutp
#define	mlc_vers	ml_ml.ml_mlc.mc_vers
#define	mll_lctp	ml_ml.ml_mll.ml_lctp

/*
 * values for ml_flag
 */

#define	VX_MLTYPEMASK	0x007f		/* mask of mlink types */
#define	VX_MLMAP	0x0001		/* mlink is for a map */
#define	VX_MLINODE	0x0002		/* mlink is for an inode */
#define	VX_MLDATA	0x0004		/* mlink is for logged data */
#define	VX_MLBUFFER	0x0008		/* mlink is for async buffer */
#define	VX_MLBUFHEAD	0x0010		/* mlink is header for async buffer */
#define	VX_MLCUT	0x0020		/* mlink is for a cut structure */
#define	VX_MLLCT	0x0040		/* mlink is for an lct structure */
#define	VX_MLAFLUSHED	0x0100		/* async flush attempted on mlink */
#define	VX_MLSFLUSHED	0x0200		/* sync flush attempted on mlink */
#define	VX_MLERROR	0x0400		/* update failed */
#define	VX_MLBFLUSHED	0x0800		/* delbuf flush attempted on mlink */
#define	VX_MLDELAYED	0x1000		/* map changes delayed until log done */
#define	VX_MLFSLIST	0x2000		/* mlink is on fsq list */

/*
 * Add/remove an entry to/from an mlink ml_forw/back list
 */

#define	VX_MLINK_ADD(mlink, nlink) { \
	(nlink)->ml_forw = (mlink)->ml_forw; \
	(nlink)->ml_back = (mlink); \
	(mlink)->ml_forw = (nlink); \
	(nlink)->ml_forw->ml_back = (nlink); \
}

#define	VX_MLINK_REM(mlink) { \
	(mlink)->ml_forw->ml_back = (mlink)->ml_back; \
	(mlink)->ml_back->ml_forw = (mlink)->ml_forw; \
	(mlink)->ml_forw = (mlink); \
	(mlink)->ml_back = (mlink); \
}

#define	VX_MLINK_FSADD(mlink, nlink) { \
	(nlink)->ml_fsforw = (mlink)->ml_fsforw; \
	(nlink)->ml_fsback = (mlink); \
	(mlink)->ml_fsforw = (nlink); \
	(nlink)->ml_fsforw->ml_fsback = (nlink); \
	(nlink)->ml_flag |= VX_MLFSLIST; \
}

#define	VX_MLINK_FSREM(mlink) { \
	(mlink)->ml_fsforw->ml_fsback = (mlink)->ml_fsback; \
	(mlink)->ml_fsback->ml_fsforw = (mlink)->ml_fsforw; \
	(mlink)->ml_fsforw = (mlink); \
	(mlink)->ml_fsback = (mlink); \
	(mlink)->ml_flag &= ~VX_MLFSLIST; \
}

/*
 * Move an mlink chain from one mlinkhd to another.
 */

#define	VX_MLINK_MOVE(ohd, nhd) { \
	TED_ASSERT("f:VX_MLINK_MOVE:1a", 			 /*TED_*/ \
		   (nhd).ml_forw == (struct vx_mlink *)&(nhd) && /*TED_*/ \
		   (nhd).ml_back == (struct vx_mlink *)&(nhd));  /*TED_*/ \
	(nhd) = (ohd); \
	(nhd).ml_forw->ml_back = (struct vx_mlink *)&(nhd); \
	(nhd).ml_back->ml_forw = (struct vx_mlink *)&(nhd); \
	(ohd).ml_forw = (struct vx_mlink *)&(ohd); \
	(ohd).ml_back = (struct vx_mlink *)&(ohd); \
}

#define	NDADDR_N	10		/* direct addresses in inode */
#define	NIMMED_N	96		/* immediate data in inode */
#define	NIADDR		2		/* indirect addresses in inode */

/*
 * flags for ex_flags 
 */

#define	VX_EXF_NEW		0x0001	/* extent is newly allocated */
#define	VX_EXF_EXTEND		0x0002	/* OK to extend previous extent */

struct vx_extent {
	off_t	ex_off;		/* offset of extent in file */
	ulong_t	ex_sz;		/* size of extent in bytes */
	daddr_t ex_st;		/* start block of extent */
	u_int	ex_flags;	/* flags */
};

/*
 * This structure is used to keep counts of free and used inodes
 * for use by sar.
 */

struct vx_fshead {
	struct vx_inode	**f_iptrs;	/* list of inode pointers */
	long		f_nptrs;	/* number of pointers allocated */
	long		f_limit;	/* absolute maximum number of inodes */
	long		f_max;		/* desired maximum number of inodes */
	long		f_slop;		/* diff between f_limit and f_max */
	long		f_curr;		/* number of inodes allocated */
	long		f_inuse;	/* number of inodes being used  */
	long		f_free;		/* number of inodes on freelist */
	long		f_fail;		/* number of failures allocating */
	long		f_isize;	/* size of an inode structure */
	long		f_incr;		/* incremental allocation size */
};

struct vx_hinode {		/* must match struct vx_inode */
	struct vx_inode	*i_forw;
	struct vx_inode	*i_back;
	struct vx_inode	*i_bforw;
	struct vx_inode	*i_bback;
};

/*
 * The head of the various inode free/dirty lists.
 *
 * The offset of av_forw and av_back must match struct vx_inode.
 */

struct vx_ifreehead {			/* must match struct vx_inode */
	struct vx_inode *i_filler[4];	/* 0x00 filler for hash chains */
	struct vx_inode *av_forw;	/* 0x10 freelist chain */
	struct vx_inode *av_back;  	/* 0x14 " */
	int		i_count;   	/* 0x18 inodes on list */
	struct vx_fshead *i_fshead;  	/* 0x1c fshead struct for freelists */
};

struct vx_inode {
	struct vx_inode *i_forw;   /* 0x00 inode hash chain */
	struct vx_inode *i_back;   /* 0x04 " */
	struct vx_inode *i_bforw;  /* 0x08 inode block hash chain */
	struct vx_inode *i_bback;  /* 0x0c " */
	struct vx_inode *av_forw;  /* 0x10 freelist chain */
	struct vx_inode *av_back;  /* 0x14 " */
 	struct vx_mlinkhd i_blink; /* 0x18 head of buffer mlinks */
 	struct vx_mlinkhd i_dlink; /* 0x20 head of logged write mlinks */
 	struct vx_mlinkhd i_tlink; /* 0x28 head of logged write trans */
 	struct vx_mlinkhd i_mlink; /* 0x30 head of inode modification mlinks */
	struct vx_mlinkhd i_alink; /* 0x38 async iupdate mlinks */
	struct vx_inode *i_aupd;   /* 0x40 async iupdate linked list */
	u_int	i_flag;		/* 0x44 flags - see below */
	u_int	i_sflag;	/* 0x48 flags protected by inode spin lock */
	u_int	i_intrflag;	/* 0x4c flags touched at interrupt time */
	off_t	i_nsize;	/* 0x50 "new" bytes in file for IDELXWRI */
	off_t	i_wsize;	/* 0x54 size of file if write suceeds */
	off_t	i_dsize;	/* 0x58 max possible inode size on disk */
	off_t	i_errsize;	/* 0x5c off of lowest failing delxwri page */
	long	i_ndblocks;	/* 0x60 number of blocks in direct extents */
	struct vx_fset *i_fset;	/* 0x64 fileset associated with this inode */
	struct vx_fs   *i_fs;	/* 0x68 aggregate associated with this inode */
	dev_t	i_dev;		/* 0x6c device where inode resides */
	ino_t	i_number;	/* 0x70 i number, 1-to-1 with device address */
	long	i_aun;		/* 0x74 au number */
	daddr_t	i_bno;		/* 0x78 ilist block - in sectors */
	off_t	i_boff;		/* 0x7c offset of disk inode in ilist block */
	off_t	i_diroff;	/* 0x80 offset in dir, where last entry found */
	ino_t	i_lastino;	/* 0x84 last inode allocated into directory */
	clock_t	i_ftime;	/* 0x88 time inode put on freelist */
	struct vx_tran *i_tran;	/* 0x8c transaction for logged writes */
	struct vx_extent i_ext;	/* 0x90 last extent for bmap cache */
	u_long	i_vcode;	/* 0xa0 version code attribute */
	int	i_advise;	/* 0xa4 advisories */
	ulong_t	i_nextr;	/* 0xa8 expected offset of next read */
	ulong_t	i_nextio;	/* 0xac offset of next read ahead I/O point */
	ulong_t	i_raend;	/* 0xb0 offset where read ahead area ends */
	int	i_ralen;	/* 0xb4 length of read ahead area */
	ulong_t	i_nextw;	/* 0xb8 expected offset of next write */
	ulong_t	i_wflush;	/* 0xbc offset of last write flush point */
	ulong_t	i_wseqstart;	/* 0xc0 start of series of sequential writes */
	int	i_wlen;		/* 0xc4 len of writes */
	int	i_pageflushes;	/* 0xc8 blocks of outstanding write flush */
	ulong_t	i_lastflushoff;	/* 0xcc offset of last completed write flush */
	ulong_t	i_nextfault;	/* 0xd0 next fault expected on vnode */
	ulong_t	i_getraoff;	/* 0xd4 getpage read ahead offset */
	short	i_nodealloc_fsq;   /* 0xd8 block inode dealloc fsq lock */
	short	i_nmlinks;	/* 0xda number of mlinks on i_mlink chain */
	short	i_nodealloc_ilist; /* 0xdc block inode dealloc ilist lock */
	short	i_inreuse;	/* 0xde block vx_itryhold references */
	int	i_swapcnt;	/* 0xe0 number times used as swap file */
	long	i_mapcnt;	/* 0xe4 number of pages of user mappings */
	short	i_diocount;	/* 0xe8 count of procs using direct I/O */
	short	i_readcnt;	/* 0xea count of procs reading the file */
	long	i_afree;	/* 0xec free space in attribute area */
	caddr_t	i_attr;		/* 0xf0 pointer to attribute area */
	long	i_attrlen;	/* 0xf4 length of attribute area */
	struct vx_ifreehead *i_freelist;/* 0xf8 inode freelist pointer */
	struct vx_fset *i_rfset;     /* 0xfc nonattr fset for attr inodes */
	long    i_aclcnt;	/* 0x100 acl count */
	long    i_daclcnt;	/* 0x104 default acl count */
	long    i_iaclcnt;	/* 0x108 count of acls to inherit */
	long    i_acllink;	/* 0x10c flag for linking directory acls */
	long    i_modemask;	/* 0x110 inheritance permission mask */
	long    i_agen;	 	/* 0x114 access generation count */
	int	i_vinactive;	/* 0x118 inode is delayed inactive */
	int	i_tranid;	/* 0x11c tranid of async logwrite tran */
	struct 	vx_icommon {		/* 0x120 disk inode */
		long	ic_mode;	/*  0x00: mode and type of file */
		long	ic_nlink;	/*  0x04: number of links to file */
		long	ic_uid;		/*  0x08: owner's user id */
		long	ic_gid;		/*  0x0c: owner's group id */
		quad	ic_size;	/*  0x10: "disk" num of bytes in file */
#ifdef	_KERNEL
		timestruc_t ic_atime;	/* 0x18: time last accessed */
		timestruc_t ic_mtime;	/* 0x20: time last modified */
		timestruc_t ic_ctime;	/* 0x28: last time inode changed */
#else
		time_t	ic_atime;	/* 0x18: time last accessed */
		long	ic_atspare;
		time_t	ic_mtime;	/* 0x20: time last modified */
		long	ic_mtspare;
		time_t	ic_ctime;	/* 0x28: last time inode changed */
		long	ic_ctspare;
#endif
		char	ic_aflags;	/* 0x30: allocation flags */
		char	ic_orgtype;	/* 0x31: org type */
		ushort	ic_eopflags;	/* 0x32: extended operations */
		long	ic_eopdata;	/* 0x34: extended operations */
		union vx_ftarea {	/* 0x38: */
			struct vx_ftarea_dev {	    /* device files */
				long	ic_rdev;    /* 0x38: device number */
			} ic_ftarea_dev;
			struct vx_ftarea_dir {	    /* directories */
				long	ic_dotdot;  /* 0x38: parent directory */
			} ic_ftarea_dir;
			struct vx_ftarea_reg {	    /* regular files */
				long	ic_reserve; /* 0x38: prealloc space */
				long	ic_fixextsize; /* 0x3c: fixed ext size*/
			} ic_ftarea_reg;
			struct vx_ftarea_vxspec {   /* vxfs private files */
				long	ic_matchino;  /* 0x38: matching inode */
				ulong	ic_fsetindex; /* 0x3c: fset index */
			} ic_ftarea_vxspec;
		} ic_ftarea;
		long	ic_blocks;	/* 0x40: blocks actually held */
		long	ic_gen;		/* 0x44: generation number */
		struct vx_version ic_vversion;	/* 0x48: inode serial number */
		union vx_org {		/* 0x50: */
			struct vx_immed {
				char ic_immed[NIMMED_N];  /* 0x50 immediate */
			} ic_vx_immed;
			struct vx_ext4 {
				long	ic_spare;	  /* 0x50: unused */
				long	ic_ies;		  /* 0x54: ind ext sz */
				daddr_t	ic_ie[NIADDR];	  /* 0x58: indir exts */
				struct vx_dext {	  /* 0x60: dir exts */
					daddr_t	ic_de;	  /* dir ext */
					long	ic_des;	  /* dir ext size */
				} ic_dext[NDADDR_N];
			} ic_vx_e4;
		} ic_org;
		long	ic_iattrino;	/* 0xb0: indirect attribute inode */
	} i_ic;				/* 0xb4 is length */
#if defined(_KERNEL) || defined(_KMEMUSER)
	struct vnode i_vnode;	/* 0x1d4 vnode associated with this inode */
	lock_t	i_intrlock;	/* 0x228/22c interrupt lock */
	lock_t	i_spinlock;	/* 0x22c/238 inode spin lock */
	sleep_t	i_ilock;	/* 0x230/244 inode lock */
	rwsleep_t i_glock;	/* 0x248/25c getpage lock */
	rwsleep_t i_plock;	/* 0x25c/270 putpage lock */
	rwsleep_t i_rwlock;	/* 0x270/284 read/write lock */
#endif	/* _KERNEL || _KMEMUSER */
};				/* 0x284/298 is len non-debug/debug */

struct vx_dinode {
	struct vx_icommon	di_ic;
};

#define	OLDINOSIZE	256		/* version 1 inode size */
#define	OLDINODESHIFT	8		/* log base 2 of OLDINOSIZE */

#define	i_mode		i_ic.ic_mode
#define	i_nlink		i_ic.ic_nlink
#define	i_uid		i_ic.ic_uid
#define	i_gid		i_ic.ic_gid

#if _VXFS_BYTE_ORDER == _VXFS_LITTLE_ENDIAN
#define	i_size		i_ic.ic_size.val[0]
#define	i_size2		i_ic.ic_size.val[1]
#endif
#if _VXFS_BYTE_ORDER == _VXFS_BIG_ENDIAN
#define	i_size		i_ic.ic_size.val[1]
#define	i_size2		i_ic.ic_size.val[0]
#endif

#define	i_atime		i_ic.ic_atime
#define	i_mtime		i_ic.ic_mtime
#define	i_ctime		i_ic.ic_ctime

#define	i_aflags	i_ic.ic_aflags
#define	i_orgtype	i_ic.ic_orgtype
#define	i_eopflags	i_ic.ic_eopflags
#define	i_eopdata	i_ic.ic_eopdata

#define	i_rdev		i_ic.ic_ftarea.ic_ftarea_dev.ic_rdev
#define	i_dotdot	i_ic.ic_ftarea.ic_ftarea_dir.ic_dotdot
#define	i_fixextsize	i_ic.ic_ftarea.ic_ftarea_reg.ic_fixextsize
#define	i_reserve	i_ic.ic_ftarea.ic_ftarea_reg.ic_reserve
#define	i_matchino	i_ic.ic_ftarea.ic_ftarea_vxspec.ic_matchino
#define	i_fsetindex	i_ic.ic_ftarea.ic_ftarea_vxspec.ic_fsetindex

#define	i_blocks	i_ic.ic_blocks
#define	i_gen		i_ic.ic_gen
#define	i_lversion	i_ic.ic_vversion.vv_lversion
#define	i_hversion	i_ic.ic_vversion.vv_hversion
#define	i_vversion	i_ic.ic_vversion
#define	i_spare		i_ic.ic_spare
#define	i_immed		i_ic.ic_org.ic_vx_immed.ic_immed

#define	i_spare2	i_ic.ic_org.ic_vx_e4.ic_spare
#define	i_ies		i_ic.ic_org.ic_vx_e4.ic_ies
#define	i_ie		i_ic.ic_org.ic_vx_e4.ic_ie
#define	i_dext		i_ic.ic_org.ic_vx_e4.ic_dext

#define	i_iattrino	i_ic.ic_iattrino


#define	di_mode		di_ic.ic_mode
#define	di_nlink	di_ic.ic_nlink
#define	di_uid		di_ic.ic_uid
#define	di_gid		di_ic.ic_gid

#if _VXFS_BYTE_ORDER == _VXFS_LITTLE_ENDIAN
#define	di_size		di_ic.ic_size.val[0]
#define	di_size2	di_ic.ic_size.val[1]
#endif
#if _VXFS_BYTE_ORDER == _VXFS_BIG_ENDIAN
#define	di_size		di_ic.ic_size.val[1]
#define	di_size2	di_ic.ic_size.val[0]
#endif

#define	di_atime	di_ic.ic_atime
#define	di_mtime	di_ic.ic_mtime
#define	di_ctime	di_ic.ic_ctime

#ifndef _KERNEL
#define	di_atspare	di_ic.ic_atspare
#define	di_mtspare	di_ic.ic_mtspare
#define	di_ctspare	di_ic.ic_ctspare
#endif

#define	di_aflags	di_ic.ic_aflags
#define	di_orgtype	di_ic.ic_orgtype
#define	di_eopflags	di_ic.ic_eopflags
#define	di_eopdata	di_ic.ic_eopdata

#define	di_rdev		di_ic.ic_ftarea.ic_ftarea_dev.ic_rdev
#define	di_dotdot	di_ic.ic_ftarea.ic_ftarea_dir.ic_dotdot
#define	di_fixextsize	di_ic.ic_ftarea.ic_ftarea_reg.ic_fixextsize
#define	di_reserve	di_ic.ic_ftarea.ic_ftarea_reg.ic_reserve
#define	di_matchino	di_ic.ic_ftarea.ic_ftarea_vxspec.ic_matchino
#define	di_fsetindex	di_ic.ic_ftarea.ic_ftarea_vxspec.ic_fsetindex

#define	di_blocks	di_ic.ic_blocks
#define	di_gen		di_ic.ic_gen
#define	di_lversion	di_ic.ic_vversion.vv_lversion
#define	di_hversion	di_ic.ic_vversion.vv_hversion
#define	di_vversion	di_ic.ic_vversion
#define	di_spare	di_ic.ic_spare
#define	di_immed	di_ic.ic_org.ic_vx_immed.ic_immed

#define	di_spare2	di_ic.ic_org.ic_vx_e4.ic_spare
#define	di_ies		di_ic.ic_org.ic_vx_e4.ic_ies
#define	di_ie		di_ic.ic_org.ic_vx_e4.ic_ie
#define	di_dext		di_ic.ic_org.ic_vx_e4.ic_dext

#define	di_iattrino	di_ic.ic_iattrino

/*
 * flags in i_flag
 */

#define	IUPD		0x00000001	/* file has been modified */
#define	IACC		0x00000002	/* inode access time to be updated */
#define	IMOD		0x00000004	/* inode has been modified */
#define	ICHG		0x00000008	/* inode has been changed */
#define	IATIMEMOD	0x00000010	/* atime modified */
#define	IMTIMEMOD	0x00000020	/* mtime modified */
#define	ITRANLAZYMOD	0x00000040	/* transaction lazy mod */
#define	IFLUSHPAGES	0x00000080	/* flush pages before removing file */
#define	IDIRTYPAGES	0x00000100	/* inode has non-logged dirty pages */
#define	ICLOSED		0x00000200	/* delxwri file was closed */
#define	IFLUSHED	0x00000400	/* delxwri file was flushed */
#define	ISHORTENED	0x00000800	/* file has been shortened */
#define	ISYNCWRITES	0x00001000	/* file has had O_SYNC writes */
#define	IDELBUF		0x00002000	/* inode has pending delayed buffers */
#define	IGHOST		0x00004000	/* inode is a ghost attribute inode */

/*
 * flags in i_intrflag
 */

#define	IDELXWRI	0x0001		/* inode has delxwri data */
#define	IDELXWRIERR	0x0002		/* delxwri write has failed */
#define	ILOGWRITE	0x0004		/* inode has pending logged writes */
#define	IDELAYEDERR	0x0008		/* delayed update error occurred */
#define	IPUTERROR	0x0010		/* putpage error occurred */
#define	ILOGWRIFLUSH	0x0020		/* logged write flush occurred */
#define	IASYNCUPD	0x0040		/* async inode write in progress */

/*
 * flags in i_sflag
 */

#define	IADDRVALID	0x00000001	/* bmap has validated address map */
#define	IBAD		0x00000002	/* inode is bad */
#define	IUEREAD		0x00000004	/* getpage did an unexpected read */
#define	INOBMAPCACHE	0x00000008	/* bmap cache is disabled */
#define	IBADUPD		0x00000010	/* must be marked bad to disk */
#define	IPUTTIME	0x00000020	/* B_FORCE putpage mtime update */
#define	IATTRREM	0x00000040	/* remove attribute on inactive */
#define INOBMAPCLUSTER	0x00000080	/* prevent bmap from clustering */

/*
 * File modes.  File types above 0xf000 are vxfs internal only, they should
 * not be passed back to higher levels of the system.  vxfs file types must
 * never have one of the regular file type bits set.
 */

#define	IFMT		0xf000f000	/* type of file */
#define	IFIFO		0x00001000	/* 0010000 named pipe (fifo) */
#define	IFCHR		0x00002000	/* 0020000 character special */
#define	IFDIR		0x00004000	/* 0040000 directory */
#define	IFNAM		0x00005000	/* 0050000 xenix special file */
#define	IFBLK		0x00006000	/* 0060000 block special */
#define	IFREG		0x00008000	/* 0100000 regular */
#define	IFLNK		0x0000a000	/* 0120000 symbolic link */

#define	IFFSH		0x10000000	/* file set header */
#define	IFILT		0x20000000	/* inode list */
#define	IFIAU		0x30000000	/* inode allocation unit */
#define	IFCUT		0x40000000	/* current usage table */
#define	IFATT		0x50000000	/* attribute inode */
#define	IFLCT		0x60000000	/* lint count table */
#define	IFIAT		0x70000000	/* indirect attribute file */
#define	IFEMR		0x80000000	/* extent map reorg file */

#define	ISUID		0x800		/* 04000 set user id on execution */
#define	ISGID		0x400		/* 02000 set group id on execution */
#define	ISVTX		0x200		/* 01000 sticky bit */
#define	IREAD		0x100		/* 0400 read permission */
#define	IWRITE		0x080		/* 0200 write permission */
#define	IEXEC		0x040		/* 0100 execute/search permission */

#define	VX_GEMODE	(ISGID | (IEXEC >> 3))	/* setgid + group exec mode */

/*
 * org types
 */

#define	IORG_NONE	0		/* inode has no format */
#define	IORG_EXT4	1		/* inode has 4 byte data block addrs */
#define	IORG_IMMED	2		/* data stored in inode */

/*
 * allocation flags
 */

#define	VX_AF_MASK	0x000f		/* allocation flags mask */
#define	VX_AF_IBAD	0x0001		/* inode is bad */
#define	VX_AF_NOEXTEND	0x0002		/* file can't be auto extended */
#define	VX_AF_ALIGN	0x0004		/* all extents must be aligned */
#define	VX_AF_NOGROW	0x0008		/* file can't grow */

/*
 * extop flags
 */

#define	VX_IEMASK	0x003f		/* extended op masks */
#define	VX_IEREMOVE	0x0001		/* deferred inode removal */
#define	VX_IETRUNC	0x0002		/* extended truncation */
#define	VX_IERTRUNC	0x0004		/* trim blocks down to i_reserve */
#define	VX_IESHORTEN	0x0008		/* shorten file to i_size */
#define	VX_IEZEROEXT	0x0010		/* zeroing an extent */
#define	VX_IETRIM	0x0020		/* trim reservation to i_size */

/*
 * vxfs specific errno values
 */

#define	VX_ERETRY	0xf001	/* iget must be retried */
#define	VX_ENOINODE	0xf002	/* out of inodes */
#define	VX_ENOENT	0xf003	/* ENOENT from vx_dirscan and vx_attr_rm */
#define	VX_EAGMOUNT	0xf004	/* aggregate mount was successful */
#define	VX_EQUOTA	0xf005	/* quota overflow */
#define	VX_EMAXTRUNC	0xf006	/* truncation must be retried */
#define	VX_EMAXALLOC	0xf007	/* too many allocations in transaction */
#define	VX_ERWLOCK	0xf008	/* need a RWLOCK in transaction */
#define VX_EMAXHOLD	0xf009	/* too many held inodes in the transaction */
#define VX_EMAPBAD	0xf00a	/* error detected in bitmap */

/*
 * Increment the access generation count.
 */

#define	VX_BUMP_AGEN(ip) {	\
	if (ITOV(ip)->v_type == VDIR) {	\
		(ip)->i_agen++;		\
		VX_CACHE_WRITE_SYNC();	\
	}				\
}

/*
 * Convert between inode pointers and vnode pointers
 */

#define	VTOI(vp)	((struct vx_inode *)(void *)(vp)->v_data)
#define	ITOV(ip)	(&(ip)->i_vnode)

#define	VX_HOLE	(daddr_t)-1	/* value used when no block allocated */

/*
 * Lock and unlock inodes.
 */

#ifdef	_KERNEL

#define	VX_IRWLOCK(ip, mode)		vx_irwlock(ip, mode)
#define	VX_IRWLOCK_TRY(ip, mode)	vx_irwlock_try(ip, mode)
#define	VX_IRWLOCK_AVAIL(ip, mode)	vx_irwlock_avail(ip, mode)
#define	VX_IRWUNLOCK(ip)		vx_irwunlock(ip)

#define	VX_IGLOCK(ip, mode)		vx_iglock(ip, mode)
#define	VX_IGLOCK_TRY(ip, mode)		vx_iglock_try(ip, mode)
#define	VX_IGLOCK_AVAIL(ip, mode)	vx_iglock_avail(ip, mode)
#define	VX_IGUNLOCK(ip)			vx_igunlock(ip)

#define	VX_IPLOCK(ip, mode)		vx_iplock(ip, mode)
#define	VX_IPLOCK_TRY(ip, mode)		vx_iplock_try(ip, mode)
#define	VX_IPLOCK_AVAIL(ip,mode)	vx_iplock_avail(ip, mode)
#define	VX_IPUNLOCK(ip)			vx_ipunlock(ip)

#define	VX_ILOCK(ip)			vx_ilock(ip)
#define	VX_ILOCK_TRY(ip)		vx_ilock_try(ip)
#define	VX_ILOCK_AVAIL(ip)		vx_ilock_avail(ip)
#define	VX_IUNLOCK(ip)			vx_iunlock(ip)
#define	VX_IUNLOCK_NOFLUSH(ip)		vx_iunlock_noflush(ip)

#ifndef	TED_
#define	VX_ISPIN_LOCK(ip)		LOCK(&(ip)->i_spinlock, PLMIN)
#else									/*TED_*/
#define	VX_ISPIN_LOCK(ip)		xted_ispin_lock(ip, PLMIN)	/*TED_*/
#endif									/*TED_*/
#define	VX_ISPIN_UNLOCK(ip, ipl) {				\
	XTED_ISPIN_UNLOCK(ip, ipl);				\
	UNLOCK(&(ip)->i_spinlock, ipl);				\
}

#ifndef	TED_
#define	VX_IINTR_LOCK(ip)		LOCK(&(ip)->i_intrlock, PLHI)
#else									/*TED_*/
#define	VX_IINTR_LOCK(ip) 		xted_iintr_lock(ip)		/*TED_*/
#endif									/*TED_*/
#define	VX_IINTR_UNLOCK(ip, ipl) {				\
	XTED_IINTR_UNLOCK(ip, ipl);				\
	UNLOCK(&(ip)->i_intrlock, ipl);				\
}

#ifndef	TED_
#define	VX_ILIST_LOCK()			LOCK(vx_ilist_lkp, PLMIN)
#else									/*TED_*/
#define	VX_ILIST_LOCK() 		xted_ilist_lock()		/*TED_*/
#endif									/*TED_*/
#define	VX_ILIST_UNLOCK(ipl) {					\
	XTED_ILIST_UNLOCK(ipl);					\
	UNLOCK(vx_ilist_lkp, ipl);				\
}

/*
 * The inode pointers lock protects the array of pointers to vxfs inodes.
 * While held exclusively, memory for inodes can be allocated or freed.
 * While held shared, the number of incore vxfs inodes and array of
 * pointers to them is stable.
 */

extern rwsleep_t		*vx_iptrs_slkp;

#define	VX_IPTRS_LOCK(mode) {					\
	XTED_IPTRS_LOCK(mode);					\
	if (mode == VX_LOCK_SHARED) {				\
		RWSLEEP_RDLOCK(vx_iptrs_slkp, PRINOD);		\
	} else {						\
		RWSLEEP_WRLOCK(vx_iptrs_slkp, PRINOD);		\
	}							\
}

#define	VX_IPTRS_UNLOCK() {					\
	XTED_IPTRS_UNLOCK();					\
	RWSLEEP_UNLOCK(vx_iptrs_slkp);				\
}


/*
 * Force any delayed writes on the inode to complete.
 */

#define	VX_DELBUF_FLUSH(ip) {			\
	if ((ip)->i_flag & IDELBUF) {		\
		vx_delbuf_flush(ip);		\
	}					\
}

#define	VX_LOGWRI_FLUSH(ip) {			\
	if ((ip)->i_intrflag & ILOGWRITE) {	\
		vx_logwrite_flush(ip);		\
	}					\
}

#define	VX_DELXWRI_FLUSH(ip, mode) {					\
	TED_ASSERT("f:VX_DELXWRI_FLUSH,1a",				\
		   XTED_IRWLOCK_OWNED(ip, XTED_LOCK_LOCKED) &&	/*TED_*/\
		   !XTED_IGLOCK_OWNED(ip, XTED_LOCK_LOCKED));	/*TED_*/\
	if ((ip)->i_intrflag & IDELXWRI) {				\
		vx_idelxwri_flush((ip), (mode));			\
	}								\
}

/*
 * Find advisories in effect for current process
 */

#define	VX_ADVISEGET(ip)	((ip)->i_advise)

/*
 * Clean up advisories on close
 */

#define	VX_ADVISECLOSE(ip) {						\
	TED_ASSERT("f:VX_ADVISECLOSE:1a", XTED_ILOCK_OWNED(ip)); /*TED_*/\
	if ((ip)->i_advise & VX_DIRECT)	{				\
		(ip)->i_diocount = 0;					\
	}								\
	(ip)->i_advise = 0;						\
}

/*
 * Swap lock and unlock.
 */

#define	VX_SWAPLOCK()	++(u.u_lwpp->l_keepcnt)
#define	VX_SWAPUNLOCK() --(u.u_lwpp->l_keepcnt)

/*
 * Put an inode on the end of the free list.
 */

#define	VX_IPFREE(ip)	{					\
	TED_ASSERT("f:VX_IPFREE:1a", ITOV(ip)->v_count == 0	\
		   && (ip)->av_forw == NULL		/*TED_*/\
		   && (ip)->av_back == NULL);		/*TED_*/\
	(ip)->i_freelist->av_back->av_forw = (ip);		\
	(ip)->av_forw = (struct vx_inode *) (ip)->i_freelist;	\
	(ip)->av_back = (ip)->i_freelist->av_back;		\
	(ip)->i_freelist->av_back = (ip);			\
	(ip)->i_freelist->i_fshead->f_inuse--;			\
	(ip)->i_freelist->i_fshead->f_free++;			\
	(ip)->i_freelist->i_count++;				\
	MET_INODE_INUSE(MET_VXFS, -1);                          \
}

/*
 * Put an inode on the front of the free list.
 */

#define	VX_IPFFREE(ip)	{					\
	TED_ASSERT("f:VX_IPFFREE:1a", ITOV(ip)->v_count == 0	\
		   && (ip)->av_forw == NULL		/*TED_*/\
		   && (ip)->av_back == NULL);		/*TED_*/\
	(ip)->i_freelist->av_forw->av_back = (ip);		\
	(ip)->av_back = (struct vx_inode *) (ip)->i_freelist;	\
	(ip)->av_forw = (ip)->i_freelist->av_forw;		\
	(ip)->i_freelist->av_forw = (ip);			\
	(ip)->i_freelist->i_fshead->f_inuse--;			\
	(ip)->i_freelist->i_fshead->f_free++;			\
	(ip)->i_freelist->i_count++;				\
        MET_INODE_INUSE(MET_VXFS, -1);                          \
}

/*
 * Remove an inode from the freelist.
 */

#define	VX_IUNFREE(ip)	{			\
	(ip)->av_back->av_forw = (ip)->av_forw;	\
	(ip)->av_forw->av_back = (ip)->av_back;	\
	(ip)->av_forw = 0;			\
	(ip)->av_back = 0;			\
	(ip)->i_freelist->i_fshead->f_inuse++;	\
	(ip)->i_freelist->i_fshead->f_free--;	\
	(ip)->i_freelist->i_count--;		\
        MET_INODE_INUSE(MET_VXFS, 1);           \
}

/*
 * Put an inode on a hash list.
 */

#define	VX_IPHASH(hip, ip)	{		\
	(hip)->i_forw->i_back = (ip);		\
	(ip)->i_forw = (hip)->i_forw;		\
	(hip)->i_forw = (ip);			\
	(ip)->i_back = (struct vx_inode *)(hip);	\
}

/*
 * Remove an inode from its hash list.  To allow multiple calls,
 * the forward and back hash pointers point back to the inode.
 */

#define	VX_IUNHASH(ip)	{			\
	(ip)->i_back->i_forw = (ip)->i_forw;	\
	(ip)->i_forw->i_back = (ip)->i_back;	\
	(ip)->i_forw = (ip);			\
	(ip)->i_back = (ip);			\
}

/*
 * Put an inode on an inode list block hash list.
 */

#define	VX_IPBHASH(hip, ip)	{		\
	(hip)->i_bforw->i_bback = (ip);		\
	(ip)->i_bforw = (hip)->i_bforw;		\
	(hip)->i_bforw = (ip);			\
	(ip)->i_bback = (struct vx_inode *)(hip);	\
}

/*
 * Remove an inode from its inode list block hash.  To allow multiple
 * calls, the forward and back hash pointers point back to the inode.
 */

#define	VX_IUNBHASH(ip)	{			\
	(ip)->i_bback->i_bforw = (ip)->i_bforw;	\
	(ip)->i_bforw->i_bback = (ip)->i_bback;	\
	(ip)->i_bforw = (ip);			\
	(ip)->i_bback = (ip);			\
}

/*
 * Hash an inode based on device, file set index, and inode number or
 * device and inode list block number.
 */

#define	VX_IHASH(idx, dev, num)	\
		(vx_hinode + ((((int)(dev) << 4) + (int)(dev) + \
			       ((int)(idx) << 3) - (int)(idx) + \
			        (int)(num)) & (vx_nhino - 1)))

#define	VX_IBHASH(dev, num)	\
		(vx_hinode + ((((int)(dev) << 4) + (int)(dev) + \
			        (int)(num)) & (vx_nhino - 1)))

/*
 * Hold a vnode that may have a delayed inactive pending.  If the vnode
 * is a delayed inactive, then take that hold and clear the inactive.
 */

#define	VX_VNHOLD(ip, vp) {			\
	VN_LOCK(vp);				\
	if ((ip)->i_vinactive) {		\
		(ip)->i_vinactive = 0;		\
		(ip)->i_ftime = 0;		\
	} else {				\
		(vp)->v_count++;		\
	}					\
	VN_UNLOCK(vp);				\
}

extern struct vx_hinode		*vx_hinode;
extern int			vx_nhino;

extern struct vnodeops		vx_vnodeops;
extern struct vnodeops		vx_attrvnodeops;
extern struct vx_fshead		vxfs_fshead;
extern struct vx_fshead		vxfs_attr_fshead;
extern struct vx_ifreehead	vx_ifreelist;
extern struct vx_ifreehead	vx_iinactivelist;
extern struct vx_ifreehead	vx_idirtypagelist;
extern struct vx_ifreehead	vx_attr_ifreelist;

extern int			vxfs_ninode;
extern int			vx_sync_time;
extern int			vx_iclean_timelag;
extern int			vx_ifree_timelag;
extern int			vx_iinactive_timelag;
extern int			vx_iinactive_delay_timelag;
extern int			vx_delaylog_timelag;

extern int			vx_flushing_threads;
extern int			vx_stopping_threads;

union vx_ipargs {
	struct vx_ipchkver {
		struct vx_version	version;
	} ipchkser;
};

#endif	/* _KERNEL */


/*
 * This overlays the fid structure (see vfs.h).
 */

struct vx_vfid {
	u_short	vfid_len;
	ino_t	vfid_ino;
	long	vfid_gen;
};

/*
 * Access Control List 
 */

#define VX_ACL_CLASS		1
#define VX_ACL_SVR4_SUBCLASS	1

struct vx_aclhd {
	long	aclcnt;		/* total number of acl entries */
	long	daclcnt;	/* number of default acl entrie */
};

/*
 * The attribute code that modifies attributes, particular the attribute
 * inheritance code, requires a number of inode locks.  Some must be
 * appropriately cleaned up/reacquired when a transaction commits and
 * all must be released when the operation completes (an operation may
 * require multiple transactions).
 *
 * See vx_attr.c for a description of attribute locking.  The fields
 * are arranged in the order that locks are acquired.  All non_NULL
 * fields represent inodes that are held and locked.  The locks are:
 *
 * Field	Lock	Explanation
 *
 * vt_dirnip	rwlock	indirect attribute inode for "parent" inode
 * vt_preinip	rwlock	pre-existing indirect attribute inode for target
 * vt_preaip	rwlock	pre-existing attribute inode for target
 * vt_newaip	ilock	newly allocated (or modified) attribute inode
 *			for target
 * vt_newinip	ilock	newly allocated (or modified) indirect attribute
 *			inode for target
 * vt_oldaip	none	an old attribute inode removed in the transaction
 *
 * vt_preinip and vt_newinip may refer to the same inode, if it was
 * pre-existing and modified in the transaction.  vt_preaip and
 * vt_newaip may also refer to the same inode, typically when the
 * inode rquires a multi-transaction allocation.
 */

struct vx_attrlocks {
	struct vx_inode *vt_dirinip;	/* directory (parent) indirect inode */
	struct vx_inode *vt_preinip;	/* pre-existing indirect inode */
	struct vx_inode	*vt_preaip;	/* pre-existing attribute inode */
	struct vx_inode	*vt_newaip;	/* new attribute inode */
	struct vx_inode	*vt_newinip;	/* new indirect attribute inode */
	struct vx_inode *vt_oldaip;	/* old (removed) attribute inode */
};

/*
 * Parameters passed through dircreate for inheritance.
 */

struct vx_inherit {
	struct vx_attrlocks	vh_alocks;	/* 0x0 attribute locks */
	struct vx_inode		*vh_linklist[5]; /* 0x18 attr inodes linked */
	int			vh_nlink;	/* 0x2c num of entries */
	struct vx_tran		*vh_tranp;	/* 0x30 current transaction */
	int			vh_onetran;	/* 0x34 fast inheritance */
};

/*
 * Structure for inode attribute area.
 */

struct vx_iattr {
	ulong	a_format;	/* format of attribute entry */
	ulong	a_length;	/* length of attribute entry */
	char	a_data[4];	/* attribute data (expandable) */
};

#define	VX_ATTROVERHEAD		(sizeof (struct vx_iattr) - 4)

/*
 * Structure for immediate attributes.
 */

struct vx_attr_immed {
	long	aim_class;	/* class of attribute */
	long	aim_subclass;	/* subclass of attribute */
	char	aim_data[4];	/* the immediate attribute data (expandable) */
};

#define	VX_ATTR_IMMEDOVER	(sizeof (struct vx_attr_immed) - 4)

/*
 * Structure for directly indexed attributes.
 */

struct vx_attr_direct {
	long	ad_class;	/* class of attribute */
	long	ad_subclass;	/* subclass of attribute */
	long	ad_length;	/* length of attribute */
	ulong	ad_ino;		/* inode containing attribute data */
};

/*
 * Structure for listing attributes.
 */

struct vx_attrlist {
	long	al_class;	/* class of attribute */
	long	al_subclass;	/* subclass of attribute */
	long	al_length;	/* length of attribute */
};

/*
 * Arguments to VX_ATTRLIST ioctl.
 */

struct vx_albuf {
	long	len;		/* length of buffer */
	caddr_t	bufp;		/* buffer for attribute list */
};

/*
 * Arguments to VX_ATTRGET ioctl.
 */

struct vx_agbuf {
	long	class;		/* class of attribute */
	long	subclass;	/* subclass of attribute */
	long	len;		/* length of buffer */
	caddr_t	bufp;		/* buffer for attribute */
};

/*
 * Format for buffers passed to VX_ATTRSET ioctl.
 */

struct vx_asent {
	int	function;	/* function */
	long	class;		/* class of attribute */
	long	subclass;	/* subclass of attribute */
	long	len;		/* length of data */
	caddr_t	bufp;		/* attribute data */
};

/*
 * Values for the set attribute functions.
 */

#define	VX_AREMOVE	0x1	/* Remove an attribute */
#define	VX_ASET		0x2	/* Set an attribute */
#define	VX_ARESTORE	0x3	/* Restore an attribute */

#define	VX_ALIST	0x4	/* Listing attributes */
#define	VX_AREAD	0x5	/* Reading attributes */

/*
 * Attribute area formats
 */

#define	VX_ATTR_IMMED		1	/* stored immediately in inode */
#define	VX_ATTR_DIRECT		2	/* attribute inum stored in inode */
#define	VX_ATTR_EXTIMDATA	3	/* extension of file immed data */

#define	VX_ATTR_INDIRECT	100	/* direct attribute stored in an
					   indirect attribute inode */

#define	VX_ATTR_ALL	-1	/* wildcard class and subclass value */

#define	VX_FREECLASS	-2	/* class value for free slot */

#define	VX_PUBLIC_CLASS	1000	/* classes below 1000 are reserved */

#define	VX_ATTR_IGNORE	1	/* Don't allow operation to see attribute */
#define	VX_ATTR_KEEP	2	/* Perform operation as requested */
#define	VX_ATTR_NEW	3	/* Substitute new attribute data */
#define	VX_ATTR_LINK	4	/* Link to parent attribute */

struct vx_attrop {
	int	(*aop_listop)();	/* Listing/Reading attributes */
	int	(*aop_changeop)();	/* Setting/Removing attributes */
	int	(*aop_creatop)();	/* Inheriting attributes */
};

struct vx_attrreg {
	struct vx_attrreg	*ar_next;	/* next registered class */
	long			ar_class;	/* registered class */
	struct vx_attrop	ar_aops;	/* attribute operations */
};

#define	VX_NREGHASH	64	/* num of hash chains of registered classes */

#ifdef _KERNEL

#define	VX_ATTROP_LOCK_INIT(flag) \
	RWSLEEP_INIT(&vx_attrop_lock, 0, &vx_attrop_lkinfo, flag);

#define VX_ATTROP_LOCK(flag) {					\
	if (flag == VX_LOCK_SHARED) {  				\
		RWSLEEP_RDLOCK(&vx_attrop_lock, PRINOD);	\
	} else {  						\
		RWSLEEP_WRLOCK(&vx_attrop_lock, PRINOD);	\
	}  							\
	XTED_ATTROP_LOCK(flag);					\
}

#define VX_ATTROP_UNLOCK() {					\
	XTED_ATTROP_UNLOCK();					\
	RWSLEEP_UNLOCK(&vx_attrop_lock);			\
}

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_VXFS_VX_INODE_H */
