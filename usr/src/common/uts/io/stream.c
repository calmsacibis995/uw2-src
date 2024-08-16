/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/stream.c	1.55"

#include <io/conf.h>
#include <io/stream.h>
#include <io/strmdep.h>
#include <io/strstat.h>
#include <io/strsubr.h>
#include <mem/anon.h>
#include <mem/kmem.h>
#include <mem/memresv.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/mod/mod_hier.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * This file contains all the STREAMS utility routines that may be used
 * by modules and drivers.  Locked (func_l) versions of functions are not
 * advertised and are not yet in the DDI/DKI.  They are included in this
 * file so they are near the unlocked (advertised) versions.
 */

extern char *strbceng;
extern volatile char strbcrunning;
extern int Nsched;
extern struct striopst *strioclist;
STATIC void flushband_l(queue_t *, unsigned char, int);

STATIC toid_t strbcid;	/* bufcall ids */

extern rwlock_t mod_cdevsw_lock, mod_fmodsw_lock;

#ifdef STRLEAK
extern fspin_t strleak_mutex;
mblk_t *strleakhead;
long strleakcnt;
struct strleak {
	long	sl_addr;	/* calling address */
	long	sl_cnt;		/* number of times in list */
};
#define STRLEAKSZ	100
struct strleak Strleak[STRLEAKSZ];	/* for tabulation purposes */
#endif

extern	long *Strcount_local;

/*
 * NOTE: Some code in streamio.c relies on knowledge of the underlying
 * message-data-buffer pool structure.
 */

/*
 * mblk_t *
 * allocb(int size, uint_t pri)
 *	Allocate a message.  No locking is necessary because this is
 *	creating the first reference to the message.
 *
 * Calling/Exit State:
 *	'pri' is unused, but is maintained
 *	for compatibility.  A size of -1 is special and should only
 *	be used by esballoc(), which has the responsibility of completing
 *	initialization of the message block.
 */

/* ARGSUSED */
mblk_t *
allocb(int size, uint_t pri)
{
	dblk_t *databp;
	mblk_t *bp;
	int tmpsiz;
	unchar *buf;
#ifdef STRLEAK
	struct mbinfo *mbp;
	struct mbinfo *tbp;
	long addr;
#endif
#ifdef STRPERF
	int stamp;
#endif

#ifdef STRLEAK
	saveaddr(&size, addr);
#endif

	/*
	 * If size >= 0, get buffer.
	 *
	 * If size < 0, don't bother with buffer, just get data
	 * and message block.
	 *
	 */
	buf = NULL;
	if (size < 0)
#ifdef STRLEAK
		tmpsiz = sizeof(struct mbinfo);
#else
		tmpsiz = sizeof(dblk_t) + sizeof(mblk_t);
#endif
	else {
		if (size == 0)
			size = 4;
#ifdef STRLEAK
		tmpsiz = size + sizeof(struct mbinfo);
#else
		tmpsiz = size + sizeof(dblk_t) + sizeof(mblk_t);
#endif
	}
	if (!mem_check(btopr(tmpsiz)))
		return(NULL);
#ifdef STRPERF
	if ((buf = kmem_zalloc(tmpsiz, KM_NOSLEEP)) == NULL)
#else
	if ((buf = kmem_alloc(tmpsiz, KM_NOSLEEP)) == NULL)
#endif
		return(NULL);


	*(Strcount_local + l.eng_num) += tmpsiz;

	MET_STRMDB(1);
	/* LINTED pointer alignment */
	bp = (mblk_t *) buf;
#ifdef STRPERF
	stamp = castimer();
	bp->b_life = castimer();
#endif
	buf += sizeof(mblk_t);
	/* LINTED pointer alignment */
	databp = (dblk_t *) buf;
	buf += sizeof(dblk_t);

#ifdef STRLEAK
	mbp = (struct mbinfo *) bp;
	buf += (sizeof(struct mbinfo) - sizeof(mblk_t) - sizeof(dblk_t));
#endif

	/*
	 * Initialize message block and
	 * data block descriptors.
	 */
	bp->b_next = NULL;
	bp->b_prev = NULL;
	bp->b_cont = NULL;
	bp->b_datap = databp;
	bp->b_band = 0;
	bp->b_flag = 0;
	databp->db_type = M_DATA;
	databp->db_odp = NULL;
	databp->db_muse = 1;
	databp->db_addr = (caddr_t)bp;
	databp->db_size = tmpsiz;
	databp->db_cpu = NULL;
	databp->db_frtnp = NULL;
#ifdef STRLEAK
	mbp->mb_func = addr;
	FSPIN_LOCK(&strleak_mutex);
	strleakcnt++;
	mbp->mb_next = strleakhead;
	strleakhead = bp;
	mbp->mb_prev = NULL;
	if (mbp->mb_next) {
		tbp = (struct mbinfo *) mbp->mb_next;
		tbp->mb_prev = bp;
	}
	FSPIN_UNLOCK(&strleak_mutex);
#endif

	/*
	 * Set reference count to 1 (first use).
	 */
	databp->db_ref = 1;

	if (size > 0) {
		databp->db_base = buf;
		databp->db_lim = buf + size;
		bp->b_rptr = buf;
		bp->b_wptr = buf;
	}
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
#endif
	return(bp);
}

/*
 * mblk_t *
 * allocb_physreq(int size, uint_t pri, physreq_t *physreq)
 *	Allocate a message subject to constraints in physreq.  No locking
 *	is necessary because this is creating the first reference to
 *	the message.
 *
 * Calling/Exit State:
 *	'pri' is unused, but is maintained
 *	for symmetry with allocb.
 */

/* ARGSUSED */
mblk_t *
allocb_physreq(int size, uint_t pri, physreq_t *physreq)
{
	dblk_t *databp;
	mblk_t *bp;
	int tmpsiz;
	unchar *buf;
	unchar *savebuf;
	int flip;
#ifdef STRLEAK
	struct mbinfo *mbp;
	struct mbinfo *tbp;
	long addr;
#endif
#ifdef STRPERF
	int stamp;
#endif

	ASSERT(physreq);
#ifdef STRLEAK
	saveaddr(&size, addr);
#endif

	/*
	 * If size >= 0, get buffer.
	 *
	 * If size < 0 it's an error.
	 */
	buf = NULL;
	if (size < 0)
		return(NULL);
	else {
		if (size == 0)
			size = 4;
#ifdef STRLEAK
		tmpsiz = size + sizeof(struct mbinfo);
#else
		tmpsiz = size + sizeof(dblk_t) + sizeof(mblk_t);
#endif
	}
	if (!mem_check(btopr(tmpsiz)))
		return(NULL);
#ifdef STRPERF
	if ((buf = kmem_zalloc_physreq(tmpsiz, physreq, KM_NOSLEEP)) == NULL)
#else
	if ((buf = kmem_alloc_physreq(tmpsiz, physreq, KM_NOSLEEP)) == NULL)
#endif
		return(NULL);


	*(Strcount_local + l.eng_num) += tmpsiz;

	MET_STRMDB(1);
	/* LINTED pointer alignment */
#ifdef STRLEAK
	if (physreq->phys_align > (sizeof(struct mbinfo)))
#else
	if (physreq->phys_align > (sizeof(mblk_t) + sizeof(dblk_t)))
#endif
		flip = 1;
	else
		flip = 0;
	savebuf = buf;
	if (flip) {
#ifdef STRLEAK
		bp = (mblk_t *) buf + sizeof(struct mbinfo);
#else
		bp = (mblk_t *) buf + sizeof(mblk_t) + sizeof(dblk_t);
#endif
		buf = (unchar *) bp + sizeof(mblk_t);
		/* LINTED pointer alignment */
		databp = (dblk_t *) buf;
	} else {
		bp = (mblk_t *) buf;
		buf += sizeof(mblk_t);
		/* LINTED pointer alignment */
		databp = (dblk_t *) buf;
		buf += sizeof(dblk_t);
	}

#ifdef STRLEAK
	if (!flip) {
		mbp = (struct mbinfo *) bp;
		buf += (sizeof(struct mbinfo) - sizeof(mblk_t) - sizeof(dblk_t));
	}
#endif

	/*
	 * Initialize message block and
	 * data block descriptors.
	 */
	bp->b_next = NULL;
	bp->b_prev = NULL;
	bp->b_cont = NULL;
	bp->b_datap = databp;
	bp->b_band = 0;
	bp->b_flag = 0;
	databp->db_type = M_DATA;
	databp->db_odp = NULL;
	databp->db_muse = 1;
	databp->db_addr = (caddr_t)savebuf;
	databp->db_size = tmpsiz;
	databp->db_cpu = NULL;
	databp->db_frtnp = NULL;
#ifdef STRLEAK
	mbp->mb_func = addr;
	FSPIN_LOCK(&strleak_mutex);
	strleakcnt++;
	mbp->mb_next = strleakhead;
	strleakhead = bp;
	mbp->mb_prev = NULL;
	if (mbp->mb_next) {
		tbp = (struct mbinfo *) mbp->mb_next;
		tbp->mb_prev = bp;
	}
	FSPIN_UNLOCK(&strleak_mutex);
#endif

	/*
	 * Set reference count to 1 (first use).
	 */
	databp->db_ref = 1;

	if (size > 0) {
		if (flip) {
			databp->db_base = savebuf;
			databp->db_lim = savebuf + size;
			bp->b_rptr = savebuf;
			bp->b_wptr = savebuf;
			bp->b_flag |= MSGFLIP;
		} else {
			databp->db_base = buf;
			databp->db_lim = buf + size;
			bp->b_rptr = buf;
			bp->b_wptr = buf;
		}
	}
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
#endif
#ifdef STRPERF
	stamp = castimer();
	bp->b_life = castimer();
#endif
	return(bp);
}

/*
 * mblk_t *
 * msgphysreq(mblk_t *mp, physreq_t *physreq)
 *	Conditionally copy a message if it doesn't meet the constraints
 *	specified by physreq
 *
 * Calling/Exit State:
 *	no locking assumptions
 */


/* ARGSUSED */
mblk_t *
msgphysreq(mblk_t *mp, physreq_t *physreq)
{
	mblk_t *bp;
	mblk_t *newmp;
	mblk_t *tbp;
	mblk_t *oldbp;
	void *base;
	size_t len;

	newmp = NULL;
	if (physreq == strphysreq) {
		/* compatibility case */
		base = mp->b_datap->db_base;
		len = mp->b_datap->db_lim - mp->b_datap->db_base;
	} else {
		/* direct call case */
		base = mp->b_rptr;
		len = mp->b_wptr - mp->b_rptr;
	}
	if (!physreq_met(base, len, physreq)) {
		newmp = copyb_physreq(mp, physreq);
		if (newmp == NULL)
			return(NULL);
		newmp->b_cont = mp->b_cont;
		tbp = newmp->b_cont;
		oldbp = newmp;
	} else {
		tbp = mp->b_cont;
		oldbp = mp;
	}
	while (tbp) {
		if (physreq == strphysreq) {
			/* compatibility case */
			base = tbp->b_datap->db_base;
			len = tbp->b_datap->db_lim - tbp->b_datap->db_base;
		} else {
			/* direct call case */
			base = tbp->b_rptr;
			len = tbp->b_wptr - tbp->b_rptr;
		}
		if (!physreq_met(base, len, physreq)) {
			bp = copyb_physreq(tbp, physreq);
			if (bp == NULL) {
				if (newmp)
					freeb(newmp);
				return(NULL);
			}
			bp->b_cont = tbp->b_cont;
			oldbp->b_cont = bp;
			freeb(tbp);
			tbp = bp;
		}
		oldbp = tbp;
		tbp = tbp->b_cont;
	}
	if (newmp) {
		freeb(mp);
		return(newmp);
	} else {
		return(mp);
	}
}

/* 
 * mblk_t *
 * esballoc(uchar_t *base, int size, int pri, frtn_t *fr_rtn)
 *	Allocate a data block and attach an externally-supplied
 *	data buffer pointed to by base to it.  No locking is
 *	necessary because this is creating the first reference
 *	to the message.  'pri' is unused, but is maintained
 *	for compatibility.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

/* ARGSUSED */
mblk_t *
esballoc(uchar_t *base, int size, int pri, frtn_t *fr_rtn)
{
	mblk_t *mp;
#ifdef STRLEAK
	long addr;
	struct mbinfo *mbp;
#endif
#ifdef STRPERF
	int stamp;
#endif

	if (!base || !fr_rtn)
		return(NULL);
#ifdef STRLEAK
	saveaddr(&base, addr);
#endif
	if (!(mp = allocb(-1, pri)))
		return(NULL);
	
#ifdef STRPERF
	stamp = castimer();
#endif
	mp->b_datap->db_frtnp = fr_rtn;
	mp->b_datap->db_base = base;
	mp->b_datap->db_lim = base + size;
#ifdef STRLEAK
	mp->b_datap->db_size = sizeof(struct mbinfo);
#else
	mp->b_datap->db_size = sizeof(dblk_t) + sizeof(mblk_t);
#endif
	mp->b_rptr = base;
	mp->b_wptr = base;
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
#ifdef STRLEAK
	mbp = (struct mbinfo *) mp;
	mbp->mb_func = addr;
#endif
	return(mp);
}

/*
 * mblk_t *
 * esballoc_u(uchar_t *base, int size, int pri, frtn_t *fr_rtn)
 *	Compatibility function for uniprocessor drivers and modules.
 *	Just like esballoc, but sets db_cpu so function will run on
 *	current processor.  No locking is necessary because this is
 *	creating the first reference to the message.
 *
 * Calling/Exit State:
 *	No locking assumptions. 'pri' is unused, but is maintained
 *	for compatibility.
 */

/* ARGSUSED */
mblk_t *
esballoc_u(uchar_t *base, int size, int pri, frtn_t *fr_rtn)
{
	mblk_t *mp;
#ifdef STRPERF
	int stamp;
#endif

	mp = esballoc(base, size, pri, fr_rtn);
#ifdef STRPERF
	stamp = castimer();
#endif
	if (mp)
		mp->b_datap->db_cpu = l.eng;
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
	return(mp);
}

/*
 * toid_t
 * esbbcall(int pri, void (*func)(), long arg)
 *	Call function 'func' with 'arg' when enough memory becomes available
 *	to allocate a message.
 *
 * Calling/Exit State:
 *	bc_mutex assumed unlocked.  'pri' is unused but is
 *	maintained for compatibility.
 */

/* ARGSUSED */
toid_t
esbbcall(int pri, void (*func)(), long arg)
{
	pl_t pl;
	struct strevent *sep;

	/*
	 * Allocate new stream event to add to linked list.
	 */
	if (!(sep = sealloc(SE_NOSLP))) {
		/*
		 *+ Kernel could not allocate enough memory for an event
		 *+ cell.  This indicates that there is not enough physical
		 *+ memory on the machine or that memory is being lost by
		 *+ the kernel.
		 */
		cmn_err(CE_WARN, "esbbcall: could not allocate stream event\n");
		return(0);
	}
	sep->se_next = NULL;
	sep->se_func = func;
	sep->se_arg = arg;
	sep->se_size = sizeof(mblk_t) + sizeof(dblk_t);
	pl = LOCK(&bc_mutex, PLSTR);

	/*
	 * Add newly allocated stream event to existing
	 * linked list of events.
	 */
	if (bcall.bc_head == NULL) {
		bcall.bc_head = sep;
		bcall.bc_tail = sep;
	} else {
		bcall.bc_tail->se_next = sep;
		bcall.bc_tail = sep;
	}
	strbcwait |= B_MP;
	/*
	 * Note, low bit is used to indicate local/global, that's why the
	 * increment is by 2
	 */
	strbcid += 2;
	if (strbcid == 0)
		strbcid = 2;
	sep->se_id = strbcid;
	UNLOCK(&bc_mutex, pl);
	return(sep->se_id);
}

