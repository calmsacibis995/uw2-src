/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_tran.h	2.29 24 Jul 1994 18:42:57 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_tran.h	1.12"

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

#ifndef	_FS_VXFS_VX_TRAN_H
#define	_FS_VXFS_VX_TRAN_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#ifndef _UTIL_KSYNCH_H
#include <util/ksynch.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/ksynch.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * transaction function ids
 */

#define	VX_IMTRAN	1			/* inode modification */
#define	VX_DATRAN	2			/* add directory entry */
#define	VX_DACTRAN	3			/* add and compress directory */
#define	VX_DRTRAN	4			/* remove directory entry */
#define	VX_IETRAN	5			/* IMAP or IEMAP operation */
#define	VX_EMTRAN	6			/* extent map */
#define	VX_TRTRAN	7			/* indirect truncation */
#define	VX_WLTRAN	8			/* write a symlink */
#define	VX_TATRAN	9			/* indirect allocation */
#define	VX_LWRTRAN	10			/* logged write */
#define	VX_LWRDATA	11			/* logged write data */
#define	VX_LWRDONE	12			/* logged writes done */
#define	VX_CUTRAN	13			/* CUT change transaction */
#define	VX_FSHDTRAN	14			/* fshead change transaction */
#define	VX_ILTRAN	15			/* ilist change transaction */
#define	VX_LCTRAN	16			/* link count table update */
#define	VX_LBTRAN	17			/* logged buffer changes */
#define	VX_LBDATA	18			/* logged buffer data */
#define	VX_DONE		100			/* done a transaction */
#define	VX_UNDO		101			/* undo a transaction */
#define VX_NULLTRAN	102			/* null (space filler) */

/*
 * transaction common area
 */

struct vx_ctran {
	char		*t_logp;	/* 0x00 pointer to log area */
	int		t_loglen;	/* 0x04 log size */
	int		t_bflags;	/* 0x08 flags */
	struct buf	*t_bp;		/* 0x0c buffer pointer */
	short		t_func;		/* 0x10 function type */
	short		t_flags;	/* 0x12 flags */
	struct vx_mlink	*t_mlink;	/* 0x14 pointer to mlink */
	struct vx_inode	*t_ip;		/* 0x18 inode pointer */
	union vx_subfunc {		/* 0x1c tran specific data */
		caddr_t			addr;
		struct vx_ietran	*ietran;
		struct vx_imtran	*imtran;
		struct vx_datran	*datran;
		struct vx_drtran	*drtran;
		struct vx_daclog	*daclog;
		struct vx_emtran	*emtran;
		struct vx_trtran	*trtran;
		struct vx_tatran	*tatran;
 		struct vx_lwrtran	*lwrtran;
 		struct vx_wltran	*wltran;
 		struct vx_iltran	*iltran;
 		struct vx_cutran	*cutran;
 		struct vx_fshdtran	*fshdtran;
 		struct vx_lctran	*lctran;
 		struct vx_lbtran	*lbtran;
 		struct vx_duclog	*duclog;
	} t_spec;
					/* 0x20 is length */
};

#define	T_ALLOCATED		0x01	/* buffer for newly allocated extent */
#define	T_SYNC			0x02	/* write buffer at commit time */
#define	T_ACOUNT		0x04	/* t_acount should be bumped */

#define	VX_ENTMAX	32	/* max number of ops in a map subfunction */

/*
 * extent map manipulation
 */

struct vx_emtran {
	struct vx_emlog {
		struct vx_ement {
			long	em_op;		/* VX_EMALLOC or VX_EMFREE */
			daddr_t	em_ebno;	/* starting block */
			long	em_elen;	/* extent length */
			int	em_inval;	/* invalidate buffer cache */
		} log_ent[VX_ENTMAX];
	} em_log;
	struct vx_emtran *em_next;		/* next emtran */
	struct vx_emtran *em_prev;		/* previous emtran */
	long		em_nent;		/* number of entries */
	struct vx_ctran *em_tcp;		/* tcp linkage */
	long		em_aun;			/* au number */
	daddr_t		em_austart;		/* start block of au */
};

