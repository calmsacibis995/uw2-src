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

#ident	"@(#)kern-i386at:io/dlpi_ether/en596/en596init.c	1.14"
#ident	"$Header: $"

/* hardware dependent code for en596 */

#ifdef _KERNEL_HEADERS

#include <io/dlpi_ether/en596/dlpi_en596.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/en596/en596.h>
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

#include <sys/dlpi_en596.h>
#include <sys/dlpi_ether.h>
#include <sys/en596.h>
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

extern	int	en596nboards;
extern	int	en596nsaps;
extern	int	en596strlog;
extern	int	en596cmajor;
extern	int	en596reset();
extern	void	en596watch_dog();
extern	char	*en596oem_id[];
extern	char	*en596brd_id[];
extern	char    *en596_ifname;
extern	struct  ifstats *ifstats;

STATIC	void    en596free_rxbufs(), en596free_txbufs();
STATIC	void	en596uninit();
STATIC	int	en596init_tx(), en596init_rx();
STATIC	int	en596diagnose_596();
STATIC	int	en596chk_slot();
STATIC	int	en596_load(), en596_unload();

int	en596init();
int	en596start();
int	en596init_596();
int    	en596ia_setup_596();
int	en596config_596();
int	en596wait_scb();
int	en596ack_596();
void	en596init_board(), en596init_chip();
void	en596port();
void	en596halt();

DL_bdconfig_t   *DLconfig;
DL_sap_t        *DLsaps;
int	        DLboards;
int		en596timer_id;
physreq_t	en596physreq;
DL_irq_t	en596irq_group[16];
char		en596id_string[] = "en596";

int             en596devflag = D_MP;    /* Multi-threaded */
static void     **cookie;		/* for cm_intr_attach */
extern void     en596intr(int);

#define EN596_HIER     2       	/* lock */

STATIC LKINFO_DECL( en596_lockinfo, "ID:en596:en596_lock", 0);

/*
 * wrapper, and function declarations and definitions for loadability
 */
#define DRVNAME	"en596 - Loadable en596 ethernet driver"

MOD_DRV_WRAPPER(en596, en596_load, en596_unload, en596halt, DRVNAME);
/**
MOD_ACHDRV_WRAPPER(en596, en596_load, en596_unload, en596halt, NULL, DRVNAME);
**/

/*
 * Wrapper functions.
 */

STATIC int
en596_load(void)
{
	int ret_code;

	cmn_err(CE_NOTE, "!MOD: in en596_load()");

	if (ret_code = en596init()) {
		en596uninit();
		return (ret_code);
	}
	en596start();
	return(0);
}

STATIC int
en596_unload(void)
{
	cmn_err(CE_NOTE, "!MOD: in en596_unload()");

	en596uninit();
	return(0);
}

int
en596start ()
{   
	rm_key_t        key;            /* key in resource manager */
	int		board;

	for (board = 0; board < DLboards; board++) {
        	key = cm_getbrdkey(DL_NAME, board);
                if (cm_intr_attach(key, en596intr,
                        	&DLdevflag, &(cookie[board])) == 0) {
                        en596config[board].flags = 0;
                        cmn_err(CE_WARN, "en596 failed to attach interrupt.");
                        return(EUNATCH);
                }
	}

	en596timer_id = itimeout(en596watch_dog, 0, EN596_TIMEOUT, plstr);
}

void
en596halt()
{
	en596uninit();
}

/*************************************************************************
 * en596init ()
 */

