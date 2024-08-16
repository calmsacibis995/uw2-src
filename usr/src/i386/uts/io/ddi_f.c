/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:io/ddi_f.c	1.12"
#ident	"$Header: $"

/*
 * platform-specific DDI routines
 */

#include <mem/faultcatch.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define _DDI_C
#include <io/ddi.h>
#include <io/f_ddi.h>


/*
 * int
 * etoimajor(major_t emajnum)     
 *	return internal major number corresponding to external
 *	major number argument or -1 if the external major number
 *	exceeds the bdevsw and cdevsw count *
 *	For the 386 internal major = external major
 *
 * Calling/Exit State:
 *	none
 */
int
etoimajor(major_t emajnum)
{
	if (emajnum > L_MAXMAJ)
		return (-1); /* invalid external major */

	return ((int) emajnum);
}

/* 
 * int
 * itoemajor(major_t imajnum, int lastemaj)
 *	return external major number corresponding to internal
 *	major number argument or -1 if no external major number
 *	can be found after lastemaj that maps to the internal
 *	major number. Pass a lastemaj val of -1 to start
 *	the search initially. (Typical use of this function is
 *	of the form:
 *	     lastemaj=-1;
 *	     while ( (lastemaj = itoemajor(imag,lastemaj)) != -1)
 *	        { process major number }
 *	For the 386 internal major = external major
 *
 * Calling/Exit State:
 *	none
 */
int
itoemajor(major_t imajnum, int lastemaj)
{
	extern int bdevcnt, cdevcnt;

	if (imajnum >= max(bdevcnt, cdevcnt))
		return (-1);

	/* if lastemaj == -1 then start from beginning of MAJOR table */
	if (lastemaj == -1)
		return ( (int) imajnum);

	return(-1);
}


#define	CALLBACK_HIER	0
#define	CALLBACK_SIZE	sizeof(struct callback)

STATIC struct callback {
	int	(*cb_func)();
	void	*cb_args;
	struct callback	*cb_next;
};

STATIC struct callback *nmihdlrs;
STATIC struct callback *wdhdlrs;

STATIC lock_t callback_lock;
STATIC LKINFO_DECL(callback_lkinfo, "DDI:CB:callback_lock", 0);

/*
 * void 
 * drv_callback(int tag, int (*handler)(), void *args)
 *	Register/Unregister event handlers.
 *
 * Calling/Exit State:
 *	May block, so should called be called with no locks held.
 */
void
drv_callback(int tag, int (*handler)(), void *args)
{
	pl_t	opl;
	struct callback *cbp;
	struct callback	**cbpp;		/* pointer to previous callback
					 * struct in the list
					 */

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	switch(tag) {

	case WATCHDOG_ALIVE_ATTACH:
		cbpp = &wdhdlrs;
		goto attach;

	case NMI_ATTACH:
		cbpp = &nmihdlrs;
attach:
		cbp = kmem_alloc(CALLBACK_SIZE, KM_SLEEP);

		(void) LOCK_PLMIN(&callback_lock);

		cbp->cb_func = handler;
		cbp->cb_args = args;

		/*
		 * Prepend the handler to the list.
		 */
		cbp->cb_next = *cbpp;
		*cbpp = cbp;

		UNLOCK_PLMIN(&callback_lock, PLBASE);
		return;

	case WATCHDOG_ALIVE_DETACH:
		cbpp = &wdhdlrs;
		goto detach;

	case NMI_DETACH:
		cbpp = &nmihdlrs;
detach:
		(void) LOCK_PLMIN(&callback_lock);

		for (; cbp = *cbpp; cbpp = &cbp->cb_next) {

			if (cbp->cb_func == handler && cbp->cb_args == args) {
				*cbpp = cbp->cb_next;
				UNLOCK_PLMIN(&callback_lock, PLBASE);
				kmem_free(cbp, CALLBACK_SIZE);
				return;
			}
		}

		UNLOCK_PLMIN(&callback_lock, PLBASE);
		return;

	default:
		break;

	}; /* end switch */

	return;
}

STATIC boolean_t nmi_shutdown;