/*
 * toid_t
 * esbbcall_u(int pri, void (*func)(), long arg)
 *	Compatibility function for uniprocessor drivers and modules.  Just
 *	like esbbcall, but function runs on current processor.
 *
 * Calling/Exit State:
 *	No locking assumptions.  Since this is a compatibility routine, locking
 *	to protect the list is unnecessary.  'pri' is unused but is maintained
 *	for compatibility.
 */

/* ARGSUSED */
int
esbbcall_u(int pri, void (*func)(), long arg)
{
	pl_t pl;
	struct strevent *sep;

	/*
	 * Allocate new stream event to add to linked list.
	 */
	if (!(sep = sealloc(SE_NOSLP))) {
		/*
		 *+ Kernel could not allocate enough memory for an event
		 *+ cell.  This indicates that there is not enough physical
		 *+ memory on the machine or that memory is being lost by
		 *+ the kernel.
		 */
		cmn_err(CE_WARN, "esbbcall: could not allocate stream event\n");
		return(0);
	}
	sep->se_next = NULL;
	sep->se_func = func;
	sep->se_arg = arg;
	sep->se_size = sizeof(mblk_t) + sizeof(dblk_t);
	pl = splstr();

	/*
	 * Add newly allocated stream event to existing
	 * linked list of events.
	 */
	if (l.bcall.bc_head == NULL ) {
		l.bcall.bc_head = sep;
		l.bcall.bc_tail = sep;
	} else {
		l.bcall.bc_tail->se_next = sep;
		l.bcall.bc_tail = sep;
	}
	(void) LOCK(&bc_mutex, PLSTR);
	strbcwait |= B_UP;
	/*
	 * Note, low bit is used to indicate local/global, that's why the
	 * increment is by 2
	 */
	strbcid += 2;
	if (strbcid == 0)
		strbcid = 2;
	sep->se_id = (strbcid | SE_LOCAL);
	strbceng[l.eng_num] = 1;
	UNLOCK(&bc_mutex, pl);
	return((int)sep->se_id);
}

/*
 * int
 * testb(int size, uint_t pri)
 *	Test if block of given size (adjusted for header information)
 *	can be allocated.
 *
 * Calling/Exit State:
 *	No locking assumptions.  'pri' is no longer used, but
 *	is retained for compatibility.
 *
 * Remarks:
 *	This function will be phased out in the future.
 */

/* ARGSUSED */
int
testb(int size, uint_t pri)
{
	return(kmem_avail(size+sizeof(mblk_t)+sizeof(dblk_t),
	       KM_NOSLEEP));
}

/*
 * toid_t
 * bufcall(uint_t size, int pri, void (*func)(), long arg)
 *	Call function 'func' with argument 'arg' when there is a reasonably
 *	good chance that a block of size 'size' can be allocated.
 *
 * Calling/Exit State:
 *	bc_mutex is assumed unlocked.
 *
 * Remarks:
 *	'pri' is no longer used, but is retained for compatibility.
 */

/* ARGSUSED */
toid_t
bufcall(uint_t size, int pri, void (*func)(), long arg)
{
	pl_t pl;
	struct strevent *sep;

	if (!(sep = sealloc(SE_NOSLP))) {
		/*
		 *+ Kernel could not allocate enough memory for an event
		 *+ cell.  This indicates that there is not enough physical
		 *+ memory on the machine or that memory is being lost by
		 *+ the kernel.
		 */
		cmn_err(CE_WARN, "bufcall: could not allocate stream event\n");
		return(0);
	}
	sep->se_next = NULL;
	sep->se_func = func;
	sep->se_arg = arg;
	sep->se_size = size + sizeof(mblk_t) + sizeof(dblk_t);
	pl = LOCK(&bc_mutex, PLSTR);

	/*
	 * add newly allocated stream event to existing
	 * linked list of events.
	 */
        if (bcall.bc_head == NULL) {
		bcall.bc_head = sep;
		bcall.bc_tail = sep;
	} else {
                bcall.bc_tail->se_next = sep;
		bcall.bc_tail = sep;
	}
	strbcwait |= B_MP;
	/*
	 * Note, low bit is used to indicate local/global, that's why the
	 * increment is by 2
	 */
	strbcid += 2;
	if (strbcid == 0)
		strbcid = 2;
	sep->se_id = strbcid;
	UNLOCK(&bc_mutex, pl);
	return(sep->se_id);
}

/*
 * toid_t
 * bufcall_u(uint_t size, int pri, void (*func)(), long arg)
 *	Compatibility function for uniprocessor drivers and modules.  Just
 *	like bufcall, but function runs on current processor.
 *
 * Calling/Exit State:
 *	No locking assumptions.  Since this is a compatibility routine, locking
 *	to protect the list is unnecessary.
 *
 * Remarks:
 *	'pri' is unused but is maintained for compatibility.
 */

/* ARGSUSED */
int
bufcall_u(uint_t size, int pri, void (*func)(), long arg)
{
	pl_t pl;
	struct strevent *sep;

	if (!(sep = sealloc(SE_NOSLP))) {
		/*
		 *+ Kernel could not allocate enough memory for an event
		 *+ cell.  This indicates that there is not enough physical
		 *+ memory on the machine or that memory is being lost by
		 *+ the kernel.
		 */
		cmn_err(CE_WARN, "bufcall: could not allocate stream event\n");
		return(0);
	}
	sep->se_next = NULL;
	sep->se_func = func;
	sep->se_arg = arg;
	sep->se_size = size + sizeof(mblk_t) + sizeof(dblk_t);
	pl = splstr();

	/*
	 * add newly allocated stream event to existing
	 * linked list of events.
	 */
        if (l.bcall.bc_head == NULL ) {
		l.bcall.bc_head = sep;
		l.bcall.bc_tail = sep;
	} else {
                l.bcall.bc_tail->se_next = sep;
		l.bcall.bc_tail = sep;
	}
	(void) LOCK(&bc_mutex, PLSTR);
	/*
	 * Note, low bit is used to indicate local/global, that's why the
	 * increment is by 2
	 */
	strbcid += 2;
	if (strbcid == 0)
		strbcid = 2;
	sep->se_id = (strbcid | SE_LOCAL);
	strbcwait |= B_UP;
	strbceng[l.eng_num] = 1;
	UNLOCK(&bc_mutex, pl);
	return((int)sep->se_id);
}

/*
 * void
 * unbufcall(toid_t id)
 *	Cancel a bufcall request.
 *
 * Calling/Exit State:
 *	bc_mutex assumed unlocked.
 */

void
unbufcall(toid_t id)
{
	int pl;
	struct strevent *sep;
	struct strevent *psep;

retry:
	while (strbcrunning)
		;
	pl = LOCK(&bc_mutex, PLSTR);
	/*
	 * Don't allow any bufcalls while some are active.  Crude
	 * synchronization, but the expected case is that this won't
	 * ever happen
	 */
	if (strbcrunning) {
		UNLOCK(&bc_mutex, pl);
		goto retry;
	}
	/* check this under lock in case the bufcall fires */
	sep = (struct strevent *) id;
	if ((id & SE_LOCAL) == 0) {
		/* global list */
		psep = NULL;
		for (sep = bcall.bc_head; sep; sep = sep->se_next) {
			if (id == sep->se_id)
				break;
			psep = sep;
		}
		if (sep) {
			/* found it */
			if (psep)
				psep->se_next = sep->se_next;
			else
				bcall.bc_head = sep->se_next;
			if (sep == bcall.bc_tail)
				bcall.bc_tail = psep;
			UNLOCK(&bc_mutex, pl);
			sefree(sep);
		} else {
			/* not found */
			UNLOCK(&bc_mutex, pl);
		}
	} else {
		/*
		 * local list, keep pl up to prevent interrupts
		 * No lock necessary on local list since there is no
		 * contention.
		 */
		UNLOCK(&bc_mutex, PLSTR);
		psep = NULL;
		for (sep = l.bcall.bc_head; sep; sep = sep->se_next) {
			if (id == sep->se_id)
				break;
			psep = sep;
		}
		if (sep) {
			if (psep)
				psep->se_next = sep->se_next;
			else
				l.bcall.bc_head = sep->se_next;
			if (sep == l.bcall.bc_tail)
				l.bcall.bc_tail = psep;
			sefree(sep);
		}
		splx(pl);
	}
	return;
}

/*
 * int
 * strioccall(int (*func)(), caddr_t arg, long iocid, queue_t *q)
 *	register function for ioctl post-processing
 *
 * Calling/Exit State:
 *	Assumes that strio_mutex is not locked.  Return 0 if the function
 *	is successfully registered, 1 otherwise.
 */

int
strioccall(int (*func)(), caddr_t arg, long iocid, queue_t *q)
{
	struct striopst *sp;
	pl_t pl;

/*
 * We assume that only 1 module/driver will post process an ioctl request.
 * If we find one on the list, just reuse the element.
 */
	sp = findioc(iocid);
	if (sp == NULL) {
		if (!mem_resv_check())
			return(1);
		sp = kmem_alloc(sizeof(struct striopst), KM_NOSLEEP);
		if (sp == NULL)
			return(1);
	}
	sp->sio_func = func;
	sp->sio_arg = arg;
	sp->sio_iocid = iocid;
	sp->sio_q = q;
	pl = LOCK(&strio_mutex, PLSTR);
	sp->sio_next = strioclist;
	strioclist = sp;
	UNLOCK(&strio_mutex, pl);
	return(0);
}

/*
 * void
 * freeb(mblk_t *bp)
 *	Free a message block.
 *
 * Calling/Exit State:
 *	mref_mutex assumed unlocked.
 */

void
freeb(mblk_t *bp)
{
	dblk_t *dp;
	dblk_t *odp;
#ifdef STRLEAK
	struct mbinfo *mbp;
	struct mbinfo *tbp;
#endif
	extern void strfreefp(mblk_t *);
	extern void strfreesb(mblk_t *);

#ifdef STRPERF
	extern void gatherstats(mblk_t *);

	/* ignore freeing time - not terribly relevant */
	bp->b_life = castimer() - bp->b_life;
	gatherstats(bp);
#endif
	ASSERT(bp);
	dp = bp->b_datap;
#ifdef STRLEAK
	mbp = (struct mbinfo *) bp;
	FSPIN_LOCK(&strleak_mutex);
	strleakcnt--;
	if (mbp->mb_prev) {
		tbp = (struct mbinfo *) mbp->mb_prev;
		tbp->mb_next = mbp->mb_next;
	}
	else
		strleakhead = mbp->mb_next;
	if (mbp->mb_next) {
		tbp = (struct mbinfo *) mbp->mb_next;
		tbp->mb_prev = mbp->mb_prev;
	}
	FSPIN_UNLOCK(&strleak_mutex);
#endif
	FSPIN_LOCK(&mref_mutex);
	ASSERT(dp->db_ref > 0);
	if (--dp->db_ref == 0) {
		FSPIN_UNLOCK(&mref_mutex);
                if (dp->db_type == M_PASSFP) {
                        strfreefp(bp);
                        return;
                }
		if (dp->db_cpu) {
			strfreesb(bp);
			return;
		}
		if (((caddr_t)bp == dp->db_addr) || (bp->b_flag & MSGFLIP)) {
			if (dp->db_frtnp) {
				strfreesb(bp);
				return;
			}
			MET_STRMDB(-1);

			*(Strcount_local + l.eng_num) -= dp->db_size;

			kmem_free(dp->db_addr, dp->db_size);
		} else {
			odp = dp->db_odp;
			if (odp) {
				odp->db_muse = 0;
				FSPIN_LOCK(&mref_mutex);
				if (odp->db_ref == 0) {
					FSPIN_UNLOCK(&mref_mutex);
					if (dp->db_frtnp) {
						strfreesb(bp);
						return;
					}
					if (dp->db_muse == 0) {
						MET_STRMDB(-1);
						
						*(Strcount_local + l.eng_num) 
							-= dp->db_size;
				
						kmem_free(dp->db_addr, dp->db_size);
					}
					MET_STRMDB(-1);

					*(Strcount_local + l.eng_num) -= 
						odp->db_size;
				
					kmem_free(odp->db_addr, odp->db_size);
				}
				else {
					FSPIN_UNLOCK(&mref_mutex);
					if (dp->db_muse == 0) {
						MET_STRMDB(-1);
						
						*(Strcount_local + l.eng_num) 
							-= dp->db_size;
				
						kmem_free(dp->db_addr, dp->db_size);
					}
				}
			} else {
				if (dp->db_frtnp) {
					strfreesb(bp);
					return;
				}
				if (dp->db_muse == 0) {
					MET_STRMDB(-1);

					*(Strcount_local + l.eng_num) -= 
						dp->db_size;

					kmem_free(dp->db_addr, dp->db_size);
				}
				MET_STRMSG(-1);
#ifdef STRLEAK
				*(Strcount_local + l.eng_num) -= 
						sizeof(struct mbinfo);
#else
				*(Strcount_local + l.eng_num) -= 
						sizeof(mblk_t);
#endif

#ifdef STRLEAK
				kmem_free((caddr_t)bp, sizeof(struct mbinfo));
#else
				kmem_free((caddr_t)bp, sizeof(mblk_t));
#endif
			}
		}
	} else {
		FSPIN_UNLOCK(&mref_mutex);
		if (((caddr_t)bp == dp->db_addr) || (bp->b_flag & MSGFLIP))
			dp->db_muse = 0;
		else {
			odp = dp->db_odp;
			if (odp && (odp->db_addr == (caddr_t)bp)) {
				odp->db_muse = 0;
				FSPIN_LOCK(&mref_mutex);
				if (odp->db_ref == 0) {
					FSPIN_UNLOCK(&mref_mutex);
					ASSERT(odp->db_frtnp == NULL);
					MET_STRMDB(-1);
					*(Strcount_local + l.eng_num) -= 
						odp->db_size;

		
					kmem_free(odp->db_addr, odp->db_size);
				}
				else {
					FSPIN_UNLOCK(&mref_mutex);
				}
				dp->db_odp = NULL;
			} else {
				MET_STRMSG(-1);
#ifdef STRLEAK
				*(Strcount_local + l.eng_num) -= 
					sizeof(struct mbinfo);
#else
				*(Strcount_local + l.eng_num) -= 
					sizeof(mblk_t);
#endif

#ifdef STRLEAK
				kmem_free((caddr_t)bp, sizeof(struct mbinfo));
#else
				kmem_free((caddr_t)bp, sizeof(mblk_t));
#endif
			}
		}
	}
} 

