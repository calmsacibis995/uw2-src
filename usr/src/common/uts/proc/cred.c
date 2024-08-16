/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/cred.c	1.20"
#ident  "$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#include <acc/audit/audit.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/uidquota.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>


/*
 * TODO:
 *	1) Need to finalize the statistics gathering macros.
 *	   For now, these are aliased out.
 */ 

#define	CRST_ACTIVE()		/* one more cred in use; stub for now */
#define	CRST_INACTIVE()		/* one less cred in use; stub for now */

ulong_t cr_seqnum;              /* global credential counter */
fspin_t cr_seq_mutex;           /* lock for cr_seqnum */

#define CR_SEQNUM(seqnum) \
{ \
	FSPIN_LOCK(&cr_seq_mutex); \
	seqnum = ++cr_seqnum; \
	FSPIN_UNLOCK(&cr_seq_mutex); \
}

STATIC size_t	crsize;		/* exists to avoid recomputation */


cred_t	*sys_cred;	/* system credentials */


/*
 * void cred_init(void)
 *	Initialize credentials data structures.
 *
 * Calling/Exit State:
 *	No parameters are given.  Upon completion, the credentials
 *	module is initialized.
 *
 * Remarks:
 *	This function must be called before any of the other
 *	credentials functions exported from this module are
 *	invoked.
 */
void
cred_init(void)
{

	/*
	 * Confirm that the configured number of supplementary groups
	 * is between the min and the max allowed.  If not, print a
	 * message and assign a reasonable value.
	 */
	if (ngroups_max < NGROUPS_UMIN) {
		/*
		 *+ Configured value for NGROUPS_MAX is less than
		 *+ the minimum allowed.  Check configured value
		 *+ for NGROUPS_MAX.
		 */
		cmn_err(CE_NOTE, 
		  "Configured value of NGROUPS_MAX (%d) is less than \
min (%d), NGROUPS_MAX set to %d\n", ngroups_max, NGROUPS_UMIN, NGROUPS_UMIN);
		ngroups_max = NGROUPS_UMIN;
	}

	if (ngroups_max > NGROUPS_UMAX) {
		/*
		 *+ Configured value for NGROUPS_MAX is greater than
		 *+ the maximum allowed.  Check configured value
		 *+ for NGROUPS_MAX.
		 */
		cmn_err(CE_NOTE,
		  "Configured value of NGROUPS_MAX (%d) is greater than \
max (%d), NGROUPS_MAX set to %d\n", ngroups_max, NGROUPS_UMAX, NGROUPS_UMAX);
		ngroups_max = NGROUPS_UMAX;
	}

	/*
	 * Initialize crsize, making sure that crsize is word-aligned.
	 */
	crsize = sizeof(cred_t) + sizeof(gid_t)*(ngroups_max-1);
	crsize = (crsize+sizeof(int)-1) & ~(sizeof(int)-1);

	cr_seqnum = 0;
	FSPIN_INIT(&cr_seq_mutex);
}


/*
 * cred_t *crget(void)
 *	Allocates a new credentials structure with a reference
 *	count of one.
 *
 * Calling/Exit State:
 *	No parameters are given to this function.  Upon completion,
 *	a pointer to the new credential structure is returned.
 *
 * Remarks:
 *	This function can block to allocate the new credentials
 *	structure.  Hence, the caller must not hold any spin locks
 *	when calling this function.
 */
cred_t *
crget(void)
{
	cred_t *credp;

	credp = (cred_t *)kmem_zalloc(crsize, KM_SLEEP);
	CRST_ACTIVE();
	FSPIN_INIT(&credp->cr_mutex);
	credp->cr_ref = 1;
	CR_SEQNUM(credp->cr_seqnum);
	return (credp);
}


/*
 * void crholdn(cred_t *credp, uint_t holdcount)
 *	Increment the reference count on the indicated credential
 *	by the specified number.
 *
 * Calling/Exit State:
 *	The credp pointer and the unsigned holdcount parameters
 *	respectively identify the credentials to be held and
 *	reference count increment.
 */
