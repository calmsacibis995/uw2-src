/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/sad/sad.c	1.18"
#ident	"$Header: $"

/*
 * STREAMS Administrative Driver
 *
 * Currently only handles autopush and module name verification.
 */

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <io/conf.h>
#include <io/sad/sad.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <util/sysmacros.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

extern int etoimajor(major_t);

STATIC struct module_info sad_minfo = {
	0x7361, "sad", 0, INFPSZ, 0, 0
};

STATIC int sadopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int sadclose(queue_t *, int, cred_t *);
STATIC int sadwput(queue_t *, mblk_t *);
void sadinit(void);

STATIC struct qinit sad_rinit = {
	NULL, NULL, sadopen, sadclose, NULL, &sad_minfo, NULL
};

STATIC struct qinit sad_winit = {
	sadwput, NULL, NULL, NULL, NULL, &sad_minfo, NULL
};

struct streamtab sadinfo = {
	&sad_rinit, &sad_winit, NULL, NULL
};

STATIC struct autopush *strpfreep;	/* autopush freelist */
STATIC lock_t autof_mutex;		/* protects freelist */
lock_t autoh_mutex;			/* protects hash cache */
STATIC lock_t sad_mutex;		/* protects saddev array */
/*
 *+ autof_mutex is a global spin lock that protects the freelist
 */
STATIC LKINFO_DECL(sad_autof_lkinfo, "SAD::autof_mutex", 0);
/*
 *+ autoh_mutex is a global spin lock that protects the hash lists
 */
STATIC LKINFO_DECL(sad_autoh_lkinfo, "SAD::autoh_mutex", 0);
/*
 *+ sad_mutex is a global spin lock that protects the saddev array
 */
STATIC LKINFO_DECL(sad_lkinfo, "SAD::sad_mutex", 0);

/*
 * security flag initialized to set device state to public
 * and  no MAC checks for data transfer.
 */
int saddevflag = (D_MP|D_INITPUB|D_NOSPECMACDATA);

/*
 * tunable parameters, defined in sad.cf
 */
extern struct saddev saddev[];		/* sad device array */
extern int sadcnt;			/* number of sad devices */
extern struct autopush autopush[];	/* autopush data array */
extern int nautopush;			/* max number of autopushable devices */

STATIC struct autopush *ap_alloc(void);
STATIC struct autopush *ap_hfind(long, long, long, uint);
STATIC void ap_free(struct autopush *);
STATIC void ap_hadd(struct autopush *);
STATIC void ap_hrmv(struct autopush *);
STATIC void apush_ioctl(queue_t *, mblk_t *);
STATIC void apush_iocdata(queue_t *, mblk_t *);
STATIC void nak_ioctl(queue_t *, mblk_t *, int);
STATIC void ack_ioctl(queue_t *, mblk_t *, int, int, int);
STATIC void vml_ioctl(queue_t *, mblk_t *);
STATIC void vml_iocdata(queue_t *, mblk_t *);
STATIC int valid_list(struct strapush *);

#define SADHIER 1

/*
 * void
 * sadinit(void)
 *	Initialize autopush freelist.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.  Data structures initialized on return.
 */

void
sadinit(void)
{
	struct autopush *ap;
	int i;

	/*
	 * initialize locks.
	 */
	LOCK_INIT(&autoh_mutex, SADHIER, plstr, &sad_autoh_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&autof_mutex, SADHIER+1, plstr, &sad_autof_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&sad_mutex, SADHIER, plstr, &sad_lkinfo, KM_NOSLEEP);

	/*
	 * build the autopush freelist.
	 */
	strpfreep = autopush;
	ap = autopush;
	for (i = 1; i < nautopush; i++) {
		ap->ap_nextp = &autopush[i];
		ap->ap_flags = APFREE;
		ap = ap->ap_nextp;
	}
	ap->ap_nextp = NULL;
	ap->ap_flags = APFREE;
}

