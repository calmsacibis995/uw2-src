/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/kbd/kbd.c	1.1"
#ident	"$Header: $"

/*
 *	Keyboard Mapping & String Translation module.
 */

#include <fs/dirent.h>
#include <io/ldterm/eucioctl.h>	/* EUC ioctl calls */
#include <io/conf.h>
#include <io/stream.h>
#include <mem/kmem.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>	/* for error numbers */
#include <util/cmn_err.h>
#include <util/param.h>	/* I wanted "user.h" and had to drag in: */
#include <util/types.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <io/ddi.h>

#include <io/kbd/kbd.h>	/* local kbd-subsystem defs */
#include <io/kbd/kbduser.h>	/* local kbd module defs */
#include <io/alp/alp.h>	/* ALP defs */

/*
 * The asymmetry in module_info reflects "major usage", which is in
 * interactive sessions.
 */
static struct module_info kbdrinfo = { 0, "kbd", 0, INFPSZ, 256, 16 };
static struct module_info kbdwinfo = { 0, "kbd", 0, INFPSZ, 1024, 128 };

STATIC int  kbdopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int  kbdrput(queue_t *, mblk_t *);
STATIC int  kbdrsrv(queue_t *);
STATIC int  kbdwput(queue_t *, mblk_t *);
STATIC int  kbdwsrv(queue_t *);
STATIC int  kbdclose(queue_t *, int, cred_t *);
STATIC int  kbdunload(struct kbduu *, mblk_t *, struct iocblk *);
STATIC int  kbd2big(int, int, int);
STATIC int  zzcompare(unsigned char *, unsigned char *);
STATIC int  kbdresult(int, unsigned char *, mblk_t **);
STATIC int  kbd_walk(queue_t *, mblk_t *, struct kbdusr *);
STATIC int  kbd_checktab(struct kbd_tab *);
STATIC int  kbd_ttype(struct kbd_tab *, struct kbd_tab *);
STATIC int  kbdlist(queue_t *, mblk_t *, int);
STATIC int  kbdfill(struct kbdusr *);
STATIC int  kbdproc(queue_t *, mblk_t *, struct kbdusr *, struct tablink *, mblk_t **, int);
STATIC void kbdioc(queue_t *, mblk_t *, struct iocblk *, struct kbdusr *);
STATIC void tabfill(struct kbd_tab *);
STATIC void kbdunlink(struct tablink *);
STATIC void freetable(struct kbd_tab *);
STATIC void kbdload(queue_t *, mblk_t *, struct iocblk *);
STATIC void kbd_publink(struct kbd_tab *);
STATIC void kbd_prvlink(struct kbd_tab *, struct kbduu *);
STATIC void kbdverb(queue_t *, struct kbdusr *);
STATIC void hotsw(struct kbdusr *);
STATIC void kbdlaux(struct kbd_tab *, struct kbduu *, struct kbd_query *);
STATIC void kbd_senderr(struct kbd_tab *, unsigned char, mblk_t **);
STATIC void kbd_deref(struct kbd_tab *, int);
STATIC void kbd_timeout(struct kbdusr *);
STATIC void kbd_runtime(struct kbdusr *);
STATIC void kbd_setime(struct kbdusr *, struct tablink *);
STATIC void kbdsend(mblk_t *, unsigned char *, mblk_t **);
STATIC void kbdone(mblk_t *, unsigned char *);
STATIC struct kbd_tab *kbdfind(unsigned char *, struct kbduu *);
STATIC struct kbd_tab *kbdlinktab(mblk_t *, struct iocblk *, struct kbduu *);
STATIC struct kbd_tab *newtable(struct kbd_load *);
STATIC struct kbd_tab *kbdnewcomp(struct kbd_tab *, struct kbdusr *);
STATIC struct kbd_tab *kbd_newalp(unsigned char *, struct kbduu *);
STATIC struct tablink *kbd_attached(unsigned char *, struct kbdusr *);
STATIC struct tablink *grabtl(struct kbd_tab *);
STATIC struct kbdusr *new_side(struct kbdusr **);
STATIC mblk_t *buf_to_msg(unsigned char *, unsigned char *);

static struct qinit kbdrinit = {
	kbdrput, kbdrsrv, kbdopen, kbdclose, NULL, &kbdrinfo, NULL
};

static struct qinit kbdwinit = {
	kbdwput, kbdwsrv, NULL, NULL, NULL, &kbdwinfo, NULL
};

struct streamtab kbdinfo = { &kbdrinit, &kbdwinit, NULL, NULL };

/*
 * Definitions in the master file:
 */
extern int kbd_maxu;	/* max tabs a user can attach at once */
extern int kbd_umem;	/* max bytes of private tables, per user */
extern int kbd_timer;	/* default "timed mode" timer, in ticks */

STATIC struct tablink *tlinkl = (struct tablink *) 0;	/* free tablinks */
STATIC struct kbd_tab *tables;	/* linked list of tables */

static int firsttime = ~0;

/* slot states: */
#define FREE	0
#define USED	0xff

/* machine state flags (l_state) */
#define IDLE	1	/* idling */
#define SRCH	2	/* currently in search mode */

/* user state flags (u_state) */
#define ALRM	1	/* alarm went off */
#define HASHOT	4	/* has a hot key defined */

int kbddevflag = 0;

/*
 * A Note on tables & tablinks.  Tablinks are used to keep a linked list
 * of the tables attached to a particular user.  The public tables are kept
 * linked together by the t_next pointers in the kbd_tab structure.  If a
 * user has some private tables loaded, those are also linked into a list
 * by the t_next pointers but are not linked into the public list.  The
 * user's attached tables are linked by the "l_next" fields of the tablink
 * structures. The "l_me" pointers point to the tables themselves. Also,
 * tablinks hold state information relating to the particular user and the
 * state of the table when it's "in use".
 * 
 * For composite tables, an ARRAY of tablinks is stored in the table COPY
 * that the user attaches to.  When a user attaches a composite table, a
 * new copy is made and put on the user's "attached" list; the ref counts
 * of all components are incremented.  When the user detaches, the copy is
 * destroyed and the ref counts of components decremented.  A side-effect
 * of always using copies for composites is that the reference counts of
 * composite tables are always 0; composites may be "unloaded" without
 * affecting users who are using them, because users are using the copies. 
 * This leads to a space-saving trick: if you want to use a composite but
 * save some core space, you can load the components, make the composite link,
 * attach the composite, then unload the composite.  The "working copy" is
 * left, and it doesn't show on the list of available tables; the
 * space saved is the size of the table header plus some pointers.  When
 * you detach the working copy, the composite disappears.
 * 
 */

/*
 * int
 * kbdopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
 *	Open routine
 *
 * Calling/Exit State:
 *	No locking
 */

/* ARGSUSED */
STATIC int
kbdopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{
	int sx;
	struct kbduu *uu;

	if (sflag != MODOPEN) {	/* can't open as a driver or clone */
		return(EIO);
	}
	if (firsttime) {
		firsttime = 0;
		if (kbd_timer < MINTV)	/* force kbd_timer in bounds */
			kbd_timer = MINTV;
		if (kbd_timer > MAXTV)
			kbd_timer = MAXTV;
		/*
		 * Staying at STR priority grab a bunch of free tablinks.
		 * Later we'll allocate out of this pool and grab more
		 * when it's exhausted.
		 */
		(void) grabtl(NULL);
	}
	/*
	 * Check first for multiple opens
	 */
	sx = splstr();
	if (! q->q_ptr) {	/* is a first-time open */
		q->q_ptr = (caddr_t) kmem_alloc(sizeof(struct kbduu), KM_NOSLEEP);
		if (! q->q_ptr) {
			splx(sx);
			return(ENOMEM);
		}
	}
	else {	/* is an n-th time open, OK */
		splx(sx);
		return(0);
	}
	/*
	 * One "kbduu" structure allocated per Stream.
	 */
	uu = (struct kbduu *) q->q_ptr;
	new_side(&uu->oside);
	new_side(&uu->iside);
	if (! uu->iside || ! uu->oside) {
		if (uu->oside)	/* make sure all come free if we fail */
			kmem_free(uu->oside, sizeof(struct kbdusr));
		if (uu->iside)
			kmem_free(uu->iside, sizeof(struct kbdusr));
		kmem_free(uu, sizeof(struct kbduu));
		return(ENOMEM);
	}
	uu->u_use = USED;
	uu->ltmp = uu->u_link = (struct kbd_tab *) 0;
	uu->lpos = (unsigned char *) 0;
	uu->zuid = crp->cr_uid;
	uu->zumem = uu->lsize = 0;
	uu->lalsz = 0;
	WR(q)->q_ptr = (caddr_t) uu;
	uu->iside->u_q = q;
	uu->oside->u_q = WR(q);
	splx(sx);
	return(0);
}

/*
 * int
 * kbdclose(queue_t *q, int flag, cred_t *crp)
 *	Close routine.  Unlink all our lists, free local memory and messages.
 *
 * Calling/Exit State:
 *	No locking
 */

/* ARGSUSED */
STATIC int
kbdclose(queue_t *q, int flag, cred_t *crp)
{
	struct kbduu *uu;
	struct kbd_tab *t;
	int sx;

	uu = (struct kbduu *) q->q_ptr;

	/*
	 * Free attached tablinks & copies of composites.
	 */
	if (uu->iside) {
		if (uu->iside->u_str[0])
			kbdverb(q, uu->iside);
		kbdunlink(uu->iside->u_list);
		kmem_free(uu->iside, sizeof(struct kbdusr));
	}
	if (uu->oside) {
		if (uu->oside->u_str[0])
			kbdverb(q, uu->oside);
		kbdunlink(uu->oside->u_list);
		kmem_free(uu->oside, sizeof(struct kbdusr));
	}
	while (uu->u_link) {	/* Free private tables */
		t = uu->u_link;
		uu->u_link = t->t_next;
		t->t_next = NULL;
		if (t->t_flag == KBD_COT)
			kbd_deref(t, R_AV);
		freetable(t);
	}
	uu->u_link = NULL;

	sx = splstr();
	kmem_free(uu, sizeof(struct kbduu));
	q->q_ptr = 0;
	OTHERQ(q)->q_ptr = 0;
	splx(sx);
	return(0);
}

/*
 * void
 * kbdunlink(struct tablink *t)
 *	Unlink a user's "u_list" and decrement reference counts on all the
 *	attached maps.  We walk down the list, unlinking each tablink
 *	structure, and returning them to the head of the free list.  Before
 *	we do each one, we decrement ref counts on the tables.  Since this
 *	is the "attached" list, we free the copies of composite tables.
 *	We do NOT keep track of uu->zumem, as this only called when closing.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbdunlink(struct tablink *t)
{
	struct tablink *nxt;
	struct kbd_tab *tmp;
	int sx;

	nxt = t;
	sx = splstr();
	while (t) {
		--(t->l_me->t_ref);
		nxt = t->l_next;	/* save next */
		/*
		 * More delicate messing with the free list of tablinks
		 */
		t->l_next = tlinkl;	/* link to head of free list */
		tlinkl = t;
		if (tlinkl->l_me->t_flag == KBD_COT) {
			/*
			 * Always free attached composites: they're copies.
			 */
			tmp = tlinkl->l_me;
			tlinkl->l_me = NULL;
			splx(sx);
			kbd_deref(tmp, R_TC);
			freetable(tmp); /* allow interrupts, etc. */
			sx = splstr();
		}
		tlinkl->l_me = NULL;
		t = nxt;
	}
	splx(sx);
}

