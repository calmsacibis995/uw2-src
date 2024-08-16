/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/loopback/ticlts/ticlts.c	1.21"
#ident	"$Header: $"

/*
 *	TPI loopback transport provider.
 *	Datagram mode.
 *	Connectionless type.
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
#include <net/loopback/ticlts.h>
#include <util/inline.h>
#include <svc/systm.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>


extern int		 tcl_cnt;
void			 tclinit(void);
STATIC int		 tcl_load(void);
STATIC int		 tcl_unload(void);

#define DRVNAME "ticlts - Loadable connectionless loopback driver"

MOD_DRV_WRAPPER(tcl, tcl_load, tcl_unload, NULL, DRVNAME);

STATIC void		 tcl_areq(queue_t *, mblk_t *);
STATIC void		 tcl_bind(queue_t *, mblk_t *);
STATIC void		 tcl_blink(tcl_endpt_t *, tcl_addr_t *);
STATIC int		 tcl_ckopt(char *, char *);
STATIC void		 tcl_ckstate(queue_t *, mblk_t *);
STATIC int		 tcl_close(queue_t *, int, cred_t *);
STATIC int		 tcl_cpabuf(tcl_addr_t *, tcl_addr_t *);
STATIC int		 tcl_data(queue_t *, mblk_t *);
STATIC void		 tcl_errack(queue_t *, mblk_t *, long, long);
STATIC void		 tcl_fatal(queue_t *, mblk_t *);
STATIC int		 tcl_flush(queue_t *);
STATIC void		 tcl_ireq(queue_t *, mblk_t *);
STATIC void		 tcl_okack(queue_t *, mblk_t *);
STATIC void		 tcl_olink(tcl_endpt_t *);
STATIC int		 tcl_open(queue_t *, dev_t *, int, int, cred_t *);
STATIC void		 tcl_optmgmt(queue_t *, mblk_t *);
STATIC int		 tcl_sumbytes(char *, int );
STATIC void		 tcl_uderr(queue_t *, mblk_t *, long);
STATIC void		 tcl_unbind(queue_t *, mblk_t *);
STATIC void		 tcl_unblink(tcl_endpt_t *);
STATIC void		 tcl_unconnect(tcl_endpt_t *);
STATIC void		 tcl_unolink(tcl_endpt_t *);
STATIC int		 tcl_wput(queue_t *, mblk_t *);
STATIC void		 tcl_wropt(long, tcl_endpt_t *, char *);
STATIC int		 tcl_wsrv(queue_t *);

STATIC void		 tcl_link(queue_t *, mblk_t *);
STATIC int		 tcl_endptinit(minor_t, cred_t *, tcl_endpt_t **);
STATIC int		 tcl_getendpt(minor_t,queue_t *,cred_t *,tcl_endpt_t **); 
STATIC tcl_addr_t	*tcl_addrinit(tcl_addr_t *);
STATIC tcl_addr_t	*tcl_getaddr(int, tcl_addr_t *, int *);

int tcldevflag = D_MP;

#define	TCL_HIER	1

/*
 *+ Endpoint state structure lock
 */
LKINFO_DECL(tcl_endpt_lkinfo, "TCL::te_lock", 0);
/*
 *+ Hash tables and global info lock
 */
LKINFO_DECL(tcl_hshlkinfo, "TCL::tcl_lock", 0);

lock_t	*tcl_lock;	/* lock to protect the following */

STATIC tcl_endpt_t	*tcl_endptopen[TCL_NMHASH];  /* open endpt hash table */
STATIC tcl_endpt_t	 tcl_defaultendpt;
STATIC tcl_addr_t	*tcl_addrbnd[TCL_NAHASH];    /* bound addr hash table */
STATIC tcl_addr_t	 tcl_defaultaddr;
STATIC char		 tcl_defaultabuf[TCL_DEFAULTADDRSZ];

STATIC struct module_info tcl_info = {
	TCL_ID,
	"tcl",
	TCL_MINPSZ,
	TCL_MAXPSZ,
	TCL_HIWAT,
	TCL_LOWAT
};

STATIC struct qinit tcl_winit = {
	tcl_wput,
	tcl_wsrv,
	NULL,
	NULL,
	NULL,
	&tcl_info,NULL
};

STATIC struct qinit tcl_rinit = {
	NULL,
	NULL,
	tcl_open,
	tcl_close,
	nulldev,
	&tcl_info,
	NULL
};

struct streamtab tclinfo = {
	&tcl_rinit,
	&tcl_winit,
	NULL,
	NULL
};


/*
 * STATIC void
 * tcl_olink(tcl_endpt_t *)
 *
 * Calling/Exit State:
 *	tcl_lock must be held on entry
 *
 * Description:
 *	link endpt to tcl_endptopen[] hash table
 */
STATIC void
tcl_olink(tcl_endpt_t *te)
{
	tcl_endpt_t		**tep;


	ASSERT(te != NULL);
	/*
	 *	add te to tcl_endptopen[] table
	 */
	tep = &tcl_endptopen[tcl_mhash(te)];
	if (*tep != NULL) {
		(*tep)->te_bolist = te;
	}
	te->te_folist = *tep;
	te->te_bolist = NULL;
	*tep = te;
	return;
}


/*
 * STATIC void
 * tcl_unolink(tcl_endpt_t *te)
 *
 * Calling/Exit State:
 *	tcl_lock must be held on entry.
 *
 * Description:
 *	unlink endpt from tcl_endptopen[] hash table
 */
STATIC void
tcl_unolink(tcl_endpt_t *te)
{
	ASSERT(te != NULL);

	/*
	 *	remove te from tcl_endptopen[] table
	 */
	if (te->te_bolist != NULL) {
		te->te_bolist->te_folist = te->te_folist;
	} else {
		tcl_endptopen[tcl_mhash(te)] = te->te_folist;
	}
	if (te->te_folist != NULL) {
		te->te_folist->te_bolist = te->te_bolist;
	}
	/*
	 *	free te
	 */
	ASSERT(te->te_bid == NULL);
	LOCK_DEALLOC(te->te_lock);
	EVENT_DEALLOC(te->te_event);
	kmem_free((char *)te,sizeof(tcl_endpt_t));
	return;
}


/*
 * STATIC void
 * tcl_blink(tcl_endpt_t *te, tcl_addr_t *ta)
 *
 * Calling/Exit State:
 * 	tcl_lock must be held on entry.
 *
 * Description:
 *	link endpt to addr, and addr to tcl_addrbnd[] hash table
 */
STATIC void
tcl_blink(tcl_endpt_t *te, tcl_addr_t *ta)
{
	tcl_addr_t		**tap;


	/*
	 *	add ta to tcl_addrbnd[] table
	 */
	tap = &tcl_addrbnd[tcl_ahash(ta)];
	if (*tap != NULL) {
		(*tap)->ta_balist = ta;
	}
	ta->ta_falist = *tap;
	ta->ta_balist = NULL;
	*tap = ta;
	/*
	 *	link te and ta together
	 */
	te->te_addr = ta;
	ta->ta_blist = te;
	return;
}


/*
 * STATIC void
 * tcl_unblink(tcl_endpt_t *te)
 *
 * Calling/Exit State:
 * 	tcl_lock must be held on entry
 *
 * Description:
 *	unlink endpt from addr, and addr from tcl_addrbnd[] hash table
 */
STATIC void
tcl_unblink(tcl_endpt_t *te)
{
	tcl_addr_t		*ta;

	ASSERT(te != NULL);
	ta = te->te_addr;
	if (ta != NULL) {
		/*
		 *	unlink te from ta
		 */
		te->te_addr = NULL;
		ASSERT(ta->ta_blist == te);
		ta->ta_blist = NULL;
		/*
		 *	remove ta from tcl_addrbnd[] table
		 */
		if (ta->ta_balist != NULL) {
			ta->ta_balist->ta_falist = ta->ta_falist;
		} else {
			tcl_addrbnd[tcl_ahash(ta)] = ta->ta_falist;
		}
		if (ta->ta_falist != NULL) {
			ta->ta_falist->ta_balist = ta->ta_balist;
		}
		/*
		 *	free ta
		 */
		kmem_free(tcl_abuf(ta),tcl_alen(ta));
		kmem_free((char *)ta,sizeof(tcl_addr_t));
	}
	return;
}


/*
 * STATIC int
 * tcl_sumbytes(char *a, int n)
 *
 * Calling/Exit State:
 * 	No locking assumptions. The buffer will not change
 *	if used correctly (ie: with local copies or with
 *	tcl_lock held for global buffers)
 *
 * Description:
 *	sum bytes of buffer (used for hashing)
 */
STATIC int
tcl_sumbytes(char *a, int n)
{
	char			*cp,*ep;
	unsigned		sum;


	ASSERT(a != NULL);
	ASSERT(n > 0);
	sum = 0;
	for (cp = &a[0], ep = &a[n]; cp < ep; cp += 1) {
		sum += (unsigned)*cp;
	}
	return((int)sum);
}


/*
 * STATIC int
 * tcl_cpabuf(tcl_addr_t *to, tcl_addr_t *from)
 *
 * Calling/Exit State:
 *	No locking assumptions. The buffers will not change under
 *	you if interface is used correctly (ie: either with local
 *	copies or with tcl_lock held on global buffers).
 *
 * Description:
 *	copy ta_abuf part of addr, together with ta_len, ta_ahash
 *	(this routine will create a ta_abuf if necessary, but won't resize one)
 */
STATIC int
tcl_cpabuf(tcl_addr_t *to, tcl_addr_t *from)
{
	char				*abuf;


	ASSERT(to != NULL);
	ASSERT(from != NULL);
	ASSERT(tcl_alen(from) > 0);
	ASSERT(tcl_abuf(from) != NULL);
	if (tcl_abuf(to) == NULL) {
		ASSERT(tcl_alen(to) == 0);
		abuf = (char *)kmem_alloc(tcl_alen(from),KM_NOSLEEP);
		if (abuf == NULL) {
			return(-1);
		}
		tcl_alen(to) = tcl_alen(from);
		to->ta_abuf = abuf;
	} else {
		ASSERT(tcl_alen(to) == tcl_alen(from));
	}
	(void)bcopy(tcl_abuf(from),tcl_abuf(to),tcl_alen(to));
	to->ta_ahash = from->ta_ahash;
	return(0);
}


