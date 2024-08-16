/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/ssm/ssm_scsi.c	1.13"
#ident	"$Header: $"

/*
 * ssm_scsi.c
 *	routines for manipulating SCSI specific pieces of the SSM.
 */

#include <fs/buf.h>
#include <io/cfg.h>
#include <io/intctl.h>
#include <io/slic.h>
#include <io/ssm/ssm_misc.h>
#include <io/ssm/ssm_scsi.h>
#include <io/ssm/ssm.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <proc/proc.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>

STATIC struct scsi_cb *probe_cbs;	/* SCSI CBs recycled during PROBE */

/*
 * void
 * init_ssm_scsi_dev(struct scb_init *)
 *	provides initialization information about a SCSI
 *	device to the SSM board.
 *
 * Calling/Exit State:
 * 	Called while probing or booting a SCSI 
 *	device.  Returns an SSM specific device 
 *	id and location of CBs for this device
 *	in the sinfo structure.  A negative device
 *	device id is returned to indicate that 
 *	the initialization failed.  Once such
 *	failure occurs if the requested SCSI
 *	target addr is that of the SSM host adapter.
 *	NOT to be called after booting.
 *
 *	Provides information to the SSM by filling 
 *	out an initialization CB and passing its 
 *	address to the SSM via a sequence of 5 SLIC 
 *	messages.  The address is also the location 
 *	of the SCSI CBs to be used thereafter.
 *
 *	If the function argument's si_mode field is 
 *	SCB_PROBE a static set of CBs is used to 
 *	probe the device and then recycled.  Likewise,
 *	a special SLIC message tells the SSM it may
 *	recycle its device i.d.  Otherwise, a new 
 *	device i.d. and set of CBs is allocated.
 *
 *	Upon failure sinfo->si_id is < 0.  Upon success
 *	it is the device i.d. assigned by the SSM and
 *	sinfo->si_ssm_slic is set to the slic address
 *	of the SSM specified by sinfo->si_scsi.
 */
