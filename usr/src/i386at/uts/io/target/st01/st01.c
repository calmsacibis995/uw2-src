/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/target/st01/st01.c	1.86.1.1"
#ident	"$Header: $"

/*	Copyright (c) 1988, 1989  Intel Corporation	*/
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */

/*
**	SCSI Tape Target Driver.
*/

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/systm.h>
#include <fs/file.h>
#include <io/open.h>
#include <io/ioctl.h>
#include <util/debug.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/st01/tape.h>
#include <io/target/st01/st01.h>
#include <io/target/sdi/dynstructs.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define	DRVNAME		"st01 - tape target driver"

void 	st01io(), st01freejob(), st01sense(), st01rinit(), st01initdealloc();
int	st01cmd(), ST01ICMD(), st01config(), ST01SEND(), st01ioctl(),
	uiophysio(), st01docmd();
int	sdi_send(), sdi_icmd();

/* Allocated in space.c */
extern	long	 	St01_cmajor;	/* Character major number   */
extern	int		St01_reserve;	/* Flag for reserving tape on open */
extern  struct  head	lg_poolhead;

static	int 		st01_tapecnt;	/* Num of tapes configured  */
static	struct tape    *st01_tape;	/* Array of Tape structures */
static	int		rinit_flag = 0;	/* flag	to protect rinit func */
static  struct owner   *st01_ownerlist = NULL;	/* List of owner structs from sdi_doconfig */
static	int	st01_dynamic = 0;
static	size_t	mod_memsz = 0;

STATIC void st01resumeq();

int st01start();
void st01comp(), st01logerr();

STATIC	int	st01_load(), st01_unload();
MOD_DRV_WRAPPER(st01, st01_load, st01_unload, NULL, DRVNAME);
/*
 * st01_load() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 */
STATIC	int
st01_load()
{
	st01_dynamic = 1;
	if (st01start()) {
		sdi_clrconfig(st01_ownerlist, SDI_DISCLAIM|SDI_REMOVE, st01rinit);
		return(ENODEV);
	}
	return(0);
}

/*
 * st01_unload() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 */
STATIC	int
st01_unload()
{
	int i;
	struct tape *tp;

	sdi_clrconfig(st01_ownerlist, SDI_DISCLAIM|SDI_REMOVE, st01rinit);

	if(mod_memsz)	{
		for (i=0, tp = st01_tape; i<st01_tapecnt; i++, tp++) {
			sdi_freebcb(tp->t_bcbp);
		}
		st01_ownerlist = NULL;
		kmem_free((caddr_t)st01_tape, mod_memsz);
		st01_tape = NULL;
	}
	return(0);
}

/* Aliases - see scsi.h */
#define ss_code		ss_addr1
#define ss_mode		ss_addr1
#define ss_parm		ss_len
#define ss_len1		ss_addr
#define ss_len2		ss_len
#define ss_cnt1		ss_addr
#define ss_cnt2		ss_len

#define	GROUP0		0
#define	GROUP1		1
#define	GROUP6		6
#define	GROUP7		7

#define	msbyte(x)	(((x) >> 16) & 0xff)
#define	mdbyte(x)	(((x) >>  8) & 0xff)
#define	lsbyte(x)	((x) & 0xff)

#define SPL     	spldisk

void	st01intn(), st01intf(), st01intrq(), st01strategy(), st01strat0();
void	st01breakup();
int	st01reserve();
int	st01release();

#ifndef PDI_SVR42
int	st01devflag = D_TAPE | D_BLKOFF;
#else /* PDI_SVR42 */
int	st01devflag = D_TAPE;
#endif /* PDI_SVR42 */

/*
 * st01start() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Called by kernel to perform driver initialization.
 *	This function does not access any devices.
 */
int
st01start()
{
	register struct tape   *tp;	/* Tape pointer		 */
	struct	owner	*op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	int  tapesz,			/* Tape size (in bytes)  */
	     tc;			/* TC number		 */
	int sleepflag;

	drv_maj.b_maj = St01_cmajor;  /* st01 has no block major */
	drv_maj.c_maj = St01_cmajor;
	drv_maj.minors_per = TP_MINORS_PER;
	drv_maj.first_minor = 0;
	st01_ownerlist = sdi_doconfig(ST01_dev_cfg, ST01_dev_cfg_size,
				"ST01 Tape Driver", &drv_maj, st01rinit);
	st01_tapecnt = 0;
	for (op = st01_ownerlist; op; op = op->target_link) {
		st01_tapecnt++;
	}

#ifdef DEBUG
	cmn_err(CE_CONT, "%d tapes claimed\n", st01_tapecnt);
#endif

	/* Check if there are devices configured */
	if (st01_tapecnt == 0) {
		return(1);
	}

	/*
	 * Allocate the tape structures
	 */
	sleepflag = st01_dynamic ? KM_SLEEP : KM_NOSLEEP;
	tapesz = st01_tapecnt * sizeof(struct tape);
	mod_memsz = tapesz;
        if ((base = kmem_zalloc(mod_memsz, sleepflag)) == NULL)
	{
				/*
			  	 *+ display err msg
				 */
                cmn_err(CE_WARN,
                        "Tape driver: Initialization failure -- insufficient memory for driver");
				/*
			  	 *+ display err msg
				 */
                cmn_err(CE_CONT,
                        "-- tape driver disabled\n");
		st01_tapecnt = 0;
		mod_memsz = 0;
		return(1);
	}
	st01_tape = (struct tape *)(void *) base;

	/*
	 * Initialize the tape structures
	 */
	tp = st01_tape;
	for(tc = 0, op = st01_ownerlist; tc < st01_tapecnt;
			tc++, op=op->target_link, tp++) {

		/* Allocate the fault SBs */
		tp->t_fltreq = sdi_getblk(sleepflag);  /* Request sense */
		tp->t_fltres = sdi_getblk(sleepflag);  /* Resume */
		tp->t_fltrst = sdi_getblk(sleepflag);  /* Reset */

#ifdef DEBUG
	cmn_err(CE_CONT, "Tape: op 0x%x ", op);
	cmn_err(CE_CONT, "edt 0x%x ", op->edtp);
	cmn_err(CE_CONT, "hba %d scsi id %d lun %d bus %d\n",
		op->edtp->scsi_adr.scsi_ctl,op->edtp->scsi_adr.scsi_target,
		op->edtp->scsi_adr.scsi_lun,op->edtp->scsi_adr.scsi_bus);
#endif
		tp->t_addr.sa_ct  = SDI_SA_CT(op->edtp->scsi_adr.scsi_ctl, 
				      	op->edtp->scsi_adr.scsi_target);
		tp->t_addr.sa_exta= (uchar_t)(op->edtp->scsi_adr.scsi_target);
		tp->t_addr.sa_lun = op->edtp->scsi_adr.scsi_lun;
		tp->t_addr.sa_bus = op->edtp->scsi_adr.scsi_bus;
		tp->t_spec	    = sdi_findspec(op->edtp, st01_dev_spec);
		tp->t_iotype = op->edtp->iotype;

		/*
		 * Call to initialize the breakup control block.
	 	*/
		if ((tp->t_bcbp = sdi_getbcb(&tp->t_addr, sleepflag)) == NULL) {
			/*
			 *+ Insufficient memory to allocate tape breakup control
			 *+ block data structure when loading driver.
			 */
 			cmn_err(CE_NOTE, "Tape Driver: insufficient memory to allocate breakup control block.");

			st01initdealloc(st01_tape);
			kmem_free(st01_tape, mod_memsz);
			st01_tapecnt = 0;
			mod_memsz = 0;
			return (1);
		}
#ifndef PDI_SVR42
		tp->t_bcbp->bcb_flags |= BCB_SYNCHRONOUS;
#endif
	}
	return(0);
}

/*
 * void
 * st01initdealloc() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Called by kernel to perform driver initialization.
 *	This function does not access any devices.
 */
void
st01initdealloc(struct tape *tp)
{
	int tc;

	for (tc = 0; tc < st01_tapecnt; tc++, tp++) {
		/* Deallocate the fault SBs */
		if (tp->t_fltreq)
			sdi_freeblk(tp->t_fltreq);  /* Request sense */
		if (tp->t_fltres)
			sdi_freeblk(tp->t_fltres);  /* Resume */
		if (tp->t_fltrst)
			sdi_freeblk(tp->t_fltrst);  /* Reset */
	}
}

