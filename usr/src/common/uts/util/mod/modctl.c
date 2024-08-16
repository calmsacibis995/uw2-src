/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/modctl.c	1.41"
#ident  "$Header: $"

#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/seg_kvn.h>
#include <proc/bind.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ghier.h>
#include <util/ksynch.h>
#include <util/mod/ksym.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_obj.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/types.h>

extern lkinfo_t mod_lkinfo;

/*
 * Check for circular dependencies.
 * If we are the process already loading this module, then
 * we're trying to load a dependent that we're already loading which
 * means we got a circular dependency.
 */
#define MOD_CIRCDEP(modp) \
	((modp)->mod_loading_context != NULL && \
	    (modp)->mod_loading_context == u.u_lwpp)

#define UNLOAD_DELAY(modp)	(long)((modp)->mod_unload_time + \
			 (modp)->mod_delay_tick - lbolt)

#define NO_AUTOUNLD(modp)	((modp)->mod_unload_time == -1)

#define CUR_CONTEXT	((void *)u.u_lwpp)

extern rwlock_t modlist_lock;

STATIC const char *getmodname(const char *);
STATIC int modunld(struct modctl *);
STATIC int mod_install_stubs(struct modctl *);
STATIC void mod_remove_stubs(struct modctl *);
STATIC int mod_connect(struct module *);
STATIC int mod_disconn(struct module *);
STATIC int mod_info(struct module *, struct modstatus *);
STATIC struct modctl *mod_min_delay(void);
STATIC int mod_cpubind(struct module *, int *);
STATIC void mod_load_helper(void *);
int modld(const char *, cred_t *, struct modctl **, uint_t flags);
int unload_modules(size_t);
int mod_static(const char *);

event_t autounload_event;

/*
 * Structure for modload.
 */
struct modlda {
	char *pathname;
};

/*
 * int modload(struct modlda *uap, rval_t *rvp)
 *	System call to demand load a loadable module.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errno returned directly by this routine is:
 *
 *		EPERM	if the user does not have the P_LOADMOD privilege
 *
 *	The module id of the successfully loaded module is
 *	returned as rvp->r_val1. The module loaded by this system call
 *	will have the flag MOD_DEMAND set so it won't be able to be
 *	auto unload directly.
 *
 * Remarks:
 *	This function invokes the MOD_SEGKVN_DRV_FLAGS macro in order to
 *	determine if tmodule being demand loaded should be loaded into
 *	DMAable memory. MOD_SEGKVN_DRV_FLAGS in turn invokes the
 *	PHYSMEM_DRV_DMA macro, which is exported by the family. This
 *	hook was placed here for the i386 family, and in that family
 *	specifically for the i386at platform; as a mechanism for
 *	implementing compatibility with 4.2 binary HBA drivers. Note
 *	that HBA drivers are always demand loaded, so that this hook is
 *	note needed in any of the autoload cases.
 */
int
modload(struct modlda *uap, rval_t *rvp)
{
	int retval;
	struct modctl *mcp;
	struct module *modp;
	char *pathname = NULL;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	if (pm_denied(CRED(), P_LOADMOD))
		return (EPERM);

	pathname = kmem_alloc(MAXPATHLEN, KM_SLEEP);
	if ((retval = copyinstr(uap->pathname, pathname, MAXPATHLEN-1, 0)) != 0)
		goto load_out;

	if ((retval = modld(pathname, CRED(), &mcp, MOD_SEGKVN_DRV_FLAGS)) != 0)
		goto load_out;

	modp = mcp->mc_modp;

	/* avoid autounload of demand loaded modules */
	if (modp->mod_flags & MOD_DEMAND) {
		UNLOCK(&modp->mod_lock, PLBASE);
	} else {
		modp->mod_flags |= MOD_DEMAND;
		MOD_HOLD_L(modp, PLBASE);
	}
	rvp->r_val1 = mcp->mc_id;
	retval = 0;

load_out:
	if (u.u_lwpp->l_auditp)
		adt_modload(retval, mcp);
	kmem_free(pathname, MAXPATHLEN);
	return (retval);
}

/*
 * Structure for moduload.
 */
struct modulda {
	int mod_id;
};

/*
 * int moduload(struct modulda *uap, rval_t *rvp)
 *	System call to demand unload a loadable module or all the
 *	unloadable modules.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *
 *		EPERM	if the user does not have the P_LOADMOD privilege
 *		EINVAL	if the specified module id is not existing
 *		EBUSY	if the specified module id is not unloadable
 *
 *	The flag MOD_DEMAND, if is set, will be turned off even if 
 *	the demand unloading failed, so the specified module(s) can 
 *	be auto unloaded later.
 */
/* ARGSUSED */
int
moduload(struct modulda *uap, rval_t *rvp)
{
	struct modctl *mcp;
	struct module *modp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	if (pm_denied(CRED(), P_LOADMOD))
		return (EPERM);

	if (uap->mod_id == 0) {
		if (unload_modules(0) == 0)
			return (EBUSY);
		else
			return (0);
	}

	(void)RW_RDLOCK(&modlist_lock, PLDLM);
	for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {
		if (mcp->mc_id == uap->mod_id)
			break;
	}

	if (mcp == NULL || mcp->mc_modp == NULL) {
		RW_UNLOCK(&modlist_lock, PLBASE);
		return (EINVAL);
	}

	modp = mcp->mc_modp;

	(void)LOCK(&modp->mod_lock, PLDLM);
	RW_UNLOCK(&modlist_lock, PLDLM);

	/* unset demand load flag and decrease hold count if the flag is set */
	if (modp->mod_flags & MOD_DEMAND) {
		modp->mod_flags &= ~MOD_DEMAND;
		MOD_RELE_L(modp, PLBASE);
	} else
		UNLOCK(&modp->mod_lock, PLBASE);

	(void)RW_RDLOCK(&modlist_lock, PLDLM);
	if ((modp = mcp->mc_modp) == NULL)
		return (0);
	(void)LOCK(&modp->mod_lock, PLDLM);
	RW_UNLOCK(&modlist_lock, PLDLM);
	if ((modp->mod_flags & MOD_TRANS) || MODBUSY(modp)) {
		UNLOCK(&modp->mod_lock, PLBASE);
		return (EBUSY);

	} else {
		modp->mod_flags |= MOD_UNLOADING;
		UNLOCK(&modp->mod_lock, PLBASE);
	}
		
	return (modunld(mcp));
}

/*
 * Structure for modstat.
 */
struct modstata {
	int mod_id;
	struct modstatus *stbuf;
	boolean_t get_next_mod;
};

/*
 * int modstat(struct modstata *uap, rval_t *rvp)
 *	System call to get information of a loadable module.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *
 *		EPERM	if the user does not have the P_LOADMOD privilege
 *		EINVAL	if the specified module id is not existing and
 *			get_next_mod is set to B_FALSE; or if there is
 *			no module id greater than the module id specified
 *			when get_next_mod is set to B_TRUE.
 */
