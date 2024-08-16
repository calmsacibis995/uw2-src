/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/target/sd01/sd01.c	1.172"
#ident	"$Header: $"

#include	<util/types.h>
#include	<io/vtoc.h>
#include	<io/target/sd01/fdisk.h>  /* Included for 386 disk layout */

#include	<fs/buf.h>		/* Included for dma routines   */
#include	<io/conf.h>
#include	<io/elog.h>
#include	<io/open.h>
#include	<io/target/altsctr.h>
#include	<io/target/alttbl.h>
#include	<io/target/scsi.h>
#include	<io/target/sd01/sd01.h>
#include	<io/target/sd01/sd01_ioctl.h>
#include	<io/target/sdi/dynstructs.h>
#include	<io/target/sdi/sdi.h>
#include	<io/target/sdi/sdi_edt.h>
#include	<io/target/sdi/sdi_hier.h>
#include	<io/uio.h>	/* Included for uio structure argument*/
#include	<mem/kmem.h>	/* Included for DDI kmem_alloc routines */
#include	<proc/cred.h>	/* Included for cred structure arg   */
#include	<proc/proc.h>	/* Included for proc structure arg   */
#include	<svc/errno.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/mod/moddefs.h>
#include	<util/param.h>

#ifndef PDI_SVR42
#include	<util/ipl.h>
#include	<util/ksynch.h>
#endif /* PDI_SVR42 */

#include	<io/ddi_i386at.h>
#include	<io/ddi.h>	/* must come last */

#define		DRVNAME	"SD01 DISK driver"
#define		DRVNAMELEN	64
#define		SCSI_NAME	"scsi"
#define		SD01_NAME	"sd01"


STATIC int sd01_load(),
           sd01_unload(),
           sd01cmp(caddr_t, caddr_t, size_t);

MOD_DRV_WRAPPER(sd01, sd01_load, sd01_unload, NULL, DRVNAME);

int
    sd01_dkinit(struct disk *, struct owner *, int),
    sd01_claim(struct owner *op),
    sd01close(dev_t, int, int, struct cred *),
    sd01cmd(struct disk *, char, uint, char *, uint, uint, ushort, int),
    sd01config(struct disk *, minor_t),
    sd01_first_open(struct disk *),
    sd01fdisk(struct ipart *, struct disk *),
    sd01_find_disk(struct owner *op),
    sd01ioctl(dev_t, int, int, int, cred_t *, int *),
    sd01gen_sjq(struct disk *, int, buf_t *, int , struct job *),
    sd01_last_close(struct disk *, int),
    sd01loadvtoc(major_t, minor_t, struct disk *, int),
    sd01open(dev_t *, int, int, struct cred *),
    sd01phyrw(struct disk *, long, struct phyio *, int),
    sd01print(dev_t, char *),
    sd01read(dev_t, struct uio *, struct cred *),
    sd01releasejob(struct job *),
    sd01rm_dev(struct scsi_adr *),
    sd01size(dev_t),
    sd01vtoc_ck(struct disk *, buf_t *, int),
#ifndef PDI_SVR42
    sd01devinfo(dev_t dev, di_parm_t parm, void **valp),
#endif /* !PDI_SVR42 */
    sd01write(dev_t, struct uio *, struct cred *);

struct job * sd01getjob(struct disk *, int);

void 
     sd01_dk_free(struct disk *dp),
     sd01_dk_gc(void),
     sd01_dk_unmark(void),
     sd01ck_badsec(buf_t *),
     sd01comp1(struct job *),
     sd01done(struct job *),
     sd01flush_job(struct disk *),
     sd01insane_dsk(struct disk *),
     sd01intn(struct sb *),
     sd01qresume(),
     sd01queue_hi(struct job *, struct disk *),
     sd01queue_low(struct job *, struct disk *),
     sd01resume(struct disk *),
     sd01retry(struct job *),
     sd01rinit(),
     sd01sendt(struct disk *),
     sd01start(),
     sd01strat0(buf_t *),
     sd01strat1(struct job *, struct disk *, buf_t *),
     sd01strategy(buf_t *);

STATIC void sd01_dotimeout(struct job *, struct disk *, int);

minor_t sd01_disk_to_minor(int);
int sd01_minor_to_disk(minor_t);

#ifdef SD01_DEBUGPR 
void sd01prt_dsk(int),
     sd01prt_job(struct job *),
     sd01prt_jq(struct disk *);
#endif /* SD01_DEBUGPR */

int  sd01send(struct disk *, struct job *, int);

extern void	sd01intb();

extern int	sd01_lightweight;
STATIC int sd01_timeslices;

/* Disk structure pointer
 * Certain assumptions are made for the locking of sd01_dp.
 * Only sd01rinit() will insert entries into sd01_dp[].
 * Only sd01rm_dev() will delete entries from sd01_dp[].
 * sd01rm_dev() will fail if the device is open.
 */
struct disk	*sd01_dp[DK_MAX_DISKS];
#define SD01_DP_LOCK() do {						\
				   pl_t pl; 				\
			           pl = LOCK(sd01_dp_mutex, pldisk);	\
			           if (sd01_dp_lock == 0) {		\
				        sd01_dp_lock = 1; 		\
				        UNLOCK(sd01_dp_mutex, pl);	\
				        break;				\
			           }					\
			        UNLOCK(sd01_dp_mutex, pl);		\
				delay(1);				\
			   } while (1)

/* sets x to 0 if the lock was not acquired, to 1 if it was */
#define SD01_DP_TRYLOCK(x) do {						\
				   pl_t pl; 				\
			           pl = LOCK(sd01_dp_mutex, pldisk);	\
 				   x = 0;				\
			           if (sd01_dp_lock == 0)		\
				        x = sd01_dp_lock = 1; 		\
			        UNLOCK(sd01_dp_mutex, pl);		\
			   } while (0)

#define SD01_DP_UNLOCK() do {						\
				pl_t pl; 				\
			        pl = LOCK(sd01_dp_mutex, pldisk);	\
				sd01_dp_lock = 0;			\
			        UNLOCK(sd01_dp_mutex, pl);		\
			 } while (0)

STATIC lock_t	*sd01_dp_mutex=NULL;
STATIC pl_t	sd01_dp_pl;
STATIC int	sd01_dp_lock=0;

STATIC lock_t	*sd01_resume_mutex=NULL;
STATIC pl_t	sd01_resume_pl;


extern sleep_t	*sdi_rinit_lock;

STATIC LKINFO_DECL( sd01_resume_lkinfo, "IO:sd01:sd01_resume_lkinfo", 0);
STATIC LKINFO_DECL( sd01_dp_lkinfo, "IO:sd01:sd01_dp_lkinfo", 0);
STATIC LKINFO_DECL( sd01dk_lkinfo, "IO:sd01:sd01dk_lkinfo", 0);

#ifdef SD01_DEBUG
daddr_t sd01alts_badspot;
struct disk *dk_badspot;
uint sd01alts_debug = (DKD | HWREA);
#endif /* SD01_DEBUG */

/*
 * STATIC int
 * sd01_load(void)
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
sd01_load(void)
{
	sd01start();
	return(0);
}

/*
 * STATIC int
 * sd01_unload(void)
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
sd01_unload(void)
{
	return(EBUSY);
}

#define	MAXFER_DEFAULT	0x10000

struct 	sb *sd01_fltsbp;		/* SB exclusively for RESUMEs 	*/
struct 	resume sd01_resume;		/* Linked list of disks waiting */

int 	sd01_diskcnt;			/* Number of disks configured,
					 * maintained strictly for debugging */

extern struct head	lg_poolhead;	/* head for dync struct routines */

#ifndef PDI_SVR42
int sd01devflag = D_NOBRKUP | D_MP | D_BLKOFF;
#else /* PDI_SVR42 */
int sd01devflag = D_NOBRKUP;
#endif /* PDI_SVR42 */

/*===========================================================================*/
/* Debugging mechanism that is to be compiled only if -DDEBUG is included
/* on the compile line for the driver                                  
/* DPR provides levels 0 - 9 of debug information.                  
/*      0: No debugging.
/*      1: Entry points of major routines.
/*      2: Exit points of some major routines. 
/*      3: Variable values within major routines.
/*      4: Variable values within interrupt routine (inte, intn).
/*      5: Variable values within sd01logerr routine.
/*      6: Bad Block Handling
/*      7: Multiple Major numbers
/*      8: - 9: Not used
/*============================================================================*/

#define O_DEBUG         0100

/*
 * void
 * sd01start(void)
 *	Initialize the SCSI disk driver. - 0dd01000
 *	This function allocates and initializes the disk driver's data 
 *	structures.  This function also initializes the disk drivers device 
 *	instance array. An instance number (starting with zero) will be 
 *	assigned for each set of block and character major numbers. Note: the
 *	system must allocate the same major number for both the block
 *	and character devices or disk corruption will occur.
 *	This function does not access any devices.
 *
 * Called by: Kernel
 * Side Effects: 
 *	The disk queues are set empty and all partitions are marked as
 *	closed. 
 * ERRORS:
 *	The disk driver can not determine equipage.  This is caused by
 *	insufficient virtual or physical memory to allocate the driver
 *	Equipped Device Table (EDT) data structure.
 *
 *	The disk driver is not fully configured.  This is caused by 
 *	insufficient virtual or physical memory to allocate internal
 *	driver structures.
 *
 * Calling/Exit State:
 *	Acquires sdi_rinit_lock if dynamic load
 */
void
sd01start(void)
{
#ifdef DEBUG
	DPR (1)("\n\t\tSD01 DEBUG DRIVER INSTALLED\n");
#endif

	sd01_diskcnt = 0;

	/* If Sd01 log flag is invalid, set it to default mode. */
	if(Sd01log_marg > 2)
		Sd01log_marg = 0;
	
	/* Setup the linked list for Resuming LU queues */
	/* allocate and initialize its lock */
 	sd01_resume.res_head = (struct disk *) &sd01_resume;
 	sd01_resume.res_tail = (struct disk *) &sd01_resume;

	sd01_resume_mutex = LOCK_ALLOC(TARGET_HIER_BASE+1, pldisk,
				       &sd01_resume_lkinfo, sdi_sleepflag);
	sd01_dp_mutex = LOCK_ALLOC(TARGET_HIER_BASE+2, pldisk,
				   &sd01_dp_lkinfo, sdi_sleepflag);

	if (!sd01_resume_mutex || !sd01_dp_mutex)
	{
		if (sd01_resume_mutex)
			LOCK_DEALLOC(sd01_resume_mutex);
		if (sd01_dp_mutex)
			LOCK_DEALLOC(sd01_dp_mutex);
		/*
		 *+ There was insufficient memory for disk data structures
		 *+ at boot-time.  This probably means that the system is
		 *+ configured with too little memory. 
		 */
		cmn_err(CE_WARN,
			"Insufficient memory to configure disk driver.\n");
		return;
	}

	if ( !(sd01_fltsbp = sdi_getblk(sdi_sleepflag)) )
	{
		/*
		 *+ There was insufficient memory for disk data structures
		 *+ at boot-time.  This probably means that the system is
		 *+ configured with too little memory. 
		 */
		cmn_err(CE_WARN,
			"Insufficient memory to configure disk driver.\n");
		return;
	}

	if (sdi_sleepflag == KM_SLEEP)
		SLEEP_LOCK(sdi_rinit_lock, pridisk);
	sd01rinit();
	if (sdi_sleepflag == KM_SLEEP)
		SLEEP_UNLOCK(sdi_rinit_lock);
}


/*
 * void
 * sd01rinit(void)
 *	Called by sdi to perform driver initialization of additional
 *	devices found after the dynamic load of HBA drivers. 
 *	This function does not access any devices.
 *
 * Calling/Exit State:
 *	sdi_rinit_lock must be held by caller
 */
void
sd01rinit(void)
{
	struct drv_majors drv_maj;
	struct owner	*ownerlist;
	int new_sd01_diskcnt;

	SD01_DP_LOCK();

	drv_maj.b_maj = Sd01_bmajor;
	drv_maj.c_maj = Sd01_cmajor;
	drv_maj.minors_per = DK_MAX_SLICE;
	ownerlist = sdi_doconfig(SD01_dev_cfg, SD01_dev_cfg_size,
				 DRVNAME, &drv_maj, sd01rinit);

	sdi_target_hotregister(sd01rm_dev, ownerlist);

	/* unmark all the disk structures */
	sd01_dk_unmark();

	/* add and new disks */
	new_sd01_diskcnt = 0;
	for (;
	     ownerlist;
	     ownerlist = ownerlist->target_link)
	{
		if (!sd01_claim(ownerlist)) {
			new_sd01_diskcnt++;
#ifdef SD01_DEBUG
			cmn_err(CE_CONT,"ownerlist 0x%x ", ownerlist);
			cmn_err(CE_CONT,"edt 0x%x ", ownerlist->edtp);
			cmn_err(CE_CONT,"hba %d scsi id %d lun %d bus %d\n",
			ownerlist->edtp->scsi_adr.scsi_ctl,
			ownerlist->edtp->scsi_adr.scsi_target,
			ownerlist->edtp->scsi_adr.scsi_lun,
			ownerlist->edtp->scsi_adr.scsi_bus);
#endif
		}
	}
#ifdef SD01_DEBUG
        cmn_err(CE_CONT,"sd01rinit %d disks claimed, previously %d\n",
		new_sd01_diskcnt, sd01_diskcnt);
#endif
	sd01_diskcnt = new_sd01_diskcnt;

	/* free any unclaimed disks */
	sd01_dk_gc();

	if (!sd01_clone)
	{
		minor_t index;

		index = sd01_disk_to_minor(0);
		if (index != 0)
			sd01_dp[0] = DKPTR(index);
	}

	SD01_DP_UNLOCK();
}

/*
 * int 
 * sd01rm_dev()
 * the function to support hot removal of disk drives.
 * This will spin down the disk and remove it from its internal
 * structures, sd01_dp.
 *
 * returns SDI_RET_ERR or SDI_RET_OK
 * will fail if the device doesn't exist, is not owned by sd01,
 * is open.
 */
int sd01rm_dev(struct scsi_adr *sa)
{
	struct sdi_edt *edtp;
	struct disk *dp;
	int index;
	int part;

	ASSERT(sa);
	edtp = sdi_rxedt(sa->scsi_ctl, sa->scsi_bus,
			 sa->scsi_target, sa->scsi_lun);
	
	if ((edtp == NULL) ||	/* no device */
	    (edtp->curdrv == NULL) || /* no owner */
	    (edtp->curdrv->maj.b_maj != Sd01_bmajor)) /* not mine */
		return SDI_RET_ERR;

	SD01_DP_LOCK();

	index = DKINDEX(edtp->curdrv->maj.first_minor);
	dp = sd01_dp[index];

	if (dp == NULL)
	{
		SD01_DP_UNLOCK();
		return SDI_RET_ERR;
	}

	/* check if the disk is open */
        for (part=0; part < V_NUMPAR; part++) {
		if (dp->dk_part_flag[part])	{
			SD01_DP_UNLOCK();
			return SDI_RET_ERR;
		}
	}

	/* check if the disk is open */
        for (part = 0; part < FD_NUMPART + 1; part++) {
		if (dp->dk_fdisk_flag[part])	{
			SD01_DP_UNLOCK();
			return SDI_RET_ERR;
		}
	}

	/* flush cache and spin down the disk */
	if (sd01cmd(dp, SS_LOAD, 0, NULL, 0, 0, SCB_READ, FALSE))
	{
		cmn_err(CE_WARN, "!Disk Driver: Couldn't spin down disk for"
			" removal %d: %d,%d,%d\n", sa->scsi_ctl, sa->scsi_bus,
			sa->scsi_lun, sa->scsi_bus);
	}


	/*
	 * For drivers that unload it is necessary to keep the list
	 * of devices returned by sdi_doconfig().  Whenever a device
	 * is added or removed from the system it is necessary to
	 * add/remove the device from the list.  The item removed
	 * must have its target_link set to NULL.
	 * This is not necessary for SD01 because it is never
	 * unloaded.
	 */
	edtp->curdrv->target_link = NULL;

	sdi_clrconfig(edtp->curdrv, SDI_REMOVE|SDI_DISCLAIM, NULL);

	sd01_dp[index]=NULL;
	sd01_dk_free(dp);
	SD01_DP_UNLOCK();
	return SDI_RET_OK;
	
}

/************************************************************
 * Unmarks all the disks so that garbage collection can
 * take place later
 ************************************************************/
void
sd01_dk_unmark(void)
{
	int index;

	for (index=1;
	     index < DK_MAX_DISKS;
	     index++)
	{
		if (sd01_dp[index] == NULL) continue;
		sd01_dp[index]->marked = 0;
		
	}
}

/************************************************************
 * Free's any disk structures that aren't marked
 ************************************************************/
void 
sd01_dk_gc(void)
{
	int index;

	for (index=1;
	     index < DK_MAX_DISKS;
	     index++)
	{
		if (sd01_dp[index] == NULL) continue;
		if (sd01_dp[index]->marked) continue;

#ifdef SD01_DEBUG
		cmn_err(CE_CONT, "sd01_dk_gc() freeing sd01_dp[%d]=0x%x\n"
			index, sd01_dp[index]);
#endif
		sd01_dk_free(sd01_dp[index]);
		sd01_dp[index] = NULL;
	}
}

/*************************************************************
 *
 * Returns the index into the sd01_dp[] associated with the passed 
 * owner pointer. Returns -1 if it can't be found.
 * NOTE: this is fairly slow as it searches all
 * entries to find the entry that matches the 
 * [Controller, Bus, Target, Lun] in the owner pointer
 ************************************************************/
int 
sd01_find_disk(struct owner *op)
{
	int index;
	int ret;

	ret = -1;

	for (index=1;
	     index < DK_MAX_DISKS;
	     index++)
	{
		if (sd01_dp[index] == NULL) continue;

		if (SDI_ADDRCMP(&(sd01_dp[index]->dk_addr),
				&(op->edtp->scsi_adr))) {
			ret = index;
			break;
		}
	}

	return ret;
}

/*************************************************************
 * Creates, initializes and marks a new disk entry.
 * If the entry already exists it is simply marked but this
 * is not an error.
 *
 * Returns: 0 success, -1 failure
 *************************************************************/
int 
sd01_claim(struct owner *op)
{
	struct disk *dp;
	int index;

	index = sd01_find_disk(op);
	if (index != -1)
	{
		sd01_dp[index]->marked = 1;
		return 0;
	}

	for (index=1;
	     index < DK_MAX_DISKS;
	     index++)
	{
		if (sd01_dp[index] == NULL)
			break;
	}
	
	if (index >= DK_MAX_DISKS)
	{
		
		/*
		 *+ All of the available spaces in sd01_dp
		 *+ have been used.  One is needed for each
		 *+ disk.  Increase the size of sd01_dp 
		 *+ in /etc/conf/pack.d/sd01.cf/Space.c
		 */
		cmn_err(CE_WARN, "Disk Driver: Maximum disk "
			"configuration exceeded.");
		return -1;
	}

	if ((dp = (struct disk *) kmem_zalloc(sizeof(struct disk),
					     sdi_sleepflag)) == NULL)
	{
		/*
		 *+ There was insuffcient memory for disk data structures.
		 *+ This probably means that the system is
		 *+ configured with too little memory. 
		 */
		cmn_err(CE_WARN, "Disk Driver: Insufficient memory to configure driver.");
		return -1;
	}

	if (!sd01_dkinit(dp, op, index))
	{
		return -1;
	}
	/* set major/minor #'s please */
	op->maj.b_maj = Sd01_bmajor;
	op->maj.c_maj = Sd01_cmajor;
	op->maj.minors_per = DK_MAX_SLICE;
	op->maj.first_minor = DKINDEX2MINOR(index);

	sd01_dp[index] = dp;
	sd01_dp[index]->marked = 1;

	return 0;
}

/*
 * int
 * sd01_dkinit(struct disk *dp, struct owner *op, int diskidx)
 *	Allocate dk_lock, initialize disk structure
 *
 */
int
sd01_dkinit(struct disk *dp, struct owner *op, int diskidx)
{

	if ( !(dp->dk_lock = LOCK_ALLOC(TARGET_HIER_BASE, pldisk,
	&sd01dk_lkinfo, sdi_sleepflag)) ||
	!(dp->dk_sv = SV_ALLOC(sdi_sleepflag)) ) {
		/*
		 *+ Insufficient memory to allocate disk data structures
		 *+ when loading driver.
		 */
		cmn_err(CE_WARN, "sd01: dkinit allocation failed\n");
		return (0);
	}

	dp->dk_ident = op->edtp->edt_ident;

	dp->dk_queue = dp->dk_lastlow = dp->dk_lasthi = (struct job *) NULL;
	dp->dk_parms.dp_secsiz    = KBLKSZ;
#ifndef PDI_SVR42
 	dp->dk_stat  = met_ds_alloc_stats("sd01", 
		diskidx,
		dp->dk_parms.dp_heads * 
		dp->dk_parms.dp_cyls *
		dp->dk_parms.dp_sectors,
		(uint)(MET_DS_BLK|MET_DS_NO_RESP_HIST|MET_DS_NO_ACCESS_HIST));
#else
	dp->dk_stat.pttrack      = dp->dk_stat.ptrackq;
	dp->dk_stat.endptrack    = 
		&dp->dk_stat.ptrackq[NTRACK];
#endif
	dp->dk_iotype = op->edtp->iotype;
	dp->dk_addr.sa_ct = SDI_SA_CT(op->edtp->scsi_adr.scsi_ctl, 
				      op->edtp->scsi_adr.scsi_target);
	dp->dk_addr.sa_exta = (uchar_t)(op->edtp->scsi_adr.scsi_target);
	dp->dk_addr.sa_lun = op->edtp->scsi_adr.scsi_lun;
	dp->dk_addr.sa_bus = op->edtp->scsi_adr.scsi_bus;
	/*
	 * Call to initialize the breakup control block.
	 */
	if ((dp->dk_bcbp = sdi_getbcb(&dp->dk_addr, sdi_sleepflag)) == NULL) {
		/*
		 *+ Insufficient memory to allocate disk breakup control
		 *+ block data structure when loading driver.
		 */
 		cmn_err(CE_NOTE, "Disk Driver: insufficient memory to allocate breakup control block.");
		return (0);
	}

	if (((sdi_hba_version(SDI_EXHAN(&(dp->dk_addr))) & HBA_VMASK)
	     >= PDI_UNIXWARE20) &&
	    !(sdi_hba_flag(SDI_EXHAN(&(dp->dk_addr))) & HBA_TIMEOUT) &&
	    (sd01_insane > 0 || sd01_timeout > 0))
	{
		int timeslice;
		if (sd01_insane > 0 && sd01_timeout > 0) {
			dp->dk_state |= DKTIMEOUT|DKINSANE;
			if (sd01_timeout < sd01_insane * 3)
				sd01_timeout = sd01_insane * 3;
			sd01_timeslices = sd01_timeout / sd01_insane;
			timeslice  = sd01_insane;
		} else if (sd01_insane > 0) {
			dp->dk_state |= DKINSANE;
			sd01_timeslices = 3;
			timeslice = sd01_insane;
		} else {
			dp->dk_state |= DKTIMEOUT;
			if (sd01_timeout < 3)
				sd01_timeout = 3;
			sd01_timeslices = 3;
			timeslice = sd01_timeout / 3;
		}
		dp->dk_timeout_id = sdi_timeout(sd01watchdog, (void *)diskidx,
						TO_PERIODIC | timeslice * HZ,
						pldisk, &(dp->dk_addr));
		dp->dk_timeout_list = kmem_zalloc(sizeof(struct job *)
						  * sd01_timeslices, sdi_sleepflag);
		dp->dk_timeout_snapshot = kmem_zalloc(sizeof(struct job *)
						  * sd01_timeslices, sdi_sleepflag);
		if (!dp->dk_timeout_list || !dp->dk_timeout_snapshot)
		{
			cmn_err(CE_NOTE, "Disk Driver: insufficient memory to"
				"allocate timeout list.");
			dp->dk_state &= ~(DKTIMEOUT|DKINSANE);
			return 0;
		}
	}
	    
	return (1);
}

