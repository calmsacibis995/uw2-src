/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/modinit.c	1.17"
#ident	"$Header: $"

#include <fs/memfs/memfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/seg_kvn.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ghier.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/sysmacros.h>
#include <util/types.h>

/* The following two variables will be filled in by unixsyms */
size_t mod_obj_size = 0;
struct modobj *mod_obj_kern = NULL;

boolean_t mod_initialized = B_FALSE;
page_t *mod_obj_plist;

int mod_symtab_lckcnt;
rwsleep_t symtab_lock;
rwlock_t modlist_lock;
rwlock_t mod_bdevsw_lock, mod_cdevsw_lock, mod_fmodsw_lock;
rwlock_t mod_execsw_lock, mod_vfssw_lock;
rwsleep_t mod_mpath_lock;
lock_t	mod_iv_lock;

/* head of the struct modctl link list */
struct modctl modhead;

void modinit(void);
extern int mod_obj_pagesymtab;	/* tunable */
extern void *calloc(ulong_t);
extern struct mod_stub_modinfo *mod_stub_tab[];

LKINFO_DECL(mod_lkinfo, "KU:dlm:mod_lock", 0);
LKINFO_DECL(mod_mpath_lkinfo, "KU:dlm:mod_mpath_lock", 0);
LKINFO_DECL(modlist_lkinfo, "KU:dlm:modlist_lock", 0);
LKINFO_DECL(symtab_lkinfo, "KU:dlm:symtab_lock", 0);
LKINFO_DECL(bdevsw_lkinfo, "IO:bdevsw:mod_bdevsw_lock", 0);
LKINFO_DECL(cdevsw_lkinfo, "IO:cdevsw:mod_cdevsw_lock", 0);
LKINFO_DECL(fmodsw_lkinfo, "IS:fmodsw:mod_fmodsw_lock", 0);
LKINFO_DECL(vfssw_lkinfo, "FS:vfssw:mod_vfssw_lock", 0);
LKINFO_DECL(execsw_lkinfo, "PP:execsw:mod_execsw_lock", 0);
LKINFO_DECL(mod_ustub_lkinfo, "KU:dlm:mod_ustub_lock", 0);
LKINFO_DECL(mod_iv_lkinfo, "KU:dlm:mod_iv_locks", 0);

/*
 * void mod_obj_kern_init(void)
 *	Relocate the struct module for the static kernel 
 *	which pointed by mod_obj_kern to a new location,
 *	so it will not become pageable later.
 *
 * Calling/Exit State:
 *	Called from sysinit(). The original location where
 *	boot program loaded the symbol table is saved in
 *	mod_obj_kern_symbase.
 */
void
mod_obj_kern_init(void)
{
	if (mod_obj_kern == NULL)
		return;

	/* initialized the head of the modctl list */
	modhead.mc_next = NULL;
	modhead.mc_id = 0;
	modhead.mc_modp = (struct module *)calloc(sizeof(struct module));
	modhead.mc_modp->mod_obj = *mod_obj_kern;

	mod_symtab_lckcnt = 1;
}

/*
 * void mod_stub_init(void)
 *	Allocate and initialize the locks for the unloadable stubs
 *	configured into the system.
 *
 * Calling/Exit State:
 *	Called from modinit().
 *	No return value.
 */
STATIC void
mod_stub_init(void)
{
	struct mod_stub_modinfo	*mip, **mipp;

	for(mipp = &mod_stub_tab[0]; (mip = *mipp) != NULL; mipp++)	{
		if(mip->modm_flags & MOD_USTUB == 0)	{
			/*
			 * This is not an unloadable stub.
			 */
			continue;
		}

		if((mip->modm_lock =
		    RW_ALLOC(DLM_HIER_SWTAB, PLDLM,
			     &mod_ustub_lkinfo, KM_NOSLEEP)) == NULL) {

			/*
			 *+ The system can't allocate the lock for the
			 *+ unloadable stubs associated with a given module.
			 *+ As a result, stub access to the module is disabled.
			 */
			cmn_err(CE_WARN,
				"Can't allocate stub lock for module: %s\n",
				mip->modm_module_name);
			continue;
		}
	}
}

/*
 * void modinit(void)
 *	Sets up pageable symbol table for the static kernel.
 *	And initialize locks for the loadable modules mechanism.
 *
 * Calling/Exit State:
 *	Called from sysinit().
 *	modulename is the full pathname from whence the kernel was booted. 
 *	Relies on earlier setup by unixsyms, mod_obj_kern_init(), and 
 *	VM init code.
 *	On return, all the mod_obj_kern fields are correct for the 
 *	kernel symbol table. If mod_obj_kern is NULL on call, nothing 
 *	is done and no kernel symbol table will be available.
 */