/*
 * int
 * kbd_walk(queue_t *q, mblk_t *mp, struct kbdusr *d)
 *	This is the function that calls "kbdproc" to dispose of data
 *	messages.  It has to take into account the fact that kbdproc
 *	may have switched tables in mid-stream, and so re-call kbdproc
 *	with the new table if that's what's necessary.  It also takes
 *	care of "putnext" on the message and all that.  The only reason
 *	we really have this function is to avoid duplicating this code
 *	everyplace we want to call kbdproc to process a message.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbd_walk(queue_t *q, mblk_t *mp, struct kbdusr *d)
{
	struct tablink *cur;
	mblk_t *out;
	int i; /* i= relative position */

	/*
	 * If kbdproc comes back with 0, it means that a table switch
	 * occurred in mid-stream.  It switched the table, and left
	 * "mp" as the un-processed part.  Anything that it has already
	 * processed is in "out".  In that case, we have to send "out"
	 * on its way, and re-process "mp" with the new table.  If
	 * kbdproc returns a 0, then it MUST NOT FREE "mp"!
	 */
again:
	i = 0;
	/*
	 * This case for composite tables.
	 */
	if (d->u_current && (d->u_current->t_flag == KBD_COT)) {
		cur = d->u_cp->l_me->t_child;
		out = (mblk_t *) 0;
		while (cur && mp) {
			if (!kbdproc(q, mp, d, cur, &out, i)) {
				cur = (struct tablink *) 0;
				/*
				 * If there's a processed part, send it,
				 * then do "mp".
				 */
				if (out)
					putnext(q, out);
				out = (mblk_t *) 0;
				if ((mp->b_rptr < mp->b_wptr) || mp->b_cont)
					goto again;
				else {
					freemsg(mp);
					return(1);
				}
			}
			cur = cur->l_next;
			mp = out;
			out = (mblk_t *) 0;
			++i;
		}
		if (mp)
			putnext(q, mp);
	}
	/*
	 * This case for simple tables.
	 */
	else {
		out = (mblk_t *) 0;
		if (!kbdproc(q, mp, d, d->u_cp, &out, 0)) {
			if (out)
				putnext(q, out);
			out = (mblk_t *) 0;
			if ((mp->b_rptr < mp->b_wptr) || mp->b_cont)
				goto again;
			else {
				freemsg(mp);
				return(1);
			}
		}
		if (out)
			putnext(q, out);
	}
	return(1);
}

/*
 * void
 * kbd_runtime(struct kbdusr *d)
 *	This is the function called when an alarm has gone off for
 *	a timing-dependent table.  We check the current table (or run
 *	through the composite components) until we find any tables that
 *	have a non-zero l_lbolt value <= lbolt.  When we find one, we
 *	take whatever data it has pending, reset its state to IDLE, and
 *	either putnext or call the rest of the tables given that data.
 *	The processing is similar to "kbd_walk", but can start up a
 *	composite "in the middle", not just at the beginning.
 *	MUST NOT be called if d->u_current is null.  If the table gets
 *	switched on us, everything will have been flushed by the
 *	table-switch routine, so we just return.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbd_runtime(struct kbdusr *d)
{
	struct tablink *cur;
	mblk_t *mp;
	mblk_t *out;
	int i;
	clock_t bolt;

	i = 0;
	bolt = lbolt;	/* for local consistency */
	if (d->u_current->t_flag == KBD_COT) {
		cur = d->u_cp->l_me->t_child;
		out = (mblk_t *) 0;
		while (cur) {
			if (cur->l_lbolt && cur->l_lbolt <= bolt)
				break;	/* found one */
			else {
				cur = cur->l_next;
				++i;	/* count of position */
			}
		}
		if (cur) {	/* found one with timed-out buffer */
			cur->l_lbolt = 0;
			if (cur->l_ptr > (unsigned char *) &(cur->l_msg[0])) {
				cur->l_node = 0;	/* reset idle state */
				cur->l_state |= IDLE;
				cur->l_state &= ~SRCH;
				mp = buf_to_msg(cur->l_msg, cur->l_ptr);
				cur->l_ptr =(unsigned char *) &(cur->l_msg[0]);
				if (! mp)
					return;
				kbd_senderr(cur->l_me, *mp->b_rptr++, &out);
				if (out) {
					if (mp->b_wptr > mp->b_rptr)
						linkb(out, mp);
					mp = out;
					out = (mblk_t *) 0;
				}
				cur = cur->l_next;
				++i;
				while (cur && mp) {
					if (!kbdproc(d->u_q, mp, d, cur, &out, i)) {
						if (out)
							putnext(d->u_q, out);
						freemsg(mp);
						return;
					}
					cur = cur->l_next;
					mp = out;
					out = (mblk_t *) 0;
					++i;
				}
				if (mp)
					putnext(d->u_q, mp);
			}
		}
		return;
	}
	else {
		/*
		 * It's a simple table: don't run the FIRST BYTE
		 * back through kbdproc!  We have to figure out if
		 * we have an error, and if so send it instead of
		 * the first byte.  If there's anything left, then
		 * send it along.
		 */
		cur = d->u_cp;
		if (bolt < cur->l_lbolt) {
			/*
			 *+ informational message
			 */
			cmn_err(CE_NOTE, "UX:KBD: Error in alarm timing\n");
			if (cur->l_tid) {
				untimeout(cur->l_tid);
				cur->l_tid = 0;
				cur->l_lbolt = 0;
			}
			return;
		}
		/*
		 * If anything in the message...
		 */
		if (cur->l_ptr > (unsigned char *) &(cur->l_msg[0])) {
			out = (mblk_t *) 0;
			cur->l_node = 0;	/* reset idle state */
			cur->l_state |= IDLE;
			cur->l_state &= ~SRCH;
			mp = buf_to_msg(cur->l_msg, cur->l_ptr);
			cur->l_ptr =(unsigned char *) &(cur->l_msg[0]);
			if (! mp)
				return;
			kbd_senderr(cur->l_me, *mp->b_rptr++, &out);
			/*
			 * now, "out" is the error string or first byte and
			 * "mp" is the rest.
			 */
			if (out) {
				putnext(d->u_q, out);
				out = (mblk_t *) 0;
			}
			if (mp->b_wptr > mp->b_rptr) {
				if (!kbdproc(d->u_q, mp, d, cur, &out, i)) {
					if (out)
						putnext(d->u_q, out);
					freemsg(mp);
				}
			}
			else
				freemsg(mp);
		}
	}
}

/*
 * int
 * kbdwput(queue_t *q, mblk_t *mp)
 * 	Put procedure for WRITE side of module.  Sends IOCTL
 *	calls to "kbdioc".  Data goes to kbdproc, same as the
 *	read side.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbdwput(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocp;
	struct kbdusr *d;
	struct kbduu *uu;

	uu = (struct kbduu *)q->q_ptr;
	d = (struct kbdusr *) uu->oside;

	switch (mp->b_datap->db_type) {
	case M_IOCTL:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;
		if (((iocp->ioc_cmd & 0xFFFFFF00) == KBD) ||
		    ((iocp->ioc_cmd & EUC_IOC) == EUC_IOC)) {
			kbdioc(q, mp, iocp, d);
		}
		else
			putnext(q, mp);
		break;
	case M_DATA:
		if (! d->u_doing) {
			putnext(q, mp);
			return(0);
		}
		/*
		 * Check for data ordering...
		 */
		else if (q->q_first || d->u_running) {
			putq(q, mp);
			qenable(q);
		}
		else
			kbd_walk(q, mp, d);
		break;
	case M_FLUSH:
		flushq(q, FLUSHDATA);
		putnext(q, mp);
		break;
	default:
		if (mp->b_datap->db_type & QPCTL)
			putnext(q, mp);
		else if (q->q_first || d->u_running) {
			putq(q, mp);	/* attempt to keep it in order */
			qenable(q);
		}
		else
			putnext(q, mp);
	}
	return(0);
}

/*
 * int
 * kbdwsrv(queue_t *q)
 *	Write side service procedure.  Only entered for queued messages
 *	when "u_doing" or if the alarm went off.  Set "u_running" on entry
 *	& unset on exit.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbdwsrv(queue_t *q)
{
	struct kbdusr *d;
	mblk_t *mp;

	d = (struct kbdusr *) ((struct kbduu *)q->q_ptr)->oside;
	d->u_running = 1;
	if (d->u_state & ALRM) {	/* alarm went off */
		d->u_state &= ~ALRM;	/* turn off flag */
		if (d->u_current)
			kbd_runtime(d);
	}
	while (mp = getq(q)) {
		if (mp->b_datap->db_type == M_DATA) {
			if (! canput(q->q_next)) {
				putbq(q, mp);
				d->u_running = 0;
				return(0);
			}
			else
				kbd_walk(q, mp, d);
		}
		else {
			if (canput(q->q_next))
				putnext(q, mp);
			else {
				putbq(q, mp);
				d->u_running = 0;
				return(0);
			}
		}
	}
	d->u_running = 0;
	return(0);
}

/*
 * int
 * kbdrput(queue_t *q, mblk_t *mp)
 *	Read side put procedure.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbdrput(queue_t *q, mblk_t *mp)
{
	struct kbdusr *d;

	d = (struct kbdusr *) ((struct kbduu *)q->q_ptr)->iside;
	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		flushq(q, FLUSHDATA);
		putnext(q, mp);
		break;
	case M_DATA:
		if (d->u_doing) {	/* if we're "on" */
			if (q->q_first || d->u_running) {
				putq(q, mp);
			}
			else {
				if (! canput(q->q_next))
					putq(q, mp);
				else
					kbd_walk(q, mp, d);
			}
			return(0);
		}
		/* FALLTHRU */
	case M_IOCACK:
	case M_IOCNAK:
		putnext(q, mp);
		break;
	default:
		if (mp->b_datap->db_type & QPCTL)
			putnext(q, mp);
		else if (q->q_first || d->u_running)
			putq(q, mp);
		else
			putnext(q, mp);
	}
	return(0);
}

/*
 * int
 * kbdrsrv(queue_t *q)
 *	Read side service procedure.  Only called if necessary.  See comments
 *	above on data ordering.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbdrsrv(queue_t *q)
{
	struct kbdusr *d;
	mblk_t *mp;

	d = (struct kbdusr *) ((struct kbduu *)q->q_ptr)->iside;
	d->u_running = 1;
	if (d->u_state & ALRM) {	/* alarm went off */
		d->u_state &= ~ALRM;	/* turn off flag */
		if (d->u_current)
			kbd_runtime(d);
	}
	while (mp = getq(q)) {
		if (mp->b_datap->db_type == M_DATA) {
			if (! canput(q->q_next)) {
				putbq(q, mp);
				d->u_running = 0;
				return(0);
			}
			else
				kbd_walk(q, mp, d);
		}
		else
			putnext(q, mp);
	}
	d->u_running = 0;
	return(0);
}

/*
 * void
 * kbdioc(queue_t *q, mblk_t *mp, struct iocblk *iocp, struct kbdusr *d)
 *	Completely handle one of our ioctl calls, including the
 *	qreply of the message.
 *
 * Calling/Exit State:
 *	No locking
 *
 * Remarks:
 *	A security note:  We limit most ioctl access to the uid that pushed
 *	the module.  There's a covert channel in the query function: if you
 *	query and get reference counts, the counts on public tables (as well
 *	as their names) can be used to send information to other users.  It
 *	can be easily taken care of by paring down the information that is
 *	transmitted back from this call, or restricting it to private tables.
 *	The names of public tables could constitute a pretty high-bandwidth
 *	channel.
 */