/************************************************************
 *  Frees a disk structure and all locks associated with it.
 ************************************************************/
void sd01_dk_free(struct disk *dp)
{
	LOCK_DEALLOC(dp->dk_lock);
	SV_DEALLOC(dp->dk_sv);
#ifndef PDI_SVR42
	met_ds_dealloc_stats(dp->dk_stat);
#endif
	sdi_freebcb(dp->dk_bcbp);
	if (dp->dk_timeout_id)
		untimeout(dp->dk_timeout_id);
	if (dp->dk_timeout_list != NULL)
		kmem_free(dp->dk_timeout_list, sizeof(struct job *)
			  * sd01_timeslices);
	if (dp->dk_timeout_snapshot != NULL)
		kmem_free(dp->dk_timeout_snapshot, sizeof(struct job *)
			  * sd01_timeslices);
	kmem_free(dp , sizeof(struct disk));
}

/*
 * struct job *
 * sd01getjob(struct disk *dk, int sleepflag)
 *	This function calls sdi_get for struct to be used as a job
 *	structure. If dyn alloc routines cannot alloc a struct the
 *	sdi_get routine will sleep until a struct is available. Upon a
 *	successful return from sdi_get, sd01getjob will then
 *	get a scb from the sdi using sdi_getblk.
 * Called by: sd01strategy and mdstrategy
 * Side Effects: 
 *	A job structure and SCSI control block is allocated.
 * Returns: A pointer to the allocated job structure.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
struct job *
sd01getjob(struct disk *dk, int sleepflag)
{
	register struct job *jp;

	jp = (struct job *) SDI_GET(&lg_poolhead, sleepflag);
	/* Get an SCB for this job */
	jp->j_cont = sdi_getblk(sleepflag);
	jp->j_cont->sb_type = SCB_TYPE;
	jp->j_cont->SCB.sc_dev = dk->dk_addr;
	return(jp);
}

/*
 * void
 * sd01freejob(struct job *jp)
 *	The routine calls sdi_freeblk to return the scb attached to 
 *	the job structure driver. This function then returns the job 
 *	structure to the dynamic allocation free list by calling sdi_free. 
 * Called by: mddone sd01done
 * Side Effects:
 *	Allocated job and scb structures are returned.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
sd01freejob(struct job *jp)
{
	sdi_freeblk(jp->j_cont);
	SDI_FREE(&lg_poolhead, (jpool_t *)jp);
}

/*
 * STATIC struct job *
 * sd01_dequeue(struct disk *dk)
 *
 * Remove and return the job at the head of the dk queue
 *
 * Call/Exit State:
 *
 *	The disks dk_lock must be held upon entry
 *	The lock is held upon exit
 */
STATIC struct job *
sd01_dequeue(struct disk *dk)
{
	struct job *jp = dk->dk_queue;
	if (jp) {
		dk->dk_queue = jp->j_next;
		if (jp == dk->dk_lastlow)
			dk->dk_lastlow = NULL;
		else if (jp == dk->dk_lasthi)
			dk->dk_lasthi = NULL;
	}
	return jp;
}


/* STATIC void
 * sd01sendq(struct disk *dk, int sleepflag)
 *
 * Send all the jobs on the queue 
 *
 * Call/Exit State:
 *      dk_lock must be held upon entry
 *      The lock released before exit
 */
STATIC void
sd01sendq(struct disk *dk, int sleepflag)
{
	struct job *jp;

	if (dk->dk_state & DKSEND) {
		long sid;
		sid = dk->dk_sendid;
		dk->dk_state &= ~DKSEND;
		SD01_DK_UNLOCK(dk);
		untimeout(sid);
		SD01_DK_LOCK(dk);
	}
	while ((jp = sd01_dequeue(dk)) != NULL) {
		if (!sd01send(dk, jp, sleepflag))
			return;
		SD01_DK_LOCK(dk);
	}
	SD01_DK_UNLOCK(dk);
}

/*
 * int
 * sd01send(struct disk *dk, struct job *, int sleepflag)
 *	This function sends a job to the host adapter driver.
 *	If the job cannot be accepted by the host adapter
 *	driver, the function will reschedule itself via the timeout
 *	mechanism. This routine must be called at pldisk.
 *
 * Called by: sd01cmd sd01strat1 sd01sendt
 * Side Effects: 
 *	Jobs are sent to the host adapter driver.
 * Return:
 *	non-zero if another job can be accepted
 *	zero if no more jobs should be sent immediately
 *
 * ERRORS:
 *	The host adapter rejected a request from the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 * Calling/Exit State:
 *	The disks dk_lock must be held on entry
 *	Releases the disk struct's dk_lock.
 */
int
sd01send(struct disk *dk, struct job *jp,  int sleepflag)
{
	int sendret;		/* sdi_send return value 	*/ 
#ifdef PDI_SVR42
	char	ops;
	time_t	lbolt_time;
	time_t	since_lasttime;
#endif
#ifdef SD01_DEBUG
	dk->dk_outcnt++;
	if (sd01alts_debug&DKADDR) 
		cmn_err(CE_CONT,"sd01send: sleepflag= %d \n", sleepflag);
#endif

	/* Update performance stats */
#ifndef PDI_SVR42
	met_ds_queued(dk->dk_stat, jp->j_bp->b_flags);
#else

	dk->dk_count--;
	dk->dk_stat.ios.io_qcnt--;
	ops = (char) *jp->j_cont->SCB.sc_cmdpt;
	if (ops == SM_READ || ops ==  SM_WRITE) {
		drv_getparm(LBOLT, (ulong *)&lbolt_time);
		if (dk->dk_stat.busycnt != 0) {
			since_lasttime = lbolt_time - dk->dk_stat.lasttime;
			dk->dk_stat.io_act += since_lasttime;
			dk->dk_stat.io_resp += since_lasttime * dk->dk_stat.busycnt;
		}
		dk->dk_stat.busycnt++;
		dk->dk_stat.lasttime = lbolt_time;
	}
#endif
	SD01_DK_UNLOCK(dk);

	/* Swap bytes in the address field */
	if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
		jp->j_cmd.cs.ss_addr = sdi_swap16(jp->j_cmd.cs.ss_addr);
	else {
		jp->j_cmd.cm.sm_addr = sdi_swap32(jp->j_cmd.cm.sm_addr);
		jp->j_cmd.cm.sm_len  = (short)sdi_swap16(jp->j_cmd.cm.sm_len);
	}

	sendret = sdi_translate(jp->j_cont, jp->j_bp->b_flags,
			jp->j_bp->b_proc, sleepflag);

	if (sendret == SDI_RET_OK) {

		sd01timeout(jp);

		if (jp->j_cont->sb_type == ISCB_TYPE)
			sendret = sdi_icmd(jp->j_cont, sleepflag);
		else
			sendret = sdi_send(jp->j_cont, sleepflag);
	}
	if (sendret != SDI_RET_OK) {

		sd01untimeout(jp, SDI_ERROR);

		if (sendret == SDI_RET_RETRY) {
			SD01_DK_LOCK(dk);
#ifdef SD01_DEBUG
			dk->dk_outcnt--;
#endif
#ifndef PDI_SVR42
			met_ds_dequeued(dk->dk_stat, jp->j_bp->b_flags);
#else
			dk->dk_count++;
			dk->dk_stat.ios.io_qcnt++;
			if (ops == SM_READ || ops ==  SM_WRITE) {
				dk->dk_stat.busycnt--;
			}
#endif
			/* Swap bytes back in the address field	*/
			if (jp->j_cont->SCB.sc_cmdsz == SCS_SZ)
				jp->j_cmd.cs.ss_addr = sdi_swap16(
						jp->j_cmd.cs.ss_addr);
			else {
				jp->j_cmd.cm.sm_addr = sdi_swap32(
						jp->j_cmd.cm.sm_addr);
				jp->j_cmd.cm.sm_len  = (short)sdi_swap16
						(jp->j_cmd.cm.sm_len);
			}
			/* relink back to the queue */
			sd01queue_hi(jp,dk);

			/* do not hold the job queue busy any more */
			/* Call back later */
			if (!(dk->dk_state & DKSEND)) {
				dk->dk_state |= DKSEND;
				if ( !(dk->dk_sendid =
				sdi_timeout(sd01sendt, (caddr_t) dk, 1, pldisk,
				   &dk->dk_addr))) {
					dk->dk_state &= ~DKSEND;
					/*
					 *+ sd01 send routine could not
					 *+ schedule a job for retry
					 *+ via itimeout
					 */
					cmn_err(CE_WARN, "sd01send: itimeout failed");
				}
			}

			SD01_DK_UNLOCK(dk);

			return 0;
		} else {
#ifdef SD01_DEBUG
			cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
			sd01comp1(jp);
		}
	}
	return 1;
}

/*
 * void
 * sd01sendt(struct disk *dk)
 *	This function call sd01sendq after it turns off the DKSEND
 *	bit in the disk status work.
 * Called by: timeout
 * Side Effects: 
 *	The send function is called and the record of the pending
 *	timeout is erased.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01sendt(struct disk *dk)
{
	int state;

#ifdef DEBUG
        DPR (1)("sd01sendt: (dk=0x%x)\n", dk);
#endif

	SD01_DK_LOCK(dk);

	state = dk->dk_state;
	dk->dk_state &= ~DKSEND;
	

	/* exit if job queue is suspended or the timeout has been cancelled */
	if ((state & (DKSUSP|DKSEND)) != DKSEND) {
		SD01_DK_UNLOCK(dk);
		return;
	}
	sd01sendq(dk, KM_NOSLEEP);

#ifdef DEBUG
        DPR (2)("sd01sendt: - exit\n");
#endif
}

/*
 * void
 * sd01strat1(struct job *jp, struct disk *dk, buf_t *bp)
 *	Level 1 (Core) strategy routine.
 *	This function takes the information included in the job
 *	structure and the buffer header, and creates a SCSI bus
 *	request.  The request is placed on the priority queue.
 *	This routine may be called at interrupt level. 
 *	The buffer and mode fields are filled in by the calling 
 *	functions.  If the partition argument is equal to 16 or
 *	greater the block address is assumed to be physical.
 * Called by: sd01strategy and sd01phyrw.
 * Side Effects: 
 *	A job is queued for the disk.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01strat1(struct job *jp, struct disk *dk, buf_t *bp)
{
	register struct scb *scb;
	register struct scm *cmd;
#ifdef PDI_SVR42
 	struct scsi_iotime *stat = &dk->dk_stat;
#endif
	int sleepflag = KM_SLEEP;

#ifdef DEBUG
        DPR (1)("sd01strat1: jp=0x%x\n", jp);
#endif
	
	scb = &jp->j_cont->sb_b.b_scb;
	cmd = &jp->j_cmd.cm;
	
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = sd01done;
	scb->sc_datapt = jp->j_memaddr;
	scb->sc_datasz = jp->j_seccnt << BLKSHF(dk);
	scb->sc_cmdpt = SCM_AD(cmd);

	/* Fill in the scb for this job */
	if (bp->b_flags & B_READ) {
		cmd->sm_op = SM_READ;
		scb->sc_mode = SCB_READ;
	} else {
		cmd->sm_op = SM_WRITE;
		scb->sc_mode = SCB_WRITE;
	}
	cmd->sm_lun = dk->dk_addr.sa_lun; /* dk_addr - read only once set */
	cmd->sm_res1 = 0;
	cmd->sm_addr = jp->j_daddr;
	cmd->sm_res2 = 0;
	cmd->sm_len = jp->j_seccnt;
	cmd->sm_cont = 0;
	scb->sc_cmdsz = SCM_SZ;
	/* The data elements are filled in by the calling routine */
	scb->sc_link = 0;
	scb->sc_time = JTIME;
	scb->sc_int = sd01intn;
	scb->sc_wd = (long) jp;
	scb->sc_dev = dk->dk_addr;
	
	if (jp->j_flags & J_FIFO)
		sleepflag = KM_NOSLEEP;

	SD01_DK_LOCK(dk);

#ifdef PDI_SVR42
	/* Update performance data */
	dk->dk_count++;
	stat->ios.io_ops++;
	stat->ios.io_qcnt++;
	if (dk->dk_count > stat->maxqlen)
		stat->maxqlen = dk->dk_count;
#endif

	if (dk->dk_queue) {
		sd01queue_low(jp, dk);
		sd01sendq(dk, sleepflag);
	}
	else {
		sd01send(dk, jp, sleepflag);
	}
	

#ifdef DEBUG
        DPR (2)("sd01strat1: - exit\n");
#endif
}

/*
 * void
 * sd01queue_hi(struct job *jp, struct disk *dk)
 *	Add high-priority job to queue
 *
 * Calling/Exit State:
 *	caller holds the disk struct's dk_lock
 */
void
sd01queue_hi(struct job *jp, struct disk *dk)
{
	/* No high priority jobs on queue: stick job in front
	 * in front of (possibly empty) queue.
	 */
	if (!dk->dk_lasthi) {
		jp->j_next = dk->dk_queue;
		dk->dk_lasthi = dk->dk_queue = jp;
		return;
	} 
	/* High priority jobs on queue: append this job to the
	 * existing high priority jobs.  This code should not
	 * be reached because sd01queue_hi() is only called for
	 * retry jobs, and we only retry one job at a time.
	 */
	jp->j_next = dk->dk_lasthi->j_next;
	dk->dk_lasthi->j_next = jp;
	dk->dk_lasthi = jp;
	return;
}

/*
 * void
 * sd01queue_low(struct job *jp, struct disk *dk)
 *	add low priority job to end of queue
 *
 * Calling/Exit State:
 *	Caller holds disk struct's dk_lock.
 */
void
sd01queue_low(struct job *jp, struct disk *dk)
{
	jp->j_next = NULL;
	if (!dk->dk_queue) {
		/* Add first job to queue */
		dk->dk_queue = dk->dk_lastlow = jp;
		return;
	}
	if (dk->dk_lastlow) {
		/* Append to low priority jobs */
		dk->dk_lastlow = dk->dk_lastlow->j_next = jp;
		return;
	}
	/* Add first low priority job to queue */
	dk->dk_lastlow = dk->dk_lasthi->j_next = jp;
	return;
}

/*
 * void
 * sd01strategy(buf_t *bp)
 *	Entry point for block I/O.  Also, entered from raw I/O path
 *	after physiock().
 *	This routine sets up the sdi_devinfo structure for SDI,
 *	including I/O capabilities, max_xfer, etc., and then calls
 *	sdi_breakup().  sd01strat0 is the entry back into sd01,
 *	at which time the I/O requests have been properly broken up,
 *	copied down, remapped, as required.
 * Called by: kernel
 * Side Effects: 
 *	DMA devices have data moved to dmaable memory when necessary.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires disk struct's dk_lock.
 */
void
sd01strategy(buf_t *bp)
{
	register struct disk *dk;
	minor_t minor;
#ifdef PDI_SVR42
	struct scsi_ad	*dev;
	struct sdi_devinfo info;
#endif

	minor = geteminor(bp->b_edev);

	dk =  DKPTR(minor);
	ASSERT(dk);

#ifdef PDI_SVR42
 	dev = &dk->dk_addr;
	info.strat = sd01strat0;
	info.iotype = dk->dk_iotype;
	info.max_xfer = HIP(HBA_tbl[SDI_EXHAN(dev)].info)->max_xfer;
	info.granularity = BLKSZ(dk);
	sdi_breakup(bp, &info);
#else
	if (dk->dk_bcbp_max &&
	   (bp->b_flags &  (B_PAGEIO | B_REMAPPED)) == B_PAGEIO &&
	   bp->b_un.b_addr == NULL)
		buf_breakup(sd01strat0, bp, dk->dk_bcbp_max);
	else
		buf_breakup(sd01strat0, bp, dk->dk_bcbp);
#endif
	return;
}

/*
 * void
 * sd01strat0(buf_t *bp)
 *	sd01strat0  generates the SCSI command needed to fulfill the
 *	I/O request.  The buffer pointer is passed from the kernel and
 *	contains the necessary information to perform the job.  Most of the
 *	work is done by sd01strat1.
 * Called by: sdi_breakup2/physcontig_breakup
 * Side Effects: 
 *	The I/O statistics are updated for the device and an I/O job is
 *	added to the work queue. 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock
 */
void
sd01strat0(buf_t *bp)
{
	register struct disk *dk;
	register daddr_t start;		/* Starting Logical Sector in part */
	register daddr_t last;		/* Last Logical Sector in partition */
	register daddr_t numblk;	/* # of Logical Sectors requested */
	register int part;
	int	 ret_val = 0;
	minor_t  minor;
	int	resid;

	minor = geteminor(bp->b_edev);

	dk =  DKPTR(minor);
	ASSERT(dk);
	part = DKSLICE(minor);

	bp->b_resid = bp->b_bcount;

	if (bp->b_blkno < 0) {
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		biodone(bp);
		return;
	}
	start = bp->b_blkno >> BLKSEC(dk);
	if (part < V_NUMPAR)
		last = dk->dk_vtoc.v_part[part].p_size;
	else
		last = dk->dk_fdisk[part - V_NUMPAR].p_size;
	numblk = (bp->b_bcount + BLKSZ(dk)-1) >> BLKSHF(dk);
	if (bp->b_flags & B_READ)    
	{                          
		if (start + numblk >  last)
		{
			if (start > last)
			{
				bp->b_flags |= B_ERROR;
				bp->b_error = ENXIO;
				biodone(bp);
				return;
			}
			
			resid =  bp->b_bcount - ((last-start)<<BLKSHF(dk));
			if (bp->b_bcount == resid)
			{	/* The request is done */
				bp->b_resid = resid;
				biodone(bp);
				return;
			}
			else
				bp->b_bcount -= resid;
		}
	}
	else	/* This is a write request */
	{
		if ((unsigned)start + numblk >  last) {
			/*
			 * Return an error if entire request is beyond
			 * the End-Of-Media.
			 */
			if (start > last) {
				bp->b_flags |= B_ERROR;
				bp->b_error = ENXIO;
				biodone(bp);
				return;
			}
			
			/*
			 * The request begins exactly at the End-Of-Media.
			 * A 0 length request is OK.  Otherwise, its an error.
			 */ 
			if (start == last) {
				if (bp->b_bcount != 0) {
					bp->b_flags |= B_ERROR;
					bp->b_error = ENXIO;
				}
				biodone(bp);
				return;
			}

			/*
			 * Only part of the request is beyond the End-Of-
			 * Media, so adjust the count accordingly.
			 */ 
			bp->b_bcount -= (last-start)<<BLKSHF(dk);
		}

		/*
		 * Make sure PD and VTOC are safe.
		 *
		 * p_start and unixst are read only once the slice is open,
		 * so there is no need to lock the dk here.
		 *
		 * No need to do the vtoc check is a special fdisk partition,
		 * since the active UNIX partition can't be accessed that way
		 */
		if (part < V_NUMPAR &&
			dk->dk_vtoc.v_part[part].p_start <=
						dk->unixst + VTBLKNO  &&
			start <= dk->unixst + VTBLKNO && 
		        (ret_val = sd01vtoc_ck(dk, bp, part)))
			
		{
			bp->b_flags |= B_ERROR;
			bp->b_error = ret_val;
			biodone(bp);
			return;
		}

		/*
		 * should read-only be supported for special fdisk partitions?
		 */
		if (part < V_NUMPAR && (!sd01_lightweight || dk->dk_vtoc.v_part[part].p_flag & V_RONLY)) {

			/* Verify that partition is really read-only */
			SD01_DK_LOCK(dk);
			if (dk->dk_vtoc.v_part[part].p_flag & V_RONLY) {
				SD01_DK_UNLOCK(dk);
				bp->b_flags |= B_ERROR;
				bp->b_error = EACCES;
				biodone(bp);
				return;
			}
			SD01_DK_UNLOCK(dk);
		}
			
	}


	if (bp->b_bcount == 0)
	{	/* The request is done */
		biodone(bp);
		return;
	}

	sd01ck_badsec(bp);

#ifdef DEBUG
        DPR (2)("sd01strat0: - exit\n");
#endif
}