/* ARGSUSED */
int
modstat(struct modstata *uap, rval_t *rvp)
{
	struct modstatus *modstatusp;
	struct modctl *mcp;
	struct modctl *exact = NULL;
	struct modctl *next_highest = NULL;
	struct module *modp;
	int retval;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	if (pm_denied(CRED(), P_LOADMOD))
		return (EPERM);

	modstatusp = (struct modstatus *)kmem_zalloc(sizeof(struct modstatus), 
						    KM_SLEEP);

/*
 *  List is stored in reverse order of keys.
 *  Find an exact match if one exists, or the next highest hit
 *  for the given key if get_next_mod is set.
 */

	RW_RDLOCK(&modlist_lock, PLDLM);
        for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {
                if (mcp->mc_modp == NULL ||
                   (mcp->mc_modp->mod_flags & MOD_LOADING))
                        continue;

                if (mcp->mc_id > uap->mod_id) {
                        next_highest = mcp;
                        continue;
                }

                if (mcp->mc_id == uap->mod_id)
                        exact = mcp;

                break;		/* < (gone past) or = (match) cases */
        }

        if (exact)
                mcp = exact;
        else if (uap->get_next_mod && next_highest)
                mcp = next_highest;
        else
        {
                RW_UNLOCK(&modlist_lock, PLBASE);
                retval = EINVAL;
                goto statout;
        }

	modp = mcp->mc_modp;
	LOCK(&modp->mod_lock, PLDLM);
	RW_UNLOCK(&modlist_lock, PLDLM);
	modp->mod_keepcnt++;
	UNLOCK(&modp->mod_lock, PLBASE);
			
	modstatusp->ms_id = mcp->mc_id;
	modstatusp->ms_unload_delay = modp->mod_delay_tick / HZ;

	modstatusp->ms_holdcnt = modp->mod_holdcnt;
	modstatusp->ms_depcnt = modp->mod_depcnt;

	modstatusp->ms_base = modp->mod_obj.md_space;
	modstatusp->ms_size = modp->mod_obj.md_space_size + 
			      modp->mod_obj.md_bss_size +
			      modp->mod_obj.md_symsize;

	strncpy(modstatusp->ms_path, modp->mod_obj.md_path, MAXPATHLEN);

	if ((retval = mod_info(modp, modstatusp)) != 0) {
		goto statout2;
	}

	retval = ucopyout(modstatusp, uap->stbuf, sizeof(struct modstatus), 0);

statout2:
	LOCK(&modp->mod_lock, PLDLM);
	modp->mod_keepcnt--;
	UNLOCK(&modp->mod_lock, PLBASE);
statout:
	kmem_free(modstatusp, sizeof(struct modstatus));
	return (retval);
}


static int
mod_getmcp(const char *module_name, const char *modname, cred_t *credentials, 
	struct modctl **retmcp, uint_t flags, int *already_loaded)
{
	extern boolean_t mod_initialized;
	struct modctl *modctlp, *mcp;
	struct module *modulep, *modp;
	struct mod_conf_data *mcdp;
	int retval;

	ASSERT(module_name != NULL);

	*already_loaded = 0;
	*retmcp = NULL;

	if (!mod_initialized) {
		/*
		 *  Attempt to load the module prior to DLM initialization.
		 */
		cmn_err(CE_WARN,
		     "Attempt to load module %s: DLM is not initialized",
		     modname);

		return ENOSYS;
	}

	if (mod_static(modname)) {
		/*
		 *+ The module is already statically linked.
		 */
		cmn_err(CE_WARN,
			"!MOD: module %s, is already statically linked.",
			modname);

		return EINVAL;
	}

	mcp = (struct modctl *)kmem_alloc(sizeof(struct modctl) +
					  strlen(modname), KM_SLEEP);
	strcpy(mcp->mc_name, modname);
	modp = (struct module *)kmem_zalloc(sizeof(struct module), KM_SLEEP);

	LOCK_INIT(&modp->mod_lock, DLM_HIER_CTL, PLDLM, &mod_lkinfo, KM_SLEEP);
	SV_INIT(&modp->mod_sv);
	modp->mod_flags = MOD_LOADING | MOD_SYMLOCKED;
	modp->mod_loading_context = CUR_CONTEXT;
	modp->mod_depcnt = 0;
	modp->mod_holdcnt = 0;
	modp->mod_unload_time = 0;

	/* check the existence of the requested module */
	RW_WRLOCK(&modlist_lock, PLDLM);
	for (modctlp = modhead.mc_next; modctlp != NULL; 
	     modctlp = modctlp->mc_next) {
		if (strcmp(modname, modctlp->mc_name) == 0) {
again:
			if (modctlp->mc_modp == NULL) {
				modctlp->mc_modp = modp;
				kmem_free(mcp, sizeof(struct modctl) +
						strlen(modname));
				mcp = modctlp;
				break;
			}

			modulep = modctlp->mc_modp;

			LOCK(&modulep->mod_lock, PLDLM);
			RW_UNLOCK(&modlist_lock, PLDLM);

			/* Check for circular dependency. */
			if (MOD_CIRCDEP(modulep)) {
				UNLOCK(&modulep->mod_lock, PLBASE);

				/*
				 *  Circular dependency found in loading a
				 *  loadable module and all the modules it
				 *  depends on.
				 */
				cmn_err(CE_NOTE,
				"!MOD: Circular dependency in module %s.",
					modctlp->mc_name);

				kmem_free(mcp, sizeof(struct modctl) +
						strlen(modname));
				LOCK_DEINIT(&modp->mod_lock);
				kmem_free(modp, sizeof(*modp));
				return EINVAL;
			}

			if (modulep->mod_flags & MOD_TRANS) {
				SV_WAIT(&modulep->mod_sv, PRIMED,
					&modulep->mod_lock);
				RW_WRLOCK(&modlist_lock, PLDLM);
				/*
				 * The struct modctl will always stay in the
				 * modhead list once it is loaded. So we can
				 * use the modulep pointer without search the
				 * list again.
				 */
				goto again;
			}

			UNLOCK(&modulep->mod_lock, PLBASE);
			LOCK_DEINIT(&modp->mod_lock);
			kmem_free(modp, sizeof(*modp));
			kmem_free(mcp, sizeof(struct modctl) + strlen(modname));
			*retmcp = modctlp;
			*already_loaded = 1;
			return 0;
		}
	}

	if (modctlp == NULL) {
		/* add to the beginning of the modhead list */
		if (modhead.mc_next == NULL)
			mcp->mc_id = 1;
		else
			mcp->mc_id = modhead.mc_next->mc_id + 1;
		mcp->mc_modp = modp;
		mcp->mc_next = modhead.mc_next;
		modhead.mc_next = mcp;
	}

	RW_UNLOCK(&modlist_lock, PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: loading %s, module id %d.\n", 
		module_name, mcp->mc_id));

	retval = mod_obj_load(module_name, credentials, &modp->mod_obj, flags);

	if (retval) {
		moddebug(cmn_err(CE_CONT, "!MOD: error loading %s\n", 
						 module_name));

		(void)RW_WRLOCK(&modlist_lock, PLDLM);
		mcp->mc_modp = NULL;
		RW_UNLOCK(&modlist_lock, PLBASE);
		SV_BROADCAST(&modp->mod_sv, 0);

		LOCK_DEINIT(&modp->mod_lock);
		kmem_free(modp, sizeof(*modp));
		return retval;
	}

	(void)LOCK(&modp->mod_lock, PLDLM);
	modp->mod_flags |= MOD_SYMTABOK;
	UNLOCK(&modp->mod_lock, PLBASE);

/*
 *  The revision number of the loadable module and the running kernel
 *  must match.  Conversions from old modwrapper structures should be
 *  done by mod_obj_load().
 */

	ASSERT(modp->mod_obj.md_mw->mw_rev == MODREV);

	mcdp = modp->mod_obj.md_mw->mw_conf_data;
	modp->mod_delay_tick = (clock_t)mcdp->mcd_unload_delay * HZ;

	*retmcp = mcp;
	return 0;
}


extern struct {char *name; struct modwrapper *mw;} name_to_modwrapper[];
#pragma weak name_to_modwrapper

struct modwrapper *
find_static_modwrapper(const char *modname)
{
	int i;

	if (name_to_modwrapper == NULL)
	{
	    cmn_err(CE_NOTE, "MOD: name_to_modwrapper structure not present\n");
	    return NULL;
	}

	for (i = 0; name_to_modwrapper[i].name; i++)
		if (strcmp(name_to_modwrapper[i].name, modname) == 0)
			return name_to_modwrapper[i].mw;

	return NULL;
}


struct mod_verify {
	char mv_modname[MODMAXNAMELEN];
	rm_key_t mv_key;
};

