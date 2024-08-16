/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_param.h	2.23 04 Oct 1994 01:24:47 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_param.h	1.15"

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

#ifndef _FS_VXFS_VX_PARAM_H
#define _FS_VXFS_VX_PARAM_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#ifndef _UTIL_PARAM_H
#include <util/param.h> /* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/param.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Block size info
 */
#define	VX_MINBSIZE	1024	/* smallest block on any vxfs fs */
#define	VX_MINBOFFMASK	0x3ff	/* BOFFMASK for VX_MINBSIZE */
#define	VX_MINBSHIFT	10	/* BSHIFT for VX_MINBSIZE */
#define	VX_MAXBSIZE	8192	/* largest block on any vxfs fs */

#define IADDREXTSIZE	8192	/* indirect address extent size */
#define IADDREXTSHIFT	13	/* shift for IADDREXTSIZE */
#define	NINDEXT		2048	/* num of addresses per indirect address ext */
#define	NINDEXTSHIFT	11	/* log 2 (NINDEXT) */

/*
 * The VX_ATOMIC_IOSZ is the unit of I/O which is guaranteed atomic.
 * This should be the smallest sector size of any device a file
 * system might reside on.  The VX_ATOMIC_IOSZ value is used to
 * break up log entries so no entry crosses an atomic I/O boundary.
 */

#define	VX_ATOMIC_IOSZ		512
#define	VX_ATOMIC_IOSHIFT	9

/*
 * The VX_MAXISIZE is the maximum size inode we support.  It must
 * be no larger than VX_ATOMIC_IOSZ.  Also, this must be small enough
 * so the size of a vx_imtran is less than VX_TRANPGSIZE (with space
 * for two inodes included).
 */

#define	VX_MAXISIZE		512

/*
 * The VX_MAX_DIO parameter is the maximum size of I/O operations issued
 * each time through the inner loop of the direct I/O code.  The VX_MAXIO
 * parameter is the largest I/O operation generated by the rest of the
 * file system.  Both must be a multiple of VX_MAXBSIZE.
 */

#define	VX_MAXIO	(64 * 1024)
#define	VX_MAX_DIO	(64 * 1024)

/*
 * When getpage is called, this is the maximum number of pages it will
 * obtain in one call.
 */

#define	VX_MAXGETPAGES		16

/*
 * When sequential asynchronous writes are issued to a file the default
 * write code will do an asynchronous flush behind every vx_prefio
 * bytes.  In low memory conditions, a synchronous flush behind is issued
 * at VX_MAXDIRTY behind the current write point.  This limits a file to
 * VX_MAXDIRTY bytes worth of dirty pages when memory is low.
 *
 * The VX_MAXDISKQ is used to limit flushing.  Building up a large disk
 * queue on one file can delay accesses for other files.  By limiting the
 * disk queue, we can maintain enough outstanding I/O to get maximum
 * streaming performance, while not blocking other accesses for long
 * periods of time.
 */

#define	VX_MAXDIRTY		(128 * 1024)
#define	VX_MAXDISKQ		(256 * 1024)

/*
 * Various time delays.  The caching policies try to use time as an aid
 * to scalability.  For example, the VX_ICLEAN_TIMELAG is a measure of
 * how long in time we want the inode freelist to be.  If the inodes are
 * being reused in less than VX_ICLEAN_TIMELAG seconds, then we want to
 * increase the size of the inode table.  If the current workload is
 * heavy, the number of inodes used in VX_ICLEAN_TIMELAG seconds may be
 * very large.  If the workload is light, then the number of inodes used
 * will be smaller.
 *
 * VX_SYNC_TIME is the amount of time it takes the vx_iflush thread to
 * sync out the inode table.
 *
 * VX_ICLEAN_TIMELAG is the minimum length of time we want inodes on
 * the freelist before reclaim.  If we are trying to reclaim inodes in
 * less time, then the inode table size is increased.
 *
 * VX_IFREE_TIMELAG is the maximum length of time we want inodes on
 * the freelist.  If inodes are staying on the freelist more than this,
 * then the inode table size is reduced.
 *
 * VX_IINACTIVE_TIMELAG is the length of time we want inodes to sit on
 * the inactive list or the dirty pages list.
 *
 * VX_IINACTIVE_DELAY_TIMELAG is the length of time we will delay
 * inactive calls on inodes.  This allows lookup to reuse the inodes
 * without having to pull them off the freelist.
 * 
 * VX_DELAYLOG_TIMELAG is the maximum length of time we will let a
 * delayed transaction sit in a log buffer without the log being flushed.
 */

#define	VX_SYNC_TIME			60
#define	VX_ICLEAN_TIMELAG		70
#define	VX_IFREE_TIMELAG		150
#define	VX_IINACTIVE_TIMELAG		10
#define	VX_IINACTIVE_DELAY_TIMELAG	10
#define	VX_DELAYLOG_TIMELAG		4

