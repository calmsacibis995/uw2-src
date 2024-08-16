/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:pdi/dpt/dpt.c	1.1"
#ident	"@(#)kern-pdi:io/hba/dpt/dpt.c	1.66.1.28"
#ident	"$Header: $"

/*      Copyright (c) 1988, 1989, 1990, 1991  Intel Corporation     */
/*	All Rights Reserved	*/

/*      INTEL CORPORATION PROPRIETARY INFORMATION                          */
/*                                                                         */
/*	This software is supplied under the terms of a license agreement   */
/*	or nondisclosure agreement with Intel Corporation and may not be   */
/*	copied or disclosed except in accordance with the terms of that    */
/*	agreement.							   */
/*                                                                         */
/*      Copyright (c) 1990 Distributed Processing Technology               */
/*      All Rights Reserved                                                */

/****************************************************************************
**      ISA/EISA SCSI Host Adapter Driver for DPT PM2011/PM2012 A/B        **
*****************************************************************************/


#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <mem/immu.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>


#if (PDI_VERSION <= 1)
#include <io/target/dynstructs.h>
#include <io/hba/dpt.h>
#else /* !(PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#include <io/hba/dpt/dpt.h>
#endif /* !(PDI_VERSION <= 1) */

#include <util/mod/moddefs.h>
#include <io/dma.h>

#if PDI_VERSION >= PDI_SVR42MP
#include <util/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <io/ddi.h>
#include <io/ddi_i386at.h>
#else /* _KERNEL_HEADERS */
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/immu.h>
#include <sys/conf.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>


#if (PDI_VERSION <= 1)
#include <sys/dynstructs.h>
#include <sys/dpt.h>
#else /* !(PDI_VERSION <= 1) */
#include <sys/dynstructs.h>
#include <sys/dpt.h>
#endif /* !(PDI_VERSION <= 1) */

#include <sys/moddefs.h>
#include <sys/dma.h>

#if (PDI_VERSION >= PDI_SVR42MP)
#include <sys/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif
#endif /* _KERNEL_HEADERS */

/* Function Prototypes */
    
STATIC void dpt_cache_detect(int c, int sleepflag);
STATIC void dpt_mode_sense(int c);
STATIC int dpt_getadr(dpt_sblk_t *);
STATIC dpt_sblk_t *dpt_schedule(struct dpt_scsi_lu *, int);
void dpt_done(int c,struct dpt_ccb *cp,int status);
void dpt_ha_done(int c,struct dpt_ccb *cp,int status);
void dpt_timer(int c);
STATIC void dpt_pass_thru(struct buf *bp);
void dpt_int(struct sb *sp);
void dpt_flushq(register struct dpt_scsi_lu *q,int cc,int flag);
STATIC void dpt_putq(register struct dpt_scsi_lu *q,register dpt_sblk_t *sp);
STATIC void dpt_getq(register struct dpt_scsi_lu *q,register dpt_sblk_t *sp);
void dpt_next(register int c,register struct dpt_scsi_lu *q);
void dpt_cmd(register dpt_sblk_t *sp);
void dpt_func(register dpt_sblk_t *sp);
void dpt_send(int c,int cmd,struct dpt_ccb *cp);
struct dpt_ccb *dpt_getccb(int c);
void dpt_freeccb(int c,struct dpt_ccb *cp);
int dpt_dmalist(register dpt_sblk_t *sp,struct proc *procp, int sleepflag);
void dpt_dma_freelist(dpt_dma_t *dmap);
int dpt_wait(int c,int time,int intr);
void dpt_init_cps(int c);
int dpt_ha_init(int c);
int EATA_ReadConfig(int port);
int dpt_illegal(minor_t hba,int scsi_id,int lun,int m);
STATIC void dpt_lockinit(int c);
STATIC void dpt_lockclean(int c);
void dpt_eisa_slotids(int);
void dpt_set_config(void);
void dpt_board_reset(int);
#if PDI_VERSION >= PDI_SVR42MP
STATIC void *dpt_kmem_zalloc_physcontig (size_t size, int flags);
STATIC void dpt_pass_thru0(struct buf *bp);
#endif /* PDI_VERSION >= PDI_SVR42MP */

#ifdef DPT_DEBUG
int dptdebug;
STATIC int dpt_schedule_debug(dpt_sblk_t *, int);

#endif

#if PDI_VERSION < PDI_SVR42MP
typedef ulong_t vaddr_t;
#define KMEM_ZALLOC kmem_zalloc
#define KMEM_FREE kmem_free
#define dpt_pass_thru0 dpt_pass_thru

#else /* PDI_VERSION < PDI_SVR42MP */

#define KMEM_ZALLOC dpt_kmem_zalloc_physcontig
#define KMEM_FREE kmem_free_physcontig

#define DPT_MEMALIGN	512
#define DPT_BOUNDARY	0

#endif /* PDI_VERSION >= PDI_SVR42MP */

extern	void mod_drvattach(),  mod_drvdetach();
#if PDI_VERSION >= PDI_SVR42MP
extern int	timeout();
#endif

extern  struct	hba_idata	dptidata[];
extern	int	dpt_cntls;

#define		DRVNAME	"dpt - DPT SCSI HBA driver"

STATIC	sv_t   *dpt_dmalist_sv;		/* sleep argument for UP	*/

STATIC char *dpt_buf;
#define DPT_BUFSIZE 100

/* dpt multi-processor locking variables */

#if PDI_VERSION >= PDI_SVR42MP

STATIC	int    dpt_devflag = HBA_MP;
STATIC lock_t *dpt_dmalist_lock;
STATIC lock_t *dpt_ccb_lock;
STATIC LKINFO_DECL(dpt_lkinfo_dmalist, "IO:dpt:dpt_dmalist_lock", 0);
STATIC LKINFO_DECL(dpt_lkinfo_ccb, "IO:dpt:dpt_ccb_lock", 0);
STATIC LKINFO_DECL(dpt_lkinfo_StPkt, "IO:dpt:ha->ha_StPkt_lock", 0);
STATIC LKINFO_DECL(dpt_lkinfo_q, "IO:dpt:q->q_lock", 0);
#else
STATIC	int	dpt_devflag = 0;
#endif /* PDI_VERSION >= PDI_SVR42MP */

#define DPT_BSIZE(sp) (sp->sbp->sb.SCB.sc_datasz>>9)
#define DPT_IS_READ(c)         (c == SS_READ || c == SM_READ)
#define DPT_IS_WRITE(c)        (c == SS_WRITE || c == SM_WRITE)
#define DPT_IS_RW(c) (DPT_IS_READ(c) || DPT_IS_WRITE(c))
#define DPT_CMD(sp) (*((char *)sp->sbp->sb.SCB.sc_cmdpt))

#define DPT_SCHED(op, c)	\
	((DPT_IS_READ(op)) ||	\
	(DPT_IS_WRITE(op) && dpt_sc_ha[c].ha_cache != DPT_CACHE_WRITEBACK))

STATIC int dpt_eisa_optimize = 1;
STATIC int dpt_i386 = 0;

MOD_HDRV_WRAPPER(dpt, dpt_load, dpt_unload, dpthalt, DRVNAME);
HBA_INFO(dpt, &dpt_devflag, 0x20000);

static	int	mod_dynamic = 0;


/* pool for dynamic struct allocation for dpt_sblk_t */
extern struct head	sm_poolhead;

/* Allocated in space.c */
extern unsigned int     dpt_sc_hacnt;	/* Total # of controllers declared*/
extern int              dpt_ctlr_id;	/* hba scsi id			*/
extern struct ver_no    dpt_sdi_ver;	/* SDI version structure	*/
extern int	 dpt_gtol[];		/* xlate global hba# to loc */
extern int	 dpt_ltog[];		/* local hba# to global     */

struct dpt_scsi_ha   	*dpt_sc_ha;	/* SCSI HA structures		*/
STATIC unsigned	dpt_ha_structsize;	/* Allocation size of dpt_sc_ha */

scsi_stat_t      *dpt_sc_hastat;	/* SCSI HA Status Packets	*/

dpt_dma_t        *dpt_dmalistpool;	/* pointer to DMA list pool     */
dpt_dma_t        *dpt_dfreelist;	/* List of free DMA lists       */
char             *dpt_sc_cmd;		/* local SCSI command buffer    */
char             dpt_polltime;		/* Poll time flag	        */
STATIC	boolean_t dpt_waitflag = TRUE;	/* Using dpt_wait, TRUE		*/
int		 dpt_hacnt;		/* # hba's found */
struct RdConfig  *eata_cfg;		/* EATA Read Config Data struc  */

extern int	dpt_lu_max;	/* For each LU, the max number */
				/* of jobs on the dpt card concurrently */

extern int	dpt_hba_max;	/* Maximum number of jobs that */
				/* allowed on the HBA at one time */
extern int 	dpt_lu_max_sched;	/* Maximum number of jobs to be scheduled together */

struct dpt_eisa_ids {
	unsigned char id_byte1;
	unsigned char id_byte2;
	unsigned char id_byte3;
} dpt_eisa_ids[DPT_NVARIETIES] = {
{ DPT_EISA_ID1, DPT_EISA_ID2},
{ DPT_EISA_ATT_ID1, DPT_EISA_ATT_ID2, DPT_EISA_ATT_ID3},
{ DPT_EISA_NEC_ID1, DPT_EISA_NEC_ID2, DPT_EISA_NEC_ID3},
};
struct dpt_cfg dpt_cfg[MAX_EISA_SLOTS];
int dpt_ncfg;

STATIC boolean_t	dpt_intr_wait = FALSE;
STATIC int		dpt_cmd_in_progress;

/* asm long
 * dpt_swap32(ulong)
 *
 * Description: Fast byte-order swap
 *
 * Calling/Exit State:
 *	None
 */
asm long 
dpt_swap32(ulong i)
{
%mem i; lab is386, done;

	movl	i,%eax
	cmpl	$0, dpt_i386
	jne	is386
	bswap	%eax
	jmp	done
is386:
	pushl	%eax
	call	sdi_swap32
	popl	%ecx
done:
}

/* asm void
 * scsi_send_cmd(ulong base, paddr_t addr, int cmd)
 *
 * Description: Send scsi command to hba
 *
 * Calling/Exit State:
 *	None
 */
asm void
scsi_send_cmd(ulong base, paddr_t addr, int cmd)
{
%mem base, addr, cmd;  lab wait_not_busy, not_eisa, done;
        
	movl    base,%edx		
	addb    $8,%dl			

wait_not_busy:
	inb     (%dx)			
	testb   $1,%al			
	jnz     wait_not_busy	
	subb    $6,%dl	

	cmpl	$0, dpt_eisa_optimize
	je	not_eisa
	
	movl	addr,%eax
	bswap	%eax
	outl	(%dx)
	movl	cmd,%eax
	addb	$5,%dl
	outb	(%dx)
	jmp	done
	
not_eisa:
	pushl	addr
        movb    0x03(%esp),%al		
        outb    (%dx)			
        incl    %edx			
        movb    0x02(%esp),%al		
        outb    (%dx)			
        incl    %edx			
        movb    0x01(%esp),%al		
        outb    (%dx)			
        incl    %edx			
        movb    (%esp),%al		
        outb    (%dx)			
	movl	cmd,%eax			
        addl    $2,%edx			
        outb    (%dx)			
	popl	%ecx
done:
}


/*
 * STATIC int
 * dpt_load(void) 
 *
 * Description:
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
dpt_load(void)
{
	mod_dynamic = 1;
	if (dptinit()) {
		return(ENODEV);
	}
	mod_drvattach(&dpt_attach_info);
	if (dptstart()) {
		mod_drvdetach(&dpt_attach_info);
		return(ENODEV);
	}
	return(0);
}

/*
 * STATIC int
 * dpt_unload(void) 
 *
 * Description:
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
dpt_unload(void)
{
	return(EBUSY);
}

/*
 * int
 * dptinit(void)
 *
 * Description:
 *	This is the initialization routine for the DPT HA driver.
 *	Called by the main start loop if static or dpt_load if loadable.
 *
 * Calling/Exit State:
 *	None.
 */

int
dptinit(void)
{
	register struct dpt_scsi_ha *ha;
	int  i, c;
	static dptfirst_time = 1;
	int sleepflag;
	uint bus_p;
	struct cpuparms	cpu;

	if (!dptfirst_time) {
		/*
		 *+ DPT initializtion routine already run, so fail this time.
		 */
		cmn_err(CE_WARN,"!DPT: Already initialized.");
		return(-1);
	}
	dptfirst_time = 0;

	dpt_polltime = TRUE;

	if (drv_gethardware(PROC_INFO, &cpu) == -1) {
		/*
		 *+ dpt: cannot determine processor information at start time
		 */
		cmn_err(CE_WARN,
		"dpt: drv_gethardware(PROC_INFO) fails");
		return(-1);
	}
	if (cpu.cpu_id == CPU_i386) {
		dpt_i386 = 1;
		dpt_eisa_optimize = 0;		/* i386 cannot do bswap */
	}
	if (drv_gethardware(IOBUS_TYPE, &bus_p) == -1) {
		/*
		 *+ dpt: cannot determine bus type at start time
		 */
		cmn_err(CE_WARN,
		"dpt: drv_gethardware(IOBUS_TYPE) fails");
		return(-1);
	}

	/* if running in a micro-channel machine, skip initialization */
	if (bus_p == BUS_MCA) {
		dpt_sc_ha = NULL;
		return(-1);
	}

	for (i=0; i < MAX_HAS; i++) {
		dpt_gtol[i] = dpt_ltog[i] = -1;
	}

	sleepflag = mod_dynamic ? KM_SLEEP : KM_NOSLEEP;
	dpt_sdi_ver.sv_release = 1;
	if (bus_p == BUS_EISA)
		dpt_sdi_ver.sv_machine = SDI_386_EISA;
	else
		dpt_sdi_ver.sv_machine = SDI_386_AT;
	dpt_sdi_ver.sv_modes   = SDI_BASIC1;

	/* need to allocate space for sc_ha, must be contiguous */
	dpt_ha_structsize = dpt_sc_hacnt*(sizeof (struct dpt_scsi_ha));

	for (i = 2; i < dpt_ha_structsize ; i *= 2) ;
	dpt_ha_structsize = i;

	dpt_sc_ha = (struct dpt_scsi_ha *)kmem_zalloc(dpt_ha_structsize, 
		sleepflag);
	if (dpt_sc_ha == NULL) {
		/*
		 *+ DPT init routine cannot allocate host adapter structures.
		 */
		cmn_err(CE_WARN,"DPT Host Adapter: Initialization error - cannot allocate host adapter structures");
		return(-1);
	}

	eata_cfg = (struct RdConfig *)KMEM_ZALLOC(sizeof(struct RdConfig),sleepflag);
	if( eata_cfg == NULL) {
		/*
		 *+ DPT Allocation of eata_cfg failed.
		 */
		cmn_err(CE_WARN, "%s: Initalization error allocating dpt_sc_hastat\n", dptidata[0].name);
		kmem_free(dpt_sc_ha, dpt_ha_structsize );
		return( -1 );
	}

	if (bus_p == BUS_EISA) {
		dpt_eisa_slotids(MAX_EISA_SLOTS);
		dpt_set_config();
	}

	for (c = 0; c < dpt_cntls; c++) {
		ha = &dpt_sc_ha[c];
		ha->ha_id     = (unsigned short) dpt_ctlr_id;
		if (!ha->ha_base)
			ha->ha_base = dptidata[c].ioaddr1;
		ha->ha_vect = dptidata[c].iov;

		/* Find non-EISA boards */
		if (!(dptidata[c].active) && dpt_ha_init(c) != 0) 
			continue;

		dptidata[c].active = 1;		/* Set for dptstart */
	}

	return (0);
}

