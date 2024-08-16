/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/fifofs/fifovnops.c	1.42"
#ident	"$Header: $"

/*
 * FIFOFS file system vnode operations.  This file system
 * type supports STREAMS-based pipes and FIFOs.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/plocal.h>
#include <util/engine.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <util/sysmacros.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <fs/file.h>
#include <mem/kmem.h>
#include <io/uio.h>
#include <fs/buf.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <fs/fifofs/fifohier.h>
#include <fs/fifofs/fifonode.h>
#include <io/poll.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/stropts.h>
#include <proc/proc.h>
#include <proc/unistd.h>
#include <fs/fs_subr.h>
#include <fs/flock.h>
#include <acc/mac/mac.h>
#include <acc/dac/acl.h>

/*
 * Define the routines/data structures used in this file.
 */

STATIC int	fifo_open(vnode_t **, int, cred_t *);
int		fifo_close(vnode_t *, int, boolean_t, off_t, cred_t *);
STATIC int	fifo_read(vnode_t *, uio_t *, int, cred_t *);
STATIC int	fifo_write(vnode_t *, uio_t *, int, cred_t *);
STATIC int	fifo_ioctl(vnode_t *, int, int, int, cred_t *, int *);
STATIC int	fifo_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC int	fifo_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC int	fifo_access(vnode_t *, int, int, cred_t *);
STATIC int	fifo_link(vnode_t *, vnode_t *, char *, cred_t *);
STATIC int	fifo_fsync(vnode_t *, cred_t *);
STATIC void	fifo_inactive(vnode_t *, cred_t *);
STATIC int	fifo_fid(vnode_t *, struct fid **);
STATIC int	fifo_rwlock(vnode_t *, off_t, int, int, int);
STATIC void	fifo_rwunlock(vnode_t *, off_t, int);
STATIC int	fifo_seek(vnode_t *, off_t, off_t *);
STATIC int	fifo_realvp(vnode_t *, vnode_t **);
STATIC int	fifo_poll(vnode_t *, int, int, short *, struct pollhead **);
STATIC int	fifo_pathconf(vnode_t *, int, ulong_t *, cred_t *);
STATIC int	fifo_getacl(vnode_t *, long, long *, struct acl *,
			    cred_t *, int *);
STATIC int	fifo_setacl(vnode_t *, long, long, struct acl *, cred_t *);
STATIC int	fifo_setlevel(vnode_t *, lid_t, cred_t *);
STATIC int	fifo_msgio(vnode_t *, struct strbuf *, struct strbuf *, 
			int, unsigned char *, int, int *, rval_t *, cred_t *);
STATIC int	fifo_frlock(vnode_t *, int, flock_t *, int, off_t, cred_t *);

/*
 * Define the routines/data structures external to this file.
 */

extern	lock_t	fifolist_mutex;
extern	dev_t	fifodev;
extern	int	fifoblksize;
extern	void	fifoclearid(ino_t);
extern  int	fiforemove(struct fifonode *);
extern	int	nm_unmountall(vnode_t *, cred_t *);
extern	void	fifo_getsz(vnode_t *, vattr_t *);

extern int stropen(vnode_t *, dev_t *, vnode_t **, int,  cred_t *);
extern int strioctl(vnode_t *, int, int, int, int, cred_t *, int *);
extern int strwrite(vnode_t *, struct uio *, cred_t *);
extern int strclose(vnode_t *, int, cred_t *);
extern int strread(vnode_t *, struct uio *, cred_t *);
extern int strpoll(stdata_t *, short, int, short *, struct pollhead **);
extern void strclean(vnode_t *, boolean_t);
extern int strgetmsg(struct vnode *, struct strbuf *, struct strbuf *,
		unsigned char *, int *, int, int, rval_t *);
extern int strputmsg(struct vnode *, struct strbuf *, struct strbuf *,
		unsigned char, int, int, int, cred_t*);
extern int bcanput_l(queue_t *, uchar_t);
extern void flushq_l(queue_t *, int );

struct  streamtab fifoinfo = { &strdata, &stwdata, NULL, NULL };

