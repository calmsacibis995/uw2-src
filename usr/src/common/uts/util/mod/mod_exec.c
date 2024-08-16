/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/mod/mod_exec.c	1.9"
#ident	"$Header: $"

#include <mem/kmem.h>
#include <mem/seg_kvn.h>
#include <proc/exec.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/moddefs.h>
#include <util/mod/modexec.h>
#include <util/types.h>

/*
 * Dynamically loadable EXEC type support.
 */

extern	volatile clock_t	lbolt;
extern	rwlock_t	mod_execsw_lock;

STATIC	int	mod_execinstall();
STATIC	int	mod_execremove();
STATIC	int	mod_execinfo();
STATIC	int	mod_execbind();

STATIC	int	mod_exec_func();
STATIC	int	mod_exec_core();
STATIC	int	mod_exec_textinfo();

struct	execsw_info	mod_execsw_info	= {
	mod_exec_func,
	mod_exec_core,
	mod_exec_textinfo,
	NULL
};

/*
 * STATIC int mod_exec_func(vnode_t *vp, struct uarg *args,
 *			    int level, long *execsz, exhda_t *ehdp)
 *
 *	Autoload exec routine for loadable EXEC type modules.
 *
 * Calling/Exit State:
 *	No locks should be held on entry and no locks are held on return.
 *
 *	This routine returns the errno ENOLOAD upon failure to load the
 *	required module; otherwise, the module's actual exec routine is
 *	called and its return value passed back to the caller.
 */
STATIC	int
mod_exec_func(vnode_t *vp, struct uarg *args, int level, long *execsz, exhda_t *ehdp)
{
	struct	modctl	*modctlp;
	struct	execsw	*execp;
	int	err;
	char	*modname;

	ASSERT(getpl() == PLBASE);

	execp = args->execinfop->ei_execsw;
	modname = execp->exec_name;

	if ((err = modld(modname, sys_cred, &modctlp, 0)) != 0) {
		return (ENOLOAD);
	}

	MOD_HOLD_L(modctlp->mc_modp, PLBASE);

	if (execp->exec_info == &mod_execsw_info) {
		MOD_RELE(modctlp->mc_modp);
		return (ENOLOAD);
	}

	err = (*execp->exec_info->esi_func)(vp, args, level, execsz, ehdp);

	return (err);
}

/*
 * STATIC int mod_exec_core(vnode_t *vp, proc_t *p, cred_t *credp,
 *			    rlim_t rlimit, int sig)
 *
 *	Autoload core routine for loadable EXEC type modules.
 *
 * Calling/Exit State:
 *	No locks should be held on entry and no locks are held on return.
 *
 *	This routine returns the errno ENOLOAD upon failure to load the
 *	required module; otherwise, the module's actual core routine is
 *	called and its return value passed back to the caller.
 */
STATIC	int
mod_exec_core(vnode_t *vp, proc_t *p, cred_t *credp, rlim_t rlimit, int sig)
{
	struct	modctl	*modctlp;
	struct	execsw	*execp;
	int	err;
	char	*modname;

	ASSERT(getpl() == PLBASE);

	execp = p->p_execinfo->ei_execsw;
	modname = execp->exec_name;

	if ((err = modld(modname, sys_cred, &modctlp, 0)) != 0) {
		return (ENOLOAD);
	}
	MOD_HOLD_L(modctlp->mc_modp, PLBASE);

	if (execp->exec_info == &mod_execsw_info) {
		MOD_RELE(modctlp->mc_modp);
		return (ENOLOAD);
	}

	if (execp->exec_info->esi_core == NULL) {
		MOD_RELE(modctlp->mc_modp);
		return (ENOEXEC);
	}

	err = (*execp->exec_info->esi_core)(vp, p, credp, rlimit, sig);

	return (err);
}

/*
 * STATIC int mod_exec_textinfo(exhda_t *ehdp, extext_t *extxp,
 *				struct execsw *execp)
 *
 *	Autoload textinfo routine for loadable EXEC type modules.
 *
 * Calling/Exit State:
 *	No locks should be held on entry and no locks are held on return.
 *
 *	This routine returns the errno ENOLOAD upon failure to load the
 *	required module; otherwise, the module's actual textinfo routine is
 *	called and its return value passed back to the caller.
 */