void
init_ssm_scsi_dev(struct scb_init *sinfo)
{
	volatile struct scsi_init_cb *icb;
	struct scsi_cb *tcb;
	paddr_t phys_icb;
	int i;
	unchar *addr_bytes = (unchar *)&phys_icb;
	unsigned int nbytes = sizeof(struct scsi_cb) * NCBPERSCSI;
	struct ssm_desc *ssm;

	/*
	 * First confirm that the request refers to a valid
	 * SSM and does not conflict with the SSM's target 
	 * addres on the SCSI bus.
	 */
	ssm = SSM_desc + sinfo->si_scsi;
	if (sinfo->si_scsi >= (uchar_t)ssm_cnt || FAILED(ssm->ssm_cd)
	||  sinfo->si_target == ssm->ssm_target_no
	||  (ssm->ssm_target_in_use & (1 << sinfo->si_target)) != 0) {
		sinfo->si_id = -1;
		return;		/* Fail the request. */
	}
	sinfo->si_ssm_slic = ssm->ssm_slicaddr;

	if (sinfo->si_mode == SCB_BOOT || !probe_cbs) {
		tcb = (struct scsi_cb *) ssm_scsi_alloc(nbytes, KM_NOSLEEP);
		if (! tcb)
			goto cb_alloc_fail;

		for (i = 0; i < NCBPERSCSI; i++) {
			tcb[i].sw.sw_un.cbs_wd1.cbs_wd2 = (struct cbs_wd2 *) 
				kmem_zalloc(sizeof(struct cbs_wd2), KM_NOSLEEP);
			if (! tcb[i].sw.sw_un.cbs_wd1.cbs_wd2) {
				for (i--; i >= 0; i--) {
					kmem_free(tcb[i].sw.sw_un.cbs_wd1.cbs_wd2, 
						sizeof(struct cbs_wd2));
				}
				ssm_scsi_free(tcb, nbytes);
				goto cb_alloc_fail;
			}
		}
		icb = (volatile struct scsi_init_cb *)(void *)tcb;
		if (sinfo->si_mode != SCB_BOOT)
			probe_cbs = tcb;
	} else {
		ASSERT(sinfo->si_mode == SCB_PROBE && probe_cbs);
		icb = (volatile struct scsi_init_cb *)(void *)probe_cbs;
	} 

	phys_icb = vtop((caddr_t)icb, NULL);

	/*
	 * Fill in the initialization CB.
	 */
	bzero((char *)icb, SCB_SHSIZ);
	icb->icb_pagesize = PAGESIZE;
	icb->icb_flags = sinfo->si_flags;
	icb->icb_sdest = SL_GROUP | TMPOS_GROUP;
	icb->icb_svec = sinfo->si_host_basevec;
	icb->icb_scmd = SL_MINTR | sinfo->si_host_bin;
	icb->icb_scsi = sinfo->si_scsi;
	icb->icb_target = sinfo->si_target;
	icb->icb_lunit = sinfo->si_lunit;
	icb->icb_control = sinfo->si_control;
	icb->icb_cmd = SCB_INITDEV;
	icb->icb_compcode = SCB_BUSY;

	/* 
	 * Use a special sequence of SLIC messages to
	 * to notify the SSM to process an initialization 
	 * CB located at the CB table address (address 
	 * sent low byte first, high byte last).
	 */
	slic_mIntr(sinfo->si_ssm_slic, SCSI_BIN, (unsigned char)
		((sinfo->si_mode == SCB_PROBE)? SCB_PROBEVEC : SCB_BOOTVEC));

	slic_mIntr(sinfo->si_ssm_slic, SCSI_BIN, addr_bytes[0]); /* low byte first */
	slic_mIntr(sinfo->si_ssm_slic, SCSI_BIN, addr_bytes[1]);
	slic_mIntr(sinfo->si_ssm_slic, SCSI_BIN, addr_bytes[2]);
	slic_mIntr(sinfo->si_ssm_slic, SCSI_BIN, addr_bytes[3]); /* high byte last */
	
	while (icb->icb_compcode == SCB_BUSY)	
		continue;		/* Poll for command completion */

	sinfo->si_id = icb->icb_id;	/* SSM device i.d. to return */
	if (sinfo->si_id >= 0) {
		/* Successful SSM/SCSI initialization */
		sinfo->si_cbs = (struct scsi_cb *)icb;	/* CB address to use */
		if (sinfo->si_mode == SCB_BOOT) {
			ssm->ssm_target_in_use |= 1 << sinfo->si_target;
		}
	} else {
		/*
		 *+ The SSM has rejected an attempt intialiate message passing
		 *+ for a SCSI device a driver wishes to configure or probe for.
		 *+ This normally indicates that the SSMs internal resource
		 *+ limit for the number of simultaneously SCSI devices it can 
		 *+ manage.  Most likely, either two many devices are 
		 *+ configured in the kernel or dynamic drivers have failed to 
		 *+ deconfigure their devices.
		 */
		cmn_err(CE_WARN, "SSM/SCSI: device initialization failed");

		if (sinfo->si_mode == SCB_BOOT) {
			/* Free CB resouces previously allocated. */
			for (i = 0; i < NCBPERSCSI; i++) {
				kmem_free(tcb[i].sw.sw_un.cbs_wd1.cbs_wd2, 
					sizeof(struct cbs_wd2));
			}
			ssm_scsi_free(tcb, nbytes);
		}
	}
	return;

cb_alloc_fail:
	/*
 	 *+ An attempt to allocate memory for a device driver to communicate
	 *+ with the SSM/SCSI bus has failed.  Subsequently, the driver
 	 *+ unit most likely has been deconfigured also.  This may be
	 *+ correctable by adding more memory to the system to support
	 *+ these devices or adjusting the kernel's tuning to consume
	 *+ less memory for other things.
	 */
	cmn_err(CE_WARN, "SSM/SCSI: message block allocation failed");
	sinfo->si_id = -1;
}

/*
 * void
 * deinit_ssm_scsi_dev(struct scb_init *)
 *	Break the SSM board's previous association with a SCSI device.
 *
 * Calling/Exit State:
 * 	Called while probing or booting a SCSI 
 *	device.  
 *
 *	The scb_init structure must be initialized with the 
 *	the device' i.d. and CB address previously returned by 
 *	init_ssm_scsi_dev(), the unit number of the SSM and the
 *	target adapter address previously supplied to init_ssm_scsi_dev().  
 *	The CB address and i.d. are unusable thereafter.
 *
 *	No return value.
 *
 * Description:
 *	Use one of the CBs to send a message to the SSM 
 *	firmware to deallocate the i.d. number provided by 
 *	the caller, polling its completion.  Then deallocate
 *	the CB's previously allocated by init_ssm_scsi_dev().
 */
