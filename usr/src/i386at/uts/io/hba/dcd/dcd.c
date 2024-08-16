/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/hba/dcd/dcd.c	1.91"
#ident	"$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#include <svc/errno.h>
#include <util/types.h>
#include <proc/signal.h>
#include <proc/cred.h>
#include <util/cmn_err.h>
#include <util/sysmacros.h>
#include <fs/buf.h>
#include <io/mkdev.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <svc/systm.h>
#include <svc/bootinfo.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>
#include <io/target/sdi/dynstructs.h>
#include <io/target/st01/st01.h>
#include <io/vtoc.h>
#include <io/hba/dcd/dcd.h>
#include <io/hba/dcd/gendev.h>
#include <io/hba/dcd/gendisk.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>
#include	<io/ddi_i386at.h>

#ifdef DEBUG
gdev_dpbp dcd_dpbp;
#endif /* DEBUG */

extern ushort	gdev_majorset();
extern int gdev_start_mult(), gdev_reload();

int	doverify(), do_readver(), format_drive(), format_track(),
	gdev_count_breakup(), gdev_mem_breakup(), gdev_sect_merge(), 
	dcd_sb2drq(), comm_cmd(), dcd_sfb_cmd(), dcd_scb_cmd();
void	mod_drvattach(), mod_drvdetach(), gdev_setintr(),
	dcd_cstart(), dcd_flushr(), dcd_suspend(), dcd_resume(), 
	dcd_test(), dcd_reqsen(), 
	dcd_inquir(), dcd_mselect(), dcd_msense(), dcd_reasgn(), dcd_format(),
	dcd_rdcap(), dcd_verify(), gdev_drainq(), dcd_drqclean(),
	gdev_reserve_drive(), gdev_release_drive(), gdev_start_req();
struct drq_entry *getdrq(), *reldrq();

extern	HBA_IDATA_STRUCT	dcd_idata[];
extern	int	dcd_cntls;
extern struct head	sm_poolhead;	/* head of pool of small dynamic */
					/* structs (28 bytes) for dcdblk's */

#if PDI_VERSION >= PDI_UNIXWARE20
int dcd_devflag = HBA_TIMEOUT;
#else
int dcd_devflag = 0;
#endif

HBA_INFO(dcd_, &dcd_devflag, 0x10000);

#define		DRVNAME	"DCD - HBA driver"
int	dcd_start();

STATIC	int	dcd_load(), dcd_unload();
int	dcd_start();
void	dcd_halt();

MOD_HDRV_WRAPPER(dcd_, dcd_load, dcd_unload, dcd_halt, DRVNAME);

static	int	mod_dynamic = 0;

STATIC int
dcd_load()
{
	mod_dynamic = 1;
	mod_drvattach( &dcd__attach_info );
	if( dcd_start() ) {
		mod_drvdetach( &dcd__attach_info );
		return( ENODEV );
	}
	return(0);
}

STATIC int
dcd_unload()
{
	return( EBUSY );
}

#define DISK_MAJOR      47 	
#define SPLDISK spldisk()

extern long		dcd_major;	    /* SCSI driver major #	*/
extern int dcdcnt;
struct ver_no		dcd_ver;	/* HA drv. SDI version structure*/

/* the following is used to convert the fill field of a SCSI address 
 * structure to a dev_t, which can then be converted into an index ino
 * the minormap structure
 */

static short dcd_ctoi[MAX_HAS];
static short dcd_ftoi[MAX_HAS * MAX_TCS];
		/* WARNING: dcd_start works for just 1 DCD instance */
		/* For multiple DCD's:  we need a dcd_hacnt, dcd_drivecnt */
		/* and dcd_sngctl for each DCD  we have. Also the 	*/
		/* device_cfg_entry must indicate which DCD it would be	*/
		/* a part of.			*/
int dcd_cnt = 0;	/* Number of DCD's we have */
int dcd_hacnt = 0;	/* Number of HBA's (controllers) we have */
int dcd_drivecnt = 0;	/* Number of drives we have 		 */
struct	dcd_sngctl {
	struct	gdev_ctl_block *s_dcbp;
	struct  gdev_parm_block *s_dpbp;
	struct  gdev_driver *s_drvp;
	int	(*s_drvint)();
	struct	gdev_parm_block *(*s_drv_INT)();
	int	s_idx;
} dcd_sngctl;
struct dcd_sngctl *dcd_sngctlp = NULL;

#define fill2devt(sa)	(makedevice(DISK_MAJOR,  \
	dcd_ftoi[ (((sa).sa_fill >> 3) & 03)  \
		* MAX_TCS + ((sa).sa_fill & 07 ) ] ) )
#define fillp2devt(sa)	(makedevice(DISK_MAJOR,  \
	dcd_ftoi[ (((sa)->sa_fill >> 3) & 03)  \
		* MAX_TCS + ((sa)->sa_fill & 07 ) ] ) )
/* need to adjust by 2 bytes because SCM_AD() removed pad field */
#define SCM_RAD(x) ((char *)x - 2)

extern  struct gdev_cfg_entry device_cfg_tbl[];
extern  ushort device_cfg_entries;
struct  drq_entry *get_memdrq();
void	dcd_pass_thru(); 

#ifndef PDI_SVR42
void	dcd_pass_thru0();
#else
#define dcd_pass_thru0 dcd_pass_thru
#endif

/* On many AT controllers you must get exclusive access to the CONTROLLER
 * before performing operations such as setting the parameters or
 * Formatting the drive.  On SCSI controllers it is only necessary to
 * get exclusive access to the DRIVE.  If we get exclusive access to
 * the controller for a SCSI we may end up waiting a long time before
 * we are able to mount or format a 2nd drive on a controller.  The
 * following macros call either gdev_getexcl or gdev_reserve_drive depending
 * upon whether the controller is an AT controller or not.
 * This should probably be part of the gdev_ctrl_blk struct
 */
#define GetExcl(dcbp, dpbp)	if(dcbp->dcb_capab & CCAP_MULTI) \
					gdev_reserve_drive(dcbp,dpbp); \
				else \
					gdev_getexcl(dcbp); 

#define RelExcl(dcbp, dpbp)	if(dcbp->dcb_capab & CCAP_MULTI) \
					gdev_release_drive(dcbp,dpbp); \
				else \
					gdev_relexcl(dcbp); 





/*
 * Disk Initialization and Configuration.
 */

int
dcd_start()
{
	static int  i = 0;
	gdev_cfgp cfgp;
	register gdev_dcbp dcbp;
	ushort  curdriv;
	ushort  firstminor;
	static 	short dcd_inited = 0;
	struct 	gdev_driver *drvp;
	void	dcd_cstart();
	int 	target_id;
	int	c = 0;	/* cntrl # (i.e. support for only one dcd instance)*/
	int	hba_no;
	int	sleepflag;
	uint	bus_p;

	if(dcd_inited) {
		cmn_err(CE_WARN,"DCD Already Initialized.\n");
		return(-1);
	}

	if (drv_gethardware(IOBUS_TYPE, &bus_p) == -1) {
		/*
		 *+ dcd: cannot determine bus type at start time
		 */
		cmn_err(CE_WARN,
		"dcd: drv_gethardware(IOBUS_TYPE) fails");
		return(-1);
	}


	/* link the drq_entries together and set up freelist.  */
	dcd_ver.sv_release = 1;
	if (bus_p == BUS_EISA)
		dcd_ver.sv_machine = SDI_386_EISA;
	else if (bus_p == BUS_MCA)
		dcd_ver.sv_machine = SDI_386_MCA;
	else
		dcd_ver.sv_machine = SDI_386_AT;
	dcd_ver.sv_modes = SDI_BASIC1;

	/* initialize dcd_ctoi map */
	for (i = 0; i < MAX_HAS; i++) {
		dcd_ctoi[i] = -1;
	}
	for (i = 0; i < MAX_HAS * MAX_TCS; i++) {
		dcd_ftoi[i] = -1;
	}
	dcd_inited = 1;

	/* Announce major number to the world and reserve full 8 minormaps */
	firstminor = gdev_majorset(DISK_MAJOR,8);
	sleepflag = mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/* loop through the entries in disk_cfg_tbl, calling all the BINIT 
	   functions.  */
	cfgp = &device_cfg_tbl[0];
	for (i=0; i<(int)device_cfg_entries; cfgp++, i++) {
		int numdrv;

		dcbp = gdev_shrdcb(cfgp);  /* get a dcb for ourselves */
		gdev_filldcb(dcbp, cfgp, (int (*)())dcd_cstart,
					 (int (*)())gdev_cplerr); /* init dcb */
		/* call the board initialization routine */
		numdrv = (cfgp->cfg_BINIT)(cfgp, dcbp);
		if (numdrv == 0) {       
		/* No drives on this controller -- unhook from dcb */
			if (--(dcbp->dcb_drivers) == 0) {       
			/* no other drivers -- give back dcb */
				gdev_nextctl -= 1;
			}
			continue;       /* try next controller */
		}
		gdev_setintr(dcbp, cfgp);
		dcb_drives(dcbp) = (ushort) numdrv;  /* Number found by low-level code */
	    	/* We own controller. */
		dcbp->dcb_flags = (CFLG_EXCL | CFLG_INIT); 

		if(dcd_drivecnt == 0) {
			/* First drive detected... get an HBA number
			 * from SDI for registry of DCD later.
			 */
			if( (hba_no = sdi_gethbano( dcd_idata[c].cntlr )) <= -1) {
	     			cmn_err (CE_NOTE,"DCD: No HBA number available.\n");
				continue;
			}
			else	{
				dcd_idata[c].cntlr = hba_no;
			}
			dcd_ctoi[hba_no] = (short) hba_no;
		}

		/* set up each gdev_parm_block for the dcb. */
		target_id = 0;

		for (curdriv=0; (int)curdriv < numdrv; curdriv++) {

			struct minormap *mp;
			register gdev_dpbp dpbp;
			
			if( dcb_baseminor(dcbp) != 0 ) {
				/***
				** We're being told what base Target ID
				** should be used for this device.
				***/
				mp = &minormap[firstminor+dcb_baseminor(dcbp)+curdriv];
				if (mp->cnf_valid) {
#ifdef DEBUG
					cmn_err( CE_WARN, "DCD_INIT: Multiple devices using Target ID %d, skipping device.",target_id);
#endif
					continue;
				}
				dcd_ftoi[(MAX_TCS * hba_no) + dcb_baseminor(dcbp) + curdriv] = (firstminor + dcb_baseminor(dcbp) + curdriv) << 4;
			}
			else {
				/***
				** Use the next available Target ID.
				***/
				for( ; target_id < GDEV_MINORMAPS; ++target_id ) {
					mp = &minormap[firstminor+target_id];
					if (mp->cnf_valid != 1) {
						break;
					}
				}
				if( target_id == GDEV_MINORMAPS ) {
#ifdef DEBUG
					cmn_err( CE_WARN, "DCD_INIT: Out of minormap entries, device skipped." );
#endif
					continue;
				}
				dcd_ftoi[(MAX_TCS * hba_no) + target_id] = (firstminor + target_id) << 4;
			}
			mp->cnf_valid = 1;
			mp->cnf_bd = dcbp - gdev_ctl_blocks;
			mp->cnf_drv = dcb_firstdriv(dcbp) + curdriv;
			mp->cnf_drvidx = dcbp->dcb_driveridx;
			mp->cnf_dcb = dcbp;
			/* Get our dpb. */
			dpbp = &gdev_parm_blocks[gdev_nextdriv++];
			bzero ((char *)dpbp,sizeof(struct gdev_parm_block));
			dcbp->dcb_dpbs[dcb_firstdriv(dcbp)+curdriv] = dpbp;
			dpbp->dpb_que = NULL;
			dpbp->dpb_nextque = NULL;
			dpbp->dpb_secsiz = dcbp->dcb_defsecsiz; /* default */
			/* Initialize the drive and the dpb. */
			dcb_curdriv(dcbp) = curdriv;
			(cfgp->cfg_DINIT)(dcbp,dpbp);
			if (dpbp->dpb_flags & DFLG_OFFLINE)
				continue;      /* bad init skip drive */
			/* get the scratch sector buffers if needed */
			if ((dcbp->dcb_capab & (CCAP_DMA | CCAP_SCATGATH))
			 	== CCAP_DMA) {       /* need 'em */
				char *bufptr = (char *)kmem_alloc(dpbp->dpb_secsiz, sleepflag);
				if (bufptr == NULL) {
					cmn_err(CE_WARN,"DCD Driver initialization failure, can't alloc buffer");
					continue;
				}
				dpbp->dpb_secbuf[0] = bufptr;
				dpbp->dpb_secbufsiz = dpbp->dpb_secsiz;
				if (dcbp->dcb_capab & CCAP_CHAINSECT) {
					/* Need 2 different buffers */
					bufptr = (char *)kmem_alloc(dpbp->dpb_secsiz, sleepflag);
					if (bufptr == NULL) {
						cmn_err(CE_WARN,"DCD Driver initialization failure, can't alloc buffer");
						kmem_free(dpbp->dpb_secbuf[0],
							  dpbp->dpb_secsiz);
						continue;
					}
				}
				dpbp->dpb_secbuf[1] = bufptr;
			}
			/* re-program controller with (possibly new) values */
			(dcb_CMD(dcbp))(DCMD_SETPARMS,dcbp,dpbp);
			dcd_drivecnt++;
		}
		/* Release exclusive lock on controller */
		dcbp->dcb_flags = CFLG_INITDONE;
		dcbp->dcb_hbano = hba_no;
		dcd_hacnt++;
	}
	if(dcd_hacnt) {
		dcd_cnt++;
		/* Register HBA with SDI */

		dcd_idata[c].version_num = 
			(dcd_idata[c].version_num & ~HBA_VMASK) | PDI_VERSION;
		if( (hba_no = sdi_register(&dcd_hba_info, &dcd_idata[c])) < 0) {
	     		cmn_err (CE_WARN,"DCD: HA %d, SDI registry failure %d.\n",
				c, hba_no);
		}
	} else {
	     	cmn_err (CE_NOTE,"!DCD: No HA's found.");
		return -1;
	}

	if ((dcd_hacnt==1) && !dcd_sngctlp) {
		dcbp = gdev_ctl_blocks;	
		drvp = dcbp->dcb_drv;
		if (!(dcbp->dcb_capab & CCAP_MULTI) && (dcbp->dcb_drivers==1) &&
			!drvp->drv_INT[1]) {
			dcd_sngctlp = &dcd_sngctl;
			dcd_sngctlp->s_dcbp = dcbp;
			dcd_sngctlp->s_drvp = drvp;
			dcd_sngctlp->s_idx  = 0;
			dcd_sngctlp->s_drvint = drvp->drv_drvint;
			dcd_sngctlp->s_drv_INT = drvp->drv_INT[0];
			if (drvp->drv_drives==1)
				dcd_sngctlp->s_dpbp = dcbp->dcb_dpbs[0];
			else
				dcd_sngctlp->s_dpbp = NULL;
		}
	} else if (dcd_hacnt!=1) 
		dcd_sngctlp = NULL;

	return 0;
}

