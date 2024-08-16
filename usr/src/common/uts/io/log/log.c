/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/log/log.c	1.22"
#ident	"$Header: $"

/*
 * Streams log driver
 */

#include <fs/file.h>
#include <io/conf.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <io/syslog.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <io/ddi.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

STATIC int logopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int logclose(queue_t *, int, cred_t *);
STATIC int logwput(queue_t *, mblk_t *);
STATIC int logrsrv(queue_t *);
STATIC void logtrc(mblk_t *);
STATIC void logerr(mblk_t *);
STATIC void logcons(mblk_t *);
STATIC int logtrace(struct log *, short, short, char);
STATIC int shouldtrace(short, short, char);
STATIC int sendmsg(struct log *, mblk_t *);
void dostrlog(mblk_t *);
void loginit(void);
int strlog(short, short, char, ushort_t, char *, ...);
int canconslog(void);

STATIC struct module_info logm_info = {
	LOG_MID, LOG_NAME, LOG_MINPS, LOG_MAXPS, LOG_HIWAT, LOG_LOWAT
};

STATIC struct qinit logrinit = {
	NULL, logrsrv, logopen, logclose, NULL, &logm_info, NULL
};

STATIC struct qinit logwinit = {
	logwput, NULL, NULL, NULL, NULL, &logm_info, NULL
};

struct streamtab loginfo = { &logrinit, &logwinit, NULL, NULL };

/*
 * set security attributes to have device state as public and
 * no MAC access checks for data transfer
 */
int logdevflag = (D_MP|D_INITPUB|D_NOSPECMACDATA);

STATIC int log_errseq;		/* logger sequence numbers */
STATIC int log_trcseq;
STATIC int log_conseq;

STATIC int numlogtrc;		/* number of processes reading trace log */
STATIC int numlogerr;		/* number of processes reading error log */
STATIC int numlogcons;		/* number of processes reading console log */

extern struct log log_log[];	/* log device state table */
extern int log_cnt;		/* number of minor devices */
extern ulong_t loghiwat;	/* tunable high water mark */
extern ulong_t loglowat;	/* tunable low water mark */

extern char    putbuf[];
extern int     putbufsz;
extern int     putbufrpos;
extern int     putbufwpos;
extern ulong_t putbufwrap;
extern lock_t  putbuf_lock;

STATIC rwlock_t *log_rwlock;
	/*+ to protect log_log[], numlogtrc, numlogerr and numlogcons */
STATIC LKINFO_DECL(logrw_lkinfo, "ID:LOG:log_rwlock", LK_BASIC);
STATIC lock_t	*log_lock;
	/*+ to avoid getq/putq race on a queue */
STATIC LKINFO_DECL(log_lkinfo, "ID:LOG:log_lock", LK_BASIC);

#define	LOG_HIER	1

/*
 * void
 * loginit(void)
 *	LOG DRIVER INITIALIZATION ROUTINE
 *
 * Calling/Exit State:
 *	Called during system initialization to initialize locks,
 *	global variables, and data structures.
 *
 * Remarks:
 *	Initialization of  the following lock are assumed to be
 *	done in cmn_err_init():
 *	- putbuf_lock
 */
void
loginit(void)
{
	int i;

	log_rwlock = RW_ALLOC(LOG_HIER, plstr, &logrw_lkinfo, KM_NOSLEEP);
	log_lock = LOCK_ALLOC(LOG_HIER+1, plstr, &log_lkinfo, KM_NOSLEEP);
	if (log_rwlock == NULL || log_lock == NULL) {
		if (log_rwlock) {
			RW_DEALLOC(log_rwlock);
			log_rwlock = NULL;
		}
		if (log_lock == NULL) {
			LOCK_DEALLOC(log_lock);
			log_lock = NULL;
		}
	}
	logm_info.mi_hiwat = loghiwat;
	logm_info.mi_lowat = loglowat;
	for (i = 0; i < log_cnt; i++)
		log_log[i].log_state = 0;
}