STATIC void
kbdioc(queue_t *q, mblk_t *mp, struct iocblk *iocp, struct kbdusr *d)
{
	int type, sx;
	struct kbduu *uu;
	struct kbd_tab *t;

	uu = (struct kbduu *) q->q_ptr;
	/*
	 * This is a preliminary switch to catch all of the
	 * permission stuff first.
	 */
	switch (iocp->ioc_cmd) {
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_MSAVE:
	case EUC_MREST:
	case EUC_OXLOFF:
	case EUC_OXLON:
	case EUC_WSET:
	case EUC_WGET:
		break;
	case KBD_LOAD:
	case KBD_CONT:
	case KBD_END:
	case KBD_UNLOAD:
	case KBD_ATTACH:
	case KBD_DETACH:
	case KBD_LINK:
	case KBD_HOTKEY:
	case KBD_ON:
	case KBD_OFF:
	case KBD_QUERY:
	case KBD_VERB:
	case KBD_LIST:
	case KBD_TSET:
	case KBD_EXT:
		if ((iocp->ioc_uid == 0)||
		   (uu->zuid == iocp->ioc_uid)) {
			break;
		}
		else {
			iocp->ioc_error = EPERM;
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_rval = -1;
			qreply(q, mp);
			return;
		}
	default:
		break;
	}

	switch (iocp->ioc_cmd) {
	case EUC_IXLOFF:	/* Question: should we respond to EUC */
	case EUC_IXLON:		/* translation ioctls, or leave these */
	case EUC_OXLOFF:	/* first four calls alone?            */
	case EUC_OXLON:		/* Are we a Translator or Converter?  */
	case EUC_MSAVE:
	case EUC_MREST:
	case EUC_WSET:		/* If we see SET/GET switches here,   */
	case EUC_WGET:		/* we're REALLY in deep trouble, but  */
				/* we ignore it and it will go away.  */
		putnext(q, mp);
		return;
	case KBD_LOAD:
	case KBD_CONT:
	case KBD_END:
	case KBD_UNLOAD:
		kbdload(q, mp, iocp);
		return;
	/*
	 * Attach/detach errors:
	 *	Bad data format - EPROTO
	 *	Table not found (or not attached) - ENOENT
	 *	ATT: Table already attached - EMLINK
	 *	DET: Table is current - EMLINK (no longer an error)
	 *	No resources (tablinks) to attach table - ENOSR
	 *	Too many tables attached - EBUSY
	 */
	case KBD_ATTACH:
		if ((mp->b_cont->b_wptr - mp->b_cont->b_rptr) !=
		    sizeof(struct kbd_tach)) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		{
			struct kbd_tach *z;
			struct tablink *tl;
			int tcnt; /* count attached tabs */

			/* LINTED pointer alignment */
			z = (struct kbd_tach *)mp->b_cont->b_rptr;
			t = kbdfind(z->t_table, uu);
			if (! t) {
				iocp->ioc_error = ENOENT;
				goto nakit;
			}
			if (z->t_type & Z_UP)
				d = uu->iside;
			if (! d) {
				iocp->ioc_error = ENOSR;
				goto nakit;
			}
			tl = d->u_list;
			tcnt = 0;
			while (tl) { /* is table already attached? */
				++tcnt;
				if ((tl->l_me == t) ||
				    zzcompare(tl->l_me->t_name, t->t_name)) {
					iocp->ioc_error = EMLINK;
					goto nakit;
				}
				tl = tl->l_next;
			}
			/*
			 * If everything's OK, then see if we
			 * have enough room...
			 */
			if (tcnt >= kbd_maxu) {
				iocp->ioc_error = EBUSY;
				goto nakit;
			}
/*
 * If a composite table, make an expanded "copy" for the attached list.
 * Composite copies also get tacked onto a user's quota (zumem).
 */

			if (t->t_flag == KBD_COT)
				t = kbdnewcomp(t, d);
			if (! (tl = grabtl(t))) {
			/* free copies of COT tables on failure! */
				if (t->t_flag == KBD_COT) {
					kbd_deref(t, R_TC);
					freetable(t);
				}
				iocp->ioc_error = ENOSR;
				goto nakit;
			}
			if (t->t_flag == KBD_COT) {
				uu->zumem += t->t_asize;
			}
			else if (t->t_flag & KBD_ALP) {
				t->t_alpfunc = (unsigned char *) alp_con(t->t_name, (caddr_t *) &(tl->l_alpid));
			}
			tl->l_next = d->u_list;
			d->u_list = tl;
			++(t->t_ref);
			/*
			 * If no current and there's only ONE
			 * table attached, make it current.
			 */
			if (! d->u_current) {
				if (d->u_list && ! d->u_list->l_next)
					hotsw(d);
			}
			if ((t->t_flag & KBD_TIME) && (t->t_flag != KBD_COT))
				tl->l_time = d->u_time;
		}
		goto ackit;
	case KBD_DETACH:
		/*
		 * Find an attachment and return the
		 * tablink structure to the free list.  We
		 * don't, of course, unlink the table itself,
		 * just the user attachment.
		 */
		if ((mp->b_cont->b_wptr - mp->b_cont->b_rptr) != sizeof(struct kbd_tach)) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		{
			struct kbd_tach *z;
			struct tablink *tl, *ttmp;
			mblk_t *svp;

			/* LINTED pointer alignment */
			z = (struct kbd_tach *)mp->b_cont->b_rptr;
			if (z->t_type & Z_UP)
				d = uu->iside;
			tl = kbd_attached(z->t_table, d);
			if (! tl) {
				iocp->ioc_error = ENOENT;
				goto nakit;
			}
			/*
			 * disconnect & flush if it's an
			 * external function
			 */
			if ((t->t_flag != KBD_COT) &&
			    (t->t_flag & KBD_ALP)) {
				if (svp = alp_discon(t->t_name, (caddr_t ) tl->l_alpid))
					putnext(RD(q), svp);
				tl->l_alpid = (struct tablink *) 0;
			}
			if (tl->l_me == d->u_current) {
/*
 * Allow this one to be detached.  We just act as if we got the hot
 * key, if one's set and it's a toggle-off mode; otherwise, force the
 * thing to NOT be the current table.
 */
				if (d->u_hot && d->u_hkm)
					hotsw(d);
				else {
					d->u_current = (struct kbd_tab *)0;
					d->u_cp = (struct tablink *) 0;
					d->u_ocp = d->u_cp;
				}
				if (d == uu->iside && d->u_str[0])
					kbdverb(q, d);
			}
			ttmp = d->u_list;
			if (ttmp == tl) {
				sx = splstr();
				--(tl->l_me->t_ref);
				d->u_list = tl->l_next;
				if (tl->l_me->t_flag == KBD_COT) {
					uu->zumem -= tl->l_me->t_asize;
					kbd_deref(tl->l_me, R_TC);
					freetable(tl->l_me);
				}
				tl->l_me = NULL;
			/*
			 * Return a tablink to free list
			 */
				tl->l_next = tlinkl;
				tlinkl = tl;
				splx(sx);
			}
			/*
			 * find the predecessor, link it to the next one
			 */
			else {
				while (ttmp) {
				    if (ttmp->l_next == tl) {
					sx = splstr();
					--(tl->l_me->t_ref);
					ttmp->l_next = tl->l_next;
					if (tl->l_me->t_flag == KBD_COT) {
						uu->zumem -= tl->l_me->t_asize;
						kbd_deref(tl->l_me, R_TC);
						freetable(tl->l_me);
					}
					tl->l_me = NULL;
					tl->l_next = tlinkl;
					tlinkl = tl;
					splx(sx);
					break;
				    }
				    ttmp = ttmp->l_next;
				}
			}
		}
		goto ackit;
	case KBD_TGET:
		{
		struct kbd_ctl *cx;

		if ((! mp->b_cont) || (iocp->ioc_count != sizeof(struct kbd_ctl))) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		/* LINTED pointer alignment */
		cx = (struct kbd_ctl *) mp->b_cont->b_rptr;
		cx->c_type = uu->iside->u_time;
		cx->c_arg = uu->oside->u_time;
		}
		goto ackit;
	case KBD_TSET:	/* Set new timer value.  */
		{
			struct kbd_ctl *cx;

			if ((! mp->b_cont) || (iocp->ioc_count != sizeof(struct kbd_ctl))) {
				iocp->ioc_error = EDOM;
				goto nakit;
			}
			/* LINTED pointer alignment */
			cx = (struct kbd_ctl *)mp->b_cont->b_rptr;
			if (cx->c_type & Z_UP)
				d = uu->iside;
			d->u_time = (unsigned short) cx->c_arg;
			if (d->u_time < MINTV)
				d->u_time = MINTV;
			if (d->u_time > MAXTV)
				d->u_time = MAXTV;
		}
		goto ackit;
	case KBD_HOTKEY:
		/* Adjust the hotkeys.  */
		{
		struct kbd_ctl *cx;
		mblk_t *mp1, *mp2;

		if (iocp->ioc_count != sizeof(struct kbd_ctl)) {
			iocp->ioc_error = EPROTO;
nakit:
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_rval = -1;
			qreply(q, mp);
			return;
		}
		/* LINTED pointer alignment */
		cx = (struct kbd_ctl *)mp->b_cont->b_rptr;
		if (cx->c_type & Z_UP)
			d = uu->iside;
		if (cx->c_type & Z_SET) {
			d->u_hot = (unsigned char)(cx->c_arg & 0xFF);
			d->u_hkm = (unsigned char)((cx->c_arg >> 8) & 0xFF);
			if (d->u_hot)
				d->u_state |= HASHOT;
			else
				d->u_state &= ~HASHOT;
			/*
			 * Spread the good news to outlying
			 * organizations: when the hot-key
			 * changes, we send M_CTL both up and
			 * downstream to inform neighbors.
			 * currently, this is unused.
			 */
			if (mp1 = copymsg(mp)) {
				mp1->b_datap->db_type = M_CTL;
				/* Prophetable cloning operation */
				if (mp2 = copymsg(mp1))
					putnext(OTHERQ(q), mp2);
				putnext(q, mp1);
			}
		}
		else {
			cx->c_arg = d->u_hot;
			cx->c_arg |= (d->u_hkm << 8);
			iocp->ioc_count = sizeof(struct kbd_ctl);
		}
ackit:
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_rval = iocp->ioc_error = 0;
		qreply(q, mp);
		}
		break;

	case KBD_ON:
		if (iocp->ioc_count != sizeof(struct kbd_ctl)) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		{
			struct kbd_ctl *cx;

			/* LINTED pointer alignment */
			cx = (struct kbd_ctl *) mp->b_cont->b_rptr;
			if (cx->c_type & Z_UP)
				d = uu->iside;
			d->u_doing = 1;
		}
		goto ackit;
	case KBD_OFF:
		if (iocp->ioc_count != sizeof(struct kbd_ctl)) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		{
			struct kbd_ctl *cx;

			/* LINTED pointer alignment */
			cx = (struct kbd_ctl *) mp->b_cont->b_rptr;
			if (cx->c_type & Z_UP)
				d = uu->iside;
			d->u_doing = 0;
		}
		goto ackit;
	case KBD_VERB:
		if ((! mp->b_cont) || (iocp->ioc_count > KBDVL)) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		d = uu->iside;
		{
			unsigned char *s, *t;

			/* copy in the verbose message */
			s = mp->b_cont->b_rptr;
			*(s + (iocp->ioc_count - 1)) = '\0';
			t = d->u_str;
			while (*s)
				*t++ = *s++;
			*t = '\0';
			/* then do it immediately */
			if (d->u_str[0])
				kbdverb(q, d);
		}
		goto ackit;
	case KBD_LIST:
		type = KBD_LIST;
		if (! mp->b_cont) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		if (iocp->ioc_error = kbdlist(q, mp->b_cont, type))
			goto nakit;

		if (type == KBD_QUERY) /* from KBD_QUERY above */
			iocp->ioc_count = 0;
		else
			iocp->ioc_count = sizeof(struct kbd_query);
		goto ackit;
	case KBD_LINK:
		if (! kbdlinktab(mp, iocp, uu))
			goto nakit;	/* kbdlinktab sets ioc_error */
		goto ackit;
	case KBD_EXT:
		/* This makes a reference to an ALP function */
		if ((! mp->b_cont) || (iocp->ioc_count < 1)) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		if (( ~(*mp->b_cont->b_rptr++) & 0x0F) != Z_PUBLIC) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		if (*mp->b_cont->b_rptr++ != 0x96) {
			iocp->ioc_error = EPROTO;
			goto nakit;
		}
		/*
		 * Create if none, else just get it.  If can't
		 * do either, then nak it.
		 */
		iocp->ioc_count -= 2;
		if (! kbd_newalp(mp->b_cont->b_rptr, uu)) {
			iocp->ioc_error = ENOENT;
			goto nakit;
		}
		goto ackit;
	default:
		putnext(q, mp);
	}
	return;
}