/*
 * int
 * dptstart(void)
 *
 * Description:
 *	This is the start routine for the DPT HA driver.
 *	Called by the main start loop if static or dpt_load if loadable.
 *
 * Calling/Exit State:
 *	None.
 */

int
dptstart(void)
{
	register struct dpt_scsi_ha *ha;
	register dpt_dma_t  *dp;
	int  i, j, c;
	unsigned  ccb_structsize, luq_structsize, dma_listsize;
	int sleepflag, cntl_num;

	HBA_IDATA(dpt_cntls);

	sleepflag = mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	/* allocate space for ccb's & LU queues, must be contiguous */

	ccb_structsize = dpt_hba_max*(sizeof (struct dpt_ccb));
	luq_structsize = MAX_EQ*(sizeof (struct dpt_scsi_lu));
	dma_listsize = NDMA * sizeof(dpt_dma_t);

#if PDI_VERSION < PDI_SVR42MP

	for (i = 2; i < ccb_structsize; i *= 2);
	ccb_structsize = i;

	for (i = 2; i < luq_structsize; i *= 2);
	luq_structsize = i;

	for (i = 2; i < dma_listsize; i *= 2);
	dma_listsize = i;
#endif /* PDI_VERSION < PDI_SVR42MP */
#ifdef	DPT_DEBUG
	if(ccb_structsize > PAGESIZE)
		/*
		 *+ DPT Allocation of Command Control blocks exceeds size
		 *+ of a page, so non-contiguous memory condition exists.
		 */
		cmn_err(CE_WARN, "%s: CCBs exceed pagesize (may cross physical page boundary)\n", dptidata[0].name);
	if(dma_listsize > PAGESIZE)
		/*
		 *+ DPT Allocation of DMA list exceeds size
		 *+ of a page, so non-contiguous memory condition exists.
		 */
		cmn_err(CE_WARN, "%s: dmalist exceeds pagesize (may cross physical page boundary)\n", dptidata[0].name);
#endif /* DPT_DEBUG */

	dpt_sc_hastat = (scsi_stat_t *)KMEM_ZALLOC((sizeof(struct Status_Packet) * dpt_cntls), sleepflag);
	if( dpt_sc_hastat == NULL ) {
		/*
		 *+ DPT Allocation of dpt_sc_hastat failed.
		 */
		cmn_err(CE_WARN, "%s: Initalization error allocating dpt_sc_hastat\n", dptidata[0].name);
		kmem_free(dpt_sc_ha, dpt_ha_structsize );
		KMEM_FREE(eata_cfg, sizeof(struct RdConfig));
		return(-1);
	}

   	dpt_dfreelist = NULL;
	for (c = 0; c < dpt_cntls; c++) {
	    dpt_dmalistpool = (dpt_dma_t *)KMEM_ZALLOC(dma_listsize, sleepflag);
	    if(dpt_dmalistpool == NULL) {
		/*
		 *+ DPT Allocation of DMA list pool failed.
		 */
		cmn_err(CE_WARN, "%s: Initalization error allocating dpt_dmalistpool\n", dptidata[0].name);
		kmem_free(dpt_sc_ha, dpt_ha_structsize );
		KMEM_FREE(dpt_sc_hastat, (sizeof(dpt_sc_hastat) * dpt_cntls));
		KMEM_FREE(eata_cfg, sizeof(struct RdConfig));
		return(-1);
	    }

	/* Build list of free DMA lists */

	    for (i = 0; i < NDMA; i++) {
	    	dp = &dpt_dmalistpool[i];
		dp->d_next = dpt_dfreelist;
		dpt_dfreelist = dp;
	    }
        }

	for (c = 0; c < dpt_cntls; c++) {

		if (!dptidata[c].active)
			/*
			 * dptinit did not find this board, so continue.
			 */
			continue;

		dptidata[c].active = 0;

		ha = &dpt_sc_ha[c];

		/* controller was found now alloc ccb's, LU Q's */
		ha->ha_ccb = (struct dpt_ccb *)KMEM_ZALLOC(ccb_structsize, sleepflag);
		if (ha->ha_ccb == NULL) {
			/*
			 *+ DPT Allocation of Command control blocks failed.
			 */
			cmn_err(CE_WARN,"DPT Host Adapter: Initialization error - cannot allocate command blocks");
			continue;
		}

		ha->ha_dev = (struct dpt_scsi_lu *)kmem_zalloc(luq_structsize, sleepflag);
		if (ha->ha_dev == NULL) {
			/*
			 *+ DPT Allocation of device queues failed.
			 */
			cmn_err(CE_WARN,"DPT Host Adapter: Initialization error - cannot allocate logical unit queues");
			continue;
		}
		
		dpt_init_cps(c);

		if ((cntl_num = sdi_gethbano(dptidata[c].cntlr)) <= -1) {
			/*
			 *+ DPT sdi_gethbano call to get registry number failed.
			 */
			cmn_err(CE_WARN,"%s: No HBA number available.",dptidata[c].name);
			continue;
		}
		dptidata[c].cntlr = cntl_num;
		dpt_gtol[cntl_num] = c;
		dpt_ltog[dpt_gtol[cntl_num]] = cntl_num;

  /* Allocate And Initalize The SMP Locks */

                dpt_lockinit(c); 

		/* Initialize ha_max_jobs so that jobs can be sent down 
		 * before ha_max_jobs gets its final value.
		 */

		dpt_sc_ha[c].ha_max_jobs = dpt_lu_max;
		if ((cntl_num=sdi_register(&dpthba_info,&dptidata[c])) < 0) {
			/*
			 *+ DPT sdi_register call to register HBA failed.
			 */
			cmn_err(CE_WARN,"%s: HA %d SDI register slot %d failed",
				dptidata[c].name, c, cntl_num);
				continue;
		}
		dptidata[c].active = 1;
		dpt_hacnt++;
	}

	/* Now cleanup any unnecessary structs */
	for (i = 0; i < dpt_cntls; i++) {
                dpt_lockclean(i);
		if( !dptidata[i].active) {
			ha = &dpt_sc_ha[i];
			if( ha->ha_ccb != NULL) {
                                KMEM_FREE((void *)ha->ha_ccb, ccb_structsize);
                                ha->ha_ccb = NULL;
			}
			if( ha->ha_dev != NULL) {
                                kmem_free((void *)ha->ha_dev, luq_structsize);
                                ha->ha_dev = NULL;
			}
		}
	}

	if (dpt_hacnt == 0) {
		/*
		 *+ DPT - No Host Adaptors found.
		 */
		cmn_err(CE_NOTE,"!%s: No HAs found.", dptidata[0].name);
		kmem_free((void *)dpt_sc_ha, dpt_ha_structsize);
		KMEM_FREE(dpt_dmalistpool, dma_listsize );
		dpt_sc_ha = NULL;
		/* Set hacnt to 0 so if dptintr gets an interrupt, the
		 * dpt_sc_ha array is not used, since it no longer exists 
		 */
		dpt_sc_hacnt = 0;
		return(-1);
	}
	/*
	 * Clear init time flag to stop the HA driver
	 * from polling for interrupts and begin taking
	 * interrupts normally.
	 */

	/* Find Random Access devices that should be scheduled */

	for (c=0; c < dpt_cntls; c++) {
		if (!dptidata[c].active)
			continue;
		for (i=0; i<=7; i++) {
			for (j=0; j<=7; j++) {
				struct sdi_edt *edtp;
				edtp = sdi_redt(dpt_ltog[c], i, j);
				if (edtp && edtp->pdtype == ID_RANDOM) {
					/* Other device types can potentially
					 * be scheduled, but the scheduling
					 * algorithm has been tuned only for
					 * disks.
					 */
					struct dpt_scsi_lu *q = &LU_Q(c, i, j);
					q->q_flag |= DPT_QSCHED;
				}
			}
		}
	}

	/* Set performance tunables */

	for (c=0; c < dpt_cntls; c++) {
		int targets = 0;
		if( !dptidata[c].active) 
			continue;

		dpt_cache_detect(c, sleepflag);

		for (i=0; i<=7; i++)  {
			for (j=0; j<=7; j++)  {
				if (sdi_redt(dpt_ltog[c], i, j)) {
					targets++;
					break;
				}
			}
		}
		/* One of the targets is the controller itself */

		targets--;
		if (targets != 0)
			dpt_sc_ha[c].ha_max_jobs = (dpt_hba_max - 1) / targets;
	}
	if (dpt_buf) 
		KMEM_FREE((void *)dpt_buf, DPT_BUFSIZE);
	dpt_polltime = FALSE;
	return(0);
}


/*
 * void
 * dpthalt(void)
 *
 * Description:
 *	Called by kernel at shutdown, the cache on all DPT controllers
 *      is flushed
 *
 * Calling/Exit State:
 *	None.
 */

void
dpthalt(void)
{
	register int	c, lun;
	register struct dpt_ccb		*cp;
	register struct dpt_scsi_ha	*ha;
	static BYTE	dpt_halt_done = 0;
	extern dpt_flush_time_out;

	if (dpt_halt_done) {
		return;
	}

	/********************************************************************
	** For each DPT HBA in the system Flush and invalidate cache for   **
	** all SCSI ID/LUNs by issuing a SCSI ALLOW MEDIA REMOVAL command. **
	** This will cause the HBA to flush all requests to the device and **
	** then invalidate its cache for that device.  This code operates  **
	** as a single thread, only one device is flushed at a time.  It   **
	** probably wont buy anything to send all of them at once, this is **
	** only run at Shutdown time.					   **
	********************************************************************/

	dpt_lu_max = 1;
	dpt_hba_max = 1;

	for (c = 0; c < dpt_sc_hacnt; c++) {

		ha = &dpt_sc_ha[c];
		/***
		** If the controller is not active, no need
		** to try flushing data 8-).
		***/
		if (!dptidata[c].active) {
			continue;
		}

		cp = dpt_getccb(c);
		cp->c_time = 0;

		/***
		** Build the EATA Command Packet structure
 		** for a SCSI Prevent/Allow Cmd
		***/
		cp->CPop.byte		= 0;
		cp->CPcdb[0]		= SS_LOCK; 
		*(ulong *)(void *)&cp->CPcdb[1]	= 0;
		cp->CPcdb[5]		= 0;
		cp->CPdataDMA 		= 0L;

		/*
		 *+ DPT issuing command to flush cache.
		 */
		cmn_err(CE_NOTE,"%s: Flushing cache, if present.",dptidata[c].name);
		for (cp->CPID=0; cp->CPID<=7; cp->CPID++) {
			if (cp->CPID == ha->ha_id)
				continue;
			for ( lun=0; lun<= 7; lun++) {

			     if( sdi_redt(dpt_ltog[c], cp->CPID, lun) == (struct sdi_edt *)0 ) {
				     continue;
			     }

			     cp->CPmsg0 = (HA_IDENTIFY_MSG | HA_DISCO_RECO)+lun;
			     cp->c_bind = NULL;
			     dpt_intr_wait = TRUE;
			     dpt_cmd_in_progress = SS_LOCK;
			     dpt_send(c, START, cp);
			     if( dpt_wait( c, dpt_flush_time_out, DPT_INTR_OFF ) == FAILURE ) {
			        /*
			         *+ DPT flush timed out before completed
				 *+ for given target,lun.
			         */
				cmn_err(CE_NOTE,"%s: Incomplete Flush - Target %d, LUN %d.",dptidata[c].name, cp->CPID,lun);
			        dpt_intr_wait = FALSE;
			     }

			}
		}

		/*
		 *+ DPT flush completed successfully.
		 */
		cmn_err(CE_NOTE,"%s: Flushing complete.",dptidata[c].name);

		dpt_freeccb(c, cp);	/* Release SCSI command block. */
	}
	dpt_halt_done = 1;
}

/*
 * int
 * dptopen(dev_t *devp, int flags, int otype, cred_t *cred_p)
 *
 * Description:
 * 	Driver open() entry point. It checks permissions, and in the
 *	case of a pass-thru open, suspends the particular LU queue.
 *
 * Calling/Exit State:
 *	None.
 */

