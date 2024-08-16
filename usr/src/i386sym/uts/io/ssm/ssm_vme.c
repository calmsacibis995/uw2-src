/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/ssm/ssm_vme.c	1.6"

/*
 * ssm_vme.c
 *	Systems Services Module (SSM) driver code for VME support.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/cmn_err.h>
#include <util/boot.h>
#include <util/debug.h>
#include <util/map.h>
#include <util/ksynch.h>
#include <io/cfg.h>
#include <io/ssm/ssm.h>
#include <io/ssm/ssm_vme.h>
#include <io/ssm/ssm_misc.h>
#include <io/slic.h>
#include <io/intctl.h>
#include <io/conf.h>
#include <io/slicreg.h>
#include <svc/systm.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <io/ddi.h>

/*
 * The following lock structure and linked
 * list are used to detect attempts to allocate
 * S-to-V mappings that occupy the same VME
 * address space/range.  Such mappings must be
 * prevented so drivers don't accidentally
 * impose upon one another.
 */
STATIC struct ssm_vme_mdesc *s2v_mappings = NULL;
#define S2V_LOCK_HEIR    32               /* Hierarchy for s2v_lock */
STATIC LKINFO_DECL(s2v_lkinfo, "SSM/VMEbus StoV map allocation lock", 0);
STATIC lock_t *s2v_lock;		/* Allocate in ssm_vme_init() for
					 * use in StoV map overlap detection */
/*
 * SSM/VME Map rams for A16 space have a special requirement
 * to compensate for unused, but floating, hardware address
 * lines: for each mapping desired, 16 must actually be allocated 
 * and mapped.  Since the maps in the group are not physically
 * contiguous, allocate them as a group such that one index 
 * corresponds to the group.  Do this such that only the VME init
 * and mapping functions actually are aware this is happening.
 */
#define A16_MAP_FACTOR	16		/* # of A16 maps required per mapping */
#define A16_FLOAT_BIT	(1 << 7)	/* Incr for floating A16 map ram bits */

/*
 * void
 * ssm_vme_init(struct ssm_desc *)
 *	Initialize the VME bus for the specified system services module.  
 *
 * Calling/Exit State:
 *	No locking assumed.  Assumes that only one processor is running,
 *	since this is called at system startup time.
 *
 *	the SSM specified is alive, the address of its system 
 *	table of contents descriptor must be in ssm->ssm_cd.
 *
 * 	If the specified SSM has a usable VME bus:
 *		it's VME physical address range on the Sequent 
 *		bus is determined and programmed into its PIC.
 *
 *		ssm->ssm_vme_alive is set to non-zero.
 *	
 *		the VME's mapping registers are memory mapped into
 *		the virtual address space.
 *
 *		allocation resources for the VME's mapping registers 
 *		and interrupt LEVEL/VECTOR assignments are established.
 *	Otherwise, ssm->vme_alive is set to zero.
 *
 *	There is no return value.
 *
 * Remarks:
 *	VMEbus monitoring of BUSERR and SYSFAIL is not enabled until
 *	ssm_vme_init() is invoked, since SSM firmware only supports
 *	enabling of them once and no disable interface exists. This
 *	requires all VMEbus activity which may drive those lines to
 *	occur in driver init() routines.  The SSM firmware may be
 *	enhanced in the future to allow selective enable/disable of
 *	this monitoring.
 */
void
ssm_vme_init(struct ssm_desc *ssm)
{
	register const struct ctlr_desc *cd = ssm->ssm_cd;
	int ssm_index = ssm - SSM_desc;
	paddr_t ssm_paddr, physaddr;
	int i, nmaps;
	ulong *comparitor;

	if (s2v_lock == NULL) {
		/* Perform one-shot allocation for this structure */
		s2v_lock = LOCK_ALLOC(S2V_LOCK_HEIR, plhi, 
					&s2v_lkinfo, KM_NOSLEEP);
		if (s2v_lock == NULL) {
			/*
			 *+ An attempt to allocate a lock for synchonizing
			 *+ access to the SSM/VMEbus has failed, most likely
			 *+ due to a low free memory condition.  The SSM/
			 *+ VMEbus is being deconfigured as a result. Possible
			 *+ corrective action is to add memory to this system
			 *+ or reconfigure its kernel to reduce its consumption.
			 */
			cmn_err(CE_WARN, "VME being deconfigured due a");
			cmn_err(CE_CONT, "lock allocation failure.\n");
			return;
		}
	}

	ssm->ssm_vme_alive = 0;	/* Until determined to be true... */
	if (! SSM_VME_EXISTS(ssm))
		return;		/* The VME bus expansion is not present. */

        if (SSM_BAD_PIC(ssm)) {
		/*
		 *+ An obsolete PICw is present on the SSM2,
		 *+ rendering its VME bus unusable.  Call 
		 *+ service to have SSM2 upgraded. 
		 */
		cmn_err(CE_WARN, "VME being deconfigured due to presence ");
		cmn_err(CE_CONT, "of an obsolete PICw on SSM2\n");
		return;
	}

	/*
	 * Determine physical address range on the 
	 * Sequent bus for this SSM's VME and program
	 * its responder address into its PIC.  The 
	 * method for programming the PIC responder address 
	 * depends on if the board is an SSM or an SSM2.
	 * Also memory map its PIC flush register and read
	 * the value of the SSM's comparitor register for 
	 * use later on.
	 */ 
        if (ssm_index >= ssmvme_phys_addr_cnt) { 
		/* 
		 *+ There are not enough corresponding entries configured
		 *+ in the SSM VME physical address table.  As a result,
		 *+ this SSM board's VME had been deconfigured.  To correct
		 *+ this, generate additional physcial address entries in
		 *+ the table "ssmvme_phys_addr" located in the SSM module'
		 *+ space.c file, and then rebuild/replace the kernel.
		 */
		cmn_err(CE_WARN, "VME deconfigured - address table exceeded\n");
        	SSM_PIC_SET(ssm->ssm_slicaddr, 0U); /* Clear former addr */
		return;
	}
	ssm_paddr = PA_SSMVME(ssm_index) & SSM_VME_PIC_MASK;

	if (cd->cd_type == SLB_SSMBOARD) {
		physaddr = PIC_REGISTER_ADDR(ssm_paddr, PIC_BCR_NARROW);
        	SSM_PIC_SET(ssm->ssm_slicaddr, ssm_paddr);
	} else {
		physaddr = PIC_REGISTER_ADDR(ssm_paddr, PIC_BCR_WIDE);
		ssm_set_vme_mem_window(ssm->ssm_mc, ssm->ssm_slicaddr, 
					SSMVME_WINDOW(ssm_index));
	}
	ssm->ssm_pic_flush_reg = (ulong *)
		/* LINTED */
		physmap(physaddr, sizeof(ulong), KM_NOSLEEP);
	ASSERT(ssm->ssm_pic_flush_reg != NULL);

	comparitor = (ulong *)
		/* LINTED */
		physmap(ssm_paddr, sizeof(ulong), KM_NOSLEEP);
	ASSERT(comparitor != NULL);
	ssm->ssm_vme_compar = *comparitor & SSM_VME_COMP_MASK;
	physmap_free((caddr_t)comparitor, sizeof (ulong), 0);

	/*
	 * Map the VME's mapping rams into the kernel's virtual 
	 * address space, so they may be programmed. Also,
 	 * initialize resources for allocation of the VME's 
	 * mapping rams.  First do the 4 sets of VtoS maps 
	 * (a16, a24, a32lo, a32hi), then do the StoVmaps. 
	 *
 	 * In order to compensate for unused, floating hardware 
	 * address lines in A16 V-to-S transfers, each mapping
	 * must actually map A16_MAP_FACTOR map rams.  Treat
	 * them as a group, so only this function and the mapping
	 * functions are aware of this correlation.
	 */
	for (i = 0; i < 4; i++) {
		physaddr = ssm_paddr | 1 << 16 | i << 14;
		ssm->ssm_vme_map[i].map_ram = (volatile ulong *)
			physmap(physaddr, SSM_VME_NMAPS * sizeof(ulong), 
				/* LINTED */
				KM_NOSLEEP);
		if (ssm->ssm_vme_map[i].map_ram == NULL) {
			/*
			 *+ An attempt to allocate a virtual address space for
			 *+ access to the SSM/VMEbus V-to-S map rams has failed.
			 *+ The SSM/VMEbus has been deconfigured as a result.
			 */
			cmn_err(CE_WARN, "VME being deconfigured due a");
			cmn_err(CE_CONT, "physmap() failure.\n");
			return;
		}
		nmaps = SSM_VME_NMAPS;
		if (i == VME_A16_SPACE)
			nmaps /= A16_MAP_FACTOR;
		ssm->ssm_vme_map[i].nmap_rams = nmaps;
		ssm->ssm_vme_map[i].rmap = rmallocmap(nmaps);
		if (ssm->ssm_vme_map[i].rmap == NULL) {
			/*
			 *+ An attempt to allocate a resource allocation map for
			 *+ the SSM/VMEbus has failed.  The SSM/VMEbus has 
			 *+ been deconfigured as a result.  Possible corrective
			 *+  action is to add memory to this system or 
			 *+ reconfigure its kernel to reduce its consumption.
			 */
			cmn_err(CE_WARN, "VME being deconfigured due a");
			cmn_err(CE_CONT, "rmallocmap() failure.\n");
			return;
		}
		rmfree(ssm->ssm_vme_map[i].rmap, nmaps, 1);
	}
	physaddr = (paddr_t) &((ulong *)(ssm_paddr | 2 << 16))[SSM_S2V_1ST_MAP];
	ssm->ssm_vme_map[SSM_VME_S2V_MAPS].map_ram = (volatile ulong *) 
		physmap(physaddr, SSM_S2V_USABLE_MAPS * sizeof(ulong), 
			/* LINTED */
			KM_NOSLEEP);
	if (ssm->ssm_vme_map[SSM_VME_S2V_MAPS].map_ram == NULL) {
		/*
		 *+ An attempt to allocate a virtual address space for
		 *+ access to the SSM/VMEbus S-to-V map rams has failed.
		 *+ The SSM/VMEbus has been deconfigured as a result.
		 */
		cmn_err(CE_WARN, "VME being deconfigured due a");
		cmn_err(CE_CONT, "physmap() failure.\n");
		return;
	}
	ssm->ssm_vme_map[SSM_VME_S2V_MAPS].nmap_rams = SSM_S2V_USABLE_MAPS;
	ssm->ssm_vme_map[SSM_VME_S2V_MAPS].rmap = 
		rmallocmap(SSM_S2V_USABLE_MAPS);
	if (ssm->ssm_vme_map[SSM_VME_S2V_MAPS].rmap == NULL) {
		/*
		 *+ An attempt to allocate a resource allocation map for
		 *+ the SSM/VMEbus has failed.  The SSM/VMEbus has 
		 *+ been deconfigured as a result.  Possible corrective
		 *+  action is to add memory to this system or 
		 *+ reconfigure its kernel to reduce its consumption.
		 */
		cmn_err(CE_WARN, "VME being deconfigured due a");
		cmn_err(CE_CONT, "rmallocmap() failure.\n");
		return;
	}
	rmfree(ssm->ssm_vme_map[SSM_VME_S2V_MAPS].rmap, SSM_S2V_USABLE_MAPS, 1);

	/*
	 * Initialize resources for interrupt LEVEL/VECTOR assignments, 
	 * and their mapping back to SLIC interrupts.
	 */ 
	ssm_clr_vme_imap(ssm->ssm_mc, ssm->ssm_slicaddr);
	
	ssm->ssm_vme_paddr = ssm_paddr;
	ssm->ssm_vme_alive = 1;		/* Initialization complete */
}

