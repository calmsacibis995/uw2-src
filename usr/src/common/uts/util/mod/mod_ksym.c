/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/mod_ksym.c	1.18"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <mem/as.h>
#include <mem/faultcatch.h>
#include <mem/faultcode.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/seg_kvn.h>
#include <mem/seg_vn.h>
#include <mem/seg_kmem.h>
#include <mem/vmparam.h>
#include <proc/obj/elf.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/mod/ksym.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_obj.h>
#include <util/sysmacros.h>
#include <util/types.h>

/* The module id for kernel module is 0 */
#define IS_KERN_MOD(mcp)	((mcp)->mc_id == 0)

STATIC boolean_t mod_obj_addrcheck(const struct modobj *, ulong_t);
STATIC char *mod_obj_searchsym(const struct modobj *, ulong_t, ulong_t *);
STATIC int mod_obj_getsymp(const char *, boolean_t, int, Elf32_Sym *);
STATIC int mod_obj_lookupsym(const struct modobj *, ulong_t, const char *, 
				Elf32_Sym *);

ulong_t mod_obj_hashname(const char *);

extern int mod_obj_pagesymtab;		/* tunable */
extern boolean_t mod_initialized;

struct gksyma {
	char *symname;
	ulong_t *value;
	ulong_t *info;
};

/*
 * int getksym(struct gksyma *uap, rval_t *rvp)
 *	System call interface to get information about a symbol in the 
 *	dynamic symbol table.
 *
 * Calling/Exit State:
 *	On call, if the space pointed to by uap->value contains 0,
 *	then the routine tries to locate a symbol with the name given in
 *	uap->symname and returns the value of that symbol in the space
 *	pointed to by uap->value and its type in the space pointed to by 
 *	uap->info. Otherwise, the routine tries to locate the symbol whose
 *	value is the closest one less than or equal to uap->value and 
 *	returns its name in uap->name and the difference in values in 
 *	uap->info.  rvp is used to return 0 on success and non-zero on 
 *	failure. The routine returns 0 on success and the appropriate 
 *	errno on failure.
 */
/* ARGSUSED */
int
getksym(struct gksyma *uap, rval_t *rvp)
{

	ulong_t kvalue;
	ulong_t kinfo;
	char *kname;
	Elf32_Sym sp;
	int error;

	if ((error = ucopyin((char *)uap->value, (char *)&kvalue, 
	    sizeof(ulong_t), 0)) != 0)
		return (error);


	kname = kmem_alloc(MAXSYMNMLEN, KM_SLEEP);
	if (kvalue == 0) {
		/* assume want to look up value for symbol in name */
		if ((error = copyinstr(uap->symname, kname, MAXSYMNMLEN, NULL))
		    != 0)
			goto getksym_return;

		if (mod_obj_getsymp(kname, B_FALSE, SLEEP, &sp) != 0) {
			error = ENOMATCH;
			goto getksym_return;
		}
		kinfo = ELF32_ST_TYPE(sp.st_info);
		if ((error = ucopyout((char *)&sp.st_value, (char *)uap->value,
		    sizeof(ulong_t), 0)) == 0) {
			error = ucopyout((char *)&kinfo, (char *)uap->info,
					 sizeof(ulong_t), 0);
		}
		goto getksym_return;
	}

	/*
	 * assume want to find symbol whose address is closest to but 
	 * not greater than kvalue
	 */

	if (mod_obj_getsymname(kvalue, &kinfo, SLEEP, kname) == NULL) {
		error = ENOMATCH;
		goto getksym_return;
	}

	if ((error = ucopyout((char *)&kinfo, (char *)uap->info,
	    sizeof(ulong_t), 0)) != 0)
		goto getksym_return;

	error = ucopyout(kname, uap->symname, strlen(kname) + 1, 0);
	
getksym_return:
	kmem_free(kname, MAXSYMNMLEN);
	return (error);
}