/* ARGSUSED */
/*
 * STATIC int
 * logopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *cr)
 *	LOG DRIVER OPEN ROUTINE
 *
 * Calling/Exit State:
 *	Two ways to get here:
 *	- normal access for loggers (/dev/log has minor CLONEMIN)
 *	  is through the clone minor; only one user per clone minor.
 *	- access to writing to the console log (/dev/conslog has
 *	  minor CONSWMIN) is through the console minor; users cannot
 *	  read from this device; any number of users can have it
 *	  open at one time.
 *	It returns:
 *	0	for successful open; *devt is changed [/dev/log].
 *	0	/dev/conslog already open on the same Q.
 *	EINVAL	attempt to open /dev/conslog for read (FREAD)
 *	ENXIO	sflag is not equal to 0 (CLONEOPEN)
 *	ENXIO	cannot find unused minor of /dev/log; like EBUSY
 *	ENXIO	minor is neither of /dev/log nor of /dev/conslog
 */
STATIC int
logopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *cr)
{
	int i;
	struct log *lp;
	pl_t pl;

	if (sflag)
		return ENXIO;

	if (log_lock == NULL)
		return ENOSPC;

	switch (getminor(*devp)) {

	case CONSWMIN:
		if (flag & FREAD)	/* you can only write to this minor */
			return EINVAL;
		if (q->q_ptr)		/* already open */
			return 0;
		pl = RW_WRLOCK(log_rwlock, plstr);
		lp = &log_log[CONSWMIN];
		lp->log_state = LOGOPEN;
		lp->log_rdq = q;
		RW_UNLOCK(log_rwlock, pl);
		break;

	case CLONEMIN:
		/*
		 * Find an unused minor > CLONEMIN.
		 */
		pl = RW_WRLOCK(log_rwlock, plstr);
		for (i = CLONEMIN+1, lp = &log_log[i]; i < log_cnt; i++, lp++) {
			if (! (lp->log_state & LOGOPEN))
				break;
		}
		if (i >= log_cnt) {
			RW_UNLOCK(log_rwlock, pl);
#ifdef CC_PARTIAL
			CC_COUNT(CC_RE_LOG, CCBITS_RE_LOG);
#endif
			return ENXIO;
		}
		lp->log_state = LOGOPEN;
		lp->log_rdq = q;
		RW_UNLOCK(log_rwlock, pl);
		*devp = makedevice(getemajor(*devp), i);
		break;

	default:
		return ENXIO;
	}
	/*
	 * Finish device initialization.
	 */
	q->q_ptr = (caddr_t) lp;
	WR(q)->q_ptr = (caddr_t) lp;
	qprocson(q);
	return 0;
}

/* ARGSUSED */
/*
 * STATIC int
 * logclose(queue_t *q, int flag, cred_t *cr)
 *	LOG DRIVER CLOSE ROUTINE
 *
 * Calling/Exit State:
 *	Always returns 0
 */
STATIC int
logclose(queue_t *q, int flag, cred_t *cr)
{
	struct log *lp;
	pl_t pl;

	qprocsoff(q);

	lp = (struct log *) q->q_ptr;
	ASSERT(lp);
	pl = RW_WRLOCK(log_rwlock, plstr);
	if (lp->log_state & LOGTRC) {
		freemsg(lp->log_tracemp);
		lp->log_tracemp = NULL;
		numlogtrc--;
	}
	if (lp->log_state & LOGERR)
		numlogerr--;
	if (lp->log_state & LOGCONS)
		numlogcons--;
	lp->log_state = 0;
	lp->log_rdq = NULL;
	RW_UNLOCK(log_rwlock, pl);
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	return 0;
}

/*
 * STATIC int
 * logwput(queue_t *q, mblk_t *bp)
 *	LOG DRIVER WRITE PUT PROCEDURE
 *
 * Calling/Exit State:
 *	Following types of messages are processed:
 *	- M_FLUSH:
 *		flush the Qs as specified.
 *	- M_IOCTL/I_CONSLOG [ioctl(2) to /dev/log]:
 *		set the Q as a console logger; may send the contents
 *		of putbuf[] to all console loggers including this one.
 *	- M_IOCTL/I_TRCLOG [ioctl(2) to /dev/log]:
 *		set the Q as a trace logger; holds the second half
 *		of the given message as list of trace_ids which is to
 *		be used to determine a message should be traced or not.
 *	- M_IOCTL/I_ERRLOG [ioctl(2) to /dev/log]:
 *		set the Q as a error logger.
 *	- M_PROTO [putmsg(2)]:
 *		sends the message to specified types of loggers.
 *	- M_DATA [write(2) to /dev/conslog]:
 *		forms a log message and sends to console loggers.
 *	Other types of message are discarded.
 *
 * Remarks:
 *	Should not call with putbuf_lock acquired.
 */