/*
 * int
 * sd01close(dev_t dev, int flags, int otype, struct cred *cred_p)
 *	Clear the open flags.
 * Called by: Kernel
 * Side Effects: 
 *	The device is marked as unopened.
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
/*ARGSUSED*/
int
sd01close(dev_t dev, int flags, int otype, struct cred *cred_p)
{
 	register struct disk *dk;
	register int	      part;
	int	err;
	minor_t minor;
 
	minor = geteminor(dev);

	SD01_DP_LOCK();

 	dk = DKPTR(minor);
	ASSERT(dk);
	part = DKSLICE(minor);

#ifdef DEBUG
        if (flags & O_DEBUG) { /* For DEBUGFLG ioctl */
		SD01_DP_UNLOCK();
                return(0);
        }
        DPR (1)("sd01close: (dev=0x%x flags=0x%x otype=0x%x cred_p=0x%x)\n", dev, flags, otype, cred_p);
#endif

	/* must acquire both dk_lock and dp_mutex to change part_flag */
	SD01_DK_LOCK(dk);

	/* Determine the type of close being requested */
	switch(otype)
	{
	  case	OTYP_BLK:			/* Close for Block I/O */
		if (part < V_NUMPAR)
			dk->dk_part_flag[part] &= ~DKBLK;
		else
			dk->dk_fdisk_flag[part - V_NUMPAR] &= ~DKBLK;
		break;

	  case	OTYP_MNT:			/* Close for Mounting */
		if (part < V_NUMPAR)
			dk->dk_part_flag[part] &= ~DKMNT;
		else
			dk->dk_fdisk_flag[part - V_NUMPAR] &= ~DKMNT;
		break;

	  case	OTYP_CHR:			/* Close for Character I/O */
		if (part < V_NUMPAR)
			dk->dk_part_flag[part] &= ~DKCHR;
		else
			dk->dk_fdisk_flag[part - V_NUMPAR] &= ~DKCHR;
		break;

	  case	OTYP_SWP:			/* Close for Swapping Device */
		if (part < V_NUMPAR)
			dk->dk_part_flag[part] &= ~DKSWP;
		else
			dk->dk_fdisk_flag[part - V_NUMPAR] &= ~DKSWP;
		break;

	  case	OTYP_LYR:			/* Layered driver close */
		if (part < V_NUMPAR)
			dk->dk_part_flag[part] -= DKLYR;
		else
			dk->dk_fdisk_flag[part - V_NUMPAR] -= DKLYR;
		break;
	}

#ifdef MULTHBA
	/* not yet supported -- placeholder for future work */

	/* if shared devices, clear reservation */
#endif /* MULTHBA */

	/*
	*  Always clear the DKONLY flag since the partition can no
	*  longer be opened for exclusive open if this function is called.
	*  Check if all flags have been cleared and the layers counter is
	*  zero before clearing the General Open flag.
	*/
	if (part < V_NUMPAR)	{
		dk->dk_part_flag[part] &= ~DKONLY;
		if (!(dk->dk_part_flag[part] & ~DKGEN))
			dk->dk_part_flag[part] &= ~DKGEN;
	}
	else	{
		dk->dk_fdisk_flag[part - V_NUMPAR] &= ~DKONLY;
		if (!(dk->dk_fdisk_flag[part - V_NUMPAR] & ~DKGEN))
			dk->dk_fdisk_flag[part - V_NUMPAR] &= ~DKGEN;
	}

        /*
         *  If any partitions on this device are open, return.
	 */

check_last:
        for (part=0; part < V_NUMPAR; part++) {
		if (dk->dk_part_flag[part])	{
			SD01_DK_UNLOCK(dk);
			SD01_DP_UNLOCK();
                        return(0);
		}
	}

        for (part=0; part < FD_NUMPART + 1; part++) {
		if (dk->dk_fdisk_flag[part])	{
			SD01_DK_UNLOCK(dk);
			SD01_DP_UNLOCK();
                        return(0);
		}
	}

	/* this is last close */

	/*
	*  If the VTOC has been not been soft-modified, invalidate
	*  v_sanity and dk_state.
	*/
	if ( dk->vtoc_state != VTOC_SOFT ) {
        	dk->dk_state &= ~(DKPARMS | DKFDISK);
		dk->dk_vtoc.v_sanity = 0;
	}

	if ( dk->dk_state & DK_FO_LC ) {
		while ( dk->dk_state & DK_FO_LC ) {
			SV_WAIT(dk->dk_sv, pridisk, dk->dk_lock);
			SD01_DK_LOCK(dk);
		}
		goto check_last;
	}
	dk->dk_state |= DK_FO_LC;

	/*
	 * This is here for Compaq ProLiant Storage System support.  It could
	 * be ifdef'ed for performance
	 */
	if (dk->dk_state & DK_RDWR_ERR)	{
		err = TRUE;
		dk->dk_state &= ~DK_RDWR_ERR;
	}
	else
		err = FALSE;

	SD01_DK_UNLOCK(dk);
	(void)sd01_last_close(dk, err);
	dk->dk_state &= ~DK_FO_LC;
	SV_BROADCAST(dk->dk_sv, 0);	/* -> sd01open, sd01close */


#ifdef DEBUG
        DPR (2)("sd01close: - exit(0)\n");
#endif

	SD01_DP_UNLOCK();
	return(0);
}

/*
 * int
 * sd01read(dev_t dev, struct uio *uio_p, struct cred *cred_p)
 *	This is the raw read routine for the SCSI disk driver.
 *	The request is validated to see that it is within the slice.  A read
 *	request will start on a disk block boundary reading the requested
 *	number of bytes.  This routine calls physiock which locks the
 *	user buffer into core and checks that the user can access the area.
 * Called by: Kernel
 * Side Effects: 
 *	Error ENXIO is returned via physiock if the read is not to a legal
 *	partition.  Indirectly, the user buffer area is locked into core,
 *	and a SCSI read job is queued for the device.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/*ARGSUSED*/
int
sd01read(dev_t dev, struct uio *uio_p, struct cred *cred_p)
{
	register struct  disk *dk;
	register int part;
	int	 ret_val;
	daddr_t	 ds;
	minor_t minor;

	minor = geteminor(dev);

	dk   = DKPTR(minor);
	ASSERT(dk);
	part = DKSLICE(minor);
	if (part < V_NUMPAR)
		ds = dk->dk_vtoc.v_part[part].p_size << BLKSEC(dk);
	else
		ds = dk->dk_fdisk[part - V_NUMPAR].p_size << BLKSEC(dk);

#ifdef DEBUG
        DPR (1)("sd01read: (dev=0x%x, uio_p=0x%x, cred_p=0x%x) dk=0x%x part=0x%x\n", dev, uio_p, cred_p, dk, part);
#endif

	ret_val = physiock(sd01strategy, 0, dev, B_READ, ds, uio_p);

#ifdef DEBUG
        DPR (2)("sd01read: - exit(%d)\n", ret_val);
#endif
	return(ret_val);
}

/*
 * int
 * sd01write(dev_t dev, struct uio *uio_p, struct cred *cred_p)
 *	This function performs a raw write to a SCSI disk.  The request is
 *	validated to see that it is within the slice. The write will
 *	always start at the beginning of a disk block boundary.
 *	This function calls physiock which locks the user buffer into
 *	core and checks that the user can access the area.
 * Called by: Kernel
 * Side Effects: 
 *	Error ENXIO is returned via physiock if the write is not to a legal
 *	partition.  Indirectly, the user buffer area is locked into core,
 *	and a SCSI read job is queued for the device.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/*ARGSUSED*/
int
sd01write(dev_t dev, struct uio *uio_p, struct cred *cred_p)
{
	register struct  disk *dk;
	register int part;
	int	 ret_val;
	daddr_t	 ds;
	minor_t minor;

	minor = geteminor(dev);

	dk   = DKPTR(minor);
	ASSERT(dk);
	part = DKSLICE(minor);                           


#ifdef DEBUG
        DPR (1)("sd01write: (dev=0x%x, uio_p=0x%x, cred_p=0x%x) dk=0x%x part=0x%x\n", dev, uio_p, cred_p, dk, part);
#endif
	if (part < V_NUMPAR)
		ds = dk->dk_vtoc.v_part[part].p_size << BLKSEC(dk);
	else
		ds = dk->dk_fdisk[part - V_NUMPAR].p_size << BLKSEC(dk);
	ret_val = physiock(sd01strategy, 0, dev, B_WRITE, ds, uio_p);
#ifdef DEBUG
        DPR (2)("sd01write: - exit(%d)\n", ret_val);
#endif
	return(ret_val);
}

/*
 * int
 * sd01loadvtoc(struct disk *dk, int slice, int mode)
 *	This is the core open routine for the disk and mirror driver.  If
 *	the VTOC has not been read in yet, sd01loadvtoc will attempt to
 *	read the pdsector and the VTOC.  
 *	The mode field indicates the type of open exclusive use or
 *	general use.  Exclusive use is requested by the mirror driver when
 *	the partition is bound to a mirror partition.  Once the partition is
 *	opened for exclusive use all other open requests for the partition
 *	are failed.  The partition must not be open for general use or the
 *	exclusive use request is failed.  The partition may be opened many
 *	times for general use. If the open is for exclusive use, the 
 *	PD sector and VTOC must be valid and the sector must not contain
 *	the VTOC.
 * Called by: sd01open, sd01strategy.
 * Side Effects: 
 *	The type of open is noted.
 * Return Values:
 * -1	The open succeeded but the VTOC is insane or the partition is
 *	undefined.
 * 0	The open succeeded.
 * >0	The error value. 
 * Errors:
 *	The disk partition table is not sane.  This is caused by a corrupted
 *	or uninitialized sanity word in the fdisk table (OBSOLETE ERROR).
 *
 *	The SCSI disk is not configured properly.  This is caused by not
 *	defining an ACTIVE operating system in the partition table (fdisk).
 *
 *	The SCSI disk is not configured for the UNIX operating System.  This
 *	is caused by specifying an ACTIVE partition in the partition table
 *	that may not be intended to be used with the UNIX Operating System.
 *
 *	The SCSI disk does not contain a VTOC.  This is caused by specifying a
 *	different VTOC location in the /etc/partitions file.  The driver expects
 *	the VTOC to be in the PD sector at location 29.
 *
 *	The UNIX system partition entry in the VTOC does not match the partition
 *	defined in the fdisk table.  This is caused by changing the starting
 *	sector address or size of slice 0 in the VTOC with out performing an
 *	fdisk(1M).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires disk struct's dk_lock.
 */
int
sd01loadvtoc(major_t emaj, minor_t emin, struct disk *dk, int slice)
{
	struct   phyio	 phyarg;	/* For reading Pd and VTOC 	*/
	struct	 ipart 	*ipart;		/* The fdisk table entry 	*/
	int	 ret_val;
	struct 	pdinfo 	*pdptr;
	struct 	vtoc 	*vtocptr;
	uint 	voffset;
	daddr_t	vtocsect;
	caddr_t	p1, p2, secbuf;
	char 	*p;
	int	i, z;
	int first_open=1;
	char name[SDI_NAMESZ];
	struct scsi_ad addr;
	uint	pd_size, dk_size, size_delta, largest_allowable;
	
	phyarg.retval = 0;

#ifdef DEBUG
        DPR (1)("sd01loadvtoc: (emaj=0x%x emin=0x%x slice=0x%x dk=%d)\n",
					emaj, emin, slice, dk);
#endif

	SD01_DK_LOCK(dk);

recheck:
#ifdef DEBUG
        DPR (3)("sa_ct=0x%x\n", dk->dk_addr.sa_ct);
#endif

	/* Is the VTOC sane? If not try to read it in. */
	if (dk->dk_vtoc.v_sanity != VTOC_SANE ||
	   (slice >= V_NUMPAR) ||
	   (dk->dk_part_flag[slice] == 0 && DK_SLICE_MODIFIED(dk, slice))) {

		/* first open of dk, or first open of this slice */
#ifdef DEBUG
		if (dk->dk_vtoc.v_sanity != VTOC_SANE || slice < V_NUMPAR)
			DPR (3)("VTOC insane\n");
#endif
		if(dk->dk_state & DKVTOC) {
			SV_WAIT(dk->dk_sv, pridisk, dk->dk_lock);
			SD01_DK_LOCK(dk);
			goto recheck;
		}
		dk->dk_state |= DKVTOC;
	}

	/* handle special fdisk partitions separately */
	if (slice >= V_NUMPAR)	{

		/*
		 * populate dk_fdisk table only on the first special
		 * partition open
		 */
		for (i = 0; i < FD_NUMPART + 1; i++)
			if(dk->dk_fdisk_flag[i])
				break;

		/* dk_fdisk already populated? */
		if (i < FD_NUMPART + 1)	{
			slice -= V_NUMPAR;

			/*
			 * validate the partition, disable access to the
			 * active UNIX partition, note the use of p_tag
			 * for ACTIVE/NOTACTIVE flags
			 */
			if (dk->dk_fdisk[slice].p_type == UNUSED ||
				(dk->dk_fdisk[slice].p_type == UNIXOS &&
					dk->dk_fdisk[slice].p_tag == ACTIVE))
				ret_val = ENODEV;
			else
				ret_val = 0;
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			return ret_val;
		}
	}


	if ((slice >= V_NUMPAR)	 || (dk->dk_vtoc.v_sanity != VTOC_SANE)) {

		SD01_DK_UNLOCK(dk);

		if ((dk->dk_state & DKINIT) == 0)
		{
			/* Initialize disk structure (done only once per dk */
			dk->dk_fltsus  =		/* Suspend       */
				sdi_getblk(KM_SLEEP);
			dk->dk_fltreq  =		/* Request Sense */
				sdi_getblk(KM_SLEEP);
			dk->dk_fltres  =		/* Reserve       */
				sdi_getblk(KM_SLEEP);
			dk->dk_bbhblk =			/* Read/Write Block*/
				sdi_getblk(KM_SLEEP);
			dk->dk_bbhmblk =		/* Reassign Block*/
				sdi_getblk(KM_SLEEP);
			dk->dk_fltfqblk =		/* Flush Queue   */
				sdi_getblk(KM_SLEEP);

			dk->dk_bbhjob = sd01getjob(dk, KM_SLEEP);
			dk->dk_bbhbuf = getrbuf(KM_SLEEP);

			dk->dk_fltsus->sb_type  = SFB_TYPE;
			dk->dk_fltreq->sb_type  = ISCB_TYPE;
			dk->dk_fltres->sb_type  = ISCB_TYPE;
			dk->dk_bbhmblk->sb_type = ISCB_TYPE;
			dk->dk_fltfqblk->sb_type = SFB_TYPE;

#ifdef PDI_SVR42
			dk->dk_stat.maj = emaj;
			dk->dk_stat.min = emin;
#endif
			dk->dk_addr.sa_major = emaj;
			dk->dk_addr.sa_minor = emin;

			dk->dk_fltsus->SFB.sf_dev    = dk->dk_addr;

			dk->dk_fltreq->SCB.sc_datapt = SENSE_AD(&dk->dk_sense);
			dk->dk_fltreq->SCB.sc_datasz = SENSE_SZ;
			dk->dk_fltreq->SCB.sc_mode   = SCB_READ;
			dk->dk_fltreq->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
			dk->dk_fltreq->SCB.sc_dev    = dk->dk_addr;
			sdi_translate(dk->dk_fltreq, B_READ, NULL, KM_SLEEP);

			dk->dk_fltres->SCB.sc_datapt = NULL;
			dk->dk_fltres->SCB.sc_datasz = 0;
			dk->dk_fltres->SCB.sc_mode   = SCB_WRITE;
			dk->dk_fltres->SCB.sc_cmdpt  = SCS_AD(&dk->dk_fltcmd);
			dk->dk_fltres->SCB.sc_dev    = dk->dk_addr;
			sdi_translate(dk->dk_fltres, B_WRITE,NULL, KM_SLEEP);

			dk->dk_bbhmblk->SCB.sc_datapt = dk->dk_dl_data;
			dk->dk_bbhmblk->SCB.sc_datasz = RABLKSSZ;
			dk->dk_bbhmblk->SCB.sc_mode   = SCB_WRITE;
			dk->dk_bbhmblk->SCB.sc_dev    = dk->dk_addr;
			sdi_translate(dk->dk_bbhmblk, B_WRITE, NULL, KM_SLEEP);

			dk->dk_state |= DKINIT;
		}

		
		SD01_DK_LOCK(dk);
	check_first:
		for(i=0; i < V_NUMPAR; i++) {
			if(dk->dk_part_flag[i])	{
				first_open = 0;
				break;
			}
		}
		
		if (slice < V_NUMPAR)	{
			for (i = 0; i < FD_NUMPART + 1; i++) {
				if(dk->dk_fdisk_flag[i]) {
					first_open = 0;
					break;
				}
			}
		}
		
		if (first_open) {
			/*
			 * If the media is removable, prevent
			 * media removal after first open of
			 * the drive.
			 */
			if ( dk->dk_state & DK_FO_LC ) {
				while ( dk->dk_state & DK_FO_LC ) {
					SV_WAIT(dk->dk_sv, pridisk,
						dk->dk_lock);
					SD01_DK_LOCK(dk);
				}
				goto check_first;
			}
			dk->dk_state |= DK_FO_LC;
			SD01_DK_UNLOCK(dk);
			(void)sd01_first_open(dk);
		}
		else	{
			SD01_DK_UNLOCK(dk);
		}

		if (slice < V_NUMPAR)	{
			dk->dk_state &= ~(DKCONFLICT | DKFDISK);
			dk->unixst   = 0;
		}

		/* Check if disk parameters need to be set */
		if ((dk->dk_state & DKPARMS) == 0) {
			ret_val = sd01config(dk, emin);
			if (ret_val > 0) {
				SD01_DK_LOCK(dk);
				dk->dk_state &= ~DKVTOC;
				SD01_DK_UNLOCK(dk);
				SV_BROADCAST(dk->dk_sv, 0);
				return(ret_val);
			}
			/*
			 * Perhaps sd01loadvtoc() should not continue
			 * if sd01config() gets a non-fatal error.
			 */
		}

		/* Allocate temporary memory for sector 0 */
		if((secbuf = kmem_alloc(BLKSZ(dk), KM_SLEEP)) == NULL)
		{ 
			SD01_DK_LOCK(dk);
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			return(-1);
		}

		/* Read in the FDISK sector */
		phyarg.sectst  = FDBLKNO; /* sector zero */
		phyarg.memaddr = (long) secbuf;
  		phyarg.datasz  = BLKSZ(dk);
		sd01phyrw(dk, V_RDABS, &phyarg, SD01_KERNEL);

		SD01_DK_LOCK(dk);
	}

	if (slice >= V_NUMPAR) {

		if (phyarg.retval != 0 ||
		   ((struct mboot *)(void *)secbuf)->signature != MBB_MAGIC)
		{
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			cmn_err(CE_NOTE, "!Disk Driver: Invalid mboot signature.");
			return(ENODEV);
		}

		/*
		 * get information from fdisk table into special partition
		 * table, sorted by relsect
		 */
		ipart = (struct ipart *)(void *)
			((struct mboot *)(void *)secbuf)->parts;
		i = sd01fdisk(ipart, dk);

		/* no valid entries in FDISK table? */
		if (i <= 1)
			cmn_err(CE_NOTE, "!Disk Driver: Invalid FDISK table.");

		/*
		 * validate the partition, disable access to the
		 * active UNIX partition, note the use of p_tag
		 * for ACTIVE/NOTACTIVE flags
		 */
		slice -= V_NUMPAR;
		if (slice >= i)
			ret_val = ENODEV;
		else if (dk->dk_fdisk[slice].p_type == UNUSED ||
			(dk->dk_fdisk[slice].p_type == UNIXOS &&
				dk->dk_fdisk[slice].p_tag == ACTIVE))
			ret_val = ENODEV;
		else
			ret_val = 0;

		dk->dk_state &= ~DKVTOC;
		SD01_DK_UNLOCK(dk);
		SV_BROADCAST(dk->dk_sv, 0);
		kmem_free(secbuf, BLKSZ(dk));
		return ret_val;
	}

	if (dk->dk_vtoc.v_sanity != VTOC_SANE) {

		if (phyarg.retval != 0 ||
		   ((struct mboot *)(void *)secbuf)->signature != MBB_MAGIC)
		{
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			if (slice > 0)
				return(ENODEV);
			return(-1);
		}

		ipart = (struct ipart *)(void *)
			((struct mboot *)(void *)secbuf)->parts;
               	/*
               	 * Find active Operating System partition.
               	 */
               	for (z = FD_NUMPART; ipart->bootid != ACTIVE; ipart++) {
			if (--z == 0) {
				dk->dk_state &= ~DKVTOC;
				SD01_DK_UNLOCK(dk);
				SV_BROADCAST(dk->dk_sv, 0);
				kmem_free(secbuf, BLKSZ(dk));
				if (slice > 0)
				{
					/*
					 *+ The disk driver could not find an
					 *+ active partition.  Run fdisk to
					 *+ select which partition should
					 *+ be active.
					 */
					cmn_err(CE_NOTE, "!Disk Driver: No ACTIVE partition.");
					return(ENODEV);
				}
				return(-1);
			}
                }

		/* Indicate Fdisk is valid */
		dk->dk_state |=  DKFDISK;

		/* Check if UNIX is the active partition */
                if (ipart->systid != UNIXOS) {
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			if (slice > 0) {
				/*
				 *+ The active partition is not a UNIX
				 *+ partition.  Run fdisk to select
				 *+ a UNIX partition as the active one.
				 */
                       		cmn_err(CE_NOTE, "!Disk Driver: ACTIVE partition is not a UNIX System partition.");
				return(ENODEV);
			}
		        return(-1);
                }

		/* Save starting sector of UNIX partition */
                dk->unixst = ipart->relsect;
			
		/* Make slice 0 the whole UNIX partition */

		dk->dk_vtoc.v_part[0].p_start = dk->unixst;
		dk->dk_vtoc.v_part[0].p_size  = ipart->numsect;
                dk->dk_parms.dp_pstartsec     = dk->unixst; 

		/* Read in the PD and VTOC sector */
		phyarg.sectst  = dk->unixst + HDPDLOC; 
		phyarg.memaddr = (long) secbuf;
  		phyarg.datasz  = BLKSZ(dk);

		SD01_DK_UNLOCK(dk);
		sd01phyrw(dk, V_RDABS, &phyarg, SD01_KERNEL);

		SD01_DK_LOCK(dk);
		pdptr = (struct pdinfo *)(void *) secbuf;
		if (pdptr->sanity != VALID_PD ||
			phyarg.retval != 0)
		{
			/* Mark the VTOC as insane */
			dk->dk_vtoc.v_sanity = 0;
			dk->dk_state &= ~DKVTOC;
#ifdef DEBUG
        DPR (2)("sd01loadvtoc: PD not valid - return(-1)\n");
#endif
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			if (slice > 0)
				return(ENODEV);
			return(-1);
		}

		/*
		 * calculate the size in sectors of the disk, based both
		 * on the information in the DK structure and the
		 * information contained in the PD sector.
		 *
		 * If the size of the disk has changed by more than 1%,
		 * disallow opens of any slice but slice 0.
		 *
		 * This is an attempt to differentiate between disk arrays
		 * that have actually changed size and disks hooked to a
		 * controller that defines a different translation than
		 * the translation in effect when the disk was initialized.
		 *
		 * If a disk has actually changed size, any information
		 * contained on it is invalid.
		 */

		pd_size = pdptr->cyls*pdptr->tracks*pdptr->sectors;
		dk_size = dk->dk_parms.dp_cyls*dk->dk_parms.dp_heads*dk->dk_parms.dp_sectors;
		if (pd_size > dk_size) {
			size_delta = pd_size - dk_size;
			largest_allowable = pd_size / 100;
		} else {
			size_delta = dk_size - pd_size;
			largest_allowable = dk_size / 100;
		}

		if (size_delta > largest_allowable) {

			addr = dk->dk_addr;
			sdi_name(&addr, name);
			/*
			 *+ The disk parameters recorded on the hard disk
			 *+ no longer match those returned by the disk.
			 *+ Either correct the disk parameters in your BIOS,
			 *+ restore the original disk translation or low-level
			 *+ format the disk.
			 */
			cmn_err(CE_WARN, "Disk Driver (%s), Unit %d, :  %s",
				name, addr.sa_lun,
				"disk size parameters have changed");
			cmn_err(CE_CONT, "Filesystems on this disk can not be accessed, see edvtoc(1m) for details.\n");
			cmn_err(CE_CONT, "!old parameters:  cylinders %d, heads %d, sectors/track %d\n",
				pdptr->cyls, pdptr->tracks, pdptr->sectors);
			cmn_err(CE_CONT, "!new parameters:  cylinders %d, heads %d, sectors/track %d\n",
				dk->dk_parms.dp_cyls, dk->dk_parms.dp_heads,
				dk->dk_parms.dp_sectors);

			/* make in-core pdinfo match DK parms */

			pdptr->cyls    = dk->dk_parms.dp_cyls;
			pdptr->tracks  = dk->dk_parms.dp_heads;
			pdptr->sectors = dk->dk_parms.dp_sectors;

			/* Make slice 0 reflect the new size */

			dk->dk_vtoc.v_part[0].p_size =
				dk_size - dk->dk_vtoc.v_part[0].p_start;
			
			/* Mark the VTOC as insane */

			dk->dk_vtoc.v_sanity = 0;
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));

			/* fail all but slice 0 opens */

			if (slice > 0)
				return(ENODEV);

			return(-1);

		}

		/* Compare pdinfo parameters with drivers */
               	if ((pdptr->cyls != dk->dk_parms.dp_cyls) ||
               		(pdptr->tracks != dk->dk_parms.dp_heads) ||
               		(pdptr->sectors != dk->dk_parms.dp_sectors) ||
               		(pdptr->bytes != BLKSZ(dk))) {

#ifdef DEBUG
        DPR (3)("SD01: Pdinfo doesn't match disk parameters\n");
        DPR (3)("cyls=0x%x / 0x%x tracks=0x%x / 0x%x sec=0x%x / 0x%x bytes=0x%x / 0x%x\n", 
	pdptr->cyls, dk->dk_parms.dp_cyls, pdptr->tracks, dk->dk_parms.dp_heads,
	pdptr->sectors, dk->dk_parms.dp_sectors, pdptr->bytes, BLKSZ(dk));
#endif

			dk->dk_parms.dp_heads   = pdptr->tracks;
			dk->dk_parms.dp_cyls    = pdptr->cyls;
			dk->dk_parms.dp_sectors = pdptr->sectors;
			dk->dk_state |= DKPARMS; 
		}

		/* Update the in-core PD info structure */
		p = (caddr_t) &dk->dk_pdsec;
		for (z = 0; z < sizeof(dk->dk_pdsec); z++,p++)
			*p = secbuf[z];

               	/* Determine VTOC location on the disk */
               	vtocsect = dk->unixst + (dk->dk_pdsec.vtoc_ptr >> BLKSHF(dk));

		/* Check if VTOC and PD info are in the same sector */
               	voffset = dk->dk_pdsec.vtoc_ptr & (BLKSZ(dk)-1);
               	if (vtocsect != (dk->unixst + HDPDLOC)) {
                       	/* VTOC is in a different sector. */
			/* Mark the VTOC as insane */
			dk->dk_vtoc.v_sanity = 0;
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			return(-1);
               	}

               vtocptr = (struct vtoc *)(void *) (secbuf + voffset);
	
               	if (vtocptr->v_sanity != VTOC_SANE) {
#ifdef DEBUG
        		DPR (3)("sd01loadvtoc: VTOC not valid - return(-1)\n");
#endif
			/* Mark the VTOC as insane */
			dk->dk_vtoc.v_sanity = 0;
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			if (slice > 0)
				return(ENODEV);
			return(-1);
               	}

               	/* Make sure slice 0 is correct.  */
               	if (vtocptr->v_part[0].p_start != dk->dk_vtoc.v_part[0].p_start
                              	|| vtocptr->v_part[0].p_size != dk->dk_vtoc.v_part[0].p_size) {
#ifdef SD01_DEBUG
			  if (slice > 0)
                       		cmn_err(CE_CONT, "!Disk Driver: Invalid slice 0");
#endif
			/* Mark the VTOC as insane */
			dk->dk_vtoc.v_sanity = 0;
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			return(-1);
               	}

		/* Update the in-core VTOC structure */
		p = (caddr_t) &dk->dk_vtoc;
		for (z=voffset, i=0; i < sizeof(dk->dk_vtoc); i++,p++) {
			*p = secbuf[z++];
		}
		dk->vtoc_state = VTOC_HARD;

		/* Get alternate sector table		*/
		if ( sd01getalts(dk) ) {
			dk->dk_vtoc.v_sanity = 0;
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			if (slice > 0)
				return(EIO);
			return(-1);
		}

		dk->dk_state &= ~DKVTOC;
		kmem_free(secbuf, BLKSZ(dk));
	}
	else if (dk->dk_part_flag[slice] == 0 && DK_SLICE_MODIFIED(dk, slice)) {

		SD01_DK_UNLOCK(dk);

		/* Allocate temporary memory for sector 0 */
		if((secbuf = kmem_alloc(BLKSZ(dk), KM_SLEEP)) == NULL)
		{ 
			SD01_DK_LOCK(dk);
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			return(-1);
		}

		/* Read in the PD and VTOC sector */
		phyarg.sectst  = dk->unixst + HDPDLOC; 
		phyarg.memaddr = (long) secbuf;
  		phyarg.datasz  = BLKSZ(dk);

		sd01phyrw(dk, V_RDABS, &phyarg, SD01_KERNEL);

		SD01_DK_LOCK(dk);

		if (phyarg.retval != 0) {
			dk->dk_state &= ~DKVTOC;
			SD01_DK_UNLOCK(dk);
			SV_BROADCAST(dk->dk_sv, 0);
			kmem_free(secbuf, BLKSZ(dk));
			return (-1);
		}

               	voffset = dk->dk_pdsec.vtoc_ptr & (BLKSZ(dk)-1);
		vtocptr = (struct vtoc *)(void *) (secbuf + voffset);

		/*
		 * Update the in-core partition structure for the
		 * modified slice.
		 */
		p1 = (caddr_t) &dk->dk_vtoc.v_part[slice];
		p2 = (caddr_t) &vtocptr->v_part[slice];
		i = sizeof(struct partition);

		while (i--)	{
			*p1++ = *p2++;
		}

		DK_SLICEMOD_CLEAR(dk, slice);
		dk->dk_state &= ~DKVTOC;
		kmem_free(secbuf, BLKSZ(dk));
	}

	/* If slice has size 0, it's not a valid slice and open should fail */
	if ((slice > 0) &&
	( !(dk->dk_vtoc.v_part[slice].p_flag & V_VALID) ||
	(dk->dk_vtoc.v_part[slice].p_size <= 0) ) ) {
		SD01_DK_UNLOCK(dk);
		SV_BROADCAST(dk->dk_sv, 0);
		return(ENODEV);
	}

	/* Check if VTOC is valid */
	if (dk->dk_vtoc.v_sanity != VTOC_SANE || 
	    dk->dk_vtoc.v_version > V_VERSION ||
	    dk->dk_vtoc.v_part[slice].p_tag == V_OTHER) {
#ifdef DEBUG
        	DPR (3)("sanity=0x%x version=0x%x size=0x%x\n",
			dk->dk_vtoc.v_sanity, dk->dk_vtoc.v_version,
			dk->dk_vtoc.v_part[slice].p_size);
        	DPR (2)("sd01loadvtoc: VTOC still insane - return(-1)\n");
#endif
		SD01_DK_UNLOCK(dk);
		SV_BROADCAST(dk->dk_sv, 0);
		return(-1);
	}

