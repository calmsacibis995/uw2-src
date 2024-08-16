/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*		Copyright (c) 1991  Intel Corporation		*/
/*			All Rights Reserved			*/

/*		INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */ 
/*	agreement with Intel Corporation and may not be copied nor	 */
/*	disclosed except in accordance with the terms of that agreement.   */	

#ident	"@(#)kern-i386at:io/dlpi_ether/ee16/ee16hrdw.c	1.10"
#ident	"$Header: $"

/*
 *  This file contains all of the hardware dependent code for EtherExpress.
 *  It is the companion file to ../../io/dlpi_ether.c
 */

#ifdef	_KERNEL_HEADERS

#include <io/dlpi_ether/ee16/dlpi_ee16.h>
#include <io/dlpi_ether/dlpi_ether.h>
#include <io/dlpi_ether/ee16/ee16.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <net/dlpi.h>
#include <net/inet/if.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/inline.h>
#include <io/ddi.h>

#elif defined(_KERNEL)

#include <sys/dlpi_ee16.h>
#include <sys/dlpi_ether.h>
#include <sys/ee16.h>
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

#endif /* _KERNEL_HEADERS */

#define BCOPY(from, to, len)	bcopy((caddr_t)(from), (caddr_t)(to), \
								(size_t)(len))
#define BCMP(s1, s2, len)	bcmp((char *)(s1), (char *)(s2), \
								(size_t)(len))
#define MB_SIZE(p)		((p)->b_wptr - (p)->b_rptr)

#define BYTE 0
#define WORD 1

extern	DL_bdconfig_t	*ee16config;
extern	DL_sap_t	*ee16saps;

extern	int		ee16boards;
extern	DL_bdconfig_t	*ee16intr_to_index[];
extern	int		ee16timer_id;
extern	int		ee16wait_scb();
extern  int 		ee16init_586(), ee16config_586();
extern	ushort		ntohs(), htons();

STATIC	void		ee16tx_done(), ee16chk_ring(), ee16sched();
STATIC	void		ee16ru_restart();

struct	debug	ee16debug = {0, 0, 0, 0, 0};

void	ee16bdspecioctl(), ee16bdspecclose();


/* ee16bdspecclose is called from DLclose->dlpi_ether.c */
void
ee16bdspecclose(q)
queue_t *q;
{
	return;
}

/* ee16bdspecioctl is called from DLioctl->dlpi_ether.c */
void
ee16bdspecioctl(q, mp)
queue_t *q;
mblk_t *mp;
{
	struct iocblk *ioctl_req= (struct iocblk *)mp->b_rptr;

	ioctl_req->ioc_error = EINVAL;
	ioctl_req->ioc_count = 0;
	mp->b_datap->db_type = M_IOCNAK;
	qreply(q, mp);
} /* end of ee16bdspecioctl */

/* Static command list chaining is implemented (refer 586 manual).
 * A packet to be sent is either 
 *   1. sent off directly (with a channel attention to 586)
 *   2. inserted in the command list (if not full), the interrupt
 *      service routine checks the command list 
 *   3. inserted in the queue (and removed by getq) when the command
 *      list is full. This case is also handled by the ISR.
 * An interrupt is generated either when a transmit command is completed
 * or when a packet has been received.
 */

/******************************************************************************
 *  ee16xmit_packet()
 *
 *  This routine is called from DLunitdata_req() and ee16tx_done(). It assumes
 *  we are at STREAMS spl interrupt level.
 */