int
en596init ()
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	DL_sap_t	*sap;
	ulong		base_io;
        int		i, board;
        int		brd_type;
	int		ret;
        cm_num_t	slot_num;
        cm_args_t	cm_args;
        cm_range_t	range;


        /*
         *  Get number of boards from Resource Manager.
         */
        DLboards = cm_getnbrd( DL_NAME );

        if (DLboards == 0) {
                cmn_err (CE_CONT, "No %s board in DCU\n", DL_NAME);
                return(ENOENT);
        }

	if (DLboards > en596nboards) {
		cmn_err (CE_WARN,
		"Too many boards, only %d will be configured", en596nboards);
		DLboards = en596nboards;
	}

        /*
         *  Allocate generic board structure
         */
        DLconfig = (DL_bdconfig_t *)kmem_zalloc(
				(sizeof(DL_bdconfig_t) * DLboards), KM_NOSLEEP);

        if (DLconfig == NULL) {
		cmn_err(CE_WARN, "en596init: kmem_zalloc bd failed");
                return (ENOMEM);
        }

        /*
         *  Allocate generic sap structure
         */
        DLsaps = (DL_sap_t *)kmem_zalloc(
			(sizeof(DL_sap_t) * DLboards * DLnsaps), KM_NOSLEEP);

        if (DLsaps == NULL) {
		cmn_err(CE_WARN, "en596init: kmem_zalloc sap failed");
                return (ENOMEM);
        }

	bzero ((caddr_t)en596irq_group, sizeof(en596irq_group));
        /*
         *  Allocate cookies for cm_intr_[at|de]tach.
         */
        cookie = (void **)kmem_zalloc((sizeof(void **) * DLboards), KM_NOSLEEP);
        if (cookie == NULL) {
                cmn_err(CE_WARN, "en596init: kmem_zalloc cookie failed");
                return (ENOMEM);
        }

	for(board = 0; board < DLboards; board++) {
		bd = &en596config[board];
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
			/*
			 *  ENOENT is permissible.
			 */
			if (ret == ENOENT)
				bd->irq_level = 0;
			else {
                		cmn_err(CE_WARN,
					"en596 board %d, no irq in DCU",board);
                		return(ENOENT);
			}
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
                                bd->io_start = 0;
                                bd->io_end = 0;
                        }
			else {
                		cmn_err(CE_WARN,
					"en596 board %d, no io in DCU", board);
                		return(ENOENT);
			}
                } else {
                        bd->io_start = range.startaddr & 0xf000;
                        bd->io_end = range.endaddr;
                }

		/*
		 * Get EISA slot number
		 */
		cm_args.cm_param = CM_SLOT;
                cm_args.cm_val = &(slot_num);
                cm_args.cm_vallen = sizeof(cm_num_t);
                ret = cm_getval(&cm_args);
                if (ret) {
                        /*
                         *  ENOENT is permissible.
                         */
                        if (ret == ENOENT)
                                slot_num = 0;
                }

		en596physreq.phys_align		= 16;
      		en596physreq.phys_boundary	= (paddr_t)0;
      		en596physreq.phys_dmasize	= (uchar_t)32;
      		en596physreq.phys_max_scgth	= (uchar_t)0;
      		en596physreq.phys_flags	       |= PREQ_PHYSCONTIG;

		if (!physreq_prep(&en596physreq, KM_NOSLEEP))
               		return(ENOENT);

                if ((brd_type = en596chk_slot(slot_num)) == -1) {
			cmn_err(CE_WARN,
			"en596 unsupported board in slot %d.", slot_num);
                        continue;
		}

		if (brd_type == EMBEDDED)
			bd->io_start = 0x300;

		base_io = bd->io_start;

		/* initialize flags and mib structure */
		bd->flags = 0;
		bzero((caddr_t)&bd->mib, sizeof(DL_mib_t));

		/* Initialize DLconfig */
		bd->major	  = en596cmajor + board;
		bd->bd_number     = board;
		bd->max_saps	  = en596nsaps;
		bd->sap_ptr       = &en596saps[bd->max_saps * board];
		bd->tx_next       = 0;
		bd->timer_val     = -1;
		bd->promisc_cnt   = 0;
		bd->multicast_cnt = 0;
		bd->bd_dependent1 = (caddr_t)kmem_zalloc(sizeof(bdd_t)+16,
						KM_NOSLEEP);
		if ( ! bd->bd_dependent1 ) {
			cmn_err(CE_WARN, "en596init no memory for bdd_t");
			return (ENOMEM);
		}
		/* 16-byte aligned */
		bdd = (bdd_t *)((long)(bd->bd_dependent1 + 0x0f) & 0xfffffff0);
		bdd->next_sap	= NULL;
		bdd->first_init	= 1;
		bdd->brd_type	= brd_type;
		bdd->slot	= slot_num;

		for (i = 0; i < MULTI_ADDR_CNT; i++)
			bdd->en596_multiaddr[i].status = 0;

		bd->bd_dependent2 = 0;
		bd->bd_dependent3 = 0;
		bd->bd_dependent4 = 0;
		bd->bd_dependent5 = 0;

		if (!(bd->bd_lock = LOCK_ALLOC( EN596_HIER, plstr, 
			&en596_lockinfo, KM_NOSLEEP))) {
			/*
			 *+ can't get lock, stop configuring the driver
			 */
			cmn_err(CE_WARN, "en596init no memory for en596_lock");
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
					"en596_init no memory for sap_sv");
				return(ENOLCK);
			}
		}

		/* Initalize internet stat statucture */
		if ((bd->ifstats = kmem_zalloc(sizeof(struct ifstats), 
			KM_NOSLEEP)) == NULL) {
			cmn_err(CE_WARN, "en596init no memory for ifstats");
			return (ENOMEM);
		}

		bd->ifstats->ifs_name   = en596_ifname;
		bd->ifstats->ifs_unit   = (short)board;
		bd->ifstats->ifs_mtu    = USER_MAX_SIZE;
		bd->ifstats->ifs_active = 1;
		bd->ifstats->ifs_ipackets = 0;
		bd->ifstats->ifs_opackets = 0;
		bd->ifstats->ifs_ierrors = 0;
		bd->ifstats->ifs_oerrors = 0;
		bd->ifstats->ifs_collisions = 0;
                ifstats_attach(bd->ifstats);

		if (base_io < 0x1000){		/* this is the motherboard */
			bdd->Port596	=	base_io;
			bdd->ChanAtn	=	base_io + 0x4;
			bdd->Control0	=	base_io + 0x10;
			bdd->IAProm	=	base_io + 0x8;

			en596init_chip(bd);

			if (cm_AT_putconf(cm_args.cm_key,
                		bd->irq_level,
                		CM_ITYPE_EDGE,
                		bd->io_start, bd->io_start + 0x1F,
                		0, 0, -1,
                		CM_SET_IRQ|CM_SET_ITYPE|CM_SET_IOADDR, 0)) {
                		cmn_err(CE_WARN, "en596 cm_AT_putconf failed");
				return(ENXIO);
        		}
		}
		else {				/* this is EISA plugin board */
			bdd->ChanAtn	=	base_io;
			bdd->Port596	=	base_io + 0x8;
			bdd->Control0	=	base_io + 0xC88;
			bdd->IAProm	=	base_io + 0xC90;

			en596init_board(bd);

			if (cm_AT_putconf(cm_args.cm_key, 0, CM_ITYPE_LEVEL,
					0, 0, 0, 0, -1, CM_SET_ITYPE, 0)) {
                		cmn_err(CE_WARN, "en596 cm_AT_putconf failed");
				return(ENXIO);
        		}
		}
			/* get ethernet address */
		for (i=0; i < 6; i++)
			bdd->eaddr[i] = bd->eaddr.bytes[i] = inb(bdd->IAProm+i);

		if (( 
			(bd->eaddr.words[0] == 0x0000) &&
			(bd->eaddr.words[1] == 0x0000) &&
			(bd->eaddr.words[2] == 0x0000) ) ||
		     (
			(bd->eaddr.words[0] == 0xFFFF) &&
			(bd->eaddr.words[1] == 0xFFFF) &&
			(bd->eaddr.words[2] == 0xFFFF) )) {
				cmn_err(CE_WARN,
		"en596 failed Ethernet address acquisition for board %d.",
				board);
				return(ENXIO);
		}

		for (i=0; i < en596nboards; i++)
			if (!(en596irq_group[bd->irq_level].bd[i]))
				break;

		en596irq_group[bd->irq_level].bd[i] = bd;

		cmn_err(CE_CONT, "board %d io_addr 0x%b%b irq %d ",
			board, (uchar_t)(base_io >> 8),
			(uchar_t)(base_io & 0x00ff), bd->irq_level);

		en596print_eaddr(bd->eaddr.bytes);

		if (en596init_596(bd)) {
                        cmn_err(CE_WARN,
                                "en596init board %d init failed", board);
                        bd->flags = 0;
                        bd->mib.ifAdminStatus = DL_DOWN;        /* SNMP */
                        bd->mib.ifOperStatus = DL_DOWN;         /* SNMP */
                        return(ENXIO);
                }

		bd->mib.ifAdminStatus	= DL_UP;		/* SNMP */
		bd->mib.ifOperStatus	= DL_UP;		/* SNMP */
		bd->flags		= BOARD_PRESENT;
	}
	return(0);
}

