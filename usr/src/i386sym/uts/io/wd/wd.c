/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/wd/wd.c	1.33"
#ident	"$Header: $"

/*
 * wd.c 
 *	SCSI disk device driver for SSM
 */

#include <fs/buf.h>
#include <fs/file.h>
#include <io/autoconf.h>
#include <io/clkarb.h>
#include <io/conf.h>
#include <io/metdisk.h>
#include <io/scsi.h>
#include <io/slicreg.h>
#include <io/ssm/ssm.h>
#include <io/ssm/ssm_cmblck.h>
#include <io/ssm/ssm_misc.h>
#include <io/ssm/ssm_scsi.h>
#include <io/uio.h>
#include <io/vtoc/vtoc.h>
#include <io/wd/dsort.h>
#include <io/wd/wd.h>
#include <mem/kmem.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <sys/open.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>	/* must come last */

/*
 * The following two macros turn on and off the 
 * front panel's disk activity light.  The splhi()
 * is required to prevent being preempted from the
 * processor while writing to the SLIC.
 */
#define	DISK_ACTIVITY_LED_ON			\
		if (fp_lights) {		\
			pl_t _pl = splhi();	\
			FP_IO_ACTIVE;		\
			splx(_pl);		\
		}

#define	DISK_ACTIVITY_LED_OFF			\
		if (fp_lights) {		\
			pl_t _pl = splhi();	\
			FP_IO_INACTIVE;		\
			splx(_pl);		\
		}

#define unittominor(x)		((x)<<8)
#define minortounit(x)		((x)>>8)
#define SSM_TARGET_BUS(A)	(0)
#define SSM_TARGET_TARGET(A)	(A)
#define SCSID_DRIVE(dev)        (minortounit(getminor((dev))))

/*
 * A physical devices' unit # corresponds to its
 * SCSI target adapter address on the SSM/SCSI bus.
 * (only embedded drives are supported).
 */
#define WD_MAX_UNITS		(SCSI_MAX_TARGET + 1)	

STATIC LKINFO_DECL(wdlockinfo, "ID:wd:wd_lock", 0);
STATIC LKINFO_DECL(wdsleep_lkinfo, "IO:wd:wd_sleep", 0);
wd_t 	**wd_info;
int	wd_base_vector = 0;
int	wd_ndevs;
int	wddevflag = D_MP | D_NOBRKUP | D_DMA;

STATIC int	wd_probe(uchar_t);
STATIC void	wd_rw(struct scsi_cb *, buf_t *);
STATIC long	wd_readc(struct scsi_cb *);
STATIC void	wd_command (struct scsi_cb *, uchar_t, uchar_t);
STATIC void 	wd_startio(wd_t *);

STATIC int	wd_cbfin(volatile struct scsi_cb *);
STATIC int	wdp_cbfin(volatile struct scsi_cb *);
STATIC int	wdp_command(struct scsi_cb *, uchar_t, vaddr_t, vaddr_t, uchar_t);
STATIC int	wd_getvtoc(wd_t *, dev_t);

extern	uchar_t wd_bin;
extern	int wdretrys;
extern	int console_slic_id;

struct dev *wd_dev;			  /* to be allocated in wdinit */

caddr_t	wd_compcode[] = {
	"Command Block Busy",
	"OK",
	"Bad Command Block",
	"No Target",
	"SCSI Bus Error",
	"Unrecognized Completion Code"
};

int	wdncompcodes = sizeof(wd_compcode) / sizeof(wd_compcode[0]);

caddr_t	wd_addsense[] = {
	"No Additional Sense",
	"No Index/Sector Signal",
	"No Seek Complete",
	"Write Fault",
	"Drive Not Ready",
	"Drive Not Selected",
	"No Track Zero Found",
	"Multiple Drives Selected",
	"Communication Failure",
	"Track Following Error",
	"Error 0x0a",
	"Error 0x0b",
	"Error 0x0c",
	"Error 0x0d",
	"Error 0x0e",
	"Error 0x0f",
	"ID Error",
	"Unrecovered Read",
	"No Addr Mark in ID",
	"No Addr Mark in Data",
	"No Record Found",
	"Seek Positioning Error",
	"Data Synch Mark Error",
	"Recovered Read, Retries",
	"Recovered Read, ECC",
	"Defect List Error",
	"Parameter Overrun",
	"Synchronous Transfer Error",
	"Defect List Not Found",
	"Compare Error",
	"Recoevered ID, ECC",
	"Error 0x1f",
	"Invalid Command Code",
	"Illegal Block Address",
	"Illegal Function",
	"Error 0x23",
	"Illegal Field in CDB",
	"Inavalid LUN",
	"Invalid Parameter",
	"Write Protected",
	"Medium Changed",
	"Power On or Reset",
	"Mode Select Changed",
	"Error 0x2b",
	"Error 0x2c",
	"Error 0x2d",
	"Error 0x2e",
	"Error 0x2f",
	"Inacompatible Cartridge",
	"Format Corrupted",
	"No Spare Available",
	"Error 0x33",
	"Error 0x34",
	"Error 0x35",
	"Error 0x36",
	"Error 0x37",
	"Error 0x38",
	"Error 0x39",
	"Error 0x3a",
	"Error 0x3b",
	"Error 0x3c",
	"Error 0x3d",
	"Error 0x3e",
	"Error 0x3f",
	"Ram Failure",
	"Data Path Diag Failure",
	"Power On Diag Failure",
	"Message Reject Error",
	"Internal Controller Error",
	"Select/Reselect Failed",
	"Unsuccessful Soft Reset",
	"Parity Error",
	"Initiator Detected Error",
	"Inappropriate/Illegal Message",
	""
};

int	wdnaddsense = sizeof(wd_addsense) / sizeof(wd_addsense[0]);

/*
 * STATIC int 
 * wd_probe(unchar_t unit)
 *	Determine if a disk drive is present at the specified
 *	SCSI target adapter address.
 *
 * Calling/Exit State:
 *	Caller must not hold any locks or sleep locks.
 *
 *	'unit' is the target adapter address where the drive is
 *	to be looked for, the primary SSM's SCSI bus is assumed.
 *
 *	returns 1 if the device is found and available; zero otherwise.
 *
 * Description:
 *	Issue a test-unit-ready command to the specified device
 *	location to determine if the target will respond and the
 *	drive is ready for use. If there is an invalid response 
 *	or the drive is not fully spun up, return failure.
 *	Otherwise, issue a inquiry-command to determine the device
 *	characteristics of the responder.  If it is a CCS compliant
 *	direct access device (disk), then return success.
 *	
 *	This function assumes it is called during system bootup,
 *	that usable drives are already powered on and spun up.
 */
