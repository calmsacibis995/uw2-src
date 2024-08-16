/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*		Copyright (c) 1991  Intel Corporation			*/
/*			All Rights Reserved				*/

/*		INTEL CORPORATION PROPRIETARY INFORMATION		*/

/*	This software is supplied to Novell under the terms of a license */ 
/*	agreement with Intel Corporation and may not be copied nor       */
/*	disclosed except in accordance with the terms of that agreement. */	

static char prog_copyright[] = "Copyright 1991 Intel Corp. 469176-010";

#ident	"@(#)kern-i386at:io/dlpi_ether/flash32/flash32init.c	1.5"
#ident	"$Header: $"

/* hardware dependent code for flash32 */

#ifdef _KERNEL_HEADERS

#include <io/dlpi_ether/flash32/dlpi_flash32.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/flash32/flash32.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <util/ksynch.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/ca/eisa/nvm.h>
#include <io/autoconf/ca/ca.h>
#include <net/inet/if.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/mod/moddefs.h>
#include <io/conf.h>
#include <io/ddi.h>

#else

#include <sys/dlpi_flash32.h>
#include <sys/dlpi_ether.h>
#include <sys/flash32.h>
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/dlpi.h>
#include <sys/ksynch.h>
#include <sys/cm_i386at.h>
#include <sys/confmgr.h>
#include <sys/nvm.h>
#include <sys/ca.h>
#include <net/if.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/debug.h>
#include <sys/inline.h>
#include <sys/moddefs.h>
#include <sys/conf.h>
#include <sys/ddi.h>

#endif

extern	int	flash32nboards;
extern	int	flash32nsaps;
extern	int     flash32strlog;
extern	int	flash32cmajor;
extern	char    *flash32_ifname;
extern	struct  ifstats *ifstats;

/* Check for validity of interrupts */

char flash32id_string[] = "EtherExpress Flash32";

STATIC	void    flash32free_rxbufs(), flash32free_txbufs();
STATIC	void	flash32uninit();
STATIC	void	flash32init_board();
STATIC	int	flash32init_tx(), flash32init_rx();
STATIC	int	flash32diagnose_596();
STATIC	int	flash32chk_slot();
STATIC	int	flash32_load(), flash32_unload();

int	flash32init();
int	flash32start();
int	flash32init_596();
int    	flash32ia_setup_596();
int	flash32config_596();
int	flash32wait_scb();
int	flash32ack_596();
void	flash32port();
void	flash32halt();

DL_bdconfig_t   *DLconfig;
DL_sap_t        *DLsaps;
int	        DLboards;
int		flash32timer_id;
physreq_t	flash32physreq;
DL_irq_t	flash32irq_group[16];
int		flash32devflag = D_MP;	/* Multi-threaded */
static	void    **cookie;               /* for cm_intr_attach */

extern	void    flash32intr(int);
extern	void	flash32watch_dog();

#define FLASH32_HIER     2       	/* lock */

STATIC LKINFO_DECL( flash32_lockinfo, "ID:flash32:flash32_lock", 0);

/*
 * wrapper, and function declarations and definitions for loadability
 */
#define DRVNAME	"flash32 - Loadable flash32 ethernet driver"

MOD_DRV_WRAPPER(flash32, flash32_load, flash32_unload, flash32halt, DRVNAME);
/*
MOD_ACHDRV_WRAPPER(flash32, flash32_load, flash32_unload, flash32halt, NULL, DRVNAME);
*/

/*
 * Wrapper functions.
 */

STATIC int
flash32_load(void)
{
	int ret_code;

	cmn_err(CE_NOTE, "!MOD: in flash32_load()");

	if (ret_code = flash32init()) {
		flash32uninit();
		return (ret_code);
	}
	flash32start();
	return(0);
}

STATIC int
flash32_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in flash32_unload()");

	flash32uninit();
	return(0);
}

void
flash32halt()
{
	flash32uninit();
}

int
flash32start ()
{   
	flash32timer_id = itimeout(flash32watch_dog, 0, FLASH32_TIMEOUT, plstr);
}

/*************************************************************************
 * flash32init ()
 */

