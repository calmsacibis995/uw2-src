/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/hba/dak/dak.c	1.33"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <fs/select.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <svc/systm.h>
#include <io/mkdev.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi/sdi_edt.h>
#include <io/target/sdi/sdi.h>

#if (PDI_VERSION <= 1)
#include <io/target/dynstructs.h>
#else /* ! (PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#include <io/target/sdi/sdi_hier.h>
#endif /* ! (PDI_VERSION <= 1) */

#include <io/hba/dak/dak.h>
#include <io/hba/dak/raidapi.h>
#include <io/hba/hba.h>
#if (PDI_VERSION >= PDI_UNIXWARE20)
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#endif
#include <util/mod/moddefs.h>
#include <util/mod/moddrv.h>

#if (PDI_VERSION >= PDI_SVR42MP)
#include <util/ksynch.h>
#endif

#include <io/ddi.h>
#include <io/ddi_i386at.h>

#else /* _KERNEL_HEADERS */

#include <sys/errno.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/fs/s5param.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/mkdev.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#if (PDI_VERSION > 1)
#include <sys/sdi_hier.h>
#endif /* (PDI_VERSION > 1) */
#include <sys/dynstructs.h>
#include "dak.h"
#include "raidapi.h"
#include <sys/hba.h>
#if (PDI_VERSION >= PDI_UNIXWARE20)
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#endif
#include <sys/immu.h>
#include <sys/moddefs.h>
#include <sys/moddrv.h>
#include <sys/uio.h>

#if (PDI_VERSION >= PDI_SVR42MP)
#include <sys/ksynch.h>
#endif

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#ifdef DDICHECK
#include <sys/ddicheck.h>
#endif
#endif /* !_KERNEL_HEADERS */

#ifdef DAK_DEBUG
int dak_debug = 1;	/* Debugging level */
#endif

#define		DRVNAME	"DAC960 SCSI HBA driver"
#define DAK_BLKSHFT	  9	/* PLEASE NOTE:  Currently pass-thru	    */
#define DAK_BLKSIZE	512	/* SS_READ/SS_WRITE, SM_READ/SM_WRITE	    */
				/* supports only 512 byte blocksize devices */
#define DAK_TEMP_BUFSZ	100
unsigned long dak_command_array[DAK_MAXCTLS][64];
unsigned char dak_command_type[DAK_MAXCTLS][64];
unsigned long dak_disk_size[DAK_MAXCTLS][8];

/* Allocated in space.c */

extern int		dak_cntls; 	/* Total # of controllers declared*/
extern struct ver_no    dak_sdi_ver;	/* SDI version structure	*/
extern int		dak_do_conc_io[8];
				/* switch to control whether or not
				 * requests go to the board simultaneously.
				 */
extern int		dak_gtol[];	/* xlate global hba# to loc */
extern int		dak_ltog[];	/* local hba# to global     */

static char *dak_disk_names="Logical Drive          ";
static	int	dak_mod_dynamic = 0;

extern struct head	sm_poolhead;

struct dak_scsi_ha   	*dak_sc_ha;	/* SCSI HA structures		*/
int dak_waitflag = TRUE;		/* dak_wait() interrupt wait state*/

dak_dma_t        *dak_dmalistpool;	/* DMA list pool        */
dak_dma_t        *dak_dfreelist;	/* List of free DMA lists       */
char             dakinit_time;		/* Init/start time (poll) flag  */
int		 dak_brdcnt;		/* # hba's found */
int		 dak_hacnt;		/* # hba's registered */
unsigned  dak_ha_structsize, dak_ccb_structsize, dak_luq_structsize,
	  dak_dma_listsize;
STATIC	sv_t   *dak_dmalist_sv;		/* sleep argument for UP	*/

#if (PDI_VERSION < PDI_SVR42MP)
STATIC	int	dak_devflag = 0;
typedef ulong_t vaddr_t;
#define KMEM_ZALLOC kmem_zalloc
#define dak_pass_thru0 dak_pass_thru

#else /* (PDI_VERSION >= PDI_SVR42MP) */

#define KMEM_ZALLOC dak_kmem_zalloc_physreq
#define DAK_MEMALIGN	512
#define DAK_BOUNDARY	0
STATIC void *dak_kmem_zalloc_physreq (size_t size, int flags);
STATIC void dak_pass_thru0(struct buf *bp);

/*
 * Multi-threading variables
 */
STATIC	int    dak_devflag = HBA_MP;
STATIC lock_t *dak_dmalist_lock;
STATIC lock_t *dak_ccb_lock;
STATIC LKINFO_DECL(dak_lkinfo_dmalist, "IO:dak:dak_dmalist_lock", 0);
STATIC LKINFO_DECL(dak_lkinfo_ccb, "IO:dak:dak_ccb_lock", 0);
STATIC LKINFO_DECL(dak_lkinfo_ctl, "IO:dak:ha->ha_ctl_lock", 0);
STATIC LKINFO_DECL(dak_lkinfo_q, "IO:dak:q->q_lock", 0);

#endif /* (PDI_VERSION >= PDI_SVR42MP) */

HBA_INFO(dak, &dak_devflag, 0x10000);

#if (PDI_VERSION >= PDI_UNIXWARE20)
/*
 * Autoconfig:
 * The name of this pointer is the same as that of the
 * original idata array. Once it it is assigned the
 * address of the new array, it can be referenced as
 * before and the code need not change.
 */
extern HBA_IDATA_STRUCT	_dakidata[];
HBA_IDATA_STRUCT	*dakidata;
int	dak_load(), dak_unload(), dakverify(), dak_mca_conf();
void	sdi_mca_conf();
MOD_ACHDRV_WRAPPER(dak, dak_load, dak_unload, dakhalt, dakverify, DRVNAME);

#else /* (PDI_VERSION < PDI_UNIXWARE20) */

/*
 * Non-Autoconfig
 */
extern HBA_IDATA_STRUCT	dakidata[];
int	dak_load(), dak_unload();
MOD_HDRV_WRAPPER(dak, dak_load, dak_unload, dakhalt, DRVNAME);

#endif /* (PDI_VERSION < PDI_UNIXWARE20) */

int	dakinit();
int	dakstart();
STATIC	int	dak_getiobus(int);
STATIC	int	dak_getitype(int);
STATIC	int	dak_eisa_iocmd(int );
STATIC	int	dak_mca_iocmd(int );
STATIC	int	dak_pci_iocmd(int );
STATIC	void	dak_eisa_halt(struct dak_scsi_ha * );
STATIC	void	dak_mca_halt(struct dak_scsi_ha * );
STATIC	void	dak_pci_halt(struct dak_scsi_ha * );
STATIC	int	dak_eisa_issue_cmd(struct dak_scsi_ha *, int, int, char *, int);
STATIC	int	dak_mca_issue_cmd(struct dak_scsi_ha *, int, int, char *, int);
STATIC	int	dak_pci_issue_cmd(struct dak_scsi_ha *, int, int, char *, int);

STATIC	int	dak_dmalist();
STATIC	void	dak_dma_freelist();
STATIC	struct  dak_ccb *dak_getccb();	/* Alloc controller cmd block struct */
STATIC	void    dak_freeccb();		/* Release SCSI command block.       */
STATIC	void    dak_init_cps();		/* Init controller CP free list.     */
STATIC	void    dak_pass_thru();	/* Send pass-thru job to the HA.     */
STATIC	void    dak_int();		/* Int Handler for pass-thru jobs.   */
STATIC	void    dak_putq();		/* Put job on Logical Unit Queue.    */
STATIC	void    dak_next();		/* Send next Logical Unit Queue Job. */
STATIC	void    dak_cmd();		/* Create/Send an SCB associated cmd */
STATIC	void    dak_func();		/* Create/Send an SFB associated cmd */
STATIC	void    dak_eisa_send();	/* Send command to the HA board.     */
STATIC	void    dak_mca_send();		/* Send command to the HA board.     */
STATIC	void    dak_pci_send();		/* Send command to the HA board.     */
STATIC	void    dak_done();		/* Intr done for CP and SCB.         */
STATIC	void    dak_timer();		/* Handles Controller command timing */
STATIC	void	dak_flushq();
STATIC	void	dak_eisa_intr();
STATIC	void	dak_mca_intr();
STATIC	void	dak_pci_intr();
STATIC	int     dak_eisa_ha_init();	/* Initialize HA communication       */
STATIC	int     dak_mca_ha_init();	/* Initialize HA communication       */
STATIC	int     dak_pci_ha_init();	/* Initialize HA communication       */
STATIC	void    dak_ha_done();		/* Intr done for CP and SFB.         */
STATIC	void	dak_break_point();
STATIC	int	dak_wait();
STATIC	int dak_illegal(short, int, unsigned char, unsigned char);
STATIC	void dak_lockinit(int);
STATIC	void dak_lockclean(int);

STATIC	USHORT          dachlpGetM16(PUCHAR p);
STATIC	ULONG           dachlpGetM24(PUCHAR p);
STATIC	ULONG           dachlpGetM32(PUCHAR p);

struct dak_functions dak_functions_array[] = {
	{NULL,NULL,NULL,NULL,NULL},			 /* AI_INTERNAL */
	{NULL,NULL,NULL,NULL,NULL},			 /* AI_ISA_BUS  */
	{dak_eisa_ha_init, dak_eisa_intr, dak_eisa_send, /*AI_EISA_BUS  */
	 dak_eisa_iocmd,   dak_eisa_halt},
	{dak_mca_ha_init,  dak_mca_intr,  dak_mca_send,  /* AI_uCHNL_BUS*/
	 dak_mca_iocmd,    dak_mca_halt},
	{NULL,NULL,NULL,NULL,NULL},			 /* AI_TURBO_BUS*/
	{dak_pci_ha_init,  dak_pci_intr,  dak_pci_send,	 /* AI_PCI_BUS  */
	 dak_pci_iocmd,    dak_pci_halt}
};

#if (PDI_VERSION >= PDI_UNIXWARE20)
STATIC int
dak_load()
{
	dak_mod_dynamic = 1;
	if( dakinit() ) {
		/*
		 * We may have allocated a new idata array at this point
		 * so attempt to free it before failing the load.
		 */
		sdi_acfree(dakidata, dak_cntls);
		cmn_err(CE_NOTE,"!DAC Host Adapter: Init Fail");
		return(ENODEV);
	}
	if (dakstart()) {
		/*
		 * At this point we have allocated a new idata array,
		 * free it before failing.
		 */
		sdi_acfree(dakidata, dak_cntls);
		return(ENODEV);
	}
	return(0);
}

#else /* (PDI_VERSION < PDI_UNIXWARE20) */
STATIC int
dak_load()
{
	dak_mod_dynamic = 1;
	if( dakinit() ) {
		cmn_err(CE_NOTE,"!DAC Host Adapter: Init Fail");
		return(ENODEV);
	}
	mod_drvattach(&dak_attach_info);
	if (dakstart()) {
		mod_drvdetach(&dak_attach_info);
		return(ENODEV);
	}
	return(0);
}
#endif /* (PDI_VERSION < PDI_UNIXWARE20) */

STATIC int
dak_unload()
{
	return(EBUSY);
}

/*
** Function name: dakopen()
** Description:
** 	Driver open() entry point. It checks permissions, and in the
**	case of a pass-thru open, suspends the particular LU queue.
** Calling/Exit State:
 *	None.
*/

/*ARGSUSED*/
STATIC int
HBAOPEN(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t	dev = *devp;
	minor_t		minor = geteminor(dev);
	register int	c = dak_gtol[SC_EXHAN(minor)];
	register int	t = SC_EXTCN(minor);
	register int	l = SC_EXLUN(minor);
	register int	b = SC_BUS(minor);
	register struct dak_scsi_lu *q;

	if (dak_illegal((minor_t)(SC_EXHAN(minor)), b, t, l)) {
		return(ENXIO);
	}

	if (t == dak_sc_ha[c].ha_id)
		return(0);

	/* This is the pass-thru section */

	q = &LU_Q(c, b, t, l);

	DAK_SCSILU_LOCK(q->q_opri);
	if ((q->q_count > 0) ||
		((! dak_do_conc_io[t]) && (q->q_flag & DAK_QBUSY)) ||
		(q->q_flag & (DAK_QSUSP | DAK_QPTHRU)))
	{
		DAK_SCSILU_UNLOCK(q->q_opri);
		return(EBUSY);
	}

	q->q_flag |= DAK_QPTHRU;
	DAK_SCSILU_UNLOCK(q->q_opri);
	return(0);
}


/*
** Function name: dakclose()
** Description:
** 	Driver close() entry point.  In the case of a pass-thru close
**	it resumes the queue and calls the target driver event handler
**	if one is present.
*/

/*ARGSUSED*/
STATIC int
HBACLOSE(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	minor_t		minor = geteminor(dev);
	register int	c = dak_gtol[SC_EXHAN(minor)];
	register int	t = SC_EXTCN(minor);
	register int	l = SC_EXLUN(minor);
	register int	b = SC_BUS(minor);
	register struct dak_scsi_lu *q;

	if (t == dak_sc_ha[c].ha_id)
		return(0);

	q = &LU_Q(c, b, t, l);

	DAK_SCSILU_LOCK(q->q_opri);
	q->q_flag &= ~DAK_QPTHRU;

	if (q->q_func != NULL) {
		DAK_SCSILU_UNLOCK(q->q_opri);
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
		DAK_SCSILU_LOCK(q->q_opri);
	}

	dak_next(q);
	return(0);
}

/*
** Function name: dakioctl()
** Description:
**	Driver ioctl() entry point.  Used to implement the following
**	special functions:
**
**	SDI_SEND     -	Send a user supplied SCSI Control Block to
**			the specified device.
**	B_GETTYPE    -  Get bus type and driver name
**	B_HA_CNT     -	Get number of HA boards configured
**	REDT	     -	Read the extended EDT from RAM memory
**	SDI_BRESET   -	Reset the specified SCSI bus
*/

/*ARGSUSED*/
STATIC int
HBAIOCTL(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	minor_t		minor = geteminor(dev);
	register int	c = dak_gtol[SC_EXHAN(minor)];
	register int	t = SC_EXTCN(minor);
	register int	l = SC_EXLUN(minor);
	register int	b = SC_BUS(minor);
	register struct sb *sp;
	struct scs	*inq_cdb;
	register struct dak_scsi_lu  *q;
	struct buf	*bp;
	caddr_t		u_bufaddr;
	unsigned short	* shortptr;
	struct dak_scsi_ha	*ha = &dak_sc_ha[c];
	pl_t		opri;
	
	switch(cmd)
	{
	case SDI_SEND:
		{
			struct sb  karg;
			int  errnum = 0, rw;
			char *save_priv;

			if (t == ha->ha_id) { 	/* illegal ID */
				return(ENXIO);
			}
			if (copyin(arg, (caddr_t)&karg, sizeof(struct sb))) {
				return(EFAULT);
			}
			if ((karg.sb_type != ISCB_TYPE) ||
			    (karg.SCB.sc_cmdsz <= 0 )   ||
			    (karg.SCB.sc_cmdsz > MAX_CMDSZ )) {
				return(EINVAL);
			}

			inq_cdb = (struct scs *)(void *)karg.SCB.sc_cmdpt;
			if(b == 0 && t == 0)
			{
			unsigned char temp_buf[40];
			char *cptr, *cptr1;
  			if(!dak_disk_size[c][ l ])
			{
				karg.SCB.sc_comp_code = SDI_SCBERR;
				return (EINVAL);
      			}
			switch(inq_cdb->ss_op)
			{
				case SS_READ:
				case SS_WRITE:
				case SM_READ:
				case SM_WRITE:
					break;		/*  tackle later */
				case SS_INQUIR:
					temp_buf[0]=ID_RANDOM;
					temp_buf[1]=0;
					temp_buf[2]=1;
					temp_buf[3]=1;
					temp_buf[4]=0x1f;
					temp_buf[5]=0;
					temp_buf[6]=0;
					temp_buf[7]=0;
					(void)strncpy((caddr_t)&temp_buf[8], dak_disk_names, 24);
					if (copyout((caddr_t)temp_buf,(caddr_t)karg.SCB.sc_datapt, karg.SCB.sc_datasz))
					{
						cmn_err(CE_NOTE,"dakioctl: Err");
						return(EFAULT);
					}

					karg.SCB.sc_comp_code = SDI_ASW;
					karg.SCB.sc_status = 0;
					if (copyout((caddr_t)&karg, arg, sizeof(struct sb)))
						return(EFAULT);
					return (0);

				case SM_RDCAP:
					cptr = (char *)karg.SCB.sc_datapt;
					cptr1=(char *) &dak_disk_size[c][ l];
					cptr[0]=cptr1[3];
					cptr[1]=cptr1[2];
					cptr[2]=cptr1[1];
					cptr[3]=cptr1[0];
					cptr[4]=0;
					cptr[5]=0;
					cptr[6]=2;
					cptr[7]=0;
					karg.SCB.sc_comp_code = SDI_ASW;
					karg.SCB.sc_status = 0;
					if (copyout((caddr_t)&karg, arg, sizeof(struct sb)))
						return(EFAULT);
					return (0);
				default:
					karg.SCB.sc_comp_code = SDI_ASW;
					karg.SCB.sc_status = 0;
					if (copyout((caddr_t)&karg, arg, sizeof(struct sb)))
						return(EFAULT);
					return (0);

			}
			}

			sp = SDI_GETBLK(KM_SLEEP);
			save_priv = sp->SCB.sc_priv;
			bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));
			sp->SCB.sc_priv = save_priv;

			q = &LU_Q(c, b, t, l);
			if (q->q_sc_cmd == NULL) {
			/*
			 * Allocate space for the SCSI command and add 
			 * sizeof(int) to account for the scm structure
			 * padding, since this pointer may be adjusted -2 bytes
			 * and cast to struct scm.  After the adjustment we
			 * still want to be pointing within our allocated space!
			 */
				q->q_sc_cmd = (char *)KMEM_ZALLOC(MAX_CMDSZ + 
					sizeof(int), KM_SLEEP) + sizeof(int);
			}
			sp->SCB.sc_cmdpt = q->q_sc_cmd;
			bp = getrbuf(KM_SLEEP);
			if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
			    sp->SCB.sc_cmdsz)) {
				errnum = EFAULT;
				goto done;
			}

			sp->SCB.sc_dev.sa_lun = (uchar_t)l;
			sp->SCB.sc_dev.sa_exta = (uchar_t)t;
			sp->SCB.sc_dev.sa_ct = SDI_SA_CT(dak_ltog[c], t);

			rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;

#if (PDI_VERSION < PDI_SVR42MP)
			bp->b_private = (char *)sp;
#else
			bp->b_priv.un_ptr = sp;