/*
 * int modverify(void *arg)
 *
 *	Invoke a loadable module's verify routine, if it exists,
 *	and return the verify routine's result to the caller.
 *
 *	The following chain of events occurs when a verify request
 *	is made:
 *
 *	1.  The specified module is laoded and its symbols are resolved.
 *
 *	2.  The module's verify wrapper entry point is called, and its
 *	    return value is saved.
 *
 *	3.  The module is unloaded and its memory freed.
 *
 *	4.  The verify return value is returned to the caller.
 *
 *	The modules load() and unload() routines are not called
 *	during this process.
 *
 * Calling/Exit State:
 *	The module_name argument gives a module name or a full path 
 *	of a module. The credentials argument gives a credential 
 *	struct of the calling process. 
 *
 *	The following flags are supported:
 *
 *		SEGKVN_DMA	Indicates that the driver is to be loaded
 *				into DMAable memory. Only used when
 *				RESTRICTED DMA support is in effect.
 *
 *	No locks should be held upon entry.
 *
 *	If the load and call of the module's verify routine succeeds,
 *	the return value of the verify routine will be returned to
 *	the caller of modverify().
 *
 *	Errnos returned directly by this routine:
 *
 *		ENOSYS	The specified module does not contain a verify
 *			entry point
 */

int
modverify(void *arg)
{
	struct mod_verify mv;
	const char *modname;
	int ret = 0;
	int (*func)(rm_key_t);
	int already_loaded;
	struct modctl *mcp;
	struct module *modp;
	struct modwrapper *static_mw_p;
	struct module fake_mod;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "MOD: modverify()\n"));

	if (copyin(arg, &mv, sizeof(struct mod_verify)))
		return (EFAULT);

	modname = getmodname(mv.mv_modname);

	if (mod_static(modname))
	{
		static_mw_p = find_static_modwrapper(modname);

		if (static_mw_p == NULL)
			return EINVAL;

		fake_mod.mod_obj.md_mw = static_mw_p;
		modp = &fake_mod;

		already_loaded = 1;
	}
	else
	{
		ret = mod_getmcp(mv.mv_modname, modname, sys_cred, &mcp, 0,
						&already_loaded);

		if (ret)
			return ret;

		modp = mcp->mc_modp;
	}

	/*
	 * call the _load() entry point in the module if the routine exists;
	 * if something failed in _load(), it should free up all the
	 * resource it allocated before return
	 */
	
	func = (int (*)(rm_key_t))(*modp->mod_obj.md_mw->mw_verify);

	if (func) {
		int cpu;
		struct engine *engp;

		cpu = -1;
		if (ret = mod_cpubind(modp, &cpu))
			goto errout;
		if (cpu != -1) {
			if (!engine_disable_offline(cpu)) {
				/*
				 *+ Could not lock engine on line.
				 */
				cmn_err(CE_NOTE,
		"MOD:modld: engine_disable_offline failed on processor %d\n",
					cpu);
				ret = EINVAL;
				goto errout;
			}
			engp = kbind(&engine[cpu]);
			DISABLE_PRMPT();
			u.u_lwpp->l_notrt++;
		}

		ret = (*func)(mv.mv_key);	/* call _verify */

		if (cpu != -1) {
			ASSERT(u.u_lwpp->l_notrt != 0);
			u.u_lwpp->l_notrt--;
			ENABLE_PRMPT();
			kunbind(engp);
		}

		moddebug(cmn_err(CE_CONT,
				 "!MOD: Returned from _verify, ret = %x\n",
				 ret));
	}
	else
		ret = ENOSYS;	 /* module has no verify entry point */

	/*
 	 *  Unload module, free allocated resources, and pass ret
 	 *  back to caller.
	 */

errout:

	if (!already_loaded) {
		(void)LOCK(&modp->mod_lock, PLDLM);
		modp->mod_flags &= ~MOD_SYMTABOK;
		UNLOCK(&modp->mod_lock, PLBASE);
#ifdef DLM_PAGING
		UNLOCKMOD(&modp->mod_obj);
#endif
		ASSERT(modp->mod_flags & MOD_SYMLOCKED);
		mod_obj_unload(&modp->mod_obj, B_TRUE);

		(void)RW_WRLOCK(&modlist_lock, PLDLM);
		mcp->mc_modp = NULL;
		RW_UNLOCK(&modlist_lock, PLBASE);
		SV_BROADCAST(&modp->mod_sv, 0);

		LOCK_DEINIT(&modp->mod_lock);
		kmem_free(modp, sizeof(*modp));
	}

	return ret;
}

/*
 * Arguments for a mod_load_helper instance.
 */
struct modld_args {
	/*
	 * Inargs to mod_load_helper
	 */
	const char	*mol_name;	/* full path module name */
	cred_t		*mol_cred;	/* credentials */
	struct modctl	*mol_mcp;	/* modp structure */
	uint_t		mol_flags;	/* flags */

	/*
	 * Outargs from mod_load_helper
	 */
	int		mol_ret;	/* return value */
	boolean_t	mol_stuck;	/* TRUE: cannot unload on error */

	/*
	 * synchronization
	 */
	event_t		mol_done;	/* signaled by helper when done */
};

/*
 * int modld(const char *module_name, cred_t *credentials, 
 *		struct modctl **retmcp, uint_t flags)
 *
 *	Common kernel interface for autoload and demand load of a 
 *	loadable module.
 *
 * Calling/Exit State:
 *	The module_name argument gives a module name or a full path 
 *	of a module. The credentials argument gives a credential 
 *	struct of the calling process. The retmcp argument is 
 *	used for returning a pointer to the pointer of modctl 
 *	structure of the loaded module.
 *
 *	The following flags are supported:
 *
 *		SEGKVN_DMA	Indicates that the driver is to be loaded
 *				into DMAable memory. Only used when
 *				RESTRICTED DMA support is in effect.
 *
 *	No locks should be held upon entry.
 *
 *	If the loading process succeed or the module is already 
 *	loaded, the retmcp argument will be set to a pointer to 
 *	the struct modctl of the specified module with the mod_lock 
 *	of the module held. The calling process is responsible 
 *	for releasing the lock.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned.  Errnos returned directly by this routine are:
 *
 *		EINVAL	if circular dependency is detected
 *		EBADVER	if the version number between the kernel loadable
 *			module mechanism and the wrapper of the module to
 *			be loaded does not match		
 */
int
modld(const char *module_name, cred_t *credentials, 
	struct modctl **retmcp, uint_t flags)
{
	extern boolean_t mod_initialized;
	struct modctl *modctlp, *mcp;
	struct module *modulep, *modp;
	const char *modname;
	struct modld_args *molp = NULL;
	int retval;

	ASSERT(module_name != NULL);

	modname = getmodname(module_name);

	if (!mod_initialized) {
		/*
		 *+ Attempt to load the module prior to DLM initialization.
		 */
		cmn_err(CE_WARN,
		     "Attempt to load module %s: DLM is not initialized",
		     modname);
		retval = ENOSYS;
		goto errout3;
	}

	if (mod_static(modname)) {
		/*
		 *+ The module is already statically linked.
		 */
		cmn_err(CE_WARN,
			"!MOD: module %s, is already statically linked.",
			modname);
		retval = EINVAL;
		goto errout3;
	}

	mcp = (struct modctl *)kmem_alloc(sizeof(struct modctl) +
					  strlen(modname), KM_SLEEP);
	strcpy(mcp->mc_name, modname);
	modp = (struct module *)kmem_zalloc(sizeof(struct module), KM_SLEEP);

	LOCK_INIT(&modp->mod_lock, DLM_HIER_CTL, PLDLM, &mod_lkinfo, KM_SLEEP);
	SV_INIT(&modp->mod_sv);
	modp->mod_flags = MOD_LOADING | MOD_SYMLOCKED;
	modp->mod_loading_context = CUR_CONTEXT;
	modp->mod_depcnt = 0;
	modp->mod_holdcnt = 0;
	modp->mod_unload_time = 0;