STATIC int
wd_probe(uchar_t unit)
{
	struct	scb_init sinit;
	struct	scinq_dev *wd_inq;
	uchar_t	devno = SSM_SCSI_DEVNO(0, unit);
	uchar_t	compcode;
	static	vaddr_t wd_sense_data = NULL;
	static	vaddr_t wd_probe_data = NULL;
	
	sinit.si_mode = SCB_PROBE;
	sinit.si_flags = 0;
	sinit.si_ssm_slic = (uchar_t) console_slic_id;
	sinit.si_scsi = 0;		/* bus no. - currently always 0 */ 
	sinit.si_target = unit;
	sinit.si_lunit = 0;
	sinit.si_control = 0;
	init_ssm_scsi_dev(&sinit);

	if (sinit.si_id < 0)
		return (0);		/* init_ssm_scsi_dev failed */

	ASSERT(sinit.si_cbs);
	sinit.si_cbs->sw.cb_unit_index = SCVEC(sinit.si_id, 0);  
        sinit.si_cbs->sw.cb_slic = (uchar_t) console_slic_id;
	
	/* 
	 * If a request sense and a data buffer aren't
	 * allocated for probing, then do so now.
 	 */
	if (!wd_sense_data) {
		wd_sense_data = (vaddr_t)
			ssm_scsi_alloc(SIZE_MAXDATA, KM_NOSLEEP);
		ASSERT(wd_sense_data);
	}
	if (!wd_probe_data) {
		wd_probe_data = (vaddr_t)
			ssm_scsi_alloc(SIZE_MAXDATA, KM_NOSLEEP);
		ASSERT(wd_probe_data);
	}

	/*
 	 * Issue a TEST-UNIT-READY command to the 
	 * specified target.  wdp_command will obtain
	 * request sense data if CHECK CONDITION occurs
     	 * in response to the command.
	 */
	compcode = wdp_command(sinit.si_cbs, SCSI_TEST, 
	    wd_sense_data, wd_probe_data, devno);
	switch (compcode) {
	case SCB_OK:
		break;
	case SCB_NO_TARGET:
	case SCB_BAD_CB:
	case SCB_SCSI_ERR:
	default:
		return (0);
	}

	/*
	 * Verify that the device that responded to the
	 * test unit ready is a hard disk, since there
	 * may be other devices present.  Use an inquiry
	 * command to verify that it is a direct access, 
	 * common command set (CCS) drive.
 	 */
	if (wdp_command(sinit.si_cbs, SCSI_INQUIRY, 
	    wd_sense_data, wd_probe_data, devno) != SCB_OK) {
		return (0);
	} else if (SCSI_CHECK_CONDITION(sinit.si_cbs->sh.cb_status)) {	
		return (0);
	}

	wd_inq = (struct scinq_dev *) wd_probe_data;
	if ((wd_inq->sc_hdr.ih_devtype == INQ_DIRECT) && 
 	    (wd_inq->sc_hdr.ih_qualif & INQ_REMOVABLE) != INQ_REMOVABLE) {
		return (1);
	} else {
		return(0);
	}
}

/*
 * void
 * wdinit(void)
 *	located and initialize embedded SCSI disk drives on the SSM.
 *
 * Calling/Exit State:
 *	Caller must not hold any locks or sleep locks.
 *
 *	Allocates and initializes the wd_info table,
 *	indexes into which reflect a devices target address
 *	on the primary SSM's SCSI bus.  Valid devices are
 *	given non-NULL wd_t pointers in this table with the 
 *	wd_alive field set non-zero.
 *
 *	wd_ndevs is set to the largest of the available drive 
 *	unit numbers.
 *
 *	No return value.
 *
 * Description:
 *	For each of the possible target adaptor locations on the
 *	primary SSM SCSI bus, invoke wd_probe() to determine if
 *	a disk drive is present and spun up.  If it is, allocate
 *	and initialize permanent SSM resources for message passing
 *	its SCSI commands to the SSM.  Also initialize its wd_t
 *	structure and mark the device as available if its capacity
 *	can be successfully read.  
 *
 *	Logical drive unit numbers are the same as their target
 *	adapter location on the SCSI bus.  Interrupt vectors (1
 *	per CB) are reserved for all possible devices so corresponding
 *	drive units can be calculated simply by wdintr().
 */
void
wdinit(void)
{
	wd_t *ip;		/* info structure pointer */ 
	struct scsi_cb *cb;
	struct scb_init sinit;
	uchar_t	unit;
	int x;
	ushort maps_per_CB;

	wd_info = (wd_t **) 
		kmem_zalloc(WD_MAX_UNITS * sizeof(wd_t *), KM_NOSLEEP);
	if (wd_info == NULL) {
		/*
		 *+ Kernel memory for the wd_info struct could
		 *+ not be allocated. Reconfigure the system
		 *+ to consume less memory.
		 */
		 cmn_err(CE_PANIC, "wdinit: no memory for wd_info structs");
	}

	wd_base_vector = ivec_alloc_group(wd_bin, WD_MAX_UNITS * 2);
	wd_ndevs = 0;

	for (unit = 0; unit < (uchar_t)WD_MAX_UNITS; unit++) {
		if (wd_probe(unit) == 0) {
			continue;	/* Disk drive not found */
		}
	
		cmn_err(CE_CONT, 
			"wd %d found on ssm 0 target %d\n", unit, unit);
		ivec_init(wd_bin, wd_base_vector + (unit * 2), wdintr);
		ivec_init(wd_bin, wd_base_vector + (unit * 2) + 1, wdintr);

		/*
		 * Allocate info struct and setup
 		 * SSM message blocks for use.
 		 */	
		wd_info[unit] = ip = 
			(wd_t *)kmem_zalloc(sizeof(wd_t), KM_NOSLEEP);
		if (ip == NULL) {
			/*
			 *+ Kernel memory for the wd_info struct could
			 *+ not be allocated. Reconfigure the system
			 *+ to consume less memory.
			 */
			 cmn_err(CE_PANIC, "wdinit: no memory for wd_info structs");
		}
		ip->wd_devno = SSM_SCSI_DEVNO(SSM_TARGET_TARGET(unit), 0);
		ip->wd_ctlr = SSM_TARGET_BUS(unit);

		sinit.si_mode = SCB_BOOT;
		sinit.si_flags = 0;
		sinit.si_ssm_slic = (uchar_t) console_slic_id; 
		sinit.si_scsi = 0;
		sinit.si_target = SSM_TARGET_TARGET(unit);
		sinit.si_lunit = 0;
		sinit.si_host_bin = wd_bin;
		sinit.si_host_basevec = wd_base_vector + (unit * 2); 
		sinit.si_control = 0;
		init_ssm_scsi_dev(&sinit);	/* Assigns SSM resources */
		if (sinit.si_id < 0) {
			/*
			 *+ There are more devices on the SCSI
			 *+ bus than the SSM can handle.  This
			 *+ device will not be usable as a result.
			 */
			cmn_err(CE_WARN,
				"wd%d: initialization error - exceeded SCSI ",
				unit);
			continue;
		}

		/*
 		 * Set up driver resources to manage the double
		 * buffered message blocks.  For each cb:
	 	 *	allocate request sense buffers
		 * 	allocate data buffers
		 *	allocate scatter gather/DMA mapping resources.
		 * 	fill in all sw fields of cbs
		 */
		ip->wd_cbdp = (struct cb_desc *) alloc_cb_desc(
			(char *) sinit.si_cbs,
			(short) NCBPERSCSI,
			(short) sizeof(struct scsi_cb),
		 	(short) FLD_OFFSET(struct scsi_cb, sw.cb_state));
		maps_per_CB = (WD_MAX_XFER * DEV_BSIZE) / ptob(1) + 1;
		/* LINTED */
		for(x = 0, cb = (struct scsi_cb *) ip->wd_cbdp->cb_cbs;
		    x < ip->wd_cbdp->cb_count; 
		    x++, cb++) {

			cb->sw.cb_vsense = (ulong_t)
				ssm_scsi_alloc(SIZE_MAXDATA, KM_NOSLEEP);
			cb->sh.cb_sense =
				(ulong_t) vtop((caddr_t)cb->sw.cb_vsense, NULL);
			cb->sw.cb_data = (ulong_t)
				ssm_scsi_alloc(SIZE_MAXDATA, KM_NOSLEEP);
			cb->sh.cb_slen = SIZE_MAXDATA;
			
			cb->sw.cb_iovstart =
				(ulong_t *)ssm_scsi_alloc(maps_per_CB * 
				/*LINTED*/
							  sizeof(ulong_t),
							  KM_NOSLEEP);
			cb->sw.cb_iovnum = maps_per_CB;
			cb->sw.cb_unit_index = SCVEC(sinit.si_id, x);  
			cb->sw.cb_scsi_device = ip->wd_devno;
			cb->sw.cb_slic = (uchar_t) console_slic_id;
		}

		/*
		 * Determine the # of usable sectors on the disk drive.
		 */
		/* LINTED */
		cb = (struct scsi_cb *) ip->wd_cbdp->cb_cbs;
		if ((ip->wd_capacity = wd_readc(cb)) <= 0) {
			/*
			 *+ The wd device failed to respond to a
			 *+ SCSI READ CAPACITY command and is
			 *+ being deconfigured.  This can indicate
			 *+ a problem with the SSM or with the drive,
			 *+ as the drive responded validly when
			 *+ previously probed and inquired of.
			 */
			cmn_err(CE_WARN,
			"wd%d: unable to READ CAPACITY ... deconfiguring\n",
				unit);
			if (fp_lights) {
				FP_IO_ERROR;
			}
			continue;
		}
        
		ip->wd_alive = 1;	/* if we got here we're alive */
		wd_ndevs = unit + 1 ;	/* Note the largest unit available */
		ip->wd_iovcount = maps_per_CB;
		ip->wd_inuse = 0;
		ip->wd_flags = WD_IS_ALIVE;
		ip->wd_lock = LOCK_ALLOC(2, pldisk, &wdlockinfo, KM_NOSLEEP);
 		if (ip->wd_lock == NULL) {
 			/*
 			 *+ Kernel memory for wd_lock struct could
 			 *+ not be allocated. Reconfigure the system
 			 *+ to consume less memory.
 			 */
 			cmn_err(CE_PANIC, "wdinit: no memory for wd_lock");
 		}	

		ip->wd_sleep = SLEEP_ALLOC(0, &wdsleep_lkinfo, KM_NOSLEEP);
 		if (ip->wd_sleep == NULL) {
 			/*
 			 *+ Kernel memory for wd_sleep struct could
 			 *+ not be allocated. Reconfigure the system
 			 *+ to consume less memory.
 			 */
 			cmn_err(CE_PANIC, "wdinit: no memory for wd_sleep");
 		}	
		/*
		 * Allocate disk stats structure
		 */
		ip->wd_stats = met_ds_alloc_stats("wd", unit, ip->wd_capacity,
					MET_DS_BLK | MET_DS_NO_RESP_HIST);
	}
}