void
crholdn(cred_t *credp, uint_t holdcount)
{
	FSPIN_LOCK(&credp->cr_mutex);
	credp->cr_ref += holdcount;
	FSPIN_UNLOCK(&credp->cr_mutex);
}


/*
 * void crfreen(cred_t *credp, uint_t derefcount)
 *	Release the specified number of references against the given
 *	credentials structure.
 *
 * Calling/Exit State:
 *	The credp pointer and the unsigned holdcount parameters
 *	respectively identify the credentials to be held and
 *	reference count decrement.
 */
void
crfreen(cred_t *credp, uint_t derefcount)
{
	FSPIN_LOCK(&credp->cr_mutex);
	if ((credp->cr_ref -= derefcount) > 0) {
		FSPIN_UNLOCK(&credp->cr_mutex);
	} else {
		FSPIN_UNLOCK(&credp->cr_mutex);
                mac_rele(credp->cr_lid);
		ADT_GETSEQ(credp);
		kmem_free((void *)credp, crsize);
		CRST_INACTIVE();
	}
}


/*
 * cred_t *crdup(cred_t *credp)
 *	Dup a cred struct to a new held one with a single reference.
 *
 * Calling/Exit State:
 *	The credential to be duplicated is given by the credp parameter.
 *	A new credentials structure is allocated, and its contents are
 *	set to those of credp.  Upon completion, this function returns
 *	a pointer to the new credential.  The original credential remains
 *	allocated.
 *
 * Remarks:
 *	This function should be used to allocate a new credentials object
 *	that is a copy of the original, when the credentials object is
 *	temporary.  That is, the returned credentials object is to only
 *	be used temporarily, and will not be installed as the new
 *	credentials of the calling process.  For the latter usage,
 *	see crdup2() and crinstall().
 *
 *	This function can block to allocate the new credentials
 *	structure.  Hence, the caller must not hold any spin locks
 *	when calling this function.
 */
cred_t *
crdup(cred_t *credp)
{
	cred_t *newcredp;

	newcredp = (cred_t *)kmem_alloc(crsize, KM_SLEEP);
	CRST_ACTIVE();
	bcopy((void *)credp, (void *)newcredp, crsize);
	FSPIN_INIT(&newcredp->cr_mutex);
	newcredp->cr_ref = 1;	/* reset reference count from bcopy() */
	newcredp->cr_flags &= ~CR_RDUMP;        /* reset flag */
        mac_hold(newcredp->cr_lid);
	CR_SEQNUM(newcredp->cr_seqnum);
	return (newcredp);
}


/*
 * cred_t *crdup2(cred_t *credp)
 *	Dup a cred struct to a new held one with two references,
 *	suitable for installation as the new process credentials via
 *	crinstall().
 *
 * Calling/Exit State:
 *	The credential to be duplicated is given by the credp parameter.
 *	A new credentials structure is allocated, and its contents are
 *	set to those of credp.  Upon completion, this function returns
 *	a pointer to the new credential.  The original credential remains
 *	allocated.
 *
 * Remarks:
 *	This function is to be used whenever the process credentials are
 *	to be changed.  The credentials copy returned has its reference
 *	count initialized to two (one for the process and one for the
 *	calling LWP).  The returned credentials copy can be modified by
 *	the caller, and then installed as the new process credentials
 *	using crinstall().
 *
 *	This function can block to allocate the new credentials structure.
 *	Hence, the caller must not hold any spin locks when calling this
 *	function.
 */