ee16xmit_packet (bd, mp, sap)
DL_bdconfig_t *bd;
mblk_t	*mp;
DL_sap_t *sap;
{
    bdd_t		*bdd = (bdd_t *) bd->bd_dependent1;
    mblk_t		*mp_tmp;
    ushort		p_tbd;
    ushort		p_cmd;
    ushort		p_txb;
    ushort		tx_size  = 0;
    ushort		base_io  = bd->io_start;
    int			msg_size = 0;
    int			start	 = 0;
    int			byte_left_over = 0;
    pack_ushort_t	partial_short;
    DL_ether_hdr_t	*hdr = (DL_ether_hdr_t *)mp->b_rptr;
    int			opri;


    /* ring buffer is empty: therefore, we need to issue a channel attention
     * signal to start CU for the current packet 
     */
    if (bdd->head_cmd == NULL) {
	bdd->head_cmd = bdd->tail_cmd = bdd->ring_buff;
	start = 1; 
    }
    /*
     * else there's at least one command pending in ring: the packet is
     * inserted in the command list (if it is not full).
     * Note that we don't check whether queue is full or not,
     * if after insertion, the list is full, we set the TX_BUSY flag.
     * This causes dlpi_ether to insert the packets in the (streams) q and
     * the packet is removed by the interrupt routine when there is enough
     * space in the command list.
     * Insertion in the command list is done from the tail end and commands
     * are removed from the command list from the head.
     */
    else {
	bdd->tail_cmd = bdd->tail_cmd->next;

	if (bdd->tail_cmd->next == bdd->head_cmd) {
#ifdef DEBUG
	    ee16debug.ring_full++;
#endif
	    bd->flags |= TX_BUSY;
	}
    }

    /* set up offsets */
    p_cmd = bdd->tail_cmd->ofst_cmd;

    /* tbd ptr guaranteed to be valid since ring buffer is used for
     * transmits only - other commands use bdd->gen_cmd */
#if defined(lint) || defined(C_PIO)
    read_word(p_cmd+6, base_io, p_tbd);         /* read tbd offset */
    read_word(p_tbd+4, base_io, p_txb);         /* read txb offset */
#else
    p_tbd = read_word(p_cmd+6, base_io);        /* read tbd offset */
    p_txb = read_word(p_tbd+4, base_io);        /* read txb offset */
#endif

    /* fill in 82586's transmit command block */
    write_word(bdd->tail_cmd->ofst_cmd, 0, base_io);	/* status  */
    outw(base_io, CS_EL | CS_CMD_XMIT | CS_INT);	/* command */
    outw(base_io, 0xffff);				/* link    */

    bcopy_to_buffer((char *)hdr->dst.bytes, bdd->tail_cmd->ofst_cmd+8, 6, base_io);
    write_word(bdd->tail_cmd->ofst_cmd+14, hdr->len_type, base_io); 
    mp->b_rptr += LLC_EHDR_SIZE;

    /* copy data to tx buffer */
    for (mp_tmp = mp; mp_tmp; mp_tmp = mp_tmp->b_cont) {

	if ((msg_size = MB_SIZE(mp_tmp)) == 0)
	    continue;

	if ( byte_left_over ) {
		msg_size--;
		partial_short.c.a[1] = *mp_tmp->b_rptr++;
		write_word(p_txb, partial_short.c.b, base_io);
		p_txb   += 2;
		tx_size += 2;
		byte_left_over = 0;
	}

	if (msg_size & 0x01) {
		msg_size--;
		partial_short.c.a[0] = *(mp_tmp->b_wptr - 1);
		byte_left_over++;
	}
opri = splhi();
	bcopy_to_buffer((caddr_t) mp_tmp->b_rptr, p_txb, msg_size, base_io);
splx(opri);
	p_txb   += msg_size;
	tx_size += msg_size;
    }

    if ( byte_left_over ) {
		write_byte(p_txb, partial_short.c.a[0], base_io);
		p_txb++;
		tx_size++;
		byte_left_over = 0;
    }

    write_word(p_tbd, tx_size | CS_EOF, base_io);	

    bd->mib.ifOutOctets += tx_size;	/* SNMP */
    bd->ifstats->ifs_opackets++;

    /*
     * if we need to start the 82586, issue a channel attention
     */
    if (start) {
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
	    cmn_err (CE_WARN, "ee16xmit_packet: scb command not cleared");
	    bd->timer_val = 0;	/* force board to reset */
	    freemsg(mp);
	    return (-1);
	}
	write_word(bdd->ofst_scb+2, SCB_CUC_STRT, base_io);	/* cmd */
	outw(base_io, bdd->tail_cmd->ofst_cmd);			/* cbl ofst */
	outb (bd->io_start + CA_CTRL, 1);
	bd->timer_val = 2;
    }
    freemsg(mp);
    return (0);
}