/*
 * int mod_obj_lookupone(const struct modobj *mp, const char *name, 
 *	int flags, Elf32_Sym *retsp)
 *
 *	Find symbol table entry associated with the symbol given by name 
 *	in the module given by mp. 
 *
 * Calling/Exit State: 
 *	Called from mod_obj_getsymp, mod_obj_ioksym, mod_obj_lookupall,
 *	and mod_obj_lookup.
 *	This routine is called with the module being held.
 *
 *	If the symbol table for this module is pageable and the
 *	NOSLEEP flags is set, or the symbol is not found in the module, then 
 *	ENOMATCH is returned else 0 is returned and the symbol table entry
 *	for the symbol is copied to the space pointed to by retsp if retsp
 *	is non-NULL.
 *
 *	The NOSLEEP flag only set when kdb calls this routine indirectly.
 *	So, no locks can be acquired while the flag is NOSLEEP.
 */
int
mod_obj_lookupone(const struct modobj *mp, const char *name, 
	int flags, Elf32_Sym *retsp)
{
	ulong_t hval;
	int error, perror;

	ASSERT(mp != NULL);

	if (mod_symtab_lckcnt == 0 && (flags & NOSLEEP))
		return (ENOMATCH);

	if (!(flags & NOSLEEP)) {
		ASSERT(KS_HOLD0LOCKS());
		(void)RWSLEEP_RDLOCK(&symtab_lock, PRIMED);
	}

	hval = mod_obj_hashname(name);

	if (mod_symtab_lckcnt == 0) {
		faultcode_t fc;

		fc = segkvn_lock(mp->md_symspace_mapcookie, SEGKVN_FUTURE_LOCK);
		if (fc) {
			/*
			 *+ Failed to lock in symbol table for the
			 *+ module. No symbolic information of the
			 *+ module will be available.
			 */
			cmn_err(CE_NOTE,
		"MOD: Failed to lock in symbol table. The error code is %d.\n",
				FC_ERRNO(fc));
			error = FC_ERRNO(fc);
			goto lookupone_exit;
		}

		CATCH_FAULTS(CATCH_SEGKVN_FAULT) {
			error = mod_obj_lookupsym(mp, hval, name, retsp);
		}
		perror = END_CATCH();
		if (perror)
			error = perror;
		segkvn_unlock(mp->md_symspace_mapcookie, SEGKVN_FUTURE_LOCK);
	} else {
		error = mod_obj_lookupsym(mp, hval, name, retsp);
	}
	
lookupone_exit:
	if (!(flags & NOSLEEP))
		RWSLEEP_UNLOCK(&symtab_lock);

	return (error);
}

/*
 * STATIC int mod_obj_lookupsym(const struct modobj *mp, ulong_t hval, 
 *			const char *name, Elf32_Sym *retsp)
 *
 *	Look for a symbol by name through the hash table.
 *
 * Calling/Exit State:
 *	Called from mod_obj_lookupone.
 *	Return 0 when the symbol is found; otherwise return ENOMATCH.
 */
STATIC int
mod_obj_lookupsym(const struct modobj *mp, ulong_t hval, const char *name, 
		Elf32_Sym *retsp)
{
	ulong_t *ip;
	char *name1;
	Elf32_Sym *sp = NULL;

	for (ip = &mp->md_buckets[hval % MOD_OBJHASH]; *ip;
	   		ip = &mp->md_chains[*ip]) {

		sp = (Elf32_Sym *)(mp->md_symspace +
			/* LINTED pointer alignment */
			mp->md_symentsize * (*ip));
		name1 = mp->md_strings + sp->st_name;
		if (strcmp(name, name1) == 0) {
			*retsp = *sp;
			return (0);
		}
	}
	return (ENOMATCH);
}

/*
 * STATIC char *mod_obj_searchsym(const struct modobj *mp, 
 *	ulong_t value, ulong_t *offset)
 *
 *	Look for a symbol with the nearest value less than or equal to
 *	the value specified by value. The search is limited within the
 *	loadable module given by mp.
 *
 * Calling/Exit State:
 *	This routine is called with the symbol table of the module
 *	locked and the module also being held (or they may be effectively
 *	locked and held by kdb).
 *
 *	If the value is not within the address range of the module,
 *	then NULL is returned; otherwise the name of the symbol is
 *	returned and the offset to the specified value is copied to the
 *	space pointed to by offset.
 */