vnodeops_t fifo_vnodeops = {
	fifo_open,
	fifo_close,
	fifo_read,
	fifo_write,
	fifo_ioctl,
	fs_setfl,
	fifo_getattr,
	fifo_setattr,
	fifo_access,
	(int (*)())fs_nosys,	/* lookup */
	(int (*)())fs_nosys,	/* create */
	(int (*)())fs_nosys,	/* remove */
	fifo_link,
	(int (*)())fs_nosys,	/* rename */
	(int (*)())fs_nosys,	/* mkdir */
	(int (*)())fs_nosys,	/* rmdir */
	(int (*)())fs_nosys,	/* readdir */
	(int (*)())fs_nosys,	/* symlink */
	(int (*)())fs_nosys,	/* readlink */
	fifo_fsync,
	fifo_inactive,
	(void (*)())fs_nosys,	/* release */
	fifo_fid,
	fifo_rwlock,
	fifo_rwunlock,
	fifo_seek,
	fs_cmp,
	fifo_frlock,
	fifo_realvp,
	(int (*)())fs_nosys,	/* getpage */
	(int (*)())fs_nosys,	/* putpage */
	(int (*)())fs_nosys,	/* mmap */
	(int (*)())fs_nosys,	/* addmap */
	(int (*)())fs_nosys,	/* delmap */
	fifo_poll,
	fifo_pathconf,
	fifo_getacl,
	fifo_setacl,
	fifo_setlevel,
	(int (*)())fs_nosys,	/* getdvstat */
	(int (*)())fs_nosys,	/* setdvstat */
	(int (*)())fs_nosys,	/* makemld */
	(int (*)())fs_nosys,	/* testmld */
	(int (*)())fs_nosys,	/* stablestore */
	(int (*)())fs_nosys,	/* relstore */
	(int (*)())fs_nosys,	/* getpagelist */
	(int (*)())fs_nosys,	/* putpagelist */
	fifo_msgio,
	(int (*)())fs_nosys,	/* filler[4]... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

/*
 * STATIC int fifo_open(vnode_t **vpp, int flag, cred_t *crp)
 * 	Open and stream a FIFO.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *
 * 	If this is the first open of the file (FIFO is not streaming),
 * 	initialize the fifonode and attach a stream to the vnode.
 */
STATIC int
fifo_open(vnode_t **vpp, int flag, cred_t *crp)
{
	vnode_t *oldvp = *vpp;
	struct fifonode *fnp = VTOF(oldvp);
	int error = 0;
	pl_t	pl;
	struct stdata *stp;
	queue_t *wqp;
	dev_t	pdev = 0;
	int	firstopen = 0;
	int	rwakeup = 0;
	int	wwakeup = 0;

	flag &= ~FCREAT;		/* paranoia */

	pl = PIPE_LOCK(fnp);
	/* if FIFOMACPRIV is set then bypass MAC */
	if (!(fnp->fn_flag & FIFOMACPRIV)) {
		/*
		 * Note: Ok to call MAC_VACCESS while holding lock, because
		 * VWRITE tests for lid equality, which will not sleep.
		 */
		if ((error = MAC_VACCESS(*vpp, VWRITE, crp)) != 0) {
			PIPE_UNLOCK(fnp, pl);
			return(error);
		}
	}

	if ((flag & FREAD) && (fnp->fn_rcnt++ == 0) 
			   && (SV_BLKD(&fnp->fn_rwait))) {
		rwakeup = 1;
	}

	if (flag & FWRITE) {
		if ((flag & (FNDELAY|FNONBLOCK)) && fnp->fn_rcnt == 0) {
			PIPE_UNLOCK(fnp, pl);
			return (ENXIO);
		}
		if ((fnp->fn_wcnt++ == 0) && (SV_BLKD(&fnp->fn_wwait)))
			wwakeup = 1;
	}
	fnp->fn_open++;
	if (flag & FREAD) {
		while (fnp->fn_wcnt == 0) {
			if (flag & (FNDELAY|FNONBLOCK))
				goto str;
			if (SV_WAIT_SIG(&fnp->fn_wwait, PRIPIPE, 
			    &fnp->fn_nodelp->n_lock) == B_FALSE) {
				(void)fifo_close(*vpp, flag & FMASK, B_TRUE, 
					(off_t)0, crp);
				return (EINTR);
			}
			(void)PIPE_LOCK(fnp);
		}
	}
	if (flag & FWRITE) {
		while (fnp->fn_rcnt == 0) {
			if (SV_WAIT_SIG(&fnp->fn_rwait, PRIPIPE, 
			    &fnp->fn_nodelp->n_lock) == B_FALSE) {
				(void)fifo_close(*vpp, flag & FMASK, B_TRUE, 
					(off_t)0, crp);
				return (EINTR);
			}
			(void)PIPE_LOCK(fnp);
		}
	}
str:
	while (fnp->fn_flag & FIFOWOPEN) {
		if (flag & (FNDELAY|FNONBLOCK)) {
			/*
			 * just undo what we did
			 */
			if (flag & FREAD)
				fnp->fn_rcnt--;
			if (flag & FWRITE)
				fnp->fn_wcnt--;
			fnp->fn_open--;
			PIPE_UNLOCK(fnp, pl);
			return (EAGAIN);
		}
		/* pipe lock is dropped in SV_WAIT_SIG */
		if (SV_WAIT_SIG(&fnp->fn_openwait, PRIPIPE, 
		    &fnp->fn_nodelp->n_lock) == B_FALSE) {
			(void)fifo_close(*vpp, flag & FMASK, B_TRUE, 
				(off_t)0, crp);
			return (EINTR);
		}
		(void)PIPE_LOCK(fnp);
	}
	fnp->fn_flag |= FIFOWOPEN;
	if (oldvp->v_stream == NULL) {
		firstopen++;
	}
	PIPE_UNLOCK(fnp, PLFIFO);

	if ((error = stropen(oldvp, &pdev, NULLVPP, flag, crp)) != 0) {
		(void)fifo_close(oldvp, flag & FMASK, B_TRUE, (off_t)0, crp);
		(void)PIPE_LOCK(fnp);
		fnp->fn_flag &= ~FIFOWOPEN;
		if (SV_BLKD(&fnp->fn_openwait)) {
			PIPE_UNLOCK(fnp, pl);
			SV_SIGNAL(&fnp->fn_openwait, 0);
		}
		else
			PIPE_UNLOCK(fnp, pl);
		return error;
	}
	
	(void)PIPE_LOCK(fnp);
	stp = oldvp->v_stream;
	if (firstopen) {
		/*
		 * If first open, twist the queues.
		 */
		stp->sd_wrq->q_next = RD(stp->sd_wrq);
	}

	/*
	 * If the vnode was switched (connld on the pipe), return the
	 * new vnode (in fn_unique field) to the upper layer and 
	 * release the old/original one.
	 */
	if (fnp->fn_flag & FIFOPASS) {
		*vpp = fnp->fn_unique;
		VN_LOCK(fnp->fn_unique);
		fnp->fn_unique->v_flag |= VNOMAP;
		VN_UNLOCK(fnp->fn_unique);
		fnp->fn_flag &= ~FIFOPASS;
		fnp->fn_flag &= ~FIFOWOPEN;
		if (SV_BLKD(&fnp->fn_openwait)) {
			PIPE_UNLOCK(fnp, pl);	/* unlock old fifonode   */
			SV_SIGNAL(&fnp->fn_openwait, 0);
		}
		else
			PIPE_UNLOCK(fnp, pl);	/* unlock old fifonode   */
		(void)fifo_close(oldvp, flag & FMASK, B_TRUE, (off_t)0, crp);
		VN_RELE(oldvp);
		fnp = VTOF(*vpp);
		PIPE_LOCK(fnp);			/* lock the new fifonode */
		stp = (*vpp)->v_stream;
	}

	/*
	 * set up the stream head for high capacity pipes
	 */

	(void)STREAM_LOCK(stp);
	stp->sd_flag |= OLDNDELAY;

	wqp = stp->sd_wrq;
	/* high/low water marks */
	wqp->q_hiwat = 2 * fifoblksize;
	RD(wqp)->q_hiwat = 2 * fifoblksize;
	/* pick 1/2 the high water mark /*
	wqp->q_lowat = fifoblksize;
	RD(wqp)->q_lowat = fifoblksize;

	/* set packet sizes */
	wqp->q_minpsz = 0;
	RD(wqp)->q_minpsz = 0;
	wqp->q_maxpsz = INFPSZ;
	RD(wqp)->q_maxpsz = INFPSZ;

	STREAM_UNLOCK(stp, PLFIFO);

	fnp->fn_flag &= ~FIFOWOPEN;
	if (SV_BLKD(&fnp->fn_openwait)) {
		PIPE_UNLOCK(fnp, pl);
		SV_SIGNAL(&fnp->fn_openwait, 0);
	}
	else
		PIPE_UNLOCK(fnp, pl);

	/*
	 * These broadcasts are only done if this open is successful.
	 */
	if (rwakeup)
		SV_BROADCAST(&fnp->fn_rwait, 0);
	if (wwakeup)
		SV_BROADCAST(&fnp->fn_wwait, 0);

	return (error);
}

/*
 * int 
 * fifo_close(vnode_t *vp, int flag, boolean_t lastclose, 
 * 		  off_t offset, cred_t *crp)
 * 	Close down a stream.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Call cleanlocks() and strclean() on every close.
 * 	If last close (count is <= 1 and vnode reference 
 * 	count is 1), call strclose() to close down the 
 * 	stream.
 * 	If closing a pipe, send hangup message and force 
 * 	the other end to be unmounted.
 */
/*ARGSUSED*/
int
fifo_close(vnode_t *vp, int flag, boolean_t lastclose, 
	   off_t offset, cred_t *crp)
{
	struct fifonode *fnp = VTOF(vp);
	struct fifonode *fnp2;
	struct stdata *stp;
	struct stdata *stp2;
	mblk_t *bp, *bp1;
	int error = 0;
	int rwakeup = 0;
	int wwakeup = 0;
	pl_t pl;
	int	 fdwakeup = 0;


	/*
	 * clean locks and clear events.
	 */
	if (vp->v_filocks) {
		SLEEP_LOCK(&fnp->fn_iolock, PRIPIPE);
		cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
		SLEEP_UNLOCK(&fnp->fn_iolock);
	}

	stp = vp->v_stream;

	if (stp != NULL)
		strclean(vp, B_FALSE);
	/*
	 * If a file still has the pipe/FIFO open, return.
	 */
	if (!lastclose)
		return (0);

	pl = PIPE_LOCK(fnp);
	/*
	 * Wake up any sleeping readers/writers.
	 */
	if (flag & FREAD) {
		if (--fnp->fn_rcnt == 0) {
			wwakeup = 1;
		}
	}
	if (flag & FWRITE) {
		if (--fnp->fn_wcnt == 0) {
			rwakeup = 1;
		}
	}
	fnp->fn_open--;

	/*
	 * if pipe/FIFO is not streaming, a previous open
	 * may have been interrupted.
	 */
	if (stp == NULL) {
		PIPE_UNLOCK(fnp, pl);
		return (0);
	}

	if (fnp->fn_open > 0) {
		PIPE_UNLOCK(fnp, pl);
		if (!(fnp->fn_flag & ISPIPE)) {
			if (rwakeup)
				SV_BROADCAST(stp->sd_read, KS_NOPRMPT);
			if (wwakeup)
				SV_BROADCAST(stp->sd_write, KS_NOPRMPT);
		}
		return (0);
	}

	/*
	 * set FIFOWOPEN to close a window where another open starts
	 * and fails in the middle, then calls fifo_close().
	 * This may cause another strclose() to start before
	 * this strclose() is finished.
	 */
	fnp->fn_flag |= FIFOWOPEN|FIFOWCLOSE;


	/*
	 * If no more readers and writers, tear down the stream, send
	 * hangup message to other side and force an unmount of
	 * other end.
	 */
	if ((fnp->fn_flag & ISPIPE) && fnp->fn_mate) {
		fnp2 = VTOF(fnp->fn_mate);
		fnp2->fn_mate = NULL;
		stp2 = fnp->fn_mate->v_stream;
		/*
		 * send 2 messages:
		 *	M_HANGUP to generate signal, and
		 *	M_TRAIL to mark the end of data stream.
		 * Allocate the message first. Send the messages
		 * after PIPE_LOCK is dropped.
		 */
		if ((bp = allocb(1, BPRI_MED)) != NULL) {
			/* To indicate M_TRAIL is following */
			bp->b_wptr = bp->b_rptr + 1;
			bp->b_datap->db_type = M_HANGUP;
		}
		else {
			/*
			 * We need to recover from this. Otherwise,
			 * sleeping readers on the other end may wait forever.
			 */
			if (SV_BLKD(stp2->sd_read))
				SV_BROADCAST(stp2->sd_read, 0);
			if (SV_BLKD(stp2->sd_write))
				SV_BROADCAST(stp2->sd_write, 0);
		}

		if ((bp1 = allocb(0, BPRI_MED)) != NULL) {
			bp1->b_datap->db_type = M_TRAIL;
		}
		/* 
		 * Since stream sd_mutex cannot not be locked here,
		 * checking sd_flag only provides a hint.
		 */
		if (stp2->sd_flag & STRMOUNT) {
			PIPE_UNLOCK(fnp, pl);
			if (bp)
				putnext(stp->sd_wrq, bp);
			if (bp1)
				putnext(stp->sd_wrq, bp1);
			(void) nm_unmountall(FTOV(fnp2), crp);
		}
		else {
			PIPE_UNLOCK(fnp, pl);
			if (bp)
				putnext(stp->sd_wrq, bp);
			if (bp1)
				putnext(stp->sd_wrq, bp1);
		}
	}
	else
		PIPE_UNLOCK(fnp, pl);

	error = strclose(vp, flag, crp);

	(void)PIPE_LOCK(fnp);
	if (fnp->fn_flag & FIFOSEND) {
		fnp->fn_flag &= ~FIFOSEND;
		if (SV_BLKD(&fnp->fn_fdwait)) {
			fdwakeup = 1;
		}
	}
	fnp->fn_flag &= ~(FIFOWOPEN|FIFOWCLOSE);
	if (SV_BLKD(&fnp->fn_openwait)) {
		PIPE_UNLOCK(fnp, pl);
		SV_SIGNAL(&fnp->fn_openwait, 0);
	}
	else
		PIPE_UNLOCK(fnp, pl);
	if (fdwakeup)
		FIFO_FDWAKEUP(fnp);
	return (error);
}

/*
 * STATIC int fifo_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcrp)
 * 	Read from a pipe or FIFO.
 *
 * Calling/Exit State:
 *	fifonode's io sleep lock, fn_iolock, is held on entry and
 *	remains locked on exit.
 *
 * Description:
 * 	return 0 if....
 *    		(1) user read request is 0 or no stream
 *    		(2) broken pipe with no data
 *    		(3) write-only FIFO with no data
 *    		(4) no data and delay flags set.
 * 	While there is no data to read.... 
 *   		-  if the NDELAY/NONBLOCK flag is set, return 0/EAGAIN.
 *   		-  unlock the fifonode and sleep waiting for a writer.
 *   		-  if a pipe and it has a mate, sleep waiting for its mate
 *      		to write.
 */
/*ARGSUSED*/
STATIC int
fifo_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcrp)
{
	cred_t *crp = u.u_lwpp->l_cred;
	struct fifonode *fnp = VTOF(vp);
	struct stdata *stp;
	int error = 0;
	pl_t pl_1;
	pl_t pl_2;

	stp = vp->v_stream;
	if (uiop->uio_resid == 0 || !stp)
		return (0);
	pl_1 = getpl();
	do {
		(void)PIPE_LOCK(fnp);
		pl_2 = STREAM_LOCK(stp);
		if (RD(stp->sd_wrq)->q_first == NULL) {
			if (fnp->fn_flag & ISPIPE) {
				if (!fnp->fn_mate) {
					STREAM_UNLOCK(stp, pl_2);
					PIPE_UNLOCK(fnp, pl_1);
					return (0);
				}
			} else {
				if (fnp->fn_wcnt == 0) {
					STREAM_UNLOCK(stp, pl_2);
					PIPE_UNLOCK(fnp, pl_1);
					return (0);
				}
			}
			if (uiop->uio_fmode & FNDELAY) {
				STREAM_UNLOCK(stp, pl_2);
				PIPE_UNLOCK(fnp, pl_1);
				return (0);
			}
			if (uiop->uio_fmode & FNONBLOCK) {
				STREAM_UNLOCK(stp, pl_2);
				PIPE_UNLOCK(fnp, pl_1);
				return (EAGAIN);
			}
			stp->sd_flag |= RSLEEP;
			/*
			 * PIPE_UNLOCK with PLSTR instead of pl_1
			 * because sd_mutex is still held.
			 */
			PIPE_UNLOCK(fnp, PLSTR);
			fifo_rwunlock(vp, 0, 0);
			/* SV_WAIT_SIG returns at pl0 with no lock held */
			if (SV_WAIT_SIG(stp->sd_read, PRIPIPE, stp->sd_mutex)
				== B_FALSE) {
				(void)STREAM_LOCK(stp);
				stp->sd_flag &= ~RSLEEP;
				STREAM_UNLOCK(stp, pl_1);
				(void)fifo_rwlock(vp, 0, 0, 0, 0);
				return (EINTR);
			}
			(void)fifo_rwlock(vp, 0, 0, 0, 0);
		}
		else {
			STREAM_UNLOCK(stp, pl_2);
			PIPE_UNLOCK(fnp, pl_1);
		}
	} while ((error = strread(vp, uiop, crp)) == ESTRPIPE); /* end do */

	(void)PIPE_LOCK(fnp);
	if (error == 0) {
		fnp->fn_atime = hrestime.tv_sec;
		if (fnp->fn_realvp) {
			fnp->fn_flag |= FIFOMODTIME;
		} else {
			if (fnp->fn_mate)
				VTOF(fnp->fn_mate)->fn_atime = fnp->fn_atime;
		}
	}
	PIPE_UNLOCK(fnp, pl_1);
	return (error);
}

