/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/priocntl.c	1.22"
#ident	"$Header: $"

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/param.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <util/debug.h>
#include <util/var.h>
#include <proc/priocntl.h>
#include <proc/class.h>
#include <util/ksynch.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <svc/systm.h>
#include <proc/disp.h>
#include <acc/mac/mac.h>
#include <acc/audit/audit.h>
#include <acc/audit/auditrec.h>

#define NZERO	20  /* for nice */
/*
 * Structure used to pass arguments to the priocntlsys() function.
 */
struct priocntlargs {
	int		pc_version;
	procset_t	*psp;
	int		cmd;
	caddr_t		arg;
};

/*
 * Structure used to pass arguments to the priocntllst() function.
 */
struct priocntllstargs {
	int		pc_version;
	lwpid_t 	*lwpidp;
	int		idcnt;
	int		cmd;
	caddr_t		arg;
};
/*

 * Structure used to pass arguments to the lwpcmp() function.
 * The arguments must be passed in a structure because lwpcmp()
 * is called indirectly through the dotolwps() function which
 * will only pass through a single one word argument.
 */
struct lwpcmpargs {
	id_t	*lwpcmp_cidp;
	int	*lwpcmp_cntp;
	int	lwpcmp_gotlwp;		/* true if we found lwpcmp_parms is valid */
	pcparms_t lwpcmp_parms;		/* parameters of current "winner" */
};

/*
 * Structure used to pass arguments to the setparms() function
 * which is called indirectly through dotolwps().
 */
struct stprmargs {
	struct pcparms	*stp_parmsp;	/* ptr to parameters */
	void		*stp_classdata;	/* ptr to per-class operation data */
	int		stp_error;	/* some errors returned here */
	int		*stp_plistp;	/* # of processes and their IDs */
};

STATIC int lwpcmp(), setparms(), set_ageparms(), get_ageparms();

/*
 * The following sleep lock provides mutex for PC_ADMIN's.
 */
STATIC sleep_t priocntl_update;
LKINFO_DECL(priocntl_updinfo, "priocntl_update sleep lock", 0);

/*
 * The following sleep lock provides mutex for priocntlsys
 * atomic changes to groups of lwp's
 */
STATIC sleep_t priocntl_lst_lck;
LKINFO_DECL(priocntl_class, "priocntl_lst sleep lock", 0);

/*
 * void priocntl_init(void)
 *
 *	Initialize priocntl state.
 *
 * Calling/Exit State:
 *
 *	Must be called on the boot processor at initialization time.
 *	Currently only needs to initialize the priocntl_update sleep lock.
 */
void
priocntl_init(void)
{
	SLEEP_INIT(&priocntl_update, 0, &priocntl_updinfo, KM_NOSLEEP);
	SLEEP_INIT(&priocntl_lst_lck, 0, &priocntl_class, KM_NOSLEEP);
}

/*
 * int priocntlsys(struct priocntlargs *uap, rval_t *rvp)
 *
 *	Front end for the priocntl() and priocntlsys()
 *	system calls.
 *
 * Calling/Exit State:
 *
 *	Called from the system call entry point.
 */
int
priocntlsys(struct priocntlargs	*uap, rval_t *rvp)
{
	procset_t psp;
	int dopriocntl();
	extern boolean_t aging_tune_priv;

	switch(uap->cmd) {

	case PC_SETAGEPARMS:
	if (pm_denied(CRED(), P_SYSOPS) && !aging_tune_priv)
		return EPERM;

	case PC_GETAGEPARMS:	/* fall thru */
	case PC_SETPARMS:	/* fall thru */
	case PC_GETPARMS:

	if (copyin((caddr_t)uap->psp, (caddr_t)&psp, sizeof(procset_t)))
		return(EFAULT);
	break;

	default:		/* fall thru */
	break;

	}

	return(dopriocntl(uap->pc_version, uap->cmd, uap->arg, &psp, rvp));
}

/*
 * int priocntllst(struct priocntllstargs *uap, rval_t *rvp)
 *
 *	The priocntllst() system call.
 *
 * Calling/Exit State:
 *
 *	Called from the system call entry point.
 */