#ifdef DEBUG
        DPR (2)("sd01loadvtoc: - exit(0)\n");
#endif
	SD01_DK_UNLOCK(dk);
	SV_BROADCAST(dk->dk_sv, 0);
	return(0);
}

/*
 * int
 * sd01open(dev_t *dev_p, int flags, int otype, struct cred *cred_p)
 *	open routine for disks
 *	Check whether sd01rinit() is waiting for an idle disk;  if so
 *	returns EBUSY.  If this is the first open to the device,
 *	reads in the VTOC.
 *	If the time stamp is set for this slice, sd01strategy will write
 *	the time stamp back with zero when the first write occurs to this slice.
 *	sd01open will fail if the pdsector and VTOC cannot be read.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/*ARGSUSED*/
int
sd01open(dev_t *dev_p, int flags, int otype, struct cred *cred_p)
{
	register struct disk *dk;
	register int	      part;
	major_t	 emaj;
	minor_t	 emin;
	int	 ret_val;

#ifdef DEBUG
        DPR (1)("\nsd01open: (dev=0x%x flags=0x%x otype=0x%x cred_p=0x%x)\n",
		*dev_p, flags, otype, cred_p);
#endif

	emin = geteminor(*dev_p);
	emaj = getemajor(*dev_p);

	SD01_DP_LOCK();

	if ((sd01_clone) && (DKINDEX(emin) == 0))
	{
		/* DKINDEX 0 is a clone device of
		 * the boot disk
		 */
		minor_t index;

		index = sd01_disk_to_minor(0);

		if (index != 0)
		{
			emin = index + DKSLICE(emin);
			*dev_p = makedevice(emaj, emin);
		}
	}

	/* Verify this disk is present so other checks are valid */
	if ((dk = DKPTR(emin)) == NULL)
	{
		SD01_DP_UNLOCK();
		return(ENXIO);
	}


	part = DKSLICE(emin);

	/*
	 * Make sure user has permission to access the whole disk
	 * special partition
	 */
	if (part == DK_WHOLE_DISK_PART)	{
		if (ret_val = drv_priv(cred_p))	{
			SD01_DP_UNLOCK();
			return(ret_val);
		}
	}

#ifdef MULTHBA
	/* not yet supported -- placeholder for future work */
	if (dk->dk_state & DKCONFLICT)
	{
		/* Return EBUSY for reservation conflict */
		SD01_DP_UNLOCK();
		dk->dk_state &= ~DKCONFLICT;
		bp->b_error = EBUSY;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return EBUSY;
	}
#endif /* MULTHBA */

	/* Don't set open flag unless open succeeds */
	if ((ret_val = sd01loadvtoc(emaj, emin, dk,  part)) > 0) {
		SD01_DK_LOCK(dk);
		goto outahere;
	}

	SD01_DK_LOCK(dk);

	/* Determine the type of open being requested */
	switch(otype)
	{
	  case	OTYP_BLK:		/* Open for Block I/O */
		if (part < V_NUMPAR)
			dk->dk_part_flag[part] |= (DKBLK|DKGEN);
		else
			dk->dk_fdisk_flag[part - V_NUMPAR] |= (DKBLK|DKGEN);
		break;

	  case	OTYP_MNT:		/* Open for Mounting */
		if(ret_val == -1)
		{ /* bad vtoc or PD sector */
			ret_val = ENXIO;
			goto outahere;
		}
		/* vtoc must be sane at this point */
		if (part < V_NUMPAR)	{
			if (dk->dk_vtoc.v_part[part].p_flag & V_UNMNT)	{
				ret_val = EACCES;
				goto outahere;
			}
			dk->dk_part_flag[part] |= (DKMNT|DKGEN);
		}
		else	/* are all fdisk special partitions mountable? */
			dk->dk_fdisk_flag[part - V_NUMPAR] |= (DKMNT|DKGEN);
		break;

	  case	OTYP_CHR:		/* Open for Character I/O */
		if (part < V_NUMPAR)
			dk->dk_part_flag[part] |= (DKCHR|DKGEN);
		else
			dk->dk_fdisk_flag[part - V_NUMPAR] |= (DKCHR|DKGEN);
		break;

	  case	OTYP_SWP:		/* Open for Swapping Device */
		if (part < V_NUMPAR)
			dk->dk_part_flag[part] |= (DKSWP|DKGEN);
		else
			dk->dk_fdisk_flag[part - V_NUMPAR] |= (DKSWP|DKGEN);
		break;

	  case	OTYP_LYR:		/* Layered driver counter */
		if (part < V_NUMPAR)	{
			dk->dk_part_flag[part] += DKLYR;
			dk->dk_part_flag[part] |= DKGEN;
		}
		else	{	/* apply to fdisk special partitions? */
			dk->dk_fdisk_flag[part - V_NUMPAR] += DKLYR;
			dk->dk_fdisk_flag[part - V_NUMPAR] |= DKGEN;
		}
		break;
	}


#ifdef DEBUG
        DPR (2)("\nsd01open: - exit\n");
#endif
	ret_val = 0;

 outahere:
	SD01_DP_UNLOCK();
	if (dk->dk_state & DK_FO_LC) {
		dk->dk_state &= ~DK_FO_LC;
		SD01_DK_UNLOCK(dk);
		SV_BROADCAST(dk->dk_sv, 0);	/* -> sd01open, sd01close */
	} else {
		SD01_DK_UNLOCK(dk);
	}
	return ret_val;
}

/*
 * void
 * sd01done(struct job *jp)
 *	This function completes the I/O request after a job is returned by
 *	the host adapter. It will return the job structure and call
 *	biodone.
 * Called by: sd01comp1
 * Side Effects: 
 *	The kernel is notified that one of its requests completed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquired the disk struct's dk_lock.
 */
void
sd01done(struct job *jp)
{
	register buf_t *bp = jp->j_bp;
	register struct disk *dk = jp->j_dk;
	char	ops;
#ifdef PDI_SVR42
	daddr_t jdaddr;
	uint_t  jseccnt;
	time_t	lbolt_time; 
	time_t	since_lasttime;
#endif
	int part;
	int flags;
	int setresid = 0;

#ifdef DEBUG
        DPR (1)("sd01done: (jp=0x%x)\n", jp);
#endif

	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {
		/* Record the error for a normal job */
		bp->b_flags |= B_ERROR;
		if (jp->j_cont->SCB.sc_comp_code == SDI_NOTEQ)
			bp->b_error = ENODEV;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_QFLUSH)
			bp->b_error = EFAULT;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_OOS)
			bp->b_error = EIO;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_NOSELE)
			bp->b_error = ENODEV;
		else if (jp->j_cont->SCB.sc_comp_code == SDI_CKSTAT && 
			 jp->j_cont->SCB.sc_status == S_RESER) {
			bp->b_error = EBUSY;/* Reservation Conflict */

			SD01_DK_LOCK(dk);
			dk->dk_state |= DKCONFLICT;
			SD01_DK_UNLOCK(dk);
		} else
			bp->b_error = EIO;
	} else {
		setresid = 1;
	}
	

	if (bp->b_error==EFAULT) {
		/*  job returned from queue flush */
		if (!jp->j_fwmate && !jp->j_bkmate) {
			part = DKSLICE(geteminor(bp->b_edev));
			sd01gen_sjq(dk, part, bp, SD01_NORMAL|SD01_FIFO, NULL);
		} else {
			sd01releasejob(jp);
		}
		return;
	}

#ifdef PDI_SVR42
	jdaddr = jp->j_daddr;
	jseccnt = jp->j_seccnt;
#endif
	ops = (char) jp->j_cont->SCB.sc_cmdpt;
	flags = jp->j_flags;

	if (sd01releasejob(jp) && !(flags & J_FREEONLY)) {
		if(setresid)
			bp->b_resid -= bp->b_bcount;

		SD01_DK_LOCK(dk);
		/* Update performance stat */
		/* If this is a read or write, also update the I/O queue */
#ifndef PDI_SVR42
 		met_ds_iodone(dk->dk_stat, bp->b_flags,
				(bp->b_bcount - bp->b_resid));

		/*
		 * This is here for Compaq ProLiant Storage System support.  It could
		 * be ifdef'ed for performance
		 */
		if (ops == SM_READ || ops ==  SM_WRITE)	{
			if (bp->b_flags & B_ERROR)
				dk->dk_state |= DK_RDWR_ERR;
			else
				dk->dk_state &= ~DK_RDWR_ERR;
		}

#else /* PDI_SVR42 */

		drv_getparm(LBOLT, (ulong*)&lbolt_time);
		if (ops == SM_READ || ops ==  SM_WRITE) {

			if (ops == SM_READ)
				dk->dk_stat.tnrreq++;
			else
				dk->dk_stat.tnwreq++;
			if(dk->dk_stat.pttrack >= dk->dk_stat.endptrack)
                        	dk->dk_stat.pttrack = &dk->dk_stat.ptrackq[0];
                	dk->dk_stat.pttrack->b_blkno = jdaddr;
                	dk->dk_stat.pttrack->bp = bp;
                	dk->dk_stat.pttrack++;
			since_lasttime = lbolt_time - dk->dk_stat.lasttime;
			dk->dk_stat.io_act += since_lasttime;
			dk->dk_stat.io_resp += since_lasttime * dk->dk_stat.busycnt;
			dk->dk_stat.io_bcnt += jseccnt;
			dk->dk_stat.busycnt--;
			dk->dk_stat.lasttime = lbolt_time;
		}
#endif /* !PDI_SVR42 */
		SD01_DK_UNLOCK(dk);
		biodone(bp);
		if (flags & J_TIMEOUT)
			jp->j_flags |= J_FREEONLY;
	}

#ifdef DEBUG
        DPR (2)("sd01done: - exit\n");
#endif
}
/*
 * int
 * sd01releasejob(struct job *jp)
 *	unchain job from forward/backward mate, 
 *	and remove the job structure.
 *	return = 0, was part of a chain,
 *	return = 1, was not part of a chain.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sd01releasejob(struct job *jp)
{
	int notchained = 1;

	if (jp->j_fwmate) {
		notchained = 0;
		jp->j_fwmate->j_bkmate = jp->j_bkmate;
	}
	if (jp->j_bkmate) {
		notchained = 0;
		jp->j_bkmate->j_fwmate = jp->j_fwmate;
	}

	/* if this job is being timed out, do not free resources
	 * in case we get a later interrupt.  If we get the interrupt
	 * while we are processing the call, we may not free the
	 * the job; this is an acceptable race condition.
	 */
	if ((jp->j_flags & (J_TIMEOUT|J_FREEONLY)) != J_TIMEOUT)
		sd01freejob(jp);
	else
		jp->j_fwmate = jp->j_bkmate = 0;

	return notchained;
}

/*
 * void
 * sd01comp1(struct job *jp)
 *	This function removes the job from the queue.  Updates the disk
 *	counts.  Restarts the logical unit queue if necessary, and prints an
 *	error for failing jobs.
 * Called by: sd01intn 
 * Side Effects: 
 *	Removes the job from the disk queue.
 *
 *	An I/O request failed due to an error returned by the host adpater.
 *	All recovery action failed and the I/O request was returned to the 
 *	requestor.  The secondary error code is equal to the SDI return
 *	code.  See the SDI error codes for more information.
 *
 *	A bad block was detected.  The driver can not read this block. A 
 *	Error Correction Code (ECC) was unsuccessful.  The data in this block
 *	has been lost.  The data can only be retrived from a previous backup.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquired the disk struct's dk_lock.
 */
void
sd01comp1(struct job *jp)
{
	register struct disk *dk;

	dk = jp->j_dk;

#ifdef  SD01_DEBUG
	SD01_DK_LOCK(dk);
	dk->dk_outcnt--;
	SD01_DK_UNLOCK(dk);
#endif

	/* Log error if necessary */
	if (jp->j_cont->SCB.sc_comp_code != SDI_ASW) {
		if(jp->j_flags & J_HDEECC) {
			if(jp->j_flags & J_OREM) {
				jp->j_flags = 0;
				jp->j_cont->SCB.sc_comp_code = SDI_ASW;
			} else {
				if (!(jp->j_flags & J_NOERR))
				{
					sd01logerr(jp->j_cont, dk, 0x6dd0e002);
				}
			}
		} else
		{
			if (!(jp->j_flags & (J_NOERR|J_TIMEOUT)))
				sd01logerr(jp->j_cont, dk, 0x6dd0e001);
		}
	}

	if (jp->j_flags & J_BADBLK) {
		/* ECC occurred on previous read.
		 * If this read succeeded, remap and restore data just read.
		 * If this is a write, remap and retry the job.
		 */
		if (jp->j_bp->b_flags & B_READ) {
			if (jp->j_flags & J_HDEBADBLK) {
				jp->j_flags &= ~J_HDEBADBLK;
			} else {
				sd01_bbhndlr(dk, jp, SD01_ECCDRD,jp->j_memaddr, jp->j_daddr);
				return;
			}
		} else {
			sd01_bbhndlr(dk, jp, SD01_ECCDWR, NULL, jp->j_daddr);
			return;
		}
	}

	SD01_DK_LOCK(dk);
	if (jp->j_flags & J_HDEBADBLK) {
		minor_t  min;                   /* Device minor number  */
		daddr_t hdesec;                 /* Bad sector number    */

		/*
		 * bad sector detected - Q has been suspended
		 */

		min = dk->dk_fltreq->SCB.sc_dev.sa_minor;
		hdesec = dk->dk_sense.sd_ba;

		/* Check if bad sector is in non-UNIX area of the disk
		 * or outside this UNIX partition.
		 * Also check if it's in a special fdisk partition
		 */
		if (DKSLICE(min) >= V_NUMPAR ||
		   (dk->dk_vtoc.v_sanity == VTOC_SANE &&
		   (dk->dk_vtoc.v_part[DKSLICE(min)].p_tag == V_OTHER ||
		    hdesec < dk->dk_vtoc.v_part[0].p_start ||
		    hdesec > (dk->dk_vtoc.v_part[0].p_start +
			      dk->dk_vtoc.v_part[0].p_size)))) {

			/* Resume the Q if suspended */
			if (dk->dk_state & DKSUSP)
				sd01qresume(dk);

			/* Clear bad block state */
			jp->j_flags &= ~J_HDEBADBLK;
		} else {
			/* let the bad block handler take this */
			SD01_DK_UNLOCK(dk);
			jp->j_bp->b_flags |= (B_ERROR|B_BAD);
			sd01_bbhndlr(dk, jp, SD01_ECCRDWR, NULL, hdesec);
			return;
		}
	} else if (!(dk->dk_state & DKMAPBAD) && (dk->dk_state & DKSUSP))
		sd01qresume(dk);        /* Resume Q     */
 
	/* start next job if necessary */
	if (!(dk->dk_state & (DKSUSP|DKSEND)) && dk->dk_queue){
		/* use timeout to restart the queue */
		dk->dk_state |= DKSEND;
		SD01_DK_UNLOCK(dk);
		if ( !(dk->dk_sendid = sdi_timeout(sd01sendt, dk, 1, pldisk,
			&dk->dk_addr)) ) {

			dk->dk_state &= ~DKSEND;
			/*
			 *+ sd01 completion routine  could not
			 *+ schedule a job for retry via itimeout
			 */
			cmn_err(CE_WARN, "sd01comp1: itimeout failed");
		}
	} else {
		SD01_DK_UNLOCK(dk);
	}
	jp->j_done(jp);			/* Call the done routine 	*/
}

