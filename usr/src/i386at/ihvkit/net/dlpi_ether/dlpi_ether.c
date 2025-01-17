/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:net/dlpi_ether/dlpi_ether.c	1.1"
#ident	"$Header: $"

/*	Copyright (c) 1989  Intel Corporation
 *	All Rights Reserved
 *
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied to AT & T under the terms of a license   
 *	agreement with Intel Corporation and may not be copied nor         
 *	disclosed except in accordance with the terms of that agreement.   
 *
 *  This is the common STREAMS interface for DLPI 802.3/ethernet drivers.
 *  It defines all of the service routines and ioctl's.  Board specific
 *  functions will be called to handle of the details of getting packet
 *  to and from the wire.
 */

#ifdef	_KERNEL_HEADERS
/*
 *	building in the kernel tree
 */
#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/ipl.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <mem/immu.h>
#include <io/stream.h>
#include <io/strstat.h>
#include <io/strlog.h>
#include <io/log/log.h>
#include <io/strmdep.h>
#include <io/stropts.h>
#include <io/termio.h>
#include <io/strlog.h>
#include <svc/errno.h>
#ifndef lint
#include <net/tcpip/byteorder.h>
#endif
#include <net/tcpip/strioc.h>
#include <net/tcpip/if.h>
#include <net/transport/socket.h>
#include <net/transport/sockio.h>
#include <net/dlpi.h>
#include <io/dlpi_ether/dlpi_ether.h>

#ifdef IMX586
#include <io/dlpi_ether/dlpi_imx586.h>
#endif
 
#ifdef I596
#include <io/dlpi_ether/dlpi_i596.h>
#endif
 
#ifdef EL16
#include <io/dlpi_ether/dlpi_el16.h>
#endif
 
#ifdef EL3
#include <io/dlpi_ether/dlpi_el3.h>
#endif
 
#ifdef WD
#include <io/dlpi_ether/dlpi_wd.h>
#endif
 
#ifdef EE16
#include <io/dlpi_ether/dlpi_ee16.h>
#endif
 
#ifdef IE6
#include <io/dlpi_ether/dlpi_ie6.h>
#endif

#ifdef ELT32
#include <io/dlpi_ether/dlpi_elt32.h>
#endif

#ifdef NE2K
#include <io/dlpi_ether/dlpi_ne2k.h>
#endif

#ifdef NE2
#include <io/dlpi_ether/dlpi_ne2.h>
#endif

/*
 * ddi.h has to be included after those
 * device dependent dlpi_xxx.h files.
 */
#include <io/ddi.h>

#else
/*
 *	NOT building in the kernel source tree!
 */

#include <sys/types.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/ipl.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/immu.h>
#include <sys/stream.h>
#include <sys/strstat.h>
#include <sys/strlog.h>
#include <sys/log.h>
#include <sys/strmdep.h>
#include <sys/stropts.h>
#include <sys/termio.h>
#include <sys/strlog.h>
#include <sys/errno.h>
#ifndef lint
#include <sys/byteorder.h>
#endif
#include <net/strioc.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/sockio.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>

#ifdef IMX586
#include <sys/dlpi_imx586.h>
#endif
 
#ifdef I596
#include <sys/dlpi_i586.h>
#endif
 
#ifdef EL16
#include <sys/dlpi_el16.h>
#endif
 
#ifdef EL3
#include <sys/dlpi_el3.h>
#endif
 
#ifdef WD
#include <sys/dlpi_wd.h>
#endif
 
#ifdef EE16
#include <sys/dlpi_ee16.h>
#endif
 
#ifdef IE6
#include <sys/dlpi_ie6.h>
#endif

#ifdef ELT32
#include <sys/dlpi_elt32.h>
#endif

#ifdef NE2K
#include <sys/dlpi_ne2k.h>
#endif

#ifdef NE2
#include <sys/dlpi_ne2.h>
#endif

#endif

/*
 * ddi.h has to be included after those
 * device dependent dlpi_xxx.h files.
 */
#include <sys/ddi.h>



	int	DLopen();
	void	DLclose(),DLprint_eaddr();
static  void	DLcmds(), DLinfo_req(), DLbind_req(), DLunbind_req(),
		DLunitdata_req(), DLioctl(), DLrsrv(),
		DLwput(), DLerror_ack(), DLuderror_ind(),DLtest_req(),
		DLsubsbind_req(),DLremove_sap(),DLinsert_sap();

static  mblk_t	*DLmk_ud_ind(),*DLmk_test_con(), *DLproc_llc(), *DLform_80223();

static	uchar_t *copy_local_addr(), *copy_broad_addr();
static	int	DLis_us(), DLis_broadcast();

extern	int	DLxmit_packet(), DLpromisc_off(), DLpromisc_on(),
		DLset_eaddr(), DLadd_multicast(), DLdel_multicast(),
		DLdisable(), DLenable(), DLreset(), DLis_multicast(),
		DLis_equalsnap(), DLget_multicast();

extern	void	DLbdspecioctl(), DLbdspecclose();

extern	int		strlen();
extern	ushort_t	ntohs();

typedef int (*PSFI)();	/* Pointer to STREAMS Function return Int */

STATIC  struct	module_info	DLrminfo = {
	DL_ID, DL_NAME, DL_MIN_PACKET, DL_MAX_PACKET, DL_HIWATER, DL_LOWATER,
};
STATIC  struct	module_info	DLwminfo = {
	DL_ID, DL_NAME, DL_MIN_PACKET, DL_MAX_PACKET, DL_HIWATER, DL_LOWATER,
};
STATIC  struct	qinit		DLrinit = {
	NULL, (PSFI)DLrsrv, DLopen, (PSFI)DLclose, NULL, &DLrminfo, NULL,
};
STATIC  struct	qinit		DLwinit = {
	(PSFI)DLwput, NULL, NULL, NULL, NULL, &DLwminfo, NULL,
};

struct	streamtab	DLinfo = { &DLrinit, &DLwinit, NULL, NULL, };

extern	int		DLboards;
extern	DL_bdconfig_t	DLconfig[];
extern	DL_sap_t	DLsaps[];
extern	int		DLstrlog;
extern	char		DLid_string[];
extern	struct	ifstats	*DLifstats;

int	DLdevflag = 0;		/* V4.0 style driver */

/******************************************************************************
 *  DLopen()
 */
/* ARGSUSED */
DLopen(q, dev, flag, sflag, credp)
queue_t		*q;
dev_t		*dev;
int		flag;
int		sflag;
struct	cred	*credp;
{
	int		i, old;
	DL_bdconfig_t	*bd;
	DL_sap_t	*sap;
	major_t	major = getmajor(*dev);
	minor_t	minor = getminor(*dev);

	DL_LOG(strlog(DL_ID, 0, 3, SL_TRACE,
		"DLopen - major %d minor %d queue %x",
					(int)major, (int)minor, (int)q));

	/*
	 *  Find the board structure for this major number.
	 */
	for (i = DLboards, bd = DLconfig; i; bd++, i--)
		if (bd->major == major)
			break;
	
	if (i == 0) {
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
			"DLopen - invalid major number (%d)", (int)major));
		return (ENXIO);
	}

	/*
	 *  Check if we found a board there.
	 */
	if (!(bd->flags & BOARD_PRESENT)) {
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
			"DLopen - board for major (%d) not installed",
								(int)major));
		return (ENXIO);
	}
		
	/*
	 *  If it's a clone device, assign a minor number.
	 */
	if (sflag == CLONEOPEN) {
		for (i = 0, sap = bd->sap_ptr; i < bd->max_saps; i++, sap++)
			if (sap->write_q == NULL)
				break;

		if (i == bd->max_saps) {
			DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
				"DLopen - no minors left"));
			return (ECHRNG);
		}
		else
			minor = (minor_t) i;
	} else if (minor > bd->max_saps) {
		DL_LOG(strlog(DL_ID, 0, 1, SL_TRACE,
			"DLopen - invalid minor number (%d)", (int)minor));
		return (ECHRNG);
	}

	/*
	 *  If this stream has not been opened before, set it up.
	 */
	if (!q->q_ptr) {
		old = splstr();

		sap->state   = DL_UNBOUND;
		sap->read_q  = q;
		sap->write_q = WR(q);

		q->q_ptr = (caddr_t) sap;
		WR(q)->q_ptr = q->q_ptr;

		/*
		 *  Need to keep track of priviledge for later reference in 
		 *  bind requests.
		 */
		if (drv_priv(credp) == 0)
			sap->flags |= PRIVILEDGED;

		splx(old);
	}

	*dev = makedevice(major, minor);
	return (0);
}

/******************************************************************************
 *  DLclose()
 */
void
DLclose(q)
queue_t	*q;
{
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	int		old;

	DL_LOG(strlog(DL_ID, 1, 3, SL_TRACE, "DLclose queue %x", (int)q));

	/*
	 *  If this was a promiscuous SAP, have board put out of promiscuous
	 *  mode.  If there are still promiscuous SAPs running, the call may
	 *  have no effect.
	 */
	if (sap->flags & PROMISCUOUS)
		(void)DLpromisc_off(sap->bd);

	/*
	 *  Cleanup SAP structure
	 */
	old = splstr();

	sap->state    = DL_UNBOUND;
	sap->sap_addr = 0;
	sap->read_q   = NULL;
	sap->write_q  = NULL;
	sap->flags    = 0;
	DLremove_sap(sap,sap->bd);
	sap->next_sap = NULL;

	splx(old);

	/* handle any board-specific close processing */
	DLbdspecclose(q);

	return;
}