/*
 * STATIC int fifo_write(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcrp)
 *
 *	write to a pipe or FIFO
 *
 * Calling/Exit State:
 *	fifonode's io sleep lock, fn_iolock, is held on entry and
 *	remains locked on exit.
 *
 * Description:
 *
 * 	send SIGPIPE and return EPIPE if ...
 *		(1) broken pipe
 *		(2) FIFO is not open for reading
 * 	return 0 if...
 *   		(1) no stream
 *   		(2) user request is 0 and STRSNDZERO is not set
 * 	While the stream is flow controlled.... 
 *   		-  if the NDELAY/NONBLOCK flag is set, return 0/EAGAIN.
 *   		-  unlock the fifonode and sleep waiting for a reader.
 *   		-  if a pipe and it has a mate, sleep waiting for its mate
 *      	   to read.
 */
/*ARGSUSED*/
STATIC int
fifo_write(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcrp)
{
	cred_t *crp = u.u_lwpp->l_cred;
	struct fifonode *fnp = VTOF(vp);
	int error = 0;
	int write_size = uiop->uio_resid;
	struct stdata *stp;
	pl_t pl_1;
	pl_t pl_2;

	uiop->uio_offset = 0;
	stp = vp->v_stream;
	if (stp == NULL)
		return (0);
	/* 
	 * Checking snap shots of conditions, locks are not needed
	 */
	if (fnp->fn_rcnt == 0 || (!fnp->fn_mate && fnp->fn_flag & ISPIPE)) {
		sigtolwp(u.u_lwpp, SIGPIPE, (sigqueue_t *)NULL);
		return (EPIPE);
	}

	if ((write_size == 0) && !(stp->sd_flag & STRSNDZERO)) {
		return (0);
	}

	while((error = strwrite(vp, uiop, crp)) == ESTRPIPE) {
		pl_1 = PIPE_LOCK(fnp);
		pl_2 = STREAM_LOCK(stp);
		if (!bcanput_l(stp->sd_wrq->q_next, 0)) {
			if (uiop->uio_fmode & FNDELAY) {
				STREAM_UNLOCK(stp, pl_2);
				PIPE_UNLOCK(fnp, pl_1);
				return (0);
			}
			if (uiop->uio_fmode & FNONBLOCK) {
				STREAM_UNLOCK(stp, pl_2);
				PIPE_UNLOCK(fnp, pl_1);
				if (uiop->uio_resid < write_size)
					return (0);
				else
					return (EAGAIN);
			}
			if (fnp->fn_rcnt == 0 || 
			    (!fnp->fn_mate && fnp->fn_flag & ISPIPE)) {
				sigtolwp(u.u_lwpp, SIGPIPE, (sigqueue_t *)NULL);
				STREAM_UNLOCK(stp, pl_2);
				PIPE_UNLOCK(fnp, pl_1);
				return (EPIPE);
			}
			stp->sd_flag |= WSLEEP;
			/*
			 * PIPE_UNLOCK with PLSTR instead of pl_1
			 * because sd_mutex is still held.
			 */
			PIPE_UNLOCK(fnp, PLSTR);
			fifo_rwunlock(vp, 0, 0);
			/* SV_WAIT_SIG returns at pl0 with no lock held */
			if (SV_WAIT_SIG(stp->sd_write, PRIPIPE, stp->sd_mutex)
				== B_FALSE) {
				(void)STREAM_LOCK(stp);
				stp->sd_flag &= ~WSLEEP;
				STREAM_UNLOCK(stp, pl_1);
				(void)fifo_rwlock(vp, 0, 0, 0, 0);
				return (EINTR);
			}
			(void)fifo_rwlock(vp, 0, 0, 0, 0);
			(void)PIPE_LOCK(fnp);

			/* if the pipe is broken, do not continue */
			if (fnp->fn_rcnt == 0 || 
			    (!fnp->fn_mate && fnp->fn_flag & ISPIPE)) {
				sigtolwp(u.u_lwpp, SIGPIPE, (sigqueue_t *)NULL);
				PIPE_UNLOCK(fnp, pl_1);
				return (EPIPE);
			}
			PIPE_UNLOCK(fnp, pl_1);
		}
		else {
			STREAM_UNLOCK(stp, pl_2);
			PIPE_UNLOCK(fnp, pl_1);
		}
	} /* end while */
	pl_1 = PIPE_LOCK(fnp);
	if (error == 0) {
		fnp->fn_mtime = fnp->fn_ctime = hrestime.tv_sec;
		if (fnp->fn_realvp) {
			fnp->fn_flag |= FIFOMODTIME;
		} else {
			if (fnp->fn_mate) {
				VTOF(fnp->fn_mate)->fn_mtime = fnp->fn_mtime;
				VTOF(fnp->fn_mate)->fn_ctime = fnp->fn_mtime;
			}
		}
	}
	else if (error == EIO) {
		if (fnp->fn_rcnt == 0 || 
		    (!fnp->fn_mate && fnp->fn_flag & ISPIPE)) {
			sigtolwp(u.u_lwpp, SIGPIPE, (sigqueue_t *)NULL);
			PIPE_UNLOCK(fnp, pl_1);
			return (EPIPE);
		}
	}
	PIPE_UNLOCK(fnp, pl_1);
	return (error);
}