/*
 * void
 * freemsg(mblk_t *bp)
 *	Free all message blocks in a message using freeb().  The message
 *	may be NULL.
 *
 * Calling/Exit State:
 *	The message may be NULL.  mref_mutex assumed unlocked.
 */

void
freemsg(mblk_t *bp)
{
	mblk_t *tp;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	while (bp) {
		tp = bp->b_cont;
#ifdef STRPERF
		bp->b_oh += castimer() - stamp;
#endif
		freeb(bp);
		bp = tp;
	}
}

/*
 * mblk_t *
 * dupb(mblk_t *bp)
 *	Duplicate a message block.  Allocate a message block and
 *	assign proper values to it (read and write pointers) and
 *	link it to existing data block.  Increment reference count
 *	of data block.
 *
 * Calling/Exit State:
 *	mref_mutex assumed unlocked.
 */

mblk_t *
dupb(mblk_t *bp)
{
	mblk_t *nbp;
#ifdef STRLEAK
	struct mbinfo *mbp;
	struct mbinfo *tbp;
	long addr;
#endif
#ifdef STRPERF
	int stamp1;
	int stamp2;

	stamp1 = castimer();
#endif

	ASSERT(bp);
#ifdef STRLEAK
	saveaddr(&bp, addr);
#endif
	FSPIN_LOCK(&mref_mutex);
	if (bp->b_datap->db_ref == 255) {
		FSPIN_UNLOCK(&mref_mutex);
#ifdef STRPERF
		bp->b_oh += castimer() - stamp1;
#endif
		return(NULL);		/* don't wrap */
	}
	FSPIN_UNLOCK(&mref_mutex);
	if (!mem_resv_check()) {
#ifdef STRPERF
		bp->b_oh += castimer() - stamp1;
#endif
		return(NULL);
	}
#ifdef STRLEAK
	nbp = (mblk_t *)kmem_alloc(sizeof(struct mbinfo), KM_NOSLEEP);
#else
	nbp = (mblk_t *)kmem_alloc(sizeof(mblk_t), KM_NOSLEEP);
#endif
	if (nbp == NULL) {
#ifdef STRPERF
		bp->b_oh += castimer() - stamp1;
#endif
		return(NULL);
	}
#ifdef STRPERF
	stamp2 = castimer();
	nbp->b_life = castimer();
#endif
	MET_STRMSG(1);
	FSPIN_LOCK(&mref_mutex);
	/* recheck in case someone else dup'ed in the interim */
	if (bp->b_datap->db_ref == 255) {
		FSPIN_UNLOCK(&mref_mutex);
#ifdef STRPERF
		bp->b_oh += castimer() - stamp1;
#endif
#ifdef STRLEAK
		kmem_free((caddr_t)nbp, sizeof(struct mbinfo));
#else
		kmem_free((caddr_t)nbp, sizeof(mblk_t));
#endif
		MET_STRMSG(-1);
		return(NULL);		/* don't wrap */
	}
	(nbp->b_datap = bp->b_datap)->db_ref++;
	FSPIN_UNLOCK(&mref_mutex);

#ifdef STRLEAK
	*(Strcount_local + l.eng_num) += sizeof(struct mbinfo);
#else
	*(Strcount_local + l.eng_num) += sizeof(mblk_t);
#endif


#ifdef STRLEAK
	mbp = (struct mbinfo *) nbp;
	mbp->mb_func = addr;
	FSPIN_LOCK(&strleak_mutex);
	strleakcnt++;
	mbp->mb_next = strleakhead;
	strleakhead = nbp;
	mbp->mb_prev = NULL;
	if (mbp->mb_next) {
		tbp = (struct mbinfo *) mbp->mb_next;
		tbp->mb_prev = nbp;
	}
	FSPIN_UNLOCK(&strleak_mutex);
#endif

	nbp->b_next = NULL;
	nbp->b_prev = NULL;
	nbp->b_cont = NULL;
	nbp->b_rptr = bp->b_rptr;
	nbp->b_wptr = bp->b_wptr;
	nbp->b_flag = bp->b_flag;
	nbp->b_band = bp->b_band;
#ifdef STRPERF
	bp->b_oh += castimer() - stamp1;
	nbp->b_oh += castimer() - stamp2;
#endif
	return(nbp);
} 

/*
 * mblk_t *
 * dupmsg(mblk_t *bp)
 *	Duplicate a message block by block (uses dupb), returning
 *	a pointer to the duplicate message.
 *
 * Calling/Exit State:
 *	Returns a non-NULL value only if the entire message
 *	was dup'd.  mref_mutex assumed unlocked.
 */

mblk_t *
dupmsg(mblk_t *bp)
{
	mblk_t *head, *nbp;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	if (!bp || !(nbp = head = dupb(bp))) {
#ifdef STRPERF
		bp->b_oh += castimer() - stamp;
#endif
		return(NULL);
	}
	while (bp->b_cont) {
		if (!(nbp->b_cont = dupb(bp->b_cont))) {
#ifdef STRPERF
			head->b_oh += castimer() - stamp;
			bp->b_oh += castimer() - stamp;
#endif
			freemsg(head);
			return(NULL);
		}
		nbp = nbp->b_cont;
		bp = bp->b_cont;
	}
#ifdef STRPERF
	head->b_oh += castimer() - stamp;
	bp->b_oh += castimer() - stamp;
#endif
	return(head);
}

/*
 * mblk_t *
 * copyb(mblk_t *bp)
 *	Copy data from message block to newly allocated message block and
 *	data block.
 *
 * Calling/Exit State:
 *	Returns new message block pointer, or NULL if error.  No locking
 *	assumptions.
 */

mblk_t *
copyb(mblk_t *bp)
{
	mblk_t *nbp;
	dblk_t *dp;
	dblk_t *ndp;
	unsigned char *base;
#ifdef STRPERF
	int stamp1;
	int stamp2;

	stamp1 = castimer();
#endif

	ASSERT(bp);
	ASSERT(bp->b_wptr >= bp->b_rptr);
	dp = bp->b_datap;
	if (!(nbp = allocb(dp->db_lim - dp->db_base, BPRI_MED))) {
#ifdef STRPERF
		bp->b_oh += castimer() - stamp1;
#endif
		return(NULL);
	}
#ifdef STRPERF
	stamp2 = castimer();
#endif
	nbp->b_flag = bp->b_flag;
	nbp->b_band = bp->b_band;
	ndp = nbp->b_datap;	
	ndp->db_type = dp->db_type;
	nbp->b_rptr = ndp->db_base + (bp->b_rptr - dp->db_base);
	nbp->b_wptr = ndp->db_base + (bp->b_wptr - dp->db_base);
	base = nbp->b_rptr;
	bcopy(bp->b_rptr, base, nbp->b_wptr - base);
#ifdef STRPERF
	bp->b_oh += castimer() - stamp1;
	nbp->b_oh += castimer() - stamp2;
#endif
	return(nbp);
}

/*
 * mblk_t *
 * copyb_physreq(mblk_t *bp, physreq_t *physreq)
 *	Copy data from message block to newly allocated message block and
 *	data block, which has the characteristics defined by physreq.
 *	If this is a compatibility copy, the offsets of b_rptr and b_wptr
 *	are preserved; otherwise the data is shifted to the front of the
 *	buffer.
 *
 * Calling/Exit State:
 *	Returns new message block pointer, or NULL if error.  No locking
 *	assumptions.
 */

mblk_t *
copyb_physreq(mblk_t *bp, physreq_t *physreq)
{
	mblk_t *nbp;
	dblk_t *dp;
	dblk_t *ndp;
#ifdef STRPERF
	int stamp1;
	int stamp2;

	stamp1 = castimer();
#endif

	ASSERT(bp);
	ASSERT(bp->b_wptr >= bp->b_rptr);
	ASSERT(physreq);
	dp = bp->b_datap;
	if (!(nbp = allocb_physreq(dp->db_lim - dp->db_base, BPRI_MED, physreq))) {
#ifdef STRPERF
		bp->b_oh += castimer() - stamp1;
#endif
		return(NULL);
	}
#ifdef STRPERF
	stamp2 = castimer();
#endif
	nbp->b_flag = bp->b_flag;
	nbp->b_band = bp->b_band;
	ndp = nbp->b_datap;	
	ndp->db_type = dp->db_type;
	if (physreq == strphysreq) {
		/* compatibility case */
		nbp->b_rptr = ndp->db_base + (bp->b_rptr - dp->db_base);
		nbp->b_wptr = ndp->db_base + (bp->b_wptr - dp->db_base);
	} else {
		nbp->b_wptr = nbp->b_rptr + (bp->b_wptr - bp->b_rptr);
	}
	bcopy(bp->b_rptr, nbp->b_rptr, nbp->b_wptr - nbp->b_rptr);
#ifdef STRPERF
	bp->b_oh += castimer() - stamp1;
	nbp->b_oh += castimer() - stamp2;
#endif
	return(nbp);
}

/*
 * mblk_t *
 * copymsg(mblk_t *bp)
 *	Copy data from message to newly allocated message using new
 *	data blocks.
 * Calling/Exit State:
 *	Returns a pointer to the new message, or NULL if error.  No locking
 *	assumptions.
 */

mblk_t *
copymsg(mblk_t *bp)
{
	mblk_t *head;
	mblk_t *nbp;
#ifdef STRPERF
	int stamp1;
	int stamp2;

	stamp1 = castimer();
#endif

	if (!bp || !(nbp = head = copyb(bp))) {
#ifdef STRPERF
		bp->b_oh += castimer() - stamp1;
#endif
		return(NULL);
	}
#ifdef STRPERF
	stamp2 = castimer();
#endif
	while (bp->b_cont) {
		if (!(nbp->b_cont = copyb(bp->b_cont))) {
#ifdef STRPERF
			head->b_oh += castimer() - stamp2;
			bp->b_oh += castimer() - stamp1;
#endif
			freemsg(head);
			return(NULL);
		}
		nbp = nbp->b_cont;
		bp = bp->b_cont;
	}
#ifdef STRPERF
	head->b_oh += castimer() - stamp2;
	bp->b_oh += castimer() - stamp1;
#endif
	return(head);
}


/*
 * mblk_t *
 * copymsg_physreq(mblk_t *bp, physreq_t *physreq)
 *	Copy data from message to newly allocated message using new
 *	data blocks that match criteria specified in physreq.
 * Calling/Exit State:
 *	Returns a pointer to the new message, or NULL if error.  No locking
 *	assumptions.
 */

mblk_t *
copymsg_physreq(mblk_t *bp, physreq_t *physreq)
{
	mblk_t *head;
	mblk_t *nbp;
#ifdef STRPERF
	int stamp1;
	int stamp2;

	stamp1 = castimer();
#endif

	if (!bp || !(nbp = head = copyb_physreq(bp, physreq))) {
#ifdef STRPERF
		bp->b_oh += castimer() - stamp1;
#endif
		return(NULL);
	}
#ifdef STRPERF
	stamp2 = castimer();
#endif
	while (bp->b_cont) {
		if (!(nbp->b_cont = copyb_physreq(bp->b_cont, physreq))) {
#ifdef STRPERF
			head->b_oh += castimer() - stamp2;
			bp->b_oh += castimer() - stamp1;
#endif
			freemsg(head);
			return(NULL);
		}
		nbp = nbp->b_cont;
		bp = bp->b_cont;
	}
#ifdef STRPERF
	head->b_oh += castimer() - stamp2;
	bp->b_oh += castimer() - stamp1;
#endif
	return(head);
}


/* 
 * void
 * linkb(mblk_t *mp, mblk_t *bp)
 *	Link a message block to tail of message.
 *
 * Calling/Exit State:
 *	No locking assumptions.  On return, bp is linked to mp.
 */

void
linkb(mblk_t *mp, mblk_t *bp)
{
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif
	ASSERT(mp && bp);
	for (; mp->b_cont; mp = mp->b_cont)
		;
	mp->b_cont = bp;
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
	mp->b_oh += castimer() - stamp;
#endif
}

/*
 * mblk_t *
 * unlinkb(mblk_t *bp)
 *	Unlink a message block from head of message.
 *
 * Calling/Exit State:
 *	Return pointer to new message or NULL if message becomes empty.
 *	No locking assumptions.
 */

mblk_t *
unlinkb(mblk_t *bp)
{
	mblk_t *bp1;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(bp);
	bp1 = bp->b_cont;
	bp->b_cont = NULL;
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
	bp1->b_oh += castimer() - stamp;
#endif
	return(bp1);
}

/* 
 * mblk_t *
 * rmvb(mblk_t *mp, mblk_t *bp)
 *	Remove message block "bp" from message "mp".
 *
 * Calling/Exit State:
 *	Return pointer to new message or NULL if no message remains.
 *	Return -1 if bp is not found in message.  No locking assumptions.
 */

mblk_t *
rmvb(mblk_t *mp, mblk_t *bp)
{
	mblk_t *tmp;
	mblk_t *lastp;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(mp && bp);
	lastp = NULL;
	for (tmp = mp; tmp; tmp = tmp->b_cont) {
		if (tmp == bp) {
			if (lastp)
				lastp->b_cont = tmp->b_cont;
			else
				mp = tmp->b_cont;
			tmp->b_cont = NULL;
#ifdef STRPERF
			mp->b_oh += castimer() - stamp;
			bp->b_oh += castimer() - stamp;
#endif
			return(mp);
		}
		lastp = tmp;
	}
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
	bp->b_oh += castimer() - stamp;
#endif
	return((mblk_t *)-1);
}

/*
 * int
 * pullupmsg(mblk_t *mp, int len)
 *	Concatenate and align first len bytes of common message type.
 *	Len == -1, means concat everything.  For compatibility with old
 *	drivers, always return memory that is maximally constrained.
 *
 * Calling/Exit State:
 *	Returns 1 on success, 0 on failure.  After the pullup, mp points
 *	to the pulled up data.
 */

