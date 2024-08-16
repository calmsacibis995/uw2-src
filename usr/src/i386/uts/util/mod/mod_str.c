/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/mod/mod_str.c	1.11"
#ident	"$Header: $"

#include <io/conf.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <mem/seg_kvn.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/mod/mod.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/moddefs.h>

/*
 * Dynamically loadable STREAMS module support.
 */

extern	int	fmodswsz;
extern	rwlock_t	mod_fmodsw_lock;

int mod_smod_open(queue_t *, dev_t *, int, int, cred_t *);
STATIC int mod_strinstall(struct mod_type_data *, struct module *);
STATIC int mod_strremove(struct mod_type_data *);
STATIC int mod_strinfo(struct mod_type_data *, int *, int *, int *);
STATIC int mod_strbind(struct mod_type_data *, int *);

static	struct	module_info	smodm_info = { 0, "MOD", 0, 0, 0, 0 };

static	struct	qinit	smodrinit = {
	putnext, NULL, mod_smod_open, NULL, NULL, &smodm_info, NULL
};
static	struct	qinit	smodwinit = {
	putnext, NULL, NULL, NULL, NULL, &smodm_info, NULL
};

static	struct	streamtab	modsminfo = { &smodrinit, &smodwinit };

static	int	modmodflag = D_MP;

struct	mod_operations	mod_str_ops	= {
	mod_strinstall,
	mod_strremove,
	mod_strinfo,
	mod_strbind
};

/*
 * int mod_str_reg(void *arg)
 *	Register loadable module for STREAMS drivers
 *
 * Calling/Exit State:
 *	If successful, the routine returns 0, else the appropriate errno.
 */
int
mod_str_reg(void *arg)
{
	struct	mod_mreg	mr;
	struct	fmodsw	*fmodp;
	pl_t pl;

	int	i;
	char	*sname, *tname;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	moddebug(cmn_err(CE_CONT, "!MOD: mod_str_adm()\n"));

	if(copyin(arg, &mr, sizeof(struct mod_mreg)))	{
		return(EFAULT);
	}

	if (mod_static(mr.md_modname))
		return (EEXIST);

	sname = mr.md_modname;
	tname = NULL;
	pl = RW_WRLOCK(&mod_fmodsw_lock, PLDLM);
	for (fmodp = fmodsw, i=0; i < fmodswsz; i++)	{
		if(*fmodp->f_name == '\0')	{
			tname = fmodp->f_name;
			break;
		}
		if(strcmp(fmodp->f_name, sname) == 0)	{
			ASSERT(fmodp->f_str != NULL);
			ASSERT(fmodp->f_flag != NULL);
			RW_UNLOCK(&mod_fmodsw_lock, pl);
			moddebug(cmn_err(CE_CONT, 
					 "!MOD: %s, reregistered\n", sname));
			return(0);
		}
		fmodp++;
	}
	if(!tname)	{
		RW_UNLOCK(&mod_fmodsw_lock, pl);
		moddebug(cmn_err(CE_CONT, "!MOD: mod_str_adm(): ECONFIG\n"));
		return(ECONFIG);
	}

	strncpy(tname, sname, FMNAMESZ + 1);
	fmodp->f_str = &modsminfo;
	fmodp->f_flag = &modmodflag;
	RW_UNLOCK(&mod_fmodsw_lock, pl);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_str_adm(): %s, %d\n", tname, i));

	return(0);
}

/*
 * int mod_smod_open(queue_t *qp, dev_t *devp, int flag, 
 *	int sflag, cred_t *credp)
 *
 * 	Autoload routine for STREAMS modules. Called through fmodsw[].
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
/* ARGSUSED */
int
mod_smod_open(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	struct	fmodsw		*fmodp;
	int	idx, err;
	pl_t	pl, oldpl;
	struct	modctl		*mcp;
	struct	module		*modp;
	queue_t *saveqp;

	ASSERT(KS_HOLD0LOCKS());

	idx = QU_MODIDX(qp);	/* passed from qattach() */

	moddebug(cmn_err(CE_CONT, "!MOD: mod_smod_open() idx = %d\n", idx));

	ASSERT(idx >= 0 && idx < fmodswsz);

	fmodp = &fmodsw[idx];
	if (fmodp->f_name[0] == '\0')
		/* Not registered. */
		return (EINVAL);

	/*
	 * The routine is called at PLSTR, we need to lower it to
	 * PLBASE when handling loading. Before we call the open
	 * routine, the ipl value will be set back to PLSTR.
	 */
	oldpl = spl0();

	/*
	 * Load the module registered to this fmodsw[] index.
	 */
	if (err = modld(fmodp->f_name, sys_cred, &mcp, 0)) {
		moddebug(cmn_err(CE_CONT, "!MOD: load failed errno = %d\n",
			 err));
		splx(oldpl);
		return (err);
	}
	modp = mcp->mc_modp;
	MOD_HOLD_L(modp, PLBASE);

	/* lock the stream */
	pl = LOCK(qp->q_str->sd_mutex, PLSTR);

	/*
	 * Remove the old queues allocated before calling this routine.
	 * It hasn't initialized anything yet and there are no messages queued.
	 */
	saveqp = qp->q_next;
	if (WR(qp)->q_next)
	        backq_l(qp)->q_next = qp->q_next;
	if (qp->q_next)
	        backq_l(WR(qp))->q_next = WR(qp)->q_next;

	/* unlock the stream */
	UNLOCK(qp->q_str->sd_mutex, pl);

	/* free the data structure */
	freeq(qp);

	/*
	 * Call qattach() recursively.
	 *
	 * saveqp is from above
	 * devp is an argument to this routine
	 * flag is an argument to this routine
	 * idx is from above
	 * credp is an argument to this routine
	 */
	err = qattach(saveqp, devp, flag, FMODSW, idx, credp, 1);

	MOD_RELE(modp);
	splx(oldpl);
	return (err);
}

