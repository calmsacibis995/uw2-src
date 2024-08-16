/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_info.h	2.17 24 Jul 1994 18:42:49 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_info.h	1.9"

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

#ifndef	_FS_VXFS_VX_INFO_H
#define	_FS_VXFS_VX_INFO_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_TYPES_H
#include <util/types.h> /* REQUIRED */
#endif

#ifndef _SVC_TIME_H
#include <svc/time.h> /* REQUIRED */
#endif

#ifndef _UTIL_ENGINE_H
#include <util/engine.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/types.h>	/* REQUIRED */
#include <sys/time.h>	/* REQUIRED */
#include <sys/engine.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * vxfs statistics
 */

struct vx_info {
	timestruc_t	vxi_tmstamp;	/* 0x000 vxfs idea of time */
	ulong	vxi_breada;		/* 0x008 vx_breada called */
	ulong	vxi_breada_io;		/* 0x00c vx_breada started an i/o */
	ulong	vxi_bio_stale;		/* 0x010 vx_bio B_STALE called */
	ulong	vxi_bio_write;		/* 0x014 vx_bio B_WRITE called */
	ulong	vxi_bio_awrite;		/* 0x018 vx_bio B_WRITE called ASYNC */
	ulong	vxi_bwrite;		/* 0x01c vx_bwrite called */
	ulong	vxi_bawrite;		/* 0x020 vx_bawrite called */
	ulong	vxi_bdwrite;		/* 0x024 vx_bdwrite called */
	ulong	vxi_bdwrite_tflush;	/* 0x028 tranflush on vx_bdwrite call */
	ulong	vxi_brelse;		/* 0x02c vx_brelse called */
	ulong	vxi_brelse_tflush;	/* 0x030 tranflush on vx_brelse call */
	ulong	vxi_btwrite;		/* 0x034 vx_btwrite called */
	ulong	vxi_buftraninval;	/* 0x038 vx_buftraninval found buffer */
	ulong	vxi_buftranhit;		/* 0x03c vx_buftraninval zapped buf */
	ulong	vxi_log;		/* 0x040 vx_log called */
	ulong	vxi_log_delayed;	/* 0x044 vx_log for delayed tran */
	ulong	vxi_log_blks;		/* 0x048 total blocks written to log */
	ulong	vxi_log_write;		/* 0x04c log writes */
	ulong	vxi_logflush;		/* 0x050 vx_logflush called */
	ulong	vxi_bmap;		/* 0x054 vx_bmap called */
	ulong	vxi_bmap_cache;		/* 0x058 vx_bmap hit in bmap cache */
	ulong	vxi_bmap_indirect;	/* 0x05c vx_bmap went into indirects */
	ulong	vxi_dirlook;		/* 0x060 vx_dirlook called */
	ulong	vxi_dirlook_dot;	/* 0x064 lookup of null name or dot */
	ulong	vxi_dirlook_dotdot;	/* 0x068 lookup of dotdot */
	ulong	vxi_dirlook_dnlc;	/* 0x06c dirlook hit in dnlc */
	ulong	vxi_dirlook_notfound;	/* 0x070 dirlook returning ENOENT */
	ulong	vxi_dirscan;		/* 0x074 dirscan rrequired */
	ulong	vxi_dirscan_seq;	/* 0x078 sequential dirscan hit */
	ulong	vxi_dirbread;		/* 0x07c dirbread had to read block */
	ulong	vxi_nodirbread;		/* 0x080 dirbread bypassed in dirscan */
	ulong	vxi_dnlc_hard;		/* 0x084 dnlc hard hold from icache */
	ulong	vxi_dirlook_fast;	/* 0x088 fast lookup */
	ulong	vxi_dirlook_slow;	/* 0x08c slow lookup */
	ulong	vxi_trunc;		/* 0x090 file removed */
	ulong	vxi_qtrunc;		/* 0x094 file removed with qtrunc */
	ulong	vxi_superwrite;		/* 0x098 write of super-block */
	ulong	vxi_readlink;		/* 0x09c VOP_READLINK called */
	ulong	vxi_access;		/* 0x0a0 VOP_ACCESS called */
	ulong	vxi_iaccess;		/* 0x0a4 vx_iaccess called */
	ulong	vxi_getattr;		/* 0x0a8 VOP_GETATTR called */
	ulong	vxi_setattr;		/* 0x0ac VOP_SETATTR called */
	ulong	vxi_setattr_nochange;	/* 0x0b0 no change in attributes */
	ulong	vxi_bufspace_tranflush;	/* 0x0b4 tran flush for buffer space */
	ulong	vxi_bufspace_delay;	/* 0x0b8 delay for buffer cache space */
	ulong	vxi_tranleft_asyncflush;/* 0x0bc async tranflush in traninit */
	ulong	vxi_tranleft_syncflush;	/* 0x0c0 sync tranflush in traninit */
	ulong	vxi_tranleft_delay;	/* 0x0c4 delay() for tran space */
	ulong	vxi_tran_space;		/* 0x0c8 total tran space used */
	ulong	vxi_tran_subfuncs;	/* 0x0cc total tran subfunctions used */
	ulong	vxi_read_throttle;	/* 0x0d0 read throttle for low mem */
	ulong	vxi_tranlogflush;	/* 0x0d4 vx_tranlogflush called */
	ulong	vxi_tranlogflush_flush;	/* 0x0d8 vx_tranlogflush flushes log */
	ulong	vxi_dirtysum;		/* 0x0dc holdmap must dirty summary */
	ulong	vxi_putmap_async;	/* 0x0e0 putmap starts async flush */
	ulong	vxi_clonemap;		/* 0x0e4 map updated with clone op */
	ulong	vxi_sumupd;		/* 0x0e8 au summary written */
	ulong	vxi_tflush_map_sync;	/* 0x0ec vx_tflush_map sync flush */
	ulong	vxi_tflush_map_async;	/* 0x0f0 vx_tflush_map async flush */
	ulong	vxi_tflush_map_clone;	/* 0x0f4 vx_tflush_map clone flush */
	ulong	vxi_tflush_cut;		/* 0x0f8 cut flush from vx_tflush_cut */
	ulong	vxi_iflush_cut;		/* 0x0fc cut flush from iupdat */
	ulong	vxi_cutwrite;		/* 0x100 current usage table writes */
	ulong	vxi_inoalloc;		/* 0x104 inode structure kmem_alloc */
	ulong	vxi_inofree;		/* 0x108 inode structure kmem_free */
	ulong	vxi_iget;		/* 0x10c vx_iget called */
	ulong	vxi_iget_loop;		/* 0x110 vx_iget hash lookup */
	ulong	vxi_iget_found;		/* 0x114 vx_iget found inode incore */
	ulong	vxi_ireclaim;		/* 0x118 vx_ireclaim called */
	ulong	vxi_ireclaim_pages;	/* 0x11c pages in vx_ireclaim */
	ulong	vxi_ireclaim_lwrites;	/* 0x120 logwrite flush in ireclaim */
	ulong	vxi_ireclaim_delbufs;	/* 0x124 delbuf flush in ireclaim */
	ulong	vxi_ireclaim_iupdat;	/* 0x128 iupdat in vx_ireclaim */
	ulong	vxi_iinactive;		/* 0x12c vx_iinactive called */
	ulong	vxi_iinactive_fast;	/* 0x130 fast vx_iinactive called */
	ulong	vxi_iinactive_slow;	/* 0x134 slow vx_iinactive called */
	ulong	vxi_iinactive_pages;	/* 0x138 pages flushed in inactive */
	ulong	vxi_iinactive_delxwri;	/* 0x13c plain delxwri in inactive */
	ulong	vxi_iinactive_ishort;	/* 0x140 shorten delxwri in inactive */
	ulong	vxi_iinactive_shorten;	/* 0x144 shorten in inactive */
	ulong	vxi_icache_clean;	/* 0x148 inactive_cache_clean called */
	ulong	vxi_icache_cleaned;	/* 0x14c ino cleaned by cache_clean */
	ulong	vxi_icache_cleaning;	/* 0x150 time in inactive_cache_clean */
	ulong	vxi_icache_flush;	/* 0x154 inactive_cache_flush called */
	ulong	vxi_icache_flushed;	/* 0x158 ino cleaned by cache_flush */
	ulong	vxi_icache_flushing;	/* 0x15c time in inactive_cache_flush */
	ulong	vxi_iupdat;		/* 0x160 vx_iupdat called */
	ulong	vxi_async_iupdat;	/* 0x164 async vx_iupdat called */
	ulong	vxi_iupdat_cluster;	/* 0x168 total inodes clustered */
	ulong	vxi_iasync_wait;	/* 0x16c vx_iasync_wait called */
	ulong	vxi_sync_delxwri;	/* 0x170 sync caused a delxwri flush */
	ulong	vxi_sync_page;		/* 0x174 sync caused a page flush */
	ulong	vxi_sync_inode;		/* 0x178 sync caused an iupdat */
	ulong	vxi_delxwri_pages;	/* 0x17c delxwri flush flushed pages */
	ulong	vxi_delxwri_async;	/* 0x180 delxwri done with async tran */
	ulong	vxi_delxwri_iupdat;	/* 0x184 delxwri done with iupdat */
	ulong	vxi_delxwri_free;	/* 0x188 delxwri done with no update */
	ulong	vxi_inode_trandone;	/* 0x18c vx_inode_trandone called */
	ulong	vxi_read_cnt;		/* 0x190 extent reads */
	ulong	vxi_write_cnt;		/* 0x194 extent writes */
	ulong	vxi_read_async;		/* 0x198 async extent reads */
	ulong	vxi_write_async;	/* 0x19c async extent writes */
	ulong	vxi_read;		/* 0x1a0 vx_read called */
	ulong	vxi_read_dio;		/* 0x1a4 a direct read */
	ulong	vxi_read_seq;		/* 0x1a8 sequential read */
	ulong	vxi_read_rand;		/* 0x1ac random read */
	ulong	vxi_write;		/* 0x1b0 vx_write called */
	ulong	vxi_write_logged;	/* 0x1b4 logged write used */
	ulong	vxi_write_donetran;	/* 0x1b8 write flushes logged writes */
	ulong	vxi_write_logonly;	/* 0x1bc logged onlywrite used */
	ulong	vxi_write_only;		/* 0x1c0 onlywrite used */
	ulong	vxi_write_dio;		/* 0x1c4 direct write */
	ulong	vxi_write_rand;		/* 0x1c8 random write */
	ulong	vxi_write_seq;		/* 0x1cc sequential write */
	ulong	vxi_async_shorten;	/* 0x1d0 async_shorten does shorten */
	ulong	vxi_async_realloc;	/* 0x1d4 async_shorten does realloc */
	ulong	vxi_immed_iinactive;	/* 0x1d8 inactive list full */
	ulong	vxi_tranidflush;	/* 0x1dc vx_tranidflush called */
	ulong	vxi_dirscan_immed;	/* 0x1e0 immediate dirscan */
	ulong	vxi_readdir;		/* 0x1e4 readdir */
	ulong	vxi_write_throttle;	/* 0x1e8 write throttle for low mem */
	ulong	vxi_tflush_iupdat;	/* 0x1ec iupdat called from tflush */
	ulong	vxi_iupdat_tran;	/* 0x1f0 vx_iupdat_tran called */
	ulong	vxi_iupdat_tran_noflush; /* 0x1f4 vx_iupdat_tran didn't flush */
	ulong	vxi_iupdat_tran_iupdat;	/* 0x1f8 vx_iupdat_tran called iupdat */
	ulong	vxi_logflush_clean;	/* 0x1fc vx_logflush cleaned logbuf */
	ulong	vxi_logflush_flush;	/* 0x200 vx_logflush wrote logbuf */
	ulong	vxi_logflush_synctran;	/* 0x204 sync tran log flush */
	ulong	vxi_logflush_flushed;	/* 0x208 log buffer already flushed */
	ulong	vxi_logflush_wait;	/* 0x20c log buffer flushed by other */
	ulong	vxi_logflush_active;	/* 0x210 log buffer waits for active */
	ulong	vxi_padding[27];	/* 0x214 pad to cache line boundary */
					/* 0x280 is length */
};

#define	VX_INFO()		(vx_info + myengnum)

#define	VX_INFO_BUMP(tag)	(VX_INFO()->tag += 1)
#define	VX_INFO_ADD(tag, count)	(VX_INFO()->tag += (count))

#ifdef	_KERNEL

extern struct vx_info	*vx_info;

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_VXFS_VX_INFO_H */