int
pullupmsg(mblk_t *mp, int len)
{
	mblk_t *bp;
	mblk_t *tmp;
	dblk_t *newdp;
	int n;
	dblk_t *odp;
	unsigned char *cp;
	int esb;
#ifdef STRLEAK
	long addr;
	struct mbinfo *mbp;
	struct mbinfo *tbp;
#endif
#ifdef STRPERF
	int stamp1;

	stamp1 = castimer();
#endif

	ASSERT(mp != NULL);
#ifdef STRLEAK
	saveaddr(&mp, addr);
#endif

	/*
	 * Quick checks , to be simple, always copy:
	 */
	if (len == -1) {
		len = xmsgsize(mp, 1);
	} else {
		if (xmsgsize(mp, 1) < len) {
#ifdef STRPERF
			mp->b_oh += castimer() - stamp1;
#endif
			return(0);
		}
	}

	/*
	 * Allocate a new mblk header.  It is used later to interchange
	 * mp and newbp.
	 */
	if (!mem_resv_check())
		return(0);
#ifdef STRLEAK
	tmp = (mblk_t *)kmem_alloc(sizeof(struct mbinfo), KM_NOSLEEP);
#else
	tmp = (mblk_t *)kmem_alloc(sizeof(mblk_t), KM_NOSLEEP);
#endif
	if (tmp == NULL)
		return(0);

#ifdef STRPERF
	tmp->b_life = castimer();
#endif
	/*
	 * Allocate the new mblk.  We might be able to use the existing
	 * mblk, but we don't want to modify it in case its shared.
	 * The new dblk takes on the type of the old dblk.
	 */
	if (!mem_resv_check()) {
#ifdef STRPERF
		mp->b_oh += castimer() - stamp1;
#endif
#ifdef STRLEAK
		kmem_free((caddr_t)tmp, sizeof(struct mbinfo));
#else
		kmem_free((caddr_t)tmp, sizeof(mblk_t));
#endif
		return(0);
	}
	newdp = (dblk_t *)kmem_alloc(len + sizeof(dblk_t), KM_NOSLEEP|KM_REQ_DMA|KM_PHYSCONTIG);
	if (newdp == NULL) {
		kmem_free((caddr_t)tmp, sizeof(mblk_t));
#ifdef STRPERF
		mp->b_oh += castimer() - stamp1;
#endif
		return(0);
	}
	
	/* count tmp and newdp */
#ifdef STRLEAK
	*(Strcount_local + l.eng_num) += (sizeof(struct mbinfo) + len);
#else
	*(Strcount_local + l.eng_num) += (sizeof(mblk_t) + sizeof(dblk_t) 
						+ len);
#endif

	MET_STRMSG(1);
#ifdef STRLEAK
	mbp = (struct mbinfo *) tmp;
	mbp->mb_func = addr;
	FSPIN_LOCK(&strleak_mutex);
	strleakcnt++;
	mbp->mb_next = strleakhead;
	strleakhead = tmp;
	mbp->mb_prev = NULL;
	if (mbp->mb_next) {
		tbp = (struct mbinfo *) mbp->mb_next;
		tbp->mb_prev = tmp;
	}
	FSPIN_UNLOCK(&strleak_mutex);
#endif
	/*
	 * this is used to band-aid a problem - odp handling needs to be
	 * redone
	 */
	if (mp->b_datap->db_frtnp)
		esb = 1;
	else
		esb = 0;

	newdp->db_type = mp->b_datap->db_type;
        odp = mp->b_datap->db_odp;
        if (odp) {
                if (odp->db_addr == (caddr_t)mp) {
                        newdp->db_odp = odp;
                        mp->b_datap->db_odp = NULL;
                } else {
                        newdp->db_odp = NULL;
                }
        } else if ((mp->b_datap->db_addr == (caddr_t)mp) || (mp->b_flag & MSGFLIP)) {
                newdp->db_odp = mp->b_datap;
        } else {
                newdp->db_odp = NULL;
        }
	newdp->db_muse = 0;
	newdp->db_addr = (caddr_t)newdp;
	newdp->db_size = len + sizeof(dblk_t);
	newdp->db_ref = 1;
	newdp->db_base = (unsigned char *)newdp + sizeof(dblk_t);
	newdp->db_lim = newdp->db_base + len;
	newdp->db_frtnp = NULL;
	newdp->db_cpu = NULL;
	cp = newdp->db_base;

	/*
	 * Scan mblks and copy over data into the new mblk.
	 * Two ways to fall out: top of loop: while (len) -
	 * bp points to the next mblk containing data or is
	 * null.  Inside loop: if (len == 0) - in this case,
	 * bp points to an mblk that still has data in it.
	 */ 
	bp = mp;
	while (len) {
		mblk_t *b_cont;

		ASSERT(bp);
		ASSERT(bp->b_wptr >= bp->b_rptr);
		n = min(bp->b_wptr - bp->b_rptr, len);
		bcopy(bp->b_rptr, cp, n);
		cp += n;
		bp->b_rptr += n;
		len -= n;
		if (len == 0)
			break;
		b_cont = bp->b_cont;
		if (bp != mp) {	/* don't free the head mblk */
#ifdef STRPERF
			bp->b_oh += castimer() - stamp1;
#endif
			freeb(bp);
		}
		bp = b_cont;
	}

	/*
	 * At this point:  newbp points to a dblk that
	 * contains the pulled up data.  The head mblk, mp, is
	 * preserved and may or may not have data in it.  The
	 * intermediate mblks are freed, and bp points to the
	 * last mblk that was pulled-up or is null.
	 *
	 * Now the tricky bit.  After this, mp points to the new dblk
	 * and tmp points to the old dblk.
	 */
	*tmp = *mp;
	mp->b_rptr = newdp->db_base;
	mp->b_wptr = cp;
	mp->b_prev = NULL;
	mp->b_next = NULL;
	mp->b_datap = newdp;

	/*
	 * If the head mblk (now tmp) still has data in it, link it to mp
	 * otherwise link the remaining mblks to mp and free the
	 * old head mblk.
	 */
	if (tmp->b_rptr != tmp->b_wptr)
		mp->b_cont = tmp;
	else {
		if (mp != bp)
			mp->b_cont = bp;
#ifdef STRPERF
		tmp->b_oh += castimer() - stamp1;
#endif
		/*
		 * if it was an esballoc'ed message, stick it on the end
		 * so things happen in the right order
		 */
		if (esb == 0)
			freeb(tmp);
		else
			linkb(mp, tmp);
	}
#ifdef STRPERF
	mp->b_oh += castimer() - stamp1;
#endif
	/* don't prune esballoc'ed messages */
	if (!esb)
		(void) mblkprune(mp);
	/* safe to turn off unconditionally */
	mp->b_flag &= ~MSGFLIP;
	return(1);
}

/*
 * mblk_t *
 * msgpullup(mblk_t *mp, int len)
 *	Concatenate and align first len bytes of common message type.
 *	len == -1, means concat everything.
 *
 * Calling/Exit State:
 *	Returns a pointer to the new message block on success, or 0 on
 *	failure.  After the pullup, mp remains unaltered.  No locking
 *	assumptions.
 *
 * Description:
 *	This routine should be used instead of pullupmsg() so we
 *	can phase pullupmsg() out, remove the db_odp and db_muse
 *	fields in the data block, and clean up freeb().
 */

mblk_t *
msgpullup(mblk_t *mp, int len)
{
	mblk_t *newmp;
	mblk_t *bp;
	mblk_t *tmp;
	int n;
#ifdef STRLEAK
	long addr;
	struct mbinfo *mbp;
#endif
#ifdef STRPERF
	int stamp1;
	int stamp2;

	stamp1 = castimer();
#endif

	ASSERT(mp != NULL);
#ifdef STRLEAK
	saveaddr(&mp, addr);
#endif

	/*
	 * Quick checks for success or failure:
	 */
	if (len == -1) {
		if (mp->b_cont == NULL && str_aligned(mp->b_rptr)) {
#ifdef STRPERF
			mp->b_oh += castimer() - stamp1;
#endif
			return(copyb(mp));
		}
		len = xmsgsize(mp, 1);
	} else {
		ASSERT(mp->b_wptr >= mp->b_rptr);
		if (mp->b_wptr - mp->b_rptr >= len && str_aligned(mp->b_rptr)) {
#ifdef STRPERF
			mp->b_oh += castimer() - stamp1;
#endif
			return(mblkprune(copymsg(mp)));
		}
		if (xmsgsize(mp, 1) < len) {
#ifdef STRPERF
			mp->b_oh += castimer() - stamp1;
#endif
			return(NULL);
		}
	}

	/*
	 * Allocate the new mblk.
	 * The new mblk takes on the type of the old mblk.
	 */
	if ((newmp = allocb(len, BPRI_MED)) == NULL)
		return(NULL);
#ifdef STRPERF
	stamp2 = castimer();
#endif
	newmp->b_datap->db_type = mp->b_datap->db_type;
	newmp->b_band = mp->b_band;
	newmp->b_flag = mp->b_flag;

	/*
	 * Scan mblks and copy over data into the new mblk.
	 * Two ways to fall out: top of loop: while (len) -
	 * bp points to the next mblk containing data.
	 * Inside loop: if (len == 0) - in this case,
	 * bp points to an mblk that still may have data in it.
	 */ 
	bp = mp;
	while (len) {
		ASSERT(bp);
		ASSERT(bp->b_wptr >= bp->b_rptr);
		n = min(bp->b_wptr - bp->b_rptr, len);
		bcopy(bp->b_rptr, newmp->b_wptr, n);
		newmp->b_wptr += n;
		len -= n;
		if (len == 0)
			break;
		bp = bp->b_cont;
	}

	/*
	 * At this point:  newmp points to an mblk that
	 * contains the pulled up data.  The old message, mp,
	 * is preserved.  bp points to the last mblk that was
	 * pulled-up.  n contains the amount of data already copied from bp.
	 */
	if ((tmp = copymsg(bp)) == NULL) {
#ifdef STRPERF
		newmp->b_oh += castimer() - stamp2;
#endif
		freeb(newmp);
#ifdef STRPERF
		mp->b_oh += castimer() - stamp1;
#endif
		return(NULL);
	}
	tmp->b_rptr += n;
	newmp->b_cont = tmp;
#ifdef STRPERF
	newmp->b_oh += castimer() - stamp2;
	mp->b_oh += castimer() - stamp1;
#endif
#ifdef STRLEAK
	mbp = (struct mbinfo *) newmp;
	mbp->mb_func = addr;
#endif
	return(mblkprune(newmp));
}

/*
 * mblk_t *
 * msgpullup_physreq(mblk_t *mp, int len, physreq_t *physreq)
 *	Concatenate and align first len bytes of common message type.
 *	len == -1, means concat everything.
 *
 * Calling/Exit State:
 *	Returns a pointer to the new message block on success, or 0 on
 *	failure.  The new message matches the constraints defined by
 *	physreq.  After the pullup, mp remains unaltered.  No locking
 *	assumptions.
 *
 * Description:
 *	This routine should be used instead of pullupmsg() so we
 *	can phase pullupmsg() out, remove the db_odp and db_muse
 *	fields in the data block, and clean up freeb().
 */

mblk_t *
msgpullup_physreq(mblk_t *mp, int len, physreq_t *physreq)
{
	mblk_t *newmp;
	mblk_t *bp;
	mblk_t *tmp;
	int n;
#ifdef STRLEAK
	long addr;
	struct mbinfo *mbp;
#endif
#ifdef STRPERF
	int stamp1;
	int stamp2;

	stamp1 = castimer();
#endif

	ASSERT(mp);
	ASSERT(physreq);
#ifdef STRLEAK
	saveaddr(&mp, addr);
#endif

	/*
	 * Quick checks for success or failure:
	 */
	if (len == -1) {
		if (mp->b_cont == NULL && str_aligned(mp->b_rptr)) {
#ifdef STRPERF
			mp->b_oh += castimer() - stamp1;
#endif
			return(copyb_physreq(mp, physreq));
		}
		len = xmsgsize(mp, 1);
	} else {
		ASSERT(mp->b_wptr >= mp->b_rptr);
		if (mp->b_wptr - mp->b_rptr >= len && str_aligned(mp->b_rptr)) {
#ifdef STRPERF
			mp->b_oh += castimer() - stamp1;
#endif
			return(mblkprune(copymsg_physreq(mp, physreq)));
		}
		if (xmsgsize(mp, 1) < len) {
#ifdef STRPERF
			mp->b_oh += castimer() - stamp1;
#endif
			return(NULL);
		}
	}

	/*
	 * Allocate the new mblk.
	 * The new mblk takes on the type of the old mblk.
	 */
	if ((newmp = allocb_physreq(len, BPRI_MED, physreq)) == NULL)
		return(NULL);
#ifdef STRPERF
	stamp2 = castimer();
#endif
	newmp->b_datap->db_type = mp->b_datap->db_type;
	newmp->b_band = mp->b_band;
	newmp->b_flag = mp->b_flag;

	/*
	 * Scan mblks and copy over data into the new mblk.
	 * Two ways to fall out: top of loop: while (len) -
	 * bp points to the next mblk containing data.
	 * Inside loop: if (len == 0) - in this case,
	 * bp points to an mblk that still may have data in it.
	 */ 
	bp = mp;
	while (len) {
		ASSERT(bp);
		ASSERT(bp->b_wptr >= bp->b_rptr);
		n = min(bp->b_wptr - bp->b_rptr, len);
		bcopy(bp->b_rptr, newmp->b_wptr, n);
		newmp->b_wptr += n;
		len -= n;
		if (len == 0)
			break;
		bp = bp->b_cont;
	}

	/*
	 * At this point:  newmp points to an mblk that
	 * contains the pulled up data.  The old message, mp,
	 * is preserved.  bp points to the last mblk that was
	 * pulled-up.  n contains the amount of data already copied from bp.
	 */
	if ((tmp = copymsg_physreq(bp, physreq)) == NULL) {
#ifdef STRPERF
		newmp->b_oh += castimer() - stamp2;
#endif
		freeb(newmp);
#ifdef STRPERF
		mp->b_oh += castimer() - stamp1;
#endif
		return(NULL);
	}
	tmp->b_rptr += n;
	newmp->b_cont = tmp;
#ifdef STRPERF
	newmp->b_oh += castimer() - stamp2;
	mp->b_oh += castimer() - stamp1;
#endif
#ifdef STRLEAK
	mbp = (struct mbinfo *) newmp;
	mbp->mb_func = addr;
#endif
	return(mblkprune(newmp));
}


/*
 * int
 * adjmsg(mblk_t *mp, int len)
 *	Trim bytes from message. len > 0, trim from head and len < 0,
 *	trim from tail
 *
 * Calling/Exit State:
 *	Returns 1 on success, 0 on failure.  No locking assumptions.
 */

int
adjmsg(mblk_t *mp, int len)
{
	mblk_t *bp;
	int n;
	int fromhead;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(mp != NULL);
	fromhead = 1;
	if (len < 0) {
		fromhead = 0;
		len = -len;
	}
	if (xmsgsize(mp, fromhead) < len) {
#ifdef STRPERF
		mp->b_oh += castimer() - stamp;
#endif
		return(0);
	}
	if (fromhead) {
		bp = mp;
		while (len) {
			n = min(bp->b_wptr - bp->b_rptr, len);
			bp->b_rptr += n;
			len -= n;
			bp = bp->b_cont;
		}
	} else {
		mblk_t *endbp = NULL;

		while (len) {
			for (bp = mp; bp->b_cont != endbp; bp = bp->b_cont)
				;
			n = min(bp->b_wptr - bp->b_rptr, len);
			bp->b_wptr -= n;
			len -= n;
			endbp = bp;
		}
	}
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
	return(1);
}

/*
 * int
 * msgdsize(mblk_t *bp)
 *	Get number of data bytes in message.
 *
 * Calling/Exit State:
 *	Returns number of bytes in message.  No locking assumptions.
 */

int
msgdsize(mblk_t *bp)
{
	int count;
#ifdef STRPERF
	int stamp;
	mblk_t *mp;

	stamp = castimer();
	mp = bp;
#endif

	count = 0;
	for (; bp; bp = bp->b_cont)
		if (bp->b_datap->db_type == M_DATA) {
			ASSERT(bp->b_wptr >= bp->b_rptr);
			count += bp->b_wptr - bp->b_rptr;
		}
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
	return(count);
}

