/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/tm/tm.c	1.5"
#ident	"$Header: $"

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * tm.c
 *	Streamer SCSI tape driver for the System's Services Module (SSM)
 */

#include <util/types.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>
#include <util/ipl.h>
#include <mem/kmem.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <sys/open.h>
#include <fs/file.h>
#include <io/uio.h>
#include <fs/buf.h>
#include <io/conf.h>
#include <io/slicreg.h>
#include <io/ssm/ssm_scsi.h>
#include <io/scsi.h>
#include <io/ssm/ssm_misc.h>
#include <io/ssm/ssm.h>
#include <io/clkarb.h>
#include <io/autoconf.h>
#include <io/tm/tm.h>
#include <proc/cred.h>

#include <io/ddi.h>	/* must come last */

/*
 * A physical devices' unit # corresponds to its
 * SCSI target adapter address on the SSM/SCSI bus.
 * (only embedded drives are supported).
 */
#define TM_MAX_UNITS	(SCSI_MAX_TARGET + 1)

/*
 * block size defines for the various QIC drives
 */
#define TM_SMALL_BLOCK          512
#define TM_LARGE_BLOCK          1024
#define TM_VARIABLE_BLOCK       0

#define TM_PL		pldisk		/* Priority for LOCK aquisition */

#define TM_CB_ACTIVE	1		/* for cb->sw.cb_state to indicate that
					 * this driver has not yet terminated
					 * this request. */
#define TM_CB_RDY	0		/* CB ready for next request */

/*
 * Define to set the write and read thresholds for the 
 * 3840 QIC drive. Range is 1..127, drive default
 * is 16 (page 12-7 of 3800 SCSI-1 manual)
 * Set it higher for a better system throughtput
 */
#define TDC3840_RW_THRESHOLD	40

/* 
 * Drive dependent information for locating
 * supported drives and drive dependent 
 * functions/data for managing them.  
 */
typedef const struct tm_drive_info {
	char	*vendor;		/* Vendor i.d. string */
	char	*product;		/* Product i.d. string */
	unchar	slen;			/* Amount of request sense buffer
					 * to use for drive type */
	unchar	mlen;			/* Amount of mode select buffer
					 * to use for drive type */
	unchar	xfer_mode;		/* Synchronous vs. Asynchronous SCSI */
	uint	max_xfer;		/* Kbyte size of max supported xfer */
	int	(*open)(); 		/* drive dependent open routine */
	int	(*set_density)(); 	/* drive dependent routine to set new
					 * density for the drive */
} tmdrive_t;

/* Values for tm_drive_info.xfer_mode */
#define TM_SYNC        1               /* Synchronous bus transfers */
#define TM_ASYNC       0               /* Asynchronous bus transfers */

/*
 * Tandberg vendor unique parameters for SCSI commands
 *
 * VENDOR UNIQUE block for MODE SENSE
 */
struct tandberg_vendor_ms {
	unchar	page_code;		/* Vendor unique page code == 0 */
	unchar	write_threshold;	/* # blocks in buffer bef tape motion */
	unchar	read_threshold;		/* # blocks free before tape motion */
	unchar	buffer_size;		/* IGNORED on TDC3840 */
	unchar	forced_streaming_cnt[2];/* forces streaming operation */
	unchar	bus_threshold;		/* blks ready before reconnection */
	unchar	copy_threshold;		/* copy command threshold */
	unchar	normal_sense_threshold;	/* IGNORED on TDC3840 */
	unchar	copy_sense_threshold;	/* for COPY use */
	unchar	load_function;		/* auto load or auto retension */
	unchar	powerup_delay;		/* auto load/restension delay */
	unchar	control_flags;		/* TDC3840: misc control flags */
};

#define TBERG_3840_BSIZE	127	/* Size of FIFO buffer on tdc3840 */

#define TBERG_AUTO_LOAD		0
#define TBERG_AUTO_RETENSION	1
#define TBERG_NO_AUTO_EITHER	2	/* TDC3840 only */

/* tandberg_vendor_ms.control_flags */
#define TBERG_AUTOREAD			0x08
#define TBERG_DADS			0x04   /* disable auto density select */
#define	TBERG_DISABLE_CORRECTION	0x02
#define TBERG_FAST			0x01   /* FAST space mode enable */

/* MODE SENSE format */
struct tm_mode_sense {
        struct scmode_hdr t_hdr;        	/* Parameter list header */
        struct scmode_blkd t_bd;        	/* Block descriptor list */
	struct tandberg_vendor_ms vendor;	/* tandberg's vendor unique */
};

/*
 *  t_hdr.mh_mtype
 *	The cartridge that currently is in the drive will be returned
 *	in this field will the following possible values
 */
#define	DC300		0		/* or DC300XLP */
#define	DC600A		1		/* or DC615 */
#define DC6150		2		/* or DC6037 or DC6250 */
#define DC6525		3		/* or DC6320 */

/* Incompatible medium error from REQUEST SENSE FRU code */
#define FRU_INCOMPATIBLE_MEDIA	0x10

/* Tandberg vendor unique for REQUEST SENSE */
struct tandberg_vendor_rs {
	unchar	src_sense_ptr;			/* source sense ptr: COPY */
	unchar  dest_sense_ptr;			/* dest sense ptr: COPY */
	unchar  ECC_correction_cnt[2];		/* TDC3840 only */
	unchar	recoverable_error_cnt[2];	/* counter */
	unchar	ercl_ercd;			/* ERCL/ERCD field */
	unchar	fru;				/* FRU: extended error code */
	unchar	block_cnt[3];			/* block counter */
	unchar	fm_cnt[2];			/* File Mark counter */
	unchar	underrun_cnt[2];		/* Underrun counter */
	unchar	marginal_cnt;			/* TDC3620 only */
	unchar	remaining_cnt;			/* TDC3620 only */
	unchar	srcdest_status;			/* used by COPY */
	unchar  srcdest_sense_byte[1];		/* used by COPY. could be
						 *  N bytes long. */
};

/* REQUEST SENSE data format */
struct tm_req_sense {
	struct scrsense	rsense;			/* standard header */
	struct tandberg_vendor_rs vendor;	/* vendor unique */
};

/*
 * Information structure definition.
 * One of these per device.
 */
typedef struct	tm_info {
	tmdrive_t *tm_dp;		/* Associated record in drive table */
	volatile uchar_t tm_open;	/* State of driver availability */
	volatile uchar_t tm_pos;	/* State of current medium possition */
	volatile uchar_t tm_amode;	/* State of current medium access */
	volatile uchar_t tm_mflags;	/* Misc state flags for driver */
	struct scsi_cb 	  *tm_cbs;	/* Addr of NCBPERSCSI CBs per device */
	caddr_t tm_buffer;		/* Address of I/O buffer */
	int	tm_blksz;		/* Size for fixed blocks, zero for 
					 * variable length block mode */
	int	tm_key;			/* Request sense key for reporting */
	long	tm_resid;		/* Request sense residual data length */
	char 	*tm_msg;		/* Request Sense error message */
	char	*tm_errmsg;		/* For reporting errors to user */
	lock_t	*tm_lock;		/* Lock access to info structure */
	sleep_t *tm_usync;		/* Sync. system call entries */
	sv_t	*tm_cwait;		/* Sync. for SCSI command completion */
} tm_t;

/* Values taken by tm_t.tm_open  */
#define	TMO_OPEN	2		/* Device is open to a process */
#define	TMO_ERR		1		/* Device is open, but had a fatal
					 * error; must now be closed */
#define	TMO_CLOSED	0		/* Device is not open */

/* Values taken by tm_t.tm_pos  */
#define	TMP_BOT		3		/* Positioned at beginning of medium. */
#define	TMP_EW		2		/* Positioned at or past EOM early-
					 * warning, but not at physical EOM */
#define	TMP_EOM		1		/* Positioned at end of medium */
#define	TMP_OTHER	0		/* Medium positioned elsewhere */

/* Values taken by tm_t.tm_amode  */
#define	TMA_LASTIOR	5		/* Last I/O was a READ.  May skip
					 * past EOF upon closing. */
#define	TMA_EOF		3		/* A filemark has been read, but not 
					 * yet reported as read. */
#define	TMA_LASTIOW	2		/* Last I/O was a WRITE. Must still
					 * write a filemark upon closing. Also,
					 * this is the end of recorded data. */
#define	TMA_EOD		1		/* Positioned at end of recorded data.
					 * Like TMA_LASTIOW, but a filemark 
					 * need not be written upon closing. */
#define	TMA_OTHER	0		/* Either reading in the middle of the
					 * tape, or idle at BOT. */


/* Misc state flags for tm_t.tm_mflags. */
#define	TMF_ATTEN	0x01		/* Unit needs attention */
#define	TMF_FAIL	0x02		/* Last operation failed */
#define	TMF_ABORTED	0x04		/* The user signaled out of the
					 * last operation, but it has yet
					 * to be completed.  */
#define TMF_VERBOSE	0xF0		/* Mask for settable verbosity flags,
					 * from tm.h */

/*
 * Forward references.
 */
void tmintr(int);
STATIC int tm_load(void);
STATIC int tm_unload(void);
STATIC void tmstrategy(buf_t *);
STATIC int tm_gen_scsi_cmd(uchar_t, tm_t *, int, uint);
STATIC int tm_raw_read(tm_t *, buf_t *);
STATIC int tm_raw_write(tm_t *, buf_t *);
STATIC tmdrive_t *tm_probe(const uchar_t);
STATIC void tm_report_sense_info(tm_t *, volatile struct scsi_cb *, int, caddr_t);
STATIC int tm_tdc3600_open(tm_t *, const dev_t, const int);
STATIC int tm_tdc3840_open(tm_t *, const dev_t, const int);
STATIC int tm_tdc3600_set_density(tm_t *, const int);
STATIC int tm_tdc3840_set_density(tm_t *, const int);

/*
 * Variables shared within this module.
 */
STATIC struct tm_info *tm_info[TM_MAX_UNITS];
STATIC int tm_ndevs;
STATIC int tm_basevec;			/* SLIC interrupt vector for tm0, CB0 */
STATIC ulong_t tm_psize;		/* One-time pagesize computation */
STATIC LKINFO_DECL(tmlockinfo, "ID:tm:tm_lock", 0);
STATIC LKINFO_DECL(tmsleep_lkinfo, "IO:tm:tm_usync", 0);
STATIC int tm_alloc_wait = KM_NOSLEEP;	/* Switch for whether we can wait during
					 * memory allocation calls.  Can't wait
					 * during static initialization, but may
					 * during dynamic initialization. */
 
STATIC tmdrive_t tm_drives[] = {
        /*
         * Vendor i.d., 
	 * Product i.d., 
	 * Request-sense buffer length in bytes, 
	 * Mode-select/sense buffer length in bytes, 
	 * Asynchronous vs. Synchronous SCSI bus transfer mode,
 	 * Kbyte size of maximum supported individual transfer,
	 * drive dependent support functions
         */
        {  
	   /* 
	    *  Tandberg 3600 series:
	    *		3620:  reads and writes QIC-24 and QIC-11(9-track),
	    *		3640:  reads QIC-120/24/11 and writes QIC-120,
	    *		3660:  reads QIC-150/120/24/11 and writes QIC-150/120,
	    */
	   "TANDBERG",  
	   " TDC 3600", 		
	   sizeof(struct tm_req_sense),
           sizeof(struct tm_mode_sense),
           TM_ASYNC,
	   128,
	   tm_tdc3600_open,
	   tm_tdc3600_set_density,
        },

        {  
	   /* 
	    * Tandberg 3840. Reads and writes QIC-525, QIC-150, 
	    * and QIC-120.  Also reads QIC-24 and QIC-11.
	    */
	   "TANDBERG",  
           " TDC 3800",			
           sizeof(struct tm_req_sense),
           sizeof(struct tm_mode_sense),
           TM_SYNC,
	   128,
	   tm_tdc3840_open,
	   tm_tdc3840_set_density,
        },

};

STATIC int tm_num_drive_types = sizeof (tm_drives) / sizeof (tmdrive_t);

/*
 * The following arrays are for producing 
 * messages summarizing a SCSI command
 * termination and noting check conditions.
 */
STATIC char *tm_scsi_errors[] = {
	"No Sense",
	"Recovered Error",
	"Not Ready",
	"Medium Error",
	"Hardware Error",
	"Illegal Request",
	"Unit Attention",
	"Data Protect",
	"Blank Check",
	"Vendor Unique",
	"Copy Aborted",
	"Aborted Command",
	"Equal",
	"Volume Overflow",
	"Miscompare",
	"Reserved Key",
};

STATIC char *tm_scsi_commands[] = {
	"Test Unit Ready",
	"Rezero/Rewind Unit",
	"Retension",
	"Request Sense",
	"Format Unit",
	"Read Block Limits",
	"0x06",
	"Reassign Blocks",
	"Read",
	"0x09",
	"Write",
	"Seek/Track Select",
	"0x0c",
	"0x0d",
	"0x0e",
	"Read Reverse",
	"Write Filemarks",
	"Space",
	"Inquiry", 
	"Verify",
	"Recover Buffered Data",
	"Mode Select",
	"Reserve Unit",
	"Release Unit",
	"Copy",
	"Erase", 
	"Mode Sense",
	"Start/Stop or Load/Unload Unit",
	"Recieve Diagnostic Results/Mode Select",
	"Send Diagnostic Results", 
	"Prevent/Allow Medium Removal", 
	"0x1f", 
	"0x20",
	"0x21",
	"0x22", 
	"0x23", 
	"0x24", 
	"Read Capacity", 
	"0x26", 
	"0x27", 
	"Read (extended)", 
	"0x29", 
	"Write (extended)", 
	"Seek (extended)", 
	"0x2c", 
	"0x2d", 
	"Write and Verify", 
	"Verify", 
	"Search Data High",
	"Search Data Equal", 
	"Search Data Low", 
	"Set Limits", 
	"0x34", 
	"0x35", 
	"0x36", 
	"0x37", 
	"0x38", 
	"Compare", 
	"Copy and Verify"
};

/*
 * Global variables defined or used within this module.
 */
int tmdevflag = D_MP | D_NOBRKUP | D_DMA;

/*
 * void
 * tminit(void)
 *	Locate and initialize embedded SSM/SCSI streamer tape drives.
 *
 * Calling/Exit State:
 *	Caller may not hold basic locks upon entry/exit.
 *
 *	Invoked during either system bootup or a dynamic driver load,
 *	in which case there should be no locking required within this
 *	function, including calls to the SSM initialization services.  
 *
 *	Initializes the tm_info table, indexes into which reflect each 
 *	device's target address on the primary SSM's SCSI bus.  Valid 
 *	entries are given non-NULL tm_t pointers in this table.
 *
 *	tm_ndevs is set to the largest of the available drive 
 *	unit numbers + 1.
 *
 *	tm_basevec is set to the first in the sequence of SLIC 
 *	interrupt vectors allocated to this driver's devices.
 *
 *	No return value.
 *
 * Description:
 *	For each of the possible target adapter locations on the
 *	primary SSM SCSI bus, invoke tm_probe() to determine if
 *	a supported tape is present.  If so, allocate and initialize 
 *	permanent SSM resources for message passing its SCSI commands 
 *	to the SSM.  Also initialize its tm_t structure in the tm_info
 *	table, which marks it as usable.  Otherwise leave its entry NULL.
 *
 *	Logical drive unit numbers are the same as their target
 *	adapter location on the SCSI bus.  Sequential interrupt vectors 
 *	(1 per CB) are reserved for all possible devices so corresponding
 *	drive units can be calculated easily by tmintr().
 *
 * Remarks:
 *	Since this driver will only use 1 CB per device, only allocate
 *	sense buffers, DMA lists, etc. for CB[0] of the device.
 *
 *	This is intended to support embedded SCSI drives only.
 */