/*************************************************************************
 * en596init_596 ()
 *
 * initialize the 82596's scp, iscp, and scb; reset the 82596;
 * do IA setup command to add the ethernet address to the 82596 IA table;
 * and enable the Receive Unit.
 */

int
en596init_596(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	paddr_t	scp_paddr;
	int	i;


	/* Clear pending int */
	if(bd->io_start >= 0x1000){		/* EISA boards */
		uchar_t	reg0;
		if ((reg0 = inb(bdd->Control0)) & PLX_R0_LSTAT)
			outb(bdd->Control0, reg0);

		en596port(bdd, 0);		/* reset the 82596 */
	}
	else{					/* Embedded 82596 */
		inb(bdd->Control0);

		/* do a hardware reset of 596 by writing 1 then 0 to control0 */
		outb(bdd->Control0,1);
		drv_usecwait(3);
		outb(bdd->Control0,0);
	}
	drv_usecwait(100);

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
	bdd->scb.ton_timer	= 110;

	/* initialize tx/rx */
	if (en596init_tx(bdd) || en596init_rx(bdd)) {
		cmn_err(CE_WARN,"en596 init_tx or init_rx failed");
		return(-1);
	}
	bdd->first_init = 0;

	/* relocate scp from default addr via 82596 PORT */
	en596port(bdd, scp_paddr);

	/* issue channel attention to start 596 init process */ 
	outl(bdd->ChanAtn, (ulong)0x0);

	/* wait for iscp busy to be cleared */
	for (i=1000; i; i--) {		
		if (!bdd->iscp.busy)
			break;
		drv_usecwait(100);
	}

	if (i == 0) {
		cmn_err(CE_WARN,"en596 iscp init failed");
		return(-1);
	} 

	for (i=1000; i; i--) {		/* wait for scb status */
		if (bdd->scb.status == (SCB_INT_CX | SCB_INT_CNA))
			break;
		drv_usecwait(10);
	}

	if (i == 0) {			/* if CX & CNA aren't set */
		cmn_err(CE_WARN,"en596 scb init failed");
		return(-1);
	}

	if ( en596ack_596(bdd) )
		return(-1);

	/* set bus throttle timers */
	bdd->scb.control = CU_START_TIMERS;

	outl(bdd->ChanAtn, (ulong)0x00);	 /* channel attention */
	drv_usecwait(10);

	for( i = 4096; i; i--) {
		drv_usecwait(10);	/* wait 10 usec */
		if(bdd->scb.status & TIMERS_LOADED)
			break;
	}

	if(i == 0) {
		cmn_err(CE_WARN,"en596 load bus throttle timers failed");
		return(-1);
	}

	if (en596ack_596(bdd))
		return(-1);

	/* configure 596 with default parameters */
       if (en596config_596(bd, PRO_OFF, LOOP_OFF)) {
                cmn_err(CE_WARN,"en596 board %d config failed", bd->bd_number);
                return (-1);
        }

        /* do IA Setup command */
        if (en596ia_setup_596(bd, bd->eaddr.bytes)) {
                cmn_err(CE_WARN,"en596 board %d ia_setup failed",bd->bd_number);
                return (-1);
        }

	/* enable 596 Receive Unit */
	bdd->scb.status		= 0;
	bdd->scb.control	= RU_START;
	bdd->scb.rfa_paddr	= vtop((caddr_t)bdd->rfd, NULL);

	outl(bdd->ChanAtn, (ulong)0);	/* channel attention */
	return (en596wait_scb(bdd, 1000));
}