/*
 * STATIC int fifo_ioctl(vnode_t *vp, int cmd, int arg, 
 * 			int mode, cred_t *cr, int *rvalp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Handle I_FLUSH and I_RECVFD request. All other requests are
 * 	directly sent to the stream head.
 * 	ASSUME pipe is locked
 */
STATIC int
fifo_ioctl(vnode_t *vp, int cmd, int arg, int mode, cred_t *cr,
	   int *rvalp)
{
	struct stdata *stp = vp->v_stream;
	struct fifonode *fnp = VTOF(vp);
	int error = 0;
	sv_t *wakeadr;
	pl_t pl_1;
	pl_t pl_2;

	switch (cmd) {

	default:
		return (strioctl(vp, cmd, arg, mode, U_TO_K, cr, rvalp));

	case I_FLUSH:
		if (arg & ~FLUSHRW)
			return (EINVAL);
		/*
		 * If there are modules on the stream, pass
		 * the flush request to the stream head.
		 */
		pl_1 = PIPE_LOCK(VTOF(vp));
		pl_2 = STREAM_LOCK(stp);
		if (stp->sd_wrq->q_next && 
			stp->sd_wrq->q_next->q_qinfo != &strdata) {
				STREAM_UNLOCK(stp, pl_2);
				PIPE_UNLOCK(VTOF(vp), pl_1);
				return (strioctl(vp, cmd, arg, mode, U_TO_K,
				    cr, rvalp));
		}
		/*
		 * flush the queues.
		 */
		if (arg & FLUSHR) {
			flushq_l(RD(stp->sd_wrq), FLUSHALL);
			SV_BROADCAST(vp->v_stream->sd_write, KS_NOPRMPT);
		}
		if ((arg & FLUSHW) && (stp->sd_wrq->q_next)) {
			flushq_l(stp->sd_wrq->q_next, FLUSHALL);
			if (fnp->fn_flag & ISPIPE && fnp->fn_mate)
				wakeadr = fnp->fn_mate->v_stream->sd_write;
			else
				wakeadr = vp->v_stream->sd_write;
			SV_BROADCAST(wakeadr, KS_NOPRMPT);
		}
		STREAM_UNLOCK(stp, pl_2);
		PIPE_UNLOCK(VTOF(vp), pl_1);
		break;

	/*
	 * Set the FIFOSEND flag to inform other processes that a file 
	 * descriptor is pending at the stream head of this pipe.
	 * If the flag was already set, sleep until the other
	 * process has completed processing the file descriptor.
	 *
	 * The FIFOSEND flag is set by CONNLD when it is about to
	 * block waiting for the server to recieve the file
	 * descriptor.
	 */
	case I_S_RECVFD:
	case I_E_RECVFD:
	case I_RECVFD: 
		if ((error = strioctl(vp, cmd, arg, mode, U_TO_K, cr,
		    rvalp)) == 0) {
			pl_1 = PIPE_LOCK(VTOF(vp));
			if (fnp->fn_flag & FIFOSEND) {
				fnp->fn_flag &= ~FIFOSEND;
				FIFO_FDWAKEUP(fnp);
			}
			PIPE_UNLOCK(VTOF(vp), pl_1);
		}
		break;
	}
	return (error);
}

