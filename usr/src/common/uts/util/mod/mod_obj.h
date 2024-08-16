/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MOD_OBJ_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_OBJ_H	/* subject to change without notice */

#ident	"@(#)kern:util/mod/mod_obj.h	1.16"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <proc/obj/elf.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/cred.h>		/* SVR4.2COMPAT */
#include <sys/elf.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#if defined(_KERNEL) || defined(_KMEMUSER)

/* used only by unixsyms at user level as well as by the kernel */
/* basic record of internal information associated with a loaded module */

/* structure stubs so no complaints if other headers not included */
struct modctl_list;
struct modwrapper;

struct modobj  {

	char *md_symspace;		/* space for symbol table info incl.
					   symbol table, string table,
					   hash chains and buckets */
	unsigned int md_symsize;	/* size of symspace */
	void *md_symspace_mapcookie;	/* seg_kvn mapcookie for md_symspace */
	unsigned int md_symentsize;	/* size of a symbol table entry from 
					   symbol table section header */
	char *md_strings;		/* pointer into symspace for symbol 
					   table string table */
	unsigned long *md_buckets;	/* buckets for hash into symtbl */
	unsigned long *md_chains;	/* chains for hash into symtbl */
	char *md_space;			/* space where text, data, 
					   and pre-allocated bss is loaded */
	unsigned int md_space_size;
	void *md_space_mapcookie;	/* seg_kvn mapcookie for md_space */
	char *md_bss;			/* space where common symbols defined 
					   in module are loaded */
	unsigned int md_bss_size;
	void *md_bss_mapcookie;		/* seg_kvn mapcookie for md_bss */
	char *md_path;			/* full path of loaded module */
	struct modwrapper *md_mw; 	/* pointer to modwrapper */
	struct modctl_list *md_mcl; 	/* list of modules on which this 
					   module is dependent */
};


#define MOD_OBJHASH	101		/* size of hash table */

#ifdef _KERNEL

struct cred;

enum symtab_lock_flag { SYMLOCK, SYMUNLOCK };

extern int mod_obj_load(const char *, struct cred *, struct modobj *, uint_t);
extern void mod_obj_unload(struct modobj *, boolean_t);
extern ulong_t mod_obj_lookup(const struct modobj *, const char *);

extern rwsleep_t symtab_lock;
extern int mod_symtab_lckcnt;

extern boolean_t mod_obj_validaddr(ulong_t);
extern ulong_t mod_obj_getsymvalue(const char *, boolean_t, int);
extern char *mod_obj_getsymname(ulong_t, ulong_t *, int, char *);
extern boolean_t mod_obj_validaddr(ulong_t);
extern struct modctl *mod_obj_getsym_mcp(const char *, boolean_t, int,
	Elf32_Sym *);
extern int mod_obj_lookupone(const struct modobj *, const char *, int,
	Elf32_Sym *);
unsigned long mod_obj_hashname(const char *);
extern void mod_obj_modrele(struct modctl *);
void mod_obj_symlock(enum symtab_lock_flag);

#endif /* _KERNEL */

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MOD_OBJ_H */
