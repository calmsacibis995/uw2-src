/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/mod_obj.c	1.29"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg_kmem.h>
#include <mem/seg_kvn.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/obj/elf.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_obj.h>
#include <util/mod/mod_objmd.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

struct intfc_list {
	struct intfc_list *il_next;
	struct interface *il_intfc;
};

extern int mod_openpath(const char *modname, cred_t *credentials, 
	char **path, int *fd);
int mod_obj_open(const char *pathname, cred_t *credentials, 
	int *fd);
STATIC int mod_obj_read(int fd, void *ptr, size_t size, ulong_t offset, 
	cred_t *credentials);
STATIC void mod_obj_close(int fd, cred_t *credentials);
STATIC int mod_obj_getprog(struct modobj *mp, int fd, const Elf32_Ehdr *elfhdr, 
	const char *scnhdrs, char **dependents, char **interfaces,
	struct modwrapper ***wrapper, cred_t *credentials, uint_t flags);
STATIC int mod_obj_dodep(struct modobj *mp, const char *dependents, 
	cred_t *credentials);
STATIC int mod_obj_procsymtab(struct modobj *mp, int fd, 
	const Elf32_Ehdr *elfhdr, const char *scnhdrs, Elf32_Shdr **symhdr,
	struct intfc_list *ilist, int exempt, cred_t *credentials);
STATIC void mod_obj_syminsert(struct modobj *mp, const char *name, 
	ulong_t index);
STATIC int mod_obj_docommon(struct modobj *mp, const Elf32_Shdr *symhdr,
			    uint_t flags);
STATIC int mod_obj_doreloc(struct modobj *mp, int fd, const Elf32_Ehdr *elfhdr, 
        const char *scnhdrs, const Elf32_Shdr *symhdr,
	cred_t *credentials);
STATIC ulong_t mod_obj_lookupall(const struct modobj *mp, const char *name);
STATIC int mod_obj_check_interfaces(struct modobj *mp, char *istrp,
	struct intfc_list **ilpp, cred_t *cred);
STATIC int mod_obj_check_symbol(struct modobj *mp,
	struct intfc_list *ilp, char **symnamep);


/* round 'a' up to next multiple of 'b' */
#define ALIGN(a, b) ((b == 0) ? (a) : ((((a) +(b) -1) / (b)) * (b)))

/*
 * int mod_obj_load(const char *modulename, cred_t *credentials, 
 *		struct modobj *mp, uint_t flags)
 *	
 *	Get the module physically loaded into memory, handle all symbol
 *	table activity, and prepare the module to be executed.
 *
 * Calling/Exit State: 
 *	The modulename argument is either the full pathname to the 
 *	module to be loaded or a simple filename, which is translated 
 *	into a full pathname using the modpath PATH mechanism.  The 
 *	credentials are used in the normal file accessing done to load 
 *	the module. mop is the pointer to the struct modobj of the
 *	module to loaded. (The struct modobj records information about 
 *	the module, particularly the symbol table, which is necessary 
 *	for loading other modules which depend on this one as well as 
 *	those commands and kernel modules e.g., debugger and profiling 
 *	which require such knowledge.
 *
 *	The following flags are supported:
 *
 *		SEGKVN_DMA	Indicates that the driver is to be loaded
 *				into DMAable memory. Only used when
 *				RESTRICTED DMA support is in effect.
 *
 *	mod_obj_load returns 0 on success or an errno (in most cases
 *	ERELOC) otherwise.
 */
int
mod_obj_load(const char *modulename, cred_t *credentials, 
	struct modobj *mp, uint_t flags)
{
	struct mod_conf_data *mcdp;
	struct intfc_list *ilist, *inext;
	int fd = 0;		/* file descriptor from open of modulename */
	int mod_obj_errno;

	/*
	 * temporary information about module/object file used 
	 * only during mod_obj_load and callees
	 */
	Elf32_Ehdr elfhdr;
	Elf32_Shdr *symhdr = NULL;	/* symbol table section header */
	char *scnhdrs = NULL;		/* section headers - 
				  	   char * for arithmetic convenience */
	char *dependents = NULL;	/* list of dependents from SHT_MOD */
	char *interfaces = NULL;	/* list of interfaces */
	struct modwrapper **wrapper = NULL;	/* pointer to wrapper address */
	int exempt = 0;

	if ((mod_obj_errno = mod_openpath(modulename, 
					  credentials, &mp->md_path, &fd)) != 0)
		goto bad;

	if ((mod_obj_errno = mod_obj_read(fd, (char *)&elfhdr, 
				      sizeof(Elf32_Ehdr), 0, credentials)) != 0)
		goto bad;


	/* must be an ELF file */
	if (strncmp(ELFMAG, (char *)elfhdr.e_ident, SELFMAG) != 0) {
		/*
		 *+ The module is not an ELF file.
		 */
		cmn_err(CE_NOTE, "MOD: module %s is not an ELF file.\n",
			mp->md_path);
		mod_obj_errno = EINVAL;
		goto bad;
	}

	/* must be a relocatable object file for this machine type */
	if (elfhdr.e_machine != MOD_OBJ_MACHTYPE || elfhdr.e_type != ET_REL) {
		/*
		 *+ The module is not a relocatable object file, or is
		 *+ not a relocatable object file for this machine type.
		 */
		cmn_err(CE_NOTE, "MOD: Bad file type for module %s.\n",
			mp->md_path);
		mod_obj_errno = EINVAL;
		goto bad;
	}

	/* get the section headers from the file */
	scnhdrs = kmem_alloc(elfhdr.e_shentsize * elfhdr.e_shnum, KM_SLEEP);

	if ((mod_obj_errno = mod_obj_read(fd, scnhdrs, 
					  elfhdr.e_shentsize * elfhdr.e_shnum,
					  elfhdr.e_shoff, credentials)) != 0) {
		/*
		 *+ Cannot read section headers for the module.
		 */
		cmn_err(CE_NOTE,
			"MOD: Cannot read section headers for module %s.\n",
			mp->md_path);
		goto bad;
	}

	/*
	 * read in all the appropriate loadable sections and fill in
	 * dependents so that the info can be used in later routines
	 */