#ifndef PDI_SVR42
/*
 * int
 * st01devinfo(dev_t dev, di_parm_t parm, void **valp)
 *	Get device information.
 *	
 * Calling/Exit State:
 *	The device must already be open.
 */
int
st01devinfo(dev_t dev, di_parm_t parm, void **valp)
{
	struct tape	*tp;	/* Tape pointer */
	unsigned long	unit;

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	/* Check for non-existent device */
	if (unit >= st01_tapecnt)
		return(ENODEV);

	switch (parm) {
		case DI_BCBP:
			*(bcb_t **)valp = tp->t_bcbp;
			return 0;

		case DI_MEDIA:
			*(char **)valp = "tape";
			return 0;

		default:
			return ENOSYS;
	}
}
#endif

/*
 * void st01rinit() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Called by sdi to perform driver initialization of additional
 *	devices found after the dynamic load of HBA drivers. This 
 *	routine is called only when st01 is a static driver.
 *	This function does not access any devices.
 */
void
st01rinit()
{
	register struct tape  *tp, *otp;	/* tape pointer	 */
	struct	owner	*nohp, *op;
	struct drv_majors drv_maj;
	caddr_t	 base;			/* Base memory pointer	 */
	pl_t prevpl;			/* prev process level for splx */
	int  tapesz,			/* tape size (in bytes) */
	     new_tapecnt,		/* number of additional devs found*/
	     otapecnt,			/* number of devs previously found*/
	     tmpcnt,			/* temp count of devs */
	     found,			/* search flag */
	     tapecnt,			/* tape instance	*/
	     sleepflag;			/* KMA sleep/nosleep	*/

	/* set rinit_flag to prevent routines from accessing st01_tape */
	/* while its being copied to bigger array.		       */
	rinit_flag = 1;
	new_tapecnt= 0;
	drv_maj.b_maj = St01_cmajor;  /* st01 has no block major */
	drv_maj.c_maj = St01_cmajor;
	drv_maj.minors_per = TP_MINORS_PER;
	drv_maj.first_minor = 0;
	/* call sdi_doconfig with NULL func so we don't get called again */
	nohp = sdi_doconfig(ST01_dev_cfg, ST01_dev_cfg_size,
				"ST01 CDROM Driver", &drv_maj, NULL);

	for (op = nohp; op; op = op->target_link) {
		new_tapecnt++;
	}
#ifdef DEBUG
	cmn_err(CE_CONT, "st01rinit %d tapes claimed\n", new_tapecnt);
#endif
	/* Check if there are additional devices configured */
	if (new_tapecnt == st01_tapecnt) {
		rinit_flag = 0;
		wakeup((caddr_t)&rinit_flag);
		return;
	}
	/*
	 * Allocate the tape structures
	 */
	sleepflag = st01_dynamic ? KM_SLEEP : KM_NOSLEEP;
	tapesz = new_tapecnt * sizeof(struct tape);
        if ((base = kmem_zalloc(tapesz, sleepflag)) == NULL) {
		/*
	  	 *+ display err msg
		 */
		cmn_err(CE_WARN,
			"CD-ROM Error: Insufficient memory to configure driver");
		/*
	  	 *+ display err msg
		 */
		cmn_err(CE_CONT,
			"!Could not allocate 0x%x bytes of memory\n",tapesz);
		rinit_flag = 0;
		wakeup((caddr_t)&rinit_flag);
		return;
	}
	/*
	 * Initialize the tape structures
	 */
	otapecnt = st01_tapecnt;
	found = 0;
	prevpl = SPL();
	for(tp = (struct tape *)(void *)base, tapecnt = 0, op = nohp; 
	    tapecnt < new_tapecnt;
	    tapecnt++, op=op->target_link, tp++) {

		/* Initialize new tape structs by copying existing tape */
		/* structs into new struct and initializing new instances */
		if (otapecnt) {
			for (otp = st01_tape, tmpcnt = 0; 
			     tmpcnt < st01_tapecnt; otp++,tmpcnt++) {
				if (SDI_ADDRCMP(&otp->t_addr, 
						&op->edtp->scsi_adr)) {
					found = 1;
					break;
				}
			}
			if (found) { /* copy otp to tp */
				*tp = *otp;
				found = 0;
				otapecnt--;
				continue;
			}
		}
		/* Its a new tape device so init tape struct */
		/* Allocate the fault SBs */
		tp->t_fltreq = sdi_getblk(sleepflag);  /* Request sense */
		tp->t_fltres = sdi_getblk(sleepflag);  /* Resume */
		tp->t_fltrst = sdi_getblk(sleepflag);  /* Reset */
#ifdef DEBUG
	cmn_err(CE_CONT, "tape: op 0x%x edt 0x%x ", op,op->edtp);
	cmn_err(CE_CONT, "hba %d scsi id %d lun %d bus %d\n",
		op->edtp->scsi_adr.scsi_ctl,op->edtp->scsi_adr.scsi_target,
		op->edtp->scsi_adr.scsi_lun,op->edtp->scsi_adr.scsi_bus);
#endif
		tp->t_addr.sa_ct  = SDI_SA_CT(op->edtp->scsi_adr.scsi_ctl, 
				      	op->edtp->scsi_adr.scsi_target);
		tp->t_addr.sa_exta= (uchar_t)(op->edtp->scsi_adr.scsi_target);
		tp->t_addr.sa_lun = op->edtp->scsi_adr.scsi_lun;
		tp->t_addr.sa_bus = op->edtp->scsi_adr.scsi_bus;
		tp->t_spec = sdi_findspec(op->edtp, st01_dev_spec);
		tp->t_iotype = op->edtp->iotype;

		/*
		 * Call to initialize the breakup control block.
	 	*/
		if ((tp->t_bcbp = sdi_getbcb(&tp->t_addr, sleepflag)) == NULL) {
			/*
			 *+ Insufficient memory to allocate tape breakup control
			 *+ block data structure when loading driver.
			 */
 			cmn_err(CE_NOTE, "Tape Driver: insufficient memory to allocate breakup control block.");
			if (tp->t_fltreq)
				sdi_freeblk(tp->t_fltreq);  /* Request sense */
			if (tp->t_fltres)
				sdi_freeblk(tp->t_fltres);  /* Resume */
			if (tp->t_fltrst)
				sdi_freeblk(tp->t_fltrst);  /* Reset */
			kmem_free(base, tapesz);
			splx(prevpl);
			rinit_flag = 0;
			wakeup((caddr_t)&rinit_flag);
			return;
		}
#ifndef PDI_SVR42
		tp->t_bcbp->bcb_flags |= BCB_SYNCHRONOUS;
#endif
	}
	/* Free up previously allocated space, if any allocated */
	if (st01_tapecnt > 0)
		kmem_free(st01_tape,mod_memsz);
	st01_tapecnt = new_tapecnt;
	st01_tape = (struct tape *)(void *)base;
	mod_memsz = tapesz;
	st01_ownerlist = nohp;
	splx(prevpl);
	rinit_flag = 0;
	wakeup((caddr_t)&rinit_flag);
}

/*
 * st01getjob() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function will allocate a tape job structure from the free
 *	list.  The function will sleep if there are no jobs available.
 *	It will then get a SCSI block from SDI.
 */
struct job *
st01getjob()
{
	register struct job *jp;

	jp = (struct job *)sdi_get(&lg_poolhead, 0);

	/* Get an SB for this job */
	jp->j_sb = sdi_getblk(KM_SLEEP);
	return(jp);
}

/*
 * st01freejob() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function returns a job structure to the free list. The
 *	SCSI block associated with the job is returned to SDI.
 */
void
st01freejob(jp)
register struct job *jp;
{
	sdi_freeblk(jp->j_sb);
	sdi_free(&lg_poolhead, (jpool_t *)jp);
}

#define ST01IOCTL(cmd, arg) st01ioctl(dev, cmd, (caddr_t) arg, oflag, cred_p, (int*)0)