#define	VX_EMALLOC	1			/* extent allocation */
#define	VX_EMFREE	2			/* extent free */

/*
 * inode map update subfunction
 */

struct vx_ietran {
	struct vx_ielog {
		long	iel_maptype;		/* IMAP or IEMAP */
		long	iel_fset;		/* file set index */
		daddr_t	iel_bno;		/* map block number */
		long	iel_aun;		/* au number */
		long	iel_ausbno;		/* au summmary bno */
		struct vx_iment {
			long	iel_ino;	/* inode number */
			long	iel_op;		/* set or clear */
		} log_ent[VX_ENTMAX];
	} ie_log;
	int	ie_nent;		/* number of entries in array */
	int	ie_flag[VX_ENTMAX];	/* flags for bad ialloc */
};

#define	ie_maptype	ie_log.iel_maptype
#define	ie_fset		ie_log.iel_fset
#define	ie_bno		ie_log.iel_bno
#define	ie_aun		ie_log.iel_aun
#define	ie_ausbno	ie_log.iel_ausbno

/*
 * log space required for a ietran subfunction, including the data
 */

#define	VX_IETRAN_LOGSZC	VX_LOGROUNDUP(sizeof (struct vx_ielog))

/*
 * ie_op values
 */

#define	VX_IASET	1			/* bit set */
#define	VX_IACLR	2			/* bit clear */

/*
 * inode modification transaction
 */

struct vx_imtran {
	struct vx_inode	*im_ip;		/* inode pointer */
	struct vx_ctran	*im_tcp;	/* ctran for this transaction */
	int		im_oldiflag;	/* flags from inode */
	int		im_ibad;	/* inode must be marked bad in undo */
	int		im_afree;	/* saved value of i_afree */
	int		im_aclcnt;	/* saved value of i_aclcnt */
	int		im_daclcnt;	/* saved value of i_daclcnt */
	int		im_iaclcnt;	/* saved value of i_iaclcnt */
	int		im_acllink;	/* saved value of i_acllink */
	int		im_modemask;	/* saved value of i_modemask */
	struct vx_imlog {
		long	log_fset;	/* file set index */
		long	log_ino;	/* inode number */
		long	log_bno;	/* inode list extent */
		off_t	log_osize;	/* saved value of i_nsize */
		char	log_inodes[4];	/* new and old inodes (variable) */
	} im_log;
};

#define	im_inodes	im_log.log_inodes
#define	im_fset		im_log.log_fset
#define	im_ino		im_log.log_ino
#define	im_bno		im_log.log_bno
#define	im_osize	im_log.log_osize

/*
 * directory entry addition transaction
 * boff is offset in block of old entry
 * if old entry has an ino, then new entry follows old entry
 * otherwise new entry overwrites old entry
 */

struct vx_datran {
	struct vx_dalog {
		long	log_fset;		/* file set index */
		long	log_ino;		/* directory inode */
		daddr_t	log_bno;		/* directory block no */
		off_t	log_boff;		/* offset in block */
		off_t	log_blen;		/* length of block */
		struct vx_mindirect log_old;	/* old directory entry */
		struct vx_direct log_new;	/* new directory entry */
	} da_log;
	struct vx_inode	*da_tdp;		/* target directory */
	struct vx_ctran	*da_side;		/* tcp if shared inode */
	off_t		da_eoff;		/* offset of block in extent */
	off_t		da_elen;		/* length of extent */
	int		da_hashoff;		/* offset of hash chain head */
	caddr_t		da_np;			/* compress block */
	off_t		da_nlen;		/* length of compress block */
};