void
tminit(void)
{
	tm_t *tp;
	volatile struct scsi_cb *cb;
	tmdrive_t *drive_p;
	struct scb_init sinit;
	int i, unit;

        tm_ndevs = 0;
	tm_psize = ptob(1);
        tm_basevec = ivec_alloc_group(tm_global.bin, TM_MAX_UNITS * NCBPERSCSI);
	if (tm_basevec < 0) {
		/* 
		 *+ The tm-driver was unable to allocate a series of
		 *+ SLICbus interrupt vectors for use with its drives.
		 *+ Therefore, the driver has been deconfigured and its
		 *+ devices remain unusable.  Reconfigure the system so
		 *+ fewer devices are attempting to map their devices to
		 *+ the SLIC interrupt bin (level) specified.
		 */
		cmn_err(CE_WARN, 
		"tm-driver deconfigured; bin %d interrupt allocation failed",
			tm_global.bin); 
		return;	
	}

        for (unit = 0; unit < TM_MAX_UNITS; unit++) {
                if ((drive_p = tm_probe(unit)) == NULL)
                        continue;       /* Drive not found in that location */

                cmn_err(CE_CONT,
                        "tm %d found on ssm 0 target %d\n", unit, unit);

                /*
                 * Allocate per-device info struct, then 
		 * initialize portions subject failure
		 * first to simplify recovery.
                 */
		sinit.si_id = 0;	/* determines cmn_err output on fail */
                if (! (tp = (tm_t *)kmem_zalloc(sizeof(tm_t), tm_alloc_wait)))
			break;
		/* initialize locks, etc */
		if (! (tp->tm_lock = LOCK_ALLOC(2, TM_PL, 
						&tmlockinfo, tm_alloc_wait)))
			break;
		if (! (tp->tm_usync = 
				SLEEP_ALLOC(0, &tmsleep_lkinfo, tm_alloc_wait)))
			break;
		if (! (tp->tm_cwait = SV_ALLOC(tm_alloc_wait)))
			break;

		/* 
  		 * Pass device parameters to the SSM in
		 * return for allocating CBs and a unique
		 * i.d. for communicating with it.
		 */
		ASSERT(drive_p->xfer_mode == TM_SYNC || 
			drive_p->xfer_mode == TM_ASYNC);
		sinit.si_mode = SCB_BOOT;
		sinit.si_flags = drive_p->xfer_mode;
		sinit.si_scsi = 0;	/* Only one scsi bus for now */
		sinit.si_target = (uchar_t)unit;
		sinit.si_lunit = 0;
		sinit.si_host_bin = (uchar_t)tm_global.bin;
		sinit.si_host_basevec = tm_basevec + unit * NCBPERSCSI; 
		sinit.si_control = 0;

		init_ssm_scsi_dev(&sinit);
		if (sinit.si_id < 0)
			break;		/* Don't continue with other units */
        	ASSERT(sinit.si_cbs);
		tp->tm_cbs = sinit.si_cbs;

		/*
	 	 * Finish one-shot initialization of CB[0]:
		 * sense buffer, DMA list, etc. 
		 */
		cb = &tp->tm_cbs[0];
		cb->sw.cb_iovnum = ((drive_p->max_xfer * 1024) / ptob(1)) + 1;
		cb->sw.cb_iovstart = (ulong *)
			ssm_scsi_alloc(cb->sw.cb_iovnum * sizeof (ulong),
				tm_alloc_wait);
       		if (cb->sw.cb_iovstart == NULL)
			break;		/* Don't continue with other units */
       		cb->sw.cb_iovpaddr = 
			vtop((caddr_t)(void *)cb->sw.cb_iovstart, NULL);
       		cb->sh.cb_slen = (drive_p->slen < sizeof (struct scinq_dev)) ?
			sizeof (struct scinq_dev) : drive_p->slen;
       		cb->sw.cb_vsense = (vaddr_t) ssm_scsi_alloc(cb->sh.cb_slen,
					tm_alloc_wait);
       		if (cb->sw.cb_vsense == NULL)
			break;		/* Don't continue with other units */
       		cb->sh.cb_sense = vtop((caddr_t)(void *)cb->sw.cb_vsense, NULL);
       		cb->sw.cb_slic = sinit.si_ssm_slic;
       		cb->sw.cb_unit_index = SCVEC(sinit.si_id, 0);
		cb->sw.cb_scsi_device = SSM_SCSI_DEVNO(unit, 0);

		/*
		 * Initialize the remainder of the info 
		 * structure, marking drive as alive.
		 */
		tp->tm_dp = drive_p;
		tp->tm_open = TMO_CLOSED;
		tp->tm_pos = TMP_OTHER;
		tp->tm_amode = TMA_OTHER;
		tp->tm_mflags = TMF_ATTEN;	
		tp->tm_cbs = sinit.si_cbs;	/* Base CB for device. */
		for (i = 0; i < NCBPERSCSI; i++) {
                	ivec_init(tm_global.bin, 
				tm_basevec + (unit * NCBPERSCSI) + i, tmintr);
		}
                tm_info[unit] = tp;		/* Device now available */
		tm_ndevs = unit + 1;
	}		/* End of per device configuration/initialization */

	if (tm_ndevs == 0) {
		/* Then release unneeded interrupt vectors */
		ivec_free_group(tm_global.bin, tm_basevec, 
			TM_MAX_UNITS * NCBPERSCSI);
		tm_basevec = -1;
	}
        if (unit == (uchar_t)TM_MAX_UNITS) 
		return;			/* Normal exit - no failures */

	/* Otherwise, the loop above was aborted - cleanup and exit */
	if(tp != NULL) {
		if (tp->tm_lock != NULL)
			LOCK_DEALLOC(tp->tm_lock);
		if (tp->tm_usync != NULL)
			SLEEP_DEALLOC(tp->tm_usync);
		if (tp->tm_cwait != NULL)
			SV_DEALLOC(tp->tm_cwait);
               	kmem_free(tp, sizeof (tm_t)); 
	}
	if (sinit.si_id >= 0) {
		if (cb->sw.cb_iovstart != NULL)
			ssm_scsi_free(cb->sw.cb_iovstart, 
					cb->sw.cb_iovnum * sizeof (ulong));
		if (cb->sw.cb_vsense != NULL)
			ssm_scsi_free((void *)cb->sw.cb_vsense, cb->sh.cb_slen);

		deinit_ssm_scsi_dev(&sinit);
	 	/*
 		 *+ Kernel memory for initializing the specified device
 		 *+ could not be allocated, so the specified device
		 *+ has been deconfigured.   Reconfigure the system
 		 *+ to consume less memory to make this device usable 
		 *+ in the future.
 		 */
 		cmn_err(CE_WARN, 
			"tm%d: device init. failed; deconfigured", unit);
	} else {
		/*
		 *+ There are more devices configured on the
		 *+ SCSI bus than the SSM can handle, so the
		 *+ specified device has been deconfigured.
		 *+ Remove excess devices from that bus in order
		 *+ to make this device usable in the future.
		 */
		cmn_err(CE_WARN,
			"tm%d: SSM/SCSI init. failed; deconfigured", unit);
	}
}

/*
 * STATIC int
 * tm_load(void)
 *	Configure embedded SSM/SCSI streamer tape drives
 *	during dynamic loading.
 *
 * Calling/Exit State:
 *      Caller may not hold basic locks upon entry/exit.
 *
 *	Returns zero if successfull, otherwise returns an errno.
 *
 * Description:
 *	Invoke tminit() to locate and configure drives.  If
 *	no drives were configured, fail this request.  Prior
 *	to calling tminit(), set tm_alloc_wait to KM_SLEEP so
 *	tminit's memory allocations will always succeed, even if
 *	they must wait for memory to be released.
 */
STATIC int
tm_load(void)
{
	int unit;

#ifdef DEBUG
        for (unit = 0; unit < TM_MAX_UNITS; unit++) {
		ASSERT(tm_info[unit] == NULL);
	}
#endif /* DEBUG */

	tm_alloc_wait = KM_SLEEP;
	tminit();
	if (tm_ndevs == 0)
		return (ENODEV);	/* no devices were configured */
	return (0);
}

/*
 * STATIC int
 * tm_unload(void)
 *	De-initialize driver support for embedded SSM/SCSI streamer tape drives.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Deallocates per device data structures from the tm_info table, 
 *	indexes into which reflect each device's target address on the 
 *	primary SSM's SCSI bus and their logical unit number.  Afterwards, 
 *	all entries will be NULL pointers.  Also deallocates previously 
 *	allocated SLIC interrupts for those devices.
 *
 *	Returns zero if successful; returns an errno if it fails.
 *
 * Description:
 *	Step throught the driver unit' information table
 *	and deinitialize any units previously configured.  
 *
 * 	For each configured unit, first wait for an "aborted" SCSI 
 *	command to complete, if necessary.  "aborted" means a system 
 *	call was interrupted/killed, although the execution of its 
 *	associated SCSI/CB could not be terminated.
 *
 *	Once the unit has been quiesced, then break the association 
 *	previously established with the SSM for it and release message 
 *	block resources.  Since deinit_ssm_scsi_dev() releases the CBs,
 * 	clean them up prior to calling it; deallocate DMA/scatter-gather
 *	lists and request sense buffers previously allocated to them.
 *	Afterwards, release the unit's sync. primatives and its 
 *	information structure.
 *
 *	Finally, after all the units have been deconfigured, release 
 *	SLIC interrupt resources the driver was using and return.
 *
 * Remarks:
 *	Since this driver will only use 1 CB per device, only deallocate
 *	sense buffers, DMA lists, etc. for CB[0] of the device.
 */
STATIC int
tm_unload(void)
{
	tm_t *tp;
	volatile struct scsi_cb *cb;
	struct scb_init sinit;
	pl_t saved_pl;
	int unit;

	ASSERT(tm_ndevs != 0);

        for (unit = 0; unit < tm_ndevs; unit++) {
		if ((tp = tm_info[unit]) == NULL)
			continue;	/* Device is not configured */

		ASSERT(tp->tm_lock != NULL);
		ASSERT(tp->tm_usync != NULL);
		ASSERT(tp->tm_cwait != NULL);
		ASSERT(tp->tm_cbs != NULL);
		ASSERT(SLEEP_LOCKAVAIL(tp->tm_usync) == TRUE);
		ASSERT(tp->tm_open == TMO_CLOSED);

		/* Wait for any previously aborted operations to complete */
		cb = &tp->tm_cbs[0];
		saved_pl = LOCK(tp->tm_lock, TM_PL);
		if ((tp->tm_mflags & TMF_ABORTED) != 0) {
			SV_WAIT(tp->tm_cwait, pritape, tp->tm_lock); 
			ASSERT(cb->sw.cb_state != TM_CB_ACTIVE);

		} else {
			UNLOCK(tp->tm_lock, saved_pl);
		}

		/* Break the SSM association */
		ASSERT(cb->sw.cb_iovstart != NULL);
		ASSERT(cb->sw.cb_vsense != NULL);
		ssm_scsi_free(cb->sw.cb_iovstart, 
			cb->sw.cb_iovnum * sizeof (ulong));
		ssm_scsi_free((void *)cb->sw.cb_vsense, cb->sh.cb_slen);
		sinit.si_scsi = 0;
		sinit.si_target = (uchar_t)unit;
		sinit.si_lunit = 0;
		sinit.si_id = SCVEC_ID(cb->sw.cb_unit_index);
		sinit.si_cbs = tp->tm_cbs;
		deinit_ssm_scsi_dev(&sinit);

		/* Release the sync. primatives and info. struct.  */
		LOCK_DEALLOC(tp->tm_lock);
		SLEEP_DEALLOC(tp->tm_usync);
		SV_DEALLOC(tp->tm_cwait);
               	kmem_free(tp, sizeof (tm_t)); 
		tm_info[unit] = NULL;

	}		/* End of per unit de-configuration */

	/* Lastly, release the SLIC interrupt resources the driver used */
	ASSERT(tm_basevec >= 0);
        ivec_free_group(tm_global.bin, tm_basevec, TM_MAX_UNITS * NCBPERSCSI);
	tm_basevec = -1;
        tm_ndevs = 0;
	return (0);
}

/*
 * int
 * tmopen(dev_t *, int, int, cred_t *)
 * 	Open a tape drive for I/O.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Medium must be installed in the drive and is
 *	generally assumed to be positioned where desired.
 *
 *	Returns zero if the open completes successfully.
 *	Otherwise returns: 
 *		ENXIO if the device number does not 
 *		reference an available tape drive.
 *
 *		EBUSY if the minor number cannot be accessed
 *		at this time, such as when it is already open
 *		(since its an exclusive access device).
 *
 *		EAGAIN if the open would block and a non-blocking
 *		open had been specified as per the oflag argument.
 *
 *		EINTR if any of the delays that occur in the course
 *		of opening the device are signalled out.
 *
 *		EIO if medium is not installed.
 *
 *		Other errnos as returned by drive dependent open().
 *
 * Description:
 *	First confirm that the minor device number pertains to a
 *	valid/active tape device.  Then acquire the unit's sleep lock
 *	to mutex syscalls to the unit, but don't wait for it if this
 *	is a non-blocking open or we get signalled out.  Since this
 *	is always an 'exclusive open' device, determine if we now 
 *	own it; bail out otherwise.  If successful, allow any remaining
 *	operation to complete that was started but signalled out;
 *	again, don't hold up non-blocking opens.
 *
 * 	At this point, the device should be quiescent and we own it.  
 *	Once the OPEN flag is set, the SLEEP lock is not needed to 
 *	block another open, so drop the mutex.  Finish the open by
 *	confirming that the drive has medium loaded, finish setting
 *	state, and then invoke a device dependent function to validate
 *	the open access modes, density selectors, etc.
 *
 * Remarks:
 * 	The minor device number contains unused bits, NOREWIND and 
 * 	RETENSION flags, a density selector, and unit number.
 */