void
deinit_ssm_scsi_dev(struct scb_init *sinfo)
{
	volatile struct scsi_cb *tcb = sinfo->si_cbs;
	unsigned int nbytes = sizeof(struct scsi_cb) * NCBPERSCSI;
	struct ssm_desc *ssm;
	int i;
	pl_t saved_pl;

	/*
	 * First confirm that the request refers to a valid SSM.
	 */
	ssm = SSM_desc + sinfo->si_scsi;
	ASSERT(sinfo->si_scsi < (uchar_t)ssm_cnt && !FAILED(ssm->ssm_cd));
        ASSERT(sinfo->si_id >= 0);

	tcb = (volatile struct scsi_cb *) sinfo->si_cbs;
	bzero((char *)tcb, SCB_SHSIZ);
	tcb->sh.cb_cmd = SCB_REMOVE_ID;
	tcb->sh.cb_compcode = SCB_BUSY;
        saved_pl = splhi();
        slic_mIntr(ssm->ssm_slicaddr, SCSI_BIN, SCVEC(sinfo->si_id, 0));
        splx(saved_pl);
        while (tcb->sh.cb_compcode == SCB_BUSY)
                continue;

        if (tcb->sh.cb_compcode != SCB_OK) {
		/*
		 *+ The SSM has rejected an attempt deintialize message 
		 *+ passing for a SCSI device a driver wishes to deconfigure.
		 *+ This may indicate that the SSM firmware does not support
	 	 *+ this request or has had an internal error.  It may also
		 *+ indicate an internal driver error resulting from it passing
		 *+ erroneous data into deinit_ssm_scsi_dev().  Check the
		 *+ driver, if possible, then confirm that the SSM firmware
		 *+ is up to date.  
		 */
		cmn_err(CE_WARN, 
			"SSM/SCSI: device deinitialization of i.d. %d failed",
			sinfo->si_id);
	} else {
		ssm->ssm_target_in_use &= ~(1 << sinfo->si_target);
	}

	for (i = 0; i < NCBPERSCSI; i++) {
		kmem_free(tcb[i].sw.sw_un.cbs_wd1.cbs_wd2, 
			sizeof(struct cbs_wd2));
	}
	ssm_scsi_free((void *)tcb, nbytes);
	if (tcb == probe_cbs) 
		probe_cbs = NULL;

	sinfo->si_id = -1;
	sinfo->si_cbs = NULL;
}

/*
 * void *
 * ssm_scsi_alloc(unsigned, int)
 *	Allocate nbytes of zeroed memory for control structures that 
 *	meets the alignment and boundary criteria for accessability 
 *	by the SSM SCSI adapter.
 *
 * Calling/Exit State:
 *      'nbytes' is the number of bytes to allocate.
 *	'flag' is passed through to the underlying allocator.
 *
 *	Returns address of allocated memory upon
 *	success or NULL upon failure.
 */
void *
ssm_scsi_alloc(unsigned nbytes, int flag)
{
	return(ssm_alloc(nbytes, SSM_ALIGN_XFER, SSM_BAD_BOUND, flag));
}

/*
 * void
 * ssm_scsi_free()
 *	Deallocate memory previously allocated by ssm_scsi_alloc().
 *
 * Calling/Exit State:
 *      'addr' is the address previously returned by ssm_scsi_alloc().
 *      'nbytes' is the number of bytes previously allocated.
 *      No return value.
 */
void
ssm_scsi_free(void *addr, unsigned nbytes)
{
	ssm_free(addr, nbytes);
}

/*
 * ulong_t
 * scb_buf_iovects(struct buf *, ulong_t *)
 *	Fill out an indirect address table for a
 *	SCSI I/O request, given a buf structure
 *	and the address of the indirect address
 *	table.
 *
 * Calling/Exit State:
 * 	Returns physical address of transfer start.
 */
ulong_t
scb_buf_iovects(struct buf *bp, ulong_t *iovp)
{
	page_t	*page;
	paddr_t	retval;
	vaddr_t	base;
	ulong_t	psize;
	ulong_t	limit;
	void *procp;
	uint_t	i;


	if ((bp->b_flags & B_PAGEIO) == B_PAGEIO) {
		page = getnextpg(bp, NULL);
		retval = *iovp++ = (ulong_t)pptophys(page);
		retval += (ulong_t) bp->b_un.b_addr;    /* add in offset */
		while ((page = getnextpg(bp, page)) != (page_t *)NULL)
			*iovp++ = (ulong_t)pptophys(page);
	} else {
		if ((bp->b_flags & B_PHYS) == B_PHYS) {
			procp = bp->b_proc;
		} else {
			procp = NULL;
		}

		retval = vtop((caddr_t)bp->b_un.b_addr, procp);	/* phys start */

		psize = ptob(1);
		base = (vaddr_t)bp->b_un.b_addr & ~(psize-1);  /* first base */
		limit = btopr (bp->b_bcount +
			       (ulong_t)bp->b_un.b_addr -
			       (ulong_t) base);

		for (i = 0; i < limit; i++) {
			*iovp++ = (ulong_t)vtop((caddr_t)base, procp);
			base += psize;		/* move to next page */
		}

	}
	return ((ulong_t)retval);
}