/*
 * void
 * ssm_vme_start(struct ssm_desc *)
 *	Complete initialization of the VME bus for the 
 *	specified system services module.  
 *
 * Calling/Exit State:
 *	No locking assumed; only one processor is running
 *	when called at system startup time, after system
 *	services are available and interrupts enabled.
 *
 *	The SSM specified is alive, the address of its system 
 *	table of contents descriptor must be in ssm->ssm_cd, 
 *	and ssm->ssm_vme_alive is set to non-zero if its VME
 *	is available.
 *
 *	There is no return value.
 *
 * Description:
 * 	If the specified SSM/VMEbus is available, then
 *	enable VMEbus monitoring of SYSFAIL and BUSERR
 *	to report catastrophic errors on that bus.  
 *
 * Remarks:
 *	A enhanced facility for this would be to enbable monitoring
 *	early in ssm_vme_init() and then temporarily disable it
 *	as each device probe on the VMEbus occurs.  Such a mechanism
 *	will be required for dynamically loadable drivers, for which
 *	SSM firmware support is currently lacking.  The shortcoming
 *	with the current mechanism is that a board reset that occurs
 *	after this time that drives SYSFAIL or BUSERR will cause an
 *	NMI reporting by the SSM.
 */
void
ssm_vme_start(struct ssm_desc *ssm)
{
	if (ssm->ssm_vme_alive != 0)
 		ssm_vme_imap_ready(ssm->ssm_mc, ssm->ssm_slicaddr);
}

/*
 * STATIC void
 * ssm_vme_nmi(void)
 *	NMI handler during VME device probe.
 *
 * Calling/Exit State:
 *	ssm_vme_probe() must have installed an ssm index 
 *	into l.nmi_arg and performed a setjmp() with 
 *	l.nmi_label, and gone to splhi() so as to not
 *	relinquish the processor. 
 *
 *	An NMI access error has occurred on the Sequent bus
 *	for the processor upon which this is running, which 
 *	will be cleared.
 *
 *	If the NMI is due to an unexpected bus timeout, then
 *	deconfigure the associated SSM/VME.
 *
 *	Upon exit, longjmp back to ssm_vme_probe(), via
 *	l.nmi_label.
 *
 * Remarks:
 * 	This is entered directly from HW vector table. 
 *	However, it expects the reason to be due to an
 *	an invalid access error from a driver's probe()
 *	procedure.  ssm_vme_probe() already considers scratch 
 *	registers as volatile (thus, no need to save/restore).
 */
STATIC void
ssm_vme_nmi(void)
{
	int ssm_index = (int)l.nmi_arg;

	/*
	 * Clear the nmi and determine if it was due to 
	 * a Sequent Bus Timeout, which is not what we
	 * are expecting. 
	 */
	if (clearnmi() != 0) {
		/*
		 *+ The VMEbus is not functioning correctly.  An attempt
		 *+ to access it resulted in an unexpected  Sequent bus 
		 *+ timeout.  It has been deconfigured.  Contact service 
		 *+ to correct it.
		 */
		cmn_err(CE_WARN,
			"ssm%d: VMEbus deconfigured due to a bus timeout",
			ssm_index);
		SSM_desc[ssm_index].ssm_vme_alive = 0;
	}

	/*
	 * "return" to ssm_vme_probe().
	 */
	longjmp(&l.nmi_label);
}

/*
 * int
 * ssm_vme_probe(int, int (* )(), void *)
 * 	Probe a vme device located on the specified SSM.
 *
 * Calling/Exit State:
 *	The caller must own the processor local nmi_handler,
 *	its argument, and its label_t.
 *
 *	This function is not reentrant.
 *
 *	Returns zero if the specified SSM or its VME
 *	bus are invalid or unusable, or if the driver's 
 * 	attempt to probe causes an NMI - access error.  
 *
 *	Otherwise, passes through the return value from 
 *	the driver's probe function.  Returns non-zero if
 *	the specified driver probe function is NULL.
 *
 * Description:
 *	Verifies that the SSM specified by the caller is
 *	valid and its VME bus is usable.  If the driver's
 *	probe function address is NULL, return non-zero.
 *	Otherwise, go to splhi() and install an NMI handler 
 *	in the event the device to be probed is not present 
 *	and probing attempts to access it result in an NMI.
 *	Then invoke the specified driver probe function with
 *	the argument provided.
 */