int
flash32init ()
{
	DL_bdconfig_t   *bd;
	bdd_t           *bdd;
	DL_sap_t        *sap;
	ulong         	base_io;
	int             i, board;
	int		ret;
	cm_num_t        slot_num;
	cm_args_t       cm_args;
	cm_range_t      range;



	/*
	 *  Get number of boards from Resource Manager.
	 */
	DLboards = cm_getnbrd( DL_NAME );

	if (DLboards == 0) {
		cmn_err (CE_CONT, "No %s board in DCU\n", DL_NAME);
		return(ENOENT);
	}

	if (DLboards > flash32nboards) {
                cmn_err (CE_WARN,
                "Too many boards, only %d will be configured", flash32nboards);
                DLboards = flash32nboards;
        }

	/*
	 *  Allocate generic board structure
	 */
	DLconfig = (DL_bdconfig_t *)kmem_zalloc(
				(sizeof(DL_bdconfig_t) * DLboards), KM_NOSLEEP);

	if (DLconfig == NULL) {
		cmn_err(CE_WARN, "flash32init: kmem_zalloc bd failed");
		return (ENOMEM);
	}

	/*
	 *  Allocate generic sap structure
	 */
	DLsaps = (DL_sap_t *)kmem_zalloc(
			(sizeof(DL_sap_t) * DLboards * DLnsaps), KM_NOSLEEP);

	if (DLsaps == NULL) {
		cmn_err(CE_WARN, "flash32init: kmem_zalloc sap failed");
		return (ENOMEM);
	}

	bzero ((caddr_t)flash32irq_group, sizeof(flash32irq_group));

	/*
	 *  Allocate cookies for cm_intr_[at|de]tach.
	 */
	cookie = (void **)kmem_zalloc((sizeof(void **) * DLboards), KM_NOSLEEP);
	if (cookie == NULL) {
		cmn_err(CE_WARN, "flash32init: kmem_zalloc cookie failed");
		return (ENOMEM);
	}

	for(board = 0; board < DLboards; board++) {
		bd = &flash32config[board];
		bd->ifstats = (struct ifstats *)NULL;
		/*
		 *  Get key for this board
		 */
		cm_args.cm_key = cm_getbrdkey(DL_NAME, board);
		cm_args.cm_n = 0;

		/*
		 *  Get interrupt vector
		 */
		cm_args.cm_param = CM_IRQ;
		cm_args.cm_val = &(bd->irq_level);
		cm_args.cm_vallen = sizeof(cm_num_t);
		ret = cm_getval(&cm_args);

		if (ret) {
                        cmn_err(CE_WARN,
			"flash32 board %d, no irq in DCU", board);
                        return(ENOENT);
                }

		/*
		 * Get EISA slot number
		 */
		cm_args.cm_param = CM_SLOT;
		cm_args.cm_val = &(slot_num);
		cm_args.cm_vallen = sizeof(cm_num_t);
		ret = cm_getval(&cm_args);
		if (ret) {
                        cmn_err(CE_WARN,
			"flash32 board %d, no DCU slot", board);
                        return(ENOENT);
		}

		/*
		 *  Get I/O Addresses
		 */
		cm_args.cm_param = CM_IOADDR;
		cm_args.cm_val = &range;
		cm_args.cm_vallen = sizeof(struct cm_addr_rng);
		ret = cm_getval(&cm_args);
		if (ret) {
			/*
			 *  ENOENT is permissible.
			 */
			if (ret == ENOENT) {
				bd->io_start = (ulong_t)slot_num << 12;
				bd->io_end = bd->io_start + 0x0fff;
			}
			else {
                        	cmn_err(CE_WARN,
				"flash32 board %d, no io addr DCU", board);
                        	return(ENOENT);
			}
		} else {
			bd->io_start = range.startaddr & 0xf000;
			bd->io_end = range.endaddr;
		}

		flash32physreq.phys_align         = 16;
                flash32physreq.phys_boundary      = (paddr_t)0;
                flash32physreq.phys_dmasize       = (uchar_t)32;
                flash32physreq.phys_max_scgth     = (uchar_t)0;
                flash32physreq.phys_flags        |= PREQ_PHYSCONTIG;

                if (!physreq_prep(&flash32physreq, KM_NOSLEEP))
                        return(ENOENT);

		if (flash32chk_slot(slot_num) == -1) {
			cmn_err(CE_WARN,
			"flash32: unsupported board in slot %d.", slot_num);
			continue;
		}

		base_io = bd->io_start;

		/* initialize flags and mib structure */
		bd->flags = 0;
		bzero((caddr_t)&bd->mib, sizeof(DL_mib_t));

		/* Initialize DLconfig */
		bd->major	  = flash32cmajor+board;
		bd->bd_number     = board;
		bd->max_saps      = flash32nsaps;
		bd->sap_ptr       = &flash32saps[bd->max_saps * board];
		bd->tx_next       = 0;
		bd->timer_val     = -1;
		bd->promisc_cnt   = 0;
		bd->multicast_cnt = 0;
		bd->bd_dependent1 = (caddr_t)kmem_zalloc(sizeof(bdd_t)+16,
						KM_NOSLEEP);
		if ( ! bd->bd_dependent1 ) {
			cmn_err(CE_WARN, "flash32init: no memory for bdd_t");
			return (ENOMEM);
		}
		/* 16-byte aligned */
		bdd = (bdd_t *)((long)(bd->bd_dependent1 + 0x0f) & 0xfffffff0);
		bdd->next_sap	= NULL;
		bdd->first_init	= 1;
                bdd->slot       = slot_num;
		bdd->brd_type	= 0;

		for (i = 0; i < MULTI_ADDR_CNT; i++)
			bdd->flash32_multiaddr[i].status = 0;

		bd->bd_dependent2 = 0;
		bd->bd_dependent3 = 0;
		bd->bd_dependent4 = 0;
		bd->bd_dependent5 = 0;

		if (!(bd->bd_lock = LOCK_ALLOC( FLASH32_HIER, plstr, 
			&flash32_lockinfo, KM_NOSLEEP))) {
			/*
			 *+ can't get lock, stop configuring the driver
			 */
			cmn_err(CE_WARN, "flash32init: no memory for lock");
			return(ENOLCK);
		}

		/* Initialize SAP structure info */
		for (sap=bd->sap_ptr, i=0; i < bd->max_saps; i++, sap++) {
			sap->state          = DL_UNBOUND;
			sap->sap_addr       = 0;
			sap->read_q         = NULL;
			sap->write_q        = NULL;
			sap->flags          = 0;
			sap->max_spdu       = USER_MAX_SIZE;
			sap->min_spdu       = USER_MIN_SIZE;
			sap->mac_type       = DL_ETHER;
			sap->service_mode   = DL_CLDLS;
			sap->provider_style = DL_STYLE1;;
			sap->bd		    = bd;

			if ((sap->sap_sv = SV_ALLOC(KM_NOSLEEP)) == NULL) {
				/*
				 *+ can't alloc sap_sv
				 */
				cmn_err(CE_WARN, 
					"flash32_init: no memory for sap_sv");
				return(ENOLCK);
			}
		}

		/* Initalize internet stat statucture */
		if ((bd->ifstats = kmem_zalloc(sizeof(struct ifstats), 
			KM_NOSLEEP)) == NULL) {
			cmn_err(CE_WARN, "flash32init: no memory for ifstats");
			return (ENOMEM);
		}

		bd->ifstats->ifs_name   = flash32_ifname;
		bd->ifstats->ifs_unit   = (short)board;
		bd->ifstats->ifs_mtu    = USER_MAX_SIZE;
		bd->ifstats->ifs_active = 1;
		bd->ifstats->ifs_ipackets = 0;
		bd->ifstats->ifs_opackets = 0;
		bd->ifstats->ifs_ierrors = 0;
		bd->ifstats->ifs_oerrors = 0;
		bd->ifstats->ifs_collisions = 0;
		ifstats_attach(bd->ifstats);

		bdd->ChanAtn  = base_io;
		bdd->Port596  = base_io + 0x8;
		bdd->Control0 = base_io + IRQCTL;
		bdd->IAProm   = base_io + 0xC90;

			/* get ethernet address */
		for(i=0;i<6;i++)
			bd->eaddr.bytes[i] = inb(bdd->IAProm + i);

		if( ( 
			(bd->eaddr.words[0] == 0x0000) &&
			(bd->eaddr.words[1] == 0x0000) &&
			(bd->eaddr.words[2] == 0x0000) ) ||
		     (
			(bd->eaddr.words[0] == 0xFFFF) &&
			(bd->eaddr.words[1] == 0xFFFF) &&
			(bd->eaddr.words[2] == 0xFFFF) ) ) {
				cmn_err(CE_WARN,
		"flash32: failed Ethernet address acquisition for board %d.",
				board);
				return(ENXIO);
		}

		if ((inb(base_io+FLCTL1) & 0x08) == 0) {
			cmn_err(CE_WARN,
"flash32: please specify cable type from ECU. Auto-detect is not supported.");
			return(ENXIO);
		}

		flash32init_board(bd);

		for (i=0; i < flash32nboards; i++)
                        if (!(flash32irq_group[bd->irq_level].bd[i]))
                                break;

                flash32irq_group[bd->irq_level].bd[i] = bd;
#ifdef TEMP
		if (bdd->brd_type == 1) {
                        if (cm_AT_putconf(cm_args.cm_key, 0, CM_ITYPE_EDGE,
                                        0, 0, 0, 0, -1, CM_SET_ITYPE, 0)) {
                            cmn_err(CE_WARN, "flash32 cm_AT_putconf failed");
                            return(ENXIO);
                        }
                }
                else {
                        if (cm_AT_putconf(cm_args.cm_key, 0, CM_ITYPE_LEVEL,
                                        0, 0, 0, 0, -1, CM_SET_ITYPE, 0)) {
                            cmn_err(CE_WARN, "flash32 cm_AT_putconf failed");
                            return(ENXIO);
                        }
                }
#endif
		if (flash32init_596(bd)) {
			cmn_err(CE_WARN,
				"flash32init: board %d init failed", board);
			return(ENXIO);
		}

                if (cm_intr_attach(cm_args.cm_key, flash32intr,
                                &DLdevflag, &(cookie[board])) == 0) {
                        cmn_err(CE_WARN, "flash32 failed to attach interrupt.");
                        return(EUNATCH);
                }

		bd->flags = BOARD_PRESENT;
		bd->mib.ifOperStatus = DL_UP;			/* SNMP */

		cmn_err(CE_CONT, "io_addr 0x%b%b irq %d ",
			(uchar_t)(base_io >> 8), (uchar_t)(base_io & 0x00ff),
								bd->irq_level);
		flash32print_eaddr(bd->eaddr.bytes);
	}
	return(0);
}