/*ARGSUSED*/
STATIC int
HBAOPEN(dev_t *devp, int flags, int otype, cred_t *cred_p)
{
	dev_t	dev = *devp;
	register int	c = dpt_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct dpt_scsi_lu *q;

	if(dpt_illegal((minor_t)(SC_HAN(dev)), t, l, 0)) {
		return(ENXIO);
	}

	if (t == dpt_sc_ha[c].ha_id)
		return(0);

	/* This is the pass-thru section */

	q = &LU_Q(c, t, l);

	DPT_SCSILU_LOCK(q->q_opri);
	if ((q->q_count > 0) ||
		(q->q_active == dpt_lu_max) ||
		(q->q_flag & (DPT_QSUSP | DPT_QPTHRU)))
	{
		DPT_SCSILU_UNLOCK(q->q_opri);
		if(q->q_flag & DPT_QSUSP) {
			/*
			 *+ DPT open: device queue is suspended,
			 *+ open failed.
			 */
			cmn_err(CE_WARN,"DPTOPEN: DPT_QSUSP Set");
		}
		return(EBUSY);
	}

	q->q_flag |= DPT_QPTHRU;
	DPT_SCSILU_UNLOCK(q->q_opri);
	return(0);
}


/*
 * int
 * dptclose(dev_t dev, int flags, int otype, cred_t *cred_p)
 *
 * Description:
 * 	Driver close() entry point.  In the case of a pass-thru close
 *	it resumes the queue and calls the target driver event handler
 *	if one is present.
 *
 * Calling/Exit State:
 *	None.
 */

/*ARGSUSED*/
STATIC int
HBACLOSE(dev_t dev, int flags, int otype, cred_t *cred_p)
{
	register int	c = dpt_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct dpt_scsi_lu *q;

	if (t == dpt_sc_ha[c].ha_id)
		return(0);

	q = &LU_Q(c, t, l);

	DPT_SCSILU_LOCK(q->q_opri);
	q->q_flag &= ~DPT_QPTHRU;

	if (q->q_func != NULL) {
                DPT_SCSILU_UNLOCK(q->q_opri);
		(*q->q_func) (q->q_param, SDI_FLT_PTHRU);
	        DPT_SCSILU_LOCK(q->q_opri);
        }
	dpt_next(c,q);
	return(0);
}



/*
 * int
 * dptioctl(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
 *
 * Description:
 *	Driver ioctl() entry point.  Used to implement the following
 *	special functions:
 *
 *	SDI_SEND     -	Send a user supplied SCSI Control Block to
 *			the specified device.
 *	B_GETTYPE    -  Get bus type and driver name
 *	B_HA_CNT     -	Get number of HA boards configured
 *	REDT	     -	Read the extended EDT from RAM memory
 *	SDI_BRESET   -	Reset the specified SCSI bus
 *
 * Calling/Exit State:
 *	None.
 */

/*ARGSUSED*/
STATIC int
HBAIOCTL(dev_t dev, int cmd, caddr_t arg, int mode, cred_t *cred_p, int *rval_p)
{
	register int	c = dpt_gtol[SC_HAN(dev)];
	register int	t = SC_TCN(dev);
	register int	l = SC_LUN(dev);
	register struct sb *sp;
	pl_t opri;

	switch(cmd)
	{
	case SDI_SEND:
		{
			register buf_t *bp;
			struct sb  karg;
			int  errnum, rw;
			char *save_priv;

			if (t == dpt_sc_ha[c].ha_id) { 	/* illegal ID */
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

			sp = SDI_GETBLK(KM_SLEEP);
			save_priv = sp->SCB.sc_priv;
			bcopy((caddr_t)&karg, (caddr_t)sp, sizeof(struct sb));

			bp = getrbuf(KM_SLEEP);
			bp->b_iodone = NULL;
			if (dpt_sc_cmd == NULL) {
				dpt_sc_cmd = kmem_alloc(MAX_CMDSZ, KM_SLEEP);
			}
			sp->SCB.sc_priv = save_priv;
			sp->SCB.sc_cmdpt = dpt_sc_cmd;

			if (copyin(karg.SCB.sc_cmdpt, sp->SCB.sc_cmdpt,
			    sp->SCB.sc_cmdsz)) {
				freerbuf(bp);
				return(EFAULT);
			}
			sp->SCB.sc_dev.sa_lun = (unchar) l;
			sp->SCB.sc_dev.sa_fill = (dpt_ltog[c] << 3) | t;

			rw = (sp->SCB.sc_mode & SCB_READ) ? B_READ : B_WRITE;
#if PDI_VERSION < PDI_SVR42MP
			bp->b_private = (char *)sp;
#else
			bp->b_priv.un_ptr = sp;
#endif

			/*
			 * If the job involves a data transfer then the
			 * request is done thru physio() so that the user
			 * data area is locked in memory. If the job doesn't
			 * involve any data transfer then dpt_pass_thru()
			 * is called directly.
			 */
			if (sp->SCB.sc_datasz > 0)
			{
				struct iovec  ha_iov;
				struct uio    ha_uio;

				ha_iov.iov_base = sp->SCB.sc_datapt;
				ha_iov.iov_len = sp->SCB.sc_datasz;
				ha_uio.uio_iov = &ha_iov;
				ha_uio.uio_iovcnt = 1;
				ha_uio.uio_offset = 0;
				ha_uio.uio_segflg = UIO_USERSPACE;
				ha_uio.uio_fmode = 0;
				ha_uio.uio_resid = sp->SCB.sc_datasz;

				if (errnum = physiock((void(*)())dpt_pass_thru0, bp, dev, rw,
				    4194303, &ha_uio)) {
					freerbuf(bp);
					return(errnum);
				}
			} else {
				bp->b_un.b_addr = sp->SCB.sc_datapt;
				bp->b_bcount = sp->SCB.sc_datasz;
				bp->b_blkno = NULL;
				bp->b_edev = dev;
				bp->b_flags |= rw;

				dpt_pass_thru(bp);  /* Bypass physiock call */
				biowait(bp);
			}

			/* update user SCB fields */

			karg.SCB.sc_comp_code = sp->SCB.sc_comp_code;
			karg.SCB.sc_status = sp->SCB.sc_status;
			karg.SCB.sc_time = sp->SCB.sc_time;

			if (copyout((caddr_t)&karg, arg, sizeof(struct sb))) {
				freerbuf(bp);
				return(EFAULT);
			}

			freerbuf(bp);
			sdi_freeblk(sp);
			break;
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
		if (copyout((caddr_t)&dpt_sdi_ver,arg, sizeof(struct ver_no)))
			return(EFAULT);
		break;
	case SDI_BRESET:
		{
			register struct dpt_scsi_ha *ha = &dpt_sc_ha[c];

		        DPT_STPKT_LOCK(opri,ha);
			if (ha->ha_npend > 0)     /* jobs are outstanding */
                          {
		                DPT_STPKT_UNLOCK(opri,ha);
				return(EBUSY);
                          }  
			else {
				/*
				 *+ DPT Host Adapter SCSI bus reset command
				 *+ issued.
				 */
				cmn_err(CE_WARN,
					"!DPT Host Adapter: HA %d - Bus is being reset\n", dpt_ltog[c]);
				outb((dpt_sc_ha[c].ha_base + HA_COMMAND), CP_EATA_RESET);
				drv_usecwait(1000);

			}
		        DPT_STPKT_UNLOCK(opri,ha);
			break;
		}

	default:
		return(EINVAL);
	}

	return(0);
}

/*
 * void
 * dptintr(unsigned int vect)
 *
 * Description:
 *      Driver interrupt handler entry point.
 *      Called by kernel when a host adapter interrupt occurs.
 *
 * Calling/Exit State:
 *	None.
 */

void
dptintr(unsigned int vect)
	/* HA interrupt vector	*/
{
	register struct dpt_scsi_ha	 *ha;
	register struct dpt_ccb      *cp;
	register scsi_stat_t     *sp;
	register int             c, stat;
        pl_t  opri;

	/********************************************************************
	** Determine which host adapter interrupted from the interrupt     **
	** vector, Auxiliary Status register AUX_INTR flag.                **
	*********************************************************************/
	for (c = 0; c < dpt_sc_hacnt; c++) {
		ha = &dpt_sc_ha[c];
		if (ha->ha_vect == vect) {
			if (!dptidata[c].active) {
                            if(!dpt_polltime) 
			        if(inb(ha->ha_base + HA_AUX_STATUS) & 
                                                                 HA_AUX_INTR ) 
	                             inb(ha->ha_base + HA_STATUS);
			    continue;
                        }
                        DPT_STPKT_LOCK(opri,ha);
			if ( inb(ha->ha_base + HA_AUX_STATUS) & HA_AUX_INTR ) {
				ha->ha_npend--;
				break;
			} else DPT_STPKT_UNLOCK(opri,ha);
		}
	}
	if ( c == dpt_sc_hacnt ) {
#ifdef DPT_DEBUG
		/*
		 *+ DPT interrupt received from HBA marked inactive.
		 */
		cmn_err(CE_NOTE,"DPT inactive HBA received an interrupt");
#endif
		return;
	}

	sp = &dpt_sc_hastat[c];		/* Set Status Packet pointer.  */

	if( (sp->SP_Controller_Status & 0x80) != 0x80 ) {
		/** Spurious? **/
		/*
		 *+ DPT unexpected interrupt received. EOC not set.
		 */
		cmn_err(CE_NOTE,"!%s: Spurious Interrupt; EOC not set.",dptidata[c].name);
	        inb(ha->ha_base + HA_STATUS);
                DPT_STPKT_UNLOCK(opri,ha);
		return;
	}

	if( sp->CPaddr.vp == 0 ) {
		/** Spurious? **/
		/*
		 *+ DPT unexpected interrupt received. Command control
		 *+ block not set.
		 */
		cmn_err(CE_NOTE,"!%s: Spurious Interrupt; CCB pointer not set.",dptidata[c].name);
	        inb(ha->ha_base + HA_STATUS);
                DPT_STPKT_UNLOCK(opri,ha);
		return;
	}

	cp = sp->CPaddr.vp;		/* Get Command Packet Vpointer.*/
	cp->c_active = FALSE;		/* Mark it not active.         */

	cp->CP_Controller_Status  = sp->SP_Controller_Status & HA_STATUS_MASK;
	cp->CP_SCSI_Status        = sp->SP_SCSI_Status;

	/***
	** Clear the sp->SP_Controller_Status so that a subsequent
	** command completion will not get a false Good code.
	***/
	sp->SP_Controller_Status = 0;

	stat = inb(ha->ha_base + HA_STATUS); /* Read Controller Status Reg. */
        DPT_STPKT_UNLOCK(opri,ha);

	dpt_waitflag = FALSE;
	if (cp->c_bind == NULL) {
		/* The command was issued internally, and dpt_wait was
		 * called to poll for completion.  Make sure the parameters
		 * indicate this, since otherwise it is spurious.
		 */
		if ((dpt_intr_wait == TRUE) && 
		    (dpt_cmd_in_progress == cp->CPcdb[0])) {
			dpt_intr_wait = FALSE;
			return;
		}
		/*
		 *+ DPT unexpected interrupt received. No associated SCSI
		 *+ block set.
		 */
		cmn_err(CE_NOTE,"!%s: Spurious Interrupt; c_bind not set.",dptidata[c].name);
		return;
	}

	if (cp->c_bind->sb_type == SFB_TYPE) {
		dpt_ha_done(c, cp, stat);
	}
	else {
		dpt_done(c, cp, stat);
	}

	return;
}

/*
 * void
 * dpt_done(int c, struct dpt_ccb *cp, int status)
 *
 * Description:
 *	This is the interrupt handler routine for SCSI jobs which have
 *	a controller CB and a SCB structure defining the job.
 *
 * Calling/Exit State:
 *	None.
 */

void
dpt_done(int c, struct dpt_ccb *cp, int status)
	  	/* HA controller number */
		/* command block */
{
	register struct dpt_scsi_lu  *q;
	register struct sb *sp;
	ulong curtime;
	int scheduled;

	/* CPID holds t and CPmsg0 holds l  */
	q = &LU_Q(c, (char)cp->CPID, (char)(cp->CPmsg0 & 0x07));

	sp = cp->c_bind;

	ASSERT(sp);

	DPT_SCSILU_LOCK(q->q_opri);
	q->q_flag &= ~DPT_QSENSE;        /* Old sense data now invalid  */
	DPT_SCSILU_UNLOCK(q->q_opri);

	/** Determine completion status of the job ***/

	if ( !(status & HA_ST_ERROR) && cp->CP_Controller_Status == 0 ) {
		sp->SCB.sc_comp_code = SDI_ASW;
	} else if (cp->CP_Controller_Status) {
		if( ((struct scs *)(void *)sp->SCB.sc_cmdpt)->ss_op != SS_INQUIR ) {
			cmn_err(CE_CONT,
			   "!DPT_DONE: Controller Status = 0x%x, OpCode = 0x%x",
			   cp->CP_Controller_Status,
			   ((struct scs *)(void *)sp->SCB.sc_cmdpt)->ss_op);
		}

		sp->SCB.sc_status = cp->CP_SCSI_Status;

		switch (cp->CP_Controller_Status) {
		case HA_ERR_SELTO:
			sp->SCB.sc_comp_code = SDI_RETRY;
			break;
		case HA_ERR_RESET:
			sp->SCB.sc_comp_code = SDI_RESET;
			break;
		case HA_ERR_INITPWR:
			sp->SCB.sc_comp_code = SDI_ASW;
			break;
		default:
			sp->SCB.sc_comp_code = SDI_RETRY;
			break;
		}

	} else if (cp->CP_SCSI_Status) {
		if( ((struct scs *)(void *)sp->SCB.sc_cmdpt)->ss_op != SS_INQUIR ) {
			cmn_err(CE_CONT,
			    "!DPT_DONE: SCSI Status = 0x%x, OpCode = 0x%x",
			    cp->CP_SCSI_Status,
			    ((struct scs *)(void *)sp->SCB.sc_cmdpt)->ss_op);
		}

	   	sp->SCB.sc_status = cp->CP_SCSI_Status;
		sp->SCB.sc_comp_code = SDI_CKSTAT;

		/* Cache request sense info into QueSense  */
		   bcopy((caddr_t)(cp->sense),
		       (caddr_t)(&q->q_sense)+1, sizeof(q->q_sense)-1);
	        DPT_SCSILU_LOCK(q->q_opri);
	        q->q_flag |= DPT_QSENSE;
	        DPT_SCSILU_UNLOCK(q->q_opri);
	}
	else {
		sp->SCB.sc_comp_code = SDI_ASW;
	}
	drv_getparm(LBOLT, (ulong *)&curtime);

	scheduled = DPT_SCHED(*(char *)sp->SCB.sc_cmdpt, c);

	sdi_callback(sp);	/* call target driver interrupt handler */

	dpt_freeccb(c, cp);	/* Release SCSI command block. */

	DPT_SCSILU_LOCK(q->q_opri);
	if (scheduled)
		q->q_active_sched--;
	q->q_active--;		/* for this queue in particular */
	dpt_next(c,q);		/* Send Next Job on the Queue. */
}


/*
 * void
 * dpt_ha_done(int c, struct dpt_ccb *cp, int status)
 *
 * Description:
 *	This is the interrupt handler routine for SCSI jobs which have
 *      a controller CP and a SFB structure defining the job.
 *
 * Calling/Exit State:
 *	None.
 */

void
dpt_ha_done(int c, struct dpt_ccb *cp, int status)
		/* HA controller number */
		/* command block */
{
	register struct dpt_scsi_lu  *q;
	register struct sb *sp;

	/* CPID holds t and CPmsg0 holds l  */
	q = &LU_Q(c, (char)cp->CPID, (char)(cp->CPmsg0 & 0x07));

	sp = cp->c_bind;

	ASSERT(sp);

	/* Determine completion status of the job */

	if ( (status & HA_ST_ERROR) && cp->CP_Controller_Status != S_GOOD ) {
		sp->SFB.sf_comp_code = SDI_RETRY;
	} else {
		sp->SFB.sf_comp_code = SDI_ASW;
		/* UPDATE: ad1542.c calls flushq(q, SDI_CRESET, 0); here */
	}

	sdi_callback(sp);	/* call target driver interrupt handler */

	dpt_freeccb(c, cp);	/* Release SCSI command block. */

	DPT_SCSILU_LOCK(q->q_opri);
	/* Do not need to decrement q->q_active_sched, since on SCB's are
	 * scheduled.
	 */
	q->q_active--;		/* for this queue in particular */

	dpt_next(c,q);		/* Send Next Job on the Queue. */
}


/*===========================================================================*/
/* SCSI Driver Interface (SDI-386) Functions
/*===========================================================================*/

/*
 * long
 * dptsend(struct hbadata *hbap)
 *
 * Description:
 * 	Send a SCSI command to a controller.  Commands sent via this
 *	function are executed in the order they are received.
 *
 * Calling/Exit State:
 *	None.
 */

/*ARGSUSED*/
STATIC long
HBASEND(struct hbadata *hbap, int sleepflag)
{
	register struct scsi_ad *sa;
	register struct dpt_scsi_lu *q;
	register dpt_sblk_t *sp = (dpt_sblk_t *) hbap;
	register int	c, t;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = dpt_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	if (sp->sbp->sb.sb_type != SCB_TYPE)
	{
		return (SDI_RET_ERR);
	}

	q = &LU_Q(c, t, sa->sa_lun);

	DPT_SCSILU_LOCK(q->q_opri);
	if (q->q_flag & DPT_QPTHRU) {
	        DPT_SCSILU_UNLOCK(q->q_opri);
		return (SDI_RET_RETRY);
	}

	sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
	sp->sbp->sb.SCB.sc_status = 0;

	dpt_putq(q, sp);

	if ( !(q->q_flag & DPT_QSUSP ) ) {
		dpt_next(c,q);
	} else {
                 DPT_SCSILU_UNLOCK(q->q_opri);
		/*
		 *+ DPT queue suspended, so not sending next job.
		 */
		cmn_err(CE_WARN,"DPTSEND: DPT_QSUSP Set, not calling dpt_next");
          }

	return (SDI_RET_OK);
}


/*
 * long
 * dpticmd(struct hbadata *hbap, int sleepflag)
 *
 * Description:
 *	Send an immediate command.  If the logical unit is busy, the job
 *	will be queued until the unit is free.  SFB operations will take
 *	priority over SCB operations.
 *
 * Calling/Exit State:
 *	None.
 */

/*ARGSUSED*/
STATIC long
HBAICMD(struct hbadata *hbap, int sleepflag)
{
	register struct scsi_ad *sa;
	register struct dpt_scsi_lu *q;
	register dpt_sblk_t *sp = (dpt_sblk_t *) hbap;
	register int	c, t, l;
	struct ident	 *inq_data;
	struct scs	     *inq_cdb;


	switch (sp->sbp->sb.sb_type)
	{
	case SFB_TYPE:
		sa = &sp->sbp->sb.SFB.sf_dev;
		c = dpt_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);

		sp->sbp->sb.SFB.sf_comp_code = SDI_ASW;

		switch (sp->sbp->sb.SFB.sf_func)
		{
		case SFB_RESUME:
		/*
		 *+ DPT device queue being resumed.
		 */
		cmn_err(CE_NOTE,"!DPTICMD: Clearing DPT_QSUSP");
	                DPT_SCSILU_LOCK(q->q_opri);
			q->q_flag &= ~DPT_QSUSP;
			dpt_next(c,q);
			break;
		case SFB_SUSPEND:
		/*
		 *+ DPT device queue being suspended.
		 */
		cmn_err(CE_NOTE,"!DPTICMD: Setting DPT_QSUSP");
	                DPT_SCSILU_LOCK(q->q_opri);
			q->q_flag |= DPT_QSUSP;
	                DPT_SCSILU_UNLOCK(q->q_opri);
			break;
		case SFB_ABORTM:
		case SFB_RESETM:
			sp->sbp->sb.SFB.sf_comp_code = SDI_PROGRES;
	                DPT_SCSILU_LOCK(q->q_opri);
			dpt_putq(q, sp);
			dpt_next(c,q);
			return (SDI_RET_OK);
		case SFB_FLUSHR:
			dpt_flushq(q, SDI_QFLUSH, 0);
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
		c = dpt_gtol[SDI_HAN(sa)];
		t = SDI_TCN(sa);
		l = SDI_LUN(sa);
		q = &LU_Q(c, t, sa->sa_lun);

		inq_cdb = (struct scs *)(void *)sp->sbp->sb.SCB.sc_cmdpt;
		if ((t == dpt_sc_ha[c].ha_id) && (l == 0) && (inq_cdb->ss_op == SS_INQUIR)) {
			inq_data = (struct ident *)(void *)sp->sbp->sb.SCB.sc_datapt;
			inq_data->id_type = ID_PROCESOR;
			(void)strncpy(inq_data->id_vendor, dptidata[c].name, 24);
			inq_data->id_vendor[23] = NULL;
			sp->sbp->sb.SCB.sc_comp_code = SDI_ASW;
			return (SDI_RET_OK);
		}

		sp->sbp->sb.SCB.sc_comp_code = SDI_PROGRES;
		sp->sbp->sb.SCB.sc_status = 0;

	        DPT_SCSILU_LOCK(q->q_opri);
		dpt_putq(q, sp);
		dpt_next(c,q);
		return (SDI_RET_OK);

	default:
		return (SDI_RET_ERR);
	}
}


/*
 * int
 * dptxlat(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
 *
 * Description:
 *	Perform the virtual to physical translation on the SCB
 *	data pointer.
 *
 * Calling/Exit State:
 *	None.
 */

/*ARGSUSED*/
STATIC HBAXLAT_DECL
HBAXLAT(struct hbadata *hbap, int flag, struct proc *procp, int sleepflag)
{
	register dpt_sblk_t *sp = (dpt_sblk_t *) hbap;
#if (PDI_VERSION <= PDI_UNIXWARE11)
	int sleepflag = KM_SLEEP;
#endif /* (PDI_VERSION <= PDI_UNIXWARE11) */

	if (sp->s_dmap)
	{
		dpt_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}

	if (sp->sbp->sb.SCB.sc_link)
	{
		/*
		 *+ DPT link commands not supported.
		 */
		cmn_err(CE_WARN, "!DPT Host Adapter: Linked commands NOT available\n");
		sp->sbp->sb.SCB.sc_link = NULL;
	}

	if (sp->sbp->sb.SCB.sc_datasz) {
		if (dpt_dmalist(sp, procp, sleepflag))
			;
			HBAXLAT_RETURN (SDI_RET_RETRY);
	} else {
		sp->s_addr = 0;
		sp->sbp->sb.SCB.sc_datasz = 0;
	}
	/* HBAXLAT_RETURN (SDI_RET_OK); */
}


/*
 * struct hbadata *
 * dptgetblk(int sleepflag)
 *
 * Description:
 *	Allocate a SB structure for the caller.  The function will
 *	sleep if there are no SCSI blocks available.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC struct hbadata *
HBAGETBLK(int sleepflag)
{
	dpt_sblk_t  *sp;

	sp = (dpt_sblk_t *)SDI_GET(&sm_poolhead, sleepflag);
	return ((struct hbadata *)sp);
}


/*
 * long
 * dptfreeblk(struct hbadata *hbap)
 *
 * Description:
 *	Release previously allocated SB structure. If a scatter/gather
 *	list is associated with the SB, it is freed via dpt_dma_freelist().
 *	A nonzero return indicates an error in pointer or type field.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC long
HBAFREEBLK(struct hbadata *hbap)
{
	register dpt_sblk_t *sp = (dpt_sblk_t *) hbap;


	if (sp->s_dmap) {
		dpt_dma_freelist(sp->s_dmap);
		sp->s_dmap = NULL;
	}
	sdi_free(&sm_poolhead, (jpool_t *)sp);
	return ((long) SDI_RET_OK);
}

/*
 * void
 * dptgetinfo(struct scsi_ad *sa, struct hbagetinfo *getinfop)
 *
 * Description:
 *	Return the name and iotype of the given device.  The name is copied
 *	into a string pointed to by the first field of the getinfo structure.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC void
HBAGETINFO(struct scsi_ad *sa, struct hbagetinfo *getinfop)
{
	register char  *s1, *s2;
	static char temp[] = "HA X TC X";
	struct dpt_scsi_ha *ha;

	s1 = temp;
	s2 = getinfop->name;
	temp[3] = SDI_HAN(sa) + '0';
	temp[8] = SDI_TCN(sa) + '0';

	while ((*s2++ = *s1++) != '\0')
		;

	ha = &dpt_sc_ha[ dpt_gtol[SDI_HAN(sa)] ];
	if (ha->ha_state & C_ISA) {
		/***
		 * A 2011, Restrict DMA to 16Mb
		***/
		getinfop->iotype = F_DMA | F_SCGTH;
	} else {
		/***
		** A 2012, UnRestricted DMA
		***/
		getinfop->iotype = F_DMA_32 | F_SCGTH;
	}
#if PDI_VERSION >= PDI_SVR42MP
	if (getinfop->bcbp) {
		getinfop->bcbp->bcb_addrtypes = BA_KVIRT;
		getinfop->bcbp->bcb_flags = 0;
		getinfop->bcbp->bcb_max_xfer = dpthba_info.max_xfer;
		getinfop->bcbp->bcb_physreqp->phys_align = DPT_MEMALIGN;
		getinfop->bcbp->bcb_physreqp->phys_boundary = DPT_BOUNDARY;
		getinfop->bcbp->bcb_physreqp->phys_dmasize = 24;
		if (getinfop->iotype == F_DMA_32)
			getinfop->bcbp->bcb_physreqp->phys_dmasize = 32;
	}
#endif
}


