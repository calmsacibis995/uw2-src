/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*                      Copyright (c) 1991  Intel Corporation           */
/*                              All Rights Reserved                     */

/*                      INTEL CORPORATION PROPRIETARY INFORMATION       */

/*      This software is supplied to AT & T under the terms of a license   */
/*      agreement with Intel Corporation and may not be copied nor         */
/*      disclosed except in accordance with the terms of that agreement.   */

#ident	"@(#)kern-i386at:io/dlpi_ether/flash32/flash32hrdw.c	1.4"
#ident	"$Header: $"

/*
 *  This file contains all of the hardware dependent code for the 82596.
 *  It is the companion file to ../../io/dlpi_ether.c
 */
#ifdef _KERNEL_HEADERS

#include <io/dlpi_ether/flash32/dlpi_flash32.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/flash32/flash32.h>
#include <io/strlog.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#ifdef ESMP
#include <util/ksynch.h>
#include <net/inet/if.h>
#else
#include <net/tcpip/if.h>
#endif
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/inline.h>
#include <io/ddi.h>

#else

#include <sys/dlpi_flash32.h>
#include <sys/dlpi_ether.h>
#include <sys/flash32.h>
#include <sys/strlog.h>
#ifdef ESMP
#include <sys/ksynch.h>
#endif
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/dlpi.h>
#include <net/if.h>
#include <sys/errno.h>
#include <sys/cmn_err.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/debug.h>
#include <sys/inline.h>
#include <sys/ddi.h>

#endif

extern	DL_bdconfig_t	*DLconfig;
extern	physreq_t	flash32physreq;
extern	DL_irq_t	flash32irq_group[];
extern	int		DLstrlog;
extern	int		flash32boards;
extern	int		flash32timer_id;

extern	int	flash32wait_scb();
extern	int	flash32init_596();
extern  void    flash32port();
extern	ushort	ntohs();
extern  ushort  flash32config_596();

STATIC	void	flash32tx_done();
STATIC	void	flash32sched();
STATIC	void	flash32ru_restart();
STATIC  int     disable_board();
STATIC  int     enable_board();
STATIC  int     reset_board();

void	flash32bdspecioctl(), flash32bdspecopen();
void	flash32watchdog();
void	flash32intr();
void    flash32free_fcn();
int	flash32xmit_packet();

#define BCOPY(from, to, len)    bcopy((caddr_t)(from), (caddr_t)(to), \
								(size_t)(len))
#define BCMP(s1, s2, len)       bcmp((char *)(s1), (char *)(s2), \
								(size_t)(len))

#ifdef TEMP

/* flash32bdspecopen is called from DLopen->dlpi_ether.c */
/* ARGSUSED */

void
flash32bdspecopen(q)
queue_t *q;
{
	DL_bdconfig_t   *bd;
	DL_sap_t        *sap;
	int             cnt = 0;
	int             i;
	int		opri;

	if (q->q_ptr) {

		bd  = ((DL_sap_t *)(q->q_ptr))->bd;

                opri = DLPI_LOCK( bd->bd_lock, plstr );

                /*
                 *  if first stream open to this device then restart the device
		 */
                for (i = 0, sap = bd->sap_ptr; i < bd->max_saps; i++, sap++)
                        if ( sap->read_q ) cnt++;

		if (cnt == 1)
			enable_board(bd);

		bd->flags = BOARD_PRESENT;

                DLPI_UNLOCK(bd->bd_lock, opri);
	}
}

/* flash32bdspecclose is called from DLclose->dlpi_ether.c */
/* ARGSUSED */
void
flash32bdspecclose(q)
queue_t *q;
{
        DL_bdconfig_t   *bd;
        DL_sap_t        *sap;
        int             cnt = 0;
        int             i;
        int             opri;


        if (q->q_ptr) {

                bd  = ((DL_sap_t *)(q->q_ptr))->bd;
                opri = DLPI_LOCK( bd->bd_lock, plstr );

		flushq (WR(q), FLUSHDATA);
                /*
                 * if last stream close to this device then shut-off the device
                 */
                for (i = 0, sap = bd->sap_ptr; i < bd->max_saps; i++, sap++)
                        if ( sap->read_q ) cnt++;

                if (cnt == 0)
                        disable_board(bd);

                q->q_ptr = (char *)NULL;
                OTHERQ(q)->q_ptr = (char *)NULL;

                DLPI_UNLOCK(bd->bd_lock, opri);
        }
}