/*
 * mblk_t *
 * getq_l(queue_t *q)
 *	Same as getq.
 *
 * Calling/Exit State:
 *	Returns first message from head of queue.  sd_mutex assumed locked.
 */

mblk_t *
getq_l(queue_t *q)
{
	mblk_t *bp;
	mblk_t *tmp;
	qband_t *qbp;
	int backenable;
	int wantw;
	queue_t *nq;
#ifdef STRPERF
	int stamp;
#endif

	ASSERT(q);
	qbp = NULL;
	backenable = 0;
	wantw = 0;
	if (!(bp = q->q_first)) {
		q->q_flag |= QWANTR;
		backenable = 1;		/* we might backenable */
		wantw = q->q_flag & QWANTW;
	} else {
#ifdef STRPERF
		stamp = castimer();
#endif
		if (bp->b_flag & MSGNOGET) {	/* hack hack hack */
			while (bp && (bp->b_flag & MSGNOGET))
				bp = bp->b_next;
			if (bp) {
				rmvq(q, bp);
#ifdef STRPERF
				bp->b_oh += castimer() - stamp;
#endif
			}
			return(bp);
		}
		if (!(q->q_first = bp->b_next))
			q->q_last = NULL;
		else
			q->q_first->b_prev = NULL;

#ifdef STRPERF
		bp->b_sched += castimer() - bp->b_stamp;
		bp->b_stamp = 0;
#endif
		if (bp->b_band == 0) {
			wantw = q->q_flag & QWANTW;
			for (tmp = bp; tmp; tmp = tmp->b_cont)
				q->q_count -= (tmp->b_wptr - tmp->b_rptr);
			if (q->q_count < q->q_hiwat)
				q->q_flag &= ~QFULL;
			if (q->q_count <= q->q_lowat)
				backenable = 1;
		} else {
			int i;

			ASSERT(bp->b_band <= q->q_nband);
			ASSERT(q->q_bandp != NULL);
			qbp = q->q_bandp;
			i = bp->b_band;
			while (--i > 0)
				qbp = qbp->qb_next;
			if (qbp->qb_first == qbp->qb_last) {
				qbp->qb_first = NULL;
				qbp->qb_last = NULL;
			} else {
				qbp->qb_first = bp->b_next;
			}
			wantw = qbp->qb_flag & QB_WANTW;
			for (tmp = bp; tmp; tmp = tmp->b_cont)
				qbp->qb_count -= (tmp->b_wptr - tmp->b_rptr);
			if ((qbp->qb_count < qbp->qb_hiwat) &&
			    (qbp->qb_flag & QB_FULL)) {
				qbp->qb_flag &= ~QB_FULL;
				q->q_blocked--;
			}
			if (qbp->qb_count <= qbp->qb_lowat)
				backenable = 1;
		}
		q->q_flag &= ~QWANTR;
		bp->b_next = NULL;
		bp->b_prev = NULL;
	}
	if (backenable) {
		/* find nearest back queue with service proc */
		for (nq = backq_l(q); nq && !(nq->q_qinfo->qi_srvp && (nq->q_flag & QPROCSON)); nq = backq_l(nq))
			;
		if (wantw) {
			if (bp && bp->b_band != 0)
				qbp->qb_flag &= ~QB_WANTW;
			else
				q->q_flag &= ~QWANTW;
			if (nq) {
				qenable_l(nq);
				setqback(nq, bp ? bp->b_band : 0);
			}
		}
		if ((q->q_flag & QWANTW) && (q->q_blocked == 0) &&
		    !(q->q_flag & QFULL) && nq) {
			/*
			 * this is the case where a getq pulled off a
			 * priority message and as a side effect, freed
			 * up flow control on band 0
			 */
			q->q_flag &= ~QWANTW;
			qenable_l(nq);
			setqback(nq, 0);
		}
	}
#ifdef STRPERF
	if (bp)
		bp->b_oh += castimer() - stamp;
#endif
	return(bp);
}

/*
 * mblk_t *
 * getq(queue_t *q)
 *	Get a message off head of queue.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked
 *
 * Description:
 *	If queue has no buffers then mark queue with QWANTR. (queue
 *	wants to be read by someone when data becomes available)
 *
 *	If there is something to take off then do so.  If queue falls below
 *	hi water mark turn off QFULL flag.  Decrement byte count of queue.
 *	Also turn off QWANTR because queue is being read.
 *
 *	The queue count is maintained on a per-band basis.  Priority band 0
 *	(normal messages) uses q_count, q_lowat, etc.  Non-zero priority
 *	bands use the fields in their respective qband structures (qb_count,
 *	qb_lowat, etc.)  All messages appear on the same list, linked via
 *	their b_next pointers.  q_first is the head of the list.  q_count
 *	does not reflect the size of all the messages on the queue.  It only
 *	reflects those messages in the normal band of flow.  The one exception
 *	to this deals with high priority messages.  They are in their own
 *	conceptual "band", but are accounted against q_count.
 *
 *	If queue count is below the lo water mark and QWANTW is set, enable
 *	the closest backq which has a service procedure and turn off the
 *	QWANTW flag.
 *
 *	getq could be built on top of rmvq, but isn't because of performance
 *	considerations.
 *
 */

mblk_t *
getq(queue_t *q)
{
	mblk_t *mp;
	pl_t pl;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	mp = getq_l(q);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(mp);
}

/*
 * void
 * rmvq(queue_t *q, mblk_t *mp)
 *	Remove a message from a queue.  The queue count and other
 *	flow control parameters are adjusted and the back queue
 *	enabled if necessary.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked
 */

void
rmvq(queue_t *q, mblk_t *mp)
{
	mblk_t *tmp;
	int i;
	qband_t *qbp;
	int backenable;
	int wantw;
	queue_t *nq;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
	mp->b_sched += castimer() - mp->b_stamp;
	mp->b_stamp = 0;
#endif
	ASSERT(q && mp);
	ASSERT(mp->b_band <= q->q_nband);
	qbp = NULL;
	backenable = 0;
	wantw = 0;
	if (mp->b_band != 0) {		/* Adjust band pointers */
		ASSERT(q->q_bandp != NULL);
		qbp = q->q_bandp;
		i = mp->b_band;
		while (--i > 0)
			qbp = qbp->qb_next;
		if (mp == qbp->qb_first) {
			if (mp->b_next && mp->b_band == mp->b_next->b_band)
				qbp->qb_first = mp->b_next;
			else
				qbp->qb_first = NULL;
		}
		if (mp == qbp->qb_last) {
			if (mp->b_prev && mp->b_band == mp->b_prev->b_band)
				qbp->qb_last = mp->b_prev;
			else
				qbp->qb_last = NULL;
		}
	}

	/*
	 * Remove the message from the list.
	 */
	if (mp->b_prev)
		mp->b_prev->b_next = mp->b_next;
	else
		q->q_first = mp->b_next;
	if (mp->b_next)
		mp->b_next->b_prev = mp->b_prev;
	else
		q->q_last = mp->b_prev;
	mp->b_next = NULL;
	mp->b_prev = NULL;

	if (mp->b_band == 0) {		/* Perform q_count accounting */
		wantw = q->q_flag & QWANTW;
		for (tmp = mp; tmp; tmp = tmp->b_cont)
			q->q_count -= (tmp->b_wptr - tmp->b_rptr);
		if (q->q_count < q->q_hiwat)
			q->q_flag &= ~QFULL;
		if (q->q_count <= q->q_lowat)
			backenable = 1;
	} else {			/* Perform qb_count accounting */
		wantw = qbp->qb_flag & QB_WANTW;
		for (tmp = mp; tmp; tmp = tmp->b_cont)
			qbp->qb_count -= (tmp->b_wptr - tmp->b_rptr);
		if ((qbp->qb_count < qbp->qb_hiwat) &&
		    (qbp->qb_flag & QB_FULL)) {
			qbp->qb_flag &= ~QB_FULL;
			q->q_blocked--;
		}
		if (qbp->qb_count <= qbp->qb_lowat)
			backenable = 1;
	}

	if (backenable) {
		/* find nearest back queue with service proc */
		for (nq = backq_l(q); nq && !(nq->q_qinfo->qi_srvp && (nq->q_flag & QPROCSON)); nq = backq_l(nq))
			;
		if (wantw) {
			if (mp->b_band != 0)
				qbp->qb_flag &= ~QB_WANTW;
			else
				q->q_flag &= ~QWANTW;
			if (nq) {
				qenable_l(nq);
				setqback(nq, mp->b_band);
			}
		}
		if ((q->q_flag & QWANTW) && (q->q_blocked == 0) &&
		    !(q->q_flag & QFULL) && nq) {
			/*
			 * this is the case where a rmvq pulled off a
			 * priority message and as a side effect, freed
			 * up flow control on band 0
			 */
			q->q_flag &= ~QWANTW;
			qenable_l(nq);
			setqback(nq, 0);
		}
	}
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
}

/*
 * void
 * flushq_l(queue_t *q, int flag)
 *	Same as flushq.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

void
flushq_l(queue_t *q, int flag)
{
	mblk_t *mp;
	mblk_t *nmp;
	qband_t *qbp;
	int backenable;
	unsigned char bpri;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(q);
	backenable = 0;
	mp = q->q_first;
	q->q_first = NULL;
	q->q_last = NULL;
	q->q_count = 0;
	for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
		qbp->qb_first = NULL;
		qbp->qb_last = NULL;
		qbp->qb_count = 0;
		qbp->qb_flag &= ~QB_FULL;
	}
	q->q_blocked = 0;
	q->q_flag &= ~QFULL;
	while (mp) {
#ifdef STRPERF
		mp->b_sched += castimer() - mp->b_stamp;
		mp->b_stamp = 0;
#endif
		nmp = mp->b_next;
		if (flag || datamsg(mp->b_datap->db_type)) {
#ifdef STRPERF
			mp->b_oh += castimer() - stamp;
#endif
			freemsg(mp);
		}
		else
			putq_l(q, mp);
		mp = nmp;
	}
	bzero((caddr_t)l.qbf, NBAND);
	bpri = 1;
	for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
		if ((qbp->qb_flag & QB_WANTW) && (qbp->qb_count <= qbp->qb_lowat)) {
			qbp->qb_flag &= ~QB_WANTW;
			backenable = 1;
			l.qbf[bpri] = 1;
		}
		bpri++;
	}
	if ((q->q_flag & QWANTW) && (q->q_count <= q->q_lowat)) {
		q->q_flag &= ~QWANTW;
		backenable = 1;
		l.qbf[0] = 1;
	}

	/*
	 * If any band can now be written to, and there is a writer
	 * for that band, then backenable the closest service procedure.
	 */
	if (backenable) {
		int i;

		/* find nearest back queue with service proc */
		for (q = backq_l(q); q && !q->q_qinfo->qi_srvp; q = backq_l(q))
			;
		if (q) {
			qenable_l(q);
			for (i = 0; i < (int)bpri; i++)
				if (l.qbf[i])
					setqback(q, i);
		}
	}
}

/*
 * void
 * flushq(queue_t *q, int flag)
 *	Empty a queue.  If flag is set, remove all messages.  Otherwise,
 *	remove only non-control messages.  If queue falls below its low
 *	water mark, and QWANTW is set , enable the nearest upstream
 *	service procedure.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

void
flushq(queue_t *q, int flag)
{
	pl_t pl;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	flushq_l(q, flag);
	UNLOCK(q->q_str->sd_mutex, pl);
}

/*
 * void
 * flushband_l(queue_t *q, unsigned char pri, int flag)
 *	Same as flushband.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */
STATIC void
flushband_l(queue_t *q, unsigned char pri, int flag)
{
	mblk_t *mp;
	mblk_t *nmp;
	mblk_t *last;
	qband_t *qbp;
	int band;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(q);
	if (pri > q->q_nband)
		return;
	if (pri == 0) {
		mp = q->q_first;
		q->q_first = NULL;
		q->q_last = NULL;
		q->q_count = 0;
		for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
			qbp->qb_first = NULL;
			qbp->qb_last = NULL;
			qbp->qb_count = 0;
			qbp->qb_flag &= ~QB_FULL;
		}
		q->q_blocked = 0;
		q->q_flag &= ~QFULL;
		while (mp) {
			nmp = mp->b_next;
			if ((mp->b_band == 0) &&
			    (mp->b_datap->db_type < QPCTL) &&
			    (flag || datamsg(mp->b_datap->db_type))) {
#ifdef STRPERF
				mp->b_oh += castimer() - stamp;
#endif
				freemsg(mp);
			}
			else
				putq_l(q, mp);
			mp = nmp;
		}
		if ((q->q_flag & QWANTW) && (q->q_count <= q->q_lowat)) {
			/* find nearest back queue with service proc */
			q->q_flag &= ~QWANTW;
			for (q = backq_l(q); q && !q->q_qinfo->qi_srvp; q = backq_l(q))
				;
			if (q) {
				qenable_l(q);
				setqback(q, pri);
			}
		}
	} else {	/* pri != 0 */
		ASSERT(q->q_bandp != NULL);
		band = pri;
		qbp = q->q_bandp;
		while (--band > 0)
			qbp = qbp->qb_next;
		mp = qbp->qb_first;
		if (mp == NULL)
			return;
		last = qbp->qb_last;
		if (mp == last)		/* only message in band */
			last = mp->b_next;
		while (mp != last) {
			nmp = mp->b_next;
			if (mp->b_band == pri) {
				if (flag || datamsg(mp->b_datap->db_type)) {
					rmvq(q, mp);
#ifdef STRPERF
					mp->b_oh += castimer() - stamp;
#endif
					freemsg(mp);
				}
			}
			mp = nmp;
		}
		if (mp && mp->b_band == pri) {
			if (flag || datamsg(mp->b_datap->db_type)) {
				rmvq(q, mp);
#ifdef STRPERF
				mp->b_oh += castimer() - stamp;
#endif
				freemsg(mp);
			}
		}
	}
}

/*
 * void
 * flushband(queue_t *q, unsigned char pri, int flag)
 *	Flush the queue of messages of the given priority band.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 *
 * Description:
 *	There is some duplication of code between flushq and flushband.
 *	This is because we want to optimize the code as much as possible.
 *	The assumption is that there will be more messages in the normal
 *	(priority 0) band than in any other.
 */

void
flushband(queue_t *q, unsigned char pri, int flag)
{
	pl_t pl;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	flushband_l(q, pri, flag);
	UNLOCK(q->q_str->sd_mutex, pl);
}