/******************************************************************************
 *  ee16intr()
 */

void
ee16intr (level)
int	level;
{
    DL_bdconfig_t	*bd;
    bdd_t		*bdd;
    ushort		base_io;
    ushort		scb_status;
    uchar_t		i;
    int			opri;


    /* map irq level to the proper board. Make sure it's ours */
    if ((bd = ee16intr_to_index [level]) == NULL) {
    	cmn_err (CE_WARN, "ee16: spurious interrupt");
    	return;
    }

#ifdef ESMP
    opri = DLPI_LOCK( bd->bd_lock, plstr );
#endif

    if (!bd->flags)	goto exit_intr;

    base_io = bd->io_start;
    bdd = (bdd_t *) bd->bd_dependent1;

    /* If scb command field doesn't get cleared, reset the board */
    if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
    	cmn_err (CE_WARN, "ee16intr: scb cmd(1) not cleared");
    	bd->timer_val = 0;	/* cause board to reset */
    	goto exit_intr;
    }

    while (1) {
#if defined(lint) || defined(C_PIO)
    	read_word(bdd->ofst_scb, base_io, scb_status);
#else
    	scb_status = read_word(bdd->ofst_scb, base_io);
#endif
    	if ((scb_status & SCB_INT_MSK) == NULL)
		break;

    	/* If scb command field doesn't get cleared, reset the board */
    	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
	    cmn_err (CE_WARN, "ee16intr: scb cmd(2) not cleared");
	    bd->timer_val = 0;	/* cause board to reset */
	    goto exit_intr;
    	}

    	/* acknowledge 82586 interrupt */
    	write_word(bdd->ofst_scb+2, scb_status & SCB_INT_MSK, base_io);
    	outb (base_io + CA_CTRL, 1);

    	if (scb_status & (SCB_INT_FR | SCB_INT_RNR))
    		ee16chk_ring(bd);

	ee16tx_done(bd);
    }

    if ((bd->flags & (TX_QUEUED | TX_BUSY)) == TX_QUEUED)
	ee16sched (bd);

exit_intr:
    /* clear interrupt */
    i = inb(base_io + SEL_IRQ);
    /* lower the intr_enable bit */
    outb(base_io + SEL_IRQ, i & 0xf7);
    /* raise the intr_enable bit */
    outb(base_io + SEL_IRQ, i | 0x08);

#ifdef ESMP
    DLPI_UNLOCK(bd->bd_lock, opri);
#endif
}

/******************************************************************************
 *  ee16tx_done()
 */