/*ARGSUSED*/
int
tmopen(dev_t *dev, int oflag, int otyp, cred_t *crp)
{
	int unit = MT_UNIT(*dev) ;
	tm_t *tp = tm_info[unit];
	int status;
	pl_t saved_pl;

	ASSERT(otyp != OTYP_BLK);

	if (unit >= tm_ndevs || tp == NULL)
		return (ENXIO);		/* Device is not available */

	/* Obtain exclusive access to the device. */
	if ((oflag & (FNDELAY | FNONBLOCK)) == 0) {
		if (SLEEP_LOCK_SIG(tp->tm_usync, pritape) == FALSE) {
			return (EINTR);		/* Signalled out */
		}
	} else if (SLEEP_TRYLOCK(tp->tm_usync) == FALSE) {
			return (EAGAIN);	/* Can't wait */
	}

	if (tp->tm_open != TMO_CLOSED) {
		SLEEP_UNLOCK(tp->tm_usync);
		return (EBUSY);		/* Device is already open */
	}

	/* Wait for previously aborted operations to complete */
	saved_pl = LOCK(tp->tm_lock, TM_PL);
	if ((oflag & (FNDELAY | FNONBLOCK)) == 0) {
		while ((tp->tm_mflags & TMF_ABORTED) != 0) {
			if (SV_WAIT_SIG(tp->tm_cwait, pritape, tp->tm_lock) 
			    == FALSE) {
				SLEEP_UNLOCK(tp->tm_usync);
				return (EINTR);	/* Signalled out of wait */
			}
			saved_pl = LOCK(tp->tm_lock, TM_PL);
		}
	} else if ((tp->tm_mflags & TMF_ABORTED) != 0) {
		UNLOCK(tp->tm_lock, saved_pl);
		SLEEP_UNLOCK(tp->tm_usync);
		return (EAGAIN);		/* Can't wait */
	}

	tp->tm_open = TMO_OPEN;
	UNLOCK(tp->tm_lock, saved_pl);
	SLEEP_UNLOCK(tp->tm_usync);

	/*
	 * Make certain that the device is online
	 * and has medium loaded.  If not, then 
 	 * mark the device as closed and return.
	 * Perform retension on open here also.
	 */ 
	if ((status = tm_gen_scsi_cmd(SCSI_TEST, tp, 0, 0)) != 0) {
		/* 
		 * Medium may simply be unloaded. 
		 * Try an explicit "load" and then
		 * retry the test-unit-ready command.
		 * Also perform retensioning at this
		 * time if opening a retension on open
		 * device.
		 */
		status = tm_gen_scsi_cmd(SCSI_LOAD_UNLOAD, tp, 0, 
			MT_RETEN(*dev) ? SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA 
					: SCSI_LOAD_MEDIA);
		if (status != 0) {
			tp->tm_open = TMO_CLOSED;
			return (status);	/* Load/retension failed  */
		}
		if ((status = tm_gen_scsi_cmd(SCSI_TEST, tp, 0, 0)) != 0) {
			tp->tm_open = TMO_CLOSED;
			return (status);	/* Still not ready */
		}
	} else if (MT_RETEN(*dev)) {
		status = tm_gen_scsi_cmd(SCSI_LOAD_UNLOAD, tp, 0,
			SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA);
		if (status != 0) {
			tp->tm_open = TMO_CLOSED;
			return (status);	/* Retension failed */
		}
	}

	/* 
	 * Finish setting initial state flags, 
	 * then invoke the device specific open().
	 */
	if (tp->tm_amode != TMA_EOD)
		tp->tm_amode = TMA_OTHER;
	tp->tm_mflags = 0;

	if ((status = (* tp->tm_dp->open)(tp, *dev, oflag)) != 0) {
		tp->tm_open = TMO_CLOSED;
		return (status);
	}
	return (0);		/* Successful open completed */
}
 
/*
 * int
 * tmclose(dev_t dev, int flag, int otyp, cred_t *crd)
 * 	Close a tape drive performing required cleanup.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	This is an exclusive access device, therefore this
 *	is its only open reference.
 *
 *	Returns zero if the open completes successfully
 *	or an appropriate errno otherwise.
 *
 * Description:
 *	Acquire the device's sleep- and basic-locks while
 *	changing its state.  Drop the basic lock afterwards,
 *	since the drive is quiescent.  Next, deallocate any
 *	buffer allocated for non-blocksize I/O.  If closing 
 *	because of an operation had a catostrophic error or 
 *	was signalled out, take no further action.
 *
 *	Otherwise, the following table indicates actions to take 
 *	based upon the most recent tape operation and the state
 *	of the minor device REWIND flag:
 *
 *	   REWIND    Last op	 Action to take
 *	  on close  read/write?
 *	  ====================================================
 *	     Yes      READ	 Rewind tape.
 *	     Yes      WRITE	 Write filemark and rewind.
 *	     Yes      neither	 Rewind tape.
 *	     No	      READ	 Position tape after filemark.
 *	     No	      WRITE	 Write filemark and position
 *				 tape after it.
 *	     No	      neither	 No tape movement.
 */
/*ARGSUSED*/
int
tmclose(dev_t dev, int flag, int otyp, cred_t *crp)
{
	int unit = MT_UNIT(dev);
	tm_t *tp = tm_info[unit];
	int status = 0;
	pl_t saved_pl;

	ASSERT(otyp != OTYP_BLK);
	ASSERT(unit < tm_ndevs && tp != NULL && tp->tm_open != TMO_CLOSED);

	SLEEP_LOCK(tp->tm_usync, pritape);
	saved_pl = LOCK(tp->tm_lock, TM_PL);

	if ((tp->tm_mflags & TMF_ABORTED) != 0 || tp->tm_open == TMO_ERR) {
		tp->tm_open = TMO_CLOSED;
		UNLOCK(tp->tm_lock, saved_pl);
		SLEEP_UNLOCK(tp->tm_usync);
		return (0); 	/* Don't touch the tape - just bail out */
        }
	tp->tm_open = TMO_CLOSED;
	UNLOCK(tp->tm_lock, saved_pl);	/* The device is now quiescent */

	if (tp->tm_amode == TMA_LASTIOW
	&& (status = tm_gen_scsi_cmd(SCSI_WFM, tp, 1, 0)) != 0) {
		if (status != EINTR) {
			/*
			 *+ The tm-driver's SCSI write-file-mark 
			 *+ command failed.  This most likely occurred
		 	 *+ along with another error, while attempting 
			 *+ to write the medium.  The medium might have 
			 *+ been protected against an attempt to write 
			 *+ while not at the beginning of the tape or 
			 *+ at the logical end of data.
			 */
			cmn_err(CE_NOTE, 
				"tm%d: error writing file mark on close", unit);
		}
		SLEEP_UNLOCK(tp->tm_usync);
		return (status);
	}

	if (MT_REWIND(dev)) {
		status = tm_gen_scsi_cmd(SCSI_LOAD_UNLOAD, tp, 0, 0);
	} else if (tp->tm_amode == TMA_LASTIOR) {
		/* 
		 * Position medium to the next file, unless it is
		 * already there because a filemark was read, but
		 * not yet reported.
		 */
		status = tm_gen_scsi_cmd(SCSI_SPACE, tp, 1, 
			SCSI_SPACE_FILEMARKS);
	}
	SLEEP_UNLOCK(tp->tm_usync);
	return (status);
}

/*
 * int
 * tmread(dev_t dev, uio_t *uiop, cred_t *crp)
 *	Execute a raw read request.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	The call through physiock() may result in adjustments
 *	to the uio structure to reflect the number of bytes actually 
 *	read, if any portion of the transfer completes.
 *
 *	Returns zero if the I/O completes or an errno if it fails.
 *
 * Description:
 *	Invoke tmstrategy() via physiock() to lock the request'
 *	data buffer into memory and perform the input.  For tape,
 *	the 'nblocks' argument for the device capacity is not
 *	applicable, so use 0 to indicate 'infinity'.
 */
/*ARGSUSED*/
int
tmread(dev_t dev, uio_t *uiop, cred_t *crp)
{
	ASSERT(MT_UNIT(dev) < tm_ndevs); 
	ASSERT(tm_info[MT_UNIT(dev)] != NULL);
	ASSERT(tm_info[MT_UNIT(dev)]->tm_open != TMO_CLOSED);
	return (physiock(tmstrategy, NULL, dev, B_READ, 0, uiop));
}

/*
 * int
 * tmwrite(dev_t dev, uio_t *uiop, cred_t *crp)
 *	Execute a raw write request.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	The call through physiock() may result in adjustments
 *	to the uio structure to reflect the number of bytes actually 
 *	written, if any portion of the transfer completes.
 *
 *	Returns zero if the I/O completes or an errno if it fails.
 *
 * Description:
 *	Invoke tmstrategy() via physiock() to lock the request'
 *	data buffer into memory and perform the output.  For tape,
 *	the 'nblocks' argument for the device capacity is not
 *	applicable, so use 0 to indicate 'infinity'.
 */
/*ARGSUSED*/
int
tmwrite(dev_t dev, uio_t *uiop, cred_t *crp)
{
	ASSERT(MT_UNIT(dev) < tm_ndevs); 
	ASSERT(tm_info[MT_UNIT(dev)] != NULL);
	ASSERT(tm_info[MT_UNIT(dev)]->tm_open != TMO_CLOSED);
	return (physiock(tmstrategy, NULL, dev, B_WRITE, 0, uiop));
}

/*
 * STATIC void
 * tmstrategy(buf_t *)
 * 	READ or WRITE to a device via the raw device interface 
 *	using a buf-structure.
 * 
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Adjusts the buf structure to reflect the completion
 *	status and the amount of data transferred.	
 *
 *	The request's transfer buffer must be locked 
 *	into memory upon entry/exit.
 *
 *	The request will be completed upon exit, although
 *	the caller does not depend upon it and therefore
 *	expects to be awakened by biodone().
 *
 *	Returns zero if successful.  Otherwise, returns:
 *
 *		EBADF if the device is not in the OPEN state.
 *
 *		EIO if the device is open, but has not yet been
 *		recovered from a fatal error on a recent operation.
 *
 *		EINVAL if the transfer count is <= zero or is not
 *		a multiple of the device's fixed block size.
 *
 *		EINTR if signalled out of delays, such as occur 
 *		with synchronizing syscall entries.
 *
 *		Other errnos returned by functions called within.
 *
 * Description:
 *	Acquire the syscall entry mutex for the device, validate a 
 *	few of the fields of the buf, wait if necessary for a previously
 *	aborted operation to complete, then invoke one of two raw-I/O
 *	functions to complete the operation, based on the request 
 *	being a read or write.  The status returned from them is
 *	then propogated to the buf structure and its originator is
 *	notified of its completion prior to returning.
 */
STATIC void
tmstrategy(buf_t *bp)
{
	int unit = MT_UNIT(bp->b_edev);
	tm_t *tp = tm_info[unit];
	int status;
	pl_t saved_pl;

	ASSERT(unit < tm_ndevs); 
	ASSERT(tp != NULL);

	bp->b_resid = bp->b_bcount;	/* Reduce as we go, as appropriate */
	if (SLEEP_LOCK_SIG(tp->tm_usync, pritape) == FALSE) {
		status = EINTR; 	/* Signalled out */
	} else {
		if (tp->tm_open == TMO_CLOSED) {
			status = EBADF;	/* Device not currently accessible */
		} else {
			/* Wait for previously aborted operations to complete */
			saved_pl = LOCK(tp->tm_lock, TM_PL);
			while ((tp->tm_mflags & TMF_ABORTED) != 0) {
				if (SV_WAIT_SIG(tp->tm_cwait, pritape, 
						tp->tm_lock) == FALSE) {
					SLEEP_UNLOCK(tp->tm_usync);
					status = EINTR;
					goto signal_exit;
				}
				saved_pl = LOCK(tp->tm_lock, TM_PL);
			}
			UNLOCK(tp->tm_lock, saved_pl);

			if (tp->tm_open == TMO_ERR) { 
				/* Not recovered from previous error */
				status = EIO;	
			} else if (bp->b_bcount <= 0 
				|| (bp->b_bcount % tp->tm_blksz) != 0) {
				status = EINVAL;	/* Bad transfer count */
			} else if ((bp->b_flags & B_READ) != 0) {
				status = tm_raw_read(tp, bp);
			} else {
				status = tm_raw_write(tp, bp);
			}
		}
signal_exit:
		SLEEP_UNLOCK(tp->tm_usync);
	}
	bioerror(bp, status);
	biodone(bp);
}
 
/*
 * void
 * tmintr(int)
 *      Command block completion interrupt handler.
 *
 * Calling/Exit State:
 *      Basic locks may be held upon entry/exit.
 *
 *      'vector' must equal: tm_basevec + (logical_unit * CBs_per_unit) + CB#.
 *
 *	Adjusts state information for the logical unit interrupting:
 *
 *	     - for SCSI check conditions retrieve the applicable
 *	       sense key and residual count from the sense data,
 *	       saving them in tp->tm_resid and tp->tm_key.  Also 
 *	       note the settings of the EOF and EOM bits sense bits.
 *	       Explicitly set the key to -1 if not a check condition.
 *
 *	     - for SSM/SCSI adapter failures set TMF_FAIL in 
 *	       tp->tm_mflags.
 *
 *	     - Take no action otherwise; its a normal completion.
 *
 *      No return value.
 *
 * Description:
 *      From the interrupt vector, calculate/locate the logical unit,
 *	its associated information structure (tp), and the terminated
 *	CB.  Then use the CB completion code, SCSI completion status,
 *	and SCSI sense data to generate state transitions for the 
 *	initiator of the CB.  Then clear the cb->sw.cb_state field to
 *	indicate that the CB has been terminated.  Finally, provide a 
 *	wakeup to any task awaiting on tp->cwait for the CB's, after 
 *	clearing the unit's TMF_ABORTED flag.
 *
 *	Display a diagnostic message if catostrophic errors are
 *	detected by this function.  For SCSI check conditions,
 *	call tm_report_sense_info() to determine if it warrants 
 *	a diagnostic message being displayed.  Also, set TMF_FAIL
 *	for all check conditions, then let command specific termination
 *	handling determine if it really isn't a problem.
 *
 * Remarks:
 *	Assume the completion is for CB0 of the device's CBs, since
 *	the driver currently on uses it.
 */