/*
 * int
 * canput_l(queue_t *q)
 *	Same as canput.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

int
canput_l(queue_t *q)
{
	if (!q)
		return(0);
	while (q->q_next && !(q->q_qinfo->qi_srvp && (q->q_flag & QPROCSON)))
		q = q->q_next;
	if ((q->q_flag & QFULL) || (q->q_blocked != 0)) {
		q->q_flag |= QWANTW;
		return(0);
	}
	return(1);
}

/*
 * int
 * canput(queue_t *q)
 *	Return 1 if the queue is not full.  If the queue is full, return
 *	0 (may not put message) and set QWANTW flag (caller wants to write
 *	to the queue).
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

int
canput(queue_t *q)
{
	pl_t pl;
	int ret;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	ret = canput_l(q);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(ret);
}

/*
 * int
 * canputnext(queue_t *q)
 *	Multiprocessor-safe equivalent of canput(q->q_next).
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

int
canputnext(queue_t *q)
{
	pl_t pl;
	int ret;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	ret = canput_l(q->q_next);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(ret);
}

/*
 * int
 * bcanput_l(queue_t *q, uchar_t pri)
 *	Same as bcanput.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

int
bcanput_l(queue_t *q, uchar_t pri)
{
	qband_t *qbp;

	if (!q)
		return(0);
	while (q->q_next && !(q->q_qinfo->qi_srvp && (q->q_flag & QPROCSON)))
		q = q->q_next;
	if (pri == 0) {
		if (q->q_flag & QFULL) {
			q->q_flag |= QWANTW;
			return(0);
		}
	} else {	/* pri != 0 */
		if (pri > q->q_nband) {

			/*
			 * No band exists yet, so return success.
			 */
			return(1);
		}
		qbp = q->q_bandp;
		while (--pri)
			qbp = qbp->qb_next;
		if (qbp->qb_flag & QB_FULL) {
			qbp->qb_flag |= QB_WANTW;
			return(0);
		}
	}
	return(1);
}

/*
 * int
 * bcanput(queue_t *q, uchar_t pri)
 *	This is the canput for use with priority bands.
 *
 * Calling/Exit State:
 *	Return 1 if the band is not full.  If the band is full, return 0
 *	(may not put message) and set QWANTW(QB_WANTW) flag for zero(non-zero)
 *	band (caller wants to write to the queue).  sd_mutex assumed unlocked.
 */

int
bcanput(queue_t *q, uchar_t pri)
{
	pl_t pl;
	int ret;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	ret = bcanput_l(q, pri);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(ret);
}

/*
 * int
 * bcanputnext(queue_t *q, uchar_t pri)
 *	Multiprocessor-safe equivalent of bcanput(q->q_next, pri).
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

int
bcanputnext(queue_t *q, uchar_t pri)
{
	pl_t pl;
	int ret;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	ret = bcanput_l(q->q_next, pri);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(ret);
}

/*
 * int
 * putq_l(queue_t *q, mblk_t *bp)
 *	Same as putq.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

int
putq_l(queue_t *q, mblk_t *bp)
{
	mblk_t *tmp;
	qband_t *qbp;
	int mcls;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(q && bp);
	qbp = NULL;
	mcls = (int) queclass(bp);

	/*
	 * Make sanity checks and if qband structure is not yet
	 * allocated, do so.
	 */
	if (mcls == QPCTL) {
		if (bp->b_band != 0)
			bp->b_band = 0;		/* force to be correct */
	} else if (bp->b_band != 0) {
		int i;
		qband_t **qbpp;

		if (bp->b_band > q->q_nband) {

			/*
			 * The qband structure for this priority band is
			 * not on the queue yet, so we have to allocate
			 * one on the fly.  It would be wasteful to
			 * associate the qband structures with every
			 * queue when the queues are allocated.  This is
			 * because most queues will only need the normal
			 * band of flow which can be described entirely
			 * by the queue itself.
			 */
			qbpp = &q->q_bandp;
			while (*qbpp)
				qbpp = &(*qbpp)->qb_next;
			while (bp->b_band > q->q_nband) {
				if ((*qbpp = allocband()) == NULL) {
#ifdef STRPERF
					bp->b_oh += castimer() - stamp;
#endif
					return(0);
				}
				(*qbpp)->qb_hiwat = q->q_hiwat;
				(*qbpp)->qb_lowat = q->q_lowat;
				q->q_nband++;
				qbpp = &(*qbpp)->qb_next;
			}
		}
		qbp = q->q_bandp;
		i = bp->b_band;
		while (--i)
			qbp = qbp->qb_next;
	}

	/*
	 * If queue is empty, add the message and initialize the pointers.
	 * Otherwise, adjust message pointers and queue pointers based on
	 * the type of the message and where it belongs on the queue.  Some
	 * code is duplicated to minimize the number of conditionals and
	 * hopefully minimize the amount of time this routine takes.
	 */
	if (!q->q_first) {
		bp->b_next = NULL;
		bp->b_prev = NULL;
		q->q_first = bp;
		q->q_last = bp;
		if (qbp) {
			qbp->qb_first = bp;
			qbp->qb_last = bp;
		}
#ifdef STRPERF
		bp->b_stamp = castimer();
		bp->b_oh += castimer() - stamp;
#endif
	} else if (!qbp) {	/* bp->b_band == 0 */

		/* 
		 * If queue class of message is less than or equal to
		 * that of the last one on the queue, tack on to the end.
		 */
		tmp = q->q_last;
		if (mcls <= (int)queclass(tmp)) {
			bp->b_next = NULL;
			bp->b_prev = tmp;
			tmp->b_next = bp;
			q->q_last = bp;
		} else {
			tmp = q->q_first;
			while ((int)queclass(tmp) >= mcls)
				tmp = tmp->b_next;
			ASSERT(tmp != NULL);

			/*
			 * Insert bp before tmp.
			 */
			bp->b_next = tmp;
			bp->b_prev = tmp->b_prev;
			if (tmp->b_prev)
				tmp->b_prev->b_next = bp;
			else
				q->q_first = bp;
			tmp->b_prev = bp;
		}
#ifdef STRPERF
		bp->b_stamp = castimer();
		bp->b_oh += castimer() - stamp;
#endif
	} else {		/* bp->b_band != 0 */
		if (qbp->qb_first) {
			ASSERT(qbp->qb_last != NULL);
			tmp = qbp->qb_last;

			/*
			 * Insert bp after the last message in this band.
			 */
			bp->b_next = tmp->b_next;
			if (tmp->b_next)
				tmp->b_next->b_prev = bp;
			else
				q->q_last = bp;
			bp->b_prev = tmp;
			tmp->b_next = bp;
		} else {
			tmp = q->q_last;
			if ((mcls < (int)queclass(tmp)) ||
			    (bp->b_band <= tmp->b_band)) {

				/*
				 * Tack bp on end of queue.
				 */
				bp->b_next = NULL;
				bp->b_prev = tmp;
				tmp->b_next = bp;
				q->q_last = bp;
			} else {
				tmp = q->q_first;
				while (tmp->b_datap->db_type >= QPCTL)
					tmp = tmp->b_next;
				while (tmp->b_band >= bp->b_band)
					tmp = tmp->b_next;
				ASSERT(tmp != NULL);

				/*
				 * Insert bp before tmp.
				 */
				bp->b_next = tmp;
				bp->b_prev = tmp->b_prev;
				if (tmp->b_prev)
					tmp->b_prev->b_next = bp;
				else
					q->q_first = bp;
				tmp->b_prev = bp;
			}
			qbp->qb_first = bp;
		}
		qbp->qb_last = bp;
#ifdef STRPERF
		bp->b_stamp = castimer();
		bp->b_oh += castimer() - stamp;
#endif
	}

#ifdef STRPERF
	stamp = castimer();
#endif
	if (qbp) {
		for (tmp = bp; tmp; tmp = tmp->b_cont)
			qbp->qb_count += (tmp->b_wptr - tmp->b_rptr);
#ifdef STRPERF
		bp->b_oh += castimer() - stamp;
#endif
		if ((qbp->qb_count >= qbp->qb_hiwat) &&
		    !(qbp->qb_flag & QB_FULL)) {
			qbp->qb_flag |= QB_FULL;
			q->q_blocked++;
		}
	} else {
		for (tmp = bp; tmp; tmp = tmp->b_cont)
			q->q_count += (tmp->b_wptr - tmp->b_rptr);
#ifdef STRPERF
		bp->b_oh += castimer() - stamp;
#endif
		if (q->q_count >= q->q_hiwat)
			q->q_flag |= QFULL;
	}
	/*
	 * This test is rather complicated now.  We do the qenable if
	 * 1) it is a high priority message, or
	 * 2) the queue can be enabled (i.e. no noenable done) and a
	 *    previous getq failed, or
	 * 3) the queue can be enabled, QWANTR is not set (indicating that
	 *    flow control has been exerted) and the current message is
	 *    in a newer (and higher) priority band
	 * Case 3 bears more explanation.  If QWANTR is not set, then the
	 * service procedure terminated early due to flow control and messages
	 * are still pending.  If a new message comes in and is queued at the
	 * head of the list, by definition it must be the highest priority
	 * band currently on the list.  Modules/drivers that support multiple
	 * priority bands will do a bcanputnext and will be allowed to send
	 * up this new message, although the previously queued ones will
	 * probably stay stuck.  A module/driver that does not support multiple
	 * bands will do a canputnext, which will still fail due to flow
	 * control constraints.
	 */
	if ((mcls > QNORM) || (canenable(q) && ((q->q_flag & QWANTR) ||
			((bp == q->q_first) && (bp->b_band > 0)))))
		qenable_l(q);
	return(1);
}

/*
 * int
 * putq(queue_t *q, mblk_t *bp)
 *	Put a message on a queue.  
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 *
 * Description:
 *	Messages are enqueued on a priority basis.  The priority classes
 *	are HIGH PRIORITY (type >= QPCTL), PRIORITY (type < QPCTL && band > 0),
 *	and B_NORMAL (type < QPCTL && band == 0). 
 *
 *	If queue hits high water mark then set QFULL flag.
 *
 *	If QNOENAB is not set (putq is allowed to enable the queue),
 *	enable the queue only if the message is PRIORITY,
 *	or the QWANTR flag is set (indicating that the service procedure 
 *	is ready to read the queue.  This implies that a service 
 *	procedure must NEVER put a priority message back on its own
 *	queue, as this would result in an infinite loop (!).
 */

int
putq(queue_t *q, mblk_t *bp)
{
	pl_t pl;
	int ret;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	ret = putq_l(q, bp);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(ret);
}

/*
 * int
 * putbq_l(queue_t *q, mblk_t *bp)
 *	Same as putbq.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

int
putbq_l(queue_t *q, mblk_t *bp)
{
	mblk_t *tmp;
	qband_t *qbp;
	int mcls;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(q && bp);
	ASSERT(bp->b_next == NULL);
	qbp = NULL;
	mcls = (int) queclass(bp);

	/*
	 * Make sanity checks and if qband structure is not yet
	 * allocated, do so.
	 */
	if (mcls == QPCTL) {
		if (bp->b_band != 0)
			bp->b_band = 0;		/* force to be correct */
	} else if (bp->b_band != 0) {
		int i;
		qband_t **qbpp;

		if (bp->b_band > q->q_nband) {
			qbpp = &q->q_bandp;
			while (*qbpp)
				qbpp = &(*qbpp)->qb_next;
			while (bp->b_band > q->q_nband) {
				if ((*qbpp = allocband()) == NULL) {
#ifdef STRPERF
					bp->b_oh += castimer() - stamp;
#endif
					return(0);
				}
				(*qbpp)->qb_hiwat = q->q_hiwat;
				(*qbpp)->qb_lowat = q->q_lowat;
				q->q_nband++;
				qbpp = &(*qbpp)->qb_next;
			}
		}
		qbp = q->q_bandp;
		i = bp->b_band;
		while (--i)
			qbp = qbp->qb_next;
	}

	/* 
	 * If queue is empty or if message is high priority,
	 * place on the front of the queue.
	 */
	tmp = q->q_first;
	if ((!tmp) || (mcls == QPCTL)) {
		bp->b_next = tmp;
		if (tmp)
			tmp->b_prev = bp;
		else
			q->q_last = bp;
		q->q_first = bp;
		bp->b_prev = NULL;
		if (qbp) {
			qbp->qb_first = bp;
			qbp->qb_last = bp;
		}
#ifdef STRPERF
		bp->b_stamp = castimer();
		bp->b_oh += castimer() - stamp;
#endif
	} else if (qbp) {	/* bp->b_band != 0 */
		tmp = qbp->qb_first;
		if (tmp) {

			/*
			 * Insert bp before the first message in this band.
			 */
			bp->b_next = tmp;
			bp->b_prev = tmp->b_prev;
			if (tmp->b_prev)
				tmp->b_prev->b_next = bp;
			else
				q->q_first = bp;
			tmp->b_prev = bp;
		} else {
			tmp = q->q_last;
			if ((mcls < (int)queclass(tmp)) ||
			    (bp->b_band < tmp->b_band)) {

				/*
				 * Tack bp on end of queue.
				 */
				bp->b_next = NULL;
				bp->b_prev = tmp;
				tmp->b_next = bp;
				q->q_last = bp;
			} else {
				tmp = q->q_first;
				while (tmp->b_datap->db_type >= QPCTL)
					tmp = tmp->b_next;
				while (tmp->b_band > bp->b_band)
					tmp = tmp->b_next;
				ASSERT(tmp != NULL);

				/*
				 * Insert bp before tmp.
				 */
				bp->b_next = tmp;
				bp->b_prev = tmp->b_prev;
				if (tmp->b_prev)
					tmp->b_prev->b_next = bp;
				else
					q->q_first = bp;
				tmp->b_prev = bp;
			}
			qbp->qb_last = bp;
		}
		qbp->qb_first = bp;
#ifdef STRPERF
		bp->b_stamp = castimer();
		bp->b_oh += castimer() - stamp;
#endif
	} else {		/* bp->b_band == 0 && !QPCTL */

		/*
		 * If the queue class or band is less than that of the last
		 * message on the queue, tack bp on the end of the queue.
		 */
		tmp = q->q_last;
		if ((mcls < (int)queclass(tmp)) || (bp->b_band < tmp->b_band)) {
			bp->b_next = NULL;
			bp->b_prev = tmp;
			tmp->b_next = bp;
			q->q_last = bp;
		} else {
			tmp = q->q_first;
			while (tmp->b_datap->db_type >= QPCTL)
				tmp = tmp->b_next;
			while (tmp->b_band > bp->b_band)
				tmp = tmp->b_next;
			ASSERT(tmp != NULL);

			/*
			 * Insert bp before tmp.
			 */
			bp->b_next = tmp;
			bp->b_prev = tmp->b_prev;
			if (tmp->b_prev)
				tmp->b_prev->b_next = bp;
			else
				q->q_first = bp;
			tmp->b_prev = bp;
		}
#ifdef STRPERF
		bp->b_stamp = castimer();
		bp->b_oh += castimer() - stamp;
#endif
	}

#ifdef STRPERF
	stamp = castimer();
#endif
	if (qbp) {
		for (tmp = bp; tmp; tmp = tmp->b_cont)
			qbp->qb_count += (tmp->b_wptr - tmp->b_rptr);
#ifdef STRPERF
		bp->b_oh += castimer() - stamp;
#endif
		if ((qbp->qb_count >= qbp->qb_hiwat) &&
		    !(qbp->qb_flag & QB_FULL)) {
			qbp->qb_flag |= QB_FULL;
			q->q_blocked++;
		}
	} else {
		for (tmp = bp; tmp; tmp = tmp->b_cont)
			q->q_count += (tmp->b_wptr - tmp->b_rptr);
#ifdef STRPERF
		bp->b_oh += castimer() - stamp;
#endif
		if (q->q_count >= q->q_hiwat)
			q->q_flag |= QFULL;
	}
	if ((mcls > QNORM) || (canenable(q) && (q->q_flag & QWANTR)))
		qenable_l(q);
	return(1);
}