int
priocntllst(struct priocntllstargs *uap, rval_t *rvp)
{
	procset_t psp;
	pcparms_t	pcparms;
	lwpid_t lwp_buf[PRLST_LOCAL];		/* local buffer for lwpid's */
	lwpid_t *lwpid_list=lwp_buf;		/* the list of lwpid's */
	int error=0;
	int cur_lwpid;
	void * classdata;


	if (uap->idcnt <= 0 )
		return(EINVAL);
	/* 
	 * to keep the manipulation of lwp's into classes
	 * consistant we lock here
	 */

	if (!SLEEP_LOCK_SIG(&priocntl_lst_lck, PRISLEP))
		return(EINTR);
						/* copy in users change data */
	switch(uap->cmd) {

		case PC_SETPARMS:	/* fall thru */
		case PC_GETPARMS:
			if (copyin(uap->arg, (caddr_t)&pcparms, sizeof(pcparms))) {
				error = EFAULT;
				goto end2;
			}

			/* check for invalid class */

			if (pcparms.pc_cid >= nclass || pcparms.pc_cid < 1) {
				error = EINVAL;
				goto end2;
			}

			/*
			 * Allocate necessary aggregation-wide data required
			 * by the scheduling class. 
			 */

			 if ((error = parmsstart(&pcparms, &classdata)))
				goto end2;
			break;

		default:	/* fall thru */
			break;

	}

	/*
	 * allocate space for the lwpid_t list from
	 * either a local array or from an allocator
  	 * in the case of really large groups
	 */

	if (uap->idcnt <= PRLST_LOCAL ){

		lwpid_list = lwp_buf;
	} else {
		lwpid_list = (lwpid_t*) kmem_alloc(uap->idcnt * sizeof(lwpid_t),
				 KM_SLEEP);
	}
							/* get lwpid list */
	if (copyin(uap->lwpidp, (caddr_t)lwpid_list, 
		sizeof(lwpid_t)*uap->idcnt)) {
		error = EFAULT;
		goto end1;
	}


	/*
	 * 	loop thru the list of lwpid's 
	 *	letting dopriocntl() do the work
	 */

	for (cur_lwpid=0; uap->idcnt >0 ; cur_lwpid++,uap->idcnt--) {

		int err=0;		/* local error accounting */

		setprocset(&psp, POP_AND, P_LWPID, 
			lwpid_list[cur_lwpid], P_ALL, 0);

		err = dopriocntl(uap->pc_version, uap->cmd,
			 uap->arg, &psp, rvp);

					/* remember non fatal error */
		if (err ) {
			error = err;

			if( error != ESRCH )
				break;
		} 
	}

	/*
	 * Indicate to the scheduling class the aggragate-operation
	 * is not complete.
	 */
	if ((uap->cmd == PC_SETPARMS) || (uap->cmd == PC_GETPARMS))
		parmsend(&pcparms, classdata);

end1:

	if (uap->idcnt > PRLST_LOCAL )		/* free any allocated memory*/
		kmem_free(lwpid_list, (uap->idcnt * sizeof(lwpid_t)));


end2:
	SLEEP_UNLOCK(&priocntl_lst_lck);	 /* release the lock */

	return(error);
}

/* 
 *  	common code for the:
 *
 *	priocntl(), priocntlset() and priocntllist()
 *      system calls
 *
 * Calling/Exit State:
 *
 *	called with priocntl_lst_lck held 
 *	only when called by priocntllist()
 */

