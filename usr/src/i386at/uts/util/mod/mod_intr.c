/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:util/mod/mod_intr.c	1.31"
#ident	"$Header: $"

#include <mem/kmem.h>
#include <svc/bootinfo.h>
#include <svc/errno.h>
#include <svc/pic.h>
#include <svc/psm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/mod/mod_intr.h>
#include <util/mod/moddrv.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define	MOD_MAX_INTR	(NPIC * 8)

STATIC struct mod_shr_v *mod_shr_ivect[MOD_MAX_INTR];
int intr_bindcpu[MOD_MAX_INTR];
uint_t intr_upcount[MOD_MAX_INTR];

int int_itype[MOD_MAX_INTR];

extern	lock_t	mod_iv_lock;

extern	int	default_bindcpu;

extern	void	intnull();
extern	void	(*ivect[])();

extern	uchar_t	intpri[];
extern	pl_t	svcpri[];
extern	int	intcpu[];
extern	int	intmp[];

int mod_add_intr(struct intr_info *, void (*)());
int mod_remove_intr(struct intr_info *, void (*)());

#define IV_LOCK()		LOCK(&mod_iv_lock, PLIV)
#define IV_UNLOCK()		UNLOCK(&mod_iv_lock, PLBASE)

/*
 * void mod_shr_intn(int iv)
 * 	Shared interrupt routine called through ivect[].
 * 	Supports the dynamic addition and deletion of
 * 	shared interrupts, required by the dynamically
 * 	loadable module feature.
 *
 * Calling/Exit State:
 *	The argument iv gives the index into ivect[].
 *	No lock should be held when enter this routine.
 *
 * Remarks:
 *	The comparison of sihp with last prevents the same interrupt
 *	handler from being called multiple times in a row, which would
 *	incur a performance penalty.  If a driver has attached itself
 *	multiple times, the interrupt handler pointers will generally
 *	be adjacent since all of the driver's instances will be
 *	initialized in sequence.
 */
void
mod_shr_intn(int iv)
{
	struct	mod_shr_v	*svp;
	void	(**sihp)();
	void	(*last)() = NULL;

	for (svp = mod_shr_ivect[iv]; svp; svp = svp->msv_next) {
		for (sihp = svp->msv_sih; *sihp; sihp++) {
			if (*sihp != intnull && last != *sihp) {
				(**sihp)(iv);
				last = *sihp;
			}
		}
	}
}

/*
 * int mod_drvattach(struct mod_drvintr *aip)
 * 	Install and enable all interrupts required by a
 * 	given device driver.
 *
 * Calling/Exit State:
 *	Called from the _load routine of loadable modules.
 *	No locks held upon calling and exit.
 */
void
mod_drvattach(struct mod_drvintr *aip)
{
	struct	intr_info	*i_infop;
	struct	intr_info	ii;
	void	(*hndlr)();

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_drvattach()\n"));

	if (!aip)
		return;

	if (aip->di_magic == MOD_INTR_MAGIC) {
		if (aip->di_version != MOD_INTR_VER)
			return;
		cm_intr_attach_all(aip->di_modname, aip->di_handler,
				   aip->di_devflagp, &aip->di_hook);
		return;
	}

	/*
	 * Convert old-style intr_info to something we can work with.
	 */
	if (!(i_infop = ((struct o_mod_drvintr *)aip)->drv_intrinfo))
		return;

	moddebug(cmn_err(CE_CONT, "!MOD:    Interrupts:\n"));

	hndlr = ((struct o_mod_drvintr *)aip)->ihndler;

	while (i_infop->ivect_no >= 0) {
		moddebug(cmn_err(CE_CONT, "!MOD:   %d\n", INTRNO(i_infop)));
		switch (INTRVER(i_infop)) {
		case 0:
			ii.ivect_no = ((struct intr_info0 *)i_infop)->ivect_no;
			ii.int_pri = ((struct intr_info0 *)i_infop)->int_pri;
			ii.itype = ((struct intr_info0 *)i_infop)->itype;
			ii.int_cpu = -1;
			ii.int_mp = 0;
			i_infop = (struct intr_info *)
					((struct intr_info0 *)i_infop + 1);
			break;
		case MOD_INTRVER_42:
			ii = *i_infop++;
			ii.ivect_no &= ~MOD_INTRVER_MASK;
			break;
		}

		if (ii.itype == 0 || ii.ivect_no == 0)
			continue;

		mod_add_intr(&ii, hndlr);
	}

	return;
}