STATIC void
ee16tx_done (bd)
DL_bdconfig_t	*bd;
{
    bdd_t	*bdd	= (bdd_t *) bd->bd_dependent1;
    ushort_t	base_io = bd->io_start;
    ushort_t	scb_status;
    ushort_t	tcb_status;
    ushort_t	tcb_cmd;
    ushort_t	ofst_cmd;
    mblk_t	*mp;


    if (bdd->head_cmd == 0)
	return;

    do {
	ofst_cmd = bdd->head_cmd->ofst_cmd;

#if defined(lint) || defined(C_PIO)
        read_word(ofst_cmd, base_io, tcb_status);       /* status  */
#else
        tcb_status = read_word(ofst_cmd, base_io);      /* status  */
#endif

	if ( ! (tcb_status & CS_CMPLT) )
		return;


	/* Read the tx status register and see if there were any problems */
	if (!(tcb_status & CS_OK)) {
	    bd->mib.ifOutErrors++;
	    bd->ifstats->ifs_oerrors++;
	}

	if (tcb_status & CS_COLLISIONS) {
	    bd->mib.ifSpecific.etherCollisions++;
	    bd->ifstats->ifs_collisions++;
	}

	if (tcb_status & CS_CARRIER)
	    bd->mib.ifSpecific.etherCarrierLost++;

        bd->flags &= ~TX_BUSY;

        tcb_cmd = read_word(ofst_cmd+2, base_io);      /* command */

	if ( tcb_cmd & (CS_INT | CS_EL) )
	    break;

	bdd->head_cmd = bdd->head_cmd->next;

    } while (1);

    /* Reset timer_val as transmit is complete */
    bd->timer_val = -1;

    /* return if last command in ring buffer */
    if (bdd->head_cmd == bdd->tail_cmd) {
	bdd->head_cmd = bdd->tail_cmd = 0;
	return;
    }
    else {
	/* there are more commands pending to be sent to the board */
	ring_t  *next_cmd;

	next_cmd = bdd->head_cmd = bdd->head_cmd->next;

	while (next_cmd != bdd->tail_cmd) {
	    /*
	     * the link has to become valid before EL can be cleared.
	     * Only the tail_cmd will have EL and I bit set.
	     */
    	    write_word(next_cmd->ofst_cmd+4, next_cmd->next->ofst_cmd, base_io);
    	    write_word(next_cmd->ofst_cmd+2, CS_CMD_XMIT, base_io);
	    next_cmd = next_cmd->next;
	}

start_cu:
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
	    cmn_err (CE_WARN, "ee16tx_done: scb command not cleared");
	    bd->timer_val = 0;	
	    return;
	}

	bd->timer_val = 2;
	write_word(bdd->ofst_scb + 2, SCB_CUC_STRT, base_io); /* cmd */
	outw(base_io, bdd->head_cmd->ofst_cmd);
	outb(base_io+CA_CTRL, 1);
    }
}

STATIC void
ee16sched(bd)
DL_bdconfig_t	*bd;
{
	bdd_t	  *bdd = (bdd_t *) bd->bd_dependent1;
	DL_sap_t  *sap = bdd->next_sap;
	mblk_t	  *mp;
	int	  i;

	/*
	 *  Indicate outstanding transmit is done and see if there is work
	 *  waiting on our queue.
	 */
	for ( i = bd->ttl_valid_sap; i; i-- ) {

		if ( sap == (DL_sap_t *)NULL )
			sap = bd->valid_sap;

		if (sap->state == DL_IDLE) {
			while (mp = getq(sap->write_q)) {
				(void) ee16xmit_packet(bd, mp, sap);
				bd->mib.ifOutQlen--;
				if( bd->flags & TX_BUSY ) {
					bdd->next_sap = sap->next_sap;
					return;
				}
			}
		}
		sap = sap->next_sap;
	}

	/* make the queue empty */
#ifdef DEBUG
	ee16debug.q_cleared++;
#endif
	bd->flags &= ~TX_QUEUED;
	bd->mib.ifOutQlen = 0;
	bdd->next_sap = (DL_sap_t *)NULL;
}

/******************************************************************************
 *  ee16chk_ring (bd)
 */