	if (mod_obj_getprog(mp, fd, &elfhdr, scnhdrs, &dependents, &interfaces,
			    &wrapper, credentials, flags) != 0) {
		/*
		 *+ Cannot get the prog sections in the module.
		 */
		cmn_err(CE_NOTE,
			"MOD: Cannot get the prog sections in module %s.\n", 
			mp->md_path);
		mod_obj_errno = ERELOC;
		goto bad;
	}
		
	if (wrapper == NULL) {
		/*
		 *+ Non-existent or invalid SHT_MOD section in the module.
		 *+ The module may not be configured as loadable.
		 */
		cmn_err(CE_NOTE,
		"MOD: Non-existent or invalid SHT_MOD section in module %s.\n",
			mp->md_path);
		mod_obj_errno = EINVAL;
		goto bad2;
	}

	/*
	 * Check interfaces required by the module against the set of
	 * supported interfaces.  Fail if they don't match.
	 */
	ilist = NULL;
	if ((mod_obj_errno = mod_obj_check_interfaces(mp, interfaces, &ilist,
						      credentials)) != 0)
		goto bad3;

	/*
	 * make sure all dependents loaded first - dependents list found in 
	 * section SHT_MOD in object file and passed through mp
	 */
	if (mod_obj_dodep(mp, dependents, credentials) != 0) {
		/*
		 *+ Dependency resolution in the module failed.
		 */
		cmn_err(CE_NOTE,
			"MOD: Dependency resolution in module %s failed.\n", 
			mp->md_path);
		mod_obj_errno = EINVAL;
		goto bad3;
	}

	/*
	 * Decide if module is exempt from interface enforcement.
	 */
	if (dependents) {
		while (*dependents == ' ' || *dependents == '\t')
			++dependents;
		if (*dependents != '\0')
			exempt = 1;
	}

	/*
	 * read in symbol table; fill in symhdr for later use;
	 * adjust values for section's  real address;
	 * set values for references to kernel and dependents;
	 * count bss_COMMON space needed
	 */
	if (mod_obj_procsymtab(mp, fd, &elfhdr, scnhdrs, &symhdr, ilist,
			       exempt, credentials) != 0) {
		/*
		 *+ Processing symbol table in the module failed.
		 */
		cmn_err(CE_WARN,
			"MOD: Processing symbol table in module %s failed\n",
			mp->md_path);
		mod_obj_errno = ERELOC;
		goto bad3;
	}

	/*
	 * allocate space for all COMMON symbols defined in this file 
	 * total space required was calculated in mod_obj_procsymtab
	 */
	if (mod_obj_docommon(mp, symhdr, flags) != 0) {
		/*
		 *+ Cannot allocate space for COMMON symbols in the module.
		 */
		cmn_err(CE_NOTE,
		"MOD: Cannot allocate space for COMMON symbols in module %s\n",			mp->md_path);
		mod_obj_errno = ENOMEM;
		goto bad3;
	}

	/* perform relocations specified in relocation tables */
	if (mod_obj_doreloc(mp, fd, &elfhdr, scnhdrs, symhdr,
			    credentials) != 0) {
		/*
		 *+ Relocation in the module failed.
		 */
		cmn_err(CE_NOTE,
			"MOD: Relocation in module %s failed\n.", 
			mp->md_path);
		mod_obj_errno = ERELOC;
		goto bad3;
	}

	/* save wrapper, allocate memory so it won't be paged out */
	mp->md_mw = (struct modwrapper *)kmem_alloc(sizeof(struct modwrapper),
						    KM_SLEEP);

	switch ((*wrapper)->mw_rev)
	{
	case 10:	/* compat with MODREV == 10 wrapper structure */
	{
		struct modwrapper_rev10 *conv =
				(struct modwrapper_rev10 *) *wrapper;

		mp->md_mw->mw_rev = MODREV;
		mp->md_mw->mw_load = conv->mw_load;
		mp->md_mw->mw_unload = conv->mw_unload;
		mp->md_mw->mw_verify = NULL;
		mp->md_mw->mw_halt = conv->mw_halt;
		mp->md_mw->mw_modlink = conv->mw_modlink;
		mcdp = kmem_alloc(sizeof *mcdp, KM_SLEEP);
		mcdp->mcd_version = MCD_VERSION;
		mcdp->mcd_unload_delay = *(time_t *)conv->mw_conf_data;
		mp->md_mw->mw_conf_data = mcdp;

		break;
	}

	case MODREV:
		*mp->md_mw = **wrapper;
		break;

	default:
		mod_obj_errno = EBADVER;
		goto bad4;
	}

	mcdp = mp->md_mw->mw_conf_data;

	switch (mcdp->mcd_version) {
	case MCD_VERSION:
		break;

	default:
		mod_obj_errno = EBADVER;
		goto bad4;
	}

	while (ilist != NULL) {
		inext = ilist->il_next;
		kmem_free(ilist, sizeof *ilist);
		ilist = inext;
	}

	/* done with object file */
	mod_obj_close(fd, credentials);

	/*
	 * copy the globals into the symspace - see mod_obj_procsymtab for 
	 * more details on this - and free up the temp symbol table
	 */
	bcopy((char *)(symhdr->sh_addr + symhdr->sh_info * symhdr->sh_entsize), 
	      mp->md_symspace +mp->md_symentsize, 
	      (size_t)(mp->md_strings - mp->md_symspace - mp->md_symentsize));
	kmem_free((char *)symhdr->sh_addr, symhdr->sh_size);
	kmem_free(scnhdrs, elfhdr.e_shentsize * elfhdr.e_shnum);

	return (0);

bad4:
	kmem_free(mp->md_mw, sizeof(struct modwrapper));
bad3:
	while (ilist != NULL) {
		inext = ilist->il_next;
		kmem_free(ilist, sizeof *ilist);
		ilist = inext;
	}
bad2:
#ifdef DLM_PAGING
	UNLOCKMOD(mp);
#endif
bad:
	if (fd != 0)
		mod_obj_close(fd, credentials);
	if (symhdr != NULL)
		kmem_free((char *)symhdr->sh_addr, symhdr->sh_size);
	if (scnhdrs)
		kmem_free(scnhdrs, elfhdr.e_shentsize * elfhdr.e_shnum);
	mod_obj_unload(mp, B_TRUE);
	return (mod_obj_errno);
}