#define	da_fset	da_log.log_fset
#define	da_ino	da_log.log_ino
#define	da_bno	da_log.log_bno
#define	da_boff	da_log.log_boff
#define	da_blen	da_log.log_blen
#define	da_old	da_log.log_old
#define	da_new	da_log.log_new

/*
 * directory entry removal transaction
 * boff is offset in block of old entry
 * if boff is not beginning of block, then 
 * otherwise new entry overwrites old entry
 */
struct vx_drtran {
	struct vx_drlog {
		long	log_fset;		/* file set index */
		long	log_ino;		/* directory inode */
		daddr_t	log_bno;		/* directory block no */
		off_t	log_poff;		/* offset in block  of prev */
		off_t	log_roff;		/* offset in block of rem */
		off_t	log_blen;		/* length of block */
		struct vx_mindirect log_prev;	/* previous directory entry */
		struct vx_direct log_rem;	/* removed directory entry */
	} dr_log;
	struct vx_inode	*dr_tdp;		/* target directory */
	struct vx_ctran	*dr_side;		/* tcp if shared inode */
	off_t		dr_eoff;		/* offset of block in extent */
	off_t		dr_elen;		/* length of extent */
	int		dr_hashoff;		/* offset of hash chain head */
	short		dr_hashold;		/* old hash values */
};

#define	dr_fset	dr_log.log_fset
#define	dr_ino	dr_log.log_ino
#define	dr_bno	dr_log.log_bno
#define	dr_poff	dr_log.log_poff
#define	dr_roff	dr_log.log_roff
#define	dr_blen	dr_log.log_blen
#define	dr_prev	dr_log.log_prev
#define	dr_rem	dr_log.log_rem

/*
 * log space required for a drtran subfunction, including the data
 */

#define	VX_DRTRAN_LOGSZC(size)	VX_LOGROUNDUP(sizeof (struct vx_drlog) + size)

/*
 * if VX_DACTRAN, then the log area is an vx_daclog structure followed by
 * the before and after contents of the entire block
 */

struct vx_daclog {
	long	log_fset;		/* file set index */
	long	log_ino;		/* inode containing extent */
	daddr_t	log_bno;		/* directory block no */
	off_t	log_oldblen;		/* length of pre-image block */
	off_t	log_newblen;		/* length of post-image block */
};

/*
 * log space required for a dactran subfunction, including the data
 */

#define	VX_DACTRAN_LOGSZC(size)	VX_LOGROUNDUP(sizeof (struct vx_daclog) + size)

/*
 * Used to log the pathname if the symbolic link doesn't go
 * into immediate data.
 */

struct vx_wltran {
	long	log_fset;		/* file set index */
	long	log_ino;		/* inode containing extent */
	daddr_t	log_bno;		/* symlink block no */
	off_t	log_len;		/* length of extent */
	off_t	log_dlen;		/* length of data */
	char	log_data[4];		/* symlink data */
};

/*
 * log space required for a wltran subfunction, including the data
 */

#define	VX_WLTRAN_LOGSZC(size)	VX_LOGROUNDUP(sizeof (struct vx_wltran) + \
					      size - 4)

/*
 * Logged write to a file.
 */

struct vx_lwrtran {
	struct vx_lwrlog {
		long	log_fset;	/* file set index */
		long	log_ino;	/* inode being written */
		u_int	log_offset;	/* starting offset of write */
		u_int	log_len;	/* length of write */
	} lwr_log;
	struct vx_inode	*lwr_ip;	/* vnode pointer */
};

#define	lwr_fset	lwr_log.log_fset
#define	lwr_ino		lwr_log.log_ino
#define	lwr_len		lwr_log.log_len
#define	lwr_offset	lwr_log.log_offset

/*
 * log space required for a lwrtran subfunction, including the data
 *
 * the data is logged as a seperate lwrdata subfunction which may span
 * multiple sectors and requires VX_LOGOVER for each additional sector
 */