STATIC void
ee16chk_ring (bd)
DL_bdconfig_t *bd;
{
    bdd_t	*bdd = (bdd_t *) bd->bd_dependent1;
    ushort	base_io = bd->io_start;
    ushort	fd;
    ushort	last_rbd;
    fd_t	fd_s;
    rbd_t	rbd_s;
    mblk_t	*mp;
    int		length;
    int		opri;


    /* for all fds */
    fd = bdd->begin_fd;

    while (1) {
	bcopy_from_buffer((char *)&fd_s, fd, sizeof(fd_t), base_io);

	if (!(fd_s.fd_status & CS_CMPLT) || (fd_s.fd_rbd_ofst == 0xffff))
	    break;

	/*
	 * find length of data and last rbd holding the received frame.
	 */
	last_rbd = fd_s.fd_rbd_ofst;
	bcopy_from_buffer((char *)&rbd_s, last_rbd, sizeof(rbd_t), base_io);

	/*
	 * we programmed 82586 not to save bad frames, so, the
	 * chances for fd_status is not OK are RU out of resource.
	 */
	if (fd_s.fd_status & CS_OK) {

	    if (rbd_s.rbd_status & CS_EOF) {

	    	length = (int)(rbd_s.rbd_status & CS_RBD_CNT_MSK);

	    	if ((mp = allocb (length+MAC_HDR_LEN, BPRI_MED)) == NULL) {
		    bd->mib.ifInDiscards++;			/* SNMP */
		    bd->mib.ifSpecific.etherRcvResources++;	/* SNMP */
	    	}
	    	else {
		    /* fill in the data from rcv buffer(s)*/
		    BCOPY(fd_s.fd_dest, mp->b_wptr, MAC_HDR_LEN);
		    mp->b_wptr += MAC_HDR_LEN;
		    bcopy_from_buffer((char *)mp->b_wptr, rbd_s.rbd_buff, length, base_io);
		    mp->b_wptr += length;

		    if ( !ee16recv(mp, bd->sap_ptr) ) {
		    	bd->mib.ifInOctets += (length + MAC_HDR_LEN);
		    	bd->ifstats->ifs_ipackets++;
		    }
		}
	    }
	}
	else {
	    /*
    	     *+ RU may be running out of resources or recv errors, or
	     *  over-sized frame; discard frame and reclaim the rbd's.
    	     */
	    cmn_err(CE_WARN, "!ee16: rcv error %x", fd_s.fd_status);
	    bd->mib.ifInDiscards++;			/* SNMP */
	    break;
	}

	/* re-queue rbd */
	bdd->begin_rbd = rbd_s.rbd_nxt_ofst;
	write_word(bdd->end_rbd+8, RBD_BUF_SIZ, base_io);	/*   !EL     */	
	bdd->end_rbd = last_rbd;				/*  new end  */
	write_word(last_rbd+8, CS_EL | RBD_BUF_SIZ, base_io);	/*    EL     */	

	/* re-queue fd */
	write_word(fd+6, 0xffff, base_io);	/* unlink fd and rbd */
	write_word(fd+2, CS_EL, base_io);	/* set EL bit at end of ring */
	write_word(bdd->end_fd+2, 0, base_io);	/* clear EL bit */

	bdd->end_fd = fd;	/* establish new end of ring */
	fd = bdd->begin_fd = fd_s.fd_nxt_ofst;
    }
    ee16ru_restart (bd);
}

/******************************************************************************
 * ee16ru_restart ()
 */

STATIC void
ee16ru_restart (bd)
DL_bdconfig_t *bd;
{
	bdd_t	*bdd	= (bdd_t *) bd->bd_dependent1;
	ushort  base_io = bd->io_start;
	ushort	scb_status;
	ushort	fd_status;


	/* RU already running -- leave it alone */
#if defined(lint) || defined(C_PIO)
        read_word(bdd->ofst_scb, base_io, scb_status);
#else
        scb_status = read_word(bdd->ofst_scb, base_io);
#endif
	if ((scb_status & SCB_RUS_READY) == SCB_RUS_READY)
		return;
	/*
	 * if the RU just went not ready and it just completed an fd --
	 * do NOT restart RU -- this will wipe out the just completed fd.
	 * There will be a second interrupt that will remove the fd via
	 * ee16chk_ring () and thus calls ee16ru_restart() which will
	 * then start the RU if necessary.
	 */
	if ( !(scb_status & SCB_RUS_NORESRC) ) {
#if defined(lint) || defined(C_PIO)
                read_word(bdd->begin_fd, base_io, fd_status);   /* status */
#else
                fd_status = read_word(bdd->begin_fd, base_io);  /* status */
#endif
		if (fd_status & CS_CMPLT) 
			return;
	}
	/*
	 * if we get here, then RU is not ready and no completed fd's are avail.
	 * therefore, follow RU start procedures listed under RUC on page 2-15
	 */
#ifdef DEBUG
	ee16debug.rcv_restart_count++;
#endif
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io)) {
		cmn_err (CE_WARN, "ru-restart: scb command not cleared");
		bd->timer_val = 0;	/* force board to reset */
		return;
	}

	/* set rbd_ofst */
	write_word(bdd->begin_fd+6, bdd->begin_rbd, base_io);	/* rbd ofst */

	write_word(bdd->ofst_scb, 0, base_io);			/* status */
	write_word(bdd->ofst_scb+6, bdd->begin_fd, base_io);	/* rfa ofst */
	write_word(bdd->ofst_scb+2, SCB_RUC_STRT, base_io);	/* cmd */
	outb (bd->io_start + CA_CTRL, 1);
	return;
}