/*
 * STATIC int mod_obj_getprog(struct modobj *mp, int fd, 
 *	const Elf32_Ehdr *elfhdr, const char *scnhdrs, char **dependents, 
 *	struct modwrapper ***wrapper, cred_t *credentials, uint_t flags)
 *
 *	Read in appropriate sections ((SHT_PROGBITS || SHT_NOBITS || SHT_MOD) 
 *	&& SHF_ALLOC) of module from object file.
 *
 * Calling/Exit State: 
 *	The structure pointed to by mp accumulates info about
 *	a module. This routine points md_space to where the sections are read.
 *	elfhdr, scnhdrs, and credentials are used to access the object file.
 *	wrapper is used to return a pointer to the wrapper structure found in
 *	the SHT_MOD section, and dependents is used to return the list of
 *	modules upon which this one depends - also found in the SHT_MOD section.
 *
 *	The following flags are supported:
 *
 *		SEGKVN_DMA	Indicates that the driver is to be loaded
 *				into DMAable memory. Only used when
 *				RESTRICTED DMA support is in effect.
 */
STATIC int
mod_obj_getprog(struct modobj *mp, int fd, const Elf32_Ehdr *elfhdr, 
		const char *scnhdrs, char **dependents, char **interfaces,
		struct modwrapper ***wrapper, cred_t *credentials, uint_t flags)
{
	ulong_t bits_ptr;
	Elf32_Shdr *shp, *shpend;
	char *strp;

	bits_ptr = 0;
	/* LINTED pointer alignment */
	shpend = (Elf32_Shdr *)(scnhdrs + elfhdr->e_shnum * elfhdr->e_shentsize);

	/*
	 * Loop through sections to find out how much space we need
	 * for text, data, (also bss that is already assigned).
	 * The section headers starting with 1 
	 * (since shdr 0 is always a dummy in ELF).
	 */
	/* LINTED pointer alignment */
	for (shp = (Elf32_Shdr *)(scnhdrs + elfhdr->e_shentsize); shp < shpend; 
	     /* LINTED pointer alignment */
	     shp = ((Elf32_Shdr *)((char *)shp + elfhdr->e_shentsize))) {

		if ((shp->sh_type != SHT_MOD && shp->sh_type != SHT_PROGBITS && 
		    shp->sh_type != SHT_NOBITS) || !(shp->sh_flags & SHF_ALLOC))
			continue;

		bits_ptr = ALIGN(bits_ptr, shp->sh_addralign);
		bits_ptr += shp->sh_size;
	}

	mp->md_space_size = ptob(btopr(bits_ptr));
	mp->md_space = (char *)segkvn_vp_mapin(0, mp->md_space_size, 0, NULL,
					       0, flags,
					       &mp->md_space_mapcookie);
	if (mp->md_space == NULL) {
		/*
		 *+ Failed to allocate memory for the module.
		 */
		cmn_err(CE_NOTE, "Failed to allocate memory for module %s.",
			mp->md_path); 
		return (-1);
	}

	segkvn_lock(mp->md_space_mapcookie, SEGKVN_MEM_LOCK);

	bits_ptr = (ulong_t)mp->md_space;

	/* now loop though sections assigning addresses and loading the data */
	/* LINTED pointer alignment */
	for (shp = (Elf32_Shdr *)(scnhdrs + elfhdr->e_shentsize); shp < shpend; 
	     /* LINTED pointer alignment */
	     shp = ((Elf32_Shdr *)((char *)shp + elfhdr->e_shentsize))) {

		switch (shp->sh_type) {

		/*
		 * only interested in loading SHT_MOD, 
		 * allocated program, and bss sections
		 */
		case SHT_PROGBITS:
		case SHT_NOBITS:
		case SHT_MOD:
		   	if (shp->sh_flags & SHF_ALLOC)
		     		break;
		default:
			continue;
		}

		bits_ptr = ALIGN(bits_ptr, shp->sh_addralign);
		if (shp->sh_type == SHT_PROGBITS || shp->sh_type == SHT_MOD) {
			if (mod_obj_read(fd, (char *)bits_ptr,
			    shp->sh_size, shp->sh_offset, credentials) != 0)
				return (-1);
			if (shp->sh_type == SHT_MOD) {
				*wrapper = (struct modwrapper **)bits_ptr;
				*dependents = NULL;
				*interfaces = NULL;
				if (shp->sh_size > sizeof(struct modwrapper *)) {
					*dependents = (char *)bits_ptr + 
					 	    sizeof(struct modwrapper *);
					strp = *dependents;
					while (strp - (char *)bits_ptr <
							    shp->sh_size - 1) {
						if (*strp++ == '\0') {
							*interfaces = strp;
							break;
						}
					}
				}
			}
		}
		shp->sh_type = SHT_PROGBITS;
		shp->sh_addr = bits_ptr;

		bits_ptr += shp->sh_size;
	}
	return (0);
}


/*
 * STATIC int mod_obj_dodep(struct modobj *mp, const char *dependents, 
 *		cred_t *credentials)
 *
 *	If this module depends on other modules as given by the module
 *	in its SHT_MOD section (and as collected by mod_obj_getprog), 
 *	make sure those modules are loaded and record the dependency 
 *	both by upping the dependency count of the module depended upon 
 *	and by keeping a pointer to the modctl structure for that module 
 *	so the dependency count can later be decreased when this module 
 *	is unloaded.
 *
 * Calling/Exit State: 
 *	The structure pointed to by mp accumulates data about the module 
 *	as it is being loaded. This routine sets md_mcl to a list
 *	of modctl pointers for modules upon which this module depends.
 *	The dependents argument has a space separated list of dependent
 *	module names and the credentials are needed to load modules as 
 *	necessary.  This routine returns 0 if all necessary dependencies 
 *	are satisfied and -1 otherwise. Assumes calling routine will clean 
 *	up partially created modctl list (md_mcl) and decrease the dependent 
 *	counts on the modules in the list appropriately.
 */