/*
 * void
 * sd01intf(struct sb *sbp)
 *	This function is called by the host adapter driver when a host
 *	adapter function request has completed.  If the request completed
 *	without error, then the block is marked as free.  If there was error
 *	the request is retried.  Used for resume function completions.
 * Called by: Host Adapter driver.
 * Side Effects: None
 *
 *	A SCSI disk driver function request was retried.  The retry 
 *	performed because the host adapter driver detected an error.  
 *	The SDI return code is the second error code word.  See
 *	the SDI return codes for more information.
 *
 *	The host adapter rejected a request from the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 *	A SCSI disk driver function request failed because  the host
 *	adapter driver detected a fatal error or the retry count was
 *	exceeded.  This failure will cause the effected unit to
 *	hang.  The system must be rebooted. The SDI return code is
 *	the second error code word.  See the SDI return codes for
 *	more information.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01intf(struct sb *sbp)
{
	register struct disk *dk;
	dev_t dev;			/* External device number	*/
	minor_t minor;

	dev = makedevice(sbp->SFB.sf_dev.sa_major, sbp->SFB.sf_dev.sa_minor);
	minor = geteminor(dev);
	dk  = DKPTR(minor);
	ASSERT(dk);

#ifdef DEBUG
        DPR (1)("sd01intf: (sbp=0x%x) dk=0x%x\n", sbp, dk);
#endif

	SD01_DK_LOCK(dk);

	if (sbp->SFB.sf_comp_code & SDI_RETRY &&
		dk->dk_spcount < SD01_RETRYCNT)
	{				/* Retry the function request */

		dk->dk_spcount++;
		dk->dk_error++;

		sd01logerr(sbp, dk, 0x4dd0f001);
		SD01_DK_UNLOCK(dk);

		if (sdi_icmd(sbp, KM_NOSLEEP) == SDI_RET_OK)
			return;
		SD01_DK_LOCK(dk);
	}
	
	if (sbp->SFB.sf_comp_code != SDI_ASW)
	{
		sd01logerr(sbp, dk, 0x6dd0f003);
	}


	dk->dk_spcount = 0;		/* Zero retry count */
 
	/*
	*  Currently, only RESUME SFB uses this interrupt handler
	*  so the following block of code is OK as is.
	*/

	sd01_resume_pl = LOCK(sd01_resume_mutex, pldisk);
 	/* This disk LU has just been resumed */
 	sd01_resume.res_head = dk->dk_fltnext;
 
	/* Are there any more disk queues needing resuming */
 	if (sd01_resume.res_head == (struct disk *) &sd01_resume)
	{	/* Queue is empty */

		/*
		*  There is a pending resume for this device so 
		*  since the Q is empty, just put the device back
		*  at the head of the Q.
		*/
		if (dk->dk_state & DKPENDRES)
		{
			dk->dk_state &= ~DKPENDRES;
			sd01_resume.res_head = dk;
			UNLOCK(sd01_resume_mutex, sd01_resume_pl);
			sd01resume(sd01_resume.res_head);
		}
		/*
		*  The Resume has finished for this device so clear
		*  the bit indicating that this device was on the Q.
		*/
		else
		{
			dk->dk_state &= ~DKONRESQ;
 			sd01_resume.res_tail = (struct disk *) &sd01_resume;
			UNLOCK(sd01_resume_mutex, sd01_resume_pl);
		}
	}
 	else
	{	/* Queue not empty */

		/*  Is there another RESUME pending for this disk */
		if (dk->dk_state & DKPENDRES)
		{
			dk->dk_state &= ~DKPENDRES;

			/* Move Next Resume for this disk to end of Queue */
			sd01_resume.res_tail->dk_fltnext = dk;
			sd01_resume.res_tail = dk;
			dk->dk_fltnext = (struct disk *) &sd01_resume;
		}
		else	/* Resume next disk */
			dk->dk_state &= ~DKONRESQ;

		UNLOCK(sd01_resume_mutex, sd01_resume_pl);
 		sd01resume(sd01_resume.res_head);
	}

	SD01_DK_UNLOCK(dk);

#ifdef DEBUG
        DPR (2)("sd01intf: - exit\n");
#endif
}

/*
 * void
 * sd01retry(struct job *jp)
 *	Retries a failed job. 
 * Called by: sd01intf, and sd01intn
 * Side Effects: If necessary the que suspended bit is set.
 *
 * Errors:
 *	The host adapter rejected a request from the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires disk struct's dk_lock.
 */
void
sd01retry(struct job *jp)
{
	register struct sb *sbp;
	struct disk *dk;

	sbp = jp->j_cont;
	dk = jp->j_dk;

#ifdef DEBUG
        DPR (1)("sd01retry: (jp=0x%x) dk=0x%x\n", jp, dk);
#endif

	SD01_DK_LOCK(dk);
	
	jp->j_error++;			/* Update the error counts */
	dk->dk_error++;
	
	if (sbp->SCB.sc_comp_code & SDI_SUSPEND)
	{
		dk->dk_state |= DKSUSP;
	}

	SD01_DK_UNLOCK(dk);
		
	sbp->SCB.sc_time = JTIME;	/* Reset the job time */
	sbp->sb_type = ISCB_TYPE;
	
	sd01timeout(jp);
	if (sdi_icmd(sbp, KM_NOSLEEP) != SDI_RET_OK)
	{
#ifdef DEBUG
		cmn_err(CE_CONT, "Disk Driver: Bad type to HA");
#endif
					/* Fail the job */
		sd01untimeout(jp, SDI_ERROR);
		sd01comp1(jp);
	}

#ifdef DEBUG
        DPR (2)("sd01retry: - exit\n");
#endif
}

/*
 * void
 * sd01intn(struct sb *sbp)
 *	Normal interrupt routine for SCSI job completion.
 *	This function is called by the host adapter driver when a
 *	SCSI job completes.  If the job completed normally the job
 *	is removed from the disk job queue, and the requester's
 *	completion function is called to complete the request.  If
 *	the job completed with an error the job will be retried when
 *	appropriate.  Requests which still fail or are not retried
 *	are failed and the error number is set to EIO.    Device and
 *	controller errors are logged and printed to the user
 *	console.
 * Called by: Host adapter driver
 * Side Effects: 
 *	Requests are marked as complete.  The residual count and error
 *	number are set if required.
 *
 * Errors:
 *	The host adapter rejected a request sense job from the SCSI 
 *	disk driver.  The originally failing job will also be failed.
 *	This is caused by a parameter mismatch within the driver. The system
 *	should be rebooted.
 *
 *	The SCSI disk driver is retrying an I/O request because of a fault
 *	which was detected by the host adapter driver.  The second error
 *	code indicates the SDI return code.  See the SDI return code for 
 *	more information as to the detected fault.
 *
 *	The addressed SCSI disk returned an unusual status.  The job
 *	will be  retried later.  The second error code is the status
 *	which was  returned.  This condition is usually caused by a
 *	problem in  the target controller.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires disk struct's dk_lock.
 */
void
sd01intn(struct sb *sbp)
{
	register struct disk *dk;
	struct job *jp;

	jp = (struct job *)sbp->SCB.sc_wd;
	sd01untimeout(jp, sbp->SCB.sc_comp_code);
	dk = jp->j_dk;

	if ((sbp->SCB.sc_comp_code == SDI_ASW)
	   || (sbp->SCB.sc_comp_code == SDI_QFLUSH))
	{
#ifdef SD01_DEBUG
		if (sd01alts_debug & DKD || sd01alts_debug & HWREA) {
			/*
			 * This is part of the bad block hook to simulate an
			 * ECC error.  A good completion occurred, however,
			 * if the block specified in sd01alts_badspot, on disk
			 * dk_badspot is part of this job, then make it look
			 * as though the completion was CKSTAT.  Enter
			 * the gauntlet, and proceed to suspend the queue.
			 * The next intercept is in sd01ints.
			 */
			if (dk_badspot == dk && sd01alts_badspot &&
		    	(sd01alts_badspot >= jp->j_daddr) &&
		    	(sd01alts_badspot<(jp->j_daddr+jp->j_seccnt))) {
				/* Simulate an error */
				jp->j_cont->SCB.sc_comp_code = SDI_CKSTAT;
				sbp->SCB.sc_status = S_CKCON;
			}
		} else
#endif
                {
			if ((jp->j_flags & J_BADBLK) &&
			    !(jp->j_bp->b_flags & B_READ)) {
				/* ECC occurred on a previous read, and this
			 	* is a write, so suspend the queue since we are
			 	* going to handle the reassign/remap now.
			 	*/
				sd01flts(dk, jp);
				return;
			}
			sd01comp1(jp);
			return;
		}
	}
	
#ifdef DEBUG
        DPR (1)("sd01intn: (sbp=0x%x) dk=0x%x\n", sbp, dk);
#endif

	SD01_DK_LOCK(dk);

	if (sbp->SCB.sc_comp_code & SDI_SUSPEND)
	{
		dk->dk_state |= DKSUSP;	/* Note the queue is suspended */
	}

	/* When a job timed out retry it immediately without issuing error */
	/* messages. If retries fail then switch to normal bad block comp  */
	/* code of SDI_CKSTAT and status code of S_CKCON so mechanism to   */
	/* remap bad block occurs                                          */
	if (sbp->SCB.sc_comp_code == SDI_TIME) {
		if (jp->j_error < SD01_RETRYCNT) {
			SD01_DK_UNLOCK(dk);
			sd01retry(jp);
			return;
		}
		else {
			sbp->SCB.sc_comp_code = SDI_CKSTAT;
			sbp->SCB.sc_status = S_CKCON;
		}
	}


	if (sbp->SCB.sc_comp_code == SDI_CKSTAT && 
		sbp->SCB.sc_status == S_CKCON &&
		jp->j_error < SD01_RETRYCNT)
	{				/* We need to do a request sense */
 		/*
		*  The job pointer is set here and cleared in the
		*  gauntlet when the job is eventually retried
		*  or when the gauntlet exits due to an error.
 		*/
		SD01_DK_UNLOCK(dk);

 		sd01flts(dk, jp);
#ifdef DEBUG
        DPR (2)("sd01intn: - return\n");
#endif
 		return;
	}
	
 	if (sbp->SCB.sc_comp_code == SDI_CRESET ||
	    sbp->SCB.sc_comp_code == SDI_RESET)
 	{
 		/*
		*  The job pointer will be cleared when the job
		*  eventually is retried or when the gauntlet
		*  decides to exit due to an error.
		*/
		SD01_DK_UNLOCK(dk);

 		sd01flts(dk, jp);
 		return;
 	}
	
	/*
	*  To get here, the failure of the job was not due to a bus reset
	*  nor to a Check Condition state.
	*
	*  Now check for the following conditions:
	*     -  The RETRY bit was not set on SDI completion code.
	*     -  Exceeded the retry count for this job.
	*     -  Job status indicates RESERVATION Conflict.
	*     -	 Job status indicates SELECTION Timeout.
	*
	*  If one of the conditions is TRUE, then return the failed job
	*  to the user.
	*/
	if ((sbp->SCB.sc_comp_code & SDI_RETRY) == 0 ||
		jp->j_error >= SD01_RETRYCNT ||
		sbp->SCB.sc_comp_code == SDI_NOSELE ||
		(sbp->SCB.sc_comp_code == SDI_CKSTAT &&
		 sbp->SCB.sc_status == S_RESER))
	{
		SD01_DK_UNLOCK(dk);

		sd01comp1(jp);
		return;
	}

	
	if (sbp->SCB.sc_comp_code == SDI_CKSTAT)
	{				/* Retry the job later */
#ifdef DEBUG
		DPR(4)("SD01: Controller %d, %d, returned an abnormal status: %x.\n",
			dk->dk_addr.sa_major, dk->dk_addr.sa_minor, 
			sbp->SCB.sc_status);
#endif
		sd01logerr(sbp, dk, 0x4dd13003);
		sdi_timeout(sd01retry, (caddr_t)jp, drv_usectohz(LATER), pldisk,
			&dk->dk_addr);
		SD01_DK_UNLOCK(dk);
		return;
	}

	/* Retry the job */
	sd01logerr(sbp, dk, 0x4dd13002);
	SD01_DK_UNLOCK(dk);
	sd01retry(jp);

#ifdef DEBUG
        DPR (2)("sd01intn: - exit\n");
#endif
}

/*
 * int
 * sd01phyrw(struct disk *dk, long dir, struct phyio *phyarg, int mode)
 *	This function performs a physical read or write without
 *	regrad to the VTOC.   It is called by the ioctl.  The
 *	argument for the ioctl is  a pointer to a structure
 *	which indicates the physical block address and the address
 *	of the data buffer in which the data is to be transferred. 
 *	The mode indicates whether the buffer is located in user or
 *	kernel space.
 * Called by: sd01wrtimestamp, sd01ioctl, sd01loadvtoc
 * Side Effects: 
 *	 The requested physical sector is read or written. If the PD sector
 *	 or VTOC have been updated they will be read in-core on the next access 
 *	 to the disk (DKUP_VTOC set in sd01vtoc_ck). However, if the FDISK 
 *	 sector is updated it will be read in-core on the next open.
 * Return Values:
 *	Status is returned in the retval byte of the structure
 *	for driver routines only.
 *
 * Calling/Exit State:
 *	No locks held across entry or exit.
 *	Acquires dk_lock.
 */
int
sd01phyrw(struct disk *dk, long dir, struct phyio *phyarg, int mode)
{
	register struct job *jp;
	register buf_t *bp;
	char		*tmp_buf;
	int 	 size;			/* Size of the I/O request 	*/
	int 	 ret_val = 0;		/* Return value			*/
	unsigned long	dksects;

#ifdef DEBUG
        DPR (1)("sd01phyrw: (dk=0x%x dir=0x%x mode=0x%x)\n", dk, dir, mode);
        DPR (1)("phyarg: sectst=0x%x memaddr=0x%x datasz=0x%x\n", phyarg->sectst, phyarg->memaddr, phyarg->datasz);
#endif

	/* prevent reading or writing beyond the end of the disk */
	dksects = DKSIZE(dk);
	if (phyarg->sectst >= dksects)	{
		phyarg->retval = dir == V_RDABS ? V_BADREAD:V_BADWRITE;
		return ENXIO;
	}
	else if (phyarg->sectst +
			(unsigned long)(phyarg->datasz >> BLKSHF(dk))
								>= dksects)
		phyarg->datasz = (dksects - phyarg->sectst) << BLKSHF(dk);

	/* If the request is for the PD sector, override the supplied
	 * sector parameter
	 */
	if (dk->dk_pdsec.sanity == VALID_PD &&
	    phyarg->sectst == dk->unixst + HDPDLOC && mode == SD01_USER) {
		if (phyarg->datasz > BLKSZ(dk))
			phyarg->datasz = BLKSZ(dk);
	}
	
	bp = getrbuf(KM_SLEEP);
	bp->b_flags |= B_BUSY;
	bp->b_error = 0;
 	bp->b_iodone = NULL;
	bp->b_bcount = BLKSZ(dk);
	bp->b_edev = makedevice(dk->dk_addr.sa_major, dk->dk_addr.sa_minor);

	if (mode != SD01_KERNEL) {
		/* Note: OK to sleep since its not a Kernel I/O request */
		tmp_buf = (char *) kmem_alloc(BLKSZ(dk), KM_SLEEP); 
	}
	
	phyarg->retval = 0;
	while(phyarg->datasz > 0) {
		size = phyarg->datasz > BLKSZ(dk) ? BLKSZ(dk) : phyarg->datasz;
		if ( mode == SD01_KERNEL)
			bp->b_un.b_addr = (char *) phyarg->memaddr;
		else
			bp->b_un.b_addr = tmp_buf;
		bp->b_bcount = size;
		bp->b_flags &= ~B_DONE;
		bp->b_error  = 0;
		bp->b_blkno = phyarg->sectst << BLKSEC(dk);

		if (dir == V_RDABS) 
			bp->b_flags |= B_READ;
		else {
			if (mode != SD01_KERNEL) {	/* Copy user's data */
				if(copyin((void *)phyarg->memaddr,
				    (caddr_t)bp->b_un.b_addr, size)) {
					phyarg->retval = V_BADWRITE;
					ret_val=EFAULT;
					freerbuf(bp);
					break;
				}
				/*
				 * If over-writing the PD sector and VTOC,
				 * make sure the PD sector and VTOC data is
				 * sane before updating the disk.
				 * Also, make sure the user is not destroying
				 * a mounted partition.
				 */
				if ((phyarg->sectst == dk->unixst + HDPDLOC) &&
				    (ret_val=sd01vtoc_ck(dk, bp, SD01_PBLK))) {
					phyarg->retval = V_BADWRITE;
					break;
				}
			}
		}

		jp = sd01getjob(dk, KM_SLEEP); /* Job is returned by sd01done */
		set_sjq_daddr(jp,phyarg->sectst);
		set_sjq_memaddr(jp,paddr(bp));
		jp->j_seccnt = size >> BLKSHF(dk);

		sd01strat1(jp, dk, bp);
		biowait(bp);
		if(bp->b_flags & B_ERROR) {			
			if (bp->b_error == EFAULT) { /* retry after software remap */
				continue;
			} else {		/* Fail the request */
				phyarg->retval = dir == V_RDABS ? V_BADREAD:V_BADWRITE;
				ret_val = bp->b_error;
				break;
			}
		}
		
		/* Check if we are updating the fdisk table */
		if (dir == V_WRABS && phyarg->sectst == FDBLKNO 
			&& mode == SD01_USER)
		{	
			if (((struct mboot *)(void *)bp->b_un.b_addr)->signature 
			   != MBB_MAGIC) {
				SD01_DK_LOCK(dk);
				dk->dk_state |= DKUP_VTOC;
				dk->dk_vtoc.v_sanity = 0;
				SD01_DK_UNLOCK(dk);
			}
		}

		if(dir == V_RDABS && mode != SD01_KERNEL)	
		{			/* Update the requestor's buffer */
			if (copyout((void *)bp->b_un.b_addr, (void *)phyarg->memaddr, size))
			{
				phyarg->retval = V_BADREAD;
				ret_val=EFAULT;
				break;
			}
		}
		
		phyarg->memaddr += size;
		phyarg->datasz -= size;
		phyarg->sectst++;
	}
	
	if (mode != SD01_KERNEL)
		kmem_free(tmp_buf, BLKSZ(dk));

	freerbuf(bp);

#ifdef DEBUG
        DPR (2)("sd01phyrw: - exit(%d)\n", ret_val);
#endif
	return(ret_val);
}

/*
 *
 * int
 * sd01cmd(struct disk *dk, char op_code, uint addr, char *buffer, uint size,
 * uint length, ushort mode, int fault)
 *	This funtion performs a SCSI command such as Mode Sense on
 *	the addressed disk. The op code indicates the type of job
 *	but is not decoded by this function. The data area is
 *	supplied by the caller and assumed to be in kernel space. 
 *	This function will sleep.
 * Called by: sd01wrtimestamp, sd01flt, sd01ioctl
 * Side Effects: 
 *	A Mode Sense command is added to the job queue and sent to
 *	the host adapter.
 * Return values:
 *	Zero is returned if the command succeeds. 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sd01cmd(struct disk *dk, char op_code, uint addr, char *buffer, uint size, uint length, ushort mode, int fault)
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
#ifdef PDI_SVR42
	register struct scsi_iotime *stat = &dk->dk_stat;
#endif /* PDI_SVR42 */
	int error;

#ifdef DEBUG
        DPR (1)("sd01cmd: (dk=0x%x)\n", dk);