/******************************************************************************
 *  DLwput()
 */
STATIC void
DLwput(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	DL_LOG(strlog(DL_ID, 2, 3, SL_TRACE,
		"DLwput queue %x type %d", (int)q, mp->b_datap->db_type));

	switch(mp->b_datap->db_type) {
		/*
		 *  Process data link commands.
		 */
		case M_PROTO:
		case M_PCPROTO:
			DLcmds(q, mp);
			break;
		/*
		 *  Flush read and write queues.
		 */
		case M_FLUSH:
			if (*mp->b_rptr & FLUSHW)
				flushq(q, FLUSHDATA);
			if (*mp->b_rptr & FLUSHR) {
				flushq(RD(q), FLUSHDATA);
				*mp->b_rptr &= ~FLUSHW;
				qreply(q, mp);
			} else
				freemsg(mp);
			break;
		/*
		 *  Process ioctls.
		 */
		case M_IOCTL:
			DLioctl(q, mp);
			break;
		/* 
		 *  Anything else we dump.
		 */
		case M_DATA:
		default:
			DL_LOG(strlog(DL_ID, 2, 1, SL_TRACE,
				"DLwput - unsupported type (%d)",
							mp->b_datap->db_type));
			freemsg(mp);
			break;
	}

	return;
}

/******************************************************************************
 *  DLcmds()
 */
STATIC void
DLcmds(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	/* LINTED pointer assignment */
	union DL_primitives  *dl = (union DL_primitives *)mp->b_datap->db_base;

	switch (dl->dl_primitive) {
		case DL_UNITDATA_REQ:
			DLunitdata_req(q, mp);
			return;			/* don't free the message */

		case DL_INFO_REQ:
			DLinfo_req(q);
			break;
		
		case DL_BIND_REQ:
			DLbind_req(q, mp);
			break;
		
		case DL_UNBIND_REQ:
			DLunbind_req(q, mp);
			break;

		case DL_TEST_REQ:
			DLtest_req(q,mp);
			return;

		case DL_SUBS_BIND_REQ:
			DLsubsbind_req(q,mp);
			return;

		case DL_ATTACH_REQ:
		case DL_DETACH_REQ:
		case DL_UDQOS_REQ:
		case DL_CONNECT_REQ:
		case DL_CONNECT_RES:
		case DL_TOKEN_REQ:
		case DL_DISCONNECT_REQ:
		case DL_RESET_REQ:
		case DL_RESET_RES:
		case DL_ENABMULTI_REQ:
		case DL_DISABMULTI_REQ:
		case DL_PROMISCON_REQ:
		case DL_PROMISCOFF_REQ:
		case DL_XID_REQ:
		case DL_XID_RES:
		case DL_TEST_RES:
		case DL_PHYS_ADDR_REQ:
		case DL_SET_PHYS_ADDR_REQ:
		case DL_GET_STATISTICS_REQ:
		case DL_DATA_ACK_REQ:
		case DL_REPLY_REQ:
		case DL_REPLY_UPDATE_REQ:
			DLerror_ack(q, mp, DL_NOTSUPPORTED);
			break;
		
		default:
			DLerror_ack(q, mp, DL_BADPRIM);
			break;
			
	}

	/*
	 *  Free the request.
	 */
	freemsg(mp);
}

/****************************************************************************
*	DLform_snap()
*
*/

mblk_t *
DLform_snap(sap,mp,src_addr)
DL_sap_t *sap;
mblk_t *mp;
unsigned char *src_addr;
{
dl_unitdata_req_t	*data_req = (dl_unitdata_req_t *)mp->b_rptr;

struct llcb *llcbp ;
mblk_t *nmp;
DL_mac_hdr_t *hdr;
caddr_t dst_addr;

        if ((nmp = allocb(sizeof (union DL_primitives),BPRI_MED)) == NULL)
		return NULL;
	nmp->b_cont = NULL;

	hdr = (DL_mac_hdr_t *)nmp->b_rptr;
	bcopy((caddr_t)src_addr,(caddr_t)hdr->src.bytes,DL_MAC_ADDR_LEN);
	hdr->mac_llc.snap.snap_length = htons(msgdsize(mp) + LLC_LSAP_HDR_SIZE + SNAP_LSAP_HDR_SIZE);
	hdr->mac_llc.snap.snap_ssap = SNAPSAP;
	hdr->mac_llc.snap.snap_control = LLC_UI;

        if (data_req->dl_dest_addr_length == (DL_MAC_ADDR_LEN + LLC_LSAP_LEN)) {		dst_addr = (caddr_t)data_req + data_req->dl_dest_addr_offset;
		bcopy(dst_addr, (caddr_t)hdr->dst.bytes,DL_MAC_ADDR_LEN);
		hdr->mac_llc.snap.snap_dsap = SNAPSAP;

		/*
                  The bytes have already been swapped and stored during
                  the DLsubsbind_req processing. In effect, the org field is
                  maintained in the network order to minimize the byte
                  swapping during data req/ind processing.
		*/

                bcopy((caddr_t)(&sap->snap_global),(caddr_t)hdr->mac_llc.snap.snap_org,SNAP_GLOBAL_SIZE);
		hdr->mac_llc.snap.snap_type = htons(sap->snap_local);
	} else {
                llcbp = (struct llcb *)((caddr_t)data_req + data_req->dl_dest_addr_offset);
           	bcopy((caddr_t)llcbp->lbf_addr,(caddr_t)hdr->dst.bytes,DL_MAC_ADDR_LEN);
        	hdr->mac_llc.snap.snap_dsap = (uchar_t)llcbp->lbf_sap;
		bcopy((caddr_t)(&llcbp->lbf_xsap),(caddr_t)hdr->mac_llc.snap.snap_org,SNAP_GLOBAL_SIZE);
		hdr->mac_llc.snap.snap_type = htons((ushort_t)llcbp->lbf_type);
	}
	nmp->b_wptr = nmp->b_rptr + SNAP_HDR_SIZE;
	return nmp;
}




/*****************************************************************************
*	DLform_80223()
*
*/

mblk_t *
DLform_80223(dst_addr,src_addr,size,dsap,ssap,rawsap,control)
unsigned char *dst_addr;
unsigned char *src_addr;
int size;
ushort_t dsap,ssap,rawsap,control;
{
	mblk_t *nmp;
	DL_mac_hdr_t *hdr;

        if ( (nmp = allocb(sizeof (union DL_primitives),BPRI_MED)) == NULL)
                return NULL;
	nmp->b_cont = NULL;
        hdr = (struct DL_mac_hdr *)nmp->b_rptr;
        bcopy((caddr_t)dst_addr,(caddr_t)hdr->dst.bytes,DL_MAC_ADDR_LEN);
        bcopy((caddr_t)src_addr,(caddr_t)hdr->src.bytes,DL_MAC_ADDR_LEN);

	if (rawsap) {
        	hdr->mac_llc.llc.llc_length = htons(size);
        	nmp->b_wptr = nmp->b_rptr + LLC_EHDR_SIZE;
	} else {
        	hdr->mac_llc.llc.llc_length = htons(size + LLC_LSAP_HDR_SIZE);
        	hdr->mac_llc.llc.llc_dsap = (char)dsap;
        	hdr->mac_llc.llc.llc_ssap = (char)ssap;
		hdr->mac_llc.llc.llc_control = (char)control;
        	nmp->b_wptr = nmp->b_rptr + LLC_HDR_SIZE;
	}
	return nmp;
}