/*
 * When the total number of transactions gets too large, we start flushing
 * file systems with more than VX_REPLAY_TRAN transactions outstanding.
 */

#define	VX_REPLAY_TRAN		250

/*
 * To balance file system usage, when the file system free space falls
 * below VX_MINFREE we will stop allocating very large extents.  When
 * an allocation unit falls below VX_AUFREE[i] blocks free we will start
 * allocating inodes into other allocation units with VX_AUFREE[i+1] blocks.
 */

#define	VX_MINFREE	10	/* minimum pct of fs that we want free */
#define	VX_AUFREE1	5
#define	VX_AUFREE2	10
#define	VX_AUFREE3	20
#define	VX_AUFREE4	30

/*
 * This controls the maximum number of inodes that may cluster as a
 * side inode update in vx_iupdat().
 */

#define	VX_MAXICLUSTER	16

/*
 * This controls the maximum number of snapshot file systems that can
 * be mounted simultaneously.  It is the number of minor device numbers
 * that will be allocated by the vx_dev_alloc() routine.
 */

#define	VX_DEV_MAXMINOR	256

/*
 * This value is used to increment the CUT entry version number.
 */

#define	VX_CUTINCR	20000

/*
 * The maximum number of bytes that can be changed when a
 * file is reorganized using VX_EXTMAP_REORG.
 */

#define	VX_MAXREALLOC_SZC	(16 * 1024 * 1024)

/*
 * vxfs message structure
 */

struct vx_msg {
	int	msg_type;	/* cmn_err message type */
	caddr_t	msg_string;	/* message text */
};

/*
 * vxfs message codes
 */

#define	VX_MSG_NOMSG		0
#define	VX_MSG_NOSPACE		1
#define	VX_MSG_WROFS		2
#define	VX_MSG_BADEMAP		3
#define	VX_MSG_BADIMAP		4
#define	VX_MSG_BADIEMAP		5
#define	VX_MSG_BADSUMMARY	6
#define	VX_MSG_BADISUMMARY	7
#define	VX_MSG_DIRERR		8
#define	VX_MSG_IDIRERR		9
#define	VX_MSG_INOTFREE		10
#define	VX_MSG_NOINODES		11
#define	VX_MSG_BADINUM		12
#define	VX_MSG_BADIEXT		13
#define	VX_MSG_ENFILE		14
#define	VX_MSG_IBADFAIL		15
#define	VX_MSG_IREADERR		16
#define	VX_MSG_MARKIBAD		17
#define	VX_MSG_DELXWRIERR	18
#define	VX_MSG_LOGOVERFLOW	19
#define	VX_MSG_LOGBAD		20
#define	VX_MSG_MOUNTSETUP	21
#define	VX_MSG_ROOTREMOUNT	22
#define	VX_MSG_UNMOUNTROOT	23
#define	VX_MSG_CUTFAIL		24
#define	VX_MSG_BADSUPER		25
#define	VX_MSG_SNAPREAD		26
#define	VX_MSG_SNAPWRITE	27
#define	VX_MSG_SNAPNOSPACE	28
#define	VX_MSG_SNAPMAPWERR	29
#define	VX_MSG_SNAPMAPRERR	30
#define	VX_MSG_DISABLE		31
#define	VX_MSG_SNAPDISABLE	32
#define	VX_MSG_BADBLOCK		33
#define	VX_MSG_RESETLOG		34
#define	VX_MSG_INACTIVE		35
#define	VX_MSG_BADLCT		36
#define	VX_MSG_METAIOERR	37
#define	VX_MSG_DATAIOERR	38
#define	VX_MSG_BADSUPER2	39

/*
 * For compatibility with earlier release of SVR4 and with other
 * operating systems (and to avoid even more casts in the code then
 * we already have), have kmem_alloc() return a caddr_t.
 */

#ifdef	TED_
#define	VX_ALLOC(size, flags)	(caddr_t) xted_alloc((size), (flags))	/*TED_*/
#define	VX_ZALLOC(size, flags)	(caddr_t) xted_zalloc((size), (flags))	/*TED_*/
#define	VX_FREE(ptr, size)	xted_free((ptr), (size))		/*TED_*/
#else	/* not TED_ */							/*TED_*/
#define	VX_ALLOC(size, flags)	(caddr_t) kmem_alloc((size), (flags))
#define	VX_ZALLOC(size, flags)	(caddr_t) kmem_zalloc((size), (flags))
#define	VX_FREE(ptr, size)	kmem_free((ptr), (size))
#endif									/*TED_*/

/*
 * Certain allocations are freed outside the file system and as
 * such cannot have a debug version.
 */

#define	VX_RZALLOC(size, flags)	(caddr_t) kmem_zalloc((size), (flags))

#if defined(__cplusplus)
	}
#endif

#endif  /* _FS_VXFS_VX_PARAM_H */