STATIC int
mod_obj_dodep(struct modobj *mp, const char *dependents, 
	      cred_t *credentials)
{
	const char *p;
	const char *limit;
	char *q;
	struct modctl *mcp;
	struct module *modp;
	struct modctl_list *mclp;

	char dependent[MODMAXNAMELEN];

	if (dependents == NULL)
		return (0);

	p = dependents;
	limit = p + strlen(dependents);

	for (;;) {
		/* skip whitespace */
		while (p < limit && (*p == ' ' || *p == '\t'))
			p++;
		if (p >= limit || *p == 0)
			break;
		q = dependent;
		while ((q - dependent < MODMAXNAMELEN) && (p < limit) &&
		       (*p) && (*p != ' ') && (*p != '\t')) {
			*q++ = *p++;
		}
		*q = 0;

		if (!mod_static(dependent)) {
			/* attempt to load the module */
			if (modld(dependent, credentials, &mcp, 0) != NULL) {
				/*
				 *+ Cannot load required dependent 
				 *+ for the module.
				 */
				cmn_err(CE_NOTE,
		"MOD: Cannot load required dependent %s for module %s.\n",
					dependent, mp->md_path);
				return (-1);
			}

			modp = mcp->mc_modp;

			/*
			 * dependent reference count and hold count
			 * of dependee increased so cannot be unloaded
			 * while this module is still loaded
			 */
			modp->mod_depcnt++;
			MOD_HOLD_L(modp, PLBASE);
	
			/*
			 * add to the list of modules that this module
			 * depends upon.
			 */
			mclp = (struct modctl_list *)
			       kmem_alloc(sizeof(struct modctl_list), KM_SLEEP);
			mclp->mcl_mcp = mcp;
			mclp->mcl_next = mp->md_mcl;
			mp->md_mcl = mclp;
		}

		/* skip to next whitespace */
		while ((p < limit) && (*p) && (*p != ' ') && (*p != '\t'))
			p++;
	}

	return (0);
}

/*
 * STATIC int mod_obj_procsymtab(struct modobj *mp, int fd, 
 *	const Elf32_Ehdr *elfhdr, const char *scnhdrs, Elf32_Shdr **symhdr,
 *	struct interface_list *ilist, int exempt, cred_t *credentials)
 *
 *	Sets up the symbol table for later use and resolves symbol 
 *	references, and calculates size of space needed for COMMON 
 *	symbols defined in this module.
 *
 * Calling/Exit State: 
 *	The structure pointed to by mp accumulates information
 *	about the module being loaded.  This routine adds a pointer 
 *	to the symbol table space (md_symspace), including space 
 *	for the string table (md_strings), the hash buckets 
 *	(md_buckets), and the hash chains (md_chains), the size 
 *	of that space (md_symsize), and the size of the space needed
 *	for COMMON symbols defined in this module. 
 *
 *	fd is the file descriptor for the file of the 
 *	module being loaded. elfhdr, scnhdrs, and credentials are all 
 *	used to access the file. symhdr is used to return a pointer 
 *	to the symbol table section header with the sh_addr field 
 *	pointing to the symbol table (including all locals). 
 *	md_symspace does not actually contain the symbol table
 *	until the end of mod_obj_load and then only contains the globals (see
 *	comments in this routine and mod_obj_load for more details).
 *
 *	Returns 0 if all OK, non-zero if error such as unresolved reference.
 */