#endif

/* flash32bdspecioctl is called from DLioctl->dlpi_ether.c */
void
flash32bdspecioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
	struct iocblk *ioctl_req= (struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(q, mp);
} /* end of flash32bdspecioctl */


/*	This driver implements semi-dynamic chaining. flash32xmit_packet() is
 *	called to create a new tcb.
 *	If the tcb ring pointed at by bdd->head_tcb is not NULL, i.e.
 *	the waiting tcb list is not empty, then the tcb is added to the end of
 *	the waiting list pointed by bdd->begin_tcb->prev->next.
 *	Otherwise the 82596 is idle so we start a new transmit command.
 *
 *	In the interrupt routine which is entered after a command chain is
 *	completed, we check if there is a waiting list.  If so then the waiting
 *	list is made the new command chain and the 596 is restarted.  Otherwise
 *	the 596 just goes idle.
 */

/******************************************************************************
 *  flash32xmit_packet()
 *
 *  This routine is called from DLunitdata_req(). It assumes
 *  we are at STREAMS spl interrupt level.
 */

flash32xmit_packet (bd, mp, sap)
DL_bdconfig_t *bd;
mblk_t	*mp;
DL_sap_t *sap;
{
	/* LINTED pointer alignment */
	bdd_t		*bdd	= (bdd_t *) bd->bd_dependent1;
	tcb_t		*p_tcb;
	mblk_t		*mp_tmp;
	caddr_t		txbuf;
	int		start_cu = 0;
	int		tx_size  = 0;
	int		msg_size;

	/*
	 * the bdd->head_tcb always points to the first unacknowledged cmd.
	 * the bdd->tail_tcb always points to the current end of CBL.
	 */

	/* If tcb ring is empty */
	if (bdd->head_tcb == (tcb_t *)NULL) {
		bdd->head_tcb = bdd->tail_tcb = bdd->tcb;
		start_cu = 1;
	}

	/*
	 * else there's at least one command pending in ring.
	 * Note that: we don't do sanity check of whether the tx_ring is full
	 * or not. When the queue is full, we flag the interface to TX_BUSY.
	 * We trust the dlpi_ether.c to put the next messages into queue and
	 * we will pick them up later in the flash32sched().
	 */
	else {
		bdd->tail_tcb = bdd->tail_tcb->next;

		if (bdd->tail_tcb->next == bdd->head_tcb)
			bd->flags |= TX_BUSY;
	}
	p_tcb = bdd->tail_tcb;

	/*
	 * fill in 82596's transmit command block
	 */
	p_tcb->status   = 0;
	p_tcb->cmd	= CS_CMD_XMIT | CS_EL | FLEX_MODE | CS_INTR;
	p_tcb->link	= CS_NO_LINK;
	txbuf           = p_tcb->txbuf;

	/*
         * copy STREAMS message to tbd data buffer
         */
        for ( mp_tmp = mp; mp_tmp; mp_tmp = mp_tmp->b_cont ) {
                msg_size = mp_tmp->b_wptr - mp_tmp->b_rptr;
                BCOPY(mp_tmp->b_rptr, txbuf, msg_size);
                tx_size += msg_size;
                txbuf   += msg_size;
        }
        p_tcb->tbd.count = (tx_size | CS_EOF);
        freemsg(mp);

        bd->mib.ifOutOctets += tx_size;
	bd->ifstats->ifs_opackets++;

	/*
	 * if cu is idle, start the 82596, issue a channel attention
	 */
	if ( start_cu ) {
		if (flash32wait_scb (bdd, 1000)) {
			cmn_err(CE_WARN,
				"flash32xmit_packet: scb command not cleared");
			bd->timer_val = 0;	/* force board to reset */
			return (-1);
		}
		bdd->scb.control   = CU_START;
		bdd->scb.cbl_paddr = vtop((caddr_t)bdd->tail_tcb, NULL);
		outl (bdd->ChanAtn, (ulong)0);
		bd->timer_val = 2; 
	}
	return (0);
}

