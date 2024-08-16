/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/loopback/ticots/ticots.c	1.27"
#ident	"$Header: $"

/*
 *	TPI loopback transport provider.
 *	Virtual-circuit mode.
 *	Connection-oriented type (with & without orderly release).
 */

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <net/netsubr.h>
#include <net/tihdr.h>
#include <net/xti.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <util/debug.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <mem/kmem.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>
#include <proc/proc_hier.h>
#include <net/loopback/ticots.h>
#include <util/inline.h>
#include <util/mod/moddefs.h>

extern int	tco_cnt;
void		tcoinit(void);
STATIC int	tco_load(void);
STATIC int	tco_unload(void);

#define DRVNAME "ticots(ord) - Loadable TPI connection oriented loopback driver"

MOD_DRV_WRAPPER(tco, tco_load, tco_unload, NULL, DRVNAME);

STATIC void	tco_areq(queue_t *, mblk_t *);
STATIC void	tco_bind(queue_t *, mblk_t *);
STATIC void	tco_blink(tco_endpt_t *, tco_addr_t *);
STATIC int	tco_ckopt(char *, char *);
STATIC void	tco_ckstate(queue_t *, mblk_t *);
STATIC int	tco_close(queue_t *, int, cred_t *);
STATIC int	tco_cpabuf(tco_addr_t *, tco_addr_t *);
STATIC void	tco_creq(queue_t *, mblk_t *);
STATIC void	tco_cres(queue_t *, mblk_t *);
STATIC int	tco_data(queue_t *, mblk_t *, int);
STATIC void	tco_dreq(queue_t *, mblk_t *);
STATIC void	tco_errack(queue_t *, mblk_t *, long, long);
STATIC void	tco_fatal(queue_t *, mblk_t *);
STATIC int	tco_flush(queue_t *, int);
STATIC void	tco_ireq(queue_t *, mblk_t *);
STATIC void	tco_okack(queue_t *, mblk_t *);
STATIC void	tco_olink(tco_endpt_t *);
STATIC int	tco_open(queue_t *, dev_t *, int, int, cred_t *);
STATIC void	tco_openwakeup(long);
STATIC void	tco_optmgmt(queue_t *, mblk_t *);
STATIC int	tco_rsrv(queue_t *);
STATIC int	tco_sumbytes(char *, int);
STATIC void	tco_unbind(queue_t *, mblk_t *);
STATIC void	tco_unblink(tco_endpt_t *);
STATIC void	tco_unconnect(tco_endpt_t *);
STATIC void	tco_unolink(tco_endpt_t *);
STATIC int	tco_wput(queue_t *, mblk_t *);
STATIC void	tco_wropt(long, tco_endpt_t *, char *);
STATIC int	tco_wsrv(queue_t *);
STATIC int	tco_getendpt(int, minor_t, queue_t *, cred_t *, tco_endpt_t **);
STATIC int	tco_endptinit(minor_t, cred_t *, tco_endpt_t **);
STATIC tco_addr_t	*tco_addrinit(tco_addr_t *);
STATIC tco_addr_t	*tco_getaddr(int, tco_addr_t *, unsigned, int *);

int tcodevflag = D_MP;

#define	TCO_HIER	1

/*
 *+ the endpoint state lock protects sanity of the endpoint structure
 */
LKINFO_DECL(tco_endpt_lkinfo, "tco::state lock", 0);
/*
 *+ the global spin lock proctects all global hash tables and
 *+ is also used to prevent deadlocks while acquiring locks across
 *+ endpoints.
 */
LKINFO_DECL(tco_hshlkinfo, "tco::hash tables lock", 0);

lock_t	*tco_lock;	/* lock to protect the following */

STATIC tco_endpt_t	*tco_endptopen[TCO_NMHASH];  /* open endpt hash table */
STATIC tco_endpt_t	tco_defaultendpt;
STATIC tco_endpt_t	*tco_rqopen[TCO_NRQHASH];    /* te_rq hash table */
STATIC tco_addr_t	*tco_addrbnd[TCO_NAHASH];    /* bound addr hash table */
STATIC tco_addr_t	tco_defaultaddr;
STATIC char		tco_defaultabuf[TCO_DEFAULTADDRSZ];

STATIC struct module_info tco_info = {
			TCO_ID,
			"tco",
			TCO_MINPSZ,
			TCO_MAXPSZ,
			TCO_HIWAT,
			TCO_LOWAT
};

STATIC struct qinit tco_winit = {
			tco_wput,
			tco_wsrv,
			NULL,
			NULL,
			NULL,
			&tco_info,
			NULL
};

STATIC struct qinit tco_rinit = {
			NULL,
			tco_rsrv,
			tco_open,
			tco_close,
			NULL,
			&tco_info,
			NULL
};

struct streamtab tcoinfo = {
			&tco_rinit,
			&tco_winit,
			NULL,
			NULL
};


/*
 * STATIC void
 * tco_olink(tco_endpt_t *te)
 *
 * Calling/Exit State:
 *	tco_lock must be held on entry.
 *
 * Description:
 *	link endpt to tco_endptopen[] hash table, and 
 *	to tco_rqopen[] hash table
 */
STATIC void
tco_olink(tco_endpt_t *te)
{
	tco_endpt_t	**tep;


	ASSERT(te != NULL);
	/*
	 *	add te to tco_endptopen[] table
	 */
	tep = &tco_endptopen[tco_mhash(te)];
	if (*tep != NULL) {
		(*tep)->te_bolist = te;
	}
	te->te_folist = *tep;
	te->te_bolist = NULL;
	*tep = te;
	/*
	 *	add te to tco_rqopen[] table
	 */
	tep = &tco_rqopen[tco_rqhash(te)];
	if (*tep != NULL) {
		(*tep)->te_brqlist = te;
	}
	te->te_frqlist = *tep;
	te->te_brqlist = NULL;
	*tep = te;
	return;
}


/*
 * STATIC void
 * tco_unolink(tco_endpt_t *te)
 *
 * Calling/Exit State:
 *	tco_lock must be held on entry.
 *
 * Description:
 *	unlink endpt from tco_endptopen[] hash table, and from 
 *	tco_rqopen[] hash table
 */
STATIC void
tco_unolink(tco_endpt_t *te)
{
	ASSERT(te != NULL);
	/*
	 *	remove te from tco_endptopen[] table
	 */
	if (te->te_bolist != NULL) {
		te->te_bolist->te_folist = te->te_folist;
	} else {
		tco_endptopen[tco_mhash(te)] = te->te_folist;
	}
	if (te->te_folist != NULL) {
		te->te_folist->te_bolist = te->te_bolist;
	}
	/*
	 *	remove te from tco_rqopen[] table
	 */
	if (te->te_brqlist != NULL) {
		te->te_brqlist->te_frqlist = te->te_frqlist;
	} else {
		tco_rqopen[tco_rqhash(te)] = te->te_frqlist;
	}
	if (te->te_frqlist != NULL) {
		te->te_frqlist->te_brqlist = te->te_brqlist;
	}
	/*
	 *	free te
	 */
	ASSERT(te->te_savelock == NULL);
	ASSERT(te->te_bid == NULL);
	LOCK_DEALLOC(te->te_lock);
	EVENT_DEALLOC(te->te_event);
	kmem_free(te,sizeof(tco_endpt_t));
	return;
}


/*
 * STATIC void
 * tco_blink(tco_endpt_t *te, tco_addr_t *ta)
 *
 * Calling/Exit State:
 *	tco_lock must be held on entry
 *
 * Description:
 *	link endpt to addr, and addr to tco_addrbnd[] hash table
 */
STATIC void
tco_blink(tco_endpt_t *te, tco_addr_t *ta)
{
	tco_endpt_t	*te1;
	tco_addr_t	**tap;


	ASSERT(te != NULL);
	ASSERT(te->te_addr == NULL);
	ASSERT(te->te_fblist == NULL);
	ASSERT(te->te_bblist == NULL);
	ASSERT(ta != NULL);
	if (ta->ta_hblist == NULL) {
		ASSERT(ta->ta_tblist == NULL);
		/*
		 *	add ta to tco_addrbnd[] table
		 */
		tap = &tco_addrbnd[tco_ahash(ta)];
		if (*tap != NULL) {
			(*tap)->ta_balist = ta;
		}
		ta->ta_falist = *tap;
		ta->ta_balist = NULL;
		*tap = ta;
	}
	/*
	 *	link ta to te, and te to ta's list of bound endpts
	 */
	te->te_addr = ta;
	if (te->te_qlen > 0) {
		/*
		 *	link te at head of ta's list
		 */
		te->te_fblist = ta->ta_hblist;
		ASSERT(te->te_bblist == NULL);
		if ((te1 = ta->ta_hblist) != NULL) {
			te1->te_bblist = te;
		}
		ta->ta_hblist = te;
		if (ta->ta_tblist == NULL) {
			ta->ta_tblist = te;
		}
	} else {
		/*
		 *	link te at tail of ta's list
		 */
		te->te_bblist = ta->ta_tblist;
		ASSERT(te->te_fblist == NULL);
		if ((te1 = ta->ta_tblist) != NULL) {
			te1->te_fblist = te;
		}
		ta->ta_tblist = te;
		if (ta->ta_hblist == NULL) {
			ta->ta_hblist = te;
		}
	}
	return;
}


/*
 * STATIC void
 * tco_unblink(tco_endpt_t *te)
 *
 * Calling/Exit State:
 *	tco_lock must be held on entry.
 *
 * Description:
 *	unlink endpt from addr, and addr from tco_addrbnd[] hash table
 */
STATIC void
tco_unblink(tco_endpt_t *te)
{
	tco_addr_t	*ta;
	tco_endpt_t	*te1;

	ASSERT(te != NULL);
	ta = te->te_addr;
	if (ta != NULL) {
		/*
		 *	unlink ta from te, and te from ta's list of bound endpts
		 */
		ASSERT(ta->ta_hblist != NULL);
		ASSERT(ta->ta_tblist != NULL);
		te->te_addr = NULL;
		if ((te1 = te->te_bblist) == NULL) {
			ta->ta_hblist = te->te_fblist;
		} else {
			te1->te_fblist = te->te_fblist;
		}
		if ((te1 = te->te_fblist) == NULL) {
			ta->ta_tblist = te->te_bblist;
		} else {
			te1->te_bblist = te->te_bblist;
		}
		te->te_fblist = NULL;
		te->te_bblist = NULL;
		if (ta->ta_hblist == NULL) {
			/*
			 *	no endpts bound to ta; 
			 *	remove ta from tco_addrbnd[] table
			 */
			ASSERT(ta->ta_tblist == NULL);
			if (ta->ta_balist != NULL) {
				ta->ta_balist->ta_falist = ta->ta_falist;
			} else {
				tco_addrbnd[tco_ahash(ta)] = ta->ta_falist;
			}
			if (ta->ta_falist != NULL) {
				ta->ta_falist->ta_balist = ta->ta_balist;
			}
			kmem_free(tco_abuf(ta),tco_alen(ta));
			kmem_free(ta,sizeof(tco_addr_t));
		}
	}

	return;
}


/*
 * STATIC int
 * tco_sumbytes(char *a, int n)
 *
 * Calling/Exit State:
 *	No locking assumptions. The buffer will not change under
 *	you if it used correctly (ie: with local copies or with tco_lock
 *	held with global buffers).
 *
 * Description:
 *	sum bytes of buffer (used for hashing)
 */
STATIC int
tco_sumbytes(char *a, int n)
{
	unsigned	sum;
	int		i;


	ASSERT(a != NULL);
	ASSERT(n > 0);
	sum = 0;
	for (i = 0; i < n; i++) {
		sum += a[i];
	}
	return((int)sum);
}


/*
 * STATIC int
 * tco_cpabuf(tco_addr_t *to, tco_addr_t *from)
 *
 * Calling/Exit State:
 *	No locking assumptions. The buffer will not change under
 *	you if it used correctly (ie: with local copies or with tco_lock
 *	held with global buffers).
 *
 * Description:
 *	copy ta_abuf part of addr, together with ta_len, ta_ahash
 *	(this routine will create a ta_abuf if necessary, but won't resize one)
 */
STATIC int
tco_cpabuf(tco_addr_t *to, tco_addr_t *from)
{
	char				*abuf;


	ASSERT(to != NULL);
	ASSERT(from != NULL);
	ASSERT(tco_alen(from) > 0);
	ASSERT(tco_abuf(from) != NULL);
	if (tco_abuf(to) == NULL) {
		ASSERT(tco_alen(to) == 0);
		abuf = (char *)kmem_alloc(tco_alen(from),KM_NOSLEEP);
		if (abuf == NULL) {
			return(-1);
		}
		to->ta_alen = tco_alen(from);
		to->ta_abuf = abuf;
	} else {
		ASSERT(tco_alen(to) == tco_alen(from));
	}
	(void)bcopy(tco_abuf(from),tco_abuf(to),tco_alen(to));
	to->ta_ahash = from->ta_ahash;
	return(0);
}


/*
 * STATIC int
 * tco_endptinit(minor_t min, cred_t *crp, tco_endpt_t **tep)
 *
 * Calling/Exit State:
 *	tco_lock must not be held on entry.
 * 
 * Description:
 *	initialize endpoint
 */
STATIC int
tco_endptinit(minor_t min, cred_t *crp, tco_endpt_t **teptr)
{
	tco_endpt_t		*te,*te1,*te2;
	minor_t			otco_minor;
	pl_t			pl;


	/*
	 *	get an endpt
	 */
	te1 = (tco_endpt_t *)kmem_zalloc(sizeof(tco_endpt_t),KM_NOSLEEP);
	if (te1 == NULL) {
		*teptr = NULL;
		return(ENOMEM);
	}
	te1->te_state = TS_UNBND;
	te1->te_lock = LOCK_ALLOC(TCO_HIER+1, plstr, &tco_endpt_lkinfo,
						KM_NOSLEEP);
	if (te1->te_lock == NULL) {
		kmem_free(te1, sizeof(tco_endpt_t));
		*teptr = NULL;
		return(ENOMEM);
	}
	if ((te1->te_event = EVENT_ALLOC(KM_NOSLEEP)) == NULL) {
		LOCK_DEALLOC(te1->te_lock);
		kmem_free(te1,sizeof(tco_endpt_t));
		*teptr = NULL;
		return(ENOMEM);
	}
	if (min == NODEV) {
		/*
		 *	no minor number requested; we will assign one
		 */
		pl = LOCK(tco_lock, plstr);
		te = &tco_defaultendpt;
		otco_minor = tco_min(te);
		for (te2 = tco_endptopen[tco_mhash(te)]; te2 != NULL; te2 = te2->te_folist) {
			while (te2 != NULL && tco_min(te2) == tco_min(te)) {
				/*
				 *	bump default minor and try again
				 */
				if (++tco_min(te) == tco_cnt) {
					te->te_min = 0;
				}
				if (tco_min(te) == otco_minor) {
				    /*
				     *	wrapped around
				     */
				    UNLOCK(tco_lock, pl);
				    LOCK_DEALLOC(te1->te_lock);
				    EVENT_DEALLOC(te1->te_event);
				    kmem_free(te1,sizeof(tco_endpt_t));
				    *teptr = NULL;
				    return(ENOSPC);
				}
				te2 = tco_endptopen[tco_mhash(te)];
			}
			/*
			 * necessary because te2 is changing 2 dimensionally
			 * ie: in the for loop and in the inner while loop.
			 */
			if (te2 == NULL)
				break;
		}
		te1->te_min = tco_min(te);
		/*
		 *	bump default minor for next time
		 */
		if (++tco_min(te) == tco_cnt) {
			te->te_min = 0;
		}
		UNLOCK(tco_lock, pl);
	} else {
		/*
		 *	a minor number was requested; copy it in
		 */
		te1->te_min = min;
	}
	/*
	 *	ident info
	 */
	ASSERT(crp != NULL);
	te1->te_uid = crp->cr_uid;
	te1->te_gid = crp->cr_gid;
	te1->te_ruid = crp->cr_ruid;
	te1->te_rgid = crp->cr_rgid;
	*teptr = te1;
	return(0);
}