STATIC int
logwput(queue_t *q, mblk_t *bp)
{
	int s;
	struct iocblk *iocp;
	struct log *lp;
	struct log_ctl *lcp;
	mblk_t *cbp, *pbp;
	int size;
	pl_t pl, pl_2;

	lp = (struct log *) q->q_ptr;
	switch (bp->b_datap->db_type) {

	case M_FLUSH:
		if (*bp->b_rptr & FLUSHW) {
			flushq(q, FLUSHALL);
			*bp->b_rptr &= ~FLUSHW;
		}
		if (*bp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHALL);
			qreply(q, bp);
		} else {
			freemsg(bp);
		}
		break;

	case M_IOCTL:
		pl = RW_WRLOCK(log_rwlock, plstr);
		if (lp == &log_log[CONSWMIN])
			/* can not ioctl CONSWMIN */
			goto lognak;
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) bp->b_rptr;
		if (iocp->ioc_count == TRANSPARENT)
			goto lognak;
		switch (iocp->ioc_cmd) {

		case I_CONSLOG:
			if (lp->log_state & LOGCONS) {
				iocp->ioc_error = EBUSY;
				goto lognak;
			}
			numlogcons++;
			lp->log_state |= LOGCONS;

			if (putbufrpos == putbufwpos)
				/* nothing to send in putbuf[] */
				goto logack;
			if (! (cbp = allocb(sizeof(struct log_ctl), BPRI_HI))) {
				iocp->ioc_error = EAGAIN;
				lp->log_state &= ~LOGCONS;
				numlogcons--;
				goto lognak;
			}
			if ((size = putbufwpos - putbufrpos) < 0)
				size += putbufsz;
			ASSERT(size > 0);
			if (! (pbp = allocb(size, BPRI_HI))) {
				freeb(cbp);
				iocp->ioc_error = EAGAIN;
				lp->log_state &= ~LOGCONS;
				numlogcons--;
				goto lognak;
			}
			RW_UNLOCK(log_rwlock, pl);
			pl_2 = LOCK(&putbuf_lock, plhi);
			while (putbufrpos != putbufwpos && size-- > 0) {
				*pbp->b_wptr++ = putbuf[putbufrpos++];
				if (putbufrpos >= putbufsz)
					putbufrpos = 0;
			}
			UNLOCK(&putbuf_lock, pl_2);
			if (pbp->b_wptr == pbp->b_rptr) {
				freemsg(cbp);
				goto logack_u;
			}
			cbp->b_datap->db_type = M_PROTO;
			cbp->b_cont = pbp;
			cbp->b_wptr += sizeof(struct log_ctl);
			/* LINTED pointer alignment */
			lcp = (struct log_ctl *) cbp->b_rptr;
			lcp->mid = LOG_MID;
			lcp->sid = lp - log_log;
			(void) drv_getparm(LBOLT, &lcp->ltime);
			(void) drv_getparm(TIME, &lcp->ttime);
			lcp->level = 0;
			lcp->flags = SL_CONSOLE;
			lcp->seq_no = (long) log_conseq++;
			/*
			 * NOTE: there is no way to give a correct priority
			 * (that is, facility|level) to the message obtained
			 * from putbuf[]; workaround here is that to handle
			 * it as strlog() message with SL_CONSOLE only.  May
			 * need future work to keep consistency for users
			 * such as syslogd(1M).
			 */
			lcp->pri = LOG_KERN|LOG_INFO;
			pl = RW_RDLOCK(log_rwlock, plstr);
			for (s = CLONEMIN+1, lp = &log_log[s]; s < log_cnt; s++, lp++)
				if (lp->log_state & LOGCONS)
					(void) sendmsg(lp, cbp);
			freemsg(cbp);
			goto logack;

		case I_TRCLOG:
			if (! (lp->log_state & LOGTRC) && bp->b_cont) {
				lp->log_tracemp = bp->b_cont;
				bp->b_cont = NULL;
				numlogtrc++;
				lp->log_state |= LOGTRC;
				goto logack;
			}
			iocp->ioc_error = EBUSY;
			goto lognak;

		case I_ERRLOG:
			if (! (lp->log_state & LOGERR)) {
				numlogerr++;
				lp->log_state |= LOGERR;
logack:
				RW_UNLOCK(log_rwlock, pl);
logack_u:
				iocp->ioc_count = 0;
				bp->b_datap->db_type = M_IOCACK;
				qreply(q, bp);
				break;
			}
			iocp->ioc_error = EBUSY;
			goto lognak;

		default:
lognak:
			RW_UNLOCK(log_rwlock, pl);
			bp->b_datap->db_type = M_IOCNAK;
			qreply(q, bp);
			break;
		}
		break;

	case M_PROTO:
		if (((bp->b_wptr - bp->b_rptr) != sizeof(struct log_ctl)) ||
		    ! bp->b_cont) {
			freemsg(bp);
			break;
		}
		/* LINTED pointer alignment */
		lcp = (struct log_ctl *) bp->b_rptr;
		pl = RW_RDLOCK(log_rwlock, plstr);
		if (lcp->flags & SL_ERROR) {
			if (numlogerr == 0)
				lcp->flags &= ~SL_ERROR;
		}
		if (lcp->flags & SL_TRACE) {
			if ((numlogtrc == 0) || ! shouldtrace(LOG_MID,
			    (struct log *)(q->q_ptr) - log_log, lcp->level))
				lcp->flags &= ~SL_TRACE;
		}
		if (! (lcp->flags & (SL_ERROR|SL_TRACE|SL_CONSOLE))) {
			RW_UNLOCK(log_rwlock, pl);
			freemsg(bp);
			break;
		}
		(void) drv_getparm(LBOLT, &lcp->ltime);
		(void) drv_getparm(TIME, &lcp->ttime);
		lcp->mid = LOG_MID;
		lcp->sid = (struct log *) q->q_ptr - log_log;
		if (lcp->flags & SL_TRACE)
			(void) logtrc(bp);
		if (lcp->flags & SL_ERROR)
			(void) logerr(bp);
		if (lcp->flags & SL_CONSOLE) {
			if ((lcp->pri & LOG_FACMASK) == LOG_KERN)
				lcp->pri |= LOG_USER;
			logcons(bp);
		}
		RW_UNLOCK(log_rwlock, pl);
		freemsg(bp);
		break;

	case M_DATA:
		if (lp != &log_log[CONSWMIN]) {
			bp->b_datap->db_type = M_ERROR;
			if (bp->b_cont) {
				freemsg(bp->b_cont);
				bp->b_cont = NULL;
			}
			bp->b_rptr = bp->b_datap->db_base;
			bp->b_wptr = bp->b_rptr + sizeof(char);
			*bp->b_rptr = EIO;
			qreply(q, bp);
			break;
		}
		/*
		 * allocate message block for proto
		 */
		if (! (cbp = allocb(sizeof(struct log_ctl), BPRI_HI))) {
			freemsg(bp);
			break;
		}
		cbp->b_datap->db_type = M_PROTO;
		cbp->b_cont = bp;
		cbp->b_wptr += sizeof(struct log_ctl);
		/* LINTED pointer alignment */
		lcp = (struct log_ctl *) cbp->b_rptr;
		lcp->mid = LOG_MID;
		lcp->sid = CONSWMIN;
		(void) drv_getparm(LBOLT, &lcp->ltime);
		(void) drv_getparm(TIME, &lcp->ttime);
		lcp->level = 0;
		lcp->flags = SL_CONSOLE;
		lcp->pri = LOG_USER|LOG_INFO;
		pl = RW_RDLOCK(log_rwlock, plstr);
		logcons(cbp);
		RW_UNLOCK(log_rwlock, pl);
		freemsg(cbp);
		break;

	default:
		freemsg(bp);
		break;
	}
	return 0;
}

