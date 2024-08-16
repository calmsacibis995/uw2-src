/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/osm/osm.c	1.3"
#ident	"$Header: $"

/*
 *	OSM - operating system messages, allows system printf's to
 *	be read via special file.
 *
 *	minor 0: starts from beginning of buffer and waits for more.
 *	minor 1: starts from beginning of buffer but doesn't wait.
 *	minor 2: starts at current buffer position and waits.
 *
 *	Writes to all minor numbers append to the end of the buffer.
 */

#include <fs/file.h>
#include <io/conf.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>	/* MUST COME LAST */

MOD_DRV_WRAPPER(osm, NULL, NULL, NULL, "osm - O/S message driver");

extern char putbuf[];		/* system putchar circular buffer */
extern int putbufsz;		/* size of above */
extern int putbufwpos;		/* next position for system putchar */
extern ulong_t putbufwrap;	/* # times putbufwpos has wrapped around */
extern lock_t putbuf_lock;	/* lock for putbuf, et al. */

int osmdevflag = D_MP;

#define OSM_MAX_MSGSZ	1024	/* maximum size of message sent upstream */

/* flag bits in minor number: */
#define OSM_NOWAIT	0x01	/* return EOF when no more available */
#define OSM_CURPOS	0x02	/* start at current buffer position */

#define OSM_MAXUNIT	2	/* maximum legal unit number */

STATIC int osmopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp);
STATIC int osmclose(queue_t *q, int flag, cred_t *crp);
STATIC void osmtimer(void *arg);
STATIC int osmwput(queue_t *q, mblk_t *mp);

STATIC struct module_info osm_info = {
	66,	/* mi_idnum */
	"osm",	/* mi_idname */
	0,	/* mi_minpsz */
	INFPSZ,	/* mi_maxpsz */
	0,	/* mi_hiwat */
	0	/* mi_lowat */
};

STATIC struct qinit osmrinit = {
	NULL,
	NULL,
	osmopen,
	osmclose,
	NULL,
	&osm_info,
	NULL
};

STATIC struct qinit osmwinit = {
	osmwput,
	NULL,
	NULL,
	NULL,
	NULL,
	&osm_info,
	NULL
};

struct streamtab osminfo = {
	&osmrinit,
	&osmwinit
};

struct osm {
	off_t	osm_roff;	/* logical read offset in putbuf */
	toid_t	osm_toid;	/* timeout ID */
	minor_t	osm_unit;	/* which device is open */
};

/* ARGSUSED */
STATIC int
osmopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	minor_t unit;
	struct osm *osmp;
	int error;
	pl_t opl;

	/* Return success if already opened */
	if (q->q_ptr != NULL)
		return 0;

	if (sflag != 0 || (oflag & FEXCL))
		return EINVAL;

	unit = getminor(*devp);
	if (unit > OSM_MAXUNIT)
		return ENODEV;

	osmp = kmem_alloc(sizeof(struct osm), KM_SLEEP);
	q->q_ptr = osmp;

	if (unit & OSM_CURPOS) {
		opl = LOCK(&putbuf_lock, plhi);
		osmp->osm_roff = putbufwrap * putbufsz + putbufwpos;
		UNLOCK(&putbuf_lock, opl);
	} else
		osmp->osm_roff = 0;

	osmp->osm_unit = unit;

	osmp->osm_toid = itimeout(osmtimer, q, 1 | TO_PERIODIC, plbase);
	if (osmp->osm_toid == 0) {
		kmem_free(osmp, sizeof(struct osm));
		return ENOSPC;
	}

	qprocson(q);

	return 0;
}


/* ARGSUSED */
STATIC int
osmclose(queue_t *q, int flag, cred_t *crp)
{
	struct osm *osmp = q->q_ptr;

	untimeout(osmp->osm_toid);
	qprocsoff(q);
	kmem_free(osmp, sizeof(struct osm));

	return 0;
}


STATIC void
osmtimer(void *arg)
{
	queue_t *rq = arg;
	struct osm *osmp = rq->q_ptr;
	off_t woff, roff;
	uint_t rboff, sz;
	mblk_t *mp;
	pl_t opl;

more:
	roff = osmp->osm_roff;

	/*
	 * This has to be done in several stages, since we can't call
	 * streams routines while holding putbuf_lock.  First see if
	 * there's anything to do.
	 */
	opl = LOCK(&putbuf_lock, plhi);
	woff = putbufwrap * putbufsz + putbufwpos;
	if (roff >= woff && !(osmp->osm_unit & OSM_NOWAIT)) {
		UNLOCK(&putbuf_lock, opl);
		return;
	}
	UNLOCK(&putbuf_lock, opl);

	/*
	 * If no more room upstream, give up for now.
	 */
	if (!canputnext(rq))
		return;

	/*
	 * We now have a lower bound on the number of bytes available.
	 * We don't have an exact number, since we dropped the lock.
	 * Allocate a message block sized accordingly.
	 */
	sz = OSM_MAX_MSGSZ;
	if (woff - roff < OSM_MAX_MSGSZ)
		sz = woff - roff;
	mp = allocb(sz, BPRI_MED);
	if (mp == NULL) {
		/* Allocation failed; try again later */
		return;
	}

	opl = LOCK(&putbuf_lock, plhi);
	/* Re-compute woff after re-acquiring the lock */
	woff = putbufwrap * putbufsz + putbufwpos;
	if (woff - roff > putbufsz) {
		/*
		 * The buffer wrapped around before we were able to pull
		 * it all out, so we lost some data.  Start at the oldest
		 * character left.
		 */
		roff = woff - putbufsz;
	}
	if (sz > woff - roff)
		sz = woff - roff;
	if ((rboff = roff % putbufsz) + sz > putbufsz)
		sz = putbufsz - rboff;
	if (sz != 0)
		bcopy(putbuf + rboff, mp->b_wptr, sz);
	UNLOCK(&putbuf_lock, opl);

	mp->b_wptr += sz;
	osmp->osm_roff = roff + sz;

	/* Put the message up the stream */
	putnext(rq, mp);

	/* Try for more */
	if (sz != 0)
		goto more;
}


/* ARGSUSED */
STATIC int
osmwput(queue_t *q, mblk_t *mp)
{
	pl_t opl;
	mblk_t *bp;

	switch (mp->b_datap->db_type) {
	case M_DATA:
		bp = mp;
		opl = LOCK(&putbuf_lock, plhi);
		while (bp) {
			while (bp->b_rptr < bp->b_wptr) {
				if (putbufwpos >= putbufsz) {
					putbufwpos = 0;
					++putbufwrap;
				}
				putbuf[putbufwpos++] = *bp->b_rptr++;
			}
			bp = bp->b_cont;
		}
		UNLOCK(&putbuf_lock, opl);
		freemsg(mp);
		break;
	case M_IOCTL:
		mp->b_datap->db_type = M_IOCNAK;
		qreply(q, mp);
		break;
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR) {
			*mp->b_rptr &= ~FLUSHW;
			qreply(q, mp);
		} else
			freemsg(mp);
		break;
	default:
		freemsg(mp);
		break;
	}

	return 0;
}