/*
 * STATIC int
 * tcl_endptinit(minor_t min, cred_t *crp, tcl_endpt_t **tep)
 *
 * Calling/Exit State:
 * 	tcl_lock must not be held on entry.
 *
 * Description:
 *	initialize endpoint
 */
STATIC int
tcl_endptinit(minor_t min, cred_t *crp, tcl_endpt_t **tep)
{
	tcl_endpt_t		*te,*te1,*te2;
	minor_t				otcl_minor;
	pl_t				pl_1;


	/*
	 *	get an endpt
	 */
	te1 = (tcl_endpt_t *)kmem_zalloc(sizeof(tcl_endpt_t),KM_NOSLEEP);
	if (te1 == NULL) {
		*tep = NULL;
		return(ENOMEM);
	}
	te1->te_state = TS_UNBND;
	te1->te_lock = LOCK_ALLOC(TCL_HIER+1, plstr, &tcl_endpt_lkinfo,
						KM_NOSLEEP);
	if (te1->te_lock == NULL) {
		kmem_free(te1, sizeof(tcl_endpt_t));
		*tep = NULL;
		return(ENOMEM);
	}
	if ((te1->te_event = EVENT_ALLOC(KM_NOSLEEP)) == NULL) {
		LOCK_DEALLOC(te1->te_lock);
		kmem_free(te1, sizeof(tcl_endpt_t));
		*tep = NULL;
		return(ENOMEM);
	}
	if (min == NODEV) {
		/*
		 *	no minor number requested; we will assign one
		 */
		pl_1 = LOCK(tcl_lock, plstr);
		te = &tcl_defaultendpt;
		otcl_minor = tcl_min(te);
		for (te2 = tcl_endptopen[tcl_mhash(te)]; te2 != NULL; te2 = te2->te_folist) {
			while (te2 != NULL && tcl_min(te2) == tcl_min(te)) {
				/*
				 *	bump default minor and try again
				 */
				if (++tcl_min(te) == tcl_cnt) {
					te->te_min = 0;
				}
				if (tcl_min(te) == otcl_minor) {
				    /*
				     *	wrapped around
				     */
				    UNLOCK(tcl_lock, pl_1);
				    LOCK_DEALLOC(te1->te_lock);
				    EVENT_DEALLOC(te1->te_event);
				    kmem_free((char *)te1,sizeof(tcl_endpt_t));
				    *tep = NULL;
				    return(ENOSPC);
				}
				te2 = tcl_endptopen[tcl_mhash(te)];
			}
			/*
			 * necessary because te2 is changing 2 dimensionally
			 * ie: in the for loop and in the inner while loop.
			 */
			if (te2 == NULL)
				break;
		}
		te1->te_min = tcl_min(te);
		/*
		 *	bump default minor for next time
		 */
		if (++tcl_min(te) == tcl_cnt) {
			te->te_min = 0;
		}
		UNLOCK(tcl_lock, pl_1);
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
	*tep = te1;
	return(0);
}


/*
 * STATIC tcl_addr_t *
 * tcl_addrinit(tcl_addr_t *ta)
 *
 * Calling/Exit State:
 * 	tcl_lock must be held on entry
 *
 * Description:
 *	initialize address
 */
STATIC tcl_addr_t *
tcl_addrinit(tcl_addr_t *ta)
{
	tcl_addr_t		*ta1,*ta2;
	int				i;
	char				*cp;


	/*
	 *	get an address
	 */
	ta1 = (tcl_addr_t *)kmem_zalloc(sizeof(tcl_addr_t),KM_NOSLEEP);
	if (ta1 == NULL) {
		return(NULL);
	}
	if (ta == NULL) {
		/*
		 *	no abuf requested; we will assign one
		 */
		ta = &tcl_defaultaddr;
		for (ta2 = tcl_addrbnd[tcl_ahash(ta)]; ta2 != NULL; ) {
			if (tcl_eqabuf(ta2,ta)) {
				/*
				 *	bump defaultaddr and try again
				 */
				for (i = 0, cp = tcl_abuf(ta); i < tcl_alen(ta); i += 1, cp += 1) {
					if ((*cp += 1) != '\0') {
						break;
					}
				}
				ta->ta_ahash = tcl_mkahash(ta);
				ta2 = tcl_addrbnd[tcl_ahash(ta)];
				if (ta2 == NULL)
					break;
			} else {
				ta2 = ta2->ta_falist;
			}
		}
		if (tcl_cpabuf(ta1,ta) == -1) {
			kmem_free((char *)ta1,sizeof(tcl_addr_t));
			return(NULL);
		}
		/*
		 *	bump defaultaddr for next time
		 */
		for (i = 0, cp = tcl_abuf(ta); i < tcl_alen(ta); i += 1, cp += 1) {
			if ((*cp += 1) != '\0') {
				break;
			}
		}
		ta->ta_ahash = tcl_mkahash(ta);
	} else {
		/*
		 *	an abuf was requested; copy it in
		 */
		if (tcl_cpabuf(ta1,ta) == -1) {
			kmem_free((char *)ta1,sizeof(tcl_addr_t));
			return(NULL);
		}
	}
	return(ta1);
}


/*
 * STATIC int
 * tcl_getendpt(minor_t min,queue_t *rq,cred_t *crp,tcl_endpt_t **tep)
 *
 * Calling/Exit State:
 * 	tcl_lock is not held on entry.
 *
 * Description:
 *	search tcl_endptopen[] for endpt
 */
STATIC int
tcl_getendpt(minor_t min, queue_t *rq, cred_t *crp, tcl_endpt_t **tep)
{
	tcl_endpt_t			endpt,*te;
	int				err;
	pl_t				pl_1;


	/*
	 *	open an endpoint
	 */
	if (min == NODEV) {
		/*
		 * no minor number requested; any free endpt will do
		 */
		return(tcl_endptinit(min, crp, tep));
	} else {
		/*
		 *	find endpt with the requested minor number
		 */
		endpt.te_min = min;
		pl_1 = LOCK(tcl_lock, plstr);
		for (te = tcl_endptopen[tcl_mhash(&endpt)]; te != NULL; te = te->te_folist) {
			if (tcl_min(te) == tcl_min(&endpt)) {
			    /*
			     * Detect if clone open and non-clone
			     * open race is in progress
			     */
			    if (rq && (te->te_rq != rq)) {
				UNLOCK(tcl_lock, pl_1);
				*tep = NULL;
				STRLOG(TCL_ID,-1,3,SL_TRACE,
				    "tcl_getendpt _%d_: CLONE/non-CLONE open race",__LINE__);
				return(ECLNRACE);
			    }
			    *tep = te;
			    UNLOCK(tcl_lock, pl_1);
			    return(0);
			}
		}
		UNLOCK(tcl_lock, pl_1);
		err = tcl_endptinit(min, crp, tep);
		if (err)
			return(err);
		else {
			if (rq && ((*tep)->te_rq != rq)) {
				kmem_free(*tep, sizeof(tcl_endpt_t));
				*tep = NULL;
				STRLOG(TCL_ID,-1,3,SL_TRACE,
				    "tcl_getendpt _%d_: CLONE/non-CLONE open race",__LINE__);
				return(ECLNRACE);
			}
			return(0);
		}
	}
	/* NOTREACHED */
}


/*
 * STATIC tcl_addr_t *
 * tcl_getaddr(int flg, tcl_addr_t *ta, int *unixerr)
 *
 * Calling/Exit State:
 *	tcl_lock must be held on entry
 *
 * Descriptions:
 *	search tcl_addrbnd[] for addr
 */
STATIC tcl_addr_t *
tcl_getaddr(int flg, tcl_addr_t *ta, int *unixerr)
{
	tcl_addr_t		*ta1;


	switch (flg) {
	    default:
		/*
		 *+ Internal Error
		 */
		cmn_err(CE_PANIC, "tcl_getaddr: incorrect flag argument");
		/* NOTREACHED */
	    case TCL_BIND:
		/*
		 *	get an addr that's free to be bound 
		 *	(i.e., not currently bound)
		 */
		if (ta == NULL) {
			/*
			 *	no abuf requested; any free addr will do
			 */
			return(tcl_addrinit(NULL));
		} else {
			/*
			 *	an abuf was requested; get addr with that abuf;
			 *	Return TADDRBUSY if addr is already being used.
			 */
			for (ta1 = tcl_addrbnd[tcl_ahash(ta)]; ta1 != NULL; ta1 = ta1->ta_falist) {
				if (tcl_eqabuf(ta1,ta)) {
					ASSERT(ta1->ta_blist != NULL);
					*unixerr = -1;
					return(NULL);
				}
			}
			return(tcl_addrinit(ta));
		}
		/* NOTREACHED */
	    case TCL_DEST:
		/*
		 *	get addr that can be talked to (i.e., is currently bound)
		 */
		ASSERT(ta != NULL);
		for (ta1 = tcl_addrbnd[tcl_ahash(ta)]; ta1 != NULL; ta1 = ta1->ta_falist) {
			ASSERT(ta1->ta_blist != NULL);
			if (tcl_eqabuf(ta1,ta)) {
				return(ta1);
			}
		}
		return(NULL);
	}
	/* NOTREACHED */
}


/*
 * STATIC int
 * tcl_ckopt(char *obuf, char *ebuf)
 *
 * Calling/Exit State:
 * 	No locking assumptions.
 *
 * Description:
 *	check validity of opt list
 */
STATIC int
tcl_ckopt(char *obuf, char *ebuf)
{
	struct tcl_opt_hdr		*ohdr,*ohdr1;
	union tcl_opt			*opt;
	int				retval = 0;


	/*
	 *	validate format & hdrs & opts of opt list
	 */
	ASSERT(obuf < ebuf);
	/* LINTED pointer alignment */
	for (ohdr = (struct tcl_opt_hdr *)obuf; ;ohdr = ohdr1) {
		if ((int)ohdr%NBPW != 0) {	/* alignment */
			STRLOG(TCL_ID,-1,4,SL_TRACE,
			    "tcl_ckopt _%d_: bad alignment",__LINE__);
			return(retval|TCL_BADFORMAT);
		}
		if ((char *)ohdr + sizeof(struct tcl_opt_hdr) > ebuf) {
			STRLOG(TCL_ID,-1,4,SL_TRACE,
			    "tcl_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCL_BADFORMAT);
		}
		if (ohdr->hdr_thisopt_off < 0) {
			STRLOG(TCL_ID,-1,4,SL_TRACE,
			    "tcl_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCL_BADFORMAT);
		}
		/* LINTED pointer alignment */
		opt = (union tcl_opt *)(obuf + ohdr->hdr_thisopt_off);
		if ((int)opt%NBPW != 0) {	/* alignment */
			STRLOG(TCL_ID,-1,4,SL_TRACE,
			    "tcl_ckopt _%d_: bad alignment",__LINE__);
			return(retval|TCL_BADFORMAT);
		}
		switch (opt->opt_type) {
		    default:
			STRLOG(TCL_ID,-1,4,SL_TRACE,
			    "tcl_ckopt _%d_: unknown opt",__LINE__);
			retval |= TCL_BADTYPE;
			break;
		    case TCL_OPT_NOOP:
			if ((char *)opt + sizeof(struct tcl_opt_noop) > ebuf) {
				STRLOG(TCL_ID,-1,4,SL_TRACE,
				    "tcl_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCL_BADFORMAT);
			}
			retval |= TCL_NOOPOPT;
			break;
		    case TCL_OPT_SETID:
			if ((char *)opt + sizeof(struct tcl_opt_setid) > ebuf) {
				STRLOG(TCL_ID,-1,4,SL_TRACE,
				    "tcl_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCL_BADFORMAT);
			}
			if ((opt->opt_setid.setid_flg & ~TCL_IDFLG_ALL) != 0) {
				STRLOG(TCL_ID,-1,4,SL_TRACE,
				    "tcl_ckopt _%d_: bad opt",__LINE__);
				retval |= TCL_BADVALUE;
				break;
			}
			retval |= TCL_REALOPT;
			break;
		    case TCL_OPT_GETID:
			if ((char *)opt + sizeof(struct tcl_opt_getid) > ebuf) {
				STRLOG(TCL_ID,-1,4,SL_TRACE,
				    "tcl_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCL_BADFORMAT);
			}
			retval |= TCL_REALOPT;
			break;
		    case TCL_OPT_UID:
			if ((char *)opt + sizeof(struct tcl_opt_uid) > ebuf) {
				STRLOG(TCL_ID,-1,4,SL_TRACE,
				    "tcl_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCL_BADFORMAT);
			}
			retval |= TCL_REALOPT;
			break;
		    case TCL_OPT_GID:
			if ((char *)opt + sizeof(struct tcl_opt_gid) > ebuf) {
				STRLOG(TCL_ID,-1,4,SL_TRACE,
				    "tcl_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCL_BADFORMAT);
			}
			retval |= TCL_REALOPT;
			break;
		    case TCL_OPT_RUID:
			if ((char *)opt + sizeof(struct tcl_opt_ruid) > ebuf) {
				STRLOG(TCL_ID,-1,4,SL_TRACE,
				    "tcl_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCL_BADFORMAT);
			}
			retval |= TCL_REALOPT;
			break;
		    case TCL_OPT_RGID:
			if ((char *)opt + sizeof(struct tcl_opt_rgid) > ebuf) {
				STRLOG(TCL_ID,-1,4,SL_TRACE,
				    "tcl_ckopt _%d_: bad offset",__LINE__);
				return(retval|TCL_BADFORMAT);
			}
			retval |= TCL_REALOPT;
			break;
		}
		if (ohdr->hdr_nexthdr_off < 0) {
			STRLOG(TCL_ID,-1,4,SL_TRACE,
			    "tcl_ckopt _%d_: bad offset",__LINE__);
			return(retval|TCL_BADFORMAT);
		}
		if (ohdr->hdr_nexthdr_off == TCL_OPT_NOHDR) {
			return(retval);
		}
		/* LINTED pointer alignment */
		ohdr1 = (struct tcl_opt_hdr *)(obuf + ohdr->hdr_nexthdr_off);
		if (ohdr1 <= ohdr) {
			/* potential loop */
			STRLOG(TCL_ID,-1,4,SL_TRACE,
			    "tcl_ckopt _%d_: potential loop",__LINE__);
			return(retval|TCL_BADFORMAT);
		}
	}
	/* NOTREACHED */
}


/*
 * STATIC void
 * tcl_wropt(long idflg, tcl_endpt_t *te, char *obuf)
 *
 * Calling/Exit State:
 * No locking assumptions.
 *
 * Description:
 *	write opt info into buf
 */
STATIC void
tcl_wropt(long idflg, tcl_endpt_t *te, char *obuf)
{
	struct tcl_opt_hdr		hdr,*ohdr,*oohdr;
	union tcl_opt			*opt;


	/*
	 *	blindly write the opt info into obuf (assume obuf already set up properly)
	 */
	ASSERT(idflg & TCL_IDFLG_ALL);
	ASSERT(((int)obuf)%NBPW == 0);
	oohdr = &hdr;
	oohdr->hdr_nexthdr_off = 0;
	if (idflg & TCL_IDFLG_UID) {
		/* LINTED pointer alignment */
		ohdr = (struct tcl_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tcl_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tcl_opt_uid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tcl_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_uid.uid_type = TCL_OPT_UID;
		opt->opt_uid.uid_val = te->te_uid;
		oohdr = ohdr;
	}
	if (idflg & TCL_IDFLG_GID) {
		/* LINTED pointer alignment */
		ohdr = (struct tcl_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tcl_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tcl_opt_gid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tcl_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_gid.gid_type = TCL_OPT_GID;
		opt->opt_gid.gid_val = te->te_gid;
		oohdr = ohdr;
	}
	if (idflg & TCL_IDFLG_RUID) {
		/* LINTED pointer alignment */
		ohdr = (struct tcl_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tcl_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tcl_opt_ruid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tcl_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_ruid.ruid_type = TCL_OPT_RUID;
		opt->opt_ruid.ruid_val = te->te_ruid;
		oohdr = ohdr;
	}
	if (idflg & TCL_IDFLG_RGID) {
		/* LINTED pointer alignment */
		ohdr = (struct tcl_opt_hdr *)(obuf + oohdr->hdr_nexthdr_off);
		ohdr->hdr_thisopt_off = oohdr->hdr_nexthdr_off + sizeof(struct tcl_opt_hdr);
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		ohdr->hdr_nexthdr_off = ohdr->hdr_thisopt_off + sizeof(struct tcl_opt_rgid);
		ASSERT((ohdr->hdr_nexthdr_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tcl_opt *)(obuf + ohdr->hdr_thisopt_off);
		opt->opt_rgid.rgid_type = TCL_OPT_RGID;
		opt->opt_rgid.rgid_val = te->te_rgid;
		oohdr = ohdr;
	}
	oohdr->hdr_nexthdr_off = TCL_OPT_NOHDR;
	return;
}


/*
 * int
 * tcl_load(void)
 *	Load routine
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

STATIC int
tcl_load(void)
{
	tclinit();
	return(0);
}

/*
 * int
 * tcl_unload(void)
 *	Unload routine
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

STATIC int
tcl_unload(void)
{
	LOCK_DEALLOC(tcl_lock);
	return(0);
}

/*
 * void
 * tclinit(void)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	driver init routine
 */
void
tclinit(void)
{
	/*
	 *	initialize default minor and addr
	 */
	tcl_defaultendpt.te_min = 0;
	tcl_defaultaddr.ta_alen = TCL_DEFAULTADDRSZ;
	tcl_defaultaddr.ta_abuf = tcl_defaultabuf;
	tcl_defaultaddr.ta_ahash = tcl_mkahash(&tcl_defaultaddr);
	/*
 	 * Allocate lock for hash lists and other global variables
	 */
	tcl_lock = LOCK_ALLOC(TCL_HIER, plstr, &tcl_hshlkinfo, KM_NOSLEEP);
	if (tcl_lock == NULL) {
	    /*
	     *+ Kernel memory for tcl_lock struct could
	     *+ not be allocated. Reconfigure the system
	     *+ to consume less memory.
	     */
	    cmn_err(CE_PANIC, "tclinit: no memory for tcl_lock. Reconfigure");
	}
	return;
}

/*
 * void
 * tcl_openwakeup(long addr)
 *	Wake up an lwp that has been sleeping, waiting for memory.
 *
 * Calling/Exit State:
 *	Posts an even for lwp's that are awaiting memory.  No locks held.
 */

STATIC  void
tcl_openwakeup(long addr)
{
	EVENT_SIGNAL((event_t *)addr, 0);
}


/*
 * STATIC int
 * tcl_open(queue_t *q, dev_t *devp, int oflg, int sflg, cred_t *crp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	driver open routine
 */
/* ARGSUSED */
STATIC int
tcl_open(queue_t *q, dev_t *devp, int oflg, int sflg, cred_t *crp)
{
	tcl_endpt_t			*te;
	minor_t				 min;
	int				 err;
	pl_t				 pl_1;
	struct stroptions		*strop;
	mblk_t				*bp;

	ASSERT(q != NULL);
	te = (tcl_endpt_t *)q->q_ptr;
	/*
	 *	is it already open?
	 */
	if (te != NULL) {
		STRLOG(TCL_ID,tcl_min(te),3,SL_TRACE,
		    "tcl_open _%d_: re-open",__LINE__);
		return(0);
	}
	/*
	 *	get endpt with requested minor number
	 */
	if (sflg == CLONEOPEN) {
		min = NODEV;
	} else {
		min = geteminor(*devp);
		if (min >= tcl_cnt) {
			return(ENXIO);
		}
	}
	err = tcl_getendpt(min, q, crp, &te);
	if (te == NULL) {
		STRLOG(TCL_ID,-1,3,SL_TRACE,
		    "tcl_open _%d_: cannot allocate endpoint, q=%x",__LINE__,(long)q);
		return(err);
	}
	/*
	 *	assign te to queue private pointers
	 */
	te->te_rq = q;
	q->q_ptr = (caddr_t)te;
	WR(q)->q_ptr = (caddr_t)te;
	/*
	 * Send an M_SETOPTS message to stream head to set SO_LOOP flag
	 */
	while ((bp = allocb((int)sizeof (struct stroptions), BPRI_MED)) == NULL) {
		te->te_bid = bufcall(sizeof(struct stroptions), BPRI_MED,
		tcl_openwakeup, (long)te->te_event);
		if (te->te_bid == NULL) {
			LOCK_DEALLOC(te->te_lock);
			EVENT_DEALLOC(te->te_event);
			kmem_free(te, sizeof(tcl_endpt_t));
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
			kmem_free(te, sizeof(tcl_endpt_t));
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
	 *	link to tcl_endptopen[] table
	 */
	pl_1 = LOCK(tcl_lock, plstr);
	(void)tcl_olink(te);
	UNLOCK(tcl_lock, pl_1);

	STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
	    "tcl_open _%d_: endpoint allocated",__LINE__);
	qprocson(q);
	*devp = makedevice(getmajor(*devp), te->te_min);
	return(0);
}


/*
 * STATIC int
 * tcl_close(queue_t *q, int cflag, cred_t *crp)
 *
 * Calling/Exit State:
 * 	No locking assumptions.
 *
 * Description:
 *	driver close routine
 */
/* ARGSUSED */
STATIC int
tcl_close(queue_t *q, int cflag, cred_t *crp)
{
	tcl_endpt_t		*te;
	pl_t			pl_1;


	ASSERT(q != NULL);
	te = (tcl_endpt_t *)q->q_ptr;
	qprocsoff(q);
	ASSERT(te != NULL);

	if (te->te_bid) {
		unbufcall(te->te_bid);
		te->te_bid = NULL;
	}
	tcl_unconnect(te);
	pl_1 = LOCK(tcl_lock, plstr);
	tcl_unblink(te);
	tcl_unolink(te);
	UNLOCK(tcl_lock, pl_1);
	STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
	    "tcl_close _%d_: endpoint deallocated",__LINE__);
	return(0);
}


/*
 * STATIC int
 * tcl_wput(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 * 	No locking assumptions.
 *
 * Description:
 *	driver write side put procedure
 */
STATIC int
tcl_wput(queue_t *q, mblk_t *mp)
{
	tcl_endpt_t		*te;
	union T_primitives	*prim;
	int				 msz;
	pl_t				pl_1;


	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tcl_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/* 
	 *	switch on streams msg type
	 */
	msz = mp->b_wptr - mp->b_rptr;
	switch (mp->b_datap->db_type) {
	    default:
		STRLOG(TCL_ID,tcl_min(te),3,SL_TRACE,
		    "tcl_wput _%d_: got illegal msg, M_type=%d",__LINE__,mp->b_datap->db_type);
		(void)freemsg(mp);
		return(-1);

		/* 
		 * Link and unlink the endpoint.
		 */
	    case M_CTL:
		/*
		 * TCL_LINK and TCL_UNLINK messages
		 * 
		 * Note: Sockmod will send these messages
		 */
		/* LINTED pointer alignment */
		switch(*(long *)mp->b_rptr) {
		default:
			break;
		case TCL_LINK:
			pl_1 = LOCK(tcl_lock, plstr);
			tcl_link(q, mp);
			UNLOCK(tcl_lock, pl_1);
			break;
		case TCL_UNLINK:
retryunlink:
			pl_1 = LOCK(tcl_lock, plstr);
			if (te->te_linkep != NULL) {
				if ((te->te_linkep->te_flg & TCL_BUSY) ||
					(te->te_flg & TCL_BUSY)) {
					/* 
				 	 * putnext in progress between endpts.
					 * It doesnt matter which direction.
					 * retry after dropping the lock.
					 */
					UNLOCK(tcl_lock, pl_1);
					goto retryunlink;
				}
				te->te_linkep->te_linkep = NULL;
				te->te_linkep = NULL;
			}
			UNLOCK(tcl_lock, pl_1);
			break;
		}
		freemsg(mp);
		return(0);
	    case M_PASSFP:
	    {
		queue_t	*destrq;

		destrq = NULL;
		/*
		 * Pass the message along to the destination endpt.
		 */
		STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
		    "tcl_wput _%d_: got M_PASSFP msg",__LINE__);
		pl_1 = LOCK(tcl_lock, plstr);
		if (te->te_linkep != NULL) {
			destrq = te->te_linkep->te_rq;
			ASSERT(destrq != NULL);
			te->te_linkep->te_flg |= TCL_BUSY;
			te->te_linkep->te_refcnt++;
			UNLOCK(tcl_lock, pl_1);
			if (destrq != NULL) {
				putnext(destrq, mp);
				pl_1 = LOCK(tcl_lock, plstr);
				te->te_linkep->te_refcnt--;
				if (te->te_linkep->te_refcnt == 0)
					te->te_linkep->te_flg &= ~TCL_BUSY;
				UNLOCK(tcl_lock, pl_1);
			}
			return(0);
		}
		UNLOCK(tcl_lock, pl_1);
		return(0);
	    }
	    case M_IOCTL:
		/* no ioctl's supported */
		STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
		    "tcl_wput _%d_: got M_IOCTL msg",__LINE__);
		mp->b_datap->db_type = M_IOCNAK;
		qreply(q,mp);
		return(0);
	    case M_FLUSH:
		STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
		    "tcl_wput _%d_: got M_FLUSH msg",__LINE__);
		if (*mp->b_rptr & FLUSHW) {
			flushq(q,FLUSHDATA);
		}
		if (!(*mp->b_rptr & FLUSHR)) {
			freemsg(mp);
		} else {
			*mp->b_rptr &= ~FLUSHW;
			flushq(OTHERQ(q),FLUSHDATA);
			qreply(q,mp);
		}
		return(0);
	    case M_DATA:
		STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
		    "tcl_wput _%d_ fatal: got M_DATA msg",__LINE__);
		tcl_fatal(q,mp);
		return(0);
	    case M_PCPROTO:
		/*
		 *	switch on tpi msg type
		 */
		if (msz < sizeof(prim->type)) {
			STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
			    "tcl_wput _%d_ fatal: bad msg ctl",__LINE__);
			tcl_fatal(q,mp);
			return(-1);
		}
		ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim = (union T_primitives *)mp->b_rptr;

		switch (prim->type) {
		    default:
			STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
			    "tcl_wput _%d_ fatal: bad prim type=%d",__LINE__,prim->type);
			tcl_fatal(q,mp);
			return(-1);
		    case T_INFO_REQ:
			STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
			    "tcl_wput _%d_: got T_INFO_REQ msg",__LINE__);
			tcl_ireq(q,mp);
			return(0);
		}
		/* NOTREACHED */
	    case M_PROTO:
		/*
		 *	switch on tpi msg type
		 */
		if (msz < sizeof(prim->type)) {
			STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
			    "tcl_wput _%d_ fatal: bad control",__LINE__);
			tcl_fatal(q,mp);
			return(-1);
		}
		ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim = (union T_primitives *)mp->b_rptr;
		ASSERT(prim != NULL);
		switch (prim->type) {
		    default:
			STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
			    "tcl_wput _%d_ fatal: bad prim type=%d",__LINE__,prim->type);
			tcl_fatal(q,mp);
			return(-1);
		    case O_T_BIND_REQ:
			STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
			    "tcl_wput _%d_: got O_T_BIND_REQ msg",__LINE__);
			goto jump;
		    case T_BIND_REQ:
			STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
			    "tcl_wput _%d_: got T_BIND_REQ msg",__LINE__);
			goto jump;
		    case T_UNBIND_REQ:
			STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
			    "tcl_wput _%d_: got T_UNBIND_REQ msg",__LINE__);
			goto jump;
		    case T_OPTMGMT_REQ:
			STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
			    "tcl_wput _%d_: got T_OPTMGMT_REQ msg",__LINE__);
jump:
			/*
			 *	if endpt is hosed, do nothing
			 */
			pl_1 = LOCK(tcl_lock, plstr);
			if (te->te_flg & TCL_ZOMBIE) {
				UNLOCK(tcl_lock, pl_1);
				STRLOG(TCL_ID,tcl_min(te),3,SL_TRACE,
				    "tcl_wput _%d_: ZOMBIE",__LINE__);
				freemsg(mp);
				return(-1);
			}
			UNLOCK(tcl_lock, pl_1);
			tcl_ckstate(q,mp);
			return(0);
		    case T_ADDR_REQ:
			STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE, 
			    "tcl_wput _%d%_: got T_ADDR_REQ msg", __LINE__);
			/*
			 *	if endpt is hosed, do nothing
			 */
			pl_1 = LOCK(tcl_lock, plstr);
			if (te->te_flg & TCL_ZOMBIE) {
				UNLOCK(tcl_lock, pl_1);
				STRLOG(TCL_ID,tcl_min(te),3,SL_TRACE,
				    "tcl_wput _%d_: ZOMBIE",__LINE__);
				freemsg(mp);
				return(-1);
			}
			UNLOCK(tcl_lock, pl_1);
			tcl_areq(q, mp);
			return(0);
		    case T_UNITDATA_REQ:
			/*
			 *	if endpt is hosed, do nothing
			 */
			STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
			    "tcl_wput _%d_: got T_UNITDATA_REQ msg",__LINE__);
			pl_1 = LOCK(tcl_lock, plstr);
			if (te->te_flg & TCL_ZOMBIE) {
				UNLOCK(tcl_lock, pl_1);
				STRLOG(TCL_ID,tcl_min(te),3,SL_TRACE,
				    "tcl_wput _%d_: ZOMBIE",__LINE__);
				freemsg(mp);
				return(-1);
			}
			UNLOCK(tcl_lock, plstr);
			(void)LOCK(te->te_lock, plstr);
			if (NEXTSTATE(TE_UNITDATA_REQ,te->te_state) ==
			    TI_BADSTATE) {
				UNLOCK(te->te_lock, pl_1);
				STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
				    "tcl_wput _%d_ fatal: TE_UNITDTA_REQ out of state, current state=%d->127",__LINE__,te->te_state);
				tcl_fatal(q,mp);
				return(-1);
			}
			UNLOCK(te->te_lock, pl_1);
			(void)putq(q,mp);
			return(0);
		}
		/* NOTREACHED */
	}
	/* NOTREACHED */
}