/*************************************************************************
 * flash32init_596 ()
 *
 * initialize the 82596's scp, iscp, and scb; reset the 82596;
 * do IA setup command to add the ethernet address to the 82596 IA table;
 * and enable the Receive Unit.
 */

int
flash32init_596(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	paddr_t	scp_paddr;
	int	i;
	uchar_t	reg0;


	/* mask off irq from FLEA, Force low */
	reg0 = inb (bdd->Control0) | 0x20;
	outb (bdd->Control0, reg0);

	flash32port(bdd, 0);	/* reset the 82596 */
	drv_usecwait(100);

	/* initialize tx/rx, make sure init_tx is done first */
	if (flash32init_tx(bdd) || flash32init_rx(bdd)) {
		cmn_err(CE_WARN,"flash32: init_tx or init_rx failed");
		return(-1);
	}
	bdd->first_init = 0;

	/* scp is on 16-byte boundary, add 2 to make it as Alternate SCP */
	scp_paddr = vtop((caddr_t) &bdd->scp, NULL) + 2;

	/* fill in scp */
	bdd->scp.zeros		= 0;
	bdd->scp.sysbus		= CSW_BIT | MODE_LINEAR;
	bdd->scp.unused[0]	= 0;
	bdd->scp.unused[1]	= 0;
	bdd->scp.iscp_paddr	= vtop((caddr_t) &bdd->iscp, NULL);

	/* fill in iscp */
	bdd->iscp.busy 		= 1;
	bdd->iscp.unused	= 0;
	bdd->iscp.scb_paddr	= vtop((caddr_t) &bdd->scb, NULL);

	/* fill in scb */
	bdd->scb.status 	= 0;
	bdd->scb.control	= 0;
	bdd->scb.cbl_paddr 	= 0;
	bdd->scb.rfa_paddr 	= 0;
	bdd->scb.crc_err	= 0;
	bdd->scb.align_err	= 0;
	bdd->scb.resource_err	= 0;
	bdd->scb.overrun_err	= 0;
	bdd->scb.rcvcdt_err	= 0;
	bdd->scb.shortframe_err	= 0;
	bdd->scb.toff_timer	= 4;
	bdd->scb.ton_timer	= 66;

	/* relocate scp from default addr via 82596 PORT */
	flash32port(bdd, scp_paddr);

	/* issue channel attention to start 596 init process */ 
	outl(bdd->ChanAtn, (ulong)0x00);

	/* wait for iscp busy to be cleared */
	for (i=1000; i; i--) {		
		if (!bdd->iscp.busy)
			break;
		drv_usecwait(100);
	}

	if (i == 0) {
		cmn_err(CE_WARN,"flash32: iscp init failed");
		return(-1);
	} 

	for (i=1000; i; i--) {		/* wait for scb status */
		if (bdd->scb.status == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait(10);
	}

	if (i == 0) {			/* if CX & CNA aren't set */
		cmn_err(CE_WARN,"flash32: scb init failed");
		return(-1);
	}

	if ( flash32ack_596(bdd) )
		return(-1);

	/* set bus throttle timers */
	bdd->scb.toff_timer		= 4;
	bdd->scb.ton_timer		= 66;
	bdd->scb.control		= CU_START_TIMERS;

	outl(bdd->ChanAtn, (ulong)0x00);	 /* channel attention */
	drv_usecwait(10);

	for( i = 4096; i; i--) {
		drv_usecwait(10);	/* wait 10 usec */
		if(bdd->scb.status & TIMERS_LOADED)
			break;
	}

	if(i == 0) {
		cmn_err(CE_WARN,"flash32: load bus throttle timers failed");
		return(-1);
	}

	if (flash32ack_596(bdd))
		return(-1);

	/* configure 596 with default parameters */
	if (flash32config_596(bd, PRO_OFF, LOOP_OFF)) {
		cmn_err(CE_WARN,"flash32: config failed");
		return(-1);
	}

	/* do IA Setup command */
	if (flash32ia_setup_596(bd, bd->eaddr.bytes)) {
		cmn_err(CE_WARN,"flash32: ia_setup failed");
		return(-1);
	}

	drv_usecwait(100);

	/* enable 596 Receive Unit */
	bdd->scb.status		= 0;
	bdd->scb.control	= RU_START;
	bdd->scb.rfa_paddr	= vtop((caddr_t)bdd->rfd, NULL);

	outl(bdd->ChanAtn, (ulong)0);	/* channel attention */

	/* unmask the interrupt */
	outb (bdd->Control0, reg0 & ~0x20);

	return(0);
}


/*************************************************************************/
STATIC int
flash32init_tx (bdd)
bdd_t    *bdd;
{
	int    i,j;
	tcb_t  *tcb;
	tbd_t  *tbd;

	/* tcb's are now dynamically allocated to avoid spanning pages; */
	if ( bdd->first_init ) {
		if ((bdd->tcb = (tcb_t *)kmem_alloc_physreq(
			PAGESIZE, &flash32physreq, KM_NOSLEEP)) == NULL)
                        return -1;
	}

	tcb = bdd->tcb;

	for ( i=0 ; i < MAX_TCB ; i++, tcb++ ) {
		tcb->status		= 0;
		tcb->cmd    		= CS_CMD_XMIT|CS_EL|FLEX_MODE;
		tcb->link   		= CS_NO_LINK;
		tcb->parms.tbd_paddr	= vtop((caddr_t)&(tcb->tbd), NULL);
		tcb->parms.tcb_count	= 0;
		tcb->parms.zeros	= 0;
		tcb->next		= tcb + 1;
		if (bdd->first_init)
                    if ((tcb->txbuf = (caddr_t)kmem_alloc_physreq(
                            XMTBUFSIZE, &flash32physreq, KM_NOSLEEP)) == NULL)
                        return -1;

		tcb->tbd.count = CS_EOF;
		tcb->tbd.zeros = 0;
		tcb->tbd.link  = CS_NO_LINK;
                tcb->tbd.bufaddr = vtop(tcb->txbuf, NULL);
	}
	(--tcb)->next = bdd->tcb;

	bdd->head_tcb	= NULL;
	bdd->tail_tcb	= NULL;
	return 0;
}


/*************************************************************************
 * flash32init_rx (bdd)
 *   initialize i596 buffer configuration for receive side
 */

STATIC int
flash32init_rx (bdd)
bdd_t     *bdd;
{
	rfd_t *rfd;
	rbd_t *rbd;
	int    i;


	rfd = bdd->rfd = (rfd_t *)((char *)bdd->tcb + (sizeof(tcb_t)*MAX_TCB));
	rbd = bdd->rbd = (rbd_t *)((char *)bdd->rfd + (sizeof(rfd_t)*MAX_RFD));

	bdd->begin_rfd = rfd;
	bdd->begin_rbd = rbd;

	bdd->rbd_paddr = vtop((caddr_t)rbd, NULL);

	/* init rfd to a circular list */

	for ( i = 0; i < MAX_RFD; i++, rfd++ ) {
		rfd->status	= 0;
		rfd->control	= FLEX_MODE;
		rfd->next	= rfd + 1;
		rfd->prev	= rfd - 1;
		rfd->link	= vtop((caddr_t)rfd->next, NULL);
		rfd->rbd_paddr	= CS_NO_LINK;
		rfd->count	= 0;
		rfd->size	= 0;
	}

	/* init last rfd */
	(--rfd)->next = bdd->begin_rfd;
	rfd->link = vtop((caddr_t)rfd->next, NULL);
	rfd->control |= CS_EL;

	/* init 1st rfd's rbd */
	bdd->begin_rfd->rbd_paddr = vtop((caddr_t)bdd->begin_rbd, NULL);
	bdd->begin_rfd->prev = rfd;
	
	/* init rbd to a circular list */

	for ( i = 0, rbd = bdd->rbd; i < MAX_RBD ; i++, rbd++ ) {
		rbd->status  = 0;
		rbd->zero1   = 0;
		rbd->next    = rbd + 1;
		rbd->prev    = rbd - 1;
		rbd->link    = vtop((caddr_t)rbd->next, NULL);

		if ( bdd->first_init ) {
		    if ((rbd->rxbuf = kmem_alloc_physreq(
                            RCVBUFSIZE, &flash32physreq, KM_NOSLEEP)) == NULL)
                        return(-1);
		}

		rbd->bufaddr = vtop((caddr_t)rbd->rxbuf, NULL);
		rbd->bufsize = RCVBUFSIZE;
		rbd->zero2   = 0; 
	}

	/* init the last rbd */
	(--rbd)->next = bdd->begin_rbd;
	rbd->link = vtop((caddr_t)rbd->next, NULL);
	rbd->bufsize = RCVBUFSIZE | CS_EL;
	bdd->begin_rbd->prev = rbd;
	return(0);
}

/*************************************************************************/
STATIC int
flash32diagnose_596(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	int		i;

	if (flash32wait_scb(bdd, 1000))
		return (1);

	if (flash32ack_596(bdd))
		return (1);

	if (flash32wait_scb(bdd, 1000))
		return (1);

	bdd->scb.status    = 0;
	bdd->scb.control   = CU_START;
	bdd->scb.cbl_paddr = vtop((caddr_t) &bdd->cb, NULL);

	bdd->cb.status	= 0;
	bdd->cb.cmd	= CS_CMD_DGNS | CS_EL;

	outl(bdd->ChanAtn, (ulong)0);		 /* channel attention */

	for (i=0; i < 100; i++) {
		if (bdd->cb.status & CS_OK)
			break;
		drv_usecwait(10);
	}

	if (i == 100) {
		cmn_err(CE_NOTE, "flash32diagnose_596: diagnose failed");
		return(1);
	}

	if (flash32ack_596(bdd))
		return (1);

	return (0);
}

/*************************************************************************/
int
flash32config_596 (bd, prm_flag, loop_flag)
DL_bdconfig_t *bd;
ushort_t prm_flag;
ushort_t loop_flag;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;

	if (flash32wait_scb(bdd, 1000))
		return (1);

	bdd->scb.status   = 0;
	bdd->scb.control  = CU_START;
	bdd->scb.cbl_paddr = vtop((caddr_t) &bdd->cb, NULL);

	bdd->cb.status	= 0;
	bdd->cb.cmd	= CS_CMD_CONF | CS_EL;
	bdd->cb.link	= CS_NO_LINK;

	bdd->cb.parm.conf.cnf_fifo_byte	= 0xfc0e;
	bdd->cb.parm.conf.cnf_add_mode	= 0x2e00 | (loop_flag?0x4000:0x0000);
	bdd->cb.parm.conf.cnf_pri_data	= 0x6000;
	bdd->cb.parm.conf.cnf_slot	= 0xf200;
	bdd->cb.parm.conf.cnf_hrdwr	= 0x0000 | (prm_flag?0x0001:0x0000);
	bdd->cb.parm.conf.cnf_min_len	= 0xff40;
	bdd->cb.parm.conf.cnf_dcr_num	= 0x3f00;

	outl(bdd->ChanAtn, (ulong)0);		 /* channel attention */

	if (flash32wait_scb(bdd, 1000))
		return (1);

	if (flash32ack_596 (bdd))
		return (1);

	return (0);
}