#endif
	
	bp = getrbuf(KM_SLEEP);
	bp->b_flags |= B_BUSY;
	bp->b_error = 0;
 	bp->b_iodone = NULL;
	
	for (;;) 
	{
	jp   = sd01getjob(dk, KM_SLEEP);
	scb  = &jp->j_cont->SCB;
	
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_un.b_addr = (caddr_t) buffer;	/* not used in sd01intn */
	bp->b_bcount = size;
	bp->b_error = 0;
	
	jp->j_dk = dk;
	jp->j_bp = bp;
	jp->j_done = sd01done;
	if (!fault)
		jp->j_flags |= J_NOERR;
	
	if (op_code & 0x20) { /* Group 1 commands */
		register struct scm *cmd;

		cmd = &jp->j_cmd.cm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = dk->dk_addr.sa_lun;	/* read only */
		cmd->sm_res1 = 0;
		cmd->sm_addr = addr;
		cmd->sm_res2 = 0;
		cmd->sm_len  = length;
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
	else {	/* Group 0 commands */
		register struct scs *cmd;

		cmd = &jp->j_cmd.cs;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = dk->dk_addr.sa_lun;	/* read only */
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = (addr & 0xFFFF);
		cmd->ss_len   = (ushort) length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
	
	scb->sc_int = sd01intn;
	/* sd01getjob did this? */  scb->sc_dev = dk->dk_addr;	/* read only */
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;
	sdi_translate(jp->j_cont, bp->b_flags,NULL, KM_SLEEP);

	SD01_DK_LOCK(dk);

#ifdef PDI_SVR42
	dk->dk_count++;
	stat->ios.io_ops++;
	stat->ios.io_misc++;
	stat->ios.io_qcnt++;
#endif
	sd01queue_low(jp, dk);
	sd01sendq(dk, KM_SLEEP);
	biowait(bp);

	/* exit if success or if error not due to software bad sector remap */
	if (bp->b_flags & B_ERROR) {
		if (bp->b_error != EFAULT)
			break;
	} else
		break;

	}

#ifdef DEBUG
	if (bp->b_flags & B_ERROR)
        	DPR (2)("sd01cmd: - exit(%d)\n", bp->b_error);
	else
        	DPR (2)("sd01cmd: - exit(0)\n");
#endif
	if (bp->b_flags & B_ERROR)
		error = bp->b_error;
	else
		error = 0;
	freerbuf(bp);
	return (error);
}


/*
 * int
 * sd01ioctl(dev_t dev, int cmd, int arg, int mode, cred_t *cred_p, int *rval_p)
 *	This function provides a number of different functions for use by
 *	utilities. They are: physical read or write, and read or write
 *	physical descriptor sector.  
 *	"READ ABSOLUTE"
 *	The Absolute Read command is used to read a physical sector on the
 *	disk regardless of the VTOC.  The data is transferred into buffer
 *	specified by the argument structure.
 *	"WRITE ABSOLUTE"
 *	The Absolute Write command is used to write a physical sector on the
 *	disk regardless of the VTOC.  The data is transferred from a buffer
 *	specified by the argument structure.
 *	"PHYSICAL READ"
 *	The Physical Read command is used to read any size data block on the
 *	disk regardless of the VTOC or sector size.  The data is transferred 
 *	into buffer specified by the argument structure.
 *	"PHYSICAL WRITE"
 *	The Physical Write command is used to write any size data block on the
 *	disk regardless of the VTOC or sector size.  The data is transferred 
 *	from a buffer specified by the argument structure.
 *	"READ PD SECTOR"
 *	This function reads the physical descriptor sector on the disk.
 *	"WRITE PD SECTOR"
 *	This function writes the physical descriptor sector on the disk.
 *	"CONFIGURATION"
 *	The Configuration command is used by mkpart to reconfigure a drive.
 *	The driver will update the in-core disk configuration parameters.
 *	"GET PARAMETERS"
 *	The Get Parameters command is used by mkpart and fdisk to get 
 *	information about a drive.  The disk parameters are transferred
 *	into the disk_parms structure specified by the argument.
 *	"RE-MOUNT"
 *	The Remount command is used by mkpart to inform the driver that the 
 *	contents of the VTOC has been updated.  The driver will update the
 *	in-core copy of the VTOC on the next open of the device.
 *	"PD SECTOR NUMBER"
 *	The PD sector number command is used by 386 utilities that need to
 *	access the PD and VTOC information. The PD and VTOC information
 *	will always be located in the 29th block of the UNIX partition.
 *	"PD SECTOR LOCATION"
 *	The PD sector location command is used by SCSI utilities that need to
 *	access the PD and VTOC information. The absolute address of this
 *	sector is transferred into an integer specified by the argument.
 *	"ELEVATOR"
 *	This Elevator command allows the user to enable or disable the
 *	use of the elevator algorithm.
 *	"RESERVE"
 *	This Reserve command will reserve the addressed device so that no other
 *	initiator can use it.
 *	"RELEASE"
 *	This Release command releases a device so that other initiators can
 *	use the device.
 *	"RESERVATION STATUS"
 *	This Reservation Status command informs the host if a device is 
 *	currently reserved, reserved by another host or not reserved.
 * Called by: Kernel
 * Side Effects: 
 *	The requested action is performed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
/*ARGSUSED*/
int
sd01ioctl(dev_t dev, int cmd, int arg, int mode, cred_t *cred_p, int *rval_p)
{
	register struct disk *dk;
	int	state;			/* Device's RESERVE Status 	*/
	struct 	phyio 	  phyarg;	/* Physical block(s) I/O	*/
	union 	io_arg 	  ioarg;
	struct 	absio 	  absarg;	/* Absolute sector I/O only	*/
	struct 	vtoc 	 vtocarg;	/* soft-VTOC only */
	dev_t 	pt_dev;			/* Pass through device number 	*/
	int 	ret_val = 0;
	minor_t minor;

	int	sd01_nodes = DK_XNUMPART;	/* used by B_GET_SUBDEVS ioclt */

	minor = geteminor(dev);

 	dk = DKPTR(minor);
	ASSERT(dk);

#ifdef DEBUG
        DPR (1)("sd01ioctl: (dev=0x%x cmd=0x%x, arg=0x%x) dk=0x%x\n", dev, cmd, arg, dk);
#endif

        if (cmd == SD01_DEBUGFLG) {
#ifdef SD01_DEBUG
                register short  dbg;

		if (copyin ((char *)arg, Sd01_debug, 10) != 0)
			return(EFAULT);

                cmn_err (CE_CONT, "\nNew debug values :");

                for (dbg = 0; dbg < 10; dbg++) {
                        cmn_err (CE_CONT, " %d", Sd01_debug[dbg]);
                }
                cmn_err (CE_CONT, "\n");
#endif
                return(0);
        }

	switch(cmd)
	{
		case V_WRABS: 	/* Write absolute sector number */
		case V_PWRITE:	/* Write physical data block(s)	*/
		case V_PDWRITE:	/* Write PD sector only		*/

                	/* Make sure user has permission */
			if (ret_val = drv_priv(cred_p))
			{
				return(ret_val);
			}
			/*FALLTHRU*/
		case V_RDABS: 	/* Read absolute sector number 	*/
		case V_PREAD:	/* Read physical data block(s)	*/
		case V_PDREAD:	/* Read PD sector only		*/

			if (cmd == V_WRABS || cmd == V_RDABS) {
				if (copyin((void *)arg, (void *)&absarg, sizeof(absarg)))
					return(EFAULT);

				phyarg.sectst  = (unsigned long) absarg.abs_sec;
				phyarg.memaddr = (unsigned long) absarg.abs_buf;
				phyarg.datasz  = BLKSZ(dk);

				ret_val = sd01phyrw(dk, (long) cmd, &phyarg, SD01_USER);
			}
			else {
				if (copyin((void *)arg, (void *)&phyarg, 
						sizeof(phyarg)))
					return(EFAULT);

				/* Assign PD sector address */
				if (cmd == V_PDREAD || cmd == V_PDWRITE)
				{
					phyarg.sectst = dk->unixst + HDPDLOC;
					cmd = cmd == V_PDREAD ? V_RDABS : V_WRABS;
				}
				else
					cmd = cmd == V_PREAD ? V_RDABS : V_WRABS;

				ret_val = sd01phyrw(dk, (long) cmd, &phyarg, SD01_USER);
				/* Copy out return value to user */
                		if (copyout((void *)&phyarg, (void *)arg, sizeof(phyarg)))
                        		return(EFAULT);
			}

			return(ret_val);

		/* Change drive configuration parameters. */
		case V_CONFIG: {
			int part;

			if ((ret_val = drv_priv(cred_p)) != 0)
				return(ret_val);

			if (copyin((void *)arg, (void *)&ioarg, sizeof(ioarg)))
				return(EFAULT);

			/* Don't allow user to change sector size. */
			if (ioarg.ia_cd.secsiz != BLKSZ(dk))
				return(EINVAL);

			/* Don't allow user to set dp_cyls to zero. */
			if (ioarg.ia_cd.ncyl == 0)
				return(EINVAL);

			SD01_DK_LOCK(dk);

			/* Make sure no other partitions are open or mirrored */
			for (part = 1; part < V_NUMPAR; part++) {
				if (dk->dk_part_flag[part] != DKFREE)	{
					SD01_DK_UNLOCK(dk);
					return(EBUSY);
				}
			}

			/* check special fdisk partitions */
			for (part = 0; part < FD_NUMPART + 1; part++) {
				if (dk->dk_fdisk_flag[part] != DKFREE)	{
					SD01_DK_UNLOCK(dk);
					return(EBUSY);
				}
			}

			dk->dk_parms.dp_heads   = ioarg.ia_cd.nhead;
			dk->dk_parms.dp_cyls    = ioarg.ia_cd.ncyl;
			dk->dk_parms.dp_sectors = ioarg.ia_cd.nsec;

			/* Indicate drive parms have been set */
			dk->dk_state |= DKPARMS; 

			SD01_DK_UNLOCK(dk);

			break;
		}

		/* Get info about the current drive configuration */
		case V_GETPARMS: {
			struct disk_parms	dk_parms;
			int part = DKSLICE(geteminor(dev));

			SD01_DK_LOCK(dk);
			if((dk->dk_state & DKPARMS) == 0)	{
				SD01_DK_UNLOCK(dk);
				return(ENXIO);
			}

			/*
			** This used to be hard coded to DPT_SCSI_HD since
			** sd01 used to be strictly a scsi driver, but with
			** the new Generic Disk Interface from ISC we're
			** incorporating into our source, this driver also
			** is the high level interface to dcd drives also.
			** Name is set to "DCD" in sdi/Space.c
			*/

			if (strncmp(IDP(HBA_tbl[SDI_EXHAN(&dk->dk_addr)].idata)->name, "DCD", 3) == 0)
                                dk_parms.dp_type     = DPT_WINI;
			else
                                dk_parms.dp_type     = DPT_SCSI_HD;

			dk_parms.dp_heads    = dk->dk_parms.dp_heads;
			dk_parms.dp_cyls     = dk->dk_parms.dp_cyls;
			dk_parms.dp_sectors  = dk->dk_parms.dp_sectors;
			dk_parms.dp_secsiz   = dk->dk_parms.dp_secsiz;
			if (part < V_NUMPAR)	{
				dk_parms.dp_ptag =
					dk->dk_vtoc.v_part[part].p_tag;
				dk_parms.dp_pflag =
					dk->dk_vtoc.v_part[part].p_flag;
				dk_parms.dp_pstartsec =
					dk->dk_vtoc.v_part[part].p_start;
				dk_parms.dp_pnumsec =
					dk->dk_vtoc.v_part[part].p_size;
			}
			else	{
				dk_parms.dp_ptag =
					dk->dk_fdisk[part - V_NUMPAR].p_tag;

				/* not used, should be zero */
				dk_parms.dp_pflag =
					dk->dk_fdisk[part - V_NUMPAR].p_flag;
				dk_parms.dp_pstartsec =
					dk->dk_fdisk[part - V_NUMPAR].p_start;
				dk_parms.dp_pnumsec =
					dk->dk_fdisk[part - V_NUMPAR].p_size;
			}
			SD01_DK_UNLOCK(dk);

                	if (copyout((void *)&dk_parms, (void *)arg, sizeof(dk_parms))) 
				return(EFAULT);
                	break;
		}

		/* Force read of vtoc on next open.*/
		case V_REMOUNT:	 {
           		register int    part;

           		/* Make sure user is root */
			if ((ret_val = drv_priv(cred_p)) != 0)
				return(ret_val);

           		/* Make sure no partitions other than 0 are open. */
			SD01_DK_LOCK(dk);
			for (part=1; part < V_NUMPAR; part++)
				if (dk->dk_part_flag[part] != DKFREE)
				{
					SD01_DK_UNLOCK(dk);
					return(EBUSY);
				}

			/* check special fdisk partitions */
			for (part = 0; part < FD_NUMPART + 1; part++) {
				if (dk->dk_fdisk_flag[part] != DKFREE)	{
					SD01_DK_UNLOCK(dk);
					return(EBUSY);
				}
			}

			dk->dk_state |= DKUP_VTOC;
			dk->dk_vtoc.v_sanity = 0;
			SD01_DK_UNLOCK(dk);
                	break;

		}

		/* Tell user the block number of the pdinfo structure */
        	case V_PDLOC: {	
                	unsigned long   pdloc;

			/* Check if fdisk is sane */
			if((dk->dk_state & DKFDISK))	{
                		pdloc = HDPDLOC;
			}
			else	{
                        	return(ENXIO);
			}

                	if (copyout((void *)&pdloc, (void *)arg, sizeof(pdloc)))
                        	return(EFAULT);
                	break;
		}

		/* update the internal alternate bad sector/track table */
		case V_ADDBAD: {
			ret_val=sd01addbad(dev,(caddr_t)arg);
			break;
		}

		/* Update soft VTOC in DK structure. */
		case V_SET_SOFT_VTOC: {
			struct bootdev bd;
			int slice;

			drv_gethardware(BOOT_DEV, &bd);
			if ( bd.bdv_type != BOOT_FLOPPY )
				return(EINVAL);

			if ((ret_val = drv_priv(cred_p)) != 0)
				return(ret_val);

			if (copyin((void *)arg, (void *)&vtocarg, sizeof(vtocarg)))
				return(EFAULT);

			SD01_DK_LOCK(dk);

			bcopy((caddr_t)&vtocarg, (caddr_t)&(dk->dk_vtoc),
				sizeof(vtocarg));

			for (slice = 0; slice < V_NUMPAR; slice++) {
				DK_SLICEMOD_CLEAR(dk, slice);
			}

			/* Indicate soft-VTOC has been set */
			dk->vtoc_state = VTOC_SOFT; 

			SD01_DK_UNLOCK(dk);

			sd01setalts_idx(dk);

			break;
		}

		/* Get the soft VTOC from the DK structure. */
		case V_GET_SOFT_VTOC: {
			if ((dk->dk_vtoc.v_sanity != VTOC_SANE)) {
				return(ENXIO);
			}

        		if (copyout((void *)&(dk->dk_vtoc),
			(void *)arg, sizeof(vtocarg)))	{
				return(EFAULT);
			}
                	break;
		}

		/* Tell user where pdinfo structure is on the disk */
        	case SD_PDLOC: {	
                	unsigned long   pdloc;

			/* Check if fdisk is sane */
			if((dk->dk_state & DKFDISK))	{
                		pdloc = dk->unixst + HDPDLOC;
			}
			else	{
                        	return(ENXIO);
			}

                	if (copyout((void *)&pdloc, (void *)arg, sizeof(pdloc)))
                        	return(EFAULT);
                	break;
		}

		case SD_ELEV:
			SD01_DK_LOCK(dk);

			if ((long) arg)
				dk->dk_state |= DKEL_OFF;
			else
				dk->dk_state &= ~DKEL_OFF;

			SD01_DK_UNLOCK(dk);
			break;

		case SDI_RESERVE:
			ret_val = sd01cmd(dk, SS_RESERV, 0, (char *) 0, 0, 0, SCB_WRITE, TRUE);
			if (ret_val == 0)
			{
				SD01_DK_LOCK(dk);
				dk->dk_state |= (DKRESERVE|DKRESDEVICE);
				SD01_DK_UNLOCK(dk);
			}
			break;

		case SDI_RELEASE:
			ret_val = sd01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ, TRUE);
			if (ret_val == 0) {
				ret_val = sd01cmd(dk, SS_RELES, 0, (char *) 0, 0, 0, SCB_WRITE, TRUE); 
				if (ret_val == 0)
				{
					SD01_DK_LOCK(dk);
					dk->dk_state &=
					  ~(DKRESERVE|DKRESDEVICE);
					SD01_DK_UNLOCK(dk);
				}
			}
			break;

		case SDI_RESTAT:
			SD01_DK_LOCK(dk);

			if (dk->dk_state & DKRESERVE)	{
				SD01_DK_UNLOCK(dk);
				state = 1;	/* Currently Reserved */
			}
			else {
				SD01_DK_UNLOCK(dk);
				if(sd01cmd(dk, SS_TEST, 0, (char *) 0, 0, 0, SCB_READ, TRUE ) == EBUSY)
					/* Reserved by another host*/
					state = 2;
				else
					/* Not Reserved */
					state = 0;
			}

			if (copyout((void *)&state, (void *)arg, sizeof(state)))
				return(EFAULT);
			break;

		case B_GETTYPE:
			if (copyout((void *)SCSI_NAME, 
				((struct bus_type *) arg)->bus_name, 
				strlen(SCSI_NAME)+1))
			{
				return(EFAULT);
			}
			if (copyout((void *)SD01_NAME, 
				((struct bus_type *) arg)->drv_name, 
				strlen(SD01_NAME)+1))
			{
				return(EFAULT);
			}
			break;

		case B_GETDEV:
			sdi_getdev(&dk->dk_addr, &pt_dev);
			if (copyout((void *)&pt_dev, (void *)arg, sizeof(pt_dev)))
				return(EFAULT);
			break;	
		case B_GET_SUBDEVS:
			if (copyout((void *)&sd01_nodes, (void *)arg, sizeof(sd01_nodes)))
				return(EFAULT);
			break;	
		default:
			return(EINVAL);
	}

#ifdef DEBUG
        DPR (2)("sd01ioctl: - exit(%d)\n", ret_val);
#endif
	return(ret_val);
}

/*
 * int
 * sd01print(dev_t dev, char *str)
 *	The function prints the name of the addressed disk unit along 
 *	with an error message provided by the kernel.
 * Called by: Kernel
 * Side Effects: None
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
sd01print(dev_t dev, char *str)
{
	register struct disk *dk;
	char name[SDI_NAMESZ];
	struct scsi_ad addr;

#ifdef DEBUG
        DPR (1)("sd01print: (dev=0x%x)\n", dev);
#endif
	
	addr.sa_major = getemajor(dev);
	addr.sa_minor = geteminor(dev);

	dk = DKPTR(addr.sa_minor);
	ASSERT(dk);
	addr = dk->dk_addr;


	sdi_name(&addr, name);
	/*
	 *+ Error message from sd01print
	 */
	cmn_err(CE_WARN, "Disk Driver (%s), Unit %d, Slice %d:  %s", name, 
		addr.sa_lun, DKSLICE(addr.sa_minor), str);

#ifdef DEBUG
        DPR (2)("sd01print: - exit(0)\n");
#endif
	return(0);
}

/*
 * void
 * sd01insane_dsk(struct disk *dk)
 *	This function prints and logs an error when some attempts to 
 *	access a disk which does not have a valid PD sector or VTOC.
 * Called by: sd01strategy, sd01loadvtoc, sd01ioctl
 * Side Effects: None
 *
 * Errors:
 *	The physical descriptor sector is bad on the addressed disk.
 *	The disk must be formated before it can be accessed for 
 *	normal use. See format(1M).
 *
 *	The Volume Table of Contents is bad on the addressed disk. The 
 *	UNIX system must be partitioned before it can be accessed for normal 
 *	use. See mkpart(1M) and edvtoc(1M).
 *
 *	The Volume Table of Contents version on the addressed disk is not 
 *	supported. The disk must be re-formated and re-partitioned before 
 *	it can be accessed for normal use. See scsiformat(1M), mkpart(1M)
 *	and edvtoc(1M).
 *
 *	The Partition Table is bad on the addressed disk. The disk must 
 *	be partitioned before it can be accessed for normal use. See fdisk(1M).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Serialized by device open.
 */
void
sd01insane_dsk(struct disk *dk)
{
	char name[DRVNAMELEN];

#ifdef DEBUG
        DPR (1)("sd01insane_dsk: (dk=0x%x)\n",dk);
#endif
	sdi_name(&dk->dk_addr, name);
	
	/* Check if fdisk has been initialized */
	if(!(dk->dk_state & DKFDISK)) {
		/*
		 *+ Invalid disk partition table.
		 *+ Power down/up to take care of transient disk failures.
		 *+ If problem persists, run 'fdisk' to correct the
		 *+ partition table 
		 */
		cmn_err(CE_WARN,
		"!Disk Driver: %s Unit %d, Invalid disk partition table\n",
			 name, dk->dk_addr.sa_lun);
	}
	/* Check if PD sector is sane */
	else if (dk->dk_pdsec.sanity != VALID_PD)
	{
		/*
		 *+ Invalid physical descriptor sector.
		 *+ Power down/up to take care of transient disk failures.
		 *+ If problem persists on non-boot disk, run 'diskadd'
		 *+ to correct.  If problem persists on the boot disk,
		 *+ complete reinstallation is necessary.
		 */
		cmn_err(CE_WARN,
		"!Disk Driver: %s Unit %d, Invalid physical descriptor sector\n",
			 name, dk->dk_addr.sa_lun);
	}
	/* Check if VTOC version is supported */
	else if (dk->dk_vtoc.v_sanity == VTOC_SANE &&
	dk->dk_vtoc.v_version > V_VERSION)
	{
		/*
		 *+ Invalid disk VTOC version
		 *+ Power down/up to take care of transient disk failures.
		 *+ If problem persists on non-boot disk, run 'diskadd'
		 *+ to correct.  If problem persists on the boot disk,
		 *+ complete reinstallation is necessary.
		 */
		cmn_err(CE_WARN,
		"!Disk Driver: %s Unit %d, Invalid disk VTOC version\n",
			 name, dk->dk_addr.sa_lun);
	}
	/* VTOC must be insane */
	else {
		/*
		 *+ Invalid disk VTOC
		 *+ Power down/up to take care of transient disk failures.
		 *+ If problem persists on non-boot disk, run 'diskadd'
		 *+ to correct.  If problem persists on the boot disk,
		 *+ complete reinstallation is necessary.
		 */
		cmn_err(CE_WARN,
		"!Disk Driver: %s Unit %d, Invalid disk VTOC\n",
		 	name, dk->dk_addr.sa_lun);
	}

#ifdef DEBUG
        DPR (2)("sd01insane_dsk: - exit\n");
#endif
}

/*
 * STATIC int
 * sd01cmp(caddr_t p1, caddr_t p2, size_t len)
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
sd01cmp(caddr_t p1, caddr_t p2, size_t len)
{
	while(len--)	{
		if ( *p1 != *p2 )
			return 1;
		++p1;
		++p2;
		/*
		if (*((char *)p1)++ != *((char *)p2)++)	{
			return (1);
		}
		*/
	}
	return (0);
}

/*
 * int
 * sd01vtoc_ck(struct disk *dk, buf_t *bp, int part)
 *	This function checks writes which might destroy the VTOC
 *	sector.  This is to prevent the user from accidently over
 *	writing the VTOC or PD sector with invalid data.  It will also
 *	invalidate the current copy of the VTOC so that the next
 *	access will read in both the VTOC and PD info. The user data must 
 *	be directly accessable when this function is called.
 *	If the partition argument is equal to 16 or greater the block address 
 *	is assumed to be physical (same as sd01strat1).
 * Called by: sd01strategy, sd01phyrw, mdstrategy, and mddone
 * Side Effects: The VTOC entry is updated.
 *
 * Calling/Exit State:
 *      No locks held on entry or exit.
 *      Acquires the disk struct's dk_lock while modifying dk_slicemod flags,
 *      via DK_SLICEMOD_SET() macro.
 */