STATIC char *
mod_obj_searchsym(const struct modobj *mp, ulong_t value, 
	ulong_t *offset)
{
	Elf32_Sym *symend;
	Elf32_Sym *sym;
	Elf32_Sym *cursym;
	unsigned int curval;

	ASSERT(mp != NULL);

	if (!mod_obj_addrcheck(mp, value))
		return (NULL);				/* not in this module */

	*offset = (ulong_t) -1;			/* assume not found */
	cursym  = NULL;

	/* LINTED pointer alignment */
	symend = (Elf32_Sym *)mp->md_strings;

	/*
	 * Scan the module's symbol table for a global symbol <= value
	 */
	/* LINTED pointer alignment */
	for (sym = (Elf32_Sym *)(mp->md_symspace); sym < symend;
		/* LINTED pointer alignment */
		sym = (Elf32_Sym *)((char *)sym + mp->md_symentsize)) {

		curval = sym->st_value;

		if (curval > value)
			continue;

		if (value - curval < *offset) {
			*offset = value - curval;
			cursym = sym;
		}
	}

	if (cursym != NULL)
		curval = cursym->st_name;
	else
		return (NULL);

	return (mp->md_strings + curval);
}

/*
 * STATIC boolean_t mod_obj_addrcheck(const struct modobj *mp, 
 *		ulong_t addr)
 *
 *	See if the given addr is in the text, data, or bss of the module
 *	given by mp. Called primarily from mod_obj_searchsym and 
 *	mod_obj_validaddr.
 *
 * Calling/Exit State: 
 *	addr conatins the address in question and mp contains a 
 *	pointer to the module to be checked. The routine returns B_TRUE
 *	if the addr is within the module and B_FALSE otherwise.
 */
STATIC boolean_t
mod_obj_addrcheck(const struct modobj *mp, ulong_t addr)
{
	if (addr >= (ulong_t) mp->md_space && 
	     addr < (ulong_t) (mp->md_space + mp->md_space_size))
		return (B_TRUE);
	if (addr >= (ulong_t) mp->md_bss 
	     && addr < (ulong_t) (mp->md_bss + mp->md_bss_size))
		return (B_TRUE);
	return (B_FALSE);
}

/*
 * ulong_t mod_obj_hashname(const char *p)
 *
 *	Hash function for symbol names.  This must be the same as 
 *	elf_hash(3E) since that is what unixsyms uses to set up 
 *	the static kernel symbol table.
 *
 * Calling/Exit State: 
 *	p contains a pointer to a '\0' terminated string and
 *	the routine returns a hash value.
 */
ulong_t
mod_obj_hashname(const char *p)
{
	ulong_t g;
	ulong_t hval;

	hval = 0;
	while (*p) {
		hval = (hval << 4) + *p++;
		if ((g = (hval & 0xf0000000)) != 0)
			hval ^= g >> 24;
		hval &= ~g;
	}
	return (hval);
}

/*
 * STATIC int mod_obj_getsymp(const char *name, boolean_t kernelonly, 
 *		int flags, Elf32_Sym *retsp)
 *
 *	Look for the symbol given by name in the static kernel and 
 *	all loaded modules (unless kernelonly is B_TRUE). Called by 
 *	getksym and mod_obj_getsymvalue.
 *
 * Calling/Exit State: 
 *	name contains a '\0' terminated string of the symbol in 
 *	question. kernelonly is B_TRUE if only the static kernel 
 *	symbol table should be searched for the symbol, B_FALSE 
 *	otherwise. 
 *
 *	The routine returns 0 and a copy of the symbol table entry
 *	in the space pointed to by retsp if it is successful in
 *	finding the symbol in the table(s), ENOMATCH otherwise.
 *
 *	The NOSLEEP flag only set when kdb calls this routine indirectly.
 *	So, no locks can be acquired, and no need for holding any module
 *	while the flag is NOSLEEP.
 */
STATIC int
mod_obj_getsymp(const char *name, boolean_t kernelonly, 
	int flags, Elf32_Sym *retsp)
{
	struct modctl *mcp;
	
	mcp = mod_obj_getsym_mcp(name, kernelonly, flags, retsp);

	if (mcp) {
		if (!IS_KERN_MOD(mcp)) {
			if (!(flags & NOSLEEP))
				MOD_RELE(mcp->mc_modp);
		}
		return (0);
	}
	return (ENOMATCH);
}