/*
 * struct tablink *
 * grabtl(struct kbd_tab *t)
 *	Grab a "new" tablink structure and attach it to table "t".
 *	If called without a "t" pointer, it can be used to prime the freelist.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC struct tablink *
grabtl(struct kbd_tab *t)
{
	struct tablink *tl;
	int sx, i;
	struct tablink *newl;

	sx = splstr();
	tl = tlinkl;
	if (! tl) {	/* prime the freelist */
		newl = (struct tablink *) kmem_zalloc(NTLINKS * sizeof(struct tablink), KM_NOSLEEP);
		if (! newl) {
			splx(sx);
			return((struct tablink *) 0);
		}
		for (i = 0; i < NTLINKS; i++) {
			newl[i].l_next = (struct tablink *) &(newl[i+1]);
		}
		newl[NTLINKS - 1].l_next = (struct tablink *) 0;
		tlinkl = newl;
		tl = tlinkl;
	}
	if (! t || ! tl) {
		splx(sx);
		return((struct tablink *) 0);
	}
	tlinkl = tlinkl->l_next;
	splx(sx);
	tl->l_me = t;
	tl->l_state = IDLE;
	tl->l_node = 0;
	tl->l_time = 0;
	tl->l_lbolt = 0;
	tl->l_tid = 0;
	tl->l_next = (struct tablink *) 0;
	tl->l_ptr = &(tl->l_msg[0]);
	return(tl);
}

/*
 * void
 * kbdsend(mblk_t *mp, unsigned char *rp, mblk_t **where)
 *	Send part of an existing message to "where".  Used to flush
 *	out the beginning parts of messages when we've discovered something
 *	we're interested in, and there were things before it.  We copy
 *	whatever is before, then reset the rptr on the message itself.
 *	In pre-pipelining days, it used to "send" it on the Stream.  Nowdays,
 *	the thing could probably be re-written to work in concert with
 *	"kbdresult" to try to keep messages big & unfragmented.  For most
 *	applications, the user is slower than the CPU so it's not a big deal.
 * 
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbdsend(mblk_t *mp, unsigned char *rp, mblk_t **where)
{
	unsigned char *c;
	mblk_t *up;

	if (rp >= mp->b_wptr)	/* nothing to send */
		return;
	if (! (up = allocb(rp - mp->b_rptr, BPRI_HI)))
		return;
	c = mp->b_rptr;
	while (c < rp)
		*up->b_wptr++ = *c++;
	if (*where)		/* link it at the "output place" */
		linkb(*where, up);
	else
		*where = up;
	mp->b_rptr = rp;
}

/*
 * int
 * kbdfill(struct kbdusr *d)
 *	Initialize a kbdusr structure with default values.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbdfill(struct kbdusr *d)
{
	int i;

	d->u_current = NULL;
	d->u_list = d->u_cp = d->u_ocp = (struct tablink *) 0;
	d->u_time = (unsigned short) kbd_timer;
	for (i = 0; i < KBDVL; i++)
		d->u_str[i] = 0;
	d->u_cp = (struct tablink *) 0;
	d->u_doing = '\0';	/* need ioctl to turn on */
	d->u_hot = '\0';
	d->u_hkm = 1;	/* default to list + non-mapped */
	d->u_running = 0;
	return(1);
}

/*
 * void
 * freetable(struct kbd_tab *t)
 *	Free a table structure.  Figure out how big it is.  We don't
 *	adjust the pools, the caller has to do that.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
freetable(struct kbd_tab *t)
{
	kmem_free(t, t->t_asize);
}

/*
 * void
 * kbd_deref(struct kbd_tab *t, int type)
 *	Decrement reference counts on all elements of a composite.
 *	This is done prior to freeing the table.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbd_deref(struct kbd_tab *t, int type)
{
	struct kbd_tab **tb;
	struct tablink *tl;
	int i;

	if (t->t_flag != KBD_COT)
		return;
	if (type == R_TC) {
		tl = t->t_child;
		while (tl) {
			--(tl->l_me->t_ref);
			tl = tl->l_next;
		}
	}
	else {
		tb = (struct kbd_tab **) t->t_child;
		for (i = 0; i < (int) t->t_nodes; i++) {
			--((*tb)->t_ref);
			++tb;
		}
	}
}

/*
 * struct kbd_tab *
 * newtable(struct kbd_load *ld)
 *	Allocate a new table structure to hold a table of the given
 *	size.  Fill some fields (esp. the name, ref count).
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC struct kbd_tab *
newtable(struct kbd_load *ld)
{
	int siz, i;
	struct kbd_tab *t;

	siz = ld->z_tabsize + ld->z_onesize + ld->z_nodesize + ld->z_textsize;
	if ((siz < sizeof(struct kbd_tab)) || (siz > KBDTMAX))
		return(NULL);
	/*
	 * The capability for "downloadable tables of arbitrary size"
	 * requires dynamic allocation.  This used to be done from a
	 * small static pool, now it's done via kmem_alloc.
	 */
	if (t = (struct kbd_tab *) kmem_alloc(siz, KM_NOSLEEP)) {
		t->t_nodes = t->t_text = t->t_flag = 0;
		t->t_ref = 0;
		t->t_next = NULL;
		/* LINTED pointer alignment */
		t->t_nodep = (struct cornode *) ((unchar *) t + sizeof(struct kbd_tab));
		t->t_oneone = (unchar *) 0;
		for (i = 0; i < KBDNL; i++)
			t->t_name[i] = ld->z_name[i];
		t->t_name[KBDNL-1] = '\0';
		t->t_asize = siz;
	}
	return((struct kbd_tab *) t);
}

/*
 * void
 * kbdload(queue_t *q, mblk_t *mp, struct iocblk *iocp)
 * 	Handle the LOAD, CONT, END ioctls.  LOAD and CONT have
 *	attached data blocks up to 256 bytes.  END has an
 *	attached structure that tells us whether to make the
 *	table private or public.  Z_PUBLIC is only usable by
 *	root to make the table public.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbdload(queue_t *q, mblk_t *mp, struct iocblk *iocp)
{
	struct kbd_tab *t;
	struct kbduu *uu;
	struct kbd_ctl *c;
	struct kbd_load *ld;

	iocp->ioc_rval = 0;
	uu = (struct kbduu *) q->q_ptr;
	/*
	 * Common loader errors here.  General permission errors are
	 * caught by the caller.
	 *
	 * Lack of data, bad format, bad LOAD/CONT order (EPROTO)
	 * Out of public or private space (ENOSPC)
	 * Alloc failed (ENOMEM)
	 * END or CONT without prior LOAD (EFAULT)
	 * CONT: loading something bigger than originally declared (E2BIG)
	 * END: smaller than declared originally (EINVAL)
	 * Table already exists by that name (EEXIST)
	 * Too big (EFBIG)
	 *
	 * Secret Incantation Failure (EXDEV)
	 */
	switch (iocp->ioc_cmd) {
	case KBD_UNLOAD:
		/* LINTED pointer alignment */
		if ((! mp->b_cont)||(*(ALIGNER *)mp->b_cont->b_rptr != 0x6361)) {
			iocp->ioc_error = EXDEV;
			goto badioc;
		}
		else {
			mp->b_cont->b_rptr += ALIGNMENT;
			iocp->ioc_count -= ALIGNMENT;
		}
		if (iocp->ioc_count != sizeof(struct kbd_tach)) {
			iocp->ioc_error = EPROTO;
			goto badioc;
		}
		/*
		 * Try to unload.  If no good, nak it.  kbdunload did the
		 * error setting.
		 */
		if (! kbdunload(uu, mp, iocp))
			goto badioc;
		break;
	case KBD_LOAD:
		/* LINTED pointer alignment */
		if (! mp->b_cont || *(ALIGNER *)mp->b_cont->b_rptr != 0x3305) {
			iocp->ioc_error = EXDEV;
			goto badioc;
		}
		else {
			mp->b_cont->b_rptr += ALIGNMENT;
			iocp->ioc_count -= ALIGNMENT;
		}
		if (uu->lpos) {
			iocp->ioc_error = EPROTO;
			goto badioc;
		}
		if (iocp->ioc_count == sizeof(struct kbd_load)) {
			int siz;

			/* LINTED pointer alignment */
			ld = (struct kbd_load *) mp->b_cont->b_rptr;
			siz = ld->z_tabsize + ld->z_onesize +
				ld->z_nodesize + ld->z_textsize;
			if (kbd2big(uu->zumem, siz, (int) iocp->ioc_uid)) {
				iocp->ioc_error = EFBIG;
				goto badioc;
			}
			if (kbdfind(ld->z_name, uu)) {
				iocp->ioc_error = EEXIST;
				goto badioc;
			}
			if (! (t = newtable(ld))) {
				iocp->ioc_error = ENOMEM;
				goto badioc;
			}
			uu->ltmp = t;
			uu->lpos = (unchar *) t;
			uu->lsize = siz;
			uu->lalsz = t->t_asize;	/* save for later */
		}
		else {
			iocp->ioc_error = EPROTO;
			goto badioc;
		}
		break;
	case KBD_CONT:
		/* LINTED pointer alignment */
		if (!mp->b_cont || (*(ALIGNER *)mp->b_cont->b_rptr != 0x3680)) {
			iocp->ioc_error = EXDEV;
			goto badioc;
		}
		else {
			mp->b_cont->b_rptr += ALIGNMENT;
			iocp->ioc_count -= ALIGNMENT;
		}
		t = uu->ltmp;
		if (! t) {
			iocp->ioc_error = EFAULT;
			goto badioc;
		}
		/*
		 * Copy data from mp->b_cont->b_rptr to q->q_ptr->lpos,
		 * and increment lpos past the added data.  We check for
		 * legitimate size here as well.
		 */
		{
			unchar *s, *r, *lim;
			int siz, tmp;

			siz = uu->lsize;
			r = uu->lpos;
			s = mp->b_cont->b_rptr;
			lim = mp->b_cont->b_wptr;
			/*
			 * Check for overflow of the table.
			 */
			tmp = lim - s;
			if (tmp > siz) {
				iocp->ioc_error = E2BIG;
				goto badioc;
			}
			uu->lpos += tmp;
			uu->lsize -= tmp;
			while (s < lim)
				*r++ = *s++;
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
		break;
	case KBD_END:
		if (iocp->ioc_count == sizeof(struct kbd_ctl)) {
			/* LINTED pointer alignment */
			c = (struct kbd_ctl *) mp->b_cont->b_rptr;
			if (c->c_arg != 0x2212) {
				iocp->ioc_error = EXDEV;
				goto badioc;
			}
			t = ((struct kbduu *)q->q_ptr)->ltmp;
			if (! t) {
				iocp->ioc_error = EFAULT;
				goto badioc;
			}
			/*
			 * Sanity check
			 */
			if (uu->lsize) {
				iocp->ioc_error = EINVAL;
				goto badioc;
			}
			/* LINTED pointer alignment */
			c = (struct kbd_ctl *)mp->b_cont->b_rptr;
			tabfill(t);
/*
 * SECURITY:
 * Scrutinize the minutest details of this thing to make sure it's not
 * an attempt to subvert the system.  If it IS a covert attempt, stop
 * it.  In practice, this sequence of "putctl1" messages on kbd_checktab
 * failure results in both the calling program and the user's shell dying
 * (i.e., the user gets logged out).  We are intentionally nasty here, because
 * (1) the table is probably a trap for someone, (2) the compiler won't
 * produce a table that fails the kbd_checktab, which means it's been
 * "doctored".  If we actually USED a "bad" table it could crash the system.
 */

			if (kbd_checktab(t)) {
				putctl1(RD(q)->q_next, M_PCSIG, SIGQUIT);
				putctl1(RD(q)->q_next, M_PCSIG, SIGKILL);
				iocp->ioc_error = EPERM;
				goto badioc;
			}
			/*
			 * We have to restore the size of the allocation,
			 * because the table was completely over-written
			 * by the download.  There's some "extra" work
			 * being done by "newtable()".
			 */
			t->t_asize = uu->lalsz;
			if ((c->c_type == Z_PUBLIC) && (iocp->ioc_uid == 0))
				kbd_publink(t);
			else	/* force privatization, like it or not */
				kbd_prvlink(t, uu);
			uu->ltmp = NULL;
			uu->lpos = 0;
		}
		else {
			iocp->ioc_error = EPROTO;
			goto badioc;
		}
		break;
	default:
		/*
		 *+ Bad information was passed to kdbload
		 */
		cmn_err(CE_PANIC, "KBD: kdbload\n");
badioc:
		if (uu->ltmp) {
			kmem_free((caddr_t) uu->ltmp, (int) ((struct kbd_tab *)(uu->ltmp)->t_asize));
			uu->ltmp = NULL;
			uu->lpos = 0;
		}
		iocp->ioc_rval = -1;
		mp->b_datap->db_type = M_IOCNAK;
		qreply(q, mp);
		return;
	}
	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = 0;
	iocp->ioc_count = 0;
	qreply(q, mp);
}