/*
 * int
 * putbq(queue_t *q, mblk_t *bp)
 *	Put stuff back at beginning of Q according to priority order.
 *	See comment on putq above for details.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

int
putbq(queue_t *q, mblk_t *bp)
{
	pl_t pl;
	int ret;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	ret = putbq_l(q, bp);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(ret);
}


/*
 * int
 * insq(queue_t *q, mblk_t *emp, mblk_t *mp)
 *	Insert a message before an existing message on the queue.  If the
 *	existing message is NULL, the new messages is placed on the end of
 *	the queue.  The queue class of the new message is ignored.  However,
 *	the priority band of the new message must adhere to the following
 *	ordering:
 *
 *		emp->b_prev->b_band >= mp->b_band >= emp->b_band.
 *
 * Calling/Exit State:
 *	All flow control parameters are updated.  sd_mutex assumed locked.
 */

int
insq(queue_t *q, mblk_t *emp, mblk_t *mp)
{
	mblk_t *tmp;
	qband_t *qbp;
	int mcls;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	qbp = NULL;
	mcls = (int) queclass(mp);
	if (mcls == QPCTL) {
		if (mp->b_band != 0)
			mp->b_band = 0;		/* force to be correct */
		if (emp && emp->b_prev &&
		    (emp->b_prev->b_datap->db_type < QPCTL))
			goto badord;
	}
	if (emp) {
		if (((mcls == QNORM) && (mp->b_band < emp->b_band)) ||
		    (emp->b_prev && (emp->b_prev->b_datap->db_type < QPCTL) &&
		    (emp->b_prev->b_band < mp->b_band))) {
			goto badord;
		}
	} else {
		tmp = q->q_last;
		if (tmp && (mcls == QNORM) && (mp->b_band > tmp->b_band)) {
badord:
			/*
			 *+ An attempt was made to insert a streams message
			 *+ onto a queue in an invalid spot (i.e. the partial
			 *+ ordering was not maintained).  This indicates a
			 *+ bug in the routine calling this one.
			 */
			cmn_err(CE_WARN, "insq: attempt to insert message out of order on q %x\n", q);
			return(0);
		}
	}

	if (mp->b_band != 0) {
		int i;
		qband_t **qbpp;

		if (mp->b_band > q->q_nband) {
			qbpp = &q->q_bandp;
			while (*qbpp)
				qbpp = &(*qbpp)->qb_next;
			while (mp->b_band > q->q_nband) {
				if ((*qbpp = allocband()) == NULL) {
#ifdef STRPERF
					mp->b_oh += castimer() - stamp;
#endif
					return(0);
				}
				(*qbpp)->qb_hiwat = q->q_hiwat;
				(*qbpp)->qb_lowat = q->q_lowat;
				q->q_nband++;
				qbpp = &(*qbpp)->qb_next;
			}
		}
		qbp = q->q_bandp;
		i = mp->b_band;
		while (--i)
			qbp = qbp->qb_next;
	}

	if ((mp->b_next = emp) != NULL) {
		if ((mp->b_prev = emp->b_prev) != NULL)
			emp->b_prev->b_next = mp;
		else
			q->q_first = mp;
		emp->b_prev = mp;
	} else {
		if ((mp->b_prev = q->q_last) != NULL)
			q->q_last->b_next = mp;
		else 
			q->q_first = mp;
		q->q_last = mp;
	}

#ifdef STRPERF
	mp->b_stamp = castimer();
#endif
	if (qbp) {	/* adjust qband pointers and count */
		if (!qbp->qb_first) {
			qbp->qb_first = mp;
			qbp->qb_last = mp;
		} else {
			if (qbp->qb_first == emp)
				qbp->qb_first = mp;
			else if (mp->b_next && (mp->b_next->b_band !=
			    mp->b_band))
				qbp->qb_last = mp;
		}
#ifdef STRPERF
		mp->b_stamp = castimer();
#endif
		for (tmp = mp; tmp; tmp = tmp->b_cont)
			qbp->qb_count += (tmp->b_wptr - tmp->b_rptr);
		if ((qbp->qb_count >= qbp->qb_hiwat) &&
		    !(qbp->qb_flag & QB_FULL)) {
			qbp->qb_flag |= QB_FULL;
			q->q_blocked++;
		}
	} else {
		for (tmp = mp; tmp; tmp = tmp->b_cont)
			q->q_count += (tmp->b_wptr - tmp->b_rptr);
		if (q->q_count >= q->q_hiwat)
			q->q_flag |= QFULL;
	}
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
	if (canenable(q) && (q->q_flag & QWANTR))
		qenable_l(q);
	return(1);
}

/*
 * void
 * put(queue_t *q, mblk_t *mp)
 *	Pass a message to q's put procedure.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

void
put(queue_t *q, mblk_t *mp)
{
	pl_t pl;
	struct strunidata *sup;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(q && mp);
	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	q->q_putcnt++;
	if (q->q_defcnt || ((q->q_flag & QBOUND) && (q->q_str->sd_cpu != l.eng))) {
		/* on wrong cpu */
		sup =  kmem_alloc(sizeof(struct strunidata), KM_NOSLEEP);
		if (sup == NULL) {
			/* this is bad! */
			q->q_putcnt--;
			UNLOCK(q->q_str->sd_mutex, pl);
#ifdef STRPERF
			mp->b_oh += castimer() - stamp;
#endif
			freemsg(mp);
			return;
		}
		q->q_defcnt++;
		UNLOCK(q->q_str->sd_mutex, pl);
		sup->su_qp = q;
		sup->su_mp = mp;
		*(Strcount_local + l.eng_num) += sizeof(struct strunidata);
#ifdef STRPERF
		mp->b_oh += castimer() - stamp;
#endif
		if (dtimeout(struniput, sup, 0, PLSTR, q->q_str->sd_cpu - engine) == 0) {
			/*
			 *+ Could not allocate timeout event to move message
			 *+ to the correct processor, dropping message
			 */
			cmn_err(CE_WARN, "put: dropping message\n");
			freemsg(mp);
			kmem_free(sup, sizeof(struct strunidata));
			*(Strcount_local + l.eng_num) -= 
				sizeof(struct strunidata);
			pl = LOCK(q->q_str->sd_mutex, PLSTR);
			q->q_defcnt--;
			q->q_putcnt--;
			UNLOCK(q->q_str->sd_mutex, pl);
		}
		return;
	}
	UNLOCK(q->q_str->sd_mutex, pl);
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
	(*q->q_putp)(q, mp);
	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	q->q_putcnt--;
	if ((q->q_flag & QFREEZE) && (q->q_putcnt == 0)) {
		UNLOCK(q->q_str->sd_mutex, pl);
		SV_SIGNAL(q->q_str->sd_freeze, 0);
	} else {
		UNLOCK(q->q_str->sd_mutex, pl);
	}
}

/*
 * int
 * putnext_l(queue_t *q, mblk_t *mp)
 *	Pass a message to the next queue's put procedure.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 *
 * Description:
 *	This is a version of putnext that assumes sd_mutex is held on entry.
 *	The lock is dropped to perform the actual put and is reacquired before
 *	leaving.  This routine exists for the stream head, which has occassion
 *	to put messages downstream with sd_mutex already held.
 */

int
putnext_l(queue_t *q, mblk_t *mp)
{
	struct strunidata *sup;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(q && mp);
	ASSERT(q->q_next);
	q = q->q_next;
	q->q_putcnt++;
	if (q->q_defcnt || ((q->q_flag & QBOUND) && (q->q_str->sd_cpu != l.eng))) {
		/* on wrong cpu */
		sup =  kmem_alloc(sizeof(struct strunidata), KM_NOSLEEP);
		if (sup == NULL) {
			/* this is bad! */
			q->q_putcnt--;
#ifdef STRPERF
			mp->b_oh += castimer() - stamp;
#endif
			freemsg(mp);
			return(0);
		}
		sup->su_qp = q;
		sup->su_mp = mp;
		q->q_defcnt++;
		*(Strcount_local + l.eng_num) += sizeof(struct strunidata);
#ifdef STRPERF
		mp->b_oh += castimer() - stamp;
#endif
		if (dtimeout(struniput, sup, 0, PLSTR, q->q_str->sd_cpu - engine) == 0) {
			/*
			 *+ Could not allocate timeout event to move message
			 *+ to the correct processor, dropping message
			 */
			cmn_err(CE_WARN, "putnext_l: dropping message\n");
			freemsg(mp);
			kmem_free(sup, sizeof(struct strunidata));
			*(Strcount_local + l.eng_num) -= 
				sizeof(struct strunidata);
			q->q_defcnt--;
			q->q_putcnt--;
		}
		return(0);
	}
	UNLOCK(q->q_str->sd_mutex, getpl());
#ifdef STRPERF
	mp->b_oh += castimer() - stamp;
#endif
	(*q->q_putp)(q, mp);
	(void) LOCK(q->q_str->sd_mutex, getpl());
	q->q_putcnt--;
	if ((q->q_flag & QFREEZE) && (q->q_putcnt == 0))
		/* can't optimize the locking on this one */
		SV_SIGNAL(q->q_str->sd_freeze, 0);
	return(0);
}

/*
 * int
 * putnext(queue_t *q, mblk_t *mp)
 *	Pass a message to the next queue's put procedure.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.  Really should be a void, but DDI/DKI
 *	has it returning an int.
 */

int
putnext(queue_t *q, mblk_t *mp)
{
	pl_t pl;

	ASSERT(q && mp);
	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	(void) putnext_l(q, mp);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(0);
}

/*
 * int
 * putctl(queue_t *q, int type)
 *	Create and put a control message on queue.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.  Returns 1 on success, 0 on failure.
 */

int
putctl(queue_t *q, int type)
{
	mblk_t *bp;
#ifdef STRPERF
	int stamp;
#endif

	if ((datamsg(type) && (type != M_DELAY)) || !(bp = allocb(0, BPRI_HI)))
		return(0);
#ifdef STRPERF
	stamp = castimer();
#endif
	bp->b_datap->db_type = (uchar_t) type;
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
#endif
	put(q, bp);
	return(1);
}

/*
 * int
 * putnextctl(queue_t *q, int type)
 *	Multiprocessor-safe equivalent of putctl(q->q_next, type).
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.  Returns 1 on success, 0 on failure.
 */

int
putnextctl(queue_t *q, int type)
{
	mblk_t *bp;
#ifdef STRPERF
	int stamp;
#endif

	if ((datamsg(type) && (type != M_DELAY)) || !(bp = allocb(0, BPRI_HI)))
		return(0);
#ifdef STRPERF
	stamp = castimer();
#endif
	bp->b_datap->db_type = (uchar_t) type;
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
#endif
	putnext(q, bp);
	return(1);
}

/*
 * int
 * putctl1(queue_t *q, int type, int param)
 *	Create and put a control message with a single-byte
 *	parameter on queue.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.  Returns 1 on success, 0 on failure.
 */

int
putctl1(queue_t *q, int type, int param)
{
	mblk_t *bp;
#ifdef STRPERF
	int stamp;
#endif

	if ((datamsg(type) && (type != M_DELAY)) || !(bp = allocb(1, BPRI_HI)))
		return(0);
#ifdef STRPERF
	stamp = castimer();
#endif
	bp->b_datap->db_type = (uchar_t) type;
	*bp->b_wptr++ = (unsigned char) param;
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
#endif
	put(q, bp);
	return(1);
}

/*
 * int
 * putnextctl1(queue_t *q, int type, int param)
 *	Multiprocessor-safe equivalent of putctl1(q->q_next, type, param).
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.  Returns 1 on success, 0 on failure.
 */

int
putnextctl1(queue_t *q, int type, int param)
{
	mblk_t *bp;
#ifdef STRPERF
	int stamp;
#endif

	if ((datamsg(type) && (type != M_DELAY)) || !(bp = allocb(1, BPRI_HI)))
		return(0);
#ifdef STRPERF
	stamp = castimer();
#endif
	bp->b_datap->db_type = (uchar_t) type;
	*bp->b_wptr++ = (unsigned char) param;
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
#endif
	putnext(q, bp);
	return(1);
}

/*
 * queue_t *
 * backq_l(queue_t *q)
 *	Return the queue upstream from this one.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

queue_t *
backq_l(queue_t *q)
{
	ASSERT(q);
	q = OTHERQ(q);
	if (q->q_next) {
		q = q->q_next;
		return(OTHERQ(q));
	}
	return(NULL);
}

/*
 * void
 * qreply(queue_t *q, mblk_t *bp)
 *	Send a block back up the queue in reverse from this
 *	one (e.g. to respond to ioctls).
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

void
qreply(queue_t *q, mblk_t *bp)
{
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif
	ASSERT(q && bp);
	q = OTHERQ(q);
#ifdef STRPERF
	bp->b_oh += castimer() - stamp;
#endif
	putnext(q, bp);
}

/*
 * void
 * qenable_l(queue_t *q)
 *	Same as qenable.
 *
 * Calling/Exit State:
 *	svc_mutex assumed unlocked.  sd_mutex assumed locked.
 */

void
qenable_l(queue_t *q)
{
	pl_t pl;
	struct qsvc *svcp;
	int bound;
	extern void svc_enqueue(queue_t *, struct qsvc *);
	extern void upbackfix(queue_t *);

	ASSERT(q);
	ASSERT(q->q_flag & QPROCSON);
	if (!q->q_qinfo->qi_srvp)
		return;
	if (q->q_flag & QFREEZE) {

		/* 
		 * This queue should not be enqueued for service now,
		 * since its being frozen.  Just flag it as QTOENAB
		 * which can be used to qenable it later, if desired.
		 */
		q->q_flag |= QTOENAB;
		return;
	}
	/* Only need the lock if we're playing around with the global list */
	bound = q->q_flag & QBOUND;
	if (bound) {
		/*
		 * This can only happen on a backenable from strread, "lock"
		 * the queue by bumping q_putcnt and finish scheduling on
		 * the correct processor.
		 */
		if (l.eng != q->q_str->sd_cpu) {
			q->q_putcnt++;
			if (dtimeout(upbackfix, q, 0, PLSTR, q->q_str->sd_cpu - engine) == 0) {
				/*
				 * Ick, the backenable effectively failed,
				 * make the stream head believe it has to
				 * do it again later and hope for more
				 * memory.
				 */
				q->q_putcnt--;
				RD(q->q_str->sd_wrq)->q_flag |= QWANTW;
			}
			return;
		}
		ASSERT(l.eng == q->q_str->sd_cpu);
		svcp = &l.qsvc;
		pl = splstr();
	} else {
		svcp = &qsvc;	
		pl = LOCK(&svc_mutex, PLSTR);
	}

	/*
	 * Do not place on run queue if already enabled.
	 */
	if (q->q_svcflag & QENAB) {
		if (bound)
			splx(pl);
		else
			UNLOCK(&svc_mutex, pl);
		return;
	}

	/*
	 * Mark queue enabled and place on run list.
	 */
	q->q_svcflag |= QENAB;
	svc_enqueue(q, svcp);

	/* 
	 * If not enough streams queue schedulers running, start one.
	 * Basically this is the place that controls the parallelism
	 * of the STREAMS queues.  The following assumes that as many
	 * scheduling entities as there are online processors, may be
	 * run in parallel.  If more sophistication is desired (i.e.
	 * only some processors should be utilized by the streams
	 * scheduler) then the following algorithm should be modified.
	 */
	if (bound) {
		splx(pl);
		setqsched(q->q_str->sd_cpu);
	} else {
		if (Nsched < nonline) {
			UNLOCK(&svc_mutex, pl);
			setqsched(NULL);
		} else {
			UNLOCK(&svc_mutex, pl);
		}
	}

	q->q_flag &= ~QTOENAB;	/* mutexed by sd_mutex */
}