/*
 * STATIC int fifo_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	If shadowing a vnode (FIFOs), apply the VOP_GETATTR to the shadowed 
 * 	vnode to Obtain the node information. If not shadowing (pipes),
 * 	obtain the node information from the credentials structure.
 */
STATIC int
fifo_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *crp)
{
	int error = 0;
	struct fifonode *fnp = VTOF(vp);
	pl_t	pl;

	if (fnp->fn_realvp) {
		/*
		 * for FIFOs or mounted pipes
		 */
		if (error = VOP_GETATTR(fnp->fn_realvp, vap, flags, crp))
			return (error);
		pl = PIPE_LOCK(fnp);
		vap->va_atime.tv_sec = fnp->fn_atime;
		vap->va_mtime.tv_sec = fnp->fn_mtime;
		vap->va_ctime.tv_sec = fnp->fn_ctime;
		/*
		 * check FIFOWCLOSE with PIPE_LOCK held to avoid
		 * a race between strclose() and fifo_getsz().
		 */
		if (fnp->fn_flag & FIFOWCLOSE) {
			vap->va_size = 0;
			vap->va_nblocks = 0;
		}
		else {
			/* Call fifo_getsz to determine size and nblocks. */
			fifo_getsz(vp, vap);
		}
		PIPE_UNLOCK(fnp, pl);
		vap->va_atime.tv_nsec = 0;
		vap->va_mtime.tv_nsec = 0;
		vap->va_ctime.tv_nsec = 0;
	} else {
		/*
		 * for non-attached/ordinary pipes
		 */
		vap->va_mode = 0;
		pl = PIPE_LOCK(fnp);
		vap->va_atime.tv_sec = fnp->fn_atime;
		vap->va_mtime.tv_sec = fnp->fn_mtime;
		vap->va_ctime.tv_sec = fnp->fn_ctime;
		PIPE_UNLOCK(fnp, pl);
		/*
		 * No need to hold PIPE_LOCK because the stream 
		 * cannot not be closed at this moment for pipes.
		 */
		fifo_getsz(vp, vap);
		vap->va_atime.tv_nsec = 0;
		vap->va_mtime.tv_nsec = 0;
		vap->va_ctime.tv_nsec = 0;
		vap->va_uid = crp->cr_uid;
		vap->va_gid = crp->cr_gid;
		vap->va_nlink = 0;
		vap->va_fsid = fifodev;
		vap->va_nodeid = fnp->fn_ino;
		vap->va_rdev = 0;
		if (vap->va_mask & AT_ACLCNT) {
			vap->va_aclcnt = NACLBASE;
		}
	}
	vap->va_type = VFIFO;
	vap->va_blksize = fifoblksize;
	vap->va_vcode = 0;
	return (0);
}