#define	VX_LWRTRAN_LOGSZC(size)	(VX_LOGROUNDUP(sizeof (struct vx_lwrlog)) + \
				 VX_LOGROUNDUP(size))

/*
 * Logged writes on a file are done.
 */

struct vx_lwrdonelog {
	long	log_fset;	/* file set index */
	long	log_ino;	/* inode number */
};

/*
 * log space required for a lwrdone subfunction, including the data
 */

#define	VX_LWRDONE_LOGSZC	VX_LOGROUNDUP(sizeof (struct vx_lwrdonelog))

/*
 * indirect extent truncation
 */

struct vx_trtran {
	struct vx_trlog {
		long	log_fset;		/* file set index */
		long	log_ino;		/* inode number */
		int	log_lvl;		/* indirection level */
		daddr_t	log_abno;		/* bno of address extent */
		struct vx_trent {
			off_t	log_aoff;	/* offset in address extent */
			daddr_t	log_ebno;	/* bno of data extent */
		} log_ent[VX_ENTMAX];
	} tr_log;
	struct vx_inode	*tr_ip;		/* inode pointer */
	int		tr_nent;	/* number of extries */
};

#define	tr_fset		tr_log.log_fset
#define	tr_ino		tr_log.log_ino
#define	tr_lvl		tr_log.log_lvl
#define	tr_abno		tr_log.log_abno
#define	tr_trent	tr_log.log_ent

/*
 * log space required for a trtran subfunction
 */

#define	VX_TRTRAN_LOGSZC	VX_LOGROUNDUP(sizeof (struct vx_trlog))

/*
 * indirect extent addition
 *
 * The bmap allocator can allocate up to VX_MAXKCONTIG extents in indirects
 * and return them as one extent.  This means the kernel must limit itself
 * to VX_MAXKALLOC allocating bmap calls to avoid overflowing the tatran.
 *
 * In fact, the tatran can be grown to hold enough entries for an entire
 * indirect address extent, but most transaction won't reserve enough log
 * space for such a big transaction, so they're just as well limiting
 * themselves to the amount defined here.
 */

#define	VX_MAXALLOC	64		/* maximum ind ext allocs per subfunc */
#define	VX_MAXKALLOC	8		/* maximum bmap allocs per tran */
#define	VX_MAXKCONTIG	8		/* VX_MAXALLOC / VX_MAXKALLOC */

struct vx_tatran {
	struct vx_talog {
		int	log_new;	/* set if addr extent to be cleared */
		long	log_fset;	/* file set index */
		long	log_ino;	/* inode number */
		int	log_lvl;	/* indirection level */
		daddr_t	log_abno;	/* bno of address extent */
		struct vx_taent {
			off_t	log_aind;	/* index in address extent */
			daddr_t	log_ebno;	/* bno of data extent */
		} log_taent[VX_MAXALLOC];
	} ta_log;
	struct vx_inode	*ta_ip;		/* inode pointer */
	int		ta_nblk;	/* number of entries in array */
	int		ta_swap;	/* part of extent swap */
	daddr_t		*ta_oldbno;	/* old block numbers */
	struct vx_talog	*ta_logp;	/* where we log from */
	daddr_t		ta_oldbnos[VX_MAXALLOC]; /* old block number storage */
};

#define	ta_new		ta_logp->log_new
#define	ta_fset		ta_logp->log_fset
#define	ta_ino		ta_logp->log_ino
#define	ta_lvl		ta_logp->log_lvl
#define	ta_abno		ta_logp->log_abno
#define	ta_taent	ta_logp->log_taent

/*
 * log space required for a tatran subfunction
 */

#define	VX_TATRAN_LOGSZC	VX_LOGROUNDUP(sizeof (struct vx_talog))