/*
 * int
 * sadopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
 *	Allocate a sad device.  Only one open at a time allowed per device.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

/* ARGSUSED */
STATIC int
sadopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	int i;
	pl_t pl;

	if (sflag)		/* no longer called from clone driver */
		return (EINVAL);

	/*
	 * Both USRMIN and ADMMIN are clone interfaces.
	 */
	pl = LOCK(&sad_mutex, plstr);
	for (i = 0; i < sadcnt; i++)
		if (saddev[i].sa_qp == NULL)
			break;
	if (i >= sadcnt) {		/* no such device */
		UNLOCK(&sad_mutex, pl);
#ifdef CC_PARTIAL
		CC_COUNT(CC_RE_SAD, CCBITS_RE_SAD);
#endif
		return (ENXIO);
	}
	saddev[i].sa_qp = (queue_t *)1;		/* reserved */
	UNLOCK(&sad_mutex, pl);

	switch (getminor(*devp)) {
	case USRMIN:			/* mere mortal */
		saddev[i].sa_flags = 0;
		break;

	case ADMMIN:			/* privileged user */
		if (pm_denied(credp, P_DRIVER)) {
			saddev[i].sa_qp = NULL;
			return (EPERM);
		}
		saddev[i].sa_flags = SADPRIV;
		break;

	default:
		saddev[i].sa_qp = NULL;
		return (EINVAL);
	}

	saddev[i].sa_qp = qp;
	qp->q_ptr = (caddr_t)&saddev[i];
	WR(qp)->q_ptr = (caddr_t)&saddev[i];
	/* skip first 2 minors, since they have special meanings */
	*devp = makedevice(getemajor(*devp), i + 2);
	qprocson(qp);
	return (0);
}

/*
 * int
 * sadclose(queue_t *qp, int flag, cret_t *credp)
 *	Clean up the data structures.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

/* ARGSUSED */
STATIC int
sadclose(queue_t *qp, int flag, cred_t *credp)
{
	struct saddev *sadp;

	qprocsoff(qp);
	sadp = (struct saddev *)qp->q_ptr;
	sadp->sa_addr = NULL;

/*
 * Note: it is important that this assignment be last since it frees
 * the slot for sadopen
 */

	sadp->sa_qp = NULL;
	qp->q_ptr = NULL;
	WR(qp)->q_ptr = NULL;
	return (0);
}

/*
 * int
 * sadwput(queue_t *qp, mblk_t *mp)
 *	Write side put procedure.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

STATIC int
sadwput(queue_t *qp, mblk_t *mp)
{
	struct iocblk *iocp;

	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR) {
			*mp->b_rptr &= ~FLUSHW;
			qreply(qp, mp);
		} else
			freemsg(mp);
		break;

	case M_IOCTL:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;
		switch (iocp->ioc_cmd) {
		case SAD_SAP:
		case SAD_GAP:
			apush_ioctl(qp, mp);
			break;

		case SAD_VML:
			vml_ioctl(qp, mp);
			break;

		default:
			nak_ioctl(qp, mp, EINVAL);
			break;
		}
		break;

	case M_IOCDATA:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;
		switch (iocp->ioc_cmd) {
		case SAD_SAP:
		case SAD_GAP:
			apush_iocdata(qp, mp);
			break;

		case SAD_VML:
			vml_iocdata(qp, mp);
			break;

		default:
#ifndef lint
			ASSERT(0);
#endif
			freemsg(mp);
			break;
		}
		break;

	default:
		freemsg(mp);
		break;
	} /* switch (db_type) */
	return (0);
}