/*
 * STATIC int fifo_setattr(vnode_t *vp, vattr_t *vap, 
 *			   int flags, int ioflags, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	If shadowing a vnode, apply the VOP_SETATTR to it.
 * 	Otherwise, set the time and return 0.
 */
STATIC int
fifo_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *crp)
{
	struct fifonode *fnp = VTOF(vp);
	pl_t	pl;
	int error = 0;

	if (fnp->fn_realvp)
		error = VOP_SETATTR(fnp->fn_realvp, vap, flags, ioflags, crp);
	if (error == 0) {
		/*
		 * If times were changed, update fifonode.
		 */
		pl = PIPE_LOCK(fnp);
		if (vap->va_mask & AT_ATIME) {
			if (flags & ATTR_UPDTIME) {
				if (vap->va_atime.tv_sec > fnp->fn_atime) {
					fnp->fn_atime = vap->va_atime.tv_sec;
				}
			}
			else
				fnp->fn_atime = vap->va_atime.tv_sec;
		}
		if (vap->va_mask & AT_MTIME) {
			if (flags & ATTR_UPDTIME) {
				if (vap->va_mtime.tv_sec > fnp->fn_mtime) {
					fnp->fn_mtime = vap->va_mtime.tv_sec;
				}
			}
			else
				fnp->fn_mtime = vap->va_mtime.tv_sec;
			fnp->fn_ctime = hrestime.tv_sec;
		} else if (vap->va_mask & (AT_MODE | AT_UID | AT_GID)) {
			fnp->fn_ctime = hrestime.tv_sec;
		}
		PIPE_UNLOCK(fnp, pl);
	}
	return (error);
}