/*
 * int
 * kbdunload(struct kbduu *uu, mblk_t *mp, struct iocblk *iocp)
 *	Unload table
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbdunload(struct kbduu *uu, mblk_t *mp, struct iocblk *iocp)
{
	struct kbd_tach *tc;
	struct kbd_tab *t, *tmp;
	
	/* LINTED pointer alignment */
	tc = (struct kbd_tach *)mp->b_cont->b_rptr;
	tc->t_table[KBDNL-1] = '\0';
	t = kbdfind(&(tc->t_table[0]), uu);
	if (! t) {
		iocp->ioc_error = ENOENT;
		return(0);
	}
	if (t->t_ref != 0) {
		iocp->ioc_error = EMLINK;
		return(0);
	}
	/*
	 * Check private list, then public list.  A note:  When a user
	 * has a composite table, it will be found in the "private"
	 * list, and then just unloaded.  Any "public" copy will not
	 * be unloaded.
	 */
	if (uu->u_link) {	/* any private list? */
		tmp = uu->u_link;
		if (tmp == t) {	/* first one */
			uu->u_link = t->t_next;
			if (tmp->t_flag == KBD_COT)
				kbd_deref(tmp, R_AV);
			uu->zumem -= tmp->t_asize;
			freetable(tmp);
			return(1);
		}
		else {
			while (tmp) {
				if (tmp->t_next == t) {
					tmp->t_next = t->t_next;
					if (tmp->t_flag == KBD_COT)
						kbd_deref(tmp, R_AV);
					uu->zumem -= tmp->t_asize;
					freetable(t);
					return(1);
				}
				tmp = tmp->t_next;
			}
		}
	}
	/*
	 * check public list
	 */
	tmp = tables;
	if (tmp == t) {	/* first one */
		if (iocp->ioc_uid != 0) {
			iocp->ioc_error = EPERM;
			return(0);
		}
		tables = t->t_next;
		if (tmp->t_flag == KBD_COT)
			kbd_deref(tmp, R_AV);
		freetable(tmp);
		return(1);
	}
	else {
		while (tmp) {
			if (iocp->ioc_uid != 0) {
				iocp->ioc_error = EPERM;
				return(0);
			}
			if (tmp->t_next == t) {
				tmp->t_next = t->t_next;
				if (tmp->t_flag == KBD_COT)
					kbd_deref(tmp, R_AV);
				freetable(t);
				return(1);
			}
			tmp = tmp->t_next;
		}
	}
	iocp->ioc_error = ESRCH;
	return(0);
}

/*
 * struct kbd_tab *
 * kbdlinktab(mblk_t *mp, struct iocblk *iocp, struct kbduu *uu)
 * 	Make "composite" tables.  Take in the data portion
 *	a 2-byte indicator for public/private link table.  (If public, then
 *	user must be root.)  Following this is a string that is the x:y,z
 *	string of names to manufacture.  The first component (before the
 *	colon) is the terminal name, the rest (comma-separated) are the
 *	components.
 *
 * Errors:
 *	EPROTO - no data or bad format (no indicator in first byte, or
 *		 fewer than 2 tables being linked).
 *	EPERM  - not root & request Z_PUBLIC.
 *	ENOMEM - can't alloc table or get other memory.
 *	EEXIST - table of final name already exists.
 *	EINVAL - one or more arguments don't exist.
 *	E2BIG  - too many names linked (limit is arbitrarily KBDLNX).
 *	EFAULT - attempt to link a composite table.
 *	EACCES - request was public and a member was found to be private.
 *		 (a public composite cannot contain private members.)
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC struct kbd_tab *
kbdlinktab(mblk_t *mp, struct iocblk *iocp, struct kbduu *uu)
{
	unsigned char *list[KBDLNX+2];
	struct kbd_tab *tabs[KBDLNX+2];
	int i, j;
	unsigned char *s;
	struct kbd_tab **tb;
	int type;	/* public or private */
	struct kbd_load ld;	/* for faking newtable() */

	if ((! mp->b_cont) || (iocp->ioc_count < 1)) {
		iocp->ioc_error = EPROTO;
		return(NULL);
	}
	s = mp->b_cont->b_rptr;
	*(s + iocp->ioc_count) = '\0';
	/*
	 * Permission checks.  If not specified it's a protocol error;
	 * if non-root tries PUBLIC it's a permission error, but is
	 * forced to private status rather than being rejected.
	 */
	type = ~(*s++) & 0x0F;
	if ((type != (unsigned char) Z_PUBLIC) &&
	    (type != (unsigned char) Z_PRIVATE)) {
		iocp->ioc_error = EPROTO;
		return(NULL);
	}
	if ((type == Z_PUBLIC) && (iocp->ioc_uid != 0)) {
		type = Z_PRIVATE;	/* force off */
	}
	if (*s++ != 0x69) {
		iocp->ioc_error = EXDEV;
		return(NULL);
	}
	list[0] = s;	/* first component */
	while (*s && *s != ':')
		++s;
	if (*s != ':') {
		iocp->ioc_error = EPROTO;	/* no ":" */
		return(NULL);
	}
	*s++ = '\0';	/* skip over colon */
	i = 1;
	while (*s) {
		list[i] = s;
		while (*s && (*s != ','))
			++s;
		if (*s == ',')
			*s++ = '\0';
		++i;
		if (i >= KBDLNX) {
			iocp->ioc_error = E2BIG;
			return(NULL);
		}
	}
	/*
	 * Now, "i" is the NUMBER of strings including terminal name.
	 * (i.e., the number of children + 1).
	 */
	if (i < 3) {	/* enough names (need final + at least 2 args) */
		iocp->ioc_error = EINVAL;
		return(NULL);
	}
	for (j = 0; j < i; j++) {	/* any null names? */
		if (! *list[j]) {
			iocp->ioc_error = EINVAL;
			return(NULL);
		}
	}
	if (tabs[0] = kbdfind(list[0], uu)) {	/* target exists? */
		iocp->ioc_error = EEXIST;
		return(NULL);
	} 
	/*
	 * Check for (1) table existence, (2) simple table, (3) table type.
	 * Must be (1) found, (2) a simple table, (3) if request is for
	 * public, all members must be public; if request if for private,
	 * then members can be private or public.
	 */
	for (j = 1; j < i; j++) {
		tabs[j] = kbdfind(list[j], uu);
		if (! tabs[j]) {
			/* no auto-loading of externals into composites */
			iocp->ioc_error = EINVAL; /* not found! */
			return(NULL);
		}
		if (tabs[j]->t_flag == KBD_COT) { /* a composite */
			iocp->ioc_error = EFAULT;
			return(NULL);
		}
		if ((kbd_ttype(tabs[j], uu->u_link) != Z_PUBLIC) &&
					(type == Z_PUBLIC)) {
			iocp->ioc_error = EACCES;
			return(NULL);
		}
	}
	tabs[i] = NULL;	/* null end of list */
	s = list[0];	/* make ld struct for newtable */
	j = 0;
	while (((ld.z_name[j++] = *s++) != 0) && (j < KBDNL))
		;
	ld.z_name[KBDNL-1] = '\0';
	ld.z_tabsize = sizeof(struct kbd_tab);
	ld.z_onesize = ld.z_nodesize = 0;
	/* alloc nchildren + 1 pointers: */
	ld.z_textsize = i * sizeof(struct kbd_tab *);
	/* note...newtable really sets alloced size in t_asize field. */
	if (kbd2big(uu->zumem, ld.z_tabsize + ld.z_textsize, iocp->ioc_uid)) {
		iocp->ioc_error = EFBIG;
		return(NULL);
	}
	if (! (tabs[0] = newtable((struct kbd_load *) &ld))) {
		iocp->ioc_error = ENOMEM;
		return(NULL);
	}
	/*
	 * Now that we got a table, fill in the blanks.  In a composite
	 * when it's on the "available" list "t_textp", "t_nodep" and
	 * "t_child" all point at an ARRAY of kbd_tab pointers (stored just
	 * after the kbd_tab).  When the user attaches one of these,
	 * then a bunch of tablink structures will be alloced, and the
	 * pointers copied into the tablinks and state set up properly.
	 */
	tabs[0]->t_textp = (unsigned char *) tabs[0]->t_nodep;
	/* LINTED pointer alignment */
	tabs[0]->t_child = (struct tablink *) tabs[0]->t_textp;
	tabs[0]->t_flag = KBD_COT;
	tb = (struct kbd_tab **) tabs[0]->t_child;
	for (j = 1; j < i; j++) {
		*tb = tabs[j];
		++(tabs[j]->t_ref); /* increment ref count of each element */
		++tb;
	}
	*tb = NULL;	/* null terminate list */
	tabs[0]->t_nodes = i - 1; /* t_nodes is number of tablinks later. */
	if (type == Z_PUBLIC)	/* permissions checked above already */
		kbd_publink(tabs[0]);
	else
		kbd_prvlink(tabs[0], uu);
	return(tabs[0]); /* return number of tables involved as members */
}