	/* check the existence of the requested module */
	RW_WRLOCK(&modlist_lock, PLDLM);
	for (modctlp = modhead.mc_next; modctlp != NULL; 
	     modctlp = modctlp->mc_next) {
		if (strcmp(modname, modctlp->mc_name) == 0) {
again:
			if (modctlp->mc_modp == NULL) {
				modctlp->mc_modp = modp;
				kmem_free(mcp, sizeof(struct modctl) +
						strlen(modname));
				mcp = modctlp;
				break;
			}

			modulep = modctlp->mc_modp;

			LOCK(&modulep->mod_lock, PLDLM);
			RW_UNLOCK(&modlist_lock, PLDLM);

			/* Check for circular dependency. */
			if (MOD_CIRCDEP(modulep)) {
				UNLOCK(&modulep->mod_lock, PLBASE);

				/*
				 *+ Circular dependency found in loading a
				 *+ loadable module and all the modules it
				 *+ depends on.
				 */
				cmn_err(CE_NOTE,
				"!MOD: Circular dependency in module %s.",
					modctlp->mc_name);
				retval = EINVAL;
				kmem_free(mcp, sizeof(struct modctl) +
						strlen(modname));
				goto errout;
			}

			if (modulep->mod_flags & MOD_TRANS) {
				SV_WAIT(&modulep->mod_sv, PRIMED,
					&modulep->mod_lock);
				RW_WRLOCK(&modlist_lock, PLDLM);
				/*
				 * The struct modctl will always stay in the
				 * modhead list once it is loaded. So we can
				 * use the modulep pointer without search the
				 * list again.
				 */
				goto again;
			}
			LOCK_DEINIT(&modp->mod_lock);
			kmem_free(modp, sizeof(*modp));
			kmem_free(mcp, sizeof(struct modctl) + strlen(modname));

			/* return with modctlp->mod_lock held */
			*retmcp = modctlp;
			return (0);
		}
	}

	if (modctlp == NULL) {
		/* add to the beginning of the modhead list */
		if (modhead.mc_next == NULL)
			mcp->mc_id = 1;
		else
			mcp->mc_id = modhead.mc_next->mc_id + 1;
		mcp->mc_modp = modp;
		mcp->mc_next = modhead.mc_next;
		modhead.mc_next = mcp;
	}

	RW_UNLOCK(&modlist_lock, PLBASE);

	/* formulate parameters for the mod_load_helper */
	molp = kmem_alloc(sizeof(struct modld_args), KM_SLEEP);
	molp->mol_name = module_name;
	molp->mol_mcp = mcp;
	molp->mol_cred = credentials;
	molp->mol_flags = flags;
	EVENT_INIT(&molp->mol_done);

	if (spawn_sys_lwp(NULL, LWP_DETACHED, mod_load_helper, molp)) {
		retval = EAGAIN;
		if (retval)
			goto errout4;
	} else {
		EVENT_WAIT(&molp->mol_done, PRIMED);
		retval = molp->mol_ret;
		if (molp->mol_stuck)
			goto errout3;
		else if (retval)
			goto errout4;
	}

	/* done with the arguments structure */
	kmem_free(molp, sizeof(struct modld_args));

	/* success, tell anyone waiting for this module */
	(void)LOCK(&modp->mod_lock, PLDLM);
	modp->mod_loading_context = NULL;
	modp->mod_flags &= ~MOD_LOADING;
	if (SV_BLKD(&modp->mod_sv))
		SV_BROADCAST(&modp->mod_sv, 0);

	/* return with the modp->mod_lock held */
	*retmcp = mcp;
	return (0);

errout4:
	/* free up the memory allocated for the module */
	(void)RW_WRLOCK(&modlist_lock, PLDLM);
	mcp->mc_modp = NULL;
	RW_UNLOCK(&modlist_lock, PLBASE);
	SV_BROADCAST(&modp->mod_sv, 0);
errout:
	LOCK_DEINIT(&modp->mod_lock);
	kmem_free(modp, sizeof(*modp));
errout3:
	if (molp)
		kmem_free(molp, sizeof(struct modld_args));
	*retmcp = NULL;
	return retval;
}

/*
 * void
 * mod_load_helper(void *arg)
 *	Temporary helper lwp to load a module into memory.
 *
 * Calling/Exit State: 
 *	This function does the work for modld, but performs
 *	it in a separate LWP (actually in sysproc). The parent LWP
 *	can be swapped out at the time we are executing, so that
 *	this LWP cannot access automatic variables in the parent's stack.
 *
 *	mod_load_helper returns the result in the mol_ret field of its
 *	argument structure. It returns 0 on success or an errno (in most
 *	cases ERELOC) otherwise.
 */