/*************************************************************************/
STATIC int
en596init_tx (bdd)
bdd_t    *bdd;
{

	int    i,j;
	tcb_t  *tcb;
	tbd_t  *tbd;

	/* tcb's are now dynamically allocated to avoid spanning pages; */
	if ( bdd->first_init ) {
		if ((bdd->tcb = (tcb_t *)kmem_alloc_physreq(
				PAGESIZE, &en596physreq, KM_NOSLEEP)) == NULL)
			return -1;
	}

	tcb = bdd->tcb;

	for ( i=0 ; i < MAX_TCB ; i++, tcb++ ) {
		tcb->status		= 0;
		tcb->cmd    		= CS_CMD_XMIT|CS_EL|FLEX_MODE|CS_INTR;
		tcb->link   		= CS_NO_LINK;
		tcb->parms.tbd_paddr	= vtop((caddr_t)(&tcb->tbd), NULL);
		tcb->parms.tcb_count	= 0;
		tcb->parms.zeros	= 0;
		tcb->next		= tcb + 1;
		if (bdd->first_init)
                        if ((tcb->txbuf = (caddr_t)kmem_alloc_physreq(
                                XMTBUFSIZE, &en596physreq, KM_NOSLEEP)) == NULL)
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
 * en596init_rx (bdd)
 *   initialize i596 buffer configuration for receive side
 */

STATIC int
en596init_rx (bdd)
bdd_t     *bdd;
{
	int		i;
	rfd_t		*rfd;
	rbd_t		*rbd;


	rfd = bdd->rfd = (rfd_t *)((char *)bdd->tcb + (sizeof(tcb_t)*MAX_TCB));
	rbd = bdd->rbd = (rbd_t *)((char *)bdd->rfd + (sizeof(rfd_t)*MAX_RFD));

	bdd->begin_rfd = rfd;
	bdd->begin_rbd = rbd;

	bdd->rbd_paddr = vtop((caddr_t)rbd, NULL);

	/* init rfd to a circular list */

	for ( i = 0; i < MAX_RFD; i++, rfd++, rbd++ ) {
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

		if (bdd->first_init) {
			if ((rbd->rxbuf = kmem_alloc_physreq(
				RCVBUFSIZE, &en596physreq, KM_NOSLEEP)) == NULL)
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
en596diagnose_596(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	int		i;

	if (en596wait_scb(bdd, 1000))
		return (1);

	if (en596ack_596(bdd))
		return (1);

	if (en596wait_scb(bdd, 1000))
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
		cmn_err(CE_NOTE, "en596diagnose_596: diagnose failed");
		return(1);
	}

	if (en596ack_596(bdd))
		return (1);

	return (0);
}

/*************************************************************************/
int
en596config_596 (bd, prm_flag, loop_flag)
DL_bdconfig_t *bd;
ushort_t prm_flag;
ushort_t loop_flag;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;

	if (en596wait_scb(bdd, 1000))
		return (1);

	bdd->scb.status   = 0;
	bdd->scb.control  = CU_START;
	bdd->scb.cbl_paddr = vtop((caddr_t) &bdd->cb, NULL);

	bdd->cb.status	= 0;
	bdd->cb.cmd	= CS_CMD_CONF | CS_EL;
	bdd->cb.link	= CS_NO_LINK;

	bdd->cb.parm.conf.cnf_fifo_byte	= 0xca0e;
	bdd->cb.parm.conf.cnf_add_mode	= 0x2e00 | (loop_flag?0x4000:0x0000);
	bdd->cb.parm.conf.cnf_pri_data	= 0x6000;
	bdd->cb.parm.conf.cnf_slot	= 0xf200;
	bdd->cb.parm.conf.cnf_hrdwr	= 0x0004 | (prm_flag?0x0001:0x0000);
	bdd->cb.parm.conf.cnf_min_len	= 0xff40;
	bdd->cb.parm.conf.cnf_dcr_num	= 0x3f00;

	outl(bdd->ChanAtn, (ulong)0);		 /* channel attention */

	if (en596wait_scb(bdd, 1000))
		return (1);

	if (en596ack_596 (bdd))
		return (1);

	return (0);
}

/*************************************************************************/
int
en596ia_setup_596 (bd, eaddr)
DL_bdconfig_t *bd;
uchar_t eaddr[];
{
	bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
	ushort	i;
	
	if (en596wait_scb(bdd, 1000))
		return (1);

	bdd->scb.status		= 0;
	bdd->scb.control	= CU_START;
	bdd->scb.cbl_paddr	= vtop((caddr_t) &bdd->cb, NULL);
	
	bdd->cb.status		= 0;
	bdd->cb.cmd		= CS_CMD_IASET | CS_EL;

	for (i=0; i<6; i++)
	    bdd->cb.parm.ia_set[i] = eaddr[i];

	outl(bdd->ChanAtn, (ulong)0);		 /* channel attention */

	if (en596wait_scb(bdd, 1000))
		return (1);

	if (en596ack_596(bdd))
		return (1);

	return (0);
}

/*************************************************************************
 * en596wait_scb (bdd, how_long)
 *
 * Acceptance of a Control Command is indicated by the 82596 clearing
 * the SCB command field page 2-16 of the intel microcom handbook.
 */
int
en596wait_scb (bdd, n)
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
en596ack_596 (bdd)
bdd_t	*bdd;
{
	if ( bdd->scb.control = (bdd->scb.status & SCB_INT_MSK) ) {	
		outl(bdd->ChanAtn, (ulong)0);	/* channel attention */

		if (en596wait_scb (bdd, 10000))
			return -1;

		if (bdd->brd_type == EMBEDDED)		/* Embedded 82596 */
			inb(bdd->Control0);
		else {					/* EISA boards */
			uchar_t reg0;

			if ((reg0 = inb(bdd->Control0)) & PLX_R0_LSTAT)
				outb(bdd->Control0, reg0);
		}
	}
	return (0);
}

/*************************************************************************/
void
en596port(bdd,value)
bdd_t	*bdd;
ulong	value;
{
	if (bdd->Port596 < 0x1000) {
		outl(bdd->Port596, value);
		drv_usecwait(5);
		outl(bdd->Port596, value);
	}
	else {
		/* this is an ENEISA board.  The PLX interfaces takes a 32
		   bit value and feeds it to the port 16 bits at a time. */
		outl(bdd->Port596, value);
	}
	drv_usecwait(10);
}
		
/*************************************************************************/
STATIC void
en596free_rxbufs(bdd)
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
en596free_txbufs(bdd)
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
en596uninit()
{
	int		board;
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	DL_sap_t        *sap;
	int		i;
	int     	opri;


	untimeout(en596timer_id);

	if (!cookie || !DLconfig)	return;

	for (board = 0; board < DLboards; board++) {

		bd = &DLconfig[board];

		if (bd->bd_lock)
			opri = DLPI_LOCK( bd->bd_lock, plstr );

		en596irq_group[bd->irq_level].bd[0] = 0;

		bd->flags = 0;

		if ( bdd = (bdd_t *)bd->bd_dependent1 ) {

			bdd->head_tcb = (tcb_t *)NULL;
			bdd->scb.control = (CU_ABORT | RU_ABORT);
			outl(bdd->ChanAtn, (ulong)0);
			en596wait_scb(bdd, 10000);

			if (bdd->brd_type == EMBEDDED) {
				inb(bdd->Control0);
				outb(bdd->Control0,1);	/* reset 82586 */
			}
			else {
				uchar_t reg0;

				if ((reg0 = inb(bdd->Control0)) & PLX_R0_LSTAT)
					outb(bdd->Control0, reg0);
				en596port(bdd, 0);	/* reset 82596 */
			}

			cm_intr_detach(cookie[board]);

			/* make sure free_rxbufs first */
			en596free_rxbufs(bdd);
			en596free_txbufs(bdd);
			kmem_free(bd->bd_dependent1, sizeof(bdd_t));
		}

		if (bd->ifstats) {
			(void)ifstats_detach(bd->ifstats);

			kmem_free(bd->ifstats, sizeof(struct ifstats));
		}

		if (sap = bd->sap_ptr) {
			for (i = 0; i < bd->max_saps; i++, sap++) {
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
		kmem_free(DLsaps, sizeof(DL_sap_t)*DLboards*en596nsaps);

       	kmem_free(cookie, (sizeof(void **) * DLboards));
       	kmem_free(DLconfig, (sizeof(DL_bdconfig_t) * DLboards));
}


void
en596init_board(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd	= (bdd_t *) bd->bd_dependent1;
	ulong  	base_io = bd->io_start;
	uchar_t inval;


	if (bdd->first_init)
		cmn_err(CE_CONT,
			"%s Network Controller in slot %d is configured as:\n",
				en596brd_id[bdd->brd_type], bdd->slot);

	/* do a reset */
	inval = inb(base_io+PLX_R3);
	outb(base_io+PLX_R3, inval | PLX_R3_RESET);
	drv_usecwait(1000);
	outb(base_io+PLX_R3, inval & ~PLX_R3_RESET);

	/* burst transfer enable */
	inval = inb(base_io+PLX_R3);
	outb(base_io+PLX_R3, inval | PLX_R3_BURST);

	/* Do NOT set the following write Burst Mode bit!!
	 * it does not necessarily work in every EISA platform
	 */
	inval = inb(base_io+PLX_R3);
	outb(base_io+PLX_R3, (inval & ~PLX_R3_WBURST));

	/*
	 * 32 bit master and slave data size, unlatched interrupt mode,
	 * level triggered interrupts
	 */
	inval  = (inb(base_io+PLX_R0)) & PLX_R0_IRQ_BITS;
        inval &= ~PLX_R0_LATCH;                         /* unlatched */
	inval |= (PLX_R0_M_SIZE | PLX_R0_S_SIZE);	/* 32-bit data size */
	inval |=  PLX_R0_PREEMPT;			/* 55 BCLK preempt */
	outb(base_io+PLX_R0, inval);

	/* check the configured cable type */
	inval  = inb(base_io+PLX_R1);
	inval &= 0x0F;
	if (bdd->first_init) {
		cmn_err(CE_CONT, "en596: ");

		/* configure the cable type */
		switch (inval) {
	    	case 0x00:
			if (bdd->brd_type == UNB596)
				cmn_err(CE_CONT, "AUI/TP ");
			else
				cmn_err(CE_CONT, "(BNC) ");
			break;
	    	case 0x01:
			if (bdd->brd_type != UNB596)
				cmn_err(CE_CONT, "(AUI) ");
			break;
	    	case 0x02:
			if (bdd->brd_type == UNB596)
				cmn_err(CE_CONT, "(BNC) ");
			break;
	    	case 0x03:
			if (bdd->brd_type != UNB596)
				cmn_err(CE_CONT, "(TP) ");
			break;
		}
	}
	outb(base_io+PLX_R1, ((inval & ~PLX_R1_ISA_IO) | PLX_R1_D_ISA_IO));

	/* force prom disabled */
	outb(base_io+PLX_R2, PLX_R2_DISABLED);

	/* enable expansion board */
	outb(base_io+PLX_EBC, PLX_EBC_EBE);
	return;
}


void
en596init_chip(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd   = (bdd_t *) bd->bd_dependent1;
	ulong	ioport = 0x28;
	uchar_t inval;


	if (bdd->first_init)
		cmn_err(CE_CONT,
			"Embeded 596 Network Controller is configured as:\n");

	inval = (inb(ioport)) & 0xc0;

	switch(inval) {
            case 0x00:
                        bd->irq_level = 9;
                        break;
            case 0x40:
                        bd->irq_level = 10;
                        break;
            case 0x80:
                        bd->irq_level = 14;
                        break;
            case 0xc0:
                        bd->irq_level = 15;
                        break;
	}
}


STATIC int
en596chk_slot(slot_num)
int slot_num;
{
	int	i;
	int	slot_address;
	uchar_t	eisa_id[EISA_ID_LEN];
	uchar_t	prod_id[OEM_ID_LEN];
        cm_args_t       cm_args;


	if (slot_num == 0)
		return(EMBEDDED);
	else {
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
	}

	i = 0;
	while ( strlen(en596oem_id[i]) ) {
		if (strcmp(en596oem_id[i], (char *)prod_id) == NULL)
			return(i);
		i++;
	}
	return(-1);
}