int
ssm_vme_probe(int ssm_index, int (*driver_probe)(), void *arg)
{
	struct ssm_desc *ssm = SSM_desc + ssm_index;
	pl_t pl;
	int ret_val;

	if (!SSM_EXISTS(ssm_index) || !ssm->ssm_vme_alive)
		return (0);

	if (! driver_probe)
		return (1);
	
	/*
	 * Temporarily take over the processor local NMI handler
	 * after going to splhi().  Call setjump() to install a
	 * return in case the driver probe results in an NMI.
 	 * setjmp returns non-zero if we are making an NMI return.
	 */
	pl = splhi();
	l.nmi_handler = ssm_vme_nmi;
	l.nmi_arg = ssm_index;
	if (setjmp(&l.nmi_label)) {
		l.nmi_handler = NULL;
		l.nmi_arg = NULL;
		(void)splx(pl);
		return (0);
	}

	ret_val = (*driver_probe)(arg);
	l.nmi_handler = NULL;
	l.nmi_arg = NULL;
	(void)splx(pl);
	return (ret_val);
}

/*
 * void *
 * ssm_vme_mdesc_alloc(int, ushort, int)
 *	Allocate a SSM VME map ram allocation descriptor.
 *
 * Calling/Exit State:
 *	'ssm_index' must reference a valid SSM VMEbus.
 *
 *	Returns NULL if the modifier's address space is not 
 *	valid (a16, a24, or a32 space).
 *
 *	'flag' must be either KM_SLEEP or KM_NOSLEEP.
 *	When flag is KM_SLEEP:
 *
 *		Basic locks may not be held upon entry/exit, but
 *		sleep locks may be held.
 *
 *		Normally, returns the address of an SSM VME map 
 *		ram allocation descriptor.
 *
 *	When flag is KM_NOSLEEP: 
 *
 *		The caller may hold locks, sleep_looks, etc. upon entry.
 *
 *		Returns the address of an SSM VME map ram 
 *		allocation descriptor if immediately available; 
 *		returns NULL otherwise.
 *
 * Description:
 *	If the SSM index is valid and a struct ssm_vme_mdesc 
 *	is allocated by kmem_alloc(), record the associated SSM's 
 *	descriptor address and the modifier within it for later 
 *	use.  Then return the address of the ssm_vme_mdesc to the 
 *	caller as a "void *" since they only need a handle to it 
 *	and should not otherwise come to depend upon fields within
 *	it or its size.
 *
 *	This functionality is disjoint from the functions that
 *	otherwise use the descriptor, so that drivers may optimize
 *	their allocation and use of these resources as they see
 *	fit.  However, a fundamental assumption is that they will
 *	always be used for the same SSM VMEbus and with the same
 *	modifier settings.
 *
 *	When the caller no longer needs this map ram descriptor,
 *	they must call ssm_vme_mdesc_free() to release it.
 */
void *
ssm_vme_mdesc_alloc(int ssm_index, ushort modifier, int flag) 
{
	register struct ssm_vme_mdesc *mdesc;

        ASSERT(SSM_EXISTS(ssm_index) && SSM_desc[ssm_index].ssm_vme_alive);
	ASSERT(flag == KM_SLEEP || flag == KM_NOSLEEP);

	switch (VME_SPACE(modifier)) {
	case VME_A16_SPACE:
	case VME_A24_SPACE:
	case VME_A32_SPACE:
		break;
	default:
		/*
 		 *+ It appears that a device driver internel 
		 *+ error has occurred; it is attempting to 
		 *+ specify a non-existant VME address space.
		 */
		cmn_err(CE_WARN, 
			"ssm%d: invalid VME address space", ssm_index);
		return (NULL);
	}
	mdesc = (struct ssm_vme_mdesc *) 
		kmem_alloc(sizeof (struct ssm_vme_mdesc), flag);
	if (mdesc == NULL)
		return (mdesc);		/* Allocation failed */

	/* Initialize the descriptor */
	mdesc->ssmp = SSM_desc + ssm_index;
	mdesc->addmod = modifier;
	mdesc->mapset = SSM_VME_NO_MAPS;

	return ((void *)mdesc);		/* Success */
}

/*
 * void 
 * ssm_vme_mdesc_free(void *)
 *	Release a SSM VME map ram allocation descriptor.
 *
 * Calling/Exit State:
 *	'mdesc' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller.
 *
 *	The caller may hold locks, sleep_looks, etc. upon entry/exit.
 *
 *	There is no return value.
 *
 * Description:
 *	This function is invoked to release resources previously
 *	acquired (and not yet released) by ssm_vme_mdesc_alloc().
 *	The structure is deallocated with kmem_free() once any
 *	SSM VME map rams still allocated to it have been released,
 *	which should not normally be the case.  This can be
 *	determined based on the value of the 'mapset' member of
 *	the descriptor; ssm_vme_release_map sets it to SSM_VME_NO_MAPS
 *	after releasing them.
 */
void
ssm_vme_mdesc_free(void *mdesc) 
{
	if ( ((struct ssm_vme_mdesc *)mdesc)->mapset != SSM_VME_NO_MAPS) 
		ssm_vme_release_map(mdesc);

	kmem_free(mdesc, sizeof (struct ssm_vme_mdesc) );
}

/*
 * void 
 * ssm_vme_release_map(void *)
 *	Release a set of previously allocated SSM VME map rams.
 *
 * Calling/Exit State:
 *	'mdesc' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller.  It is updated here.
 *
 *	'mdesc' must describe a valid set of SSM VME map rams 
 *	still belonging to the caller by and allocated by 
 *	ssm_s2v_map(), ssm_v2s_map(), ssm_v2s_map_bp(), or 
 *	ssm_v2s_reserve().
 *
 *	The caller may hold locks, sleep_looks, etc. upon entry/exit.
 *
 *	For s-to-v maps, mdesc is deleted from linked list of
 *	active mapping requests, used for overlap detection.
 *
 *	There is no return value.
 *
 * Description:
 *	This function is invoked to release SSM VME map rams previously
 *	acquired and not yet released.  The argument to this function
 *	is a map alloction descriptor, from which the specific set of
 *	map rams to release can be determined.  Validate the map ram
 *	set being specified and release them using rmfree() on the
 *	appropriate resource pool, located via the associated SSM desc.
 *	S2V mappings also require that their virtual address space 
 *	mapping be released via physmap_free() and that their descriptor
 *	be deleted from an overlap detection list.
 *
 *	Once the maps have been released, reset the allocation descriptor
 *	to indicate that no maps are now allocated to it, in the event
 *	it is reused.
 *
 * Remarks:
 *	As map rams are released they are not invalidated, leaving 
 *	the posibilty that an erroneous request could result in an 
 *	unexpected access and transfer through them.  Such a request
 *	would have been denied had they been invalidated by clearing
 *	the released map ram's SSM_MAP_HIT bit.  Doing so would have
 *	a potentially adverse performance impact.  Also, even if they
 *	were invalidated here, the same potential problem exists while
 *	they are valid, but not being used for their intended purpose.
 *	So, if the drivers can be trusted, there should be no problem.
 */