/*
 * void
 * wdstart(void)
 *      Complete initialization of embedded SCSI disk drives on the SSM.
 *
 * Calling/Exit State:
 *      Caller must not hold any locks or sleep locks.
 *
 *      Upon completion, each configured SSM/SCSI disk drive
 *      will have the head of its request queue initialized.
 *
 *      No return value.
 *
 * Description:
 *      The buf header for the disksort() request queue of each
 *      configured disk drive has not yet been allocated because
 *      wdinit() is not allowed to call getrbuf().  Perform the
 *      allocation now.
 */
void
wdstart(void)
{
        wd_t *ip;               /* info structure pointer */
        uchar_t unit;

        for (unit = 0; unit < (uchar_t)wd_ndevs; unit++) {
                ip = wd_info[unit];
                if (ip == NULL || ip->wd_alive == 0)
                        continue;       /* Device is not configured */

                if ((ip->wd_bufh = getrbuf(KM_NOSLEEP)) == NULL) {
                        /*
                         *+ getrbuf() was unable to allocated a
                         *+ "struct buf" for wd_bufh.  Reconfigure
                         *+ the system to consume less memory.
                         */
                        cmn_err(CE_PANIC, "wdstart: no memory for wd_bufh");
                }
                ip->wd_bufh->av_forw = ip->wd_bufh->av_back = NULL;
        }
}

/*
 * int
 * wdopen(dev_t *dev, int oflag, int otyp, cred_t *crp)
 * 	open a disk drive or logical partition thereof for I/O.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Count the number of active accesses to determine
 *	when to validate/invalidate the volume label and
 *	permit exclusive access.
 *
 *	Returns zero if the open completes successfully.
 *	Otherwise returns: 
 *		ENXIO if the device number does not 
 *		reference a valid/available disk drive 
 *		or partition.
 *
 *		EPERM if exclusive access is requested by
 *		a non-priviledged caller.
 *
 *		EBUSY is the minor number cannot be accessed
 *		at this time, such as when it is already open
 *		with exclusive access.
 *
 *		EAGAIN if the open would block and a non-blocking
 *		open had been specified as per the oflag argument.
 *
 *		EIO if an attempt to read its volume label fails.
 *
 * Description:
 *	Determine the drive unit referenced by the device number
 *	and validate that the drive is available.  Validate the
 *	drive's volume label (VTOC) on every "first open" when the 
 *	device is to be partitioned.  If the minor device is a slice 
 *	of the partitioned device, verify that the specified slice
 *	was marked usable in the VTOC.
 *
 *	Also, the unpartitioned device cannot be opened while
 *	a slice of its partitioned device is in use, and vice-versa.
 *
 *	The unit's sleep-lock must be aquired to protect the unit's 
 *	open counters, state flags, and partition table against 
 *	concurrent access by wdopen(), wdclose(), and wdsize().
 */