void
tmintr(int vector)
{
	int unit = (vector -= tm_basevec) / NCBPERSCSI;
	volatile struct scsi_cb *cb;
	struct scrsense *sensebuf;
	caddr_t	errmsg;
	tm_t *tp;
	pl_t saved_pl;

	if (unit < 0 || unit >= tm_ndevs || (tp = tm_info[unit]) == NULL
	|| (cb = tp->tm_cbs)->sw.cb_state != TM_CB_ACTIVE) {
		/*
		 *+ The tm driver received an interrupt for a 
		 *+ nonconfigured unit.  This can indicate 
		 *+ that the system's interrupt vector table has
		 *+ been corrupted.
		 */
		cmn_err(CE_WARN, "tmintr: Invalid interrupt vector ignored");
		return;
	}

	saved_pl = LOCK(tp->tm_lock, TM_PL);
	errmsg = NULL;
	tp->tm_key = -1;

	switch (cb->sh.cb_compcode) {	/* Analyze the SCSI-CB termination */
	case SCB_BUSY:
		/* Not really done yet; ignore this interrupt */
		/*
		 *+ The tm driver received an interrupt for a valid
		 *+ device and SCSI command, but the command status
		 *+ indicates that it is still executing.  There might
		 *+ be a problem with the SSM.
		 */
		cmn_err(CE_WARN, "tmintr: tm%d: interrupt ignored; CB is busy",
			unit);
		UNLOCK(tp->tm_lock, saved_pl);
		return;

	case SCB_BAD_CB:
		errmsg = "CB is not acceptable";
		break;

	case SCB_NO_TARGET:
		errmsg = "Target adapter does not respond";
		break;

	case SCB_SCSI_ERR:
		errmsg = "A SCSI protocol error occurred";
		break;

	case SCB_OK:
		/* 
		 * Good CB termination; Now check its
		 * SCSI termination status for a check-condition.
		 */
		sensebuf = (struct scrsense *)cb->sw.cb_vsense;
		if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
			if (SCSI_RS_ERR_CLASS(sensebuf) != RS_CLASS_EXTEND
			||  SCSI_RS_ERR_CODE(sensebuf) != RS_CODE_EXTEND) {
				errmsg = "Unexpected check condition type";
				break;
			}

			tp->tm_key = sensebuf->rs_key & RS_SENSEKEYMASK;
			if ((sensebuf->rs_key & RS_EOM) != 0)
				tp->tm_pos = TMP_EOM;
			if ((sensebuf->rs_key & RS_FILEMARK) != 0)
				tp->tm_amode = TMA_EOF;
			if ((sensebuf->rs_error & RS_VALID) != 0) {
				tp->tm_resid = (long)sensebuf->rs_info[0] << 24;
				tp->tm_resid |= (long)sensebuf->rs_info[1] << 16;
				tp->tm_resid |= (long)sensebuf->rs_info[2] << 8;
				tp->tm_resid |= (long)sensebuf->rs_info[3];
			} else {
				tp->tm_resid = 0;
			}
			tm_report_sense_info(tp, cb, unit, 
					tm_scsi_errors[tp->tm_key]);
			tp->tm_mflags |= TMF_FAIL;

		} else if (!SCSI_GOOD(cb->sh.cb_status)) {
			errmsg = "Unknown Program Error";
		} else {
			tp->tm_resid = 0;
		}
		break;
	default:
		/*
		 *+ The tm driver received an unknown SSM completion
		 *+ code for a SCSI command.  
		 */
		cmn_err(CE_PANIC,
		"tmintr: tm%d: Bad SCSI command block completion status", unit);
		/*NOTREACHED*/
	}

	if (errmsg != NULL) {
		tp->tm_mflags |= TMF_FAIL;
		/*
		 *+ A SCSI command issued by the tm driver
		 *+ had an error termination.  A summary has
		 *+ been displayed containing the reason for
		 *+ the SCSI termination and the SCSI command 
		 *+ being executed.
		 */
		cmn_err(CE_NOTE, "tm%d: %s on command %s.\n",
			unit, errmsg, tm_scsi_commands[cb->sh.cb_scmd[0]]);
	}

	/* Awaken potential waiting process */
	cb->sw.cb_state = TM_CB_RDY;
	tp->tm_mflags &= ~TMF_ABORTED;
	SV_BROADCAST(tp->tm_cwait, 0);
	UNLOCK(tp->tm_lock, saved_pl);
}
 
/*
 * int
 * tmioctl(dev_t, int, void *, int, cred_t *, int *)
 *	Perform special device operations.
 *	
 * Calling/Exit State:
 *	Basic lock must not be held upon entry/exit.
 *
 *	The driver' state information for the specified
 *	unit may be modified upon exit to reflect the
 *	device's new state.  The request may write upon
 *	and/or reposition the medium, as directed.
 *	
 *	Returns zero if the requested operation is 
 *	successfully completed.  Otherwise an applicable
 *	errno is returned.
 *
 * Description:
 *	First acquire exclusive syscall access to the device
 * 	and await completion of any active, but previously
 *	aborted operations.   Then retension the medium if
 *	the medium has recently been changed and the operation
 *	repositions it to either the BOT mark or logical end of
 *	recorded data.  Skip the retension if the operation
 *	effectively does it or it is irrelevat to the operation.
 *	Finally, validate the requested operation and, if necessary, 
 *	invoke tm_gen_scsi_cmd() to execute SCSI commands to
 *	fulfill the request and make appropriate state 
 *	transitions.  Afterwards, release syscall mutex for
 *	the device and then return the operation's completion
 *	status value.
 */
/*ARGSUSED*/
int
tmioctl(dev_t dev, int cmd, void *arg, int mode, cred_t *crp, int *rvalp)
{
	int unit = MT_UNIT(dev);
	tm_t *tp = tm_info[unit];
	int opcount, status = 0;
	struct blklen blk;
	pl_t saved_pl;

	ASSERT(unit < tm_ndevs); 
	ASSERT(tp != NULL);

	if (SLEEP_LOCK_SIG(tp->tm_usync, pritape) == FALSE)
		return (EINTR); 	/* Signalled out */

	if (tp->tm_open == TMO_CLOSED) {
		SLEEP_UNLOCK(tp->tm_usync);
		return (EBADF);		/* Device not currently accessible */
	}

	/* Wait for previously aborted operations to complete */
	saved_pl = LOCK(tp->tm_lock, TM_PL);
	while ((tp->tm_mflags & TMF_ABORTED) != 0) {
		if (SV_WAIT_SIG(tp->tm_cwait, pritape, tp->tm_lock) == FALSE) {
			SLEEP_UNLOCK(tp->tm_usync);
			return (EINTR); 	/* Signalled out */
		}
		saved_pl = LOCK(tp->tm_lock, TM_PL);
	}
	UNLOCK(tp->tm_lock, saved_pl);

	if ((tp->tm_mflags & TMF_ATTEN) != 0) {
		/* 
		 * Respond to the medium having been changed
		 * since the device was opened.
		 */
		switch (cmd) {
		case T_RETENSION:
		case T_RWD:
		case T_ERASE:
		case T_STD:
		case T_LOAD:
		case T_UNLOAD:
		case T_RDBLKLEN:
		case T_WRBLKLEN:
		case T_PREVMV:
		case T_ALLOMV:
		case T_VERBOSE:
			break;		/* No action required */

		case T_WRFILEM:
		case T_SFF:
		case T_SBF:
		case T_SFB:
		case T_SBB:
			/* 
			 * Since the medium may not be where it 
			 * is expected, fail these requests until
			 * TMF_ATTEN is cleared by some other
			 * action indicating the caller knows the
			 * medium's current position.
			 */
			SLEEP_UNLOCK(tp->tm_usync);
			return (EIO);	

		case T_EOD:
		default:
			if (MT_RETEN(dev)) {
				/* Retension the medium first. */
				status = tm_gen_scsi_cmd(SCSI_LOAD_UNLOAD, tp,
					0, SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA);
				if (status != 0) {
					SLEEP_UNLOCK(tp->tm_usync);
					return (status);/* Retension failed */
				}
				ASSERT((tp->tm_mflags & TMF_ATTEN) == 0);
				ASSERT(tp->tm_pos == TMP_BOT);
			}
			break;
		}
	}

	/* Perform the specific tape operation. */
	switch (cmd) {
	case T_RETENSION:
		status = tm_gen_scsi_cmd(SCSI_LOAD_UNLOAD, tp, 0,
			SCSI_LOAD_MEDIA | SCSI_RETEN_MEDIA);
		break;

	case T_RWD:
		/*
		 * Append a filemark prior to rewinding
		 * if the tape had been written upon, but
		 * the last operation did not write a 
		 * filemark.  Continue with the rewind, even
		 * if the filemark can't be written since it
		 * may recover the drive state.
		 */
		if (tp->tm_amode == TMA_LASTIOW) {
			if (tm_gen_scsi_cmd(SCSI_WFM, tp, 1, 0) != 0) {
				/* 
				 *+ The tm-driver was unable to write a
				 *+ filemark on the medium inserted in
				 *+ the specified drive, prior to 
				 *+ rewinding it.  The medium had data
				 *+ written on it prior to this failure.
				 */
				cmn_err(CE_WARN, 
				"tm%d: can't write filemark prior to rewinding",
					 unit);
			}
		}
		status = tm_gen_scsi_cmd(SCSI_REWIND, tp, 0, 0);
		break;

	case T_ERASE:
		if ((mode & FWRITE) == 0) {
			status = EACCES;	/* Not open for write */
		} else {
			status = tm_gen_scsi_cmd(SCSI_ERASE, tp, 0, 0);
		}
		break;

	case T_WRFILEM:
		opcount = (int)arg;
		if (opcount < 0 || opcount > 0x7ff) {
			status = EINVAL;	/* Count is out of range */
		} else if ((mode & FWRITE) == 0) {
			status = EACCES;	/* Not open for write */
		} else {
			status = tm_gen_scsi_cmd(SCSI_WFM, tp, opcount, 0);
		}
		break;

	case T_EOD:
		status = tm_gen_scsi_cmd(SCSI_SPACE, tp, 0, 
			SCSI_SPACE_ENDOFDATA);
		break;

	case T_STD:
		/* 
		 * Change the drive's current density setting.
		 * Invoke a drive specific function to do this.
		 */
		if ((int)arg < 0 || (int)arg > 0xFF) {
			status = EINVAL;	/* Density value invalid */
		} else {
			status = (* tp->tm_dp->set_density)(tp, (int)arg);
		}
		break;

	case T_SFF:
	case T_SBF:
	case T_SFB:
	case T_SBB:
		if (tp->tm_amode == TMA_LASTIOW || tp->tm_amode == TMA_EOD) {
			/*
			 *+ The attempted reposition operation is not 
			 *+ supported by the tm-driver when in write mode.
			 */
			cmn_err(CE_WARN, 
				"tm%d: bad operation after write", unit);
			status = EINVAL;
			break;
		}
		opcount = (int)arg;
		if (opcount < 0 || opcount > 0x7ff) {
			status = EINVAL;	/* Count is out of range */
			break;
		}
		switch (cmd) {
		case T_SFF:
			status = tm_gen_scsi_cmd(SCSI_SPACE, tp, opcount, 
					SCSI_SPACE_FILEMARKS);
			break;
		case T_SFB:
			status = tm_gen_scsi_cmd(SCSI_SPACE, tp, -opcount, 
					SCSI_SPACE_FILEMARKS);
			break;
		case T_SBF:
			status = tm_gen_scsi_cmd(SCSI_SPACE, tp, opcount, 
					SCSI_SPACE_BLOCKS);
			break;
		case T_SBB:
			status = tm_gen_scsi_cmd(SCSI_SPACE, tp, -opcount, 
					SCSI_SPACE_BLOCKS);
			break;
		}
		break;

	case T_LOAD:
		status = tm_gen_scsi_cmd(SCSI_LOAD_UNLOAD, 
					tp, 0, SCSI_LOAD_MEDIA);
		break;

	case T_UNLOAD:
		status = tm_gen_scsi_cmd(SCSI_LOAD_UNLOAD, tp, 0, 0);
		break;

	case T_PREVMV:
		status = tm_gen_scsi_cmd(SCSI_PA_REMOVAL, 
				tp, 0, SCSI_REMOVAL_PREVENT);
		break;

	case T_ALLOMV:
		status = tm_gen_scsi_cmd(SCSI_PA_REMOVAL, 
				tp, 0, SCSI_REMOVAL_ALLOW);
		break;

	case T_VERBOSE:
		if (((int)arg & ~TMF_VERBOSE) != 0)
			status = EINVAL;
		else
			tp->tm_mflags |= (uchar_t)arg;
		break;

	case T_RDBLKLEN:
		/* 
		 * The block size for this driver is fixed,
		 * so just return the same value for both
		 * minimum and maximum.
		 */
		blk.res1 = 0;
		blk.max_blen = tp->tm_blksz;
		blk.min_blen = tp->tm_blksz;
		if (copyout(&blk, arg, sizeof (struct blklen)) != 0)
			status = EFAULT;
		break;

	case T_WRBLKLEN: 	/* Not allowed to change the block size */
	default:
		status = EINVAL;
		break;
	}

	SLEEP_UNLOCK(tp->tm_usync);
	return (status);
}
 
/*
 * STATIC int
 * tm_gen_scsi_cmd(uchar_t, tm_t *, int, uint)
 *	Execute a SCSI command via the SSM/SCSI bus.
 *
 * Calling/Exit State:
 *      Basic locks must not be held upon entry/exit.
 *
 *      The caller is expected to have validated the
 *      request prior to calling this function and to
 *      have waited for any aborted operations to complete;
 *      the device associated with 'tp' is presumed to 
 *	be idle and owned by this process/task upon entry.
 *
 *	'opcount' pertains only to the SCSI_WFM, and 
 *	SCSI_SPACE commands.  
 *
 *	'qualifier' pertains to the TM_UNLOAD_LOAD command 
 *	(either 'load' or 'unload') and the SCSI_SPACE 
 *	command (qualifies 'end-of-data', 'blocks', or 
 *	'filemarks').
 *
 *      Returns zero if the operation completes successfully.
 *	Otherwise, returns an applicable errno for the failure.
 *
 * Description:
 *	Translate the request into a SCSI command block, contained
 *	within an SSM/SCSI-CB for the specified device.  Then
 *	notify the SSM/SCSI adapter to execute it and await 
 *	notification that it has completed; tmintr() will be
 *	recieve the termination interrupt and then re-awakens this 
 *	function to continue.
 *
 *      Fail the request if the SSM detected a bus or adapter errror
 *      during command execution.  If the command terminated with a
 *      SCSI check condition, analyze the returned request-sense data
 *      to determine if the operation completed correctly and make 
 *	the appropriate driver state transitions for the unit prior 
 *	to returning the appropriate completion status.
 *
 * Remarks:
 *	Attempt these operations to be executed in spite of tp->tm_open
 *	being set to TMO_ERR on the chance the operation will bring 
 *	the drive to a well defined state, facilitating its recovery.
 *	For example, REWIND returns the medium to its load point.
 *	Likewise, set TMO_ERR if a failure occurs such that the device
 *	must be reset, repositioned, or closed to recover.
 */		