void 
ssm_vme_release_map(void *mdesc)
{
	register struct ssm_vme_mdesc *mp = (struct ssm_vme_mdesc *)mdesc;
	struct ssm_vme_mdesc *svp;
	pl_t tplock;

	switch (mp->mapset) {
	case SSM_VME_S2V_MAPS:
		/* 
		 * Cycle through the linked list of active s-to-v 
	 	 * mappings and delete this mapping from it, as
	 	 * it is no longer in use.  Then release its associated
		 * address mapping.
	 	 */
		tplock = LOCK(s2v_lock, plhi);
		if (s2v_mappings == mp) {
			s2v_mappings = mp->next;
		} else {
			svp = s2v_mappings; 
			for ( ; svp->next != mp; svp = svp->next)
				continue;
			svp->next = mp->next;
		}
		UNLOCK(s2v_lock, tplock);
		physmap_free(mp->va, mp->nbytes, 0);

		/* Fall through and perform deallocation as for other mapsets */

		/*LINTED*/
	case SSM_VME_A16_MAPS:
	case SSM_VME_A24_MAPS:
	case SSM_VME_A32_LO_MAPS:
	case SSM_VME_A32_HI_MAPS:
		rmfree(mp->ssmp->ssm_vme_map[mp->mapset].rmap, 
			mp->nmaps, mp->firstmap + 1);
		mp->mapset = SSM_VME_NO_MAPS;
		break;
	case SSM_VME_NO_MAPS:
		/*
 		 *+ It appears that a device driver internel 
		 *+ error has occurred; it is attempting to 
		 *+ release SSM VMEbus resources that appear 
		 *+ to have once belonged to it, but which
		 *+ have already been relinquished.
		 */
		cmn_err(CE_WARN, "ssm%d: attempt to release free mapping");
		return;
	default:
		/*
 		 *+ It appears that a device driver internel 
		 *+ error has occurred; it is attempting to 
		 *+ release SSM VMEbus resources that appear 
		 *+ not to belong to it.
		 */
		cmn_err(CE_PANIC, "ssm%d: attempt to release invalid mapping");
	}
}

/*
 * caddr_t
 * ssm_s2v_map(void *, paddr_t, uint, int)
 *	Attempt to memory map the specied VME address space so 
 *	that it may be accessed by Sequent bus initiators.
 *
 * Calling/Exit State:
 *	'mdesc' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller.  There must not be any
 *	map rams currently allocated to it.  It is updated upon
 *	successful completion.
 *
 *	'vme_addr' and 'nbytes' are the base VME address
 *	and amount of the VME address space for which to 
 *	provide the StoV mapping.	
 *
 *	'flag' must be either KM_SLEEP or KM_NOSLEEP.
 *	Basic locks may not be held upon entry/exit
 *	if set to KM_SLEEP; otherwise they may be held.
 *
 *	Sleep locks may be held upon entry/exit.
 *
 *	If successful, mdesc appended to the a linked list of
 *	active mapping requests, used for overlap detection.
 *
 *	Returns the base address of the mapped VME memory's
 *	virtual address space upon success; NULL otherwise.
 *
 * Description:
 *	Determine the number of map rams required to map the 
 *	request, then attempt to allocate a set that size from
 *	the StoV pool of SSM specified in the allocation descriptor.  
 *	The pool is managed via rmalloc() and the maps must be 
 *	contiguous to create a physically contiguous address space.
 *
 *	The physical address space is then mapped to a virtual
 *	contiguous address space using physmap().	
 *
 *	If StoV map rams are not immediately available and 'flag' 
 *	is KM_NOSLEEP, return NULL.  Otherwise, wait for the 
 *	resources to become available.
 *
 *	If this request overlaps an existing s-to-v mapping 
 *	then reject this request, returning NULL.
 *
 *	Call ssm_vme_release_map() to release the mapping when
 *	it is no longer needed.
 */
caddr_t
ssm_s2v_map(void *mdesc, paddr_t vme_addr, uint nbytes, int flag)
{
	register struct ssm_vme_mdesc *mp = (struct ssm_vme_mdesc *)mdesc;
	struct ssm_desc *ssm = mp->ssmp;
	paddr_t physaddr;
	int i;
	volatile ulong *map_ram;
	ushort addmod, rotation;
	struct ssm_vme_mdesc *svp;
	pl_t tplock;

	ASSERT(flag == KM_SLEEP || flag == KM_NOSLEEP);
	ASSERT(mp->mapset == SSM_VME_NO_MAPS);

	/* 
	 * Determine the appropriate number of map rams 
	 * and allocate them.
	 */
	mp->nmaps = howmany((vme_addr & ~SSM_S2V_MAP_MASK) + nbytes,
			    SSM_S2V_MAP_SIZE);
	if (flag == KM_NOSLEEP) {
		mp->firstmap = rmalloc(ssm->ssm_vme_map[SSM_VME_S2V_MAPS].rmap, 
					mp->nmaps);
	} else {
		mp->firstmap = 
			rmalloc_wait(ssm->ssm_vme_map[SSM_VME_S2V_MAPS].rmap, 
					mp->nmaps);
	}
	if (mp->firstmap == 0)
		return (NULL);		/* Allocation failed */
	mp->firstmap--;			/* Adjust for map base */

	/* 
	 * Calculate the associated Sequent bus physical 
	 * address space, then map that into the kernel's
	 * virtual address space.  Note that our firstmap
	 * value is really relative to SSM_S2V_1ST_MAP.
	 */
	physaddr = ssm->ssm_vme_paddr | ((mp->firstmap + SSM_S2V_1ST_MAP) << 14)
			| (vme_addr & ~SSM_S2V_MAP_MASK);

	if ((mp->va = physmap(physaddr, nbytes, flag)) == NULL) {
		rmfree(ssm->ssm_vme_map[SSM_VME_S2V_MAPS].rmap, 
			mp->nmaps, mp->firstmap + 1);
		return (NULL);		/* Virtual addr allocation failed */
	}

	/* 
	 * Cycle through all the active s-to-v mappings and
	 * disallow this request if it overlaps another which
	 * already exists.  Note that this test is done last
	 * so the mapping stays on the s2v_mappings list, instead
	 * of having to back it out prior to exiting this routine.
 	 *
	 * BTW - s-to-v mappings should occur rather infrequently,
	 * so the overhead for testing overlap should not be important
	 * to the overall system performance.
	 */
	tplock = LOCK(s2v_lock, plhi);
	for (svp = s2v_mappings; svp != NULL; svp = svp->next) {
		/*
		 * Reject if another mapping uses the same SSM
		 * and VME address space and any part of the
		 * requested address range (the test for which
		 * is rather messy):
		 *	- does new mapping start in existing mapping?
		 *	- does new mapping end in existing mapping?
		 *	- does new mapping contain the existing mapping?
		 */
		if (VME_SPACE(mp->addmod) == VME_SPACE(svp->addmod)
		&&  ssm == svp->ssmp
		&&  ((vme_addr >= svp->pa && vme_addr < svp->pa + svp->nbytes)
		     || (vme_addr + nbytes > svp->pa 
		           && vme_addr + nbytes <= svp->pa + svp->nbytes)
		     || (svp->pa > vme_addr && svp->pa < vme_addr + nbytes))) {

			/* Overlap detected, back out of related allocations */
			UNLOCK(s2v_lock, tplock);
			rmfree(ssm->ssm_vme_map[SSM_VME_S2V_MAPS].rmap, 
				mp->nmaps, mp->firstmap + 1);
			physmap_free(mp->va, nbytes, 0);
			return (NULL);		/* An overlap was found */
		}
	}
	/* 
	 * No overlap detected, so register this
	 * new mapping in the s2v_mappings list
	 * and resume programming up the map rams,
	 * prior to returning its base virtual.
	 */
	mp->pa = vme_addr;
	mp->nbytes = nbytes;
	mp->mapset = SSM_VME_S2V_MAPS;
	mp->next = s2v_mappings;
	s2v_mappings = mp;
	UNLOCK(s2v_lock, tplock);

	addmod = SSMVME_AMOD(mp->addmod);
	rotation = SSMVME_ROT(mp->addmod);
	map_ram = mp->map_ram = 
		ssm->ssm_vme_map[SSM_VME_S2V_MAPS].map_ram + mp->firstmap;
	vme_addr &= SSM_S2V_MAP_MASK;
	for (i = mp->nmaps; i != 0; i--, map_ram++) {
		*map_ram = vme_addr | (addmod << 8) | 
			(0xf << 4) | (rotation << 1) | SSM_MAP_HIT;
		vme_addr += SSM_S2V_MAP_SIZE;
	}
	return (mp->va);
}