cred_t *
crdup2(cred_t *credp)
{
	cred_t *newcredp;

	newcredp = (cred_t *)kmem_alloc(crsize, KM_SLEEP);
	CRST_ACTIVE();
	bcopy((void *)credp, (void *)newcredp, crsize);
	FSPIN_INIT(&newcredp->cr_mutex);
	newcredp->cr_ref = 2;	/* 2 refs, one for process, one for LWP */
        mac_hold(newcredp->cr_lid);
	newcredp->cr_flags &= ~CR_RDUMP;        /* reset flag */
	CR_SEQNUM(newcredp->cr_seqnum);
	return (newcredp);
}


/*
 * void crinstall(cred_t *ncrp)
 *	Install the given credentials object as the new credentials for
 *	the calling LWP and process.
 *
 * Calling/Exit State:
 *	The caller must have already established the appropriate references
 *	needed against the given credentials object.  Upon return, the
 *	given credentials object is installed as the credentials of both
 *	the calling LWP and process.  All other LWPs in the process will
 *	have their EVF_PL_CRED trap event flag set, alerting them of the
 *	need to update their own copy of the process credentials before
 *	the start of their next system call.
 *
 * Remarks:
 *	As part of the installation process, the references to the old
 *	credentials from both the calling LWP and process are discarded
 *	by this function.
 *
 *	The crinstall() function should typically be used in a
 *	crdup2()/crinstall() sequence as follows:
 *
 *		crp = crdup2(u.u_lwpp->l_cred);
 *
 *		-- Change the returned credentials (crp) as appropriate.
 *
 *		crinstall(crp);		-- install the new credentials.
 */
void
crinstall(cred_t *ncrp)
{
	proc_t *p = u.u_procp;		/* calling process */
	cred_t *pcrp;			/* old process credentials */ 
	cred_t *lwpcrp;			/* old LWP credentials */
	uidquo_t *nuidp = NULL;
	uidres_t uidres;
	pl_t pl;

	lwpcrp = u.u_lwpp->l_cred;
	u.u_lwpp->l_cred = ncrp;
	if (lwpcrp->cr_ruid != ncrp->cr_ruid) {
		nuidp = uidquota_get(ncrp->cr_ruid);
		struct_zero(&uidres, sizeof(uidres_t));
		uidres.ur_prcnt = 1;
	}
	pl = LOCK(&p->p_mutex, PLHI);
	if (nuidp != NULL) {		/* changing real user-ID */
		uidres.ur_lwpcnt = p->p_ntotallwp;
		uidquota_decr(p->p_uidquotap, &uidres);
		uidres.ur_prcnt = 0;	/* uidquota_get returns w/1 already */
		(void) uidquota_incr(nuidp, &uidres, B_TRUE);
		p->p_uidquotap = nuidp;
	}
	pcrp = p->p_cred;
	p->p_cred = ncrp;
	if (!SINGLE_THREADED())
		trapevproc(p, EVF_PL_CRED, B_FALSE);
	UNLOCK(&p->p_mutex, pl);
	if (lwpcrp == pcrp) {		/* very likely */
		crfreen(pcrp, 2);
	} else {
		crfree(pcrp);
		crfree(lwpcrp);
	}
}


/*
 * size_t crgetsize(void)
 *	Return the size of a credentials structure.
 *
 * Calling/Exit State:
 *	No parameters are supplied.
 *
 *	The return value of this function is the size of a credentials
 *	structure.
 *
 * Remarks:
 *	This is a function, since the crsize variable maintaining the size
 *	of a credentials structure is a static variable that is readonly to
 *	the outside world via this function.
 */
size_t
crgetsize(void)
{
	return(crsize);
}


/*
 * int groupmember(gid_t gid, cred_t *credp)
 *	Determine whether the supplied group id is a member of the group
 *	described by the supplied credentials.
 *
 * Calling/Exit State:
 *	The gid parameter identifies the group-ID to be located within the
 *	group set described by credp.
 *
 *	This function returns a non-zero value if the given group-ID is
 *	within the group set identified by credp.  Otherwise, zero is
 *	returned.
 */