/*
 * abort_open()
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Clean up flags, so a failed open call can be aborted.
 *	Clears T_OPENING flag, and wakes up anyone sleeping
 *	for the current unit.
 */
void
abort_open(tp)
struct tape *tp;
{
    (void) st01release(tp);
    tp->t_state &= ~T_OPENING;
    (void) wakeup((caddr_t) &tp->t_state);
}

/*
 * st01open() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 * 	Driver open() entry point.  Opens the device and reserves
 *	it for use by that process only.
 */
/* ARGSUSED */
st01open(devp, oflag, otyp, cred_p)
dev_t	*devp;
cred_t	*cred_p;
int oflag, otyp;
{
	dev_t		dev = *devp;
	register struct tape *tp;
	unsigned	unit;
	pl_t		oldpri; /* save processor priority level*/
	int		error; /* save rval of subfunctions */

	if (oflag & FAPPEND)
		return(ENXIO);

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	/* Check for non-existent device */
	if (unit >= st01_tapecnt)
		return(ENXIO);

	/*
	 * Allow single open. If there is an outstanding aborted
	 * command, hold up the open unil the command terminates.
	 */
	oldpri = SPL();
#ifdef PDI_SVR42
        while (tp->t_state & T_OPENING)
			sleep((caddr_t) &tp->t_state, PRIBIO);
#else
	while (tp->t_state & (T_OPENING|T_ABORTED)) {
		if( sleep((caddr_t) &tp->t_state, PCATCH|(PZERO+1)) == 1 ) {
			splx(oldpri);
			return(EINTR);
		}
	}
#endif
	tp->t_state |= T_OPENING;
	splx(oldpri);

	if (tp->t_state & T_OPENED) {
		tp->t_state &= ~T_OPENING;
		return(EBUSY);
	}

	tp->t_state &= (T_PARMS | T_OPENING);
		/* clear all bits except T_PARMS and T_OPENING */

	tp->t_addr.sa_major = getemajor(dev);
	tp->t_addr.sa_minor = geteminor(dev);

	/* Initialize the fault SBs */
	tp->t_fltcnt = 0;
	tp->t_fltres->SFB.sf_dev    = tp->t_addr;
	tp->t_fltrst->SFB.sf_dev    = tp->t_addr;

	tp->t_fltreq->sb_type = ISCB_TYPE;
	tp->t_fltreq->SCB.sc_datapt = SENSE_AD(&tp->t_sense);
	tp->t_fltreq->SCB.sc_datasz = SENSE_SZ;
	tp->t_fltreq->SCB.sc_mode   = SCB_READ;
	tp->t_fltreq->SCB.sc_cmdpt  = SCS_AD(&tp->t_fltcmd);
	tp->t_fltreq->SCB.sc_dev    = tp->t_addr;
	sdi_translate(tp->t_fltreq, B_READ, 0, KM_SLEEP);

	if (St01_reserve) {
		if (error=st01reserve(tp)) {
			abort_open(tp);
			return(error);
		}
	}

	if (((tp->t_state & T_PARMS) == 0) && (error=st01config(tp))) {
		abort_open(tp);
		return(error);
	}

	/***
	** Return an access error if tape is
	** write protected, and user wants to write.
	***/
	if (tp->t_mode.md_wp && (oflag & FWRITE)) {
		tp->t_state &= ~T_PARMS;
		abort_open(tp);
		return(EACCES);
	}

	if (RETENSION_ON_OPEN(dev))
		ST01IOCTL(T_RETENSION, 0);
	tp->t_state |= T_OPENED;
	tp->t_state &= ~T_OPENING;
	(void) wakeup((caddr_t) &tp->t_state);
	return(0);
}

/*
 * st01close() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 * 	Driver close() entry point.  If the device has been opened
 *	for writing, a file mark will be written.  If the device
 *	has been opened to rewind on close, a rewind will be
 *	performed; otherwise the tape head will be positioned after
 *	the first filemark.
 */
/* ARGSUSED */
st01close(dev, oflag, otyp, cred_p)
cred_t	*cred_p;
register dev_t dev;
int oflag, otyp;
{
	register struct tape *tp;
	unsigned unit;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	if (NOREWIND(dev)) {
		/* move past next file mark */
		if ((tp->t_state & T_READ)
		&& !(tp->t_state & (T_FILEMARK | T_TAPEEND)))
			ST01IOCTL(T_SFF, 1);
		/* write filemark */
		if (tp->t_state & T_WRITTEN)
			ST01IOCTL(T_WRFILEM, 1);
	} else {
		/* rewind the tape */
		ST01IOCTL(T_RWD, 0);
		tp->t_state &= ~T_PARMS;
	}

	if (DOUNLOAD(dev)) {
		ST01IOCTL(T_UNLOAD,0);
		tp->t_state &= ~T_PARMS;
	}

	tp->t_state &= ~T_OPENED;
	if (St01_reserve) {
		if (st01release(tp)) {
			return(ENXIO);
		}
	}

	return(0);
}

/*
 * void
 * st01strategy(buf_t *bp)
 *	Driver strategy entry point.
 *	st01strategy  determines the flow of control through
 *	restricted dma code, by checking the device's I/O capability,
 *	then sends the request on to st01strat0.
 * Called by: kernel
 * Side Effects: 
 *	DMA devices have data moved to dmaable memory when necessary.
 *
 * Calling/Exit State:
 *	None.
 */
void
st01strategy(buf_t *bp)
{
	register struct tape *tp;
	unsigned unit;
#ifdef PDI_SVR42
	struct scsi_ad  *dev;
	struct sdi_devinfo info;
#endif
	size_t bsize;

	unit = UNIT(bp->b_edev);
	tp = &st01_tape[unit];

#ifdef PDI_SVR42
	dev = &tp->t_addr;
	info.strat = st01strat0;
	info.iotype = tp->t_iotype;
	info.max_xfer = (HIP(HBA_tbl[SDI_EXHAN(dev)].info)->max_xfer - ptob(1));
	info.granularity = bsize;
	sdi_breakup(bp, &info);
#else
	buf_breakup(st01strat0, bp, tp->t_bcbp);
#endif

        return;
}

/*
 * void
 * st01strat0(buf_t *bp)
 * Called by: sdi_breakup2
 * 	Initiate I/O to the device. This function only checks the 
 *	validity of the request.  Most of the work is done by st01io().
 * Side Effects: 
 *
 * Calling/Exit State:
 *	None.
 */
void
st01strat0(buf_t *bp)
{
	register struct tape *tp;
	unsigned unit;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(bp->b_edev);
	tp = &st01_tape[unit];

	bp->b_resid = bp->b_bcount;

	if (tp->t_state & T_TAPEEND) {
		bp->b_flags |= B_ERROR;
		bp->b_error = ENOSPC;
#ifdef PDI_SVR42
		biodone(bp);
#else
		if( tp->t_state & T_ABORTED ) {
			tp->t_state &= ~T_ABORTED;
			freerbuf(bp);
			(void) wakeup((caddr_t) &tp->t_state);
		}       
		else {
			biodone(bp);
		}
#endif
		return;
	}

	if ((tp->t_state & T_FILEMARK) && (bp->b_flags & B_READ)) {
#ifdef PDI_SVR42
		biodone(bp);
#else
		if( tp->t_state & T_ABORTED ) {
			tp->t_state &= ~T_ABORTED;
			freerbuf(bp);
			(void) wakeup((caddr_t) &tp->t_state);
		}       
		else {
			biodone(bp);
		}
#endif
		return;
	}

	st01io(tp, bp);
}

/*
 * st01read() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 * Description:
 * 	Driver read() entry point.  Performs a raw read from the
 *	device.  The function calls physio() which locks the user
 */