/******************************************************************************
 *  flash32intr()
 */

void
flash32intr (irq)
int	irq;
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	ushort		scb_status;
	uchar_t		reg0;
	int             i, opri;
	int		bailout = 0;


	/* map irq level to the proper board. Make sure it's ours */
	for (i = 0; i < DLboards; i++) {

		if ((bd = flash32irq_group[irq].bd[i]) == NULL) {
#ifdef DEBUG
			if (i == 0)
	            		cmn_err (CE_WARN, "flash32 spurious interrupt");
#endif
	            	return;
	    	}

	    	bdd = (bdd_t *)bd->bd_dependent1;

#ifdef ESMP
	    	opri = DLPI_LOCK( bd->bd_lock, plstr );
#endif

loop_intr:
		while (1) {

	    		scb_status = bdd->scb.status;

	        	if( (scb_status & SCB_INT_MSK) == NULL )
	                        break;

			if (flash32wait_scb(bdd, 1000)) {
	                        cmn_err(CE_WARN, "flash32intr cmd not cleared");
	                        bd->timer_val = 0;      /* force to reset */
	                        goto exit_intr;
			}
#ifdef DEBUG
			if (scb_status & SCB_INT_RNR) {
				if ((scb_status & RU_MASK) == RU_IS_READY) {
					scb_status &= ~SCB_INT_RNR;
					bailout++;
				}
			}

			if (scb_status & SCB_INT_CNA) {
				if ((scb_status & SCB_CUS_MSK) == SCB_CUS_ACTV){
					scb_status &= ~SCB_INT_CNA;
					bailout++;
				}
			}
#endif
	                /* acknowledge interrupt with a channel attention */
	                bdd->scb.control = scb_status & SCB_INT_MSK;
	                outl(bdd->ChanAtn, (ulong)0);

#ifdef DEBUG
			if (bailout) {
				cmn_err(CE_WARN, "flash32 bailout %x",
							bdd->scb.status);
#ifdef ESMP
				DLPI_UNLOCK( bd->bd_lock, opri );
#endif
				return;
			}
#endif
	                /* handle receive before transmit; */
	                if (scb_status & (SCB_INT_FR | SCB_INT_RNR))
	                        flash32chk_ring(bd);

	                if (bdd->head_tcb && (scb_status & SCB_INT_CNA)) {
			    flash32tx_done(bd);

	                    if ((bd->flags & (TX_QUEUED|TX_BUSY)) == TX_QUEUED)
	                            flash32sched (bd);
	                }
	        }
exit_intr:
		if ((reg0 = inb(bdd->Control0)) & PLX_R0_LSTAT)
			outb (bdd->Control0, reg0);

		if (inb(bdd->Control0) & PLX_R0_LSTAT)
			goto loop_intr;

		if ((reg0 = inb(bd->io_start+PLX_R0)) & PLX_R0_LSTAT)
			outb(bd->io_start+PLX_R0, reg0);

#ifdef ESMP
		DLPI_UNLOCK( bd->bd_lock, opri );
#endif
    }
}