/******************************************************************************
 * ee16watch_dog (bd)
 */

void
ee16watch_dog ()
{
	DL_bdconfig_t	*bd;
	bdd_t		*bdd;
	ushort		base_io;
	ushort		tmp;
	int		i;
	int		opri;


	for (i=0, bd=ee16config; i < ee16boards; bd++, i++) {

		base_io = bd->io_start;
		bdd = (bdd_t *) bd->bd_dependent1;

		opri = DLPI_LOCK( bd->bd_lock, plstr );

		/* Store error statistics in the MIB structure */
#if defined(lint) || defined(C_PIO)
                read_word(bdd->ofst_scb + 8, base_io,
                                bd->mib.ifSpecific.etherCRCerrors);
#else
                bd->mib.ifSpecific.etherCRCerrors =
                                        read_word(bdd->ofst_scb+8, base_io);
#endif
		bd->mib.ifSpecific.etherAlignErrors   = inw(base_io);
		bd->mib.ifSpecific.etherMissedPkts    = inw(base_io);
		bd->mib.ifSpecific.etherOverrunErrors = inw(base_io);
		bd->mib.ifInErrors =	bd->mib.ifSpecific.etherCRCerrors +
					bd->mib.ifSpecific.etherAlignErrors +
					bd->mib.ifSpecific.etherMissedPkts +
					bd->mib.ifSpecific.etherOverrunErrors;

		bd->ifstats->ifs_ierrors = bd->mib.ifInErrors;

		if (bd->timer_val > 0)
			bd->timer_val--;

		if (bd->timer_val == 0) {
			bd->flags = 0;
			cmn_err (CE_WARN, "ee16: board %d timed out.", i);

			if (ee16init_586(bd))
			    cmn_err (CE_WARN, "ee16: board %d reset error.", i);

			bd->timer_val = -1;
	    		bd->flags = BOARD_PRESENT;
		}
		DLPI_UNLOCK( bd->bd_lock, opri );
	}

	/* reset timeout to 5 seconds */
	ee16timer_id = itimeout (ee16watch_dog, 0, EE16_TIMEOUT, plstr);
}

/******************************************************************************
 * ee16promisc_on (bd)
 */

ee16promisc_on (bd)
DL_bdconfig_t	*bd;
{
	int	opri;
	int	ret;

	opri = DLPI_LOCK( bd->bd_lock, plstr );

	/* If already in promiscuous mode, just return */
	if (bd->promisc_cnt++)
		ret = 0;
	else
		ret = ee16config_586 (bd, PRO_ON);

	DLPI_UNLOCK( bd->bd_lock, opri );
	return (ret);
}

/******************************************************************************
 * ee16promisc_off (bd)
 */

