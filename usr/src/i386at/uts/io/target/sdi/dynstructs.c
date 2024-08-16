/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/target/sdi/dynstructs.c	1.16"
#ident	"$Header: $"

#include <io/target/sdi/dynstructs.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>	/* must come last */

#ifdef PDI_SVR42
STATIC void cleanone(struct head *, struct jdata *);
#endif

#ifndef PDI_SVR42
/* sdi_get(), sdi_free() supplied for compatibility */

/*
 * struct jpool *
 * sdi_get(struct head *headp, int flag)
 *	Get jpool struct.
 *
 * Calling/Exit State:
 *	None.
 */
struct jpool *
sdi_get(struct head *headp, int flag)
{
	return (struct jpool *)kmem_zalloc(headp->f_isize, flag);
}

/*
 * void
 * sdi_free(struct head *headp, struct jpool *jp)
 *	Free jpool struct.
 *
 * Calling/Exit State:
 *	None.
 */
void
sdi_free(struct head *headp, struct jpool *jp)
{
	kmem_free(jp, headp->f_isize);
	return;
}

#else /* PDI_SVR42 */

/*
 * void
 * sdi_free(struct head *headp, struct jpool *jp)
 *
 * Calling/Exit State:
 *	None
 */
void
sdi_free(struct head *headp, struct jpool *jp)
{
	struct	jpool	*freelistp;
	pl_t	prevpl;

	freelistp = headp->f_freelist;

	prevpl = spldisk();
	if (POOL_TODATA(jp)->jd_inuse < headp->f_idmin) {
		INS_FREETAIL(freelistp, jp);
	} else {
		INS_FREEHEAD(freelistp, jp);
	}
	splx(prevpl);

	drv_getparm(LBOLT, (ulong *)&POOL_TODATA(jp)->jd_lastuse);
	if (headp->f_flag & POOLWANT) {
		wakeup((caddr_t)headp);
		headp->f_flag &= ~POOLWANT;
	}
	else {
		prevpl = spldisk();
		/* If multiple clusters allocated see if we can free any */
		if (headp->f_curr > headp->f_inum)
			cleanone(headp, jp->j_data);
		splx(prevpl);
	}
	return;
}

/*
 * void
 * sdi_poolinit(struct head *headp)
 *
 * Calling/Exit State:
 *	None
 */
void
sdi_poolinit(struct head *headp)
{
	int	asize;
	int	isize;
	int	inum;
	int	frag;

	isize = headp->f_isize;
	asize = headp->f_maxpages * PAGESIZE;
	inum = (asize - sizeof (struct jdata))/ isize;
	frag = asize - (sizeof (struct jdata) + inum * isize);

	/*
	 * The head contents should be zero'ed out in the
	 * filesystem dependent code and then initialized
	 * with filesystem specific data.
	 */
	headp->f_asize = asize;
	headp->f_inum = (short) inum;
	headp->f_idmin = (inum*115)/100 - inum; /* 15% */
	headp->f_frag = (short) frag;
	headp->f_jdata.jd_next = headp->f_jdata.jd_prev = &headp->f_jdata;
	headp->f_jdata.jd_head = headp;

	/* To prevent it from being selected for deallocation */
	headp->f_jdata.jd_inuse = 0x7fff;

	ASSERT(headp->f_frag >= 0);
#ifdef DEBUG
	cmn_err(CE_CONT,
		"freel 0x%x, isize %d, asize %d, inum %d, idmin, %d frag %d\n",
		headp->f_freelist, headp->f_isize, headp->f_asize, 
		headp->f_inum, headp->f_idmin, headp->f_frag);
#endif

}

/*
 * STATIC struct jdata *
 * poolalloc(struct head *headp)
 *
 * Calling/Exit State:
 *	None
 */
STATIC struct jdata *
poolalloc(struct head *headp)
{
	struct	jdata	*jdatap;
	int		asize;
	int		inum;

	asize = headp->f_asize;
	inum = headp->f_inum;

	if ((jdatap  = (void *)kmem_zalloc(asize, KM_NOSLEEP)) == NULL) {
		return (NULL);
	}

	drv_getparm(LBOLT, (ulong *)&jdatap->jd_lastuse);
	jdatap->jd_total = (short) inum;
	/* jd_inuse is set to inum since it gets decremented as structs */
	/* added to freelist.						*/
	jdatap->jd_inuse = (short) inum;

	return(jdatap);
}