#endif

			/*
			 * If the job involves a data transfer then the
			 * request is done thru physio() so that the user
			 * data area is locked in memory. If the job doesn't
			 * involve any data transfer then dak_pass_thru()
			 * is called directly.
			 */
			if (sp->SCB.sc_datasz > 0)
			{
				struct iovec  ha_iov;
				struct uio    ha_uio;
				int offset = 0;
				char op;

				ha_iov.iov_base = sp->SCB.sc_datapt;
				ha_iov.iov_len = sp->SCB.sc_datasz;
				ha_uio.uio_iov = &ha_iov;
				ha_uio.uio_iovcnt = 1;
				op = q->q_sc_cmd[0];
				if (op == SS_READ || op == SS_WRITE) {
					struct scs *scs = 
						(struct scs *)q->q_sc_cmd;
					offset = (sdi_swap16(scs->ss_addr) + 
					(scs->ss_addr1 << 16)) * DAK_BLKSIZE;
				}
				if (op == SM_READ || op == SM_WRITE) {
					struct scm *scm = 
					(struct scm *)((char *)q->q_sc_cmd - 2);
					offset = sdi_swap32(scm->sm_addr) * 
						DAK_BLKSIZE;
				}
				ha_uio.uio_offset = offset;
				ha_uio.uio_segflg = UIO_USERSPACE;
				ha_uio.uio_fmode = 0;
				ha_uio.uio_resid = sp->SCB.sc_datasz;

				if (errnum = physiock((void(*)())dak_pass_thru0, bp, dev, rw,
				    HBA_MAX_PHYSIOCK, &ha_uio)) {
					goto done;
				}
			} else {
				bp->b_un.b_addr = sp->SCB.sc_datapt;
				bp->b_bcount = sp->SCB.sc_datasz;
				bp->b_blkno = NULL;
				bp->b_edev = dev;
				bp->b_flags |= rw;

				dak_pass_thru(bp);  /* fake physio call */
				biowait(bp);
			}

			/* update user SCB fields */

			karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
			karg.SCB.sc_status = sp->SCB.sc_status;
			karg.SCB.sc_time = sp->SCB.sc_time;

			if (copyout((caddr_t)&karg, arg, sizeof(struct sb))) {
				errnum = EFAULT;
				goto done;
			}

			done:
			freerbuf(bp);
			sdi_freeblk(sp);
			return(errnum);
		}

	case B_GETTYPE:
		if (copyout("scsi", ((struct bus_type *)arg)->bus_name, 5)) {
			return(EFAULT);
		}
		if (copyout("scsi", ((struct bus_type *)arg)->drv_name, 5)) {
			return(EFAULT);
		}
		break;

	case	HA_VER:
		if (copyout((caddr_t)&dak_sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;

	case	SDI_HBANAME:
		if (copyout((caddr_t)dakidata[c].name, arg, 
			strlen(dakidata[c].name)))
			return(EFAULT);
		break;

	case DIOSTART_CHAN:
	case DIODCMD:{
		int error;
		ulong_t xfersize=0;
		int uoffset;
		struct dacimbx	*ioc_mbx;

		bp = geteblk();
		bp->b_iodone = NULL;
		if (copyin(arg,bp->b_un.b_addr,sizeof(struct dacimbx)) ) {
			cmn_err(CE_NOTE,"copyin error DIODCMD\n");
			brelse(bp);
			return(EFAULT);
		}

		ioc_mbx = (struct dacimbx *)(void *)bp->b_un.b_addr;
		u_bufaddr = ioc_mbx->ioc_buf;
		xfersize = ioc_mbx->ioc_bufsz;
	  	if ( ioc_mbx->ioc_mbx0 == PORT_READ ){
			unsigned int port_in;
			port_in = (int )inb((ioc_mbx->ioc_mbx2 << 8)+ioc_mbx->ioc_mbx3);
			if ( copyout((caddr_t)&port_in,(caddr_t)u_bufaddr,4)== -1){
				brelse(bp);
				return(EFAULT);
		   	}
			brelse(bp);
			return 0;
	  	} 
	  	if ( ioc_mbx->ioc_mbx0 == MIOC_ADP_INFO ){
			int c;
			struct _ADAPTER_INFO adinfo;

			if ((xfersize == 0) || copyin((caddr_t)u_bufaddr, 
			     (caddr_t)&adinfo, xfersize)== -1){
				brelse(bp);
				return(EFAULT);
		   	}
			c = adinfo.AdapterIndex;
			if (c >= dak_cntls || dakidata[c].active == 0) {
				brelse(bp);
				return(NOMORE_ADAPTERS);
			}
			ha = &dak_sc_ha[c];
    			adinfo.AdpFeatures.MaxSysDrv = 8;
    			adinfo.AdpFeatures.MaxTgt = ha->ha_ntargets;
    			adinfo.AdpFeatures.MaxChn = dakidata[c].idata_nbus;
    			adinfo.AdpFeatures.MaxCmd = ha->ha_maxjobs;
    			adinfo.AdpFeatures.MaxSgEntries = 16;

    			adinfo.SysResources.BusInterface =  ha->ha_iobustype;
    			adinfo.SysResources.IrqVector = dakidata[c].iov;
    			adinfo.SysResources.IrqType = (ha->ha_itype==4)?1:0;
    			adinfo.SysResources.IoAddress = dakidata[c].ioaddr1;

    			adinfo.VerControl.MinorFirmwareRevision = ha->ha_minor_fwver;
    			adinfo.VerControl.MajorFirmwareRevision = ha->ha_major_fwver;

			if ( copyout((caddr_t)&adinfo, (caddr_t)u_bufaddr,
			     xfersize)== -1){
				brelse(bp);
				return(EFAULT);
		   	}
			brelse(bp);
			return(0);
		}
		bp->b_private = (char *)B_DCMD;
		bp->b_edev = dev;
		bp->b_blkno = 0;
		
		bp->b_bcount = 1;
		bp->av_forw=ha->ioctl_que;
		ha->ioctl_que = bp;
		DAK_CTL_LOCK(opri, ha);
		if ((*ha->ha_func.dak_iocmd)(c)) {
			DAK_CTL_UNLOCK(opri, ha);
			brelse(bp);
			return (ENOMEM);
		}
		DAK_CTL_UNLOCK(opri, ha);
		biowait(bp);
		uoffset = 0;
		shortptr = (unsigned short *)(void *)((caddr_t)ioc_mbx+16);
		error = *shortptr;
		if ( ioc_mbx->ioc_mbx0 == DAC_RBLD ){
			copyout((caddr_t)ioc_mbx+16,u_bufaddr,2);
			uoffset = 2;
		}
		if ( xfersize ){
			if (copyout((caddr_t)ioc_mbx + 32,
					u_bufaddr+uoffset,xfersize) ){
				cmn_err(CE_NOTE,"copyout error DIODCMD\n");
				brelse(bp);
				return(EFAULT);
			}
		}
		brelse(bp);
		return error;
		}
	case DIODCDB:{
		int error;
		struct daccdb *cdbptr;
		struct proc	*procp;

		if (error = drv_priv(cred_p)){
			cmn_err(CE_NOTE,"!DIODCDB No privledge \n");
			return(EPERM);
		}

		bp = geteblk();
		if (copyin(arg, bp->b_un.b_addr,sizeof(struct daccdb)) ) {
			cmn_err(CE_NOTE,"copyin error DIODCDB\n");
			brelse(bp);
			return(EFAULT);
		}
		bp->b_private = (char *)B_DCDB;
		bp->b_blkno = 0;
		bp->b_bcount = 1;
		bp->b_edev = dev;
		bp->b_resid = 0;
		bp->b_iodone = NULL;
		cdbptr = ( struct daccdb *)(void *)bp->b_un.b_addr;
		u_bufaddr = cdbptr->cdb_databuf;
		copyin((caddr_t)cdbptr->cdb_databuf,
			(caddr_t)(bp->b_un.b_addr+DATAOFST),512);
		drv_getparm(UPROCP, (ulong*)&procp);
		cdbptr->cdb_databuf=(caddr_t)vtop(bp->b_un.b_addr+DATAOFST, procp);
		cdbptr->cdb_cmdctl |= CDB_DISCON;
		cdbptr->cdb_senselen = 14; /* 14 bytes of sense data */	

		bp->av_forw=ha->ioctl_que;
		ha->ioctl_que = bp;
		DAK_CTL_LOCK(opri, ha);
		if ((*ha->ha_func.dak_iocmd)(c)) {
			DAK_CTL_UNLOCK(opri, ha);
			brelse(bp);
			return (ENOMEM);
		}
		DAK_CTL_UNLOCK(opri, ha);
		biowait(bp);
		error = 0;
		shortptr = (unsigned short *)(void *)(bp->b_un.b_addr+16);
		error |= *shortptr;
		if ( !error ){
			if ( copyout((caddr_t)(bp->b_un.b_addr+DATAOFST),
				u_bufaddr,cdbptr->cdb_xfersz ) ){
			cmn_err(CE_NOTE,"DIODCDB copyout error on user buf\n");
			brelse(bp);
			error = EFAULT;
			}
		}
		brelse(bp);
		return error;
		}
	default:
		return(EINVAL);
	}

	return(0);
}

/*
** Function name: dakhalt()
** Description:
**      Flushes all adapters.
**
*/

void
dakhalt()
{

	int c;
	register struct dak_scsi_ha	 *ha;

	cmn_err(CE_NOTE,"dak: Flushing DAC960");
	for (c = 0; c < dak_cntls; c++) {
		ha = &dak_sc_ha[c];
		if(!ha || !ha->ha_base )
			return ;
		if (!dakidata[c].active)
			continue;
		(*ha->ha_func.dak_halt)(ha);
	}
}

/*
** Function name: dak_eisa_halt()
** Description:
**      Flushes all eisa adapters.
**
*/

STATIC void
dak_eisa_halt(struct dak_scsi_ha *ha)
{
	unsigned long cptr = 0;

	while(inb(ha->ha_base + 0x0d) & 1)  /* loop till free */
		drv_usecwait(10);
		
	/* Disable Interrupts */
	outb(ha->ha_base + 0x09, 0);
	outb(ha->ha_base + 0x0e, 0);
	
	dak_eisa_issue_cmd(ha, OP_FLUSH, FLUSH_ID, (char *)&cptr, DAK_INTR_OFF);
}

/*
** Function name: dak_mca_halt()
** Description:
**      Flushes all mca adapters.
**
*/

STATIC void
dak_mca_halt(struct dak_scsi_ha *ha)
{
	unsigned long cptr = 0;

	while(ha->ha_mbx_addr[0])
		drv_usecwait(10);
		
	/* Disable Interrupts */
	outb(ha->ha_base + 0x05, 0x42);
	
	dak_mca_issue_cmd(ha, OP_FLUSH, FLUSH_ID, (char *)&cptr, DAK_INTR_OFF);

}

/*
** Function name: dak_pci_halt()
** Description:
**      Flushes all pci adapters.
**
*/

STATIC void
dak_pci_halt(struct dak_scsi_ha *ha)
{
	unsigned long cptr = 0;

	while(inb(ha->ha_base + 0x40) & 1)   /* loop till free */
		drv_usecwait(10);
		
	/* Disable Interrupts */
	outb(ha->ha_base + 0x43, 0);
	
	dak_pci_issue_cmd(ha, OP_FLUSH, FLUSH_ID, (char *)&cptr, DAK_INTR_OFF);

}

/*
** Function name: dakintr()
** Description:
**      Driver interrupt handler entry point.
**      Called by kernel when a host adapter interrupt occurs.
** Calling/Exit State:
**	None.
**	ha_ctl_lock acquired within.
*/

void
dakintr(unsigned int vect)
{
	register struct dak_scsi_ha	 *ha;
	register int             c;

	/********************************************************************
	** Determine which host adapter interrupted from the interrupt     **
	*********************************************************************/
	for (c = 0; c < dak_cntls; c++) {
		ha = &dak_sc_ha[c];
		if (ha->ha_vect == vect) {
			(*ha->ha_func.dak_intr)(c, ha);
		}
	}
	return;
}

/*
** Function name: dak_eisa_intr()
** Description:
**      Driver interrupt handler entry point.
**      Called by kernel when a host adapter interrupt occurs.
** Calling/Exit State:
**	None.
**	ha_ctl_lock acquired within.
*/

STATIC void
dak_eisa_intr(int c, struct dak_scsi_ha *ha)
{
	register struct dak_ccb      *cp;
	unsigned char error,status,retid;
	register char *		 message = "";
	pl_t opri;

	DAK_CTL_LOCK(opri, ha);
	if ( inb(ha->ha_base + 0x0f) & 1 ) {
		if (!dakidata[c].active) {
			/* Ack the interrupt */
			outb(ha->ha_base + 0x0d,2);
			DAK_CTL_UNLOCK(opri, ha);
			return;
		}
		ha->ha_npend--;
	} else {
		/* Interrupt no for this controller */
		DAK_CTL_UNLOCK(opri, ha);
		return;
	}
	status=inb(ha->ha_base+0x1e);
	error=inb(ha->ha_base+0x1f);
	retid=inb(ha->ha_base+0x1d);
	outb(ha->ha_base + 0x0f,3);
	cp = (struct dak_ccb *)dak_command_array[c][retid];
	outb(ha->ha_base + 0x0d,2);  /* Ack the interrupt */

	cp = (struct dak_ccb *)dak_command_array[c][retid];
	if(cp == 0) {
		DAK_CTL_UNLOCK(opri, ha);
		cmn_err(CE_WARN, "DAC Host Adapter %c: Spurious interrupt received%s\n", c, message);
		return;
	}
	dak_command_array[c][retid]=0;

	if(dak_command_type[c][retid] == IOCTL)
	{
		unsigned short *shortptr;
		struct buf	*bp;
		
		DAK_CTL_UNLOCK(opri, ha);
		bp = (struct buf *)cp;
		shortptr=(unsigned short *)(void *)(bp->b_un.b_addr+16);
		*shortptr = ((unsigned short)error << 8) | status;

		bp->b_resid = 0;
		biodone(bp);
		return;
	}
	DAK_CTL_UNLOCK(opri, ha);
	cp->c_active = FALSE;		/* Mark it not active.         */

	cp->CP_Controller_Status  = ((unsigned short)error << 8) | status;
	cp->CP_SCSI_Status        = 0;

	if (cp->c_bind == NULL) {
		/* The command was issued internally, and dak_wait was
		 * called to wait for completion.  
		 */
		dak_waitflag = FALSE;
		return;
	}

	if (cp->c_bind->sb_type == SFB_TYPE) /* Call the appropriate command*/
		dak_ha_done(c, cp, cp->CP_Controller_Status);    /*   done routine.             */
	else /*           SFB or SCB        */
		dak_done(c, cp, cp->CP_Controller_Status);

	dak_waitflag = FALSE;
	/*
	 * Check for queues that still need service
	 * Look at ha_que first without the controller lock,
	 * then, lock and look again.
	 */
	if (ha->ha_que) {
		struct dak_scsi_lu *q;

		DAK_CTL_LOCK(opri, ha);
		if ((q = ha->ha_que) != NULL) {
			ha->ha_que = q->q_next;
			DAK_CTL_UNLOCK(opri, ha);

			DAK_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~DAK_QNEEDSRV;
			dak_next(q);
		} else
			DAK_CTL_UNLOCK(opri, ha);
	}

	return;
}

/*
** Function name: dak_mca_intr()
** Description:
**      Driver interrupt handler entry point.
**      Called by kernel when a host adapter interrupt occurs.
** Calling/Exit State:
**	None.
**	ha_ctl_lock acquired within.
*/

STATIC void
dak_mca_intr(int c, struct dak_scsi_ha *ha)
{
	register struct dak_ccb      *cp;
	char *mbxp = ha->ha_mbx_addr;
	unsigned char error,status,retid;
	register char *		 message = "";
	pl_t opri;

	DAK_CTL_LOCK(opri, ha);
	if(inb(ha->ha_base + 0x07) & 2) {
		if (!dakidata[c].active) {
			/* Ack the interrupt */
			outb(ha->ha_base + 0x04,0xd1);
			DAK_CTL_UNLOCK(opri, ha);
			return;
		}
		ha->ha_npend--;
	} else {
		/* Interrupt no for this controller */
		DAK_CTL_UNLOCK(opri, ha);
		return;
	}
	status = mbxp[0xe];
	error  = mbxp[0xf];
	retid  = mbxp[0xd];
	cp = (struct dak_ccb *)dak_command_array[c][retid];
	outb(ha->ha_base + 0x04,0xd1);

	cp = (struct dak_ccb *)dak_command_array[c][retid];
	if(cp == 0) {
		DAK_CTL_UNLOCK(opri, ha);
		cmn_err(CE_WARN, "DAC Host Adapter %c: Spurious interrupt received%s\n", c, message);
		return;
	}
	dak_command_array[c][retid]=0;

	if(dak_command_type[c][retid] == IOCTL)
	{
		unsigned short *shortptr;
		struct buf	*bp;
		
		DAK_CTL_UNLOCK(opri, ha);
		bp = (struct buf *)cp;
		shortptr=(unsigned short *)(void *)(bp->b_un.b_addr+16);
		*shortptr = ((unsigned short)error << 8) | status;

		bp->b_resid = 0;
		biodone(bp);
		return;
	}
	DAK_CTL_UNLOCK(opri, ha);
	cp->c_active = FALSE;		/* Mark it not active.         */

	cp->CP_Controller_Status  = ((unsigned short)error << 8) | status;
	cp->CP_SCSI_Status        = 0;

	if (cp->c_bind == NULL) {
		/* The command was issued internally, and dak_wait was
		 * called to wait for completion.  
		 */
		dak_waitflag = FALSE;
		return;
	}

	if (cp->c_bind->sb_type == SFB_TYPE) /* Call the appropriate command*/
		dak_ha_done(c, cp, cp->CP_Controller_Status);    /*   done routine.             */
	else /*           SFB or SCB        */
		dak_done(c, cp, cp->CP_Controller_Status);

	dak_waitflag = FALSE;
	/*
	 * Check for queues that still need service
	 * Look at ha_que first without the controller lock,
	 * then, lock and look again.
	 */
	if (ha->ha_que) {
		struct dak_scsi_lu *q;

		DAK_CTL_LOCK(opri, ha);
		if ((q = ha->ha_que) != NULL) {
			ha->ha_que = q->q_next;
			DAK_CTL_UNLOCK(opri, ha);

			DAK_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~DAK_QNEEDSRV;
			dak_next(q);
		} else
			DAK_CTL_UNLOCK(opri, ha);
	}

	return;
}

/*
** Function name: dak_pci_intr()
** Description:
**      Driver interrupt handler entry point.
**      Called by kernel when a host adapter interrupt occurs.
** Calling/Exit State:
**	None.
**	ha_ctl_lock acquired within.
*/