/* ARGSUSED */
st01read(dev, uio_p, cred_p)
uio_t	*uio_p;
cred_t	*cred_p;
register dev_t dev;
{
	unsigned unit;

	int		cnt;
	ulong_t		xfer_req;
	ulong_t		max_xfer;
	struct iovec	*iov;
	struct tape	*tp;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);

	/* Check for non-existent device */
	if (unit >= st01_tapecnt)
		return(ENXIO);

	tp = &st01_tape[unit];
	if ( ! tp->t_bsize ) {

		xfer_req = (ulong_t)0;
		for (cnt = 0, iov = uio_p->uio_iov; cnt++ < uio_p->uio_iovcnt; iov++) {
			xfer_req += (ulong_t)iov->iov_len;
		}

		/*
		 * The tape is in Variable Length Mode. We need to make sure that
		 * the requested transfer size is not greater than DRV_MAXBIOSIZE.
		 * uiophysio(), called out of physiock(), breaks requests into
		 * multiple DRV_MAXBIOSIZE size transfers. Since we cannot break up
		 * requests in Variable Length Mode, fail the reqeust.
		 */
		drv_getparm(DRV_MAXBIOSIZE,&max_xfer);

		/*
		 * We use 1 Page less than the HBA's MaxTransfer in case an I/O request
		 * comes down that is not aligned on a bcb_granularity boundary.
		 */
		if ( xfer_req > (max_xfer - ptob(1)) ) {
			return(EINVAL);
		}
	}


	return (physiock(st01strategy, 0, dev, B_READ, 0, uio_p));
}

/*
 * st01write() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 * 	Driver write() entry point.  Performs a raw write to the
 *	device.  The function calls physio() which locks the user
 *	buffer into core.
 */
/* ARGSUSED */
st01write(dev, uio_p, cred_p)
uio_t	*uio_p;
cred_t	*cred_p;
register dev_t dev;
{
	unsigned unit;

	int		cnt;
	ulong_t		xfer_req;
	ulong_t		max_xfer;
	struct iovec	*iov;
	struct tape	*tp;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);

	/* Check for non-existent device */
	if (unit >= st01_tapecnt)
		return(ENXIO);

	tp = &st01_tape[unit];
	if ( ! tp->t_bsize ) {

		xfer_req = (ulong_t)0;
		for (cnt = 0, iov = uio_p->uio_iov; cnt++ < uio_p->uio_iovcnt; iov++) {
			xfer_req += (ulong_t)iov->iov_len;
		}

		/*
		 * The tape is in Variable Length Mode. We need to make sure that
		 * the requested transfer size is not gretaer than DRV_MAXBIOSIZE.
		 * uiophysio(), called out of physiock(), breaks requests into
		 * multiple DRV_MAXBIOSIZE size transfers. Since we cannot break up
		 * requests in Variable Length Mode, fail the reqeust.
		 */
		drv_getparm(DRV_MAXBIOSIZE,&max_xfer);

		/*
		 * We use 1 Page less than the HBA's MaxTransfer in case an I/O request
		 * comes down that is not aligned on a bcb_granularity boundary.
		 */
		if ( xfer_req > (max_xfer - ptob(1)) ) {
			return(EINVAL);
		}
	}

	return (physiock(st01strategy, 0, dev, B_WRITE, 0, uio_p));
}

/*
 * st01ioctl() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Driver ioctl() entry point.  Used to implement the following
 *	special functions:
 *
 *    B_GETTYPE		-  Get bus type and driver name
 *    B_GETDEV		-  Get pass-through major/minor numbers
 *
 *    T_RWD		-  Rewind tape to BOT
 *    T_WRFILEM		-  Write file mark
 *    T_SFF/SFB		-  Space filemarks forward/backwards
 *    T_SBF/SBB		-  Space blocks forward/backwards
 *    T_LOAD/UNLOAD	-  Medium load/unload
 *    T_ERASE		-  Erase tape
 *    T_RETENSION	-  Retension tape
 *    T_RST		-  Reset tape
 *
 *    T_PREVMV		-  Prevent medium removal
 *    T_ALLOMV		-  Allow medium removal
 *    T_RDBLKLEN	-  Read block length limits
 *    T_WRBLKLEN	-  Write block length to be used
 *    T_STD		-  Set tape density
 *
 */