/*
 * struct modctl *mod_obj_getsym_mcp(const char *name, boolean_t kernelonly,
 *      int flags, Elf32_Sym *retsp)
 *
 *	Look for the symbol given by name in the static kernel and 
 *	all loaded modules (unless kernelonly is B_TRUE).
 *
 * Calling/Exit State:
 *	Called by mod_obj_getsymp and the kmem driver.
 *	The routine returns the pointer to the modctl struct of the module
 *	that contains the symbol and a copy of the symbol table entry
 *	in the space pointed to by retsp if it is successful in
 *	finding the symbol in the table(s), NULL otherwise.
 */
struct modctl *
mod_obj_getsym_mcp(const char *name, boolean_t kernelonly,
        int flags, Elf32_Sym *retsp)
{
	struct modctl *mcp;
	struct module *modp;

	if (modhead.mc_modp == NULL)
		return (NULL);

	/*
	 * Search in the static kernel first. No lock is needed
	 * since the static kernel should never be unloaded.
	 */
	if (mod_obj_lookupone(&modhead.mc_modp->mod_obj, name, flags, retsp)
			== 0)
		return (&modhead);
	if (kernelonly)
		return (NULL);

	if (!(flags & NOSLEEP)) {
		ASSERT(getpl() == PLBASE);
		(void)RW_RDLOCK(&modlist_lock, PLDLM);
	}
		
	/*  search in the rest of the loaded modules */
	for (mcp = modhead.mc_next; mcp != NULL; mcp = mcp->mc_next) {
		if (mcp->mc_modp == NULL)
			continue;
		modp = mcp->mc_modp;

		if (!(flags & NOSLEEP)) {
			(void)LOCK(&modp->mod_lock, PLDLM);
			if (!(modp->mod_flags & MOD_SYMTABOK)) {
				UNLOCK(&modp->mod_lock, PLDLM);
				continue;
			}
			RW_UNLOCK(&modlist_lock, PLDLM);

			/*
			 * must put a hold on the module in case 
			 * the module is unloaded while sleeping
			 * on a page fault resolution.
			 */
			MOD_HOLD_L(modp, PLBASE);
		} else {
			if (!(modp->mod_flags & MOD_SYMTABOK))
				continue;
		}
		
		if (mod_obj_lookupone(&modp->mod_obj, name, flags, retsp)
		    == 0)
			return (mcp);

		if (!(flags & NOSLEEP)) {
			MOD_RELE(modp);
			(void)RW_RDLOCK(&modlist_lock, PLDLM);
		}

	}
	if (!(flags & NOSLEEP))
		RW_UNLOCK(&modlist_lock, PLBASE);

	return (NULL);
}


/*
 * char *mod_obj_getsymname(ulong_t value, ulong_t *offset, int flags, 
 *			char *retspace)
 *
 *	Look for a symbol with the nearest value less than or equal to
 *	the value specified by value within the static kernel and all
 *	the loadable modules.
 *
 * Calling/Exit State:
 *	If symbol tables are pageable and the flags is NOSLEEP,
 *	or the value is not invalid, then NULL is returned;
 *	otherwise the name of the symbol is returned and the offset to
 *	the specified value is copied to the space pointed to by offset.
 *
 *	If retspace is not NULL, the name of the symbol is copied
 *	to the space. Since kdb cannot allocate the space, the routine
 *	just return a pointer into the string table. In the normal
 *	case, the retspace should not be NULL, since the string table
 *	may be paged out.
 *
 *	Only kdb call this routine with NOSLEEP flag set.
 *	So, no locks can be acquired, and no need for holding any module
 *	while the flag is NOSLEEP.
 */