STATIC void
dak_pci_intr(int c, struct dak_scsi_ha *ha)
{
	register struct dak_ccb      *cp;
	unsigned short status;
	unsigned char retid;
	register char *		 message = "";
	pl_t opri;

	DAK_CTL_LOCK(opri, ha);
	if ( inb(ha->ha_base + 0x41) & 1 ) {
		if (!dakidata[c].active) {
			/* Ack the interrupt */
			outb(ha->ha_base + 0x41,1);
			outb(ha->ha_base + 0x40,2);
			DAK_CTL_UNLOCK(opri, ha);
			return;
		}
		ha->ha_npend--;
	} else {
		/* Interrupt no for this controller */
		DAK_CTL_UNLOCK(opri, ha);
		return;
	}
	status=inw(ha->ha_base+0x0e);
	retid=inb(ha->ha_base+0x0d);
	cp = (struct dak_ccb *)dak_command_array[c][retid];
	outb(ha->ha_base + 0x41,1);
	outb(ha->ha_base + 0x40,2);

	cp = (struct dak_ccb *)dak_command_array[c][retid];
	if(cp == 0) {
		DAK_CTL_UNLOCK(opri, ha);
		cmn_err(CE_WARN, "DAC Host Adapter %c: Spurious interrupt received%s\n", c, message);
		return;
	}
	dak_command_array[c][retid]=0;

	if(dak_command_type[c][retid] == IOCTL)
	{
		unsigned short *shortptr;
		struct buf	*bp;
		
		DAK_CTL_UNLOCK(opri, ha);
		bp = (struct buf *)cp;
		shortptr=(unsigned short *)(void *)(bp->b_un.b_addr+16);
		*shortptr = status;

		bp->b_resid = 0;
		biodone(bp);
		return;
	}
	DAK_CTL_UNLOCK(opri, ha);
	cp->c_active = FALSE;		/* Mark it not active.         */

	cp->CP_Controller_Status  = (unsigned char)status & 0xff;
	cp->CP_SCSI_Status        = 0;

	if (cp->c_bind == NULL) {
		/* The command was issued internally, and dak_wait was
		 * called to wait for completion.  
		 */
		dak_waitflag = FALSE;
		return;
	}

	if (cp->c_bind->sb_type == SFB_TYPE) /* Call the appropriate command*/
		dak_ha_done(c, cp, cp->CP_Controller_Status);    /*   done routine.             */
	else /*           SFB or SCB        */
		dak_done(c, cp, cp->CP_Controller_Status);

	dak_waitflag = FALSE;
	/*
	 * Check for queues that still need service
	 * Look at ha_que first without the controller lock,
	 * then, lock and look again.
	 */
	if (ha->ha_que) {
		struct dak_scsi_lu *q;

		DAK_CTL_LOCK(opri, ha);
		if ((q = ha->ha_que) != NULL) {
			ha->ha_que = q->q_next;
			DAK_CTL_UNLOCK(opri, ha);

			DAK_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~DAK_QNEEDSRV;
			dak_next(q);
		} else
			DAK_CTL_UNLOCK(opri, ha);
	}

	return;
}

/*
** Function name: dak_done()
** Description:
**	This is the interrupt handler routine for SCSI jobs which have
**	a controller CB and a SCB structure defining the job.
** Calling/Exit State:
**	No locks held on entry or exit.
**	q_lock acquired within, and held on entry to dak_next().
*/

void
dak_done(c, cp, status)
int		c;	  	/* HA controller number */
register int	status;		/* HA status */
struct dak_ccb	*cp;		/* command block */
{
	register struct dak_scsi_lu  *q;
	register struct sb *sp;
	ulong curtime;

	/* CPID holds t and lun holds l  */
	q = &LU_Q(c, cp->dcdb.device>>4, (char)cp->CPID, (char)(cp->lun & 0x07));

	sp = cp->c_bind;

	ASSERT(sp);

	DAK_SCSILU_LOCK(q->q_opri);
	q->q_flag &= ~DAK_QSENSE;       /* Old sense data now invalid  */
	DAK_SCSILU_UNLOCK(q->q_opri);

	/** Determine completion status of the job ***/

	if ( status == 0 ) {

		sp->SCB.sc_comp_code = SDI_ASW;
		if (cp->dcdb.cdb[0] == SS_INQUIR) {
			struct ident *inqp;
			inqp = (struct ident *)(void *)sp->SCB.sc_datapt;
			if (inqp->id_type == ID_RANDOM)
				inqp->id_type = ID_NODEV;
		}
	} else if (status == RETRY) {

		sp->SCB.sc_comp_code = SDI_RETRY;

	} else {    /* Error bit WAS set in status */

		sp->SCB.sc_status = cp->CP_SCSI_Status;
		if((cp->CPID) == 0) {
			sp->SCB.sc_status = cp->CP_SCSI_Status = SC_NOSENSE;
			sp->SFB.sf_comp_code = SDI_HAERR;
			/* TEMP out CP_SCSI_Status == SC_NOSENSE; */
		} else if((status & 0xffff) == CHK_COND) {
			dak_break_point();
			sp->SCB.sc_status = CHK_COND;
			sp->SCB.sc_comp_code = SDI_CKSTAT;
			HBA_SENSE_COPY(cp->dcdb.sense, sp);
			/* Cache request sense info into QueSense  */
			DAK_SCSILU_LOCK(q->q_opri);
			bcopy((caddr_t)(cp->dcdb.sense),
			    (caddr_t)(&q->q_sense)+1, sizeof(q->q_sense)-1);
			q->q_flag |= DAK_QSENSE;
			DAK_SCSILU_UNLOCK(q->q_opri);
		} else {
			sp->SCB.sc_comp_code = SDI_NOSELE;
		}
	}

	drv_getparm(LBOLT, (ulong *)&curtime);
	sp->SCB.sc_time = ((curtime - cp->c_start)+HZ-1) / HZ;

	sdi_callback(sp);	/* call target driver interrupt handler */

	dak_freeccb(c, cp);	/* Release SCSI command block. */

	DAK_SCSILU_LOCK(q->q_opri);
	q->q_flag &= ~DAK_QBUSY;/* Clear busy flag on Que entry*/
	dak_next(q);		/* Send Next Job on the Queue. */
}


/*
** Function name: dak_ha_done()
** Description:
**	This is the interrupt handler routine for SCSI jobs which have
**      a controller CP and a SFB structure defining the job.
** Calling/Exit State:
**	No locks held on entry or exit.
**	q_lock acquired within, and held on entry to dak_next().
*/

void
dak_ha_done(int c, struct dak_ccb *cp, int status)
{
	register struct dak_scsi_lu  *q;
	register struct sb *sp;

	/* CPID holds t and lun holds l  */
	q = &LU_Q(c, cp->dcdb.device>>4, (char)cp->CPID, (char)(cp->lun & 0x07));

	sp = cp->c_bind;

	ASSERT(sp);

	/* Determine completion status of the job */

	if ( status ) {
		sp->SFB.sf_comp_code = SDI_HAERR;
		q->q_flag |= DAK_QSUSP;
	} else {
		sp->SFB.sf_comp_code = SDI_ASW;
	}

	sdi_callback(sp);	/* call target driver interrupt handler */

	dak_freeccb(c, cp);	/* Release SCSI command block. */

	DAK_SCSILU_LOCK(q->q_opri);
	q->q_flag &= ~DAK_QBUSY;/* Clear Busy flag.            */
	dak_next(q);		/* Send Next Job on the Queue. */
}



/*
** Function name: dak_timer()
** Description:
**	Scheduled at minute intervals to perform timing.  Callout
**	routines (like this one) run at hi priority.  No spl necessary.
*/

void
dak_timer(c)
{
	struct dak_scsi_ha *ha = &dak_sc_ha[c];
	register struct dak_ccb *cp;
	register int n;
	ulong curtime;

	cp = &ha->ha_ccb[0];	/* Get controller dak_ccb pointer. */
	for (n = 0; n < NCPS; n++, cp++)
	{
		if (!cp->c_active)	/* If no command pending       */
			continue;	/*      or                     */
		if (cp->c_time == 0)	/* command not being timed     */
			continue;	/* then just continue.         */

		drv_getparm(LBOLT, (ulong *)&curtime);
		if ((cp->c_time + cp->c_start) > curtime)
			continue;	/* Not timed out yet.          */

		/******************************************
		** Job timed out - terminate the command **
		*******************************************/
		cp->c_time = 0;		/* Set as not being timed.     */
#if 0
		(*ha->ha_func.dak_send)(c, ABORT, cp);	/* Send ABORT Command.         */
#endif
	}

	(void) ITIMEOUT(dak_timer, (caddr_t)c, 60*HZ, pldisk);
}


/*===========================================================================*/
/* SCSI Driver Interface (SDI-386) Functions
/*===========================================================================*/


/*
** Function name: dakinit()
** Description:
**	This is the initialization routine for the DAC HA driver.
**	Called by the main init loop if static or dak_load if loadable.
*/

int
dakinit()
{
	register struct dak_scsi_ha *ha;
	register struct dak_scsi_lu *q;
	register dak_dma_t  *dp;
	int  i, j, c;
	uint	bus_p;
	int sleepflag;
	int maxchannel = 1;

	dakinit_time = TRUE;

#if (PDI_VERSION >= PDI_UNIXWARE20)
	/*
	 * Allocate and populate a new idata array based on the
	 * current hardware configuration for this driver.
	 */
	dakidata = sdi_hba_autoconf("dak", _dakidata, &dak_cntls);
	if(dakidata == NULL)    {
		return (-1);
	}
#endif /* (PDI_VERSION >= PDI_UNIXWARE20) */

	if (drv_gethardware(IOBUS_TYPE, &bus_p) == -1) {
		cmn_err(CE_WARN,"!DAC Host Adapter: drv_gethardware(IOBUS_TYPE) failed");
		return(-1);
	}

	switch(bus_p) {
	case BUS_EISA:
		dak_sdi_ver.sv_machine = SDI_386_EISA;
		break;
	case BUS_MCA:
		dak_sdi_ver.sv_machine = SDI_386_MCA;
#if (PDI_VERSION >= PDI_UNIXWARE20)
		sdi_mca_conf(dakidata, dak_cntls, dak_mca_conf);
#endif /* (PDI_VERSION >= PDI_UNIXWARE20) */
		break;
	default:
		dak_sdi_ver.sv_machine = SDI_386_AT;
		break;
	}

	for (i=0; i < MAX_HAS; i++) {
		dak_gtol[i] = dak_ltog[i] = -1;
	}

	sleepflag = dak_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	dak_sdi_ver.sv_release = 1;
	dak_sdi_ver.sv_modes   = SDI_BASIC1;

	/* need to allocate space for sc_ha, must be contiguous */
	dak_ha_structsize = dak_cntls*(sizeof (struct dak_scsi_ha));

	for (i = 2; i < dak_ha_structsize ; i *= 2) ;
	dak_ha_structsize = i;

	dak_sc_ha = (struct dak_scsi_ha *)kmem_zalloc(dak_ha_structsize, sleepflag);
	if (dak_sc_ha == NULL) {
		cmn_err(CE_WARN,"DAC Host Adapter: Initialization error - cannot allocate host adapter structures");
		return(-1);
	}
		
	/* allocate space for ccb's & LU queues, must be contiguous */
	dak_ccb_structsize = NCPS*(sizeof (struct dak_ccb));
	dak_luq_structsize = MAX_EQ*(sizeof (struct dak_scsi_lu));
	dak_dma_listsize = NDMA * sizeof(dak_dma_t);

#if (PDI_VERSION < PDI_SVR42MP)
	for (i = 2; i < dak_ccb_structsize; i *= 2);
	dak_ccb_structsize = i;

	for (i = 2; i < dak_luq_structsize; i *= 2);
	dak_luq_structsize = i;

	for (i = 2; i < dak_dma_listsize; i *= 2);
	dak_dma_listsize = i;
#endif /* (PDI_VERSION < PDI_SVR42MP) */

	for (c = 0; c < dak_cntls; c++) {
		ha = &dak_sc_ha[c];
		ha->ha_id     = dakidata[c].ha_id;
		ha->ha_base = dakidata[c].ioaddr1;
  		ha->ha_vect = dakidata[c].iov;

		if ((ha->ha_iobustype = dak_getiobus(c)) < 0) {
			cmn_err(CE_NOTE,"!DAC Host Adpater[%d] undefined iobus", c);
			continue;
		}
		ha->ha_itype = dak_getitype(c);
		ha->ha_func = dak_functions_array[ha->ha_iobustype];

		/* initialize HA */
		if ((*ha->ha_func.dak_ha_init)(c, sleepflag) != 0) 
			continue;
		dakidata[c].iov= ha->ha_vect;
		cmn_err(CE_NOTE,"!DAC Host Adpater[%d]: host adapter using IRQ %d",c,ha->ha_vect);

#if (PDI_VERSION >= PDI_UNIXWARE20)
		maxchannel = dakidata[c].idata_nbus;
#endif
		/* controller was found now alloc ccb's, LU Q's */
		ha->ha_ccb = (struct dak_ccb *)KMEM_ZALLOC(dak_ccb_structsize, sleepflag);
		if (ha->ha_ccb == NULL) {
			cmn_err(CE_WARN,"DAC Host Adapter: Initialization error - cannot allocate command blocks");
			continue;
		}

		ha->ha_dev = (struct dak_scsi_lu *)kmem_zalloc(
			dak_luq_structsize*maxchannel, sleepflag);
		if (ha->ha_dev == NULL) {
			cmn_err(CE_WARN,"DAC Host Adapter: Initialization error - cannot allocate logical unit queues");
			break;
		}
		for (j = 0; j < MAX_EQ*maxchannel; j++) {
			q = &ha->ha_dev[j];
			q->controller = (uchar_t)c;
		}
		
		dak_init_cps(c);
		dakidata[c].active = 1;
		dak_brdcnt++;
		dak_lockinit(c);	/* Allocate and Initialize MP locks*/
	}
	if (!dak_brdcnt) {
		kmem_free(dak_sc_ha, dak_ha_structsize );
		return(-1);
	}

	/* Build list of free DMA lists */
   	dak_dfreelist = NULL;
	for (c = 0; c < dak_cntls; c++) {
	    dak_dmalistpool = (dak_dma_t *)KMEM_ZALLOC(dak_dma_listsize, sleepflag);
	    if(dak_dmalistpool == NULL) {
		/*
		 *+ DAK Allocation of DMA list pool failed.
		 */
		cmn_err(CE_WARN, "%s: Initalization error allocating dak_dmalistpool\n", dakidata[0].name);
		kmem_free(dak_sc_ha, dak_ha_structsize );
		return(-1);
	    }

	    /* Build list of free DMA lists */

	    for (i = 0; i < NDMA; i++) {
		dp = &dak_dmalistpool[i];
		dp->d_next = dak_dfreelist;
		dak_dfreelist = dp;
	    }
        }
#if (PDI_VERSION >= PDI_UNIXWARE20)
	/*
	 * Attach interrupts for each "active" device. The driver
	 * must set the active filed of the idata structure to 1
	 * for each device it has configured.
	 *
	 * Note: Interrupts are attached here, in init(), as opposed
	 * to load(), because this is required for static autoconfig
	 * drivers as well as loadable.
	 */
	sdi_intr_attach(dakidata, dak_cntls, dakintr, dak_devflag);
#endif /* (PDI_VERSION >= PDI_UNIXWARE20) */

	return(0);
}

/*
** Function name: dakstart()
** Description:
**	This is the start routine for the DAC HA driver.
**	Called by the main I/O start loop if static or dak_load if loadable.
**	Interrupts are now enabled.
*/

int
dakstart()
{
	register struct dak_scsi_ha *ha;
	int  i, c;
	int cntl_num;
	int maxchannel = 1;

	HBA_IDATA(dak_cntls);

	for (c = 0; c < dak_cntls; c++) {

		if (!dakidata[c].active)
			continue;

		if ((cntl_num = sdi_gethbano(dakidata[c].cntlr)) <= -1) {
			cmn_err(CE_WARN,"%s: No HBA number available.",dakidata[c].name);
			dakidata[c].active = 0;
			continue;
		}
		dakidata[c].cntlr = cntl_num;
		dak_gtol[cntl_num] = c;
		dak_ltog[dak_gtol[cntl_num]] = cntl_num;

#ifdef DAK_DEBUG
		if (dak_debug) {
		cmn_err(CE_NOTE,"DAC Host Adapter:sdi_register");
		cmn_err(CE_NOTE,"DAC Host Adapter: %d",dakidata[c].version_num);
		cmn_err(CE_NOTE,"DAC Host Adapter: %d",dakidata[c].ha_id);
		cmn_err(CE_NOTE,"DAC Host Adapter: %d",dakidata[c].ioaddr1);
		cmn_err(CE_NOTE,"DAC Host Adapter: %d",dakidata[c].dmachan1);
		cmn_err(CE_NOTE,"DAC Host Adapter: %d",dakidata[c].iov);
		cmn_err(CE_NOTE,"DAC Host Adapter: %d",dakidata[c].cntlr);
		cmn_err(CE_NOTE,"DAC Host Adapter: %d",dakidata[c].active);
		}
#endif /* DAK_DEBUG */
		if ((cntl_num=sdi_register(&dakhba_info,&dakidata[c])) < 0) {
			cmn_err(CE_WARN,"!%s: HA %d SDI register slot %d failed",
				dakidata[c].name, c, cntl_num);
			dakidata[c].active = 0;
			continue;
		}
		dak_hacnt++;

		dak_timer(c);   /* start timer */
	}
	/* Now cleanup any unnecessary structs */
	for (i = 0; i < dak_cntls; i++) {
		dak_lockclean(i);
		if( !dakidata[i].active) {
			ha = &dak_sc_ha[i];
			if( ha->ha_ccb != NULL) {
                                kmem_free((void *)ha->ha_ccb, dak_ccb_structsize);
                                ha->ha_ccb = NULL;
			}
			if( ha->ha_dev != NULL) {
#if (PDI_VERSION >= PDI_UNIXWARE20)
				maxchannel = dakidata[i].idata_nbus;
#endif
				kmem_free((void *)ha->ha_dev, 
					dak_luq_structsize*maxchannel);
				ha->ha_dev = NULL;
			}
		}
	}

	if (dak_hacnt == 0) {
		if (dakidata == NULL)
			dakidata = _dakidata;
		cmn_err(CE_NOTE,"!%s: %d HAs found, %d HAs registered.", 
			dakidata[0].name, dak_brdcnt, dak_hacnt);
		/*
		 * Free ha structures only if no boards exist.
		 * since interrupts might still have to be processed
		 * for a statically loaded driver.
		 */
		if (dak_brdcnt == 0) {
			if (dak_sc_ha)
				kmem_free((void *)dak_sc_ha, dak_ha_structsize);
			dak_sc_ha = NULL;
		}
		return(-1);
	}
	/*
	 * Clear init time flag to stop the HA driver
	 * from waiting for interrupts and begin taking
	 * interrupts normally.
	 */
	dakinit_time = FALSE;

	return(0);
}


/*
** Function name: daksend()
** Description:
** 	Send a SCSI command to a controller.  Commands sent via this
**	function are executed in the order they are received.
** Calling/Exit State:
**	No locks held on entry or exit.
**	q_lock acquired within, and held on entry to dak_next().
*/
/* static char cptrs[10]; */