/*
 * STATIC int
 * logrsrv(queue_t *q)
 *	LOG DRIVER READ SERVICE PROCEDURE
 *
 * Calling/Exit State:
 *	Send a log message up a given log stream.
 */
STATIC int
logrsrv(queue_t *q)
{
	mblk_t *mp;

	while (mp = getq(q)) {
		if (! canputnext(q)) {
			putbq(q, mp);
			break;
		}
		putnext(q, mp);
	}
	return 0;
}

/* PRINTFLIKE5 */
/*
 * int
 * strlog(short mid, short sid, char level, ushort_t flags, char *fmt, ...)
 * 	Kernel logger interface function
 *
 * Calling/Exit State:
 *	It attempts to construct a log message and send it up the
 *	logger stream.  Delivery will not be done if message blocks
 *	cannot be allocated or if the logger is not registered
 *	(exception is console logger).
 *	It returns 0 if a message is not seen by a reader, either
 *	because nobody was reading or an allocation failed; returns
 *	1 otherwise.
 *
 * Remarks:
 *	This routine can be called by any of the kernel components,
 *	while driver-defined basic locks, read/write locks, sleep locks,
 *	or read/write sleep locks held.
 */
int
strlog(short mid, short sid, char level, ushort_t flags, char *fmt, ...)
{
	char *dst, *src;
	int i;
	VA_LIST argp;
	struct log_ctl *lcp;
	mblk_t *dbp, *cbp;

	if (log_lock == NULL)
		return 0;

	if (! (flags & (SL_ERROR|SL_TRACE|SL_CONSOLE)))
		return 0;
	/*
	 * allocate message blocks for log text, log header, and
	 * proto control field.
	 */
	if (! (dbp = allocb(LOGMSGSZ, BPRI_HI)))
		return 0;
	if (! (cbp = allocb(sizeof(struct log_ctl), BPRI_HI))) {
		freeb(dbp);
		return 0;
	}
	/*
	 * estimate the memory size which will be allocated in
	 * dostrlog() via dupb() and copyb().
	 */
	if (! testb((sizeof(mblk_t) + sizeof(struct log_ctl)) *
		    (numlogerr + numlogtrc + numlogcons), BPRI_HI)) {
		freeb(cbp);
		freeb(dbp);
		return 0;
	}
	/*
	 * copy log text into text message block.  This consists of a
	 * format string and NLOGARGS integer arguments.
	 */
	dst = (char *) dbp->b_wptr;
	src = (char *) fmt;
	logstrcpy(dst, src);
	/*
	 * dst now points to the null byte at the end of the format string.
	 * Move the wptr to the first int boundary after dst.
	 */
	dbp->b_wptr = (uchar_t *) logadjust(dst);

	ASSERT((int)(dbp->b_datap->db_lim-dbp->b_wptr) >= NLOGARGS*sizeof(int));

	VA_START(argp, fmt);

	for (i = 0; i < NLOGARGS; i++) {
		/* LINTED pointer alignment */
		*((int *) dbp->b_wptr) = VA_ARG(argp, int);
		dbp->b_wptr += sizeof(int);
	}
	/*
	 * set up proto header
	 */
	cbp->b_datap->db_type = M_PROTO;
	cbp->b_cont = dbp;
	cbp->b_wptr += sizeof(struct log_ctl);
	cbp->b_flag |= MSGLOG;
	/* LINTED pointer alignment */
	lcp = (struct log_ctl *) cbp->b_rptr;
	lcp->mid = mid;
	lcp->sid = sid;
	(void) drv_getparm(LBOLT, &lcp->ltime);
	(void) drv_getparm(TIME, &lcp->ttime);
	lcp->level = level;
	lcp->flags = flags;
	/*
	 * give it to the daemon
	 */
	strdolog(cbp);
	return 1;
}