/*
 * void
 * ack_ioctl(queue_t *qp, mblk_t *mp, int count, int rval, int errno)
 *	Send an M_IOCACK message in the opposite direction from qp.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

STATIC void
ack_ioctl(queue_t *qp, mblk_t *mp, int count, int rval, int errno)
{
	struct iocblk *iocp;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	iocp->ioc_count = count;
	iocp->ioc_rval = rval;
	iocp->ioc_error = errno;
	mp->b_datap->db_type = M_IOCACK;
	qreply(qp, mp);
}

/*
 * void
 * nak_ioctl(queue_t *qp, mblk_t *mp, int errno)
 *	Send an M_IOCNAK message in the opposite direction from qp.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

STATIC void
nak_ioctl(queue_t *qp, mblk_t *mp, int errno)
{
	struct iocblk *iocp;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	iocp->ioc_error = errno;
	if (mp->b_cont) {
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
	}
	mp->b_datap->db_type = M_IOCNAK;
	qreply(qp, mp);
}

/*
 * void
 * apush_ioctl(queue_t *qp, mblk_t *mp)
 *	Handle the M_IOCTL messages associated with the autopush feature.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

STATIC void
apush_ioctl(queue_t *qp, mblk_t *mp)
{
	struct iocblk *iocp;
	struct copyreq *cqp;
	struct saddev *sadp;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	if (iocp->ioc_count != TRANSPARENT) {
		nak_ioctl(qp, mp, EINVAL);
		return;
	}
	sadp = (struct saddev *)qp->q_ptr;
	switch (iocp->ioc_cmd) {
	case SAD_SAP:
		if (!(sadp->sa_flags & SADPRIV)) {
			nak_ioctl(qp, mp, EPERM);
			break;
		}
		/* FALLTHRU */

	case SAD_GAP:
		/* LINTED pointer alignment */
		cqp = (struct copyreq *) mp->b_rptr;
		/* LINTED pointer alignment */
		cqp->cq_addr = (caddr_t) *(long *) mp->b_cont->b_rptr;
		sadp->sa_addr = cqp->cq_addr;
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
		cqp->cq_size = sizeof(struct strapush);
		cqp->cq_flag = 0;
		cqp->cq_private = (mblk_t *)GETSTRUCT;
		mp->b_datap->db_type = M_COPYIN;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		qreply(qp, mp);
		break;

	default:
#ifndef lint
		ASSERT(0);
#endif
		nak_ioctl(qp, mp, EINVAL);
		break;
	} /* switch (ioc_cmd) */
}