ee16promisc_off (bd)
DL_bdconfig_t	*bd;
{
	int	opri;
	int	ret;

	/* return if the board is not in a promiscuous mode */
	if (!bd->promisc_cnt)
		return (0);

	opri = DLPI_LOCK( bd->bd_lock, plstr );

	/* return if this is not the last promiscuous SAP */
	if (--bd->promisc_cnt)
		ret = 0;
	else
		ret = ee16config_586 (bd, PRO_OFF);

	DLPI_UNLOCK( bd->bd_lock, opri );
	return (ret);
}

#ifdef ALLOW_SET_EADDR
/******************************************************************************
 *  ee16set_eaddr()
 */
ee16set_eaddr (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
	int	opri;
	int	ret;

	opri = DLPI_LOCK( bd->bd_lock, plstr );

	BCOPY( eaddr, bd->eaddr.bytes, DL_MAC_ADDR_LEN );

	/*
	 * do Individual Address Setup command
	 */
	ret = ee16ia_setup_586 (bd, bd->eaddr.bytes);

	DLPI_UNLOCK( bd->bd_lock, opri );
        return(ret);
}
#endif

/******************************************************************************
*   ee16get_multicast()
*
*/
ee16get_multicast(bd,mp)
DL_bdconfig_t *bd;
mblk_t *mp;
{
	bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp = &(bdd->ee16_multiaddr[0]);
	int i;
	uchar_t *dp;
	int found = 0;

	if((int)(mp->b_wptr - mp->b_rptr) == 0)
		found = bd->multicast_cnt;
	else {
		dp = mp->b_rptr;
		for (i = 0;(i < MULTI_ADDR_CNT) && (dp < mp->b_wptr);i++,mcp++)
			if (mcp->status) {
				BCOPY(mcp->entry,dp,DL_MAC_ADDR_LEN);
				dp += DL_MAC_ADDR_LEN;
				found++;
			}
		mp->b_wptr = dp;
	}
	return found;
}

/******************************************************************************
 *
 *  ee16set_multicast()
 */
ee16set_multicast(bd)
DL_bdconfig_t *bd;
{
	bdd_t   *bdd = (bdd_t *) bd->bd_dependent1;
	ushort  base_io = bd->io_start;
	ushort i,j,total;
	mcad_t mcad;
	mcat_t *mcp = &(bdd->ee16_multiaddr[0]);
 
	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	write_word(bdd->ofst_scb, 0, base_io);  /* status */
	outw(base_io, SCB_CUC_STRT);		/* cmd : auto-incr */
	outw(base_io, bdd->gen_cmd);		/* cbl : auto-incr */

	write_word(bdd->gen_cmd, 0, base_io);	/* status */
	outw(base_io, CS_CMD_MCSET | CS_EL);	/* cmd : auto-incr */
	outw(base_io, 0xffff);			/* link : auto-incr */

	for (i = 0,j = 0; i <  MULTI_ADDR_CNT; i++,mcp++)
		if (mcp->status) {
			BCOPY(mcp->entry, &(mcad.mc_addr[j]), DL_MAC_ADDR_LEN);
			j += DL_MAC_ADDR_LEN;
		}
	mcad.mc_cnt = j;
	write_word(bdd->gen_cmd+6, mcad.mc_cnt, base_io);
	for (i = 0; i < j; i++)
		outb(base_io, mcad.mc_addr[i]);

	outb (base_io + CA_CTRL, 1);

	if (ee16wait_scb (bdd->ofst_scb, 1000, base_io))
		return (1);

	if (ee16ack_586 (bdd->ofst_scb, base_io))
		return (1);

	return (0);
}

/******************************************************************************
 *  ee16add_multicast()
 */
ee16add_multicast (bd, maddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*maddr;
{
	bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
	mcat_t  *mcp = &(bdd->ee16_multiaddr[0]);
	int i, rval;
	int opri;


	if ((bd->multicast_cnt >= MULTI_ADDR_CNT) || !(maddr->bytes[0] & 0x1)) {
		return 1;
	}

	if (ee16is_multicast(bd,maddr)) {
		return 0;
	}

	opri = DLPI_LOCK( bd->bd_lock, plstr );

	for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++) {
		if (!mcp->status)
			break;
	}
	mcp->status = 1;
	bd->multicast_cnt++;
	bcopy((caddr_t)maddr->bytes,(caddr_t)mcp->entry,DL_MAC_ADDR_LEN);
	rval = ee16set_multicast(bd);
	DLPI_UNLOCK( bd->bd_lock, opri );
	return rval;
}