STATIC void
flash32tx_done ( DL_bdconfig_t *bd )
{
	/* LINTED pointer alignment */
	bdd_t		*bdd = (bdd_t *) bd->bd_dependent1;
	tcb_t		*tcb;
	scb_t		*scb = &bdd->scb;
	ushort		status;


	/* if ring is empty, we have nothing to process */
	if (bdd->head_tcb == 0)
		return;
	do {
		tcb	= bdd->head_tcb;
		status	= tcb->status;		/* xmit status */

		/* the following condition should never happen, just in case. */
		if ( !(status & CS_COMPLETE) ) {
			if( scb->status & SCB_INT_CNA )
				goto start_cu;
			else
				return;
		}

		if ( !(status & CS_OK) ) {
			bd->mib.ifOutErrors++;
			bd->ifstats->ifs_oerrors++;

			if (status & CS_S8)
                                bd->mib.ifSpecific.etherUnderrunErrors++;

			if (status & CS_CARRIER)
				bd->mib.ifSpecific.etherCarrierLost++;

			if (status & CS_COLLISIONS) {
				bd->ifstats->ifs_collisions =
				++bd->mib.ifSpecific.etherCollisions;
			}
		}
		if ( tcb->cmd & (CS_INTR | CS_EL) )
			break;
		bdd->head_tcb = bdd->head_tcb->next;
	} while (1);

        /* Reset timer_val as transmit is complete */
        bd->timer_val = -1;
	bd->flags &= ~TX_BUSY;

	/* if last command in tcb list */
	if (bdd->head_tcb == bdd->tail_tcb) {
		bdd->head_tcb = bdd->tail_tcb = (tcb_t *)NULL;
		return;
	}
	else {
		/* there are more commands pending to be xmit */
		tcb = bdd->head_tcb = bdd->head_tcb->next;
		while (tcb != bdd->tail_tcb) {
			tcb->link = vtop((caddr_t)tcb->next, NULL);
			tcb->cmd &= ~(CS_EL | CS_INTR);
			tcb = tcb->next;
		}
start_cu:
		if (flash32wait_scb (bdd, 1000)) {
			cmn_err(CE_WARN,
				"flash32tx_done: scb command not cleared");
			bd->timer_val = 0;	/* force board to reset */
			return;
		}
		bd->timer_val = 2;
		scb->control = CU_START;
		scb->cbl_paddr = vtop((caddr_t)bdd->head_tcb, NULL);
		outl (bdd->ChanAtn, (ulong)0);
	}
}


STATIC void
flash32sched( bd ) 
DL_bdconfig_t *bd;
{
	bdd_t	  *bdd = (bdd_t *) bd->bd_dependent1;
	DL_sap_t  *sap = bdd->next_sap;
	mblk_t	  *mp;
	int	  i;

	/*
	 *  Indicate outstanding transmit is done and
	 *  see if there is work waiting on our queue.
	 */
	for ( i = bd->ttl_valid_sap; i; i-- ) {

		if ( sap == (DL_sap_t *)NULL )
			sap = bd->valid_sap;

		if ( sap->state == DL_IDLE ) {
			while ( mp = getq(sap->write_q) ) {
				(void) flash32xmit_packet(bd, mp, sap);
				bd->mib.ifOutQlen--;
				if( bd->flags & TX_BUSY ) {
					bdd->next_sap = sap->next_sap;
					return;
				}
			}
		}
		sap = sap->next_sap;
	}

	/*
	 *  Nobody's left to service, make the queue empty.
	 */
	bd->flags &= ~TX_QUEUED;
	bd->mib.ifOutQlen = 0;
	bdd->next_sap = (DL_sap_t *)NULL;
}

/******************************************************************************/