/*ARGSUSED*/
STATIC long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	register struct scsi_ad *sa;
	register struct dak_scsi_lu *q;
	register dak_sblk_t *sp = (dak_sblk_t *) hbap;
	register int	c, b, t, l;
	struct scs	     *inq_cdb;

	if (sp->sbp->sb.sb_type != SCB_TYPE)
	{
		return (SDI_RET_ERR);
	}

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = dak_gtol[SDI_EXHAN(sa)];
	t = SDI_EXTCN(sa);
	l = SDI_EXLUN(sa);
	b = SDI_BUS(sa);

	inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;
	if(b == 0 && t == 0) {
		char *cptr, *cptr1;
		if(!dak_disk_size[c][ l]) {
			sp->sbp->sb.SCB.sc_comp_code = SDI_SCBERR;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
      		}
		switch(inq_cdb->ss_op) {
			case SS_READ:
			case SS_WRITE:
			case SM_READ:
			case SM_WRITE:
				break;		/*  tackle later */
			case SS_INQUIR:
				{
				struct ident	 inq;

				bzero((char *)&inq, sizeof(struct ident));
				inq.id_type = ID_RANDOM;
				(void)strncpy(inq.id_vendor, dak_disk_names, 
					VID_LEN+PID_LEN+REV_LEN);
				bcopy((char *)&inq, sp->sbp->sb.SCB.sc_datapt,
					sp->sbp->sb.SCB.sc_datasz);
				sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
				sp->sbp->sb.SCB.sc_status = 0;
				sdi_callback(&sp->sbp->sb);
				return (SDI_RET_OK);
				}
			case SM_RDCAP:
				cptr = (char *)sp->sbp->sb.SCB.sc_datapt;
				cptr1=(char *) &dak_disk_size[c][ l ];
				cptr[0]=cptr1[3];
				cptr[1]=cptr1[2];
				cptr[2]=cptr1[1];
				cptr[3]=cptr1[0];
				cptr[4]=0;
				cptr[5]=0;
				cptr[6]=2;
				cptr[7]=0;
				sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
				sp->sbp->sb.SCB.sc_status = 0;
				sdi_callback(&sp->sbp->sb);
				return (SDI_RET_OK);
			default:
				sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
				sp->sbp->sb.SCB.sc_status = 0;
				sdi_callback(&sp->sbp->sb);
				return (SDI_RET_OK);
		}
			
	} else {
		if( l != 0 )  {
			sp->sbp->sb.SCB.sc_comp_code = SDI_SCBERR;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}
	}

	/* change #1 */		/* XXX Check this */
	if (dak_illegal(SDI_HAN(sa), b, t, l) )
	{
		sp->sbp->sb.SCB.sc_comp_code = SDI_SCBERR;
		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_OK);
	}

	q = &LU_Q(c, b, t, sa->sa_lun);
	DAK_SCSILU_LOCK(q->q_opri);

	if (q->q_flag & DAK_QPTHRU) {
		DAK_SCSILU_UNLOCK(q->q_opri);
		return (SDI_RET_RETRY);
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	dak_putq(q, sp);
	if ((dak_do_conc_io[t] || ( !(q->q_flag & DAK_QBUSY))) &&
		( !(q->q_flag & DAK_QSUSP)))
	{
		dak_next(q);
	} else {
		DAK_SCSILU_UNLOCK(q->q_opri);
	}

	return (SDI_RET_OK);
}


/*
** Function name: dakicmd()
** Description:
**	Send an immediate command.  If the logical unit is busy, the job
**	will be queued until the unit is free.  SFB operations will take
**	priority over SCB operations.
** Calling/Exit State:
**	No locks held on entry or exit.
**	q_lock acquired within, and held on entry to dak_next().
*/

/*ARGSUSED*/
STATIC long
HBAICMD(struct  hbadata *hbap, int sleepflag)
{
	register struct scsi_ad *sa;
	register struct dak_scsi_lu *q;
	register dak_sblk_t *sp = (dak_sblk_t *) hbap;
	register int	c, b, t, l;
	struct scs	     *inq_cdb;

	switch (sp->sbp->sb.sb_type)
	{
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = dak_gtol[SDI_EXHAN(sa)];
		t = SDI_EXTCN(sa);
		l = SDI_EXLUN(sa);
		b = SDI_BUS(sa);
		q = &LU_Q(c, b, t, sa->sa_lun);

		/* change #1 */		/* XXX Check this */
		if (dak_illegal(SDI_HAN(sa), b, t, l))
		{
			sp->sbp->sb.SFB.sf_comp_code = SDI_SFBERR;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func)
		{
		case SFB_RESUME:
			DAK_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~DAK_QSUSP;
			dak_next(q);
			break;
		case SFB_SUSPEND:
			DAK_SCSILU_LOCK(q->q_opri);
			q->q_flag |= DAK_QSUSP;
			DAK_SCSILU_UNLOCK(q->q_opri);
			break;
		case SFB_ABORTM:
		case SFB_RESETM:
			DAK_SCSILU_LOCK(q->q_opri);
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
			dak_putq(q, sp);
			dak_next(q);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			dak_flushq(q, SDI_QFLUSH, 0);
			break;
		case SFB_NOPF:
			break;
		default:
			sp->sbp->sb.SFB.sf_comp_code = SDI_SFBERR;
		}

		sdi_callback(&sp->sbp->sb);
		return (SDI_RET_OK);

	case ISCB_TYPE:
		sa = &sp->sbp->sb.SCB.sc_dev;
		c = dak_gtol[SDI_EXHAN(sa)];
		t = SDI_EXTCN(sa);
		l = SDI_EXLUN(sa);
		b = SDI_BUS(sa);
		q = &LU_Q(c, b, t, sa->sa_lun);

		inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;

		if(b == 0 && t == 0)
		{
			char *cptr, *cptr1;
			if(!dak_disk_size[c][ l ]) {
				sp->sbp->sb.SCB.sc_comp_code = SDI_NOSELE;
				sdi_callback(&sp->sbp->sb);
				return (SDI_RET_OK);
			}
			switch(inq_cdb->ss_op) {

			case SS_READ:
			case SS_WRITE:
			case SM_READ:
			case SM_WRITE:
				break;		/*  tackle later */
			case SS_INQUIR:
				{
				struct ident	 inq;

				bzero((char *)&inq, sizeof(struct ident));
				inq.id_type = ID_RANDOM;
				inq.id_type =  ID_RANDOM ;
				inq.id_ver = 1 ;
				inq.id_form = 1 ;
				inq.id_len = 0x1f ;
				(void)strncpy(inq.id_vendor, dak_disk_names, 
					VID_LEN+PID_LEN+REV_LEN);
				bcopy((char *)&inq, sp->sbp->sb.SCB.sc_datapt,
					sp->sbp->sb.SCB.sc_datasz);
				sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
				sp->sbp->sb.SCB.sc_status = 0;
				sdi_callback(&sp->sbp->sb);
				return (SDI_RET_OK);
				}
			case SM_RDCAP:
				cptr = (char *)sp->sbp->sb.SCB.sc_datapt;
				cptr1=(char *) &dak_disk_size[c][ l];
				cptr[0]=cptr1[3];
				cptr[1]=cptr1[2];
				cptr[2]=cptr1[1];
				cptr[3]=cptr1[0];
				cptr[4]=0;
				cptr[5]=0;
				cptr[6]=2;
				cptr[7]=0;
				sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
				sp->sbp->sb.SCB.sc_status = 0;
				sdi_callback(&sp->sbp->sb);
				return (SDI_RET_OK);
			default:
				sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
				sp->sbp->sb.SCB.sc_status = 0;
				sdi_callback(&sp->sbp->sb);
				return (SDI_RET_OK);
			}
			
		} else {
			if( l != 0 ) {
				sp->sbp->sb.SCB.sc_comp_code = SDI_NOSELE;
				sdi_callback(&sp->sbp->sb);
				return (SDI_RET_OK);
			}
		}
		if((t == dak_sc_ha[c].ha_id) && (inq_cdb->ss_op == SS_INQUIR)) {
			struct ident inq;
			struct ident *inq_data;
			int inq_len;

			bzero((char *)&inq, sizeof(struct ident));
			inq.id_type = ID_PROCESOR;
			(void)strncpy(inq.id_vendor, dakidata[c].name, 24);

			inq_data = (struct ident *)(void *)sp->sbp->sb.SCB.sc_datapt;
			inq_len = sp->sbp->sb.SCB.sc_datasz;
			bcopy((char *)&inq, (char *)inq_data, inq_len);
			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}

		/* change #1 */
		if (dak_illegal(SDI_HAN(sa), b, t, l) && ! dakinit_time)
		{
			sp->sbp->sb.SCB.sc_comp_code = SDI_SCBERR;
			sdi_callback(&sp->sbp->sb);
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

		DAK_SCSILU_LOCK(q->q_opri);
		dak_putq(q, sp);
		dak_next(q);
		return (SDI_RET_OK);

	default:
		return (SDI_RET_ERR);
	}
}


/*
** Function name: dakxlat()
** Description:
**	Perform the virtual to physical translation on the SCB
**	data pointer.
** Calling/Exit State:
**	None.
*/

/*ARGSUSED*/
STATIC HBAXLAT_DECL
HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	register dak_sblk_t *sp = (dak_sblk_t *) hbap;
#if (PDI_VERSION <= PDI_UNIXWARE11)
	int sleepflag;
#endif

	if (sp->s_dmap)
	{
		dak_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}

	if (sp->sbp->sb.SCB.sc_link)
	{
		cmn_err(CE_WARN, "!DAC Host Adapter: Linked commands NOT available\n");
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if (sp->sbp->sb.SCB.sc_datasz) {
		if (dak_dmalist(sp, procp, sleepflag))
			HBAXLAT_RETURN (SDI_RET_RETRY);
	} else {
		sp->s_addr = 0;
		sp->sbp->sb.SCB.sc_datasz = 0;
	}
	HBAXLAT_RETURN (SDI_RET_OK);
}


/*
** Function name: dakgetblk()
** Description:
**	Allocate a SB structure for the caller.  The function will
**	sleep if there are no SCSI blocks available.
** Calling/Exit State:
**	None.
*/

STATIC struct hbadata *
HBAGETBLK(int sleepflag)
{
	dak_sblk_t  *sp;

	sp = (dak_sblk_t *)SDI_GET(&sm_poolhead, sleepflag);
	return ((struct hbadata *)sp);
}


/*
** Function name: dakfreeblk()
** Description:
**	Release previously allocated SB structure. If a scatter/gather
**	list is associated with the SB, it is freed via dak_dma_freelist().
**	A nonzero return indicates an error in pointer or type field.
** Calling/Exit State:
**	None.
*/

STATIC long
HBAFREEBLK(hbap)
register struct hbadata *hbap;
{
	register dak_sblk_t *sp = (dak_sblk_t *) hbap;


	if (sp->s_dmap) {
		dak_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}
	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return ((long) SDI_RET_OK);
}

/*
** Function name: dakgetinfo()
** Description:
**	Return the name and iotype of the given device.  The name is copied
**	into a string pointed to by the first field of the getinfo structure.
** Calling/Exit State:
**	None.
*/

STATIC void
HBAGETINFO(struct scsi_ad *sa, struct hbagetinfo *getinfop)
{
	register char  *s1, *s2;
	static char temp[] = "HA X TC X";
	s1 = temp;
	s2 = getinfop->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;
	getinfop->iotype =  F_DMA_32 | F_SCGTH;

#if (PDI_VERSION >= PDI_SVR42MP)
	if (getinfop->bcbp) {		/* XXX Check this */
		getinfop->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfop->bcbp->bcb_flags = 0;
		getinfop->bcbp->bcb_max_xfer = dakhba_info.max_xfer;
		getinfop->bcbp->bcb_physreqp->phys_align = DAK_MEMALIGN;
		getinfop->bcbp->bcb_physreqp->phys_boundary = DAK_BOUNDARY;
		getinfop->bcbp->bcb_physreqp->phys_dmasize = 0;
	}
#endif
}


/*===========================================================================*/
/* SCSI Host Adapter Driver Utilities
/*===========================================================================*/


#if (PDI_VERSION >= PDI_SVR42MP)
/*
 * STATIC void
 * dak_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
dak_pass_thru0(buf_t *bp)
{
	minor_t	minor = geteminor(bp->b_edev);
	int	c = dak_gtol[SC_EXHAN(minor)];
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	int	b = SC_BUS(minor);
	struct dak_scsi_lu	*q = &LU_Q(c, b, t, l);

	if (!q->q_bcbp) {
		struct sb *sp;
		struct scsi_ad *sa;

		sp = bp->b_private;
		sa = &sp->SCB.sc_dev;
		if ((q->q_bcbp = sdi_getbcb(sa, KM_SLEEP)) == NULL) {
			sp->SCB.sc_comp_code = SDI_ABORT;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		q->q_bcbp->bcb_addrtypes = BA_KVIRT;
		q->q_bcbp->bcb_flags = BCB_SYNCHRONOUS;
		q->q_bcbp->bcb_max_xfer = dakhba_info.max_xfer - ptob(1);
		q->q_bcbp->bcb_physreqp->phys_align = DAK_MEMALIGN;
		q->q_bcbp->bcb_physreqp->phys_boundary = DAK_BOUNDARY;
		q->q_bcbp->bcb_physreqp->phys_dmasize = 0;
		q->q_bcbp->bcb_granularity = 1;
	}

	buf_breakup(dak_pass_thru, bp, q->q_bcbp);
}
#endif /* (PDI_VERSION >= PDI_SVR42MP) */

/*
** Function name: dak_pass_thru()
** Description:
**	Send a pass-thru job to the HA board.
** Calling/Exit State:
**	No locks held on entry or exit.
**	q_lock acquired within, and held on entry to dak_next().
*/

void
dak_pass_thru(bp)
struct buf  *bp;
{
	minor_t	minor = geteminor(bp->b_edev);
	int	c = dak_gtol[SC_EXHAN(minor)];
	int	t = SC_EXTCN(minor);
	int	l = SC_EXLUN(minor);
	int	b = SC_BUS(minor);
	register struct dak_scsi_lu	*q;
	register struct sb *sp;
	caddr_t *addr;
	char op;

#if (PDI_VERSION < PDI_SVR42MP)
	sp = (struct sb *) bp->b_private;
#else
	sp = bp->b_priv.un_ptr;
#endif

	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_datasz = bp->b_bcount;
	sp->SCB.sc_int = dak_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP);

	q = &LU_Q(c, b, t, l);
	op = q->q_sc_cmd[0];
	if (op == SS_READ || op == SS_WRITE) {
		struct scs *scs = (struct scs *)q->q_sc_cmd;
		daddr_t blkno = bp->b_blkno;
		scs->ss_addr1 = (blkno & 0x1F0000) >> 16;
		scs->ss_addr  = sdi_swap16(blkno);
		scs->ss_len   = (char)(bp->b_bcount >> DAK_BLKSHFT);
	}
	if (op == SM_READ || op == SM_WRITE) {
		struct scm *scm = (struct scm *)((char *)q->q_sc_cmd - 2);
		scm->sm_addr = sdi_swap32(bp->b_blkno);
		scm->sm_len  = sdi_swap16(bp->b_bcount >> DAK_BLKSHFT);
	}

	DAK_SCSILU_LOCK(q->q_opri);
	dak_putq(q, (dak_sblk_t *)((struct xsb *)sp)->hbadata_p);
	if (dak_do_conc_io[t] || ( !(q->q_flag & DAK_QBUSY))) {
		dak_next(q);
	} else {
		DAK_SCSILU_UNLOCK(q->q_opri);
	}
}


/*
** Function name: dak_int()
** Description:
**	This is the interrupt handler for pass-thru jobs.  It just
**	wakes up the sleeping process.
** Calling/Exit State:
**	None.
*/

void
dak_int(sp)
struct sb *sp;
{
	struct buf  *bp;

	bp = (struct buf *) sp->SCB.sc_wd;
	biodone(bp);
}



/*
** Function name: dak_flushq()
** Description:
**	Empty a logical unit queue.  If flag is set, remove all jobs.
**	Otherwise, remove only non-control jobs.
** Calling/Exit State:
**	No locks held on entry or exit.
**	q_lock acquired within.
*/

void
dak_flushq(struct dak_scsi_lu *q, int cc, int flag)
{
	register dak_sblk_t  *sp, *nsp;

	ASSERT(q);

	DAK_SCSILU_LOCK(q->q_opri);
	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (dak_qclass(sp) > QNORM))
			dak_putq(q, sp);
		else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(&sp->sbp->sb);
		}
		sp = nsp;
	}
	DAK_SCSILU_UNLOCK(q->q_opri);
}


/*
** Function name: dak_putq()
** Description:
**	Put a job on a logical unit queue.  Jobs are enqueued
**	on a priority basis.
** Calling/Exit State:
**	DAK_SCSILU_LOCK(q->q_opri) for q
*/

STATIC void
dak_putq(struct dak_scsi_lu *q, dak_sblk_t *sp)
{
	register cls = dak_qclass(sp);

	ASSERT(q);

	q->q_count++;
	/*
	 * If queue is empty or queue class of job is less than
	 * that of the last one on the queue, tack on to the end.
	 */
	if ( !q->q_first || (cls <= dak_qclass(q->q_last)) ){
		if (q->q_first) {
			q->q_last->s_next = sp;
			sp->s_prev = q->q_last;
		} else {
			q->q_first = sp;
			sp->s_prev = NULL;
		}
		sp->s_next = NULL;
		q->q_last = sp;

	} else {
		register dak_sblk_t *nsp = q->q_first;

		while (dak_qclass(nsp) >= cls)
			nsp = nsp->s_next;
		sp->s_next = nsp;
		sp->s_prev = nsp->s_prev;
		if (nsp->s_prev)
			nsp->s_prev->s_next = sp;
		else
			q->q_first = sp;
		nsp->s_prev = sp;
	}
}


/*
** Function name: dak_next()
** Description:
**	Attempt to send the next job on the logical unit queue.
**	All jobs are not sent if the Q is busy.
** Calling State:
**	DAK_SCSILU_LOCK(q->q_opri) for q
**	ha->ha_ctl lock held within
** Exit State:
**	None.
*/