/*
 * STATIC int
 * alloc_v2s_maps(struct ssm_vme_mdesc *, size_t, int)
 *	Common allocator for VtoS map rams. 
 *
 * Calling/Exit State:
 *	'mp' must address a valid SSM VME map ram allocation 
 *	descriptor.  It must not already describe an allocated
 * 	set map rams and will be updated to describe the 
 *	allocated map rams if the allocation succeeds.
 *
 *	'nmaps' is the nonzero number of mappings to allocate.
 *
 *      'flag' must be either KM_SLEEP or KM_NOSLEEP.
 *      When flag is KM_SLEEP basic locks may not be held
 *      upon entry/exit, otherwise they may be held.
 *
 *      Sleep locks may be held upon entry/exit.
 *
 *	Returns zero if mapping rams cannot be acquired 
 *	immediately and 'flag' is KM_NOSLEEP, or if they 
 *	never will be available.  Returns non-zero otherwise.
 *
 * Description:
 *      Attempts to acquire a set of contiguous map from 
 *	the VtoS pool associated with the SSM descriptor 
 *	specified in the allocation descriptor, for the VME 
 *	space also described by the allocation descriptor's 
 *	transfer modifier.  
 *
 *	The VtoS map ram pools are managed via rmalloc() and 
 *	the maps must be contiguous to create a physically 
 *	contiguous VME address space.
 *
 *      For A32 space there are two disjoint sets of map 
 * 	rams from which allocation may occur: A32_HI, A32_LO.  
 *	Try A32_LO first using KM_NOSLEEP.  If that fails then
 *      try A32_HI with the normal wait semantics.
 *
 *	While the maps are being allocated, save time later by
 *	computing both the kernel address of the map rams in
 *	the allocation and the VME bus base address for the
 *	area mapped by those map rams when they are filled out.
 */
STATIC int
alloc_v2s_maps(struct ssm_vme_mdesc *mp, size_t nmaps, int flag)
{
	struct ssm_desc *ssm = mp->ssmp;
	ulong msb, addr_mask;
	ushort mapset;

	ASSERT(flag == KM_SLEEP || flag == KM_NOSLEEP);
	ASSERT(mp->mapset == SSM_VME_NO_MAPS);

	/*
         * Establish bit masks for for the address space
	 * and map pool being used; only A32HI sets msb.
         */
        msb = 0;
        switch (VME_SPACE(mp->addmod)) {
        case VME_A16_SPACE:
		mapset = SSM_VME_A16_MAPS;
                addr_mask = 0x0000ffff;
                break;
        case VME_A24_SPACE:
		mapset = SSM_VME_A24_MAPS;
                addr_mask = 0x00ffffff;
                break;
        case VME_A32_SPACE:
		mapset = SSM_VME_A32_LO_MAPS;
                addr_mask = (ulong)0xffffffff;
                mp->firstmap = rmalloc(ssm->ssm_vme_map[mapset].rmap, nmaps);
        	if (mp->firstmap == 0) {
			mapset = SSM_VME_A32_HI_MAPS;
                	msb = (ulong)0x80000000;
		} else {
			goto maps_reserved;
		}
                break;
        }

        if (flag == KM_NOSLEEP) {
                mp->firstmap = 
			rmalloc(ssm->ssm_vme_map[mapset].rmap, nmaps);
        } else {
                mp->firstmap =
                        rmalloc_wait(ssm->ssm_vme_map[mapset].rmap, nmaps);
        }
        if (mp->firstmap == 0)
                return (0);	/* Allocation failed */
maps_reserved:
	/* 
	 * Save information about the allocation in the map 
	 * descriptor, including the associated base VME bus 
	 * physical address that will be used later.
	 */
        mp->firstmap--;                 	/* Adjust for map base */
	mp->nmaps = nmaps;
	mp->mapset = mapset;
	mp->map_ram = ssm->ssm_vme_map[mapset].map_ram + mp->firstmap;
	mp->vme_base = 
		(msb | ssm->ssm_vme_compar | (mp->firstmap << 11)) & addr_mask;
	return (1);
}

/*
 * STATIC paddr_t
 * program_v2s_maps(struct ssm_vme_mdesc *, caddr_t, struct proc *, size_t)
 *	Fill in the specified SSM VMEbus' VME initiator map
 *	rams starting at the virtual address provided.
 *
 * Calling/Exit State:
 *	'mp' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller. It must also describe
 *	the set of map rams to use for the mapping.
 *
 *	'addr' and 'nmaps' are the base virtual address from 
 *	which to provide the VtoS mapping and the number of	
 *	map rams to program (a subset of those described by mp).
 *
 *	'proc' is the proc addr to use for vtop() for this buffer.
 *	Its NULL when 'addr' is a kernel address, non-NULL if a 
 *	user address.  Its normally obtained from a struct buf or
 *	NULL for streams buffers.
 *
 *	Basic locks and sleep locks may be held upon entry/exit.
 *
 *	Returns the VME bus address to be used by a VME intiator
 *	to transfer data to or from the specified buffer.
 *
 * Remarks:
 * 	The map descriptor already contains the rotational mask 
 *	for programming the map rams, the kernel address of the 
 *	first in a series of map rams to program, and the VMEbus 
 *	base address corresponding to area mapped by them.  Use
 *	them for filling out the map rams and compute the return 
 *	value by masking in the start offset relative to the 
 *	beginning of the area the 1st map ram covers.
 */
STATIC paddr_t
program_v2s_maps(struct ssm_vme_mdesc *mp, caddr_t addr, 
		 struct proc *proc, size_t nmaps)
{
	paddr_t physaddr, ret_val;
	volatile ulong *map_ram;
	ulong j16;
	ushort rotation;
	int i;

	/* Compute the return value now, since addr is changed below */
	ret_val = mp->vme_base | ((ulong)addr & ~SSM_VME_MAP_ADDR_MASK);

	/* 
	 * Align on a map ram boundary, then
	 * fill in the map rams, the
	 * bit patterns for which are from
	 * the SSM hardware functional spec.
	 */
	addr = (caddr_t) ((ulong)addr & SSM_VME_MAP_ADDR_MASK);		
	rotation = SSMVME_ROT(mp->addmod);
	map_ram = mp->map_ram;
	for (i = nmaps; i != 0; i--, map_ram++) {
		physaddr = vtop(addr, proc);
        	if (mp->mapset == SSM_VME_A16_MAPS) {
 	 		/*
			 * In order to compensate for unused, floating hardware 
			 * address lines in A16 V-to-S transfers, each mapping
			 * must actually map A16_MAP_FACTOR map rams.  Treat
			 * them as a group, so only mapping functions and the 
			 * init function are aware of this correlation.
			 */
                        for (j16 = A16_FLOAT_BIT;
			     j16 < (A16_MAP_FACTOR * A16_FLOAT_BIT); 
			     j16 += A16_FLOAT_BIT) {
				*(ulong *)((ulong)map_ram | j16) = physaddr | 
					(0xf << 7) | (rotation << 1) 
					|  SSM_MAP_HIT;

			}
		}
		*map_ram = physaddr | (0xf << 7) | 
				(rotation << 1) |  SSM_MAP_HIT;
		addr += SSM_VME_BYTES_PER_MAP;		/* The next mapping */
	}
	return (ret_val);		/* Return the xfer start addr */
}