/******************************************************************************
 *  ee16del_multicast()
 */
ee16del_multicast (bd, maddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*maddr;
{
    bdd_t   *bdd = (bdd_t *)bd->bd_dependent1;
    mcat_t  *mcp = &(bdd->ee16_multiaddr[0]);
    int i, rval;
    int  opri;

    if (!ee16is_multicast(bd,maddr))
	return 1;

    opri = DLPI_LOCK( bd->bd_lock, plstr );

    for (i = 0; i < MULTI_ADDR_CNT; i++,mcp++)
	if ( (mcp->status) &&
	    (BCMP(maddr->bytes, mcp->entry, DL_MAC_ADDR_LEN) == 0) )
	    break;

    mcp->status = 0;
    bd->multicast_cnt--;
    rval = ee16set_multicast(bd);
    DLPI_UNLOCK( bd->bd_lock, opri );
    return rval;
}

/******************************************************************************
 *  ee16disable()
 */
ee16disable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  ee16enable()
 */
ee16enable (bd)
DL_bdconfig_t	*bd;
{
	return (1);	/* not supported yet */
}

/******************************************************************************
 *  ee16reset()
 */
ee16reset (bd)
DL_bdconfig_t	*bd;
{
	DL_sap_t *sap;
	int ret;
	int opri;

	opri = DLPI_LOCK( bd->bd_lock, plstr );
	bd->timer_val = -1;
	sap = bd->valid_sap;
	while (sap) {
		if ( (sap->write_q != NULL) && (sap->state == DL_IDLE) )
			flushq(sap->write_q, FLUSHDATA);
		sap = sap->next_sap;
	}
      	bd->flags &= ~(TX_BUSY|TX_QUEUED);
	ret = ee16init_586(bd);
	DLPI_UNLOCK( bd->bd_lock, opri );
	return ( ret );
}

/******************************************************************************
 *  ee16is_multicast()
 */
/*ARGSUSED*/
ee16is_multicast (bd, eaddr)
DL_bdconfig_t	*bd;
DL_eaddr_t	*eaddr;
{
    bdd_t *bdd = (bdd_t *)bd->bd_dependent1;
    mcat_t *mcp = &bdd->ee16_multiaddr[0];
    int i;
    int rval = 0;

    if (bd->multicast_cnt) {
	if (eaddr->bytes[0] & 0x1) {
	    for (i = 0; i < MULTI_ADDR_CNT;i++,mcp++) {
	    	if ( (mcp->status) &&
		    (BCMP(eaddr->bytes, mcp->entry, DL_MAC_ADDR_LEN) == 0) ) {
			rval = 1;
			break;
		}
	    }
	}
    }
    return (rval);
}

#ifdef C_PIO
/******************************************************************************/
void
bcopy_to_buffer(src, dest, count, base_io)
char   *src;
ushort dest;
ushort count;
ushort base_io;
{
	ushort i;

    	write_byte(dest, *src, base_io);
	src++;
	for (i=1; i< count; i++,src++) 
		outb(base_io, *src);	
}

/******************************************************************************/
void
bcopy_from_buffer(dest, src, count, base_io)
char   *dest;
ushort src;
ushort count;
ushort base_io;
{
	ushort i;

	/* This in is needed to avoid a race condition in the HW */
	i = inb(base_io + AUTOID);
	outw(base_io + RDPTR, src);
	*dest = (char) inb(base_io);
	dest++;
	for (i=1; i< count; i++,dest++) 
		(*dest) = (char) inb(base_io);
}

#endif		/* ifdef C_PIO */