/*
 * STATIC int fifo_access(vnode_t *vp, int mode, int flags, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *
 * 	If shadowing a vnode, apply VOP_ACCESS to it.
 * 	Otherwise, return 0 (allow all access).
 */
STATIC int
fifo_access(vnode_t *vp, int mode, int flags, cred_t *crp)
{

	if (VTOF(vp)->fn_realvp)
		return (VOP_ACCESS(VTOF(vp)->fn_realvp, mode, flags, crp));
	else
		return (0);
}

/*
 * STATIC int fifo_link(vnode_t *tdvp, vnode_t *vp, char *tnm, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	If shadowing a vnode, apply the VOP_LINK to it.
 * 	Otherwise, return ENOENT.
 */
STATIC int
fifo_link(vnode_t *tdvp, vnode_t *vp, char *tnm, cred_t *crp)
{

	if (VTOF(vp)->fn_realvp)
		return (VOP_LINK(tdvp, VTOF(vp)->fn_realvp, tnm, crp));
	else
		return (ENOENT);
}

/*
 * STATIC int fifo_fsync(vnode_t *vp, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	If shadowing a vnode, do time update if applicable,
 *	then apply the VOP_FSYNC to it.
 * 	Otherwise, return 0.
 */
STATIC int
fifo_fsync(vnode_t *vp, cred_t *crp)
{
	struct fifonode *fnp = VTOF(vp);
	struct vattr va;
	pl_t pl;

	if (fnp->fn_realvp) {
		if (fnp->fn_flag & FIFOMODTIME) {
			va.va_mask = AT_ATIME|AT_MTIME;
			pl = PIPE_LOCK(fnp);
			fnp->fn_flag &= ~FIFOMODTIME;
			va.va_atime.tv_sec = fnp->fn_atime;
			va.va_mtime.tv_sec = fnp->fn_mtime;
			PIPE_UNLOCK(fnp, pl);
			va.va_atime.tv_nsec = 0;
			va.va_mtime.tv_nsec = 0;
			(void)VOP_SETATTR(fnp->fn_realvp, &va, 
					ATTR_UPDTIME, 0, crp);
		}
		return (VOP_FSYNC(fnp->fn_realvp, crp));
	}
	else
		return (0);
}

/*
 * STATIC void fifo_inactive(vnode_t *vp, cred_t *crp)
 *
 * Calling/Exit State:
 *	If there is no race for the vnode, the vnode will be freed.
 *
 * Description:
 * 	Called when the upper level no longer holds references to the
 * 	vnode. Sync the file system and free the fifonode.
 */
STATIC void
fifo_inactive(vnode_t *vp, cred_t *crp)
{
	struct fifonode *fnp = VTOF(vp);
	pl_t pl;

	if (fnp->fn_realvp) {
		SLEEP_LOCK(&fnp->fn_iolock, PRIPIPE);
		if (vp->v_count != 1) {
			VN_LOCK(vp);
			if (vp->v_count != 1) {
				vp->v_count--;
				VN_UNLOCK(vp);
				FIFO_UNLOCK(fnp);
				return;
			}
			VN_UNLOCK(vp);
		}
		ASSERT(vp->v_count == 1);
		vp->v_count = 0;
		pl = LOCK(&fifolist_mutex, PLFIFO);
		if (fiforemove(fnp) != 0) {
			UNLOCK(&fifolist_mutex, pl);
			FIFO_UNLOCK(fnp);
			return;
		}
		UNLOCK(&fifolist_mutex, pl);
		FIFO_UNLOCK(fnp); 
		(void) fifo_fsync(vp, crp);
		VN_RELE(fnp->fn_realvp);
	}
	if (fnp->fn_nodelp && --fnp->fn_nodelp->n_count == 0) {
		LOCK_DEINIT(&fnp->fn_nodelp->n_lock);
		kmem_free((caddr_t)fnp->fn_nodelp, sizeof(struct nodelock));
		if (fnp->fn_flag & ISPIPE)
			fifoclearid(fnp->fn_ino);
	}
	SLEEP_DEINIT(&fnp->fn_iolock);
	VN_DEINIT(vp);
	kmem_free((caddr_t)fnp, sizeof(struct fifonode));
}

/*
 * STATIC int fifo_fid(vnode_t *vp, struct fid **fidfnp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	If shadowing a vnode, apply the VOP_FID to it.
 * 	Otherwise, return EINVAL.
 */
STATIC int
fifo_fid(vnode_t *vp, struct fid **fidfnp)
{
	if (VTOF(vp)->fn_realvp)
		return (VOP_FID(VTOF(vp)->fn_realvp, fidfnp));
	else
		return (EINVAL);
}

/*
 * STATIC int fifo_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 * 	Lock a fifonode.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
/*ARGSUSED*/
STATIC int
fifo_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	struct fifonode *fnp = VTOF(vp);

	SLEEP_LOCK(&fnp->fn_iolock, PRINOD);
	return(0);
}

/*
 * STATIC void fifo_rwunlock(vnode_t *vp, off_t off, int len)
 * 	Unlock a fifonode.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
/*ARGSUSED*/
STATIC void
fifo_rwunlock(vnode_t *vp, off_t off, int len)
{
	struct fifonode *fnp = VTOF(vp);

	SLEEP_UNLOCK(&fnp->fn_iolock);
}