/*
 * STATIC int
 * tcl_wsrv(queue_t *q)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	driver write side service routine
 */
STATIC int
tcl_wsrv(queue_t *q)
{
	tcl_endpt_t			*te;
	mblk_t			*mp;
	union T_primitives	*prim;


	ASSERT(q != NULL);
	te = (tcl_endpt_t *)q->q_ptr;
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
			cmn_err(CE_PANIC, "tcl_wsrv: Incorrect msg type");
			/* NOTREACHED */
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
				cmn_err(CE_PANIC, "tcl_wsrv: Incorrect prim type");
				/* NOTREACHED */
			    case T_UNITDATA_REQ:
				STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
				    "tcl_wsrv _%d_: got T_UNITADTA_REQ msg",__LINE__);
				if (tcl_data(q,mp) != 0) {
					STRLOG(TCL_ID,tcl_min(te),3,SL_TRACE,
					    "tcl_wsrv _%d_: tcl_data() failure",__LINE__);
					return(0); /* flow control or  */
						   /* memory limitations */
				}
				break;
			}
			break;
		}
	}
	return(0);
}



/*
 * STATIC void
 * tcl_okack(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks must be held on entry
 *
 * Description:
 *	handle ok ack
 */
STATIC void
tcl_okack(queue_t *q, mblk_t *mp)
{
	union T_primitives	*prim;
	mblk_t				*mp1;
	long				type;


	ASSERT(q != NULL);
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
		if ((mp1 = allocb(sizeof(struct T_ok_ack),BPRI_HI)) == NULL) {
			STRLOG(TCL_ID,-1,1,SL_TRACE,
			    "tcl_okack _%d_ fatal: allocb() failure",__LINE__);
			tcl_fatal(q,mp);
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
 * tcl_errack(queue_t *q, mblk_t *mp, long tli_err, long unix_err)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	handle error ack
 */
STATIC void
tcl_errack(queue_t *q, mblk_t *mp, long tli_err, long unix_err)
{
	union T_primitives	*prim;
	mblk_t			*mp1;
	long			type;


	ASSERT(q != NULL);
	ASSERT(q->q_ptr != NULL);
	ASSERT(mp != NULL);
	ASSERT((int)(mp->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	/*
	 *	prepare nack msg
	 */
	ASSERT(prim != NULL);
	type = prim->type;
	if ((mp->b_datap->db_lim - mp->b_datap->db_base) < sizeof(struct T_error_ack)) {
		if ((mp1 = allocb(sizeof(struct T_error_ack),BPRI_HI)) == NULL) {
			STRLOG(TCL_ID,-1,1,SL_TRACE,
			    "tcl_errack _%d_ fatal: couldn't allocate msg",__LINE__);
			tcl_fatal(q,mp);
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
 * tcl_fatal(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks must be held on entry
 *
 * Description:
 *	handle fatal condition (endpt is hosed)
 */
STATIC void
tcl_fatal(queue_t *q, mblk_t *mp)
{
	tcl_endpt_t		*te;
	pl_t				pl_1;

	ASSERT(q != NULL);
	ASSERT(mp != NULL);
	te = (tcl_endpt_t *)q->q_ptr;
	ASSERT(te != NULL);
	/*
	 *	prepare err msg
	 */
	tcl_unconnect(te);
	pl_1 = LOCK(tcl_lock, plstr);
	te->te_flg |= TCL_ZOMBIE;
	UNLOCK(tcl_lock, pl_1);
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
 * tcl_flush(queue_t *q)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	flush rd & wr queues
 */
STATIC int
tcl_flush(queue_t *q)
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
	*mp->b_rptr = FLUSHRW;
	/*
	 *	send flush msg
	 */
	putnext(RD(q), mp);
	return(0);
}


/*
 * STATIC void
 * tcl_ckstate(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	q->q_ptr->te_lock must not be held on entry
 *
 * Description:
 *	check interface state and handle event
 */
STATIC void
tcl_ckstate(queue_t *q, mblk_t *mp)
{
	tcl_endpt_t		*te;
	union T_primitives	*prim;
	char			ns;
	pl_t				pl_1;


	te = (tcl_endpt_t *)q->q_ptr;
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
		cmn_err(CE_PANIC, "tcl_ckstate: Incorrect prim type");
		/* NOTREACHED */
	    case O_T_BIND_REQ:
	    case T_BIND_REQ:
		pl_1 = LOCK(te->te_lock, plstr);
		if ((ns = NEXTSTATE(TE_BIND_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock, pl_1);
		tcl_bind(q,mp);
		return;
	    case T_UNBIND_REQ:
		pl_1 = LOCK(te->te_lock, plstr);
		if ((ns = NEXTSTATE(TE_UNBIND_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock, pl_1);
		tcl_unbind(q,mp);
		return;
	    case T_OPTMGMT_REQ:
		pl_1 = LOCK(te->te_lock, plstr);
		if ((ns = NEXTSTATE(TE_OPTMGMT_REQ,te->te_state)) ==
		    TI_BADSTATE) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_ckstate _%d_ errack: tli_err=TOUTSTATE, unix_err=0, state=%d->127",__LINE__,te->te_state);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TOUTSTATE,0);
			return;
		}
		te->te_state = ns;
		UNLOCK(te->te_lock,  pl_1);
		tcl_optmgmt(q,mp);
		return;
	}
	/* NOTREACHED */
}


/*
 * STATIC void
 * tcl_ireq(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 * 	No locking assumptions.
 *
 * Description:
 *	handle info request
 */
STATIC void
tcl_ireq(queue_t *q, mblk_t *mp)
{
	tcl_endpt_t		*te;
	union T_primitives	*prim;
	mblk_t				*mp1;

	te = (tcl_endpt_t *)q->q_ptr;
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	/*
	 *	prepare ack msg
	 */
	if ((mp1 = allocb(sizeof(struct T_info_ack),BPRI_HI)) == NULL) {
		STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
		    "tcl_ireq _%d_ fatal: allocb() failure",__LINE__);
		tcl_fatal(q,mp);
		return;
	}
	mp1->b_datap->db_type = M_PCPROTO;
	mp1->b_wptr = mp1->b_rptr + sizeof(struct T_info_ack);
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp1->b_rptr;
	prim->info_ack.PRIM_type = T_INFO_ACK;
	prim->info_ack.SERV_type = TCL_SERVTYPE;
	prim->info_ack.ADDR_size = TCL_ADDRSZ;
	prim->info_ack.OPT_size = TCL_OPTSZ;
	prim->info_ack.TIDU_size = TCL_TIDUSZ;
	prim->info_ack.TSDU_size = TCL_TSDUSZ;
	prim->info_ack.ETSDU_size = TCL_ETSDUSZ;
	prim->info_ack.CDATA_size = TCL_CDATASZ;
	prim->info_ack.DDATA_size = TCL_DDATASZ;
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
 * tcl_bind(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	tcl_lock must not be held on entry.
 * Description:
 *	handle bind request
 */
STATIC void
tcl_bind(queue_t *q, mblk_t *mp)
{
	tcl_endpt_t		*te;
	union T_primitives	*prim,*prim2;
	tcl_addr_t			addr,*ta;
	mblk_t				*mp1,*mp2;
	struct stroptions		*so;
	int				alen,aoff,msz,msz2;
	pl_t				pl_1;
	int				unixerr;


	te = (tcl_endpt_t *)q->q_ptr;
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(te->te_addr == NULL);
	/*
	 *	set stream head options
	 */
	if ((mp1 = allocb(sizeof(struct stroptions),BPRI_HI)) == NULL) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		tcl_errack(q,mp,TSYSERR,ENOMEM);
		return;
	}
	mp1->b_datap->db_type = M_SETOPTS;
	mp1->b_wptr = mp1->b_rptr + sizeof(struct stroptions);
	/* LINTED pointer alignment */
	so = (struct stroptions *)mp1->b_rptr;
	so->so_flags = SO_MINPSZ|SO_MAXPSZ|SO_HIWAT|SO_LOWAT|SO_WROFF|SO_READOPT;
	so->so_readopt = 0;
	so->so_wroff = 0;
	so->so_minpsz = TCL_MINPSZ;
	so->so_maxpsz = TCL_MAXPSZ;
	so->so_lowat = TCL_LOWAT;
	so->so_hiwat = TCL_HIWAT;
	qreply(q,mp1);
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	if (msz < sizeof(struct T_bind_req)) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_bind _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tcl_errack(q,mp,TSYSERR,EINVAL);
		return;
	}
	alen = prim->bind_req.ADDR_length;
	aoff = prim->bind_req.ADDR_offset;
	if ((alen < 0) || (alen > TCL_ADDRSZ)) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_bind _%d_ errack: tli_err=TBADADDR, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tcl_errack(q,mp,TBADADDR,0);
		return;
	}
	if (alen > 0) {
		if ((aoff < 0) || ((aoff + alen) > msz)) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_bind _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TSYSERR,EINVAL);
			return;
		}
	}
	/*
	 *	negotiate addr
	 */
	unixerr = 0;
	pl_1 = LOCK(tcl_lock, plstr);
	if (alen == 0) {
		ta = tcl_getaddr(TCL_BIND,NULL,&unixerr);
	} else {
		addr.ta_alen = alen;
		addr.ta_abuf = (char *)(mp->b_rptr + aoff);
		addr.ta_ahash = tcl_mkahash(&addr);
		ta = tcl_getaddr(TCL_BIND,&addr,&unixerr);
	}
	if (ta == NULL) {
	    if (unixerr == 0) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		UNLOCK(tcl_lock, pl_1);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tcl_errack(q,mp,TSYSERR,ENOMEM);
		return;
	    } else {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_bind _%d_ errack: tli_err=TADDRBUSY, unix_err=0",__LINE__);
		UNLOCK(tcl_lock, pl_1);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tcl_errack(q,mp,TADDRBUSY,0);
		return;
	    }
	}
	ASSERT(tcl_abuf(ta) != NULL);
	ASSERT(tcl_alen(ta) != 0);	/* may be != alen */
	/*
	 *	prepare ack msg
	 */
	msz2 = sizeof(struct T_bind_ack) + tcl_alen(ta);
	if ((mp2 = allocb(msz2,BPRI_HI)) == NULL) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_bind _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
		kmem_free(tcl_abuf(ta),tcl_alen(ta));
		kmem_free((char *)ta,sizeof(tcl_addr_t));
		UNLOCK(tcl_lock, pl_1);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		(void)tcl_errack(q,mp,TSYSERR,ENOMEM);
		return;
	}
	mp2->b_datap->db_type = M_PCPROTO;
	mp2->b_wptr = mp2->b_rptr + msz2;
	ASSERT((int)(mp2->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim2 = (union T_primitives *)mp2->b_rptr;
	prim2->bind_ack.PRIM_type = T_BIND_ACK;
	prim2->bind_ack.CONIND_number = 0;	/* connectionless */
	prim2->bind_ack.ADDR_offset = sizeof(struct T_bind_ack);
	prim2->bind_ack.ADDR_length = tcl_alen(ta);
	addr.ta_alen = tcl_alen(ta);
	addr.ta_abuf = (char *)(mp2->b_rptr + prim2->bind_ack.ADDR_offset);
	(void)tcl_cpabuf(&addr,ta);	/* cannot fail */
	/*
	 *	do the bind
	 */
	tcl_blink(te,ta);
	UNLOCK(tcl_lock, pl_1);
	freemsg(mp);
	pl_1 = LOCK(te->te_lock, plstr);
	te->te_state = NEXTSTATE(TE_BIND_ACK,te->te_state);
	ASSERT(te->te_state != TI_BADSTATE);
	UNLOCK(te->te_lock, pl_1);
	/*
	 *	send ack msg
	 */
	qreply(q,mp2);
	STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
	    "tcl_bind _%d_: bound",__LINE__);
	return;
}


/*
 * STATIC void
 * tcl_unbind(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks must be held on entry.
 *
 * Description:
 *	handle unbind request
 */
STATIC void
tcl_unbind(queue_t *q, mblk_t *mp)
{
	tcl_endpt_t		*te;
	pl_t				 pl_1;

	te = (tcl_endpt_t *)q->q_ptr;
	/*
	 *	flush queues
	 */
	if (tcl_flush(q) == -1) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_unbind _%d_ errack: tli_err=TSYSERR, unix_err=EIO",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tcl_errack(q,mp,TSYSERR,EIO);
		return;
	}
	/*
	 *	do the unbind
	 */
	pl_1 = LOCK(tcl_lock, plstr);
	tcl_unblink(te);
	UNLOCK(tcl_lock, pl_1);
	pl_1 = LOCK(te->te_lock, plstr);
	te->te_state = NEXTSTATE(TE_OK_ACK1,te->te_state);
	ASSERT(te->te_state != TI_BADSTATE);
	UNLOCK(te->te_lock, pl_1);
	/*
	 *	send ack msg
	 */
	tcl_okack(q,mp);
	STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
	    "tcl_unbind _%d_: unbound",__LINE__);
	return;
}


/*
 * STATIC void
 * tcl_optmgmt(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	The state lock must not held on entry
 *
 * Description:
 *	handle option mgmt request
 */
STATIC void
tcl_optmgmt(queue_t *q, mblk_t *mp)
{
	tcl_endpt_t			*te;
	union T_primitives		*prim,*prim1;
	mblk_t				*mp1;
	int				olen,ooff,msz,msz1,ckopt;
	struct tcl_opt_hdr		*ohdr;
	union tcl_opt			*opt;
	pl_t				 pl_1;


	te = (tcl_endpt_t *)q->q_ptr;
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	if (msz < (sizeof(struct T_optmgmt_req))) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tcl_errack(q,mp,TSYSERR,EINVAL);
		return;
	}
	olen = prim->optmgmt_req.OPT_length;
	ooff = prim->optmgmt_req.OPT_offset;
	/* olen, ooff are validated below */
	/*
	 *	switch on optmgmt request type
	 */
	switch (prim->optmgmt_req.MGMT_flags) {
	    default:
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_optmgmt _%d_ errack: tli_err=TBADFLAG, unix_err=0",__LINE__);
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		tcl_errack(q,mp,TBADFLAG,0);
		return;
	    case T_CHECK:
		/*
		 *	validate opt list
		 */
		if ((olen < 0) || (olen > TCL_OPTSZ)
		||  (ooff < 0) || (ooff + olen > msz)) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TSYSERR,EINVAL);
			return;
		}
		ckopt = tcl_ckopt((char *)(mp->b_rptr+ooff),
					(char *)(mp->b_rptr+ooff+olen));
		if (ckopt & TCL_BADFORMAT) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_optmgmt _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TBADOPT,0);
			return;
		}
		/* re-use msg block: */
		mp->b_datap->db_type = M_PCPROTO;
		prim1 = prim;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		if (ckopt & (TCL_REALOPT|TCL_NOOPOPT)) {
			prim1->optmgmt_ack.MGMT_flags = T_SUCCESS;
		}
		if (ckopt & (TCL_BADTYPE|TCL_BADVALUE)) {
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
		msz1 = sizeof(struct T_optmgmt_ack) + sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_noop);
		if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TSYSERR,ENOMEM);
			return;
		}
		mp1->b_datap->db_type = M_PCPROTO;
		mp1->b_wptr = mp1->b_rptr + msz1;
		ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		prim1 = (union T_primitives *)mp1->b_rptr;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		prim1->optmgmt_ack.MGMT_flags = T_DEFAULT;
		prim1->optmgmt_ack.OPT_length = sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_noop);
		prim1->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
		ASSERT((prim1->optmgmt_ack.OPT_offset)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		ohdr = (struct tcl_opt_hdr *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset);
		ohdr->hdr_thisopt_off = sizeof(struct tcl_opt_hdr);
		ohdr->hdr_nexthdr_off = TCL_OPT_NOHDR;
		ASSERT((ohdr->hdr_thisopt_off)%NBPW == 0);	/* alignment */
		/* LINTED pointer alignment */
		opt = (union tcl_opt *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset + ohdr->hdr_thisopt_off);
		opt->opt_type = TCL_OPT_NOOP;	/* default opt */
		pl_1 = LOCK(te->te_lock, plstr);
		te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
		ASSERT(te->te_state != TI_BADSTATE);
		UNLOCK(te->te_lock, pl_1);
		freemsg(mp);
		qreply(q,mp1);
		return;
	    case T_CURRENT:
		/*
		 *      return current options in effect.
		 *
		 *      Validate options list
		 */
		if (olen == 0) {
			/*
			 *      retrieve default opt
			 */
			msz1 = sizeof(struct T_optmgmt_ack) + sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_noop);
			if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
				STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
				    "tcl_optmgmt _%d_ errack: tli_err=TSYSERR, uknix_err=ENOMEM",__LINE__);
				pl_1 = LOCK(te->te_lock, plstr);
				te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
				ASSERT(te->te_state != TI_BADSTATE);
				UNLOCK(te->te_lock, pl_1);
				tcl_errack(q,mp,TSYSERR,ENOMEM);
				return;
			}
			mp1->b_datap->db_type = M_PCPROTO;
			mp1->b_wptr = mp1->b_rptr + msz1;
			ASSERT((int)(mp1->b_rptr)%NBPW == 0);   /* alignment */
			/* LINTED pointer alignment */
			prim1 = (union T_primitives *)mp1->b_rptr;
			prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
			prim1->optmgmt_ack.MGMT_flags = T_CURRENT;
			prim1->optmgmt_ack.OPT_length = sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_noop);
			prim1->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
			/* LINTED pointer alignment */
			ohdr = (struct tcl_opt_hdr *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset);
			ohdr->hdr_thisopt_off = sizeof(struct tcl_opt_hdr);
			ohdr->hdr_nexthdr_off = TCL_OPT_NOHDR;
			/* LINTED pointer alignment */
			opt = (union tcl_opt *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset + ohdr->hdr_thisopt_off);
			opt->opt_type = TCL_OPT_NOOP;   /* default opt */
			freemsg(mp);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			qreply(q,mp1);
			return;
		}
		if ((olen < 0) || (olen > TCL_OPTSZ)
		||  (ooff < 0) || (ooff + olen > msz)) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TSYSERR,EINVAL);
			return;
		}

		/* re-use msg block: */
		mp->b_datap->db_type = M_PCPROTO;
		prim1 = prim;
		prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		/*
		 *      do the opts
		 */
		/* LINTED pointer alignment */
		for (ohdr = (struct tcl_opt_hdr *)(mp->b_rptr + ooff); ;
			/* LINTED pointer alignment */
			ohdr =(struct tcl_opt_hdr *)(mp->b_rptr + ooff + ohdr->hdr_nexthdr_off)) {
			/* LINTED pointer alignment */
			opt = (union tcl_opt *)(mp->b_rptr + ooff + ohdr->hdr_thisopt_off);
			switch (opt->opt_type) {
			    default:
				/*
				 *+ Covered all options.
				 */
				goto coveredall;
			    case TCL_OPT_NOOP:
				break;
			    case TCL_OPT_GETID:
				pl_1 = LOCK(te->te_lock, plstr);
				opt->opt_getid.getid_flg = te->te_idflg;
				UNLOCK(te->te_lock, pl_1);
				break;
			    case TCL_OPT_UID:
				opt->opt_uid.uid_val = te->te_uid;
				break;
			    case TCL_OPT_GID:
				opt->opt_gid.gid_val = te->te_gid;
				break;
			    case TCL_OPT_RUID:
				opt->opt_ruid.ruid_val = te->te_ruid;
				break;
			    case TCL_OPT_RGID:
				opt->opt_rgid.rgid_val = te->te_rgid;
				break;
			}
			if (ohdr->hdr_nexthdr_off == TCL_OPT_NOHDR) {
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
			msz1 = sizeof(struct T_optmgmt_ack) + sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_noop);
			if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
				STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
				    "tcl_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=ENOMEM",__LINE__);
				pl_1 = LOCK(te->te_lock, plstr);
				te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
				ASSERT(te->te_state != TI_BADSTATE);
				UNLOCK(te->te_lock, pl_1);
				tcl_errack(q,mp,TSYSERR,ENOMEM);
				return;
			}
			mp1->b_datap->db_type = M_PCPROTO;
			mp1->b_wptr = mp1->b_rptr + msz1;
			/* LINTED pointer alignment */
			prim1 = (union T_primitives *)mp1->b_rptr;
			prim1->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
			prim1->optmgmt_ack.MGMT_flags = T_NEGOTIATE;
			prim1->optmgmt_ack.OPT_length = sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_noop);
			prim1->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
			/* LINTED pointer alignment */
			ohdr = (struct tcl_opt_hdr *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset);
			ohdr->hdr_thisopt_off = sizeof(struct tcl_opt_hdr);
			ohdr->hdr_nexthdr_off = TCL_OPT_NOHDR;
			/* LINTED pointer alignment */
			opt = (union tcl_opt *)(mp1->b_rptr + prim1->optmgmt_ack.OPT_offset + ohdr->hdr_thisopt_off);
			opt->opt_type = TCL_OPT_NOOP;	/* default opt */
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_OPTMGMT_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			freemsg(mp);
			qreply(q,mp1);
			return;
		}
		/*
		 *	validate opt list
		 */
		if ((olen < 0) || (olen > TCL_OPTSZ)
		||  (ooff < 0) || (ooff + olen > msz)) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_optmgmt _%d_ errack: tli_err=TSYSERR, unix_err=EINVAL",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TSYSERR,EINVAL);
			return;
		}
		ckopt = tcl_ckopt((char *)(mp->b_rptr+ooff),
					(char *)(mp->b_rptr+ooff+olen));
		if (ckopt & (TCL_BADFORMAT|TCL_BADTYPE|TCL_BADVALUE)) {
			STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
			    "tcl_optmgmt _%d_ errack: tli_err=TBADOPT, unix_err=0",__LINE__);
			pl_1 = LOCK(te->te_lock, plstr);
			te->te_state = NEXTSTATE(TE_ERROR_ACK,te->te_state);
			ASSERT(te->te_state != TI_BADSTATE);
			UNLOCK(te->te_lock, pl_1);
			tcl_errack(q,mp,TBADOPT,0);
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
		for (ohdr = (struct tcl_opt_hdr *)(mp->b_rptr + ooff); ;
			/* LINTED pointer alignment */
			ohdr = (struct tcl_opt_hdr *)(mp->b_rptr + ooff + ohdr->hdr_nexthdr_off)) {
			/* LINTED pointer alignment */
			opt = (union tcl_opt *)(mp->b_rptr + ooff + ohdr->hdr_thisopt_off);
			switch (opt->opt_type) {
			    default:
				/*
				 *+ Internal error
				 */
				cmn_err(CE_PANIC, "tcl_optmgmt: Invalid options");
				/* NOTREACHED */
			    case TCL_OPT_NOOP:
				break;
			    case TCL_OPT_SETID:
				ASSERT((opt->opt_setid.setid_flg & ~TCL_IDFLG_ALL) == 0);
				pl_1 = LOCK(te->te_lock, plstr);
				te->te_idflg = opt->opt_setid.setid_flg;
				UNLOCK(te->te_lock, pl_1);
				break;
			    case TCL_OPT_GETID:
				opt->opt_getid.getid_flg = te->te_idflg;
				break;
			    case TCL_OPT_UID:
				opt->opt_uid.uid_val = te->te_uid;
				break;
			    case TCL_OPT_GID:
				opt->opt_gid.gid_val = te->te_gid;
				break;
			    case TCL_OPT_RUID:
				opt->opt_ruid.ruid_val = te->te_ruid;
				break;
			    case TCL_OPT_RGID:
				opt->opt_rgid.rgid_val = te->te_rgid;
				break;
			}
			if (ohdr->hdr_nexthdr_off == TCL_OPT_NOHDR) {
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
 * tcl_unconnect(tcl_endpt_t *te)
 *
 * Calling/Exit State:
 *	No locks must be held on entry
 *
 * Description:
 *	cleanup utility routine
 */
STATIC void
tcl_unconnect(tcl_endpt_t *te)
{
	queue_t		*q;
	pl_t		 pl_1;


	ASSERT(te != NULL);
	q = te->te_rq;
	ASSERT(q != NULL);
	pl_1 = LOCK(te->te_lock, plstr);
	te->te_state = TI_BADSTATE;
	UNLOCK(te->te_lock, pl_1);
retry:
	pl_1 = LOCK(tcl_lock, plstr);
	if (te->te_linkep != NULL) {
	    if ((te->te_flg & TCL_BUSY) ||
		(te->te_linkep->te_flg & TCL_BUSY)) {
		/*
		 * putnext in progress
		 */
		UNLOCK(tcl_lock, pl_1);
		goto retry;
	    } else {
		te->te_linkep->te_linkep = NULL;
		te->te_linkep = NULL;
	    }
	}
	UNLOCK(tcl_lock, pl_1);
	flushq(q,FLUSHALL);
	return;
}


/*
 * STATIC void
 * tcl_uderr(queue_t *q, mblk_t *mp, long err)
 *
 * Calling/Exit State:
 * 	No locking assumptions.
 *
 * Description:
 *	handle unitdata error
 */
STATIC void
tcl_uderr(queue_t *q, mblk_t *mp, long err)
{
	tcl_endpt_t		*te;
	union T_primitives	*prim,*prim1;
	long				msz1;
	mblk_t				*mp1;
	tcl_addr_t			addr,addr1;
	int				alen,olen;
#ifndef	NO_UDERR
	pl_t				pl_1;

	te = (tcl_endpt_t *)q->q_ptr;
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim->type == T_UNITDATA_REQ);
	/*
	 *	prepare the indication msg
	 */
	msz1 = sizeof(struct T_uderror_ind);
	alen = prim->unitdata_req.DEST_length;
	if (alen > 0) {
		msz1 += alen;
	}
	olen = prim->unitdata_req.OPT_length;
	if (olen > 0) {
		msz1 += olen + NBPW;	/* allow for alignment */
	}
	if (msz1 > TCL_TIDUSZ) {
		STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
		    "tcl_uderr _%d_ fatal: msg too big",__LINE__);
		tcl_fatal(q,mp);
		return;
	}
	if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
		STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
		    "tcl_uderr _%d_ fatal: allocb() failure",__LINE__);
		tcl_fatal(q,mp);
		return;
	}
	mp1->b_datap->db_type = M_PROTO;
	mp1->b_wptr = mp1->b_rptr + msz1;
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim1 = (union T_primitives *)mp1->b_rptr;
	prim1->uderror_ind.PRIM_type = T_UDERROR_IND;
	prim1->uderror_ind.ERROR_type = err;
	prim1->uderror_ind.DEST_length = alen;
	if (alen <= 0) {
		prim1->uderror_ind.DEST_offset = 0;
	} else {
		prim1->uderror_ind.DEST_offset = sizeof(struct T_uderror_ind);
		addr.ta_alen = alen;
		addr.ta_abuf = (char *)(mp->b_rptr + prim->unitdata_req.DEST_offset);
		addr1.ta_alen = alen;
		addr1.ta_abuf = (char *)(mp1->b_rptr + prim1->uderror_ind.DEST_offset);
		(void)tcl_cpabuf(&addr1,&addr);		/* cannot fail */
	}
	prim1->uderror_ind.OPT_length = olen;
	if (olen <= 0) {
		prim1->uderror_ind.OPT_offset = 0;
	} else {
		prim1->uderror_ind.OPT_offset = sizeof(struct T_uderror_ind) + prim1->uderror_ind.DEST_length;
		/* blindly copy the option list, retaining alignment */
		while ((prim1->uderror_ind.OPT_offset)%NBPW != (prim->unitdata_req.OPT_offset)%NBPW) {
			prim1->uderror_ind.OPT_offset += 1;	/* alignment */
		}
		(void)bcopy(mp->b_rptr+prim->unitdata_req.OPT_offset,mp1->b_rptr+prim1->uderror_ind.OPT_offset,olen);
	}
	freemsg(mp);
	/*
	 *	send the indication msg
	 */
	pl_1 = LOCK(te->te_lock, plstr);
	te->te_state = NEXTSTATE(TE_UDERROR_IND,te->te_state);
	ASSERT(te->te_state != TI_BADSTATE);
	UNLOCK(te->te_lock, pl_1);
	putnext(RD(q), mp1);
	return;