/*
 * STATIC tco_addr_t *
 * tco_addrinit(tco_addr_t *ta)
 *
 * Calling/Exit State:
 *	tco_lock must be held on entry.
 *
 * Description:
 *	initialize address
 */
STATIC tco_addr_t *
tco_addrinit(tco_addr_t *ta)
{
	tco_addr_t		*ta1,*ta2;
	int				i;
	char				*cp;


	/*
	 *	get an address
	 */
	ta1 = (tco_addr_t *)kmem_zalloc(sizeof(tco_addr_t),KM_NOSLEEP);
	if (ta1 == NULL) {
		return(NULL);
	}
	if (ta == NULL) {
		/*
		 *	no abuf requested; we will assign one
		 */
		ta = &tco_defaultaddr;
		for (ta2 = tco_addrbnd[tco_ahash(ta)]; ta2 != NULL; ) {
			if (tco_eqabuf(ta2,ta)) {
				/*
				 *	bump defaultaddr and try again
				 */
				for (i = 0, cp = tco_abuf(ta); i < tco_alen(ta); i += 1, cp += 1) {
					if ((*cp += 1) != '\0') {
						break;
					}
				}
				ta->ta_ahash = tco_mkahash(ta);
				ta2 = tco_addrbnd[tco_ahash(ta)];
				if (ta2 == NULL)
					break;
			} else {
				ta2 = ta2->ta_falist;
			}
		}
		if (tco_cpabuf(ta1,ta) == -1) {
			kmem_free(ta1,sizeof(tco_addr_t));
			return(NULL);
		}
		/*
		 *	bump defaultaddr for next time
		 */
		for (i = 0, cp = tco_abuf(ta); i < tco_alen(ta); i += 1, cp += 1) {
			if ((*cp += 1) != '\0') {
				break;
			}
		}
		ta->ta_ahash = tco_mkahash(ta);
	} else {
		/*
		 *	an abuf was requested; copy it in
		 */
		if (tco_cpabuf(ta1,ta) == -1) {
			kmem_free(ta1,sizeof(tco_addr_t));
			return(NULL);
		}
	}
	return(ta1);
}


/*
 * STATIC int
 * tco_getendpt(int flg,minor_t min,queue_t *rq,cred_t *crp,tco_endpt_t **tep)
 *
 * Calling/Exit State:
 * 	tco_lock is not held on entry.
 *
 * Description:
 *	search tco_endptopen[] or tco_rqopen[] for endpt
 */
STATIC int
tco_getendpt(int flg, minor_t min, queue_t *rq, cred_t *crp, tco_endpt_t **tep)
{
	tco_endpt_t			endpt,*te;
	int				err;
	pl_t				pl;

	switch (flg) {
	    default:
		/*
	 	 *+ Internal error
		 */
		cmn_err(CE_PANIC, "tco_getendpt: Incorrect flag");
		/* NOTREACHED */
	    case TCO_OPEN:
		/*
		 *	open an endpoint
		 */
		if (min == NODEV) {
			/*
			 * no minor number requested; any free endpt will do
			 */
			return(tco_endptinit(min, crp, tep));
		} else {
			/*
			 *	find endpt with the requested minor number
			 */
			endpt.te_min = min;
			pl = LOCK(tco_lock, plstr);
			for (te = tco_endptopen[tco_mhash(&endpt)]; te != NULL; te = te->te_folist) {
				if (tco_min(te) == tco_min(&endpt)) {	
					/*
					 * Detect if clone open and non-clone
					 * open race is in progress
					 */
					if ( rq && (te->te_rq != rq)) {
					    UNLOCK(tco_lock, pl);
					    *tep = NULL;
					    STRLOG(TCO_ID,-1,3,SL_TRACE,
						"tco_getendpt _%d_: CLONE/non-CLONE open race",__LINE__);
					    return(ECLNRACE);
					}
					*tep = te;
					UNLOCK(tco_lock, pl);
					return(0);
				}
			}
			UNLOCK(tco_lock, pl);
			err = tco_endptinit(min, crp, tep);
			if (err)
				return(err);
			else {
				if (rq && ((*tep)->te_rq != rq)) {
					kmem_free(*tep, sizeof(tco_endpt_t));
					*tep = NULL;
					STRLOG(TCO_ID,-1,3,SL_TRACE,
					    "tco_getendpt _%d_: CLONE/non-CLONE open race",__LINE__);
					return(ECLNRACE);
				}
				return(0);
			}
		}
		/* NOTREACHED */
	    case TCO_RQ:
		/*
		 *	find endpt with the requested te_rq
		 */
		pl = LOCK(tco_lock, plstr);
		endpt.te_rq = rq;
		endpt.te_rqhash = tco_mkrqhash(&endpt);
		for (te = tco_rqopen[tco_rqhash(&endpt)]; te != NULL; te = te->te_frqlist) {
			if (te->te_rq == (&endpt)->te_rq) {
				*tep = te;
				UNLOCK(tco_lock, pl);
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_getendpt _%d_: TCO_RQ found",__LINE__);
				return(0);
			}
		}
		UNLOCK(tco_lock, pl);
		STRLOG(TCO_ID,-1,3,SL_TRACE,
		    "tco_getendpt _%d_: TCO_RQ not found",__LINE__);
		*tep = NULL;
		return(0);	/* The error in this case is *tep == NULL */
	}
	/* NOTREACHED */
}


/* STATIC tco_addr_t *
 * tco_getaddr(int flg, tco_addr_t *ta, unsigned qlen, int *unixerr)
 *
 * Calling/Exit State:
 *	tco_lock must be held on entry.
 *
 * Description:
 *	search tco_addrbnd[] for addr
 */
STATIC tco_addr_t *
tco_getaddr(int flg,tco_addr_t *ta, unsigned qlen, int *unixerr)
{
	tco_addr_t		*ta1;

	switch (flg) {
	    default:
		/*
		 *+ Internal error
		 */
		cmn_err(CE_PANIC, "tco_getaddr: incorrect flag argument");
		/* NOTREACHED */
	    case TCO_BIND:
		/*
		 *	get an addr that's free to be bound
		 */
		if (ta == NULL) {
			/*
			 *	no abuf requested; any free addr will do
			 */
			return(tco_addrinit(NULL));
		} else {
			/*
			 *	an abuf was requested; get addr with that abuf;
			 *	Return TADDRBUSY if addr is already being used.
			 */
			for (ta1 = tco_addrbnd[tco_ahash(ta)]; ta1 != NULL; ta1 = ta1->ta_falist) {
				if (tco_eqabuf(ta1,ta)) {
					ASSERT(ta1->ta_hblist != NULL);
					ASSERT(ta1->ta_tblist != NULL);
					if ((qlen == 0) || (ta1->ta_hblist->te_qlen == 0)) {
						return(ta1);
					} else {
						/* Address is busy */
						*unixerr = -1;
						return(NULL);
					}
				}
			}
			/*
			 * Add new address to hash list
			 */
			return(tco_addrinit(ta));
		}
		/* NOTREACHED */
	    case TCO_CONN:
		/*
		 *	get addr that can be connected to (i.e., is currently bound
		 *	to an endpoint with qlen>0)
		 */
		ASSERT(ta != NULL);
		for (ta1 = tco_addrbnd[tco_ahash(ta)]; ta1 != NULL; ta1 = ta1->ta_falist) {
			ASSERT(ta1->ta_hblist != NULL);
			if (tco_eqabuf(ta1,ta)) {
				if (ta1->ta_hblist->te_qlen > 0) {
					return(ta1);
				} else {
					return(NULL);
				}
			}
		}
		return(NULL);
	}
	/* NOTREACHED */
}


/*
 * STATIC int
 * tco_ckopt(char *obuf, char *ebuf)
 *
 * Calling/Exit State:
 * 	No locking assumptions.
 *
 * Description:
 *	check validity of opt list
 */