int
wdopen(dev_t *dev, int oflag, int otyp, cred_t *crp)
{
	int drive = SCSID_DRIVE(*dev);
	wd_t *ip;
	struct partt *pt;
	int compcode = 0, *flag_ptr, *minor_nopen, fake_nopen = 0;

	if (drive >= wd_ndevs)
		return (ENXIO);		/* Unit info struct not found table */

	ip = wd_info[drive];
	if (ip == NULL || (ip->wd_flags & WD_IS_ALIVE) == 0)
		return (ENXIO);		/* Unit is not available for use */

	/* 
	 * Acquire the unit's sleep lock to mutex
	 * wdopen(), wdclose(), and wdsize() while
	 * the open-counts, state flags, and partition 
	 * tables may be volatile.  However, don't
	 * wait if the caller wanted a non-blocking open.
	 */
	if ((oflag & (FNDELAY | FNONBLOCK)) == 0) {
		SLEEP_LOCK(ip->wd_sleep, pridisk);
	} else {
		if (SLEEP_TRYLOCK(ip->wd_sleep) == B_FALSE) 
			return (EAGAIN);
	}

	/*
	 * Allow opens to the partitioned device
	 * only if the unpartitioned device is 
	 * closed, and vice-versa.  
	 */
	if (ip->wd_nopen > 0) {
		/*
		 * The device is already open, so make
		 * certain this open is compatible.  For
		 * partitioned devices, verify that the
		 * specified slice is valid also.
		 */
		if (VPART(*dev) == V_RAW_MINOR) {
		 	if ((ip->wd_flags & WD_RAW) == 0) {
				compcode = EBUSY;
				goto exit_open;
			}
		} else if ((ip->wd_flags & WD_RAW) != 0) {
			compcode = EBUSY;
			goto exit_open;
		} else if ((pt = &ip->wd_p[VPART(*dev)])->p_start < 0) {
			compcode = ENXIO;	/* Invalid slice */
			goto exit_open;
		}
	} else if (VPART(*dev) != V_RAW_MINOR) {
		/* 
 		 * Since partitioned device is not already
		 * open, fetch its VTOC and verify that it 
		 * and the specified slice are valid.
		 */
		if ((compcode = wd_getvtoc(ip, *dev)) != 0) {
			goto exit_open;		/* Failure getting VTOC */
		}

		if ((pt = &ip->wd_p[VPART(*dev)])->p_start < 0) {
			compcode = ENXIO;	/* Invalid slice */
			goto exit_open;
		}
	}

	/* 
	 * The remainder of wdopen is essentially the same 
	 * for both partitioned and unpartitioned devices 
	 * if we locate the applicable set of minor device 
	 * flags and open counts. 
	 */
	if (VPART(*dev) == V_RAW_MINOR) {
		/* use fields from wd_info structure */
		flag_ptr = &ip->wd_flags;
		minor_nopen = &fake_nopen;	/* We don't really need this */
	} else {
		/* use fields from the slice info in the partition table */
		flag_ptr = &pt->p_flags;
		minor_nopen = &pt->p_nopen;
	}

	/*
	 * Fail this open if the minor device is 
	 * already opened exclusively.  Also fail if 
	 * this is an exclusive open for an already open 
	 * minor device or a non-priviledged caller.
	 */
	if (*flag_ptr & WD_EXCLUSIVE) {
		compcode = EBUSY;		/* Don't permit another open */
		goto exit_open;
	} else if (oflag & FEXCL) {		
		if (!drv_priv(crp)) {
			compcode = EPERM;
			goto exit_open;
		} else if (*minor_nopen) {
			compcode = EBUSY;
			goto exit_open;
		}
		*flag_ptr |= WD_EXCLUSIVE;	/* Allow exclusive access  */
	}

	/* 
	 * Complete the open by counting only
 	 * open()s for which there will be a
 	 * corresponding close later.  This is 
	 * the case only once each for all 
	 * OTYP_BLK and OTYP_CHR opens; every 
	 * OTYP_LYR open counts.
	 */
	switch (otyp) {
	case OTYP_BLK:
		if ((*flag_ptr & WD_BLK) == 0) {
			*flag_ptr |= WD_BLK;
			(*minor_nopen)++;
			ip->wd_nopen++;
		}
		break;
	case OTYP_CHR:
		if ((*flag_ptr & WD_CHR) == 0) {
			*flag_ptr |= WD_CHR;
			(*minor_nopen)++;
			ip->wd_nopen++;
		}
		break;
	case OTYP_LYR:
		(*minor_nopen)++;
		ip->wd_nopen++;
		break;
	}

	if (VPART(*dev) == V_RAW_MINOR)
		ip->wd_flags |= WD_RAW;		/* Lock out partitioned opens */
exit_open:
	SLEEP_UNLOCK(ip->wd_sleep);
	return (compcode);
}

/*
 * int
 * wdclose(dev_t dev, int flag, int otyp, cred_t *crd)
 * 	close a disk drive or logical partition thereof.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Count the number of active accesses to determine
 *	when to validate/invalidate the volume label and
 *	permit exclusive access.
 *
 *	returns zero if the open completes successfully,
 *	or an errno if an attempt to close a partition
 *	fails.
 *
 * Description:
 *	Determine whether the minor device is for a partitioned 
 *	or unpartitioned minor device, to locate the applicable 
 *	open-counter and state flags to be adjusted for this close.
 *	There is only one close() each for all OTYP_BLK and OTYP_CHR 
 * 	opens on the minor device; every OTYP_LYR open has a 
 *	corresponding close() though.  They must be tracked carefully
 *	so it can be determined when the unit is entirely closed,
 *	after which the VTOC must be reread/validated for the next
 *	access to the partitioned version of the device.  This is
 *	also crucial for switching between using the device in a
 *	partitioned vs. unpartitioned manner, since concurrent use
 *	of both modes is prohibited by the driver.
 */
/*ARGSUSED*/
int
wdclose(dev_t dev, int flag, int otyp, cred_t *crp)
{
	int drive = SCSID_DRIVE(dev);
	wd_t *ip = wd_info[drive];
	int *flag_ptr, *minor_nopen, fake_nopen = 0;

	/* 
	 * Acquire the unit's sleep lock to mutex
	 * wdopen(), wdclose(), and wdsize() while
	 * the open-counts, state flags, and partition 
	 * tables may be volatile.
	 */
	SLEEP_LOCK(ip->wd_sleep, pridisk);
	ASSERT(ip->wd_nopen > 0);

	/* 
	 * Adjust open counts. There is only one
	 * close() each for all OTYP_BLK and OTYP_CHR 
	 * opens on the minor device; every OTYP_LYR 
	 * open has a corresponding close() though.
	 *
	 * This is essentially the same for both partitioned 
	 * and unpartitioned devices if we locate applicable 
	 * minor device flags and open counts. 
	 */
	if ((ip->wd_flags & WD_RAW) != 0) {
		/* use fields from wd_info structure */
		flag_ptr = &ip->wd_flags;
		minor_nopen = &fake_nopen;	/* We don't really need this */
	} else {
		/* use fields from the slice info in the partition table */
		flag_ptr = &ip->wd_p[VPART(dev)].p_flags;
		minor_nopen = &ip->wd_p[VPART(dev)].p_nopen;
	}
	switch (otyp) {
	case OTYP_BLK:
		ASSERT((*flag_ptr & WD_BLK) != 0);
		*flag_ptr &= ~(WD_BLK | WD_EXCLUSIVE);
		break;
	case OTYP_CHR:
		ASSERT((*flag_ptr & WD_CHR) != 0);
		*flag_ptr &= ~(WD_CHR | WD_EXCLUSIVE);
		break;
	case OTYP_LYR:
		*flag_ptr &= ~WD_EXCLUSIVE;
		break;
	}
	(*minor_nopen)--;
	ip->wd_nopen--;

	if (ip->wd_nopen == 0) {
		ip->wd_flags &= ~WD_RAW;
	}
	SLEEP_UNLOCK(ip->wd_sleep);
	return (0);
}

/*
 * int
 * wdread(dev_t dev, uio_t *uiop, cred_t *crp)
 *	execute a raw read request.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Adjusts the uio structure to reflect the number
 *	of bytes actually read if any portion of the 
 *	transfer completes.
 *
 *	returns zero if the I/O completes or an errno if 
 *	it fails.
 *
 * Description:
 *	Invoke wdstrategy() via physiock() to lock the request
 *	data buffer into memory, break the request up if 
 *	neccessary, and truncate it to stay within the device's 
 *	addressable limit.  If the minor device is for an 
 *	unpartitioned device, then the addressable limit is the
 *	sector capacity of the drive.  Otherwise, the slice
 *	capacity is retrieved from the unit's partition table.
 *	wdstrategy will execute the request from there.
 */
/*ARGSUSED*/
int
wdread(dev_t dev, uio_t *uiop, cred_t *crp)
{
	daddr_t lim;
	int drive = SCSID_DRIVE(dev);
	wd_t *ip = wd_info[drive];

	ASSERT(SCSID_DRIVE(dev) < wd_ndevs);
	lim = ((ip->wd_flags & WD_RAW) != 0) ? 
		ip->wd_capacity : ip->wd_p[VPART(dev)].p_size;
	return (physiock((void (*)())wdstrategy, (buf_t *)0, dev, B_READ,
			 lim, uiop));
}