/*===========================================================================*/
/* SCSI Host Adapter Driver Utilities
/*===========================================================================*/


#if PDI_VERSION >= PDI_SVR42MP
/*
 * STATIC void
 * dpt_pass_thru0(buf_t *bp)
 *	Calls buf_breakup to make sure the job is correctly broken up/copied.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
dpt_pass_thru0(buf_t *bp)
{
	int	c = dpt_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	struct dpt_scsi_lu	*q = &LU_Q(c, t, l);

	if (!q->q_bcbp) {
		struct sb *sp;
		struct scsi_ad *sa;

		sp = bp->b_priv.un_ptr;
		sa = &sp->SCB.sc_dev;
		if ((q->q_bcbp = sdi_getbcb(sa, KM_SLEEP)) == NULL) {
			sp->SCB.sc_comp_code = SDI_ABORT;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		q->q_bcbp->bcb_granularity = 1;
	}

	buf_breakup(dpt_pass_thru, bp, q->q_bcbp);
}
#endif /* PDI_VERSION >= PDI_SVR42MP */

/*
 * STATIC void
 * dpt_pass_thru(buf_t *bp)
 *	Send a pass-thru job to the HA board.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
dpt_pass_thru(buf_t *bp)
{
	int	c = dpt_gtol[SC_HAN(bp->b_edev)];
	int	t = SC_TCN(bp->b_edev);
	int	l = SC_LUN(bp->b_edev);
	register struct dpt_scsi_lu	*q;
	register struct sb *sp;

#if (PDI_VERSION < PDI_SVR42MP)
	sp = (struct sb *)bp->b_private;
#else
	sp = bp->b_priv.un_ptr;
#endif
	sp->SCB.sc_wd = (long)bp;
	sp->SCB.sc_datapt = (caddr_t) paddr(bp);
	sp->SCB.sc_int = dpt_int;

	SDI_TRANSLATE(sp, bp->b_flags, bp->b_proc, KM_SLEEP);

	if (bp->b_flags & B_READ)
		sp->SCB.sc_mode = SCB_READ;
	else
		sp->SCB.sc_mode = SCB_WRITE;

	q = &LU_Q(c, t, l);

	DPT_SCSILU_LOCK(q->q_opri);
	dpt_putq(q, (dpt_sblk_t *)((struct xsb *)sp)->hbadata_p);
	dpt_next(c,q);
}


/*
 * void
 * dpt_int(struct sb *sp)
 *
 * Description:
 *	This is the interrupt handler for pass-thru jobs.  It just
 *	wakes up the sleeping process.
 *
 * Calling/Exit State:
 *	None.
 */

void
dpt_int(struct sb *sp)
{
	struct buf  *bp;

	bp = (struct buf *) sp->SCB.sc_wd;
	biodone(bp);
}



/*
 * void
 * dpt_flushq(struct dpt_scsi_lu *q, int cc, int flag)
 *
 * Description:
 *	Empty a logical unit queue.  If flag is set, remove all jobs.
 *	Otherwise, remove only non-control jobs.
 *
 * Calling/Exit State:
 *	None.
 */