flash32chk_ring( bd )
DL_bdconfig_t   *bd;
{
    bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
    rfd_t	*rfd;
    rbd_t	*rbd;
    frtn_t      *frtn;
    mblk_t	*mp;
    int		len;
    caddr_t     buf, nbuf;


    rfd = bdd->begin_rfd;

    /* check to see if a rbd is attached to the rfd */
    while (rfd->rbd_paddr != CS_NO_LINK) {

	rbd = &bdd->rbd[(rbd_t *)rfd->rbd_paddr - (rbd_t *)bdd->rbd_paddr];

	/* to see frame reception is still going on */
	if (rfd->status & CS_BUSY)
	    break;

	/*
	 * we programmed 82596 not to save bad frames, so, the
	 * chances for rfd status is not OK are RU out of resource.
	 */
	if ((rfd->status & CS_OK) && (rbd->status & CS_EOF)) {
	    /*
	     * if end-of-frame bit is not set, we must have received an
	     * oversized frame. Treat it as an error and discard it.
	     */
	    if ((nbuf = kmem_alloc_physreq(
                        RCVBUFSIZE, &flash32physreq, KM_NOSLEEP)) == NULL) {
#ifdef DEBUG
                cmn_err(CE_WARN, "flash32chk_ring: alloc failed.");
#endif
                bd->mib.ifInDiscards++;                 /* SNMP */
                bd->mib.ifSpecific.etherRcvResources++; /* SNMP */
                /* discarding the frame */
            }
            else {
                buf = rbd->rxbuf;       /* containing frame just received */
                len = rbd->status & CS_RBD_CNT_MSK;

                frtn = (frtn_t *)(buf + (RCVBUFSIZE - sizeof(frtn_t)));
                frtn->free_func = flash32free_fcn;
                frtn->free_arg  = (void *)buf;

                if (mp = esballoc((void *)buf, RCVBUFSIZE, BPRI_HI, frtn)) {
                    mp->b_wptr = mp->b_rptr + len;

                    /* swap the msgb out with new one */
                    rbd->rxbuf   = nbuf;
                    rbd->bufaddr = vtop(rbd->rxbuf, NULL);

                    if (!DLrecv(mp, bd->sap_ptr)) {
                        bd->mib.ifInOctets += len;
                        bd->ifstats->ifs_ipackets++;
                    }
                }
                else
                    kmem_free (nbuf, RCVBUFSIZE);
            }
	}
	else {
	    /*
	     *+ RU may be running out of resources or recv errors, or received
	     *  oversized frame. Discarding frame and reclaiming the rbd's.
	     */
	    cmn_err(CE_WARN, "flash32: rcv error %x", rfd->status);
	    bd->mib.ifInDiscards++;     /* SNMP */
	}
	rfd->status = 0;
	rfd->rbd_paddr = CS_NO_LINK;
	rfd = rfd->next;
    }

    /* re-queue rbd */
    bdd->begin_rbd->prev->bufsize = RCVBUFSIZE;	/* clear old end-of-list */
    bdd->begin_rbd = rbd;
    rbd->prev->bufsize = RCVBUFSIZE | CS_EL;	/* set new end-of-list */

    /* re-queue rfd */
    bdd->begin_rfd->prev->control = FLEX_MODE;	/* clear EL */
    bdd->begin_rfd = rfd;
    rfd->prev->control = (CS_EL | FLEX_MODE);

    if (bdd->scb.status & RU_IS_READY)
	return;

    flash32ru_restart(bd);
}

/********************************************************************/
STATIC void
flash32ru_restart(bd)
DL_bdconfig_t   *bd;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	rfd_t	*begin_rfd;
	int	i;


	begin_rfd = bdd->begin_rfd;

	/*
	 * if the RU just went not ready and it just completed an rfd --
	 * do NOT restart RU -- this will wipe out the just completed rfd.
	 * There will be a second interrupt that will remove the rfd via
	 * flash32chk_ring () and thus calls flash32ru_restart() which will
	 * then start the RU if necessary.
	 */
	if (begin_rfd->status & CS_COMPLETE)
		return;

	/*
	 * if we get here, then RU is not ready and
	 * no completed fd's are avail. therefore, start RU
	 */
	if (flash32wait_scb(bdd,1000)) {
		cmn_err (CE_WARN, "flash32: ru-restart, scb cmd not cleared");
		bd->timer_val = 0;	/* force board to reset */
		return;
	}

	begin_rfd->rbd_paddr = vtop((caddr_t)bdd->begin_rbd, NULL);
	bdd->scb.rfa_paddr   = vtop((caddr_t)begin_rfd, NULL);
	bdd->scb.control     = RU_START;
	outl(bdd->ChanAtn,(ulong) 0);
	return;
}

void
flash32free_fcn(buf)
caddr_t buf;
{
        kmem_free(buf, RCVBUFSIZE);
}


/******************************************************************************
 * flash32promisc_on (bd)
 */

flash32promisc_on (bd)
DL_bdconfig_t	*bd;
{
        int     rval;
	int	opri;


        opri = DLPI_LOCK( bd->bd_lock, plstr );

	/* If already in promiscuous mode, just return */
	if (bd->promisc_cnt++)
		rval = 0;
	else
                rval = flash32config_596 (bd, PRO_ON, LOOP_OFF);

	DLPI_UNLOCK( bd->bd_lock, opri );
        return (rval);
}


/******************************************************************************
 * flash32promisc_off (bd)
 */