char *
mod_obj_getsymname(ulong_t value, ulong_t *offset, int flags, char *retspace)
{
	char *name;
	struct modctl *mcp;
	struct module *modp;
	int error;

	ASSERT((flags & NOSLEEP) || retspace != NULL);

	if (modhead.mc_modp == NULL || (mod_symtab_lckcnt == 0 &&
					flags & NOSLEEP))
		return (NULL);

	if (!(flags & NOSLEEP)) {
		ASSERT(KS_HOLD0LOCKS());
		ASSERT(getpl() == PLBASE);

		(void)RWSLEEP_RDLOCK(&symtab_lock, PRIMED);
		(void)RW_RDLOCK(&modlist_lock, PLDLM);
	}

	error = 0;
	for (mcp = &modhead; mcp != NULL; mcp = mcp->mc_next) {
		if (mcp->mc_modp == NULL)
			continue;
		modp = mcp->mc_modp;

		if (!IS_KERN_MOD(mcp)) {
			if (!(flags & NOSLEEP)) {
				(void)LOCK(&modp->mod_lock, PLDLM);
				if (!(modp->mod_flags & MOD_SYMTABOK)) {
					UNLOCK(&modp->mod_lock, PLDLM);
					continue;
				}
				RW_UNLOCK(&modlist_lock, PLDLM);
				MOD_HOLD_L(modp, PLBASE);
			} else {
				if (!(modp->mod_flags & MOD_SYMTABOK))
					continue;
			}

		} else {
			if (!(flags & NOSLEEP))
				RW_UNLOCK(&modlist_lock, PLBASE);
		}
		
		if (mod_symtab_lckcnt == 0) {
			faultcode_t fc;

			fc = segkvn_lock(modp->mod_obj.md_symspace_mapcookie,
				    SEGKVN_FUTURE_LOCK);
			if (fc) {
				/*
				 *+ Failed to lock in symbol table for the
				 *+ module. No symbolic information of the
				 *+ module will be available.
				 */
				cmn_err(CE_NOTE,
		"MOD: Failed to lock in symbol table. The error code is %d.\n",
					FC_ERRNO(fc));
				name = NULL;
				break;
			}

			CATCH_FAULTS(CATCH_SEGKVN_FAULT) {
				if ((name = mod_obj_searchsym(&modp->mod_obj, value, 
							offset)) != NULL) {

					if (retspace != NULL) {
						strncpy(retspace, name, MAXSYMNMLEN);
						name = retspace;
					}
				}
			}
			error = END_CATCH();

			segkvn_unlock(modp->mod_obj.md_symspace_mapcookie,
				      SEGKVN_FUTURE_LOCK);
		} else {

			if ((name = mod_obj_searchsym(&modp->mod_obj, value, 
						offset)) != NULL) {

				if (retspace != NULL) {
					strncpy(retspace, name, MAXSYMNMLEN);
					name = retspace;
				}
			}
		}

		if (!(flags & NOSLEEP)) {
			if (!IS_KERN_MOD(mcp))
				MOD_RELE(modp);
			(void)RW_RDLOCK(&modlist_lock, PLDLM);
		}

		if (error) {
			name = NULL;
			break;
		}

		/* if name is not NULL, the symbol is found */
		if (name)
			break;
	}

	if (!(flags & NOSLEEP)) {
		RW_UNLOCK(&modlist_lock, PLBASE);
		RWSLEEP_UNLOCK(&symtab_lock);
	}
	return (name);
}


/*
 * ulong_t mod_obj_getsymvalue (const char *name, 
 *	boolean_t kernelonly, int flags)
 *
 *	Get the value of a symbol using the given search parameters.
 *
 * Calling/Exit State:
 *	Called from kdb and mod_install_stubs().
 *	name contains a '\0' terminated string of the symbol in
 *	question. kernelonly is B_TRUE if only the static kernel
 *	symbol table should be searched for the symbol, B_FALSE
 *	otherwise.
 *
 *	Only kdb calls this routine with NOSLEEP flag set.
 *	The routine returns the value of the symbol if found and 0 otherwise.
 *
 * Description:
 *	Calls mod_obj_getsymp to do the work and extracts the value
 *	from the returned symbol table entry.
 */
ulong_t
mod_obj_getsymvalue (const char *name, boolean_t kernelonly, int flags)
{
	Elf32_Sym sp;

	if (mod_obj_getsymp(name, kernelonly, flags, &sp) != 0)
		return (0L);

	return (sp.st_value);
}