/*
 * STATIC int fifo_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag,
 *			  off_t offset, cred_t *cr)
 * 	File/Record lock a fifonode.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
/*ARGSUSED*/
STATIC int
fifo_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag,
	    off_t offset, cred_t *cr)
{
	struct fifonode *fnp = VTOF(vp);
	vattr_t	va;
	int	error;

	SLEEP_LOCK(&fnp->fn_iolock, PRINOD);
	/*
	 * Call fifo_getsz to determine size and nblocks.
	 */
	fifo_getsz(vp, &va);

	error = fs_frlock(vp, cmd, bfp, flag, offset, cr, va.va_size);
	SLEEP_UNLOCK(&fnp->fn_iolock);
	return(error);
}
/*
 * STATIC int fifo_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 * 	Return error since seeks are not allowed on pipes.
 */
/*ARGSUSED*/
STATIC int
fifo_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	return (ESPIPE);
}

/*
 * STATIC int fifo_realvp(vnode_t *vp, vnode_t **vpp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 * 	If there is a realvp associated with vp, return it.
 */
STATIC int
fifo_realvp(vnode_t *vp, vnode_t **vpp)
{
	struct fifonode *fnp = VTOF(vp);
	struct vnode *rvp;

	if ((vp = fnp->fn_realvp) != NULL)
		if (VOP_REALVP(vp, &rvp) == 0)
			vp = rvp;
	*vpp = vp;
	return (0);
}

/*
 * STATIC int fifo_poll(vnode_t *vp, int events, int anyyet, short *reventsp, 
 *			struct pollhead **phpp)
 * 	Poll for interesting events on a stream pipe
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC int
fifo_poll(vnode_t *vp, int events, int anyyet, short *reventsp, 
	  struct pollhead **phpp)
{
	if (!vp->v_stream)
		return (EINVAL);
	return (strpoll(vp->v_stream, events, anyyet, reventsp, phpp));
}

/*
 * STATIC int fifo_getacl(vnode_t *vp, long nentries, long *dentriesp,
 *			  struct acl *aclbufp, cred_t *cr, int *rvalp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 * 	This routine returns the ACL for a FIFO.
 */
STATIC int
fifo_getacl(vnode_t *vp, long nentries, long *dentriesp,
	    struct acl *aclbufp, cred_t *cr, int *rvalp)
{
	struct fifonode *fnp = VTOF(vp);

	if (fnp->fn_realvp)	/* is a fifo */
		return (VOP_GETACL(fnp->fn_realvp, nentries,
				dentriesp, aclbufp, cr, rvalp));
	else
		return (ENOSYS);

}

/*
 * STATIC int fifo_setacl(vnode_t *vp, long nentries, long dentriesp,
 * 			  struct acl *aclbufp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	This routine sets the ACL for a FIFO.
 */
STATIC int
fifo_setacl(vnode_t *vp, long nentries, long dentriesp,
	    struct acl *aclbufp, cred_t *cr)
{
	struct fifonode *fnp = VTOF(vp);

	if (fnp->fn_realvp)	/* is a fifo */
		return (VOP_SETACL(fnp->fn_realvp, nentries,
				dentriesp, aclbufp, cr));
	else
		return (ENOSYS);
}

/*
 * STATIC int fifo_setlevel(vnode_t *vp, lid_t level, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	This routine sets the real vnode and fifonode level.
 * 	This call is only supported for fifos.
 * 	Tranquility is enforced by checking fn_open.
 */
/*ARGSUSED*/
STATIC int
fifo_setlevel(vnode_t *vp, lid_t level, cred_t *crp)
{
	struct fifonode *fnp = VTOF(vp);
	int error;

	if (fnp->fn_realvp) {		/* is a fifo */
		/*
		 * Check tranquility only when MAC is installed.
		 */
		if (mac_installed && fnp->fn_open)
			error = EBUSY;
		else {	
			error = VOP_SETLEVEL(fnp->fn_realvp, level, crp);
			if (error == 0)
				vp->v_lid = level;
		}
	} else				/* is a pipe */
		error = ENOSYS;

	return (error);
}

/*
 * STATIC int
 * fifo_pathconf(vnode_t *vp, int cmd, ulong_t *valp, cred_t *cr)
 * 	POSIX pathconf() support.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Return the fifofs definition for _PC_PIPE_BUF and for
 * 	named pipes we switch-out to file system specific
 * 	routine where the named pipe resides.
 */
/*ARGSUSED*/
STATIC int
fifo_pathconf(vnode_t *vp, int cmd, ulong_t *valp, cred_t *cr)
{
	int error = 0;
	struct fifonode *fnp = VTOF(vp);
	ulong_t val;

	if ((cmd != _PC_PIPE_BUF) && fnp->fn_realvp) {
		error = VOP_PATHCONF(fnp->fn_realvp, cmd, &val, cr);
	} else {

		switch (cmd) {

		case _PC_LINK_MAX:
		case _PC_MAX_CANON:
		case _PC_MAX_INPUT:
		case _PC_NAME_MAX:
		case _PC_PATH_MAX:
		case _PC_NO_TRUNC:
		case _PC_VDISABLE:
		case _PC_CHOWN_RESTRICTED:
			error = EINVAL;
			break;

		case _PC_PIPE_BUF:
			val = fifoblksize;
			break;

		default:
			error = EINVAL;
			break;
		}
	}

	if (error == 0)
		*valp = val;
	return error;
}

/*
 * int
 * fifo_msgio(vnode_t *vp, struct strbuf *mctl, struct strbuf *mdata,
 *		int mode, unsigned char *prip, int fmode, int *flagsp,
 *		rval_t *rvp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	This routine will call strputmsg/strgetmsg.
 */
/*ARGSUSED*/
int
fifo_msgio(vnode_t *vp, struct strbuf *mctl, struct strbuf *mdata,
		int mode, unsigned char *prip, int fmode, int *flagsp,
		rval_t *rvp, cred_t *cr)
{

	if (mode == FREAD)
		return(strgetmsg(vp, mctl, mdata, prip, flagsp, 
				fmode, U_TO_K, rvp));
	else
		return(strputmsg(vp, mctl, mdata, *prip, *flagsp, 
			fmode, U_TO_K, cr));
}