/*
 * STATIC paddr_t
 * pageio_program_v2s_maps(struct ssm_vme_mdesc *, struct buf *, size_t)
 *	Fill in the specified SSM VMEbus' VME initiator map
 *	rams for the specified struct buf, using PAGEIO.
 *
 * Calling/Exit State:
 *	'mp' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller. It must also describe
 *	the set of map rams to use for the mapping.
 *
 *	'bp' describes the buffer to be mapped using B_PAGEIO.
 *
 *	'nmaps' is the number of contiguous mappingss to fill in
 *	for this VtoS mapping; a subset of those described by 'mp'.
 *
 *	Basic locks and sleep locks may be held upon entry/exit.
 *
 *	Returns the VME bus address to be used by a VME intiator
 *	to transfer data to or from the specified buffer.
 *
 * Remarks:
 *	This function is very similar to program_v2s_maps().
 *
 * 	The map descriptor already contains the rotational mask 
 *	for programming the map rams, the kernel address of the 
 *	first in a series of map rams to program, and the VMEbus 
 *	base address corresponding to area mapped by them.  Use
 *	them for filling out the map rams and compute the return 
 *	value by masking in the start offset relative to the 
 *	beginning of the area the 1st map ram covers.
 *
 */
STATIC paddr_t
pageio_program_v2s_maps(struct ssm_vme_mdesc *mp, struct buf *bp, size_t nmaps)
{
	paddr_t physaddr;
	volatile ulong *map_ram;
	ulong page_offset, page_size, j16;
	ushort rotation;
	page_t  *page;
	int i;

	ASSERT(bp->b_flags & B_PAGEIO);

	/*
	 * Now walk the page list filling in the map 
	 * rams, bit patterns for which are from the 
	 * SSM hardware functional spec. Align the start 
	 * offset on a map ram boundary for computing 
	 * the physical address that goes into map rams.
	 */
	page_offset = (ulong)bp->b_un.b_addr & SSM_VME_MAP_ADDR_MASK;	
	page_size = ptob(1);
	page = getnextpg(bp, NULL);	/* fetch descriptor for first page */
	physaddr = ((ulong_t)pptophys(page)) + page_offset; /* Mem start addr */
	rotation = SSMVME_ROT(mp->addmod);
	map_ram = mp->map_ram;

	for (i = nmaps; ;map_ram++) {
        	if (mp->mapset == SSM_VME_A16_MAPS) {
 	 		/*
			 * In order to compensate for unused, floating hardware 
			 * address lines in A16 V-to-S transfers, each mapping
			 * must actually map A16_MAP_FACTOR map rams.  Treat
			 * them as a group, so only mapping functions and the 
			 * init function are aware of this correlation.
			 */
                        for (j16 = A16_FLOAT_BIT;
			     j16 < (A16_MAP_FACTOR * A16_FLOAT_BIT); 
			     j16 += A16_FLOAT_BIT) {
				*(ulong *)((ulong)map_ram | j16) = physaddr | 
					(0xf << 7) | (rotation << 1) 
					|  SSM_MAP_HIT;

			}
		}
		*map_ram = physaddr | (0xf << 7) | 
			(rotation << 1) |  SSM_MAP_HIT;

		/* Now do the next mapping */
		if (--i == 0)
			break;		/* avoid extra getnextpg() */
		page_offset += SSM_VME_BYTES_PER_MAP;
		if (page_offset >= page_size) {
			page = getnextpg(bp, page);	/* Go to next page */
			physaddr = (ulong_t)pptophys(page); 
			page_offset = 0;
		} else {
			physaddr += SSM_VME_BYTES_PER_MAP;
		}
	}
	return (mp->vme_base | 
		((ulong)bp->b_un.b_addr & ~SSM_VME_MAP_ADDR_MASK));
}

/*
 * paddr_t
 * ssm_v2s_map(void *, caddr_t, uint, int)
 *	Acquire a set of VtoS map resources and provide
 *	a VtoS mapping for the specified transfer buffer.
 *
 * Calling/Exit State:
 *	'mdesc' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller.  There must not be any
 *	map rams currently allocated to it.  It is updated upon
 *	successful completion.
 *
 *	'addr' and 'nbytes' are the base kernel virtual 
 *	address and size of the buffer for which to provide 
 *	the VtoS mapping.	
 *
 *      Basic locks may not be held upon entry/exit if 'flag'
 *      is KM_SLEEP.  They may be held otherwise.
 *
 *      Sleep locks may be held upon entry/exit.
 *
 *      Returns the buffer mapping's VME intiator address,
 *      upon success.  Returns VME_INVALID_ADDR if mapping
 *      resources cannot be acquired immediately and 'flag'
 *      is KM_NOSLEEP, or if they never can be.
 *
 * Remarks:
 *      Determine the number of map rams required to map the 
 *	request, then invoke alloc_v2s_maps() to aquire them.
 *	The maps form a physically contiguous mapping.
 *
 *	If the maps were available, invoke program_v2s_maps() 
 *	to fill them in.
 *	
 *	Call ssm_vme_release_map() to release the mapping 
 *	when it is no longer needed.
 */
paddr_t
ssm_v2s_map(void *mdesc, caddr_t addr, uint nbytes, int flag)
{
	register struct ssm_vme_mdesc *mp = (struct ssm_vme_mdesc *)mdesc;
	size_t nmaps;

        nmaps = howmany(((ulong)addr & ~SSM_VME_MAP_ADDR_MASK) + nbytes, 
			SSM_VME_BYTES_PER_MAP);

        if (! alloc_v2s_maps(mp, nmaps, flag))
                return (VME_INVALID_ADDR);      /* Allocation failed */

	return (program_v2s_maps(mp, addr, NULL, nmaps));
}

/*
 * paddr_t
 * ssm_v2s_map_bp(void *mdesc, struct buf *bp, int flag)
 *	Acquire a set of VtoS map resources and provide
 *	a VtoS mapping for the specified transfer buf.
 *
 * Calling/Exit State:
 *	'mdesc' must address a SSM VME map ram allocation 
 *	descriptor, allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller.  It is updated
 *	if the allocation is successful.
 *
 *      'bp' describes the buffer to be mapped.
 *
 * 	Basic locks may not be held upon entry/exit if 'flag'
 *	is KM_SLEEP.  They may be held otherwise.
 *
 *      Sleep locks may be held upon entry/exit.
 *
 *	Returns the buffer mapping's VME intiator address,
 *	upon success.  Returns VME_INVALID_ADDR if mapping 
 * 	resources cannot be acquired immediately and 'flag' 
 *	is KM_NOSLEEP, or if they never can be.
 *
 * Description:
 *	Determine the number of map rams required to map the 
 *	request, then invoke alloc_v2s_maps() to acquire them.  
 *	The maps form a physically contiguous mapping.
 *
 *	If the maps were available, invoke another function to 
 *	fill them in.  If the buffer type is B_PAGEIO then use 
 *	pageio_program_v2s_maps(), otherwise use program_v2s_maps().
 *
 *	Call ssm_vme_release_map() to release the mapping when
 *	it is no longer needed.
 */
paddr_t
ssm_v2s_map_bp(void *mdesc, struct buf *bp, int flag)
{
	register struct ssm_vme_mdesc *mp = (struct ssm_vme_mdesc *)mdesc;
	size_t nmaps;

        /*
         * Determine the appropriate map ram set and 
 	 * number of map rams, then allocate them.
         */
        nmaps = howmany(((ulong)bp->b_un.b_addr & ~SSM_VME_MAP_ADDR_MASK)
                        + bp->b_bcount, SSM_VME_BYTES_PER_MAP);

	if (! alloc_v2s_maps(mp, nmaps, flag))
		return (VME_INVALID_ADDR);	/* Allocation failed */

	if ((bp->b_flags & B_PAGEIO) != 0) {
                return (pageio_program_v2s_maps(mp, bp, nmaps));
        } else {
                return (program_v2s_maps(mp, bp->b_un.b_addr, 
					 bp->b_proc, nmaps)  );
        }
}