/*
 * void
 * apush_iocdata(queue_t *qp, mblk_t *mp)
 *	Handle the M_IOCDATA messages associated with the autopush feature.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

STATIC void
apush_iocdata(queue_t *qp, mblk_t *mp)
{
	pl_t pl;
	int i;
	int idx;
	struct copyreq *cqp;
	struct copyresp *csp;
	struct strapush *sap;
	struct autopush *ap;
	struct saddev *sadp;

	/* LINTED pointer alignment */
	csp = (struct copyresp *) mp->b_rptr;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *) mp->b_rptr;
	if (csp->cp_rval) {	/* if there was an error */
		freemsg(mp);
		return;
	}
	if (mp->b_cont)
		/* sap needed only if mp->b_cont is set */
		/* LINTED pointer alignment */
		sap = (struct strapush *) mp->b_cont->b_rptr;
	switch (csp->cp_cmd) {
	case SAD_SAP:
		switch ((int)csp->cp_private) {
		case GETSTRUCT:
			switch (sap->sap_cmd) {
			case SAP_ONE:
			case SAP_RANGE:
			case SAP_ALL:
				if ((sap->sap_npush == 0) ||
				    (sap->sap_npush > MAXAPUSH) ||
				    (sap->sap_npush > nstrpush)) {

					/* invalid number of modules to push */

					nak_ioctl(qp, mp, EINVAL);
					break;
				}
				if ((idx = etoimajor((major_t) sap->sap_major)) == -1) {

					/* invalid major device number */

					nak_ioctl(qp, mp, EINVAL);
					break;
				}
				if ((sap->sap_cmd == SAP_RANGE) &&
				    (sap->sap_lastminor <= sap->sap_minor)) {

					/* bad range */

					nak_ioctl(qp, mp, ERANGE);
					break;
				}
				if (cdevsw[idx].d_str == NULL) {

					/* not a STREAMS driver */

					nak_ioctl(qp, mp, ENOSTR);
					break;
				}
				pl = LOCK(&autoh_mutex, plstr);
				ap = ap_hfind(sap->sap_major, sap->sap_minor,
				    sap->sap_lastminor, sap->sap_cmd);
				if (ap) {

					/* already configured */

					UNLOCK(&autoh_mutex, pl);
					nak_ioctl(qp, mp, EEXIST);
					break;
				}
				if (!valid_list(sap)) {

					/* bad module name */

					UNLOCK(&autoh_mutex, pl);
					nak_ioctl(qp, mp, EINVAL);
					break;
				}
				if ((ap = ap_alloc()) == NULL) {

					/* no autopush structures - EAGAIN? */

					UNLOCK(&autoh_mutex, pl);
					nak_ioctl(qp, mp, ENOSR);
					break;
				}
				ap->ap_common = sap->sap_common;
				for (i = 0; i < ap->ap_npush; i++)
					ap->ap_list[i] = (ushort) findmod(sap->sap_list[i]);
				ap_hadd(ap);
				UNLOCK(&autoh_mutex, pl);
				ack_ioctl(qp, mp, 0, 0, 0);
				break;

			case SAP_CLEAR:
				if ((idx = etoimajor((major_t) sap->sap_major)) == -1) {

					/* invalid major device number */

					nak_ioctl(qp, mp, EINVAL);
					break;
				}
				if (cdevsw[idx].d_str == NULL) {

					/* not a STREAMS driver */

					nak_ioctl(qp, mp, ENOSTR);
					break;
				}
				pl = LOCK(&autoh_mutex, plstr);
				if ((ap = ap_hfind(sap->sap_major, sap->sap_minor, sap->sap_lastminor, sap->sap_cmd)) == NULL) {

					/* not configured */

					UNLOCK(&autoh_mutex, pl);
					nak_ioctl(qp, mp, ENODEV);
					break;
				}
				if ((ap->ap_type == SAP_RANGE) && (sap->sap_minor != ap->ap_minor)) {

					/* starting minors do not match */

					UNLOCK(&autoh_mutex, pl);
					nak_ioctl(qp, mp, ERANGE);
					break;
				}
				if ((ap->ap_type == SAP_ALL) && (sap->sap_minor != 0)) {

					/* SAP_ALL must have minor == 0 */

					UNLOCK(&autoh_mutex, pl);
					nak_ioctl(qp, mp, EINVAL);
					break;
				}
				ap_hrmv(ap);
				UNLOCK(&autoh_mutex, pl);
				ap_free(ap);
				ack_ioctl(qp, mp, 0, 0, 0);
				break;

			default:
				nak_ioctl(qp, mp, EINVAL);
				break;
			} /* switch (sap_cmd) */
			break;

		default:
#ifndef lint
			ASSERT(0);
#endif
			freemsg(mp);
			break;
		} /* switch (cp_private) */
		break;

	case SAD_GAP:
		switch ((int)csp->cp_private) {
		case GETSTRUCT:
			if ((idx = etoimajor((major_t) sap->sap_major)) == -1) {

				/* invalid major device number */

				nak_ioctl(qp, mp, EINVAL);
				break;
			}
			if (cdevsw[idx].d_str == NULL) {

				/* not a STREAMS driver */

				nak_ioctl(qp, mp, ENOSTR);
				break;
			}
			pl = LOCK(&autoh_mutex, plstr);
			if ((ap = ap_hfind(sap->sap_major, sap->sap_minor, sap->sap_lastminor, SAP_ONE)) == NULL) {

				/* not configured */

				UNLOCK(&autoh_mutex, pl);
				nak_ioctl(qp, mp, ENODEV);
				break;
			}
			sap->sap_common = ap->ap_common;
			for (i = 0; i < ap->ap_npush; i++)
				strcpy(sap->sap_list[i], fmodsw[ap->ap_list[i]].f_name);
			UNLOCK(&autoh_mutex, pl);
			for ( ; i < MAXAPUSH; i++)
				bzero(sap->sap_list[i], FMNAMESZ + 1);
			mp->b_datap->db_type = M_COPYOUT;
			mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
			cqp->cq_private = (mblk_t *)GETRESULT;
			sadp = (struct saddev *)qp->q_ptr;
			cqp->cq_addr = sadp->sa_addr;
			cqp->cq_size = sizeof(struct strapush);
			cqp->cq_flag = 0;
			qreply(qp, mp);
			break;

		case GETRESULT:
			ack_ioctl(qp, mp, 0, 0, 0);
			break;

		default:
#ifndef lint
			ASSERT(0);
#endif
			freemsg(mp);
			break;
		} /* switch (cp_private) */
		break;

	default:	/* can't happen */
#ifndef lint
		ASSERT(0);
#endif
		freemsg(mp);
		break;
	} /* switch (cp_cmd) */
}