/*
 * void 
 * _nmi_hook(int *r0ptr)
 *
 * Calling/Exit State:
 *      The arguments are the saved registers which will be restored
 *      on return from this routine.
 *
 * Description:
 *      Currently, NMIs are presented to the processor in these situations:
 *
 *		- [Software NMI] 
 *		- [Access error from access to reserved processor
 *			LOCAL address]
 *		- Access error:
 *			- Bus Timeout
 *			- ECC Uncorrectable error
 *			- Parity error from System Memory
 *			- Assertion of IOCHK# (only expansion board assert this)
 *			- Fail-safe Timer Timeout
 *		- [Cache Parity error (these hold the processor & freeze
 *                                      the bus)]
 */
void
_nmi_hook(int *r0ptr)
{
	int	status;
	uint_t	fcflags;
	struct callback	*nmicb;
	boolean_t recognized = B_FALSE;

	/*
	 * Save the current fault-catch flags, and disable fault-catching
	 * during this routine.  Note that we may get here before it's safe
	 * to access "u.", but then it would be too early for printf's to work,
	 * so it doesn't matter how we panic, and--except for NMI_BENIGN,
	 * which is unlikely to happen during early initialization--we're
	 * going to panic anyway.
	 */
	fcflags = u.u_fault_catch.fc_flags;
	u.u_fault_catch.fc_flags = 0;

	/*
	 * We're playing fast-and-loose with locking in cmn_err calls below.
	 * Disable locktest checks temporarily.
	 */
	++l.disable_locktest;

	for (nmicb = nmihdlrs; nmicb != NULL; nmicb = nmicb->cb_next) {

		/*
		 * Call the registered NMI handler.
		 */
		status = (*nmicb->cb_func)(nmicb->cb_args);

		switch (status) {
		case NMI_BUS_TIMEOUT:
			if ((fcflags & (CATCH_BUS_TIMEOUT|CATCH_ALL_FAULTS))
			     && !was_servicing_interrupt()) {
				u.u_fault_catch.fc_flags = 0;
				u.u_fault_catch.fc_errno = EFAULT;
				r0ptr[T_EIP] = (int)u.u_fault_catch.fc_func;
				--l.disable_locktest;
				return;
			} else {
				/*
				 *+ Fatal Bus Timeout NMI error.
				 */
				cmn_err(CE_PANIC, "Bus Timeout NMI error");
			}
			/* NOTREACHED */

		case NMI_FATAL:
			cmn_err(CE_PANIC, "Fatal NMI Interrupt");
			/* NOTREACHED */

		case NMI_REBOOT:
			nmi_shutdown = B_TRUE;
			/* FALLTHROUGH */

		case NMI_BENIGN:
			recognized = B_TRUE;
			break;

		case NMI_UNKNOWN:
		default:
			break;

		}; /* end case */

	} /* end for */

	if (!recognized) {
		/*
		 *+ A Non-Maskable Interrupt was generated by an 
		 *+ unknown source. 
		 */
		cmn_err(CE_PANIC, "An unknown NMI interrupt occurred.");
		/* NOTREACHED */
	}

	/* Restore fault-catch flags. */
	u.u_fault_catch.fc_flags = fcflags;
	--l.disable_locktest;
}

/*
 * void
 * nmi_poll(void *arg)
 *	Periodic timer for NMI support.
 *
 * Calling/Exit State:
 *	Called as a timeout routine with no locks held.
 */
/* ARGSUSED */
void
nmi_poll(void *arg)
{
	if (nmi_shutdown) {
		drv_shutdown(SD_SOFT, AD_BOOT);
		nmi_shutdown = B_FALSE;
	}
}

/*
 * void
 * _watchdog_hook(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
_watchdog_hook(void)
{
	struct callback	*wdcb;

	for (wdcb = wdhdlrs; wdcb != NULL; wdcb = wdcb->cb_next)
		(void) (*wdcb->cb_func)(wdcb->cb_args);
}

/*
 * void
 * ddi_init_f(void)
 *
 * Calling/Exit State:
 *      none
 */
void
ddi_init_f(void)
{
	LOCK_INIT(&callback_lock, CALLBACK_HIER, PLMIN, &callback_lkinfo, 0);
	if (itimeout(nmi_poll, NULL, HZ | TO_PERIODIC, PLTIMEOUT) == 0) {
		/*
		 *+ Insufficient resources to start a timer during startup.
		 */
		cmn_err(CE_PANIC, "ddi_init_f: can't start nmi_poll timer");
		/* NOTREACHED */
	}
}