/* ARGSUSED */
st01ioctl(dev, cmd, arg, oflag, cred_p, rval_p)
cred_t	*cred_p;
int	*rval_p;
dev_t dev;
int cmd, oflag;
caddr_t arg;
{
	unsigned unit;

	int cnt;
	int code;
	int dc;
	int ret_code = 0;

	dev_t	pdev;

	struct tape *tp;
#ifdef T_INQUIR
	struct ident *idp;
#endif
	struct blklen *cp;
	struct mode *mp;

	/* check if st01rinit is in process of creating new st01_tape struct*/
	while (rinit_flag) {
		sleep((caddr_t)&rinit_flag, PRIBIO);
	}

	unit = UNIT(dev);
	tp = &st01_tape[unit];

	switch(cmd) {

	case T_RST:	/* Reset tape */
#ifdef TARGET_RESET
		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}

		tp->t_fltrst->sb_type =     SFB_TYPE;
		tp->t_fltrst->SFB.sf_int =  st01intf;
		tp->t_fltrst->SFB.sf_dev =  tp->t_addr;
		tp->t_fltrst->SFB.sf_wd =   (long) tp;
		tp->t_fltrst->SFB.sf_func = SFB_RESETM;
		ret_code = st01docmd (sdi_icmd, tp, tp->t_fltrst, NULL, KM_SLEEP);

		if (ret_code != SDI_RET_OK) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
#endif
		break;

	case T_RWD:	/* Rewind to beginning of tape */

		if (tp->t_state & T_WRITTEN) {
			ST01IOCTL(T_WRFILEM, 2);
		}

		while ((ret_code = st01cmd(tp, SS_REWIND, 0, NULL, 0, 0,
		    SCB_READ, 0, 0)) == EBUSY ) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_WRFILEM:		/* Write filemarks */

		cnt = (int) arg;
		if (cnt < 0) {
			return(EINVAL);
		}

		while ((ret_code = st01cmd(tp, SS_FLMRK, cnt>>8, NULL, 0,
		    cnt, SCB_WRITE, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state |= T_FILEMARK;
		tp->t_state &= ~T_WRITTEN;
		break;

	case T_SFF:	/* Space filemarks forward   */
	case T_SFB:	/* Space filemarks backwards */
	case T_SBF:	/* Space blocks forward    */
	case T_SBB:	/* Space blocks backwards  */

		cnt = (int)arg;

		if (tp->t_state & T_WRITTEN) {
			return(EINVAL);
		}

		if( cnt & ~0xffffff ) {
			/*
			 * 'arg' is required to be positive,
			 * and 'cnt' has a 24-bit size limit
			 */
			return(EINVAL);
		}

		if (cmd == T_SFF || cmd == T_SFB) {
			code = FILEMARKS;
		}
		else {
			code = BLOCKS;
		}

		if (cmd == T_SFB || cmd == T_SBB) {
			cnt = (-1) * cnt;
		}

		while ((ret_code = st01cmd(tp, SS_SPACE, cnt>>8, NULL, 0,
		    cnt, SCB_READ, code, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_LOAD:	/* Medium load/unload */

		if (tp->t_state & T_WRITTEN) {
			ST01IOCTL(T_WRFILEM, 2);
		}

		while ((ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, LOAD,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_UNLOAD:		/* Unload media */

		if (tp->t_state & T_WRITTEN) {
			/* write double filemark */
			ST01IOCTL(T_WRFILEM, 2);
		}

		while ((ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, UNLOAD,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		tp->t_state &= ~T_PARMS;
		break;

	case T_ERASE:		/* Erase Tape */

		ST01IOCTL(T_RWD, 1);	/* Must rewind first */

		while ((ret_code = st01cmd(tp, SS_ERASE, 0, NULL, 0, 0,
		    SCB_WRITE, LONG, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case T_RETENSION:	/* Retension tape */

		if (tp->t_state & T_WRITTEN) {
			ST01IOCTL(T_WRFILEM, 2);
		}

		while ((ret_code = st01cmd(tp, SS_LOAD, 0, NULL, 0, RETENSION,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

	case B_GETTYPE:		/* Tell user bus and driver name */

		if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}

		if (copyout("st01", ((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}

		break;

	case B_GETDEV:			/* Return pass-thru device       */
					/* major/minor to user in 'arg'. */

		sdi_getdev(&tp->t_addr, &pdev);
		if (copyout((caddr_t)&pdev, arg, sizeof(pdev)))
			return(EFAULT);
		break;

	/*
	 * The following ioctls are group 0 commands
	 */
#ifdef T_TESTUNIT
	case T_TESTUNIT:		/* Test Unit Ready */

		while ((ret_code = st01cmd(tp, SS_TEST, 0, NULL, 0, 0,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;
#endif

	case T_PREVMV:			/* Prevent media removal */

		while ((ret_code = st01cmd(tp, SS_LOCK, 0, NULL, 0, 1,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;

	case T_ALLOMV:			/* Allow media removal */

		while ((ret_code = st01cmd(tp, SS_LOCK, 0, NULL, 0, 0,
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;

#ifdef T_INQUIR
	case T_INQUIR:			/* Inquiry */

		idp = (struct ident *)&tp->t_ident;

		while ((ret_code = st01cmd(tp, SS_INQUIR, 0, idp,
		    sizeof(struct ident), sizeof(struct ident),
		    SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		if (copyout((char *)idp, arg, sizeof(struct ident))) {
			return(EFAULT);
		}
		break;
#endif

	case T_RDBLKLEN:		/* Read block length limits */

		cp = &tp->t_blklen;
		while ((ret_code = st01cmd(tp, SS_RDBLKLEN, 0, cp,
		    RDBLKLEN_SZ, 0, SCB_READ, 0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		cp->max_blen = sdi_swap24(cp->max_blen);
		cp->min_blen = sdi_swap16(cp->min_blen);
		if (copyout((char *)cp, arg, sizeof(struct blklen))) {
			return(EFAULT);
		}

		break;

	case T_WRBLKLEN:		/* Write block length to be used */

		mp = &tp->t_mode;
		cp = &tp->t_blklen;
		if (copyin(arg, (char *)cp, sizeof(struct blklen)) < 0) {
			return(EFAULT);
		}

		while ((ret_code = st01cmd(tp, SS_MSENSE, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_READ, 0,
		    0)) == EBUSY ) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		/*
		 * If the requested legnth is greater than the HBA's Maximum
		 * Transfer, fail the requet.
		 *
		 * We use 1 Page less than the HBA's MaxTransfer in case an I/O request
		 * comes down that is not aligned on a bcb_granularity boundary.
		 */
		if( cp->max_blen > (HIP(HBA_tbl[SDI_EXHAN(&tp->t_addr)].info)->max_xfer - ptob(1)) ) {
			return(EINVAL);
		}

		mp->md_len = 0;				 /* Reserved field */
		mp->md_media = 0;			 /* Reserved field */
		mp->md_wp = 0;				 /* Reserved field */
		mp->md_nblks = 0;			 /* Reserved field */
		mp->md_bsize = sdi_swap24(cp->max_blen); /* Fix block size */

		while ((ret_code = st01cmd(tp, SS_MSELECT, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_WRITE,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_bsize = mp->md_bsize;	/* Note the new block size */
		break;

	case T_STD:			/* Set Tape Density */

		dc = (int) arg;
		mp = &tp->t_mode;

		if (dc < 0 || dc > 0xff) {
			return(EINVAL);
		}

		while ((ret_code = st01cmd(tp, SS_MSENSE, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_READ,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		mp->md_len = 0;			/* Reserved field 	*/
		mp->md_media = 0;		/* Reserved field 	*/
		mp->md_wp = 0;			/* Reserved field 	*/
		mp->md_nblks = 0;		/* Reserved field 	*/
		mp->md_dens = dc;		/* User's density code */
		while ((ret_code = st01cmd(tp, SS_MSELECT, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_WRITE,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;

	case T_EOD:			/* Position to end of recorded data */

		code = EORD;

		if (tp->t_state & T_WRITTEN) {
			return(EINVAL);
		}

		while ((ret_code = st01cmd(tp, SS_SPACE, 0, NULL, 0, 0,
		    SCB_READ, code, 0)) == EBUSY ) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		tp->t_state &= ~(T_FILEMARK | T_TAPEEND | T_READ | T_WRITTEN);
		break;

#ifdef T_MSENSE
	case T_MSENSE:			/* Mode sense */

		/*
		 * (Application has to swap all multi-byte fields)
		 */
		mp = &tp->t_mode;

		if (copyin(arg, (char *)mp, sizeof(struct mode)) < 0) {
			return(EFAULT);
		}

		while ((ret_code = st01cmd(tp, SS_MSENSE, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_READ,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		if (copyout((char *)mp, arg, sizeof(struct mode))) {
			return(EFAULT);
		}

		break;
#endif
#ifdef T_MSELECT
	case T_MSELECT:			/* Mode select */

		/*
		 * (Application has to swap all multi-byte fields)
		 */
		mp = &tp->t_mode;

		if (copyin(arg, (char *)mp, sizeof(struct mode)) < 0) {
			return(EFAULT);
		}

		while ((ret_code = st01cmd(tp, SS_MSELECT, 0, mp,
		    sizeof(struct mode), sizeof(struct mode), SCB_WRITE,
		    0, 0)) == EBUSY) {
			delay(1 * HZ);
		}

		if (ret_code != 0 ) {
			return(EIO);
		}

		break;
#endif

	default:
		ret_code = EINVAL;
	}

	return(ret_code);
}

/*
 * st01print() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function prints the error message provided by the kernel.
 */
int
st01print(dev, str)
dev_t dev;
register char *str;
{
	char name[SDI_NAMESZ];
	register struct tape *tp;

	tp = &st01_tape[UNIT(dev)];
	sdi_name(&tp->t_addr, name);
		/*
	  	 *+ display err msg
		 */
        cmn_err(CE_WARN, "Tape driver: %s, %s\n", name, str);
	return(0);
}

/*
 * st01io() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function creates and sends a SCSI I/O request.
 */
void
st01io(tp, bp)
register struct tape *tp;
register buf_t *bp;
{
	register struct job *jp;
	register struct scb *scb;
	register struct scs *cmd;
	register int nblk;

	jp = st01getjob();
	jp->j_tp = tp;
	jp->j_bp = bp;

	jp->j_sb->sb_type = SCB_TYPE;
	jp->j_time = JTIME;

	/*
	 * Fill in the scb for this job.
	 */

	scb = &jp->j_sb->SCB;
	scb->sc_cmdpt = SCS_AD(&jp->j_cmd);
	scb->sc_cmdsz = SCS_SZ;
	scb->sc_datapt = bp->b_un.b_addr;
	scb->sc_datasz = bp->b_bcount;
	scb->sc_link = NULL;
	scb->sc_mode = (bp->b_flags & B_READ) ? SCB_READ : SCB_WRITE;
	scb->sc_dev = tp->t_addr;

	sdi_translate(jp->j_sb, bp->b_flags, bp->b_proc, KM_SLEEP);

	scb->sc_int = st01intn;
	scb->sc_time = JTIME;
	scb->sc_wd = (long) jp;

	/*
	 * Fill in the command for this job.
	 */

	cmd = (struct scs *)&jp->j_cmd;
	cmd->ss_op = (bp->b_flags & B_READ) ? SS_READ : SS_WRITE;
	cmd->ss_lun  = tp->t_addr.sa_lun;

	if (tp->t_bsize) {	 /* Fixed block transfer mode	*/
		cmd->ss_mode = FIXED;
		nblk = bp->b_bcount / tp->t_bsize;
		cmd->ss_len1 = mdbyte(nblk) << 8 | msbyte(nblk);
		cmd->ss_len2 = lsbyte(nblk);
	} else {		/* Variable block transfer mode */
		cmd->ss_mode = VARIABLE;
		cmd->ss_len1 = mdbyte(bp->b_bcount) << 8 | msbyte(bp->b_bcount);
		cmd->ss_len2 = lsbyte(bp->b_bcount);
	}
	cmd->ss_cont = 0;

	st01docmd(sdi_send, jp->j_tp, jp->j_sb, jp, KM_SLEEP);	/* send it */
}

/*
 * st01comp() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Called on completion of a job, both successful and failed.
 *	The function de-allocates the job structure used and calls
 *	biodone().  Restarts the logical unit queue if necessary.
 */
void
st01comp(jp)
register struct job *jp;
{
        register struct tape *tp;
	register struct buf *bp;

        tp = jp->j_tp;
	bp = jp->j_bp;

	/* Check if job completed successfully */
	if (jp->j_sb->SCB.sc_comp_code == SDI_ASW) {
		bp->b_resid  = 0;
		tp->t_lastop = jp->j_cmd.ss.ss_op;
		if (tp->t_lastop == SS_READ)
			tp->t_state |= T_READ;
		if (tp->t_lastop == SS_WRITE) {
			tp->t_state &= ~T_FILEMARK;
			tp->t_state |= T_WRITTEN;
		}
	} else {
		bp->b_flags |= B_ERROR;

		if (jp->j_sb->SCB.sc_status == S_BUSY ) {
			bp->b_error = EBUSY;
		}
		else {

			if (jp->j_sb->SCB.sc_comp_code == SDI_NOSELE) {
				bp->b_error = ENODEV;
			}
			else {
				bp->b_error = EIO;
			}

			if (tp->t_state & T_TAPEEND)	{
				if(bp->b_bcount && (bp->b_resid == bp->b_bcount)) {
					bp->b_error = ENOSPC;
				}
				else {
					bp->b_flags &= ~B_ERROR;
				}
			}
			else if(tp->t_state & T_FILEMARK) {
				bp->b_flags &= ~B_ERROR;
			}
			else if ((jp->j_cmd.ss.ss_op == SS_READ ||
				  jp->j_cmd.ss.ss_op == SS_WRITE) &&
				  tp->t_bsize == 0 && bp->b_resid >= 0)	{
				bp->b_flags &= ~B_ERROR;
			}
		}
	}

#ifdef PDI_SVR42
	biodone(bp);
#else
	if( tp->t_state & T_ABORTED ) {
		tp->t_state &= ~T_ABORTED;
		freerbuf(bp);
		(void) wakeup((caddr_t) &tp->t_state);
	}
	else {
		biodone(bp);
	}
#endif

	st01freejob(jp);

	/* Resume queue if suspended */
	if (tp->t_state & T_SUSPEND)
	{
		tp->t_fltres->sb_type = SFB_TYPE;
		tp->t_fltres->SFB.sf_int  = st01intf;
		tp->t_fltres->SFB.sf_dev  = tp->t_addr;
		tp->t_fltres->SFB.sf_wd = (long) tp;
		tp->t_fltres->SFB.sf_func = SFB_RESUME;
		if (st01docmd (sdi_icmd, tp, tp->t_fltres, NULL, KM_NOSLEEP) ==
		    SDI_RET_OK) {
			tp->t_state &= ~T_SUSPEND;
			tp->t_fltcnt = 0;
		}
	}

	return;
}

/*
 * st01intn() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function is called by the host adapter driver when an
 *	SCB job completes.  If the job completed with an error, the
 *	appropriate error handling is performed.
 */
void
st01intn(sp)
register struct sb *sp;
{
	register struct tape *tp;
	register struct job *jp;

	jp = (struct job *)sp->SCB.sc_wd;
	tp = jp->j_tp;

	if (sp->SCB.sc_comp_code == SDI_ASW) {
		st01comp(jp);
		return;
	}

	if (sp->SCB.sc_comp_code & SDI_SUSPEND)
		tp->t_state |= T_SUSPEND;

	if (sp->SCB.sc_comp_code == SDI_CKSTAT &&
	    sp->SCB.sc_status == S_CKCON)
	{
		struct sense *sensep = sdi_sense_ptr(sp);
		tp->t_fltjob = jp;

		if ((sensep->sd_key != SD_NOSENSE) || sensep->sd_fm 
		  || sensep->sd_eom || sensep->sd_ili) {
			bcopy(sensep, &tp->t_sense,sizeof(struct sense));
			st01sense(tp);
			return;
		}
		tp->t_fltreq->sb_type = ISCB_TYPE;
		tp->t_fltreq->SCB.sc_int = st01intrq;
		tp->t_fltreq->SCB.sc_cmdsz = SCS_SZ;
		tp->t_fltreq->SCB.sc_time = JTIME;
		tp->t_fltreq->SCB.sc_mode = SCB_READ;
		tp->t_fltreq->SCB.sc_dev = sp->SCB.sc_dev;
		tp->t_fltreq->SCB.sc_wd = (long) tp;
		tp->t_fltreq->SCB.sc_datapt = SENSE_AD(&tp->t_sense);
		tp->t_fltreq->SCB.sc_datasz = SENSE_SZ;
		tp->t_fltcmd.ss_op = SS_REQSEN;
		tp->t_fltcmd.ss_lun = sp->SCB.sc_dev.sa_lun;
		tp->t_fltcmd.ss_addr1 = 0;
		tp->t_fltcmd.ss_addr = 0;
		tp->t_fltcmd.ss_len = SENSE_SZ;
		tp->t_fltcmd.ss_cont = 0;

		/* clear old sense key */
		tp->t_sense.sd_key = SD_NOSENSE;

		if (st01docmd (sdi_icmd, tp, tp->t_fltreq, NULL, KM_NOSLEEP) !=
		    SDI_RET_OK) {
			/* Fail the job */
			st01logerr(tp, sp);
			st01comp(jp);
		}
	}
	else
	{
		/* Fail the job */
		st01logerr(tp, sp);
		st01comp(jp);
	}

	return;
}

/*
 * st01intrq() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function is called by the host adapter driver when a
 *	request sense job completes.  The job will be retried if it
 *	failed.  Calls st01sense() on successful completions to
 *	examine the request sense data.
 */
void
st01intrq(sp)
register struct sb *sp;
{
	register struct tape *tp;

	tp = (struct tape *)sp->SCB.sc_wd;

	if (sp->SCB.sc_comp_code != SDI_CKSTAT  &&
	    sp->SCB.sc_comp_code &  SDI_RETRY   &&
	    ++tp->t_fltcnt <= MAX_RETRY)
	{
		sp->SCB.sc_time = JTIME;
		if (st01docmd (sdi_icmd, tp, sp, NULL, KM_NOSLEEP) != SDI_RET_OK) {
			st01logerr(tp, sp);
			st01comp(tp->t_fltjob);
		}
		return;
	}

	if (sp->SCB.sc_comp_code != SDI_ASW) {
		st01logerr(tp, sp);
		st01comp(tp->t_fltjob);
		return;
	}

	st01sense(tp);
}

/*
 * st01intf() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function is called by the host adapter driver when a host
 *	adapter function request has completed.  If there was an error
 *	the request is retried.  Used for resume function completions.
 */
void
st01intf(sp)
register struct sb *sp;
{
	register struct tape *tp;

	tp = (struct tape *)sp->SFB.sf_wd;

	if (sp->SFB.sf_comp_code & SDI_RETRY  &&
	    ++tp->t_fltcnt <= MAX_RETRY)
	{
		if (st01docmd (sdi_icmd, tp, sp, NULL, KM_NOSLEEP) != SDI_RET_OK) {
			st01logerr(tp, sp);
		}
		return;
	}

	if (sp->SFB.sf_comp_code != SDI_ASW)
		st01logerr(tp, sp);
}

/*
 * st01sense() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Performs error handling based on SCSI sense key values.
 */
void
st01sense(tp)
register struct tape *tp;
{
	register struct job *jp;
	register struct sb *sp;

	jp = tp->t_fltjob;
	sp = jp->j_sb;

        switch(tp->t_sense.sd_key) {
	case SD_VOLOVR:
		tp->t_state |= T_TAPEEND;
		/* FALLTHROUGH */

	case SD_NOSENSE:
		if (jp->j_cmd.ss.ss_op == SS_READ  ||
		    jp->j_cmd.ss.ss_op == SS_WRITE)
		{
			register struct buf *bp = jp->j_bp;
			register nblks;

			if (tp->t_sense.sd_valid) {
				if (tp->t_bsize) {	/* Fixed Block Len */
					nblks = sdi_swap32(tp->t_sense.sd_ba);
					bp->b_resid = tp->t_bsize * nblks;
				} else {		/* Variable	   */
					bp->b_resid = sdi_swap32(tp->t_sense.sd_ba);
				}
			}
			else {
				bp->b_resid = 0;
			}

			if (tp->t_sense.sd_ili &&
			    (tp->t_bsize || bp->b_resid < 0)) {

				st01logerr(tp, sp);

				/*
				 *+ display err msg
				 */
                                cmn_err(CE_WARN,
                                "Tape driver: Block length mismatch.");

				/*
				 *+ display err msg
				 */
                                cmn_err(CE_CONT,
                                "Driver requested %d, Tape block length = %d\n",
                                bp->b_bcount, bp->b_bcount - (int) bp->b_resid);

				/*
				 * The t_sense.sd_ba information field now
				 * contains:
				 *
				 * (signed)((bp->b_bcount)-(actual recordsize))
				 *
				 * If the actual record size was larger than
				 * the requested amount, requested data has
				 * been read, but we have skipped over thee
				 * rest of the record.
				 */
				if ((int) bp->b_resid < 0) {
					bp->b_resid = 0;
				}

				bp->b_flags |= B_ERROR;
			}
		}

		if (tp->t_sense.sd_fm)
			tp->t_state |= T_FILEMARK;
		if (tp->t_sense.sd_eom)
			tp->t_state |= T_TAPEEND;

		st01comp(jp);
		break;

	case SD_UNATTEN:
		if (++tp->t_fltcnt > MAX_RETRY) {
			st01logerr(tp, sp);
			st01comp(jp);
		} else {
			sp->sb_type = ISCB_TYPE;
			sp->SCB.sc_time = JTIME;
			if (st01docmd (sdi_icmd, tp, sp, NULL, KM_NOSLEEP) != 
			    SDI_RET_OK) {
				st01logerr(tp, sp);
				st01comp(jp);
			}
		}
		break;

	case SD_RECOVER:
		st01logerr(tp, sp);
		sp->SCB.sc_comp_code = SDI_ASW;
		st01comp(jp);
		break;

	/* Some drives give a blank check during positioning */
	case SD_BLANKCK:
		/*
		if ((jp->j_cmd.ss.ss_op == SS_READ)
		||  (jp->j_cmd.ss.ss_op == SS_WRITE))
			st01logerr(tp, sp);
		else
			sp->SCB.sc_comp_code = SDI_ASW;
		*/
		st01logerr(tp, sp);
		st01comp(jp);
		break;

	default:
		st01logerr(tp, sp);
		st01comp(jp);
	}
}

/*
 * st01logerr() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function will print the error messages for errors detected
 *	by the host adapter driver.  No message will be printed for
 *	those errors that the host adapter driver has already reported.
 */
void
st01logerr(tp, sp)
register struct tape *tp;
register struct sb *sp;
{
	if (sp->sb_type == SFB_TYPE) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,&tp->t_sense,SDI_SFB_ERR,0);
		}
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT  && sp->SCB.sc_status == S_CKCON) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,&tp->t_sense,SDI_CKCON_ERR,0);
		}
		return;
	}

	if (sp->SCB.sc_comp_code == SDI_CKSTAT) {
		if ((tp->t_state & T_OPENING) == 0 ) {
			sdi_errmsg("Tape",&tp->t_addr,sp,&tp->t_sense,SDI_CKSTAT_ERR,0);
		}
	}
}

/*
 * st01config() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Initializes the tape driver's tape parameter structure. To
 *	support Autoconfig, the command sequence should be:
 *
 *		INQUIRY*
 *		LOAD
 *		TEST UNIT READY
 *		MODE SENSE
 *		MODE SELECT*
 *
 *	Autoconfig is not implemented at this point. In this case, both
 *	INQUIRY and MODE SELECT are not called.	 The driver will use
 *	whatever settings returned by MODE SENSE as default.
 */
st01config(tp)
register struct tape *tp;
{
	int	rtcde;
	struct	blklen	*cp;
	struct	mode	*mp;

	cp = &tp->t_blklen;

	while ((rtcde = st01cmd(tp, SS_LOAD, 0, NULL, 0, LOAD, SCB_READ, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( rtcde != 0 ) {
		return( EIO );
	}

	while ((rtcde = st01cmd(tp, SS_TEST, 0, NULL, 0, 0, SCB_READ, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( rtcde != 0 ) {
		return( EIO );
	}

	while ((rtcde = st01cmd(tp, SS_RDBLKLEN, 0, (char *)cp, RDBLKLEN_SZ, 0, SCB_READ, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( rtcde != 0 ) {
		return( EIO );
	}

	cp->max_blen = sdi_swap24(cp->max_blen);
	cp->min_blen = sdi_swap16(cp->min_blen);

	/* Send Mode Sense	*/
	mp = &tp->t_mode;
	while ((rtcde = st01cmd(tp, SS_MSENSE, 0, mp, sizeof(struct mode),
	    sizeof(struct mode), SCB_READ, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( rtcde != 0 ) {
		return( EIO );
	}

	mp->md_bsize = sdi_swap24(mp->md_bsize);
	if ((mp->md_bsize < cp->min_blen) && (mp->md_bsize != 0)) {
		return(ENXIO);
	}

	if ((mp->md_bsize > cp->max_blen) && (mp->md_bsize != 0) &&
	    (cp->max_blen !=0)) {
		return(ENXIO);
	}

	tp->t_bsize = mp->md_bsize;

#ifndef PDI_SVR42
	if (tp->t_bsize) {
		tp->t_bcbp->bcb_granularity = tp->t_bsize;
		tp->t_bcbp->bcb_flags |= BCB_EXACT_SIZE;
		tp->t_bcbp->bcb_flags &= ~BCB_ONE_PIECE;
	}
	else {
		tp->t_bcbp->bcb_granularity = 1;
		tp->t_bcbp->bcb_flags &= ~BCB_EXACT_SIZE;
		tp->t_bcbp->bcb_flags |= BCB_ONE_PIECE;
	}

	/*
	 * We use 1 Page less than the HBA's MaxTransfer in case an I/O request
	 * comes down that is not aligned on a bcb_granularity boundary.
	 */
	if( tp->t_bcbp->bcb_granularity > (HIP(HBA_tbl[SDI_EXHAN(&tp->t_addr)].info)->max_xfer - ptob(1)) ) {
		/*
		 * Granularity is greater than the HBA's max_xfer, use the HBA's limit.
		 */
		tp->t_bcbp->bcb_max_xfer = (HIP(HBA_tbl[SDI_EXHAN(&tp->t_addr)].info)->max_xfer - ptob(1));
	}
	else {
		/*
		 * Find the largest multiple of the granularity
		 * that will fit in the HBA's max_fer.
		 *
		 * We use 1 Page less than the HBA's MaxTransfer in case an I/O request
		 * comes down that is not aligned on a bcb_granularity boundary.
		 */
		tp->t_bcbp->bcb_max_xfer = tp->t_bcbp->bcb_granularity;
		while( (tp->t_bcbp->bcb_max_xfer + tp->t_bcbp->bcb_granularity) <=
		        (HIP(HBA_tbl[SDI_EXHAN(&tp->t_addr)].info)->max_xfer - ptob(1)) ) {

			tp->t_bcbp->bcb_max_xfer += tp->t_bcbp->bcb_granularity;
		}
	}

#ifdef DEBUG
	cmn_err(CE_NOTE,"st01config: max_xfer = %d",HIP(HBA_tbl[SDI_EXHAN(&tp->t_addr)].info)->max_xfer);
	cmn_err(CE_NOTE,"st01config: bcb_granularity = %d",tp->t_bcbp->bcb_granularity);
	cmn_err(CE_NOTE,"st01config: bcb_max_xfer = %d",tp->t_bcbp->bcb_max_xfer);
#endif
#endif /* !PDI_SVR42 */

	/*
	 * Indicate parameters are set and valid
	 */
	tp->t_state |= T_PARMS;
	return(0);
}

/*
 * st01cmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	This function performs a SCSI command such as Mode Sense on
 *	the addressed tape.  The op code indicates the type of job
 *	but is not decoded by this function.  The data area is
 *	supplied by the caller and assumed to be in kernel space.
 *	This function will sleep.
 */
st01cmd(tp, op_code, addr, buffer, size, length, mode, param, control)
register struct tape	*tp;
unsigned char	op_code;		/* Command opcode		*/
unsigned int	addr;			/* Address field of command 	*/
char		*buffer;		/* Buffer for command data 	*/
unsigned int	size;			/* Size of the data buffer 	*/
unsigned int	length;			/* Block length in the CDB	*/
unsigned short	mode;			/* Direction of the transfer 	*/
unsigned int	param;
unsigned int	control;
{
	register struct job *jp;
	register struct scb *scb;
	register buf_t *bp;
	int error;

	bp = getrbuf(KM_SLEEP);

	jp = st01getjob();
	scb = &jp->j_sb->SCB;

	bp->b_iodone = NULL;
	bp->b_blkno = addr;
	bp->b_flags |= mode & SCB_READ ? B_READ : B_WRITE;
	bp->b_error = 0;

	jp->j_bp = bp;
	jp->j_tp = tp;
	jp->j_sb->sb_type = SCB_TYPE;

	switch(op_code >> 5){
	case	GROUP7:
	{
		register struct scm *cmd;

		cmd = (struct scm *)&jp->j_cmd.sm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = tp->t_addr.sa_lun;
		cmd->sm_res1 = 0;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_len  = sdi_swap16(length);
		cmd->sm_res1 = param;
		cmd->sm_cont = control;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
		break;
	case	GROUP6:
	{
		register struct scs *cmd;

		cmd = (struct scs *)&jp->j_cmd.ss;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = tp->t_addr.sa_lun;
		cmd->ss_addr1 = ((addr & 0x1F0000) >> 16);
		cmd->ss_addr  = sdi_swap16(addr & 0xFFFF);
		cmd->ss_len   = (char)length;
		cmd->ss_cont  = (char)control;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
		break;
	case	GROUP1:
	{
		register struct scm *cmd;

		cmd = (struct scm *)&jp->j_cmd.sm;
		cmd->sm_op   = op_code;
		cmd->sm_lun  = tp->t_addr.sa_lun;
		cmd->sm_res1 = param;
		cmd->sm_addr = sdi_swap32(addr);
		cmd->sm_res2 = 0;
		cmd->sm_len  = sdi_swap16(length);
		cmd->sm_cont = 0;

		scb->sc_cmdpt = SCM_AD(cmd);
		scb->sc_cmdsz = SCM_SZ;
	}
		break;
	case	GROUP0:
	{
		register struct scs *cmd;

		cmd = (struct scs *)&jp->j_cmd.ss;
		cmd->ss_op    = op_code;
		cmd->ss_lun   = tp->t_addr.sa_lun;
		cmd->ss_addr1 = param;
		cmd->ss_addr  = sdi_swap16(addr & 0xFFFF);
		cmd->ss_len   = (char)length;
		cmd->ss_cont  = 0;

		scb->sc_cmdpt = SCS_AD(cmd);
		scb->sc_cmdsz = SCS_SZ;
	}
		break;
#ifdef DEBUG
        default:
				/*
			  	 *+ display err msg
				 */
                cmn_err(CE_WARN,"Tape driver: Unknown op_code = %x\n",op_code);
#endif
	}

	/* Fill in the SCB */
	scb->sc_int = st01intn;
	scb->sc_dev = tp->t_addr;
	scb->sc_datapt = buffer;
	scb->sc_datasz = size;
	scb->sc_mode = mode;
	scb->sc_resid = 0;
	scb->sc_time = 180 * ONE_MIN;
	scb->sc_wd = (long) jp;
	sdi_translate(jp->j_sb, bp->b_flags, NULL, KM_SLEEP);

	st01docmd(sdi_send, jp->j_tp, jp->j_sb, jp, KM_SLEEP);

#ifdef PDI_SVR42
	biowait(bp);
#else
	if( biowait_sig(bp) == B_FALSE ) {
		tp->t_state = T_ABORTED;
		(void) wakeup((caddr_t) &tp->t_state);
		return(EINTR);
	}
#endif

	if (bp->b_error == EBUSY) {
		error = EBUSY;
	}
	else if (bp->b_flags & B_ERROR) {
		error = EIO;
	}
	else {
		error = 0;
	}

	freerbuf(bp);
	return(error);
}

/*
 * st01docmd() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 */
int
st01docmd(fcn, tp, sbp, jp, sleepflag)
int (*fcn)();
struct tape *tp;
struct sb *sbp;
struct job *jp;
int sleepflag;
{
	struct dev_spec *dsp = tp->t_spec;
	int cmd;
	int ret;
	pl_t opri;

	/* Only queue jobs if we have a job pointer */
	if (jp && tp->t_head && !(tp->t_state & T_RESUMEQ)) {
		opri = SPL();
		tp->t_tail->j_next = jp;
		tp->t_tail = jp;
		jp->j_next = NULL;
		jp->j_func = fcn;
		if (!(tp->t_state & T_SEND)) {
			(void) sdi_timeout(st01resumeq, (caddr_t)tp, 1, pldisk,
				&tp->t_addr);
			tp->t_state |= T_SEND;
		}
		splx(opri);
		return SDI_RET_RETRY;
	}
	if (dsp && sbp->sb_type != SFB_TYPE) {
		cmd = ((struct scs *)(void *)sbp->SCB.sc_cmdpt)->ss_op;
		if (!CMD_SUP(cmd, dsp)) {
			return SDI_RET_ERR;
		} else if (dsp->command && CMD_CHK(cmd, dsp)) {
			(*dsp->command)(tp, sbp);
		}
	}
	ret = (*fcn)(sbp, sleepflag);
	if (ret == SDI_RET_RETRY) {
		if (!jp) {
			/*
			 *+ Tape driver cannot retry job
			 */
			cmn_err(CE_WARN, "!st01: Cannot retry job");
			return SDI_RET_RETRY;
		}
		opri = SPL();
		if (tp->t_head == NULL)
			tp->t_tail = jp;
		jp->j_next = tp->t_head;
		tp->t_head = jp;
		jp->j_func = fcn;
		if (!(tp->t_state & T_SEND)) {
			(void) sdi_timeout(st01resumeq, (caddr_t)tp, 1, pldisk,
				&tp->t_addr);
			tp->t_state |= T_SEND;
		}
		splx(opri);
		return SDI_RET_RETRY;
	}
	return ret;
}

/*
 * STATIC void
 * st01resumeq()
 *
 * Calling/Exit State:
 *
 * Description:
 *	This function is called by timeout to resume a queue.
 */
STATIC void
st01resumeq(tp)
struct tape *tp;
{
	int ret;

	tp->t_state &= ~T_SEND;
	tp->t_state |= T_RESUMEQ;
	while (tp->t_head) {
		struct job *jp = tp->t_head;
		tp->t_head = tp->t_head->j_next;
		ret = st01docmd(jp->j_func, jp->j_tp, jp->j_sb, jp, KM_NOSLEEP);
		if (ret == SDI_RET_RETRY)
			break;
	}
	tp->t_state &= ~T_RESUMEQ;
}

/*
 * st01reserve() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Reserve a device.
 */
int
st01reserve(tp)
register struct tape *tp;
{
	int ret_val;

	if (tp->t_state & T_RESERVED) {
		return(0);
	}

	while ((ret_val = st01cmd(tp, SS_RESERV, 0, (char *)NULL, 0, 0, SCB_WRITE, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( ret_val != 0 ) {
		return( EIO );
	}

	tp->t_state |= T_RESERVED;
	return(0);
}

/*
 * st01release() 
 *
 * Calling/Exit State:
 *
 * Descriptions:
 *	Release a device.
 */
int
st01release(tp)
register struct tape *tp;
{
	int ret_val;

	if (! (tp->t_state & T_RESERVED)) {	/* if already released */
		return(0);
	}

	while ((ret_val = st01cmd(tp, SS_RELES, 0, (char *)NULL, 0, 0, SCB_WRITE, 0, 0)) == EBUSY) {
		delay(1 * HZ);
	}

	if( ret_val != 0 ) {
		/*
		 * release cmd returns an error only when device was reserved
		 * and release failed to release the device.  In this case,
		 * the device is still reserved, so don't clear the
		 * T_RESERVED flag.
		 */
		return( EIO );
	}

	tp->t_state &= ~T_RESERVED;
	return(0);
}