STATIC int
tm_gen_scsi_cmd(uchar_t cmd, tm_t *tp, int opcount, uint qualifier)
{
	volatile struct scsi_cb *cb;
	int retries = 0;
	pl_t saved_pl;
	int status;

	ASSERT((tp->tm_mflags & TMF_ABORTED) == 0);
	cb = tp->tm_cbs;	/* Always use CB[zero]. */

retry_cmd:
	/*
	 * Clear the shared portion of the cb, except 
	 * the address and size of the request-sense 
	 * buffer.
	 */
	bzero(SWBZERO(cb), SWBZERO_SIZE);

	/* 
	 * Perform SCSI command specific initialization.
	 */
	tp->tm_mflags &= ~TMF_FAIL;
	cb->sh.cb_clen = SCSI_CMD6SZ;
	cb->sh.cb_scmd[1] = cb->sw.cb_scsi_device << SCSI_LUNSHFT;
	switch (cb->sh.cb_scmd[0] = cmd) {
	case SCSI_ERASE:
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_scmd[1] |= SCSI_ERASE_LONG;
		break;

	case SCSI_PA_REMOVAL:
	case SCSI_LOAD_UNLOAD:
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_scmd[4] = (uchar_t)qualifier;
		break;

	case SCSI_REWIND:
		cb->sh.cb_cmd = SCB_READ;
		break;

	case SCSI_WFM:
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_scmd[2] = (unchar)(opcount >> 16);
		cb->sh.cb_scmd[3] = (unchar)(opcount >> 8);
		cb->sh.cb_scmd[4] = (unchar)opcount;
		break;

	case SCSI_TEST:
		cb->sh.cb_cmd = SCB_READ;
		break;

	case SCSI_RSENSE:
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_addr = cb->sh.cb_sense;
		cb->sh.cb_count = cb->sh.cb_slen;
		cb->sh.cb_scmd[4] = cb->sh.cb_slen;
		break;

	case SCSI_MODES:
	case SCSI_MSENSE:
		/* 
		 * Assume the caller is using tp->tm_buffer as a
		 * data buffer of size tp->tm_dp->mlen bytes.  For mode 
		 * selection, the caller must have already initialized 
		 * its contents.
		 */
		cb->sh.cb_cmd = (cmd == SCSI_MODES) ? SCB_WRITE : SCB_READ;
		cb->sh.cb_addr = (ulong_t) vtop(tp->tm_buffer, NULL);
		cb->sh.cb_count = tp->tm_dp->mlen;
		cb->sh.cb_scmd[4] = tp->tm_dp->mlen;
		break;

	case SCSI_SPACE:
		if (tp->tm_amode == TMA_EOF) {
			/* 
			 * A filemark has already been read, but 
			 * not reported to the user.  This must
			 * be compensated for in order to end up
			 * in the expected location.
			 */
			if (qualifier == SCSI_SPACE_FILEMARKS) {
				opcount--;	/* Simply adjust opcount */
			} else if (qualifier == SCSI_SPACE_BLOCKS) {
				/* 
				 * Reposition to the BOT side of
				 * the previously read filemark
				 * prior to issuing the space-blocks
				 * operation to make this work
				 * as expected.
				 */
				status = tm_gen_scsi_cmd(SCSI_SPACE, tp, -1, 
						SCSI_SPACE_FILEMARKS);
				if (status != 0) 
					return (status); /* Operation failed */ 
			}
		}
		cb->sh.cb_cmd = SCB_READ;
		cb->sh.cb_scmd[1] |= qualifier;
		cb->sh.cb_scmd[2] = (unchar)(opcount >> 16);
		cb->sh.cb_scmd[3] = (unchar)(opcount >> 8);
		cb->sh.cb_scmd[4] = (unchar)opcount;
		break;

	default:
		return (EINVAL);	/* Invalid/unimplemented command */
	}

	/* 
	 * Notify the SSM to execute request, then wait for
	 * its completion which is recieved by tmintr().  
	 * Prior to tmintr() signalling its completion it
	 * will set status indicating if it completed
 	 * successfully, had an SSM/SCSI failure, or had
	 * a check condition.  For check conditions it will
	 * make some state transitions for EOF/EOM/BOT, set
	 * the residual count, and sense key if applicable.
 	 * Also, TMF_FAIL is set by the completion handler
	 * for all check conditions or SSM/SCSI failures.  
	 * Its up to the code below to determine if its not 
	 * really a failure.
	 */
	cb->sh.cb_cmd |= SCB_IENABLE;	
	cb->sh.cb_compcode = SCB_BUSY;

	(void)LOCK(tp->tm_lock, plhi);
	cb->sw.cb_state = TM_CB_ACTIVE;
	slic_mIntr(cb->sw.cb_slic, SCSI_BIN, cb->sw.cb_unit_index);
	switch (cmd) {
	case SCSI_MODES:
	case SCSI_MSENSE:
		/*
		 * These commands cannot be aborted once
		 * started since they finish quickly and
		 * the buffers for them can't be deallocated 
		 * until they complete.
		 */
		SV_WAIT(tp->tm_cwait, pritape, tp->tm_lock);
		ASSERT(cb->sw.cb_state != TM_CB_ACTIVE);
		break;
	default:
		while (cb->sw.cb_state == TM_CB_ACTIVE) {
			if (SV_WAIT_SIG(tp->tm_cwait, pritape, tp->tm_lock) 
			    == FALSE) {
				saved_pl = LOCK(tp->tm_lock, TM_PL);
				if (cb->sw.cb_state == TM_CB_ACTIVE) {
					tp->tm_mflags |= TMF_ABORTED;
				}
				UNLOCK(tp->tm_lock, saved_pl);
				return (EINTR);	/* Signalled out of wait */
			}
			saved_pl = LOCK(tp->tm_lock, TM_PL);
		}
		UNLOCK(tp->tm_lock, saved_pl);
		break;
	}

	ASSERT (tp->tm_key == -1 || (tp->tm_mflags & TMF_FAIL) != 0);
	switch (tp->tm_key) {
	case -1:
		/* 
		 * Not a check condition.  Determine if it was
		 * a successful completion or SSM/SCSI failure
		 * based on TMF_FAIL.
		 */
		if ((tp->tm_mflags & TMF_FAIL) != 0) {
			tp->tm_open = TMO_ERR;	/* This is a catostrophic */
			return (EIO);		/* SCSI failure. 	  */
		}

		/* Otherwise, the operation completed without problems */
		break;

	case RS_NOTRDY:		/* Medium not inserted or not loaded */
		tp->tm_pos = TMP_OTHER;		/* We don't know where it is */
		tp->tm_open = TMO_ERR;		/* These are catostrophic */
		tp->tm_amode = TMA_OTHER;
		break;

	case RS_UNITATTN:	/* Cartridge replaced since last operation */
		tp->tm_amode = TMA_OTHER;
		tp->tm_pos = TMP_BOT;
		tp->tm_mflags |= TMF_ATTEN;

		switch (cmd) { 
		case SCSI_ERASE:
		case SCSI_REWIND:
		case SCSI_LOAD_UNLOAD:
		case SCSI_MSENSE:
		case SCSI_MODES:
		case SCSI_TEST:
			/* 
			 * These commands can be retried since
			 * new medium being loaded should not
			 * disrupt them and the Unit-attention
			 * condition should clear immediately.
		  	 */
			if (retries == 0) {
				retries++;
				goto retry_cmd;	
			}
		}
		break;

	case RS_NOSENSE:
	case RS_RECERR:
		/* 
		 * Let command specific processing determine 
		 * if this was a problem base on state of 
		 * EOM,EOF, etc.
		 */
		tp->tm_mflags &= ~TMF_FAIL;	
		break;	
		
	default:
		break;
	}

	switch (cmd) { 
	case SCSI_ERASE:
	case SCSI_REWIND:
	case SCSI_LOAD_UNLOAD:
	case SCSI_MODES:
		if ((tp->tm_mflags & TMF_FAIL) != 0)
			return ((cmd == SCSI_MODES) ? EINVAL : EIO);	
		/* 
		 * The medium is now positioned successfully
		 * to the BOT mark, ready for more operations.
		 */
		if (tp->tm_open == TMO_ERR)
			tp->tm_open = TMO_OPEN;	/* Recovery successful */
		tp->tm_pos = TMP_BOT;
		tp->tm_amode = TMA_OTHER;
		if (cmd != SCSI_MODES)
			tp->tm_mflags &= (uchar_t) ~TMF_ATTEN;
		return (0);		/* Successful completion */

	case SCSI_MSENSE:
	case SCSI_PA_REMOVAL:
	case SCSI_TEST:
		if ((tp->tm_mflags & TMF_FAIL) != 0)
			return (EIO);	/* Notify caller of failure */
		return (0);		/* Successful completion */

	case SCSI_WFM:
		if ((tp->tm_mflags & TMF_FAIL) != 0)
			return (EIO);	/* Notify caller of failure */

		tp->tm_amode = TMA_EOD;
		tp->tm_mflags &= (uchar_t) ~TMF_ATTEN;
		if (tp->tm_pos == TMP_BOT) {
			tp->tm_pos = TMP_OTHER;
		} else if (tp->tm_pos == TMP_EOM) {
			/* 
			 * EOM or its early warning encountered.
			 * Retry remaining part of the command once
			 * if not all the filemarks were written.
			 * If it fails again, error out.
			 */
			if (tp->tm_resid != 0) {
				if (retries == 0) {
					ASSERT(tp->tm_resid < opcount);
					opcount = tp->tm_resid;
					retries++;
					goto retry_cmd;
				} else {
					return (EIO);	/* Must be EOM */
				}
			}
			tp->tm_pos = TMP_EW;	/* Must be early warning */
		}
		return (0);		/* Successful completion */
	case SCSI_SPACE:
		/* 
		 * Note that if the medium state can be
		 * clearly observed, then TMO_ERR can be
		 * cleared.
		 */
		if (tp->tm_key == RS_BLANK && opcount > 0) {
			/* Logical end of recorded data encountered */
			tp->tm_pos = TMP_OTHER;
			tp->tm_amode = TMA_EOD;
			tp->tm_mflags &= (uchar_t) ~TMF_ATTEN;
			if (tp->tm_open == TMO_ERR)
				tp->tm_open = TMO_OPEN;
			return (EIO);
		} 
		if (tp->tm_pos == TMP_EOM) {
			/* Physical beginning or end of medium encountered */
			if (opcount < 0)
				tp->tm_pos = TMP_BOT;
			tp->tm_amode = TMA_OTHER;
			tp->tm_mflags &= (uchar_t) ~TMF_ATTEN;
			if (tp->tm_open == TMO_ERR)
				tp->tm_open = TMO_OPEN;
			return (EIO);
		}
		if (tp->tm_amode == TMA_EOF) {
			/* 
			 * A filemark was reported.  It indicates 
			 * an error only if skipping blocks.  
			 * Otherwise its just informatory.
			 */ 
			tp->tm_pos = TMP_OTHER;
			tp->tm_amode = TMA_OTHER;
			tp->tm_mflags &= (uchar_t) ~TMF_ATTEN;
			if (qualifier == SCSI_SPACE_BLOCKS) 
				return (EIO);
		}
		if ((tp->tm_mflags & TMF_FAIL) != 0) {
			/* 
			 * Some other type of failure occurred, in 
			 * which case, the current state of the
			 * medium is not known; error this out.
			 */
			tp->tm_open = TMO_ERR;
			return (EIO);	/* Notify caller of failure */
		}

		tp->tm_pos = TMP_OTHER;
		tp->tm_amode = (qualifier == SCSI_SPACE_ENDOFDATA) 
				? TMA_EOD : TMA_OTHER;
		tp->tm_mflags &= (uchar_t) ~TMF_ATTEN;
		return (0);		/* Successful completion */

	case SCSI_RSENSE:
		return (0);		/* Either way its data is valid */
	}
	/*NOTREACHED*/
}

/*
 * STATIC int
 * tm_dma_map(tm_t *, buf_t *, uint)
 * 	Provide a scatter-gather/DMA mapping for a data transfer.
 * 
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Assumes the unit's SSM/SCSI CBs have already been
 *	allocated, along with the DMA list to CB[0] and that
 * 	the request is strictly to map a user-buffer, which
 *	must already be locked into memory.
 *
 *	If successful, it will have filled out the DMA table 
 *	for the transfer and leaves the start address in 
 *	cb->sw.cb_data, then returns zero.  Returns an
 *	appropriate errno upon failure.
 *	
 * Description:
 *	Determine how many user pages are involved in the
 *	transfer and fill out the DMA list with the physical page
 *	bases for each of the virtual page involved.
 */
STATIC int
tm_dma_map(tm_t *tp, buf_t *bp, uint nbytes)
{
	uint i, upages;
        caddr_t base;
	ulong_t *iovp;

	ASSERT((bp->b_flags & (B_PAGEIO | B_PHYS)) == B_PHYS);
	base = (caddr_t)((ulong_t)bp->b_un.b_addr & ~(tm_psize - 1));
	upages = btopr((ulong_t)bp->b_un.b_addr - (ulong_t) base + nbytes);

	for (iovp = tp->tm_cbs->sw.cb_iovstart, i = 0; 
		i < upages; i++, base += tm_psize, iovp++) {
		if ((*iovp = (ulong_t)vtop((caddr_t)base, bp->b_proc)) == 0) {
			return (EFAULT);
		}
        }
	tp->tm_cbs->sw.cb_data = (ulong_t)bp->b_un.b_addr;
	return (0);
}

/*
 * STATIC int
 * tm_raw_read(tm_t *, buf_t *)
 * 	Perform a SCSI-READ of the specified device.
 * 
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	The caller is expected to have validated the
 *	request prior to calling this function and to
 *	have waited for any aborted operations to complete;
 *	the device is presumed to be idle and owned by this
 *	process/task upon entry.
 *
 *	Expects the buf structure's b_resid field to equal 
 *	the b_bcount field upon entry.  b_resid will be
 *	decremented by the number of data bytes actually 
 *	transferred into the requestor's data buffer 
 *	(which must be locked into memory upon entry/exit).
 *
 *	Returns zero if successful.  Otherwise, returns
 *	an appropriate errno for the failure encountered.
 *
 * Description:
 *	Determine if the operation is permissible based upon the
 *	device's current state information.  If the device is
 *	currently in "write mode", then fail the request.  If the
 *	the medium has recently been changed without an explicit
 *	repositioning by the user or the physical end-of-medium 
 *	has been reached, then fail the request.  
 *
 *	Once the medium is in a readable state, determine if it should
 *	be read.  If the last operation read both data and a filemark, 
 *	then return the filemark indicator (no data read, no error) 
 *	this time because the data was last time.  Perform a device
 *	read by generating and passing a SCSI-READ command block to
 *	the device via the SSM/SCSI adapter and then awaiting its
 *	termination when the SSM is done with it of which tmintr()
 *	is notified and then re-awakens this function to continue.
 *	Note that the data buffer must be mapped into a scatter
 *	gather DMA list for the transfer to occur.
 *	
 *	Fail the request if the SSM detected a bus or adapter errror
 *	during command execution.  If the command terminated with a
 *	SCSI check condition, analyze the returned request-sense data
 *	to determine how much, if any, of the data transferred correctly
 *	and make the appropriate driver state transitions for the unit
 *	prior to returning the appropriate completion status.
 *
 * Remarks:
 *	If the request exceeds the device's individual transfer 
 *	maximum transfer limit,	the driver will only transfer
 *	the portion it is able to.  Further, it assumes the caller
 *	can handle that situation and may make multiple calls to
 *	complete the request.  An alternative is to fail the
 *	request with EINVAL.	
 */