STATIC void
DLtest_req(q,mp)
queue_t *q;
mblk_t *mp;
{
DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
DL_bdconfig_t *bd = sap->bd;
mblk_t *nmp, *tmp ;
dl_test_req_t *test_req = (dl_test_req_t *)mp->b_rptr;
struct llcc *llccp;
DL_eaddr_t *ether_addr;
int size,local,old,multicast;
ushort dsap,ssap,control;

	if (sap->state != DL_IDLE) {
		DLerror_ack(q,mp,DL_OUTSTATE);
		freemsg(mp);
		return;
	}

	if((sap->mac_type != DL_CSMACD)||(sap->flags & (SNAPCSMACD|RAWCSMACD))){
		DLuderror_ind(q,mp,DL_UNDELIVERABLE);
		return;
	}

	size = msgdsize(mp);
	if ((size > sap->max_spdu)) {
		DLuderror_ind(q, mp, DL_BADDATA);
		return;
	}
	ether_addr = (DL_eaddr_t*)(mp->b_rptr + test_req->dl_dest_addr_offset);
        local = DLis_us(bd, ether_addr);
        multicast = DLis_broadcast(ether_addr)|| DLis_multicast(sap->bd,ether_addr);
        llccp = (struct llcc *)((caddr_t)test_req + test_req->dl_dest_addr_offset);
        if (test_req->dl_flag & DL_POLL_FINAL)
		control = LLC_TEST |LLC_P;
	else
               	control = LLC_TEST;
	dsap = llccp->lbf_sap;
	ssap = sap->sap_addr;

	/* 
	   At this time we support only end to end TEST pdus. Any test command 
	   PDU destined locally will be error acked. However, as the protocol 
	   dictates, all incoming test command PDUs will be responded to by 
	   the provider. Also incoming TEST pdu responses will be handed over 
	   to the appropriate sap. Similarly TEST Command PDUs bound for 
	   Multicast or Broadcast address will only be sent out but not looped 
	   back.
	*/

	if (local) {
		DLerror_ack(q,mp,DL_UNSUPPORTED);
		freemsg(mp);
		return;
	}

	if ((nmp = DLform_80223(llccp->lbf_addr,bd->eaddr.bytes,size,dsap,ssap,(ushort_t)0,control)) == NULL) {
		DLerror_ack(q,mp,DL_SYSERR);
		freemsg(mp);
		return;
	}
	tmp = rmvb(mp,mp);
	freeb(mp);
	linkb(nmp,tmp);
        size += LLC_LSAP_HDR_SIZE;
	if (size < sap->min_spdu) {
		if ((mp = allocb(sap->min_spdu, BPRI_MED)) == NULL) {
			DLuderror_ind(q, mp, DL_UNDELIVERABLE);
			return;
		}
		mp->b_wptr = mp->b_rptr + (sap->min_spdu - size);
		linkb(nmp,mp);
	}

        old = splstr();
        if (bd->flags & TX_BUSY) {
                bd->flags |= TX_QUEUED;
                bd->mib.ifOutQlen++;            /* SNMP */
                putq(q, nmp);
                DL_LOG(strlog(DL_ID, 6, 2, SL_TRACE,
                        "DLtestdata hardware busy - xmit deferred for queue %x",                                                                (int)q));
	}
	else {
		if ( DLxmit_packet ( bd, nmp ) == 1 ) {
			/*
			 *      xmit was full -- queue this one up
			 */
		  	bd->flags |= (TX_QUEUED | TX_BUSY);
			bd->mib.ifOutQlen++;            /* SNMP */
			putq(q, nmp);
		}
       	}
        splx(old);
}

/******************************************************************************
 *  DLinfo_req()
 */
STATIC void
DLinfo_req(q)
queue_t	*q;
{
	dl_info_ack_t	*info_ack;
	mblk_t		*resp;
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	
	DL_LOG(strlog(DL_ID, 3, 3, SL_TRACE,
		"DLinfo request for queue %x", (int)q));

	/*
	 *  If we can't get the memory, just ignore the request.
	 */
	if ((resp = allocb(DL_PRIMITIVES_SIZE + DL_TOTAL_ADDR_LEN + DL_MAC_ADDR_LEN, BPRI_MED)) == NULL)/*	VERSION 2 DLPI SUPPORT */
		return;

	/* LINTED pointer assignment */
	info_ack			= (dl_info_ack_t *)resp->b_wptr;
	info_ack->dl_primitive		= DL_INFO_ACK;
	info_ack->dl_max_sdu		= sap->max_spdu;
	info_ack->dl_min_sdu		= sap->min_spdu;
	info_ack->dl_addr_length	= DL_TOTAL_ADDR_LEN;
	info_ack->dl_mac_type		= sap->mac_type;
	info_ack->dl_current_state	= sap->state;
	info_ack->dl_service_mode	= sap->service_mode;
	info_ack->dl_qos_length		= 0;
	info_ack->dl_qos_offset		= 0;
	info_ack->dl_qos_range_length	= 0;
	info_ack->dl_qos_range_offset	= 0;
	info_ack->dl_provider_style	= sap->provider_style;
	info_ack->dl_addr_offset	= DL_INFO_ACK_SIZE;
	info_ack->dl_growth		= 0;
	info_ack->dl_brdcst_addr_length = DL_MAC_ADDR_LEN; 
				/* VERSION 2 DLPI SUPPORT */
        info_ack->dl_brdcst_addr_offset = DL_TOTAL_ADDR_LEN + DL_INFO_ACK_SIZE;
			       /* VERSION 2 DLPI SUPPORT */
	resp->b_wptr += DL_INFO_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/*
	 *  DLPI spec says if stream is not bound, address will be 0.
	 */
	if (sap->state == DL_IDLE) {
		/* LINTED pointer assignment */
		resp->b_wptr = copy_local_addr(sap, (ushort_t*)resp->b_wptr);
	} else {
		info_ack->dl_addr_offset = 0;
		info_ack->dl_brdcst_addr_offset = DL_INFO_ACK_SIZE;
				/* VERSION 2  DLPI SUPPORT */
	}
	resp->b_wptr = copy_broad_addr((ushort_t*)resp->b_wptr);
	qreply(q, resp);
}

/******************************************************************************
 *  DLbind_req()
 */
STATIC void
DLbind_req(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	/* LINTED pointer assignment */
	dl_bind_req_t	*bind_req  = (dl_bind_req_t *)mp->b_datap->db_base;
	DL_sap_t	*sap       = (DL_sap_t *)q->q_ptr;
	DL_sap_t	*tmp;
	dl_bind_ack_t	*bind_ack;
	mblk_t		*resp;
	int		 old, i;
	ushort_t	 dlsap;

	DL_LOG(strlog(DL_ID, 4, 3, SL_TRACE,
		"DLbind request to sap 0x%x on queue %x",
				((int)bind_req->dl_sap) & 0xffff, (int)q));

	/*
	 *  If the stream is already bound, return an error.
	 */
	if (sap->state != DL_UNBOUND) {
		DL_LOG(strlog(DL_ID, 4, 1, SL_TRACE,
			"DLbind not valid for state %d on queue %x",
							sap->state, (int)q));
		DLerror_ack(q, mp, DL_OUTSTATE);
		return;
	}

	/*
	 *  Only a prevledged user can bind to the promiscuous SAP or a SAP
	 *  that is already bound.
	 */
	if (!(sap->flags & PRIVILEDGED)) {
		if ((ushort_t)bind_req->dl_sap == PROMISCUOUS_SAP) {
			DLerror_ack(q, mp, DL_ACCESS);
			return;
		}

		tmp = sap->bd->sap_ptr;
		i   = sap->bd->max_saps;
		while (i--) {
			if ((tmp->state    == DL_IDLE) &&
			    (tmp->sap_addr == (ushort_t)bind_req->dl_sap)) {
				DLerror_ack(q, mp, DL_BOUND);
				return;
			}
			tmp++;
		}

	}

	/*
	 *  Assign the SAP and set state.
	 */
	dlsap = (ushort_t)bind_req->dl_sap;
	old = splstr();
	if (dlsap <= MAXSAPVALUE) {
		/* Do not allow binds to null saps or group saps */
		if ( (!dlsap) || (dlsap & 0x1) || (dlsap == LLC_GLOBAL_SAP) ) {
			DLerror_ack(q, mp, DL_BADADDR);
			return;
		}
		sap->mac_type = DL_CSMACD;
		sap->max_spdu = DL_MAX_PACKET_LLC;
	} else {
		sap->mac_type = DL_ETHER;
		sap->max_spdu = DL_MAX_PACKET;
	}
	sap->sap_addr = dlsap;  /* Stored in host order */
	sap->state    = DL_IDLE;
	sap->next_sap = NULL;
	DLinsert_sap(sap,sap->bd);

	splx(old);

	if ((resp = allocb(DL_PRIMITIVES_SIZE + 
			DL_TOTAL_ADDR_LEN, BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 4, 1, SL_TRACE,
			"DLbind allocb failure on queue %x", (int)q));
		return;
	}

	/* LINTED pointer assignment */
	bind_ack		 = (dl_bind_ack_t *)resp->b_wptr;
	bind_ack->dl_primitive   = DL_BIND_ACK;
	bind_ack->dl_sap	 = sap->sap_addr;
	bind_ack->dl_addr_length = DL_TOTAL_ADDR_LEN;
	bind_ack->dl_addr_offset = DL_BIND_ACK_SIZE;
	bind_ack->dl_max_conind  = 0;
	bind_ack->dl_xidtest_flg = 0;	/*	VERSION 2 DLPI SUPPORT */

	resp->b_wptr          += DL_BIND_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/* LINTED pointer assignment */
	resp->b_wptr = copy_local_addr(sap, (ushort_t*)resp->b_wptr);
	qreply(q, resp);
}

/******************************************************************************
*   DLsubsbind_req()
*/