flash32promisc_off (bd)
DL_bdconfig_t	*bd;
{
        int     rval;
	int	opri;

	/* return if the board is not in a promiscuous mode */
	if (!bd->promisc_cnt)
		return (0);

        opri = DLPI_LOCK( bd->bd_lock, plstr );

	/* return if this is not the last promiscuous SAP */
	if (--bd->promisc_cnt)
		rval = 0;
        else
		rval = flash32config_596 (bd, PRO_OFF, LOOP_OFF);

	DLPI_UNLOCK( bd->bd_lock, opri );
	return (0);
}

#ifdef ALLOW_SET_EADDR
/******************************************************************************
 *  flash32set_eaddr()
 */
int
flash32set_eaddr (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
	int opri;
	int val;

	opri = DLPI_LOCK( bd->bd_lock, plstr );

	BCOPY( eaddr, bd->eaddr.bytes, DL_MAC_ADDR_LEN );

	/* do IA Setup command */
	val = flash32ia_setup_596(bd, bd->eaddr.bytes);

	DLPI_UNLOCK( bd->bd_lock, opri );
	return(val);
}
#endif	/* ALLOW_SET_EADDR */


/******************************************************************************
 *  flash32is_multicast()
 */

/*ARGSUSED*/
flash32is_multicast(bd, eaddr)
DL_bdconfig_t *bd;
DL_eaddr_t *eaddr;
{
    bdd_t       *bdd = (bdd_t *)bd->bd_dependent1;
    mcat_t      *mcp = &bdd->flash32_multiaddr[0];
    int         i;
    int         rval = 0;

    if (bd->multicast_cnt) {
        if (eaddr->bytes[0] & 0x1) {
            for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
                if ((mcp->status) &&
                    (BCMP(eaddr->bytes, mcp->entry, DL_MAC_ADDR_LEN) == 0)) {
                    rval = 1;
                    break;
                }
            }
        }
    }
    return (rval);
}


/******************************************************************************
 *  flash32get_multicast()
 */
int
flash32get_multicast(bd,mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
	bdd_t	*bdd	= (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp	= &(bdd->flash32_multiaddr[0]);
	uchar_t *dp;
	int i;
	int found = 0;


	if((int)(mp->b_wptr - mp->b_rptr) == 0)
		found = bd->multicast_cnt;
	else {
		dp = mp->b_rptr;

		for (i = 0;(i < MULTI_ADDR_CNT) && (dp < mp->b_wptr);i++,mcp++)
			if (mcp->status) {
				BCOPY(mcp->entry, dp, DL_MAC_ADDR_LEN);
				dp += DL_MAC_ADDR_LEN;
				found++;
			}
		mp->b_wptr = dp;
	}
	return found;
}

/******************************************************************************
 *  flash32set_multicast()
 */
int
flash32set_multicast(bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t 	*mcp = &(bdd->flash32_multiaddr[0]);
	int i, j;

	if (flash32wait_scb(bdd, 1000))
		return (1);

	bdd->scb.status   = 0;
	bdd->scb.control  = CU_START;
	bdd->scb.cbl_paddr = vtop((caddr_t)(cb_t *) &bdd->cb, NULL);

	bdd->cb.status	= 0;
	bdd->cb.cmd	= CS_CMD_MCSET | CS_EL;
	bdd->cb.link	= CS_NO_LINK;

	for (i =0,j = 0;i < MULTI_ADDR_CNT;i++,mcp++) {
		if (mcp->status) {
			BCOPY(mcp->entry, &(bdd->cb.parm.mcad.mc_addr[j]), DL_MAC_ADDR_LEN);
			j += DL_MAC_ADDR_LEN;
		}
	}
	bdd->cb.parm.mcad.mc_cnt = j;

	outl(bdd->ChanAtn, (ulong)0);	 /* channel attention */

	if (flash32wait_scb(bdd, 1000))
		return (1);

	if (flash32ack_596(bdd))
		return (1);

	return (0);
}

/*******************************************************************************
 *  flash32add_multicast()
 */
int
flash32add_multicast(bd, maddr)
DL_bdconfig_t *bd;
DL_eaddr_t *maddr;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t 	*mcp = &(bdd->flash32_multiaddr[0]);
	int	i, rval;
	int	opri;

        if ((bd->multicast_cnt >= MULTI_ADDR_CNT) || (!(maddr->bytes[0] & 0x1)))
		return 1;

	if (!flash32is_multicast(bd,maddr))
		return 1;

        opri = DLPI_LOCK( bd->bd_lock, plstr );

	for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
		if (!mcp->status)
			break;
	}
	mcp->status = 1;
	bd->multicast_cnt++;
	BCOPY(maddr->bytes, mcp->entry, DL_MAC_ADDR_LEN);
	rval = flash32set_multicast(bd);
	DLPI_UNLOCK( bd->bd_lock, opri );
	return rval;
}