STATIC void
dak_next(struct dak_scsi_lu *q)
{
	register struct scsi_ad *sa;
	register dak_sblk_t  *sp;
	register struct dak_scsi_ha	 *ha;
	struct dak_ccb *cp;
	int c,t, type;
	pl_t opri;

	ASSERT(q);

	c = q->controller;
	ha = &dak_sc_ha[c];
	DAK_CTL_LOCK(opri, ha);

	/* See if ioctl que has any thing pending, if so fire it */
	if (ha->ha_npend < ha->ha_maxjobs)
		(*ha->ha_func.dak_iocmd)(c);

	/* See if the controller can take any more jobs */
	if ((ha->ha_cblist == NULL) || (ha->ha_npend >= ha->ha_maxjobs))
	{
		if (q->q_first && 
		  !(q->q_flag & (DAK_QNEEDSRV | DAK_QBUSY))) {
			q->q_next = ha->ha_que;
			ha->ha_que = q;
			q->q_flag |= DAK_QNEEDSRV;
		}
		DAK_CTL_UNLOCK(opri, ha);
		DAK_SCSILU_UNLOCK(q->q_opri);
 		return;
	}
	/* Check for queue empty  */
	if ((sp = q->q_first) == NULL) {
		DAK_CTL_UNLOCK(opri, ha);
		DAK_SCSILU_UNLOCK(q->q_opri);
		return;
	}
	if((type = sp->sbp->sb.sb_type) != SFB_TYPE)
		ha->ha_npend++;    /* (pre)Increment number pending on ctlr */
				   /* so that it is protected by CTL lock   */
	cp = dak_getccb(c);
	DAK_CTL_UNLOCK(opri, ha);

	if(type == SFB_TYPE)
		sa = &sp->sbp->sb.SFB.sf_dev;
	else
		sa = &sp->sbp->sb.SCB.sc_dev;
	t = SDI_TCN(sa);

	/*  Check for device busy or Q suspended  */
	if (((! dak_do_conc_io[t]) && (q->q_flag & DAK_QBUSY))
	    || (q->q_flag & DAK_QSUSP  &&  type == SCB_TYPE))
	{
		dak_freeccb(c, cp);
		if(type != SFB_TYPE) {
			DAK_CTL_LOCK(opri, ha);
			ha->ha_npend--;	/* Decrement number pending on ctlr */
			DAK_CTL_UNLOCK(opri, ha);
		}
		DAK_SCSILU_UNLOCK(q->q_opri);
		return;
	}

	q->q_flag |= DAK_QBUSY;

	if ( !(q->q_first = sp->s_next))
		q->q_last = NULL;
	else
		q->q_first->s_prev = NULL;

	q->q_count--;
	DAK_SCSILU_UNLOCK(q->q_opri);

	if (type == SFB_TYPE)
		dak_func(sp, cp);
	else
		dak_cmd(sp, cp);

	if(dakinit_time)		/* need to poll */
	{
		if(dak_wait(c, 30000) == FAILURE) {
			sp->sbp->sb.SCB.sc_comp_code = SDI_TIME;
			sdi_callback(&sp->sbp->sb);
		}
	}
	return;

}


/*
** Function name: dak_cmd()
** Description:
**	Create and send an SCB associated command.
** Calling/Exit State:
**	No locks held on entry or exit.
**	q_lock acquired within.
*/

STATIC void
dak_cmd(dak_sblk_t *sp, struct dak_ccb *cp)
{
	struct dak_scsi_ha *ha;
	register struct scsi_ad *sa;
	register struct dak_scsi_lu	*q;
	register int  i;
	register char *p;
	unsigned long cnt;
	int  c, b, t;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = dak_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);
	b = SDI_BUS(sa);

	cp->c_bind = &sp->sbp->sb;
	cp->c_time = (sp->sbp->sb.SCB.sc_time*HZ) / 1000;

	/* Build the Command Packet structure */
	cp->CP_OpCode        = CP_DMA_CMD;
	cp->CPop.byte        = HA_AUTO_REQ_SEN;
	cp->CPID             = (BYTE)(t);
	cp->lun              = sa->sa_lun;

	if (sp->s_CPopCtrl) {
		cp->CPop.byte |= sp->s_CPopCtrl;
		sp->s_CPopCtrl = 0;
	}
	if (sp->s_dmap)  {              /* scatter/gather is used      */
		cp->CPop.bit.Scatter = 1;
/*		cnt = sp->s_dmap->SG_size;  */
		cnt = sp->sbp->sb.SCB.sc_datasz;
	} else {                             /* block mode                  */
		cnt = sp->sbp->sb.SCB.sc_datasz;
	}
	cp->CPop.bit.DataIn  = 0;
	cp->CPop.bit.DataOut = 0;
	if (cnt) {
		if (sp->sbp->sb.SCB.sc_mode & SCB_READ) {
			cp->CPop.bit.DataIn  = 1;
		} else {
			cp->CPop.bit.DataOut = 1;
		}
		cp->CPdataLen = cnt;
	} else
		cp->CPdataLen = 0;

	cp->CPdataDMA = sp->s_addr;
	cp->no_breaks = sp->no_breaks;

	q = &LU_Q(c, b, t, sa->sa_lun);         /* Get SCSI Dev/Lun Pointer.   */

	/*********************************************************************
	** If a Request Sense command and ReqSen Data cached then copy to   **
	**   data buffer and return.                                        **
	**********************************************************************/
	p = sp->sbp->sb.SCB.sc_cmdpt;        /* Get Command CDB Pointer.    */

	DAK_SCSILU_LOCK(q->q_opri);
	if ( (q->q_flag & DAK_QSENSE) && (*p == SS_REQSEN)) {
		int copyfail = 0;
		caddr_t toaddr;

		q->q_flag &= ~DAK_QSENSE;
		DAK_SCSILU_UNLOCK(q->q_opri);
		/*
		 * If the REQ SENSE data is going to a buffer within kernel
		 * space, then the data is just copied.  Otherwise, the
		 * data is going to a user-space buffer (Pass-thru) so the
		 * data must be copied to the individual segments of
		 * contiguous physical memory that make up the user's buffer.
		 * Note - The buffer address sent by the user
		 * (sp->sbp->sb.SCB.sc_datapt) can not be used directly
		 * since this routine may be executing on the interrupt thread.
		 */
dak_break_point();
		if (! cp->CPop.bit.Scatter) {
#ifdef DAC_DEBUG
	printf("dak_cmd: one big chunk to kernel memory.\n");
#endif /* DAC_DEBUG */
			if (toaddr = physmap(sp->s_addr, cnt, KM_NOSLEEP)) {
				bcopy((caddr_t)(&q->q_sense)+1, toaddr, cnt);
				physmap_free(toaddr, cnt, 0);
			} else
				copyfail++;
		} else {
		/* copy req sns data to places defined by scat/gath list */
			SG_vect	*VectPtr;
			int	VectCnt;
			caddr_t	Src;
			caddr_t	optr, Dest;
			int	Count;

			if (VectPtr = (SG_vect *)(void *)physmap (sp->s_addr,
					cnt, KM_NOSLEEP)) {
				optr = (caddr_t)(void *)VectPtr;
				VectCnt = cnt / sizeof(*VectPtr);
				Src = (caddr_t)(&q->q_sense) + 1;
				for (i=0; i < VectCnt; i++) {
					Dest = (caddr_t)VectPtr->Phy;
					Count = (int)(VectPtr->Len);

					if (Dest = (caddr_t)physmap((unsigned long) Dest,
						Count, KM_NOSLEEP)) {

						bcopy (Src, Dest, Count);
						Src += Count;
						VectPtr++;
						physmap_free(Dest, Count, 0);
					} else {
						copyfail++;
						break;
					}
				}
				physmap_free(optr, cnt, 0);
			} else
				copyfail++;
		}
		cp->CP_Controller_Status = S_GOOD;
		cp->dcdb.device= (b << 4) + t;
		dak_done(c, cp, copyfail?RETRY:0);
		dak_waitflag = FALSE;
		return;
	} else {
		DAK_SCSILU_UNLOCK(q->q_opri);
	}

	for (i=0; i < sp->sbp->sb.SCB.sc_cmdsz; i++)
		cp->dcdb.cdb[i] = *p++;        /* Copy SCB cdb to CP cdb.     */
	cp->dcdb.cdb_len = sp->sbp->sb.SCB.sc_cmdsz;
	cp->dcdb.device = (b << 4) + t;
	cp->dcdb.sense_len = sizeof(struct sense);
	ha = &dak_sc_ha[c];
	(*ha->ha_func.dak_send)(c, START, cp); /* Send command to the HA.     */
}


/*
** Function name: dak_func()
** Description:
**	Create and send an SFB associated command.
** Calling/Exit State:
**	None.
*/

STATIC void
dak_func(dak_sblk_t *sp, struct dak_ccb *cp)
{
	register struct scsi_ad *sa;
	int  c, t;

	/* Only SFB_ABORTM and SFB_RESETM messages get here.                */

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = dak_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

#ifdef DEBUG
	if(sp->sbp->sb.SFB.sf_func!=SFB_ABORTM && sp->sbp->sb.SFB.sf_func!=SFB_RESETM)
	{
		dak_freeccb(c, cp);	/* Release SCSI command block. */
		cmn_err(CE_WARN, "DAC Host Adapter: Unsupported SFB command: %X\n",sp->sbp->sb.SFB.sf_func);
		return;
	}
#endif

	cp->c_bind = &sp->sbp->sb;
	cp->CPID   = (BYTE)t;
	cp->lun = sa->sa_lun;
	cp->c_time = 0;

	cmn_err(CE_WARN, "!DAC Host Adapter: HA %d - Bus is being reset\n", dak_ltog[c]);
	cp->CP_Controller_Status = 0;
	cp->CP_SCSI_Status       = 0;
	dak_ha_done(c, cp, 0);
}

void
dak_break_point()
{
}


/*
** Function name: dak_eisa_send()
** Description:
**      Send a command to the host adapter board.
** Calling/Exit State:
**	DAK_CTL_LOCK for ha within.
**	No locks held on entry/exit.
*/

STATIC void
dak_eisa_send(int c, int cmd, struct dak_ccb *cp)
{
	int	i;
	BYTE opcode;
	unsigned long phys_addr;
	unsigned long blocks,blockAddr;
	register struct dak_scsi_ha *ha = &dak_sc_ha[c];
	int	b, t, l;
	pl_t opri;

	t =cp->CPID;
	l =cp->lun & 7;
	b =cp->dcdb.device >> 4;

#ifdef DAK_DEBUG
	if (dak_debug > 1) {
		printf("dak_send: ");
		for(i=0;i<12;i++)
			printf("%x ,",cp->dcdb.cdb[i]);
		printf("b=%d,t=%d,l=%d",b,t,l);
		printf("\n");
	}
#endif

	DAK_CTL_LOCK(opri,ha);
	while(inb(ha->ha_base + 0x0d) & 1);  /* loop till free */

	for(i=0;i<64;i++) {
		if(dak_command_array[c][i] == 0)
			break;
	}
	if( i == 64) {
		 cmn_err(CE_WARN, "DAC Host Adapter[%d]: no_free_command_id",c);
		 cmn_err(CE_PANIC, "pending=%x,address of dak_command_array=%lx",ha->ha_npend,&dak_command_array[c][0]);
	}
	dak_command_array[c][i]=(unsigned long)cp;
	dak_command_type[c][i]=IO;

	outb(ha->ha_base + 0x11, i);

	if(b == 0 && t == 0)
	{
	switch(cp->dcdb.cdb[0])
	{
		case SM_READ:
			opcode = LDAC_READ;
			blocks = (ULONG)dachlpGetM16(&cp->dcdb.cdb[7]);
			blockAddr = dachlpGetM32(&cp->dcdb.cdb[2]);
			break;

		case SM_WRITE:
			opcode = LDAC_WRITE;
			blocks = (ULONG)dachlpGetM16(&cp->dcdb.cdb[7]);
			blockAddr = dachlpGetM32(&cp->dcdb.cdb[2]);
			break;


		case SS_READ:
			opcode = LDAC_READ;
			blocks = (ULONG)cp->dcdb.cdb[4];
			blockAddr = dachlpGetM24(&cp->dcdb.cdb[1]) & 0x1fffff;
			break;

		case SS_WRITE:
			opcode = LDAC_WRITE;
			blocks = (ULONG)cp->dcdb.cdb[4];
			blockAddr = dachlpGetM24(&cp->dcdb.cdb[1]) & 0x1fffff;
			break;

		default:
			/* If we are here, we need to pray */
			cmn_err(CE_PANIC, "DAC Host Adapter[%d]: Unrecognized command cdb opcode = %xH",c,cp->dcdb.cdb[0]);

	}

	if ( cp->CPop.bit.Scatter) {
		opcode |= 0x80;
		outb(ha->ha_base + 0x1c, cp->no_breaks);
	}

	outb(ha->ha_base + 0x10, opcode);

	phys_addr = cp->CPdataDMA;
	outb(ha->ha_base + 0x18, (phys_addr & 0xff));
	outb(ha->ha_base + 0x19, ((phys_addr >> 8) & 0xff));
	outb(ha->ha_base + 0x1a, ((phys_addr >> 16) & 0xff));
	outb(ha->ha_base + 0x1b, ((phys_addr >> 24) & 0xff));

	/* No blocks */
	outb(ha->ha_base + 0x12, (blocks & 0xff));

	/* Block Addr */
	outb(ha->ha_base + 0x14, (blockAddr & 0xff));
	outb(ha->ha_base + 0x15, ((blockAddr >> 8) & 0xff));
	outb(ha->ha_base + 0x16, ((blockAddr >> 16) & 0xff));
	outb(ha->ha_base + 0x13, (((blockAddr >> 24) & 0xff) << 6));

	/* Drive No  */
	outb(ha->ha_base + 0x17, l);


	} else if(l==0) {
#if (PDI_VERSION <= PDI_SVR42MP)
		cp->dcdb.device=(ha->tape_channel[t] << 4) + t;
#endif
		cp->dcdb.cdb[1] &= 0x1f; /* Mask out LUN */
		if(cp->CPop.bit.DataIn == 1)
			cp->dcdb.dir = DATA_IN;
		else if(cp->CPop.bit.DataOut == 1)
			cp->dcdb.dir = DATA_OUT;
		else
			cp->dcdb.dir = DATA_NONE;
		cp->dcdb.byte_cnt = cp->CPdataLen;
	  	cp->dcdb.ptr = cp->CPdataDMA;
/*	  	cp->dcdb.ptr = 0x00e0000; */

		opcode = OP_DCDB;
		if ( cp->CPop.bit.Scatter) {
			opcode |= 0x80;
			outb(ha->ha_base + 0x1c, cp->no_breaks);
		}
		outb(ha->ha_base + 0x10, opcode);

		phys_addr =	cp->c_cdb_addr;
		if(phys_addr == 0)
			phys_addr = vtop((caddr_t)&cp->dcdb, NULL);
		outb(ha->ha_base + 0x18, (phys_addr & 0xff));
		outb(ha->ha_base + 0x19, ((phys_addr >> 8) & 0xff));
		outb(ha->ha_base + 0x1a, ((phys_addr >> 16) & 0xff));
		outb(ha->ha_base + 0x1b, ((phys_addr >> 24) & 0xff));
	} else {
		DAK_CTL_UNLOCK(opri,ha);
		cmn_err(CE_WARN, "DAC Host Adapter: b=%d,t=%d,l=%d\n",b,t,l);
		return;
	}
	if (cmd == START) {
		cp->c_active = TRUE;	/* Set Command Active flag.         */
		drv_getparm(LBOLT, (ulong *)&cp->c_start); /* Set start time */
	} else if (cmd == ABORT) {
	/* 	outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);  */
		drv_usecwait(1000);
		DAK_CTL_UNLOCK(opri,ha);
		return;
	}

	/* Fire Command */
	outb(ha->ha_base + 0x0d, 1);
	DAK_CTL_UNLOCK(opri,ha);

	if((cp->CPdataLen > (unsigned long) 0xf000) ) {
		dak_break_point();
	}
}

/*
** Function name: dak_mca_send()
** Description:
**      Send a command to the host adapter board.
** Calling/Exit State:
**	DAK_CTL_LOCK for ha within.
**	No locks held on entry/exit.
*/

STATIC void
dak_mca_send(int c, int cmd, struct dak_ccb *cp)
{
	int	jobid;
	BYTE opcode;
	unsigned long phys_addr;
	unsigned long blocks,blockAddr;
	register struct dak_scsi_ha *ha = &dak_sc_ha[c];
	char	*mbxp = ha->ha_mbx_addr;
	unsigned char	*cptr;
	int	b, t, l;
	pl_t	opri;

	t =cp->CPID;
	l =cp->lun & 7;
	b =cp->dcdb.device >> 4;

#ifdef DAK_DEBUG
	if (dak_debug > 1) {
		int i;
		printf("dak_send: ");
		for(i=0;i<12;i++)
			printf("%x ,",cp->dcdb.cdb[i]);
		printf("b=%d,t=%d,l=%d",b,t,l);
		printf("\n");
	}
#endif

	DAK_CTL_LOCK(opri,ha);
	while(mbxp[0])
		drv_usecwait(10);

	for(jobid=0;jobid<64;jobid++) {
		if(dak_command_array[c][jobid] == 0)
			break;
	}
	if( jobid == 64) {
		 cmn_err(CE_WARN, "DAC Host Adapter[%d]: no_free_command_id",c);
		 cmn_err(CE_PANIC, "pending=%x,address of dak_command_array=%lx",ha->ha_npend,&dak_command_array[c][0]);
	}
	dak_command_array[c][jobid]=(unsigned long)cp;
	dak_command_type[c][jobid]=IO;

	if(b == 0 && t == 0)
	{
	switch(cp->dcdb.cdb[0])
	{
		case SM_READ:
			opcode = LDAC_READ;
			blocks = (ULONG)dachlpGetM16(&cp->dcdb.cdb[7]);
			blockAddr = dachlpGetM32(&cp->dcdb.cdb[2]);
			break;

		case SM_WRITE:
			opcode = LDAC_WRITE;
			blocks = (ULONG)dachlpGetM16(&cp->dcdb.cdb[7]);
			blockAddr = dachlpGetM32(&cp->dcdb.cdb[2]);
			break;


		case SS_READ:
			opcode = LDAC_READ;
			blocks = (ULONG)cp->dcdb.cdb[4];
			blockAddr = dachlpGetM24(&cp->dcdb.cdb[1]) & 0x1fffff;
			break;

		case SS_WRITE:
			opcode = LDAC_WRITE;
			blocks = (ULONG)cp->dcdb.cdb[4];
			blockAddr = dachlpGetM24(&cp->dcdb.cdb[1]) & 0x1fffff;
			break;

		default:
			/* If we are here, we need to pray */
			cmn_err(CE_PANIC, "DAC Host Adapter[%d]: Unrecognized command cdb opcode = %xH",c,cp->dcdb.cdb[0]);

	}

	if ( cp->CPop.bit.Scatter) {
		opcode |= 0x80;
	}
	mbxp[12] = cp->no_breaks;

	phys_addr = cp->CPdataDMA;

	/* No blocks */
	mbxp[2] = blocks & 0xff;

	/* Block Addr */
	mbxp[4] = blockAddr & 0xff;
	mbxp[5] = (blockAddr >> 8) & 0xff;
	mbxp[6] = (blockAddr >> 16) & 0xff;
	mbxp[3] = ((blockAddr >> 24) & 0xff) << 6;

	/* Drive No  */
	mbxp[7] = (char)l;


	} else if(l==0) {
#if (PDI_VERSION <= PDI_SVR42MP)
		cp->dcdb.device=(ha->tape_channel[t] << 4) + t;
#endif
		cp->dcdb.cdb[1] &= 0x1f; /* Mask out LUN */
		if(cp->CPop.bit.DataIn == 1)
			cp->dcdb.dir = DATA_IN;
		else if(cp->CPop.bit.DataOut == 1)
			cp->dcdb.dir = DATA_OUT;
		else
			cp->dcdb.dir = DATA_NONE;
		cp->dcdb.byte_cnt = cp->CPdataLen;
	  	cp->dcdb.ptr = cp->CPdataDMA;
/*	  	cp->dcdb.ptr = 0x00e0000; */

		opcode = OP_DCDB;
		if ( cp->CPop.bit.Scatter) {
			opcode |= 0x80;
		}
		mbxp[12] = cp->no_breaks;

		phys_addr =	cp->c_cdb_addr;
		if(phys_addr == 0)
			phys_addr = vtop((caddr_t)&cp->dcdb, NULL);
	} else {
		DAK_CTL_UNLOCK(opri,ha);
		cmn_err(CE_WARN, "DAC Host Adapter: b=%d,t=%d,l=%d\n",b,t,l);
		return;
	}
	if (cmd == START) {
		cp->c_active = TRUE;	/* Set Command Active flag.         */
		drv_getparm(LBOLT, (ulong *)&cp->c_start); /* Set start time */
	} else if (cmd == ABORT) {
	/* 	outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);  */
		drv_usecwait(1000);
		DAK_CTL_UNLOCK(opri,ha);
		return;
	}

	cptr=(unsigned char *)&phys_addr;

	mbxp[0] = opcode;
	mbxp[1] = (char)jobid;
	mbxp[8] =  cptr[0];
	mbxp[9] =  cptr[1];
	mbxp[10] = cptr[2];
	mbxp[11] = cptr[3];
			
	/* Fire Command */
	outb(ha->ha_base + 0x04, 0xd0);
	DAK_CTL_UNLOCK(opri,ha);

	if((cp->CPdataLen > (unsigned long) 0xf000) ) {
		dak_break_point();
	}
	return;
}