/*
 * struct autopush *
 * ap_alloc(void)
 *	Allocate an autopush structure.
 *
 * Calling/Exit State:
 *	Called with autoh_mutex held.
 */

STATIC struct autopush *
ap_alloc(void)
{
	struct autopush *ap;
	pl_t pl;

	pl = LOCK(&autof_mutex, plstr);
	if (strpfreep == NULL) {
		UNLOCK(&autof_mutex, pl);
		return (NULL);
	}
	ap = strpfreep;
	ASSERT(ap->ap_flags == APFREE);
	strpfreep = strpfreep->ap_nextp;
	UNLOCK(&autof_mutex, pl);
	ap->ap_nextp = NULL;
	ap->ap_flags = APUSED;
	return (ap);
}

/*
 * void
 * ap_free(struct autopush *ap)
 *	Give an autopush structure back to the freelist.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

STATIC void
ap_free(struct autopush *ap)
{
	pl_t pl;

	ASSERT(ap->ap_flags & APUSED);
	ASSERT(!(ap->ap_flags & APHASH));
	ap->ap_flags = APFREE;
	pl = LOCK(&autof_mutex, plstr);
	ap->ap_nextp = strpfreep;
	strpfreep = ap;
	UNLOCK(&autof_mutex, pl);
}

/*
 * void
 * ap_hadd(struct autopush *ap)
 *	Add an autopush structure to the hash list.
 *
 * Calling/Exit State:
 *	Called with autoh_mutex held.
 */

STATIC void
ap_hadd(struct autopush *ap)
{
	ASSERT(ap->ap_flags & APUSED);
	ASSERT(!(ap->ap_flags & APHASH));
	ap->ap_nextp = strphash(ap->ap_major);
	strphash(ap->ap_major) = ap;
	ap->ap_flags |= APHASH;
}

/*
 * void
 * ap_hrmv(struct autopush *ap)
 *	Remove an autopush structure from the hash list.
 *
 * Calling/Exit State:
 *	Called with autoh_mutex held.
 */

STATIC void
ap_hrmv(struct autopush *ap)
{
	struct autopush *hap;
	struct autopush *prevp = NULL;

	ASSERT(ap->ap_flags & APUSED);
	ASSERT(ap->ap_flags & APHASH);
	hap = strphash(ap->ap_major);
	while (hap) {
		if (ap == hap) {
			hap->ap_flags &= ~APHASH;
			if (prevp)
				prevp->ap_nextp = hap->ap_nextp;
			else
				strphash(ap->ap_major) = hap->ap_nextp;
			return;
		} /* if */
		prevp = hap;
		hap = hap->ap_nextp;
	} /* while */
}

/*
 * struct autopush *
 * ap_hfind(long maj, long minor, long last, uint cmd)
 *	Look for an autopush structure in the hash list based on major,
 *	minor, lastminor, and command.
 *
 * Calling/Exit State:
 *	Called with autoh_mutex held.
 */

STATIC struct autopush *
ap_hfind(long maj, long minor, long last, uint cmd)
{
	struct autopush *ap;

	ap = strphash(maj);
	while (ap) {
		if (ap->ap_major == maj) {
			if (cmd == SAP_ALL)
				break;
			switch (ap->ap_type) {
			case SAP_ALL:
				break;

			case SAP_ONE:
				if (ap->ap_minor == minor)
					break;
				if ((cmd == SAP_RANGE) &&
				    (ap->ap_minor >= minor) &&
				    (ap->ap_minor <= last))
					break;
				ap = ap->ap_nextp;
				continue;

			case SAP_RANGE:
				if ((cmd == SAP_RANGE) &&
				    (((minor >= ap->ap_minor) &&
				      (minor <= ap->ap_lastminor)) ||
				     ((ap->ap_minor >= minor) &&
				      (ap->ap_minor <= last))))
					break;
				if ((minor >= ap->ap_minor) &&
				    (minor <= ap->ap_lastminor))
					break;
				ap = ap->ap_nextp;
				continue;

			default:
#ifndef lint
				ASSERT(0);
#endif
				break;
			}
			break;
		}
		ap = ap->ap_nextp;
	}
	return (ap);
}