STATIC int
tm_raw_read(tm_t *tp, buf_t *bp)
{
	volatile struct scsi_cb *cb;
	uint nbytes, nblocks;
	int status = 0;

	ASSERT(tp->tm_blksz > 0);
	ASSERT(bp->b_resid == bp->b_bcount); /* Caller must set this */
	ASSERT((tp->tm_mflags & TMF_ABORTED) == 0);

	switch (tp->tm_amode) {
	case TMA_LASTIOW:
	case TMA_EOD:
		/*
		 *+ tm-driver devices do not support reading
		 *+ immediately following a write.  No data
		 *+ can be read from the specified drive until
		 *+ the medium is rewound.
		 */
		cmn_err(CE_NOTE, 
			"tm%d: illegal read after going into write mode", 
			MT_UNIT(bp->b_edev));
		return (EIO);

	case TMA_EOF:
		/*
		 * Return a filemark previously read, 
		 * but not reported by the last operation.
		 */
		tp->tm_amode = TMA_OTHER;
		return (0);		

	case TMA_OTHER:
	case TMA_LASTIOR:
		if ((tp->tm_mflags & TMF_ATTEN) != 0) {
			/* 
			 * The medium has recently been changed 
			 * since the device was opened.  Fail this 
			 * type of request until actions occur 
			 * indicating its new postion is known,
			 * such as rewinding it or closing the device.
			 */
			return (EIO);
		}
		break;			/* Go ahead and read the data */
	}

	ASSERT(tp->tm_pos != TMP_EW);	/* Only applicable during writes */
	if (tp->tm_pos == TMP_EOM)
		return (EIO);		/* Read past physical end of medium */

	/* 
	 * Set up a DMA mapping for the transfer, 
	 * then build an SSM/SCSI read command to 
	 * execute the request.
	 */
	nbytes = min(tp->tm_dp->max_xfer * 1024, bp->b_resid); 
	if ((status = tm_dma_map(tp, bp, nbytes)) != 0)
		return (status);	/* DMA mapping failed */

	nblocks = nbytes / tp->tm_blksz;
	cb = tp->tm_cbs;
	bzero(SWBZERO(cb), SWBZERO_SIZE);
	cb->sh.cb_cmd = SCB_READ | SCB_IENABLE;
	cb->sh.cb_clen = SCSI_CMD6SZ;
	cb->sh.cb_scmd[0] = SCSI_READ;
	cb->sh.cb_scmd[1] = SCSI_FIXED_BLOCKS;
	cb->sh.cb_scmd[2] = (unchar)(nblocks >> 16);
	cb->sh.cb_scmd[3] = (unchar)(nblocks >> 8);
	cb->sh.cb_scmd[4] = (unchar)nblocks;
	cb->sh.cb_addr = cb->sw.cb_data;
	cb->sh.cb_iovec = (ulong_t *)cb->sw.cb_iovpaddr;
	cb->sh.cb_count = nbytes;
	cb->sh.cb_compcode = SCB_BUSY;

	/* 
	 * Notify the SSM to execute request, then wait for
	 * its completion which is recieved by tmintr().  
	 * Prior to tmintr() signalling its completion it
	 * will set status indicating if it completed
 	 * successfully, had an SSM/SCSI failure, or had
	 * a check condition.  For check conditions it will
	 * make some state transitions for EOF/EOM/BOT, set
	 * the residual count, and sense key if applicable.
 	 * Also, TMF_FAIL is set by the completion handler
	 * for all check conditions or SSM/SCSI failures.  
	 * Its up to the code below to determine if its not 
	 * really a failure.
	 */
	(void)LOCK(tp->tm_lock, plhi);
	cb->sw.cb_state = TM_CB_ACTIVE;
	tp->tm_mflags &= ~TMF_FAIL;
	slic_mIntr(cb->sw.cb_slic, SCSI_BIN, cb->sw.cb_unit_index);
	SV_WAIT(tp->tm_cwait, pritape, tp->tm_lock);
	ASSERT(cb->sw.cb_state != TM_CB_ACTIVE);


	ASSERT (tp->tm_key == -1 || (tp->tm_mflags & TMF_FAIL) != 0);
	switch (tp->tm_key) {
	case -1:
		/* 
		 * Not check condition.  Determine if it was
		 * a successful completion or SSM/SCSI failure
		 * based on TMF_FAIL.
		 */
		if ((tp->tm_mflags & TMF_FAIL) != 0) {
			tp->tm_open = TMO_ERR;	/* This is a catostrophic */
			return (EIO);		/* SCSI failure. 	  */
		}

		/* Otherwise, the read completed without problems */
		ASSERT(tp->tm_resid == 0);
		tp->tm_pos = TMP_OTHER;
		tp->tm_amode = TMA_LASTIOR;
		goto data_read;

	case RS_NOSENSE: 	
	case RS_RECERR:		
		/* 
		 * Either a filemark has been read, or the drive
		 * had difficulty reading, but recovered from it.
		 * Its not really a failure.
		 */
		ASSERT(tp->tm_pos == TMP_OTHER || tp->tm_pos == TMP_BOT);
		tp->tm_pos = TMP_OTHER;
		tp->tm_mflags &= ~TMF_FAIL;
		break;

	case RS_BLANK:
		/* 
		 * If Logical EOM (end or recorded data) was read 
		 * than treat this like a filemark, since double 
		 * filemarks cannot be used to mark logical end of 
		 * tape (cannot overwrite when appending to the tape)
		 * as they could on 1/2" tapes.
		 * Otherwise, this is a failed read.
		 */
		tp->tm_pos = TMP_OTHER;
		tp->tm_amode = TMA_EOF;
		tp->tm_mflags &= ~TMF_FAIL;
		break;

	case RS_MEDERR:
		/* 
		 * Physical EOM reached if TMP_EOM is set.
		 * Otherwise an unrecoverable data error occurred
		 * during the read.  Both case are failures, but
		 * defer reporting the former if any data was
		 * read first; the later is catostrophic.
		 */
		if (tp->tm_pos == TMP_EOM) {
			if (tp->tm_resid != 0)
				tp->tm_mflags &= ~TMF_FAIL;
		} else {
			tp->tm_open = TMO_ERR;
		}
		break;

	case RS_NOTRDY:		/* Medium not inserted or not loaded */
	case RS_HRDERR:		/* Parity or hard drive error */
		tp->tm_pos = TMP_OTHER;		/* We don't know where it is */
		tp->tm_open = TMO_ERR;		/* These are catostrophic */
		tp->tm_amode = TMA_OTHER;
		break;

	case RS_ILLREQ:		
		tp->tm_open = TMO_ERR;		/* This is bad news */
		break;

	case RS_UNITATTN:	/* Cartridge replaced since last operation */
		tp->tm_amode = TMA_OTHER;
		tp->tm_pos = TMP_BOT;
		tp->tm_mflags |= TMF_ATTEN;
		break;

	default:
		tp->tm_open = TMO_ERR;		/* Handle unexpected cases */
		break;
	}

	if ((tp->tm_mflags & TMF_FAIL) != 0) {
		return (EIO);			/* The read errored out */
	} else if (tp->tm_resid > nblocks) {
		tp->tm_open = TMO_ERR;		/* Unexpected, fatal error */
		return (EIO);	
	} 

	if (tp->tm_resid != 0) {
		/* 
		 * Re-calculated bytes read based on 
		 * nblocks since nbytes may not be a
		 * device blocksize multiple.
		 */
		nbytes = (nblocks - tp->tm_resid) * tp->tm_blksz;
		
	}

	if (nbytes == 0) {
		ASSERT(tp->tm_amode == TMA_EOF);
		/*
		 * Move out of LASTIOR/TMA_EOF state so a close
		 * does not reposition forward.
		 */
		tp->tm_amode = TMA_OTHER;
	} else if (tp->tm_amode != TMA_EOF) {
		tp->tm_amode = TMA_LASTIOR;
	}

data_read:
	/* Read completed successfully or partially successfully */
	bp->b_resid -= nbytes;

	return (0);
}

/*
 * STATIC int
 * tm_raw_write(tm_t *, buf_t *)
 * 	Perform a SCSI-WRITE of the specified device.
 * 
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	The caller is expected to have validated the
 *	request prior to calling this function and to
 *	have waited for any aborted operations to complete;
 *	the device is presumed to be idle and owned by this
 *	process/task upon entry.
 *
 *	Expects the buf structure's b_resid field to equal 
 *	the b_bcount field upon entry.  b_resid will be
 *	decremented by the number of data bytes actually 
 *	transferred out of the requestor's data buffer 
 *	(which must be locked into memory upon entry/exit).
 *
 *	Returns zero if successful.  Otherwise, returns
 *	an appropriate errno for the failure encountered.
 *
 * Description:
 *	Determine if the operation is permissible based upon the
 *	device's current state information.  If the device is
 *	currently in "read mode", then fail the request.  If the
 *	the medium has recently been changed without an explicit
 *	repositioning by the user or the physical end-of-medium 
 *	has been reached, then fail the request.  
 *
 *	Once the medium is in a writable state, determine if it 
 *	should be written.  Perform a device write by generating
 *	and passing a SCSI-WRITE command block to the device via 
 *	the SSM/SCSI adapter and then awaiting its termination 
 *	when the SSM is done with it of which tmintr() is notified 
 *	and then re-awakens this function to continue.  Note that 
 *	the data buffer must be mapped into a scatter gather DMA 
 * 	list for the transfer to occur.
 *	
 *	Fail the request if the SSM detected a bus or adapter errror
 *	during command execution.  If the command terminated with a
 *	SCSI check condition, analyze the returned request-sense data
 *	to determine how much, if any, of the data transferred correctly
 *	and make the appropriate driver state transitions for the unit
 *	prior to returning the appropriate completion status.
 *
 * Remarks:
 *	If the request exceeds the device's individual transfer 
 *	maximum transfer limit,	the driver will only transfer
 *	the portion it is able to.  Further, it assumes the caller
 *	can handle that situation and may make multiple calls to
 *	complete the request.  An alternative is to fail the
 *	request with EINVAL.	
 */
STATIC int
tm_raw_write(tm_t *tp, buf_t *bp)
{
	volatile struct scsi_cb *cb;
	uint nbytes, nblocks;
	int status = 0;

	ASSERT(tp->tm_blksz > 0);
	ASSERT(bp->b_resid == bp->b_bcount); /* Caller must set this */
	ASSERT((tp->tm_mflags & TMF_ABORTED) == 0);

	switch (tp->tm_amode) {
	case TMA_LASTIOR:
	case TMA_EOF:
		/*
		 *+ tm-driver devices do not support writing
		 *+ immediately following a read.  No data
		 *+ can be written from the specified drive 
		 *+ until the medium is rewound or repositioned
		 *+ to the end of recorded data.
		 */
		cmn_err(CE_NOTE, 
			"tm%d: illegal write after read", MT_UNIT(bp->b_edev));
		return (EIO);

	case TMA_OTHER:
		if ((tp->tm_mflags & TMF_ATTEN) != 0) {
			/* 
			 * The medium has recently been changed 
			 * since the device was opened.  Fail this 
			 * type of request until actions occur 
			 * indicating its new postion is known,
			 * such as rewinding it or closing the device.
			 */
			return (EIO);
		} else if (tp->tm_pos != TMP_BOT) {
			/* 
			 * Can write only from BOT/EOD; its
			 * not positioned to either 
			 */
			return (EIO);	
		}
		break;			/* Go ahead and start the write */

	case TMA_LASTIOW:
	case TMA_EOD:
		if ((tp->tm_mflags & TMF_ATTEN) != 0) {
			/* 
			 * The last write operation terminated in
			 * a failure, for which the tape has yet
			 * to be repositioned/recovered prior to
			 * trying another write; don't permit another
			 * write until this has been corrected.
			 */
			return (EIO);
		}
		break;			/* Go ahead and read the data */
	}

	if (tp->tm_pos == TMP_EW) {
		/* 
		 * A write to early warning has occurred, 
		 * but the writter has not been notified
		 * yet, so its not yet an error.  Report
		 * it now and set up so further attempts
		 * error out.
		 */
		tp->tm_pos = TMP_EOM;
		return (0);
	} else if (tp->tm_pos == TMP_EOM) {
		/*
		 * Write past physical end of medium 
		 * or continued writing past a reported
		 * early warning is an error.
 		 */
		return (ENOSPC);
	}

	/* 
	 * Set up a DMA mapping for the transfer 
	 * and then build an SSM/SCSI write command 
 	 * to execute the request.
	 */
	nbytes = min(tp->tm_dp->max_xfer * 1024, bp->b_resid); 
	if ((status = tm_dma_map(tp, bp, nbytes)) != 0)
		return (status);	/* DMA mapping failed */

	nblocks = nbytes / tp->tm_blksz;
	cb = tp->tm_cbs;
	bzero(SWBZERO(cb), SWBZERO_SIZE);
	cb->sh.cb_cmd = SCB_WRITE | SCB_IENABLE;
	cb->sh.cb_clen = SCSI_CMD6SZ;
	cb->sh.cb_scmd[0] = SCSI_WRITE;
	cb->sh.cb_scmd[1] = SCSI_FIXED_BLOCKS;
	cb->sh.cb_scmd[2] = (unchar)(nblocks >> 16);
	cb->sh.cb_scmd[3] = (unchar)(nblocks >> 8);
	cb->sh.cb_scmd[4] = (unchar)nblocks;
	cb->sh.cb_addr = cb->sw.cb_data;
	cb->sh.cb_iovec = (ulong_t *)cb->sw.cb_iovpaddr;
	cb->sh.cb_count = nbytes;
	cb->sh.cb_compcode = SCB_BUSY;

	/* 
	 * Notify the SSM to execute request, then wait for
	 * its completion which is recieved by tmintr().  
	 * Prior to tmintr() signalling its completion it
	 * will set status indicating if it completed
 	 * successfully, had an SSM/SCSI failure, or had
	 * a check condition.  For check conditions it will
	 * make some state transitions for EOF/EOM/BOT, set
	 * the residual count, and sense key if applicable.
 	 * Also, TMF_FAIL is set by the completion handler
	 * for all check conditions or SSM/SCSI failures.  
	 * Its up to the code below to determine if its not 
	 * really a failure.
	 */
	(void)LOCK(tp->tm_lock, plhi);
	cb->sw.cb_state = TM_CB_ACTIVE;
	tp->tm_mflags &= ~TMF_FAIL;
	slic_mIntr(cb->sw.cb_slic, SCSI_BIN, cb->sw.cb_unit_index);
	SV_WAIT(tp->tm_cwait, pritape, tp->tm_lock);
	ASSERT(cb->sw.cb_state != TM_CB_ACTIVE);

	ASSERT (tp->tm_key == -1 || (tp->tm_mflags & TMF_FAIL) != 0);
	switch (tp->tm_key) {
	case -1:
		/* 
		 * Not check condition.  Determine if it was
		 * a successful completion or SSM/SCSI failure
		 * based on TMF_FAIL.
		 */
		if ((tp->tm_mflags & TMF_FAIL) != 0) {
			tp->tm_open = TMO_ERR;	/* This is a catostrophic */
			return (EIO);		/* SCSI failure. 	  */
		}

		/* Otherwise, the write completed without problems */
		ASSERT(tp->tm_resid == 0);
		tp->tm_pos = TMP_OTHER;
		tp->tm_amode = TMA_LASTIOR;
		goto data_written;

	case RS_NOSENSE: 	
		if (tp->tm_pos == TMP_EOM) {
			/* 
		 	 * The early warning for EOM has been 
			 * detected.  If data was transferred
			 * then we will have to wait for the
			 * next request to return a zero byte
			 * count and then upgrade this to an 
			 * EOM.
			 */
			tp->tm_pos = TMP_EW;
		} else {
			ASSERT(tp->tm_pos == TMP_OTHER 
				|| tp->tm_pos == TMP_BOT);
			tp->tm_pos = TMP_OTHER;
		}
		tp->tm_mflags &= ~TMF_FAIL;
		break;

	case RS_RECERR:		
		/* 
		 * The drive had difficulty writting, but recovered 
		 * from it; this is not really a failure.
		 */
		ASSERT(tp->tm_pos == TMP_OTHER || tp->tm_pos == TMP_BOT);
		tp->tm_pos = TMP_OTHER;
		tp->tm_mflags &= ~TMF_FAIL;
		break;

	case RS_OVFLOW:
		if (tp->tm_pos == TMP_EOM) {
			/* Physical EOM reported; handled below */
			tp->tm_mflags &= ~TMF_FAIL;
		}
		break;		
		
	case RS_MEDERR:
		/* 
		 * An unrecoverable data error occurred
		 * during the transfer, which is catostrophic.
		 */
		tp->tm_open = TMO_ERR;
		break;

	case RS_NOTRDY:		/* Medium not inserted or not loaded */
	case RS_HRDERR:		/* Parity or hard drive error */
		tp->tm_pos = TMP_OTHER;		/* We don't know where it is */
		tp->tm_open = TMO_ERR;		/* These are catostrophic */
		tp->tm_amode = TMA_OTHER;
		break;

	case RS_ILLREQ:			/* Most likely not at BOT or EOD. */
	case RS_PROTECT:		/* Data is protected against writes. */
		break;		

	case RS_UNITATTN:	/* Cartridge replaced since last operation */
		tp->tm_amode = TMA_OTHER;
		tp->tm_pos = TMP_BOT;
		tp->tm_mflags |= TMF_ATTEN;
		break;

	default:
		tp->tm_open = TMO_ERR;		/* Handle unexpected cases */
		break;
	}

	if ((tp->tm_mflags & TMF_FAIL) != 0) {
		return (EIO);			/* The read errored out */
	} else if (tp->tm_resid > nblocks) {
		tp->tm_open = TMO_ERR;		/* Unexpected, fatal error */
		return (EIO);	
	} 

	if (tp->tm_resid != 0) {
		/* 
		 * Re-calculated bytes read based on 
		 * nblocks since nbytes may not be a
		 * device blocksize multiple.
		 */
		nbytes = (nblocks - tp->tm_resid) * tp->tm_blksz;
		
	}

	if (nbytes == 0) {
		if (tp->tm_pos == TMP_EW) {
			tp->tm_pos = TMP_EOM;	/* early warning reported. */
		} else if (tp->tm_pos == TMP_EOM) {
			return (ENOSPC);	/* Report physical EOM */
		}
	}

data_written:
	bp->b_resid -= nbytes;
	tp->tm_amode = TMA_LASTIOW;
	return (0);
}