void
mod_load_helper(void *argp)
{
	struct modld_args *molp = (struct modld_args *)argp;
	const char *module_name = molp->mol_name;
	cred_t *credentials = molp->mol_cred;
	uint_t flags = molp->mol_flags;
	struct modctl *mcp = molp->mol_mcp;
	struct module *modp = mcp->mc_modp;
	struct mod_conf_data *mcdp;
	int retval;
	int (*func)();
	int cpu;
	engine_t *engp;

	u.u_lwpp->l_name = "mod_load_helper";

	moddebug(cmn_err(CE_CONT, "!MOD: loading %s, module id %d.\n", 
		module_name, mcp->mc_id));

	molp->mol_stuck = B_FALSE;

	if ((retval = mod_obj_load(module_name, credentials, 
				   &modp->mod_obj, flags)) != 0) {
		moddebug(cmn_err(CE_CONT,
				 "!MOD: error loading %s\n", 
				 module_name));
		goto errout4;
	}

	(void)LOCK(&modp->mod_lock, PLDLM);
	modp->mod_flags |= MOD_SYMTABOK;
	UNLOCK(&modp->mod_lock, PLBASE);

	/*
	 * The revision number of the loadable module and the running
	 * kernel must match. Conversions from old modwrapper structures
	 * should be done by mod_obj_load().
	 */

	ASSERT(modp->mod_obj.md_mw->mw_rev == MODREV);

	mcdp = modp->mod_obj.md_mw->mw_conf_data;
	modp->mod_delay_tick = (clock_t)mcdp->mcd_unload_delay * HZ;

	/*
	 * call the _load() entry point in the module if the routine exists;
	 * if something failed in _load(), it should free up all the
	 * resource it allocated before return
	 */
	
	func = (int (*)())(*modp->mod_obj.md_mw->mw_load);
	if (func != NULL) {
		cpu = -1;
		if ((retval = mod_cpubind(modp, &cpu)) != 0)
			goto errout2;
		if (cpu != -1) {
			if (!engine_disable_offline(cpu)) {
				/*
				 *+ Could not lock engine on line.
				 */
				cmn_err(CE_NOTE,
		"MOD:modld: engine_disable_offline failed on processor %d\n",
					cpu);
				retval = EINVAL;
				goto errout2;
			}
			engp = kbind(&engine[cpu]);
			DISABLE_PRMPT();
			u.u_lwpp->l_notrt++;
		}

		retval = (*func)();	/* call _load */

		if (cpu != -1) {
			ASSERT(u.u_lwpp->l_notrt != 0);
			u.u_lwpp->l_notrt--;
			ENABLE_PRMPT();
			kunbind(engp);
		}

		moddebug(cmn_err(CE_CONT,
				 "!MOD: Returned from _load, retval = %x\n",
				 retval));

		if (retval != 0) {
			moddebug(cmn_err(CE_CONT, 
			       "!MOD: modld:%s(%s): _load returned error %d.\n",
					 module_name, mcp->mc_name, retval));
			goto errout2;
		}
	}

	if ((retval = mod_connect(modp)) != 0) {
		moddebug(cmn_err(CE_CONT, 
			"!MOD: modld:%s(%s): mod_connect returned error %d.\n",
			 	 module_name, mcp->mc_name, retval));
		/* undo whatever done in _load() */
		if ((func = (int (*)())(*modp->mod_obj.md_mw->mw_unload)) !=
		    NULL) {
			(void)(*func)();
		}
		goto errout2;
	}

	/*
	 * install stubs if any
	 */
	if ((retval = mod_install_stubs(mcp)) != 0) {
		cmn_err(CE_CONT,
		     "!MOD: cannot install stub functions for the module %s\n",
			mcp->mc_name);
		(void)LOCK(&modp->mod_lock, PLDLM);
		modp->mod_flags &= ~MOD_LOADING;
		modp->mod_flags |= MOD_UNLOADING;
		UNLOCK(&modp->mod_lock, PLBASE);
		UNLOCKMOD(&modp->mod_obj);
		if (modunld(mcp) != 0) {
			/*
			 *+ The installation of stubs in the loadable module
			 *+ failed, and the system cannot unload the module.
			 */
			cmn_err(CE_NOTE,
    "MOD: cannot unload module %s after failed installing stubs in the module.",
				mcp->mc_name);
			mod_remove_stubs(mcp);
		}
		goto errout3;
	}

	/* start paging the symbol table of the module if possible */
	RWSLEEP_RDLOCK(&symtab_lock, PRIMED);
	if (mod_symtab_lckcnt == 0) {
		segkvn_unlock(modp->mod_obj.md_symspace_mapcookie,
			      SEGKVN_MEM_LOCK);
	} else {
		int lckcnt = mod_symtab_lckcnt - 1;

		while (lckcnt-- != 0) {
			if (segkvn_lock(modp->mod_obj.md_symspace_mapcookie,
				        SEGKVN_MEM_LOCK)) {
				/*
				 *+ Failed to lock the symbol table for the
				 *+ module. The symbolic information will
				 *+ not be available for the module.
				 */
				cmn_err(CE_NOTE,
		    "MOD: Failed to lock symbol table for kernel module %s.\n",
					mcp->mc_name);
			}
		}
	}
	(void)LOCK(&modp->mod_lock, PLDLM);
	modp->mod_flags &= ~MOD_SYMLOCKED;
	UNLOCK(&modp->mod_lock, PLBASE);
	RWSLEEP_UNLOCK(&symtab_lock);

#ifdef DLM_PAGING
	/* unlock all the pages of the module */
	UNLOCKMOD(&modp->mod_obj);
#endif

	/* success */
	retval = 0;
	goto done;

errout3:
	molp->mol_stuck = B_TRUE;
	goto errout4;

errout2:
	/* unload the memory allocated for the module */
	(void)LOCK(&modp->mod_lock, PLDLM);
	modp->mod_flags &= ~MOD_SYMTABOK;
	UNLOCK(&modp->mod_lock, PLBASE);
#ifdef DLM_PAGING
	UNLOCKMOD(&modp->mod_obj);
#endif
	ASSERT(modp->mod_flags & MOD_SYMLOCKED);
	mod_obj_unload(&modp->mod_obj, B_TRUE);
errout4:
done:
	molp->mol_ret = retval;
	EVENT_SIGNAL(&molp->mol_done, 0);
	lwp_exit();
	/* NOTREACHED */
}


/*
 * static const char *getmodname(const char *module_name)
 *	Find the module name from a full path.
 *
 * Calling/Exit State:
 *	Return a pointer to the module name.	
 */
STATIC const char *
getmodname(const char *module_name)
{
	char *p = (char *)module_name + strlen(module_name);

	while (p != module_name && *(p - 1) != '/')
		--p;
	return (p);
}

/*
 * int unload_modules(size_t memsize)
 *	Unload loadable modules that are unloadable.
 *	Common interface for auto unload and demand unload.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 *	The memsize argument indicates the minimum size of memory the routine
 *	is supposed to recover by unloading modules.
 *
 *	If memsize is 0, the routine is called by demand unload. All the
 *	modules that are not in use will be unloaded.
 *
 *	If memsize is greater than 0, the routine is called by auto unload.
 *	All the modules whose holdcnt has been 0 for at least
 *	"unload_delay" seconds will be unloaded first. If the memory
 *	recovered is less than memsize, then remaining modules
 *	with holdcnt 0 will be unloaded one at a time until the memory
 *	recovered becomes greater than or equal to memsize;
 *	these modules will be unloaded in order of increasing
 *	remaining "unload_delay" time.
 *
 *	The routine will return non-zero if any memory was recovered.
 */
int
unload_modules(size_t memsize)
{
	struct modctl *mcp;
	struct module *modp;
	size_t size_freed = 0, modsize;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	(void)RW_RDLOCK(&modlist_lock, PLDLM);
	for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {

		if (mcp->mc_modp == NULL)
			continue;
		modp = mcp->mc_modp;

		(void)LOCK(&modp->mod_lock, PLDLM);
		RW_UNLOCK(&modlist_lock, PLDLM);

		/* turn off demand load flag if this is demand unload */
		if (memsize == 0 && modp->mod_flags & MOD_DEMAND) {
			modp->mod_flags &= ~MOD_DEMAND;
			MOD_RELE_L(modp, PLBASE);
		} else
			UNLOCK(&modp->mod_lock, PLBASE);

		(void)RW_RDLOCK(&modlist_lock, PLDLM);
		if ((modp = mcp->mc_modp) == NULL)
			continue;
		(void)LOCK(&modp->mod_lock, PLDLM);
		RW_UNLOCK(&modlist_lock, PLDLM);
		if (MODBUSY(modp) || (modp->mod_flags & MOD_TRANS) ||
		    (memsize != 0 && (NO_AUTOUNLD(modp) ||
				      UNLOAD_DELAY(modp) > 0))) {
			UNLOCK(&modp->mod_lock, PLBASE);
			moddebug(cmn_err(CE_CONT, 
			         "!MOD: unload_modules(): skip module %s\n",
				       mcp->mc_name));
		} else {
			modp->mod_flags |= MOD_UNLOADING;
			UNLOCK(&modp->mod_lock, PLBASE);
			modsize = modp->mod_obj.md_symsize +
				  modp->mod_obj.md_space_size +
				  modp->mod_obj.md_bss_size;
			if (modunld(mcp) == 0) {
				size_freed += modsize;
			} else {
				cmn_err(CE_CONT,
					"!MOD: can't unload module %s\n",
					mcp->mc_name);
			}
		}
		(void)RW_RDLOCK(&modlist_lock, PLDLM);
	}
	RW_UNLOCK(&modlist_lock, PLBASE);

	if (size_freed >= memsize)
		return (size_freed);

	while (size_freed < memsize) {
		if ((mcp = mod_min_delay()) == NULL)
			return (0);
		modp = mcp->mc_modp;

		modp->mod_flags |= MOD_UNLOADING;

		/* mod_min_delay() return with the module lock held */
		UNLOCK(&modp->mod_lock, PLBASE);
		modsize = modp->mod_obj.md_symsize +
			  modp->mod_obj.md_space_size +
			  modp->mod_obj.md_bss_size;
		if (modunld(mcp) == 0) {
			size_freed += modsize;
		} else {
			cmn_err(CE_CONT, "!MOD: can't unload module %s\n",
				mcp->mc_name);
		}
	}
	return (1);
}

/*
 * STATIC struct modctl *mod_min_delay(void)
 *	Get the module which is not busy and has the minimum amount of 
 *	unload delay time remain.
 *
 * Calling/Exit State:
 *	If no unloadable module found, the routine return NULL pointer.
 *	If an eligible module is found, the pointer to that module's
 *	modctl structure is returned with the module lock held.
 */