void
dpt_flushq(struct dpt_scsi_lu *q, int cc, int flag)
{
	register dpt_sblk_t  *sp, *nsp;

	ASSERT(q);

	DPT_SCSILU_LOCK(q->q_opri);
	sp = q->q_first;
	q->q_first = q->q_last = NULL;
	q->q_count = 0;
        DPT_SCSILU_UNLOCK(q->q_opri);

	while (sp) {
		nsp = sp->s_next;
		if (!flag && (dpt_qclass(sp) > DPT_QNORM))
                  {
	                DPT_SCSILU_LOCK(q->q_opri);
			dpt_putq(q, sp);
	                DPT_SCSILU_UNLOCK(q->q_opri);
                  }
		else {
			sp->sbp->sb.SCB.sc_comp_code = (ulong)cc;
			sdi_callback(&sp->sbp->sb);
		}
		sp = nsp;
	}
}

/*
 * STATIC void
 * dpt_putq(struct dpt_scsi_lu *q, dpt_sblk_t *sp)
 *	Put a job on a logical unit queue in FIFO order.
 *
 * Calling/Exit State:
 *	DPT_SCSILU_LOCK(q->q_opri) for q
 */
STATIC void
dpt_putq(struct dpt_scsi_lu *q, dpt_sblk_t *sp)
{
	int cls;
	dpt_sblk_t *ip = q->q_last;		/* insert job after ip */

#ifdef DPT_DEBUG
	if(dptdebug > 0)
		cmn_err ( CE_CONT, "dpt_putq(%x, %x)\n", q, sp);
#endif
	q->q_count++;
	cls = dpt_qclass(sp);
	if (!ip) {
		sp->s_prev = sp->s_next = NULL;
		q->q_first = q->q_last = sp;
		return;
	}
	cls = dpt_qclass(sp);
	while(ip && dpt_qclass(ip) < cls)
		ip = ip->s_prev;

	if (!ip) {
		sp->s_next = q->q_first;
		sp->s_prev = NULL;
		q->q_first = q->q_first->s_prev = sp;
	} else if (!ip->s_next) {
		sp->s_next = NULL;
		sp->s_prev = q->q_last;
		q->q_last = ip->s_next = sp;
	} else {
		sp->s_next = ip->s_next;
		sp->s_prev = ip;
		ip->s_next = ip->s_next->s_prev = sp;
	}
}

/*
 * STATIC void
 * dpt_getq(struct dpt_scsi_lu *q, dpt_sblk_t *sp)
 *	remove a job from a logical unit queue.
 *
 * Calling/Exit State:
 *	DPT_SCSILU_LOCK(q->q_opri) for q
 */
STATIC void
dpt_getq(struct dpt_scsi_lu *q, dpt_sblk_t *sp)
{
	ASSERT(q);
	ASSERT(sp);
#ifdef DPT_DEBUG
	if(dptdebug > 0)
		cmn_err ( CE_CONT, "dpt_getq(%x, %x)\n", q, sp);
#endif
	if (sp->s_prev && sp->s_next) {
		sp->s_next->s_prev = sp->s_prev;
		sp->s_prev->s_next = sp->s_next;
	} else {
		if (!sp->s_prev) {
			q->q_first = sp->s_next;
			if (q->q_first)
				q->q_first->s_prev = NULL;
		} 
		if (!sp->s_next) {
			q->q_last = sp->s_prev;
			if (q->q_last)
				q->q_last->s_next = NULL;
		} 
	}
	q->q_count--;
}

/*
 * STATIC int
 * dpt_getadr(dpt_sblk_t *sp)
 *	Return the logical address of the disk request pointed to by
 *	by sp.  If there is no associated disk address associated with
 *	sp, return -1.
 *
 * Calling/Exit State:
 *	None
 */
STATIC int
dpt_getadr(dpt_sblk_t *sp)
{
	char *p;
	ASSERT(sp->sbp->sb.sb_type != SFB_TYPE);
	ASSERT(DPT_IS_RW(DPT_CMD(sp)));
        p = (char *)sp->sbp->sb.SCB.sc_cmdpt;
        switch(p[0]) {
        case SM_READ:
        case SM_WRITE:
                return dpt_swap32(*((long *)(void *)(&p[2])));
        case SS_READ:
        case SS_WRITE:
                return ((p[1]&0x1f) << 16) | (p[2] << 8) | p[3];
        }
	/* NOTREACHED */
}

/* STATIC int
 * dpt_serial(dpt_sblk_t *first, dpt_sblk_t *last)
 *	Return non-zero if the last job in the chain from
 *	first to last can be processed ahead of all the 
 *	other jobs on the chain.
 *
 * Calling/Exit State:
 *	DPT_SCSILU_LOCK(q->q_opri) for q held on entry
 *	DPT_SCSILU_LOCK(q->q_opri) for q held on exit
 */
STATIC int
dpt_serial(dpt_sblk_t *first, dpt_sblk_t *last)
{
	while (first != last) {
		unsigned int sb1 = dpt_getadr(last);
		unsigned int eb1 = sb1 + DPT_BSIZE(last) - 1;
		if (DPT_IS_WRITE(DPT_CMD(first)) || 
		    DPT_IS_WRITE(DPT_CMD(last))) {
			unsigned int sb2 = dpt_getadr(first);
			unsigned int eb2 = sb2 + DPT_BSIZE(first) - 1;
			if (sb1 <= sb2 && eb1 >= sb2)
				return 0;
			if (sb2 <= sb1 && eb2 >= sb1)
				return 0;
		}
		first = first->s_next;
	}
	return 1;
}
			
#define DPT_ABS_DIFF(x,y)	(x < y ? y - x : x - y)

/*
 * STATIC dpt_sblk_t *
 * dpt_schedule(struct dpt_scsi_lu *, int)
 *	Select the next job to process.  This routine assumes jobs
 *	are placed on the queue in sorted order.
 *
 * Calling/Exit State:
 *	DPT_SCSILU_LOCK(q->q_opri) for q held on entry
 *	DPT_SCSILU_LOCK(q->q_opri) for q held on exit
 */
STATIC dpt_sblk_t *
dpt_schedule(struct dpt_scsi_lu *q, int c)
{
        dpt_sblk_t *sp = q->q_first;
        dpt_sblk_t  *best = NULL;
        unsigned int	best_position, best_distance, distance, position;
        int class;
	int prefer_write = 0;
	int writeback = 0;
#ifdef DPT_DEBUG
        int count;
#endif
	if (!sp || dpt_qclass(sp) == SFB_TYPE || !(q->q_flag & DPT_QSCHED))
		return sp;
	class = dpt_qclass(sp);
	best_distance = (unsigned int)~0;
	
#if DPT_DEBUG
	if (dptdebug > 4)
		count = dpt_schedule_debug(sp, q->q_addr);
#endif
	if (dpt_sc_ha[c].ha_cache == DPT_CACHE_WRITEBACK) {
		writeback = 1;
		prefer_write = 1;
	}

        /* Implement shortest seek first */

        while (sp && dpt_qclass(sp) == class && DPT_IS_RW(DPT_CMD(sp))) {

		if (prefer_write && DPT_IS_WRITE(DPT_CMD(sp))) {
			if (dpt_serial(q->q_first, sp)) 
				return sp;
			/* Assume almost all jobs are serializable.  Since
			 * testing serializability can get expensive on
			 * long queues, if first write is not serializable, 
			 * just look for the nearest read.  
			 */
			prefer_write = 0;
			sp = sp->s_next;
			continue;
		}
		position = dpt_getadr(sp);
		distance = DPT_ABS_DIFF(q->q_addr, position);
		if (distance < best_distance) {
			best_position = position;
			best_distance = distance;
			best = sp;
		}
		sp = sp->s_next;
        }
#ifdef DPT_DEBUG
        if (dptdebug > 2 && count && best) 
		cmn_err(CE_CONT,"Selected position %d\n", best_position);
#endif
	if (best && dpt_serial(q->q_first, best)) {
		q->q_addr = best_position + DPT_BSIZE(best);
		return best;
	} 
	/* Should rarely if ever get here with a read/write on the queue */
	if (DPT_IS_READ(DPT_CMD(q->q_first)))
		q->q_addr = dpt_getadr(q->q_first) + DPT_BSIZE(q->q_first);
	else if (!writeback && DPT_IS_WRITE(DPT_CMD(q->q_first)))
		q->q_addr = dpt_getadr(q->q_first) + DPT_BSIZE(q->q_first);
	return q->q_first;
}
	
#ifdef DPT_DEBUG

/* Only print scheduler choices when there are more than DPT_QDBG jobs */
#define DPT_QDBG 2

/* 
 * STATIC int
 * dpt_schedule_debug(dpt_sblk_t *sp, int head)
 *	Debug the disk scheduler.  Return non-zero if number of jobs being
 *	considered is at least DPT_QDBG.
 *
 * Calling/Exit State:
 *	DPT_SCSILU_LOCK(q->q_opri) for q held on entry
 *	DPT_SCSILU_LOCK(q->q_opri) for q held on exit
 */
STATIC int
dpt_schedule_debug(dpt_sblk_t *sp, int head)
{
        dpt_sblk_t *tsp = sp;
        int count=0;
        while (tsp && dpt_qclass(tsp) == dpt_qclass(sp)) {
                count++;
                tsp = tsp->s_next;
        }
        if (count < DPT_QDBG)
                return 0;
        cmn_err(CE_CONT, "\n\nSchedule:\n");
        cmn_err(CE_CONT, "\thead position: %d\n", head);
        cmn_err(CE_CONT, "\tChoosing among %d choices\n",count);
        cmn_err(CE_CONT, "\tAddresses:");
        tsp = sp;
        while (tsp && dpt_qclass(tsp) == dpt_qclass(sp)) {
		if (tsp->sbp->sb.sb_type==SFB_TYPE ||!DPT_IS_RW(DPT_CMD(tsp)))
			cmn_err(CE_CONT, " COM");
		else
			cmn_err(CE_CONT, " %d", dpt_getadr(tsp));
                tsp = tsp->s_next;
        }
        cmn_err(CE_CONT, "\n");
        return count;
}
#endif /* DPT_DEBUG */

/*
 * void
 * dpt_next(int c, struct dpt_scsi_lu *q)
 *
 * Description:
 *	Attempt to send the next job on the logical unit queue.
 *	All jobs are not sent if the Q is busy.
 *
 * Calling/Exit State: DPT_SCSILU_LOCK(q->q_opri) for q on entry.
 * 	None on exit.
 */

void
dpt_next(int c, struct dpt_scsi_lu *q)
{
	dpt_sblk_t  *sp;
	struct dpt_scsi_ha *ha = &dpt_sc_ha[c];
	pl_t opri;

	ASSERT(q);

	sp = dpt_schedule(q, c);

	if (sp == NULL)			/*  queue empty  */
	{
	        DPT_SCSILU_UNLOCK(q->q_opri);
		return;
	}

        DPT_CCB_LOCK(opri);

	/* Conditions to send job:
	 * 1. Still room on controller
	 * 2. Target has not overrun its allotment of total jobs
	 * 3. Target has not overrun its allotment of scheduled jobs
	 */
	if (ha->ha_active_jobs >= dpt_hba_max 
	||   (int) q->q_active >= ha->ha_max_jobs) {
                DPT_CCB_UNLOCK(opri);
	        DPT_SCSILU_UNLOCK(q->q_opri);
		return;
	}
	if  (sp->sbp->sb.sb_type == SCB_TYPE 
	  && DPT_SCHED(DPT_CMD(sp), c) 
	  && (int) q->q_active_sched >= dpt_lu_max_sched) {
		DPT_CCB_UNLOCK(opri);
		DPT_SCSILU_UNLOCK(q->q_opri);
		return;
	}
        DPT_CCB_UNLOCK(opri);

	if (sp->sbp->sb.sb_type == SCB_TYPE)
	{
		if (q->q_flag & DPT_QSUSP) {	/*  Q suspended  */
			/*
			 *+ DPT device queue being suspended.
			 */
			cmn_err(CE_WARN,"!DPT_NEXT: DPT_QSUSP Set");
			DPT_SCSILU_UNLOCK(q->q_opri);
			return;
		}
		if (DPT_SCHED(DPT_CMD(sp), c))
			q->q_active_sched++;
	}

	dpt_getq(q, sp);
	q->q_active++;
        DPT_SCSILU_UNLOCK(q->q_opri);

	if(dpt_polltime)		/* need to poll */
	{
		if (sp->sbp->sb.sb_type == SFB_TYPE)
			dpt_func(sp);
		else
			dpt_cmd(sp);

		/***
		** Wait up to 5 seconds for the command to complete.
		** I know this is a __LONG__ time, but we've seen
		** 2011 with Rev 3C firmware take over 1 second to
		** show completion of SS_INQUIR on some devices.
		***/
		if(dpt_wait(dpt_gtol[SC_HAN(ad2dev_t(sp->sbp->sb.SCB.sc_dev))], 5000, DPT_INTR_ON) == FAILURE) {
			sp->sbp->sb.SCB.sc_comp_code = SDI_TIME;
			sdi_callback(&sp->sbp->sb);
		}
	} else {
		if (sp->sbp->sb.sb_type == SFB_TYPE)
			dpt_func(sp);
		else
			dpt_cmd(sp);
	}
}


/*
 * void
 * dpt_cmd(dpt_sblk_t *sp)
 *
 * Description:
 *	Create and send an SCB associated command.
 *
 * Calling/Exit State:
 *	None.
 */