/*
 * STATIC int
 * tm_probe_cmd(const uchar_t, volatile struct scsi_cb *)
 *	Execute the requested SCSI command in polled mode,
 *	using the provided SSM/SCSI cb. 
 *
 * Calling/Exit State:
 *	Caller may hold locks or sleep locks upon entry/exit.
 *
 *	'cmd' must be one of SCSI_TEST or SCSI_INQUIRY.
 *
 *	The cb must be initialized to contain the slic address
 *	for the SSM/SCSI, the cb's unique i.d. code, the physical/
 *	virtual address and length of its sense buffer.  It must
 *	be owned exclusively by the caller and will have been
 *	updated by the SSM after command execution completes.
 *	
 *	Returns one upon successful command completion, zero otherwise.
 *
 * Remarks:
 *	"successful command completion" includes the possibility that
 *	a check condition occurred and sense data was recieved.
 *
 *	The cb's request sense buffer will also be used for inquiry
 *	data, since only one or the other will be valid at any given
 *	time.
 *	
 *	Logical unit number is assumed to be zero (embedded target).
 */		
STATIC int
tm_probe_cmd(const uchar_t cmd, volatile struct scsi_cb *cb)
{
	pl_t saved_pl;

	/* Fill out the CB for the SCSI command */
	ASSERT(cmd == SCSI_TEST || cmd == SCSI_INQUIRY);
	bzero((caddr_t) SWBZERO(cb), SWBZERO_SIZE);
	cb->sh.cb_cmd = SCB_READ;
	cb->sh.cb_clen = SCSI_CMD6SZ; 
	if  ((cb->sh.cb_scmd[0] = cmd) == SCSI_INQUIRY) { 
		/* Fill in additional CB fields. */
		ASSERT(cb->sh.cb_slen >= sizeof (struct scinq_dev));
		cb->sh.cb_addr = cb->sh.cb_sense;
		cb->sh.cb_count = sizeof (struct scinq_dev);
		cb->sh.cb_scmd[4] = sizeof (struct scinq_dev);
	}

	/* Start the SSM processing the CB and poll for completion. */
	cb->sh.cb_compcode = SCB_BUSY;
	saved_pl = splhi();
	slic_mIntr(cb->sw.cb_slic, SCSI_BIN, cb->sw.cb_unit_index);
	splx(saved_pl);
	while (cb->sh.cb_compcode == SCB_BUSY) 
		continue;

	/*
	 * Return a value appropriate for the
	 * command completion.
	 */
	switch (cb->sh.cb_compcode) {
	case SCB_BAD_CB:
		/*
		 *+ While probing, the tm driver received an
		 *+ SSM completion code indicating an invalid 
		 *+ SCSI command.  This indicates either an 
		 *+ internal tm driver error or an SSM error.
		 */
		cmn_err(CE_WARN,
			"tm_probe - Bad SCSI command block on SSM@SLIC %d",
			 cb->sw.cb_slic);
		return (0);
	case SCB_SCSI_ERR:
		/*
		 *+ While probing, the tm driver received an
		 *+ SSM completion code indicating a SCSI
		 *+ protocol error.  This indicates a problem
		 *+ with either the SSM or a device on the SCSI bus.
		 */
		cmn_err(CE_WARN,
			"tm_probe - Protocol error on SSM@SLIC %d",
			cb->sw.cb_slic);
		return (0);
	case SCB_NO_TARGET:
		/* 
		 * Selection phase timed out; no such 
		 * target adapter. Note: if target adapter 
		 * present and logical unit is not, a 
		 * SCSI_CHECK_CONDITION would result, 
		 * with cb_compcode == SCB_OK.
		 */
		return (0);
	case SCB_OK:
		return (1);		/* Command completed successfully */
	default:
		/* Shouldn't happen... */
		/*
		 *+ The tm driver received an unknown completion
		 *+ code for a SCSI command.  
		 */
		cmn_err(CE_PANIC,
			"tm_probe - Bad SSM/SCSI status on SSM@SLIC %d\n", 
			cb->sw.cb_slic);
	}
	/*NOTREACHED*/
}

/*
 * STATIC int 
 * tm_probe(const unchar_t unit)
 *	Determine if a supported tape drive is present at the 
 *	specified SCSI target adapter address.
 *
 * Calling/Exit State:
 *	Caller may hold locks or sleep locks upon entry/exit.
 *
 *	'unit' is the target adapter address where the drive is
 *	to be looked for, the primary SSM's SCSI bus is assumed.
 *	
 *	If found, returns the address of the devices corresponding
 *	entry in tm_drives table.  Returns NULL otherwise.
 *
 * Description:
 *	First, initialize adapter resources so we can communicate
 *	with the specified target adapter.  This consists of filling
 *	out a scb_init structure, then invoking init_ssm_scsi_dev()
 *	to allocate CBs (message blocks) and a unique id number for 
 *	probing.  Then allocate a buffer to be used for both inquiry
 *	data and sense data if a check condition occurs.  It will
 *	be deallocated upon exit.
 *
 *	Issue an INQUIRY command to the specified SCSI target
 *	to determine if the target will respond and, if so,
 *	use the returned inquiry data to determine if the responder
 *	is a supported tape drive, according to the tm_drives table.
 *
 *	The INQUIRY may fail with a SCSI check condition if the drive
 *	is simply reporting that it was recently powered up or reset.
 *	In that case, it may take a few TEST-UNIT-READY commands to
 *	clear the reported condition, afterwhich the INQUIRY is retried.
 *
 * Assumptions:
 *	Called during either system bootup or a dynamic driver load,
 *	in which case there should be no locking required within this
 *	function, including calls to the SSM initialization services.  
 *
 *	Usable drives are already powered on, but do not necessarily 
 *	have medium installed.
 */
STATIC tmdrive_t *
tm_probe(const uchar_t unit)
{
	volatile struct scsi_cb *cb;
	struct scb_init sinit;
	struct scinq_dev *inquiry;
	tmdrive_t *dp;
	int i, compcode;

	/* Provide SSM parameters for probing.  */
	bzero(&sinit, sizeof (struct scb_init));
	sinit.si_mode = SCB_PROBE;
	sinit.si_scsi = 0;	/* Only a single SCSI bus for NOW */
	sinit.si_target = unit;
	sinit.si_lunit = 0;
	sinit.si_flags = SCBF_ASYNCH;
	init_ssm_scsi_dev(&sinit);
	if (sinit.si_id < 0)
		return (NULL); 	/* An initialization failure occurred */

	ASSERT(sinit.si_cbs);
	cb = sinit.si_cbs;	/* CB to use for SCSI probe commands */
	cb->sw.cb_unit_index = SCVEC(sinit.si_id, 0);
	cb->sw.cb_slic = sinit.si_ssm_slic;
	cb->sw.cb_vsense = (vaddr_t) 
		ssm_scsi_alloc(SIZE_MAXDATA, tm_alloc_wait);
	if (! cb->sw.cb_vsense) {
		return (NULL);
	}
	cb->sh.cb_sense = vtop((caddr_t)(void *)cb->sw.cb_vsense, NULL);
	cb->sh.cb_slen = SIZE_MAXDATA;

	/* 
	 * Determine if a supported tape drive is present.
	 */
	if ((compcode = tm_probe_cmd(SCSI_INQUIRY, cb)) == 0) {
		ssm_scsi_free((void *)cb->sw.cb_vsense, SIZE_MAXDATA);
		return (NULL);	/* Target is not usable */
	}

	if (SCSI_CHECK_CONDITION(cb->sh.cb_status)) {
		for (i = 4; i && tm_probe_cmd(SCSI_TEST, cb) 
		     && SCSI_CHECK_CONDITION(cb->sh.cb_status); i--)
			continue;
		compcode = tm_probe_cmd(SCSI_INQUIRY, cb);
	}

	inquiry = (struct scinq_dev *) cb->sw.cb_vsense;
	if (compcode == 0 || !SCSI_GOOD(cb->sh.cb_status)
	||  inquiry->sc_hdr.ih_devtype != INQ_SEQ 
	||  inquiry->sc_hdr.ih_qualif != INQ_REMOVABLE) {
		ssm_scsi_free((void *)cb->sw.cb_vsense, SIZE_MAXDATA);
		return (NULL);		/* Tape drive not found */
	}

	/* 
	 * Look for a match in the drive info table. 
	 * Both the vendor and product i.d. from the
	 * table and inquiry data must match.  If found, 
	 * return address of table entry.
	 */
	for (i = 0, dp = &tm_drives[0]; i < tm_num_drive_types; i++, dp++) {
		if (strncmp(dp->vendor, inquiry->sc_vendor, 
			  strlen(dp->vendor)) == 0
		&&  strncmp(dp->product, inquiry->sc_product, 
			  strlen(dp->product)) == 0) {
			ssm_scsi_free((void *)cb->sw.cb_vsense, SIZE_MAXDATA);
			return (dp);	/* Match found */
                }
	}
	ssm_scsi_free((void *)cb->sw.cb_vsense, SIZE_MAXDATA);
	return (NULL);			/* Supported device not found */
}

/*
 * STATIC void
 * tm_report_sense_info(tm_t *, volatile struct scsi_cb *, int, caddr_t)
 *	Display information about a SCSI check condition.
 *
 * Calling/Exit State:
 *	Basic locks may be held upon entry/exit.
 *
 *	'tp' addresses a valid tm-device info. structure,
 *	to which 'cb' and 'unit' apply.  'cb' is a recently
 *	completed SSM/SCSI command block which presumably 
 *	contains request sense data in its sense buffer.
 *	The caller must have exclusive access to these
 *	structures upon entry/exit.
 *
 *	'msg' may address diagnostic text to be displayed along 
 *	with a dump of the sense data buffer.  Otherwise, its NULL.
 *
 *	Displays diagnostic information about the CBs completion
 *	if the error is considered critical or if the driver is
 *	currently configured to display sense data for the type of
 *	operation which was executed (determined by tp->tm_mflags). 
 *
 *	No return value.
 */
STATIC void
tm_report_sense_info(tm_t  *tp, volatile struct scsi_cb *cb, int unit, caddr_t msg)
{
	unchar *cp;
	struct scrsense *sensebuf;
	uint command;
	int count = 0;
	int printflag;

	sensebuf = (struct scrsense *)cb->sw.cb_vsense;
	command = (uint)cb->sh.cb_scmd[0];
	switch  (tp->tm_key) {
	case RS_MEDERR:
	case RS_HRDERR:
	case RS_ILLREQ:
	case RS_VENDUNIQ:
	case RS_ABORT:
	case RS_RESKEY:
		printflag = 1;
		break;
	default:
		printflag = 0;
		break;
	}

	if ((tp->tm_mflags & TMF_PRSENSE) != 0 
	||  printflag || msg
	    && ((tp->tm_mflags & TMF_RSENSE) != 0 && command == SCSI_READ 
	        || (tp->tm_mflags & TMF_SSENSE) != 0 && command == SCSI_SPACE
	        || (tp->tm_mflags & TMF_WSENSE) != 0 
		   && (command == SCSI_WRITE || command == SCSI_WFM))) { 
		/* 
		 * Attempt to display the message and 
		 * sense buffer data.  If displayed
		 * NULL the message pointer field to
		 * prevent additional printing of it.
		 */
		if (msg) {
			cmn_err(CE_CONT, "tm%d: %s on command %s", unit,
				msg, tm_scsi_commands[command]);
			cmn_err(CE_CONT, "; error code=%x", 
			       sensebuf->rs_error & (RS_ERRCODE | RS_ERRCLASS));
			if (sensebuf->rs_key & RS_FILEMARK)
				cmn_err(CE_CONT, "; filemark");
			if (sensebuf->rs_key & RS_EOM)
				cmn_err(CE_CONT, "; end of media");
			cmn_err(CE_CONT, "\n");
		}
		cmn_err(CE_CONT, "tm%d: sense buf:", unit);
		for (cp = (unchar *)sensebuf, 
		     count = cb->sh.cb_slen; count > 0; count--)
			cmn_err(CE_CONT, "%x ", *cp++);
		cmn_err(CE_CONT, "\ntm%d: cmd buf:", unit);
		for (cp = (unchar *)cb->sh.cb_scmd, 
		     count = cb->sh.cb_clen; count > 0; count--)
			cmn_err(CE_CONT, "%x ", *cp++);
		cmn_err(CE_CONT, "\n");
	}
}