STATIC struct modctl *
mod_min_delay(void)
{
	struct modctl *mcp, *minmcp = NULL;
	struct module *modp;

	ASSERT(getpl() == PLBASE);

	(void)RW_WRLOCK(&modlist_lock, PLDLM);
	for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {

		if (mcp->mc_modp == NULL)
			continue;
		modp = mcp->mc_modp;

		(void)LOCK(&modp->mod_lock, PLDLM);
		if (MODBUSY(modp)) {
			UNLOCK(&modp->mod_lock, PLDLM);
			continue;
		}

		if (UNLOAD_DELAY(modp) <= 0) {
			RW_UNLOCK(&modlist_lock, PLDLM);
			return (mcp);
		}

		if ((minmcp == NULL) || (UNLOAD_DELAY(modp) <
					 UNLOAD_DELAY(minmcp->mc_modp)))
			minmcp = mcp;
		UNLOCK(&modp->mod_lock, PLDLM);
	}
	if (minmcp != NULL) {
		(void)LOCK(&minmcp->mc_modp->mod_lock, PLDLM);
		RW_UNLOCK(&modlist_lock, PLDLM);
	} else
		RW_UNLOCK(&modlist_lock, PLBASE);

	return (minmcp);
}

		
/*
 * static int modunld(struct modctl *mcp)
 *	Common kernel interface for auto unload and demand unload of a 
 *	loadable module.
 *
 * Calling/Exit State:
 *	The mcp argument gives a pointer to the modctl structure
 *	of the module to be unloaded.
 *
 *	The routine is called with the MOD_UNLOADING flag set for
 *	the module which mcp points to, so the module will not be
 *	unloaded by other control thread.
 *
 *	If the module unloaded successfully, the routine returns 0;
 *	otherwise the routine returns a valid errno and unset the 
 *	MOD_UNLOADING flag.
 *
 *	If this routine is called by demand unload, the flag
 *	MOD_DEMAND should be unset already in the moduload()
 *	or unload_modules() before entering this routine. 
 *	In case the demand unload failed, the module will be
 *	available for auto unload.
 *	So, if the MOD_DEMAND flag is still set when entering this routine 
 *	it must be auto unloading a demand loaded module, the routine will 
 *	return EBUSY.
 */
STATIC int
modunld(struct modctl *mcp)
{
	int retval;
	int (*func)();
	struct module *modp;
	int cpu;
	struct engine *engp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	modp = mcp->mc_modp;

	(void)LOCK(&modp->mod_lock, PLDLM);
	if (modp->mod_flags & MOD_DEMAND) {
		modp->mod_flags &= ~MOD_UNLOADING;
		UNLOCK(&modp->mod_lock, PLBASE);
		return (EBUSY);
	}

	UNLOCK(&modp->mod_lock, PLBASE);
	mod_disconn(modp);
	mod_remove_stubs(mcp);

	LOCKMOD(&modp->mod_obj);

	if ((func = modp->mod_obj.md_mw->mw_unload) != NULL) {

		cpu = -1;
		if ((retval = mod_cpubind(modp, &cpu)) != 0) {
			UNLOCKMOD(&modp->mod_obj);
			return (retval);
		}
		if (cpu != -1) {
			engp = kbind(&engine[cpu]);
			DISABLE_PRMPT();
			u.u_lwpp->l_notrt++;
		}

		/* call _unload */
		retval = (*func)();

		if (cpu != -1) {
			ASSERT(u.u_lwpp->l_notrt != 0);
			u.u_lwpp->l_notrt--;
			ENABLE_PRMPT();
			kunbind(engp);
		}

		if (retval != 0) {
			/*
			 * try to restore the switch tables if the
			 * _unload() routine failed
			 */
			if (mod_connect(modp) != 0 ||
			    mod_install_stubs(mcp) != 0) {
				/*
				 *+ The unload of a loadable module failed, and
				 *+ the system cannot reset the state of the 
				 *+ module which is being unloaded.
				 */
				cmn_err(CE_PANIC, 
			"MOD: can't reconnect the module %s back to the kernel",
					 mcp->mc_name);
			}
			UNLOCKMOD(&modp->mod_obj);
			(void)LOCK(&modp->mod_lock, PLDLM);
			modp->mod_flags &= ~MOD_UNLOADING;
			UNLOCK(&modp->mod_lock, PLBASE);
			return (retval);
		}
	}

	UNLOCKMOD(&modp->mod_obj);

	moddebug(cmn_err(CE_CONT, "!MOD: unloading %s, module id %d.\n",
		         mcp->mc_name, mcp->mc_id));

	/* unlock and free the module and it's symbol table */
	modp->mod_flags &= ~MOD_SYMTABOK;
	mod_obj_unload(&modp->mod_obj, (modp->mod_flags & MOD_SYMLOCKED));

	(void)RW_WRLOCK(&modlist_lock, PLDLM);
	mcp->mc_modp = NULL;
	RW_UNLOCK(&modlist_lock, PLBASE);
	if (SV_BLKD(&modp->mod_sv))
		SV_BROADCAST(&modp->mod_sv, 0);
	LOCK_DEINIT(&modp->mod_lock);
	kmem_free(modp, sizeof(*modp));
	return (0);
}

/*
 * STATIC int mod_cpubind(struct module *modp, int *cpup)
 *	Decide whether a module should be bound to a cpu.
 *	If yes, return the cpu number in cpup.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 */
STATIC int
mod_cpubind(struct module *modp, int *cpup)
{
	struct modlink *linkp;
	int retval;
	extern int default_bindcpu;

	ASSERT(KS_HOLD0LOCKS());

	linkp = modp->mod_obj.md_mw->mw_modlink;

	while (linkp->ml_ops != NULL) {
		retval = MODL_BIND(linkp, cpup);
		if (retval != 0)
			return (retval);
		if (*cpup == -2)
			*cpup = default_bindcpu;
		linkp++;
	}
	return (0);
}
			
/*
 * STATIC int mod_connect(struct module *modp)
 *	Connect a module to the running kernel.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 */
STATIC int
mod_connect(struct module *modp)
{
	int retval = 0;
	struct modlink *linkp, *linkp1;

	ASSERT(KS_HOLD0LOCKS());

	linkp = modp->mod_obj.md_mw->mw_modlink;

	while (linkp->ml_ops != NULL) {
		if ((retval = MODL_INSTALL(linkp, modp)) != 0) {

			/*
			 * if one of the connection failed,
			 * remove all the installed linkage
			 */
			linkp1 = modp->mod_obj.md_mw->mw_modlink;

			while (linkp1 != linkp) {
				/* MODL_REMOVE should not fail */
				MODL_REMOVE(linkp1); /* clean up */
				linkp1++;
			}
			break;
		}
		linkp++;
	}

	return (retval);
}

/*
 * STATIC int mod_disconn(struct module *modp)
 *	Disconnect a module from the running kernel.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 *
 * Remark:
 *	The type specific remove routine should never fail.
 */
STATIC int
mod_disconn(struct module *modp)
{
	struct modlink *linkp;

	ASSERT(KS_HOLD0LOCKS());

	/* loop through ml_linkage */
	linkp = modp->mod_obj.md_mw->mw_modlink;
	while (linkp->ml_ops != NULL) {
		MODL_REMOVE(linkp);
		linkp++;
	}

	return (0);
}

/*
 * STATIC int mod_info(struct module *modp, struct modstatus *modstatusp)
 *	Get loadable module status.
 *
 * Calling/Exit State:
 *	The mod_keepcnt of the specified module is non-zero upon
 *	entry and exit of this routine, so the module will not
 *	be unloaded.
 */
