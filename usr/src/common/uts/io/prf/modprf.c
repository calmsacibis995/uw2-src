/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/prf/modprf.c	1.2"
#ident	"$Header: $"

#include <io/prf/prf.h>
#include <mem/faultcatch.h>
#include <mem/kmem.h>
#include <proc/obj/elf.h>
#include <proc/obj/elftypes.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_obj.h>


/*
 * int mod_prf(struct mprf **retprf)
 *	Set or unset the MOD_PRF flag on the static kernel and
 *	all the loaded modules to lock them in the running kernel.
 *	The routine returns information about the static kernel
 *	and all the loadable modules loaded at the time the routine
 *	is called.
 *
 * Calling/Exit State:
 *	If the argument retprf is non-NULL, the MOD_PRF flag will
 *	be set on all the loaded modules; otherwise the flag will
 *	be unset.
 *
 *	The MOD_PRF flag on the static kernel means the profiling
 *	is turned on. It is not allowed to turn on profiling again
 *	while it's already on.
 *
 *	Information returned in retprf includes the starting address
 *	and load path of each loaded module. The first mprf struct
 *	includes number of modules, static kernel is also counted, and
 *	the total length of all the loading paths. The second mprf
 *	struct is for the static kernel. The loading path field for
 *	the static kernel is a null string.
 *
 *	No lock held upen calling and exit the routine.
 */
int
mod_prf(struct mprf **retprf)
{
	struct modctl *mcp;
	struct module *modp;
	unsigned int mcount;
	struct mprf *rpp;
	unsigned int namespcnt;
	unsigned int saveoff;
	char *namesp;
	extern boolean_t mod_initialized;

	if (!mod_initialized)
		return (ENOLOAD);

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (retprf == NULL) {
		/*
		 * turn profiling off
		 */
		(void)RW_RDLOCK(&modlist_lock, PLDLM);
		for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {
			if (mcp->mc_modp == NULL)
				continue;
			modp = mcp->mc_modp;

			(void)LOCK(&modp->mod_lock, PLDLM);
			modp->mod_flags ^= MOD_PRF;
			UNLOCK(&modp->mod_lock, PLDLM);
		}
		RW_UNLOCK(&modlist_lock, PLBASE);

		(void)LOCK(&modhead.mc_modp->mod_lock, PLDLM);
		modhead.mc_modp->mod_flags ^= MOD_PRF;
		UNLOCK(&modhead.mc_modp->mod_lock, PLDLM);

		return (0);
	}

	/*
	 * turn profiling on
	 */

	mcount = 1;
	namespcnt = 1;

	(void)LOCK(&modhead.mc_modp->mod_lock, PLDLM);
	if (modhead.mc_modp->mod_flags & MOD_PRF) {
		/*
		 * cannot turn profiling on again without turning it off first
		 */
		UNLOCK(&modhead.mc_modp->mod_lock, PLBASE);
		return (EBUSY);
	} else
		modhead.mc_modp->mod_flags |= MOD_PRF;
	UNLOCK(&modhead.mc_modp->mod_lock, PLDLM);

	(void)RW_RDLOCK(&modlist_lock, PLDLM);
	for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {
		if (mcp->mc_modp == NULL)
			continue;
		modp = mcp->mc_modp;

		(void)LOCK(&modp->mod_lock, PLDLM);
		if (modp->mod_flags & MOD_TRANS) {
			/*
			 * skip the module which is being loading
			 * or unloading
			 */
			UNLOCK(&modp->mod_lock, PLDLM);
			continue;
		}
		modp->mod_flags |= MOD_PRF;
		UNLOCK(&modp->mod_lock, PLDLM);
		mcount++;
		namespcnt += (strlen(modp->mod_obj.md_path) + 1);
	}
	RW_UNLOCK(&modlist_lock, PLBASE);

	/*
	 * Collect information for the static kernel and each loaded module.
	 */
	rpp = *retprf = (struct mprf *)kmem_alloc((mcount + 1) *
				     sizeof(struct mprf) + namespcnt, KM_SLEEP);
	namesp = (char *)(rpp + mcount + 1);

	rpp->mprf_addr = mcount;
	rpp->mprf_offset = namespcnt;
	rpp++;

	/* fill in the struct for the static kernel */
	rpp->mprf_addr = (unsigned long)modhead.mc_modp->mod_obj.md_space;
	rpp->mprf_offset = 0;
	*namesp = '\0';

	saveoff = 1;
	(void)RW_RDLOCK(&modlist_lock, PLDLM);
	for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {
		if (mcp->mc_modp == NULL)
			continue;
		modp = mcp->mc_modp;

		(void)LOCK(&modp->mod_lock, PLDLM);
		if (!(modp->mod_flags & MOD_PRF)) {
			/*
			 * Only get those modules whose MOD_PRF flag set
			 * in the previous loop. Any module loaded after 
			 * the snap shoot will be ignored here.
			 */
			UNLOCK(&modp->mod_lock, PLDLM);
			continue;
		}
		UNLOCK(&modp->mod_lock, PLDLM);
		rpp++;
		rpp->mprf_addr = (unsigned long)(modp->mod_obj.md_space);
		rpp->mprf_offset = saveoff;
		strcpy(namesp + saveoff, modp->mod_obj.md_path);
		saveoff += strlen(modp->mod_obj.md_path) + 1;
	}
	RW_UNLOCK(&modlist_lock, PLBASE);

	return (0);
}