void
dpt_cmd(dpt_sblk_t *sp)
{
	register struct scsi_ad *sa;
	register struct dpt_ccb *cp;
	register struct dpt_scsi_lu	*q;
	register int  i;
	register char *p;
	unsigned long cnt;
	int  c, t;

	sa = &sp->sbp->sb.SCB.sc_dev;
	c = dpt_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	cp = dpt_getccb(c);
	cp->c_bind = &sp->sbp->sb;
	cp->c_time = (sp->sbp->sb.SCB.sc_time*HZ) / 1000;

	/* Build the EATA Command Packet structure */
	cp->CP_OpCode        = CP_DMA_CMD;
	cp->CPop.byte        = HA_AUTO_REQ_SEN;
	cp->CPID             = (BYTE)(t);
	cp->CPmsg0           = (HA_IDENTIFY_MSG | HA_DISCO_RECO) + sa->sa_lun;

	if (sp->s_CPopCtrl) {
		cp->CPop.byte |= sp->s_CPopCtrl;
		sp->s_CPopCtrl = 0;
	}
	if (sp->s_dmap)  {              /* scatter/gather is used      */
#ifdef DPT_DEBUG
		cmn_err(CE_CONT, "dpt_cmd: scatter/gather is being used.\n");
#endif
		cp->CPop.bit.Scatter = 1;
		cnt = sp->s_dmap->SG_size;
	} else {                             /* block mode                  */
#ifdef DPT_DEBUG
		cmn_err(CE_CONT, "dpt_cmd: block mode is being used.\n");
#endif
		cnt = sp->sbp->sb.SCB.sc_datasz;
	}
	if (cnt) {
		if (sp->sbp->sb.SCB.sc_mode & SCB_READ) {
#ifdef DPT_DEBUG
			cmn_err(CE_CONT, "dpt_cmd: doing a READ.\n");
#endif
			cp->CPop.bit.DataIn  = 1;
		} else {
#ifdef DPT_DEBUG
			cmn_err(CE_CONT, "dpt_cmd: doing a WRITE.\n");
#endif
			cp->CPop.bit.DataOut = 1;
		}
		*(unsigned long *)(void *)cp->CPdataLen = dpt_swap32(cnt);
	} else
		*(unsigned long *)(void *)cp->CPdataLen = 0;

	cp->CPdataDMA = dpt_swap32(sp->s_addr);

	q = &LU_Q(c, t, sa->sa_lun);         /* Get SCSI Dev/Lun Pointer.   */

	/*********************************************************************
	** If a Request Sense command and ReqSen Data cached then copy to   **
	**   data buffer and return.                                        **
	**********************************************************************/
	p = sp->sbp->sb.SCB.sc_cmdpt;        /* Get Command CDB Pointer.    */

	DPT_SCSILU_LOCK(q->q_opri);
	if ( (q->q_flag & DPT_QSENSE) && (*p == SS_REQSEN)) {
		int	copyfail;
		caddr_t	toaddr;

		copyfail = 0;

		q->q_flag &= ~DPT_QSENSE;
	        DPT_SCSILU_UNLOCK(q->q_opri);
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
		if (! cp->CPop.bit.Scatter) {
#ifdef DPT_DEBUG
			cmn_err(CE_CONT,
                        "dpt_cmd: one big chunk to kernel memory.\n");
#endif
			if (toaddr = physmap(sp->s_addr, cnt, KM_NOSLEEP)) {
 				bcopy((caddr_t)(&q->q_sense)+1, toaddr, cnt);
#ifdef DPT_DEBUG
				cmn_err ( CE_CONT, "BCOPY DONE>\n");
#endif
				physmap_free(toaddr, cnt, 0);
			} else
				copyfail++ ;
		} else {
		/* copy req sns data to places defined by scat/gath list */
			SG_vect	*VectPtr;
			int	VectCnt;
			caddr_t	Src;
			caddr_t	optr, Dest;
			int	Count;

#ifdef DPT_DEBUG
	cmn_err(CE_CONT, "dpt_cmd: scatter/gather results being returned.\n");
#endif
			if (VectPtr = (SG_vect *)(void *)physmap(sp->s_addr, cnt, KM_NOSLEEP)) {
				optr = (caddr_t)(void *)VectPtr;
 				VectCnt = cnt / sizeof(*VectPtr);
 				Src = (caddr_t)(&q->q_sense) + 1;
 				for (i=0; i < VectCnt; i++) {
					Dest = (caddr_t) dpt_swap32(VectPtr->Phy.l);
					Count = (int) dpt_swap32(VectPtr->Len.l);

					if ( Dest = (caddr_t)physmap((unsigned long) Dest, 
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
		if(!copyfail) {
			i = HA_ST_SEEK_COMP | HA_ST_READY;
			cp->CP_Controller_Status = S_GOOD;
		} else {
			i = HA_ST_ERROR;
			cp->CP_Controller_Status = HA_ERR_SELTO;
		}
		dpt_done(c, cp, i);
		return;
	} else
	       DPT_SCSILU_UNLOCK(q->q_opri);
        
#ifdef DPT_DEBUG
	cmn_err(CE_CONT, "dpt_cmd: not request sense, send command to HBA.\n");
#endif

	for (i=0; i < sp->sbp->sb.SCB.sc_cmdsz; i++)
		cp->CPcdb[i] = *p++;        /* Copy SCB cdb to CP cdb.     */
	dpt_send(c, START, cp);             /* Send command to the HA.     */
}


/*
 * void
 * dpt_func(dpt_sblk_t *sp)
 *
 * Description:
 *	Create and send an SFB associated command.
 *
 * Calling/Exit State:
 *	None.
 */

void
dpt_func(dpt_sblk_t *sp)
{
	register struct scsi_ad *sa;
	register struct dpt_ccb *cp;
	int  c, t;
	struct sdi_edt *edtp;
        pl_t opri;
        struct dpt_scsi_ha *ha;

	/* Only SFB_ABORTM and SFB_RESETM messages get here.                */

#ifdef DPT_DEBUG
	if(sp->sbp->sb.SFB.sf_func!=SFB_ABORTM && sp->sbp->sb.SFB.sf_func!=SFB_RESETM)
	{
		/*
		 *+ DPT SFB command unknown. Command is not sent.
		 */
		cmn_err(CE_WARN, "DPT Host Adapter: Unsupported SFB command: %X\n",sp->sbp->sb.SFB.sf_func);
		return;
	}
#endif

	sa = &sp->sbp->sb.SFB.sf_dev;
	c = dpt_gtol[SDI_HAN(sa)];
	t = SDI_TCN(sa);

	cp = dpt_getccb(c);
	cp->c_bind = &sp->sbp->sb;
	cp->CPID   = (BYTE)t;
	cp->CPmsg0 = sa->sa_lun;
	cp->c_time = 0;

	if( (edtp = sdi_redt(c, cp->CPID, cp->CPmsg0)) != (struct sdi_edt *)0 &&
	    edtp->pdtype != ID_TAPE ) {

		/*
		 *+ DPT SFB_ABORTM/SFB_RESETM command received,
		 *+ SCSI bus reset command being issued.
		 */
		cmn_err(CE_WARN, "!DPT Host Adapter: HA %d - Bus is being reset\n", dpt_ltog[c]);
	        ha = &dpt_sc_ha[c];
                DPT_STPKT_LOCK(opri,ha);
		outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(1000);
                DPT_STPKT_UNLOCK(opri,ha);
	}

	cp->CP_Controller_Status = 0;
	cp->CP_SCSI_Status       = 0;
	dpt_ha_done(c, cp, 0);
}



/*
 * void
 * dpt_send(int c, int cmd, struct dpt_ccb *cp)
 *
 * Description:
 *      Send a command to the host adapter board.
 *
 * Calling/Exit State:
 *	None.
 */

void
dpt_send(int c, int cmd, struct dpt_ccb *cp)
			/* HA controller number  */
			/* HA control command    */
			/* command block pointer */
{
        pl_t opri;
	register struct dpt_scsi_ha *ha = &dpt_sc_ha[c];

        DPT_STPKT_LOCK(opri,ha);
	if (cmd == START) {
		cp->c_active = TRUE;	/* Set Command Active flag.         */
		drv_getparm(LBOLT, (ulong *)&cp->c_start); /* Set start time */
		ha->ha_npend++;		/* Increment number pending on ctlr */
	} else if (cmd == ABORT) {
		outb((ha->ha_base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(1000);
                DPT_STPKT_UNLOCK(opri,ha);
		return;
	}

	/* UPDATE: this call resolved in dpt_a.s -- remove from here*/

	scsi_send_cmd(ha->ha_base, cp->c_addr, CP_DMA_CMD);
        DPT_STPKT_UNLOCK(opri,ha);

	/********** This does the same thing as the above call **************
		CPaddr = (char *)&cp->c_addr;
		while (inb(ha->ha_base + HA_AUX_STATUS) & HA_AUX_BUSY )  ;
	
		outb((ha->ha_base + HA_DMA_BASE + 3), *CPaddr++);
		outb((ha->ha_base + HA_DMA_BASE + 2), *CPaddr++);
		outb((ha->ha_base + HA_DMA_BASE + 1), *CPaddr++);
		outb((ha->ha_base + HA_DMA_BASE + 0), *CPaddr);
		outb((ha->ha_base + HA_COMMAND), CP_DMA_CMD);
	*********************************************************************/
}

/*
 * struct dpt_ccb *
 * dpt_getccb(int c)
 *
 * Description:
 *      Allocate a controller command block structure.
 *
 * Calling/Exit State:
 *	None.
 */

struct dpt_ccb *
dpt_getccb(int c)
{
	register struct dpt_scsi_ha	*ha = &dpt_sc_ha[c];
	register struct dpt_ccb	*cp;
        pl_t opri;

        DPT_CCB_LOCK(opri);
	if (ha->ha_cblist) {
		cp = ha->ha_cblist;
		ha->ha_cblist = cp->c_next;
	        ++ha->ha_active_jobs;	
                DPT_CCB_UNLOCK(opri);
		return (cp);
	}
	/*
	 *+ DPT Unexpected condition - no more command blocks.
	 */
	cmn_err(CE_PANIC, "DPT Host Adapter: Out of command blocks");
        DPT_CCB_UNLOCK(opri);
	return(NULL); /* make lint happy */
}


/*
 * void
 * dpt_freeccb(int c, struct dpt_ccb *cp)
 *
 * Description:
 *      Release a previously allocated command block.
 *
 * Calling/Exit State:
 *	None.
 */

void
dpt_freeccb(int c, struct dpt_ccb *cp)
{
	register struct dpt_scsi_ha	*ha = &dpt_sc_ha[c];
	pl_t opri;

        DPT_CCB_LOCK(opri);
	cp = &ha->ha_ccb[cp->c_index];
	cp->c_bind = NULL;
	cp->c_next = ha->ha_cblist;
	ha->ha_cblist = cp;
	--ha->ha_active_jobs;	/* One less job on the board */
        DPT_CCB_UNLOCK(opri);
}


/*
 * int
 * dpt_dmalist(dpt_sblk_t *sp, struct proc *procp, int sleepflag)
 *
 * Description:
 *	Build the physical address(es) for DMA to/from the data buffer.
 *	If the data buffer is contiguous in physical memory, only 1 base
 *	address is provided for a regular SB.  If not, a scatter/gather
 *	list is built, and the SB will point to that list instead.
 *
 * Calling/Exit State:
 *	None.
 */
int
dpt_dmalist(dpt_sblk_t *sp, struct proc *procp, int sleepflag)
{
	SG_vect	tmp_list[MAX_DMASZ];
	register SG_vect  *pp;
	register dpt_dma_t  *dmap;
	register long   count, fraglen, thispage;
	caddr_t		vaddr;
	paddr_t		addr, base;
	int		i;
        pl_t            opri;
#ifdef DPT_DEBUG
	cmn_err(CE_CONT, "dpt_dmalist: on entry\n");
#endif
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
		pp->Phy.l = dpt_swap32(base);
		pp->Len.l = dpt_swap32(fraglen);
	}
	if (count != 0)
		/*
		 *+ DPT Job received too big for DMA list. 
		 */
		cmn_err(CE_PANIC, "DPT Host Adapter: Job too big for DMA list");

	if (i == 1)
		/*
		 * The data buffer was contiguous in physical memory.
		 * There is no need for a scatter/gather list.
		 */
		sp->s_addr = (paddr_t) (dpt_swap32(tmp_list[0].Phy.l));
	else {
		/*
		 * We need a scatter/gather list.
		 * Allocate one and copy the list we built into it.
		 */
#ifdef DPT_DEBUG
	cmn_err(CE_CONT, "dpt_dmalist: building a scatter/gather list.\n");
#endif
		DPT_DMALIST_LOCK(opri);
		if (!dpt_dfreelist && (sleepflag == KM_NOSLEEP)) {
			DPT_DMALIST_UNLOCK(opri);
			return (1);
		}
		while (!(dmap = dpt_dfreelist)) {
			SV_WAIT(dpt_dmalist_sv, PRIBIO, dpt_dmalist_lock);
		        DPT_DMALIST_LOCK(opri);
		}
		dpt_dfreelist = dmap->d_next;
		DPT_DMALIST_UNLOCK(opri);

		sp->s_dmap = dmap;
		sp->s_addr = vtop((caddr_t) dmap->d_list, procp);
		dmap->SG_size = i * sizeof(SG_vect);
		bcopy((caddr_t) &tmp_list[0],
		    (caddr_t) dmap->d_list, dmap->SG_size);
	}
#ifdef DPT_DEBUG
	cmn_err(CE_CONT, "dpt_dmalist: on exit\n");
#endif
	return (0);
}


/*
 * void
 * dpt_dma_freelist(dpt_dma_t *dmap)
 *
 * Description:
 *	Release a previously allocated scatter/gather DMA list.
 *
 * Calling/Exit State:
 *	None.
 */
void
dpt_dma_freelist(dpt_dma_t *dmap)
{
	register pl_t opri;

	ASSERT(dmap);

	DPT_DMALIST_LOCK(opri);
	dmap->d_next = dpt_dfreelist;
	dpt_dfreelist = dmap;
	if (dmap->d_next == NULL) {
		DPT_DMALIST_UNLOCK(opri);
		SV_BROADCAST(dpt_dmalist_sv, 0);
	} else
		DPT_DMALIST_UNLOCK(opri);
}

/*
 * int
 * dpt_wait(int c, int time, intr)
 *
 * Description:
 *	Poll for a completion from the host adapter.  If an interrupt
 *	is seen, the HA's interrupt service routine is manually called.
 *	The intr arguments specifies whether interrupts are inabled
 *	or disabled when this routine is called:
 *		intr == 0	Interrupts disabled
 *			1	Interrupts enabled
 *  NOTE:
 *	This routine allows for no concurrency and as such, should
 *	be used selectively.
 *
 * Calling/Exit State:
 *	None.
 */
int
dpt_wait(int c, int time, int intr)
{
	struct dpt_scsi_ha  *ha = &dpt_sc_ha[c];
	int act;
	int ret = FAILURE;

	act = dptidata[c].active;
        dptidata[c].active = 1;
	
	while (time > 0) {
		if (intr == DPT_INTR_OFF) {
			if ((dpt_intr_wait == FALSE ) &&
			    (dpt_waitflag == FALSE)) {
				/***
				** Controller has generated an interrupt to
				** acknowledge completion of command, and
				** dptintr() has servicved the interrupt.
				***/
				ret = SUCCESS;
				break;
			}
			if(inb(ha->ha_base + HA_AUX_STATUS) & HA_AUX_INTR) {
				dptintr(ha->ha_vect);
				ret = SUCCESS;
				break;
			}
		} else {
			if (dpt_waitflag == FALSE) {
				ret = SUCCESS;
				break;
			}
		}
		drv_usecwait(1000);
		time--;
	}
	dpt_waitflag = TRUE;
        dptidata[c].active = act;
	if (ret == FAILURE) {
		/*
	 	*+ DPT command timed out.
		*+ Command we are waiting for has not completed.
	 	*/
		cmn_err(CE_WARN, "!%s: Command completion not indicated, dpt_wait()\n",dptidata[c].name);
	}
	return (ret);	
}


/*
 * void
 * dpt_init_cps(int c)
 *
 * Description:
 *      Initialize the controller CP free list.
 *
 * Calling/Exit State:
 *	None.
 */
void
dpt_init_cps(int c)
{
	register struct dpt_scsi_ha  *ha = &dpt_sc_ha[c];
	register struct dpt_ccb  *cp;
	register int    i;

	ha->ha_cblist = NULL;

	for (i = 0; i < dpt_hba_max; i++)
	{
		cp = &ha->ha_ccb[i];
		cp->c_index   = (unsigned short) i;

		/** Save data addresses into CP in  68000 format ***/
		cp->c_addr    = vtop((caddr_t)cp, NULL);
		cp->c_addr    = dpt_swap32(cp->c_addr);
		cp->CPstatDMA = vtop((caddr_t)&dpt_sc_hastat[c], NULL);
		cp->CPstatDMA = dpt_swap32(cp->CPstatDMA);
		cp->CP_ReqDMA = vtop((caddr_t)cp->sense, NULL);
		cp->CP_ReqDMA = dpt_swap32(cp->CP_ReqDMA);

		/** Save Command Packet virtual address pointer ***/
		cp->CPaddr.vp = cp;
		cp->ReqLen    = sizeof(struct sense);
		cp->c_next    = ha->ha_cblist;
		ha->ha_cblist = cp;
	}
}


/*
 * int
 * dpt_ha_init(c)
 *
 * Description:
 *      Reset the HA board and initialize the communications
 *	for ISA boards.
 *
 * Calling/Exit State:
 *	None.
 */

int
dpt_ha_init(int c)  /* HA controller number */
{
	register struct dpt_scsi_ha  *ha = &dpt_sc_ha[c];
	int i;

	dpt_board_reset(ha->ha_base);

	/* Issue an EATA Read Config command to the controller, if necessary*/
	/*  make any systems configs ... like allocate a DMA Channel.	    */

	if (EATA_ReadConfig(ha->ha_base)) {

#ifdef DPT_DEBUG
		cmn_err(CE_CONT, "dpt_ha_init: EATA_ReadConfig succeeded.\n");
#endif

		if (eata_cfg->DMAChannelValid) {
			/* the card is a 2011 */
#ifdef DPT_DEBUG
			cmn_err(CE_CONT, "DPT 2011 Host Adapter identified.\n");
#endif
			ha->ha_state |= C_ISA;
#if PDI_VERSION >= PDI_SVR42MP
			if (dma_cascade(8 - eata_cfg->DMA_Channel,
					DMA_ENABLE  | DMA_NOSLEEP) == B_FALSE) {
				/*
				 *+ The DMA channel in question is already in use
				 *+ and is not in cascade mode.
				 */
                                cmn_err(CE_WARN,
					"DPT Host Adapter: DMA channel %d is not available",
					8 - eata_cfg->DMA_Channel);
				return (1);
			}
#else /* PDI_VERSION < PDI_SVR42MP */
			dptH_DMA_Setup(eata_cfg->DMA_Channel);
#endif /* PDI_VERSION < PDI_SVR42MP */
			dpt_eisa_optimize = 0;
		}
		else if (eata_cfg->DMAsupported) {
			/* the card is a 2012 , but it was not matched */
			/* to a board in the slot detection, so the	*/
			/* idata may be misconfigured	*/
			struct dpt_cfg *cfg = dpt_cfg;
#ifdef DPT_DEBUG
			cmn_err(CE_CONT, "dpt_ha_init: We got a 2012.\n");
#endif
			for (i=0; i<dpt_ncfg; i++, cfg++) {
				if (cfg->cf_idata == -1) {
					/*
					 *+ DPT EISA board was configured in
					 *+ the System file, which was not
					 *+ matched with the boards actually
					 *+ found.  Probably the System file
					 *+ contains an incorrect IRQ for the
					 *+ board.
					 */
					cmn_err(CE_NOTE, "DPT board at I/O addr 0x%x, IRQ %d, \n",
					cfg->cf_eaddr, cfg->cf_irq);
					cmn_err(CE_CONT, "does not match DPT configuration I/O addr 0x%x, IRQ %d\n",
					ha->ha_base, dptidata[c].iov);
				}
			}
#ifdef DPT_DEBUG
			cmn_err(CE_CONT, "DPT Host Adapter found at address 0x%X, but improperly configured\n", ha->ha_base);
#endif
			return( 1 );
		}

		if (inb(ha->ha_base + HA_STATUS) == (HA_ST_SEEK_COMP | HA_ST_READY) &&
		    ha->ha_vect == eata_cfg->IRQ_Number) {
#ifdef DPT_DEBUG
			cmn_err(CE_CONT, "dpt_ha_init: Marking Host Adapter as operational\n");
#endif
			/*
			 *+ DPT Host Adaptor found at given address.
			 */
			cmn_err(CE_NOTE, "DPT Host Adapter found at address 0x%X\n", ha->ha_base);
			ha->ha_state |= C_SANITY; /* Mark HA operational */
			return( 0 );
		}
		else {
			if (ha->ha_vect != eata_cfg->IRQ_Number) {
				cmn_err(CE_CONT, "DPT Host Adapter NOT configured at correct IRQ, board setting is %d, configuration is %d\n",
				eata_cfg->IRQ_Number, ha->ha_vect);
			}
#ifdef DPT_DEBUG
			cmn_err(CE_CONT, "DPT Host Adapter NOT found at address 0x%X\n", ha->ha_base);
#endif
			return( 1 );
		}
	}
	else {
#ifdef DPT_DEBUG
		cmn_err(CE_CONT, "dpt_ha_init: EATA_ReadConfig failed.\n");
#endif
		return( 1 );
	}
}

/*
 * void
 * dpt_board_reset(int base)
 *
 * Description:
 *      Reset the HA board.
 *
 * Calling/Exit State:
 *	None.
 */
void
dpt_board_reset(int base)
{
	if (inb(base + HA_STATUS) != (HA_ST_SEEK_COMP | HA_ST_READY)) {
		outb((base + HA_COMMAND), CP_EATA_RESET);
		drv_usecwait(4000000);  /* 4 full second wait */
	}
}

/*
 * void
 * dpt_eisa_slotids(int nslots)
 *
 * Description:
 *      Read the board ids from the EISA slot and
 *
 * Calling/Exit State:
 *	None.
 */
void
dpt_eisa_slotids(int nslots)
{
	int i, j, slot_id_addr, slot_io_addr, found;
	unsigned char idbyte1, idbyte2;
	struct dpt_cfg *cfg = dpt_cfg;

	for (i = 1; i <= nslots; i++) {
		/* DPT EISA base | slot */
		slot_id_addr = SLOT_ID_ADDR(i); 
		idbyte1 = (char)inb(slot_id_addr);
		idbyte2 = (char)inb(slot_id_addr + 1);
		/* ADD the 3rd byte check when value known
		idbyte3 = (char)inb(slot_id_addr + 2);
		   ADD */

		for (j=0, found=0; j< DPT_NVARIETIES; j++) {
			if((dpt_eisa_ids[j].id_byte1 == idbyte1) &&
			   (dpt_eisa_ids[j].id_byte2 == idbyte2)) {
				/* ADD the 3rd byte check when value known
				if ((idbyte1 != DPT_EISA_ID1) && 
				   (dpt_eisa_ids[j].id_byte3 != idbyte3 )) {
					break;
				}
				ADD */
				found = 1;
				break;
			}
		}
		if (found) {
			slot_io_addr = SLOT_BASE_IO_ADDR(i);
			dpt_board_reset(slot_io_addr);
			if (EATA_ReadConfig(slot_io_addr)) {
				cfg->cf_eaddr = (unsigned short)slot_io_addr;
				cfg->cf_irq = eata_cfg->IRQ_Number;
				cfg->cf_itype = eata_cfg->IRQ_Trigger;
				cfg->cf_daddr = eata_cfg->Secondary ? 0x170 : 0x1F0;
				cfg->cf_bdtype = eata_cfg->DMAChannelValid ?
					C_ISA : C_EISA;
				cfg->cf_idata = (short)-1;
				{ char *bdtype = (cfg->cf_bdtype == C_ISA) ?
						"ISA":"EISA";
				  char *itype = cfg->cf_itype ? 
						"Level":"Edge";
				  cmn_err(CE_CONT, "!DPT %s board at 0x%x (0x%x), IRQ %d, %s\n",
				    bdtype, cfg->cf_eaddr, cfg->cf_daddr, 
				    cfg->cf_irq, itype);
				}
				dpt_ncfg++;
				cfg++;
			} else {
				cmn_err(CE_WARN, "!dpt_eisa_slotids: EATA_ReadConfig failed, slot %d\n", i);
			}
		}
	}
	if (dpt_ncfg != dpt_cntls) {
		cmn_err(CE_CONT, "!dpt_eisa_slotids: %d DPT EISA boards found\n",
			dpt_ncfg);
		cmn_err(CE_CONT, "!dpt_eisa_slotids: %d DPT ISA/EISA boards configured\n",
			dpt_cntls);
	}
	
}

/*
 * void
 * dpt_set_config(void)
 *
 * Description:
 *      Match EISA board configuration with idata configuration,
 *	and set new base address.
 *
 * Calling/Exit State:
 *	None.
 */
void
dpt_set_config(void) 
{
	int i, j, nj;
	struct dpt_cfg *cfg = dpt_cfg;
	struct dpt_scsi_ha  *ha = dpt_sc_ha;

	for (i=0; i<dpt_ncfg; i++,cfg++) {
		for (nj= -1,j=0; j<dpt_cntls; j++, ha++) {
			if (dptidata[j].active)
				continue;
			if (cfg->cf_irq != dptidata[j].iov)
				continue;
			if (nj == -1) {
				nj = j;
				continue;
			}
			if (dptidata[nj].ioaddr1 > dptidata[j].ioaddr1) {
				nj = j;
			}
		}
		if (nj != -1) {
			if (inb(cfg->cf_eaddr + HA_STATUS) == 
			   (HA_ST_SEEK_COMP | HA_ST_READY)) {
				cmn_err(CE_CONT, "!Setting DPT %d to base 0x%x (was 0x%x), IRQ %d\n",
					nj, cfg->cf_eaddr, 
					dptidata[nj].ioaddr1, cfg->cf_irq);
				ha->ha_base = cfg->cf_eaddr;
				ha->ha_state |= (C_EISA | C_SANITY);
				dptidata[nj].ioaddr1 = cfg->cf_eaddr;
				dptidata[nj].active++;
				cfg->cf_idata = (short)nj;
#ifdef DPT_DEBUG
				cmn_err(CE_CONT, "dpt_set_config: Marking Host Adapter as operational\n");
#endif
				/*
				 *+ DPT Host Adaptor found at given address.
				 */
				cmn_err(CE_NOTE, "DPT Host Adapter found at address 0x%X\n", ha->ha_base);
			}
		} else
			cmn_err(CE_CONT, "!dpt_set_config: NO DPT configured for base 0x%x, IRQ %d\n", cfg->cf_eaddr, cfg->cf_irq); 
	}
}

/*
 * int
 * EATA_ReadConfig(int port)
 *
 * Issue an EATA Read Config Command, Process PIO.
 *
 * Calling/Exit State:
 *	None.
 */
int
EATA_ReadConfig(int port)
{
	register int status;
	ulong	 loop = 50000L;

	/* Wait for controller not busy */
	status = inb(port + HA_STATUS) & HA_ST_BUSY;
	while ( status == HA_ST_BUSY && loop--) {
		drv_usecwait(1);
		status = inb(port + HA_STATUS) & HA_ST_BUSY;
	}

	if ( status == HA_ST_BUSY ) {
#ifdef DPT_DEBUG
		cmn_err(CE_CONT,"EATA_ReadConfig: controller BUSY, timed out, status = 0x%x, returning ...\n",status);
#endif
		return(0);
	}

	/* Send the Read Config EATA PIO Command */
	outb(port + HA_COMMAND, CP_READ_CFG_PIO);

	/* Wait for DRQ Interrupt       */
	loop   = 50000L;
	while (((status = inb(port + HA_STATUS)) != ( HA_ST_DRQ + HA_ST_SEEK_COMP + HA_ST_READY)) && loop--) {
		drv_usecwait(1);
	}

	if( loop == 0 ) {
		/****
		** Timed Out Waiting For DRQ
		****/
#ifdef DPT_DEBUG
		cmn_err(CE_CONT,"EATA_ReadConfig: controller timed out waiting for DRQ\n");
#endif
		return(0);
	}

	/* Take the Config Data         */
	repinsw(port+HA_DATA, (ushort_t *)(void *)eata_cfg, 512 / 2 );

	if ( (status = inb(port + HA_STATUS) ) & HA_ST_ERROR ) {
#ifdef DPT_DEBUG
		cmn_err(CE_CONT,"EATA_ReadConfig: controller HA_ST_ERROR, status = 0x%X.\n",status);
#endif
		return(0);
	}

	/* Verify that it is an EATA Controller		*/
	if (eata_cfg->EATAsignature[0] != 'E' ||
	    eata_cfg->EATAsignature[1] != 'A' ||
	    eata_cfg->EATAsignature[2] != 'T' ||
	    eata_cfg->EATAsignature[3] != 'A' ) {
#ifdef DPT_DEBUG
		cmn_err(CE_CONT,"EATA_ReadConfig: signature wrong.\n");
#endif
		return(0);
	}

	return(1);
}

/*
 * int
 * dpt_illegal(minor_t hba, int scsi_id, int lun, int m)
 *
 * Description:
 *
 * Calling/Exit State:
 * 	None.
 */
/*ARGSUSED*/
int
dpt_illegal(minor_t hba, int scsi_id, int lun, int m)
{
	if (sdi_redt((int)hba, scsi_id, lun)) {
		return 0;
	} else {
		return 1;
	}
}

#if PDI_VERSION >= PDI_SVR42MP
/*
 * STATIC void
 * dpt_lockinit(int c)
 *
 * Description:  Initialize dpt locks:
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
dpt_lockinit(int c)
{
	register struct dpt_scsi_ha  *ha;
	register struct dpt_scsi_lu *q;
	register int	t,l;
	int		sleepflag;
	static		firsttime = 1;

#ifdef DPT_DEBUG
	if(dptdebug > 0)
		cmn_err ( CE_CONT, "dpt_lockinit()\n");
#endif
	sleepflag = mod_dynamic ? KM_SLEEP : KM_NOSLEEP;

	if (firsttime) {

		dpt_dmalist_sv = SV_ALLOC (sleepflag);
		dpt_dmalist_lock = 
                    LOCK_ALLOC(DPT_HIER, pldisk, &dpt_lkinfo_dmalist, sleepflag);
		dpt_ccb_lock = 
                    LOCK_ALLOC(DPT_HIER+1, pldisk, &dpt_lkinfo_ccb, sleepflag);
		firsttime = 0;
	}

	ha = &dpt_sc_ha[c];
	ha->ha_StPkt_lock = 
            LOCK_ALLOC(DPT_HIER, pldisk, &dpt_lkinfo_StPkt, sleepflag);
	for (t = 0; t < MAX_TCS; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			q = &LU_Q (c,t,l);
			q->q_lock = 
                           LOCK_ALLOC(DPT_HIER, pldisk, &dpt_lkinfo_q, sleepflag);
		}
	}
}

/*
 * STATIC void
 * dpt_lockclean(int c)
 *
 *	Removes unneeded locks.  Controllers that are not active will
 *	have all locks removed.  Active controllers will have locks for
 *	all non-existant devices removed.
 *
 * Calling/Exit State:
 *	None.
 */

STATIC void
dpt_lockclean(int c)
{
	register struct dpt_scsi_ha  *ha;
	register struct dpt_scsi_lu *q;
	register int	t,l;
	static		firsttime = 1;

#ifdef DPT_DEBUG
	if(dptdebug > 0)
		cmn_err ( CE_CONT, "dpt_lockclean(%d)\n", c);
#endif
	if (firsttime && !dpt_hacnt) {

		if (dpt_dmalist_sv == NULL)
			return;
		SV_DEALLOC (dpt_dmalist_sv);
		LOCK_DEALLOC (dpt_dmalist_lock);
		LOCK_DEALLOC (dpt_ccb_lock);
		firsttime = 0;
	}

	if( !dptidata[c].active) {
		ha = &dpt_sc_ha[c];
		if (ha->ha_StPkt_lock == NULL)
			return;
		LOCK_DEALLOC (ha->ha_StPkt_lock);
	}

	for (t = 0; t < MAX_TCS; t++) {
		for (l = 0; l < MAX_LUS; l++) {
			if (!dptidata[c].active || dpt_illegal(dpt_ltog[c], t, l,0)) {
				q = &LU_Q (c,t,l);
				LOCK_DEALLOC (q->q_lock);
			}
		}
	}
}
#else /* PDI_VERSION < PDI_SVR42MP */

/*
 * STATIC void
 * dpt_lockinit(int) 
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
dpt_lockinit(int c)
{
}

/*
 * STATIC void
 * dpt_lockclean(int) 
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
dpt_lockclean(int c)
{
}
#endif /* PDI_VERSION < PDI_SVR42MP */

#if PDI_VERSION >= PDI_SVR42MP
/*
 * STATIC void *
 * dpt_kmem_zalloc_physcontig (size_t size, int flags)
 *		
 * function to be used in place of kma_zalloc
 * which allocates contiguous memory, using kmem_alloc_physcontig,
 * and zero's the memory.
 *
 * Entry/Exit Locks: None.
 */
STATIC void *
dpt_kmem_zalloc_physcontig (size_t size, int flags)
{
	void *mem;
	physreq_t *preq;

	preq = physreq_alloc(flags);
	if (preq == NULL)
		return NULL;
	preq->phys_align = DPT_MEMALIGN;
	preq->phys_boundary = DPT_BOUNDARY;
	preq->phys_dmasize = 24;
	if (!physreq_prep(preq, flags)) {
		physreq_free(preq);
		return NULL;
	}
	mem = kmem_alloc_physcontig(size, preq, flags);
	physreq_free(preq);
	if (mem)
		bzero(mem, size);

	return mem;
}
#endif /* PDI_VERSION >= PDI_SVR42MP */

/* The following macros should be used to navigate the 
 * Hardware Configuration Page 
 */

	/* Length of page data: include first 4 bytes */
#define DPT_HCP_LENGTH(page)	(sdi_swap16(*(short *)(void *)(&page[2]))+4)

	/* Address of first log parameter */
#define DPT_HCP_FIRST(page)	(&page[4])

	/* Address of next log parameter */
#define DPT_HCP_NEXT(parm)	(&parm[3 + parm[3] + 1])

	/* parameter code */
#define DPT_HCP_CODE(parm)	sdi_swap16(*(short *)(void *)parm)


	/* Buffer for Mode Sense/Log Sense page */
/*
 * STATIC void
 * dpt_cache_detect(int contoller, int sleepflag)
 *
 * Description: Detect parameters of any contoller cache.
 *
 * Entry/Exit Locks: None.
 */
STATIC void
dpt_cache_detect(int c, int sleepflag)
{
	int bytes;
	struct dpt_ccb		*cp = dpt_getccb(c);
	char 			*name = dptidata[c].name;
	char *parm;

	/* In case of error, don't hurt best performing system */
	dpt_sc_ha[c].ha_cache = DPT_CACHE_WRITEBACK;

	if (!dpt_buf) 
		dpt_buf = KMEM_ZALLOC(DPT_BUFSIZE, sleepflag);
	if (!dpt_buf) {
		/*
		 *+ DPT Cannot allocate memory to detect cache
		 */
		cmn_err(CE_NOTE,"%s: Cannot Allocate Memory to Detect Cache", name);
		return;
	}
	cp->CP_OpCode        	= CP_DMA_CMD;
	cp->c_time 		= 0;
	cp->CPop.byte		= 0;
	cp->CPop.bit.DataIn	= 1;
	cp->CPop.bit.Interpret	= 1;
	cp->CPdataDMA 		= vtop((caddr_t) dpt_buf, NULL);
	cp->CPdataDMA 		= (paddr_t) dpt_swap32((long) cp->CPdataDMA);
	cp->CPID		= 0;
	cp->CPmsg0		= HA_IDENTIFY_MSG | HA_DISCO_RECO;
	cp->c_bind		= NULL;
	/*
	** Build the EATA Command Packet structure
	** for a Log Sense Command.
	*/
	cp->CPcdb[0]		= 0x4d;		/* log sense */
	cp->CPcdb[1]		= 0x0;
	cp->CPcdb[2]		= 0x40;
	cp->CPcdb[2]		|= 0x33;	/* controller configuration */
	cp->CPcdb[5]		= 0;
	cp->CPcdb[6]		= 0;
	*(ushort *)(void *)&cp->CPcdb[7] = sdi_swap16(DPT_BUFSIZE);
	*(unsigned long *)(void *)cp->CPdataLen = dpt_swap32(DPT_BUFSIZE);
	
	bzero(dpt_buf, DPT_BUFSIZE);	/* Enable sanity check below */

	dpt_intr_wait = TRUE;
	dpt_cmd_in_progress = 0x4d;
	dpt_send(c, START, cp);

	if (dpt_wait(c, 10000, DPT_INTR_ON) == FAILURE) {
		/*
		 *+ DPT Log Page timeout
		 */
		cmn_err(CE_NOTE,"%s: Log Page timeout", name);
		dpt_intr_wait = FALSE;
		return;
	}
	dpt_freeccb(c, cp);

	if (dpt_buf[0] != 0x33) {	/* Sanity check */
		/*
		 *+ DPT Failed to get Log Page
		 */
		cmn_err(CE_NOTE,"%s: Failed to get Log Page", name);
		return;
	}

	bytes = DPT_HCP_LENGTH(dpt_buf);
	parm  = DPT_HCP_FIRST(dpt_buf);

	if (DPT_HCP_CODE(parm) != 1) {
		/*
		 *+ DPT Log Page layout error 
		 */
		cmn_err(CE_NOTE, "%s: Log Page (1) layout error", name);
		return;
	}
	if (!(parm[4] & 0x4)) {
		cmn_err(CE_NOTE,"!%s: No Controller Cache Detected.\n", name);
		dpt_sc_ha[c].ha_cache = DPT_NO_CACHE;
		return;
	}

	cmn_err(CE_NOTE,"!%s: Controller Cache Detected.\n", name);

	while (DPT_HCP_CODE(parm) != 6)  {
		parm = DPT_HCP_NEXT(parm);
		if (parm < dpt_buf || parm >= &dpt_buf[bytes]) {
			dpt_mode_sense(c);
			return;
		}
	}

	if (parm[4] & 0x2) {
		cmn_err(CE_NOTE,"!%s: Controller Cache Disabled", name);
		dpt_sc_ha[c].ha_cache = DPT_NO_CACHE;
		return;
	}
	if (parm[4] & 0x4) {
		cmn_err(CE_NOTE,"!%s: Controller Cache Writethrough", name);
		dpt_sc_ha[c].ha_cache = DPT_CACHE_WRITETHROUGH;
		return;
	}
	return;
}

/*
 * STATIC void
 * dpt_mode_sense(int c)
 *
 * Description: Send Mode Sense to HBA to determine cache type.  This routine
 *	is only called on firmware that does not return enough information
 *	from Log Sense. 
 *	
 *	This routine should only be called if cache has already been detected
 *	on the controller.
 *
 * Entry/Exit Locks: None.
*/
STATIC void
dpt_mode_sense(int c)
{
	struct dpt_ccb		*cp = dpt_getccb(c);
	char 			*name = dptidata[c].name;
	char			*page;

	cp->CP_OpCode        	= CP_DMA_CMD;
	cp->c_time 		= 0;
	cp->CPop.byte		= 0;
	cp->CPop.bit.DataIn	= 1;
	cp->CPop.bit.Interpret	= 1;
	cp->CPdataDMA 		= vtop((caddr_t) dpt_buf, NULL);
	cp->CPdataDMA 		= (paddr_t) dpt_swap32((long) cp->CPdataDMA);
	cp->CPID		= 0;
	cp->CPmsg0		= HA_IDENTIFY_MSG | HA_DISCO_RECO;
	cp->c_bind 		= NULL;
	/*
	** Build the EATA Command Packet structure
	** for a Mode Sense Command.
	*/
	cp->CPcdb[0]		= SS_MSENSE;	/* Mode Sense */
	cp->CPcdb[1]		= 0;
	cp->CPcdb[2]		= 0x00;		/* PC: Current Values */
	cp->CPcdb[2]		|= 0x8;		/* Caching Page */
	cp->CPcdb[3]		= 0;
	cp->CPcdb[4] 		= DPT_BUFSIZE;
	*(unsigned long *)(void *)cp->CPdataLen = dpt_swap32(DPT_BUFSIZE);
	
	bzero(dpt_buf, DPT_BUFSIZE);	/* Enable sanity check below */

	dpt_intr_wait = TRUE;
	dpt_cmd_in_progress = SS_MSENSE;
	dpt_send(c, START, cp);

	if (dpt_wait(c, 10000, DPT_INTR_ON) == FAILURE) {
		/*
		 *+ DPT Mode Sense timeout
		 */
		cmn_err(CE_NOTE,"%s: Mode Sense timeout", name);
		dpt_intr_wait = FALSE;
		return;
	}
	dpt_freeccb(c, cp);

	page =  &dpt_buf[4] + dpt_buf[3];

	if ((page[0]&0x3F) != 0x8 || page[1] != 0xA) {	
		/*
		 *+ DPT Caching Page layout error
		 */
		cmn_err(CE_NOTE,"%s: Caching Page layout error", name);
		return;
	}
	if (!(page[2] & 0x4)) {
		cmn_err(CE_NOTE,"!%s: Controller Cache Writethrough", name);
		dpt_sc_ha[c].ha_cache = DPT_CACHE_WRITETHROUGH;
		return;
	}
	return;
}

#if PDI_VERSION < PDI_SVR42MP
/*
 * STATIC void
 * dptH_DMA_Setup(int chan)
 *
 * Description: Enable cascase mode on dma channel.
 *
 * Entry/Exit Locks: None.
*/
dptH_DMA_Setup(int chan)
{
	register int Channel;

	Channel = (8 - chan) & 7;  /* DMA channel 5,6,7,0 maps from 3,2,1,0 */

	if (Channel < 4) {
		outb(DMA0_3MD, Channel | CASCADE_DMA);
		outb(DMA0_3MK, Channel);
	} else {
		outb(DMA4_7MD, (Channel & 3) | CASCADE_DMA);
		outb(DMA4_7MK,  Channel & 3);
	}

	return(Channel);
}
#endif	/* PDI_VERSION < PDI_SVR42MP */