STATIC int
mod_obj_procsymtab(struct modobj *mp, int fd, const Elf32_Ehdr *elfhdr, 
	const char *scnhdrs, Elf32_Shdr **symhdr, struct intfc_list *ilist,
	int exempt, cred_t *credentials)
{
	uint_t i;
	Elf32_Sym *sp, *spend;
	Elf32_Shdr *shp, * strhdr;
	Elf32_Shdr *shend;
	Elf32_Shdr *lsymhdr;	
	char *symname;
	int err = 0;
	uint_t bss_align = 0;
	uint_t bss_ptr = 0;
	uint_t ngsyms;
	ulong_t kval;
	int nonconform;

	/* find symhdr */
	/* LINTED pointer alignment */
	shend = (Elf32_Shdr *)(scnhdrs + elfhdr->e_shnum * elfhdr->e_shentsize);
	/* LINTED pointer alignment */
	for (lsymhdr = (Elf32_Shdr *)(scnhdrs + elfhdr->e_shentsize);
	     lsymhdr < shend;
	     /* LINTED pointer alignment */
	     lsymhdr = (Elf32_Shdr *)((char *)lsymhdr + elfhdr->e_shentsize)) {
		if (lsymhdr->sh_type == SHT_SYMTAB)
			break;
	}

	if (lsymhdr >= shend) {
		/*
		 *+ No symbol table in the module.
		 */
		cmn_err(CE_NOTE,
			"MOD: No symbol table in loadable module %s.\n",
			mp->md_path);
		return (-1);
	}

	/* get the associated string table header */
	if (lsymhdr->sh_link >= elfhdr->e_shnum) {
		/*
		 *+ String table link for symbol table in the module
		 *+ is invalid.
		 */
		cmn_err(CE_NOTE,
	"MOD: String table link for symbol table in module %s is invalid.\n",
			mp->md_path);
		return (-1);
	}

	/* LINTED pointer alignment */
	strhdr = (Elf32_Shdr *)(scnhdrs + lsymhdr->sh_link * elfhdr->e_shentsize);
	if (strhdr->sh_type != SHT_STRTAB) {
		/*
		 *+ Symbol table does not point to valid string 
		 *+ table in the module.
		 */
		cmn_err(CE_NOTE,
	"MOD: Symbol table points to invalid string table in module %s\n",
			mp->md_path);
		return (-1);
	}

	mp->md_symentsize = lsymhdr->sh_entsize;

	/*
	 * keep full symbol table here for relocation purposes;
	 * allocate only enough memory in symspace for the globals 
	 * (including the empty symbol entry);
	 * at the end of mod_obj_load, the globals will be bcopied to symspace;
	 * this allows all symspace to be allocated and freed as one ;
	 * use of lsymhdr->sh_addr should be safe since no symbols are defined
	 * in the symbol table section and no relocations done relative to 
	 * symhdr section
	 */
	lsymhdr->sh_addr = (Elf32_Addr)kmem_zalloc(lsymhdr->sh_size, KM_SLEEP);

	ngsyms = lsymhdr->sh_size / mp->md_symentsize - lsymhdr->sh_info + 1;

	/*
	 * space for all globals, the whole string header 
	 * (do not want to incur overhead of copying only 
	 * strings for globals), the buckets, the chains
	 * slots (again only for globals), and an extra 
	 * long for rounding slop
	 */
	mp->md_symsize = ptob(btopr(ngsyms * mp->md_symentsize +
	       strhdr->sh_size + (1 + MOD_OBJHASH + ngsyms) * sizeof(ulong_t)));

	mp->md_symspace = (char *)segkvn_vp_mapin(0, mp->md_symsize, 0, NULL,
						  0, SEGKVN_NOFLAGS,
						  &mp->md_symspace_mapcookie);
	if (mp->md_symspace == NULL) {
		/*
		 *+ Failed to allocate memory for the module.
		 */
		cmn_err(CE_NOTE,
			"MOD: Failed to allocate memory for module %s.",
			mp->md_path); 
		return (-1);
	}

	segkvn_lock(mp->md_symspace_mapcookie, SEGKVN_MEM_LOCK);

	mp->md_strings = mp->md_symspace + ngsyms * mp->md_symentsize;
	mp->md_buckets =
		(ulong_t *)((((ulong_t)mp->md_strings + strhdr->sh_size) | 
			     (sizeof(ulong_t)-1)) + 1);
	mp->md_chains = mp->md_buckets + MOD_OBJHASH; 

	if (mod_obj_read(fd, (char *)lsymhdr->sh_addr, lsymhdr->sh_size, 
			 lsymhdr->sh_offset, credentials) != 0 ||
	    mod_obj_read(fd, mp->md_strings, strhdr->sh_size, strhdr->sh_offset,
			 credentials) != 0 ) {
		/*
		 *+ Cannot read symbol table or string table in the module.
		 */
		cmn_err(CE_NOTE,
	       "MOD: Cannot read symbol table or string table in module %s.\n",
			mp->md_path);
		return (-1);
	}

	bzero((char *)mp->md_buckets, 
		(MOD_OBJHASH + ngsyms + 1) * sizeof(long));
	bzero(mp->md_symspace, mp->md_symentsize);

	/*
	 * Loop through the symbol table adjusting values to account
	 * for where each section got loaded into memory.  Also
	 * fill in the hash table.
	 * Skip the first entry in the symbol table.
	 */

	spend = (Elf32_Sym *)(lsymhdr->sh_addr + lsymhdr->sh_size);

	for (sp = (Elf32_Sym *)(lsymhdr->sh_addr + lsymhdr->sh_entsize), i = 1;
		sp < spend; 
		/* LINTED pointer alignment */
		sp = (Elf32_Sym *)((char *)sp + lsymhdr->sh_entsize), i++) {

		/* SHN_COMMON and SHN_ABS both above SHN_LORESERVE */
		if (sp->st_shndx < SHN_LORESERVE) {
			if (sp->st_shndx >= elfhdr->e_shnum) {
				/*
				 *+ Bad section index for the symbol.
				 */
				cmn_err(CE_NOTE, 
				    "MOD: Bad shndx for symbol index %d\n", i);
				err = -1;
				continue;
			}
			shp = (Elf32_Shdr *)(scnhdrs +
					    /* LINTED pointer alignment */
				 	    sp->st_shndx * elfhdr->e_shentsize);

			sp->st_value += shp->sh_addr;
		}


		/*
		 * net result of code below is that only globals 
		 * defined in this file wind up being hashed and 
		 * therefore found on subsequent lookup
		 */

		/* eliminate locals */
		if (ELF32_ST_BIND(sp->st_info) == STB_LOCAL)
			continue;

		if (sp->st_name == 0)
			continue;
		if (sp->st_name >= strhdr->sh_size) {
			/*
			 *+ String table index for a symbol in the
			 *+ module is out of range.
			 */
			cmn_err(CE_NOTE,
       "MOD: String table index for symbol %d in module %s is out of range.\n",
				i, mp->md_path);
			return (-1);
		}

		symname = mp->md_strings + sp->st_name;

		/*
		 * just need to hash those symbols that are already known
		 * to be defined in this file; hash with index relative 
		 * to globals since symspace will only have the globals 
		 * (assumes that NO calls to mod_obj_lookupone for this 
		 * module until after return from mod_obj_load when the 
		 * globals have been copied to symspace)
		 */
		if (sp->st_shndx != SHN_UNDEF && sp->st_shndx != SHN_COMMON) {
			mod_obj_syminsert(mp, symname, 
					  i - lsymhdr->sh_info + 1);
			continue;
		}


		/*
		 * check if the symbol needs to be remapped or rejected
		 */
		nonconform = (mod_obj_check_symbol(mp, ilist, &symname) != 0);

		/* look for symbol resolving reference */
		if ((kval = mod_obj_lookupall(mp, symname)) != 0) {
			if (nonconform && !exempt &&
			    ELF32_ST_BIND(sp->st_info) != STB_WEAK) {
				cmn_err(CE_CONT,
					"MOD: non-conforming symbol, %s\n",
					symname);
				err = -1;
				continue;
			}
			sp->st_shndx = SHN_ABS;	
			sp->st_value = kval;
			continue;
		}

		if (sp->st_shndx == SHN_UNDEF && ELF32_ST_BIND(sp->st_info) 
						 != STB_WEAK) {
			/*
			 *+ Found undefined symbol in the module.
			 */
			cmn_err(CE_CONT, 
				"Undefined symbol %s in loadable module %s.\n", 
				symname, mp->md_path);
			err = -1;
			continue;
		}

		/*
		 * it's a common symbol defined in this file - 
		 * st_value is the required alignment
		 */
		if (sp->st_value > bss_align)
			bss_align = sp->st_value;
		bss_ptr = ALIGN(bss_ptr, sp->st_value);
		bss_ptr += sp->st_size;
		mod_obj_syminsert(mp, symname, i - lsymhdr->sh_info + 1);
	}

	mp->md_bss_size = ptob(btopr(bss_ptr + bss_align));

	*symhdr = lsymhdr;
	return (err);
}

/*
 * STATIC int mod_obj_docommon(struct modobj *mp, const Elf32_Shdr *symhdr,
 *			       uint_t flags)
 *	
 *	Allocate space for COMMON symbols defined in this file and adjust symbol
 *	table pointers accordingly.
 *
 * Calling/Exit State: 
 *	The structure pointed to by mp is used to accumulate information 
 *	about the module being loaded.  This routine adds the space for 
 *	the COMMON symbols defined in this module (md_bss).
 *	Size of space needed calculated in mod_obj_procsymtab and "passed" in
 *	mp->md_bss_size.
 *	symhdr is used to update the symbol table entries.
 *
 *	The following flags are supported:
 *
 *		SEGKVN_DMA	Indicates that the driver is to be loaded
 *				into DMAable memory. Only used when
 *				RESTRICTED DMA support is in effect.
 *
 *	mod_obj_load returns 0 on success or an errno (in most cases
 *	ERELOC) otherwise.
 */