/*
 * int
 * wdwrite(dev_t dev, uio_t *uiop, cred_t *crp)
 *	execute a raw write request.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Adjusts the uio structure to reflect the number
 *	of bytes actually read if any portion of the 
 *	transfer completes.
 *
 *	returns zero if the I/O completes or an errno if 
 *	it fails.
 *
 * Description:
 *	Invoke wdstrategy() via physiock() to lock the request
 *	data buffer into memory, break the request up if 
 *	neccessary, and truncate it to stay within the device's 
 *	addressable limit.  If the minor device is for an 
 *	unpartitioned device, then the addressable limit is the
 *	sector capacity of the drive.  Otherwise, the slice
 *	capacity is retrieved from the unit's partition table.
 *	wdstrategy will execute the request from there.
 */
/*ARGSUSED*/
int
wdwrite(dev_t dev, uio_t *uiop, cred_t *crp)
{
	daddr_t lim;
	int drive = SCSID_DRIVE(dev);
	wd_t *ip = wd_info[drive];

	ASSERT(SCSID_DRIVE(dev) < wd_ndevs);
	lim = ((ip->wd_flags & WD_RAW) != 0) ? 
		ip->wd_capacity : ip->wd_p[VPART(dev)].p_size;
	return (physiock((void (*)())wdstrategy, (buf_t *)0, dev, B_WRITE,
			 lim, uiop));
}

/*
 * int
 * wdstrategy(buf_t *bp)
 *	SSM SCSI disk strategy routine.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Adjusts the buf structure to reflect the completion
 *	status and the amount of data transferred.	
 *
 *	The transfer buffer must be locked into memory
 *	upon entry/exit.
 *
 *	Request normally are not be completed upon exit;
 *	the caller is expected to be awakened by biodone().
 *
 *	returns EINVAL if the request exceeds the device
 *	capacity or if either the transfer size or block
 *	address are negative.  Returns zero otherwise.
 *
 * Description:
 *	First verify that the request has a transfer count that is
 *	a positive multiple of the device sector size, that
 *	the sectors to transfer are addressable for the minor 
 *	device, and that the transfer count does not exceed the 
 *	driver's maximum transfer size.  Terminate the request 
 * 	with EINVAL if any of these tests fail.
 *
 *	If the minor device is for an unpartitioned device the 
 *	addressable limit of the minor device is the sector capacity 
 *	of the drive.  Otherwise, its the slice capacity for the minor 
 *	device, which is retrieved from the unit's partition table
 *	along with the slice's starting sector address relative to
 *	the start of the unpartitioned device.  The slice's start
 *	sector is used to compute the request's starting physical
 *	device address, which is stored in bp->b_priv2.un_daddr to be
 *	used by seek optimization algorithms and to execute the
 *	actual transfer.
 *
 *	Next, invoke disksort() to perform seek optimization and queue 
 *	the request to be started by wd_startio() when a driver message 
 *	block becomes available.  disksort() expects the sortkey, 
 *	which is the transfer's physical start sector address, to be in 
 *	bp->b_priv2.un_daddr.  
 *
 * 	Also, The logical unit's wd_lock must be locked while executing
 *	disksort() and wd_startio(), to ensure the integrity of the
 *	sort queue while those functions manipulate the list.  Once the
 *	request has been queued and, possibly, started, return to the
 *	caller.  wdintr() will eventually deliver the request completion 
 *	to the calling task using biodone().
 */
/*ARGSUSED*/
int
wdstrategy(buf_t *bp)
{
	wd_t *ip;
	pl_t s;
	int length;
	int pstart;

	/*
	 * Validate the request's parameters.
	 */
	ip = wd_info[SCSID_DRIVE(bp->b_edev)];
	ASSERT(ip != NULL);
 
	/*
	 * Determine the capacity and start block
	 * offset of the minor device on the unit.
	 *
	 * CAUTION: bp->b_edev must be used to determine
	 * the type of minor device being accessed, since
	 * the unit may not be open() yet when the VTOC is
	 * read by wd_getvtoc().  In that case ip->wd_flags
	 * would not have been correctly set yet.
	 */
	if (VPART(bp->b_edev) == V_RAW_MINOR) {
		length = ip->wd_capacity;
		pstart = 0 ;
	} else {
		length = ip->wd_p[VPART(bp->b_edev)].p_size ;
		pstart = ip->wd_p[VPART(bp->b_edev)].p_start;
		ASSERT(pstart >= 0);
	}

	if (bp->b_bcount <= 0				/* Bad transfer count */
	||  (bp->b_bcount & (DEV_BSIZE - 1)) != 0	/* Not a full block */
	||  bp->b_bcount > WD_MAX_XFER * DEV_BSIZE	/* Exceeds xfer limit */
	||  bp->b_blkno < 0				/* Invalid block addr */
	||  bp->b_blkno + howmany(bp->b_bcount, DEV_BSIZE) > length) {
		bp->b_resid = bp->b_bcount;	
		bioerror(bp, EINVAL);
		biodone(bp);
		return (EINVAL);
	}

	/*
	 * Set up the disk sort key for seek optimization,
	 * lock the device so this request may be placed
	 * onto sort list and an attempt made to start
	 * the next request from that list.
         */
	bp->b_priv2.un_daddr = bp->b_blkno + pstart;
	s = LOCK(ip->wd_lock, pldisk);
	disksort(ip->wd_bufh, bp);
	/*
	 * Update disk stats
	 */
	met_ds_queued(ip->wd_stats, bp->b_flags);
	wd_startio(ip);
	UNLOCK(ip->wd_lock, s);
	return (0);
}

/*
 * STATIC void
 * wd_startio(wd_t *ip)
 * 	Attempt to start the next queued request for the specified device.
 *
 * Calling/Exit State:
 *	The specified device's wd_lock must be held upon entry/exit.
 *	Other locks may be held upon entry/exit as well.
 *
 *	Deletes the next element from the device's sort list if it
 *	can start it now.
 *
 *	No return value.
 *
 * Description:
 *	Return immediately if there are no requests queued to the
 *	specified device's sort queue or if all SSM message blocks 
 *	for that device are currently in use.  Otherwise, delete
 * 	the next request from the sort list and communicate it to
 *	the SSM.  Also turn on the front panel disk activity light
 * 	while the request is active.
 */
STATIC void
wd_startio(wd_t *ip)
{
	struct scsi_cb *cb;
	buf_t *bp;

	if ((bp = ip->wd_bufh->av_forw) == NULL)
		return;			/* No requests waiting */

	/* LINTED */
	if ((cb = (struct scsi_cb *) get_cb(ip->wd_cbdp)) == NULL)
		return;			/* No SSM resources to start it with */

	if (ip->wd_inuse == 0) {
		DISK_ACTIVITY_LED_ON;
	}
 	ip->wd_inuse++;
	ip->wd_bufh->av_forw = bp->av_forw;	/* Delete from request queue */
	wd_rw(cb, bp);			/* Start execution of the request */
}

/*
 * void 
 * wdintr(int vector) 
 * 	Command block completion interrupt handler.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry/exit.
 *
 *	vector must equal: wd_base_vector + (logical_unit * CBs_per_unit) + CB#.
 *
 *	The associated buf address must be stored in the terminating CB.
 *	
 *	No return value, although a completion status and the number of
 * 	bytes successfully transferred are reported in the associated buf
 *	prior to calling biodone() to terminate it.
 *
 * Description:
 *	From the interrupt vector, calculate/locate the logical unit, 
 *	associated message block, and then the associated buf for the 
 *	request reporting termination.  Use the CB completion code, 
 *	and, possibly, SCSI completion status and SCSI sense data to 
 *	determine if the next course of action.
 *
 *	If the command completed without error, or the drive recovered
 *	from an internal error then release resources, update the xtranfer 
 *	count, and call biodone() to notify the owner of the buf.
 *
 *	If the command block completed with an SSM/SCSI error then 
 *	assume the problem will not be recoverable and terminate 
 *	the request with EIO.  Likewise if a check condition
 *	occurred and the sense data is questionable.  Otherwise,
 *	attempt a few driver retries of the command before terminating.
 */