int
dopriocntl(int version, int cmd, void *uarg, procset_t *psp, rval_t *rvp)
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	pcadmin_t	pcadmin;
	struct stprmargs stprmargs;
	struct lwpcmpargs lwpcmpargs;
	int		count;
	int		clnullflag;
	int	error = 0;
	void		*classdata = NULL;
	lwp_t	*lwpp;
	ageparms_t age_args;

	/*
	 * First just check the version number. Right now there
	 * is only one version we know about and support.  If we
	 * get some other version number from the application it
	 * may be that the application was built with some future
	 * version and is trying to run on an old release of the
	 * system (that's us).  In any case if we don't recognize
	 * the version number all we can do is return error.
	 */
	if (version != PC_VERSION)
		return(EINVAL);


	switch(cmd) {

	case PC_SETAGEPARMS:
		if (copyin(uarg, (caddr_t)&age_args, sizeof(ageparms_t)))
			return(EFAULT);

		error = dotoprocs(psp, B_TRUE, set_ageparms, &age_args);
		break;


	case PC_GETAGEPARMS:
		if (copyin(uarg, (caddr_t)&age_args, sizeof(ageparms_t)))
			return(EFAULT);

		if (psp->p_lidtype != P_PID)
			return EINVAL;
		error = dotoprocs(psp, B_TRUE, get_ageparms, &age_args);
		if (!error) {
			if (copyout((caddr_t)&age_args, uarg,
					sizeof(ageparms_t)))
				return(EFAULT);
		}
		break;

	case PC_GETCID:
		/*
		 * If the arg pointer is NULL, the user just wants to know
		 * the number of classes. If non-NULL, the pointer should
		 * point to a valid user pcinfo buffer.
		 */
		if (uarg == NULL) {
			rvp->r_val1 = nclass;
			break;
		} else {
			if (copyin(uarg, (caddr_t)&pcinfo, sizeof(pcinfo)))
				return(EFAULT);
		}

		/*
		 * Get the class ID corresponding to user supplied name.
		 */
		error = getcid(pcinfo.pc_clname, &pcinfo.pc_cid);
		if (error)
			return(error);

		/*
		 * Can't get info about the sys class.
		 */
		if (pcinfo.pc_cid == 0)
			return(EINVAL);

		/*
		 * Get the class specific information.
		 */
		error = CL_GETCLINFO(&class[pcinfo.pc_cid], pcinfo.pc_clinfo,
			u.u_lwpp->l_cid, u.u_procp->p_cred);
		if (error)
			return(error);

		if (copyout((caddr_t)&pcinfo, uarg, sizeof(pcinfo)))
			return(EFAULT);

		rvp->r_val1 = nclass;

		break;

	case PC_GETCLINFO:

		/*
		 * If the arg pointer is NULL, the user just wants to know
		 * the number of classes. If non-NULL, the pointer should
		 * point to a valid user pcinfo buffer.
		 */
		if (uarg == NULL) {
			rvp->r_val1 = nclass;
			break;
		} else {
			if (copyin(uarg, (caddr_t)&pcinfo, sizeof(pcinfo)))
				return(EFAULT);
		}

		if (pcinfo.pc_cid >= nclass || pcinfo.pc_cid < 1)
			return(EINVAL);

		bcopy(class[pcinfo.pc_cid].cl_name, pcinfo.pc_clname, PC_CLNMSZ);

		/*
		 * Get the class specific information.
		 */
		error = CL_GETCLINFO(&class[pcinfo.pc_cid], pcinfo.pc_clinfo,
			u.u_lwpp->l_cid, u.u_procp->p_cred);
		if (error)
			return(error);

		if (copyout((caddr_t)&pcinfo, uarg, sizeof(pcinfo)))
			return(EFAULT);

		rvp->r_val1 = nclass;

		break;

	case PC_SETPARMS:
		if (copyin(uarg, (caddr_t)&pcparms, sizeof(pcparms)))
			return(EFAULT);

		/*
		 * First check the validity of the parameters
		 * we got from the user.  We don't do any permissions
		 * checking here because it's done on a per process
		 * basis by parmsset().
		 */
		error = parmsin(&pcparms);
		if (error)
			return(error);

		stprmargs.stp_parmsp = &pcparms;
		stprmargs.stp_classdata = classdata;
		stprmargs.stp_error = 0;
		stprmargs.stp_plistp = NULL;
		if (u.u_lwpp->l_auditp) {
			CL_AUDIT(&class[pcparms.pc_cid], ADTTST, cmd, 0, NULL, NULL);
			ADT_SCHEDINIT(u.u_lwpp->l_auditp, stprmargs);
		}

		/*
		 * The dotolwps() call below will cause setparms()
		 * to be called for each lwp in the specified
		 * set. setparms() will in turn call parmsset()
		 * (which does the real work).
		 */
		error = dotolwps(psp, B_TRUE,
				 setparms, (void *)&stprmargs);
	
		/*
		 * If setparms() encounters a permissions error for
		 * one or more of the processes (lwp's) it returns
		 * EPERM in stp_error so dotolwps() will continue
		 * through the process set.  If dotolwps() returned
		 * an error above, it was more serious than permissions
		 * and dotolwps quit when the error was encountered.
		 * We return the more serious error if there was one,
		 * otherwise we return EPERM if we got that back.
		 */
		if (error == 0 && stprmargs.stp_error != 0)
			error = stprmargs.stp_error;

		if (u.u_lwpp->l_auditp && (u.u_lwpp->l_auditp->al_flags & AUDITME))
                        CL_AUDIT(&class[pcparms.pc_cid], ADTDUMP, cmd, error,
				 (void *)stprmargs.stp_parmsp,
                                 (void *)stprmargs.stp_classdata); 

		break;

	case PC_GETPARMS:
		if (copyin(uarg, (caddr_t)&pcparms, sizeof(pcparms)))
			return(EFAULT);

		if (pcparms.pc_cid >= nclass ||
		    (pcparms.pc_cid < 1 && pcparms.pc_cid != PC_CLNULL))
			return(EINVAL);


		/*
		 * Select the process (from the set) whose
		 * parameters we are going to return.  First we
		 * set up some locations for return values, then
		 * we call lwpcmp() indirectly through dotolwps().
		 * lwpcmp() will call a class specific routine which
		 * actually does the selection.  To understand how
		 * this works take a careful look at the code below,
		 * the dotolwps() function, the lwpcmp() function,
		 * and the class specific cl_lwpcmp() functions.
		 */
		if (pcparms.pc_cid == PC_CLNULL)
			clnullflag = 1;
		else
			clnullflag = 0;
		count = 0;
		lwpcmpargs.lwpcmp_cidp = &pcparms.pc_cid;
		lwpcmpargs.lwpcmp_cntp = &count;
		lwpcmpargs.lwpcmp_gotlwp = 0;
		bzero(&lwpcmpargs.lwpcmp_parms, sizeof(lwpcmpargs.lwpcmp_parms));

		error = dotolwps(psp, B_FALSE,
				 lwpcmp, (void *)&lwpcmpargs);
		if (error)
			return(error);

		/*
		 * If dotolwps returned success it found at least
		 * one process in the set.  If lwpcmp() failed to
		 * select a process it is because the user specified
		 * a class and none of the processes in the set
		 * belonged to that class.
		 */
		if (!lwpcmpargs.lwpcmp_gotlwp) {
			return(ESRCH);
		}

		/*
		 * User can only use PC_CLNULL with one process in set.
		 */
		if (clnullflag && count > 1)
			return(EINVAL);

		/*
		 * We've selected a process so now get the parameters.
		 */

		/*
		 * Prepare to return parameters to the user
		 */
		error = parmsout(&lwpcmpargs.lwpcmp_parms);
		if (error)
			return(error);

		if (copyout((caddr_t)&lwpcmpargs.lwpcmp_parms, uarg,
			    sizeof(lwpcmpargs.lwpcmp_parms)))
			return(EFAULT);

		/*
		 * And finally, return the pid of the selected process.
		 */
		rvp->r_val1 = lwpcmpargs.lwpcmp_gotlwp;
		break;

	case PC_ADMIN:
		if (copyin(uarg, (caddr_t)&pcadmin, sizeof(pcadmin_t)))
			return(EFAULT);

		if (pcadmin.pc_cid >= nclass || pcadmin.pc_cid < 1)
			return(EINVAL);

		/*
		 * We provide interlock here as a courtesy to the
		 * individual scheduling classes.  It can be argued
		 * this interlock should be provided by the scheduling
		 * class on its own behalf, however, the requirement for
		 * such an interlock is most certainly necessary for
		 * all, non-trivial scheduling classes.
		 */
		if (!SLEEP_LOCK_SIG(&priocntl_update, PRISLEP))
			return(EINTR);

		/*
		 * Have the class do whatever the user is requesting.
		 */
		error = CL_ADMIN(&class[pcadmin.pc_cid], pcadmin.pc_cladmin,
		    u.u_lwpp->l_cid, u.u_lwpp->l_cred);

		SLEEP_UNLOCK(&priocntl_update);
		if (u.u_lwpp->l_auditp)
                        CL_AUDIT(&class[pcadmin.pc_cid], ADTDUMP, cmd, error,
                                 pcadmin.pc_cladmin, NULL);

		break;

	case PC_PRMPTPOLL:
		break;

	case PC_YIELD:
		lwpp = u.u_lwpp;
		error = CL_YIELD(lwpp, lwpp->l_cllwpp, (int)uarg);
		break;

	default:
		error = EINVAL;
		break;
	}
	return(error);
}