/*************************************************************************/
int
flash32ia_setup_596 (bd, eaddr)
DL_bdconfig_t *bd;
uchar_t eaddr[];
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	ushort	i;
	
	if (flash32wait_scb(bdd, 1000))
		return (1);

	bdd->scb.status		= 0;
	bdd->scb.control	= CU_START;
	bdd->scb.cbl_paddr	= vtop((caddr_t) &bdd->cb, NULL);
	
	bdd->cb.status		= 0;
	bdd->cb.cmd		= CS_CMD_IASET | CS_EL;

	for (i=0; i<6; i++)
	    bdd->cb.parm.ia_set[i] = eaddr[i];

	outl(bdd->ChanAtn, (ulong)0);		 /* channel attention */

	if (flash32wait_scb(bdd, 1000))
		return (1);

	if (flash32ack_596(bdd))
		return (1);

	return (0);
}

/*************************************************************************
 * flash32wait_scb (bdd, how_long)
 *
 * Acceptance of a Control Command is indicated by the 82596 clearing
 * the SCB command field page 2-16 of the intel microcom handbook.
 */
int
flash32wait_scb (bdd, n)
bdd_t 	*bdd;
int	n;
{
	int i = 0x10;		/* wait every 0x10 times */

	do {
		if (bdd->scb.control == 0)
			return(0);

		if (--i == 0) {
			drv_usecwait(10);	/* wait 10 usec */
			i = 0x10;
		}
	} while (n--);
	return (1);
}