#else
	freemsg(mp);
	return;
#endif
}


/*
 * STATIC int
 * tcl_data(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	handle unitdata request
 */
STATIC int
tcl_data(queue_t *q, mblk_t *mp)
{
	tcl_endpt_t		*te,*te1;
	mblk_t			*mp1;
	union T_primitives	*prim,*prim1;
	tcl_addr_t		*ta;
	tcl_addr_t		addr;
	int			alen,aoff,olen,olen1,ooff,msz,msz1;
	long			idflg;
	pl_t			 pl_1;
	pl_t			 pl_2;
	queue_t			*rq;
	int 			unixerr;

	te = (tcl_endpt_t *)q->q_ptr;		/* te = sender */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim->type == T_UNITDATA_REQ);
	/*
	 *	validate the request
	 */
	msz = mp->b_wptr - mp->b_rptr;
	alen = prim->unitdata_req.DEST_length;
	aoff = prim->unitdata_req.DEST_offset;
	olen = prim->unitdata_req.OPT_length;
	ooff = prim->unitdata_req.OPT_offset;
	if ((msz < sizeof(struct T_unitdata_req))
	||  ((alen > 0) && ((aoff + alen) > msz))
	||  ((olen > 0) && ((ooff + olen) > msz))) {
		STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
		    "tcl_data _%d_ fatal: bad control info",__LINE__);
		tcl_uderr(q,mp,TCL_BADADDR);
		return(0);
	}
	if ((alen <= 0) || (alen > TCL_ADDRSZ)) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_data _%d_ uderr: bad dest",__LINE__);
		tcl_uderr(q,mp,TCL_BADADDR);
		return(0);
	}
	if (aoff < 0) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_data _%d_ uderr: bad dest",__LINE__);
		tcl_uderr(q,mp,TCL_BADADDR);
		return(0);
	}
	if (olen != 0) {
		/*
		 *	no opts supported here
		 */
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_data _%d_ uderr: bad opt",__LINE__);
		tcl_uderr(q,mp,TCL_BADOPT);
		return(0);
	}
	if (msgdsize(mp) > TCL_TSDUSZ) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_data _%d_ uderr: bad opt",__LINE__);
		tcl_uderr(q,mp,TBADDATA);
		return(0);
	}
	/*
	 *	get destination endpoint
	 */
	unixerr = 0;
	addr.ta_alen = alen;
	addr.ta_abuf = (char *)(mp->b_rptr + aoff);
	pl_1 = LOCK(tcl_lock, plstr);
	addr.ta_ahash = tcl_mkahash(&addr);
	ta = tcl_getaddr(TCL_DEST,&addr,&unixerr);
	if (ta == NULL) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_data _%d_ uderr: bad dest",__LINE__);
		UNLOCK(tcl_lock, pl_1);
		tcl_uderr(q,mp,TCL_NOPEER);
		return(0);
	}
	te1 = ta->ta_blist;		/* te1 = receiver */
	ASSERT(te1 != NULL);
	pl_2 = LOCK(te1->te_lock, plstr);
	if (te1->te_state != TS_IDLE) {
		STRLOG(TCL_ID,tcl_min(te),2,SL_TRACE,
		    "tcl_data _%d_ uderr: dest busy",__LINE__);
		UNLOCK(te1->te_lock, pl_2);
		UNLOCK(tcl_lock, pl_1);
		tcl_uderr(q,mp,TCL_PEERBADSTATE);
		return(0);
	}
	if (!canputnext(te1->te_rq)) {
		STRLOG(TCL_ID,tcl_min(te),3,SL_TRACE,
		    "tcl_data _%d_: canputnext() failure",__LINE__);
		UNLOCK(te1->te_lock, pl_2);
		UNLOCK(tcl_lock, pl_1);
		freemsg(mp); /*Very reasonable thing to do for datagram*/
		return(0);
	}
	/*
	 *	prepare indication msg
	 */
	msz1 = sizeof(struct T_unitdata_ind) + tcl_alen(te->te_addr);
	idflg = te1->te_idflg;
	ASSERT((idflg & ~TCL_IDFLG_ALL) == 0);
	if (idflg != 0) {
		olen1 = 0;
		if (idflg & TCL_IDFLG_UID) {
			olen1 += sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_uid);
		}
		if (idflg & TCL_IDFLG_GID) {
			olen1 += sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_gid);
		}
		if (idflg & TCL_IDFLG_RUID) {
			olen1 += sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_ruid);
		}
		if (idflg & TCL_IDFLG_RGID) {
			olen1 += sizeof(struct tcl_opt_hdr) + sizeof(struct tcl_opt_rgid);
		}
		msz1 += olen1 + NBPW;	/* allow for aligment */
	}
	if (msz1 >= TCL_TIDUSZ) {
		STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
		    "tcl_data _%d_ fatal: msg too big",__LINE__);
		UNLOCK(te1->te_lock, pl_2);
		UNLOCK(tcl_lock, pl_1);
		tcl_fatal(q,mp);
		return(0);
	}
	if ((mp1 = allocb(msz1,BPRI_HI)) == NULL) {
		STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
		    "tcl_data _%d_: allocb() failure",__LINE__);
		UNLOCK(te1->te_lock, pl_2);
		UNLOCK(tcl_lock, pl_1);
		tcl_fatal(q,mp);
		return(0);
	}
	mp1->b_datap->db_type = M_PROTO;
	mp1->b_wptr = mp1->b_rptr + msz1;
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);	/* alignment */
	/* LINTED pointer alignment */
	prim1 = (union T_primitives *)mp1->b_rptr;
	prim1->type = T_UNITDATA_IND;
	prim1->unitdata_ind.SRC_length = tcl_alen(te->te_addr);
	prim1->unitdata_ind.SRC_offset = sizeof(struct T_unitdata_ind);
	addr.ta_alen = tcl_alen(te->te_addr);
	addr.ta_abuf = (char *)(mp1->b_rptr + prim1->unitdata_ind.SRC_offset);
	(void)tcl_cpabuf(&addr,te->te_addr);	/* cannot fail */
	if (idflg == 0) {
		prim1->unitdata_ind.OPT_offset = 0;
		prim1->unitdata_ind.OPT_length = 0;
	} else {
		prim1->unitdata_ind.OPT_offset = prim1->unitdata_ind.SRC_offset + prim1->unitdata_ind.SRC_length;
		while ((prim1->unitdata_ind.OPT_offset)%NBPW != 0) {
			prim1->unitdata_ind.OPT_offset += 1;	/* alignment */
		}
		prim1->unitdata_ind.OPT_length = olen1;
		tcl_wropt(idflg,te,(char *)(mp1->b_rptr+prim1->unitdata_ind.OPT_offset));
		ASSERT((tcl_ckopt((char *)(mp1->b_rptr+prim1->unitdata_ind.OPT_offset),
		    (char *)(mp1->b_rptr+prim1->unitdata_ind.OPT_offset+prim1->unitdata_ind.OPT_length))
		    & (TCL_BADFORMAT|TCL_BADTYPE|TCL_BADVALUE)) == 0);
	}
	/*
	 *	relink data blocks from mp to mp1
	 */
	/* following is faster than (void)linkb(mp1,unlinkb(mp)); */
	mp1->b_cont = mp->b_cont;
	mp->b_cont = NULL;
	/*
	 *	send indication msg
	 */
	STRLOG(TCL_ID,tcl_min(te),4,SL_TRACE,
	    "tcl_data _%d_: sending T_UNITDATA_IND",__LINE__);
	te1->te_state = NEXTSTATE(TE_UNITDATA_IND,te1->te_state);
	ASSERT(te1->te_state != TI_BADSTATE);
	rq = te1->te_rq;
	te1->te_flg |= TCL_BUSY;
	te1->te_refcnt++;
	UNLOCK(te1->te_lock, pl_2);
	UNLOCK(tcl_lock, pl_1);
	(void)putnext(rq,mp1);
	freeb(mp);
	pl_1 = LOCK(tcl_lock, plstr);
	te1->te_refcnt--;
	if (te1->te_refcnt == 0)
		te1->te_flg &= ~TCL_BUSY;
	UNLOCK(tcl_lock, pl_1);
	return(0);
}