void
modinit(void)
{
	vaddr_t base;
	long addrel;
 	vnode_t *vp;
 	void *mapcookie;
	struct module *modp;
	extern void mod_fs_init(void);

	RW_INIT(&modlist_lock, DLM_HIER_LIST, PLDLM, &modlist_lkinfo,
		KM_SLEEP);
	RWSLEEP_INIT(&symtab_lock, 0, &symtab_lkinfo, KM_SLEEP);
	RWSLEEP_INIT(&mod_mpath_lock, 0, &mod_mpath_lkinfo, KM_SLEEP);
	RW_INIT(&mod_bdevsw_lock, DLM_HIER_SWTAB, PLDLM, &bdevsw_lkinfo,
		  KM_SLEEP);
	RW_INIT(&mod_cdevsw_lock, DLM_HIER_SWTAB, PLDLM, &cdevsw_lkinfo,
		  KM_SLEEP);
	RW_INIT(&mod_fmodsw_lock, DLM_FMODSW_HIER, PLDLM, &fmodsw_lkinfo,
		  KM_SLEEP);
	RW_INIT(&mod_vfssw_lock, DLM_HIER_SWTAB, PLDLM, &vfssw_lkinfo,
		  KM_SLEEP);
	RW_INIT(&mod_execsw_lock, DLM_HIER_SWTAB, PLDLM, &execsw_lkinfo,
		  KM_SLEEP);
	LOCK_INIT(&mod_iv_lock, DLM_HIER_IVECT, PLIV, &mod_iv_lkinfo,
		  KM_SLEEP);

	if (mod_obj_kern == NULL) {
		/*
		 *+ If unixsyms was not run and hence mod_obj_kern not
		 *+ filled in, no kernel symbol table is available.
		 *+ The loadable module will not work and no symbolic
		 *+ information will be available for getksym() and kdb.
		 */
		cmn_err(CE_WARN,
	"Dynamic symbol table for kernel is not constructed appropriately.\nNo symbolic information will be available, hence the loadable\nmodule mechanism will not work.\n");
		return;
	}

	/*
	 * Obtain a memfs file for mapping the kernel's symbol table
	 * into kernel virtual, binding the kernel symbol table pages
	 * into the file.
	 *
	 * memfs_bind() will free the symbol table pages and release the
	 * M_REAL reservation we hold for these pages. However, the pages
	 * will end up on the page_dirtyalist, so that they cannot be reused
	 * for another purpose until they are cleaned. Since it is too early
	 * for swap space to be added, they cannot be cleaned at this time.
	 *
	 * This call also passes control of the M_SWAP reservation (which
	 * we hold for the symbol table pages) to memfs.
	 */
	vp = memfs_create_unnamed( mod_obj_size, MEMFS_NORESV);
	memfs_bind(vp, mod_obj_size, mod_obj_plist);

	/*
	 * Now, map the vnode into kernel virtual space.
	 */
	base = segkvn_vp_mapin(0, mod_obj_size, 0, vp, 0, SEGKVN_NOFLAGS,
			       &mapcookie);

	/*
	 * The symbol table is pageable now, so release the initial lock.
	 */
	if (mod_obj_pagesymtab != MOD_NOPAGE)
		mod_symtab_lckcnt--;

	/*
	 * Lock the pages of the symbol table into memory if we so required.
	 */
	if (mod_symtab_lckcnt != 0) {
		if (segkvn_lock(mapcookie, SEGKVN_MEM_LOCK)) {
			/*
			 *+ Failed to lock the kernel symbol table. The
			 *+ symbolic information will not be available
			 *+ for the static kernel.
			 */
			cmn_err(CE_WARN,
			    "MOD: Failed to lock the kernel symbol table.\n");
		}
	}

	addrel = (long)base - (long)mod_obj_kern;
	modp = modhead.mc_modp;
	modp->mod_obj.md_symspace += addrel;
	modp->mod_obj.md_strings += addrel;
	modp->mod_obj.md_buckets = 
		/* LINTED pointer alignment */
		(ulong_t *)((char *)modp->mod_obj.md_buckets + addrel);

	modp->mod_obj.md_chains = 
		/* LINTED pointer alignment */
		(ulong_t *)((char *)modp->mod_obj.md_chains + addrel);

	modp->mod_obj.md_path = (char *)NULL;

	LOCK_INIT(&modp->mod_lock, DLM_HIER_CTL, PLDLM, &mod_lkinfo, KM_SLEEP);
	SV_INIT(&modp->mod_sv);

	/*
	 * Save the mapcookie for the kernel symbol table in the
	 * kernel's modobj structure.
	 */
	modp->mod_obj.md_symspace_mapcookie = mapcookie;

	/*
	 * Initialize the locks for unloadable stubs.
	 */
	mod_stub_init();

	/*
	 * Type-specific initialization.
	 */
	mod_fs_init();

	mod_initialized = B_TRUE;
}