STATIC int
mod_obj_docommon(struct modobj *mp, const Elf32_Shdr *symhdr, uint_t flags)
{
	Elf32_Sym *sp, *symend;
	ulong_t bss_ptr;

	if (mp->md_bss_size) {
		mp->md_bss = (char *)segkvn_vp_mapin(0, mp->md_bss_size, 0,
						     NULL, 0, flags,
						     &mp->md_bss_mapcookie);
		if (mp->md_bss == NULL) {
			/*
			 *+ Failed to allocate memory for the module.
			 */
			cmn_err(CE_NOTE,
				"MOD: Failed to allocate memory for module %s.",
				mp->md_path); 
			return (-1);
		}

		segkvn_lock(mp->md_bss_mapcookie, SEGKVN_MEM_LOCK);

		bss_ptr = (ulong_t)mp->md_bss;
		symend = (Elf32_Sym *)(symhdr->sh_addr + symhdr->sh_size);

		/* loop through symbols starting with first STB_GLOBAL */
		for (sp = (Elf32_Sym *)(symhdr->sh_addr + symhdr->sh_info *
					symhdr->sh_entsize);
		     sp < symend; 
		     /* LINTED pointer alignment */
		     sp = (Elf32_Sym *)((char *)sp + symhdr->sh_entsize)) {

		     	if (sp->st_shndx != SHN_COMMON)
				continue;

			bss_ptr = ALIGN(bss_ptr, sp->st_value);
			sp->st_shndx = SHN_ABS;
			sp->st_value = bss_ptr;
			bss_ptr += sp->st_size;
		}
	}
	return (0);
}

/*
 * STATIC int mod_obj_doreloc(struct modobj *mp, int fd, 
 *		const Elf32_Ehdr *elfhdr, const char *scnhdrs, 
 *		const Elf32_Shdr *symhdr, cred_t *credentials)
 *
 *	Perform symbol relocation for this module.
 *
 * Calling/Exit State: 
 *	The structure pointed to by mp is used to accumulate 
 *	information about the module being loaded. This routine 
 *	does not add any new information but uses some information 
 *	accumulated in other routines.
 *	The reminaing arguments are used to access the relocation 
 *	and symbol table information for the module. This routine 
 *	returns 0 if all relocations were successful, non-zero otherwise.
 *
 * Remark:
 *	This routine calls the machine dependent mod_obj_relone.
 */
STATIC int
mod_obj_doreloc(struct modobj *mp, int fd, const Elf32_Ehdr *elfhdr, 
		const char *scnhdrs, const Elf32_Shdr *symhdr, 
		cred_t *credentials)
{

	Elf32_Shdr *shp, *shend;
	const Elf32_Shdr *rshp;
	uint_t nreloc;
	Elf32_Rel *reltbl = NULL;

	/* LINTED pointer alignment */
	shend = (Elf32_Shdr *)(scnhdrs + elfhdr->e_shnum * elfhdr->e_shentsize);

	/* find relocation headers, skip hdr 0 since that is always a dummy */
	/* LINTED pointer alignment */
	for (rshp = (Elf32_Shdr *)(scnhdrs + elfhdr->e_shentsize); rshp < shend;
	     /* LINTED pointer alignment */
	     rshp = (Elf32_Shdr *)((char *)rshp + elfhdr->e_shentsize)) {

		if (!MOD_OBJ_VALRELOC(rshp->sh_type)) {
			if (MOD_OBJ_ERRRELOC(rshp->sh_type)) {
				/*
				 *+ Can't process the reloc type.
				 */
				cmn_err(CE_NOTE, 
			    "MOD: Can't process reloc type %d in module %s.\n",
					rshp->sh_type, mp->md_path);
				goto bad;
			}
			continue;
		}

		if ((Elf32_Shdr *)(scnhdrs + rshp->sh_link *
					     /* LINTED pointer alignment */
					     elfhdr->e_shentsize) != symhdr) {
			/*
			 *+ Reloc for non-default symtab in the module.
			 */
			cmn_err(CE_NOTE, 
			   "MOD: Reloc for non-default symtab in module %s.\n",
				mp->md_path);
			goto bad;
		}

		if (rshp->sh_info >= elfhdr->e_shnum) {
			/*
			 *+ The section header index for this relocation
			 *+ table is out of range.
			 */
			cmn_err(CE_NOTE, 
				"MOD: Sh_info out of range %d in module %s.\n",
				 rshp->sh_info, mp->md_path);
			goto bad;
		}

		/* get the section header that this reloc table refers to */
		/* LINTED pointer alignment */
		shp = (Elf32_Shdr *)(scnhdrs + rshp->sh_info * elfhdr->e_shentsize);

		/* skip relocations for non allocated sections */
		if (!(shp->sh_flags & SHF_ALLOC))
			continue;

		nreloc = rshp->sh_size / rshp->sh_entsize;
		reltbl = (Elf32_Rel *)kmem_alloc(rshp->sh_size, KM_SLEEP);

		if (mod_obj_read(fd, reltbl, rshp->sh_size, rshp->sh_offset, 
				 credentials) != 0) {
			/*
			 *+ Failed to read the relocation table in the module.
			 */
			cmn_err(CE_NOTE,
			"MOD: Read of relocation table in module %s failed.\n",
				mp->md_path);
			return (-1);
		}

		if (mod_obj_relone(mp, reltbl, nreloc, rshp->sh_entsize, shp,
				   symhdr) != 0)
			goto bad;

		kmem_free(reltbl, rshp->sh_size);
		reltbl = NULL;
	}
	return (0);
bad:
	if (reltbl)
		kmem_free(reltbl, rshp->sh_size);
	return (-1);
}


/*
 * STATIC ulong_t
 * mod_obj_lookupall(const struct modobj *mp, const char *name)
 *	
 *	Look for symbol given by name in modules upon which the 
 *	module given by mp depends and in the static kernel module 
 *	returning the value of that symbol.
 *
 * Calling/Exit State:
 *	Returns 0 if symbol is not found.
 */