/*
 * int mod_drvdetach(struct mod_drvintr *aip)
 *	 Remove and disable all interrupts used by a given device driver.
 *
 * Calling/Exit State:
 *	Called from the _unload routine of loadable modules.
 *	No locks held upon calling and exit.
 */
void
mod_drvdetach(struct mod_drvintr *aip)
{
	struct	intr_info	*i_infop;
	struct	intr_info	ii;
	void	(*hndlr)();

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	moddebug(cmn_err(CE_CONT, "!MOD: mod_drvdetach()\n"));

	if (!aip)
		return;

	if (aip->di_magic == MOD_INTR_MAGIC) {
		if (aip->di_version != MOD_INTR_VER)
			return;
		cm_intr_detach_all(aip->di_hook);
		return;
	}

	/*
	 * Convert old-style intr_info to something we can work with.
	 */
	if (!(i_infop = ((struct o_mod_drvintr *)aip)->drv_intrinfo))
		return;

	moddebug(cmn_err(CE_CONT, "!MOD:    Interrupts:\n"));

	hndlr = ((struct o_mod_drvintr *)aip)->ihndler;

	while (i_infop->ivect_no >= 0) {
		moddebug(cmn_err(CE_CONT, "!MOD:   %d\n", INTRNO(i_infop)));
		switch (INTRVER(i_infop)) {
		case 0:
			ii.ivect_no = ((struct intr_info0 *)i_infop)->ivect_no;
			ii.int_pri = ((struct intr_info0 *)i_infop)->int_pri;
			ii.itype = ((struct intr_info0 *)i_infop)->itype;
			ii.int_cpu = -1;
			ii.int_mp = 0;
			i_infop = (struct intr_info *)
					((struct intr_info0 *)i_infop + 1);
			break;
		case MOD_INTRVER_42:
			ii = *i_infop++;
			ii.ivect_no &= ~MOD_INTRVER_MASK;
			break;
		}

		if (ii.itype == 0 || ii.ivect_no == 0)
			continue;

		mod_remove_intr(&ii, hndlr);
	}

	return;
}

/*
 * int mod_add_intr(struct intr_info *iip, void (*ihp)())
 *	Add the interrupt handler ihp to the vector defined by iip.
 *
 * Calling/Exit State:
 *	None.
 */