/*
 * void
 * qenable(queue_t *q)
 *	Enable a queue: put it on list of those whose service procedures are
 *	ready to run and set up the scheduling mechanism.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

void
qenable(queue_t *q)
{
	pl_t pl;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	qenable_l(q);
	UNLOCK(q->q_str->sd_mutex, pl);
}

/*
 * int
 * qsize_l(queue_t *q)
 *	Same as qsize.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

int
qsize_l(queue_t *q)
{
	mblk_t *mp;
	int count;

	ASSERT(q);
	count = 0;
	for (mp = q->q_first; mp; mp = mp->b_next)
		count++;
	return(count);
}

/* 
 * int
 * qsize(queue_t *q)
 *	Return number of messages on queue.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

int
qsize(queue_t *q)
{
	pl_t pl;
	int count;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	count = qsize_l(q);
	UNLOCK(q->q_str->sd_mutex, pl);
	return(count);
}

/*
 * void
 * noenable(queue_t *q)
 *	Set queue flag so that putq() will not enable it.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

void
noenable(queue_t *q)
{ 
	pl_t pl;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	q->q_flag |= QNOENB;
	UNLOCK(q->q_str->sd_mutex, pl);
}

/*
 * void
 * enableok(queue_t *q)
 *	Set queue flag so that putq() can enable it.
 *
 * Calling/Exit State:
 *	Stream assumed unlocked.
 */

void
enableok(queue_t *q)
{
	pl_t pl;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	q->q_flag &= ~QNOENB;
	UNLOCK(q->q_str->sd_mutex, pl);
}

/*
 * int
 * strqset(queue_t *q, qfields_t what, unsigned char pri, long val)
 *	Set queue fields.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

int
strqset(queue_t *q, qfields_t what, unsigned char pri, long val)
{
	qband_t *qbp;
	int error;

	qbp = NULL;
	error = 0;
	if (what >= QBAD)
		return(EINVAL);
	if (pri != 0) {
		int i;
		qband_t **qbpp;

		if (pri > q->q_nband) {
			qbpp = &q->q_bandp;
			while (*qbpp)
				qbpp = &(*qbpp)->qb_next;
			while (pri > q->q_nband) {
				if ((*qbpp = allocband()) == NULL)
					return(EAGAIN);
				(*qbpp)->qb_hiwat = q->q_hiwat;
				(*qbpp)->qb_lowat = q->q_lowat;
				q->q_nband++;
				qbpp = &(*qbpp)->qb_next;
			}
		}
		qbp = q->q_bandp;
		i = pri;
		while (--i)
			qbp = qbp->qb_next;
	}
	switch (what) {
	case QHIWAT:
		if (qbp)
			qbp->qb_hiwat = (ulong)val;
		else
			q->q_hiwat = (ulong)val;
		break;

	case QLOWAT:
		if (qbp)
			qbp->qb_lowat = (ulong)val;
		else
			q->q_lowat = (ulong)val;
		break;

	case QMAXPSZ:
		if (qbp)
			error = EINVAL;
		else
			q->q_maxpsz = val;
		break;

	case QMINPSZ:
		if (qbp)
			error = EINVAL;
		else
			q->q_minpsz = val;
		break;

	case QCOUNT:
	case QFIRST:
	case QLAST:
	case QFLAG:
		error = EPERM;
		break;

	default:
		error = EINVAL;
		break;
	}
	return(error);
}

/*
 * int
 * strqget(queue_t *q, qfields_t what, unsigned char pri, long *valp)
 *	Get queue fields.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

int
strqget(queue_t *q, qfields_t what, unsigned char pri, long *valp)
{
	qband_t *qbp;
	int error;

	qbp = NULL;
	error = 0;
	if (what >= QBAD)
		return(EINVAL);
	if (pri != 0) {
		int i;
		qband_t **qbpp;

		if (pri > q->q_nband) {
			qbpp = &q->q_bandp;
			while (*qbpp)
				qbpp = &(*qbpp)->qb_next;
			while (pri > q->q_nband) {
				if ((*qbpp = allocband()) == NULL)
					return(EAGAIN);
				(*qbpp)->qb_hiwat = q->q_hiwat;
				(*qbpp)->qb_lowat = q->q_lowat;
				q->q_nband++;
				qbpp = &(*qbpp)->qb_next;
			}
		}
		qbp = q->q_bandp;
		i = pri;
		while (--i)
			qbp = qbp->qb_next;
	}
	switch (what) {
	case QHIWAT:
		if (qbp)
			*(ulong *)valp = qbp->qb_hiwat;
		else
			*(ulong *)valp = q->q_hiwat;
		break;

	case QLOWAT:
		if (qbp)
			*(ulong *)valp = qbp->qb_lowat;
		else
			*(ulong *)valp = q->q_lowat;
		break;

	case QMAXPSZ:
		if (qbp)
			error = EINVAL;
		else
			*(long *)valp = q->q_maxpsz;
		break;

	case QMINPSZ:
		if (qbp)
			error = EINVAL;
		else
			*(long *)valp = q->q_minpsz;
		break;

	case QCOUNT:
		if (qbp)
			*(ulong *)valp = qbp->qb_count;
		else
			*(ulong *)valp = q->q_count;
		break;

	case QFIRST:
		if (qbp)
			*(mblk_t **)valp = qbp->qb_first;
		else
			*(mblk_t **)valp = q->q_first;
		break;

	case QLAST:
		if (qbp)
			*(mblk_t **)valp = qbp->qb_last;
		else
			*(mblk_t **)valp = q->q_last;
		break;

	case QFLAG:
		if (qbp)
			*(ulong *)valp = qbp->qb_flag;
		else
			*(ulong *)valp = q->q_flag;
		break;

	default:
		error = EINVAL;
		break;
	}
	return(error);
}

/*
 * pl_t
 * freezestr(queue_t *q)
 *	Freeze a stream.  Does not stop current activity.  Merely prevents
 *	state of stream from changing and no more put or service procedures
 *	will be started until unfrozen.
 *
 * Calling/Exit State:
 *	sd_mutex unlocked on entry, locked on exit.  IPL set to PLSTR
 *	and old IPL is returned.
 */

pl_t
freezestr(queue_t *q)
{
	return(LOCK(q->q_str->sd_mutex, PLSTR));
}

/*
 * void
 * unfreezestr(queue_t *q, pl_t pl)
 *	Unfreeze a stream.
 *
 * Calling/Exit State:
 *	sd_mutex locked on entry, unlocked on exit.  IPL set to pl.
 */

void
unfreezestr(queue_t *q, pl_t pl)
{
	UNLOCK(q->q_str->sd_mutex, pl);
}

/* 
 * void
 * qprocson(queue_t *q)
 *	Enable the put and service procedures of the queue pair.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 *
 * Description:
 *	Prior to this call, the put procedures will be logically equivalent
 *	to "putnext", i.e.  the module will be bypassed by all traffic.
 *	Hence no flow-control flags need to be fussed.  However, for safety
 *	reasons, the queue-pair is frozen before the switch is done.
 */

void
qprocson(queue_t *q)
{
	pl_t pl;
	queue_t *wq;

	ASSERT(q);
	ASSERT(q->q_flag & QREADR);
	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	if (!(q->q_flag & QPROCSON)) {
		wq = WR(q);
		freezeprocs(q);
		if (q->q_flag & QINTER) {
			q->q_putp = strintercept;
			wq->q_putp = strintercept;
		} else {
			q->q_putp = q->q_qinfo->qi_putp;
			wq->q_putp = wq->q_qinfo->qi_putp;
		}
		q->q_flag |= QPROCSON;
		wq->q_flag |= QPROCSON;
	}
	UNLOCK(q->q_str->sd_mutex, pl);
}

/*
 * void
 * qprocsoff(queue_t *q)
 *	Wait until all currently active put and service procedures
 *	finishes, and disable all further activation of these procedures
 *	(by unlinking the module from the stream).
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.  svc_mutex assumed unlocked.
 *
 * Description:
 *	Does not free the queue pair.  No locks should be held by the
 *	caller since this routine may sleep before it finishes.
 */

void
qprocsoff(queue_t *q)
{
	pl_t pl;
	pl_t pl_1;
	queue_t *wq;
	struct qsvc *svcp;
	int bound;

	ASSERT(q);
	ASSERT(q->q_flag & QREADR);
	wq = WR(q);
	pl = LOCK(q->q_str->sd_mutex, PLSTR);

	/*
	 * Dequeue the queue pair if scheduled to run.
	 */
	bound = q->q_flag & QBOUND;
	if (bound) {
		svcp = &l.qsvc;
	} else {
		svcp = &qsvc;
		pl_1 = LOCK(&svc_mutex, PLSTR);
	}
	if (q->q_svcflag & QENAB) {
		svc_dequeue(q, svcp);
		q->q_svcflag &= ~QENAB;
	}
	if (wq->q_svcflag & QENAB) {
		svc_dequeue(wq, svcp);
		wq->q_svcflag &= ~QENAB;
	}
	if (!bound)
		UNLOCK(&svc_mutex, pl_1);
	/* still at PLSTR */
	freezeprocs(q);
	flushq_l(q, FLUSHALL);
	flushq_l(wq, FLUSHALL);

	/*
	 * Replace the put procedures so messages flow around the
	 * queues and mark the procedures as disabled.
	 */
	q->q_putp = putnext;
	wq->q_putp = putnext;
	q->q_flag &= ~QPROCSON;
	wq->q_flag &= ~QPROCSON;
	UNLOCK(q->q_str->sd_mutex, pl);
}


/*
 * int
 * SAMESTR_l(queue_t *q)
 *	Given a queue, determine if the next queue in the stream
 *	is the same type.  If not, we are at a pipe boundary where
 *	the stream twists.
 *
 * Calling/Exit State:
 *	sd_mutex assumed locked.
 */

int
SAMESTR_l(queue_t *q)
{
	int ret;

	ASSERT(q);
	ret = (((q)->q_next) && (((q)->q_flag&QREADR) == ((q)->q_next->q_flag&QREADR)));
	return(ret);
}


/*
 * int
 * SAMESTR(queue_t *q)
 *	Given a queue, determine if the next queue in the stream
 *	is the same type.  If not, we are at a pipe boundary where
 *	the stream twists.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */

int
SAMESTR(queue_t *q)
{
	pl_t pl;
	int ret;

	ASSERT(q);
	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	ret = (((q)->q_next) && (((q)->q_flag&QREADR) == ((q)->q_next->q_flag&QREADR)));
	UNLOCK(q->q_str->sd_mutex, pl);
	return(ret);
}

/*
 * queue_t *
 * RD(queue_t *q)
 *	Given one of a pair of queues, find the read queue.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

queue_t *
RD(queue_t *q)
{
	return(((q)->q_flag&QREADR) ? (q) : (q)-1);
}

/*
 * queue_t *
 * WR(queue_t *q)
 *	Given one of a pair of queues, find the write queue.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

queue_t *
WR(queue_t *q)
{
	return(((q)->q_flag&QREADR) ? (q)+1 : (q));
}

/*
 * queue_t *
 * OTHERQ(queue_t *q)
 *	Given one of a pair of queues, find the other queue.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

queue_t *
OTHERQ(queue_t *q)
{
	return(((q)->q_flag&QREADR) ? (q)+1 : (q)-1);
}

/*
 * int
 * datamsg(uchar_t type)
 *	Test if data block type is one of the data messages (i.e. not a
 *	control message).
 *
 * Calling/Exit State:
 *	No locking status.
 */

int
datamsg(uchar_t type)
{
	return(((type) == M_DATA) || ((type) == M_PROTO) ||
	    ((type) == M_PCPROTO) || ((type) == M_DELAY));
}

/*
 * int
 * pcmsg(uchar_t type)
 *	Test if message type is priority.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

int
pcmsg(uchar_t type)
{
	return((type) >= QPCTL);
}


#ifdef STRLEAK
/*
 * void
 * print_leaklist(void)
 *	Dump out all of the in-use messages
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

void
print_leaklist(void)
{
	struct mbinfo *mbp;
	long l;
	extern int adrtoext();	/* steal routine from kdb */
	extern char *sl_name;	/* also from kdb */

	mbp = (struct mbinfo *) strleakhead;
	while (mbp) {
		if ((l = adrtoext(mbp->mb_func)) == 0)
			dbprintf("mblk: %x  addr: %s\n", mbp, sl_name);
		else if (l != -1)
			dbprintf("mblk: %x  addr: %s+%x\n", mbp, sl_name, l);
		mbp = (struct mbinfo *) mbp->mb_next;
	}
	return;
}

/*
 * void
 * print_leakcnt(void)
 *	Tabulate callers of in-use messages
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

void
print_leakcnt(void)
{
	int i;
	struct mbinfo *mbp;
	long l;
	extern int adrtoext();	/* steal routine from kdb */
	extern char *sl_name;	/* also from kdb */

	/* First, clear the array */
	for (i = 0; i < STRLEAKSZ; i++) {
		Strleak[i].sl_addr = 0;
		Strleak[i].sl_cnt = 0;
	}

	mbp = (struct mbinfo *) strleakhead;
	while (mbp) {
		for (i = 0; i < STRLEAKSZ; i++) {
			if (Strleak[i].sl_addr == mbp->mb_func) {
				Strleak[i].sl_cnt++;
				break;
			}
			if (Strleak[i].sl_addr == 0)
				break;
		}
		if (i == STRLEAKSZ) {
			/*
			 *+ debug buffer too small
			 */
			cmn_err(CE_CONT, "Strleak too small\n");
			mbp = (struct mbinfo *) mbp->mb_next;
			continue;
		}
		if (Strleak[i].sl_addr) {
			mbp = (struct mbinfo *) mbp->mb_next;
			continue;
		}
		Strleak[i].sl_addr = mbp->mb_func;
		Strleak[i].sl_cnt = 1;
		mbp = (struct mbinfo *) mbp->mb_next;
	}
	for (i = 0; i < STRLEAKSZ; i++) {
		if (Strleak[i].sl_addr) {
			if ((l = adrtoext(Strleak[i].sl_addr)) == 0)
				dbprintf("addr: %s   count: %d\n", sl_name,
					Strleak[i].sl_cnt);
			else if (l != -1)
				dbprintf("addr: %s+%x   count: %d\n", sl_name, l,
					Strleak[i].sl_cnt);
		} else {
			break;
		}
	}
	return;
}

#endif