/*
 * STATIC int mod_strinstall(struct mod_type_data *strdatap, 
 *	struct module *modp)
 *
 *	Connect the STREAMS module to the fmodsw table.
 *
 * Calling/Exit State:
 *	The routine will fail if the STREAMS module is not registered.
 *	No locks held upon calling and exit.
 */
STATIC int
mod_strinstall(struct mod_type_data *strdatap, struct module *modp)
{
	struct	fmodsw	*dta;
	struct	fmodsw	*fmodp;
	int	idx;
	char	*name;
	pl_t	pl;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	moddebug(cmn_err(CE_CONT, "!MOD: mod_strinstall()\n"));

	dta = (struct fmodsw *)strdatap->mtd_pdata;
	name = dta->f_name;

	/*
	 * Find the fmodsw[] table entry for the module.
	 */
	if((idx = findmod(name)) < 0)	{
		moddebug(cmn_err(CE_CONT, 
			"!MOD: module %s, not registered\n", name));
		return(EINVAL);
	}

	moddebug(cmn_err(CE_CONT, "!MOD: f_name = %s, idx = %d\n", name, idx));

	/*
	 * Populate the fmodsw[] entry with the module's information.
	 */
	pl = RW_WRLOCK(&mod_fmodsw_lock, PLDLM);
	fmodp = &fmodsw[idx];
	fmodp->f_str = dta->f_str;
	fmodp->f_flag = dta->f_flag;

	/*
	 * Save the address of the module's fmodsw[] entry.
	 */ 
	strdatap->mtd_pdata = (void *)fmodp;
	fmodp->f_modp = modp;
	RW_UNLOCK(&mod_fmodsw_lock, pl);

	return(0);
}

/*
 * STATIC int mod_strremove(struct mod_type_data *strdatap)
 *	Disconnect the STREAMS module from the fmodsw table.
 *
 * Calling/Exit State:
 *	No locks held upon calling and exit.
 */
STATIC int
mod_strremove(struct mod_type_data *strdatap)
{
	struct	fmodsw	*dta;
	pl_t pl;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	moddebug(cmn_err(CE_CONT, "!MOD: mod_strremove()\n"));

	/*
	 * Saved in mod_strinstall().
	 */
	dta = (struct fmodsw *)strdatap->mtd_pdata;

	moddebug(cmn_err(CE_CONT, 
		"!MOD: f_name = %s, disconnected\n", dta->f_name));

	/*
	 * Reset for next load.
	 */
	pl = RW_WRLOCK(&mod_fmodsw_lock, PLDLM);
	dta->f_str = &modsminfo;
	dta->f_flag = &modmodflag;
	dta->f_modp = NULL;
	RW_UNLOCK(&mod_fmodsw_lock, pl);

	return(0);
}

/*
 * STATIC int mod_strinfo(struct mod_type_data *strdatap, 
 *			  int *p0, int *p1, int *type)
 *	Return the module type and the index of the file system
 *	type into vfssw table.
 *
 * Calling/Exit State:
 *	The keepcnt of the STREAMS module is non-zero upon
 *	calling and exit of this routine.
 */
STATIC int
mod_strinfo(struct mod_type_data *strdatap, int *p0, int *p1, int *type)
{
	struct	fmodsw	*dta;

	/*
	 * Saved in mod_strinstall().
	 */
	dta = (struct fmodsw *)strdatap->mtd_pdata;

	*type = MOD_TY_STR;

	*p0 = dta - fmodsw;
	*p1 = -1;

	return(0);
}

/*
 * STATIC int mod_strbind(struct mod_type_data *strdatap, int *cpup)
 *	Routine to handle cpu binding for non-MP modules.
 *	The cpu to be bound to is returned in cpup.
 *
 * Calling/Exit State:
 *	Returns 0 on success or appropriate errno if failed.
 */
STATIC int
mod_strbind(struct mod_type_data *strdatap, int *cpup)
{
	struct fmodsw	*dta;

	dta = (struct fmodsw *)strdatap->mtd_pdata;

	/* do nothing if the module is multi threaded */
	if (*dta->f_flag & D_MP)
		return (0);

	if (*cpup > 0) {
		/*
		 *+ STREAMS modules, if not multithreaded, must be bound
		 *+ to cpu 0. If the module is configured to bind with another
		 *+ cpu through other linkages, the load of the module will
		 *+ be failed.
		 */
		cmn_err(CE_WARN,
	"The STREAMS module %s is illegally configured to bind with cpu %d,\n\
the module cannot be loaded.", dta->f_name, *cpup);
		return (ECONFIG);
	}

	/*
	 * Single threaded STREAMS module always bind to cpu 0.
	 */
	*cpup = 0;
	return (0);
}