/*
 * void
 * kbd_publink(struct kbd_tab *t)
 *	Link a table to the FRONT of the public table list.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbd_publink(struct kbd_tab *t)
{
	int sx;

	sx = splstr();
	if (tables)
		t->t_next = tables;
	tables = t;
	splx(sx);
}

/*
 * void
 * kbd_prvlink(struct kbd_tab *t, struct kbduu *uu)
 *	Link a table to the front of some user's private list.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbd_prvlink(struct kbd_tab *t, struct kbduu *uu)
{
	int sx, tmp;

	sx = splstr();
	t->t_next = uu->u_link;
	uu->u_link = t;
	tmp = t->t_asize;
	uu->zumem += tmp;
	splx(sx);
}

/*
 * struct kbd_tab *
 * kbdnewcomp(struct kbd_tab *t, struct kbdusr *d)
 *	Make a new composite table.  Take a pointer to a
 *	composite table on the "available" list and make
 *	a new table that's a copy of the kbd_tab struct,
 *	does NOT contain the "Text" part, and contains a
 *	bunch of tablinks for it.  Set state to IDLE.
 *	(Note: Attaching a composite will never cause the
 *	quota to be exceeded.)
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC struct kbd_tab *
kbdnewcomp(struct kbd_tab *t, struct kbdusr *d)
{
	struct kbd_load ld;
	unsigned char *s;
	int i;
	struct kbd_tab *new, **tb;
	struct tablink *tl;

	if (t->t_flag != KBD_COT)
		return(t);
	s = &(t->t_name[0]);
	i = 0;
	/*
	 * copy name & make load structure
	 */
	while (((ld.z_name[i++] = *s++) != 0) && (i < KBDNL))
		;
	ld.z_name[KBDNL-1] = '\0';
	ld.z_tabsize = sizeof(struct kbd_tab);
	ld.z_onesize = ld.z_nodesize = 0;
	ld.z_textsize = t->t_nodes * sizeof(struct tablink);
	if (! (new = newtable((struct kbd_load *) &ld))) {
		return(NULL);
	}
	new->t_flag = KBD_COT;
	new->t_textp = (unsigned char *) new->t_nodep;
	/* LINTED pointer alignment */
	tl = (struct tablink *) new->t_nodep;
	new->t_ref = 0;
	new->t_child = tl;
	tb = (struct kbd_tab **) t->t_child;	/* &(t->t_child[0]) */
	new->t_nodes = t->t_nodes;
	for (i = 0; i < (int) t->t_nodes; i++) {
		tl->l_me = *tb;
		++((*tb)->t_ref); /* increment ref count of each element */
		if ((*tb)->t_flag & KBD_TIME)
			tl->l_time = d->u_time; /* set timer if needed */
		tl->l_next = tl+1;
		tl->l_ptr = (unsigned char *) &(tl->l_msg[0]);
		tl->l_node = 0;
		tl->l_state = IDLE;
		++tl; ++tb;
	}
	--tl; /* because the loop ++ed it past last element */
	tl->l_next = (struct tablink *) 0;
	return(new);
}

/*
 * struct kbd_tab *
 * kbd_newalp(unsigned char *s, struct kbduu *uu)
 *	Allocate a new kbd_tab for the given external function (in "s").
 *	Make it public.  Can be used also to "auto-create" tables for
 *	all loaded external functions.  (See the note above about
 *	possibly auto-linking externals into composites.)  If the thing
 *	already exists, it is just returned.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC struct kbd_tab *
kbd_newalp(unsigned char *s, struct kbduu *uu)
{
	struct algo *a;
	struct kbd_tab *t;

	if (t = kbdfind(s, uu))	/* already exists */
		return(t);
	if (a = alp_query(s)) {	/* if there is one by this name... */
		if (! (t = kmem_zalloc(sizeof(struct kbd_tab), KM_NOSLEEP)))
			return(NULL);
		t->t_asize = sizeof(struct kbd_tab);
		t->t_flag = KBD_ALP;
		t->t_alp = (struct cornode *) a;
		strcpy((char *) t->t_name, (char *) s);
		t->t_alpfunc = (unsigned char *) a->al_func;
		t->t_nodes = t->t_text = t->t_error = 0;
		t->t_ref = 0;
		kbd_publink(t);
		return(t);
	}
	return(NULL);
}

/*
 * void
 * tabfill(struct kbd_tab *t)
 * 	Fills in the "in-core" fields of a kbd_tab structure.
 *	Only for "simple" tables.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
tabfill(struct kbd_tab *t)
{
	unchar *cp;

	cp = (unchar *) t;
	cp += sizeof(struct kbd_tab);
	if (t->t_flag & KBD_ONE) {
		t->t_oneone = cp;
		cp += 256;	/* size of oneone table */
	}
	else
		t->t_oneone = (unchar *) 0;
	/* LINTED pointer alignment */
	t->t_nodep = (struct cornode *) cp;
	cp += (t->t_nodes * sizeof(struct cornode));
	t->t_textp = cp;
	t->t_ref = 0;
}

/*
 * int
 * kbdlist(queue_t *q, mblk_t *mp, int type)
 *	Return a "kbd_query" structure, based on seq and type.
 *	If it can't go "seq" down in the "type"th list, it returns EAGAIN.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbdlist(queue_t *q, mblk_t *mp, int type)
{
	struct kbd_tab *t;
	struct kbduu *uu;
	struct kbd_query *uqs;	/* user's query struct */

	uu = (struct kbduu *)q->q_ptr;
	switch (type) {
	case KBD_QUERY:
		return(0);
	case KBD_LIST:
		/* LINTED pointer alignment */
		uqs = (struct kbd_query *) mp->b_rptr;
		if (uqs->q_flag & KBD_QF_PUB)
			t = tables;
		else
			t = uu->u_link;
		while (uqs->q_seq && t) {	/* go "seq" times */
			t = t->t_next;		/* or until no next */
			--uqs->q_seq;
		}
		if (! t)			/* can't go? end of list */
			return(EAGAIN);
		kbdlaux(t, uu, uqs);
		return(0);
	default:
		/*
		 *+ Bad argument passed to kdblist
		 */
		cmn_err(CE_PANIC, "KBD: kdblist\n");
	}
	/* NOTREACHED */
}

/*
 * void
 * kbdlaux(struct kbd_tab *t, struct kbduu *uu, struct kbd_query *uqs)
 *	Auxiliary list routine.  copy info into kbd_query for one table.
 *
 * Calling/Exit State:
 *	no locking
 */

STATIC void
kbdlaux(struct kbd_tab *t, struct kbduu *uu, struct kbd_query *uqs)
{
	int i;
	struct kbd_tab **tb;

	for (i = 0; i < KBDNL; i++)
		uqs->q_name[i] = t->t_name[i];
	uqs->q_id = (long) t;
	uqs->q_ref = t->t_ref;
	uqs->q_asize = t->t_asize;
	uqs->q_flag = 0;
	if (t->t_flag == KBD_COT) {
		uqs->q_flag |= KBD_QF_COT;
		uqs->q_nchild = t->t_nodes;
		/*
		 * We are only doing tables on the "available" list,
		 * not "attached" tables; therefore, we "know" that
		 * t_child is an array of pointers to the tables...
		 */
		tb = (struct kbd_tab **) t->t_child;
		for (i = 0; i < uqs->q_nchild; i++) {
			uqs->q_child[i] = (long) *tb;
			if ((*tb)->t_flag & KBD_TIME)
				uqs->q_chtim[i] = 1;
			else
				uqs->q_chtim[i] = 0;
			++tb;
		}
	}
	else {
		uqs->q_nchild = 0;
		if (t->t_flag & KBD_TIME)
			uqs->q_flag |= KBD_QF_TIM;
		if (t->t_flag & KBD_ALP)
			uqs->q_flag |= KBD_QF_EXT;
	}
	if (kbd_ttype(t, uu->u_link) == Z_PUBLIC)
		uqs->q_flag |= KBD_QF_PUB;
	uqs->q_tach = 0;
	if (uu->iside && kbd_attached(t->t_name, uu->iside))
		uqs->q_tach |= Z_UP;
	if (uu->oside && kbd_attached(t->t_name, uu->oside))
		uqs->q_tach |= Z_DOWN;
	if (uu->iside)
		uqs->q_hkin = uu->iside->u_hot;
	else
		uqs->q_hkin = '\0';
	if (uu->oside)
		uqs->q_hkout = uu->oside->u_hot;
	else
		uqs->q_hkout = '\0';
}

/*
 * void
 * kbdone(mblk_t *mp, unsigned char *s)
 *	Do one-one keyboard mapping on the message; change the message
 *	in-place, don't copy it.
 *
 * Calling/Exit State:
 *	no locking
 */

STATIC void
kbdone(mblk_t *mp, unsigned char *s)
{
	unsigned char *r, *w;

	do {
		r = mp->b_rptr;
		w = mp->b_wptr;
		while (r < w) {
			*r = s[*r];
			++r;
		}
		mp = mp->b_cont;
	} while (mp);
}

/*
 * int
 * kbd2big(int umem, int siz, int id)
 *	Is it reasonable to even try to load this thing?  If it's bigger than
 *	the max table or smaller than the header, it's bad.  If a normal user
 *	would exceed quota, it's too big.
 *
 * Calling/Exit State:
 *	no locking
 */

STATIC int
kbd2big(int umem, int siz, int id)
{
	/*
	 * If too small or too big don't even try.
	 */
	if ((siz > KBDTMAX) || (siz <= sizeof(struct kbd_tab))) {
		return(1);	/* bad size */
	}
	if (id != 0) {
		if ((umem + siz) > kbd_umem)
			return(1);	/* no space left to load it */
	}
	return(0);
}

/*
 * struct kbd_tab *
 * kbdfind(unsigned char *name, struct kbduu *uu)
 * 	Find out if there is a table by the given name that
 *	is accessible to this user.  We first check all the
 *	private tables the user has in core, then check the
 *	public list of tables.  If we find one, return its
 *	address.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC struct kbd_tab *
kbdfind(unsigned char *name, struct kbduu *uu)
{
	struct kbd_tab *t;

	/*
	 * Check private before public.
	 */
	t = uu->u_link;
	while (t) {
		if (zzcompare(name, t->t_name)) {
			return(t);
		}
		t = t->t_next;
	}
	t = tables;
	while (t) {
		if (zzcompare(name, t->t_name)) {
			return(t);
		}
		t = t->t_next;
	}
	return(NULL);
}

/*
 * struct tablink *
 * kbd_attached(unsigned char *name, struct kbdusr *d)
 *	Look in a user's list of currently attached tables and see if
 *	the named table is attached.  If so, return the address of the
 *	tablink structure that points to it.  We compare by NAME for
 *	a couple of reasons: (1) checking for attachments during LOAD
 *	ioctl we only get the name, (2) when checking attachments for
 *	the LIST ioctl, we need to check by name, because it may be
 *	a composite table, and the "attached" table will NOT have the
 * 	same address as the "available" composite (see kbdlinktab() and
 *	kbdnewcomp()).
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC struct tablink *
kbd_attached(unsigned char *name, struct kbdusr *d)
{
	struct tablink *tl;

	tl = d->u_list;
	while (tl) {
		if (zzcompare(name, tl->l_me->t_name))
			return(tl);
		tl = tl->l_next;
	}
	return((struct tablink *) 0);
}

/*
 * int
 * zzcompare(unsigned char *x, unsigned char *y)
 *	Compare two strings.  Return 1 if match, else 0 if no match.
 *
 * Calling/Exit State:
 *	no locking
 */

STATIC int
zzcompare(unsigned char *x, unsigned char *y)
{
	while (*x == *y) {
		if (*x == '\0')
			return(1);
		++x;
		++y;
	}
	return(0);
}

/*
 * struct kbdusr *
 * new_side(struct kbdusr **addr)
 *	Routines to allocate the kbdusr structures for different sides of
 *	the module.  Take a pointer to one of either "iside" or "oside" to
 *	put a pointer to the new structure in.  The double indirection
 *	saves having two separate functions, one for each side.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC struct kbdusr *
new_side(struct kbdusr **addr)
{
	*addr = (struct kbdusr *) kmem_alloc(sizeof(struct kbdusr), KM_NOSLEEP);
	if (*addr)
		kbdfill(*addr);
	return(*addr);
}

/*
 * int
 * kbd_ttype(struct kbd_tab *ut, struct kbd_tab *re)
 *	Return Z_PRIVATE or Z_PUBLIC depending on where the table
 *	(ut) is found.  Check private list first.  We just check to
 *	see if the table (ut) is the same as one on a list.  This is
 *	called on tables retrieved via "kbdfind()". "ut" is the table,
 *	"re" is the list.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbd_ttype(struct kbd_tab *ut, struct kbd_tab *re)
{
	while (re) {	/* check private first */
		if (ut == re)
			return(Z_PRIVATE);
		re = re->t_next;
	}
	re = tables;
	while (re) {	/* then check public */
		if (ut == re)
			return(Z_PUBLIC);
		re = re->t_next;
	}
	return(0);
}