/*
 * STATIC int
 * tm_tdc3600_open(tm_t *, const dev_t, const int)
 * 	Drive specific open code for a Tandberg/3600 series tape drive.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Medium must be installed in the drive and is
 *	generally assumed to be positioned where desired.
 *
 *	Returns zero if the completes successfully.
 *	Otherwise returns: 
 *		EACCES if the medium is write protected and
 *		an attempt to open in write mode is made.
 *
 *		ENXIO if the minor device is unsupported.
 *
 *		EIO if the minor device specifies an unacceptable 
 *		access mode for the devices current state (i.e.,
 *		QIC-120 or attempting to select a new density while
 *		in the middle of a tape using a different density).
 *
 *		Errnos returned by functions called within.
 *
 * Description:
 *	Confirm that the minor device number is potentially supported.
 * 	Then retrieve the current drive settings and the medium's 
 *	current position.  
 *
 *	Fail the request if the drive is write-protected and write 
 *	access is requested.  If the medium is at BOT attempt to select 
 *	new modes to ensure proper setting of drive tunables, but only 
 *	change the denisity for write access; it auto-selects for read.  
 *	If mode-selection fails, fail the request.  if not at BOT and
 *	write access is requested, but the desired and current density
 *	differ, then fail the request since mode selection to change
 *	it can only occur at BOT.  Otherwise assume the density and 
 *	tunables are now correctly set and return success.
 *
 * Remarks:
 * 	Set the drive's read/write buffer thresholds to 1/3 of its
 *	internal fifo buffer size to facilitate streaming without 
 *	host buffering; the drive's fifo should suffice.  Since the 
 *	fifo-buffer size may vary from drive to drive, simply scale
 *	the buffer size returned by mode-sense.  The defaults are 
 *	known to be insufficient.
 */
STATIC int
tm_tdc3600_open(tm_t *tp, const dev_t dev, const int oflag)
{
	int density, status = 0;
	struct tm_mode_sense *ms;

	ASSERT(tp->tm_buffer == NULL);
	if ((oflag & (FNDELAY | FNONBLOCK)) == 0) {
		tp->tm_buffer = ssm_scsi_alloc(tp->tm_dp->mlen, KM_SLEEP);
	} else {
		tp->tm_buffer = ssm_scsi_alloc(tp->tm_dp->mlen, KM_NOSLEEP);
		if (tp->tm_buffer == NULL) 
			return (EAGAIN);	/* alloc failed; can't wait */
	}
	
	if ((status = tm_gen_scsi_cmd(SCSI_MSENSE, tp , 0 , 0)) != 0)
		goto exit_open;		/* mode-sense failed */

	ms = (struct tm_mode_sense *) (void *)tp->tm_buffer;

	if ((ms->t_hdr.mh_mode & MSENSE_WP) != 0 && (oflag & FWRITE) != 0) { 
		status = EACCES; 
		goto exit_open; 	/* Medium is write protected */
	}

	switch MT_DENSITY(minor(dev)) {
	case MTD_NC:
		density = ms->t_bd.mb_density;	
		break;
	case MTD_DEF:
		density = MODE_DEN_DEFAULT;
		break;
	case MTD_Q11:
		density = MODE_DEN_QIC11_9;
		break;
	case MTD_Q24:
		density = MODE_DEN_QIC24;
		break;
	case MTD_Q120:
		density = MODE_DEN_QIC120;
		break;
	case MTD_Q150:
		density = MODE_DEN_QIC150;
		break;
	default:
		status = ENXIO;
		goto exit_open;
	}

	if (tp->tm_pos == TMP_BOT) {
		/* 
		 * Ensure that the threshold, etc. values are 
		 * updated, but don't change the density unless
		 * write access is being requested. It autoselects
		 * for read densities.
		 */
		ms->t_hdr.mh_sdlen = 0;	/* clear reserved fields */
               	ms->t_hdr.mh_mtype = 0;
               	ms->t_hdr.mh_mode = MSEL_BFM_ASYNC | MSEL_SPD_DEFAULT;
                ms->t_hdr.mh_dlen = sizeof(struct scmode_blkd);
		if ((oflag & FWRITE) != 0)
			ms->t_bd.mb_density = (uchar_t)density;
		ASSERT(((ms->t_bd.mb_bsize[0] << 16) 
			| (ms->t_bd.mb_bsize[1] << 8)
			| ms->t_bd.mb_bsize[2]) == TM_SMALL_BLOCK);
                ms->vendor.write_threshold = ms->vendor.buffer_size / 3;
                ms->vendor.read_threshold = ms->vendor.buffer_size / 3;
                ms->vendor.load_function = TBERG_AUTO_LOAD;
		if ((status = tm_gen_scsi_cmd(SCSI_MODES, tp, 0 ,0)) != 0)
			goto exit_open;		/* mode-select failed */
	} else if ((oflag & FWRITE) != 0 && ms->t_bd.mb_density != density
		   && density != MODE_DEN_DEFAULT) {
		status = EIO;; 
		goto exit_open;		/* Invallid write density */
	}
	tp->tm_blksz = TM_SMALL_BLOCK;	/* Use small, fixed size blocks */

exit_open:
	ssm_scsi_free(tp->tm_buffer, tp->tm_dp->mlen);
	tp->tm_buffer = NULL;
	return (status);
}

/*
 * STATIC int
 * tm_tdc3840_open(tm_t *, const dev_t, const int)
 * 	Drive specific open code for a Tandberg/3840 tape drive.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Medium must be installed in the drive and is
 *	generally assumed to be positioned where desired.
 *
 *	Returns zero if the completes successfully.
 *	Otherwise returns: 
 *		EACCES if the medium is write protected and
 *		an attempt to open in write mode is made.
 *
 *		ENXIO if the minor device is unsupported.
 *
 *		EIO if the minor device specifies an unacceptable 
 *		access mode for the devices current state (i.e.,
 *		QIC-24 or attempting to select a new density while
 *		in the middle of a tape using a different density).
 *
 *		Errnos returned by functions called within.
 *
 * Description:
 *	Confirm that the minor device number is potentially supported.
 *	Then retrieve the current drive settings and the medium's 
 *	current position.  
 *
 *	Fail the request if the drive is write-protected and write 
 *	access is requested.  If the medium is at BOT attempt to select 
 *	new modes to ensure proper setting of drive tunables, but only 
 *	change the denisity for write access; it auto-selects for read.  
 *	If mode-selection fails, fail the request.  If not at BOT and
 *	write access is requested, but the desired and current density
 *	differ, then fail the request since mode selection to change
 *	it can only occur at BOT.  Otherwise assume the density and 
 *	tunables are now correctly set and return success.
 *
 * Remarks:
 * 	Set the drive's read/write buffer thresholds to 1/3 of its
 *	internal fifo buffer size to facilitate streaming without 
 *	host buffering; the drive's fifo should suffice.  Since the 
 *	fifo-buffer size may vary from drive to drive, simply scale
 *	the buffer size returned by mode-sense.  The defaults are 
 *	known to be insufficient.
 *
 *	Only allow the drive to perform auto-write-density selection
 *	if the default density has been selected; select it explicitly
 *	otherwise.  Densities for reading will always be auto-selected.
 */
STATIC int
tm_tdc3840_open(tm_t *tp, const dev_t dev, const int oflag)
{
	int density, status = 0;
	struct tm_mode_sense *ms;

	ASSERT(tp->tm_buffer == NULL);
	if ((oflag & (FNDELAY | FNONBLOCK)) == 0) {
		tp->tm_buffer = ssm_scsi_alloc(tp->tm_dp->mlen, KM_SLEEP);
	} else {
		tp->tm_buffer = ssm_scsi_alloc(tp->tm_dp->mlen, KM_NOSLEEP);
		if (tp->tm_buffer == NULL) 
			return (EAGAIN);	/* alloc failed; can't wait */
	}
	
	if ((status = tm_gen_scsi_cmd(SCSI_MSENSE, tp , 0 , 0)) != 0)
		goto exit_open;		/* mode-sense failed */
	ms = (struct tm_mode_sense *) (void *)tp->tm_buffer;

	if ((ms->t_hdr.mh_mode & MSENSE_WP) != 0 && (oflag & FWRITE) != 0) { 
		status = EACCES; 
		goto exit_open; 	/* Medium is write protected */
	}

	switch MT_DENSITY(minor(dev)) {
	case MTD_NC:
		density = ms->t_bd.mb_density;	
		break;
	case MTD_DEF:
		density = MODE_DEN_DEFAULT;
		break;
	case MTD_Q120:
		density = MODE_DEN_QIC120;
		break;
	case MTD_Q150:
		density = MODE_DEN_QIC150;
		break;
	case MTD_Q525:
		density = MODE_DEN_QIC525;
		break;
	default:
		status = ENXIO;
		goto exit_open;
	}

	if (tp->tm_pos == TMP_BOT) {
		/* 
		 * Ensure that the threshold, etc. values are 
		 * updated, but don't change the density unless
		 * write access is being requested. It autoselects
		 * for read densities and does the same for write
		 * densities if the default density is selected.
		 */
		ms->t_hdr.mh_sdlen = 0;	/* clear reserved fields */
               	ms->t_hdr.mh_mtype = 0;
               	ms->t_hdr.mh_mode = MSEL_BFM_ASYNC | MSEL_SPD_DEFAULT;
                ms->t_hdr.mh_dlen = sizeof(struct scmode_blkd);

		if ((oflag & FWRITE) != 0) {
			ms->t_bd.mb_density = (uchar_t)density;
			if (density == MODE_DEN_DEFAULT) {
				/* Have the drive select the best density */
				ms->vendor.control_flags = TBERG_AUTOREAD;
			} else {
				/* Explicitly select the write density */
				ms->vendor.control_flags = 
					TBERG_AUTOREAD | TBERG_DADS;
			}
		}

		ms->t_bd.mb_bsize[0] = (uchar_t)((ulong_t)TM_SMALL_BLOCK >> 16);
		ms->t_bd.mb_bsize[1] = (uchar_t)((ulong_t)TM_SMALL_BLOCK >> 8);
		ms->t_bd.mb_bsize[2] = (uchar_t)TM_SMALL_BLOCK;
                ms->vendor.write_threshold = TBERG_3840_BSIZE / 3;
                ms->vendor.read_threshold = TBERG_3840_BSIZE / 3;
                ms->vendor.load_function = TBERG_AUTO_LOAD;

		if ((status = tm_gen_scsi_cmd(SCSI_MODES, tp, 0 ,0)) != 0)
			goto exit_open;		/* mode-select failed */
	} else if ((oflag & FWRITE) != 0 
	           && ms->t_bd.mb_density != density 
		   && density != MODE_DEN_DEFAULT) {
		status = EIO;; 
		goto exit_open;		/* Invallid write density */
	}
	tp->tm_blksz = TM_SMALL_BLOCK;	/* Use small, fixed size blocks */

exit_open:
	ssm_scsi_free(tp->tm_buffer, tp->tm_dp->mlen);
	tp->tm_buffer = NULL;
	return (status);
}

/*
 * STATIC int
 * tm_tdc3600_set_density(tm_t *, const int)
 * 	Drive specific code for setting drive density for
 *	Tandberg 3600 series tape drives.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Caller must have exclusive access to the drive
 *	and is presumed to have validated the density
 *	to some extent (this function will not). 
 *
 *	Returns zero if the completes successfully.
 *	Otherwise returns an applicable errno. 
 *
 * Description:
 * 	To change the drive's density, first execute a mode-sense to 
 *	read the drive's current settings.  Then set the new density 
 *	value in the mode-sense data, then finally perform a 
 *	mode-select back to the drive.
 *
 *	tm_gen_scsi_cmd() assumes the caller is using tp->tm_buffer 
 *	as the mode sense/select buffer of size tp->tm_dp->mlen bytes.  
 *	So, allocate a buffer for changing modes and deallocate it
 *	upon completion. 
 */
STATIC int
tm_tdc3600_set_density(tm_t *tp, const int density)
{
	int status = 0;
	struct tm_mode_sense *ms;

	ASSERT(tp->tm_buffer == NULL);
	tp->tm_buffer = ssm_scsi_alloc(tp->tm_dp->mlen, KM_SLEEP);

	if ((status = tm_gen_scsi_cmd(SCSI_MSENSE, tp , 0 , 0)) == 0) {
		/* 
		 * Mode-sense succeeded; set new density.
		 * Some other fields must be set to clear
		 * values returned during sense, but invalid
		 * to set.
		 */
		ms = (struct tm_mode_sense *) (void *)tp->tm_buffer;
		ms->t_hdr.mh_sdlen = 0;	/* clear reserved fields */
               	ms->t_hdr.mh_mtype = 0;
               	ms->t_hdr.mh_mode = MSEL_BFM_ASYNC | MSEL_SPD_DEFAULT;
                ms->t_hdr.mh_dlen = sizeof(struct scmode_blkd);
		ms->t_bd.mb_density = (uchar_t)density;
		status = tm_gen_scsi_cmd(SCSI_MODES, tp, 0 ,0);
	}

	ssm_scsi_free(tp->tm_buffer, tp->tm_dp->mlen);
	tp->tm_buffer = NULL;
	return (status);
}

/*
 * STATIC int
 * tm_tdc3840_set_density(tm_t *, const int)
 * 	Drive specific code for setting drive density for
 *	Tandberg 3800 tape drive.
 *
 * Calling/Exit State:
 *	Basic locks must not be held upon entry/exit.
 *
 *	Caller must have exclusive access to the drive
 *	and is presumed to have validated the density
 *	to some extent (this function will not). 
 *
 *	Returns zero if the completes successfully.
 *	Otherwise returns an applicable errno. 
 *
 * Description:
 * 	To change the drive's density, first execute a mode-sense to 
 *	read the drive's current settings.  Then set the new density 
 *	value in the mode-sense data, then finally perform a 
 *	mode-select back to the drive.
 *
 *	tm_gen_scsi_cmd() assumes the caller is using tp->tm_buffer 
 *	as the mode sense/select buffer of size tp->tm_dp->mlen bytes.  
 *	So, allocate a buffer for changing modes and deallocate it
 *	upon completion. 
 *
 * Remarks:
 *	This is nearly the same as tm_tdc3600_set_density(), except that
 *	for that the 3840 has AUTO-READ and DISABLE-AUTO-DENSITY bits 
 *	whose values vary based on the density being set.
 *	
 */
STATIC int
tm_tdc3840_set_density(tm_t *tp, const int density)
{
	int status = 0;
	struct tm_mode_sense *ms;

	ASSERT(tp->tm_buffer == NULL);
	tp->tm_buffer = ssm_scsi_alloc(tp->tm_dp->mlen, KM_SLEEP);

	if ((status = tm_gen_scsi_cmd(SCSI_MSENSE, tp , 0 , 0)) == 0) {
		/* 
		 * Mode-sense succeeded; set new density.
		 * Some other fields must be set to clear
		 * values returned during sense, but invalid
		 * to set.
		 */
		ms = (struct tm_mode_sense *) (void *)tp->tm_buffer;
		ms->t_hdr.mh_sdlen = 0;	/* clear reserved fields */
               	ms->t_hdr.mh_mtype = 0;
               	ms->t_hdr.mh_mode = MSEL_BFM_ASYNC | MSEL_SPD_DEFAULT;
                ms->t_hdr.mh_dlen = sizeof(struct scmode_blkd);
		ms->t_bd.mb_density = (uchar_t)density;
		if (density == MODE_DEN_DEFAULT) {
			/* Have the drive select the best density */
			ms->vendor.control_flags = TBERG_AUTOREAD;
		} else {
			/* Explicitly select the write density */
			ms->vendor.control_flags = 
				TBERG_AUTOREAD | TBERG_DADS;
		}

		status = tm_gen_scsi_cmd(SCSI_MODES, tp, 0 ,0);
	}

	ssm_scsi_free(tp->tm_buffer, tp->tm_dp->mlen);
	tp->tm_buffer = NULL;
	return (status);
}