struct vx_iltran {
	struct vx_illog	{
		long	log_fset;	/* file set index */
		long	log_ino;	/* inode number */
		struct vx_ilent {
			int	log_slot;	/* slot number */
			daddr_t	log_oldbno;	/* old slot contents */
			daddr_t	log_newbno;	/* new slot contents */
		} log_ent[VX_ENTMAX];
	} il_log;
	struct vx_inode	*il_ip;		/* inode pointer */
	daddr_t		il_offset;	/* file offset */
	int		il_nent;	/* numbr of entries in subfunction */
	struct vx_fset	*il_fsetp;	/* file set pointer */
	struct vx_ctran	*il_sidep;	/* replica iltran subfunction */
};

#define	il_fset		il_log.log_fset
#define	il_ino		il_log.log_ino
#define	il_ent		il_log.log_ent

/*
 * Fileset header modification subfunction.
 */

struct vx_fshdtran {
	struct vx_fslog {
		long	log_fset;	/* file set index */
		long	log_ino;	/* inode number */
		long	log_bno;	/* block number */
		long	log_index;	/* offset in file */
		struct vx_fsethead log_old;	/* old file set header */
		struct vx_fsethead log_new;	/* new file set header */
	} fhd_log;
	struct vx_fset		*fhd_fsetp;	/* pointer to file set */
	struct vx_inode		*fhd_ip;	/* pointer to inode */
	struct vx_ctran		*fhd_sidep;	/* replica fshd subfunction */
	daddr_t			fhd_iaubno;	/* extent containing new iau */
	daddr_t			*fhd_lctblks;	/* extents containing new lct */
	int			fhd_removed;	/* removed file set */
};

#define	fhd_fset	fhd_log.log_fset
#define	fhd_ino		fhd_log.log_ino
#define	fhd_bno		fhd_log.log_bno
#define	fhd_index	fhd_log.log_index
#define	fhd_old		fhd_log.log_old
#define	fhd_new		fhd_log.log_new

/*
 * log space required for a fshdtran subfunction
 */

#define	VX_FSHDTRAN_LOGSZC	VX_LOGROUNDUP(sizeof (struct vx_fslog))

/*
 * Done and undo subfunction information.
 */

struct vx_duclog {
	int	duc_logid;	/* log id of failed transaction */
	int	duc_err;	/* error code */
};

/*
 * CUT modification subfunction.
 */

struct vx_cutran {
	struct vx_culog {
		int		log_index;	/* index of entry in CUT */
		daddr_t		log_bno;	/* block number of CUT block */
		struct vx_cuent	log_old;	/* old cut entry */
		struct vx_cuent	log_new;	/* new cut entry */
	} cut_log;
	long		cut_delta;	/* change in blocks used */
	int		cut_alloc;	/* newly allocated cut entry */
	int		cut_free;	/* free the cut entry */
	struct vx_fset	*cut_fset;	/* fset owning this CUT entry */
	struct vx_cut	*cut_cutp;	/* CUT table */
};

#define	cut_index	cut_log.log_index
#define	cut_bno		cut_log.log_bno
#define	cut_old		cut_log.log_old
#define	cut_new		cut_log.log_new

/*
 * log space required for a cutran subfunction
 */

#define	VX_CUTRAN_LOGSZC	VX_LOGROUNDUP(sizeof (struct vx_culog))

/*
 * link count table update subfunction
 */

struct vx_lctran {
	struct vx_lclog {
		daddr_t	log_bno;		/* lct block number */
		struct vx_lcent {
			long		 lcl_ino;   /* inode number */
			union vx_lctdisk lcl_new;   /* new value */
			union vx_lctdisk lcl_old;   /* old value */
			int		 lcl_delta; /* change in value */
			int		 lcl_extop; /* IEREMOVE set on file */
		} log_ent[VX_ENTMAX];
	} lct_log;
	struct vx_lct	*lct_lctp;		/* pointer to lct structure */
	int		lct_nent;		/* number of entries in log */
	int		lct_holdcnt;		/* number of holds on lct */
	struct vx_inode	*lct_ips[VX_ENTMAX];	/* pointers to inodes */
};