/*
 * void
 * dostrlog(mblk_t *cbp);
 * 	latter half of strlog()
 *
 * Calling/Exit State:
 *	It attempts to send the given message up the logger stream.
 *	Delivery will not be done if the logger is not registered
 *	(exception is console logger).
 */
void
dostrlog(mblk_t *cbp)
{
	struct log_ctl *lcp;
	pl_t pl;

	/* LINTED pointer alignment */
	lcp = (struct log_ctl *) cbp->b_rptr;

	pl = RW_RDLOCK(log_rwlock, plstr);
	if (lcp->flags & SL_TRACE) {
		if ((numlogtrc > 0) &&
		    shouldtrace(lcp->mid, lcp->sid, lcp->level)) {
			lcp->pri = LOG_KERN|LOG_DEBUG;
			(void) logtrc(cbp);
		}
	}
	if (lcp->flags & SL_ERROR) {
		if (numlogerr > 0) {
			lcp->pri = LOG_KERN|LOG_ERR;
			(void) logerr(cbp);
		}
	}
	if (lcp->flags & SL_CONSOLE) {
		if (lcp->flags & SL_FATAL)
			lcp->pri = LOG_KERN|LOG_CRIT;
		else if (lcp->flags & SL_ERROR)
			lcp->pri = LOG_KERN|LOG_ERR;
		else if (lcp->flags & SL_WARN)
			lcp->pri = LOG_KERN|LOG_WARNING;
		else if (lcp->flags & SL_NOTE)
			lcp->pri = LOG_KERN|LOG_NOTICE;
		else if (lcp->flags & SL_TRACE)
			lcp->pri = LOG_KERN|LOG_DEBUG;
		else
			lcp->pri = LOG_KERN|LOG_INFO;
		logcons(cbp);
	}
	RW_UNLOCK(log_rwlock, pl);
	freemsg(cbp);
	return;
}