void
wdintr(int vector)
{
	wd_t *ip; 
	struct buf *bp;
	struct scsi_cb *cb;
	uint_t level = vector - wd_base_vector;
	struct scrsense *rsense;
	pl_t s;
	uchar_t command;

	ASSERT((level >> WD_LEVEL_SHIFT) < wd_ndevs);
	ip = wd_info[level >> WD_LEVEL_SHIFT];
	ASSERT(ip != 0);
	s = LOCK(ip->wd_lock, pldisk);
	/* LINTED */
	cb = (struct scsi_cb *) ip->wd_cbdp->cb_cbs + 
		(level & WD_LEVEL_CB);
	ASSERT(cb->sw.cb_state & CB_BUSY);
	bp = cb->sw.cb_bp;

	switch(cb->sh.cb_compcode) {
	case SCB_OK:
		if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
			rsense = (struct scrsense *) cb->sw.cb_vsense;
			if (SCSI_RS_ERR_CLASS(rsense) != RS_CLASS_EXTEND
			||  SCSI_RS_ERR_CODE(rsense) != RS_CODE_EXTEND) {
				/* unexpected sense code; don't retry */
				bioerror(bp, EIO);
				bp->b_resid = cb->sh.cb_count;

			} else if ((rsense->rs_key & RS_SENSEKEYMASK) == 
					RS_RECERR) {
				/* 
				 * The drive completed the request, 
				 * but had some difficulty doing so.
				 */
				bp->b_resid = 0; 

			} else if ((int)(cb->sw.cb_errcnt += 1) < wdretrys) {
				/*
				 * The drive errored while executing this
				 * request and was unable to correct it
				 * itself.  Consider retrying a few times.
				 */
				command = (cb->sw.cb_bp->b_flags & B_READ) 
				   ? SCSI_READ : SCSI_WRITE;
				wd_command(cb, command, SCB_IENABLE);
				UNLOCK(ip->wd_lock, s);
				return;

			} else {
				/* Give up - retries don't seem to be working */
				bioerror(bp, EIO);
				bp->b_resid = cb->sh.cb_count;
			}
		} else {	
			bp->b_resid = 0; /* Successful completion */
		}
		break;

	case SCB_BAD_CB:
	case SCB_NO_TARGET:
	case SCB_SCSI_ERR:
	default:
		/* Recovery from this problem is unlikely - kill it */
		bioerror(bp, EIO);
		bp->b_resid = cb->sh.cb_count;
		break;
	}

	/*
	 * This point is reached only if the request is not
	 * being retried.  The buf's completion status and
	 * transfer count should already be updated.  Release
	 * the SSM message block that was being used, try to
	 * start the next queued request (if any), then call
	 * biodone() notify the owner of the buf of its 
	 * completion.
	 */
	free_cb((caddr_t) cb, ip->wd_cbdp);
	/*
	 * Update disk stats
	 */
	met_ds_iodone(ip->wd_stats, bp->b_flags, (bp->b_bcount - bp->b_resid));
	if (ip->wd_bufh->av_forw != NULL)	/* Check for requests */
		wd_startio(ip);
	if (--ip->wd_inuse == 0) {
		DISK_ACTIVITY_LED_OFF;
	}
	/*
	 * Update more disk stats
	 */
	met_ds_hist_stats(ip->wd_stats, bp->b_priv2.un_daddr, NULL, NULL);
	/*
	 * Release lock before calling biodone(); a layered
	 * driver might may call back into this driver.
	 */
	UNLOCK(ip->wd_lock, s);
	biodone(bp);
	return;
}

/*
 * STATIC void
 * wd_rw(struct scsi_cb *cb, buf_t *bp)
 * 	Start a SCSI read or write command and return immediately.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry/exit.
 *
 *	The specified message block, 'cb', must be owned by the caller.
 *
 *	No return value.
 *
 * Remarks:
 * 	Only called the first time a request is made on the buffer;
 * 	retries go through wd_command.  Command completion is 
 *	handled by wdintr().
 */
STATIC void
wd_rw(struct scsi_cb *cb, buf_t *bp)
{
	cb->sw.cb_bp = bp;
	cb->sw.cb_errcnt = 0;
	wd_command(cb, ((bp->b_flags & B_READ) ? SCSI_READ : SCSI_WRITE), 
		SCB_IENABLE);
}

/*
 * STATIC long
 * wd_readc(struct scsi_cb *cb)
 * 	Retrieve the # of usable blocks on the specified SCSI disk drive.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry/exit.
 *
 *	The specified message block, 'cb', must be owned by the caller,
 * 	and have been initialized with the SSM and have and sense 
 *	and data buffers already attached.
 *
 *	Returns the -1 upon failure or the number of addressable
 *	logical blocks (sectors) on the drive upon success.
 *
 * Description:
 *	Call wd_command() to start a SCSI read-capacity command
 *	for the drive specified by the message block, then use
 *	wd_cbfin() poll for completion and retry failures.  If
 *	the command is successfully completed, the last addressable
 *	block is retrieved from the request's data buffer.
 *	Adding one to the last addressable block # yields the
 *	capacity to be returned to the caller.
 *	
 * 	wd_readc is only called when the request is first issued;
 * 	retries and command completion are handled by wd_cbfin().
 */
STATIC long
wd_readc(struct scsi_cb *cb)
{
	struct screadcap *capdat;
	long nblocks = 0;		/* number of blocks on disk */

	cb->sw.cb_errcnt = 0;
	
	wd_command(cb, SCSI_READC, 0);
	if (wd_cbfin(cb) != SCB_OK)
		return (-1);

	/* Extract the returned capacity, correcting the byte order.  */
	capdat = (struct screadcap *)cb->sw.cb_data;
	nblocks = (capdat->rc_nblocks0 << 24) |
		  (capdat->rc_nblocks1 << 16) |
		  (capdat->rc_nblocks2 << 8)  |
		  (capdat->rc_nblocks3) ;
	return (nblocks + 1);
}

/*
 * STATIC void
 * wd_command (struct scsi_cb *cb, uchar_t command, uchar_t flags)
 *	Build a SCSI command in the specified SSM message block (cb)
 *	and notify the SSM to begin executing it.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry/exit.
 *
 *	The caller must have exclusive ownership of the specified CB.
 *
 *	The Scatter-gather/DMA vector must already have been allocated.
 *
 *	The software portion of the message block must be filled in
 *	prior to entry; it will not be changed by this function.
 *
 *	No return value; always starts the command.
 *
 * Remarks:
 *	Uses the specified CB to fill the SCSI command portion in 
 *	based on a template for the command argument passed.  
 *	scb_buf_iovects() is invoked to map scatter gather transfers.
 *
 * 	Commands other than SCSI_READ that need to get data returned 
 *	from the SCSI device will have said data returned in the 
 *	request sense buffer.  Also, use the extended versions of
 *	SCSI_READ and SCSI_WRITE, since non-extended versions cannot
 *	address the entirety of many disk drives today.
 *	
 *	The sc_compcode field must be checked upon command completion
 *	to make sure that the data returned is from the command and 
 *	not request sense data.
 */
