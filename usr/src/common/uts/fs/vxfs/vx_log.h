/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* @(#)usr/src/common/uts/fs/vxfs/vx_log.h	2.16 04 Oct 1994 10:34:22 - Copyright (c) 1994 VERITAS Software Corp. */
#ident	"@(#)kern:fs/vxfs/vx_log.h	1.9"

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

#ifndef _FS_VXFS_VX_LOG_H
#define _FS_VXFS_VX_LOG_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Generic log record - variable length
 * Log record consists of 1 or more functions constituting a
 * transaction. Each function in the transaction has the same
 * log_id value. The log_ser value is incremented for each function.
 * The log record for a function is always a multiple of 16 bytes
 * and does not cross a VX_ATOMIC_IOSZ boundary. The log_len is always the
 * exact number of data bytes, and does not include padding. If the
 * dependent data will cause a VX_ATOMIC_IOSZ boundary to be crossed, a
 * continuation record is used. The continuation records have the
 * same log_id, log_func, log_ser fields, but have an incrementing log_fser
 * field. Since many log records are short, one log block may contain
 * multiple transactions.
 *
 * log_id is the transaction id number, which is an incrementing serial
 * number held from fs_logid. If the id wraps to zero, it is incremented
 * to one, so an id of zero indicates nothing there. There should never
 * be a missing number in the log. A change in log_id indicates that a
 * new transaction has begun.
 *
 * log_func is the function type
 *
 * log_ser is the function number within the transaction (0-127)
 * log_lser is the last function number within the transaction
 *
 * log_fser is the continuation record number within the function (0-127)
 * log_lfser is the last continuation record number within the function
 * a continuation record is used whenever a VX_ATOMIC_IOSZ boundary is crossed
 *
 * log_len is the number of data bytes in the record. If a function has
 * continuation records, the number of data bytes in the function is the
 * sum of the log_len fields. Note that records are always a multiple of
 * 16 bytes, but the log_len field is always exact.
 */

#define	VX_LOGFILL	4			/* round to 16 bytes */

struct vx_log {
	long		log_id;			/* transaction id number */
	short		log_func;		/* function */
	char		log_ser;		/* serial # within trans */
	char		log_lser;		/* last serial # within trans */
	char		log_fser;		/* serial # within func */
	char		log_lfser;		/* last serial # within func */
	short		log_len;		/* data length */
	char		log_depend[VX_LOGFILL];	/* variable */
};

#define VX_LOGMASK	(sizeof (struct vx_log) - 1)
#define VX_LOGOVER	(sizeof (struct vx_log) - VX_LOGFILL)
#define VX_LOGMAX	(VX_ATOMIC_IOSZ - VX_LOGOVER)

#define VX_LOGROUNDUP(len) \
	(((len) + VX_LOGOVER + VX_LOGMASK)  & ~VX_LOGMASK)

/*
 * This is the size of a done or undo subfunction.  To make sure we
 * don't cross any sector boundaries when putting done records into the
 * log, we round the buffer offset to a VX_DONELOGSIZE boundary.  The
 * code assumes that VX_ATOMIC_IOSZ is a multiple of VX_DONELOGSIZE.
 * The done/undo subfunction contains 8 bytes of data.
 */

#define VX_DONELOGSIZE	0x20
#define VX_DONELOGSHIFT	5

/*
 * When the logid reaches 1 billion, we reset the logid at the next
 * 60 second sync interval.  By doing this, we can avoid dealing
 * with logid's that wrap.  If the reset hasn't happened by the
 * time 500 million more transactions occur, then the file system
 * will be disabled.
 *
 * We treat the log offset in the same manner.  When it reaches 1 billion
 * sectors, we reset the log.
 */

#define	VX_MAXLOGID	(1 << 30)
#define	VX_DISLOGID	(VX_MAXLOGID + (1 << 29))
#define	VX_MAXLOGOFF	(1 << 30)
#define	VX_DISLOGOFF	(VX_MAXLOGID + (1 << 29))

/*
 * Structure for log buffer management.
 */

struct vx_logbuf {
	struct vx_logbuf	*lb_next;	/* next buf in active list */
	struct vx_logbuf	*lb_prev;	/* prev buf in active list */
	struct vx_tran		*lb_logq;	/* q of trans logged in buf */
	struct vx_tran		*lb_lastlogq;	/* last entry on lb_logq */
	struct vx_tran		*lb_undoq;	/* q of undo records in buf */
	struct vx_tran		*lb_doneq;	/* q of done records in buf */
	struct vx_fs		*lb_fs;		/* fs that owns buf */
	struct buf		*lb_bp;		/* buffer header */
	caddr_t			lb_addr;	/* buffer space */
	long			lb_logoff;	/* position of buffer in log */
	int			lb_flags;	/* flags - see below */
	int			lb_error;	/* error writing log */
	int			lb_len;		/* length of buffer */
	int			lb_off;		/* current end of buffer */
	int			lb_holdcnt;	/* hold count on log buffer */
	int			lb_minid;	/* min tran id in log buffer */
	int			lb_maxid;	/* max tran id in log buffer */
};

/*
 * Flag values for log buffers.
 */

#define	VX_LB_DONE	0x01	/* log I/O is done */
#define	VX_LB_FLOGBUF	0x02	/* buffer covers first sector of log */
#define	VX_LB_FLUSHER	0x04	/* buffer has a synchronous flusher */

/*
 * This value is the default size of log buffers.
 */

extern int vx_logbufszc;

#ifndef	TED_
#define VX_CURLB_LOCK(fs)		LOCK((fs)->fs_curlb_lkp, PLHI)
#else									/*TED_*/
#define	VX_CURLB_LOCK(fs) 		xted_curlb_lock(fs)		/*TED_*/
#endif									/*TED_*/

#define	VX_CURLB_UNLOCK(fs, ipl) {					\
	XTED_CURLB_UNLOCK(fs, ipl);				/*TED_*/\
	UNLOCK((fs)->fs_curlb_lkp, ipl);				\
}

#define VX_CURLB_SLEEP(fs) {						\
	XTED_CURLB_UNLOCK(fs, PLBASE);					\
	SV_WAIT(fs->fs_curlb_svp, PRINOD + 2, fs->fs_curlb_lkp);	\
}

#define VX_CURLB_WAKEUP(fs)	SV_BROADCAST(fs->fs_curlb_svp, vx_noprmpt)

#define VX_CURLB_MEMSLEEP(fs) {						\
	XTED_CURLB_UNLOCK(fs, PLBASE);					\
	SV_WAIT(fs->fs_curlbmem_svp, PRINOD + 2, fs->fs_curlb_lkp);	\
}

#define VX_CURLB_MEMWAKEUP(fs)	SV_BROADCAST(fs->fs_curlbmem_svp, vx_noprmpt)

#define VX_TLOGDONE_SLEEP(fs) {						\
	XTED_TRANQ_UNLOCK(fs, PLBASE);					\
	SV_WAIT(fs->fs_tranlog_svp, PRINOD + 2, fs->fs_tranq_lkp);	\
}

#define VX_TLOGDONE_WAKEUP(fs)	SV_BROADCAST(fs->fs_tranlog_svp, vx_noprmpt)

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_VXFS_VX_LOG_H */