void
dcd_intr(level)
int level;
{
	register struct gdev_int_entry *intp; 
	register gdev_dcbp dcbp; 
	register struct gdev_driver *drvp;
	ushort oldidx;
	int idx;
	int i;
	gdev_dpbp dpbp;

/* 	Call all interrupt handlers for this level in turn 		*/
	intp = gdev_int_routines[level];
	while (intp) {
		dcbp = intp->int_ctl;
		idx = intp->int_idx;
		oldidx = dcbp->dcb_driveridx;   /* save for intr code 	*/
		dcbp->dcb_driveridx = 0;
		if (dcbp->dcb_capab & CCAP_MULTI) {
/*
 * 			Multi-thread controller type -- just call
 * 			master interrupt handler if not null
 */
			if ( dcbp->dcb_multint == NULL )
				cmn_err(CE_WARN,"!DCD: NULL master interrupt routine");
			else
				(dcbp->dcb_multint)(dcbp,idx);
		} else { /* single-thread controller. Call all handlers.*/
			drvp = dcbp->dcb_drv;
			i = dcbp->dcb_drivers;

			/* call each interrupt handler. */
			for ( ; i>0; i--,drvp++,dcbp->dcb_driveridx++) {       
				if (drvp->drv_INT[idx] != NULL) {       
					dpbp=(drvp->drv_INT[idx])(dcbp,idx);
					if(dpbp && (dpbp->dpb_intret !=
						DINT_CONTINUE)) {       
						/* Call driver completion */
						(drvp->drv_drvint)(dcbp,dpbp);
					}
				}
			}
		}
		dcbp->dcb_driveridx = oldidx;   /* restore driveridx 	*/
		intp = intp->int_link;
	}
}
/*
** Function name: dcb_getblk()
** Description:
**	Allocate a SB structure for the caller.  sdi_get will
**	sleep if there are no dyn structs available.
*/

struct hbadata *
dcd_getblk(sleepflag)
{
	dcdblk_t  *sp;

	sp = (dcdblk_t *)sdi_get(&sm_poolhead, sleepflag);
	return ((struct hbadata *)sp);
}


/*
** Function name: dcd_freeblk()
** Description:
**	Release previously allocated SB structure. If a scatter/gather
**	list is associated with the SB, it is freed via dma_freelist().
**	A nonzero return indicates an error in pointer or type field.
*/

long
dcd_freeblk(hbap)
register struct hbadata *hbap;
{
	register dcdblk_t *sp = (dcdblk_t *) hbap;

	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return (SDI_RET_OK);
}

/*
**  Function name: dcd_xlat()
** Description:
*/
/* ARGSUSED */
int
dcd_xlat(hbap, flag, procp, sleepflag)
register struct  hbadata *hbap;
int flag;
struct proc *procp;
int sleepflag;
{
	dcdblk_t *dcdp = (dcdblk_t *) hbap;

	dcdp->b_flags = flag;	/* we will need to know which flags are set */
	dcdp->b_procp = procp;
	return (SDI_RET_OK);
}


/*
** Function name: dcd_getinfo()
** Description:
**	Return the name and iotype of the given device.  The name is copied
**	into a string pointed to by the first field of the getinfo structure.
*/

void
dcd_getinfo(sa, getinfo)
struct scsi_ad *sa;
struct hbagetinfo *getinfo;
{
	register char  *s1, *s2;
	static char temp[] = "HA X TC X";
	gdev_dcbp dcbp; 
	dev_t dev;

	s1 = temp;
	s2 = getinfo->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;
	
	dev = fillp2devt(sa);
	dcbp = dcbptr(dev);
	getinfo->iotype = 0;
	if(dcbp) {
		if (dcbp->dcb_capab & CCAP_DMA) {
			getinfo->iotype |= F_DMA_24;
#ifndef PDI_SVR42
			if (getinfo->bcbp) {
				getinfo->bcbp->bcb_flags = 0;
				getinfo->bcbp->bcb_physreqp->phys_align = 512;
				getinfo->bcbp->bcb_physreqp->phys_boundary = 64 * 1024;
				getinfo->bcbp->bcb_physreqp->phys_dmasize = 24;
			}
#endif /* PDI_SVR42 */
		}
		if (dcbp->dcb_capab & CCAP_SCATGATH) {
			getinfo->iotype |= F_SCGTH;
#ifndef PDI_SVR42
			if (getinfo->bcbp) {
				getinfo->bcbp->bcb_flags = 0;
				getinfo->bcbp->bcb_physreqp->phys_align = 512;
				getinfo->bcbp->bcb_physreqp->phys_boundary = 64 * 1024;
				getinfo->bcbp->bcb_physreqp->phys_dmasize = 24;
			}
#endif /* PDI_SVR42 */
		}
		if (dcbp->dcb_capab & CCAP_PIO)
			getinfo->iotype |= F_PIO;
#ifndef PDI_SVR42
		if (getinfo->bcbp) {
			getinfo->bcbp->bcb_addrtypes = BA_KVIRT;
			getinfo->bcbp->bcb_max_xfer = dcd_hba_info.max_xfer;
		}
#endif
	}
}

/*
** Function name: dcd_send()
** Description:
** 	Send a SCSI command to a controller.  Commands sent via this
**	function are executed in the order they are received.
*/

long
dcd_send(hbap, sleepflag)
register struct hbadata *hbap;
int sleepflag;
{
	dcdblk_t *dcdp = (dcdblk_t *) hbap;
	
	if (comm_cmd(dcdp, sleepflag))
		return (SDI_RET_ERR);

	return (SDI_RET_OK);
}


/*
** Function name: dcd_icmd()
** Description:
**	Send an immediate command.  If the logical unit is busy, the job
**	will be queued until the unit is free.  SFB operations will take
**	priority over SCB operations.
*/

long
dcd_icmd(hbap, sleepflag)
register struct  hbadata *hbap;
int sleepflag;
{
	dcdblk_t *dcdp = (dcdblk_t *) hbap;

	if (comm_cmd(dcdp, sleepflag))
		return (SDI_RET_ERR);

	return (SDI_RET_OK);
}