int
groupmember(gid_t gid, cred_t *credp)
{
	gid_t *gp, *endgp;

	if (gid == credp->cr_gid) {
		return (1);
	}
	endgp = &credp->cr_groups[credp->cr_ngroups];
	for (gp = credp->cr_groups; gp < endgp; gp++) {
		if (*gp == gid)
			return (1);
	}
	return (0);
}
 

/*
 * int hasprocperm(cred_t *tcredp, cred_t *scredp)
 *	Determine if the credentials set "scredp" has permission to
 *	act upon the credentials set: "tcredp."  This function enforces
 *	the permission requirements needed to send a signal to a process.
 *	The same requirements are imposed by other system calls as well.
 *
 * Calling/Exit State:
 *	Scredp points to the credentials being tested for permissions
 *	against the credentials identified by tcredp.
 *
 *	This function returns a non-zero value if permissions are granted.
 *	Otherwise, zero is returned.
 */
int
hasprocperm(cred_t *tcredp, cred_t *scredp)
{
	return (!pm_denied(scredp, P_OWNER) ||
		scredp->cr_uid  == tcredp->cr_ruid ||
		scredp->cr_ruid == tcredp->cr_ruid ||
		scredp->cr_uid  == tcredp->cr_suid ||
		scredp->cr_ruid == tcredp->cr_suid);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_cred(const cred_t *crp)
 *	Formatted dump of a credential structure
 *
 * Calling/Exit State:
 *      None. I'll try to dump whatever you pass to me.
 */
void
print_cred(const cred_t *crp)
{
	uint_t i;

	debug_printf("cred structure dump at 0x%lx\n\n", (ulong_t)crp);

	debug_printf("cr_mutex=0x%x\n", crp->cr_mutex);
	debug_printf("cr_ref=0x%lx\n", crp->cr_ref);
	debug_printf("cr_ngroups=0x%x\n", (uint_t)crp->cr_ngroups);
	debug_printf("cr_uid=0x%x\n", crp->cr_uid);
	debug_printf("cr_gid=0x%x\n", crp->cr_gid);
	debug_printf("cr_ruid=0x%x\n", crp->cr_ruid);
	debug_printf("cr_rgid=0x%x\n", crp->cr_rgid);
	debug_printf("cr_suid=0x%x\n", crp->cr_suid);
	debug_printf("cr_sgid=0x%x\n", crp->cr_sgid);
	debug_printf("cr_savpriv=0x%lx\n", crp->cr_savpriv);
	debug_printf("cr_wkgpriv=0x%lx\n", crp->cr_wkgpriv);
	debug_printf("cr_maxpriv=0x%lx\n", crp->cr_maxpriv);
	debug_printf("cr_lid=0x%lx\n", crp->cr_lid);
	debug_printf("cr_flags=0x%lx\n", crp->cr_flags);
	debug_printf("cr_seqnum=0x%lx\n", crp->cr_seqnum);
	debug_printf("cr_groups[]= ");
	for (i = 0; i < crp->cr_ngroups; i++)
		debug_printf("%d, ", crp->cr_groups[i]);
	debug_printf("\n");
}

/*
 * void
 * print_proc_cred(const proc_t *procp)
 *      Formatted dump of the PROC's credential structure
 *
 * Calling/Exit State:
 *      None. I'll try to dump whatever you pass to me.
 */
void
print_proc_cred(const proc_t *procp)
{
	const cred_t *crp = procp->p_cred;

	if (crp)
		print_cred(crp);
	else
		debug_printf("NULL cred structure\n");
}

/*
 * void
 * print_lwp_cred(const lwp_t *lwpp)
 *      Formatted dump of the LWP's credential structure
 *
 * Calling/Exit State:
 *      None. I'll try to dump whatever you pass to me.
 */
void
print_lwp_cred(const lwp_t *lwpp)
{
	const cred_t *crp = lwpp->l_cred;

	if (crp)
		print_cred(crp);
	else
		debug_printf("NULL cred structure\n");
}

#endif /* DEBUG || DEBUG_TOOLS */