/*
 * RESULTNODE is true for nodes that are a result.  Their "child" pointers
 * point to text space offset, rather than to the next level in the tree.
 */
#define RESULTNODE(Q)	(n[Q].c_flag & ND_RESULT)
/*
 * LASTNODE is true for nodes that are the last node at their level.
 * They signal the end of processing for a certain tree level.  (They
 * may, of course, also be result nodes.)
 */
#define LASTNODE(Q)	(n[Q].c_flag & ND_LAST)
/*
 * INLINENODE is true when the "result" (a one-byte string) has been
 * hoisted in-line; that is, "child" IS the result.  We can save some
 * space, etc. with this.
 */
#define INLINENODE(Q)	(n[Q].c_flag & ND_INLINE)
/*
 * A match is found when the current pointer is equal to the value
 * of the node we're looking at.
 */
#define MATCHNODE(X, Y)	(X == n[Y].c_val)
/*
 * This is what kbdproc does to "put" what it produces:
 * Usually, it's linkb(*out, mp) or *out = mp.  It's also used
 * by the subsidiary routine kbdresult (below).
 */
#define PROCPUT(x, y) if (x) linkb(x, y); else x = y;

/*
 * int
 * kbdproc(queue_t *q, mblk_t *mp, struct kbdusr *d, struct tablink *sta, mblk_t **out, int pos)
 *	First, we check for one-one mapping and get that out of the way.
 *	Then we check for HOT keys in the "not-translating" state (i.e.,
 *	no current table), which will allow us to switch into a "translating"
 *	state.  In the translating state, we check the whole message for hot
 *	keys (if idle), or if we're in the "search" state, we check each byte
 *	against the current level in the table.  When we find a match of the
 *	current byte to something at the current level then we either (1) jump
 *	to the next level, if it's NOT a result node, or (2) send up the result.
 *	The code is somewhat contorted trying to cover all the bases.  Inside
 *	the outermost "while" loop, there are basically two things.  One
 *	is if we have NO table, the other is if we HAVE a table.  Inside
 *	the "no table" block, we just loop again looking for something to
 *	turn us on.  If we find anything, we break back out to the outer loop,
 *	otherwise, we can dispose of the message very quickly.
 *
 *	Return value is 0 if we switched tables, else non-zero.  If we
 *	switch tables, we have to run the rest of the message on the
 *	new table (if any).  If switch to "no table", then return the
 *	message in "out" (caller will put it).
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbdproc(queue_t *q, mblk_t *mp, struct kbdusr *d, struct tablink *sta, mblk_t **out, int pos)
{
	unsigned char *rp;
	int j;
	struct cornode *n;
	struct kbd_tab *t;
	mblk_t *svp;

	t = (sta ? sta->l_me : NULL);
	/*
	 * If it's an external algorithm, run it.  The stuff around
	 * "t->t_alpfunc" is casting a cornode pointer (t_oneone, alias
	 * "t_alpfunc") into a pointer to a function returning a pointer
	 * to an mblk_t...and then calling it with two arguments.
	 */
	if (t && (t->t_flag & KBD_ALP)) {
		/*
		 * If we're checking for hotkeys then:
		 */
		if ((pos == 0) && (d->u_hot)) {
			if (mp->b_cont)
				pullupmsg(mp, -1); /* cat together */
			rp = mp->b_rptr;
			while (rp < mp->b_wptr) {
				if (*rp == d->u_hot) {
					/*
					 * Split the message in 2 pieces:
					 * process "svp", leave "mp".
					 */
					svp = buf_to_msg(mp->b_rptr, rp);
					mp->b_rptr = ++rp; /* after hotkey! */
					if (svp) {
/*
 * The first call sends stuff BEFORE the hot key to the algorithm;
 * the second makes sure it's flushed completely, because we're switching
 * off of it.  If the state is not currently synchronized, there will
 * be some "blips" leftover...
 */
						if (svp = (*(mblk_t *(*)()) t->t_alpfunc)(svp, sta->l_alpid))
							PROCPUT(*out, svp);

						if (svp = (*(mblk_t *(*)()) t->t_alpfunc)((mblk_t *) 0, sta->l_alpid))
							PROCPUT(*out, svp);
					}
					hotsw(d);
					if (d->u_str[0])
						kbdverb(q, d);
					return(0);	/* switched */
				}
				++rp;
			}
		}
		if (mp = (*(mblk_t *(*)()) t->t_alpfunc)(mp, sta->l_alpid))
			PROCPUT(*out, mp);
		return(1);
	}

	if (t && (t->t_flag & KBD_ONE)) { /* do any one-one swapping FIRST */
		kbdone(mp, t->t_oneone);
	}
loop:
	rp = mp->b_rptr;
	for (;;) {
		/*
		 * See if we have looked at a block without doing
		 * anything to it.  If we have, unlink & ship it.
		 */
		if (rp >= mp->b_wptr) {
			svp = mp;
			mp = unlinkb(mp);
			if (rp > svp->b_rptr) {
				PROCPUT(*out, svp);
			}
			else
				freeb(svp);
			if (! mp)
				return(1);
			rp = mp->b_rptr;
		}
		/*
		 * If no table, look for hot keys (if user defined a
		 * hot key).
		 */
		if (! t) {	/* not xlating: start now */
			if (d->u_hot) {
				while (rp < mp->b_wptr) {
					if (*rp++ == d->u_hot) {
						hotsw(d);
						/*
						 * Ship up all before this.
						 */
						if (rp > mp->b_rptr &&
						    rp < mp->b_wptr) {
							kbdsend(mp, rp - 1, out);
						}
						mp->b_rptr = rp;
						/*
						 * In verbose mode, inform the
						 * user of the new table.
						 */
						if (d->u_str[0])
							kbdverb(q, d);
						return(0);
					}
				}
				/*
				 * We just ran through whole block and saw
				 * nothing interesting.  Unlink if linked,
				 * else just send it along
				 */
				if (! mp->b_cont) {
					PROCPUT(*out, mp);
					return(1);
				}
				else {
					svp = mp;
					mp = unlinkb(mp);
					PROCPUT(*out, svp);
					if (! mp)
						return(1);
					goto loop;
				}
				
			}
			else {
				PROCPUT(*out, mp);
				return(1);
			}
		}
		if (! sta) {
			/*
			 *+ KBD internal problem
			 */
			cmn_err(CE_WARN, "UX:KBD: 'kbdproc' without state\n");
			return(1);
		}
		j = sta->l_node;
		if (sta->l_state & IDLE) {
			/*
			 * Check for hot key before anything else.  Toggle
			 * to next or "off" depending on mode.  Only the
			 * lowermost table (! pos) checks for hot keys
			 * here.  If we don't have a table, then hot keys
			 * are checked above.
			 */
			if (! pos) {
				if (d->u_hot && (*rp == d->u_hot)) {
					++rp;
					++mp->b_rptr;
					hotsw(d);
					if (d->u_str[0])
						kbdverb(q, d);
					return(0);
				}
			}
			if (! t->t_nodes) {
				++rp;
				continue;
			}
			/*
			 * This checks the current table.  We start at the
			 * top node (0) of the table and look straight
			 * through nodes until we find a correspondence,
			 * or until we run out of nodes (the ND_LAST bit
			 * of c_flag is set).
			 */
			n = t->t_nodep;
/*
 * If a "full table", do by index operation, otherwise, use the do loop.
 * In a full table, the first (tmax-tmin) nodes will be filled with "root"
 * stuff.  We'll index, and if we come up with an "ND_INLINE" node, then
 * we'll replace the contents we have with what's in "child".  If we have
 * something that's outside of tmin/tmax boundaries, it will never fit, so
 * just continue immediately.  If we have a NON-inline node, and it's in
 * bounds, then it MUST be the "top" of a subtree, so when we "goto
 * doit_doit", we'll have a match immediately.  Could be somewhat more
 * optimal in where it goes from here (i.e., we have a match, so goto
 * INSIDE the next do loop).
 */
			if (t->t_flag & KBD_FULL) {
				if ((*rp > t->t_max) || (*rp < t->t_min)) {
					/* no chance to map */
					++rp;
					continue; /* still idle */
				}
				j = (int) (*rp - t->t_min); /* re-use j */
				if (! INLINENODE(j)) {
					--j; /* because of "++j" below */
					goto doit_doit;
				}
				else {
					*rp++ = (unsigned char) n[j].c_child;
					continue;
				}
			}
			j = (-1);	/* see the other do loop */
		doit_doit:
			do {
				++j;
				if (MATCHNODE(*rp, j)) {
/*
 * This handles "new style" one-one mappings, where the child IS the result.
 * We just replace the contents, then act as if we haven't seen anything.
 * The result is that the char is replaced by the result, and we just keep
 * going: stay IDLE! NOTE: we could re-work some of the code above so that
 * we just do an index above then jump into this loop at this point, because
 * we "know" that we have a match.
 */
					if (INLINENODE(j)) {
						*rp = (unsigned char) n[j].c_child;
						break;	/* out of do...while */
					}
					if (rp > mp->b_rptr)
						kbdsend(mp, rp, out);
					++rp;
					++mp->b_rptr;
					if (RESULTNODE(j)) {
/*
 * Found a result that's a one-many mapping.
 */
						kbdresult((int) n[j].c_child, t->t_textp, out);
						sta->l_node = 0;
						sta->l_state |= IDLE;
						sta->l_state &= ~SRCH;
						goto idlebreak;
					}
					else {
/*
 * This is the ugly part.  We have to save up all the stuff that goes
 * into making up one of these input strings.
 */
						sta->l_state &= ~IDLE;
						sta->l_state |= SRCH;
						sta->l_node = n[j].c_child;
						sta->l_ptr = &(sta->l_msg[0]);
						*sta->l_ptr++ = *(rp - 1);
						if (sta->l_time)
							kbd_setime(d, sta);
					}
					goto planB;
				}
			} while (! LASTNODE(j)); /* see next do loop */
			/*
			 * If we're still idle, check the rest...At this
			 * point, we have something at the beginning of
			 * the message and it doesn't match.  We have
			 * to conserve it, not toss it...which is why
			 * we use "rp".
			 */
			if (sta->l_state & IDLE) {
				++rp;
				continue;
			}
		}

planB:		/*
		 * l_state & SRCH is set when we have seen the first of a
		 * sequence.  We look for subsequent bytes of the sequence.
		 * If we find a match, we put it in l_node.  If we have an
		 * actual result, we're ok.  If we get to the end of
		 * the list at the current level, and we STILL haven't seen
		 * a match, then we're in trouble.  Luckily, we've saved
		 * the input sequence in l_msg, so we can reconstruct what
		 * happened!
		 */
		if ((sta->l_state & SRCH) && (rp < mp->b_wptr)) {
			int flag;
			mblk_t *tp;
			j = sta->l_node - 1;
			n = t->t_nodep;
			flag = 0;
			do {
				++j;
				if (MATCHNODE(*rp, j)) {
					++flag;
					++rp;
					++mp->b_rptr;
					if (RESULTNODE(j)) {
/*
 * Here, we have to catch ND_INLINE nodes also.  They are RESULT
 * nodes, but we don't need kbdresult().  Just decrement b_rptr,
 * stuff in the c_child contents, and go idle.  If we ever get
 * any other stuff, the IDLE code will notice that rp > b_rptr,
 * and send it upstream.
 */
						if (INLINENODE(j))
							*(--mp->b_rptr) = (unsigned char) n[j].c_child;
						else
							kbdresult((int) n[j].c_child, t->t_textp, out);
						sta->l_node = 0;
						sta->l_state |= IDLE;
						sta->l_state &= ~SRCH;
						if (sta->l_tid) {
							untimeout(sta->l_tid);
							sta->l_lbolt = 0;
							sta->l_tid = 0;
						}
						goto idlebreak;
					}
					else {
						sta->l_node = n[j].c_child;
						*sta->l_ptr++ = *(rp - 1);
						if (rp >= mp->b_wptr)
							goto idlebreak;
					}
				}
			} while (! LASTNODE(j));
			if (! flag) {
				/*
				 * Reset the l_node counter and send up the
				 * first byte of the original.  Leave the
				 * rest in the buffer by copying the
				 * sta->l_msg buffer and try it next
				 * as a top level entry (note: we never
				 * incremented rp after seeing that it
				 * didn't match, so it's still at "rp".
				 */
				sta->l_node = 0;
				sta->l_state |= IDLE;
				sta->l_state &= ~SRCH;
				if (sta->l_tid) {
					untimeout(sta->l_tid);
					sta->l_lbolt = 0;
					sta->l_tid = 0;
				}
				tp = buf_to_msg(sta->l_msg, sta->l_ptr);
				sta->l_ptr = &(sta->l_msg[0]);
/*
 * At this point "rp" is the byte that failed; tp holds all
 * the rest of the stuff.  We send the first byte of tp, then if there's
 * anything left, link "mp" onto the end of "tp", reset "mp" to point at
 * "tp" and loop.  This lets us check out everything EXCEPT the first
 * byte of the stuff that didn't match.  In the case where it's a 2-byte
 * sequence, the 2nd of which didn't match, "tp" will end up being empty,
 * so we chuck it, which leaves the current "rp" as-is, with the state
 * reset so we'll check it against the top-most level.
 */
				if (tp) {
					kbd_senderr(t, *tp->b_rptr++, out);
/*
 * Save the byte that failed.  The rest of the stuff in "mp" had better
 * be duplicated in "tp"...
 */
					mp->b_rptr = rp;
					if (tp->b_rptr < tp->b_wptr) {
						tp->b_cont = mp;
						mp = tp;
					}
					else
						freemsg(tp);
				}
				goto loop;
			}
		}
	idlebreak:
		if (rp >= mp->b_wptr) {
			svp = mp;
			mp = unlinkb(mp);
			if (rp > svp->b_rptr) {
				PROCPUT(*out, svp);
			}
			else
				freeb(svp);
			if (! mp)
				return(1);
			rp = mp->b_rptr;
		}
	}
	/* NOTREACHED */
}