STATIC	int
mod_exec_textinfo(exhda_t *ehdp, extext_t *extxp, struct execsw *execp)
{
	struct	modctl	*modctlp;
	int	err;
	char	*modname;

	ASSERT(getpl() == PLBASE);

	modname = execp->exec_name;

	if ((err = modld(modname, sys_cred, &modctlp, 0)) != 0) {
		return (ENOLOAD);
	}
	MOD_HOLD_L(modctlp->mc_modp, PLBASE);

	if (execp->exec_info == &mod_execsw_info) {
		MOD_RELE(modctlp->mc_modp);
		return (ENOLOAD);
	}

	err = (*execp->exec_info->esi_textinfo)(ehdp, extxp, execp);

	return (err);
}

struct	mod_operations	mod_exec_ops	= {
	mod_execinstall,
	mod_execremove,
	mod_execinfo,
	mod_execbind
};

/*
 * STATIC int mod_execinstall(struct mod_type_data *execdatap, 
 *	struct module *modp)
 *
 *	Install lodable EXEC type.
 *
 * Calling/Exit State:
 *	No locks should be held on entry and no locks are held on return.
 *
 *	A return value of 0 indicates success; otherwise a valid errno is returned.
 *	Errnos returned directly by this routine are:
 *
 *		EINVAL	The module being loaded has not been registered.
 */
STATIC	int
mod_execinstall(struct mod_type_data *execdatap, struct module *modp)
{
	struct	mod_exec_data	*dta;
	struct	execsw		*execp;
	struct	execsw_info	*infop;
	char	*name;

	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_execinstall()\n"));

	dta = (struct mod_exec_data *)execdatap->mtd_pdata;
	name = dta->med_name;

	infop = (struct execsw_info *)kmem_alloc(sizeof(struct execsw_info), KM_SLEEP);

	/*
	 * Find the first execsw entry for the module.
	 */
	(void)RW_WRLOCK(&mod_execsw_lock, PLDLM);
	execp = execsw;

	while(execp) {
		if (strcmp(name, execp->exec_name) == 0) {
			break;
		}
		execp = execp->exec_next;
	}

	if (!execp) {
		RW_UNLOCK(&mod_execsw_lock, PLBASE);
		kmem_free(infop, sizeof(struct execsw_info));
		return (EINVAL);
	}

	dta->med_info.esi_modp = modp;

	/*
	 * Saved for use by mod_execremove() and mod_execinfo().
	 */
	dta->med_execp = execp;

	/*
	 * Populate the execsw entry with the module's information.
	 */
	name = execp->exec_name;
	*infop = dta->med_info;

	while(execp && execp->exec_name == name) {
		execp->exec_info = infop;
		execp = execp->exec_next;
	}

	RW_UNLOCK(&mod_execsw_lock, PLBASE);

	return (0);
}

/*
 * STATIC int mod_execremove(struct mod_type_data *execdatap)
 * 	Remove a loaded EXEC type.
 *
 * Calling/Exit State:
 *	No locks should be held on entry and no locks are held on return.
 *
 *	This routine always returns 0, indicating success.
 */
STATIC	int
mod_execremove(struct mod_type_data *execdatap)
{
	struct	mod_exec_data	*dta;
	struct	execsw		*execp;
	struct	execsw_info	*infop = NULL;
	char	*name;

	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_execremove()\n"));

	(void)RW_WRLOCK(&mod_execsw_lock, PLDLM);

	dta = (struct mod_exec_data *)execdatap->mtd_pdata;

	/*
	 * Saved in mod_execinstall().
	 */
	execp = dta->med_execp;

	/*
	 * Reset for next load.
	 */
	name = execp->exec_name;
	while(execp && execp->exec_name == name) {
		if(infop == NULL) {
			infop = execp->exec_info;
		}
		execp->exec_info = &mod_execsw_info;
		execp = execp->exec_next;
	}

	RW_UNLOCK(&mod_execsw_lock, PLBASE);

	kmem_free(infop, sizeof(struct execsw_info));

	return (0);
}

/*
 * STATIC int mod_execinfo(struct mod_type_data *execdatap, 
 *	int *p0, int *p1, int *type)
 *
 *	Get information about a loaded EXEC type module.
 *
 * Calling/Exit State:
 *	No locks should be held on entry and no locks are held on return.
 *
 *	This routine always returns 0, indicating success.
 */
STATIC	int
mod_execinfo(struct mod_type_data *execdatap, int *p0, int *p1, int *type)
{
	struct	mod_exec_data	*dta;

	dta = (struct mod_exec_data *)execdatap->mtd_pdata;

	*type = MOD_TY_EXEC;

	if (dta->med_execp->exec_magic == NULL) {
		*p0 = -1;
	}
	else {
		*p0 = *dta->med_execp->exec_magic;
	}

	*p1 = -1;

	return (0);
}