/*
 * paddr_t
 * ssm_v2s_remap(void *, caddr_t, uint)
 *	Remap a previously acquired set of VtoS map rams
 *	for another VtoS transfer.
 *
 * Calling/Exit State:
 *	'mdesc' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller.  It must also describe 
 *	a valid set of SSM VME map rams to reuse.
 *
 *	'addr' and 'nbytes' are the base kernel virtual 
 *	address and size of the buffer for which to provide 
 *	the VtoS mapping.	
 *
 *	Basic locks and sleep locks may be held upon entry/exit.
 *
 *      Returns the buffer mapping's VME intiator address,
 *      upon success.  Returns VME_INVALID_ADDR if there are 
 *	not already enough VtoS maps reserved for the mapping.
 *
 * Remarks:
 *	After verifying that there are enough map rams for the
 *	mapping, invoke program_v2s_maps() to fill them in.
 *	
 *      Call ssm_vme_release_map() to release the mapping when
 *      it is no longer needed or reuse it via ssm_v2s_remap() 
 *	or ssm_v2s_remap_bp().
 *
 *	Excess map rams in the group passed to this function,
 *	which are not actually remapped are not invalidated, leaving 
 *	the posibilty that an erroneous request could result in an 
 *	unexpected access and transfer through them.  Such a request
 *	would have been denied had they been invalidated by clearing
 *	the released map ram's SSM_MAP_HIT bit.  Doing so would have
 *	a potentially adverse performance impact.  Also, even if they
 *	were invalidated here, the same potential problem exists while
 *	they are valid, but not being used for their intended purpose.
 *	So, if the drivers can be trusted, there should be no problem.
 */
paddr_t
ssm_v2s_remap(void *mdesc, caddr_t addr, uint nbytes)
{
	register struct ssm_vme_mdesc *mp = (struct ssm_vme_mdesc *)mdesc;
	size_t nmaps;

	ASSERT(mp->mapset == SSM_VME_A16_MAPS 
		|| mp->mapset == SSM_VME_A24_MAPS 
		|| mp->mapset == SSM_VME_A32_LO_MAPS 
		|| mp->mapset == SSM_VME_A32_HI_MAPS);

	nmaps = howmany(((ulong)addr & ~SSM_VME_MAP_ADDR_MASK) + nbytes, 
			SSM_VME_BYTES_PER_MAP);
	
	if (nmaps > mp->nmaps) {
		return (VME_INVALID_ADDR);	/* Not enough maps */
	}
	return (program_v2s_maps(mp, addr, NULL, nmaps));
}

/*
 * paddr_t
 * ssm_v2s_remap_bp(void *mdesc, struct buf *bp)
 *	Remap a previously acquired set of VtoS map rams
 *	for another VtoS transfer, using a buf structure.
 *
 * Calling/Exit State:
 *	'mdesc' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller.  It must also describe 
 *	a valid set of SSM VME map rams to reuse.
 *
 *	'bp' describes the buffer to be mapped.
 *
 *	Basic locks and sleep locks may be held upon entry/exit.
 *
 *      Returns the buffer mapping's VME intiator address,
 *      upon success.  Returns VME_INVALID_ADDR if there are 
 *	not already enough VtoS maps reserved for the mapping.
 *
 * Remarks:
 *	After verifying that there are enough map rams for the
 *	mapping, invoke a common function to fill them in.  If
 *	the buffer type is B_PAGEIO use pageio_program_v2s_maps(),
 * 	otherwise use program_v2s_maps().
 *	
 *      Call ssm_vme_release_map() to release the mapping when
 *      it is no longer needed or reuse it via ssm_v2s_remap() 
 *	or ssm_v2s_remap_bp().
 *	
 *	This function is very similar to ssm_v2s_remap().
 *
 *	Excess map rams in the group passed to this function,
 *	which are not actually remapped are not invalidated, leaving 
 *	the posibilty that an erroneous request could result in an 
 *	unexpected access and transfer through them.  Such a request
 *	would have been denied had they been invalidated by clearing
 *	the released map ram's SSM_MAP_HIT bit.  Doing so would have
 *	a potentially adverse performance impact.  Also, even if they
 *	were invalidated here, the same potential problem exists while
 *	they are valid, but not being used for their intended purpose.
 *	So, if the drivers can be trusted, there should be no problem.
 */
paddr_t
ssm_v2s_remap_bp(void *mdesc, struct buf *bp)
{
	register struct ssm_vme_mdesc *mp = (struct ssm_vme_mdesc *)mdesc;
	size_t nmaps;

	ASSERT(mp->mapset == SSM_VME_A16_MAPS 
		|| mp->mapset == SSM_VME_A24_MAPS 
		|| mp->mapset == SSM_VME_A32_LO_MAPS 
		|| mp->mapset == SSM_VME_A32_HI_MAPS);

	nmaps = howmany(((ulong)bp->b_un.b_addr & ~SSM_VME_MAP_ADDR_MASK) 
			+ bp->b_bcount, SSM_VME_BYTES_PER_MAP);

	if (nmaps > mp->nmaps) {
		return (VME_INVALID_ADDR);	/* Not enough maps */
	}

	if ((bp->b_flags & B_PAGEIO) != 0) {
		return (pageio_program_v2s_maps(mp, bp, nmaps));
	} else {
		return (program_v2s_maps(mp, bp->b_un.b_addr, 
					 bp->b_proc, nmaps)  );
	}
}

/*
 * int
 * ssm_v2s_reserve(void *, uint, uint, int)
 *	Reserve a set of VtoS map resources sufficient
 *	to VtoS map a buffer with the specified worst'
 *	case size and alignment characteristics.
 *	
 * Calling/Exit State:
 *	'mdesc' must address a SSM VME map ram allocation 
 *	descriptor, previously allocated by ssm_vme_mdesc_alloc, 
 *	and still owned by the caller.  There must not be any
 *	map rams currently allocated to it.  It is updated upon
 *	successful completion.
 *
 *	'nbytes' and 'align' describe the maximum size buffer 
 *	to be mapped with this allocation and the its worst case
 *	byte alignment, which must be a power of 2.
 *
 *      Basic locks may not be held upon entry/exit if 'flag'
 *      is KM_SLEEP.  They may be held otherwise.
 *
 *      Sleep locks may be held upon entry/exit.
 *
 *	Returns non-zero if sufficient map rams have been located 
 *	and reserved.  Returns zero if mapping rams cannot be 
 *	acquired immediately and 'flag' is KM_NOSLEEP, or if 
 *	they never can be.
 *
 *
 * Remarks:
 *      Determine the worst case number of map rams required, based 
 *	'nbytes' and 'align'. then invoke alloc_v2s_maps() to acquire 
 *	them.  The maps form a physically contiguous mapping.
 *
 *	This routine is intended to be used in conjuction with
 *	ssm_v2s_remap() and ssm_v2s_remap_bp() when the size and
 *	number of transfer buffers a driver requires mapping rams
 *	for is fairly static.  This may provide lower overhead than 
 *	completely dynamic allocation.  
 *
 *      Call ssm_vme_release_map() to release the mapping when
 *      it is no longer needed.
 */
int
ssm_v2s_reserve(void *mdesc, uint nbytes, uint align, int flag)
{
	register struct ssm_vme_mdesc *mp = (struct ssm_vme_mdesc *)mdesc;
	uint worst_offset;
	size_t nmaps;

	ASSERT(flag == KM_SLEEP || flag == KM_NOSLEEP);
	ASSERT(mp->mapset == SSM_VME_NO_MAPS);

	align %= SSM_VME_BYTES_PER_MAP;
	worst_offset = (align == 0) ? 0 : (SSM_VME_BYTES_PER_MAP - align);
        nmaps = howmany(nbytes + worst_offset, SSM_VME_BYTES_PER_MAP);

	return (alloc_v2s_maps(mp, nmaps, flag));
}