/*
 * STATIC int
 * logtrace(struct log *lp, short mid, short sid, char level)
 *	An internal routine for shouldtrace()
 *
 * Calling/Exit State:
 *	Checks <mid>, <sid>, and <level> against list of values
 *	requested by processes reading trace messages.
 *	Returns 1 if the given mid or sid is found, or the given
 *	level is lower than the one listed.  Returns 0 otherwise.
 */
STATIC int
logtrace(struct log *lp, short mid, short sid, char level)
{
	struct trace_ids *tid;
	int i;
	int ntid;

	ASSERT(lp->log_tracemp);
	/* LINTED pointer alignment */
	tid = (struct trace_ids *) lp->log_tracemp->b_rptr;
	ntid = (long) (lp->log_tracemp->b_wptr - lp->log_tracemp->b_rptr) /
	    sizeof(struct trace_ids);
	for (i = 0; i < ntid; tid++, i++) {
		if ((tid->ti_level < level) && (tid->ti_level >= 0))
			continue;
		if ((tid->ti_mid != mid) && (tid->ti_mid >= 0))
			continue;
		if ((tid->ti_sid != sid) && (tid->ti_sid >= 0))
			continue;
		return 1;
	}
	return 0;
}

/*
 * STATIC int
 * shouldtrace(short mid, short sid, char level)
 *	An internal routine for logwput() and strlog(D3DK)
 *
 * Calling/Exit State:
 *	Returns 1 if someone wants to see the trace message for the
 *	given module id, sub-id, and level.  Returns 0 otherwise.
 */
STATIC int
shouldtrace(short mid, short sid, char level)
{
	struct log *lp;
	int i;

	for (i = CLONEMIN+1, lp = &log_log[i]; i < log_cnt; i++, lp++)
		if ((lp->log_state & LOGTRC) && lp->log_tracemp &&
		    logtrace(lp, mid, sid, level))
			return 1;
	return 0;
}

/*
 * STATIC int
 * sendmsg(struct log *lp, mblk_t *mp)
 *	Send a log message to a reader.
 *
 * Calling/Exit State:
 *	It attempts to pull out messages from the specified Q, when
 *	the Q is full, before puts the given message to the Q.
 *	Returns 1 if the message was sent, and 0 otherwise.
 */
STATIC int
sendmsg(struct log *lp, mblk_t *mp)
{
	pl_t pl;
	mblk_t *bp2, *mp2;
	long qflag;

	if (! (mp2 = copyb(mp)))
		return 0;
	if (! (bp2 = dupb(mp->b_cont))) {
		freeb(mp2);
		return 0;
	}
	mp2->b_cont = bp2;
	/*
	 * NOTE: wanted to hold the lock at plhi.  Freezing given stream to
	 * get q_flag value, however, attempts to hold sd_mutex at plstr
	 * (which is less than plhi).   _LOCKTEST disallows to lower ipl.
	 */
	pl = LOCK(log_lock, plstr); /* plhi */
	for (;;) {
		(void) freezestr(lp->log_rdq);
		(void) strqget(lp->log_rdq, QFLAG, 0, &qflag);
		unfreezestr(lp->log_rdq, plstr); /* plhi */
		if (! (qflag & QFULL))
			break;
		freemsg(getq(lp->log_rdq));
	}
	putq(lp->log_rdq, mp2);
	UNLOCK(log_lock, pl);
	return 1;
}

/*
 * STATIC void
 * logtrc(mblk_t *mp)
 *	Log a trace message.
 *
 * Calling/Exit State:
 *	Returns 1 if everyone sees the message, and 0 otherwise.
 *
 * Remarks:
 *	Assumes log_rwlock acquired on entry.
 */
STATIC void
logtrc(mblk_t *mp)
{
	int i;
	struct log *lp;
	struct log_ctl *lcp;

	/* LINTED pointer alignment */
	lcp = (struct log_ctl *) mp->b_rptr;
	lcp->seq_no = (long) log_trcseq++;
	for (i = CLONEMIN+1, lp = &log_log[i]; i < log_cnt; i++, lp++)
		if (lp->log_state & LOGTRC)
			(void) sendmsg(lp, mp);
}