/*
 * void
 * yield(void)
 *	Direct kernel interface to CL_YIELD, for binary add-on modules.
 *
 * Calling/Exit State:
 *	No locks held.  Yields the processor before returning.
 */

void
yield(void)
{
	(void) CL_YIELD(u.u_lwpp, u.u_lwpp->l_cllwpp, B_TRUE);
}


/*
 * We support the nice system call for compatibility although
 * the priocntl system call supports a superset of nice's functionality.
 * We support nice only for time sharing processes.  It will fail
 * if called by a process from another class.
 * The code below is an aberration.  Class independent kernel code
 * (such as this) should never require specific knowledge of any
 * particular class.  An alternative would be to add a CL_NICE
 * function call to the class switch and require all classes other than
 * time sharing to implement a class specific nice function which simply
 * returns EINVAL.  This would be even uglier than the approach we have
 * chosen below.
 */

struct niceargs {
	int	niceness;
};

extern int donice(lwp_t *, cred_t *, int, int *);

/*
 * 
 * int
 * nice(struct niceargs *uap, rval_t *rvp)
 *	Nice(2) system call.
 *
 * Calling/Exit State:
 *	No locking on required.
 *
 */
int
nice(struct niceargs *uap, rval_t *rvp)
{
	int error, retval;

	if (error = donice(u.u_lwpp, u.u_procp->p_cred, uap->niceness, &retval))
		return(error);

	rvp->r_val1 = retval;
	return(0);
}


