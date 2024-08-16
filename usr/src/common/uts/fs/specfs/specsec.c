/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/specfs/specsec.c	1.11"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/lwp.h>
#include <proc/proc_hier.h>
#include <util/plocal.h>
#include <svc/systm.h>
#include <mem/kmem.h>
#include <util/sysmacros.h>
#include <io/conf.h>
#include <fs/vnode.h>
#include <fs/specfs/snode.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <acc/mac/mac.h>
#include <fs/specfs/devmac.h>

/* 
 * this file contains all the routines used by the
 * enhanced  security system calls and other
 * calls within SPECFS to perform security access checks 
 */


/*
 * void
 * devsec_dcidup(snode_t *sp, snode_t *tsp)
 *
 * Calling/Exit State:
 *	The common snode of the sp is locked shared on
 * 	entry and remains locked at exit.
 *
 * Description:
 *	This routine is called to copy the security attributes
 *	from one snode to another. It is assumed that target sp
 *	security pointer is null. This routine is called
 *	in spec_open when a new clone is created. 
 *	It increments the reference count on the snode and its common.
 */
void
devsec_dcidup(snode_t *sp, snode_t *tsp)
{
	struct devmac *ksecp;

	tsp->s_dstate = sp->s_dstate;
	tsp->s_dmode = sp->s_dmode;
	tsp->s_secflag = sp->s_secflag;
	if (sp->s_dsecp != NULL) {
		ksecp = kmem_zalloc(sizeof(*ksecp), KM_SLEEP);
		*ksecp = *(sp->s_dsecp);
		VN_FIRMHOLD(STOV(tsp));
		VN_HOLD(tsp->s_commonvp);
		tsp->s_dsecp = ksecp;
		tsp->s_dsecp->d_relflag = DEV_LASTCLOSE;
	}
}

/*
 * vnode_t *
 * devsec_cloneopen(vnode_t *vp, dev_t newdev, vtype_t type)
 *
 * Calling/Exit State:
 *	The common snode of vp is locked shared on entry and
 *	remains locked at exit.
 *
 * Description:
 *	This routine is called in spec_open to handle clone devices.
 *	In addition to calling makespecvp to create a new snode,
 *	the routine also copies the security attributes of a cloneable device
 *	to the newly created clone
 */

vnode_t *
devsec_cloneopen(vnode_t *vp, dev_t newdev, vtype_t type)
{
	vnode_t *newvp;
	snode_t *sp = VTOS(vp);
	snode_t *newsp;

	if ((newvp = makespecvp(newdev, type)) == NULL)
		return NULL;
	/* copy information to the newly created clone */
	newvp->v_lid = vp->v_lid;
	newsp = VTOS(newvp);
	devsec_dcidup(sp, newsp);
	return newvp;
}

/*
 * void
 * devset_state(snode_t *sp)
 *
 * Calling/Exit State:
 *	The snode rwlock is held exclusive on entry and
 *	remains locked at exit.
 *
 * Description:
 *	This routine initialize the state of a device by populating
 *	the snode s_dstate field.
 */

void
devset_state(snode_t *sp)
{
	if (sp->s_secflag & D_INITPUB)	
		sp->s_dstate = DEV_PUBLIC;
	else
		sp->s_dstate = DEV_PRIVATE;
}

/*
 * void 
 * devsec_dcifree(snode_t *sp)
 *
 * Calling/Exit State:
 *	The snode rwlock is held exclusive on entry and remains
 *	locked at exit.
 *
 * Description:
 *	This routine frees the security attributes of a device, and
 *	resets the device attributes to system settings. It decrements
 *	the reference count on the the vnode inside the snode and the
 *	vnode inside its common snode.
 *	It expects sp->s_dsecp not to be NULL
 */
void 
devsec_dcifree(snode_t *sp)
{
	struct devmac *locbufp;
	vnode_t *cvp = sp->s_commonvp;

	ASSERT(sp->s_dsecp != NULL);

	locbufp = sp->s_dsecp;
	sp->s_dsecp = NULL;
	kmem_free(locbufp, sizeof(*locbufp));
	devset_state(sp);
	devset_state(VTOS(cvp));
	VN_FIRMRELE(STOV(sp));
	VN_RELE(cvp);
}

/*
 * void
 * devsec_close(snode_t *sp)
 *
 * Calling/Exit State:
 *	The snode rwlock is locked exclusive on entry and
 *	remains locked at exit.
 * 
 * Description:
 *	This routine is called on the last close of a device. If the security
 *	attributes were set to last close, then they have to be released.
 */

void
devsec_close(snode_t *sp)
{
	if (REL_FLAG(sp) == DEV_LASTCLOSE) 
		devsec_dcifree(sp);
}


/*
 * void
 * devsec_dcicreat(snode_t *sp)
 *
 * Calling/Exit State:
 *	The snode rwlock is locked exclusive on entry and
 *	remains locked at exit.
 *
 * Description:
 *	This routine creates an new device security structure and
 *	increments the reference count on the snode and its common.
 *	it assumes the snode security pointer is null.
 */
void
devsec_dcicreat(snode_t *sp)
{
	ASSERT(sp->s_dsecp == NULL);

	sp->s_dsecp = kmem_zalloc(sizeof(struct devmac), KM_SLEEP);
	VN_FIRMHOLD(STOV(sp));
	VN_HOLD((sp)->s_commonvp);
}