int
mod_add_intr(struct intr_info *iip, void (*ihp)())
{
	uint_t	iv;
	int	cpu, oldcpu;
	void	(**sihp)(), (**ivp)();
	struct	mod_shr_v *sivp, **sivpp, *nsivp;
	extern int nintr;

	iv = iip->ivect_no;
	if (iv >= MOD_MAX_INTR)
		return(EINVAL);

	if (iip->itype == 0)
  		return(EINVAL);

	if (iv == 0)
		return(EBUSY);

	ivp = &ivect[iv];

	moddebug(cmn_err(CE_CONT, "!MOD: mod_add_intr(): %d\n", iv));
	nsivp = (struct mod_shr_v *)kmem_zalloc(sizeof(struct mod_shr_v),
						KM_SLEEP);

	cpu = iip->int_cpu;
	if (cpu == -2)
		cpu = default_bindcpu;

	(void)IV_LOCK();

	if (*ivp == intnull) {
		/*
		 * The interrupt vector is not currently used.
		 * We assume the interrupt is disabled.
		 */
		intpri[iv] = iip->int_pri;
		svcpri[iv] = MIN(iip->int_pri, PLHI);
		int_itype[iv] = iip->itype;
		intr_upcount[iv] = !iip->int_mp;

		if ((intr_bindcpu[iv] = cpu) == -1 && intr_upcount[iv] != 0)
			cpu = 0;

		*ivp = ihp;

		if (iv >= nintr)
			nintr = iv + 1;
		psm_intron(iv, intpri[iv], intcpu[iv] = cpu, 1, iip->itype);

		IV_UNLOCK();
		kmem_free(nsivp, sizeof(struct mod_shr_v));
		return(0);
	}

	if (iip->itype != int_itype[iv] || intpri[iv] != iip->int_pri) {
		IV_UNLOCK();
		kmem_free(nsivp, sizeof(struct mod_shr_v));
		return(EBUSY);
	}

	oldcpu = intr_bindcpu[iv];
	if (oldcpu == -1 && intr_upcount[iv] != 0)
		oldcpu = 0;

	if (cpu != -1) {
		if (intr_bindcpu[iv] != -1 &&
		    intr_bindcpu[iv] != cpu) {
			IV_UNLOCK();
			kmem_free(nsivp, sizeof(struct mod_shr_v));
			return(EBUSY);
		}
		intr_bindcpu[iv] = cpu;
	}

	if (!iip->int_mp)
		intr_upcount[iv]++;

	cpu = intr_bindcpu[iv];
	if (cpu == -1 && intr_upcount[iv] != 0)
		cpu = 0;

	if (cpu != oldcpu) {
		psm_introff(iv, intpri[iv], intcpu[iv] = oldcpu, iip->itype);
		psm_intron(iv, intpri[iv], intcpu[iv] = cpu, 1, iip->itype);
	}

	if (*ivp != mod_shr_intn) {
		/*
		 * The interrupt is shared for the first time.
		 *
		 * Use the pre-allocated memory for the first packet,
		 * assign the 2 interrupt handlers to it, and update
		 * the packet's count.
		 */
		nsivp->msv_sih[0] = *ivp;
		nsivp->msv_sih[1] = ihp;
		nsivp->msv_cnt = 2;
		mod_shr_ivect[iv] = nsivp;

		/*
		 * Point ivect[iv] to the shared interrupt routine.
		 */
		*ivp = mod_shr_intn;
		IV_UNLOCK();
		return(0);
	}

	/*
	 * If we get here, the interrupt vector is shared multiply already.
	 */

	sivpp = &mod_shr_ivect[iv];

	/*
	 * Find an empty slot in the list for the new handler.
	 */
	for (;;) {
		if ((sivp = *sivpp) != NULL)	{
			if (sivp->msv_cnt == MOD_NSI)	{
				sivpp = &(sivp->msv_next);
				continue;
			}
			/*
			 * List is not full, we have our slot.
			 */
			kmem_free(nsivp, sizeof(struct mod_shr_v));
			break;
		} else {
			/*
			 * No slots free, allocate a new packet.
			 */
			sivp = nsivp;
			break;
		}
	}

	/*
	 * Add the newly allocated packet to the chain.
	 */
	if (*sivpp == NULL)
		*sivpp = sivp;

	/*
	 * At this point sivp points to a packet with at least
	 * one empty slot.
	 */

	sihp = sivp->msv_sih;

	/*
	 * Find the empty slot in the packet.
	 */
	while (*sihp != NULL && *sihp != intnull)
		sihp++;

	/*
	 * Assign the new handler to the slot and
	 * increment the packet's count.
	 */
	*sihp = ihp;
	sivp->msv_cnt++;

	IV_UNLOCK();
	return(0);
}

/*
 * int mod_remove_intr(struct intr_info *iip, void (*ihp)())
 *	Remove the interrupt handler ihp from the vector defined by iip.
 *
 * Calling/Exit State:
 *	None.
 */