STATIC int
tco_ckopt(char *obuf, char *ebuf)
{
	struct tco_opt_hdr		*ohdr,*ohdr1;
	union tco_opt			*opt;
	int				retval = 0;


	/*
	 *	validate format & hdrs & opts of opt list
	 */
	ASSERT(obuf < ebuf);
	/* LINTED pointer alignment */
	for (ohdr = (struct tco_opt_hdr *)obuf; ; ohdr = ohdr1) {
		if ((int)ohdr%NBPW != 0) {	/* alignment */
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad alignment",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		if ((char *)ohdr + sizeof(struct tco_opt_hdr) > ebuf) {
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		if (ohdr->hdr_thisopt_off < 0) {
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		/* LINTED pointer alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		if ((int)opt%NBPW != 0) {	/* alignment */
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad alignment",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		switch (opt->opt_type) {
		    default:
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: unknown opt",__LINE__);
			retval |= TCO_BADTYPE;
			break;
		    case TCO_OPT_NOOP:
			if ((char *)opt + sizeof(struct tco_opt_noop) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_NOOPOPT;
			break;
		    case TCO_OPT_SETID:
			if ((char *)opt + sizeof(struct tco_opt_setid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			if ((opt->opt_setid.setid_flg & ~TCO_IDFLG_ALL) != 0) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad opt",__LINE__);
				retval |= TCO_BADVALUE;
				break;
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_GETID:
			if ((char *)opt + sizeof(struct tco_opt_getid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_UID:
			if ((char *)opt + sizeof(struct tco_opt_uid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_GID:
			if ((char *)opt + sizeof(struct tco_opt_gid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_RUID:
			if ((char *)opt + sizeof(struct tco_opt_ruid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		    case TCO_OPT_RGID:
			if ((char *)opt + sizeof(struct tco_opt_rgid) > ebuf) {
				STRLOG(TCO_ID,-1,4,SL_TRACE,
				    "tco_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCO_BADFORMAT);
			}
			retval |= TCO_REALOPT;
			break;
		}
		if (ohdr->hdr_nexthdr_off < 0) {
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
		if (ohdr->hdr_nexthdr_off == TCO_OPT_NOHDR) {
			return(retval);
		}
		/* LINTED pointer alignment */
		ohdr1 = (struct tco_opt_hdr *)(obuf + ohdr->hdr_nexthdr_off);
		if (ohdr1 <= ohdr) {
			/* potential loop */
			STRLOG(TCO_ID,-1,4,SL_TRACE,
			    "tco_ckopt _%d_: potential loop",__LINE__);
			return(retval|TCO_BADFORMAT);
		}
	}
	/* NOTREACHED */
}


/*
 * STATIC void
 * tco_wropt(long idflg, tco_endpt_t *te, char *obuf)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	write opt info into buf
 */
STATIC void
tco_wropt(long idflg, tco_endpt_t *te, char *obuf)
{
	struct tco_opt_hdr		hdr,*ohdr,*oohdr;
	union tco_opt			*opt;


	/*
	 *	blindly write the opt info into obuf 
	 *	(assume obuf already set up properly)
	 */
	ASSERT(idflg & TCO_IDFLG_ALL);
	ASSERT(((int)obuf)%NBPW == 0);
	oohdr = &hdr;
	oohdr->hdr_nexthdr_off = 0;
	if (idflg & TCO_IDFLG_UID) {
		/* LINTED pointer alignment */
		ohdr = (struct tco_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tco_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tco_opt_uid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_uid.uid_type = TCO_OPT_UID;
		opt->opt_uid.uid_val = te->te_uid;
		oohdr = ohdr;
	}
	if (idflg & TCO_IDFLG_GID) {
		/* LINTED pointer alignment */
		ohdr = (struct tco_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tco_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tco_opt_gid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_gid.gid_type = TCO_OPT_GID;
		opt->opt_gid.gid_val = te->te_gid;
		oohdr = ohdr;
	}
	if (idflg & TCO_IDFLG_RUID) {
		/* LINTED pointer alignment */
		ohdr = (struct tco_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tco_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tco_opt_ruid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_ruid.ruid_type = TCO_OPT_RUID;
		opt->opt_ruid.ruid_val = te->te_ruid;
		oohdr = ohdr;
	}
	if (idflg & TCO_IDFLG_RGID) {
		/* LINTED pointer alignment */
		ohdr = (struct tco_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tco_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tco_opt_rgid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tco_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_rgid.rgid_type = TCO_OPT_RGID;
		opt->opt_rgid.rgid_val = te->te_rgid;
		oohdr = ohdr;
	}
	oohdr->hdr_nexthdr_off = TCO_OPT_NOHDR;
	return;
}


/*
 * int
 * tco_load(void)
 *	Load routine
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 */

STATIC int
tco_load()
{
	tcoinit();
	return(0);
}

/*
 * int
 * tco_unload(void)
 *	Unload routine
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 */

STATIC int
tco_unload()
{
	LOCK_DEALLOC(tco_lock);
	return(0);
}

/*
 * void
 * tcoinit(void)
 *
 * Calling/Exit State:
 * 	No locking assumptions.
 *
 * Description:
 *	driver init routine
 */
void
tcoinit(void)
{

	/*
	 *	We use "tco_endpt_t *"'s as "TLI connect sequence number"'s.
	 *	This implementation gives the best performance, but it
	 *	causes some problems, because at user-level the invalid
	 *	seq. num. is BADSEQNUM (= -1), while here the invalid
	 *	tco_endpt_t ptr is NULL.
	 *	Assumption: (tco_endpt_t *)BADSEQNUM is an invalid 
	 *		    tco_endpt_t ptr.
	 */

	/*
	 *	initialize default minor and addr
	 */
	tco_defaultendpt.te_min = 0;
	tco_defaultaddr.ta_alen = TCO_DEFAULTADDRSZ;
	tco_defaultaddr.ta_abuf = tco_defaultabuf;
	tco_defaultaddr.ta_ahash = tco_mkahash(&tco_defaultaddr);
	/*
	 * Allocate lock for hash lists and other global variables
	 */
	tco_lock = LOCK_ALLOC(TCO_HIER, plstr, &tco_hshlkinfo, KM_NOSLEEP);
	if (tco_lock == NULL)	 {
	    /*
	     *+ Kernel memory for tco_lock struct could
	     *+ not be allocated. Reconfigure the system
	     *+ to consume less memory.
	     */
	    cmn_err(CE_PANIC, "tcoinit: no memory for tco_lock. Reconfigure");
	}
	return;
}

/*
 * void
 * tco_openwakeup(long addr)
 *	Wake up an lwp that has been sleeping, waiting for memory.
 *
 * Calling/Exit State:
 *	Posts an event for lwp's that are awaiting memory.  No locks held.
 */

STATIC	void
tco_openwakeup(long addr)
{
	EVENT_SIGNAL((event_t *)addr, 0);
}

/*
 * STATIC int
 * tco_open(queue_t *q, dev_t *devp, int oflg, int sflag, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	driver open routine
 */
/* ARGSUSED */
STATIC int
tco_open(queue_t *q, dev_t *devp, int oflg, int sflag, cred_t *crp)
{
	minor_t				min;
	tco_endpt_t			*te;
	int				error;
	pl_t				pl_1;
	struct stroptions		*strop;
	mblk_t				*bp;

	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	/*
	 *	is it already open?
	 */
	if (te != NULL) {
		STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
		    "tco_open _%d_: re-open",__LINE__);
		return(0);
	}
	/*
	 *	get endpt with requested minor number
	 */
	if (sflag == CLONEOPEN) {
		min = NODEV;
	} else {
		min = geteminor(*devp);
		if (min >= tco_cnt) {
			return(ENXIO);
		}
	}
	error = tco_getendpt(TCO_OPEN, min, q, crp, &te);
	if (te == NULL) {
		STRLOG(TCO_ID,-1,3,SL_TRACE,
		    "tco_open _%d_: cannot allocate endpoint, q=%x",__LINE__,q);
		return(error);
	}
	/*
	 *	assign te to queue private pointers
	 */
	q->q_ptr = (caddr_t)te;
	WR(q)->q_ptr = (caddr_t)te;
	te->te_rq = q;
	/*
	 * Send an M_SETOPTS message to stream head to set SO_LOOP flag
	 */
	while ((bp = allocb((int)sizeof (struct stroptions), BPRI_MED)) == NULL) {
		te->te_bid = bufcall(sizeof(struct stroptions), BPRI_MED,
					tco_openwakeup, (long)te->te_event);
		if (te->te_bid == NULL) {
			LOCK_DEALLOC(te->te_lock);
			EVENT_DEALLOC(te->te_event);
			kmem_free(te, sizeof(tco_endpt_t));
			q->q_ptr = NULL;
			WR(q)->q_ptr = NULL;
			return (ENOSPC);
		}
		if (EVENT_WAIT_SIG(te->te_event, primed) == B_FALSE) {
			if (te->te_bid)
				unbufcall(te->te_bid);
			/* Dump the state structure, then unlink it */
			LOCK_DEALLOC(te->te_lock);
			EVENT_DEALLOC(te->te_event);
			kmem_free(te, sizeof(tco_endpt_t));
			q->q_ptr = NULL;
			WR(q)->q_ptr = NULL;
			return (EINTR);
		}
		te->te_bid = 0;
	}
	/* LINTED pointer alignment */
	strop = (struct stroptions *) bp->b_wptr;
	strop->so_flags = SO_LOOP;
	bp->b_wptr += sizeof (struct stroptions);
	bp->b_datap->db_type = M_SETOPTS;
	putnext(q, bp);
	/*
	 *	link to tco_endptopen[] and tco_rqopen[] tables
	 */
	pl_1 = LOCK(tco_lock, plstr);
	te->te_rqhash = tco_mkrqhash(te);
	tco_olink(te);
	UNLOCK(tco_lock, pl_1);
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_open _%d_: endpoint allocated",__LINE__);
	qprocson(q);
	*devp = makedevice(getmajor(*devp), te->te_min);
	return(0);
}


/*
 * STATIC int
 * tco_close(queue_t *q)
 *
 * Calling/Exit State:
 * 	No locking assumptions
 *
 * Description:
 *	driver close routine
 */
/* ARGSUSED */
STATIC int
tco_close(queue_t *q, int flag, cred_t *crp)
{
	tco_endpt_t		*te;
	pl_t			pl_1, pl_2;
	ulong_t 		i;

	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	qprocsoff(q);
	ASSERT(te != NULL);

	pl_1 = LOCK(te->te_lock, plstr);
	if (te->te_bid) {
		unbufcall(te->te_bid);
		te->te_bid = NULL;
	}
	UNLOCK(te->te_lock, pl_1);

	tco_unconnect(te);

retry_close:
	pl_1 = LOCK(tco_lock, plstr);
	pl_2 = LOCK(te->te_lock, plstr);
	if (te->te_flg & TCO_BUSY) {
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		/*
		 * Simple backoff algorithm. Works always.
		 */
		for(i=0;i<100000;i++);
		goto retry_close;
	}
	/*
	 * reuse i to save te_min. so that in debug case STRLOG
	 * can use i to get the minor device number of driver.
	 */
	i = (ulong_t)te->te_min;
	UNLOCK(te->te_lock, pl_2);
	tco_unblink(te);
	tco_unolink(te);
	UNLOCK(tco_lock, pl_1);
	STRLOG(TCO_ID,i,4,SL_TRACE,
	    "tco_close _%d_: endpoint deallocated",__LINE__);
	return(0);
}


/*
 * STATIC int
 * tco_wput(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locking assumptions
 *
 * Description:
 *	driver write side put procedure
 */
STATIC int
tco_wput(queue_t *q, mblk_t *mp)
{
	tco_endpt_t		*te;
	union T_primitives	*prim;
	int			msz;
	pl_t			pl_1;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	switch on streams msg type
	 */
	msz = mp->b_wptr - mp->b_rptr;
	switch (mp->b_datap->db_type) {
	    default:
		STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
		    "tco_wput _%d_: got illegal msg",__LINE__);
		freemsg(mp);
		return(-1);
	    case M_PASSFP:
	    {
		queue_t	*destrq;

		destrq = NULL;

		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_wput _%d_: got M_PASSFP msg",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		/*
		 * Can only pass M_PASSFP if endpt connection is established
		 * ie: connected endpts share lock te->te_lock.
		 */
		if (te->te_con == NULL) {
			UNLOCK(te->te_lock, pl_1);
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_wput _%d_: got M_PASSFP without getting connected",__LINE__);
			return(0);
		}
		destrq = te->te_con->te_rq;
		te->te_con->te_flg |= TCO_BUSY;
		UNLOCK(te->te_lock, pl_1);
		if (destrq != NULL) {
			putnext(destrq, mp);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_con->te_flg &= ~TCO_BUSY;
			UNLOCK(te->te_lock, pl_1);
			return(0);
		}
		return(0);
	    }
	    case M_IOCTL:
		/* no ioctl's supported */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_wput _%d_: got M_IOCTL msg",__LINE__);
		mp->b_datap->db_type = M_IOCNAK;
		qreply(q,mp);
		return(0);
	    case M_FLUSH:
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_wput _%d_: got M_FLUSH msg",__LINE__);
		if (*mp->b_rptr & FLUSHW) {
			flushq(q,FLUSHDATA);
		}
		if (!(*mp->b_rptr & FLUSHR)) {
			freemsg(mp);
		} else {
			flushq(OTHERQ(q),FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply(q,mp);
		}
		return(0);
	    case M_DATA:

		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_wput _%d_: got M_DATA msg",__LINE__);

		/*
		 *	if idle state or endpt hosed, do nothing
		 */
		pl_1 = LOCK(te->te_lock, plstr);
		if (te->te_state == TS_IDLE) {
			UNLOCK(te->te_lock, pl_1);
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_wput _%d_: IDLE",__LINE__);
			freemsg(mp);
			return(-1);
		}
		if (te->te_flg & TCO_ZOMBIE) {
			UNLOCK(te->te_lock, pl_1);
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_wput _%d_: ZOMBIE",__LINE__);
			freemsg(mp);
			return(-1);
		}
		if (NEXTSTATE(TE_DATA_REQ,te->te_state) == TI_BADSTATE) {
			UNLOCK(te->te_lock, pl_1);
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: TE_DATA_REQ out of state, state=%d->127",__LINE__,te->te_state);
			tco_fatal(q,mp);
			return(-1);
		}
		UNLOCK(te->te_lock, pl_1);
		(void)putq(q,mp);
		return(0);
	    case M_PCPROTO:
		/*
		 *	switch on tpi msg type
		 */
		ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim = (union T_primitives *)mp->b_rptr;
		ASSERT(prim != NULL);
		if (msz < sizeof(prim->type)) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: bad control",__LINE__);
			tco_fatal(q,mp);
			return(-1);
		}
		switch (prim->type) {
		    default:
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: bad prim type=%d",__LINE__,prim->type);
			tco_fatal(q,mp);
			return(-1);
		    case T_INFO_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_INFO_REQ msg",__LINE__);
			tco_ireq(q,mp);
			return(0);
		}
		/* NOTREACHED */
	    case M_PROTO:
		/*
		 *	switch on tpi msg type
		 */
		if (msz < sizeof(prim->type)) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: bad control",__LINE__);
			tco_fatal(q,mp);
			return(-1);
		}
		ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim = (union T_primitives *)mp->b_rptr;
		ASSERT(prim != NULL);
		pl_1 = LOCK(te->te_lock, plstr);
		switch (prim->type) {
		    default:
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_wput _%d_ fatal: bad prim type=%d",__LINE__,prim->type);
			UNLOCK(te->te_lock, pl_1);
			tco_fatal(q,mp);
			return(-1);

		    case O_T_BIND_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got O_T_BIND_REQ msg",__LINE__);
			goto jump;
		    case T_BIND_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_BIND_REQ msg",__LINE__);
			goto jump;
		    case T_UNBIND_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_UNBIND_REQ msg",__LINE__);
			goto jump;
		    case T_OPTMGMT_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_OPTMGMT_REQ msg",__LINE__);
			goto jump;
		    case T_CONN_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_CONN_REQ msg",__LINE__);
			goto jump;
		    case T_CONN_RES:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_CONN_RES msg",__LINE__);
			goto jump;
		    case T_DISCON_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_DISCON_REQ msg",__LINE__);
jump:
			/*
			 *	if endpt hosed, do nothing
			 */
			if (te->te_flg & TCO_ZOMBIE) {
				UNLOCK(te->te_lock, pl_1);
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: ZOMBIE",__LINE__);
				freemsg(mp);
				return(-1);
			}
			UNLOCK(te->te_lock, pl_1);
			tco_ckstate(q,mp);
			return(0);
		    case T_ADDR_REQ:
			UNLOCK(te->te_lock, pl_1);
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d%_: got T_ADDR_REQ msg", __LINE__);
			tco_areq(q, mp);
			return(0);
		    case T_DATA_REQ:
			/*
			 *	if idle state or endpt hosed, do nothing
			 */
			if (te->te_state == TS_IDLE) {
				UNLOCK(te->te_lock, pl_1);
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: IDLE",__LINE__);	
				freemsg(mp);
				return(-1);
			}
			if (te->te_flg & TCO_ZOMBIE) {
				UNLOCK(te->te_lock, pl_1);
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: ZOMBIE",__LINE__);
				freemsg(mp);
				return(-1);
			}
			if (NEXTSTATE(TE_DATA_REQ,te->te_state) ==
			    TI_BADSTATE) {
				UNLOCK(te->te_lock, pl_1);
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_wput _%d_ fatal: TE_DATA_REQ out of state, state=%d->127",__LINE__,te->te_state);
				tco_fatal(q,mp);
				return(-1);
			}
			UNLOCK(te->te_lock, pl_1);
			(void)putq(q,mp);
			return(0);
		    case T_EXDATA_REQ:
			/*
			 *	if idle state or endpt hosed, do nothing
			 */
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_EXDATA_REQ msg",__LINE__);
		
			if (te->te_state == TS_IDLE) {
				UNLOCK(te->te_lock, pl_1);
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: IDLE",__LINE__);
				freemsg(mp);
				return(-1);
			}
			if (te->te_flg & TCO_ZOMBIE) {
				UNLOCK(te->te_lock, pl_1);
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wput _%d_: ZOMBIE",__LINE__);
				freemsg(mp);
				return(-1);
			}
			if (NEXTSTATE(TE_EXDATA_REQ,te->te_state) ==
			    TI_BADSTATE) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_wput _%d_ fatal: TE_EXDATA_REQ out of state, state=%d->127",__LINE__,te->te_state);
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(q,mp);
				return(-1);
			}
			UNLOCK(te->te_lock, pl_1);
			(void)putq(q,mp);
			return(0);
#ifdef TICOTSORD
		    case T_ORDREL_REQ:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wput _%d_: got T_ORDREL_REQ msg",__LINE__);
			if (NEXTSTATE(TE_ORDREL_REQ,te->te_state) ==
			    TI_BADSTATE) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_wput _%d_ fatal: TE_ORDREL_REQ out of state, state=%d->127",__LINE__,te->te_state);
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(q,mp);
				return(-1);
			}
			UNLOCK(te->te_lock, pl_1);
			(void)putq(q,mp);
			return(0);
#endif
		}
		/* NOTREACHED */
	}
	/* NOTREACHED */
}
  

/*
 * STATIC int
 * tco_wsrv(queue_t *q)
 *
 * Calling/Exit State:
 *	No locking assumptions
 *
 * Description:
 *	driver write side service routine
 */
STATIC int
tco_wsrv(queue_t *q)
{
	tco_endpt_t			*te;
	mblk_t			*mp;
	union T_primitives	*prim;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	loop through msgs on queue
	 */
	while ((mp = getq(q)) != NULL) {
		/*
		 *	switch on streams msg type
		 */
		switch (mp->b_datap->db_type) {
		    default:
			/*
			 *+ Internal error
			 */
			cmn_err(CE_PANIC, "tco_wsrv: Incorrect msg type");
			/* NOTREACHED */
		    case M_DATA:
			STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
			    "tco_wsrv _%d_: got M_DATA msg",__LINE__);
			if (tco_data(q,mp,TE_DATA_REQ) != 0) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
				    "tco_wsrv _%d_: tco_data() failure",__LINE__);
				return(0);
			}
			break;
		    case M_PROTO:
			/*
			 *	switch on tpi msg type
			 */
			ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
			/* LINTED pointer alignment */
			prim = (union T_primitives *)mp->b_rptr;
			switch (prim->type) {
			    default:
				/*
				 *+ Internal error
				 */
				cmn_err(CE_PANIC, "tco_wsrv: Incorrect prim type");
				/* NOTREACHED */
			    case T_DATA_REQ:
				STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
				    "tco_wsrv _%d_: got T_DATA_REQ msg",__LINE__);
				if (tco_data(q,mp,TE_DATA_REQ) != 0) {
					STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
					    "tco_wsrv _%d_: tco_data() failure",__LINE__);
					return(0);
				}
				break;
			    case T_EXDATA_REQ:
				STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
				    "tco_wsrv _%d_: got T_DATA_REQ msg",__LINE__);
				if (tco_data(q,mp,TE_EXDATA_REQ) != 0) {
					STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
					    "tco_wsrv _%d_: tco_data() failure",__LINE__);
					return(0);
				}
				break;
#ifdef TICOTSORD
			    case T_ORDREL_REQ:
				STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
				    "tco_wsrv _%d_: got T_DATA_REQ msg",__LINE__);
				if (tco_data(q,mp,TE_ORDREL_REQ) != 0) {
					STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
					    "tco_wsrv _%d_: tco_data() failure",__LINE__);
					return(0);
				}
				break;
#endif
			}
			break;
		}
	}
	return(0);
}


/*
 * STATIC int
 * tco_rsrv(queue_t *q)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Descriptions:
 *	driver read side service routine only for back enabling.
 */
STATIC int
tco_rsrv(queue_t *q)
{
	tco_endpt_t		*te;
	pl_t				pl_1;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	enable queue for data transfer
	 */

	pl_1 = LOCK(te->te_lock, plstr);
	if ((te->te_state == TS_DATA_XFER)
#ifdef TICOTSORD
	||  (te->te_state == TS_WIND_ORDREL)
	||  (te->te_state == TS_WREQ_ORDREL)
#endif
	) {
		ASSERT(te->te_con != NULL);
		qenable(WR(te->te_con->te_rq));
	}
	UNLOCK(te->te_lock, pl_1);
	return(0);
}


/*
 * STATIC void
 * tco_okack(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	handle ok ack
 */
STATIC void
tco_okack(queue_t *q, mblk_t *mp)
{
	tco_endpt_t			*te;
	union T_primitives	*prim;
	mblk_t				*mp1;
	long				type;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT(mp != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	/*
	 *	prepare ack msg
	 */
	type = prim->type;
	if ((mp->b_datap->db_lim - mp->b_datap->db_base) < sizeof(struct T_ok_ack)) {
		/* LINTED pointer alignment */
		if ((mp1 = allocb(sizeof(struct T_ok_ack),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_okack _%d_ fatal: allocb() failure",__LINE__);
			tco_fatal(q,mp);
			return;
		}
		freemsg(mp);
		mp = mp1;
	}
	mp->b_datap->db_type = M_PCPROTO;
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_ok_ack);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	prim->ok_ack.PRIM_type = T_OK_ACK;
	prim->ok_ack.CORRECT_prim = type;
	/*
	 *	send ack msg
	 */
	freemsg(unlinkb(mp));
	putnext(RD(q), mp);
	return;
}


/*
 * STATIC void
 * tco_errack(queue_t *q, mblk_t *mp, long tli_err, long unix_err)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	handle error ack
 */
STATIC void
tco_errack(queue_t *q, mblk_t *mp, long tli_err, long unix_err)
{
	tco_endpt_t			*te;
	mblk_t				*mp1;
	long				type;
	union T_primitives	*prim;


	ASSERT(q != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT(mp != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	/*
	 *	prepare nack msg
	 */
	type = prim->type;
	if ((mp->b_datap->db_lim - mp->b_datap->db_base) < sizeof(struct T_error_ack)) {
		if ((mp1 = allocb(sizeof(struct T_error_ack),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_errack _%d_ fatal: allocb() failure",__LINE__);
			tco_fatal(q,mp);
			return;
		}
		freemsg(mp);
		mp = mp1;
	}
	mp->b_datap->db_type = M_PCPROTO;
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_error_ack);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	prim->error_ack.PRIM_type = T_ERROR_ACK;
	prim->error_ack.ERROR_prim = type;
	prim->error_ack.TLI_error = tli_err;
	prim->error_ack.UNIX_error = unix_err;
	/*
	 *	send nack msg
	 */
	freemsg(unlinkb(mp));
	putnext(RD(q), mp);
	return;
}


/*
 * STATIC void
 * tco_fatal(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	the endpt lock (q->q_ptr->te_lock) must not be held on entry.
 *
 * Description:
 *	handle fatal condition (endpt is hosed)
 */
STATIC void
tco_fatal(queue_t *q, mblk_t *mp)
{
	tco_endpt_t		*te;
	pl_t				pl_1;

	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	prepare err msg
	 */
	tco_unconnect(te);
	pl_1 = LOCK(te->te_lock, plstr);
	te->te_flg |= TCO_ZOMBIE;
	UNLOCK(te->te_lock, pl_1);
	mp->b_datap->db_type = M_ERROR;
	ASSERT(mp->b_datap->db_lim - mp->b_datap->db_base >= sizeof(char));
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_rptr + sizeof(char);
	*mp->b_rptr = EPROTO;
	/*
	 *	send err msg
	 */
	freemsg(unlinkb(mp));
	putnext(RD(q), mp);
	return;
}


/*
 * STATIC int
 * tco_flush(queue_t *q, int flushopts)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	flush rd & wr queues
 */
STATIC int
tco_flush(queue_t *q, int flushopts)
{
	mblk_t				*mp;


	ASSERT(q != NULL);
	/*
	 *	prepare flush msg
	 */
	if ((mp = allocb(sizeof(char),BPRI_HI)) == NULL) {
		return(-1);
	}
	mp->b_datap->db_type = M_FLUSH;
	mp->b_wptr = mp->b_rptr + sizeof(char);
	*mp->b_rptr = (char)flushopts;
	/*
	 *	send flush msg
	 */
	putnext(RD(q), mp);
	return(0);
}


/*
 * STATIC void
 * tco_ckstate(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	check interface state and handle event
 */
STATIC void
tco_ckstate(queue_t *q, mblk_t *mp)
{
	union T_primitives	*prim;
	tco_endpt_t		*te;
	char			ns;
	pl_t				pl_1;

	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	/*
	 *	switch on tpi msg type
	 */
	switch (prim->type) {
	    default:
		/*
		 *+ Internal error
		 */
		cmn_err(CE_PANIC, "tco_ckstate: Incorrect prim type");
		/* NOTREACHED */
	    case O_T_BIND_REQ:
	    case T_BIND_REQ:
		pl_1 = LOCK(te->te_lock, plstr);
		if ((ns = NEXTSTATE(TE_BIND_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock, pl_1);
		tco_bind(q,mp);
		return;
	    case T_UNBIND_REQ:
		pl_1 = LOCK(te->te_lock, plstr);	
		if ((ns = NEXTSTATE(TE_UNBIND_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0,state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock, pl_1);
		tco_unbind(q,mp);
		return;
	    case T_OPTMGMT_REQ:
		pl_1 = LOCK(te->te_lock, plstr);
		if ((ns = NEXTSTATE(TE_OPTMGMT_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock, pl_1);
		tco_optmgmt(q,mp);
		return;
	    case T_CONN_REQ:
		pl_1 = LOCK(te->te_lock, plstr);
		if ((ns = NEXTSTATE(TE_CONN_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock, pl_1);
		tco_creq(q,mp);
		return;
	    case T_CONN_RES:
		pl_1 = LOCK(te->te_lock, plstr);
		if ((ns = NEXTSTATE(TE_CONN_RES,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock, pl_1);
		tco_cres(q,mp);
		return;
	    case T_DISCON_REQ:
		pl_1 = LOCK(te->te_lock, plstr);
		if ((ns = NEXTSTATE(TE_DISCON_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock, pl_1);
		tco_dreq(q,mp);
		return;
	}
	/* NOTREACHED */
}


/*
 * STATIC void
 * tco_ireq(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	handle info request
 */
STATIC void
tco_ireq(queue_t *q, mblk_t *mp)
{
	union T_primitives		*prim;
	tco_endpt_t			*te;
	mblk_t				*mp1;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tco_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim != NULL);
	ASSERT(prim->type == T_INFO_REQ);
	/*
	 *	prepare ack msg
	 */
	if ((mp1 = allocb(sizeof(struct T_info_ack),BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_ireq _%d_ fatal: allocb() failure",__LINE__);
		tco_fatal(q,mp);
		return;
	}
	mp1->b_datap->db_type = M_PCPROTO;
	mp1->b_wptr = mp1->b_rptr + sizeof(struct T_info_ack);
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp1->b_rptr;	/* reuse prim */
	prim->info_ack.PRIM_type = T_INFO_ACK;
	prim->info_ack.SERV_type = TCO_SERVTYPE;
	prim->info_ack.ADDR_size = TCO_ADDRSZ;
	prim->info_ack.OPT_size = TCO_OPTSZ;
	prim->info_ack.TIDU_size = TCO_TIDUSZ;
	prim->info_ack.TSDU_size = TCO_TSDUSZ;
	prim->info_ack.ETSDU_size = TCO_ETSDUSZ;
	prim->info_ack.CDATA_size = TCO_CDATASZ;
	prim->info_ack.DDATA_size = TCO_DDATASZ;
	prim->info_ack.CURRENT_state = te->te_state;
	prim->info_ack.PROVIDER_flag = SENDZERO|XPG4_1;
	freemsg(mp);
	/*
	 *	send ack msg
	 */
	qreply(q,mp1);
	return;
}


/*
 * STATIC void
 * tco_bind(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks must held on entry.
 *
 * Description:
 *	handle bind request
 */
STATIC void
tco_bind(queue_t *q, mblk_t *mp)
{
	tco_endpt_t		*te;
	union T_primitives	*prim,*prim2;
	tco_addr_t			addr,*ta;
	mblk_t				*mp1,*mp2;
	struct stroptions		*so;
	unsigned			qlen;
	int				alen,aoff,msz,msz2;
	pl_t				pl_1, pl_2;
	int				unixerr;

	te = (tco_endpt_t *)q->q_ptr;
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(te->te_addr == NULL);
	ASSERT(te->te_fblist == NULL);
	ASSERT(te->te_bblist == NULL);
	/*
	 *	set stream head options
	 */
	if ((mp1 = allocb(sizeof(struct stroptions),BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,ENOMEM);
		return;
	}
	mp1->b_datap->db_type = M_SETOPTS;
	mp1->b_wptr = mp1->b_rptr + sizeof(struct stroptions);
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	so = (struct stroptions *)mp1->b_rptr;
	so->so_flags = SO_MINPSZ|SO_MAXPSZ|SO_HIWAT|SO_LOWAT|SO_WROFF|SO_READOPT;
	so->so_readopt = 0;
	so->so_wroff = 0;
	so->so_minpsz = TCO_MINPSZ;
	so->so_maxpsz = TCO_MAXPSZ;
	so->so_lowat = TCO_LOWAT;
	so->so_hiwat = TCO_HIWAT;
	qreply(q,mp1);
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	if (msz < sizeof(struct T_bind_req)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,EINVAL);
		return;
	}
	alen = prim->bind_req.ADDR_length;
	aoff = prim->bind_req.ADDR_offset;
	if ((alen < 0) || (alen > TCO_ADDRSZ)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TBADADDR, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADADDR,0);
		return;
	}
	if (alen > 0) {
		if ((aoff < 0) || ((aoff + alen) > msz)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TSYSERR,EINVAL);
			return;
		} 
	}
	/*
	 *	negotiate qlen
	 */
	qlen = prim->bind_req.CONIND_number;	/* note: unsigned */
	if (qlen > TCO_MAXQLEN) {
		qlen = TCO_MAXQLEN;
	}
	/*
	 *	negotiate addr
	 */
	unixerr = 0;
	pl_1 = LOCK(tco_lock, plstr);
	if (alen == 0) {
		ta = tco_getaddr(TCO_BIND,NULL,qlen,&unixerr);
	} else {
		addr.ta_alen = alen;
		addr.ta_abuf = (char *)(mp->b_rptr + aoff);
		addr.ta_ahash = tco_mkahash(&addr);
		ta = tco_getaddr(TCO_BIND,&addr,qlen,&unixerr);
	}
	if (ta == NULL) {
	    pl_2 = LOCK(te->te_lock, plstr);
	    te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
	    UNLOCK(te->te_lock, pl_2);
	    ASSERT(te->te_state != TI_BADSTATE);
	    if (unixerr == 0) {
		UNLOCK(tco_lock, pl_1);
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		tco_errack(q,mp,TSYSERR,ENOMEM);
		return;
	    } else {
		/*
		 * binding address to second endpoint with qlen > 0  - (XPG4)
		 */
		UNLOCK(tco_lock, pl_1);
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TADDRBUSY, unix_err=0",__LINE__);
		tco_errack(q,mp,TADDRBUSY,0);
		return;
	    }
	}

	ASSERT(tco_abuf(ta) != NULL);
	ASSERT(tco_alen(ta) != 0);	/* may be != alen */
	/*
	 *	prepare ack message
	 */
	msz2 = sizeof(struct T_bind_ack) + tco_alen(ta);
	if ((mp2 = allocb(msz2,BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		kmem_free(tco_abuf(ta),tco_alen(ta));
		kmem_free(ta,sizeof(tco_addr_t));
		UNLOCK(tco_lock, pl_1);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,ENOMEM);
		return;
	}
	mp2->b_datap->db_type = M_PCPROTO;
	mp2->b_wptr = mp2->b_rptr + msz2;
	ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim2 = (union T_primitives *)mp2->b_rptr;
	prim2->bind_ack.PRIM_type = T_BIND_ACK;
	prim2->bind_ack.CONIND_number = qlen;
	prim2->bind_ack.ADDR_offset = sizeof(struct T_bind_ack);
	prim2->bind_ack.ADDR_length = tco_alen(ta);
	addr.ta_alen = tco_alen(ta);
	addr.ta_abuf = (char *)(mp2->b_rptr + prim2->bind_ack.ADDR_offset);
	(void)tco_cpabuf(&addr,ta);	/* cannot fail */
	/*
	 *	do the bind
	 */
	tco_blink(te,ta);
	UNLOCK(tco_lock, pl_1);
	freemsg(mp);
	pl_1 = LOCK(te->te_lock, plstr);
	te->te_qlen = (unsigned char) qlen;
	te->te_state = NEXTSTATE(TE_BIND_ACK,te->te_state);
	ASSERT(te->te_state != TI_BADSTATE);
	UNLOCK(te->te_lock, pl_1);
	/*
	 *	send ack msg
	 */
	qreply(q,mp2);
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_bind _%d_: bound",__LINE__);
	return;
}


/*
 * STATIC void
 * tco_unbind(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	no locks must be held on entry.
 *
 * Description:
 *	handle unbind request
 */
STATIC void
tco_unbind(queue_t *q, mblk_t *mp)
{
	tco_endpt_t		*te;
	pl_t			pl_1;


	/* LINTED pointer alignment */
	te = (tco_endpt_t *)q->q_ptr;
	/*
	 *	flush queues
	 */
	if (tco_flush(q, FLUSHRW) == -1) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_unbind _%d_ errack: tli_err=TSYSERR, unix_err=EIO",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,EIO);
		return;
	}
	/*
	 *	do the unbind
	 */
	pl_1 = LOCK(tco_lock, plstr);
	tco_unblink(te);
	UNLOCK(tco_lock, pl_1);
	pl_1 = LOCK(te->te_lock, plstr);
	te->te_qlen = 0;
	te->te_state = NEXTSTATE(TE_OK_ACK1,te->te_state);
	ASSERT(te->te_state != TI_BADSTATE);
	UNLOCK(te->te_lock, pl_1);
	/*
	 *	send ack msg
	 */
	tco_okack(q,mp);
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_unbind _%d_: unbound",__LINE__);
	return;
}


/*
 * STATIC void
 * tco_optmgmt(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	handle option mgmt request
 */
STATIC void
tco_optmgmt(queue_t *q, mblk_t *mp)
{
	tco_endpt_t		*te;
	union T_primitives	*prim,*prim1;
	mblk_t				*mp1;
	mblk_t				*nmp;
	int				olen,ooff,msz,msz1,ckopt;
	struct tco_opt_hdr		*ohdr;
	union tco_opt			*opt;
	pl_t				pl_1;


	/* LINTED pointer alignment */
	te = (tco_endpt_t *)q->q_ptr;
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	if (msz < sizeof(struct T_optmgmt_req)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,EINVAL);
		return;
	}
	olen = prim->optmgmt_req.OPT_length;
	ooff = prim->optmgmt_req.OPT_offset;
	/* olen, ooff validated below */
	/*
	 * If another module/driver is using the message block,
	 * create a new one and copy content of old one.
	 */
	if (mp->b_datap->db_ref > 1) {
		if ((nmp = copymsg(mp)) == NULL) {
			STRLOG(TCO_ID, tco_min(te), 2, SL_TRACE,
				"tco_optmgmt _%d_ errack: can't copyb()", __LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TSYSERR,EINVAL);
			return;
		}
		freemsg(mp);
		mp =nmp;
	}
	/*
	 *	switch on optmgmt request type
	 */
	switch (prim->optmgmt_req.MGMT_flags) {
	    default:
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_optmgmt _%d_ errack: tli_err=TBADFLAG, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADFLAG,0);
		return;
	    case T_CHECK:
		/*
		 *	validate opt list
		 */
		if ((olen < 0) || (olen > TCO_OPTSZ)
		||  (ooff < 0) || (ooff + olen > msz)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TSYSERR,EINVAL);
			return;
		}
		ckopt = tco_ckopt((char *)(mp->b_rptr+ooff),
						(char *)(mp->b_rptr+ooff+olen));
		if (ckopt & TCO_BADFORMAT) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TBADOPT,0);
			return;
		}
		/* re-use msg block: */
		mp->b_datap->db_type = M_PCPROTO;
		prim1 = prim;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		if (ckopt & (TCO_REALOPT|TCO_NOOPOPT)) {
			prim1->optmgmt_ack.MGMT_flags = T_SUCCESS;
		}
		if (ckopt & (TCO_BADTYPE|TCO_BADVALUE)) {
			prim1->optmgmt_ack.MGMT_flags = T_FAILURE;
		}
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		qreply(q,mp);
		return;
	    case T_DEFAULT:
		/*
		 *	retrieve default opt
		 */
		msz1 = sizeof(struct T_optmgmt_ack) + sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_noop);
		if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TSYSERR,ENOMEM);
			return;
		}
		mp1->b_datap->db_type = M_PCPROTO;
		mp1->b_wptr = mp1->b_rptr + msz1;
		ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim1 = (union T_primitives *)mp1->b_rptr;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		prim1->optmgmt_ack.MGMT_flags = T_DEFAULT;
		prim1->optmgmt_ack.OPT_length = sizeof(struct tco_opt_hdr) +sizeof(struct tco_opt_noop);
		prim1->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
		ASSERT((prim1->optmgmt_ack.OPT_offset)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		ohdr = (struct tco_opt_hdr *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset);
		ohdr->hdr_thisopt_off = sizeof(struct tco_opt_hdr);
		ohdr->hdr_nexthdr_off = TCO_OPT_NOHDR;
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tco_opt *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset + ohdr->hdr_thisopt_off);
		opt->opt_type = TCO_OPT_NOOP;	/* default opt */
		freemsg(mp);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		qreply(q,mp1);
		return;
	    case T_CURRENT:
		/*
		 *	return current options in effect.
		 *
		 *	Validate options list
		 */
		if (olen == 0) {
			/*
			 *	retrieve default opt
			 */
			msz1 = sizeof(struct T_optmgmt_ack) + sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_noop);
			if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
				STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
				    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, uknix_err=ENOMEM",__LINE__);
				pl_1 = LOCK(te->te_lock, plstr);
				te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
				ASSERT(te->te_state != TI_BADSTATE);
				UNLOCK(te->te_lock, pl_1);
				tco_errack(q,mp,TSYSERR,ENOMEM);
				return;
			}
			mp1->b_datap->db_type = M_PCPROTO;
			mp1->b_wptr = mp1->b_rptr + msz1;
			ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
			/* LINTED pointer alignment */
			prim1 = (union T_primitives *)mp1->b_rptr;
			prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
			prim1->optmgmt_ack.MGMT_flags = T_CURRENT;
			prim1->optmgmt_ack.OPT_length = sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_noop);
			prim1->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
			/* LINTED pointer alignment */
			ohdr = (struct tco_opt_hdr *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset);
			ohdr->hdr_thisopt_off = sizeof(struct tco_opt_hdr);
			ohdr->hdr_nexthdr_off = TCO_OPT_NOHDR;
			/* LINTED pointer alignment */
			opt = (union tco_opt *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset + ohdr->hdr_thisopt_off);
			opt->opt_type = TCO_OPT_NOOP;	/* default opt */
			freemsg(mp);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			qreply(q,mp1);
			return;
		}
		if ((olen < 0) || (olen > TCO_OPTSZ)
		||  (ooff < 0) || (ooff + olen > msz)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TSYSERR,EINVAL);
			return;
		}

		/* re-use msg block: */
		mp->b_datap->db_type = M_PCPROTO;
		prim1 = prim;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		/*
		 *	do the opts
		 */
		/* LINTED pointer alignment */
		for (ohdr = (struct tco_opt_hdr *)(mp->b_rptr + ooff); ; ohdr = (struct tco_opt_hdr *)(mp->b_rptr + ooff + ohdr->hdr_nexthdr_off)) {
			/* LINTED pointer alignment */
			opt = (union tco_opt *)(mp->b_rptr + ooff + ohdr->hdr_thisopt_off);
			switch (opt->opt_type) {
			    default:
				/*
				 *+ Covered all options.
				 */
				goto coveredall;
			    case TCO_OPT_NOOP:
				break;
			    case TCO_OPT_GETID:
				pl_1 = LOCK(te->te_lock, plstr);
				opt->opt_getid.getid_flg = te->te_idflg;
				UNLOCK(te->te_lock, pl_1);
				break;
			    case TCO_OPT_UID:
				opt->opt_uid.uid_val = te->te_uid;
				break;
			    case TCO_OPT_GID:
				opt->opt_gid.gid_val = te->te_gid;
				break;
			    case TCO_OPT_RUID:
				opt->opt_ruid.ruid_val = te->te_ruid;
				break;
			    case TCO_OPT_RGID:
				opt->opt_rgid.rgid_val = te->te_rgid;
				break;
			}
			if (ohdr->hdr_nexthdr_off == TCO_OPT_NOHDR) {
				break;
			}
		}
coveredall:
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		qreply(q,mp);
		return;
	    case T_NEGOTIATE:
		/*
		 *	negotiate opt
		 */
		if (olen == 0) {
			/*
			 *	retrieve default opt
			 */
			msz1 = sizeof(struct T_optmgmt_ack) + sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_noop);
			if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
				STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
				    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, uknix_err=ENOMEM",__LINE__);
				pl_1 = LOCK(te->te_lock, plstr);
				te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
				ASSERT(te->te_state != TI_BADSTATE);
				UNLOCK(te->te_lock, pl_1);
				tco_errack(q,mp,TSYSERR,ENOMEM);
				return;
			}
			mp1->b_datap->db_type = M_PCPROTO;
			mp1->b_wptr = mp1->b_rptr + msz1;
			ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
			/* LINTED pointer alignment */
			prim1 = (union T_primitives *)mp1->b_rptr;
			prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
			prim1->optmgmt_ack.MGMT_flags = T_NEGOTIATE;
			prim1->optmgmt_ack.OPT_length = sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_noop);
			prim1->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
			/* LINTED pointer alignment */
			ohdr = (struct tco_opt_hdr *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset);
			ohdr->hdr_thisopt_off = sizeof(struct tco_opt_hdr);
			ohdr->hdr_nexthdr_off = TCO_OPT_NOHDR;
			/* LINTED pointer alignment */
			opt = (union tco_opt *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset + ohdr->hdr_thisopt_off);
			opt->opt_type = TCO_OPT_NOOP;	/* default opt */
			freemsg(mp);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			qreply(q,mp1);
			return;
		}
		/*
		 *	validate opt list
		 */
		if ((olen < 0) || (olen > TCO_OPTSZ)
		||  (ooff < 0) || (ooff + olen > msz)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TSYSERR,EINVAL);
			return;
		}
		ckopt = tco_ckopt((char *)(mp->b_rptr+ooff),
						(char *)(mp->b_rptr+ooff+olen));
		if (ckopt & (TCO_BADFORMAT|TCO_BADTYPE|TCO_BADVALUE)) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_optmgmt _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tco_errack(q,mp,TBADOPT,0);
			return;
		}
		/* re-use msg block: */
		mp->b_datap->db_type = M_PCPROTO;
		prim1 = prim;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		/*
		 *	do the opts
		 */
		/* LINTED pointer alignment */
		for (ohdr = (struct tco_opt_hdr *)(mp->b_rptr + ooff); ; ohdr = (struct tco_opt_hdr *)(mp->b_rptr + ooff + ohdr->hdr_nexthdr_off)) {
			/* LINTED pointer alignment */
			opt = (union tco_opt *)(mp->b_rptr + ooff + ohdr->hdr_thisopt_off);
			switch (opt->opt_type) {
			    default:
				/*
				 *+ Internal error
				 */
				cmn_err(CE_PANIC, "tco_optmgmt: Incorrect options type");
				/* NOTREACHED */
			    case TCO_OPT_NOOP:
				break;
			    case TCO_OPT_SETID:
				ASSERT((opt->opt_setid.setid_flg & ~TCO_IDFLG_ALL) == 0);
				pl_1 = LOCK(te->te_lock, plstr);
				te->te_idflg = opt->opt_setid.setid_flg;
				UNLOCK(te->te_lock, pl_1);
				break;
			    case TCO_OPT_GETID:
				pl_1 = LOCK(te->te_lock, plstr);
				opt->opt_getid.getid_flg = te->te_idflg;
				UNLOCK(te->te_lock, pl_1);
				break;
			    case TCO_OPT_UID:
				opt->opt_uid.uid_val = te->te_uid;
				break;
			    case TCO_OPT_GID:
				opt->opt_gid.gid_val = te->te_gid;
				break;
			    case TCO_OPT_RUID:
				opt->opt_ruid.ruid_val = te->te_ruid;
				break;
			    case TCO_OPT_RGID:
				opt->opt_rgid.rgid_val = te->te_rgid;
				break;
			}
			if (ohdr->hdr_nexthdr_off == TCO_OPT_NOHDR) {
				break;
			}
		}
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		qreply(q,mp);
		return;
	}
	/* NOTREACHED */
}


/*
 * STATIC void
 * tco_creq(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held on entry.
 *
 * Description:
 *	handle connect request
 */
STATIC void
tco_creq(queue_t *q, mblk_t *mp)
{
	tco_endpt_t		*te,*te2,**tep,**etep;
	union T_primitives	*prim,*prim2;
	tco_addr_t			addr,*ta;
	mblk_t				*mp1,*mp2;
	int				alen,aoff,olen,olen2;
	int				ooff,msz,msz2,err;
	long				idflg2;
	queue_t				*rq;
	pl_t				pl_1, pl_2, pl_3;
	int				unixerr;

	te = (tco_endpt_t *)q->q_ptr;
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	alen = prim->conn_req.DEST_length;
	aoff = prim->conn_req.DEST_offset;
	olen = prim->conn_req.OPT_length;
	ooff = prim->conn_req.OPT_offset;
	if ((msz < sizeof(struct T_conn_req))
	||  ((alen > 0) && ((aoff + alen) > msz))
	||  ((olen > 0) && ((ooff + olen) > msz))) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,EINVAL);
		return;
	}
	if (msgdsize(mp) > TCO_CDATASZ) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TBADATA, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADDATA,0);
		return;
	}
	if ((alen <= 0) || (alen > TCO_ADDRSZ) || (aoff < 0)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TBADADDR, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADADDR,0);
		return;
	}
	if (olen != 0) {
		/*
		 *	no opts supported here
		 */
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADOPT,0);
		return;
	}

	/*
	 *	ack validity of request
	 */
	if ((mp1 = copymsg(mp)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_creq _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,ENOMEM);
		return;
	}

	/*
	 *	get endpt to connect to
	 */
	unixerr = 0;
	pl_1 = LOCK(tco_lock, plstr);
	addr.ta_alen = alen;
	addr.ta_abuf = (char *)(mp->b_rptr + aoff);
	addr.ta_ahash = tco_mkahash(&addr);
	ta = tco_getaddr(TCO_CONN,&addr,0,&unixerr);
	err = 0;

	if (ta == NULL) {
		STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
		    "tco_creq _%d_: cannot connect, err=NOPEER",__LINE__);
		err = TCO_NOPEER;
	} else {
		te2 = ta->ta_hblist;	/* te = client; te2 = server */
		ASSERT(te2 != NULL);
		pl_2 = LOCK(te2->te_lock, plstr);
		if (te2->te_nicon >= te2->te_qlen) {
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_creq _%d_: cannot connect, err=PEERNOROOMONQ",__LINE__);
			err = TCO_PEERNOROOMONQ;
		} else if (te2->te_con != NULL) {
			/*
			 * This provider does not support multiple
			 * connections to same remote address. - (XPG4)
			 */
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_creq _%d_ errack: tli_err=TADDRBUSY, unix_err=0",__LINE__);
			pl_3 = LOCK_SH(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_3);
			UNLOCK(te2->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			freemsg(mp1);
			tco_errack(q,mp,TADDRBUSY,0);
			return;
		} else if (!((te2->te_state == TS_IDLE) || (te2->te_state == TS_WRES_CIND))) {
			/*
			 * te2 endpt is not in TS_IDLE state or is not waiting
			 * for response of connection indication
			 */
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_creq _%d_: cannot connect, err=PEERBADSTATE, state=%d",__LINE__,te2->te_state);
			err = TCO_PEERBADSTATE;
		}
		UNLOCK(te2->te_lock, pl_2);
	}

	pl_2 = LOCK(te->te_lock, plstr);
	te->te_state = NEXTSTATE(TE_OK_ACK1,te->te_state);
	ASSERT(te->te_state != TI_BADSTATE);
	UNLOCK(te->te_lock, pl_2);

	if (err != 0) {
		UNLOCK(tco_lock, pl_1);
		tco_okack(q,mp1);
		if ((mp2 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_creq _%d_ fatal: allocb() failure",__LINE__);
			tco_fatal(q,mp);
			return;
		}
		mp2->b_datap->db_type = M_PROTO;
		mp2->b_wptr = mp2->b_rptr + sizeof(struct T_discon_ind);
		ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim2 = (union T_primitives *)mp2->b_rptr;
		prim2->type = T_DISCON_IND;
		prim2->discon_ind.SEQ_number = BADSEQNUM;
		prim2->discon_ind.DISCON_reason = err;
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_DISCON_IND1,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		freemsg(mp);
		qreply(q,mp2);
		return;
	}
	/*
	 *	prepare indication msg (with tco_lock held).
	 */
	pl_2 = LOCK(te2->te_lock, plstr);
	msz2 = sizeof(struct T_conn_ind) + tco_alen(te->te_addr);
	idflg2 = te2->te_idflg;
	UNLOCK(te2->te_lock, pl_2);
	ASSERT((idflg2 & ~TCO_IDFLG_ALL) == 0);
	if (idflg2 != 0) {
		olen2 = 0;
		if (idflg2 & TCO_IDFLG_UID) {
			olen2 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_uid);
		}
		if (idflg2 & TCO_IDFLG_GID) {
			olen2 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_gid);
		}
		if (idflg2 & TCO_IDFLG_RUID) {
			olen2 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_ruid);
		}
		if (idflg2 & TCO_IDFLG_RGID) {
			olen2 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_rgid);
		}
		msz2 += olen2;
	}
	if (msz2 > TCO_TIDUSZ) {
		UNLOCK(tco_lock, pl_1);
		tco_okack(q, mp1);
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_creq _%d_ fatal: msg too big",__LINE__);
		tco_fatal(q,mp);
		return;
	}
	if ((mp2 = allocb(msz2,BPRI_HI)) == NULL) {
		UNLOCK(tco_lock, pl_1);
		tco_okack(q, mp1);
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_creq _%d_ fatal: allocb() failure",__LINE__);
		tco_fatal(q,mp);
		return;
	}
	mp2->b_datap->db_type = M_PROTO;
	mp2->b_wptr = mp2->b_rptr + msz2;
	ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim2 = (union T_primitives *)mp2->b_rptr;
	prim2->type = T_CONN_IND;
	prim2->conn_ind.SRC_offset = sizeof(struct T_conn_ind);
	prim2->conn_ind.SRC_length = tco_alen(te->te_addr);
	ASSERT((long)te != BADSEQNUM);
	ASSERT(te != NULL);
	prim2->conn_ind.SEQ_number = (long)te;
	addr.ta_alen = prim2->conn_ind.SRC_length;
	addr.ta_abuf = (char *)(mp2->b_rptr + prim2->conn_ind.SRC_offset);
	(void)tco_cpabuf(&addr,te->te_addr);	/* cannot fail */
	/*
	 * At this point it is safe to switch te->te_lock with
	 * (server's) te2->te_lock and reassign original
	 * te->te_lock to te->te_savelock.
	 */
	pl_2 = LOCK(te->te_lock, plstr);
	te->te_flg |= TCO_CLIENT;
	te->te_savelock = te->te_lock;
	te->te_lock = te2->te_lock;
	UNLOCK(te->te_savelock, pl_2);
	/*
	 * te_lock is shared between client and server (although server
	 * actually "owns" it).
	 */
	pl_2 = LOCK(te->te_lock, plstr);
	if (idflg2 == 0) {
		prim2->conn_ind.OPT_offset = 0;
		prim2->conn_ind.OPT_length = 0;
	} else {
		prim2->conn_ind.OPT_offset = prim2->conn_ind.SRC_offset + prim2->conn_ind.SRC_length;
		while ((prim2->conn_ind.OPT_offset)%NBPW != 0) {
			prim2->conn_ind.OPT_offset += 1;	/* alignment */
		}
		prim2->conn_ind.OPT_length = olen2;
		tco_wropt(idflg2,te,
			(char *)(mp2->b_rptr+prim2->conn_ind.OPT_offset));
		ASSERT((tco_ckopt((char *)(mp2->b_rptr+prim2->conn_ind.OPT_offset),
		    (char *)(mp2->b_rptr+prim2->conn_ind.OPT_offset+prim2->conn_ind.OPT_length))
		    & (TCO_BADFORMAT|TCO_BADTYPE|TCO_BADVALUE)) == 0);
	}
	/*
	 *	register the connection request
	 */
	for (tep = &te2->te_icon[0], etep = &te2->te_icon[TCO_MAXQLEN]; 
							tep < etep; tep += 1) {
		if (*tep == NULL) {
			*tep = te;
			break;
		}
	}
	ASSERT(tep < etep);
	te2->te_nicon += 1;
	te->te_ocon = te2;
	/*
	 *	relink data blocks from mp to mp2
	 */
	/* following is faster than (void)linkb(mp2,unlinkb(mp)); */
	mp2->b_cont = mp->b_cont;
	mp->b_cont = NULL;
	/*
	 *	send ok ack on self and
	 *	indication msg on server.
	 */
	te2->te_state = NEXTSTATE(TE_CONN_IND,te2->te_state);
	ASSERT(te2->te_state != TI_BADSTATE);
	rq = te2->te_rq;
	te2->te_flg |= TCO_BUSY;
	UNLOCK(te->te_lock, pl_2);
	UNLOCK(tco_lock, pl_1);
	tco_okack(q, mp1);
	putnext(rq,mp2);
	pl_1 = LOCK(te->te_lock, plstr);
	te2->te_flg &= ~TCO_BUSY;
	UNLOCK(te->te_lock, pl_1);
	freeb(mp);
	return;
}