STATIC int
mod_info(struct module *modp, struct modstatus *modstatusp)
{
	int i;
	int retval = 0;
	struct modspecific_stat *mssp;
	struct modlink *linkp;

	modstatusp->ms_rev = modp->mod_obj.md_mw->mw_rev;

	linkp = modp->mod_obj.md_mw->mw_modlink;
	mssp = &modstatusp->ms_msinfo[0];

	for (i = 0; i < MODMAXLINK; i++) {
		if (linkp->ml_ops == NULL)
			break;
		strncpy(mssp->mss_linkinfo,
			((struct mod_type_data *)linkp->ml_type_data)->mtd_info,
			MODMAXLINKINFOLEN);
		if ((retval = MODL_INFO(linkp, mssp->mss_p0, mssp->mss_p1,
					&mssp->mss_type)) != 0)
			return (retval);
		linkp++;
		mssp++;
	}
	return (0);
}

/*
 * int mod_stub_load(struct mod_stub_modinfo *mip)
 *	Install a loadable module through the stub function.
 *
 * Calling/Exit State:
 *	No locks held upon entry and exit.
 *
 *	The routine will return 0 if the stub load successfully;
 *	otherwise return 1.
 *
 *	It is required to have an error function specified.
 *	The system will panic if the load failed and there is no
 *	error function specified for the stub function.
 */
int
mod_stub_load(struct mod_stub_modinfo *mip)
{
	struct modctl *mcp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	if (modld(mip->modm_module_name, sys_cred, &mcp, 0) == 0) {
		/* unlock the module */
		UNLOCK(&mcp->mc_modp->mod_lock, PLBASE);
		return (0);
	}

	return (1);
}

/*
 * static int mod_install_stubs(struct modctl *mcp)
 *	Replace all the stub functions in the module with real functions.
 *
 * Calling/Exit State:
 *	No locks held upon entry and exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errno returned directly by this routine is:
 *
 *		ERELOC	if any stub function is not properly defined
 *
 * Remarks:
 *	The module auto loaded through a well defined interface should
 *	not have any stub function.
 */
STATIC int
mod_install_stubs(struct modctl *mcp)
{
	struct mod_stub_modinfo *mp;
	struct mod_stub_info *sp;
	unsigned long offset, funcadr;
	char *funcname;
	char namebuf[MODMAXNAMELEN + 9];
	int i;
	char *fnamebuf;
	struct module *modp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	strcpy(namebuf, mcp->mc_name);
	strcat(namebuf, "_modinfo");

	if ((mp = (struct mod_stub_modinfo *)mod_obj_getsymvalue(namebuf, 
		B_FALSE, SLEEP)) == 0)
		return (0);

	modp = mcp->mc_modp;
	modp->mod_stub_info = (caddr_t)mp;
	fnamebuf = kmem_alloc(MAXSYMNMLEN, KM_SLEEP);

	for (i = 0, sp = mp->modm_stubs; sp->mods_func_adr; i++, sp++) {
		funcname = mod_obj_getsymname((unsigned long)sp->mods_stub_adr,
						&offset, SLEEP, fnamebuf);
		if (funcname == NULL) {
			kmem_free(fnamebuf, MAXSYMNMLEN);
			/*
			 *+ Cannot find the symbol name of a stub in the 
			 *+ running kernel.
			 */
			cmn_err(CE_NOTE, 
		"!MOD: mod_install_stubs: can't find symbol name for 0x%x",
				sp->mods_stub_adr);

			return (ERELOC);
		}
		funcadr = mod_obj_lookup(&modp->mod_obj, funcname);
		if (funcadr == 0) {
			kmem_free(fnamebuf, MAXSYMNMLEN);
			/*
			 *+ The stub routine in the running kernel which
			 *+ cause the auto load is not properly defined in
			 *+ the loadable module specified in the stub entry.
			 */
			cmn_err(CE_NOTE, 
	"!MOD: mod_install_stubs: %s() not defined properly in module %s",
				funcname, mcp->mc_name);

			return (ERELOC);
		}
		sp->mods_func_adr = (caddr_t)funcadr;
	}

	kmem_free(fnamebuf, MAXSYMNMLEN);
	/*
	 * There is no need to lock here because if
	 * mod_ustub_hold() picks up a NULL modp, it
	 * will block in modld() until this load has
	 * completed. modld() will then  return this
	 * modp to mod_ustub_hold().
	 */
	mp->modm_modp = modp;

	/* If the module is not unloadable, lock it in with an extra hold */
	if (i > 0 && !(mp->modm_flags & MOD_USTUB))
		MOD_HOLD(modp);
	return (0);
}

/*
 * int mod_ustub_hold(struct mod_stub_info *stub)
 *	Load and/or hold the module associated with an unloadable stub.
 *
 * Calling/Exit State:
 *	No locks held upon entry and exit.
 *
 *	A return value of 0 indicates success, non-zero failure.
 */
int
mod_ustub_hold(struct mod_stub_info *stub)
{
	struct modctl		*mcp;
	struct module		*modp;
	struct mod_stub_modinfo	*mip;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());

	mip = stub->mods_modinfo;

	if (!(mip->modm_flags & MOD_USTUB))
		return (-1);

	if (mip->modm_lock == NULL) {
		/*
		 *+ The unloadable stubs for the loadable module are
		 *+ not appropriately initialized. This indicates a
		 *+ problem in the configuration files for the module.
		 */
		cmn_err(CE_NOTE,
	"The unloadable stubs for %s are not appropriately initialized.\n",
			mip->modm_module_name);
		return (-1);
	}
	pl = RW_RDLOCK(mip->modm_lock, PLDLM);

	if ((modp = mip->modm_modp) != NULL &&
	    !MOD_IS_UNLOADING(modp)) {
		RW_UNLOCK(mip->modm_lock, PLDLM);
		MOD_HOLD_L(modp, pl);
		return (0);
	}

	RW_UNLOCK(mip->modm_lock, pl);

	if (modld(mip->modm_module_name, sys_cred, &mcp, 0) != 0)
		return (-1);

	MOD_HOLD_L(mcp->mc_modp, pl);

	/*
	 * The module loaded OK, but the function address for
	 * this stub has not been updated.
	 */
	if (stub->mods_func_adr == (caddr_t)-1)	{
		MOD_RELE(mcp->mc_modp);
		return (-1);
	}

	return (0);
}

/*
 * int mod_ustub_rele(struct mod_stub_modinfo *mip)
 *	Release the module associated with an unloadable stub.
 *
 * Calling/Exit State:
 *	No locks held upon entry and exit.
 *
 *	No return value.
 */
void
mod_ustub_rele(struct mod_stub_modinfo *mip)
{
	MOD_RELE(mip->modm_modp);
}

/*
 * STATIC void
 * mod_remove_stubs(struct modctl *mcp)
 *	Reset all the stubs associated with the module specified by mcp.
 *
 * Calling/Exit State:
 *	No locks held upon entry and exit.
 */
STATIC void
mod_remove_stubs(struct modctl *mcp)
{
	struct mod_stub_modinfo *mp;
	struct mod_stub_info *sp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/* LINTED pointer alignment */
	if ((mp = (struct mod_stub_modinfo *)mcp->mc_modp->mod_stub_info) ==
	    NULL)
		return;
	
	for (sp = mp->modm_stubs; sp->mods_func_adr; sp++)
		sp->mods_func_adr = sp->mods_func_save;

	/* return if this is not unloadable stub */
	if (!(mp->modm_flags & MOD_USTUB))
		return;

	(void)RW_WRLOCK(mp->modm_lock, PLDLM);
	mp->modm_modp = NULL;
	RW_UNLOCK(mp->modm_lock, PLBASE);
}

/*
 * int mod_static(const chat *mod_name)
 *
 *	Check the input module name against the static_modules
 *	array to see if the module is already statically linked
 *	with the running kernel.
 *
 * Calling/Exit State:
 *	Returns 1 if the module whose name is pointed to by mod_name
 *	is statically configured in the running kernel, 0 otherwise.
 */