STATIC void
wd_command (struct scsi_cb *cb, uchar_t command, uchar_t flags)
{
	buf_t *bp;
	pl_t sx;
	uint_t translength;

	bp = cb->sw.cb_bp;
	bzero((caddr_t) SWBZERO(cb), SWBZERO_SIZE);

	switch (command) {

	case SCSI_TEST:
		cb->sh.cb_count = SCSI_L_TEST;
		cb->sh.cb_cmd = SCB_READ | flags;
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT; 
		cb->sh.cb_clen = SCSI_CMD6SZ;
		break;

	case SCSI_INQUIRY:
		cb->sh.cb_count = INQ_LEN_DEV;
		cb->sh.cb_addr = vtop((caddr_t)cb->sw.cb_data, NULL);
		cb->sh.cb_cmd = SCB_READ | flags;
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT;
		cb->sh.cb_scmd[4] = INQ_LEN_DEV;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		break;

	case SCSI_READC:
		cb->sh.cb_count = SCSI_L_READC;
		cb->sh.cb_addr = vtop((caddr_t)cb->sw.cb_data, NULL);
		cb->sh.cb_cmd = SCB_READ | flags;
		/*
		 * PMI 0
		 * Returns capacity of entire disk
		 */
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT; 
		cb->sh.cb_clen  = SCSI_CMD10SZ;
		break;

	case SCSI_REZERO:
		cb->sh.cb_cmd = SCB_READ | flags;
		cb->sh.cb_scmd[0] = command;
		cb->sh.cb_scmd[1] |= cb->sw.cb_scsi_device << SCSI_LUNSHFT;
		cb->sh.cb_clen = SCSI_CMD6SZ;
		break;

	case SCSI_READ:
	case SCSI_WRITE:
		if ((cb->sh.cb_addr = scb_buf_iovects(bp, cb->sw.cb_iovstart))
		    == NULL) {
			/*
			 *+ The wd driver was unable to perform
			 *+ DMA mapping for a requested data
			 *+ transfer.  Either the transfer length
			 *+ or the alignment exceeds the resources for
			 *+ this device.
			 */
			cmn_err(CE_PANIC,
				"wd%d:  wd_command: scb_buf iovects error",
				SCSID_DRIVE(bp->b_edev));	
		}
		cb->sh.cb_iovec = (ulong_t *)
				vtop((caddr_t)cb->sw.cb_iovstart, NULL);
		cb->sh.cb_count = roundup(bp->b_bcount, (ulong_t) DEV_BSIZE);
		cb->sh.cb_cmd = (command == SCSI_READ) ? SCB_READ : SCB_WRITE;
		cb->sh.cb_cmd |= flags;

		translength = (uint_t) howmany(bp->b_bcount, DEV_BSIZE);

		if (command == SCSI_READ)
			cb->sh.cb_scmd[0] = SCSI_READ_EXTENDED;
		else
			cb->sh.cb_scmd[0] = SCSI_WRITE_EXTENDED;
		cb->sh.cb_scmd[1] = cb->sw.cb_scsi_device << SCSI_LUNSHFT;

		/*
		 * We default the DPO and FUA bits to zero.
		 * They could be set here if a decision was
		 * made to change this.
		 */
		cb->sh.cb_scmd[2] = (uchar_t) (bp->b_priv2.un_daddr >> 24);
		cb->sh.cb_scmd[3] = (uchar_t) (bp->b_priv2.un_daddr >> 16);
		cb->sh.cb_scmd[4] = (uchar_t) (bp->b_priv2.un_daddr >> 8);
		cb->sh.cb_scmd[5] = (uchar_t) bp->b_priv2.un_daddr;
		cb->sh.cb_scmd[6] = (uchar_t) 0;
		cb->sh.cb_scmd[7] = (uchar_t) (translength >> 8);
		cb->sh.cb_scmd[8] = (uchar_t) translength;
		cb->sh.cb_scmd[9] = (uchar_t) 0;
		cb->sh.cb_clen = SCSI_CMD10SZ;
		break;

	default:
		/*
		 *+ The wd driver attempted to generate a SCSI command
		 *+ that it does not support.  This is a driver internal error.
		 */
		cmn_err(CE_WARN, "wd%d:  wd_command: unsupported SCSI command %d",
			SCSID_DRIVE(bp->b_edev), command);
		return;
	}

	/*
	 * Start execution of the SSM SCSI command block by
 	 * notifying the SSM it is available with an interrupt.
 	 */
	sx = splhi();
	slic_mIntr(cb->sw.cb_slic, SCSI_BIN, cb->sw.cb_unit_index);
	splx(sx);
}

/*
 * int
 * wdsize(dev_t dev)
 *	Return the physical size in sectors of the specified minor device.
 *
 * Calling/Exit State:
 * 	Basic locks may not be held upon entry.
 *
 *	Returns the # of sector for the minor device 
 *	upon success, or -1 if the device is invalid.
 *
 * Remarks:
 * 	Doesn't assume that the device is open or alive at all
 * 	Hence there are more parameter checks than are sometimes
 *	really needed.  If the minor device requested is for the
 *	the unpartitioned disk, then return the capacity of the
 *	drive which was determined at bootup.
 *
 *	Otherwise, determine the size of the slice from the contents
 *	of the partition table.  Call wd_getvtoc() to initialize
 *	it if the device is not already open.
 */
int
wdsize(dev_t dev)
{
	int drive;
	wd_t *ip;
	int retval;

	drive = SCSID_DRIVE(dev);
	ip = wd_info[drive];

	if(ip == NULL				/* set in boot? */
	|| drive >= wd_ndevs			/* Binary conf'd? */
	|| !ip->wd_alive)			/* Passed probing? */
		return (-1);

	if (VPART(dev) == V_RAW_MINOR)
		return (ip->wd_capacity);	/* Unpartitioned device size */

	/* 
	 * For a partition on the device, ensure the
	 * VTOC is/has been read first, verify the slice
 	 * requested is valid, then return its size from
	 * the unit's partition table if so.  The VTOC
	 * must be read/validate if the partitioned device 
	 * is not currently open.
	 *
	 * Acquire the unit's sleep lock to mutex
	 * wdopen(), wdclose(), and wdsize() while
	 * the open-counts, state flags, and partition 
	 * tables may be volatile.
	 */
	SLEEP_LOCK(ip->wd_sleep, pridisk);

	if ((ip->wd_nopen == 0 || (ip->wd_flags & WD_RAW) != 0) 
	&&  wd_getvtoc(ip, dev) != 0) {
		retval = -1;		/* Couldn't read/validate the VTOC */
	} else if (ip->wd_p[VPART(dev)].p_start < 0) {
		retval = -1;		/* Invalid slice in the VTOC */
	} else {
		retval = ip->wd_p[VPART(dev)].p_size;	/* Valid slice */
	}

	SLEEP_UNLOCK(ip->wd_sleep);
	return (retval);
}

/* 
 * STATIC int
 * wd_cbfin(volatile struct scsi_cb *cb)
 * 	Polls for completion SSM/SCSI command with error retry.
 *
 * Calling/Exit State:
 *
 *	Basic locks must not be held upon entry.
 *
 *	The caller must own the specified message block,
 *	which must be already have been started by wd_command().
 *
 *	Returns the SCB_OK if the command completes successfully,
 *	even with limited retries.  Otherwise, returns -1.
 */