int
mod_remove_intr(struct intr_info *iip, void (*ihp)())
{
	uint_t	iv;
	int	cpu, oldcpu;
	void	(**sihp)(), (**ivp)();
	struct	mod_shr_v *sivp, **sivpp;

	iv = iip->ivect_no;
	if (iv >= MOD_MAX_INTR)
		return(EINVAL);

	if (iip->itype == 0)
  		return(EINVAL);

	ivp = &ivect[iv];

	moddebug(cmn_err(CE_CONT, "!MOD: mod_remove_intr(): %d\n", iv));

	(void)IV_LOCK();

	/*
	 * If the parameters don't match, don't do anything.
	 * This prevents us from detaching an interrupt we didn't attach.
	 */
	if (iip->int_pri != intpri[iv] || iip->itype != int_itype[iv]) {
		IV_UNLOCK();
		return(ENOENT);
	}

	oldcpu = intr_bindcpu[iv];
	if (oldcpu == -1 && intr_upcount[iv] != 0)
		oldcpu = 0;

	if (*ivp != mod_shr_intn) {
		/*
		 * The interrupt is not shared,
		 * reset ivect[].
		 */

		/*
		 * If we didn't find our handler, don't do anything.  This
		 * prevents us from detaching an interrupt we didn't attach.
		 */
		if (*ivp != ihp) {
			IV_UNLOCK();
			return(ENOENT);
		}

		/*
		 * Disable the interrupt vector.
		 */
		psm_introff(iv, intpri[iv], intcpu[iv] = oldcpu, iip->itype);

		*ivp = intnull;

		ASSERT(intr_upcount[iv] == (iip->int_mp ? 0 : 1));

		/*
		 * Do not need to set svcpri because only the PSM
		 * must know about the svcpri and not the base kernel.
		 */
		intpri[iv] = 0;
		int_itype[iv] = 0;
		intr_bindcpu[iv] = -1;
		intr_upcount[iv] = 0;
		IV_UNLOCK();
		return(0);
	}

	/*
	 * If we get here, the interrupt vector is shared.
	 */

	/*
	 * Search the packets for the handler.
	 */
	sivpp = &mod_shr_ivect[iv];
	for (sivp = *sivpp; sivp; sivp = sivp->msv_next) {
		for (sihp = sivp->msv_sih; *sihp; sihp++) {
			if (*sihp == ihp)
				break;
		}
		if (*sihp)
			break;
		sivpp = &(sivp->msv_next);
	}

	/*
	 * If we didn't find our handler, don't do anything.  This
	 * prevents us from detaching an interrupt we didn't attach.
	 */
	if (*sihp != ihp) {
		IV_UNLOCK();
		return(ENOENT);
	}

	if (!iip->int_mp)
		intr_upcount[iv]--;

	cpu = intr_bindcpu[iv];
	if (cpu == -1 && intr_upcount[iv] != 0)
		cpu = 0;

	if (cpu != oldcpu) {
		psm_introff(iv, intpri[iv], intcpu[iv] = oldcpu, iip->itype);
		psm_intron(iv, intpri[iv], intcpu[iv] = cpu, 1, iip->itype);
	}

	/*
	 * At this point we should have found the handler.
	 */

	ASSERT(*sihp == ihp);
	ASSERT(sivp != NULL);

	if (--sivp->msv_cnt == 0) {
		/*
		 * If the packet is now empty, free it.
		 */
		*sivpp = sivp->msv_next;
		kmem_free(sivp, sizeof(struct mod_shr_v));
	} else {
		/*
		 * The packet is not empty, just remove the handler.
		 * The pointer is reset to intnull instead of NULL
		 * so it can be distinguish from the end of the handler
		 * list.
		 */
		*sihp = intnull;
	}

	/*
	 * If there is only one interrupt handler left in the list,
	 * call it directly through ivect[].
	 */
	sivp = mod_shr_ivect[iv];
	if (sivp->msv_cnt == 1 && sivp->msv_next == NULL) {

		/*
		 * Find the remaining handler...
		 */
		for (sihp = sivp->msv_sih; *sihp != NULL; sihp++) {
			if (*sihp != intnull)
				break;
		}

		ASSERT(*sihp != NULL);

		*ivp = *sihp;	/* assign it to ivect[] */

		/*
		 * Free the last packet.
		 */
		kmem_free(sivp, sizeof(struct mod_shr_v));
		mod_shr_ivect[iv] = NULL;
	}

	IV_UNLOCK();
	return(0);
}