#define	lct_bno		lct_log.log_bno
#define	lct_ent		lct_log.log_ent

/*
 * log space required for a lctran subfunction
 */

#define	VX_LCTRAN_LOGSZC	VX_LOGROUNDUP(sizeof (struct vx_lclog))

/*
 * logged buffer changes subfunction.  This is followed in
 * the log by a logged buffer data subfunction.
 */

struct vx_lbtran {
	struct vx_lblog {
		daddr_t	log_bno;	/* block number */
		int	log_offset;	/* offset in block */
		int	log_length;	/* length */
	} lb_log;
	struct vx_ctran	*lb_dtcp;	/* pointer to data subfunction */
	caddr_t		lb_data;	/* pointer to new and old data */
	int		lb_len;		/* total length of data */
};

#define	lb_bno		lb_log.log_bno
#define	lb_offset	lb_log.log_offset
#define	lb_length	lb_log.log_length

/*
 * log space required for a lbtran subfunction, including the data
 */

#define	VX_LBTRAN_LOGSZC(size)	VX_LOGROUNDUP(sizeof (struct vx_lblog) + size)

/*
 * transaction control structure
 * contains a list of subfunctions, log areas
 * and transaction control areas
 */

#define	VX_MAXTRAN	32		/* maximum number of subfunctions */
#define	VX_NMLINKS	8		/* mlink pointers in tran struct */

struct vx_tran {
	int		t_logid;	/* 0x00 transaction log id */
	int		t_logerr;	/* 0x04 error */
	short		t_logflag;	/* 0x08 logging control flags */
	short		t_nlog;		/* 0x0a number of log entries */
	short		t_reserve;	/* 0x0c log reservation */
	short		t_replaytran;	/* 0x0e log transaction count */
	short		t_ntran;	/* 0x10 number of subfunctions */
	short		t_acount;	/* 0x12 async activity count */
	struct vx_tran	*t_lognext;	/* 0x14 log/done queue linkage */
	struct vx_tran	*t_forw;	/* 0x18 active tran q forward link */
	struct vx_tran	*t_back;	/* 0x1c active tran q back link */
	struct vx_tran	*t_next;	/* 0x20 pointer to next trans */
	long		t_logoff;	/* 0x24 offset of tran in log */
	struct vx_ctran	*t_ctran;	/* 0x28 common area pointer */
	caddr_t		*t_pages;	/* 0x2c pointer to array of pages */
	short		t_pgindex;	/* 0x30 index into t_pages */
	short		t_poff;		/* 0x32 offset from start of page */
	int		t_undoid;	/* 0x34 log id of undo record */
	struct vx_mlinkhd t_ilink;	/* 0x38 trans/inode linkage */
	struct vx_mlink **t_mlinks;	/* 0x40 pointer to list of mlinks */
	struct vx_mlink	*t_mlinkp[VX_NMLINKS];	/* 0x44 mlink pointers */
					/* 0x64 end */
};

/*
 *  t_logflag values
 */

#define VX_TLOGDONE	0x001		/* log record written */
#define VX_TLOGGED	0x002		/* transaction put into log queue */
#define VX_TLOGDELAY	0x004		/* delayed log write for transaction */
#define VX_TLOGSYNC	0x008		/* must not delay log write */
#define VX_TLOGUNDO	0x010		/* undo of log record written */
#define VX_TLOGDELFREE	0x020		/* delayed extent free transaction */
#define VX_TRANPGFREE	0x040		/* transaction page free skipped */
#define VX_TLOGPOSTLOG	0x080		/* transaction has postlog routines */
#define VX_TNOQUOTA	0x100		/* bypass quota checks */

/*
 * Transaction commit types
 */

#define	VX_TCOM_ASYNC	0		/* always delayed */
#define	VX_TCOM_DELAY	10		/* delayed if delaylog, tmplog */
#define	VX_TCOM_TMP	20		/* delayed if tmplog */
#define	VX_TCOM_SYNC	30		/* do commit synchronously */