/*
 * STATIC int mod_execbind(struct mod_type_data *execdatap, int *cpup)
 *
 *	Dummy routine, no need to bind exec modules.
 *
 * Calling/Exit State:
 *
 *	This routine always returns 0, indicating success.
 */
/* ARGSUSED */
STATIC	int
mod_execbind(struct mod_type_data *execdatap, int *cpup)
{
	return (0);
}

/*
 * int mod_exec_reg(void *arg)
 *	Name registration for EXEC types.
 *
 * Calling/Exit State:
 *	arg points to a mod_mreg structure in user address
 *	space. This structure contains the information
 *	needed to register the given EXEC type module.
 *	The md_typedata field of mod_mreg points to a
 *	mod_exec_tdata structure in user address space.
 *	This structure contains the EXEC type specific
 *	information required for registration.
 *
 *	No locks should be held upon entry and no locks
 *	are held on return.
 *
 *	A return value of 0 indicates success; otherwise
 *	a valid errno is returned. Errnos returned directly
 *	by this routine are:
 *
 *		EINVAL	The number of supported magic numbers
 *			is zero or negative.
 *
 *		EEXIST	The module is statically configured.
 *
 *		EFAULT	Memory fault copying data from user space.
 *
 *		EEXIST	The module is already registered.
 */
int
mod_exec_reg(void *arg)
{
	struct	mod_mreg	mr;
	struct	mod_exec_tdata	metd;
	struct	execsw		*execp, *exec_nodes, *exec_nodes_end;
	struct	execsw		**insert_nextp, **backpp;

	char		*tname;
	ushort_t	*magic, *mp;
	int		i, nm;
	size_t		memsz;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	moddebug(cmn_err(CE_CONT, "!MOD: mod_exec_adm()\n"));

	if (copyin(arg, &mr, sizeof(struct mod_mreg))) {
		return (EFAULT);
	}

	if (mod_static(mr.md_modname)) {
		return (EEXIST);
	}

	if (copyin(mr.md_typedata, &metd, sizeof(struct mod_exec_tdata))) {
		return (EFAULT);
	}

	if (metd.met_nmagic <= 0) {
		return (EINVAL);
	}

	magic = (ushort_t *)kmem_alloc(metd.met_nmagic * sizeof(ushort_t), KM_SLEEP);

	if (copyin(metd.met_magic, magic, metd.met_nmagic * sizeof(ushort_t))) {
		return (EFAULT);
	}

	nm = metd.met_nmagic + metd.met_wildcard;
	memsz = nm * sizeof(struct execsw);
	exec_nodes = (struct execsw *)kmem_alloc(memsz, KM_SLEEP);

	tname = kmem_alloc((size_t)(strlen(mr.md_modname)+1), KM_SLEEP);
	strcpy(tname, mr.md_modname);

	mp = magic;
	execp = exec_nodes;

	for (i = 0; i < nm; i++) {
		execp->exec_magic = mp;
		execp->exec_order = metd.met_order;
		execp->exec_name = tname;
		execp->exec_info = &mod_execsw_info;
		execp->exec_next = execp + 1;
		execp++;
		mp++;
	}
	exec_nodes_end = --execp;

	if (metd.met_wildcard) {
		execp->exec_magic = NULL;
	}

	insert_nextp = NULL;

	(void)RW_WRLOCK(&mod_execsw_lock, PLDLM);

	execp = execsw;
	backpp = &execsw;

	while(execp) {
		if (strcmp(execp->exec_name, tname) == 0) {
			moddebug(cmn_err(CE_CONT, 
				"!MOD: %s,  already registered\n", tname));

			kmem_free(magic, metd.met_nmagic * sizeof(ushort_t));
			kmem_free(exec_nodes, memsz);
			kmem_free(tname, (size_t)(strlen(mr.md_modname)+1));
			RW_UNLOCK(&mod_execsw_lock, PLBASE);

			return (EEXIST);
		}

		if (insert_nextp == NULL) {
			if (execp->exec_order <= metd.met_order) {
				insert_nextp = backpp;
			}
		}

		backpp = &(execp->exec_next);
		execp = execp->exec_next;
	}

	exec_nodes_end->exec_next = *insert_nextp;
	*insert_nextp = exec_nodes;

	RW_UNLOCK(&mod_execsw_lock, PLBASE);

	return (0);
}