STATIC ulong_t
mod_obj_lookupall(const struct modobj *mp, const char *name)
{
	Elf32_Sym sp;
	struct modctl_list *mclp;

	for (mclp = mp->md_mcl; mclp; mclp = mclp->mcl_next) {
		if (mod_obj_lookupone(&mclp->mcl_mcp->mc_modp->mod_obj, name,
				      SLEEP, &sp) == 0) {
			return (sp.st_value);
		}
	}
	if (mod_obj_lookupone(&modhead.mc_modp->mod_obj, name, SLEEP, &sp) ==
									    0)
		return (sp.st_value);

	return (0);
}

/*
 * STATIC void mod_obj_syminsert(struct modobj *mp, const char *name,
 *		ulong_t index)
 *	
 *	Adds symbol given by name and symbol table index to hash 
 *	tables of module given by mp.
 *
 * Calling/Exit State: 
 *	This routine is called while the symbol table pages are
 *	locked in memory.
 */
STATIC void
mod_obj_syminsert(struct modobj *mp, const char *name, ulong_t index)
{
	ulong_t hval;

	hval = mod_obj_hashname(name) % MOD_OBJHASH;
	mp->md_chains[index] = mp->md_buckets[hval];
	mp->md_buckets[hval] = index;
}


/*
 * int mod_obj_open(const char *pathname, 
 *	cred_t *credentials, int *fd)
 *
 *	Tries to open the file given by pathname using vn_open.  If the 
 *	credentials argument is sys_cred, then the u_area's notion of root
 *	and the credentials are changed to be the system root and sys_cred
 *	so that vn_open and lookuppn will find the right file and be able 
 *	to open it.  They are restored to their proper values after the 
 *	return from vn_open.
 *	The credentials are only sys_cred when this is a call to 
 *	autoload a module. demand loading via the system call has the user's
 *	credentials in the credentials variable and therefore uses the user's
 *	idea of root and the credentials to find and open the file.
 *
 * Calling/Exit State:
 *	pathname contains the file to be opened.
 *	credentials may be used to open the file. On a successful open,
 *	this routine returns 0 and fd is used to return the resulting file
 *	descriptor (which is really a vnode_t *).  On failure the routine 
 *	returns non-zero.
 */
int
mod_obj_open(const char *pathname, cred_t *credentials, int *fd)
{
	vnode_t *saveroot;
	cred_t *savecred;
	int error;

	if (credentials == sys_cred) {
		/* autoload */

		/* find module from real root rather than chroot */
		saveroot = u.u_lwpp->l_rdir;
		u.u_lwpp->l_rdir = rootdir;

		/*
		 * allows module to be loaded by system even if user
		 * is not privileged
		 */
		savecred = u.u_lwpp->l_cred;
		u.u_lwpp->l_cred = sys_cred;
		error = vn_open((char *)pathname, UIO_SYSSPACE, FREAD, 0,
			(vnode_t **)fd, 0);
		u.u_lwpp->l_rdir = saveroot;
		u.u_lwpp->l_cred = savecred;
		return (error);
	} else {
		/* demand load */

		return (vn_open((char *)pathname, UIO_SYSSPACE, FREAD, 0,
			(vnode_t **)fd, 0));
	}
}


/*
 * STATIC void mod_obj_close(int fd, cred_t *credentials)
 *	
 *	close the open file descriptor (really a vnode_t * originally 
 *	obtained from mod_obj_open) using VOP_CLOSE. The vnode is also 
 *	released after the close.
 *
 * Calling/Exit State: 
 *	fd is a pointer to the vnode to be closed and
 *	the credentials are used to do so.
 */
STATIC void
mod_obj_close(int fd, cred_t *credentials)
{
	VOP_CLOSE((vnode_t *)fd, FREAD, 1, 0, (cred_t *)credentials);
	VN_RELE((vnode_t *)fd);
}

/*
 * STATIC int mod_obj_read(int fd, void *buf, size_t size, 
 *		ulong_t offset, cred_t *credentials)
 *
 *	read information from the file given by the file descriptor 
 *	using vn_rdwr
 *
 * Calling/Exit State: 
 *	fd contains a pointer to the vnode for the file to be read.
 *	size bytes are read starting at offset into buf using the credentials.
 *	On success the rouine returns 0, else non-zero.
 */
STATIC int
mod_obj_read(int fd, void *buf, size_t size, ulong_t offset, 
	cred_t *credentials)
{
	int resid;

	return (vn_rdwr(UIO_READ, (vnode_t *)fd, buf, size, offset, 
		UIO_SYSSPACE, 0, 0, (cred_t *)credentials, &resid));
}

/*
 * void mod_obj_unload(struct modobj *mp, boolean_t symtab_locked)
 *
 *	Frees up all space associated with module and "notifies" modules upon
 *	which this one depends that the dependency no longer exists
 *
 * Calling/Exit State: 
 *	mp is used to access all the space to be freed, and the 
 *	dependee modules.  symtab_locked is false once the symbol table
 *	has been fully loaded and (un)locked to match mod_symtab_lckcnt.
 */
void
mod_obj_unload(struct modobj *mp, boolean_t symtab_locked)
{
	struct modctl_list *mclp, *mclp2;
	struct module *modp;

	mclp = mp->md_mcl;

	/* if there are any modules on which this module depends ... */
	for (mclp = mp->md_mcl; mclp; mclp = mclp2) {
		/* let it know the dependency no longer exists */
		modp = mclp->mcl_mcp->mc_modp;

		LOCK(&modp->mod_lock, PLDLM);
		modp->mod_depcnt--;
		MOD_RELE_L(modp, PLBASE);
		mclp2 = mclp->mcl_next;
		kmem_free(mclp, sizeof(struct modctl_list));
	}


	/*
	 * DLM_PAGING: Unlock all the pages of the module here.
	 * This should be removed when all the paging issues resolved.
	 */
	UNLOCKMOD(mp);
	/*
	 * End of temporary fix for the DLM paging.
	 */


	if (mp->md_bss)
		segkvn_mapout(mp->md_bss_mapcookie);
	if (mp->md_space)
		segkvn_mapout(mp->md_space_mapcookie);
	if (mp->md_symspace) {
		int lckcnt;

		RWSLEEP_RDLOCK(&symtab_lock, PRIMED);
		if (symtab_locked)
			lckcnt = 1;
		else
			lckcnt = mod_symtab_lckcnt;
		while (lckcnt-- != 0)
			segkvn_unlock(mp->md_symspace_mapcookie,
				      SEGKVN_MEM_LOCK);
		RWSLEEP_UNLOCK(&symtab_lock);
		segkvn_mapout(mp->md_symspace_mapcookie);
	}
	if (mp->md_path)
		kmem_free(mp->md_path, strlen(mp->md_path) + 1);
	if (mp->md_mw)
		kmem_free(mp->md_mw, sizeof(struct modwrapper));
}