STATIC void
DLsubsbind_req(q,mp)
queue_t *q;
mblk_t *mp;
{
dl_subs_bind_req_t * subs_bind;
dl_subs_bind_ack_t * subs_bind_ack;
register int i;
struct snap_sap *subs_sap;

DL_sap_t *sap = q->q_ptr,*tsap;
DL_bdconfig_t *bd = sap->bd;
ulong_t snap_global = 0;
ushort_t snap_local = 0;
unsigned char *ptmp;
ulong snap_tmp;

ushort_t sapval;
int oldlevel;

	subs_bind = (dl_subs_bind_req_t *)mp->b_rptr;

	oldlevel = splstr();
        if (sap->state != DL_IDLE) {
		DLerror_ack(q, mp, DL_OUTSTATE);
		freemsg(mp);
		return;
	}
        sapval = sap->sap_addr;

        if ( (sapval != SNAPSAP) ||
		(subs_bind->dl_subs_sap_length != sizeof(struct snap_sap))) {
                        DLerror_ack(q, mp, DL_BADADDR);
                        freemsg(mp);
                        return;
	}
        subs_sap = (struct snap_sap *)((caddr_t)subs_bind + subs_bind->dl_subs_sap_offset);
	sap->state = DL_SUBS_BIND_PND;

        /*
	It is probably worth it to do some extra work here so that we can
	avoid the byte swapping when unitdata reqs are sent out
	*/
	snap_tmp = htonl(subs_sap->snap_global);
	ptmp = (unsigned char *)&snap_tmp;
	ptmp++;

	bcopy((caddr_t)ptmp,(caddr_t)&snap_global,SNAP_GLOBAL_SIZE);
	snap_local = ((ushort_t)subs_sap->snap_local);

        for (i = 0, tsap = bd->sap_ptr;i < bd->max_saps; i++,tsap++) {
		if ( (tsap->state == DL_IDLE) && (tsap->sap_addr == SNAPSAP) &&
                                (tsap->snap_global == snap_global)
                                        && (tsap->snap_local == snap_local) ) {
                        DLerror_ack(q, mp, DL_BOUND);
                        freemsg(mp);
                        return;
		}
	}
        sap->snap_global = snap_global;
	sap->snap_local = snap_local;
	sap->state = DL_IDLE;
	sap->flags |= SNAPCSMACD;
	sap->max_spdu = DL_MAX_PACKET_SNAP;

        subs_bind_ack = (dl_subs_bind_ack_t *)mp->b_rptr;
	subs_bind_ack->dl_primitive = DL_SUBS_BIND_ACK;
	subs_bind_ack->dl_subs_sap_offset = DL_SUBS_BIND_ACK_SIZE;
	subs_bind_ack->dl_subs_sap_length = SNAPSAP_SIZE;

        ptmp = (unsigned char *)((caddr_t)subs_bind_ack +
                                        subs_bind_ack->dl_subs_sap_offset);
	bcopy((caddr_t)&sap->snap_global,(caddr_t)ptmp,SNAP_GLOBAL_SIZE);
	ptmp += SNAP_GLOBAL_SIZE;
	bcopy((caddr_t)&sap->snap_local,(caddr_t)ptmp,SNAP_LOCAL_SIZE);
	mp->b_wptr = mp->b_rptr + DL_SUBS_BIND_ACK_SIZE + SNAPSAP_SIZE;
	qreply(q,mp);
	splx(oldlevel);
	return;
}



/******************************************************************************
 *  DLunbind_req()
 */
STATIC void
DLunbind_req(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	DL_sap_t	*sap = (DL_sap_t *)q->q_ptr;
	mblk_t		*resp;
	dl_ok_ack_t	*ok_ack;
	int		 old;

	DL_LOG(strlog(DL_ID, 5, 3, SL_TRACE,
		"DLunbind request for sap 0x%x on queue %x",
							sap->sap_addr, (int)q));

	/*
	 *  See if we are in the proper state to honor this request.
	 */
	if (sap->state != DL_IDLE) {
		DL_LOG(strlog(DL_ID, 5, 1, SL_TRACE,
			"DLunbind not valid for state %d on queue %x",
							sap->state, (int)q));
		DLerror_ack(q, mp, DL_OUTSTATE);
		return;
	}
		
	/* 
	 *  Allocate memory for response.  If none, toss the request.
	 */
	if ((resp = allocb(DL_PRIMITIVES_SIZE, BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 5, 1, SL_TRACE,
			"DLunbind allocb failure on queue %x", (int)q));
		return;
	}

	/*
	 *  Mark the SAP out of service and flush both queues
	 */
	old = splstr();

	sap->state    = DL_UNBOUND;
	sap->sap_addr = 0;
	sap->snap_global = sap->snap_local = 0;
	DLremove_sap(sap,sap->bd);
	sap->next_sap = NULL;
	flushq(q, FLUSHDATA);
	flushq(RD(q), FLUSHDATA);

	splx(old);
	/*
	 *  Generate and send the response.
	 */
	/* LINTED pointer assignment */
	ok_ack                       = (dl_ok_ack_t *)resp->b_wptr;
	ok_ack->dl_primitive         = DL_OK_ACK;
	ok_ack->dl_correct_primitive = DL_UNBIND_REQ;

	resp->b_wptr          += DL_OK_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;
	qreply(q, resp);

	return;
}

/******************************************************************************
 *  DLunitdata_req()
 */
STATIC void
DLunitdata_req(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	/* LINTED pointer assignment */
	dl_unitdata_req_t	*data_req = (dl_unitdata_req_t *)mp->b_rptr;
	DL_sap_t		*sap      = (DL_sap_t *)q->q_ptr;
	int			 size     = msgdsize(mp->b_cont);
	DL_eaddr_t		*ether_addr;
	int			 local, multicast, old;


	mblk_t *nmp,*tmp,*xmp;
	DL_mac_hdr_t *hdr;
	DL_bdconfig_t *bd = sap->bd;
	struct llcc *llccp;
	struct llcb *llcbp;
	int rval;


	DL_LOG(strlog(DL_ID, 6, 3, SL_TRACE,
		"DLunitdata request of %d bytes for sap 0x%x on queue %x",
					size, sap->sap_addr, (int)q));

	/*
	 *  If the board has gone down, reject the request.
	 */
	if((sap->bd->flags & (BOARD_PRESENT | BOARD_DISABLED))!=BOARD_PRESENT) {
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
			"DLunitdata request on disabled board for queue %x",
								(int)q));
		DLerror_ack(q, mp, DL_NOTINIT);
		freemsg(mp);
		return;
	}

	/*
	 *  Check for proper state and frame size.
	 */
	if (sap->state != DL_IDLE) {
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
			"DLunitdata not valid for state %d on queue %x",
							sap->state, (int)q));
		DLuderror_ind(q, mp, DL_OUTSTATE);
		return;
	}
		
	if (size > sap->max_spdu ) {
		DL_LOG(strlog(DL_ID, 6, 1, SL_TRACE,
			"DLunitdata frame size of %d is invalid for queue %x",
								size, (int)q));
		DLuderror_ind(q, mp, DL_BADDATA);
		return;
	}

	/*
	 *  Check for frame that we should send to ourself.
	 */
	/* LINTED pointer assignment */
	ether_addr = (DL_eaddr_t*)(mp->b_rptr + data_req->dl_dest_addr_offset);
	local = DLis_us(bd, ether_addr);
	multicast = DLis_broadcast(ether_addr) || DLis_multicast(sap->bd, ether_addr);
	/*
	 *  Update SNMP stats
	 */
	if (multicast)
		sap->bd->mib.ifOutNUcastPkts++;		/* SNMP */
	else
		sap->bd->mib.ifOutUcastPkts++;		/* SNMP */


	switch(sap->mac_type) {
	case DL_CSMACD:
		if (sap->sap_addr == SNAPSAP) {
			if ( (!(sap->flags & SNAPCSMACD)) || 
			      (!(nmp = DLform_snap(sap,mp,bd->eaddr.bytes)))){
			        	DLuderror_ind(q, mp, DL_UNDELIVERABLE);
					return;
			}
			size += (LLC_LSAP_HDR_SIZE + SNAPSAP_SIZE);
		} else {
		        llccp = (struct llcc *)((caddr_t)data_req + 
                                    data_req->dl_dest_addr_offset);
			if ( (nmp = DLform_80223(llccp->lbf_addr,bd->eaddr.bytes,size, (ushort_t)llccp->lbf_sap,sap->sap_addr, (ushort_t)(sap->flags & RAWCSMACD),(ushort_t)LLC_UI)) == NULL) {
				DLuderror_ind(q, mp, DL_UNDELIVERABLE);
				return;
			}
		        /* Add the LLC_LSAP_HDR_SIZE only for non RAW SAPs */
                        if (!(sap->flags & RAWCSMACD))
                                size += LLC_LSAP_HDR_SIZE;
		}
		break;
	case DL_ETHER:
		if ( (nmp = allocb(sizeof (union DL_primitives),BPRI_MED))
								  == NULL) {
			DLuderror_ind(q, mp, DL_UNDELIVERABLE);
			return;
		}
		nmp->b_cont = NULL;
		hdr = (struct DL_mac_hdr *)nmp->b_rptr;
		bcopy((caddr_t)ether_addr->bytes,(caddr_t)hdr->dst.bytes,DL_MAC_ADDR_LEN);
		bcopy((caddr_t)bd->eaddr.bytes,(caddr_t)hdr->src.bytes,DL_MAC_ADDR_LEN);
		hdr->mac_llc.ether.len_type = htons(sap->sap_addr);
		nmp->b_wptr = nmp->b_rptr + LLC_EHDR_SIZE;
		break;
	default:
		freemsg(mp);
		freemsg(nmp);
		return;
	}
	tmp = rmvb(mp,mp);
	freeb(mp);
	linkb(nmp,tmp);

	/* 	Use the original proto to pad the outbound message.
		We are assuming atleast 64 bytes in the original proto
		message and all we need is a maximum of 46.
	*/

	if (size < sap->min_spdu) {
		if ((mp = allocb(sap->min_spdu, BPRI_MED)) == NULL) {
			DLuderror_ind(q, mp, DL_UNDELIVERABLE);
			return;
		}
		mp->b_wptr = mp->b_rptr + (sap->min_spdu - size);
		linkb(nmp,mp);
	}

	if (local || multicast) {
		if(((xmp = dupmsg(nmp))== NULL) || (DLrecv(xmp,bd->sap_ptr)))
				bd->mib.ifOutDiscards++;
		if (local & (!(sap->flags & SEND_LOCAL_TO_NET)) ) {
			freemsg(nmp);
			return;
		}
	}

	/*
	 *  If controller is not busy, transmit the frame.  Otherwise put
	 *  it on our queue for the tx interrupt routine to handle.
	 */
	old = splstr();

	if (bd->flags & TX_BUSY) {
		bd->flags |= TX_QUEUED;
		bd->mib.ifOutQlen++;		/* SNMP */
		putq(q, nmp);
		DL_LOG(strlog(DL_ID, 6, 2, SL_TRACE,
			"DLunitdata hardware busy - xmit deferred for queue %x",
								(int)q));
	}
	else {
		if ( DLxmit_packet ( bd, nmp ) == 1 ) {
			/*
			 *      xmit was full -- queue this one up
			 */
		  	bd->flags |= (TX_QUEUED | TX_BUSY);
			bd->mib.ifOutQlen++;            /* SNMP */
			putq(q, nmp);
		}
       	}
	splx(old);
	return;
}