/*******************************************************************************
 *  flash32del_multicast()
 */
int
flash32del_multicast(bd, maddr)
DL_bdconfig_t *bd;
DL_eaddr_t *maddr;
{
	bdd_t	*bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t 	*mcp = &(bdd->flash32_multiaddr[0]);
	int	i, rval;
	int	opri;


	if (!flash32is_multicast(bd,maddr))
		 return 1;

	for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++)
		if ( (mcp->status) &&
			(BCMP(maddr->bytes, mcp->entry, DL_MAC_ADDR_LEN) == 0) )
                       	break;

	opri = DLPI_LOCK( bd->bd_lock, plstr );
	mcp->status = 0;
	bd->multicast_cnt--;
	rval = flash32set_multicast(bd);

	DLPI_UNLOCK( bd->bd_lock, opri );
	return rval; 
}

STATIC int
disable_board (bd)
DL_bdconfig_t   *bd;
{
        bdd_t    *bdd = (bdd_t *)bd->bd_dependent1;
        DL_sap_t *sap;
	uchar_t	  reg0;

        if (bd->flags & BOARD_DISABLED)
		return(0);

        bd->timer_val = -1;
	bdd->head_tcb = (tcb_t *)NULL;

	/* mask off irq from FLEA, Force low */
	reg0 = inb (bdd->Control0) | 0x20;
	outb (bdd->Control0, reg0);

        bdd->scb.control = (CU_ABORT | RU_ABORT);
        outl(bdd->ChanAtn,(ulong) 0);

        if (flash32wait_scb(bdd, 1000)) {
                cmn_err(CE_WARN, "flas32 reset command failed");
                return -1;
        }

	flash32ack_596 (bdd);

        sap = bd->valid_sap;
        while (sap) {
                if ((sap->write_q != NULL) && (sap->state == DL_IDLE))
                        flushq(sap->write_q, FLUSHDATA);
                sap = sap->next_sap;
        }

	bd->flags |= BOARD_DISABLED;

        return (0);
}


/*******************************************************************************
 *  flash32disable()
 */
/* ARGSUSED */
flash32disable (bd)
DL_bdconfig_t	*bd;
{
	int	ret;
	int     opri;


        opri = DLPI_LOCK( bd->bd_lock, plstr );
	ret = disable_board(bd);
	DLPI_UNLOCK( bd->bd_lock, opri );
	return (ret);
}


STATIC int
enable_board(bd)
DL_bdconfig_t   *bd;
{
        bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
	rfd_t	*begin_rfd;
	uchar_t	reg0;


	begin_rfd = bdd->begin_rfd;

        if (bd->flags & BOARD_DISABLED) {

		/* enable irq from FLEA */
		reg0 = inb (bdd->Control0) & ~0x20;
		outb (bdd->Control0, reg0);

		flash32wait_scb(bdd, 10000);

		begin_rfd->rbd_paddr = vtop((caddr_t)bdd->begin_rbd, NULL);
		bdd->scb.rfa_paddr   = vtop((caddr_t)begin_rfd, NULL);
		bdd->scb.control     = RU_START;
		outl(bdd->ChanAtn,(ulong) 0);

                bd->flags = BOARD_PRESENT;
        }
        return (0);
}

/******************************************************************************
 *  flash32enable()
 */