STATIC int
wd_cbfin(volatile struct scsi_cb *cb)
{
	int error;
	volatile struct scrsense *rsense;

	for (error = 0; ; ) {
		/*
	 	 * Firmware will timeout if commands take to long
		 */
		while (cb->sh.cb_compcode == SCB_BUSY)
			;

		switch(cb->sh.cb_compcode) {
		case SCB_OK:
			if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
				rsense = (volatile struct scrsense *) 
					cb->sw.cb_vsense;
				if (SCSI_RS_ERR_CLASS(rsense) != RS_CLASS_EXTEND
				|| SCSI_RS_ERR_CODE(rsense) != RS_CODE_EXTEND 
				|| (rsense->rs_key & RS_SENSEKEYMASK) 
				   != RS_RECERR)
					break;	/* try again ? */
			}
			return SCB_OK;	/* Completed successfully */

		case SCB_BAD_CB:
		case SCB_NO_TARGET:
		case SCB_SCSI_ERR:
		default:
			break;	/* try again ? */
		}

		if (++error >= wdretrys)
			return -1;	/* Enough retries - its a failure */

		/* Initiate a retry of the same command */
		wd_command((struct scsi_cb *) cb, cb->sh.cb_scmd[0], 0); 
	}
	/*NOTREACHED*/
}

/* 
 * STATIC int
 * wdp_cbfin(volatile struct scsi_cb *cb)
 * 	Polls for completion SSM/SCSI command without error retry.
 *
 * Calling/Exit State:
 *
 *	Basic locks must not be held upon entry.
 *
 *	The caller must own the specified message block,
 *	which must be already have been started by wdp_command().
 *
 *	Returns the SCB_* status of the command upon completion
 *	without interpretation and retries.
 */
STATIC int
wdp_cbfin(volatile struct scsi_cb *cb)
{

	/*
 	 * Firmware will timeout if commands take to long
	 */
	while (cb->sh.cb_compcode == SCB_BUSY)
		;

	if (cb->sh.cb_compcode == SCB_OK && 
	    !SCSI_CHECK_CONDITION(cb->sh.cb_status))
		return (cb->sh.cb_compcode);
	else
		return (cb->sh.cb_compcode);
}

/*
 * STATIC int
 * wdp_command(struct scsi_cb *cb, uchar_t cmd, vaddr_t sense, vaddr_t buffer,
 *	Execute SCSI command to the SSM at boottime and poll for completion..
 *
 * Calling/Exit State:
 *
 *	Basic locks must not be held upon entry.
 *
 *	The caller must own the specified message block.
 *
 *	Returns the SCB_* status of the command upon completion.
 *
 */
STATIC int
wdp_command(struct scsi_cb *cb, uchar_t cmd, vaddr_t sense, vaddr_t buffer,
	uchar_t devno)
{
	cb->sw.cb_vsense = sense;
	cb->sh.cb_sense = (ulong_t) vtop((caddr_t)sense, NULL);
	cb->sh.cb_slen = SIZE_MAXDATA;
	cb->sw.cb_data = buffer;
	cb->sw.cb_scsi_device = devno;
	wd_command(cb, cmd, 0);
	return (wdp_cbfin(cb));
}

/*
 * void
 * wdprint (dev_t dev, char *str)
 *	Display a message through this driver on the console.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry/exit.
 *
 *	no return value.
 *
 * Description:
 *	Not much to do here: Just tag the specified message
 *	with an indication of which device it pertains to
 *	and pass it off to CMN_ERR(). device
 */
void
wdprint (dev_t dev, char *str)
{
	cmn_err(CE_CONT, "%s on scsi disk %d\n", str, SCSID_DRIVE(dev));
}

/*
 * STATIC int
 * wd_vtoc_cksum(struct vtoc *)
 *	Calculate a checksum for the specified VTOC structure.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry/exit.
 *
 *	Returns the calculated checksum value.
 *
 * Description:
 *	Pretty simple: Determine how many longs are in the
 *	VTOC struct passed in and then walk through them
 *	sequentially adding all the longs together without
 *	regard for overflow, etc. Return the result.
 *	This is used to help validate the VTOC.
 */
STATIC int
wd_vtoc_cksum(struct vtoc *v)
{
	long sum;
	int  nelem = sizeof(struct vtoc) / sizeof(long);
	long *lptr = (long *)v;

	for (sum = 0; nelem > 0; nelem--, lptr++) {
		sum += *lptr;
	}
	return (sum);
}

/*
 * STATIC int
 * wd_getvtoc(wd_t *, dev_t)
 *	Retrieve and validate the device's VTOC.
 *
 * Calling/Exit State:
 *	Basic locks may not be held upon entry/exit.
 *
 *	Returns a non-zero errno upon failure.
 *	Otherwise, the wd_t's partition table is
 *	initialized with validated slice information
 *	and zero is returned.
 *
 * Description:
 *	Allocates a buf header and a kernal buffer to
 * 	read the VTOC into from the disk in a standard
 *	sector location.  Once read, verify that the
 *	VTOC looks reasonable and checksums correctly.
 *	If all looks well, initialize the partition table
 *	with relevant data from the VTOC, prior
 *	to releasing allocated resources and returning.	
 */
STATIC int
wd_getvtoc(wd_t *ip, dev_t dev)
{
	struct vtoc *vp;
	struct partition *pp;
	struct partt *ipp;
	buf_t *bp;
	long i, maxsec;
	ulong cksum;
	int error;
	
	/* First aquire a buf and read the vtoc into it */
	bp = getrbuf(KM_SLEEP);
	bp->av_forw = bp->av_back = NULL;
	vp = (struct vtoc *)kmem_zalloc(V_SIZE, KM_SLEEP);

	bp->b_un.b_addr = (caddr_t)vp;
	bp->b_blkno = V_VTOCSEC;
	bp->b_resid = bp->b_bcount = V_SIZE;
	bzero(bp->b_un.b_addr, V_SIZE);
	bp->b_edev = dev | V_RAW_MINOR;
	bp->b_flags = bp->b_flags | B_READ;
	bioerror(bp, 0);

	wdstrategy(bp);
	biowait(bp);

	if ((error = geterror(bp)) == 0) {
		/* Validate the vtoc header */
		cksum = vp->v_cksum;
		vp->v_cksum = 0;
		if (vp->v_sanity != VTOC_SANE) {
			error = EINVAL;
		} else if (vp->v_version != V_VERSION_1) {
			error = EINVAL;
		} else if (wd_vtoc_cksum(vp) != cksum) {
			error = EINVAL;
		} else {
			/* Now validate the vtoc slice information */
			maxsec = (long)ip->wd_capacity;
			pp = vp->v_part;
			i = vp->v_nparts;
			for (; i > 0; i--) {
				if (pp->p_type != V_NOPART 
				&&  pp->p_type != V_RESERVED
				&&  pp->p_start + pp->p_size > maxsec) {
					error = EINVAL;
					break;
				}
				pp++;
			}

			if (error == 0) {
				/* Valid vtoc; save the relevant information */
				pp = vp->v_part;
				ipp = &ip->wd_p[0];
				for (i = 0; i < V_NUMPAR; i++, ipp++, pp++) {
					if ((ushort_t)i >= vp->v_nparts
					||  pp->p_type == V_NOPART 
					||  pp->p_type == V_RESERVED) {
						/* Mark slice as unusable */
						ipp->p_start = -1;
						ipp->p_size = 0;
					} else {
						/* Init valid slice info */
						ipp->p_start = pp->p_start;
						ipp->p_size = pp->p_size;
					}
					ipp->p_flags = 0;
					ipp->p_nopen = 0;
				}
			}
		}
	}
	kmem_free(vp, V_SIZE);
	freerbuf(bp);
	return (error);
}