extern void	ts_donice();

/*
 * 
 * int
 * donice(lwp_t *lwpp, cred_t *cr, int incr, int *retvalp)
 *	Performs some error checking before calling ts_donice.
 *
 * Calling/Exit State:
 *	No locking required.
 *
 */
int
donice(lwp_t *lwpp, cred_t *cr, int incr, int *retvalp)
{

	pl_t pl;

	/*
	 * Check that the class is time-sharing.
	 */

	if (strcmp(class[lwpp->l_cid].cl_name, "TS") != 0)
		/*
		 * Process from some class other than time-sharing.
		 */
		return(EINVAL);

	if ((incr < 0 || incr > 2 * NZERO) && pm_denied(cr, P_TSHAR))
		return(EPERM);

	/*
	 * Call the time-sharing class to take care of
	 * all the time-sharing specific stuff.
	 */

	pl = LOCK(&lwpp->l_mutex,PLHI);

	ts_donice(lwpp->l_cllwpp,incr,retvalp);

	UNLOCK(&lwpp->l_mutex,pl);

	return(0);
}

/*
 *
 * STATIC
 * int set_ageparms(proc_t *pp, ageparms_t *argp)
 *	Set the aging parameters in argp for the process pp.
 *
 * Calling/Exit State:
 *	Called with p_mutex of the process held and is held on the
 *	return.
 *
 * Remarks:
 *	If any of the parameters has value of 0, then this signifies
 *	that the parameter should be left unchanged.
 */
STATIC int
set_ageparms(proc_t *pp, ageparms_t *argp)
{
	if (pp->p_as == NULL)	/* things like nfsd */
		return 0;
	if (argp->max_agequantum && argp->min_agequantum) {
		if (argp->min_agequantum > argp->max_agequantum)
			return EINVAL;
	} else if (argp->min_agequantum) {
		if (argp->min_agequantum > pp->p_as->a_max_agequantum)
			return EINVAL;
	} else if (argp->max_agequantum) {
		if (argp->max_agequantum < pp->p_as->a_min_agequantum)
			return EINVAL;
	} 
			
	if (argp->maxrss)
		pp->p_as->a_maxrss = argp->maxrss;
	if (argp->et_age_interval)
		pp->p_as->a_et_age_interval = argp->et_age_interval * HZ;
	if (argp->init_agequantum)
		pp->p_as->a_init_agequantum = argp->init_agequantum;
	if (argp->min_agequantum)
		pp->p_as->a_min_agequantum = argp->min_agequantum;
	if (argp->max_agequantum)
		pp->p_as->a_max_agequantum = argp->max_agequantum;

	return 0;
}

/*
 *
 * STATIC
 * int get_ageparms(proc_t *pp, ageparms_t *argp)
 *	Obtain the aging parameters in outarg argp for the process pp.
 *
 * Calling/Exit State:
 *	Called with p_mutex of the process held and is held on the
 *	return.
 *
 * Description:
 *	The parameters are set to if p_as is NULL.
 */
STATIC int
get_ageparms(proc_t *pp, ageparms_t *argp)
{
	if (pp->p_as == NULL)	{ /* things like nfsd */
		bzero(argp, sizeof(ageparms_t));
		return 0;
	}
	argp->maxrss = pp->p_as->a_maxrss;
	argp->et_age_interval = pp->p_as->a_et_age_interval / HZ;
	argp->init_agequantum = pp->p_as->a_init_agequantum;
	argp->min_agequantum = pp->p_as->a_min_agequantum;
	argp->max_agequantum = pp->p_as->a_max_agequantum;

	return 0;
}