int
sd01vtoc_ck(struct disk *dk, buf_t *bp, int part)
{
	char  	*pt;
	long    start, slice, voffset, vtocblk, blksz, offset;

	slice   = DKSLICE(geteminor(bp->b_edev));

	/* just in case */
	if (slice >= V_NUMPAR)
		return 0;

	vtocblk = dk->unixst + VTBLKNO;
	blksz   = sizeof(dk->dk_pdsec) + sizeof(dk->dk_vtoc);

	if (part < SD01_PBLK)			/* Logical address  */
		start = (bp->b_blkno >> BLKSEC(dk)) +
			dk->dk_vtoc.v_part[slice].p_start;
	else					/* Physical address */
		start = bp->b_blkno >> BLKSEC(dk);

#ifdef DEBUG
        DPR (1)("sd01vtoc_ck: (dk=0x%x bp=0x%x) start=0x%x bcount=0x%x blkno=0x%x\n",
		dk, bp, start, bp->b_bcount, bp->b_blkno);
#endif

	if (start > vtocblk || start < vtocblk 
	   && bp->b_bcount <= BLKSZ(dk) * (vtocblk - start)) {
		return(0);	/* We are not hitting the PD/VTOC sector */
	}
		
	if ((start < vtocblk && bp->b_bcount > BLKSZ(dk) * (vtocblk-start)) &&
		(bp->b_bcount < BLKSZ(dk) * (vtocblk-start) + blksz) ||
		(start == vtocblk && bp->b_bcount < blksz)) {
		return(EACCES);	/* Must write the entire PD/VTOC sector */
	}

#ifndef	PDI_SVR42
	if (bp->b_flags & (B_PAGEIO | B_PHYS))
#else
	if (bp->b_flags & B_PAGEIO) 
#endif
		bp_mapin(bp);

	/* check new PD info sanity */
	offset = (char *) &dk->dk_pdsec.sanity - (char *) &dk->dk_pdsec;
	pt = bp->b_un.b_addr + (BLKSZ(dk)*(vtocblk-start)) + offset;

	if (*((unsigned long *)(void *)pt) != VALID_PD) {
		return(EACCES);	/* The new one is not sane. */
	}

	/* check new vtoc sanity */
	pt = bp->b_un.b_addr + (BLKSZ(dk)*(vtocblk-start));
	voffset = ((struct pdinfo *)(void *) pt)->vtoc_ptr % (int)BLKSZ(dk);
	offset = (char *) &dk->dk_vtoc.v_sanity - (char *) &dk->dk_vtoc;
	pt += voffset + offset;

	if (*((unsigned long *)(void *)pt) != VTOC_SANE) {
		return(EACCES);	/* The new one is not sane. */
	}

	/* Check if the number of partitions is supported */
	pt = bp->b_un.b_addr + (BLKSZ(dk)*(vtocblk-start)) + voffset;

	if(((struct vtoc *)(void *) pt)->v_nparts > V_NUMPAR)
		return(EACCES);

	/*
	 * Has any slice specific information changed?
	 */
	SD01_DK_LOCK(dk);
	for (slice = 0; slice < V_NUMPAR; slice++)	{
		if (sd01cmp((caddr_t)&dk->dk_vtoc.v_part[slice],
			    (caddr_t)&(((struct vtoc *)(void *)pt)->v_part[slice]),
			    sizeof(struct partition)) != 0) {
			DK_SLICEMOD_SET(dk, slice);
		}
	}
	SD01_DK_UNLOCK(dk);

#ifdef DEBUG
        DPR (2)("sd01vtoc_ck: - exit(0)\n");
#endif
	return(0);
}

/*
 * void
 * sd01resume(struct disk *dk)
 *	This function is called only if a queue has been suspended and must
 *	now be resumed. It is called by sd01comp1 when a job has been
 *	failed and a disk queue must be resumed or by sdflterr when there
 *	is no job to fail but the queue needs to be resumed anyway.
 * Called by: sd01comp1, sd01flterr,
 * Side Effects: THe LU queue will have been resumed.
 * Errors:
 *	The HAD rejected a Resume function request by the SCSI disk driver.
 *	This is caused by a parameter mismatch within the driver.
 *	The system should be rebooted.
 *
 * Calling/Exit State:
 *	Caller holds disk struct's dk_lock.
 */
void
sd01resume(struct disk *dk)
{
#ifdef DEBUG
        DPR (1)("sd01resume: (dk=0x%x)\n", dk);
#endif

	dk->dk_spcount = 0;	/* Reset special count */
	sd01_fltsbp->sb_type = SFB_TYPE;
	sd01_fltsbp->SFB.sf_int = sd01intf;
	sd01_fltsbp->SFB.sf_dev = dk->dk_addr;
	sd01_fltsbp->SFB.sf_func = SFB_RESUME;

	SD01_DK_UNLOCK(dk);
	sdi_icmd(sd01_fltsbp, KM_NOSLEEP);
	SD01_DK_LOCK(dk);
	dk->dk_state &= ~DKSUSP;

#ifdef DEBUG
        DPR (2)("sd01resume: - exit\n");
#endif
}

/*
 * void
 * sd01qresume(struct disk *dk)
 *	Checks if the SB used for resuming a LU queue is currently busy.
 *	If it is busy, the current disk is added to the end of the list 
 *	of disks waiting for a resume to be issued.
 *	If the SB is not busy, this disk is put at the front of the
 *	list and the resume for this disk is started immediately.
 *
 *	Called by sd01comp1, sd01flterr
 *	Side effects: A disk structure is added to the Resume queue.
 *
 *	Calling/Exit State:
 *		Disk struct's dk_lock held on entry and return, but
 *		dropped in the middle.
 *		Acquires sd01_resume_mutex while modifying sd01_resume.
 */
void
sd01qresume(dk)
struct disk *dk;
{
#ifdef DEBUG
        DPR (1)("sd01qresume: (dk=0x%x)\n", dk);
#endif

	sd01_resume_pl = LOCK(sd01_resume_mutex, pldisk);
	/* Check if the Resume SB is currently being used */
	if (sd01_resume.res_head == (struct disk *) &sd01_resume)
	{	/* Resume Q not busy */

		dk->dk_state |= DKONRESQ;
		sd01_resume.res_head = dk;
		sd01_resume.res_tail = dk;
		dk->dk_fltnext = (struct disk *) &sd01_resume;
		UNLOCK(sd01_resume_mutex, sd01_resume_pl);
		/* ok to hold dk_lock across this */
		sd01resume(dk);
	}
	else
	{	/* Resume Q is Busy */

		/*
		*  This disk may already be on the Resume Queue.
		*  If it is, then set the flag to indicate that
		*  another Resume is pending for this disk.
		*/
		if (dk->dk_state & DKONRESQ)
		{
			dk->dk_state |= DKPENDRES;
		}
		else
		{	/* Not on Q, put it there */
			dk->dk_state |= DKONRESQ;
			sd01_resume.res_tail->dk_fltnext = dk;
			sd01_resume.res_tail = dk;
			dk->dk_fltnext = (struct disk *) &sd01_resume;
		}
		UNLOCK(sd01_resume_mutex, sd01_resume_pl);
	}

#ifdef DEBUG
        DPR (2)("sd01qresume: - exit\n");
#endif
}

/*
 * int
 * sd01config(struct disk *dk, minor_t minor)
 *	This function initializes the driver's disk parameter structure.
 *	If the READ CAPACITY or MODE SENSE commands fail, a non-fatal
 *	error status is returned so that sd01loadvtoc() routine does not
 *	fail.  In this case, the V_CONFIG ioctl can still be used to set
 *	the drive parameters.
 * Called by: sd01loadvtoc
 * Side Effects: The disk state flag will indicate if the drive parameters 
 *	 	  are valid.
 * Return Values:
 *	-1: Non-fatal error detected
 *	 0: Successfull completion
 *	>0: Fatal error detected - Error code is returned
 * Errors:
 *	The sectors per cylinder parameter calculates to less than or equal
 *	to zero.  This is caused by incorrect data returned by the MODE
 *	SENSE command or the drivers master file. This may not be an AT&T 
 *	supported device.
 *
 *	The number of cylinder calculates to less than or equal to zero.  
 *	This is caused by incorrect data returned by the READ CAPACITY
 *	command. This may not be an AT&T supported device.
 *
 *	The READ CAPACITY command failed.
 *
 *	The Logical block size returned by the READ CAPACITY command is
 *	invalid i.e. it is not a power-of-two multiple of the kernel
 *	block size (KBLKSZ).
 *
 *	Unable able to allocate memory for the Margianl Block data area
 *	for this device.
 *
 *	The MODE SENSE command for Page 3 failed.
 *
 *	The MODE SENSE command for Page 4 failed.
 *
 * Calling/Exit State:
 *	Access is serialized by the disk struct's dk_sv.
 */
int
sd01config(struct disk *dk, minor_t minor)
{
	DADF_T	   *dadf = (DADF_T *) NULL;
	RDDG_T	   *rddg = (RDDG_T *) NULL;
	CAPACITY_T *cap  = (CAPACITY_T *) NULL;

	uint	pg_asec_z;
	long	cyls = 0;
	long	sec_cyl;
	int	i;
	int 	unit;
	int	gotparams = 0, disk_is_SCSI;
	
#ifdef DEBUG
        DPR (1)("sd01config: (dk=0x%x)\n", dk);
#endif

	unit = sd01_minor_to_disk(minor);

	ASSERT(unit != -1);
	/* Send READ CAPACITY to obtain last sector address */
	if (sd01cmd(dk,SM_RDCAP,0,dk->dk_rc_data,RDCAPSZ,0,SCB_READ, TRUE)) {
		return(EIO);
	}

	cap = (CAPACITY_T *)(void *) (dk->dk_rc_data);

	cap->cd_addr = sdi_swap32(cap->cd_addr);
	cap->cd_len  = sdi_swap32(cap->cd_len);

	/*
	 * Init the Block<->Logical Sector convertion values.
	 */
	for (i=0; i < (32-KBLKSHF); i++) {
		if ((KBLKSZ << i) == cap->cd_len) {
			break;
		}
	}
	if ((KBLKSZ << i) != cap->cd_len) {
		/*
		 *+ The Logical block size returned by the READ CAPACITY
		 *+ command is invalid i.e. it is not a power-of-two 
		 *+ multiple of the kernel block size (KBLKSZ).
		 *+ Probably caused by a bad value in the sd01diskinfo
		 *+ structure in the driver's Space.c
		 */
		cmn_err (CE_WARN, "Disk Driver: Sect size (%x) Not power-of-two of %x", cap->cd_len, KBLKSZ);
		return(-1);
	}

	dk->dk_parms.dp_secsiz = cap->cd_len;
	dk->dk_sect.sect_blk_shft = i;
	dk->dk_sect.sect_byt_shft = KBLKSHF + i;
	dk->dk_sect.sect_blk_mask = ((1 << i) - 1);

	disk_is_SCSI = strncmp(IDP(HBA_tbl[SDI_EXHAN(&dk->dk_addr)].idata)->name, "DCD", 3);

	if (disk_is_SCSI && unit == 0) {
		/*
		 * Currently, the boot program supplies BIOS
		 * parameters for the two primary disks.
		 */
		struct hdparms hd;

		hd.hp_unit = unit;
		if(drv_gethardware(HD_PARMS, &hd) != -1 ) {
			dk->dk_parms.dp_sectors = (unchar) hd.hp_nsects;
			dk->dk_parms.dp_heads   = (unchar) hd.hp_nheads;
			sec_cyl = dk->dk_parms.dp_sectors * dk->dk_parms.dp_heads;
			gotparams = 1;
		}
	}
	if(!gotparams) {
	   /*
	    * If this is a SCSI based drive, then use the virtual
	    * geometry if specified.  Otherwise, use the physical
	    * geometry as returned by the MODE SENSE Page 3 and 4.
	    */
	   if (disk_is_SCSI && (Sd01diskinfo[i] != 0)) {
#ifdef DEBUG
	DPR (3)("Using a Virtual geometry\n");
#endif
		/*
		 * Use the virtual geometry specified in the space.c file
		 */
		dk->dk_parms.dp_sectors  = (Sd01diskinfo[i] >> 8) & 0x00FF;
		dk->dk_parms.dp_heads    = Sd01diskinfo[i] & 0x00FF;

		sec_cyl = dk->dk_parms.dp_sectors * dk->dk_parms.dp_heads;
	    } else {
#ifdef DEBUG
		DPR (3)("Using the disks Physical geometry\n");
#endif
		/*
		 * Get the sectors/track value from MODE SENSE Page-3.
		 */
		if (sd01cmd(dk,SS_MSENSE,0x0300,dk->dk_ms_data,FPGSZ,FPGSZ,SCB_READ, TRUE)) {
			return(-1);
		}

		dadf = (DADF_T *)(void *) (dk->dk_ms_data + SENSE_PLH_SZ + 
	       		((SENSE_PLH_T *)(void *) dk->dk_ms_data)->plh_bdl);

		/* Swap Page-3 data */
		dadf->pg_sec_t   = sdi_swap16(dadf->pg_sec_t);
		dadf->pg_asec_z  = sdi_swap16(dadf->pg_asec_z);
		dadf->pg_bytes_s = sdi_swap16(dadf->pg_bytes_s);

		dk->dk_parms.dp_sectors =
			(dadf->pg_bytes_s * dadf->pg_sec_t) / cap->cd_len;  

		pg_asec_z = dadf->pg_asec_z;

		/*
		 * Get # of heads from MODE SENSE Page-4.
		 */
		if (sd01cmd(dk,SS_MSENSE,0x0400,dk->dk_ms_data,RPGSZ,RPGSZ,SCB_READ, TRUE)) {
			return(-1);
		}

		rddg = (RDDG_T *) (void *)(dk->dk_ms_data + SENSE_PLH_SZ + 
	       		((SENSE_PLH_T *)(void *) dk->dk_ms_data)->plh_bdl);

		sec_cyl = rddg->pg_head * (dk->dk_parms.dp_sectors - pg_asec_z);

		dk->dk_parms.dp_heads = rddg->pg_head;
	    }
	}

	/* Check sec_cly calculation */
	if (sec_cyl <= 0)
	{
		/*
		 *+ The sectors per cylinder parameter calculates to less
		 *+ than or equal to zero.  This is caused by incorrect data
		 *+ returned by the MODE SENSE command.  Possibly the CMOS
		 *+ parameters have an invalid value, or the SCSI drive
		 *+ has errors, or this is not a supported device.
		 */
       		cmn_err(CE_WARN, "!Disk Driver: Sectors per cylinder error cyls=0x%x\n",cyls);
		return(-1);
	}

	cyls = (cap->cd_addr + 1) / sec_cyl;

	/* Check cyls calculation */
	if (cyls <= 0)
	{
		/*
		 *+ The number of cylinder parameter calculates to less
		 *+ than or equal to zero.  This is caused by incorrect data
		 *+ returned by the MODE SENSE command.  Possibly the CMOS
		 *+ parameters have an invalid value, or the SCSI drive
		 *+ has errors, or this is not a supported device.
		 */
       		cmn_err(CE_NOTE, "!Disk Driver: Number of cylinders error.");
		return(-1);
	}

	/* Make room for diagnostic scratch area */
	if ((cap->cd_addr + 1) == (cyls * sec_cyl))
		cyls--;

	/* Assign cylinder parameter for V_GETPARMS */
	dk->dk_parms.dp_cyls = (ushort) cyls;

	/*
	 * Set the breakup granularity to the device sector size.
	 */
#ifndef PDI_SVR42
	dk->dk_bcbp->bcb_granularity = BLKSZ(dk);

	if (!(dk->dk_bcbp->bcb_flags & BCB_PHYSCONTIG) &&
	    !(dk->dk_bcbp->bcb_addrtypes & BA_SCGTH) &&
	    !(dk->dk_iotype & F_PIO)) {
		/*
		 * Set up a bcb for maxxfer and maxxfer - ptob(1), and
		 * set phys_align for drivers that are not using buf_breakup
		 * to break at every page, and not using the BA_SCGTH
		 * feature of buf_breakup, and not programmed I/O.
		 * (e.g. HBAs that are still doing there own scatter-
		 * gather lists.)
		 */
		dk->dk_bcbp_max = sdi_getbcb(&dk->dk_addr, KM_SLEEP);
		dk->dk_bcbp_max->bcb_granularity = BLKSZ(dk);
		dk->dk_bcbp_max->bcb_physreqp->phys_align = BLKSZ(dk);
		dk->dk_bcbp_max->bcb_max_xfer = 
		   HIP(HBA_tbl[SDI_EXHAN(&dk->dk_addr)].info)->max_xfer;

		dk->dk_bcbp->bcb_physreqp->phys_align = BLKSZ(dk);
		dk->dk_bcbp->bcb_max_xfer = dk->dk_bcbp_max->bcb_max_xfer
		   - ptob(1);

		(void)physreq_prep(dk->dk_bcbp->bcb_physreqp, KM_SLEEP);
		(void)physreq_prep(dk->dk_bcbp_max->bcb_physreqp, KM_SLEEP);
	}
#endif

	/* Indicate parameters are set and valid */
	dk->dk_state |= DKPARMS;

#ifdef SD01_DEBUG
	if(sd01alts_debug & DKADDR) {
        	cmn_err(CE_CONT,"sd01config: sec/trk=0x%x secsiz=0x%x heads=0x%x cyls=0x%x ", 
		dk->dk_parms.dp_sectors, dk->dk_parms.dp_secsiz, 
		dk->dk_parms.dp_heads, dk->dk_parms.dp_cyls);
	}
#endif

#ifdef DEBUG
        DPR (3)("sec=0x%x siz=0x%x heads=0x%x cyls=0x%x\n", dk->dk_parms.dp_sectors, dk->dk_parms.dp_secsiz, dk->dk_parms.dp_heads, dk->dk_parms.dp_cyls);
#endif
	
#ifdef DEBUG
        DPR (2)("sd01config: parms set - exit(0)\n");
#endif
	return(0);
}


/*
 * int
 * sd01size(dev_t dev)
 *	Returns the number of 512-byte units on a partition.
 * Called by: Kernel
 * Side Effects: None
 * Returns: Number of 512-byte units, or -1 for failure.
 *
 * Calling/Exit State:
 *	None
 */
int
sd01size(dev_t dev)
{
	struct disk *dk;
	int slice;
	minor_t minor;

	minor = geteminor(dev);

	if ((dk = DKPTR(minor)) == NULL)
		return (-1);

	slice = DKSLICE(minor);
	if (slice >= V_NUMPAR && slice < DK_XNUMPART)	{
		slice -= V_NUMPAR;

		/* check for an unused or unopened partition */
		if (dk->dk_fdisk[slice].p_type == UNUSED ||
					dk->dk_fdisk_flag[slice] == DKFREE)
			return -1;

		return dk->dk_fdisk[slice].p_size << BLKSEC(dk);
	}

	if ((slice = DKSLICE(minor)) < 0 || slice >= V_NUMPAR ||
	!dk->dk_part_flag[slice] || dk->dk_vtoc.v_sanity != VTOC_SANE ) {
		return -1;
	}
	return dk->dk_vtoc.v_part[DKSLICE(minor)].p_size << BLKSEC(dk);
}

/*
 * void
 * sd01flush_job(struct disk *dk)
 *	Flush all jobs in the disk queue
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01flush_job(struct disk *dk)
{
	struct	job *jp;
	struct	job *jp_next;


	SD01_DK_LOCK(dk);

	jp = dk->dk_queue;
	dk->dk_queue = dk->dk_lastlow = dk->dk_lasthi = (struct job *)NULL;

	SD01_DK_UNLOCK(dk);

	while (jp) {
		jp_next = jp->j_next;
		jp->j_next = (struct job *)NULL;
		jp->j_cont->SCB.sc_comp_code = SDI_QFLUSH;
		sd01done(jp);
		jp = jp_next;
	}
}

/*
 * void
 * sd01ck_badsec(buf_t *bp)
 *	Check for any bad sector for the disk request.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 */
void
sd01ck_badsec(buf_t *bp)
{
	struct	disk *dk;
	int	part;
	minor_t minor;

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Entering sd01ck_badsec\n");
	}
#endif
	minor = geteminor(bp->b_edev);
	dk = DKPTR(minor);
	ASSERT(dk);
	part = DKSLICE(minor);

	/* Lightweight check before loop.  If the lightweight check
	 * says DKMAPBAD we wait for the SV_BROADCAST.  If the lightweight
	 * check fails, then the locked check will be equally likely
	 * to fail.  We are not really synchronizing anything here.
	 */
	if (!sd01_lightweight || dk->dk_state & DKMAPBAD) {
		/* wait until dynamic bad sector mapping has been completed */
		SD01_DK_LOCK(dk);
		while(dk->dk_state & DKMAPBAD)	{
			SV_WAIT(dk->dk_sv, pridisk, dk->dk_lock);
			SD01_DK_LOCK(dk);
		}
		SD01_DK_UNLOCK(dk);
	}

	/* keep start time */
	drv_getparm(LBOLT, &(bp->b_start));

	sd01gen_sjq(dk, part, bp, SD01_NORMAL, NULL);

#ifdef SD01_DEBUG
	if(sd01alts_debug & STFIN) {
		cmn_err(CE_CONT,"Leaving sd01ck_badsec\n");
	}
#endif
}

/*
 * int
 * sd01gen_sjq(struct disk *dk, int part, buf_t *bp, int  flag,
 *	struct job *prevjp)
 *		Generate the scsi job queue
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	Acquires the disk struct's dk_lock.
 *
 *	Returns 0 on successful job generation.
 *	        1 on failure.  The only failure is a result of no
 *		  memory for allocating job structures, so if the call
 *		  is not SD01_IMMED, then the routine will not fail.
 */