/*
** Function name: dak_pci_send()
** Description:
**      Send a command to the host adapter board.
** Calling/Exit State:
**	DAK_CTL_LOCK for ha within.
**	No locks held on entry/exit.
*/

STATIC void
dak_pci_send(int c, int cmd, struct dak_ccb *cp)
{
	int	i;
	BYTE opcode;
	unsigned long phys_addr;
	unsigned long blocks,blockAddr;
	register struct dak_scsi_ha *ha = &dak_sc_ha[c];
	int	b, t, l;
	pl_t opri;

	t =cp->CPID;
	l =cp->lun & 7;
	b =cp->dcdb.device >> 4;

#ifdef DAK_DEBUG
	if (dak_debug > 1) {
		printf("dak_send: ");
		for(i=0;i<12;i++)
			printf("%x ,",cp->dcdb.cdb[i]);
		printf("b=%d,t=%d,l=%d",b,t,l);
		printf("\n");
	}
#endif

	DAK_CTL_LOCK(opri,ha);
	while(inb(ha->ha_base + 0x40) & 1)   /* loop till free */
		drv_usecwait(10);

	for(i=0;i<64;i++) {
		if(dak_command_array[c][i] == 0)
			break;
	}
	if( i == 64) {
		 cmn_err(CE_WARN, "DAC Host Adapter[%d]: no_free_command_id",c);
		 cmn_err(CE_PANIC, "pending=%x,address of dak_command_array=%lx",ha->ha_npend,&dak_command_array[c][0]);
	}
	dak_command_array[c][i]=(unsigned long)cp;
	dak_command_type[c][i]=IO;

	outb(ha->ha_base + 0x01, i);

	if(b == 0 && t == 0)
	{
	switch(cp->dcdb.cdb[0])
	{
		case SM_READ:
			opcode = LDAC_READ;
			blocks = (ULONG)dachlpGetM16(&cp->dcdb.cdb[7]);
			blockAddr = dachlpGetM32(&cp->dcdb.cdb[2]);
			break;

		case SM_WRITE:
			opcode = LDAC_WRITE;
			blocks = (ULONG)dachlpGetM16(&cp->dcdb.cdb[7]);
			blockAddr = dachlpGetM32(&cp->dcdb.cdb[2]);
			break;


		case SS_READ:
			opcode = LDAC_READ;
			blocks = (ULONG)cp->dcdb.cdb[4];
			blockAddr = dachlpGetM24(&cp->dcdb.cdb[1]) & 0x1fffff;
			break;

		case SS_WRITE:
			opcode = LDAC_WRITE;
			blocks = (ULONG)cp->dcdb.cdb[4];
			blockAddr = dachlpGetM24(&cp->dcdb.cdb[1]) & 0x1fffff;
			break;

		default:
			/* If we are here, we need to pray */
			cmn_err(CE_PANIC, "DAC Host Adapter[%d]: Unrecognized command cdb opcode = %xH",c,cp->dcdb.cdb[0]);

	}

	if ( cp->CPop.bit.Scatter) {
		opcode |= 0x80;
		outb(ha->ha_base + 0x0c, cp->no_breaks);
	}

	outb(ha->ha_base + 0x00, opcode);

	phys_addr = cp->CPdataDMA;
	outb(ha->ha_base + 0x08, (phys_addr & 0xff));
	outb(ha->ha_base + 0x09, ((phys_addr >> 8) & 0xff));
	outb(ha->ha_base + 0x0a, ((phys_addr >> 16) & 0xff));
	outb(ha->ha_base + 0x0b, ((phys_addr >> 24) & 0xff));

	/* No blocks */
	outb(ha->ha_base + 0x02, (blocks & 0xff));

	/* Block Addr */
	outb(ha->ha_base + 0x04, (blockAddr & 0xff));
	outb(ha->ha_base + 0x05, ((blockAddr >> 8) & 0xff));
	outb(ha->ha_base + 0x06, ((blockAddr >> 16) & 0xff));
	outb(ha->ha_base + 0x03, (((blockAddr >> 24) & 0xff) << 6));

	/* Drive No  */
	outb(ha->ha_base + 0x07, l);


	} else if(l==0) {
#if (PDI_VERSION <= PDI_SVR42MP)
		cp->dcdb.device=(ha->tape_channel[t] << 4) + t;
#endif
		cp->dcdb.cdb[1] &= 0x1f; /* Mask out LUN */
		if(cp->CPop.bit.DataIn == 1)
			cp->dcdb.dir = DATA_IN;
		else if(cp->CPop.bit.DataOut == 1)
			cp->dcdb.dir = DATA_OUT;
		else
			cp->dcdb.dir = DATA_NONE;
		cp->dcdb.byte_cnt = cp->CPdataLen;
	  	cp->dcdb.ptr = cp->CPdataDMA;
/*	  	cp->dcdb.ptr = 0x00e0000; */

		opcode = OP_DCDB;
		if ( cp->CPop.bit.Scatter) {
			opcode |= 0x80;
			outb(ha->ha_base + 0x0c, cp->no_breaks);
		}
		outb(ha->ha_base + 0x00, opcode);

		phys_addr =	cp->c_cdb_addr;
		if(phys_addr == 0)
			phys_addr = vtop((caddr_t)&cp->dcdb, NULL);
		outb(ha->ha_base + 0x08, (phys_addr & 0xff));
		outb(ha->ha_base + 0x09, ((phys_addr >> 8) & 0xff));
		outb(ha->ha_base + 0x0a, ((phys_addr >> 16) & 0xff));
		outb(ha->ha_base + 0x0b, ((phys_addr >> 24) & 0xff));
	} else {
		DAK_CTL_UNLOCK(opri,ha);
		cmn_err(CE_WARN, "DAC Host Adapter: b=%d,t=%d,l=%d\n",b,t,l);
		return;
	}
	if (cmd == START) {
		cp->c_active = TRUE;	/* Set Command Active flag.         */
		drv_getparm(LBOLT, (ulong *)&cp->c_start); /* Set start time */
	} else if (cmd == ABORT) {
	/* 	outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);  */
		drv_usecwait(1000);
		DAK_CTL_UNLOCK(opri,ha);
		return;
	}

	/* Fire Command */
	outb(ha->ha_base + 0x40, 1);
	DAK_CTL_UNLOCK(opri,ha);

	if((cp->CPdataLen > (unsigned long) 0xf000) ) {
		dak_break_point();
	}
}

/*
** Function name: dak_eisa_iocmd()
** Description:
**      Send an ioctl command to the host adapter board.
** Calling/Exit State:
**	DAK_CTL_LOCK for ha
**	DAK_SCSILU_LOCK(q->q_opri) for q
*/

STATIC int
dak_eisa_iocmd(int c)
{
	struct buf	*bp;
	int	i;
	BYTE opcode;
	unsigned long phys_addr;
	register struct dak_scsi_ha *ha = &dak_sc_ha[c];
	struct dacimbx *ioc;

	bp = ha->ioctl_que ;
	if(bp == NULL) {
		return (1);
	}
	ha->ioctl_que = bp->av_forw;


	while(inb(ha->ha_base + 0x0d) & 1);  /* loop till free */

	for(i=0;i<64;i++) {
		if(dak_command_array[c][i] == 0)
			break;
	}
	if( i == 64) {
		 cmn_err(CE_WARN, "DAC Host Adapter[%d]: no_free_command_id",c);
		 cmn_err(CE_WARN, "pending=%x,address of dak_command_array=%lx",ha->ha_npend,&dak_command_array[c][0]);
		 return (1);
	}
	dak_command_array[c][i]=(unsigned long)bp;
	dak_command_type[c][i]=IOCTL;

	outb(ha->ha_base + 0x11, i);

	if( (uint_t)bp->b_private  &  B_DCMD) {
		ioc = (struct dacimbx *)(void *)bp->b_un.b_addr;
		opcode = ioc->ioc_mbx0;
		outb(ha->ha_base + 0x10, opcode);
		outb(ha->ha_base + 0x12, ioc->ioc_mbx2);
		outb(ha->ha_base + 0x17, ioc->ioc_mbx2);
		outb(ha->ha_base + 0x13, ioc->ioc_mbx3);
		outb(ha->ha_base + 0x14, ioc->ioc_mbx4);

		phys_addr = (unsigned long)vtop(bp->b_un.b_addr + 32, bp->b_proc);
		outb(ha->ha_base + 0x18, (phys_addr & 0xff));
		outb(ha->ha_base + 0x19, ((phys_addr >> 8) & 0xff));
		outb(ha->ha_base + 0x1a, ((phys_addr >> 16) & 0xff));
		outb(ha->ha_base + 0x1b, ((phys_addr >> 24) & 0xff));
	} else {
		outb(ha->ha_base + 0x10, OP_DCDB);
		phys_addr = (unsigned long)vtop(bp->b_un.b_addr, bp->b_proc);
		outb(ha->ha_base + 0x18, (phys_addr & 0xff));
		outb(ha->ha_base + 0x19, ((phys_addr >> 8) & 0xff));
		outb(ha->ha_base + 0x1a, ((phys_addr >> 16) & 0xff));
		outb(ha->ha_base + 0x1b, ((phys_addr >> 24) & 0xff));
	}

	ha->ha_npend++;		/* Increment number pending on ctlr */
	outb(ha->ha_base + 0x0d, 1);
	return (0);
}

/*
** Function name: dak_mca_iocmd()
** Description:
**      Send an ioctl command to the host adapter board.
** Calling/Exit State:
**	DAK_CTL_LOCK for ha
**	DAK_SCSILU_LOCK(q->q_opri) for q
*/

STATIC int
dak_mca_iocmd(int c)
{
	struct buf	*bp;
	int	i;
	BYTE opcode;
	unsigned long phys_addr;
	register struct dak_scsi_ha *ha = &dak_sc_ha[c];
	struct dacimbx *ioc;
	char *mbxp = (char *)ha->ha_mbx_addr;

	bp = ha->ioctl_que ;
	if(bp == NULL) {
		return (1);
	}
	ha->ioctl_que = bp->av_forw;

	while(inb(ha->ha_base + 0x40) & 1)   /* loop till free */
		drv_usecwait(10);

	for(i=0;i<64;i++) {
		if(dak_command_array[c][i] == 0)
			break;
	}
	if( i == 64) {
		 cmn_err(CE_WARN, "DAC Host Adapter[%d]: no_free_command_id",c);
		 cmn_err(CE_WARN, "pending=%x,address of dak_command_array=%lx",ha->ha_npend,&dak_command_array[c][0]);
		 return (1);
	}
	dak_command_array[c][i]=(unsigned long)bp;
	dak_command_type[c][i]=IOCTL;

	mbxp[1] = (char)i;

	if( (uint_t)bp->b_private  &  B_DCMD) {
		ioc = (struct dacimbx *)(void *)bp->b_un.b_addr;
		opcode = ioc->ioc_mbx0;
		mbxp[0] = opcode;
		mbxp[2] = ioc->ioc_mbx2;
		mbxp[7] = ioc->ioc_mbx2;
		mbxp[3] = ioc->ioc_mbx3;
		mbxp[4] = ioc->ioc_mbx4;

		phys_addr = (unsigned long)vtop(bp->b_un.b_addr + 32, bp->b_proc);
		mbxp[8] = phys_addr & 0xff;
		mbxp[9] = (phys_addr >> 8) & 0xff;
		mbxp[10] = (phys_addr >> 16) & 0xff;
		mbxp[11] = (phys_addr >> 24) & 0xff;
	} else {
		mbxp[0] = OP_DCDB;
		phys_addr = (unsigned long)vtop(bp->b_un.b_addr, bp->b_proc);
		mbxp[8] = phys_addr & 0xff;
		mbxp[9] = (phys_addr >> 8) & 0xff;
		mbxp[10] = (phys_addr >> 16) & 0xff;
		mbxp[11] = (phys_addr >> 24) & 0xff;
	}

	ha->ha_npend++;		/* Increment number pending on ctlr */
	outb(ha->ha_base + 0x04, 0xd0);
	return (0);
}

/*
** Function name: dak_pci_iocmd()
** Description:
**      Send an ioctl command to the host adapter board.
** Calling/Exit State:
**	DAK_CTL_LOCK for ha
**	DAK_SCSILU_LOCK(q->q_opri) for q
*/

STATIC int
dak_pci_iocmd(int c)
{
	struct buf	*bp;
	int	i;
	BYTE opcode;
	unsigned long phys_addr;
	register struct dak_scsi_ha *ha = &dak_sc_ha[c];
	struct dacimbx *ioc;

	bp = ha->ioctl_que ;
	if(bp == NULL) {
		return (1);
	}
	ha->ioctl_que = bp->av_forw;

	while(inb(ha->ha_base + 0x40) & 1)   /* loop till free */
		drv_usecwait(10);

	for(i=0;i<64;i++) {
		if(dak_command_array[c][i] == 0)
			break;
	}
	if( i == 64) {
		 cmn_err(CE_WARN, "DAC Host Adapter[%d]: no_free_command_id",c);
		 cmn_err(CE_WARN, "pending=%x,address of dak_command_array=%lx",ha->ha_npend,&dak_command_array[c][0]);
		 return (1);
	}
	dak_command_array[c][i]=(unsigned long)bp;
	dak_command_type[c][i]=IOCTL;

	outb(ha->ha_base + 0x01, i);

	if( (uint_t)bp->b_private  &  B_DCMD) {
		ioc = (struct dacimbx *)(void *)bp->b_un.b_addr;
		opcode = ioc->ioc_mbx0;
		outb(ha->ha_base + 0x00, opcode);
		outb(ha->ha_base + 0x07, ioc->ioc_mbx2);
		outb(ha->ha_base + 0x02, ioc->ioc_mbx2);
		outb(ha->ha_base + 0x03, ioc->ioc_mbx3);
		outb(ha->ha_base + 0x04, ioc->ioc_mbx4);

		phys_addr = (unsigned long)vtop(bp->b_un.b_addr + 32, bp->b_proc);
		outb(ha->ha_base + 0x08, (phys_addr & 0xff));
		outb(ha->ha_base + 0x09, ((phys_addr >> 8) & 0xff));
		outb(ha->ha_base + 0x0a, ((phys_addr >> 16) & 0xff));
		outb(ha->ha_base + 0x0b, ((phys_addr >> 24) & 0xff));
	} else {
		outb(ha->ha_base + 0x00, OP_DCDB);
		phys_addr = (unsigned long)vtop(bp->b_un.b_addr, bp->b_proc);
		outb(ha->ha_base + 0x08, (phys_addr & 0xff));
		outb(ha->ha_base + 0x09, ((phys_addr >> 8) & 0xff));
		outb(ha->ha_base + 0x0a, ((phys_addr >> 16) & 0xff));
		outb(ha->ha_base + 0x0b, ((phys_addr >> 24) & 0xff));
	}

	ha->ha_npend++;		/* Increment number pending on ctlr */
	outb(ha->ha_base + 0x40, 1);
	return (0);
}

/*
** Function name: dak_getccb()
** Description:
**      Allocate a controller command block structure.
** Calling/Exit State:
**	DAK_CCB_LOCK within.
*/

STATIC struct dak_ccb *
dak_getccb(int c)
{
	register struct dak_scsi_ha	*ha = &dak_sc_ha[c];
	register struct dak_ccb	*cp;
	pl_t opri;

	DAK_CCB_LOCK(opri);
	if (ha->ha_cblist) {
		cp = ha->ha_cblist;
		ha->ha_cblist = cp->c_next;
		DAK_CCB_UNLOCK(opri);
		return (cp);
	}
	cmn_err(CE_PANIC, "DAC Host Adapter[%d]: Out of command blocks",c);
	/* NOTREACHED */
}


/*
** Function name: dak_freeccb()
** Description:
**      Release a previously allocated command block.
** Calling/Exit State:
**	DAK_CCB_LOCK within.
*/

STATIC void
dak_freeccb(int c, struct dak_ccb *cp)
{
	register struct dak_scsi_ha	*ha = &dak_sc_ha[c];
	pl_t opri;

	DAK_CCB_LOCK(opri);
	cp = &ha->ha_ccb[cp->c_index];
	cp->c_bind = NULL;
	cp->c_next = ha->ha_cblist;
	ha->ha_cblist = cp;
	DAK_CCB_UNLOCK(opri);
}