/*
 *
 * int lwpcmp(lwp_t *lwpp, struct lwpcmpargs *argp, int *gotpid)
 *
 *	Select the lwp representative of a particular scheduling
 *	class.
 *
 * Calling/Exit State:
 *
 *	Called with lwpp's mutex and containing process's mutex held.
 *
 * Description:
 *
 *	The lwpcmp() function is part of the implementation of the
 *	PC_GETPARMS command of the priocntl system call.  This function
 *	works with the system call code and with the class specific
 *	cl_lwpcmp() function to select one lwp from all the lwp's in
 *	procset based on class specific criteria. lwpcmp() is called
 *	indirectly from the priocntl code through the dotolwps function.
 *	Basic strategy is dotolwps() calls us once for each lwp in
 *	the set.  We in turn call the class specific function to compare
 *	the current lwp from dotolwps to the "best" (according to
 *	the class criteria) set of parameters found so far.  We keep the
 *	"best" parameters in lwpcmp_parms.pc_clparms.
 */
/* ARGSUSED */
STATIC int
lwpcmp(lwp_t *lwpp, struct lwpcmpargs *argp, int *gotpid)
{
	void *cllwp1p;

	/*
	 * Don't consider lwp's which we don't have permission to look
	 * at their parameters.
	 */
	if (MAC_ACCESS(MACDOM, u.u_lwpp->l_cred->cr_lid, 
	    lwpp->l_procp->p_cred->cr_lid) && 
	    pm_denied(CRED(), P_MACREAD))
		return(ESRCH);

	(*argp->lwpcmp_cntp)++;	/* Increment count of procs in the set */
	if (*argp->lwpcmp_cidp == PC_CLNULL)
		*argp->lwpcmp_cidp = lwpp->l_cid;
	else if (lwpp->l_cid != *argp->lwpcmp_cidp)

		/*
		 * Process is in set but not in class.
		 */
		return(0);

	if (!argp->lwpcmp_gotlwp) {
		/*
		 * First time through for this set.
		 */
		argp->lwpcmp_gotlwp = lwpp->l_procp->p_pidp->pid_id;
		argp->lwpcmp_parms.pc_cid = lwpp->l_cid;
		CL_PARMSGET(lwpp, lwpp->l_cllwpp,
			    argp->lwpcmp_parms.pc_clparms);
		return(0);
	}

	cllwp1p = lwpp->l_cllwpp;
	if (CL_LWPCMP(lwpp, cllwp1p, argp->lwpcmp_parms.pc_clparms) > 0) {
		CL_PARMSGET(lwpp, cllwp1p, argp->lwpcmp_parms.pc_clparms);
		argp->lwpcmp_gotlwp = lwpp->l_procp->p_pidp->pid_id;
	}
	return(0);
}


/*
 *
 * int setparms(lwp_t *targlwpp, struct stprmargs *stprmp, int *gotpid)
 *
 *	Interface between dotolwps and parmsset.
 *
 * Calling/Exit State:
 *
 *	Called with the l_mutex of targlwpp and the p_mutex of the
 *	enclosing process held.  Returns any error encountered (which
 *	terminates the dotolwps) except for EPERM.  In the EPERM case, the
 *	operation is continued and the error message is stashed off to
 *	the side to be reflected back to the user.  Thus, in EPERM, the
 *	operation is applied to all allowed and an indication is set to
 *	indicate some haven't received the operation.
 *
 * Description:
 *
 *	The setparms() function is called indirectly by priocntlsys()
 *	through the dotolwps() function).  Setparms() acts as an
 *	intermediary between dotolwps() and the parmsset() function,
 *	calling parmsset() for each process in the set and handling
 *	the error returns on their way back up to dotolwps().
 */
STATIC int
setparms(lwp_t *targlwpp, struct stprmargs *stprmp, int *gotpid)
{
	int	error;

	ASSERT(targlwpp->l_cllwpp != NULL);
	ADT_SCHED_GPID(stprmp, gotpid, targlwpp);
	error = parmsset(stprmp->stp_classdata, stprmp->stp_parmsp,
			 u.u_lwpp, targlwpp);
	if (error == EPERM) {
		stprmp->stp_error = EPERM;
		return(0);
	} else
		return(error);
}