/*
 * STATIC void
 * cleanone(struct head *headp, struct jdata *start_jdatap)
 *	cleanone looks for a pool which can be deallocated. The pool must have
 *	been around longer than deallocate time and have no structs in use.
 *	This routine is protected by its caller 
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
cleanone(struct head *headp, struct jdata *start_jdatap)
{
	struct	jdata	*jdatap;
	struct	jdata	*end_jdatap;
	clock_t		curtime;
	int	poolfree();

	if (start_jdatap == NULL) {
		end_jdatap = jdatap = headp->f_jdata.jd_next;
		ASSERT(jdatap->jd_prev == &headp->f_jdata);
	} else {
		end_jdatap = jdatap = start_jdatap;
	}
	
	if (jdatap == &headp->f_jdata)
		return;

	/*
	 * Here we want to select only one pool for deallocation.
	 * During our selection we do not sleep.  Thus all the
	 * pointers (forward and back) that we are currently looking 
	 * at are valid.
	 */

	drv_getparm(LBOLT, (ulong *)&curtime);
	do {
		if ((jdatap->jd_inuse == 0) &&
		   ((jdatap->jd_lastuse + MIN_WAITTIME) < curtime)) {
			poolfree(jdatap);
			break;
		}
		jdatap = jdatap->jd_next;
	} while (jdatap != end_jdatap);
}

/*
 * STATIC int
 * poolfree(struct jdata *jdatap)
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
poolfree(struct jdata *jdatap)
{
	struct	head	*headp;
	char		*njp;
	int 		i;

	headp = jdatap->jd_head;
	ASSERT(jdatap != &(jdatap->jd_head)->f_jdata);
	if (jdatap == &(jdatap->jd_head)->f_jdata)
		return(0);

	/* take structs off freelist */
	njp = (char *)jdatap + sizeof (struct jdata);
	for (i = 0; i < jdatap->jd_total; i++) {
		RM_FREELIST(njp);
		njp += headp->f_isize;
	}
	/*
	 * Remove the pool from the linked list of pools
	 */
	RM_POOL(jdatap);
	kmem_free(jdatap, headp->f_asize);
	headp->f_curr -= headp->f_inum;
	return(0);
}

/*
 * struct jpool *
 * sdi_get(struct head *headp, int flag)
 *
 * Calling/Exit State:
 *	None
 */
struct jpool *
sdi_get(struct head *headp, int flag)
{
	struct	jpool	*freep;
	struct	jpool	*jp;
	struct	jdata	*save_jdata;
	char		*njp;
	struct	jdata	*njdatap;
	pl_t		prevpl;
	int		i;

	prevpl = spldisk(); 
	freep = headp->f_freelist;
retry:
	if ((jp = freep->j_ff) != freep) {
		ASSERT(jp->j_fb->j_ff == jp);
		ASSERT(jp->j_ff->j_fb == jp);
		RM_FREELIST(jp);
		splx(prevpl);
		save_jdata = jp->j_data;
		bzero((caddr_t)jp, (size_t)headp->f_isize);
		jp->j_data = save_jdata;
		return(jp);
	}

	/* Allocate space for an additional set of inodes */
	njdatap = poolalloc(headp);
	if (njdatap == NULL) {
		if ( flag == KM_NOSLEEP )
			return (struct jpool *)0;	/* shrug */
		headp->f_flag |= POOLWANT;
		sleep((caddr_t)headp,PRIBIO);
		goto retry;
	}

	/* Link the inode pool to the filesystem head */
	njdatap->jd_head = headp;
	INS_POOLHEAD(&headp->f_jdata, njdatap);
	headp->f_curr += njdatap->jd_total;

	/* Update inuse count so the INS_FREEHEAD() macro can decrement later */
	njp = (char *)njdatap + sizeof (struct jdata);

	for (i = 0; i < njdatap->jd_total; i++) {
		POOL_TODATA(njp) = njdatap;
		INS_FREEHEAD(freep, njp);
		njp += headp->f_isize;
	}
	goto retry;
}

#ifdef DEBUG
/*
 * void
 * pooldump(struct head *headp)
 *
 * Calling/Exit State:
 *	None
 */
void
pooldump(struct head *headp)
{

	struct	jdata	*jdata;
	
	cmn_err(CE_CONT,
	"freel 0x%x, isize %d, asize %d, inum %d, idmin %d curr %d frag %d\n",
		headp->f_freelist, headp->f_isize, headp->f_asize, 
		headp->f_inum, headp->f_idmin, headp->f_curr, headp->f_frag);

	jdata = headp->f_jdata.jd_next;
	while (jdata != &headp->f_jdata) {
		cmn_err(CE_CONT,
			"jdata 0x%x, inuse %d\n", jdata, jdata->jd_inuse);
			jdata = jdata->jd_next;
	}
}
#endif /* DEBUG */

#endif /* PDI_SVR42 */