/*
 * int
 * valid_list(struct strapush *sap)
 *	Step through the list of modules to autopush and validate their names.
 *
 * Calling/Exit State:
 *	Called with autoh_mutex held.  Return 1 if the list is valid and 0
 *	if it is not.
 */

STATIC int
valid_list(struct strapush *sap)
{
	int i;

	for (i = 0; i < sap->sap_npush; i++)
		if (findmod(sap->sap_list[i]) == -1)
			return (0);
	return (1);
}

/*
 * void
 * vml_ioctl(queue_t *qp, mblk_t *mp)
 *	Handle the M_IOCTL message associated with a request to validate a
 *	module list.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

STATIC void
vml_ioctl(queue_t *qp, mblk_t *mp)
{
	struct iocblk *iocp;
	struct copyreq *cqp;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	if (iocp->ioc_count != TRANSPARENT) {
		nak_ioctl(qp, mp, EINVAL);
		return;
	}
	ASSERT (iocp->ioc_cmd == SAD_VML);
	/* LINTED pointer alignment */
	cqp = (struct copyreq *) mp->b_rptr;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) *(long *) mp->b_cont->b_rptr;
	freemsg(mp->b_cont);
	mp->b_cont = NULL;
	cqp->cq_size = sizeof(struct str_list);
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)GETSTRUCT;
	mp->b_datap->db_type = M_COPYIN;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	qreply(qp, mp);
}

/*
 * void
 * vml_iocdata(queue_t *qp, mblk_t *mp)
 *	Handle the M_IOCDATA messages associated with a request to validate
 *	a module list.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */

STATIC void
vml_iocdata(queue_t *qp, mblk_t *mp)
{
	int i;
	struct copyreq *cqp;
	struct copyresp *csp;
	struct str_mlist *lp;
	struct str_list *slp;
	struct saddev *sadp;

	/* LINTED pointer alignment */
	csp = (struct copyresp *) mp->b_rptr;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *) mp->b_rptr;
	if (csp->cp_rval) {	/* if there was an error */
		freemsg(mp);
		return;
	}
	ASSERT (csp->cp_cmd == SAD_VML);
	sadp = (struct saddev *)qp->q_ptr;
	switch ((int)csp->cp_private) {
	case GETSTRUCT:
		/* LINTED pointer alignment */
		slp = (struct str_list *) mp->b_cont->b_rptr;
		if (slp->sl_nmods <= 0) {
			nak_ioctl(qp, mp, EINVAL);
			break;
		}
		sadp->sa_addr = (caddr_t)slp->sl_nmods;
		cqp->cq_addr = (caddr_t)slp->sl_modlist;
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
		cqp->cq_size = (int)sadp->sa_addr * sizeof(struct str_mlist);
		cqp->cq_flag = 0;
		cqp->cq_private = (mblk_t *)GETLIST;
		mp->b_datap->db_type = M_COPYIN;
		mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
		qreply(qp, mp);
		break;

	case GETLIST:
		lp = (struct str_mlist *)mp->b_cont->b_rptr;
		for (i = 0; i < (int)sadp->sa_addr; i++, lp++)
			if (findmod(lp->l_name) == -1) {
				ack_ioctl(qp, mp, 0, 1, 0);
				return;
			}
		ack_ioctl(qp, mp, 0, 0, 0);
		break;

	default:
#ifndef lint
		ASSERT(0);
#endif
		freemsg(mp);
		break;
	} /* switch (cp_private) */
}