/*
 * STATIC void
 * tco_cres(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held on entry.
 *
 * Description:
 *	handle connect response
 */
STATIC void
tco_cres(queue_t *q, mblk_t *mp)
{
	tco_endpt_t			*te,*te3,**tep,**etep;
	tco_endpt_t			*te1;
	union T_primitives		*prim,*prim3;
	mblk_t				*mp2,*mp3;
	tco_addr_t			addr3;
	int				olen,olen3,ooff,msz,msz3;
	long				idflg3;
	queue_t				*rq;
	tco_addr_t			*addr;
	pl_t				pl_1, pl_2, pl_3;


	te = (tco_endpt_t *)q->q_ptr;
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	olen = prim->conn_res.OPT_length;
	ooff = prim->conn_res.OPT_offset;
	if ((msz < sizeof(struct T_conn_res))
	||  ((olen > 0) && ((ooff + olen) > msz))) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,EINVAL);
		return;
	}
	if (msgdsize(mp) > TCO_CDATASZ) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADDATA, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADDATA,0);
		return;
	}
	if (olen < 0) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADOPT,0);
		return;
	}
	if (olen > 0) {
		/*
		 *	no opts supported here
		 */
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		  "tco_cres _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADOPT,0);
		return;
	}
	/*
	 *	get accepting endpt
	 */
	(void)tco_getendpt(TCO_RQ,0,prim->conn_res.QUEUE_ptr, 
						(cred_t *)NULL, &te1);
	/* te, te1 = server: te = listening endpt, te1 = accepting endpt */
	/*
	 *	if endpt doesn't exist, or is hosed, or not idle, send nack
	 */
	pl_1 = LOCK(tco_lock, plstr);
	pl_2 = LOCK(te->te_lock,  plstr);
	if ((te1 == NULL) || (te1->te_flg & TCO_ZOMBIE)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADF, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_errack(q,mp,TBADF,0);
		return;
	}
	if (te != te1) {
		pl_3 = LOCK_SH(te1->te_lock,  plstr);
		if (te1->te_state != TS_IDLE) {
			/* attempt to bind to te's addr - (XPG4) */
			tco_blink(te1, te->te_addr);
			te1->te_state = NEXTSTATE(TE_BIND_REQ,te1->te_state);
			ASSERT(te1->te_state != TI_BADSTATE);
			te1->te_state = NEXTSTATE(TE_BIND_ACK,te1->te_state);
			ASSERT(te1->te_state != TI_BADSTATE);
		}
		UNLOCK(te1->te_lock, pl_3);
	}
	/*
	 *	 get endpt to which connect will be made
	 *
	 *	 Must verify if SEQ_number is valid before
	 *	 dereferencing it. It could be bogus!!
	 *
	 * 	 te3 and te must share te_lock from an earlier
	 * 	 successful connect request
	 */
	te3 = (tco_endpt_t *)(prim->conn_res.SEQ_number);    /* te3 = client */
	if (te3 == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		  "tco_cres _%d_ errack: tli_err=TBADSEQ, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_errack(q,mp,TBADSEQ,0);
		return;
	}

	for (tep = &te->te_icon[0], etep = &te->te_icon[TCO_MAXQLEN];
							tep < etep; tep += 1) {
		if (*tep == te3) {
			*tep = NULL;
			break;
		}
	}
	if (tep >= etep) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TBADSEQ, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_errack(q,mp,TBADSEQ,0);
		return;
	}
	ASSERT(te->te_nicon >= 1);
	if ((te == te1) && (te->te_nicon > 1)) {
		/*
		 * 	Cannot allow T_CONN_RES if more than 1 pending
		 * 	incoming requests.
		 */
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TINDOUT, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_errack(q,mp,TINDOUT,0);
		return;
	}

	/*
	 * fd != resfd and
	 * resfd was bound with with a qlen > 0  - (XPG4)
	 */
	if ((te != te1) && (te1->te_qlen)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TRESQLEN, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_errack(q,mp,TRESQLEN,0);
		return;
	}

	/*
	 *	ack validity of request
	 */
	if ((mp2 = copymsg(mp)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_cres _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_errack(q,mp,TSYSERR,ENOMEM);
		return;
	}
	if (te != te1) {
		if (te->te_nicon != 1)
			te->te_state = NEXTSTATE(TE_OK_ACK4,te->te_state);
		else
			te->te_state = NEXTSTATE(TE_OK_ACK3,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		pl_3 = LOCK_SH(te1->te_lock, plstr);
		te1->te_state = NEXTSTATE(TE_PASS_CONN,te1->te_state);
		ASSERT(te1->te_state != TI_BADSTATE);
		rq = te1->te_rq;
		/*
		 * set the busy flag on te1 so that it will not go away
		 */
		te1->te_flg |= TCO_BUSY;
		addr = te1->te_addr;
		UNLOCK(te1->te_lock, pl_3);
	} else {
		/*
		 * Already established above that if te == te1 then
		 * te->te_nicon == 1
		 */
		te->te_state = NEXTSTATE(TE_OK_ACK2,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		rq = te1->te_rq;
		addr = te1->te_addr;
	}
	te->te_nicon -= 1;
	/*
	 *	validate state
	 */
	if (te3->te_state != TS_WCON_CREQ) {
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_okack(q, mp2);
		if ((tco_flush(rq, FLUSHR) == -1)
		||  ((mp3 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL)) {
			STRLOG(TCO_ID,tco_min((tco_endpt_t *)rq->q_ptr),1,SL_TRACE,
			    "tco_cres _%d_ fatal: allocb() failure",__LINE__);
			tco_fatal(rq,mp);
			if (te1 != te) {
				/* 
				 * Clear the busy flag on te1
				 */
				pl_1 = LOCK(tco_lock, plstr);
				pl_2 = LOCK(te1->te_lock, plstr);
				te1->te_flg &= ~TCO_BUSY;
				UNLOCK(te1->te_lock, pl_2);
				UNLOCK(tco_lock, pl_1);
			}
			return;
		}
		mp3->b_datap->db_type = M_PROTO;
		mp3->b_wptr = mp3->b_rptr + sizeof(struct T_discon_ind);
		ASSERT((int)(mp3->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim3 = (union T_primitives *)mp3->b_rptr;
		prim3->type = T_DISCON_IND;
		prim3->discon_ind.SEQ_number = BADSEQNUM;
		prim3->discon_ind.DISCON_reason = TCO_PEERBADSTATE;
		if (te != te1)
			pl_1 = LOCK(tco_lock, plstr);
		pl_2 = LOCK(te1->te_lock, plstr);
		te1->te_state = NEXTSTATE(TE_DISCON_IND1,te1->te_state);
		ASSERT(te1->te_state != TI_BADSTATE);
		UNLOCK(te1->te_lock, pl_2);
		if (te != te1)
			UNLOCK(tco_lock, pl_1);
		freemsg(mp);
		putnext(rq,mp3);
		if (te != te1) {
			pl_1 = LOCK(tco_lock, plstr);
			pl_2 = LOCK(te1->te_lock, plstr);
			te1->te_flg &= ~TCO_BUSY;
			UNLOCK(te1->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
		}
		return;
	}
	/*
	 *	prepare confirmation msg
	 */
	msz3 = sizeof(struct T_conn_con) + tco_alen(addr);
	idflg3 = te3->te_idflg;
	ASSERT((idflg3 & ~TCO_IDFLG_ALL) == 0);
	if (idflg3 != 0) {
		olen3 = 0;
		if (idflg3 & TCO_IDFLG_UID) {
			olen3 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_uid);
		}
		if (idflg3 & TCO_IDFLG_GID) {
			olen3 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_gid);
		}
		if (idflg3 & TCO_IDFLG_RUID) {
			olen3 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_ruid);
		}
		if (idflg3 & TCO_IDFLG_RGID) {
			olen3 += sizeof(struct tco_opt_hdr) + sizeof(struct tco_opt_rgid);
		}
		msz3 += olen3 + NBPW;	/* allow for alignment */
	}
	if (msz3 >= TCO_TIDUSZ) {
		UNLOCK(te->te_lock, pl_2);
		if (te1 != te) {
			pl_2 = LOCK(te1->te_lock, plstr);
			te1->te_flg &= ~TCO_BUSY;
			UNLOCK(te1->te_lock, pl_2);
		}
		UNLOCK(tco_lock, pl_1);
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_cres _%d_ fatal: msg too big",__LINE__);
		tco_okack(q, mp2);
		tco_fatal(q,mp);
		return;
	}
	if ((mp3 = allocb(msz3,BPRI_HI)) == NULL) {
		UNLOCK(te->te_lock, pl_2);
		if (te1 != te) {
			pl_2 = LOCK(te1->te_lock, plstr);
			te1->te_flg &= ~TCO_BUSY;
			UNLOCK(te1->te_lock, pl_2);
		}
		UNLOCK(tco_lock, pl_1);
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_cres _%d_ fatal: allocb() failure",__LINE__);
		tco_okack(q, mp2);
		tco_fatal(q,mp);
		return;
	}
	mp3->b_datap->db_type = M_PROTO;
	mp3->b_wptr = mp3->b_rptr + msz3;
	ASSERT((int)(mp3->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim3 = (union T_primitives *)mp3->b_rptr;
	prim3->type = T_CONN_CON;
	prim3->conn_con.RES_offset = sizeof(struct T_conn_con);
	prim3->conn_con.RES_length = tco_alen(addr);
	addr3.ta_alen = prim3->conn_con.RES_length;
	addr3.ta_abuf = (char *)(mp3->b_rptr + prim3->conn_con.RES_offset);
	ASSERT(addr != NULL);
	(void)tco_cpabuf(&addr3,addr);	/* cannot fail */
	ASSERT(olen >= 0);
	if (idflg3 == 0) {
		prim3->conn_con.OPT_offset = 0;
		prim3->conn_con.OPT_length = 0;
	} else {
		prim3->conn_con.OPT_offset = prim3->conn_con.RES_offset + prim3->conn_con.RES_length;
		while ((prim3->conn_con.OPT_offset)%NBPW != 0) {
			prim3->conn_con.OPT_offset += 1;	/* alignment */
		}
		prim3->conn_con.OPT_length = olen3;
		tco_wropt(idflg3,te1,
			(char *)(mp3->b_rptr+prim3->conn_con.OPT_offset));
		ASSERT((tco_ckopt((char *)(mp3->b_rptr+prim3->conn_con.OPT_offset),
		    (char *)(mp3->b_rptr+prim3->conn_con.OPT_offset+prim3->conn_con.OPT_length))
		    & (TCO_BADFORMAT|TCO_BADTYPE|TCO_BADVALUE)) == 0);
	}
	/*
	 *	make the connection. 
	 *
	 *	Note that te3 is a client and
	 *	was using (server's) te->te_lock in te3->te_lock. 
	 *	So te3->te_lock	does not have to be saved in
	 *	te3->te_savelock.
	 */
	if (te != te1) {
		pl_3 = LOCK_SH(te1->te_lock, plstr);
		te1->te_flg &= ~TCO_BUSY;
		te3->te_lock = te1->te_lock;
	}

	te3->te_con = te1;
	te1->te_con = te3;
	te1->te_ocon = NULL;
	te3->te_ocon = NULL;
	STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
	    "tco_cres _%d_: connected",__LINE__);
	/*
	 *	relink data blocks from mp to mp3
	 */
	/* following is faster than (void)linkb(mp3,unlinkb(mp)); */
	mp3->b_cont = mp->b_cont;
	mp->b_cont = NULL;

	/*
	 *	send confirmation msg
	 */
	te3->te_state = NEXTSTATE(TE_CONN_CON,te3->te_state);
	ASSERT(te3->te_state != TI_BADSTATE);
	rq = te3->te_rq;
	te3->te_flg |= TCO_BUSY;
	if (te != te1)
		UNLOCK(te1->te_lock, pl_3);
	UNLOCK(te->te_lock, pl_2);
	UNLOCK(tco_lock, pl_1);
	tco_okack(q,mp2);
	putnext(rq,mp3);
	if (te1 != te) {
		/*
		 * te1 & te3 share a lock and the TCO_BUSY on te3
		 * will prevent it from going away.
		 */
		pl_2 = LOCK(te3->te_lock, plstr);
		te3->te_flg &= ~TCO_BUSY;
		UNLOCK(te3->te_lock, pl_2);
	} else {
		/*
		 * te & te3 share a lock and the TCO_BUSY on te3
		 * will prevent it from going away.
		 */
		pl_1 = LOCK(te->te_lock, plstr);
		te3->te_flg &= ~TCO_BUSY;
		UNLOCK(te->te_lock, pl_1);
	}
	freeb(mp);
	return;
}


/*
 * STATIC void
 * tco_dreq(queue_t *q, mblk_t *mp))
 *
 * Calling/Exit State:
 *	No locks held on entry.
 *
 * Description:
 *	handle disconnect request
 */
STATIC void
tco_dreq(queue_t *q, mblk_t *mp)
{
	union T_primitives	*prim,*prim2;
	tco_endpt_t		*te,*te2,**tep,**etep;
	mblk_t				*mp1,*mp2;
	int				i, msz;
	queue_t				*rq;
	pl_t				pl_1, pl_2, pl_3;


	te = (tco_endpt_t *)q->q_ptr;		/* endpt being disconnected */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	if (msz < sizeof(struct T_discon_req)) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_dreq _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TSYSERR,EINVAL);
		return;
	}
	if (msgdsize(mp) > TCO_DDATASZ) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_dreq _%d_ errack: tli_err=TBADDATA, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tco_errack(q,mp,TBADDATA,0);
		return;
	}
retry_dreq:
	pl_1 = LOCK(tco_lock, plstr);
	pl_2 = LOCK(te->te_lock, plstr);
	if (te->te_flg & TCO_BUSY) {
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		for(i=0;i<100000;i++);
		goto retry_dreq;
	}
	ASSERT(te->te_nicon >= 0);
	if (te->te_nicon > 0) {
		ASSERT((te->te_flg & TCO_CLIENT) == 0);
		/*
		 * 	Server Endpt requesting disconnect of existing
		 *	pending incoming request to it.
		 *
		 *	validate sequence number
		 */
		if (prim->discon_req.SEQ_number == (long)NULL) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_dreq _%d_ errack: tli_err=TBADSEQ, unix_err=0",__LINE__);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			tco_errack(q,mp,TBADSEQ,0);
			return;
		}
		te2 = (tco_endpt_t *)(prim->discon_req.SEQ_number);
		for (i = 0; i < TCO_MAXQLEN; i++) {
			if (te->te_icon[i] == te2) {
				te->te_icon[i] = NULL;
				/*
				 *	te = server; te2 = client
				 */
				goto foundendpt;
			}
		}
		/*
		 * Error. sequence number not in pending incoming
		 * connect requests. This implies the sequence number
		 * is invalid
		 */
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		  "tco_dreq _%d_ errack: tli_err=TBADSEQ, unix_err=0",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_errack(q,mp,TBADSEQ,0);
		return;
	}
foundendpt:

	/* 
	 *	ack validity of request
	 */
	if ((mp1 = copymsg(mp)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
		    "tco_dreq _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		tco_errack(q,mp,TSYSERR,ENOMEM);
		return;
	}
	if (te->te_nicon == 0)
		te->te_state = NEXTSTATE(TE_OK_ACK1,te->te_state);
	else if (te->te_nicon == 1)
		te->te_state = NEXTSTATE(TE_OK_ACK2,te->te_state);
	else
		te->te_state = NEXTSTATE(TE_OK_ACK4,te->te_state);

	ASSERT(te->te_state != TI_BADSTATE);
	if (te->te_nicon <= 1) {
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		if (tco_flush(q, FLUSHRW) == -1) {
			STRLOG(TCO_ID,tco_min(te),2,SL_TRACE,
			    "tco_dreq _%d_ errack: tli_err=TSYSERR, unix_err=EIO",__LINE__);
			pl_2 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_2);
			tco_errack(q,mp,TSYSERR,EIO);
			return;
		}
		pl_1 = LOCK(tco_lock, plstr);
		pl_2 = LOCK(te->te_lock, plstr);
	}
	/*
	 *	do the work.
	 *
	 * 	Note: te->te_lock is common to both te & te2 and is
	 *	      held at this point
	 */
	if (te->te_nicon > 0) {
		/*
		 *	disconnect an incoming connect request pending to te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_dreq _%d_: disconnect incoming",__LINE__);
		ASSERT(te->te_con == NULL);
		te->te_nicon -= 1;
		ASSERT(te2->te_flg & TCO_CLIENT);
		ASSERT(te2->te_ocon == te);
		rq = te2->te_rq;
		te2->te_flg |= TCO_BUSY;
		if ((mp2 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te2),1,SL_TRACE,
			    "tco_dreq _%d_ fatal: allocb() failure",__LINE__);
			UNLOCK(te->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			tco_okack(q, mp1);
			tco_fatal(rq,mp);
			pl_1 = LOCK(tco_lock, plstr);
			pl_2 = LOCK(te->te_lock, plstr);
			te2->te_flg &= ~TCO_BUSY;
			UNLOCK(te->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			return;
		}
		te2->te_flg &= ~TCO_CLIENT;
		pl_3 = LOCK_SH(te2->te_savelock, plstr);
		te2->te_lock = te2->te_savelock;
		te2->te_savelock = NULL;
		ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim2 = (union T_primitives *)mp2->b_rptr;
		ASSERT(prim2 != NULL);
		prim2->discon_ind.SEQ_number = BADSEQNUM;
		te2->te_state = NEXTSTATE(TE_DISCON_IND1,te2->te_state);
		ASSERT(te2->te_state != TI_BADSTATE);
		te2->te_ocon = NULL;
		UNLOCK(te2->te_lock, pl_3);
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock,pl_1);
	} else if (te->te_ocon != NULL) {
		/*
		 *	disconnect an outgoing connect request pending from te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_dreq _%d_: disconnect outgoing",__LINE__);
		ASSERT(te->te_nicon == 0);
		ASSERT(te->te_con == NULL);
		ASSERT(te->te_flg & TCO_CLIENT);
		te2 = te->te_ocon;	/* te = client; te2 = server */
		rq = te2->te_rq;
		te2->te_flg |= TCO_BUSY;
		if ((mp2 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
			STRLOG(TCO_ID,tco_min(te->te_ocon),1,SL_TRACE,
			    "tco_dreq _%d_ fatal: allocb() failure",__LINE__);
			UNLOCK(te->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			tco_okack(q, mp1);
			tco_fatal(rq,mp);
			pl_1 = LOCK(tco_lock, plstr);
			pl_2 = LOCK(te->te_lock, plstr);
			te2->te_flg &= ~TCO_BUSY;
			UNLOCK(te->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			return;
		}
		for (tep = &te2->te_icon[0], etep = &te2->te_icon[TCO_MAXQLEN];
							tep < etep; tep += 1) {
			if (*tep == te) {
				*tep = NULL;
				ASSERT(te2->te_nicon >= 1);
				if (te2->te_nicon == 1) {
					te2->te_state = NEXTSTATE(TE_DISCON_IND2,te2->te_state);
					ASSERT(te2->te_state != TI_BADSTATE);
				} else {
					te2->te_state = NEXTSTATE(TE_DISCON_IND3,te2->te_state);
					ASSERT(te2->te_state != TI_BADSTATE);
				}
				te2->te_nicon -= 1;
				break;
			}
		}
		te->te_flg &= ~TCO_CLIENT;
		pl_3 = LOCK_SH(te->te_savelock, plstr);
		te->te_lock = te->te_savelock;
		te->te_savelock = NULL;
		/* LINTED pointer alignment */
		prim2 = (union T_primitives *)mp2->b_rptr;
		ASSERT(prim2 != NULL);
		prim2->discon_ind.SEQ_number = (long)te;
		te->te_ocon = NULL;
		UNLOCK(te->te_lock, pl_3);
		UNLOCK(te2->te_lock, pl_2);
		UNLOCK(tco_lock,pl_1);
	} else if (te->te_con != NULL) {
		/*
		 *	disconnect an existing connection to te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_dreq _%d_: disconnect connection",__LINE__);
		te2 = te->te_con;       /* te, te2 are connected peers */
		ASSERT(te2->te_con == te);
		rq = te2->te_rq;
		te2->te_flg |= TCO_BUSY;
		/*
		 * 	Identify client in this connection. (Client is 
		 * 	the endpt that initiated the connect request)
		 */
		if (te->te_flg & TCO_CLIENT) {	/* te is client */
			pl_3 = LOCK_SH(te->te_savelock, plstr);
			te->te_lock = te->te_savelock;
			te->te_savelock = NULL;
		} else {				/* te2 is client */
			ASSERT(te2->te_flg & TCO_CLIENT);
			pl_3 = LOCK_SH(te2->te_savelock, plstr);
			te2->te_lock = te2->te_savelock;
			te2->te_savelock = NULL;
		}
		te2->te_state = NEXTSTATE(TE_DISCON_IND1,te2->te_state);
		ASSERT(te2->te_state != TI_BADSTATE);
		te2->te_con = NULL;
		te->te_con = NULL;
		if (te->te_flg & TCO_CLIENT) {
			te->te_flg &= ~TCO_CLIENT;
			UNLOCK(te->te_lock, pl_3);
			UNLOCK(te2->te_lock, pl_2);
		} else {
			te2->te_flg &= ~TCO_CLIENT;
			UNLOCK(te2->te_lock, pl_3);
			UNLOCK(te->te_lock, pl_2);
		}
		UNLOCK(tco_lock, pl_1);
		if ((tco_flush(rq, FLUSHR) == -1)
		||  ((mp2 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL)) {
			STRLOG(TCO_ID,tco_min(te2),1,SL_TRACE,
			    "tco_dreq _%d_ fatal: allocb() failure",__LINE__);
			tco_okack(q, mp1);
			tco_fatal(rq,mp);
			pl_1 = LOCK(tco_lock, plstr);
			pl_2 = LOCK(te2->te_lock, plstr);
			te2->te_flg &= ~TCO_BUSY;
			UNLOCK(te2->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			return;
		}
		flushq(WR(rq),FLUSHALL);
		flushq(q,FLUSHALL);
		ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim2 = (union T_primitives *)mp2->b_rptr;
		prim2->discon_ind.SEQ_number = BADSEQNUM;
	} else {
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock,pl_1);
		/*
		 *+ Internal error
		 */
		cmn_err(CE_PANIC, "tco_dreq: Inconsistent state for a disconnect request");
		/* NOTREACHED */
	}
	/*
	 *	prepare indication msg
	 */
	mp2->b_datap->db_type = M_PROTO;
	mp2->b_wptr = mp2->b_rptr + sizeof(struct T_discon_ind);
	prim2->discon_ind.PRIM_type = T_DISCON_IND;
	prim2->discon_ind.DISCON_reason = TCO_PEERINITIATED;
	/*
	 *	relink data blocks from mp to mp2
	 */
	/* following is faster than (void)linkb(mp2,unlinkb(mp)); */
	mp2->b_cont = mp->b_cont;
	mp->b_cont = NULL;
	/*
	 *	send indication msg
	 */
	putnext(rq,mp2);
	pl_1 = LOCK(tco_lock, plstr);
	pl_2 = LOCK(te2->te_lock, plstr);
	te2->te_flg &= ~TCO_BUSY;
	UNLOCK(te2->te_lock, pl_2);
	UNLOCK(tco_lock, pl_1);
	tco_okack(q, mp1);
	freeb(mp);
	return;
}


/*
 * STATIC void
 * tco_unconnect(tco_endpt_t *te)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	cleanup utility routine
 */
STATIC void
tco_unconnect(tco_endpt_t *te)
{
	union T_primitives	*prim;
	tco_endpt_t		*te1;
	tco_endpt_t		**tep;
	tco_endpt_t		**etep;
	queue_t			*q;
	queue_t			*rq;
	queue_t			*wq;
	mblk_t			*mp1;
	pl_t			pl_1, pl_2, pl_3;
	int			i;


	q = te->te_rq;

	flushq(q,FLUSHALL);

retry_unconnect:
	pl_1 = LOCK(tco_lock, plstr);
	if (te->te_flg & TCO_BUSY) {
		/*
		 * Lost race. Endpt (te) is being modified by either:
		 *	1. an endpt (te1) that initiated a connect request or
		 *	2. an endpt (te1) that recived a connect request or
		 *	3. a connected endpt (te1).
		 *
		 * Simple backoff algorithm. Works always.
		 */
		UNLOCK(tco_lock, pl_1);
		for(i=0;i<100000;i++);
		goto retry_unconnect;
	}
	ASSERT(te->te_nicon >= 0);
	if (te->te_nicon > 0) {
		pl_2 = LOCK(te->te_lock, plstr);
		/*
		 *	unconnect incoming connect requests pending to te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_unconnect _%d_: disconnect incoming",__LINE__);
		ASSERT(te->te_ocon == NULL);
		ASSERT(te->te_con == NULL);
		for (tep = &te->te_icon[0], etep = &te->te_icon[TCO_MAXQLEN];
							tep < etep; tep += 1) {
			if (*tep == NULL) {
				continue;
			}

			te1 = *tep;	/* te = server; te1 = clients */

			if ((mp1 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL) {
				STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
					"tco_unconnect _%d_: allocb() failure",__LINE__);
				UNLOCK(te->te_lock, pl_2);
				UNLOCK(tco_lock, pl_1);
				goto out;
			}
			*tep = NULL;
			te->te_nicon--;
			ASSERT(te1->te_ocon == te);
			te1->te_ocon = NULL;
			ASSERT(te1->te_con == NULL);
			te1->te_state = NEXTSTATE(TE_DISCON_IND1,te1->te_state);
			ASSERT(te1->te_state != TI_BADSTATE);
			rq = te1->te_rq;
			te1->te_flg |= TCO_BUSY;
			ASSERT(te1->te_flg & TCO_CLIENT);
			te1->te_flg &= ~TCO_CLIENT;
			pl_3 = LOCK_SH(te1->te_savelock, plstr);
			te1->te_lock = te1->te_savelock;
			te1->te_savelock  = NULL;
			mp1->b_datap->db_type = M_PROTO;
			mp1->b_wptr = mp1->b_rptr + sizeof(struct T_discon_ind);
			ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
			/* LINTED pointer alignment */
			prim = (union T_primitives *)mp1->b_rptr;
			prim->discon_ind.PRIM_type = T_DISCON_IND;
			prim->discon_ind.DISCON_reason = TCO_PROVIDERINITIATED;
			prim->discon_ind.SEQ_number = BADSEQNUM;
			UNLOCK(te1->te_lock, pl_3);
			UNLOCK(te->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			putnext(rq,mp1);
			pl_1 = LOCK(tco_lock, plstr);
			pl_2 = LOCK(te->te_lock, plstr);
			pl_3 = LOCK_SH(te1->te_lock, plstr);
			te1->te_flg &= ~TCO_BUSY;
			UNLOCK(te1->te_lock, pl_3);
		}
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
	} else if (te->te_ocon != NULL) {
		pl_2 = LOCK(te->te_lock, plstr);
		/*
		 *	unconnect outgoing connect request pending from te
		 */
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_unconnect _%d_: disconnect incoming",__LINE__);
		ASSERT(te->te_nicon == 0);
		ASSERT(te->te_con == NULL);

		te1 = te->te_ocon;	/* te = client; te1 = server */
		ASSERT(te1 != NULL);

		if ((mp1 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL){
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_unconnect _%d_: allocb() failure",__LINE__);
			UNLOCK(te->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			goto out;
		}
		for (tep = &te1->te_icon[0], etep = &te1->te_icon[TCO_MAXQLEN];
							tep < etep; tep += 1) {
			if (*tep == te) {
				*tep = NULL;
				ASSERT(te1->te_nicon >= 1);
				if (te1->te_nicon == 1) {
					te1->te_state = NEXTSTATE(TE_DISCON_IND2,te1->te_state);
					ASSERT(te1->te_state != TI_BADSTATE);
				} else {
					te1->te_state = NEXTSTATE(TE_DISCON_IND3,te1->te_state);
					ASSERT(te1->te_state != TI_BADSTATE);
				}
				te1->te_nicon -= 1;
				te->te_ocon = NULL;
				break;
			}
		}
		rq = te1->te_rq;
		te1->te_flg |= TCO_BUSY;
		te->te_flg &= ~TCO_CLIENT;
		pl_3 = LOCK_SH(te->te_savelock, plstr);
		te->te_lock = te->te_savelock;
		te->te_savelock = 0;

		mp1->b_datap->db_type = M_PROTO;
		mp1->b_wptr = mp1->b_rptr + sizeof(struct T_discon_ind);
		ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim = (union T_primitives *)mp1->b_rptr;
		ASSERT(prim != NULL);
		prim->discon_ind.SEQ_number = (long)te;
		prim->discon_ind.PRIM_type = T_DISCON_IND;
		prim->discon_ind.DISCON_reason = TCO_PROVIDERINITIATED;
		UNLOCK(te->te_lock, pl_3);
		UNLOCK(te1->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		putnext(rq,mp1);
		pl_1 = LOCK(tco_lock, plstr);
		pl_2 = LOCK(te1->te_lock, plstr);
		te1->te_flg &= ~TCO_BUSY;
		UNLOCK(te1->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
	} else if (te->te_con != NULL) {
		/*
		 *	unconnect existing connection to te
		 */
		pl_2 = LOCK(te->te_lock, plstr);
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_unconnect _%d_: disconnect connection",__LINE__);
		ASSERT(te->te_nicon == 0);
		ASSERT(te->te_ocon == NULL);
		te1 = te->te_con;       /* te, te1 are connected peers */
		ASSERT(te1 != NULL);
		rq = te1->te_rq;
		wq = WR(rq);
		te1->te_flg |= TCO_BUSY;
		UNLOCK(te->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
		if ((tco_flush(rq, FLUSHR) == -1)
		||  ((mp1 = allocb(sizeof(struct T_discon_ind),BPRI_HI)) == NULL)) {
			STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
			    "tco_unconnect _%d_: allocb() failure",__LINE__);
			pl_1 = LOCK(tco_lock, plstr);
			pl_2 = LOCK(te1->te_lock, plstr);
			te1->te_flg &= ~TCO_BUSY;
			UNLOCK(te1->te_lock, pl_2);
			UNLOCK(tco_lock, pl_1);
			goto out;
		}
		mp1->b_datap->db_type = M_PROTO;
		mp1->b_wptr = mp1->b_rptr + sizeof(struct T_discon_ind);
		pl_1 = LOCK(tco_lock, plstr);
		pl_2 = LOCK(te->te_lock, plstr);
		if (te->te_flg & TCO_CLIENT) {
			pl_3 = LOCK_SH(te->te_savelock, plstr);
			te->te_lock = te->te_savelock;
			te->te_savelock = 0;
		} else {
			pl_3 = LOCK_SH(te1->te_savelock, plstr);
			te1->te_lock = te1->te_savelock;
			te1->te_savelock = 0;
		}
		te->te_con = NULL;
		te1->te_con = NULL;
		te1->te_state = NEXTSTATE(TE_DISCON_IND1,te1->te_state);
		ASSERT(te1->te_state != TI_BADSTATE);
		if (te->te_flg & TCO_CLIENT) {
			te->te_flg &= ~TCO_CLIENT;
			UNLOCK(te->te_lock, pl_3);
			UNLOCK(te1->te_lock, pl_2);
		} else {
			te1->te_flg &= ~TCO_CLIENT;
			UNLOCK(te1->te_lock, pl_3);
			UNLOCK(te->te_lock, pl_2);
		}
		UNLOCK(tco_lock,pl_1);

		flushq(wq,FLUSHALL);
		/* LINTED pointer alignment */
		prim = (union T_primitives *)mp1->b_rptr;
		prim->discon_ind.SEQ_number = BADSEQNUM;
		prim->discon_ind.PRIM_type = T_DISCON_IND;
		prim->discon_ind.DISCON_reason = TCO_PROVIDERINITIATED;
		putnext(rq,mp1);
		pl_1 = LOCK(tco_lock, plstr);
		pl_2 = LOCK(te1->te_lock, plstr);
		te1->te_flg &= ~TCO_BUSY;
		UNLOCK(te1->te_lock, pl_2);
		UNLOCK(tco_lock, pl_1);
	} else {
		UNLOCK(tco_lock, pl_1);
		STRLOG(TCO_ID,tco_min(te),4,SL_TRACE,
		    "tco_unconnect _%d_: nothing to unconnect",__LINE__);
	}
    out:
	pl_2 = LOCK(te->te_lock, plstr);
	te->te_state = TI_BADSTATE;
	UNLOCK(te->te_lock, pl_2);
	return;
}


/*
 * STATIC int
 * tco_data(queue_t *q, mblk_t *mp, int evtype)
 *
 * Calling/Exit State:
 *	No locks to be held on entry.
 *
 * Description:
 *	handle data request
 */
STATIC int
tco_data(queue_t *q, mblk_t *mp, int evtype)
{
	tco_endpt_t		*te,*te1;
	union T_primitives	*prim;
	queue_t				*q1, *wq;
	int				msz;
	mblk_t				*nmp;
	pl_t				pl_1;
#ifdef TICOTSORD
	pl_t				pl_2, pl_3;
#endif


	te = (tco_endpt_t *)q->q_ptr;

	if ((int)(mp->b_rptr)%NBPW != 0) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_data _%d_: (mp->b_rptr)%%NBPW!=0",__LINE__);
	}
	/*
	 *	validate msg
	 */
	msz = mp->b_wptr - mp->b_rptr;
	switch (mp->b_datap->db_type) {
	    default:
		/*
		 *+ Internal error
		 */
		cmn_err(CE_PANIC, "tco_data: Incorrect data type");
		/* NOTREACHED */
	    case M_DATA:
		break;
	    case M_PROTO:
		switch (evtype) {
		    default:
			/*
			 *+ Internal error
			 */
			cmn_err(CE_PANIC, "tco_data: Incorrect event type arg");
			/* NOTREACHED */
		    case TE_DATA_REQ:
			if (msz < sizeof(struct T_data_req)) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: bad control",__LINE__);
				tco_fatal(q,mp);
				return(0);
			}
			break;
		    case TE_EXDATA_REQ:
			if (msz < sizeof(struct T_exdata_req)) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: bad control",__LINE__);
				tco_fatal(q,mp);
				return(0);
			}
			break;
#ifdef TICOTSORD
		    case TE_ORDREL_REQ:
			if (msz < sizeof(struct T_ordrel_req)) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: bad control",__LINE__);
				tco_fatal(q,mp);
				return(0);
			}
			break;
#endif
		}
		break;
	}
	/*
	 *	get connected endpt
	 */
	pl_1 = LOCK(te->te_lock, plstr);	/* locks both endpts */

	te1 = te->te_con;	/* te = sender; te1 = receiver */
	if (te1 == NULL) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_data _%d_ fatal: not connected",__LINE__);
		UNLOCK(te->te_lock, pl_1);
		tco_fatal(q,mp);
		return(0);
	}
	ASSERT(te->te_ocon == NULL);
	ASSERT(te1->te_ocon == NULL);
	q1 = te1->te_rq;
	wq = WR(q1);
	if (!canputnext(q1)) {
		STRLOG(TCO_ID,tco_min(te),3,SL_TRACE,
		    "tco_data _%d_: canput() failure",__LINE__);
		UNLOCK(te->te_lock, pl_1);
		putbq(q,mp);
		return(-1);
	}
	/*
	 *	check state
	 */
	switch (mp->b_datap->db_type) {
	    default:
		UNLOCK(te->te_lock, pl_1);
		/*
		 *+ Internal error
		 */
		cmn_err(CE_PANIC, "tco_data: Incorrect data type");
		/* NOTREACHED */
	    case M_DATA:
		if ((te->te_state = NEXTSTATE(TE_DATA_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
			    "tco_data _%d_ fatal: out of state, state-%d",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_fatal(q,mp);
			return(0);
		}
		te1->te_flg |= TCO_BUSY;
		if ((te1->te_state = NEXTSTATE(TE_DATA_IND,te1->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCO_ID,tco_min((tco_endpt_t *)wq->q_ptr),1,SL_TRACE,
			    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te1->te_state);
			UNLOCK(te->te_lock, pl_1);
			tco_fatal(wq,mp);
			pl_1 = LOCK(te->te_lock, plstr);
			te1->te_flg &= ~TCO_BUSY;
			UNLOCK(te->te_lock, pl_1);
			return(0);
		}
		break;
	    case M_PROTO:
		/* LINTED pointer alignment */
		prim = (union T_primitives *)mp->b_rptr;
		/*
		 * If another module/driver is using the message block,
		 * create a new one and copy content of old one.
		 */
		if (mp->b_datap->db_ref > 1) {
			if ((nmp = copymsg(mp)) == NULL) {
				STRLOG(TCO_ID, tco_min(te), 1, SL_TRACE,
					"tco_data _%d_ fatal: can't copyb()",
						__LINE__);
				te1->te_flg |= TCO_BUSY;
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(wq, mp);
				pl_1 = LOCK(te->te_lock, plstr);
				te1->te_flg &= ~TCO_BUSY;
				UNLOCK(te->te_lock, pl_1);
				return(0);
			}
			freemsg(mp);
			mp = nmp;
		}

		switch (evtype) {
		    default:
			UNLOCK(te->te_lock, pl_1);
			/*
			 *+ Incorrect error
			 */
			cmn_err(CE_PANIC, "tco_data: Incorrect event type arg");
			/* NOTREACHED */
		    case TE_DATA_REQ:
			if ((te->te_state =
			     NEXTSTATE(TE_DATA_REQ,te->te_state)) ==
			    TI_BADSTATE) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te->te_state);
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(q,mp);
				return(0);
			}
			te1->te_flg |= TCO_BUSY;
			if ((te1->te_state =
			     NEXTSTATE(TE_DATA_IND,te1->te_state)) ==
			    TI_BADSTATE) {
				STRLOG(TCO_ID,tco_min((tco_endpt_t *)wq->q_ptr),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te1->te_state);
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(wq,mp);
				pl_1 = LOCK(te1->te_lock, plstr);
				te1->te_flg &= ~TCO_BUSY;
				UNLOCK(te1->te_lock, pl_1);
				return(0);
			}
			/* re-use msg block: */
			prim->type = T_DATA_IND;
			break;
		    case TE_EXDATA_REQ:
			if ((te->te_state =
			     NEXTSTATE(TE_EXDATA_REQ,te->te_state)) ==
			    TI_BADSTATE) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te->te_state);
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(q,mp);
				return(0);
			}
			te1->te_flg |= TCO_BUSY;
			if ((te1->te_state =
			     NEXTSTATE(TE_EXDATA_IND,te1->te_state)) ==
			    TI_BADSTATE) {
				STRLOG(TCO_ID,tco_min((tco_endpt_t *)wq->q_ptr),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te1->te_state);
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(wq,mp);
				pl_1 = LOCK(te1->te_lock, plstr);
				te1->te_flg &= ~TCO_BUSY;
				UNLOCK(te1->te_lock, pl_1);
				return(0);
			}
			/* re-use msg block: */
			prim->type = T_EXDATA_IND;
			break;
#ifdef TICOTSORD
		    case TE_ORDREL_REQ:
			if ((te->te_state =
			     NEXTSTATE(TE_ORDREL_REQ,te->te_state)) ==
			    TI_BADSTATE) {
				STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te->te_state);
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(q,mp);
				return(0);
			}
			te1->te_flg |= TCO_BUSY;
			if ((te1->te_state =
			     NEXTSTATE(TE_ORDREL_IND,te1->te_state)) ==
			    TI_BADSTATE) {
				STRLOG(TCO_ID,tco_min((tco_endpt_t *)wq->q_ptr),1,SL_TRACE,
				    "tco_data _%d_ fatal: out of state, state=%d",__LINE__,te1->te_state);
				UNLOCK(te->te_lock, pl_1);
				tco_fatal(wq,mp);
				pl_1 = LOCK(te1->te_lock, plstr);
				te1->te_flg &= ~TCO_BUSY;
				UNLOCK(te1->te_lock, pl_1);
				return(0);
			}
			/*
			 * This looks unreasonably heavyweight,but
			 * it is an orderly release disconnect via a
			 * a data message. So it should not matter.
			 * Other approaches would add this overhead
			 * to every tco_data request.
			 */
			if (te1->te_state == TS_IDLE) {
				UNLOCK(te->te_lock, pl_1);
				/*
				 * Need to do this to respect locking
				 * hierarchy and prevent deadlocks
				 */
				pl_1 = LOCK(tco_lock, plstr);
				pl_2 = LOCK(te->te_lock, plstr);
				if (te->te_flg & TCO_CLIENT) {
					pl_3 = LOCK_SH(te->te_savelock, plstr);
					te->te_lock = te->te_savelock;
					te->te_savelock = NULL;
					te->te_flg &= ~TCO_CLIENT;
					te->te_con = NULL;
					te1->te_con = NULL;
					UNLOCK(te->te_lock, pl_3);
					UNLOCK(te1->te_lock, pl_2);
				} else {
					pl_3 = LOCK_SH(te1->te_savelock, plstr);
					te1->te_lock = te1->te_savelock;
					te1->te_savelock = NULL;
					te1->te_flg &= ~TCO_CLIENT;
					te1->te_con = NULL;
					te->te_con = NULL;
					UNLOCK(te1->te_lock, pl_3);
					UNLOCK(te->te_lock, pl_2);
				}
				UNLOCK(tco_lock, pl_1);
				pl_1 = LOCK(te->te_lock, plstr);
			}
			/* re-use msg block: because T_ORDREL_REQ and
			 * 		     T_ORDREL_IND are of same size
			 */
			prim->type = T_ORDREL_IND;
#endif
		}
		break;
	}
	/*
	 *	send data to connected peer
	 */
	UNLOCK(te->te_lock, pl_1);
	putnext(q1,mp);
	pl_1 = LOCK(te->te_lock, plstr);
	te1->te_flg &= ~TCO_BUSY;
	UNLOCK(te->te_lock, pl_1);
	return(0);
}


/*
 * STATIC void
 * tco_areq(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held on entry.
 *
 * Description:
 *	Routine to handle "get protocol addresses" request
 */
STATIC void
tco_areq(queue_t *q, mblk_t *mp)
{
	union T_primitives		*prim;
	tco_endpt_t			*te;
	mblk_t				*mp1;
	int				msz1;
	tco_addr_t		 	addr;
	pl_t				pl_1;


	te = (tco_endpt_t *)q->q_ptr;

	/*
	 *	prepare ack msg
	 */
	pl_1 = LOCK(te->te_lock, plstr);
	if (te->te_state == TS_UNBND) {
	    /*
	     * endpt is not bound
	     */
	    msz1 = sizeof(struct T_addr_ack);
	} else {
	    /*
	     * endpt is bound ....
	     */
	    ASSERT(te->te_addr != NULL);
	    msz1 = (sizeof(struct T_addr_ack) + tco_alen(te->te_addr));
	    if (te->te_state == TS_DATA_XFER) {
		/*
		 * ... and is in data transfer state then remote
		 *     address is also available. Allocate message
		 *     buffer for both local and remote addresses.
		 */
		ASSERT(te->te_con != NULL);
		msz1 += tco_alen(te->te_con->te_addr);
	    }
	}
	if ((mp1 = allocb(msz1, BPRI_HI)) == NULL) {
		STRLOG(TCO_ID,tco_min(te),1,SL_TRACE,
		    "tco_areq _%d_ fatal: allocb() failure",__LINE__);
		UNLOCK(te->te_lock, pl_1);
		tco_fatal(q,mp);
		return;
	}
	mp1->b_datap->db_type = M_PCPROTO;
	mp1->b_wptr = mp1->b_rptr + sizeof(struct T_addr_ack);
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp1->b_rptr;
	prim->addr_ack.PRIM_type = T_ADDR_ACK;

	if (te->te_state != TS_UNBND) 	{
	    /*
	     * endpt is bound.....
	     */
	    ASSERT(te->te_addr != NULL);
	    prim->addr_ack.LOCADDR_length = tco_alen(te->te_addr);
	    prim->addr_ack.LOCADDR_offset = sizeof(struct T_addr_ack);
	    addr.ta_abuf = (char *)mp1->b_wptr;
	    addr.ta_alen = tco_alen(te->te_addr);
	    (void)tco_cpabuf(&addr, te->te_addr);		/* Local addr */
	    mp1->b_wptr += tco_alen(te->te_addr);
	    if (te->te_state == TS_DATA_XFER) {
		/*
		 * ... and is in data transfer state. Remote 
		 *     addresses are to be copied into message.
		 */	
		prim->addr_ack.REMADDR_length = tco_alen(te->te_con->te_addr);
		prim->addr_ack.REMADDR_offset = (sizeof(struct T_addr_ack) +
						tco_alen(te->te_addr));
		addr.ta_abuf = (char *)mp1->b_wptr;
		addr.ta_alen = tco_alen(te->te_con->te_addr);
		tco_cpabuf(&addr, te->te_con->te_addr); /* Rem. addr */
		mp1->b_wptr += tco_alen(te->te_con->te_addr);
		UNLOCK(te->te_lock, pl_1);
	    } else {
		/*
		 * ... and is *NOT* in data transfer state. Remote
		 *     address unavailable.
		 */
		UNLOCK(te->te_lock, pl_1);
		prim->addr_ack.REMADDR_length = 0;
		prim->addr_ack.REMADDR_offset = 0;
	    }
	} else 	{
		/*
		 * endpt not bound => local address is zero
		 */
		UNLOCK(te->te_lock, pl_1);
		prim->addr_ack.LOCADDR_length = 0;
		prim->addr_ack.LOCADDR_offset = 0;
		prim->addr_ack.REMADDR_length = 0;
		prim->addr_ack.REMADDR_offset = 0;
	}

	/*
	 *	send ack msg
	 */
	qreply(q,mp1);
	freemsg(mp);
	return;
}