/******************************************************************************
 *  DLioctl()
 */
STATIC void
DLioctl(q, mp)
queue_t	*q;
mblk_t	*mp;
{
	/* LINTED pointer assignment */
	struct iocblk	*ioctl_req = (struct iocblk *)mp->b_rptr;
	DL_sap_t	*sap       = (DL_sap_t *)q->q_ptr;
	DL_bdconfig_t	*bd	   = sap->bd;
	int		old, failed, size1, size2;

	int	*tint;
	DL_sap_t *tsap;
	short count = 0;

	DL_LOG(strlog(DL_ID, 8, 3, SL_TRACE,
		"DLioctl request for command %d on queue %x",
					ioctl_req->ioc_cmd, (int)q));
	/*
	 *  Assume good stuff for now.
	 */
	ioctl_req->ioc_error = 0;
	mp->b_datap->db_type = M_IOCACK;

	/*
	 *  Screen for priviledged requests.
	 */
	switch (ioctl_req->ioc_cmd) {

	case DLIOCSMIB:		/* Set MIB */
	case DLIOCSENADDR:	/* Set ethernet address */
	case DLIOCSLPCFLG:	/* Set local packet copy flag */
	case DLIOCSPROMISC:	/* Toggle promiscuous state */
	case DLIOCADDMULTI:	/* Add multicast address */
	case DLIOCDELMULTI:	/* Delete multicast address */
	case DLIOCGETMULTI:	/* Get list of multicast addresses */
	case DLIOCDISABLE:	/* Disable controller */
	case DLIOCENABLE:	/* Enable controller */
	case DLIOCRESET:	/* Reset controller */
	case DLIOCCSMACDMODE:	/* Toggle CSMACD modes */
		/*
		 *  Must be privledged user to do these.
		 */
		if (drv_priv(ioctl_req->ioc_cr)) {
			DL_LOG(strlog(DL_ID, 8, 1, SL_TRACE,
			    "DLioctl invalid priviledge for queue %x", (int)q));

			ioctl_req->ioc_error = EPERM;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		}
	}

	/*
	 *  Make sure IOCTL's that require buffers have them.
	 */
	switch (ioctl_req->ioc_cmd) {

	case DLIOCGMIB:		/* Get MIB */
	case DLIOCSMIB:		/* Set MIB */
	case DLIOCGENADDR:	/* Get ethernet address */
	case DLIOCSENADDR:	/* Set ethernet address */
	case DLIOCADDMULTI:	/* Add multicast address */
	case DLIOCDELMULTI:	/* Delete multicast address */
	case DLIOCGETMULTI:	/* Get list of multicast address */
		/*
		 * Must have a non-null b_cont pointer.
		 */
		if (!mp->b_cont) {
			DL_LOG(strlog(DL_ID, 8, 1, SL_TRACE,
			    "DLioctl no data supplied by user for queue %x",
								(int)q));
			ioctl_req->ioc_error = EINVAL;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		}
	}

	/*
	 *  Process request.
	 */
	switch (ioctl_req->ioc_cmd) {
	case DLIOCCSMACDMODE:	/* Toggle CSMACD modes */
		if ( (sap->state != DL_IDLE) || (sap->mac_type != DL_CSMACD)					|| (sap->sap_addr == SNAPSAP)) {
			ioctl_req->ioc_error = EINVAL;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
		} else {
			old = splstr();
			DLremove_sap(sap,bd);
			flushq(sap->read_q, FLUSHDATA);
			flushq(sap->write_q, FLUSHDATA);
			if (sap->flags & RAWCSMACD)
				sap->flags &= ~RAWCSMACD;
			else
				sap->flags |= RAWCSMACD;
			DLinsert_sap(sap,bd);
			splx(old);
		}
		break;
	case DLIOCGETMULTI: 	/* Get list of multicast addresses	*/
		if (ioctl_req->ioc_count % 6) {
			ioctl_req->ioc_error = EINVAL;
			ioctl_req->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			return;
		} else
			ioctl_req->ioc_rval = DLget_multicast(bd,mp->b_cont);
		break;
	case DLIOCGMIB:		/* Get MIB */
		/*
		 *  We'll send as much as they asked for.
		 */
		size1 = min(sizeof(DL_mib_t), ioctl_req->ioc_count);
		size2 = min(strlen(DLid_string) + 1, ioctl_req->ioc_count - size1);

		/*
		 *  Set some MIB items before copy
		 */
		bd->mib.ifDescrLen = size2;
		/* LINTED pointer assignment */
		(void)copy_local_addr(sap, (ushort_t*)&bd->mib.ifPhyAddress);
		mp->b_cont->b_wptr = mp->b_cont->b_rptr;

		old = splstr();
		bcopy((caddr_t)&bd->mib, (caddr_t)mp->b_cont->b_wptr, size1);
		splx(old);

		mp->b_cont->b_wptr += size1;

		if (size2) {
			strncpy((char*)mp->b_cont->b_wptr, DLid_string, size2);
			*(char*)(mp->b_cont->b_wptr + size2 - 1) = '\0';
			mp->b_cont->b_wptr += size2;
		}

		ioctl_req->ioc_count = size1 + size2;

		break;

	case DLIOCSMIB:		/* Set MIB */
		/*
		 *  We currently don't let them set the "ifDecr".
		 */
		old = splstr();
		bcopy((caddr_t)mp->b_cont->b_rptr,(caddr_t)&bd->mib,
				min(sizeof(DL_mib_t), ioctl_req->ioc_count));
		ioctl_req->ioc_count = 0;

		if (!(bd->flags & BOARD_PRESENT))
			bd->mib.ifOperStatus = DL_DOWN;

		splx(old);
		break;

	case DLIOCGENADDR:	/* Get ethernet address */
		/*
		 *  We'll send as much as they asked for.
		 */
		size1 = min(sizeof(DL_eaddr_t), ioctl_req->ioc_count);
		bcopy((caddr_t)&bd->eaddr, (caddr_t)mp->b_cont->b_rptr, size1);
		ioctl_req->ioc_count = size1;

		break;

	case DLIOCSENADDR:	/* Set ethernet address */
#ifdef ALLOW_SET_EADDR
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
		    DLset_eaddr(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		} else {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
#else
		ioctl_req->ioc_error = EINVAL;
		mp->b_datap->db_type = M_IOCNAK;
#endif

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCGLPCFLG:	/* Get local packet copy flag */
		ioctl_req->ioc_rval  = (bd->flags & SEND_LOCAL_TO_NET);
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCSLPCFLG:	/* Set local packet copy flag */
		bd->flags |= SEND_LOCAL_TO_NET;
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCGPROMISC:	/* Get promiscuous state */
		ioctl_req->ioc_rval  = (sap->flags & PROMISCUOUS);
		ioctl_req->ioc_count = 0;
		break;

	case DLIOCSPROMISC:	/* Toggle promiscuous state */
		if (sap->flags & PROMISCUOUS)
			failed = DLpromisc_off(bd);
		else
			failed = DLpromisc_on(bd);

		if (failed) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		} else
			sap->flags ^= PROMISCUOUS;

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCADDMULTI:	/* Add multicast address */
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
		    DLadd_multicast(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}

		ioctl_req->ioc_count = 0;
		break;

	case DLIOCDELMULTI:	/* Delete multicast address */
		if ((ioctl_req->ioc_count < sizeof(DL_eaddr_t)) ||
			/* LINTED pointer assignment */
		    DLdel_multicast(bd, (DL_eaddr_t*)mp->b_cont->b_rptr)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}

		ioctl_req->ioc_count = 0;
		break;
	
	case DLIOCDISABLE:	/* Disable controller */
		if (DLdisable(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		else
			bd->flags |= BOARD_DISABLED;
		break;

	case DLIOCENABLE:	/* Enable controller */
		if (DLenable(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		else
			bd->flags &= ~BOARD_DISABLED;
		break;

	case DLIOCRESET:	/* Reset controller */
		if (DLreset(bd)) {
			ioctl_req->ioc_error = EINVAL;
			mp->b_datap->db_type = M_IOCNAK;
		}
		break;

	case SIOCGIFFLAGS:	/* IP get ifnet flags */
		break;		/* just ignore or */

	default:
		/* Give the ioctl one more chance to be recognized */
		DLbdspecioctl(q, mp);
	}

	qreply(q, mp);
}

/******************************************************************************
 *  DLrsrv()
 */
STATIC void
DLrsrv(q)
queue_t	*q;
{
	DL_sap_t	*sap;
	DL_sap_t	*next_sap;
	DL_bdconfig_t	*bd;
	mblk_t		*mp_data, *mp_ind, *mp_dup;
	DL_mac_hdr_t	*hdr;
	int		i;
	ushort_t	sap_id,dsap,ssap,control;

	/*
	 *  Process all messages waiting.
	 */

	sap = (DL_sap_t *)q->q_ptr;
	bd  = sap->bd;
	while (mp_data = getq(q)) {
		/*
		 *  Set some convient pointers.
		 */
		/* LINTED pointer assignment */
		hdr = (DL_mac_hdr_t*)mp_data->b_rptr;
		switch (sap->mac_type) {
		case DL_ETHER:
			/*
		 	*  Convert the SAP to host order
		 	*/
			sap_id = ntohs(hdr->mac_llc.ether.len_type);
			/*
		 	*  Create an indication message for this frame.
		 	*/
			if ((mp_ind = DLmk_ud_ind(mp_data,sap)) == NULL) {
				freemsg(mp_data);
				DL_LOG(strlog(DL_ID, 9, 1, SL_TRACE,
				   "DLrsrv: - No receive buffer resources"));
				bd->mib.ifInDiscards++;		/* SNMP */
				continue;
			}
			break;
		case DL_CSMACD:
			sap_id = LLC_DSAP(hdr);
			/*
		 	*  Create an indication message for this frame.
		 	*/
			if ((mp_ind = DLproc_llc(q,mp_data)) == NULL) {
				freemsg(mp_data);
				continue;
			}
			break;
		default:
			freemsg(mp_data);
			continue;
		}
		/*
		 *  Go through the SAP list and see if anyone else is
		 *  interested in this frame.
		 */

		linkb(mp_ind, mp_data);
		for ( next_sap = bd->valid_sap;next_sap;next_sap= next_sap->next_sap) {
			/*
			 *  Skip SAP indication this came in on.
			 */
			if (next_sap == sap) {
				continue;
			} else if (next_sap->state != DL_IDLE) {
				continue;
			} else if (next_sap->sap_addr == PROMISCUOUS_SAP) {
				CHK_FLOWCTRL_DUP_PUT(next_sap,mp_ind,mp_dup);
			} else if ( (sap->sap_addr == PROMISCUOUS_SAP) || (sap->flags & RAWCSMACD) ) {
				continue;
			} else if (next_sap->mac_type != sap->mac_type) {
				continue;
			} else if (next_sap->mac_type == DL_ETHER) {
				if (next_sap->sap_addr == sap_id)
				   CHK_FLOWCTRL_DUP_PUT(next_sap,mp_ind,mp_dup);
			} else if (next_sap->mac_type == DL_CSMACD) {
				if ( (!(next_sap->flags & RAWCSMACD)) && ((next_sap->sap_addr == sap_id) || (sap_id  == LLC_GLOBAL_SAP)) ) {
					if ( (sap_id == SNAPSAP) && (!DLis_equalsnap(sap,next_sap)) )
						continue;
				   CHK_FLOWCTRL_DUP_PUT(next_sap,mp_ind,mp_dup);
				}
			}
		}
		/*
		 *  Don't forget the one who brung us.
		 */
		if (canput(sap->read_q->q_next)) {
			putnext(sap->read_q, mp_ind);

			DL_LOG(strlog(DL_ID, 9, 3, SL_TRACE, "DLrsrv queue %x",
							(int)sap->read_q));
			/*
			 *  Update SNMP stats.
			 */
			if (IS_MULTICAST(hdr->dst))		
				bd->mib.ifInNUcastPkts++;
			else
				bd->mib.ifInUcastPkts++;
		} else {
			freemsg(mp_ind);
			bd->mib.ifInDiscards++;			/* SNMP */
		}
	}
	return;
}

STATIC mblk_t *
DLmk_test_con(dst,src,dsap_id,ssap_id,control)
DL_eaddr_t *dst;
DL_eaddr_t *src;
ushort_t dsap_id;
ushort_t ssap_id;
ushort_t control;
{
dl_test_con_t *test_con;
mblk_t *mp;
register struct llcc *llccp;

	if ((mp = allocb((DL_TEST_CON_SIZE + (sizeof(DL_eaddr_t)  * 2) +
                                                (sizeof(ushort_t)    * 2)),
                                                        BPRI_MED)) == NULL)
                return (NULL);
	mp->b_datap->db_type  = M_PROTO;
	test_con = (dl_test_con_t *)mp->b_rptr;
	test_con->dl_primitive = DL_TEST_CON;
	if (control & LLC_P)
		test_con->dl_flag = DL_POLL_FINAL;
	test_con->dl_dest_addr_offset = DL_TEST_CON_SIZE;
	test_con->dl_dest_addr_length = LLC_LIADDR_LEN;
	test_con->dl_src_addr_length  = LLC_LIADDR_LEN;
	test_con->dl_src_addr_offset  = DL_TEST_CON_SIZE + LLC_LIADDR_LEN;
	llccp = (struct llcc *)( (caddr_t)test_con + DL_UNITDATA_IND_SIZE);
	bcopy((caddr_t)dst,(caddr_t)llccp->lbf_addr,DL_MAC_ADDR_LEN);
	llccp->lbf_sap = dsap_id;
	llccp++;
	bcopy((caddr_t)src,(caddr_t)llccp->lbf_addr,DL_MAC_ADDR_LEN);
	llccp->lbf_sap = ssap_id;
	mp->b_wptr = mp->b_rptr + (DL_TEST_CON_SIZE + 2 * LLC_LIADDR_LEN);
	return mp;
}

/******************************************************************************
 *  DLerror_ack()
 */
STATIC	void
DLerror_ack(q, mp, error)
queue_t	*q;
mblk_t	*mp;
int	error;
{
	/* LINTED pointer assignment */
	union	DL_primitives	*prim = (union DL_primitives*)mp->b_rptr;
	dl_error_ack_t		*err_ack;
	mblk_t			*resp;

	/*
	 *  Allocate the response resource.  If we can't, just return.
	 */
	if ((resp = allocb(sizeof(union DL_primitives), BPRI_MED)) == NULL) {
		DL_LOG(strlog(DL_ID, 10, 1, SL_TRACE,
		    "DLerror_ack - no resources for error response on queue %x",
								(int)q));
		return;
	}

	/*
	 *  Fill it in.
	 */
	/* LINTED pointer assignment */
	err_ack                     = (dl_error_ack_t*)resp->b_wptr;
	err_ack->dl_primitive       = DL_ERROR_ACK;
	err_ack->dl_error_primitive = prim->dl_primitive;
	err_ack->dl_errno           = (ulong)error;
	err_ack->dl_unix_errno      = 0;

	resp->b_wptr += DL_ERROR_ACK_SIZE;
	resp->b_datap->db_type = M_PCPROTO;

	/*
	 *  Send it
	 */
	qreply(q, resp);
	return;
}

/******************************************************************************
 *  DLuderror_ind()
 */
STATIC	void
DLuderror_ind(q, mp, error)
queue_t	*q;
mblk_t	*mp;
int	error;
{
	/* LINTED pointer assignment */
	dl_uderror_ind_t   *uderr_ind = (dl_uderror_ind_t*)mp->b_rptr;

	/*
	 *  The unit data request is guaranteed to accomodate a unit data
	 *  error indication so we will just convert the data request.
	 */
	uderr_ind->dl_primitive = DL_UDERROR_IND;
	/* uderr_ind->dl_reserved  = 0; */	/* VERSION 2 DLPI SUPPORT */
	uderr_ind->dl_errno     = (ulong)error;

	qreply(q, mp);
	return;
}

/******************************************************************************
 *  DLis_us()
 */
DLis_us(bd, dst)
DL_bdconfig_t *bd;
DL_eaddr_t	*dst;
{
	return bcmp((char *)bd->eaddr.bytes,(char *)dst->bytes,(size_t)DL_MAC_ADDR_LEN) == 0;
}

/******************************************************************************
 *  DLis_broadcast()
 */
DLis_broadcast(dst)
DL_eaddr_t		*dst;
{
        int     i;

        for (i = 0; i < (sizeof(DL_eaddr_t) / 2); i++)
                if (dst->words[ i ] != 0xffff)
                        return (0);
        return (1);
}

/******************************************************************************
 *  ntoa()
 */
static char
ntoa(val)
uchar_t	val;
{
	if (val < 10)
		return (val + 0x30);
	else
		return (val + 0x57);
}

/******************************************************************************
 *  DLprint_eaddr()
 */
void
DLprint_eaddr(addr)
uchar_t	addr[];
{
	int	i;
	char	a_eaddr[ DL_MAC_ADDR_LEN * 3 ];

	for (i = 0; i < DL_MAC_ADDR_LEN; i++) {
		a_eaddr[ i * 3     ] = ntoa((uchar_t)(addr[ i ] >> 4 & 0x0f));
		a_eaddr[ i * 3 + 1 ] = ntoa((uchar_t)(addr[ i ]      & 0x0f));
		a_eaddr[ i * 3 + 2 ] = ':';
	}

	a_eaddr[ sizeof(a_eaddr) - 1 ] = '\0';
	cmn_err(CE_CONT, "Ethernet Address: %s\n", a_eaddr);
}

/******************************************************************************
 *  copy_local_addr()
 */
static uchar_t*
copy_local_addr(sap, dst)
DL_sap_t	*sap;
ushort_t	*dst;
{
	ushort_t	*src = sap->bd->eaddr.words;
	int		i;

	for (i = 0; i < (sizeof (DL_eaddr_t)) / 2; i++)
		*dst++ = *src++;
	*dst++ = sap->sap_addr;

	return((uchar_t*)dst);
}

static uchar_t *
copy_broad_addr(dst)
ushort_t *dst;
{
        int     i;

        for (i = 0; i < (sizeof(DL_eaddr_t) / 2); i++)
                *dst++ = 0xffff;
	return((uchar_t*)dst);
}


mblk_t *
DLproc_llc(q,mp_data)
queue_t *q;
mblk_t *mp_data;
{
	DL_mac_hdr_t *hdr = (DL_mac_hdr_t *)mp_data->b_rptr;
	DL_mac_hdr_t *newhdr;
	DL_sap_t *sap = (DL_sap_t *)q->q_ptr;
	DL_bdconfig_t *bd = sap->bd;
	ushort_t dsap,ssap,control;
	mblk_t *data_ind = NULL,*nmp,*xmp = NULL;
	int old;
	ushort actlen;


	dsap = LLC_DSAP(hdr);
	ssap = LLC_SSAP(hdr);
	control = LLC_CONTROL(hdr);

	if (sap->flags & RAWCSMACD) {
		if ( (data_ind = DLmk_ud_ind(mp_data,sap)) )
			return data_ind;
		else
			goto llc_discard;
	}

	if (ssap & LLC_RESPONSE) {
			/* 
                           Discard non test responses and test responses that
			   cannot be translated into test confirmations.
			*/
		if ( (control == LLC_TEST) || (control == (LLC_TEST|LLC_P)) ) {
			if (canput(sap->read_q->q_next)) {
				if ( (xmp = copyb(mp_data)) && (data_ind = DLmk_test_con(&hdr->dst,&hdr->src, dsap,ssap, sap->mac_type)) ) {
                                        newhdr = (DL_mac_hdr_t *)xmp->b_rptr;
                                        actlen = LLC_LENGTH(newhdr);
					xmp->b_rptr += LLC_HDR_SIZE;
                                        xmp->b_wptr = xmp->b_rptr +
                                                (actlen - LLC_LSAP_HDR_SIZE);
					linkb(data_ind,xmp);
					putnext(sap->read_q,data_ind);
					return NULL;
				} else {
					if (xmp)
						freemsg(xmp);
					if (data_ind)
						freemsg(data_ind);
				}
			}
		}
		goto llc_discard;
	} else {
		switch(control) {
		case LLC_UI:
			/* 
			    Discard  UI messages that cannot be translated into
			    data indications; Return a valid data indication.
			*/ 
			if ( (data_ind = DLmk_ud_ind(mp_data,sap)) == NULL)
				goto llc_discard;
			return data_ind;
		case LLC_TEST:
		case LLC_TEST |LLC_P:
			if ( (xmp = copymsg(mp_data)) == NULL) {
				goto llc_discard;
			}
			xmp->b_rptr += LLC_HDR_SIZE;
			if ( (nmp = DLform_80223(hdr->src.bytes,bd->eaddr.bytes, msgdsize(xmp),ssap,(sap->sap_addr |LLC_RESPONSE),(ushort_t)0,control))== NULL) {
				freemsg(xmp);
				goto llc_discard;
			}
			nmp->b_cont = xmp;
			break;

		case LLC_XID:		/* Need to respond to a XID command */
		case LLC_XID|LLC_P:
			if ( (nmp = allocb(sizeof(union DL_primitives) + 
					(2* LLC_LIADDR_LEN),BPRI_MED)) == NULL)
				goto llc_discard;
			newhdr = (DL_mac_hdr_t *)nmp->b_rptr;
			bcopy((caddr_t)hdr->src.bytes,(caddr_t)newhdr->dst.bytes, LLC_ADDR_LEN);
			bcopy((caddr_t)bd->eaddr.bytes,(caddr_t)newhdr->src.bytes, LLC_ADDR_LEN);
			newhdr->mac_llc.llc.llc_dsap = ssap;
			newhdr->mac_llc.llc.llc_ssap=sap->sap_addr|LLC_RESPONSE;
			newhdr->mac_llc.llc.llc_control = control;
			newhdr->mac_llc.llc.llc_info[0] = LLC_XID_FMTID;
			newhdr->mac_llc.llc.llc_info[1] = LLC_SERVICES;
			newhdr->mac_llc.llc.llc_info[2] = 0;
			newhdr->mac_llc.llc.llc_length = htons(LLC_XID_INFO_SIZE+LLC_LSAP_HDR_SIZE);
			nmp->b_wptr = nmp->b_rptr + LLC_XID_INFO_SIZE+LLC_HDR_SIZE;
			/* A valid XID response can now be transmitted */
			break;
		default:
				goto llc_discard;
		}
	        old = splstr();
        	if (bd->flags & TX_BUSY) {
 			bd->flags |= TX_QUEUED;
                	bd->mib.ifOutQlen++;            /* SNMP */
                	putq(sap->write_q, nmp);
                	DL_LOG(strlog(DL_ID, 6, 2, SL_TRACE,
                       	     "DLunitdata hardware busy - xmit deferred for queue %x",(int)q));
		}
		else {
			if ( DLxmit_packet ( bd, nmp ) == 1 ) {
				/*
				 *      xmit was full -- queue this one up
				 */
			  	bd->flags |= (TX_QUEUED | TX_BUSY);
				bd->mib.ifOutQlen++;            /* SNMP */
				putq(q, nmp);
			}
        	}
        	splx(old);
		return NULL;
	}
llc_discard:
	bd->mib.ifInDiscards++;	/* SNMP */
	return NULL;
}

void
DLremove_sap(sap,bd)
DL_sap_t *sap;
DL_bdconfig_t *bd;
{
DL_sap_t *t1,*t2;

	for(t1= bd->valid_sap,t2 = t1;t1 && t1 != sap;t2 = t1,t1= t1->next_sap)
				;
	if (!t1)
		return;
	if (t1 == t2)
		bd->valid_sap = t1->next_sap;
	else
		t2->next_sap = t1->next_sap;
	sap->next_sap = NULL;

	bd->ttl_valid_sap = 0;
	t1 = bd->valid_sap;
	while( t1 ) {
		bd->ttl_valid_sap++;
		t1 = t1->next_sap;
	}
}

void
DLinsert_sap(sap,bd)
DL_sap_t *sap;
DL_bdconfig_t *bd;
{
DL_sap_t *t1;
DL_sap_t *t2;

	if (!bd->valid_sap) {
		bd->valid_sap = sap;
		sap->next_sap = NULL;
	} else {
		if (sap->sap_addr == PROMISCUOUS_SAP) {
			t1 = bd->valid_sap;
			while(t1->next_sap)
				t1 = t1->next_sap;
			t1->next_sap = sap;
			sap->next_sap = NULL;
		} else if (sap->flags & RAWCSMACD) {
			t1 = t2 = bd->valid_sap;
			while (t1) {
				if ( (t1->sap_addr == PROMISCUOUS_SAP) ||
					(t1->flags & RAWCSMACD) )
					break;
				t2 = t1;
				t1 = t1->next_sap;
			}
			if (!t1) {
				t2->next_sap  = sap;
				sap->next_sap = NULL;
			} else if (t2 == t1) {
				sap->next_sap = t2;
				bd->valid_sap = sap;
			} else {
				t2->next_sap = sap;
				sap->next_sap = t1;
			}	
		} else {
			t1 = bd->valid_sap;
			bd->valid_sap = sap;
			sap->next_sap = t1;
		}
	}
	bd->ttl_valid_sap = 0;
	t1 = bd->valid_sap;
	while( t1 ) {
		bd->ttl_valid_sap++;
		t1 = t1->next_sap;
	}
}

DLis_equalsnap(s1,s2)
DL_sap_t *s1;
DL_sap_t *s2;
{
	return ((s1->snap_local == s2->snap_local) && (s1->snap_global == s2->snap_global));
}


STATIC  mblk_t*
DLmk_ud_ind(mp_data,sap)
mblk_t *mp_data;
DL_sap_t *sap;
{
register struct llca *llcap;
register struct llcb *llcbp;
register struct llcc *llccp;
dl_unitdata_ind_t *data_ind;
mblk_t *mp;
ushort_t *word_p;
unsigned char dsap;
int i;
uint msgtype,len_type_field;
ushort actlen;

DL_mac_hdr_t *hdr = (DL_mac_hdr_t *)mp_data->b_rptr;

	len_type_field = LLC_LENGTH(hdr);
	msgtype = (len_type_field > DL_MAX_PACKET) ? DL_ETHER : DL_CSMACD;

        if ((mp = allocb((DL_UNITDATA_IND_SIZE + (sizeof(struct llcb) * 2)),BPRI_MED)) == NULL)
                return (NULL);
        /* set data type */
        mp->b_datap->db_type = M_PROTO;
        data_ind                      =(dl_unitdata_ind_t *)mp->b_rptr;
        data_ind->dl_primitive        = DL_UNITDATA_IND;
        data_ind->dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;

	switch(msgtype) {
	case DL_ETHER:
		data_ind->dl_dest_addr_length = DL_TOTAL_ADDR_LEN;
                data_ind->dl_src_addr_length  = DL_TOTAL_ADDR_LEN;
                data_ind->dl_src_addr_offset  = DL_UNITDATA_IND_SIZE +
                                                        DL_TOTAL_ADDR_LEN;
                llcap = (struct llca *)( (caddr_t)data_ind +
                                                DL_UNITDATA_IND_SIZE);
                bcopy((caddr_t)hdr->dst.bytes,(caddr_t)llcap->lbf_addr,DL_MAC_ADDR_LEN);
                llcap->lbf_sap = len_type_field;
                llcap++;
                bcopy((caddr_t)hdr->src.bytes,(caddr_t)llcap->lbf_addr,DL_MAC_ADDR_LEN);
                llcap->lbf_sap = len_type_field;
                mp->b_wptr = mp->b_rptr + (DL_UNITDATA_IND_SIZE +
                                                2 * DL_TOTAL_ADDR_LEN);
		mp_data->b_rptr += LLC_EHDR_SIZE;
                return mp;
	case DL_CSMACD:
		dsap = LLC_DSAP(hdr);
		actlen = LLC_LENGTH(hdr);
		if ( (sap->flags & RAWCSMACD) || (dsap != SNAPSAP) ) {
                	data_ind->dl_dest_addr_length = LLC_LIADDR_LEN;
                	data_ind->dl_src_addr_length  = LLC_LIADDR_LEN;
                	data_ind->dl_src_addr_offset  = DL_UNITDATA_IND_SIZE +
                                                        LLC_LIADDR_LEN;
                	llccp = (struct llcc *)( (caddr_t)data_ind +
                                                DL_UNITDATA_IND_SIZE);
                	bcopy((caddr_t)hdr->dst.bytes,(caddr_t)llccp->lbf_addr,DL_MAC_ADDR_LEN);
                	llccp->lbf_sap = dsap;
                	llccp++;
                	bcopy((caddr_t)hdr->src.bytes,(caddr_t)llccp->lbf_addr,DL_MAC_ADDR_LEN);
                	llccp->lbf_sap = LLC_SSAP(hdr);
                	mp->b_wptr = mp->b_rptr + (DL_UNITDATA_IND_SIZE +
                                                2 * LLC_LIADDR_LEN);
			if (sap->flags & RAWCSMACD) {
				mp_data->b_rptr += LLC_EHDR_SIZE;
				mp_data->b_wptr = mp_data->b_rptr + actlen;
			} else {
				mp_data->b_rptr += LLC_HDR_SIZE;
				mp_data->b_wptr = mp_data->b_rptr +
                                        (actlen - LLC_LSAP_HDR_SIZE);
			}
		} else {
			data_ind->dl_dest_addr_length = sizeof(struct llcb); 
			data_ind->dl_src_addr_length = sizeof(struct llcb); 
			data_ind->dl_src_addr_offset = DL_UNITDATA_IND_SIZE + sizeof(struct llcb);
        		llcbp = (struct llcb *) ((caddr_t)data_ind + data_ind->dl_dest_addr_offset);
        		bcopy((caddr_t)hdr->dst.bytes,(caddr_t)llcbp->lbf_addr,DL_MAC_ADDR_LEN);        
			llcbp->lbf_sap = dsap;

        		llcbp->lbf_xsap = 0;
        		bcopy((caddr_t)hdr->mac_llc.snap.snap_org,(caddr_t)&llcbp->lbf_xsap, SNAP_GLOBAL_SIZE);

        		llcbp->lbf_type = SNAP_TYPE(hdr);
        		llcbp++;
        		bcopy((caddr_t)hdr->src.bytes,(caddr_t)llcbp->lbf_addr,DL_MAC_ADDR_LEN);        
			llcbp->lbf_sap = sap->sap_addr;
        		llcbp->lbf_xsap = sap->snap_global;
        		llcbp->lbf_type = SNAP_TYPE(hdr);
        		mp->b_wptr = mp->b_rptr + DL_UNITDATA_IND_SIZE + (2 * sizeof(struct llcb));
			mp_data->b_rptr += SNAP_HDR_SIZE;
			mp_data->b_wptr = mp_data->b_rptr +
                                (actlen - SNAPSAP_SIZE - LLC_LSAP_HDR_SIZE);
		}
                return mp;
	}
}

int
DLrecv(mp,tsap)
mblk_t *mp;
DL_sap_t *tsap;
{
register DL_bdconfig_t *bd;
int msgtype, len_type_field;
register int i;
int valid;
DL_mac_hdr_t *hdr;
uchar_t dsap;
DL_sap_t *sap;

        if ((tsap == NULL) || ((bd = tsap->bd) == NULL)) {
                freemsg(mp);
                return 1;
        }
	if ((int)(mp->b_wptr - mp->b_rptr) > DL_MAX_PLUS_HDR) {
		cmn_err(CE_CONT, "an unusually large packet of size %d was received and discarded", (int)(mp->b_wptr - mp->b_rptr));
		hdr = (DL_mac_hdr_t *)mp->b_rptr;
		cmn_err(CE_WARN, "Source of errant packet:");
		for (i = 0; i < 6; i++)
			cmn_err(CE_CONT, "%x:", hdr->src.bytes[i]);
		bd->mib.ifInUnknownProtos++;
		freemsg(mp);
		return(0);
	}
        len_type_field = LLC_LENGTH(mp->b_rptr);
        msgtype = (len_type_field > DL_MAX_PACKET) ? DL_ETHER : DL_CSMACD;
        hdr = (DL_mac_hdr_t *)mp->b_rptr;

        if ( (bd->multicast_cnt > 0) || (bd->promisc_cnt > 0)) {
                hdr = (DL_mac_hdr_t *)mp->b_rptr;
                valid = DLis_us(bd,(DL_eaddr_t *)&(hdr->dst))
                                ||
                        DLis_broadcast((DL_eaddr_t *)&(hdr->dst))
                                ||
                        DLis_multicast(bd,(DL_eaddr_t *)&(hdr->dst));
        } else
                valid = 1;
        for ( sap = bd->valid_sap;sap;sap= sap->next_sap) {
                if ( (sap->state != DL_IDLE) || (sap->write_q == NULL) )
                        continue;
                if (sap->sap_addr == PROMISCUOUS_SAP) {
                        CHK_FLOWCTRL_PUT(sap,mp);
                } else if (sap->mac_type != msgtype) {
                        continue;
                } else if ( valid && sap->mac_type == DL_ETHER &&
                                        (sap->sap_addr == len_type_field) ) {
                        CHK_FLOWCTRL_PUT(sap,mp);
                } else if (valid && sap->mac_type == DL_CSMACD)  {
                        dsap = hdr->mac_llc.llc.llc_dsap;
                        if (dsap == sap->sap_addr) {
                           if ( (dsap == SNAPSAP)&&(!DLis_validsnap(hdr,sap)) )
                                        continue;
                                CHK_FLOWCTRL_PUT(sap,mp);
                        } else if ( (dsap == LLC_GLOBAL_SAP) &&
                                   (!(sap->flags & (SNAPCSMACD|RAWCSMACD)))) {
                                CHK_FLOWCTRL_PUT(sap,mp);
                        } else if ( sap->flags & RAWCSMACD) {
                                CHK_FLOWCTRL_PUT(sap,mp);
                        }
                }
        }
        bd->mib.ifInUnknownProtos++;
        freemsg(mp);
        return 1;
}


int
DLis_validsnap(hdr,sap)
DL_mac_hdr_t *hdr;
DL_sap_t *sap;
{
        ulong_t snap_global = 0;

        if (!(sap->flags & SNAPCSMACD))
                return 0;
        bcopy((caddr_t)hdr->mac_llc.snap.snap_org,(caddr_t)&snap_global,SNAP_GLOBAL_SIZE);
        if ( (snap_global != sap->snap_global) || (sap->snap_local != SNAP_TYPE(hdr)) )
                return 0;
        return 1;
}