/*
** Function name: dak_dmalist()
** Description:
**	Build the physical address(es) for DMA to/from the data buffer.
**	If the data buffer is contiguous in physical memory, only 1 base
**	address is provided for a regular SB.  If not, a scatter/gather
**	list is built, and the SB will point to that list instead.
** Calling/Exit State:
**	DAK_DMALIST_LOCK within.
*/
STATIC int
dak_dmalist(dak_sblk_t *sp, struct proc *procp, int sleepflag)
{
	SG_vect	tmp_list[MAX_DMASZ];
	register SG_vect  *pp;
	register dak_dma_t  *dmap;
	register long   count, fraglen, thispage;
	caddr_t		vaddr;
	paddr_t		addr, base;
	int		i;
	pl_t		opri;
/*
#ifdef DAC_DEBUG
	printf("dak_dmalist: on entry\n");
#endif 
*/
	vaddr = sp->sbp->sb.SCB.sc_datapt;
	count = sp->sbp->sb.SCB.sc_datasz;
	pp = &tmp_list[0];

	/* First build a scatter/gather list of physical addresses and sizes */

	for (i = 0; (i < MAX_DMASZ) && count; ++i, ++pp) {
		base = vtop(vaddr, procp);	/* Physical addr of segment */
		fraglen = 0;			/* Zero bytes so far */
		do {
			thispage = min(count, pgbnd(vaddr));
			fraglen += thispage;	/* This many more are contig */
			vaddr += thispage;	/* Bump virtual address */
			count -= thispage;	/* Recompute amount left */
			if (!count)
				break;		/* End of request */
			addr = vtop(vaddr, procp); /* Get next page's address */
		} while (base + fraglen == addr);

		/* Now set up dma list element */
		pp->Phy = base;
		pp->Len = fraglen;
	}
	if (count != 0)
		cmn_err(CE_PANIC, "DAC Host Adapter: Job too big for DMA list");

	if (i == 1)
		/*
		 * The data buffer was contiguous in physical memory.
		 * There is no need for a scatter/gather list.
		 */
		sp->s_addr = (paddr_t) (tmp_list[0].Phy);
	else {
		/*
		 * We need a scatter/gather list.
		 * Allocate one and copy the list we built into it.
		 */
		DAK_DMALIST_LOCK(opri);
		if (!dak_dfreelist && (sleepflag == KM_NOSLEEP)) {
			DAK_DMALIST_UNLOCK(opri);
			return (1);
		}
		while ( !(dmap = dak_dfreelist)) {
			SV_WAIT(dak_dmalist_sv, PRIBIO, dak_dmalist_lock);
			DAK_DMALIST_LOCK(opri);
		}
		dak_dfreelist = dmap->d_next;
		DAK_DMALIST_UNLOCK(opri);

		sp->s_dmap = dmap;
		sp->s_addr = vtop((caddr_t)dmap->d_list, procp);
		sp->no_breaks = (char)i;
		dmap->SG_size = i * sizeof(SG_vect);
		bcopy((caddr_t) &tmp_list[0],
		    (caddr_t) dmap->d_list, dmap->SG_size);
	}
/*
#ifdef DAC_DEBUG
	printf("dak_dmalist: on exit\n");
#endif
*/
	return (0);
}


/*
** Function name: dak_dma_freelist()
** Description:
**	Release a previously allocated scatter/gather DMA list.
** Calling/Exit State:
**	DAK_DMALIST_LOCK within.
*/
STATIC void
dak_dma_freelist(dak_dma_t *dmap)
{
	pl_t  opri;

	ASSERT(dmap);

	DAK_DMALIST_LOCK(opri);
	dmap->d_next = dak_dfreelist;
	dak_dfreelist = dmap;
	if (dmap->d_next == NULL) {
		DAK_DMALIST_UNLOCK(opri);
		SV_BROADCAST(dak_dmalist_sv, 0);
	} else {
		DAK_DMALIST_UNLOCK(opri);
	}
}

/*
** Function Name: dak_wait()
** Description:
**	Poll for a completion from the host adapter.  If an interrupt
**	is seen, the HA's interrupt service routine sets
**	dak_waitflag to FALSE.
**  NOTE:
**	This routine allows for no concurrency and as such, should
**	be used selectively.
** Calling/Exit State:
**	None.
*/
STATIC int
dak_wait(int c, int time)
{
	int ret = FAILURE;

	while (time > 0) {
		if (dak_waitflag == FALSE) {
			ret = SUCCESS;
			break;
		}
		drv_usecwait(1000);
		time--;
	}
	dak_waitflag = TRUE;
	if (ret == FAILURE) {
		cmn_err(CE_WARN, "!%s: Command Completion not indicated, dak_wait()\n", dakidata[c].name);
	}

	return (ret);
}


/*
** Function name: dak_init_cps()
** Description:
**      Initialize the controller CP free list.
** Calling/Exit State:
**	None.
*/
STATIC void
dak_init_cps(int c)
{
	register struct dak_scsi_ha  *ha = &dak_sc_ha[c];
	register struct dak_ccb  *cp;
	register int    i;

	ha->ha_cblist = NULL;

	for (i = 0; i < NCPS; i++)
	{
		cp = &ha->ha_ccb[i];
		cp->c_active  = 0;
		cp->c_index   = (ushort_t)i;
		cp->c_bind    = NULL;

		/** Save data addresses into CP ***/
		cp->c_addr    = vtop((caddr_t)cp, NULL);
		cp->c_cdb_addr    = vtop((caddr_t)&cp->dcdb, NULL);

		/** Save Command Packet virtual address pointer ***/
		cp->vp = cp;
		cp->c_next    = ha->ha_cblist;
		ha->ha_cblist = cp;
	}
}


/*
** Function name: dak_eisa_ha_init()
** Description:
**      Reset the HA board and initialize the communications.
** Calling/Exit State:
**	None.
*/

STATIC int
dak_eisa_ha_init(int c, int sleepflag)  /* HA controller number */
{
	unsigned long lphys;
	struct ENQ_DATA *enq_buf_ptr;
	struct DIRECT_CDB *dcdb_ptr;
	register struct dak_scsi_ha  *ha = &dak_sc_ha[c];
	int i, slot_id_addr,found;
	char *temp_buffer;
	int channel, target;

#if PDI_VERSION >= PDI_UNIXWARE20
	if ((dakidata[c].ioaddr1 & DAK_IOADDR_MASK) == DAK_UNDEFINED){
		ha->ha_base = 
		dakidata[c].ioaddr1 = (dakidata[c].ioaddr1 & (~DAK_IOADDR_MASK))
					+ DAK_EISA_BASE;
	}
	found = 0;
	slot_id_addr = SLOT_ID_ADDR((ha->ha_base & 
			(~DAK_IOADDR_MASK)) >> 12); 
	if ((inb(slot_id_addr) == 0x35) &&
	    (inb(slot_id_addr + 1) == 0x98) &&
	    (inb(slot_id_addr + 2) == 0) &&
	   ((inb(slot_id_addr + 3) & 0xf0) == 0x70) )
		found++;

	if (!found)
		return(1);

#else /* PDI_VERSION >= PDI_UNIXWARE20 */
	found = 0;
	for (i = 1; i <= 15; i++) {
		slot_id_addr = SLOT_ID_ADDR(i); /* iC80 */
		if ((inb(slot_id_addr) == 0x35) &&
		    (inb(slot_id_addr + 1) == 0x98) &&\
		    (inb(slot_id_addr + 2) == 0) &&\
       		   ((inb(slot_id_addr +3) & 0xf0) == 0x70) ) {
			found++;
			if(found != (c+1)) 
				continue;
#ifdef DAC_DEBUG
	cmn_err(CE_NOTE,"DAC Host Adapter[%d]: We are in slot %d\n",c,i);
#endif /* DAC_DEBUG */
			break;
		}
	}
	if(i == 16)
		return (1);
	if(!found)
		return (1);
#if 0
	/* found adapter , patch Sd01diskinfo variable to reflect correct 
	  geometry this we need to do because of shortcomings in UnixWare 
	  sd01 driver which hard codes geometry */

	if(Sd01diskinfo[0] != 0x2080)
	{
		cmn_err(CE_WARN,"dak: setting disk geometry to reflect 128 heads/Cyl");
		Sd01diskinfo[0] = 0x2080;
	}
#endif

	ha->ha_base = SLOT_ID_ADDR(i); /* zC80 */
	dakidata[c].ioaddr1 = ha->ha_base;
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	if ((temp_buffer = KMEM_ZALLOC(DAK_TEMP_BUFSZ, sleepflag)) == NULL) {
		return (1);
	}
	enq_buf_ptr = (struct ENQ_DATA *)(void *)temp_buffer;
	lphys= vtop((caddr_t)enq_buf_ptr, NULL);
	ha->ha_state |= C_SANITY; /* Mark HA operational */

#if PDI_VERSION < PDI_UNIXWARE20
	switch((inb(ha->ha_base + IRQNO)) & 0x60)
	{
		case 0:
			ha->ha_vect = 15;
			break;
		case 0x20:
			ha->ha_vect = 11;
			break;
		case 0x40:
			ha->ha_vect = 12;
			break;
		case 0x60:
			ha->ha_vect = 14;
			break;

	}
	if(dak_mod_dynamic)
	{
		dak_af = ( struct mod_drvintr *)&dak_attach_info;

  		 * set interrupt parameters

		dak_af->drv_intrinfo[c].ivect_no = ha->ha_vect;
		dak_af->drv_intrinfo[c].int_pri = 5;
		dak_af->drv_intrinfo[c].itype=3;
		dak_af->ihndler = dakintr;
	}
#endif /* PDI_VERSION < PDI_UNIXWARE20 */

	/*  Now fill in drive sizes */
		
	/* Disable Interrupts */
	outb(ha->ha_base + 0x09, 0);
	outb(ha->ha_base + 0x0e, 0);
		
	enq_buf_ptr->nsdrives=0;  /* Just in case */
	if (dak_eisa_issue_cmd(ha, OP_ENQ, 0, (char *)&lphys, DAK_INTR_OFF))
		return (1);
	/*
	 * Set max jobs, but leave a buffer of 2 for high priority jobs
	 */
	ha->ha_maxjobs = enq_buf_ptr->max_iop - 2;
    	ha->ha_minor_fwver = enq_buf_ptr->dac_minor_fwver;
    	ha->ha_major_fwver = enq_buf_ptr->dac_major_fwver;

  	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Max concurrent jobs= %d",c,enq_buf_ptr->max_iop);

  	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: No of Logical Drives= %d",c,enq_buf_ptr->nsdrives);

	for(i=0;i<enq_buf_ptr->nsdrives;i++) {
		dak_disk_size[c][i] = enq_buf_ptr->sdrvsz[i];
#ifdef DAK_DEBUG
		if (dak_debug)
			cmn_err(CE_NOTE,"dak: disk %d size = %d",i,enq_buf_ptr->sdrvsz[i]);
#endif
	}
				
#if (PDI_VERSION >= PDI_UNIXWARE20)
	/* Now get number of channels  */
		
	for ( channel = 0; ; channel++){

		outb(ha->ha_base + 0x12, channel);
		outb(ha->ha_base + 0x13, 0);
		if (dak_eisa_issue_cmd(ha, OP_GETDEV_STATE, 0, (char *)&lphys, 
		    DAK_INTR_OFF)) {
			break;
		}
	}
	
	dakidata[c].idata_nbus = (channel <= MAXCHANNEL) ? 
				 (uchar_t)channel:(uchar_t)MAXCHANNEL;

	/* Now get number of targets  */
		
	for ( target = 0; ; target++){

		outb(ha->ha_base + 0x12, 0);
		outb(ha->ha_base + 0x13, target);
		if (dak_eisa_issue_cmd(ha, OP_GETDEV_STATE, 0, (char *)&lphys, 
		    DAK_INTR_OFF)) {
			break;
		}
	}
	
	ha->ha_ntargets = (ushort_t)target;

#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	/* Now hunt for Non Disk devices */
 	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Scanning for Non Disk Devices",c);

	dcdb_ptr = (struct DIRECT_CDB *)(void *)temp_buffer;
	dcdb_ptr->dir = DATA_IN;
	dcdb_ptr->ptr = lphys+30; /* Grab Inq data in buffer itself in sense area */
	dcdb_ptr->cdb_len = 6;
	dcdb_ptr->cdb[0]=0x12;   /* Inquiry */
	dcdb_ptr->cdb[1]=0;
	dcdb_ptr->cdb[2]=0;
	dcdb_ptr->cdb[3]=0;
	dcdb_ptr->cdb[4]=0x30;
	dcdb_ptr->cdb[5]=0;

 	/* Init tape_channel to say there are no NON_DISK devices  */

	for(target = 0;target < MAXTARGET ; target++)
		ha->tape_channel[target] = 0xff;

	for(channel = 0; channel < (int)dakidata[c].idata_nbus; channel++) {
		for(target = 0; target < MAXTARGET; target++) {
			dcdb_ptr->device = (channel << 4) | target;
			dcdb_ptr->byte_cnt = 0x30;
			dcdb_ptr->sense_len = 0;
			if(dak_eisa_issue_cmd(ha, OP_DCDB, 0, (char *)&lphys, DAK_INTR_OFF) == 0) {
				if(temp_buffer[30] != ID_RANDOM) {
					if(temp_buffer[30] == 1) {
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d [TAPE]",c,channel,target);
					} else if(temp_buffer[30] == 5) {
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d [CDROM]",c,channel,target);
					} else
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d, Type = %d",c,channel,target,temp_buffer[30]);
					ha->tape_channel[target] = (char)channel;
				}
			}
		}
	}
	/* Enable Interrupts */
	outb(ha->ha_base + 0x09, 1);
	outb(ha->ha_base + 0x0e, 1);
		
	return( 0 );
}

/*
** Function name: dak_mca_ha_init()
** Description:
**      Reset the HA board and initialize the communications.
** Calling/Exit State:
**	None.
*/

STATIC int
dak_mca_ha_init(int c, int sleepflag)  /* HA controller number */
{
	unsigned long lphys;
	struct ENQ_DATA *enq_buf_ptr;
	struct DIRECT_CDB *dcdb_ptr;
	register struct dak_scsi_ha  *ha = &dak_sc_ha[c];
	char *temp_buffer;
	int i, channel, target;

	if ((ha->ha_mbx_addr = physmap(dakidata[c].idata_memaddr
		+ DAK_MCA_BIOS_OFF, DAK_MCA_MEMMAP_SZ, sleepflag)) == NULL) {
		cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Cannot physmap mem at %x",c,dakidata[c].idata_memaddr+DAK_MCA_BIOS_OFF);
		return (1);
	}
	if ((temp_buffer = KMEM_ZALLOC(DAK_TEMP_BUFSZ, sleepflag)) == NULL) {
		cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Cannot alloc temp",c);
		return (1);
	}
	enq_buf_ptr = (struct ENQ_DATA *)(void *)temp_buffer;
	lphys= vtop((caddr_t)enq_buf_ptr, NULL);
	ha->ha_state |= C_SANITY; /* Mark HA operational */

#if PDI_VERSION < PDI_UNIXWARE20
	if(dak_mod_dynamic)
	{
		dak_af = ( struct mod_drvintr *)&dak_attach_info;

  		 * set interrupt parameters

		dak_af->drv_intrinfo[c].ivect_no = ha->ha_vect;
		dak_af->drv_intrinfo[c].int_pri = 5;
		dak_af->drv_intrinfo[c].itype=3;
		dak_af->ihndler = dakintr;
	}
#endif /* PDI_VERSION < PDI_UNIXWARE20 */

	/* Disable Interrupts */
	outb(ha->ha_base + 0x05, 0x42);
		
	enq_buf_ptr->nsdrives=0;  /* Just in case */
	if (dak_mca_issue_cmd(ha, OP_ENQ, 0, (char *)&lphys, DAK_INTR_OFF))
		return (1);
	/*
	 * Set max jobs, but leave a buffer of 2 for high priority jobs
	 */
	ha->ha_maxjobs = enq_buf_ptr->max_iop - 2;
    	ha->ha_minor_fwver = enq_buf_ptr->dac_minor_fwver;
    	ha->ha_major_fwver = enq_buf_ptr->dac_major_fwver;

  	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Max concurrent jobs= %d",c,enq_buf_ptr->max_iop);

  	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: No of Logical Drives= %d",c,enq_buf_ptr->nsdrives);
	for(i=0;i<enq_buf_ptr->nsdrives;i++) {
		dak_disk_size[c][i] = enq_buf_ptr->sdrvsz[i];
#ifdef DAK_DEBUG
		if (dak_debug)
			cmn_err(CE_NOTE,"dak: disk %d size = %d",i,enq_buf_ptr->sdrvsz[i]);
#endif
	}
				
#if (PDI_VERSION >= PDI_UNIXWARE20)

	/* Now get number of channels  */
		
	for ( channel = 0; ; channel++){
		char *mbxp = (char *)ha->ha_mbx_addr;

		mbxp[2] = (char)channel;
		mbxp[3] = 0;
		if (dak_mca_issue_cmd(ha, OP_GETDEV_STATE, 0, (char *)&lphys, 
		    DAK_INTR_OFF)) {
			break;
		}
	}
	
	dakidata[c].idata_nbus = (channel <= MAXCHANNEL) ? 
				 (uchar_t)channel:(uchar_t)MAXCHANNEL;

	/* Now get number of targets  */
		
	for ( target = 0; ; target++){
		char *mbxp = (char *)ha->ha_mbx_addr;

		mbxp[2] = 0;
		mbxp[3] = (char)target;
		if (dak_mca_issue_cmd(ha, OP_GETDEV_STATE, 0, (char *)&lphys, 
		    DAK_INTR_OFF)) {
			break;
		}
	}
	
	ha->ha_ntargets = (ushort_t)target;
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	/* Now hunt for Non Disk devices */
	
 	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Scanning for Non Disk Devices",c);

	dcdb_ptr = (struct DIRECT_CDB *)(void *)temp_buffer;
	dcdb_ptr->dir = DATA_IN;
	dcdb_ptr->ptr = lphys+30; /* Grab Inq data in buffer itself in sense area */
	dcdb_ptr->cdb_len = 6;
	dcdb_ptr->cdb[0]=0x12;   /* Inquiry */
	dcdb_ptr->cdb[1]=0;
	dcdb_ptr->cdb[2]=0;
	dcdb_ptr->cdb[3]=0;
	dcdb_ptr->cdb[4]=0x30;
	dcdb_ptr->cdb[5]=0;

 	/* Init tape_channel to say there are no NON_DISK devices  */

	for(target = 0;target < MAXTARGET ; target++)
		ha->tape_channel[target] = 0xff;

	for(channel = 0; channel < (int)dakidata[c].idata_nbus; channel++) {
		for(target = 0; target < MAXTARGET; target++) {
			dcdb_ptr->device = (channel << 4) | target;
			dcdb_ptr->byte_cnt = 0x30;
			dcdb_ptr->sense_len = 0;
			if(dak_mca_issue_cmd(ha, OP_DCDB, 0, (char *)&lphys, DAK_INTR_OFF) == 0) {
				if(temp_buffer[30] != ID_RANDOM) {
					if(temp_buffer[30] == 1) {
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d [TAPE]",c,channel,target);
					} else if(temp_buffer[30] == 5) {
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d [CDROM]",c,channel,target);
					} else
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d, Type = %d",c,channel,target,temp_buffer[30]);
					ha->tape_channel[target] = (char)channel;
				}
			}
		}
	}
	/* Enable Interrupts */
	outb(ha->ha_base + 0x05, 0x43);
		
	return( 0 );
}

/*
** Function name: dak_pci_ha_init()
** Description:
**      Reset the HA board and initialize the communications.
** Calling/Exit State:
**	None.
*/