/*
 * STATIC void
 * logerr(mblk_t *mp)
 *	Log an error message.
 *
 * Calling/Exit State:
 *	Returns 1 if everyone sees the message, and 0 otherwise.
 *
 * Remarks:
 *	Assumes log_rwlock acquired on entry.
 */
STATIC void
logerr(mblk_t *mp)
{
	int i;
	struct log *lp;
	struct log_ctl *lcp;

	/* LINTED pointer alignment */
	lcp = (struct log_ctl *) mp->b_rptr;
	lcp->seq_no = (long) log_errseq++;
	for (i = CLONEMIN+1, lp = &log_log[i]; i < log_cnt; i++, lp++)
		if (lp->log_state & LOGERR)
			(void) sendmsg(lp, mp);
}

/*
 * STATIC void
 * logcons(mblk_t *mp)
 *	Log a console message.
 *
 * Calling/Exit State:
 *	It copies contents of the given message (from the second
 *	message block) to putbuf[].
 *	Returns 1 if everyone sees the message, and 0 otherwise.
 *
 * Remarks:
 *	Assumes log_rwlock acquired on entry.
 *	Should not called with putbuf_lock acquired on entry.
 */
STATIC void
logcons(mblk_t *mp)
{
	int i;
	uchar_t *cp;
	struct log *lp;
	mblk_t *bp;
	struct log_ctl *lcp;
	int nlog = 0;
	int didsee = 0;
	int rpos, wpos;
	pl_t pl;

	pl = LOCK(&putbuf_lock, plhi);
	/*
	 * Assumption that only short messages get logged to the
	 * console.  That's why we don't use bcopy().
	 */
	bp = mp;
	for (mp = mp->b_cont; mp; mp = mp->b_cont) {
		cp = mp->b_rptr;
		while (cp < mp->b_wptr) {
			if (putbufwpos >= putbufsz) {
				putbufwpos = 0;
				++putbufwrap;
			}
			putbuf[putbufwpos++] = *cp++;
		}
	}
	rpos = putbufrpos;
	wpos = putbufwpos;
	UNLOCK(&putbuf_lock, pl);
	/* LINTED pointer alignment */
	lcp = (struct log_ctl *) bp->b_rptr;
	lcp->seq_no = (long) log_conseq++;
	for (i = CLONEMIN+1, lp = &log_log[i]; i < log_cnt; i++, lp++)
		if (lp->log_state & LOGCONS) {
			nlog++;
			didsee += sendmsg(lp, bp);
		}
	/*
	 * while we are sending the message out, somebody might move the
	 * putbufrpos/putbufwpos.  Here:
	 * - rpos != putbufrpos indicates cmn_err() wrote to putbuf[], or
	 *   another I_CONSLOG read from putbuf[].  In either case,
	 *   putbufrpos points to the same place as putbufwpos does.
	 * - rpos == putbufrpos indicates nobody reads/writes putbuf[], or
	 *   another logcons() call might append further message to putbuf[]
	 *   and move putbufwpos.  In these cases, we want to set putbufrpos
	 *   to the place where (previously) putbufwpos points to.
	 */
	pl = LOCK(&putbuf_lock, plhi);
	if (didsee && rpos == putbufrpos)
		putbufrpos = wpos;
	UNLOCK(&putbuf_lock, pl);
}

/*
 * int
 * canconslog(void)
 *	Examine console loggers exist or not.
 *
 * Calling/Exit State:
 *	Returns 1 if there exists at least one console logger;
 *	returns 0 otherwise.
 *
 * Description:
 *	This routine is used to examine console logging can be enabled.
 *	When console logging is disabled all console messages are sent
 *	directly to the system console.  We need to disable console
 *	logging during some kernel functions, for instance when the
 *	system enters debug mode.
 *	This is not a part of DDI/DKI, but is shared with cmn_err(D3DK)
 *	family functions as well as with those of AA:AUDIT.
 *
 * Remarks:
 *	This information will potentially be stale and is only a hint.
 *	As such, no locking is needed.  However, calling cmn_err() in
 *	this routine causes:
 *		PANIC: recursive acquisition of lock
 */
int
canconslog(void)
{
	return ((numlogcons > 0) ? 1 : 0);
}
