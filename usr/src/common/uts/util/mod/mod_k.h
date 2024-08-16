/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MOD_K_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_K_H	/* subject to change without notice */

#ident	"@(#)kern:util/mod/mod_k.h	1.23"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* 
   This file is available only for use by crash(1M).  The contents
   are subject to change without notice.
*/
#ifdef _KERNEL_HEADERS

#include <util/ksynch.h>	/* REQUIRED */
#include <util/mod/mod_obj.h>	/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/mod_obj.h>	/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#include <sys/cred.h>		/* SVR4.2COMPAT */
#include <sys/mod.h>		/* SVR4.2COMPAT */
#include <sys/moddefs.h>	/* SVR4.2COMPAT */

#endif	/* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

struct cred;

struct modctl {
	struct modctl *mc_next;		/* loaded module list pointers */
	uint_t mc_id;			/* module id */
	struct module *mc_modp;		/* pointer to module info */
	char mc_name[1];		/* module name, variable length */
};

struct module {
	struct lwp *mod_loading_context;	/* pointer to loading process */
	sv_t mod_sv;			/* synchronization variable */
	lock_t mod_lock;		/* module lock */
	ushort_t mod_flags;		/* flags */
	int mod_depcnt;			/* number of loaded dependents */
	int mod_holdcnt;		/* count to hold module in phy mem */
	int mod_keepcnt;		/* count to keep module in kernel */
	clock_t mod_unload_time;	/* the lbolt value when the last
					 * reference is completed */
	clock_t mod_delay_tick;		/* minimum number of ticks before
					 * module can be auto-unloaded */
	struct modobj mod_obj;		/* pointer to object file info */
	caddr_t mod_stub_info;		/* cache the pointer to stub info */
};

/* list of pointers to modctl structures used to maintain dependent list */
struct modctl_list {
	struct modctl *mcl_mcp;
	struct modctl_list *mcl_next;
};

/* bit masks for mod_flags */
#define	MOD_PRF		0x1
#define	MOD_LOADING	0x2
#define	MOD_UNLOADING	0x4
#define	MOD_DEMAND	0x8
#define	MOD_SYMTABOK	0x10
#define	MOD_LOCKING	0x20
#define MOD_SYMLOCKED	0x40
#define MOD_TRANS	(MOD_LOADING | MOD_UNLOADING)
#define MOD_PAGEABLE	0x80

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#ifdef _KERNEL

#define MODBUSY(modp)	((modp)->mod_holdcnt || \
		((modp)->mod_flags & MOD_PRF) || (modp)->mod_keepcnt)

#ifndef NO_RDMA
#define MOD_SEGKVN_DRV_FLAGS	(PHYSMEM_DRV_DMA() ? \
					SEGKVN_DMA : SEGKVN_NOFLAGS)
#else	/* NO_RDMA */
#define MOD_SEGKVN_DRV_FLAGS	SEGKVN_NOFLAGS
#endif /* NO_RDMA */

#define PAGEABLE(modp)	((modp)->mod_flags | MOD_PAGEABLE)

#define LOCKMOD(mp)	{ \
	segkvn_lock((mp)->md_space_mapcookie, SEGKVN_MEM_LOCK); \
	if ((mp)->md_bss) \
		segkvn_lock((mp)->md_bss_mapcookie, SEGKVN_MEM_LOCK); \
}
#define UNLOCKMOD(mp)	{ \
	if ((mp)->md_space) \
		segkvn_unlock((mp)->md_space_mapcookie, SEGKVN_MEM_LOCK); \
	if ((mp)->md_bss) \
		segkvn_unlock((mp)->md_bss_mapcookie, SEGKVN_MEM_LOCK); \
}

/*
 * When this macro evaluates to B_FALSE, the module lock for modp is held.
 * The caller must then release any type specific locks, and call MOD_HOLD_L()
 * to increment the module's hold count and release the module lock.
 *
 * When it evaluates to B_TRUE, it means the module is in the process of being
 * unloaded. The caller may call modld(), to wait for the unload to complete
 * and reload the module.
 */
#define MOD_IS_UNLOADING(modp) 						     \
	( (void)LOCK(&(modp)->mod_lock, PLDLM), 			     \
	((modp)->mod_flags & MOD_UNLOADING) ? 				     \
	UNLOCK(&(modp)->mod_lock, PLBASE), B_TRUE : B_FALSE )

#define	MOD_HOLD(modp)	{ \
	pl_t mod_hold_pl; \
	mod_hold_pl = LOCK(&(modp)->mod_lock, PLDLM); \
	MOD_HOLD_L(modp, mod_hold_pl); \
}

extern void mod_hold_locked(struct module *, pl_t);
#define	MOD_HOLD_L(modp, pl)	mod_hold_locked(modp, pl)

#define	MOD_RELE(modp)	{ \
	pl_t mod_rele_pl; \
	mod_rele_pl = LOCK(&(modp)->mod_lock, PLDLM); \
	MOD_RELE_L(modp, mod_rele_pl); \
}

#define MOD_RELE_L(modp, pl) { \
	if ((--(modp)->mod_holdcnt) == 0) { \
		(modp)->mod_unload_time = lbolt; \
		UNLOCK(&(modp)->mod_lock, (pl)); \
		UNLOCKMOD((&(modp)->mod_obj)); \
	} else \
		UNLOCK(&(modp)->mod_lock, (pl)); \
}

/* bit masks for modm_flags */
#define MOD_USTUB	0x1

/* data structures for stubs mechanism */
struct mod_stub_info {
	caddr_t mods_func_adr;
	struct mod_stub_modinfo *mods_modinfo;
	caddr_t mods_stub_adr;
	caddr_t mods_func_save;
	int (*mods_errfcn)();
};

struct mod_stub_modinfo {
	ulong_t modm_flags;
	char *modm_module_name;
	struct module	*modm_modp;
	rwlock_t *modm_lock;
	struct mod_stub_info modm_stubs[1];	/* variable length */
};


#define MOD_DEFPATH	"/etc/conf/mod.d"

extern int modld(const char *, struct cred *, struct modctl **, uint_t flags);
extern int unload_modules(size_t);
extern int mod_static(const char *);
extern void mod_rele_byname(const char *);

/* tunable and definitions */
#define MOD_NOPAGE	0
#define MOD_PAGE	1
#define MOD_FPAGE	2
extern int mod_obj_pagesymtab;

extern struct modctl modhead;
extern rwlock_t modlist_lock;

/* macros to vector to the appropriate module specific routine */
#define	MODL_INSTALL(MODL, MODP) \
	(*(MODL)->ml_ops->modm_install)((MODL)->ml_type_data, MODP)
#define	MODL_REMOVE(MODL) \
	(*(MODL)->ml_ops->modm_remove)((MODL)->ml_type_data)
#define	MODL_INFO(MODL, P0, P1, TYPE)	\
	(*(MODL)->ml_ops->modm_info)((MODL)->ml_type_data, P0, P1, TYPE)
#define	MODL_BIND(MODL, cpup) \
	(*(MODL)->ml_ops->modm_bind)((MODL)->ml_type_data, cpup);

/* For internal debug purpose only. */
#ifdef MODDEBUG
#define moddebug(x)	x
#else
#define moddebug(x)	/* undef */
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MOD_K_H */