/*
 * STATIC void
 * tcl_areq(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	q->ptr->te_lock must not be held on entry
 *
 * Description:
 *	Handle the "get protocol addresses" request. Since this a 
 *	connectionless service provider REMADDR details are not
 *	available.
 */
STATIC void
tcl_areq(queue_t *q, mblk_t *mp)
{

	union T_primitives		*prim;
	tcl_endpt_t			*te;
	mblk_t				*mp1;
	int				 msz1;
	tcl_addr_t			 addr;
	pl_t				 pl_1;

	te = (tcl_endpt_t *)q->q_ptr;		/* te = sender */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp->b_rptr;
	ASSERT(prim->type == T_ADDR_REQ);

	/*
	 * prepare ack msg
	 */
	pl_1 = LOCK(te->te_lock, plstr);
	if (te->te_state == TS_UNBND) {
		/*
		 * endpt is not bound
		 */
		msz1 = sizeof(struct T_addr_ack);
	} else {
		/*
		 * endpt is bound...
		 */
	    ASSERT(te->te_addr != NULL);
	    msz1 = (sizeof(struct T_addr_ack) + tcl_alen(te->te_addr));
	}

	if ((mp1 = allocb(msz1, BPRI_HI)) == NULL) {
		STRLOG(TCL_ID,tcl_min(te),1,SL_TRACE,
		    "tcl_areq _%d_ fatal: allocb() failure",__LINE__);
		UNLOCK(te->te_lock, pl_1);
		tcl_fatal(q,mp);
		return;
	}
	mp1->b_datap->db_type = M_PCPROTO;
	mp1->b_wptr = mp1->b_rptr + sizeof(struct T_addr_ack);
	ASSERT((int)(mp1->b_rptr)%NBPW == 0);   /* alignment */
	/* LINTED pointer alignment */
	prim = (union T_primitives *)mp1->b_rptr;
	prim->addr_ack.PRIM_type = T_ADDR_ACK;
	if (te->te_state != TS_UNBND)   {
		/*
		 * endpt is bound.....
		 */
		UNLOCK(te->te_lock, pl_1);
		ASSERT(te->te_addr != NULL);
		prim->addr_ack.LOCADDR_length = tcl_alen(te->te_addr);
		prim->addr_ack.LOCADDR_offset = sizeof(struct T_addr_ack);
		addr.ta_abuf = (char *)mp1->b_wptr;
		addr.ta_alen = tcl_alen(te->te_addr);
		(void)tcl_cpabuf(&addr, te->te_addr);	/* Local addr */
		mp1->b_wptr = mp1->b_wptr + tcl_alen(te->te_addr);
		prim->addr_ack.REMADDR_length = 0;
		prim->addr_ack.REMADDR_offset = 0;
	} else  {
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
	 * send ack msg
	 */
	qreply(q,mp1);
	freemsg(mp);
	return;
}

/*
 * STATIC void
 * tcl_link(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	tcl_lock must be held on entry.
 *
 * Description:
 * 	This routine establishes a link with destination endpoint
 *	so that M_PASSFP messages can be sent across.
 *
 */
STATIC void
tcl_link(queue_t *q, mblk_t *mp)
{
	tcl_addr_t		 addr;
	tcl_addr_t		*ta;
	struct tcl_sictl	*tcl_sictl;
	tcl_endpt_t		*te, *te1;

	/* LINTED pointer alignment */
	tcl_sictl = (struct tcl_sictl *)mp->b_rptr;
	te = (tcl_endpt_t *)q->q_ptr;

	addr.ta_alen = tcl_sictl->ADDR_len;
	addr.ta_abuf = (caddr_t)(mp->b_rptr + tcl_sictl->ADDR_offset);
	addr.ta_ahash = tcl_mkahash(&addr);

	if ((ta = tcl_getaddr(TCL_DEST, &addr, 0)) != NULL) {
		/*
		 * get the destination endpt and set destination
	 	 * endpt ptr in te->te_linkep.
		 */
		te1 = ta->ta_blist;
		ASSERT(te1 != NULL);

		/*
		 * If no prior link to another endpt
 		 * then establish link else ignore request.
		 */
		if (te->te_linkep == NULL) {
			te->te_linkep = te1;
			te1->te_linkep = te;
		}
	}
	return;
}