#ifdef MERGE386

struct asyc_ipl {
	struct asyc_ipl *next;
	int iv;
	int refcnt;
};

static struct asyc_ipl *asyc_ipl_list = NULL;


void
downgrade_asyc_ipl(int iv, void (*ihp)())
{
	struct asyc_ipl *new, *p, *pp;
	struct	mod_shr_v	*svp;
	void	(**sihp)();
	int count = 0, foreign = 0;

	new = (struct asyc_ipl *)kmem_alloc(sizeof(struct asyc_ipl), KM_SLEEP);

	(void)IV_LOCK();

	if (ivect[iv] == mod_shr_intn) {
		for (svp = mod_shr_ivect[iv]; svp; svp = svp->msv_next)
			for (sihp = svp->msv_sih; *sihp; sihp++) {
				if (*sihp != intnull)
				{
					if (*sihp == ihp)
						count++;
					else
						foreign++;
				}
			}
	} else if (ivect[iv] == ihp)
		count++;

	if (count == 0) {
		IV_UNLOCK();
		cmn_err(CE_WARN, "downgrade_async_ipl: no proper handler on IRQ %d, not downgrading\n", iv);
		kmem_free(new, sizeof(struct asyc_ipl));
		return;
	}

	if (foreign)
		cmn_err(CE_WARN, "downgrade_async_ipl: multiple handlers on IRQ %d, downgrading to PLHI anyway\n", iv);

	for (pp = NULL, p = asyc_ipl_list; p; pp = p, p = p->next)
		if (p->iv == iv)
			break;

	if (p) {
		ASSERT(p->refcnt >= 0);
		p->refcnt++;
	} else {
		new->iv = iv;
		new->refcnt = 1;
		new->next = NULL;

		if (pp)
			pp->next = new;
		else
			asyc_ipl_list = new;

		p = new;
		new = NULL;
	}

	if (p->refcnt == 1) {
		ASSERT(intpri[iv] == PLMAX);
		psm_introff(iv, intpri[iv], intcpu[iv], int_itype[iv]);
		intpri[iv] = PLHI;
		psm_intron(iv, intpri[iv], intcpu[iv], 1, int_itype[iv]);
	}

	IV_UNLOCK();

	if (new)
		kmem_free(new, sizeof(struct asyc_ipl));
}


void
restore_asyc_ipl(int iv, void (*ihp)())
{
	struct asyc_ipl *p;
	struct	mod_shr_v	*svp;
	void	(**sihp)();

	(void)IV_LOCK();

	if (ivect[iv] == mod_shr_intn) {
		for (svp = mod_shr_ivect[iv]; svp; svp = svp->msv_next)
			for (sihp = svp->msv_sih; *sihp; sihp++)
				if (*sihp != intnull && ihp != *sihp) {
					IV_UNLOCK();
					cmn_err(CE_WARN, "restore_asyc_ipl: can't restore PLMAX on IRQ %d\n", iv);
					return;		/* failure */
				}
	} else {
		ASSERT(ivect[iv] == ihp);
	}

	for (p = asyc_ipl_list; p; p = p->next)
		if (p->iv == iv)
			break;

	ASSERT(p && p->refcnt > 0);
	p->refcnt--;

	if (p->refcnt == 0) {
		psm_introff(iv, intpri[iv], intcpu[iv], int_itype[iv]);
		intpri[iv] = PLMAX;
		psm_intron(iv, intpri[iv], intcpu[iv], 1, int_itype[iv]);
	}

	IV_UNLOCK();
}
#endif