/*
 * The transaction page size is the unit of memory allocation
 * for transaction subfunctions.  Since subfunctions will not
 * spill across a tranpgsize boundary, it must be at least as
 * large as an imtran, and should be larger to reduce fragmentation.
 * Upto VX_MAXTRAN pages are allocated per transaction.
 */

extern int vx_tranpgsize;

/*
 * Return true if the log is upto the "low" flush point, and this
 * transaction is more than halfway back in the log.
 */

#define VX_TRAN_LOGLOW(fs, tranp) \
	(((fs)->fs_tranleft < (fs)->fs_tranlow || \
	  (ATOMIC_INT_READ(&vx_ntran) > vx_maxtran && \
	   (fs)->fs_replaytran > vx_replay_tran)) && \
	 (fs)->fs_logoff - (tranp)->t_logoff > \
	 (((fs)->fs_tranmax - (fs)->fs_tranleft - (fs)->fs_ntran) >> 1))

/*
 * Return true if the log is upto the "medium" flush point, and this
 * transaction is more than halfway back in the log.
 */

#define VX_TRAN_LOGMED(fs, tranp) \
	(((fs)->fs_tranleft < (fs)->fs_tranmed || \
	  (ATOMIC_INT_READ(&vx_ntran) > vx_maxtran && \
	   (fs)->fs_replaytran > vx_replay_tran)) && \
	 (fs)->fs_logoff - (tranp)->t_logoff > \
	 (((fs)->fs_tranmax - (fs)->fs_tranleft - (fs)->fs_ntran) >> 1))

/*
 * Transaction function switch.   These routines are called for each
 * subfunction in a transaction to perform subfunction specific
 * processing.
 *
 * The precommit routine is called at the beginning of vx_trancommit.
 * If it returns an error, the commit will fail and the transaction
 * is aborted.
 *
 * The postcommit routine is called at the end of the commit.  When it
 * is called, the transaction has successfully committed, and the routine
 * cannot return an error.
 *
 * The preundo routine is called at the beginning of vx_tranundo, before
 * the undo log record is written to disk.  It must undo any changes
 * made by the subfunction.  These changes might be on disk or in memory.
 *
 * The undologwrite routine is called in vx_log_undotran before the undo
 * log record is inserted into the log.  This is used by quota subfunctions
 * that must make changes to data in log order.  This routine cannot sleep.
 *
 * The postundo routine is called in vx_tranundo after the undo log record
 * is written to disk.  This is used by subfunctions that must have the
 * transaction nullified in the log before they can undo changes.
 *
 * The prelogwrite routine is called for each subfunction when the
 * transaction is being placed in the log.  It cannot sleep.  This routine
 * is used by quota subfunctions that must make changes to data is
 * log order.
 *
 * The postlogwrite routine is called for delayed transactions after
 * the log has been written.
 */

struct vx_transw {
	int	(*sw_precommit)();		/*  pre commit function */
	void	(*sw_postcommit)();		/*  post commit function */
	void	(*sw_preundo)();		/*  undo before abort logged */
	void	(*sw_undologwrite)();		/*  undo insertion into log */
	void	(*sw_postundo)();		/*  undo after abort logged */
	void	(*sw_prelogwrite)();		/*  pre log insert function */
	void	(*sw_postlogwrite)();		/*  log write done function */
};

#ifdef _KERNEL

extern struct vx_transw		vx_transw[];	/* transaction switch */

/*
 * Increment and decrement the total number of transactions in the system.
 */

#define	VX_NTRAN_INC()	ATOMIC_INT_INCR(&vx_ntran)
#define	VX_NTRAN_DEC()	ATOMIC_INT_DECR(&vx_ntran)
#define	VX_NTRAN_SUB(n)	ATOMIC_INT_SUB(&vx_ntran, (n))

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_VXFS_VX_TRAN_H */