int
sd01gen_sjq(struct disk *dk, int part, buf_t *bp, int  flag, struct job *prevjp)
{
	register struct	job *diskhead = NULL;
	register struct	job *jp, *nextjp;
	int	ac, sleepflag;

	sleepflag = (flag & SD01_IMMED) ? KM_NOSLEEP:KM_SLEEP;

	/* generate a scsi disk job */
	if ((diskhead = (struct job *)sd01getjob(dk, sleepflag)) == NULL)
		return (1);
	diskhead->j_seccnt = bp->b_bcount >> BLKSHF(dk);
	set_sjq_memaddr(diskhead,paddr(bp));
	set_sjq_daddr(diskhead,sd01_blk2sec(bp->b_blkno,dk,part));

#ifdef SD01_DEBUG
	if(sd01alts_debug & DXALT) {
		cmn_err(CE_CONT,
		"sd01gen_sjq: diskhead: part= %d seccnt= 0x%x memaddr= 0x%x daddr= 0x%x\n",
		part,diskhead->j_seccnt,sjq_memaddr(diskhead), 
		sjq_daddr(diskhead));
		cmn_err(CE_CONT,
		"buf: bcount= 0x%x paddr(bp)= 0x%x blkno= 0x%x\n",
		bp->b_bcount, paddr(bp), bp->b_blkno);
	}
#endif

	/* If we are are not doing lightweight checking or
	 * we have remapped a block, check for a bad sector.
	 *
	 * Lightweight case: No synchronization on dk_altcount[part].  
	 * We must be prepared to handle the case that we 
	 * discover a new bad block and then immediately 
	 * send another job to that block.
	 */
	if (part < V_NUMPAR && (!sd01_lightweight || dk->dk_altcount[part])) {
 
		/* check for bad sector in this disk request */

		SD01_DK_LOCK(dk);
		ac = dk->dk_altcount[part];
		SD01_DK_UNLOCK(dk);

		if (ac)
			if (sd01alt_badsec(dk, diskhead, part, sleepflag)) {
				for (jp=diskhead; jp; jp=jp->j_fwmate) {
					sd01freejob(jp);
				}
				return (1);
			}
	}

	/* SD01_IMMED and jp indicate we're resending a write with a	*/
	/* newly remapped/reassigned bad sector. We attach the old	*/
	/* existing job structs so a biodone is done only once		*/
	/* only once when all the old and new jobs are completed	*/

	if ((flag & SD01_IMMED) && (prevjp)) {
		prevjp->j_fwmate = diskhead;
		diskhead->j_bkmate = prevjp;
	}

	for (jp=diskhead; jp; jp=nextjp) {
		nextjp = jp->j_fwmate;
		if (flag & SD01_IMMED)
			jp->j_cont->sb_type = ISCB_TYPE;	
		if (flag & SD01_FIFO)
			jp->j_flags |= J_FIFO;
		sd01strat1(jp,dk,bp);
	}
	return (0);
}

#ifndef PDI_SVR42
/*
 * int
 * sd01devinfo(dev_t dev, di_parm_t parm, bcb_t *bcbp)
 *	Get device information.
 *	
 * Calling/Exit State:
 *	The device must already be open.
 */
int
sd01devinfo(dev_t dev, di_parm_t parm, void **valp)
{
	struct disk *dk;
	minor_t minor;

	minor = geteminor(dev);

	if ((dk = DKPTR(minor)) == NULL)
		return ENODEV;

	switch (parm) {
		case DI_BCBP:
			*(bcb_t **)valp = dk->dk_bcbp;
			return 0;
		case DI_MEDIA:
			if (dk->dk_iotype & F_RMB)
				*(char **)valp = "disk";
			else
				*(char **)valp = NULL;
			return 0;
		default:
			return ENOSYS;
	}
}
#endif

#ifdef DEBUG
/*
 * void
 * sd01prt_jq(struct disk *dk)
 *
 * Calling/Exit State:
 *	None
 */
void
sd01prt_jq(struct disk *dk)
{
	struct	job *jp;
	struct 	scm *cmd;
	int	i;

	if (dk == (struct disk *)NULL)
		dk = sd01_dp[0];

	cmn_err(CE_CONT,"sd01print_job dk=0x%x\n", dk);
	cmn_err(CE_CONT,"HIGH PRIORITY QUEUE\n");
	jp = dk->dk_queue;
	for (i=0; jp!= dk->dk_lasthi; jp=jp->j_next, i++) {

		cmd = &jp->j_cmd.cm;

		if (jp->j_cont->sb_type == SCB_TYPE) {
			cmn_err(CE_CONT,"JBQ[%d]0x%x: op=%s sec=0x%x cnt=%d mem=0x%x\n",
				i,jp,(cmd->sm_op==SM_READ)?"Rd":"Wr",
				cmd->sm_addr, cmd->sm_len,
				jp->j_cont->SCB.sc_datapt);
		}
	}
	cmn_err(CE_CONT,"LOW PRIORITY QUEUE\n");
	for (i=0; jp; jp=jp->j_next, i++) {

		cmd = &jp->j_cmd.cm;

		if (jp->j_cont->sb_type == SCB_TYPE) {	
			cmn_err(CE_CONT,"JBQ[%d]0x%x: op=%s sec=0x%x cnt=%d mem=0x%x\n",
				i,jp,(cmd->sm_op==SM_READ)?"Rd":"Wr", 
				cmd->sm_addr, cmd->sm_len, 
				jp->j_cont->SCB.sc_datapt);
		}
	}
}

/*
 * void
 * sd01prt_job(struct job *jp)
 *
 * Calling/Exit State:
 *	None
 */
void
sd01prt_job(struct job *jp)
{
	cmn_err(CE_CONT,"JOB jp= 0x%x j_cont= 0x%x j_done= 0x%x bp=0x%x dk= 0x%x\n",
		jp, jp->j_cont, jp->j_done, jp->j_bp, jp->j_dk);
	cmn_err(CE_CONT,"fwmate= 0x%x bkmate= 0x%x daddr=0x%x memaddr= 0x%x seccnt= 0x%x\n",
		jp->j_fwmate, jp->j_bkmate, jp->j_daddr,
		jp->j_memaddr, jp->j_seccnt);
}

/*
 * void
 * sd01prt_dsk(int idx)
 *
 * Calling/Exit State:
 *	None
 */
void
sd01prt_dsk(int idx)
{
	struct	disk	*dp;

	dp = sd01_dp[idx];

	cmn_err(CE_CONT,
		"DISK dp= 0x%x dk_state= 0x%x dk_addr.sa_ct= 0x%x"
		" dk_addr.sa_lun= 0x%x\n",
		dp, dp->dk_state,
		dp->dk_addr.sa_ct, dp->dk_addr.sa_lun);
}
#endif /* DEBUG */


int
sd01fdisk(struct ipart *table, struct disk *dk)
{
	int	i, j, s, n;
	int	o[FD_NUMPART];
	struct ipart	*p;


	for (i = 0, s = 0; i < FD_NUMPART; i++)	{
		if (table[i].systid == UNUSED ||
				(table[i].bootid == NOTACTIVE &&
					table[i].beghead == 0 &&
					table[i].begsect == 0))
			continue;

		/* anything sorted yet */
		if ((n = s) > 0)	{

			/* if so, find spot for new entry */
			for (j = s - 1; j >= 0; j--)	{
				if (table[i].relsect >= table[o[j]].relsect)
					break;

				n = j;
			}

			/* shift down entries below this one */
			for (j = s; j > n; j--)
				o[j] = o[j - 1];
		}

		o[n] = i;
		s++;
	}

	/*
	 * first set up entry 0 to access the whole disk
	 */
	dk->dk_fdisk[0].p_start = 0;
	dk->dk_fdisk[0].p_size = DKSIZE(dk);
	dk->dk_fdisk[0].p_type = DK_WHOLE_DISK_TYPE;

	/*
	 * and now entries 1-4 for the sorted fdisk partitions, pointed to
	 * by o[0]-o[3]
	 */
	for (i = 1; i <= s; i++)	{
		p = &table[o[i - 1]];
		dk->dk_fdisk[i].p_start = p->relsect;
		dk->dk_fdisk[i].p_size = p->numsect;
		dk->dk_fdisk[i].p_type = p->systid & 0xFF;

		/* a little trick to keep track of the active partition */
		dk->dk_fdisk[i].p_tag = p->bootid & 0xFF;
	}
	for (; i < FD_NUMPART + 1; i++)	{
		dk->dk_fdisk[i].p_type = UNUSED;
		dk->dk_fdisk[i].p_tag = NOTACTIVE;
	}

#ifdef DEBUG
	cmn_err(CE_CONT,"DISK sa_ct= 0x%x sa_lun= 0x%x\n", dk->dk_addr.sa_ct, dk->dk_addr.sa_lun);
	cmn_err(CE_CONT, "valid fdisk partitions= %d\n", s);
	for (i = 0; i < FD_NUMPART + 1; i++)	{
		struct xpartition	*x = &dk->dk_fdisk[i];
		cmn_err(CE_CONT,
			"%d: p_tag= 0x%x p_type= 0x%x p_start= 0x%x p_size= 0x%x\n",
			i, x->p_tag, x->p_type, x->p_start, x->p_size);
	}
#endif

	return s + 1;
}

/*
 * int
 * sd01_first_open()
 * 
 * this routine is called the first time a disk is opened in any
 * of its slices or partitions.
 *
 * return SDI_RET_OK, SDI_RET_ERR
 */
int
sd01_first_open(struct disk *dk)
{
	int ret=SDI_RET_OK;
	struct sense *sd01_sense;
	struct scsi_adr	scsi_adr;

	sd01_sense = (struct sense *)kmem_alloc(sizeof(struct sense),
					       sdi_sleepflag);
	if (sd01_sense == NULL)
	{
		cmn_err(CE_WARN, "disk driver: Unable to "
			"allocate memory to open disk.\n");
		return SDI_RET_ERR;
	}


	if (((dk->dk_ident.id_ver & 0x07) > (unsigned)1) && 
	    sd01cmd(dk, SS_LOAD, 0, NULL, 0, 1, SCB_READ, FALSE))
	{
		
		/* A REQUEST SENSE to clear any possible 
		 * Unit Attentions or such */
		(void)sd01cmd(dk, SS_REQSEN, 0, (char *)sd01_sense, 
			SENSE_SZ, SENSE_SZ, SCB_READ, TRUE);

		if (sd01cmd(dk, SS_LOAD, 0, NULL, 0, 1, SCB_READ, FALSE))
		{
			/*EMPTY*/
#if SD01_DEBUG

			cmn_err(CE_WARN, "!Disk Driver: Couldn't spin up "
				"disk for open %d: %d,%d,%d\n",
				SDI_EXHAN(dk->scsi_ad), SDI_BUS(dk->scsi_ad),
				SDI_EXTCN(dk->scsi_ad), SDI_EXLUN(dk->scsi_ad));
#endif
		}
	}
	kmem_free(sd01_sense, sizeof(struct sense));

	if(dk->dk_iotype & F_RMB) {
		if (sd01cmd(dk, SS_LOCK, 0, NULL, 0, 1, SCB_READ, TRUE))
		{
			/*EMPTY*/
#if SD01_DEBUG
			cmn_err(CE_WARN, "!Disk Driver: Couldn't lock drive"
				" %d: %d,%d,%d\n",
				SDI_EXHAN(dk->scsi_ad), SDI_BUS(dk->scsi_ad),
				SDI_EXTCN(dk->scsi_ad), SDI_EXLUN(dk->scsi_ad));
#endif
		}
	}

	/*
	 * This is here for Compaq ProLiant Storage System support.  It could
	 * be ifdef'ed for performance
	 */
	scsi_adr.scsi_ctl = SDI_EXHAN(&dk->dk_addr);
	scsi_adr.scsi_target = SDI_EXTCN(&dk->dk_addr);
	scsi_adr.scsi_lun = SDI_EXLUN(&dk->dk_addr);
	scsi_adr.scsi_bus = SDI_BUS(&dk->dk_addr);
	sdi_notifyevent(SDI_FIRSTOPEN, &scsi_adr, (struct sb *)NULL);

	return ret;
}

/*
 * int
 * sd01_last_close()
 *
 * this routine is called when all slices have partitions on a disk have
 * been closed.
 *
 * return SDI_RET_OK, SDI_RET_ERR
 */
int sd01_last_close(struct disk *dk, int err)
{
	int ret=SDI_RET_OK;
	struct scsi_adr	scsi_adr;

	if (sd01_sync && (((dk->dk_ident.id_ver & 0x07) <= (unsigned)1) ||
	    sd01cmd(dk, SM_SYNC, 0, NULL, 0, 0, SCB_READ, FALSE)))
	{
		/*EMPTY*/
#if SD01_DEBUG
		cmn_err(CE_WARN, "!Disk Driver: Couldn't spin up disk for open"
			" %d: %d,%d,%d\n",
			SDI_EXHAN(dk->scsi_ad), SDI_BUS(dk->scsi_ad),
			SDI_EXTCN(dk->scsi_ad), SDI_EXLUN(dk->scsi_ad));
#endif
	}

	if(dk->dk_iotype & F_RMB) {
		/* Unlock/Unreserve the the drive so that media can be removed */
		if (sd01cmd(dk, SS_LOCK, 0, NULL, 0, 0, SCB_READ, TRUE))
		{
			/*EMPTY*/
#if SD01_DEBUG
			cmn_err(CE_WARN, "!Disk Driver: Couldn't lock drive"
				" %d: %d,%d,%d\n",
				SDI_EXHAN(dk->scsi_ad), SDI_BUS(dk->scsi_ad),
				SDI_EXTCN(dk->scsi_ad), SDI_EXLUN(dk->scsi_ad));
#endif
		}
	}

	/*
	 * This is here for Compaq ProLiant Storage System support.  It could
	 * be ifdef'ed out for performance
	 */
	scsi_adr.scsi_ctl = SDI_EXHAN(&dk->dk_addr);
	scsi_adr.scsi_target = SDI_EXTCN(&dk->dk_addr);
	scsi_adr.scsi_lun = SDI_EXLUN(&dk->dk_addr);
	scsi_adr.scsi_bus = SDI_BUS(&dk->dk_addr);
	sdi_notifyevent(err ? SDI_LASTCLOSE_ERR : SDI_LASTCLOSE,
						&scsi_adr, (struct sb *)NULL);

	return ret;
}

/*
 * Returns the minor number for the specified disk
 * A minor number of 0 is returned if the disk can not be
 * determined.
 * NOTE: This will not return the minor numbers
 * 0..DK_MAX_SLICE-1 which are the minor numbers for the
 * sd01 boot disk clone device
 */
minor_t
sd01_disk_to_minor(int unit)
{
    int c, b, t, l, edtc;
    struct sdi_edt *edtp;
    int count=-1;        /* we have found count disks so far */
    
    for (c = 0; c < MAX_EXHAS; c++)
    {
        edtc = sdi_edtindex(c);
        if (edtc < 0) continue;

        for (b = 0; b < MAX_BUS; b++)
        {
            for (t = 0; t < MAX_EXTCS; t++)
            {
                for (l = 0; l < MAX_EXLUS; l++)
                {
                    edtp = sdi_rxedt(edtc, b, t, l);
                    if (edtp == NULL) continue;
                    
                    if (edtp->pdtype == ID_RANDOM)
                    {
                        if (edtp->curdrv == NULL)
                            continue;

                        count++;

                        if (count == unit)
                            return edtp->curdrv->maj.first_minor;
                    }
                }
            }
        }
    }
    return 0;
}


/*
 * Returns the unit number for the disk with the passed minor number
 * A unit number of -1 is returned if the disk can not be
 * determined.
 * NOTE: This will not find the unit number for minor numbers
 * 0..DK_MAX_SLICE-1 which are the minor numbers for the
 * sd01 boot disk clone device
 */
int
sd01_minor_to_disk(minor_t minor)
{
    int c, b, t, l, edtc;
    struct sdi_edt *edtp;
    int count=-1;        /* we have found count disks so far */
    
    minor = DKINDEX2MINOR(DKINDEX(minor)); /* gets the first minor */

    if ((!sd01_clone) && (DKINDEX(minor) == 0))
    {
	    /* DKINDEX 0 is a clone device of
	     * the boot disk
	     */
	    minor = sd01_disk_to_minor(0);
    }
    

    for (c = 0; c < MAX_EXHAS; c++)
    {
        edtc = sdi_edtindex(c);
        if (edtc < 0) continue;

        for (b = 0; b < MAX_BUS; b++)
        {
            for (t = 0; t < MAX_EXTCS; t++)
            {
                for (l = 0; l < MAX_EXLUS; l++)
                {
                    edtp = sdi_rxedt(edtc, b, t, l);
                    if (edtp == NULL) continue;
                    
                    if (edtp->pdtype == ID_RANDOM)
                    {
                        if (edtp->curdrv == NULL)
                            continue;

                        count++;

                        if (edtp->curdrv->maj.first_minor == minor)
                            return count;
                    }
                }
            }
        }
    }
    return -1;
}

/*
 * void
 * sd01timeout(struct job *)
 *
 * If the job is a read or write then 
 * sets the timeout value for the passed disk job.
 * If this HBA is know to not support timeouts then
 * this means putting the job into the appropriate timeout
 * bucket.  Otherwise simpl sets sc_time.
 */
void
sd01timeout(struct job *j)
{
	int op;

	if (j == NULL || !(j->j_dk->dk_state & (DKTIMEOUT|DKINSANE)))
		return;

	j->j_timeout_next = j->j_timeout_prev = NULL;

	if (j->j_cont->sb_type != SCB_TYPE && j->j_cont->sb_type != ISCB_TYPE)
		return;

	op = ((char *)(j->j_cont->SCB.sc_cmdpt))[0];

	if (op != SS_READ && op != SS_WRITE && op != SM_READ && op != SM_WRITE)
		return;

	SD01_DK_LOCK(j->j_dk);

	j->j_timeout = j->j_dk->dk_time;
	j->j_timeout_next = j->j_dk->dk_timeout_list[j->j_timeout];
	j->j_dk->dk_timeout_list[j->j_timeout] = j;
	if (j->j_timeout_next)
		j->j_timeout_next->j_timeout_prev = j;

	SD01_DK_UNLOCK(j->j_dk);
}

/*
 * void
 * sd01untimeout(struct job * j)
 *
 * removes, if necessary the passed job from the timeout
 * bucket array.
 */
void
sd01untimeout(struct job *j, int status)
{
	struct disk *dk;
	if (j == NULL || (j->j_flags & J_TIMEOUT)) return;
	dk = j->j_dk;

	if (!(dk->dk_state & (DKTIMEOUT|DKINSANE)))
		return;
	SD01_DK_LOCK(dk);

	if (status == SDI_ASW) {
		dk->dk_last_asw = dk->dk_time;
	}
	if (j->j_timeout_next)
		j->j_timeout_next->j_timeout_prev = j->j_timeout_prev;

	if (j->j_timeout_prev)
		j->j_timeout_prev->j_timeout_next = j->j_timeout_next;

	if (j->j_timeout == -1) {
		/* Timeout on dk_oldjobs list */
		if (dk->dk_oldjobs == j)
			dk->dk_oldjobs = j->j_timeout_next;
	}
	else if (dk->dk_timeout_list[j->j_timeout] == j)
		dk->dk_timeout_list[j->j_timeout] = j->j_timeout_next;
	SD01_DK_UNLOCK(dk);
}

void
sd01watchdog(int d)
{
	struct disk *dk;
	struct job *j, *savej, *oj;
	int i, previous, acquired;

	SD01_DP_TRYLOCK(acquired);
	/* don't wait for the lock */
	if (!acquired)  
		return;

	if ((dk = sd01_dp[d]) == NULL) {
		SD01_DP_UNLOCK();
		return;
	}

	SD01_DK_LOCK(dk);

	if (dk->dk_state & DKSUSP) {
		SD01_DK_UNLOCK(dk);
		SD01_DP_UNLOCK();
		return;
	}

	previous = dk->dk_time;
	dk->dk_time = (previous + 1) % sd01_timeslices;

	if (dk->dk_last_asw != previous && (dk->dk_state & DKINSANE)) {

		/* Have not seen a successful completion in sd01_insane seconds.
		 * Timeout all jobs in any bucket except previous.
		 */

		for (i=0; i<sd01_timeslices; i++) {
			if (i == previous)
				continue;
			j = dk->dk_timeout_snapshot[i] = dk->dk_timeout_list[i];
			dk->dk_timeout_list[i] = NULL;
			while (j) {
				j->j_flags |= J_TIMEOUT;
				j = j->j_timeout_next;
			}
		}
		oj = j = dk->dk_oldjobs;
		dk->dk_oldjobs = NULL;
		while (j) {
			j->j_flags |= J_TIMEOUT;
			j = j->j_timeout_next;
		}
		SD01_DK_UNLOCK(dk);
		for (i=0; i<sd01_timeslices; i++) {
			if (i == previous)
				continue;
			sd01_dotimeout(dk->dk_timeout_snapshot[i], dk, 1);
		}
		sd01_dotimeout(oj, dk, 1);
		SD01_DP_UNLOCK();
		return;
	}

	/* We are here if we think the disk is still sane.  Check
	 * if any job has taken an extraordinary amount of time.
	 */
	savej = j = dk->dk_timeout_list[dk->dk_time];
	dk->dk_timeout_list[dk->dk_time] = NULL;

	if (dk->dk_state & DKTIMEOUT) {
		while (j) {
			j->j_flags |= J_TIMEOUT;
			j = j->j_timeout_next;
		}
		SD01_DK_UNLOCK(dk);
		sd01_dotimeout(savej, dk, 0);
	} else {
		/* Place jobs in dk_oldjobs */
		while (j) {
			j->j_timeout = -1;
			j = j->j_timeout_next;
			savej->j_timeout_next = dk->dk_oldjobs;
			if (dk->dk_oldjobs)
				dk->dk_oldjobs->j_timeout_prev = savej;
			dk->dk_oldjobs = savej;
			savej->j_timeout_prev = NULL;
			savej = j;
		}
		SD01_DK_UNLOCK(dk);
	}
	SD01_DP_UNLOCK();
	return;
}
	
STATIC void
sd01_dotimeout(struct job *j, struct disk *dk,  int type)
{
	struct job *next;

	while (j) {
		dk->dk_state |= DK_RDWR_ERR;
		j->j_error = SD01_RETRYCNT;	/* Do not retry */
		cmn_err(CE_WARN, "Disk Driver: Timeout(%d) job 0x%x", type, j);
		next = j->j_timeout_next;
		j->j_cont->SCB.sc_comp_code  = SDI_TIME;
		if (j->j_cont->SCB.sc_int)
			j->j_cont->SCB.sc_int(j->j_cont);
		j = next;
	} 
}