/* ARGSUSED */
flash32enable (bd)
DL_bdconfig_t	*bd;
{
	int	ret;
        int     opri;

        opri = DLPI_LOCK( bd->bd_lock, plstr );
	ret = enable_board(bd);
        DLPI_UNLOCK( bd->bd_lock, opri );
	return (ret);
}

STATIC int
reset_board(bd)
DL_bdconfig_t   *bd;
{
        bdd_t    *bdd = (bdd_t *)bd->bd_dependent1;
        DL_sap_t *sap;


        bd->timer_val = -1;
        bdd->head_tcb = (tcb_t *)NULL;

        sap = bd->valid_sap;
        while (sap) {
                if ((sap->write_q != NULL) && (sap->state == DL_IDLE)) {
                        flushq(sap->write_q, FLUSHDATA);
                        flushq(sap->read_q, FLUSHDATA);
		}
                sap = sap->next_sap;
        }

        bd->flags = 0;

        if (flash32init_596(bd)) {
                cmn_err(CE_WARN, "flash32 board %d init failed", bd->bd_number);
                return (-1);
        }
        bd->flags = BOARD_PRESENT;
        return (0);
}

/******************************************************************************
 *  flash32reset()
 */
flash32reset (bd)
DL_bdconfig_t	*bd;
{
	int       ret;
	int       opri;


	opri = DLPI_LOCK( bd->bd_lock, plstr );
	ret = reset_board(bd);
	DLPI_UNLOCK( bd->bd_lock, opri );
	return(ret);
}


void
flash32watch_dog ()
{	
    DL_bdconfig_t	*bd;
    bdd_t		*bdd;
    scb_t		*scb;
    int			i;
    int			ierror = 0;
    int			opri;


    for (i=0, bd=flash32config; i < flash32boards; bd++, i++) {

	/* LINTED pointer alignment */
	bdd = (bdd_t *)bd->bd_dependent1;
	scb = &bdd->scb;

	/*
	 *  Store error statistics in the MIB structure.
	 */
	bd->mib.ifSpecific.etherAlignErrors   = scb->align_err;
	ierror += scb->align_err;
	bd->mib.ifSpecific.etherCRCerrors     = scb->crc_err;
	ierror += scb->crc_err;
	bd->mib.ifSpecific.etherMissedPkts    = scb->resource_err;
	ierror += scb->resource_err;
	bd->mib.ifSpecific.etherOverrunErrors = scb->overrun_err;
	ierror += scb->overrun_err;
	bd->mib.ifSpecific.etherCollisions    = scb->rcvcdt_err;
	ierror += scb->rcvcdt_err;

	bd->mib.ifInErrors = ierror;
	bd->ifstats->ifs_ierrors = bd->mib.ifInErrors;

	opri = DLPI_LOCK( bd->bd_lock, plstr );

	if ( scb->status & RU_NO_RBDRSC )
		flash32chk_ring(bd);

	if (bd->timer_val > 0)
	    bd->timer_val--;

	if (bd->timer_val == 0) {
	    bd->timer_val = -1;
	    bd->flags = 0;
	    /*
	     *+ debug message
	     */
	    cmn_err (CE_NOTE,"flash32: board %d timed out.", bd->bd_number);

	    (void)reset_board(bd);
	    bd->flags = BOARD_PRESENT;
	}
	DLPI_UNLOCK( bd->bd_lock, opri );
    }

    /*
     * reset timeout to 3 seconds
     */
#ifdef ESMP
    flash32timer_id = itimeout (flash32watch_dog, 0, FLASH32_TIMEOUT, plstr);
#else
    flash32timer_id = timeout (flash32watch_dog, 0, FLASH32_TIMEOUT);
#endif
    return;
}

#ifdef FLASH32DEBUG
flash32dump()
{
	char *dumparea;

	dumparea  = (char *)((uint)flash32dump_buf & 0xFFFFFFF0);
	cmn_err(CE_NOTE, "Doing 82596 dump to address %x %x",
		dumparea, vtop((caddr_t)dumparea, NULL));
	outl(0x5000+0x8, vtop((caddr_t)dumparea,NULL)+3);
	drv_usecwait(10000);
}
#endif	/* FLASH32DEBUG */