/*
 * ulong_t mod_obj_lookup(const struct modobj *mp, const char *name)
 *
 *	Find value associated with symbol given by 'name' in module 
 *	given by 'mp'. Called primarily when resolving stub references.
 *
 * Calling/Exit State: 
 *	Returns value of found symbol or 0 if no symbol is found.
 */
ulong_t
mod_obj_lookup(const struct modobj *mp, const char *name)
{
	Elf32_Sym sp;

	if (mod_obj_lookupone(mp, name, SLEEP, &sp) != 0)
		return (0);
	return (sp.st_value);
}


/*
 * STATIC int
 * mod_obj_check_interfaces(struct modobj *mp, char *istrp,
 *			    struct intfc_list **ilpp, cred_t *cred)
 *	Check if the interfaces used by a module are supported;
 *	load modules depended on by the interfaces.
 *
 * Calling/Exit State:
 *	Returns an errno or 0 on success.
 */
STATIC int
mod_obj_check_interfaces(struct modobj *mp, char *istrp,
			 struct intfc_list **ilpp, cred_t *cred)
{
	struct intfc_list *ilp;
	struct interface *intp, *intp2, *intp3;
	char *name;

	/*
	 * Build up interface lists from the string version in the module.
	 * Pick the version of interface to use for each $interface line.
	 */
	if (istrp == NULL) {
		/*
		 * Old (HBA) modules didn't have interface lists.
		 * Make a best guess: ddi.4 plus sdi.1.
		 */
		istrp = "ddi\0" "4\0\0sdi\0" "1\0\0\0";
	}

	for (; *istrp != '\0'; istrp += strlen(istrp) + 1) {
		name = istrp;
		ilp = kmem_alloc(sizeof *ilp, KM_SLEEP);
		ilp->il_next = *ilpp;
		*ilpp = ilp;

		/* find matching interface name */
		if (strcmp(name, "base") == 0 ||
		    strcmp(name, "nonconforming") == 0) {
			ilp->il_intfc = NULL;
			istrp += strlen(istrp) + 1;
			continue;
		}
		for (intp = interfaces; intp; intp = intp->if_next_intfc) {
			if (strcmp(name, intp->if_name) == 0)
				break;
		}
		if (intp == NULL) {
			/*
			 *+ A module declared a $interface line for an
			 *+ interface which is not supported by the system.
			 */
			cmn_err(CE_NOTE,
			  "MOD: Module %s requires unsupported interface, %s\n",
				mp->md_path, name);
			return ERELOC;
		}
		/* find the best matching version */
		intp2 = NULL;
		while (*(istrp += strlen(istrp) + 1) != '\0') {
			for (intp3 = intp; intp3; intp3 = intp3->if_next_ver) {
				if (strcmp(istrp, intp3->if_version) == 0) {
					if (intp2 == NULL ||
					    intp2->if_order < intp3->if_order)
						intp2 = intp3;
					break;
				}
			}
		}
		if (intp2 == NULL) {
			/*
			 *+ A module declared a $interface line for an
			 *+ interface which is not supported by the system.
			 */
			cmn_err(CE_NOTE,
			  "MOD: Module %s requires unsupported version(s)"
			  " of interface, %s\n",
				mp->md_path, name);
			return ERELOC;
		}
#ifdef DEBUG
		cmn_err(CE_CONT, "MOD: Choosing %s interface version %s\n",
				intp2->if_name, intp2->if_version);
#endif
		ilp->il_intfc = intp2;
		/*
		 * Get dependents from the selected interface and any it
		 * replaces, and load them.
		 */
		do {
			if (mod_obj_dodep(mp, intp2->if_depends, cred) != 0) {
				/*
				 *+ Dependency resolution in the module failed.
				 */
				cmn_err(CE_NOTE,
					"MOD: Dependency resolution in"
					" module %s failed.\n", 
					mp->md_path);
				return EINVAL;
			}
		} while ((intp2 = intp2->if_rep_intfc) != NULL);
	}

	return 0;
}


/*
 * STATIC int
 * mod_obj_check_symbol(struct modobj *mp, struct intfc_list *ilp,
 *			char **symnamep)
 *	Check if a symbol needs to be remapped.
 *
 * Calling/Exit State:
 *	Returns 0 if the symbol (*symnamep) conforms to the set of interfaces
 *	for the module, with (*symnamep) possibly changed to reflect a
 *	symbol mapping.
 */
STATIC int
mod_obj_check_symbol(struct modobj *mp, struct intfc_list *ilp, char **symnamep)
{
	struct interface *intp;
	struct intfc_sym *symp;
	boolean_t has_base = B_FALSE;
	char *symname = *symnamep;

	while (ilp != NULL) {
		intp = ilp->il_intfc;
		ilp = ilp->il_next;
		if (intp == NULL) {
			has_base = B_TRUE;
			continue;
		}
		symp = intp->if_symbols;
		for (;;) {
			if (symp->ifs_name == NULL) {
				if ((intp = intp->if_rep_intfc) == NULL)
					break;
				symp = intp->if_symbols;
				continue;
			}
			if (strcmp(symp->ifs_name, symname) == 0)
				break;
			++symp;
		}
		if (intp == NULL || symp->ifs_newname == SYM_DROPPED)
			continue;
		if (symp->ifs_newname)
			*symnamep = symp->ifs_newname;
		return 0;
	}

	/*
	 * Check through a special list of symbols which may be referenced
	 * by the module wrapper, rather than the module itself, and thus
	 * may not be in an interface file.  For now, just hard-code this.
	 */
	if (strcmp(*symnamep, "nodev") == 0 ||
	    strcmp(*symnamep, "nulldev") == 0 ||
	    strcmp(*symnamep, "mod_enosys") == 0 ||
	    strcmp(*symnamep, "intnull") == 0)
		return 0;

	return !has_base;
}