/*************************************************************************/
int
flash32ack_596 (bdd)
bdd_t	*bdd;
{
	uchar_t	reg0;

	if ( bdd->scb.control = (bdd->scb.status & SCB_INT_MSK) ) {	
		outl(bdd->ChanAtn, (ulong)0);	/* channel attention */

		if ((reg0 = inb(bdd->Control0)) & PLX_R0_LSTAT)
                       	outb(bdd->Control0, reg0);

		return ( flash32wait_scb (bdd, 10000) );
	}
	return (0);
}

/*************************************************************************/
STATIC void
flash32port(bdd,value)
bdd_t	*bdd;
ulong	value;
{
	/* The PLX interfaces takes a 32 bit value and
	   feeds it to the port 16 bits at a time. */
	outl(bdd->Port596, value);
	drv_usecwait(10);
}
		
/*************************************************************************/
STATIC void
flash32free_rxbufs(bdd)
bdd_t	*bdd;
{
	rbd_t	*rbd;
	ushort	i;

	for ( i=0, rbd=bdd->rbd ; i<MAX_RBD ; i++,rbd++ ){
		if(rbd->rxbuf){
                        kmem_free(rbd->rxbuf, RCVBUFSIZE);
                        rbd->rxbuf = NULL;
		}
	}
}