int
comm_cmd(dcdp, sleepflag)
register dcdblk_t  *dcdp;
{
	register struct  sb *sb = (struct sb *)dcdp->sb;
	dev_t dev;
	gdev_dpbp dpbp;
	gdev_dcbp dcbp;
	struct blklen *blklenptr;

	switch (sb->sb_type)
	{
	case SFB_TYPE:
		switch(sb->SFB.sf_func){
		case SFB_NOPF:
			sb->SFB.sf_comp_code = SDI_ASW;
			/* FALLTHRU */
		case SFB_ABORTM:
			/* FALLTHRU */
		case SFB_RESETM:
			dev = fill2devt(sb->SFB.sf_dev);
			dcbp = dcbptr(dev);
			dpbp = dpbptr(dcbp,dev);
			if( dpbp->dpb_devtype == DTYP_TAPE ) {
				if (dcd_sfb_cmd(dcdp, DCMD_RESET, sleepflag))
					return (-1);
			}
			else {
				sb->SFB.sf_comp_code = SDI_SFBERR;
			}
			break;
		case SFB_FLUSHR:
			dcd_flushr(dcdp);
			break;
		case SFB_SUSPEND:
			dcd_suspend(dcdp);
			break;
		case SFB_RESUME:
			dcd_resume(dcdp);
			break;
		default:
			sb->SFB.sf_comp_code = SDI_SFBERR;
		}
		sdi_callback(sb);
		break;
	case ISCB_TYPE:
	case SCB_TYPE:
		sb->SCB.sc_comp_code = SDI_PROGRES;
		sb->SCB.sc_status = S_GOOD;  

		/* figure out if its a 6 or 10 byte SCSI command */
		if(sb->SCB.sc_cmdsz == SCS_SZ)
		{
			struct scs *scsp;
			scsp = (struct scs *) (void *) sb->SCB.sc_cmdpt;
			switch(scsp->ss_op){
			
			case SS_REWIND:
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if ( dpbp->dpb_devtype == DTYP_TAPE ) {
				  if (dcd_scb_cmd(dcdp, DCMD_REWIND, sleepflag))
					return (-1);
				}
				else {
					sb->SCB.sc_comp_code = SDI_ERROR;
					sdi_callback(sb);
				}
				break;
			case SS_ERASE:
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if( dpbp->dpb_devtype == DTYP_TAPE ) {
				  if (dcd_scb_cmd(dcdp, DCMD_ERASE, sleepflag))
					return (-1);
				}
				else {
					sb->SCB.sc_comp_code = SDI_ERROR;
					sdi_callback(sb);
				}
				break;
			case SS_FLMRK:
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if( dpbp->dpb_devtype == DTYP_TAPE ) {
					if( (int)scsp->ss_len <= 0 ) {
						sb->SCB.sc_comp_code = SDI_TCERR;
						sdi_callback(sb);
					}
					else if (dcd_scb_cmd(dcdp, DCMD_WFM, sleepflag)) {
						return (-1);
					}
				}
				else {
					sb->SCB.sc_comp_code = SDI_ERROR;
					sdi_callback(sb);
				}
				break;
			case SS_SPACE:
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if( dpbp->dpb_devtype == DTYP_TAPE ) {
					if( scsp->ss_addr1 == FILEMARKS ) {
						if( (uint)scsp->ss_len == 0 ) {
							sb->SCB.sc_comp_code = SDI_ASW;
							sdi_callback(sb);
						}
						else if( (uint)scsp->ss_len > 125 ) {
							sb->SCB.sc_comp_code = SDI_TCERR;
							sdi_callback(sb);
						}
						else if(dcd_scb_cmd(dcdp,DCMD_SEOF, sleepflag)){
							return (-1);
						}
					} else {
						sb->SCB.sc_comp_code = SDI_TCERR;
						sdi_callback(sb);
					}
				}
				else {
					sb->SCB.sc_comp_code = SDI_ERROR;
					sdi_callback(sb);
				}
				break;
			case SS_LOAD:
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if( dpbp->dpb_devtype == DTYP_TAPE ) {
					switch (scsp->ss_len) {
					case LOAD:
						if (dcd_scb_cmd(dcdp, DCMD_LOAD, sleepflag)) {
							return (-1);
						}
						break;
					case UNLOAD:
						sb->SCB.sc_comp_code = SDI_ASW;
						sdi_callback(sb);
						break;
					case RETENSION:
						if (dcd_scb_cmd(dcdp, DCMD_RETENSION, sleepflag)) {
							return (-1);
						}
						break;
					}
				}
				else {
					sb->SCB.sc_comp_code = SDI_ERROR;
					sdi_callback(sb);
				}
				break;
			case SS_LOCK:
				sb->SCB.sc_comp_code = SDI_ERROR;
				sdi_callback(sb);

				break;
			case SS_RDBLKLEN:
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if( dpbp->dpb_devtype == DTYP_TAPE ) {
					blklenptr = (struct blklen *) (void *)(sb->SCB.sc_datapt);
					blklenptr->res1 =0;
					blklenptr->max_blen = sdi_swap24(512);
					blklenptr->min_blen = sdi_swap16(512);
					sb->SCB.sc_comp_code = SDI_ASW;
				}
				else {
					sb->SCB.sc_comp_code = SDI_ERROR;
				}

				sdi_callback(sb);

				break;
			case SS_TEST:	    /* Test unit ready       */
				dcd_test(dcdp);
				sdi_callback(sb);
				break;
			case SS_REQSEN:	    /* Request sense         */
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if( dpbp->dpb_devtype == DTYP_TAPE ) {
					if (dcd_scb_cmd(dcdp, DCMD_REQSEN, sleepflag)) {
						return (-1);
					}
				}
				else {
					dcd_reqsen(dcdp);
					sdi_callback(sb);
				}
				break;
			case SS_READ:	    /* Read                  */
			case SS_WRITE:	    /* Write                 */
				if (dcd_sb2drq(dcdp, sleepflag))
					return (-1);
				break;
			case SS_INQUIR:	    /* Inquire               */
				dcd_inquir(dcdp);
				sdi_callback(sb);
				break;
			case SS_MSELECT:    /* Mode select           */
				dcd_mselect(dcdp);
				sdi_callback(sb);
				break;
			case SS_RESERV:	    /* Reserve unit          */
				sb->SCB.sc_comp_code = SDI_ASW;
				sdi_callback(sb);
				/* LFH need to check if extent bits
      				 * which we don't support are set
				 */
				break;
			case SS_RELES:	    /* Release unit          */
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if( dpbp->dpb_devtype == DTYP_TAPE ) {
					if (dcd_scb_cmd(dcdp, DCMD_RELES, sleepflag)) {
						return (-1);
					}
				} else {
					sb->SCB.sc_comp_code = SDI_ASW;
					sdi_callback(sb);
				}
				break;
			case SS_MSENSE:	    /* Mode Sense            */
				sb = &dcdp->sb->sb;
				dev = fill2devt(sb->SCB.sc_dev);
				dcbp = dcbptr(dev);
				dpbp = dpbptr(dcbp,dev);
				if( dpbp->dpb_devtype == DTYP_TAPE ) {
					if (dcd_scb_cmd(dcdp, DCMD_MSENSE, sleepflag)) {
						return (-1);
					}
				}
				else {
					dcd_msense(dcdp);
					sdi_callback(sb);
				}
				break;
			case SS_SDDGN:	    /* Send diagnostic       */
				sb->SCB.sc_comp_code = SDI_ASW;
				sdi_callback(sb);
				/*LFH need to check the bits */
				break;
			case SS_REASGN:	    /* Reassign blocks      */
				dcd_reasgn(dcdp);
				sdi_callback(sb);
				break;
			case 0x04:		/* format */
				dcd_format(dcdp);
				sdi_callback(sb);
				break;
			default:
				/*lfh do something here */
				sb->SCB.sc_comp_code = SDI_ERROR;
				sdi_callback(sb);
				break;
			}
		}
		else if (sb->SCB.sc_cmdsz == SCM_SZ){
			struct scm *scm;
			scm = (struct scm *) (void *)(SCM_RAD(sb->SCB.sc_cmdpt));
			switch(scm->sm_op){
			case  SM_RDCAP:		/* Read capacity      */
				dcd_rdcap(dcdp);
				sdi_callback(sb);
				break;
			case  SM_READ:		/* Read extended      */
			case  SM_WRITE:		/* Write extended     */
				if (dcd_sb2drq(dcdp, sleepflag))
					return (-1);
				break;
			case  SM_SEEK:		/* Seek extended      */
				sb->SCB.sc_comp_code = SDI_ASW;
				sdi_callback(sb);
				break;
			case  0x2fL:		/* Verify command     */
				dcd_verify(dcdp);
				sdi_callback(sb);
				break;
			case  SM_RDDL:		/* Read defect list   */
			case  SM_RDDB:		/* Read data buffer   */
			case  SM_WRDB:		/* Write data buffer  */

			case  SD_ERRHEAD:	/* Expected error code */
			case  SD_ERRHEAD1:	/* Deferred expected err code */
				sb->SCB.sc_comp_code = SDI_ASW;
				sdi_callback(sb);
				break;
			default:
				sb->SCB.sc_comp_code = SDI_TCERR;
				sdi_callback(sb);
				break;
				/*lfh do something here */
			}
		}
		else{
			sb->SCB.sc_comp_code = SDI_TCERR;
			sdi_callback(sb);
			/* error */
		}
		break;
	default:
		break;
		
	}
	return (0);
}

int
dcd_scb_cmd(dcdp, dcd_cmd, sleepflag)
	dcdblk_t *dcdp;
	int	dcd_cmd;
	int	sleepflag;
{
	pl_t	ospl;
	int	callback = 0;

	dev_t dev;

	gdev_dcbp dcbp;
	gdev_dpbp dpbp;

	struct drq_entry *drqp;
	struct scs *scs;
	struct sb *sb = &dcdp->sb->sb;

	dev = fill2devt(sb->SCB.sc_dev);

	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dcbp->dcb_flags |= CFLG_BUSY;
	dpbp = dpbptr(dcbp,dev);

	if ((drqp = getdrq(sleepflag)) == NULL)
		return (-1);

	if( dcd_cmd == DCMD_REQSEN ) 
		drqp->drq_addr1 = (ulong)sb->SCB.sc_wd;
	else
		drqp->drq_addr1 = (ulong)sb->SCB.sc_datapt;

	drqp->drq_type = DRQT_TPCMD;
	dpbp->dpb_req = drqp;
	set_drq_bufp (drqp, dcdp);

	if( dcd_cmd == DCMD_WFM ||
	    dcd_cmd == DCMD_SEOF ) {
		scs = (struct scs *) (void *)sb->SCB.sc_cmdpt;
		drqp->drq_count = (uint)scs->ss_len;
	}
	else {
		drqp->drq_count = 0;
	}

	/* save time when command was started */
	drv_getparm(LBOLT, &dcbp->dcb_laststart);
	if( (dcb_CMD(dcbp))(dcd_cmd,dcbp,dpbp) == 0 ) {
		sb->SCB.sc_comp_code = SDI_ASW;
	}
	else {
		sb->SCB.sc_comp_code = SDI_ERROR;
		callback = 1;
	}

	if( dcd_cmd == DCMD_MSENSE ||
	    dcd_cmd == DCMD_LOAD ||
	    dcd_cmd == DCMD_REQSEN ||
	    dcd_cmd == DCMD_RELES ) {

		/***                                           ****
		** These commands do not generate an interrupt,  **
		** thus they do not pass through the "normal"    **
		** gendev logic that would free the drq's, and   **
		** clear CFLG_BUSY. Therefore we free them here. **
		****                                           ***/

		callback = 1;
		ospl = SPLDISK;
		dpbp->dpb_req = reldrq( drqp );
		splx( ospl );
		dcbp->dcb_flags &= ~CFLG_BUSY;
	}
	if (callback)
		sdi_callback(sb);
	return (0);
}

int
dcd_sfb_cmd(dcdp, dcd_cmd, sleepflag)
	dcdblk_t *dcdp;
	int	dcd_cmd;
	int	sleepflag;
{
	pl_t	ospl;

	dev_t dev;

	gdev_dcbp dcbp;
	gdev_dpbp dpbp;

	struct drq_entry *drqp;
	struct sb *sb = &dcdp->sb->sb;

	dev = fill2devt(sb->SFB.sf_dev);

	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dcbp->dcb_flags |= CFLG_BUSY;
	dpbp = dpbptr(dcbp,dev);
	dpbp->dpb_sectcount = 0;

	if ((drqp = getdrq(sleepflag)) == NULL)
		return (-1);

	drqp->drq_type = DRQT_TPCMD;
	dpbp->dpb_req = drqp;
	set_drq_bufp (drqp, dcdp);

	/* save time when command was started */
	drv_getparm(LBOLT, &dcbp->dcb_laststart);
	if( (dcb_CMD(dcbp))(dcd_cmd,dcbp,dpbp) == 0 ) {
		sb->SFB.sf_comp_code = SDI_ASW;
	}
	else {
		sb->SFB.sf_comp_code = SDI_SFBERR;
	}

	if( dcd_cmd == DCMD_RESET ) {

		/***                                           ****
		** These commands do not generate an interrupt,  **
		** thus they do not pass through the "normal"    **
		** gendev logic that would free the drq's, and   **
		** clear CFLG_BUSY. Therefore we free them here. **
		****                                           ***/

		ospl = SPLDISK;
		dpbp->dpb_req = reldrq( drqp );
		splx( ospl );
		dcbp->dcb_flags &= ~CFLG_BUSY;
	}
	return (0);
}