/*
 * int
 * kbdresult(int off, unsigned char *textp, mblk_t **where)
 *	Send the result up.
 *
 * Calling/Exit State:
 *	no locking
 */

STATIC int
kbdresult(int off, unsigned char *textp, mblk_t **where)
{
	unsigned char *p, *s;
	mblk_t *mp;
	int len;

	p = s = textp + off;
	len = 0;
	while (*s++)
		++len;
	if (! (mp = allocb(len, BPRI_HI))) {
		return(0);
	}
	while (*p)
		*mp->b_wptr++ = *p++;
	PROCPUT(*where, mp);
	return(1);
}

#define is_a_reader(x)	(x->q_flag & QREADR)

/*
 * void
 * kbdverb(queue_t *q, struct kbdusr *d)
 *	We need to know what kind of queue this is.  We send the
 *	verbose message out the write queue side only.  Reset the
 *	queue if it's the wrong side.  Realistically, people won't
 *	be putting verbose messages on the wrong side.
 *
 * Calling/Exit State:
 *	no locking
 */

STATIC void
kbdverb(queue_t *q, struct kbdusr *d)
{
	unchar *s, *t;
	int len;
	mblk_t *mp;
	int chk;

	s = t = d->u_str;
	len = 0;
	while (*t++)	/* figure out max length of string to send */
		len++;
	if (d->u_current) {	/* don't index if none... */
		t = d->u_current->t_name;
		while (*t++)
			len++;
	}
	/*
	 * Try to get 128 bytes; if that fails, try for "len".
	 * Print up to the available buffer size only.
	 */
	if (! (mp = allocb(128, BPRI_MED))) {
		if (! (mp = allocb(len, BPRI_MED)))
			return;
	}
	else
		len = 128;
	chk = 0;
	while (*s && chk < len) {
		if ((*s == '%') && (*(s+1) == 'n')) {
			/*
			 * If no table, don't print %n, leave it blank.
			 */
			if (d->u_current) {
				/* insert the table name for "%n" */
				t = d->u_current->t_name;
				while (*t) {
					*mp->b_wptr++ = *t++;
					++chk;
				}
			}
			s += 2;
		}
		else {
			*mp->b_wptr++ = *s++;
			++chk;
		}
	}
	if (q->q_flag & QREADR)	/* Never send a verbose message on */
		q = WR(q);	/* the wrong side, use the write side */
	putnext(q, mp);
}

/*
 * int
 * kbd_checktab(struct kbd_tab *t)
 *	We already know the size had better match properly, so check ALL the
 *	nodes for proper range, check the last node to make sure it has a
 *	last-node flag set, check the last byte of text to make sure it's null.
 *	If anything fails, return POSITIVE.  If it's clean, return 0.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC int
kbd_checktab(struct kbd_tab *t)
{
	unsigned short i, bound, tx;
	struct cornode *n;

	n = t->t_nodep;
	bound = (unsigned short) t->t_nodes;
	tx = (unsigned short) t->t_text;
	for (i = 0; i < bound; i++) {
		/*
		 * for "text pointer" type nodes:
		 */
		if (! (n[i].c_flag & ND_INLINE)) {
			/*
			 * If not a result and child is not in node bounds
			 * then fail.
			 */
			if ((!(n[i].c_flag & ND_RESULT)) && n[i].c_child >= bound) {
				return(1);  /* pointer node out of bounds */
			}
			/*
			 * If it is a result node and child out of TEXT
			 * bounds then fail it.
			 */
			if ((n[i].c_flag & ND_RESULT) && (n[i].c_child >= tx)){
				return(1);  /* text pointer out of bounds */
			}
		}
		/*
		 * and for "inline result" type nodes:
		 */
		else {
			if (! (n[i].c_flag & ND_RESULT)) {
				return(1);
			}
		}
		if (n[i].c_flag & ND_RESERVED) {
			return(1);  /* illegal bits (later compiler ver.) */
		}
	}
	if (t->t_nodes && (! n[t->t_nodes - 1].c_flag & ND_LAST)) {
		return(1);	/* node list not terminated */
	}
	if (t->t_text && (*((char *) t->t_textp + t->t_text - 1))) {
		return(1);	/* text not terminated */
	}
	return(0);
}

/*
 * void
 * hotsw(struct kbdusr *d)
 *	Switch on hot key, set current table.
 *
 * hot key modes:
 *
 * 0	Toggle through the list endlessly.  Never toggle to the
 *	off state.  E.g. tab1 tab2 tab1 tab2...
 *
 * 1	(Default) Toggle to the off state at the end of the table
 *	list.  After the off state, starts at top of list again.
 *	E.g.: tab1 tab2 off tab1...
 *
 * 2	If more than one table, toggles to the off state between
 *	every table, otherwise, behaves as mode 0.
 *	E.g. tab1 off tab2 off tab1...
 *
 * When we switch, we have to set the NEW table to "IDLE" so we don't
 * start up in the middle somewhere.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
hotsw(struct kbdusr *d)
{
	struct tablink *enfant;

	if (! d->u_list)
		return;
	switch (d->u_hkm) {
	default:
	case Z_HK0:
		if (! d->u_cp) {	/* if no current, set head */
			d->u_cp = d->u_list;
			d->u_current = d->u_list->l_me;
			d->u_ocp = (struct tablink *) 0;
			break;
		}
		if (d->u_cp->l_next) {	/* have one, next exists */
			d->u_ocp = d->u_cp;
			d->u_cp = d->u_cp->l_next;
			d->u_current = d->u_cp->l_me;
			break;
		}
		d->u_ocp = d->u_cp;	/* wrap */
		d->u_cp = d->u_list;
		d->u_current = d->u_cp->l_me;
		break;
	case Z_HK1:
		if (! d->u_cp) {	/* nothing now, set to list */
			d->u_cp = d->u_list;
			d->u_current = d->u_list->l_me;
			d->u_ocp = (struct tablink *) 0;
			break;
		}
		if (d->u_cp->l_next) {	/* working through list */
			d->u_current = d->u_cp->l_next->l_me;
			d->u_ocp = d->u_cp;
			d->u_cp = d->u_cp->l_next;
			break;
		}
		d->u_ocp = d->u_cp;	/* at end of list */
		d->u_current = NULL;
		d->u_cp = (struct tablink *) 0;
		break;
	case Z_HK2:
		if (! d->u_current) {	/* turn on next */
			if (d->u_ocp && d->u_ocp->l_next)
				d->u_current = d->u_ocp->l_next->l_me;
			else
				d->u_current = NULL;
			if (! d->u_current) {
				d->u_current = d->u_list->l_me;
				d->u_cp = d->u_list;
			}
			if (! d->u_ocp)
				d->u_ocp = d->u_cp;
		}
		else {	/* turn off current */
			d->u_ocp = d->u_cp;	/* save current */
			d->u_current = NULL;
			d->u_cp = (struct tablink *) 0;
		}
		break;
	}
	/*
	 * If we switched onto a table, set the state to IDLE.
	 * For composites, set all the children to IDLE.
	 */
	if (d->u_cp) {
		d->u_cp->l_state = IDLE;
		if (d->u_current->t_flag == KBD_COT) {
			enfant = d->u_current->t_child;
			while (enfant) {
				enfant->l_state = IDLE;
				enfant = enfant->l_next;
			}
		}
	}
}

/*
 * void
 * kbd_senderr(struct kbd_tab *t, unsigned char c, mblk_t **where)
 *	If there's an error string, send it, otherwise send the byte "c",
 *	because there's nothing else to do.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbd_senderr(struct kbd_tab *t, unsigned char c, mblk_t **where)
{
	mblk_t *mp;
	int i;
	unsigned char *s, *err;

	if (t->t_flag & KBD_ERR) {
		s = err = (t->t_textp + t->t_error);
		i = 0;
		while (*s++)	/* find length of error string */
			++i;
		if (mp = allocb(i, BPRI_MED)) {
			s = err;
			while (*s)
				*mp->b_wptr++ = *s++;
		}
		else
			return;
	}
	else {
		if (mp = allocb(1, BPRI_MED))
			*mp->b_wptr++ = c;
		else
			return;
	}
	PROCPUT(*where, mp);
}

/*
 * mblk_t *
 * buf_to_msg(unsigned char *base, unsigned char *top)
 * 	Take pointers into a buffer, alloc a message
 *	and copy the buffer contents into the message.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC mblk_t *
buf_to_msg(unsigned char *base, unsigned char *top)
{
	mblk_t *mp;

	if (base >= top)
		return((mblk_t *) 0);
	if (! (mp = allocb((top - base), BPRI_HI)))
		return((mblk_t *) 0);
	while (base < top)
		*mp->b_wptr++ = *base++;
	return(mp);
}

/*
 * STATIC void
 * kbd_setime(struct kbdusr *d, struct tablink *tl)
 *	Set a timer.  We keep track of what id got
 *	associated with what d and tl.
 * 
 * Calling/Exit State:
 *	no locking
 */

STATIC void
kbd_setime(struct kbdusr *d, struct tablink *tl)
{
	tl->l_lbolt = lbolt + tl->l_time;
	tl->l_tid = timeout(kbd_timeout, (caddr_t)d, tl->l_time);
}
		
/*
 * STATIC void
 * kbd_timeout(struct kbdusr *d)
 *	Called by timeout.  Set a flag and
 *	schedule the queue to be run.
 *
 * Calling/Exit State:
 *	No locking
 */

STATIC void
kbd_timeout(struct kbdusr *d)
{
	d->u_state |= ALRM;
	qenable(d->u_q);
}