int
mod_static(const char *mod_name)
{
	char **sm;
	extern char *static_modules[];

	for (sm = static_modules; *sm != NULL; sm++) {
		if (strcmp(mod_name, *sm) == 0)
			return (1);
	}

	return (0);
}

/*
 * void modhalt(void)
 * 	Loops through the modhead list calling the halt(D2D)
 *	routines of currently loaded modules.
 *
 * Calling/Exit State:
 *	Called on system shutdown from io_halt[] table.
 *	Only one processor is active at the time.
 */
void
modhalt(void)
{
	struct modctl *mcp;
	struct module *modp;
	void	(*func)();
	int cpu;
	struct engine *engp;

	moddebug(cmn_err(CE_CONT, "MOD: modhalt()\n"));

	for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {
		if (mcp->mc_modp == NULL ||
		    (modp = mcp->mc_modp)->mod_holdcnt == 0)
			continue;

		moddebug(cmn_err(CE_CONT, "MOD: %s, halt = 0x%x\n",
			mcp->mc_name, modp->mod_obj.md_mw->mw_halt));
		if ((func = modp->mod_obj.md_mw->mw_halt) != NULL) {
			moddebug(cmn_err(CE_CONT, "MOD: halt called\n"));

			cpu = -1;
			if (mod_cpubind(modp, &cpu) != 0)
				continue;
			if (cpu != -1) {
				engp = kbind(&engine[cpu]);
				DISABLE_PRMPT();
				u.u_lwpp->l_notrt++;
			}

			(*func)();

			if (cpu != -1) {
				ASSERT(u.u_lwpp->l_notrt != 0);
				u.u_lwpp->l_notrt--;
				ENABLE_PRMPT();
				kunbind(engp);
			}
		}
	}
}

/*
 * void mod_rele_byname(char *modname)
 *
 *	Decrement the hold count of the module specified by modname.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry and exit.
 */
void
mod_rele_byname(const char *modname)
{
	struct modctl *mcp;
	struct module *modp;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());

	pl = RW_RDLOCK(&modlist_lock, PLDLM);
	for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {
		if ((modp = mcp->mc_modp) == NULL)
			continue;
		if (strcmp(modname, mcp->mc_name) == 0) {
			(void)LOCK(&modp->mod_lock, PLDLM);
			RW_UNLOCK(&modlist_lock, PLDLM);
			MOD_RELE_L(modp, pl);
			return;
		}
	}
	RW_UNLOCK(&modlist_lock, pl);
}
			
/*
 * int mod_zero(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return 0.
 */
int
mod_zero(void)
{
	return (0);
}

/*
 * int mod_minus(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return -1.
 */
int
mod_minus(void)
{
	return (-1);
}

/*
 * int mod_einval(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return EINVAL.
 */
int
mod_einval()
{
	return (EINVAL);
}

/*
 * int mod_enosys(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return ENOSYS.
 */
int
mod_enosys()
{
	return (ENOSYS);
}
/*
 * int mod_enoload(void)
 *	Return function used by stubs mechanism.
 *
 * Calling/Exit State:
 *	Return ENOLOAD.
 */
int
mod_enoload()
{
	return (ENOLOAD);
}

/*
 * void mod_hold_locked(struct module *modp, pl_t pl)
 *
 *	Routine to increase the hold count of a module while
 *	the module is locked.
 *
 * Calling/Exit State:
 *	Called with the module locked. Exit with module unlocked.
 */
void
mod_hold_locked(struct module *modp, pl_t pl)
{
	while ((modp)->mod_flags & MOD_LOCKING) {
		(modp)->mod_keepcnt++;
		SV_WAIT(&(modp)->mod_sv, PRIZERO, &(modp)->mod_lock);
		(void)LOCK(&(modp)->mod_lock, PLDLM);
		(modp)->mod_keepcnt--;
	}
	if ((modp)->mod_holdcnt++ == 0) {
		(modp)->mod_flags |= MOD_LOCKING;
		UNLOCK(&(modp)->mod_lock, PLBASE);
		LOCKMOD(&(modp)->mod_obj);
		(void)LOCK(&(modp)->mod_lock, PLDLM);
		(modp)->mod_flags &= ~MOD_LOCKING;
		UNLOCK(&(modp)->mod_lock, pl);
		if (SV_BLKD(&(modp)->mod_sv))
			SV_BROADCAST(&(modp)->mod_sv, 0);
	} else {
		UNLOCK(&(modp)->mod_lock, pl);
	}
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_static_modules(void)
 * 	Formatted dump of the static_modules array
 *
 * Calling/Exit State:
 * 	None. 
 */
void
print_static_modules(void)
{
        extern char *static_modules[];
        char **sm;
	int i = 0;

	debug_printf("static_modules:\n\t");
        for (sm = static_modules; *sm != NULL; sm++) {
		if (sm != static_modules) {
			if (i++ == 8) {
				debug_printf(",\n\t");
				i = 1;
			} else
				debug_printf(", ");
		}
		debug_printf("%s", *sm);
		if (debug_output_aborted())
			return;
        }
	debug_printf("\n");
}

#endif /* DEBUG || DEBUG_TOOLS */

/*
 * void
 * autounload_wakeup(void)
 *	Wake up the autounload daemon.
 *
 * Calling/Exit State:
 *	Called every unload_interval seconds.
 *	The value of unload_interval is a tunable.
 */
void
autounload_wakeup(void)
{
	EVENT_SIGNAL(&autounload_event, 0);
}

/*
 * void
 * autounload(void *unload_argp)
 *	This daemon is woken up every unload_interval seconds.
 *	The value of unload_interval is a tunable.
 *	This daemon will unload all the loaded modules that
 *	are not pageable, and not in use for at least unload_delay
 *	seconds. The unload_delay is a per-module tunable. 
 *	If the unload_delay is -1, the module will not be auto
 *	unloaded.
 *
 * Calling/Exit State:
 *	Never exits.
 */
/* ARGSUSED */
void
autounload(void *unload_argp)
{
	struct modctl *mcp;
	struct module *modp;

	u.u_lwpp->l_name = "autounload";

	for (;;) {
		EVENT_WAIT(&autounload_event, PRIMED);

		(void)RW_RDLOCK(&modlist_lock, PLDLM);
		for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {

			if (mcp->mc_modp == NULL)
				continue;
			modp = mcp->mc_modp;

			(void)LOCK(&modp->mod_lock, PLDLM);
			RW_UNLOCK(&modlist_lock, PLDLM);

			if (MODBUSY(modp) || (UNLOAD_DELAY(modp) > 0) ||
			    NO_AUTOUNLD(modp) || PAGEABLE(modp) ||
			    (modp->mod_flags & MOD_TRANS)) {
				UNLOCK(&modp->mod_lock, PLBASE);
				moddebug(cmn_err(CE_CONT, 
			       		"!MOD: autounload(): skip module %s\n",
					mcp->mc_name));
			} else {
				modp->mod_flags |= MOD_UNLOADING;
				UNLOCK(&modp->mod_lock, PLBASE);
				if (modunld(mcp) != 0) {
					cmn_err(CE_CONT,
					    "!MOD: can't unload module %s\n",
						mcp->mc_name);
				}
			}
			(void)RW_RDLOCK(&modlist_lock, PLDLM);
		}
		RW_UNLOCK(&modlist_lock, PLBASE);
	}
}

/*
 * void
 * modpostroot(void)
 *	Post-root initializations.
 *
 * Calling/Exit State:
 *	Called from main() after mounting root.
 */
void
modpostroot(void)
{
	extern ulong_t unload_interval;

	EVENT_INIT(&autounload_event);

	/* spawn autounload daemon */
	(void)spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL,
			autounload, NULL);
	
	/* TO_PERIODIC */
	itimeout(autounload_wakeup, NULL, unload_interval | TO_PERIODIC,
		 PLBASE);
}