/*************************************************************************/
STATIC void
flash32free_txbufs(bdd)
bdd_t	*bdd;
{
	tcb_t	*tcb;
	int	i;

	if (tcb = bdd->tcb) {
		for (i = 0; i < MAX_TCB; i++, tcb++) {
			if (tcb->txbuf)
                                kmem_free(tcb->txbuf, XMTBUFSIZE);
                        tcb->txbuf = NULL;
		}

		/* free the dynamically allocated page of tcb's; */
		kmem_free(bdd->tcb, PAGESIZE);
	}
}

/*************************************************************************/
STATIC void
flash32uninit()
{
	int		board;
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	DL_sap_t        *sap;
        uchar_t		reg0;
	int		i;
	int		opri;

	untimeout(flash32timer_id);

	if (!cookie || !DLconfig)       return;

	for(board = 0; board < DLboards; board++){

		bd = &DLconfig[board];

		if (bd->bd_lock)
			opri = DLPI_LOCK( bd->bd_lock, plstr );

		flash32irq_group[bd->irq_level].bd[0] = 0;

                bd->flags = 0;

		if ( bdd = (bdd_t *)bd->bd_dependent1 ) {
			bdd->head_tcb = (tcb_t *)NULL;
                        bdd->scb.control = (CU_ABORT | RU_ABORT);
                        outl(bdd->ChanAtn, (ulong)0);
                        flash32wait_scb(bdd, 10000);

                        if ((reg0 = inb(bdd->Control0)) & PLX_R0_LSTAT)
                        	outb(bdd->Control0, reg0);

			/* mask off irq from FLEA, Force low */
			outb (bdd->Control0, reg0 | 0x20);

			flash32port(bdd, 0);		/* reset the 82596 */
			flash32port(bdd, 0);		/* reset the 82596 */

			cm_intr_detach(cookie[board]);

			/* make sure free_rxbufs first */
			flash32free_rxbufs(bdd);
			flash32free_txbufs(bdd);
			kmem_free(bd->bd_dependent1, sizeof(bdd_t));
		}

		if (bd->ifstats) {
	    		(void)ifstats_detach(bd->ifstats);

	    		kmem_free(bd->ifstats, sizeof(struct ifstats));
		}

		if (sap = bd->sap_ptr) {
			for (i=0; i < bd->max_saps; i++, sap++) {
				if (sap->write_q != NULL)
                                        flushq(sap->write_q, FLUSHDATA);

				if (sap->sap_sv) {
					SV_DEALLOC(sap->sap_sv);
				}
			}
		}

		if (bd->bd_lock) {
			DLPI_UNLOCK( bd->bd_lock, opri );

			LOCK_DEALLOC(bd->bd_lock);
		}
	}

	if (DLsaps)
		kmem_free(DLsaps, sizeof(DL_sap_t)*DLboards*flash32nsaps);

	kmem_free(cookie, (sizeof(void **) * DLboards));
	kmem_free(DLconfig, (sizeof(DL_bdconfig_t) * DLboards));
}