STATIC int
dak_pci_ha_init(int c, int sleepflag)  /* HA controller number */
{
	unsigned long lphys;
	struct ENQ_DATA *enq_buf_ptr;
	struct DIRECT_CDB *dcdb_ptr;
	register struct dak_scsi_ha  *ha = &dak_sc_ha[c];
	int i;
	char *temp_buffer;
	int channel, target;

	if ((temp_buffer = KMEM_ZALLOC(DAK_TEMP_BUFSZ, sleepflag)) == NULL) {
		return (1);
	}
	enq_buf_ptr = (struct ENQ_DATA *)(void *)temp_buffer;
	lphys= vtop((caddr_t)enq_buf_ptr, NULL);
	ha->ha_state |= C_SANITY; /* Mark HA operational */

#if PDI_VERSION < PDI_UNIXWARE20
	switch((inb(ha->ha_base + IRQNO)) & 0x60)
	{
		case 0:
			ha->ha_vect = 15;
			break;
		case 0x20:
			ha->ha_vect = 11;
			break;
		case 0x40:
			ha->ha_vect = 12;
			break;
		case 0x60:
			ha->ha_vect = 14;
			break;

	}
	if(dak_mod_dynamic)
	{
		dak_af = ( struct mod_drvintr *)&dak_attach_info;

  		 * set interrupt parameters

		dak_af->drv_intrinfo[c].ivect_no = ha->ha_vect;
		dak_af->drv_intrinfo[c].int_pri = 5;
		dak_af->drv_intrinfo[c].itype=3;
		dak_af->ihndler = dakintr;
	}
#endif /* PDI_VERSION < PDI_UNIXWARE20 */

	/*  Now fill in drive sizes */
		
	/* Disable Interrupts */
	outb(ha->ha_base + 0x43, 0);
		
	enq_buf_ptr->nsdrives=0;  /* Just in case */
	if (dak_pci_issue_cmd(ha, OP_ENQ, 0, (char *)&lphys, DAK_INTR_OFF))
		return (1);
	/*
	 * Set max jobs, but leave a buffer of 2 for high priority jobs
	 */
	ha->ha_maxjobs = enq_buf_ptr->max_iop - 2;
    	ha->ha_minor_fwver = enq_buf_ptr->dac_minor_fwver;
    	ha->ha_major_fwver = enq_buf_ptr->dac_major_fwver;

  	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Max concurrent jobs= %d",c,enq_buf_ptr->max_iop);
			
  	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: No of Logical Drives= %d",c,enq_buf_ptr->nsdrives);
	for(i=0;i<enq_buf_ptr->nsdrives;i++) {
		dak_disk_size[c][i] = enq_buf_ptr->sdrvsz[i];
#ifdef DAK_DEBUG
		if (dak_debug)
			cmn_err(CE_NOTE,"dak: disk %d size = %d",i,enq_buf_ptr->sdrvsz[i]);
#endif
	}
				
#if (PDI_VERSION >= PDI_UNIXWARE20)
	/* Now get number of channels  */
		
	for ( channel = 0; ; channel++){

		outb(ha->ha_base + 0x02, channel);
		outb(ha->ha_base + 0x03, 0);
		if (dak_pci_issue_cmd(ha, OP_GETDEV_STATE, 0, (char *)&lphys,
			DAK_INTR_OFF)) {
			break;
		}
	}
	
	dakidata[c].idata_nbus = (channel <= MAXCHANNEL) ? 
				 (uchar_t)channel:(uchar_t)MAXCHANNEL;

	/* Now get number of targets  */
		
	for ( target = 0; ; target++){

		outb(ha->ha_base + 0x02, 0);
		outb(ha->ha_base + 0x03, target);
		if (dak_pci_issue_cmd(ha, OP_GETDEV_STATE, 0, (char *)&lphys,
			DAK_INTR_OFF)) {
			break;
		}
	}
	
	ha->ha_ntargets = (ushort_t)target;
#endif /* PDI_VERSION >= PDI_UNIXWARE20 */

	/* Now hunt for Non Disk devices */
 	cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Scanning for Non Disk Devices",c);

	dcdb_ptr = (struct DIRECT_CDB *)(void *)temp_buffer;
	dcdb_ptr->dir = DATA_IN;
	dcdb_ptr->ptr = lphys+30; /* Grab Inq data in buffer itself in sense area */
	dcdb_ptr->cdb_len = 6;
	dcdb_ptr->cdb[0]=0x12;   /* Inquiry */
	dcdb_ptr->cdb[1]=0;
	dcdb_ptr->cdb[2]=0;
	dcdb_ptr->cdb[3]=0;
	dcdb_ptr->cdb[4]=0x30;
	dcdb_ptr->cdb[5]=0;

 	/* Init tape_channel to say there are no NON_DISK devices  */

	for(target = 0;target < MAXTARGET ; target++)
		ha->tape_channel[target] = 0xff;

	for(channel = 0; channel < (int)dakidata[c].idata_nbus; channel++) {
		for(target = 0; target < MAXTARGET; target++) {
			dcdb_ptr->device = (channel << 4) | target;
			dcdb_ptr->byte_cnt = 0x30;
			dcdb_ptr->sense_len = 0;
			if (dak_pci_issue_cmd(ha, OP_DCDB, 0, (char *)&lphys, DAK_INTR_OFF) == 0) {

				if(temp_buffer[30] != ID_RANDOM) {
					if(temp_buffer[30] == 1) {
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d [TAPE]",c,channel,target);
					} else if(temp_buffer[30] == 5) {
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d [CDROM]",c,channel,target);
					} else
	 					cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: channel = %d,target = %d, Type = %d",c,channel,target,temp_buffer[30]);
					ha->tape_channel[target] = (char)channel;
				}
			}
		}
	}
	/* Enable Interrupts */
	outb(ha->ha_base + 0x43, 1);
		
	return(0);
}

/*
** STATIC int dak_illegal
** Calling/Exit State:
**	None.
*/
STATIC int
dak_illegal(short hba, int bus, unsigned char scsi_id, unsigned char lun)
{
	if (SDI_REDT(hba, bus, scsi_id, lun)) {
		return 0;
	} else {
		return 1;
	}
}

#if (PDI_VERSION >= PDI_SVR42MP)
/*
 * STATIC void *
 * dak_kmem_zalloc_physreq (size_t size, int flags)
 *		
 * function to be used in place of kma_zalloc
 * which allocates contiguous memory, using kmem_alloc_physreq,
 * and zero's the memory.
 *
** Calling/Exit State:
**	None.
 */
STATIC void *
dak_kmem_zalloc_physreq (size_t size, int flags)
{
	void *mem;
	physreq_t *preq;

	preq = physreq_alloc(flags);
	if (preq == NULL)
		return NULL;
	preq->phys_align = DAK_MEMALIGN;
	preq->phys_boundary = DAK_BOUNDARY;
	preq->phys_dmasize = 24;
	preq->phys_flags |= PREQ_PHYSCONTIG;
	if (!physreq_prep(preq, flags)) {
		physreq_free(preq);
		return NULL;
	}
	mem = kmem_alloc_physreq(size, preq, flags);
	physreq_free(preq);
	if (mem)
		bzero(mem, size);

	return mem;
}
#endif /* (PDI_VERSION >= PDI_SVR42MP) */
STATIC
USHORT  dachlpGetM16(PUCHAR p)
{
	USHORT  s;
	PUCHAR  sp=(PUCHAR)&s;

	sp[0] = p[1];
	sp[1] = p[0];
	return(s);
}

STATIC
ULONG   dachlpGetM24(PUCHAR p)
{
	ULONG   l;
	PUCHAR  lp=(PUCHAR)&l;

	lp[0] = p[2];
	lp[1] = p[1];
	lp[2] = p[0];
	lp[3] = 0;
	return(l);
}

STATIC
ULONG   dachlpGetM32(PUCHAR p)
{
	ULONG   l;
	PUCHAR  lp=(PUCHAR)&l;

	lp[0] = p[3];
	lp[1] = p[2];
	lp[2] = p[1];
	lp[3] = p[0];
	return(l);
}

#if PDI_VERSION >= PDI_SVR42MP
/*
 * STATIC void
 * dak_lockinit(int c)
 *
 * Description:  Initialize dak locks:
 *		1) device queue locks (one per queue),
 *		2) dmalists lock (one only),
 *		3) ccb lock (one only),
 *		4) scatter/gather lists lock (one only),
 *		5) Command Status Packet lock (one per controller).
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
dak_lockinit(int c)
{
	register struct dak_scsi_ha  *ha;
	register struct dak_scsi_lu *q;
	register int	b,t,l;
	int		sleepflag;
	static		firsttime = 1;

#ifdef DAK_DEBUG
	if(dak_debug > 0)
		cmn_err ( CE_CONT, "dak_lockinit()\n");
#endif
	sleepflag = dak_mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if (firsttime) {

		dak_dmalist_sv = SV_ALLOC (sleepflag);
		dak_dmalist_lock = 
                    LOCK_ALLOC(DAK_HIER, pldisk, &dak_lkinfo_dmalist, sleepflag);
		dak_ccb_lock = 
                    LOCK_ALLOC(DAK_HIER+2, pldisk, &dak_lkinfo_ccb, sleepflag);
		firsttime = 0;
	}

	ha = &dak_sc_ha[c];
	ha->ha_ctl_lock = 
            LOCK_ALLOC(DAK_HIER+1, pldisk, &dak_lkinfo_ctl, sleepflag);
	for (b = 0; b < (int)(dakidata[c].idata_nbus); b++) {
		for (t = 0; t < MAX_TCS; t++) {
			for (l = 0; l < MAX_LUS; l++) {
				q = &LU_Q (c,b,t,l);
				q->q_lock = LOCK_ALLOC(DAK_HIER, pldisk, 
						&dak_lkinfo_q, sleepflag);
			}
		}
	}
}

/*
 * STATIC void
 * dak_lockclean(int c)
 *
 *	Removes unneeded locks.  Controllers that are not active will
 *	have all locks removed.  Active controllers will have locks for
 *	all non-existant devices removed.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC void
dak_lockclean(int c)
{
	register struct dak_scsi_ha  *ha;
	register struct dak_scsi_lu *q;
	register int	b,t,l;
	static		firsttime = 1;

#ifdef DAK_DEBUG
	if(dak_debug > 0)
		cmn_err ( CE_CONT, "dak_lockclean(%d)\n", c);
#endif
	if (firsttime && !dak_hacnt) {

		if (dak_dmalist_sv == NULL)
			return;
		SV_DEALLOC (dak_dmalist_sv);
		LOCK_DEALLOC (dak_dmalist_lock);
		LOCK_DEALLOC (dak_ccb_lock);
		firsttime = 0;
	}

	if( !dakidata[c].active) {
		ha = &dak_sc_ha[c];
		if (ha->ha_ctl_lock == NULL)
			return;
	}

	for (b = 0; b < (int)(dakidata[c].idata_nbus); b++) {
		for (t = 0; t < MAX_TCS; t++) {
			for (l = 0; l < MAX_LUS; l++) {
				if (!dakidata[c].active || 
				    dak_illegal(dak_ltog[c], b, t, l)) {
					q = &LU_Q (c,b,t,l);
					LOCK_DEALLOC (q->q_lock);
				}
			}
		}
	}
}
#else /* PDI_VERSION < PDI_SVR42MP */

/*
 * STATIC void
 * dak_lockinit(int) 
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
dak_lockinit(int c)
{
}

/*
 * STATIC void
 * dak_lockclean(int) 
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
dak_lockclean(int c)
{
}
#endif /* PDI_VERSION < PDI_SVR42MP */

#if (PDI_VERSION >= PDI_UNIXWARE20)
/*
 * dakverify(rm_key_t key)
 *
 * Calling/Exit State:
 *	none
 *
 * Description:
 * 	Verify the board instance.
 */
int
dakverify(rm_key_t key)
{
	HBA_IDATA_STRUCT	vfy_idata;
	int	rv;

	/*
	 * Read the hardware parameters associated with the
	 * given Resource Manager key, and assign them to
	 * the idata structure.
	 */
	rv = sdi_hba_getconf(key, &vfy_idata);
	if(rv != 0)	{
		return(rv);
	}

	/*
	 * Verify hardware parameters in vfy_idata,
	 * return 0 on success, ENODEV otherwise.
	 */

	return (0);

}

int
dak_mca_conf(HBA_IDATA_STRUCT *idatap, int *ioalen, int *memalen)
{
	unsigned char pos[4];
	int iov;

	if (cm_read_devconfig(idatap->idata_rmkey, 2, pos, 4) != 4) {
		cmn_err(CE_CONT,
			"!%s could not read POS registers for MCA device\n",
			idatap->name);
		return(1);
	}

	iov = (pos[0]>>6) & 3;
	switch (iov) {
		case 0:
		idatap->iov = 14;
		break;

		case 2:
		idatap->iov = 11;
		break;

		case 3:
		idatap->iov = 10;
		break;

		default:
		cmn_err(CE_CONT,
		"!%s invalid IRQ in POS 0 registers for MCA device pos[0]=%x\n",
		idatap->name, pos[0]);
		return(1);

	}
	idatap->ioaddr1 = 0x1c00 + (0x2000 * ((pos[3]>>3) & 7));
	idatap->idata_memaddr = 0xc0000 + (0x1000 * ((pos[0]>>1) & 0x1f));
	idatap->dmachan1 = -1;

	*ioalen = 0x20;
	*memalen = 0x2000;
	return(0);
}

STATIC int
dak_getiobus(int c)
{
	cm_num_t	bus_type;
	cm_args_t	cma;

	cma.cm_key = cm_getbrdkey( "dak", c );
	cma.cm_n = 0;
	cma.cm_param = CM_BRDBUSTYPE;
	cma.cm_val = &bus_type;
	cma.cm_vallen = sizeof(cm_num_t);

	cm_getval(&cma);

	switch(bus_type) {
	case CM_BUS_EISA:
		bus_type = AI_EISA_BUS;
		break;
	case CM_BUS_MCA:
		bus_type = AI_uCHNL_BUS;
		break;
	case CM_BUS_PCI:
		bus_type = AI_PCI_BUS;
		break;
	default:
		bus_type = -1;
		break;
	}
	return (int)bus_type;
}

STATIC int
dak_getitype(int c)
{
	cm_args_t	cma;
	cm_num_t	itype;

	cma.cm_key = cm_getbrdkey( "dak", c );
	cma.cm_n = 0;
	cma.cm_param = CM_ITYPE;
	cma.cm_val = &itype;
	cma.cm_vallen = sizeof(cm_num_t);

	cm_getval(&cma);

	return (int)itype;
}
#else /* (PDI_VERSION < PDI_UNIXWARE20) */

STATIC int
dak_getiobus(int c)
{
	int bus_type;

	switch(dak_sdi_ver.sv_machine) {
	case SDI_386_EISA:
		bus_type = AI_EISA_BUS;
		break;
	case SDI_386_MCA:
		bus_type = AI_uCHNL_BUS;
		break;
	default:
		bus_type = -1;
		break;
	}
	return bus_type;
}
#endif /* (PDI_VERSION < PDI_UNIXWARE20) */

dak_eisa_issue_cmd(struct dak_scsi_ha *ha, int cmd, int id, char *addr,int intr)
{
	int c, status, time, error, ret = 0;

	outb(ha->ha_base + 0x10, cmd);
	outb(ha->ha_base + 0x11, id);
	outb(ha->ha_base + 0x18, addr[0]);
	outb(ha->ha_base + 0x19, addr[1]);
	outb(ha->ha_base + 0x1a, addr[2]);
	outb(ha->ha_base + 0x1b, addr[3]);
			
	/* Fire Command */
	outb(ha->ha_base + 0x0d, 1);

	if (intr == DAK_INTR_ON)
		return (0);

	time = DAK_TEN_SEC;
	while(time--) {
		if(inb(ha->ha_base + 0x0f) & 1) break;
		drv_usecwait(1000);
	}
	if (time <= 0) {
		c = ha - &dak_sc_ha[0];
		cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Not responding, cmd %d",
			c, cmd);
		return (-1);
	}

	status = inb(ha->ha_base + 0x1e);
	error  = inb(ha->ha_base + 0x1f);
	
	if (status != 0 || error != 0) {
		c = ha - &dak_sc_ha[0];
		cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Status = %d Error = %d",
			c,status,error);
		ret = -1;
	}
			
	outb(ha->ha_base + 0x0f,3);
	outb(ha->ha_base + 0x0d,2);  /* Ack the interrupt */
	return (ret);
}
			
dak_mca_issue_cmd(struct dak_scsi_ha *ha, int cmd, int id, char *addr, int intr)
{
	char *mbxp = (char *)ha->ha_mbx_addr;
	int c, status, time, error, ret = 0;

	mbxp[0] = (char)cmd;
	mbxp[1] = (char)id;
	mbxp[8] =  addr[0];
	mbxp[9] =  addr[1];
	mbxp[10] = addr[2];
	mbxp[11] = addr[3];
			
	/* Fire Command */
	outb(ha->ha_base + 0x04, 0xd0);

	if (intr == DAK_INTR_ON)
		return (0);

	time = DAK_TEN_SEC;
	while(time--) {
		if(inb(ha->ha_base + 0x07) & 2) break;
		drv_usecwait(1000);
	}
	if (time <= 0) {
		c = ha - &dak_sc_ha[0];
		cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Not responding, cmd %d",
			c, cmd);
		return (-1);
	}
			
	if ((status = mbxp[0xe]) != 0 || (error = mbxp[0xf] != 0)) {
		c = ha - &dak_sc_ha[0];
		cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Status = %d, Error = %d",
			c, status, error);
		ret = -1;
	}
	
	outb(ha->ha_base + 0x04,0xd1);	/* Ack the controller */
	return (ret);
}

dak_pci_issue_cmd(struct dak_scsi_ha *ha, int cmd, int id, char *addr, int intr)
{
	int c, status, time, ret = 0;

	outb(ha->ha_base + 0x00, cmd);
	outb(ha->ha_base + 0x01, id);
	outb(ha->ha_base + 0x08, addr[0]);
	outb(ha->ha_base + 0x09, addr[1]);
	outb(ha->ha_base + 0x0a, addr[2]);
	outb(ha->ha_base + 0x0b, addr[3]);
			
	/* Fire Command */
	outb(ha->ha_base + 0x40, 1);
		
	if (intr == DAK_INTR_ON)
		return (0);

	time = DAK_TEN_SEC;
	while(time--) {
		if(inb(ha->ha_base + 0x41) & 1) break;
		drv_usecwait(1000);
	}
	if (time <= 0) {
		c = ha - &dak_sc_ha[0];
		cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Not responding",c);
		return (1);
	}
			
	status = inw(ha->ha_base + 0x0e);
	if (status != 0 ) {
		c = ha - &dak_sc_ha[0];
		cmn_err(CE_NOTE,"!DAC Host Adapter[%d]: Status = %d",c,status);
		ret = -1;
	}
	
	outb(ha->ha_base + 0x41, 1); /* Ack the controller */
	outb(ha->ha_base + 0x40, 2);
	return (ret);
}