int
dcd_sb2drq(dcdp, sleepflag)
dcdblk_t *dcdp;
int sleepflag;
{
	register gdev_dcbp dcbp;
	register gdev_dpbp dpbp;
	struct  drq_entry memhead;      /* for memory section chain */
	struct  drq_entry diskhead;     /* for disk section chain */
	struct drq_entry *drqp;         /* temp drq for creating chains */
	struct drq_entry *newdrqp;      /* temp drq for joining drqs */ 
	struct  drq_entry *disktail = &diskhead;
	struct  drq_entry *memtail = &memhead;
	dcdblk_t *firstdcd = dcdp;      /* the one which goes in drive queue */
	dcdblk_t *insertque; 	/* temp queue ptr for queue insertion */
	dcdblk_t *next_dcdblk; 	/* temp queue ptr for queue insertion */
	ushort  secsiz;         /* local copy (saves indirect ptr ref) */
	ushort  driveridx;
	struct  sb     *sb = (struct sb *)dcdp->sb;
	dev_t   dev = fill2devt(sb->SCB.sc_dev);
	int direction;
	unsigned int sector;
	unsigned int count;
	ulong memaddr, vaddr;
	pl_t     oldpri;
	int 	dist1, dist2, cyl1, cyl2;

	memhead.drq_link = NULL;
	diskhead.drq_link = NULL;

	if(sb->SCB.sc_cmdsz == SCS_SZ){
		struct scs * scs;
		scs = (struct scs *) (void *)sb->SCB.sc_cmdpt;
		direction = scs->ss_op == SS_READ ? B_READ : B_WRITE;
		sector = sdi_swap16(scs->ss_addr) + ((scs->ss_addr1 << 16 ) & 0xFF);
		count = scs->ss_len;
	} else if (sb->SCB.sc_cmdsz == SCM_SZ){
		struct scm * scm;
		scm = (struct scm *) (void *)(SCM_RAD(sb->SCB.sc_cmdpt));
		direction = scm->sm_op == SM_READ ? B_READ : B_WRITE;
		sector = (unsigned int)sdi_swap32(scm->sm_addr);
		count = sdi_swap16(scm->sm_len);
	}else {
		return (-1);
	}

	if ((drqp = getdrq(sleepflag)) == NULL)
		return (-1);
	dcbp = dcbptr(dev);       /* get ctl blk ptr */
	driveridx = dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);  /* get drv parm ptr */
	secsiz = dpbp->dpb_secsiz;      /* save for later */
	drqp->drq_type = DRQT_START;    /* assume start of disk section */
	set_drq_daddr (drqp, sector);
	drqp->drq_count = count;
	set_drq_memaddr (drqp, sb->SCB.sc_datapt);
	set_drq_memaddr (&memhead, sb->SCB.sc_datapt);  
	set_drq_vaddr (drqp, sb->SCB.sc_datapt);
	set_drq_vaddr (&memhead, sb->SCB.sc_datapt);  
	disktail->drq_link = drqp;
	disktail = drqp;

	/* Get another drq entry for memory section for this buf */
	if ((drqp = getdrq(sleepflag)) == NULL) {
		reldrq(disktail);
		return (-1);
	}
	drqp->drq_type = DRQT_MEMBREAK;
	drqp->drq_count = sb->SCB.sc_datasz;
	set_drq_bufp (drqp, dcdp);
	dcbp->dcb_driveridx = driveridx;  /* Might sleep in getdrq */
	memtail->drq_link = drqp;
	memtail = drqp;

	/*
	 * If the controller can't handle a request which spans a cylinder
	 * boundary, check for any such crossings as we may have and turn
	 * them into multiple sections.
	 */
	if (dcbp->dcb_capab & CCAP_CYLLIM)
	{       /* Don't let sections cross cyl boundaries */
		ushort  cylsiz = dpbp->dpb_sectors * dpbp->dpb_heads;
		for (drqp=diskhead.drq_link; drqp; drqp=drqp->drq_link)
		{
			register ulong firstsec = (ulong)drq_daddr(drqp);
			register ulong lastsec = firstsec + (drqp->drq_count - 1);
			while ((firstsec/cylsiz) != (lastsec/cylsiz))
			{       /* Break this one up */
				ushort oldsiz = cylsiz - (firstsec % cylsiz);

				if ((disktail = getdrq(sleepflag)) == NULL) {
					dcd_drqclean(diskhead.drq_link);
					dcd_drqclean(memhead.drq_link);
					return (-1);
				}
				dcbp->dcb_driveridx = driveridx;
				disktail->drq_type = DRQT_START; /* new cyl */
				disktail->drq_link = drqp->drq_link;
				drqp->drq_link = disktail;
				set_drq_memaddr (disktail,
				    (ulong)(drq_memaddr(drqp)) +
				    (oldsiz * secsiz));
				set_drq_vaddr (disktail,
				    (ulong)(drq_memaddr(drqp)) +
				    (oldsiz * secsiz));
				set_drq_daddr (disktail, drq_daddr(drqp) +
				    oldsiz);
				disktail->drq_count = drqp->drq_count - oldsiz;
				drqp->drq_count = oldsiz;
				firstsec += oldsiz;
				drqp = disktail;
			}
		}
	}

	/*
	 * Make sure no disk section is larger than the maximum count allowed
	 * by the controller...
	 */
	if (count > dcbp->dcb_maxsec)
		if (gdev_count_breakup(dcbp,&diskhead,secsiz,sleepflag) == -1) {
			dcd_drqclean(diskhead.drq_link);
			dcd_drqclean(memhead.drq_link);
			return (-1);
		}
	/*
	 * We need to look through our memory section chain and break it up
	 * at any discontiguous page boundaries.
	 * (We'll also look for 64-K physical memory boundaries and break
	 * there if CCAP_16BIT is set).
	 * If we're NOT doing physical I/O, then xfer is to buffer space and
	 * we can do a much simpler version of this (unless CCAP_16BIT set).
	 */
	if ((dcbp->dcb_capab & (CCAP_DMA | CCAP_SCATGATH)) == CCAP_DMA) {
		if (gdev_mem_breakup(firstdcd->b_flags,firstdcd->b_procp, dcbp,
		    &memhead,sleepflag) == -1) {
			dcd_drqclean(diskhead.drq_link);
			dcd_drqclean(memhead.drq_link);
			return (-1);
		}
	}
	/*
	 * Now we merge the disk and memory chains into a single request
	 * chain.  This will be left in 'diskhead'.  How we do this depends
	 * on the settings of CCAP_DMA and CCAP_SCATGATH.  If DMA and not
	 * SCATGATH, we build a chain which includes MEMXFER-type entries.
	 * Work in this type of chain causes all activity to be sector-
	 * aligned.  There will be no MEMBREAK entries in such a chain
	 * except for pointing at buffer headers which get completed.
	 * For a SCATGATH (or non-DMA) chain, we work at the byte level.
	 * MEMBREAK entries are inserted into the START/CONT chain as
	 * needed to handle memory discontinuities.
	 */

	if (gdev_sect_merge(dcbp,dpbp,&diskhead,&memhead,direction, 
	    firstdcd->b_procp, sleepflag) == -1) {
		dcd_drqclean(diskhead.drq_link);
		dcd_drqclean(memhead.drq_link);
		return (-1);
	}
	/*
	 * We now loop through our disk sections, physicalizing the virtual
	 * addresses.
	 */

	for (drqp=diskhead.drq_link; drqp; drqp=drqp->drq_link)
	{
		if (!((drqp->drq_type == DRQT_START) || 
			(drqp->drq_type == DRQT_CONT))) {
			firstdcd->drq_end = drqp;
			continue;       /* not a disk section 		*/
		}
		if(drqp->drq_phys) {
			continue;
		}
#ifdef PDI_SVR42
		if (drq_memaddr(drqp) < KVBASE)  {
			vaddr = vtop((caddr_t)drq_memaddr(drqp),
				    firstdcd->b_procp) + KVBASE;
			set_drq_vaddr(drqp, vaddr);
		} else
#endif /* PDI_SVR42 */
			set_drq_vaddr(drqp, drq_memaddr(drqp));
		set_drq_memaddr(drqp, vtop((caddr_t)drq_memaddr(drqp),
			firstdcd->b_procp));
		drqp->drq_phys = 1;
	}
	/*
	 * Hang the (completed) request chain to firstdcd's drq_srt link
	 * and append the dcd block to the drive's queue. 
	 */
	firstdcd->drq_srt = (struct drq_entry *)diskhead.drq_link;   
	firstdcd->av_forw = NULL;
	oldpri = SPLDISK;       

	/* dcdblk (firstdcd) now has drq chains appended so we */
	/* now add it to the appropriate queue. We're using an */
	/* ascending queue, with dpb_que is the list of jobs   */
	/* currently being done, and dpb_nextque is the start  */
	/* next pass. If jobs are queued, dpb_que is non-Null, */
	/* dpb_nextque may or may not have values. The idea is */
	/* to get the job on a queue as quickly as possible    */
	/* disk interrupts are blocked.                        */

	if ((dpbp->dpb_que == NULL) || (sb->sb_type == ISCB_TYPE)) {
		if (dpbp->dpb_que != NULL)
			firstdcd->av_forw = dpbp->dpb_que;
		dpbp->dpb_que = firstdcd; 
		dcbp->dcb_driveridx = 0;
		if (!(dcbp->dcb_flags & CFLG_EXCL) && 
		   !(dcbp->dcb_flags & CFLG_BUSY))
			(dcb_START(dcbp))(dcbp);  /* Start activity */
		splx(oldpri);
		return (0);
	} 
	else {       
	/*
	 * queue disk jobs using shortest seek time first algorithm.
	 *	Look at each job in the disk queue.  For each queued
	 *	job j1, compute d1, the distance in cylinders between j1
	 *	and the incoming job.  Also compute d2, the distance in
	 *	cylinders between the job j1 and the job j2 which follows
	 *	it on the queue.  If the first distance d1 is less than
	 *	the second distance d2, then add the new job to the queue
	 *	so that it immediately follows job j1.  If d1 is greater
	 *	than d2, then continue to examine the jobs in the queue.
	 *	If no queued job is found where d1 < d2, then the new job
	 *	ends up at the end of the queue.
	 *
	 * When the new job is inserted either before or after a job at
	 *	the same cylinder, the job is inserted so that jobs at
	 *	the same cylinder are serviced in order of increasing
	 *	sector number.
	 *
	 * In the code that follows:
	 *	insertque is job j1
	 *	next_dcdblk is job j2
	 *	cyl, cyl1, cyl2 are the cylinder numbers of the incoming job,
	 *		job j1, and job j2 respectively
	 *	sector, sec2 are the sector numbers of the incoming job and
	 *		job j2 respectively
	 *	dist1 and dist2 are distances d1 and d2
	 */
		int cylsiz = dpbp->dpb_sectors * dpbp->dpb_heads;
		int cyl = sector / cylsiz;
		int sec2;

		insertque = dpbp->dpb_que;
		cyl1 = drq_daddr(insertque->drq_srt) / cylsiz;
		while ((next_dcdblk = insertque->av_forw) != NULL) {
			cyl2 = drq_daddr(next_dcdblk->drq_srt) / cylsiz;
			if (cyl == cyl2) {

			/*
			 * The new job is at same cylinder as next_dcdblk, so
			 * continue through queue, until any of the following
			 * are true:
			 *	(1) next job is not at same cylinder,
			 *	(2) next job is at higher sector
			 *	(3) end of queue is hit
			 */
				do {
					sec2 = drq_daddr(next_dcdblk->drq_srt);
					if ((sec2 > sector) ||
							((sec2 / cylsiz) != cyl))
						break;
					insertque = next_dcdblk;
					next_dcdblk = insertque->av_forw;
				} while (next_dcdblk != NULL);
				break;
			}

			/*
			 * compute and compare distances; break out of the
			 *	loop if dist1 is less than dist2
			 */
			if ((dist1 = (cyl1 - cyl)) < 0)
				dist1 = -dist1;
			if ((dist2 = (cyl1 - cyl2)) < 0)
				dist2 = -dist2;
			if (dist1 < dist2)
				break;
			insertque = next_dcdblk;
			cyl1 = cyl2;
		}

		drqp = insertque->drq_srt;
		if ((dcbp->dcb_capab&(CCAP_DMA|CCAP_SCATGATH) != CCAP_DMA) &&
		   (firstdcd->b_flags&B_READ)==(insertque->b_flags&B_READ) &&
		   ((drq_daddr(drqp) + drqp->drq_count) == sector) &&
		   ((drqp->drq_count + count) < dcbp->dcb_maxsec)) {
			/* join firstdcd at end of insertque */
			newdrqp = firstdcd->drq_srt;
			drqp->drq_count += count;
			memaddr = drq_memaddr(newdrqp);
			vaddr = drq_vaddr(newdrqp);
			newdrqp = reldrq(newdrqp);
			drqp = insertque->drq_end;
			set_drq_memaddr(drqp,memaddr);
			set_drq_vaddr(drqp,vaddr);
			drqp->drq_link = newdrqp;
			insertque->drq_end = firstdcd->drq_end;
		}
		else {
			if ((dcbp->dcb_capab&(CCAP_DMA|CCAP_SCATGATH) != CCAP_DMA) &&
			   (next_dcdblk != NULL) &&
			   ((drqp = next_dcdblk->drq_srt) != NULL) &&
			   ((firstdcd->b_flags&B_READ)==(next_dcdblk->b_flags&B_READ)) &&
			   ((sector+count)== drq_daddr(drqp)) &&
		   	   ((drqp->drq_count + count) < dcbp->dcb_maxsec)) {
				/* join firstdcd at front of next_dcdblk */
				newdrqp = firstdcd->drq_srt;
				newdrqp->drq_count += drqp->drq_count;
				memaddr = drq_memaddr(drqp);
				vaddr = drq_vaddr(drqp);
				drqp = reldrq(drqp);
				newdrqp = firstdcd->drq_end;
				set_drq_memaddr(newdrqp,memaddr);
				set_drq_vaddr(newdrqp,vaddr);
				newdrqp->drq_link = drqp;
				next_dcdblk->drq_srt = firstdcd->drq_srt;
			}
			else {
			/* Noncontiguous or different type (read vs. write) */
				insertque->av_forw = firstdcd;
				if (next_dcdblk != NULL)
					firstdcd->av_forw = next_dcdblk;
			}
		}
	}
	dcbp->dcb_driveridx = 0;
	if (!(dcbp->dcb_flags & CFLG_EXCL) && !(dcbp->dcb_flags & CFLG_BUSY))
		(dcb_START(dcbp))(dcbp);  /* Start activity if necessary */
	splx(oldpri);
	return (0);
}