/* 
 * void
 * ssm_vme_dma_flush(int)
 *  	Flush/finishup the SSM VMEbus' most recent DMA through 
 *	its buffered data path - the PIC.  
 *
 * Calling/Exit State:
 *	The PIC flush register addess was determined at
 *	by ssm_vme_init() and stored in its SSM descriptor.
 *
 *	Basic and Sleep locks may be held upon entry/exit.
 *	
 *	The PIC may have some remaining DMA or readahead
 *	data buffered from a recent transfer, but will be
 *	flushed to its intended destination, if necessary,
 *	during this operation.
 *
 *	No return value;
 *
 * Description:
 *	This is a time critical operation - it happens a lot.
 *	Therefore, the PIC flush register has already been
 *	determined and stored for quick reference in the SSM
 *      descriptor indexed by ssm_index.  The correct algorithm
 *      to flush the PIC is:
 *
 *              issue a READ CSR to reset, and throw away the value,
 *              issue the "WRITE w/ PIC FLUSH" command,
 *              then Issue a READ to reset, and throw away the value.
 *
 * Remarks:
 *      Even with multiple processors the sequence will always
 *	ensure proper flushing. One processor's read may do 
 *	another processor's write op, but thats okay; a flush 
 *	doesn't need to be tied to the process that issued it. 
 *	It just needs to be done.
 *
 *   	DON'T use the ACK status bit.  Its caused problems in
 *	the past.  
 *
 *	The original method used the SLIC to write a slave 
 *	register; this is MUCH faster.
 *	
 *	Due to the PIC's readahead capability and buffer size,
 *	some drivers may need to flush both prior to starting
 *	a command/transfer and after it completes, just to
 *	ensure they don't get stale data.  Its even possible
 *	for drivers with buffers close by to cause your data
 *	to become stale.  If sequential transfers can be spread 
 *	out sufficiently (128 byte boundaries with PICw on SSM2),
 *	this operation may even become unnecessary - a further
 * 	optimization.
 *
 *	By declaring flush_reg volatile, the apparently meaningless
 *	accesses in this code should NOT be optimized out!
 */
void 
ssm_vme_dma_flush(int ssm_index)
{
	register volatile long *flush_reg;

	ASSERT(SSM_EXISTS(ssm_index) && SSM_desc[ssm_index].ssm_vme_alive);
	flush_reg = (volatile long *)SSM_desc[ssm_index].ssm_pic_flush_reg;
	ASSERT(flush_reg != NULL); 
		
	/*LINTED*/
	*flush_reg;			/* Read from flush reg */
	*flush_reg = PIC_BCR_FLUSH;	/* Write to flush reg */
	/*LINTED*/
	*flush_reg;			/* Read from flush reg again */
}

/*
 * int
 * ssm_vme_intr_setup(int, int, int, unchar, unchar, unchar)
 *	Set up a translation from a SSM VME bus interrupt
 *	to a SLIC interrupt in the SSM VME adapter.
 *	
 * Calling/Exit State:
 * 	Caller must own the specified SSM's misc. CB.
 *
 *	Basic and Sleep locks may be held upon entry/exit.
 *
 *	If successful: 
 *		the VME interrupt level and vector are
 *		registered with the SSM to prevent future 
 *		conflicts, 
 *
 *		the SSM is informed of the translation,
 *
 *		VME_INTMAP_OK (zero) is returned.
 *
 *	Other possible return values:
 *		VME_INVALID_BUS	   invalid SSM specified,
 *		VME_INTLVL_BAD	   invalid VME interupt level,
 *		VME_INTVEC_BAD	   invalid VME interupt vector,
 *		VME_INTMAP_BAD	   could not finish the mapping.
 *	
 * Description:
 *	Validate the SSM VMEbus index, VME interrupt description,
 *	and that they are not already in use by scanning the 
 * 	SSM's linked list of in use values.  If everything checks
 *	out, note these values are in use also, then notify the
 *	SSM via its misc CB of the new VME interrupt translation.
 *
 * Remarks: 
 *	The SSM only checks the last 8 bits of interrupt vectors.  
 *	it also *ignores* the interrupt level when translating the 
 *	VME interrupt into a SLIC interrupt, meaning the same 
 *	vector number *cannot* be used with two different levels
 *	to form distinct mappings.  It does respect the interrupt
 *	level as a priority mechanism, however.
 *
 *	For loadable module support, a mechanism to delete individual 
 *	interrupt mappings is also desirable; an enhancement request 
 *	has been filed against the SSM firmware to add such a facility.  
 *	Currently, the only close approximation would be to clear
 *	all mappings with ssm_clr_vme_imap(), then call ssm_set_vme_imap() 
 *	for all remaining mappings in the linked list.  This would
 *	open a critical region in which an interrupt could easily be lost.
 */
int
ssm_vme_intr_setup(int ssm_index, int bus_level, int bus_vector, 
		unchar slic_bin, unchar slic_vector, unchar slic_dest)
{
	struct ssm_desc *ssm = SSM_desc + ssm_index;
	struct ssm_vme_int *intr;

	if (! SSM_EXISTS(ssm_index) || ! ssm->ssm_vme_alive)
		return (VME_INVALID_BUS);

	if (bus_level < 1 || bus_level > 7)
		return (VME_INTLVL_BAD);

	for (intr = ssm->ssm_vints; intr != NULL; intr = intr->next) {
		if ((bus_vector & 0xff) == (intr->vector & 0xff)) 
			return (VME_INTVEC_BAD);
	}

	/* Add this one to the list and notify the SSM of its mapping */
	intr = (struct ssm_vme_int *) 
		kmem_alloc(sizeof (struct ssm_vme_int), KM_NOSLEEP);
	if ( !intr) {
		/*
		 *+ There was no kernel memory left when a VMEbus 
		 *+ device tried to set up its interrupt mappings.  
 		 *+ The request failed as a result.  
		 */
		cmn_err( CE_WARN, 
			"ssm_vme_intr_setup: no memory for intr mapping");
		return (VME_INTMAP_BAD);
	}
	intr->level = (unchar)bus_level;
	intr->vector = bus_vector;
	intr->next = ssm->ssm_vints;
	ssm->ssm_vints = intr;

	ssm_set_vme_imap(ssm->ssm_mc, ssm->ssm_slicaddr, (unchar)bus_level, 
			 (unchar)bus_vector, slic_dest, slic_vector, 
			 SL_MINTR | slic_bin);

	return (VME_INTMAP_OK);
}

/*
 * int
 * ssm_vme_assign_vec(int , int, unchar, unchar)
 *	Locate an available VMEbus vector on the assigned
 * 	SSM and map it to the designated SLIC interrupt.
 *
 * Calling/Exit State:
 *	Caller must own the misc. CB for the assigned SSM.
 *
 *      Basic and Sleep locks may be held upon entry/exit.
 *
 *      If successful the SSM is informed of assigned 
 *	VME to SLIC interrupt mapping, and the assigned
 *	VME interrupt vector is returned.
 *	Otherwise, -1 is returned.
 *
 * Description:
 *	Normally invoked during driver intialization, when
 *	the system is still single threaded.  It sets out
 *	to find the caller an available interrupt vector
 *	to use on the VMEbus, since many board's have
 *	software programable interrupt vectors.
 *
 *	The algorithm performs a simple linear search trying
 * 	the possible VME interrupt vector values until it
 *	finds one that is available, errors out, or exhausts
 *	the possibilities.  It invokes ssm_vme_intr_setup()
 *	to test each combination and perform the required
 *	mapping to the SSM VME adapter if it finds a usable
 *	combination.
 */
int
ssm_vme_assign_vec(int ssm_index, int vme_level, 
		unchar slic_bin, unchar slic_vec)
{
	int vme_vec, stat;

	for (vme_vec = 0; vme_vec <= MAX_VME_VEC; vme_vec++) {

		stat = ssm_vme_intr_setup(ssm_index, vme_level, vme_vec,
			 slic_bin, slic_vec, TMPOS_GROUP | SL_GROUP);

		switch (stat) {
		case VME_INTMAP_OK: 
			return(vme_vec);	/* Done - successful */

		case VME_INVALID_BUS: 
		case VME_INTLVL_BAD: 
		 	return (-1);		/* Failure - bad arg */

		case VME_INTVEC_BAD: 
			continue;	/* Already in use - try next vector */

		case VME_INTMAP_BAD:
		 	return (-1);		/* Failure - resource problem */
		}
	}
	return (-1);		/* Failure - exhausted the possibilities */
}