STATIC void
flash32init_board(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	ulong  	base_io = bd->io_start;
	uchar_t c, inval;
	int	i;


	cmn_err(CE_CONT,
		"%s in slot %d is configured as network device: flash32_%d\n",
		flash32id_string, bdd->slot, bd->bd_number);


	/* enable burst mode if latest PLX rev */
	inval = inb(base_io+PLX_R3);
	if (inval & 0x08)
		outb(base_io+PLX_R3, inval | PLX_R3_RBURST);

	/* to see if FLEA extended irq is selected */
	switch (bd->irq_level) {
		case 3: case 7: case 12: case 15:
			bdd->brd_type = 1;
	}

	cmn_err (CE_CONT, "network media: ");

	/* get the configured cable type */
	inval = (inb(base_io+PLX_R1)) & 0x03;
	switch (inval) {
	    case 0:
               	cmn_err(CE_CONT, "BNC ");
               	break;
	    case 2:
               	cmn_err(CE_CONT, "AUI ");
               	break;
	    case 3:
               	cmn_err(CE_CONT, "TP ");
               	break;
	}

	inval = inb (base_io+8) & 0xfe;
	outb (base_io+8, inval);

	/* flash bios and remote boot prom disable */
	inval = inb(base_io+FLCTL0) & 0xf0;
	outb(base_io+FLCTL0, inval | 0x08);

	inval = inb(base_io+FLCTL1) & 0xcb;
	outb(base_io+FLCTL0, inval);
}


STATIC int
flash32chk_slot(slot_num)
int slot_num;
{
	int	i;
	int	slot_address;
	uchar_t	eisa_id[EISA_ID_LEN];
	uchar_t	prod_id[OEM_ID_LEN];


	slot_address = slot_num << 12;

	for (i = 0; i < EISA_ID_LEN; i++)
		eisa_id[i] = inb(slot_address + EISA_ID_OFFSET + i);

	prod_id[0] = (eisa_id[0] >> 2 & 0x1f) + '@';
	prod_id[1] = ((eisa_id[0] << 3 | eisa_id[1] >> 5) & 0x1f) + '@';
	prod_id[2] = (eisa_id[1] & 0x1f) + '@';
	prod_id[3] = HTOA(eisa_id[2] >> 4);
	prod_id[4] = HTOA(eisa_id[2] & 0x0f);
	prod_id[5] = HTOA(eisa_id[3] >> 4);
	prod_id[6] = HTOA(eisa_id[3] & 0x0f);
	prod_id[7] = '\0';

	if (strcmp("INT1010", (char *)prod_id) == NULL)
		return(0);
	else
		return(-1);
}