/*
 * boolean_t mod_obj_validaddr(ulong_t value)
 *	Check if given value is a valid address for the kernel or 
 *	currently loaded modules.
 *
 * Calling/Exit State: 
 *	Called only from the kdb so no locks acquired.
 *	Returns B_TRUE if the given value is a valid address and
 *	B_FALSE otherwise.
 */
boolean_t
mod_obj_validaddr(ulong_t value)
{
	struct modctl *mcp;
	struct module *modp;
	extern char _end[];

	/* Check kernel and all the currently loaded modules */
	for (mcp = &modhead; mcp != NULL; mcp = mcp->mc_next) {
		if (mcp->mc_modp == NULL)
			continue;
		modp = mcp->mc_modp;

		if (!IS_KERN_MOD(mcp)) {
			/* Don't check modules on their way in or out */
			if (!(modp->mod_flags & MOD_SYMTABOK))
				continue;
		} else {
			/*
			 * If there is no kernel symbol table, use the globals 
			 * indicating where text is located
			 */
			if (modhead.mc_modp == NULL) {
				return (value >= (ulong_t)KVBASE && 
					value < (ulong_t)_end);
			}
		}


		if (mod_obj_addrcheck(&modp->mod_obj, value))
			return (B_TRUE);
	}
	return (B_FALSE);
}


/*
 * void mod_obj_symlock(enum symtab_lock_flag flag)
 *	Lock or unlock all the pageable symbol tables in memory
 *	depends on the flag.
 *
 * Calling/Exit State:
 *	If the loadable module mechanism is not initialized
 *	correctly, the routine just increase the mod_symtab_lckcnt
 *	and return.
 */
void
mod_obj_symlock(enum symtab_lock_flag flag)
{
	struct modctl *mcp;
	struct module *modp;
	void *mapcookie;
	boolean_t chg_lck = B_FALSE;

	if (mod_obj_pagesymtab != MOD_PAGE)
		return;

	if (flag != SYMUNLOCK) {
		if (!mod_initialized) {
			mod_symtab_lckcnt++;
			return;
		}
	} else {
		ASSERT(mod_initialized);
	}

	(void)RWSLEEP_WRLOCK(&symtab_lock, PRIMED);
	if (flag != SYMUNLOCK) {
		if (mod_symtab_lckcnt++ == 0)
			chg_lck = B_TRUE;		
	} else {
		ASSERT(mod_symtab_lckcnt != 0);
		if (--mod_symtab_lckcnt == 0)
			chg_lck = B_TRUE;		
	}
	if (chg_lck) {
		(void)RW_RDLOCK(&modlist_lock, PLDLM);
		for (mcp = &modhead; mcp != NULL; mcp = mcp->mc_next) {
			if (mcp->mc_modp == NULL)
				continue;
			modp = mcp->mc_modp;

			if (!IS_KERN_MOD(mcp)) {
				(void)LOCK(&modp->mod_lock, PLDLM);
				if (modp->mod_flags & MOD_SYMLOCKED) {
					UNLOCK(&modp->mod_lock, PLDLM);
					continue;
				}
				RW_UNLOCK(&modlist_lock, PLDLM);
				MOD_HOLD_L(modp, PLBASE);
			} else {
				RW_UNLOCK(&modlist_lock, PLBASE);
			}
			mapcookie = modp->mod_obj.md_symspace_mapcookie;

			if (flag != SYMUNLOCK)
				segkvn_lock(mapcookie, SEGKVN_MEM_LOCK);
			else
				segkvn_unlock(mapcookie, SEGKVN_MEM_LOCK);

			if (!IS_KERN_MOD(mcp))
				MOD_RELE(modp);
			(void)RW_RDLOCK(&modlist_lock, PLDLM);
		}
		RW_UNLOCK(&modlist_lock, PLBASE);
	}
	RWSLEEP_UNLOCK(&symtab_lock);
}

/*
 * void mod_obj_modrele(struct modctl *mcp, int flags)
 *	Decrease the reference count of the module specified.
 *	
 * Calling/Exit State:
 *	Called from kmem driver.
 *	No spin lock held when calling this routine.
 */
void
mod_obj_modrele(struct modctl *mcp)
{
	struct module *modp = mcp->mc_modp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (!IS_KERN_MOD(mcp))
		MOD_RELE(modp);
}