void
dcd_reqsen(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	struct scs *scb = (struct scs *) (void *)sb->SCB.sc_cmdpt;
	dev = fill2devt(sb->SCB.sc_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SCB.sc_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SCB.sc_comp_code = SDI_SCBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	bcopy(SENSE_AD(&dpbp->dpb_reqdata), sb->SCB.sc_datapt, scb->ss_len);
	dpbp->dpb_reqdata.sd_key = 0;
	dpbp->dpb_reqdata.sd_errc = 0x70;
	dpbp->dpb_reqdata.sd_len = 0xe;
	dpbp->dpb_reqdata.sd_valid = 0;
	dpbp->dpb_reqdata.sd_sencode = SC_NOSENSE;
	sb->SCB.sc_status = S_GOOD;  
	sb->SCB.sc_comp_code = SDI_ASW;
}

void
dcd_inquir(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	struct scs *scb = (struct scs *) (void *)sb->SCB.sc_cmdpt;
	int	i;

	/* dcbp ptr of first disk thru inq */

	dev = fill2devt(sb->SCB.sc_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (valid_dev(dev) && sb->SCB.sc_dev.sa_lun == 0) { 
		dcbp = dcbptr(dev);
		dcbp->dcb_driveridx = driver_idx(dev);
		dpbp = dpbptr(dcbp,dev);
		bcopy((char *)&dpbp->dpb_inqdata, sb->SCB.sc_datapt , scb->ss_len);
		sb->SCB.sc_comp_code = SDI_ASW;
		return;
	}
	if ((SDI_TCN(&(sb->SCB.sc_dev)) == DCD_HA) && (sb->SCB.sc_dev.sa_lun == 0)) {
		/* make target 0 = disk */
		struct ident inq;
		struct ident *inq_data;
		int inq_len;
		struct scsi_ad *sa;

		sa = &sb->SCB.sc_dev;
		bzero(&inq, sizeof(struct ident));
		(void)strncpy(inq.id_vendor, dcd_idata[0].name, 
			VID_LEN+PID_LEN+REV_LEN);
		inq.id_type = ID_PROCESOR;

		inq_data = (struct ident *)(void *)sb->SCB.sc_datapt;
		inq_len = sb->SCB.sc_datasz;
		bcopy((char *)&inq, (char *)inq_data, inq_len);
		sb->SCB.sc_comp_code = SDI_ASW;
	}
	else	sb->SCB.sc_comp_code = SDI_SCBERR;
}


void
dcd_test(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	dev = fill2devt(sb->SCB.sc_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SCB.sc_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SCB.sc_comp_code = SDI_SCBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	if(dpbp->dpb_flags & DFLG_OFFLINE) {
		sb->SCB.sc_comp_code = SDI_CKSTAT;
		sb->SCB.sc_status = SD_UNATTEN;
	} else
		sb->SCB.sc_comp_code = SDI_ASW;
}


void
dcd_mselect(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	struct scs *cmd = (struct scs *) (void *)sb->SCB.sc_cmdpt;
	char *page;
	struct  {
		ulong   cyls;
		ushort  heads;
		ushort  sectors;
		ushort  secsiz;
		ushort  flags;
		} sav;
	pl_t     oldpri;
	SENSE_PLH_T *plh; 
	dev = fill2devt(sb->SCB.sc_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SCB.sc_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SCB.sc_comp_code = SDI_SCBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
/*
 * Save the existing drive
 * parameters in case reconfig at bottom fails.
 */
	sav.cyls = dpbp->dpb_cyls;
	sav.heads = dpbp->dpb_heads;
	sav.sectors = dpbp->dpb_sectors;
	sav.secsiz = dpbp->dpb_secsiz;
	sav.flags = dpbp->dpb_flags;
	plh = (SENSE_PLH_T *) (void *)(dcdblkp->sb->sb.SCB.sc_datapt);
	page = (char *) (dcdblkp->sb->sb.SCB.sc_datapt)
		+ SENSE_PLH_SZ + plh->plh_bdl;
	/* check to see which page code we have */
	if((cmd->ss_addr & 0x3f) == 0x00) {
		/* page 0 (vendor specific, in our case pdinfo ) */
		struct pdinfo *pdinfo =  (struct pdinfo *) (void *)page;
		dpbp->dpb_cyls = pdinfo->cyls;
		dpbp->dpb_sectors = pdinfo->sectors;
		dpbp->dpb_secsiz = pdinfo->bytes;
		dpbp->dpb_secovhd = pdinfo->secovhd;
		dpbp->dpb_pcyls = pdinfo->pcyls;
		dpbp->dpb_psectors = pdinfo->psectors;
		dpbp->dpb_pbytes = pdinfo->pbytes;
		dpbp->dpb_interleave = pdinfo->interleave;
		dpbp->dpb_skew = pdinfo->skew;
	}
	else 
		if((cmd->ss_addr & 0x3f) == 0x3) {
			/* page 3 */
			DADF_T *pg3 =  (DADF_T *) (void *)page;
			dpbp->dpb_sectors = pg3->pg_sec_t ;
		} else 
			if ((cmd->ss_addr & 0x3f) == 0x4) {
				/* page 4 */
				RDDG_T *pg4 =  (RDDG_T *) (void *)page;
				dpbp->dpb_cyls = (pg4->pg_cylu << 8) | pg4->pg_cyll;
				dpbp->dpb_heads = pg4->pg_head;
			}
	/*
 	 * Reconfigure the controller.
 	 * If all ok, return.  If not, restore old params and config again.
 	 */
	oldpri = SPLDISK;
	dcb_curdriv(dcbp) = unit(dev);  /* set us to proper drive */
	/* save time when command was started */
	drv_getparm(LBOLT, &dcbp->dcb_laststart);
	(dcb_CMD(dcbp))(DCMD_SETPARMS,dcbp,dpbp);
	if (sb->SCB.sc_comp_code != SDI_ASW) {       
		/* command failed.  Put it back... */
		dpbp->dpb_cyls = sav.cyls;
		dpbp->dpb_heads = sav.heads;
		dpbp->dpb_sectors = sav.sectors;
		dpbp->dpb_secsiz = sav.secsiz;
		dpbp->dpb_flags = sav.flags;
		/* save time when command was started */
		drv_getparm(LBOLT, &dcbp->dcb_laststart);
		(dcb_CMD(dcbp))(DCMD_SETPARMS,dcbp,dpbp);
	}
	splx(oldpri);
	return;

}



void
dcd_msense(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	struct scs *cmd = (struct scs *) (void *)sb->SCB.sc_cmdpt;
	struct mdata  mdata;
	dev = fill2devt(sb->SCB.sc_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SCB.sc_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SCB.sc_comp_code = SDI_SCBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	mdata.plh.plh_type = 0;
	mdata.plh.plh_wp = dpbp->dpb_flags & DFLG_REMOVE ? 1 : 0;
	mdata.plh.plh_bdl = 8;
	/* check to see which page code we have */
	if((cmd->ss_addr & 0x3f) == 0x00) {
		/* page 0 (vendor specific, in our case pdinfo ) */
		mdata.pdata.pg3.pg_pc = 0x0;
		mdata.pdata.pg3.pg_res1	= 0;
		mdata.pdata.pg3.pg_len = sizeof(struct pdinfo);
		mdata.pdata.pg0.cyls = dpbp->dpb_cyls;
		mdata.pdata.pg0.sectors = dpbp->dpb_sectors;
		mdata.pdata.pg0.bytes = dpbp->dpb_secsiz;
		mdata.pdata.pg0.secovhd = dpbp->dpb_secovhd;
		mdata.pdata.pg0.pcyls = dpbp->dpb_pcyls;
		mdata.pdata.pg0.psectors = dpbp->dpb_psectors;
		mdata.pdata.pg0.pbytes = dpbp->dpb_pbytes;
		mdata.pdata.pg0.interleave = dpbp->dpb_interleave;
		mdata.pdata.pg0.skew = dpbp->dpb_skew;
	}
	if((cmd->ss_addr & 0x3f) == 0x3) {
		/* page 3 */
		mdata.pdata.pg3.pg_pc = 0x3;
		mdata.pdata.pg3.pg_res1	= 0;
		mdata.pdata.pg3.pg_len = sizeof(DADF_T);
		mdata.plh.plh_len = 
			3 + mdata.plh.plh_bdl + mdata.pdata.pg3.pg_len;
		mdata.pdata.pg3.pg_trk_z = sdi_swap16(0);
		mdata.pdata.pg3.pg_asec_z = sdi_swap16(0);
		mdata.pdata.pg3.pg_atrk_z = sdi_swap16(0);
		mdata.pdata.pg3.pg_atrk_v = sdi_swap16(0);
		mdata.pdata.pg3.pg_sec_t = sdi_swap16(dpbp->dpb_sectors);
		mdata.pdata.pg3.pg_bytes_s = sdi_swap16(dpbp->dpb_secsiz);
		mdata.pdata.pg3.pg_intl	= sdi_swap16(dpbp->dpb_interleave);
		mdata.pdata.pg3.pg_trkskew = sdi_swap16(dpbp->dpb_skew);
		mdata.pdata.pg3.pg_cylskew = sdi_swap16(0);
		mdata.pdata.pg3.pg_res2	= 0;
		mdata.pdata.pg3.pg_ins	=0;
		mdata.pdata.pg3.pg_surf	= 0;
		mdata.pdata.pg3.pg_rmb	= dpbp->dpb_flags & DFLG_REMOVE ? 1 : 0;
		mdata.pdata.pg3.pg_hsec	= 0x0;
			mdata.pdata.pg3.pg_ssec	= 0x1;
	}else if ((cmd->ss_addr & 0x3f)  == 0x4) {
		/* page 4 */
		mdata.pdata.pg4.pg_pc  = 0x4;
		mdata.pdata.pg4.pg_res1	= 0;
		mdata.pdata.pg4.pg_len = sizeof(RDDG_T);
		mdata.plh. plh_len = 
			3 + mdata.plh. plh_bdl + mdata.pdata.pg3.pg_len;
		mdata.pdata.pg4.pg_cylu	= (dpbp->dpb_cyls>>8) & 0xFFFF;
		mdata.pdata.pg4.pg_cyll = dpbp->dpb_cyls & 0xFF;
		mdata.pdata.pg4.pg_head = dpbp->dpb_heads;
		mdata.pdata.pg4.pg_wrpcompu = (dpbp->dpb_wpcyl>>8) & 0xFFFF;
		mdata.pdata.pg4.pg_wrpcompl = dpbp->dpb_wpcyl & 0xFF;
		mdata.pdata.pg4.pg_redwrcur = sdi_swap16(dpbp->dpb_parkcyl); /* ? */
		mdata.pdata.pg4.pg_drstep = sdi_swap16(1); /* ???? */
		mdata.pdata.pg4.pg_landu = (dpbp->dpb_parkcyl>>8) & 0xFFFF;
		mdata.pdata.pg4.pg_landl = dpbp->dpb_parkcyl & 0xFF;
		mdata.pdata.pg4.pg_res2	= 0;
	} else {
		dpbp->dpb_reqdata.sd_key = SD_ILLREQ;
		dpbp->dpb_reqdata.sd_valid = 0;
		dpbp->dpb_reqdata.sd_sencode = SC_INVOPCODE;
		dpbp->dpb_reqdata.sd_errc = 0x70;
		dpbp->dpb_reqdata.sd_len = 0xe;
		sb->SCB.sc_comp_code = SDI_CKSTAT;
		sb->SCB.sc_status = S_CKCON;  
		return;
	}
	bcopy((char *) &mdata,sb->SCB.sc_datapt, cmd->ss_len);
	sb->SCB.sc_comp_code = SDI_ASW;
	return;
}

void
dcd_reasgn(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	int smx_addr;

	dev = fill2devt(sb->SCB.sc_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SCB.sc_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SCB.sc_comp_code = SDI_SCBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	smx_addr = ((int *) (void *)(sb->SCB.sc_datapt))[1];
	dpbp->dpb_reqdata.sd_ba = smx_addr;
	dpbp->dpb_reqdata.sd_sencode = SC_NODFCT;
	dpbp->dpb_reqdata.sd_key = SD_MEDIUM;
	dpbp->dpb_reqdata.sd_valid = 1;
	sb->SCB.sc_comp_code = SDI_CKSTAT;
	sb->SCB.sc_status = S_CKCON;  
}

void
dcd_format(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	struct scs_format *fmtcmd = (struct scs_format *)sb->SCB.sc_cmdpt;
	int error;
	
	dev = fill2devt(sb->SCB.sc_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SCB.sc_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SCB.sc_comp_code = SDI_SCBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	if ((dpbp->dpb_drvflags & DPCF_NOTRKFMT) )
		error = format_drive(dcbp, dpbp, dev, fmtcmd);
	else
		error = format_track(dcbp, dpbp, dev, fmtcmd);
	if (error) 
		sb->SCB.sc_comp_code = SDI_SCBERR;
	else
		sb->SCB.sc_comp_code = SDI_ASW;
}

int
format_track(dcbp, dpbp, dev, fmtcmd)
gdev_dcbp dcbp;
gdev_dpbp dpbp;
dev_t dev;
struct scs_format *fmtcmd;
{
	long c, h, s;
	ulong maxtrack;
	pl_t oldpri;
	int i;
	int	error = 0;

	c = (dpbp->dpb_pcyls ? dpbp->dpb_pcyls : dpbp->dpb_cyls)
							- dpbp->dpb_rescyls;
	h = dpbp->dpb_pheads ? dpbp->dpb_pheads : dpbp->dpb_heads;
	s = dpbp->dpb_psectors ? dpbp->dpb_psectors : dpbp->dpb_sectors;
	maxtrack = h * c;
	dpbp->dpb_interleave = 
		(fmtcmd->fmt_intlmsb << 8) & fmtcmd->fmt_intllsb;
	/*
	 * Get the controller and issue the format cmds.
	 */
	oldpri = SPLDISK;
	dcbp->dcb_driveridx = driver_idx(dev);
	dcb_curdriv(dcbp) = unit(dev);  /* set us to proper drive */
	dpbp->dpb_flags |= DFLG_SPECIAL;        /* special command */

	/* format the requested tracks. */
	for (i=0; i < maxtrack; i++) {
		dpbp->dpb_cursect = i * (s + dpbp->dpb_ressecs);
		/* save time when command was started */
		drv_getparm(LBOLT, &dcbp->dcb_laststart);
		if ((dcb_CMD(dcbp))(DCMD_FORMAT,dcbp,dpbp)) {
			error = 1;
			break;  /* command failed */
		}
	}
	dpbp->dpb_flags &= ~DFLG_SPECIAL;
	splx(oldpri);
	return(error);
}

int
format_drive(dcbp, dpbp, dev, fmtcmd)
gdev_dcbp dcbp;
gdev_dpbp dpbp;
dev_t dev;
struct scs_format *fmtcmd;
{
	pl_t oldpri;
	int error = 0;

	oldpri = SPLDISK;
	dpbp->dpb_flags |= DFLG_SPECIAL;
	dcb_curdriv(dcbp) = unit(dev);
	dpbp->dpb_interleave = 
		(fmtcmd->fmt_intlmsb << 8) & fmtcmd->fmt_intllsb;
	/* save time when command was started */
	drv_getparm(LBOLT, &dcbp->dcb_laststart);
	if((dcb_CMD(dcbp))(DCMD_FMTDRV,dcbp,dpbp)) 
		error = 1;
	dpbp->dpb_flags &= ~DFLG_SPECIAL;
	splx(oldpri);
	return(error);
}

void
dcd_rdcap(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	CAPACITY_T cap;

	dev = fill2devt(sb->SCB.sc_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SCB.sc_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SCB.sc_comp_code = SDI_SCBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	cap.cd_addr = sdi_swap32(dpbp->dpb_heads * dpbp->dpb_cyls * dpbp->dpb_sectors);
	cap.cd_len = sdi_swap32(dpbp->dpb_secsiz);
	bcopy((char *)&cap, sb->SCB.sc_datapt, RDCAPSZ);
	sb->SCB.sc_comp_code = SDI_ASW;
}

void
dcd_verify(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;

	dev = fill2devt(sb->SCB.sc_dev);
	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SCB.sc_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SCB.sc_comp_code = SDI_SCBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	dpbp->dpb_reqdata.sd_key = SD_ILLREQ;
	dpbp->dpb_reqdata.sd_errc = 0x70;
	dpbp->dpb_reqdata.sd_len = 0xe;
	dpbp->dpb_reqdata.sd_valid = 0;
	dpbp->dpb_reqdata.sd_sencode = SC_NOSENSE;
	sb->SCB.sc_comp_code = SDI_CKSTAT;
	sb->SCB.sc_status = S_CKCON;  
	return;
}
/*============================================================================
** SFB routines
*/

void
dcd_flushr(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	dev = fill2devt(sb->SFB.sf_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SFB.sf_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SFB.sf_comp_code = SDI_SFBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	gdev_drainq(dpbp, SDI_QFLUSH);
	sb->SFB.sf_comp_code = SDI_ASW;
}

void
dcd_suspend(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	dev = fill2devt(sb->SFB.sf_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SFB.sf_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SFB.sf_comp_code = SDI_SFBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	dpbp->dpb_flags |= DFLG_SUSPEND;
	sb->SFB.sf_comp_code = SDI_ASW;
}

void
dcd_resume(dcdblkp)
dcdblk_t *dcdblkp;
{
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;
	struct sb *sb = &dcdblkp->sb->sb;
	pl_t	oldpri;
	dev = fill2devt(sb->SFB.sf_dev);

	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev) || sb->SFB.sf_dev.sa_lun != 0) {       
		/* This device doesn't exist */
		sb->SFB.sf_comp_code = SDI_SFBERR;
		return;
	}
	dcbp = dcbptr(dev);
	dcbp->dcb_driveridx = driver_idx(dev);
	dpbp = dpbptr(dcbp,dev);
	dpbp->dpb_flags &= ~DFLG_SUSPEND;
	sb->SFB.sf_comp_code = SDI_ASW;
	dcbp->dcb_driveridx = 0;
	oldpri = SPLDISK;       
	(dcb_START(dcbp))(dcbp);  /* Start activity if necessary */
	splx(oldpri);
}

/*============================================================================
** Function name: dcd_open()
** Description:
** 	Driver Open Entry Point. It checks permissions. And in the case of
**	a pass-thru open it suspends the particular TC LU queue.
*/

/* ARGSUSED */

int
dcd_open(devp, flags, otype, cred_p)
dev_t	*devp;
cred_t	*cred_p;
int	flags;
int	otype;
{
	register int	c = SC_CONTROL(geteminor(*devp));
	register int	t = SC_TARGET(geteminor (*devp));

	if (SC_ILLEGAL(c, t)) 
		return(ENXIO);
	/* This is the pass-thru section */
	return(0);
}


/*============================================================================
** Function name: dcd_close()
** Description:
** 	Driver Close Entry Point. In the case of a pass-thru close it 
**	resumes the queue and calls the target driver event handler if 
**	one is present.
*/

/* ARGSUSED */

int
dcd_close(dev, flags, otype, cred_p)
cred_t	*cred_p;
dev_t	dev;	/* External Device # */
int	flags;
int	otype;
{
	register int	c = SC_CONTROL(geteminor (dev));
	register int	t = SC_TARGET(geteminor (dev));

	if (SC_ILLEGAL(c, t)) 
		return(ENXIO);
	/* getting and releasing exclusive access is do in pass-thru func */
	return(0);
}

/*============================================================================
** Function name: dcd_ioctl()
** Description:
**	Driver ioctl () Entry Point. Used to implement the following 
**	special functions:
**
**
**    SDI_SEND		- Send a user supplied SCSI Control Block to the
**			  specified scsi function.
**    GETVER		- Get the host adapter HW version
**    HA_VER		- Get sdi version structure 
**    REDT		- Read the extended EDT from RAM memory
**    SDI_BRESET	- Reset the specified bus 
**    RD_EXP		- Read the parameters setting the HA is using.
**    WR_EXP		- Write parameters for the HA to use.
**
*/

/* ARGSUSED */
int
dcd_ioctl(dev, cmd, argp, mode, cred_p, rval_p)
cred_t	*cred_p;
int	*rval_p;
dev_t	dev;
int	cmd;
caddr_t	argp;
int	mode;
{
	register struct sb *sp;
	pl_t		oip;
	struct bus_type	b;
	int		c = SC_CONTROL(geteminor (dev));
	int		t = SC_TARGET(geteminor (dev));
	int		uerror = 0;

	if (SC_ILLEGAL(c, t))
		return(ENXIO);
	switch (cmd)
	{
	case	SDI_SEND:	
		{
		register buf_t *bp;
		struct sb  karg;
		int  rw;
		char *save_priv;
		gdev_dcbp dcbp;
		gdev_dpbp dpbp;
		dev_t ldev;

		if (copyin(argp, (caddr_t)&karg, sizeof(struct sb))) {
			uerror = EFAULT;
			break;
		}
		if ((karg.sb_type != ISCB_TYPE) ||
		    (karg.SCB.sc_cmdsz <= 0 )   ||
		    (karg.SCB.sc_cmdsz > MAX_CMDSZ )) { 
			uerror = EINVAL;
			break;
		}
		if(karg.SCB.sc_cmdsz == SCM_SZ &&
			 ((struct scm *) (void *)SCM_RAD(karg.SCB.sc_cmdpt))->sm_op == 0x2f) {
			uerror = doverify(dev, argp);
			break;
		}
		bp = getrbuf(KM_SLEEP);
		sp = sdi_getblk(KM_SLEEP);
		save_priv = sp->SCB.sc_priv;
		bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));
		oip = SPLDISK;
		bp->b_iodone = NULL;
		sp->SCB.sc_priv = save_priv;
		sp->SCB.sc_dev.sa_lun = 0;
		sp->SCB.sc_dev.sa_fill = (c << 3) | t;

		ldev = fill2devt(sp->SCB.sc_dev);
		dcbp = dcbptr(ldev);
		dpbp = dpbptr(dcbp,ldev);
		if (dpbp->dpb_sc_cmd == NULL) {
			dpbp->dpb_sc_cmd = 
			  (char *)kmem_zalloc(MAX_CMDSZ + sizeof(int), KM_SLEEP)
			  + sizeof(int);
		}
		sp->SCB.sc_cmdpt = dpbp->dpb_sc_cmd;
		if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
				sp->SCB.sc_cmdsz)) {
			uerror = EFAULT;
			goto done;
		}
		rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
#ifdef PDI_SVR42
		bp->b_private = (char *)sp;
#else
		bp->b_priv.un_ptr = sp;
#endif
		/*
		 * If the job involves a data transfer then the
		 * request is done thru physio() so that the user
		 * data area is locked in memory. If the job doesn't
		 * involve any data transfer then dcd_pass_thru()
		 * is called directly.
		 */
		if (sp->SCB.sc_datasz > 0) { 
			struct iovec  ha_iov;
			struct uio    ha_uio;
			int offset = 0;
			char op;

			ha_iov.iov_base = sp->SCB.sc_datapt;	
			ha_iov.iov_len = sp->SCB.sc_datasz;	
			ha_uio.uio_iov = &ha_iov;
			ha_uio.uio_iovcnt = 1;
			op = sp->SCB.sc_cmdpt[0];
			if (op == SS_READ || op == SS_WRITE) {
				struct scs *scs = 
					(struct scs *)sp->SCB.sc_cmdpt;
				offset = (sdi_swap16(scs->ss_addr) + 
				(scs->ss_addr1 << 16)) * dpbp->dpb_secsiz;
			}
			if (op == SM_READ || op == SM_WRITE) {
				struct scm *scm = 
				(struct scm *)(char *)SCM_RAD(sp->SCB.sc_cmdpt);
				offset = sdi_swap32(scm->sm_addr) * 
					dpbp->dpb_secsiz;
			}
			ha_uio.uio_offset = offset;
			ha_uio.uio_segflg = UIO_USERSPACE;
			ha_uio.uio_fmode = 0;
			ha_uio.uio_resid = sp->SCB.sc_datasz;
			uerror = physiock((void (*)())dcd_pass_thru0, bp, dev, rw, 4194303, &ha_uio);
		}
		else {
			bp->b_un.b_addr = sp->SCB.sc_datapt;
			bp->b_bcount = sp->SCB.sc_datasz;
			bp->b_blkno = NULL;
			bp->b_edev = dev;
			bp->b_flags |= rw;

			dcd_pass_thru(bp);  /* bypass physiock call */
			biowait(bp);
		}

		/* update user SCB fields */
		karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
		karg.SCB.sc_status = sp->SCB.sc_status;
		karg.SCB.sc_time = sp->SCB.sc_time;
		if (copyout((caddr_t)&karg, argp, sizeof(struct sb)))
		        uerror = EFAULT;
	   done:
		freerbuf(bp);
		sdi_freeblk(sp);
		splx(oip);
		break;
		}
	case	SDI_BRESET:
/*		don't do a reset on direct couple devices */
		break;
	case	B_GETTYPE:
		b.bus_name[0] = 'd';
		b.bus_name[1] = 'c';
		b.bus_name[2] = 'd';
		b.bus_name[3] = NULL;
		b.bus_name[4] = NULL;
		b.drv_name[0] = NULL;
		if (copyout ((caddr_t)&b, argp, sizeof (struct bus_type)) != 0)
			uerror = EFAULT;
		break;
	case	HA_VER:
		if (copyout((caddr_t)&dcd_ver,argp, sizeof(struct ver_no)))
			uerror = EFAULT;
		break;
	case	B_HA_CNT:
		if (copyout((caddr_t)&dcd_hacnt,argp, sizeof(dcd_hacnt)))
			uerror = EFAULT;
		break;
	default:		/* Invalid Request */
		uerror = EINVAL;
		break;
	} 
	return(uerror);
}


/*
** Function name: dcd_devcon()
** Description:
**	Convert an external dev type to an internal dcd "scsi" dev type.
*/
int
dcd_devcon(dev)
dev_t dev;
{
    	struct scsi_ad sc_dev;
	int	hba = (geteminor(dev) >> 5) & 0x7;
	int	t = SC_TARGET(geteminor(dev));

	sc_dev.sa_lun = 0;
	sc_dev.sa_fill = (hba << 3) | t;
	return (fill2devt(sc_dev));
}

#ifndef PDI_SVR42
/*
 * STATIC void
 * dcd_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
dcd_pass_thru0(buf_t *bp)
{
	register struct sb *sp;
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t dev;

#ifdef PDI_SVR42
	sp = (struct sb *) bp->b_private;
#else
	sp = bp->b_priv.un_ptr;
#endif
	sp->SCB.sc_wd = (long)bp;
	dev = fill2devt(sp->SCB.sc_dev);
	dcbp = dcbptr(dev);
	dpbp = dpbptr(dcbp,dev);
	
	if (!dpbp->dpb_bcbp) {
		struct sb *sp;
		struct scsi_ad *sa;
#ifdef PDI_SVR42
		sp = (struct sb *)bp->b_private;
#else
		sp = bp->b_priv.un_ptr;
#endif
		sa = &sp->SCB.sc_dev;
		if ((dpbp->dpb_bcbp = sdi_getbcb(sa, KM_SLEEP)) == NULL) {
			sp->SCB.sc_comp_code = SDI_ABORT;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		dpbp->dpb_bcbp->bcb_granularity = 1;
		dpbp->dpb_bcbp->bcb_flags = BCB_SYNCHRONOUS;
		if (dcbp->dcb_capab & (CCAP_DMA | CCAP_SCATGATH)) {
			dpbp->dpb_bcbp->bcb_physreqp->phys_align = 512;
			dpbp->dpb_bcbp->bcb_physreqp->phys_boundary = 64 * 1024;
			dpbp->dpb_bcbp->bcb_physreqp->phys_dmasize = 24;
		}
		dpbp->dpb_bcbp->bcb_addrtypes = BA_KVIRT;
		dpbp->dpb_bcbp->bcb_max_xfer = dcd_hba_info.max_xfer;
	}

	buf_breakup(dcd_pass_thru, bp, dpbp->dpb_bcbp);
}
#endif /* !PDI_SVR42 */

/*
 * STATIC void
 * dcd_pass_thru(buf_t *bp)
 *	Send a pass-thru job to the HA board.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
dcd_pass_thru(buf_t *bp)
{
	register struct sb *sp;
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	pl_t  oip;
	dev_t dev;
	void dcd_int();
	caddr_t *addr;
	char op;

#ifdef PDI_SVR42
	sp = (struct sb *) bp->b_private;
#else
	sp = bp->b_priv.un_ptr;
#endif
	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = dcd_int;
	dev = fill2devt(sp->SCB.sc_dev);
	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dev)) {       
		/* This device doesn't exist */
		sp->SCB.sc_comp_code = SDI_SCBERR;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	dcbp = dcbptr(dev);
	dpbp = dpbptr(dcbp,dev);
	sdi_translate(sp, bp->b_flags, bp->b_proc, KM_SLEEP);
	op = sp->SCB.sc_cmdpt[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)sp->SCB.sc_cmdpt;
		daddr_t blkno = bp->b_blkno;
		scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
		scs->ss_addr  = sdi_swap16(blkno);
		scs->ss_len   = (char)(bp->b_bcount / dpbp->dpb_secsiz);
	}
	if (op == SM_READ || op == SM_WRITE) {
		struct scm *scm = (struct scm *)(char *)SCM_RAD(sp->SCB.sc_cmdpt);
		scm->sm_addr = sdi_swap32(bp->b_blkno);
		scm->sm_len  = sdi_swap16(bp->b_bcount / dpbp->dpb_secsiz);
	}
	oip = SPLDISK;
	GetExcl(dcbp,dpbp);
	comm_cmd((dcdblk_t *)((struct xsb *)sp)->hbadata_p, KM_SLEEP);
	RelExcl(dcbp,dpbp);
	splx(oip);
	return;
}

/*============================================================================
** Function name: dcd_int()
** Description:
**	This is the interrupt handler for pass-thru jobs. It basically
**	just wakes up the ll_send_scb() function.
*/

void
dcd_int (psb)
struct sb	*psb;
{
	buf_t	*bp = (buf_t *)psb->SCB.sc_wd;

	if (psb->SCB.sc_comp_code == SDI_PROGRES) {
		cmn_err (CE_WARN,"DCD Host Adapter: Bad completion code (0x%x) returned during passthru operation.\n",psb->SCB.sc_comp_code);
		psb->SCB.sc_comp_code = SDI_ASW;
	}
	biodone (bp);
}

int
doverify(dev, argp)
dev_t	dev;
caddr_t	argp;
{
	
	char *datapt;
	struct scm *cmdpt;
	struct sb  karg;
	dev_t dcddev;
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	int start, end;
	int  i, size, lun;

	dcddev = dcd_devcon(dev);
	/* The only lun that we use is 0 all others are invalid */
	if (!valid_dev(dcddev))
		return (ENXIO);
	dcbp = dcbptr(dcddev);
	dpbp = dpbptr(dcbp,dcddev);
	if (copyin(argp, (caddr_t)&karg, sizeof(struct sb))) {
		return (EFAULT);
	}
	size = dpbp->dpb_sectors * dpbp->dpb_secsiz;
	datapt = kmem_zalloc(size,KM_SLEEP);
	cmdpt = (struct scm *) (void *)SCM_RAD(karg.SCB.sc_cmdpt);
	start = sdi_swap32(cmdpt->sm_addr);
	end = sdi_swap16(cmdpt->sm_len) + start;
	lun = cmdpt->sm_lun;
	for(i = start; i < end; i += dpbp->dpb_sectors) {
		int len;
		int startsav = i;

		dpbp->dpb_flags &= ~(DFLG_RETRY | DFLG_ERRCORR);
		len = ((end-i)>(int)dpbp->dpb_sectors)
			?dpbp->dpb_sectors:(end-i);
		if(do_readver(&karg, i, len, lun, datapt,
			 len * dpbp->dpb_secsiz, dev)) {
			/* gotta read each sector in track */
			int j;
			int addr;
			for(j = 0, addr = startsav; j < len; j++, addr++) {
				if(do_readver(&karg, addr, 1, lun, datapt,
				  dpbp->dpb_secsiz, dev)) {
					karg.SCB.sc_comp_code = SDI_CKSTAT;
					karg.SCB.sc_status = S_CKCON;  
					dpbp->dpb_reqdata.sd_key = SD_MEDIUM;
					dpbp->dpb_reqdata.sd_errc = 0x70;
					dpbp->dpb_reqdata.sd_len = 0xe;
					dpbp->dpb_reqdata.sd_ba = sdi_swap32(addr);
					dpbp->dpb_reqdata.sd_valid = 1;
					dpbp->dpb_reqdata.sd_sencode = SC_IDERR;
					if (copyout((caddr_t)&karg, argp,
						    sizeof(struct sb))) {
						kmem_free(datapt, size);
						return (EFAULT);
					}
					kmem_free(datapt, size);
					return (0);
				}
			}
		}

		dpbp->dpb_flags |= (DFLG_RETRY | DFLG_ERRCORR);
	}
	karg.SCB.sc_comp_code = SDI_ASW;	/* temp */
	karg.SCB.sc_status = S_GOOD;  	/* temp */
	if (copyout((caddr_t)&karg, argp, sizeof(struct sb))) {
		kmem_free(datapt, size);
		return (EFAULT);
	}
	kmem_free(datapt, size);
	return (0);
}

/* return 0 if everything OK, 1 if found badblock */
int
do_readver(karg, addr, len, lun, datapt, size, dev)
struct sb *karg;
int addr;	/*starting address */
int len;	/* amount to read */
int lun;
dev_t	dev;
int size;	/* size of data */
{
	int	c = (geteminor(dev) >> 5) & 0x7;
	int	t = SC_TARGET(geteminor(dev));
	register buf_t *bp;
	struct scm *scm;
	register struct sb *sp;
	int  rw;
	pl_t  oip;
	int uerror = 0;
	char *save_priv;
	gdev_dcbp dcbp;
	gdev_dpbp dpbp;
	dev_t ldev;

	bp = getrbuf(KM_SLEEP);
	sp = sdi_getblk(KM_SLEEP);
	save_priv = sp->SCB.sc_priv;
	bcopy((caddr_t)karg, (caddr_t)sp, sizeof(struct sb));
	oip = SPLDISK;
	bp->b_iodone = NULL;
	sp->SCB.sc_priv = save_priv;
	sp->SCB.sc_datapt = (caddr_t) datapt;
	sp->SCB.sc_cmdsz = 10;
	sp->SCB.sc_dev.sa_lun = 0;
	sp->SCB.sc_dev.sa_fill = (c << 3) | t;

	ldev = fill2devt(sp->SCB.sc_dev);
	dcbp = dcbptr(ldev);
	dpbp = dpbptr(dcbp,ldev);
	if (dpbp->dpb_sc_cmd == NULL) {
		dpbp->dpb_sc_cmd = 
		  (char *)kmem_zalloc(MAX_CMDSZ + sizeof(int), KM_SLEEP)
		  + sizeof(int);
	}
	sp->SCB.sc_cmdpt = dpbp->dpb_sc_cmd;
	/* setup scsi command */
	scm = (struct scm *)(char *)SCM_RAD(sp->SCB.sc_cmdpt);
	scm->sm_op = SM_READ;
	scm->sm_lun = lun;
	scm->sm_addr = sdi_swap32(addr);
	scm->sm_len = sdi_swap16(len);
	rw = B_READ;

#ifdef PDI_SVR42
	bp->b_private = (char *)sp;
#else
	bp->b_priv.un_ptr = sp;
#endif

	/* If the job involves a data transfer then the request is done 
	thru physio() so that the user data area is locked in memory. If 
	the job doesn't involve any data transfer then dcd_pass_thru()
	is called directly.  */
	if (sp->SCB.sc_datasz > 0) {
		struct iovec  ha_iov;
		struct uio    ha_uio;
		int offset = 0;
		char op;

		ha_iov.iov_base = sp->SCB.sc_datapt;
		ha_iov.iov_len = sp->SCB.sc_datasz;
		ha_uio.uio_iov = &ha_iov;
		ha_uio.uio_iovcnt = 1;
		op = sp->SCB.sc_cmdpt[0];
		if (op == SS_READ || op == SS_WRITE) {
			struct scs *scs = 
				(struct scs *)sp->SCB.sc_cmdpt;
			offset = (sdi_swap16(scs->ss_addr) + 
			(scs->ss_addr1 << 16)) * dpbp->dpb_secsiz;
		}
		if (op == SM_READ || op == SM_WRITE) {
			struct scm *scm = 
			(struct scm *)(char *)SCM_RAD(sp->SCB.sc_cmdpt);
			offset = sdi_swap32(scm->sm_addr) * 
				dpbp->dpb_secsiz;
		}
		ha_uio.uio_offset = offset;
		ha_uio.uio_segflg = UIO_USERSPACE;
		ha_uio.uio_fmode = 0;
		ha_uio.uio_resid = sp->SCB.sc_datasz;

		uerror = physiock((void (*)())dcd_pass_thru0, bp, dev, rw,
		    4194303, &ha_uio);
	}
	else {
		bp->b_un.b_addr = sp->SCB.sc_datapt;
		bp->b_bcount = sp->SCB.sc_datasz = size;
		bp->b_blkno = addr;
		bp->b_edev = dev;
		bp->b_flags |= rw;
		dcd_pass_thru(bp);  /* bypass physiock call */
		biowait(bp);
	}
	/* update user SCB fields */
	karg->SCB.sc_time += sp->SCB.sc_time;
	if(sp->SCB.sc_comp_code != SDI_ASW)
		uerror = 1;
	sdi_freeblk(sp);
	freerbuf(bp);
	splx(oip);
	return (uerror);
}


/*
 * dcd_cstart -- Start a controller moving if it isn't already...
 */

void
dcd_cstart(dcbp)
register gdev_dcbp dcbp;
{
	register int busydrives;

/*	check for exclusive control and busy				*/
	if (dcbp->dcb_flags & (CFLG_EXCL|CFLG_BUSY))
		return;         
	if (!dcd_sngctlp || !dcd_sngctl.s_dpbp)
		busydrives=gdev_start_mult(dcbp);
	else {
		if (dcd_sngctl.s_dpbp->dpb_flags & DFLG_BUSY)
			return;
		if (gdev_reload(dcd_sngctl.s_dpbp))
			gdev_start_req(dcbp, dcd_sngctl.s_dpbp);
		if (dcd_sngctl.s_dpbp->dpb_flags & DFLG_BUSY)
			return;
		else
			busydrives=0;
	}
	/*
 	 * if no busy drives, grant exclusivity if requested and we're the first
 	 * driver. Let our caller know if anyone we called got things moving...
 	 */
	if (!(dcbp->dcb_flags & CFLG_BUSY) && !busydrives) {
	/*
 	 * We're the first driver in the chain & controller's idle.
 	 * If somebody has exclusive access already, let him keep it.  
 	 * If not and somebody wants exclusivity, grant it.
 	*/
		if (dcbp->dcb_driveridx == 0) {
			if ((dcbp->dcb_flags & (CFLG_EXCLREQ | CFLG_EXCL)) == 
				CFLG_EXCLREQ) 	
				wakeup ((char *)dcbp);
		}
	}
}
/* the following routines were left since they're very useful when problems
 * occur. The ifdef should be removed and the routines can be invoked by
 * kdb or called in the above routines.
 */
#ifdef DCD_DEBUG
print_chain(drqp)
struct drq_entry *drqp;
{
while (drqp)
	{
	int ct = drqp->drq_type;
	switch (ct)     {
	case DRQT_START:
		cmn_err(CE_CONT,
		"START -- count %d, memaddr (%s) %x, sect %d vadr %x\n",
			drqp->drq_count,
			(drqp->drq_phys ? "phys" : "virt"),
			drq_memaddr(drqp),
			drq_daddr(drqp),
			drq_vaddr(drqp));
		break;
	case DRQT_CONT:
		cmn_err(CE_CONT,
		"CONT  -- count %d, memaddr (%s) %x, sect %d vadr %x\n",
			drqp->drq_count,
			(drqp->drq_phys ? "phys" : "virt"),
			drq_memaddr(drqp),
			drq_daddr(drqp),
			drq_vaddr(drqp));
		break;
	case DRQT_MEMBREAK:
		cmn_err(CE_CONT,
		"BREAK -- count %d, memaddr %x, %s %x vadr %x\n",
			drqp->drq_count,
			drq_memaddr(drqp),
			(drqp->drq_virt ? "(virt)" : "bufp"),
			drq_bufp(drqp),
			drq_vaddr(drqp));
		break;
	case DRQT_MEMXFER:
		cmn_err(CE_CONT,
		"XFER  -- count %d, srcaddr %x, dstaddr %x vadr %x\n",
			drqp->drq_count,
			drq_srcaddr(drqp),
			drq_dstaddr(drqp),
			drq_vaddr(drqp));
		break;
	case DRQT_MEMCLEAR:
		cmn_err(CE_CONT,
		"CLEAR  -- count %d, dstaddr %x vadr %x\n",
			drqp->drq_count,
			drq_dstaddr(drqp),
			drq_vaddr(drqp));
		break;
	case DRQT_IGNORE:
		cmn_err(CE_CONT,
		"IGNORE  -- count %d, srcaddr %x vadr %x\n",
			drqp->drq_count,
			drq_srcaddr(drqp),
			drq_vaddr(drqp));
		break;
	default:
		cmn_err(CE_CONT,
		"ILLEGAL TYPE %d.  RETURNING\n",ct);
		cmn_err(CE_CONT,
		"ILLEGAL TYPE  -- count %d, srcaddr %x, dstaddr %x vadr %x\n",
			drqp->drq_count,
			drq_srcaddr(drqp),
			drq_dstaddr(drqp),
			drq_vaddr(drqp));
		break;
		}
	drqp = drqp->drq_link;
	}
	cmn_err(CE_CONT, "END-of-CHAIN\n");
}

dcdpr_dcdblk(dpbp)
struct gdev_parm_block *dpbp;
{
	struct sb     	*sb; 
	dcdblk_t	*dp;
	int		i;
	struct scs 	*scs;
	struct scm 	*scm;
	int 		direction;
	int 		sector;
	int 		count;
	short		inchr;

	if (!dpbp)
		dpbp = dcd_dpbp;
	dp = (dcdblk_t *)dpbp->dpb_que;
	for (i=0; dp; dp=dp->av_forw, i++) {
		sb = (struct sb *)dp->sb;
		if(sb->SCB.sc_cmdsz == SCS_SZ){
			scs = (struct scs *)sb->SCB.sc_cmdpt;
			direction = scs->ss_op == SS_READ ? B_READ : B_WRITE;
			sector = sdi_swap16(scs->ss_addr) + ((scs->ss_addr1 << 16 ) & 0xFF);
			count = scs->ss_len;
		} else {
			scm = (struct scm *)(SCM_RAD(sb->SCB.sc_cmdpt));
			direction = scm->sm_op == SM_READ ? B_READ : B_WRITE;
			sector = sdi_swap32(scm->sm_addr);
			count = sdi_swap16(scm->sm_len);
		}
		cmn_err(CE_CONT,
		"DCDBLK[%d]0x%x:%s: sb_sec= 0x%x cnt=0x%x\n",
			i, dp, (direction==B_READ)?"RD":"WR", 
			sector, count);
	}
}
#endif

void
dcd_halt()
{
	extern int dcd_halt_delay;

	if( dcd_halt_delay > 0 ) {
		drv_usecwait( dcd_halt_delay * 1000000 );
	}

	return;
}

void
dcd_drqclean(struct drq_entry *drqp) 
{
	struct drq_entry *tdrqp;

	while (drqp) {
		tdrqp = drqp->drq_link;
		reldrq(drqp);
		drqp = tdrqp;
	}
}
