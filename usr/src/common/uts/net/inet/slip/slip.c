/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/slip/slip.c	1.8"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */


#include <util/types.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <net/dlpi.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>

/* for SIOC* defs */
#include <net/socket.h>
#include <net/sockio.h>
#include <net/inet/if.h>

/* headers needed for keeping netstat stat's. */
#include <mem/kmem.h>
#include <net/socketvar.h>

#include <net/inet/in.h>
#include <net/inet/in_systm.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/in_comp.h>
#include <net/inet/slip/slip.h>

#include <io/ddi.h>		/* must come last */

#if !defined(MAX_HDR)
#define MAX_HDR	0
#endif
#if !defined(MIN_RECV_MTU)
#define MIN_RECV_MTU 1500
#endif
#define SS_MTU (ssp->ss_mtu < MIN_RECV_MTU ? MIN_RECV_MTU : ssp->ss_mtu)
#define RECV_SPACE (SS_MTU + MAX_HDR)

#define FRAME_END		0xc0		/* Frame End */
#define FRAME_ESCAPE		0xdb		/* Frame Esc */
#define TRANS_FRAME_END		0xdc		/* transposed frame end */
#define TRANS_FRAME_ESCAPE	0xdd		/* transposed frame esc */

#define SLIPHIER  1	/* slip lock hierarchy value */

#define	SLIPMODNAME	"slip - Loadable Serial Line IP module"

MOD_STR_WRAPPER(slip, NULL, NULL, SLIPMODNAME);

STATIC struct module_info slip_rwinfo = {
	SLIP_MODID,
	"slip",
	0,
	INFPSZ,
	8192,			/* HIWAT */
	2048			/* LOWAT */
};

STATIC int slip_open(queue_t *, dev_t *, int, int, cred_t *);
STATIC int slip_close(queue_t *, int, cred_t *);
STATIC int slip_rput(queue_t *, mblk_t *);
STATIC int slip_rsrv(queue_t *);
STATIC void slip_in(queue_t *, mblk_t *);
STATIC int slip_wput(queue_t *, mblk_t *);
STATIC int slip_wsrv(queue_t *);
STATIC void slip_info(queue_t *, mblk_t *);
STATIC void slip_bind(queue_t *, mblk_t *);
STATIC void slip_unbind(queue_t *, mblk_t *);
STATIC void slip_out(queue_t *, mblk_t *);
STATIC void slip_ioctl(queue_t *, mblk_t *);
STATIC mblk_t * slip_alloc_unitdata(void);
STATIC void slip_dlpi_error(queue_t *, mblk_t *, unsigned long, 
		unsigned long, unsigned long);
STATIC void slip_dlpi_uderror(queue_t *, mblk_t *, unsigned long);

STATIC struct qinit slip_rinit = {
	slip_rput, slip_rsrv, slip_open, slip_close, NULL, &slip_rwinfo, NULL
};

STATIC struct qinit slip_winit = {
	slip_wput, slip_wsrv, NULL, NULL, NULL, &slip_rwinfo, NULL
};

struct streamtab slipinfo = {
	&slip_rinit, &slip_winit, NULL, NULL
};

/*
 * The following disgusting hack gets around the problem that IP TOS
 * can't be set in BSD/Sun OS yet.  We want to put "interactive"
 * traffic on a high priority queue.  To decide if traffic is
 * interactive, we check that a) it is TCP and b) one of it's ports
 * is telnet, rlogin or ftp control.
 * 
 * And we use it here in SysV in addition to checking TOS.
 */
STATIC u_short interactive_ports[8] = {
        0,      513,    0,      0,
        0,      21,     0,      23,
};
#define INTERACTIVE(p) (interactive_ports[(p) & 7] == (p))

STATIC int slip_pp_frame_end = 1;

int slipdevflag = D_MP;

STATIC LKINFO_DECL(slip_lkinfo, "NETINET:SLIP:ss_lock", 0);

/*
 * STATIC int
 * slip_open(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	allocate the state structures, initialize the ifstat structure.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
/*ARGSUSED*/
STATIC int
slip_open(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	slip_state_t	*ssp;
	struct ifstats	*statp;
	pl_t	pl;

	if (q->q_ptr != NULL)
		return (0);		/* already attached */

	if (sflag != MODOPEN)
		return (EINVAL);

	if ((ssp = (slip_state_t *)kmem_zalloc((int)sizeof(slip_state_t), 
			KM_NOSLEEP)) == NULL)
		return (ENOSPC);

	if ((ssp->ss_lock = LOCK_ALLOC(SLIPHIER, plstr, &slip_lkinfo, 
			KM_NOSLEEP)) == NULL) {
		kmem_free(ssp, sizeof(slip_state_t));
		return (ENOSPC);
	}

	q->q_ptr = (caddr_t)ssp;
	WR(q)->q_ptr = (caddr_t)ssp;

	ssp->ss_flags = SS_INIT_FLAGS;
	ssp->ss_ioctl_mp = NULL;
	ssp->ss_mtu = MAXSLIP;
	bcopy("slip", ssp->ss_ifname, 5);
	if (!(ssp->ss_mp = allocb(RECV_SPACE, BPRI_MED))) {
		LOCK_DEALLOC(ssp->ss_lock);
		kmem_free(ssp, sizeof(slip_state_t));
		return (ENOSPC);
	}
	ssp->ss_mp->b_wptr = ssp->ss_mp->b_rptr += MAX_HDR;
	ssp->ss_comp = incompalloc();
	if (!ssp->ss_comp) {
		freemsg(ssp->ss_mp);
		LOCK_DEALLOC(ssp->ss_lock);
		kmem_free(ssp, sizeof(slip_state_t));
		return (ENOSPC);
	}
	in_compress_init(ssp->ss_comp, MAX_STATES, MAX_STATES);

	if ((ssp->ss_ifstats = (struct ifstats *)kmem_zalloc(
			(int)sizeof(struct ifstats), KM_NOSLEEP)) == NULL) {
		incompfree(ssp->ss_comp);
		freemsg(ssp->ss_mp);
		LOCK_DEALLOC(ssp->ss_lock);
		kmem_free(ssp, sizeof(slip_state_t));
		return (ENOSPC);
	}
	statp = ssp->ss_ifstats;
	statp->ifs_name = ssp->ss_ifname;
	statp->ifs_active = 1;
	statp->iftype = IFSLIP;	
	statp->ifs_mtu = ssp->ss_mtu;
	ifstats_attach(statp);

	qprocson(q);
	return(0);
}

/*
 * STATIC int
 * slip_close(queue_t *q, int cflag, cred_t *crp)
 *	free up the state structure.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
/*ARGSUSED*/
STATIC int
slip_close(queue_t *q, int cflag, cred_t *crp)
{
	slip_state_t	*ssp;
	pl_t		pl;

	qprocsoff(q);
	ssp = (slip_state_t *)q->q_ptr;

	if (ssp->ss_mp != NULL)
		freemsg(ssp->ss_mp);

	if (ssp->ss_ioctl_mp)
		freemsg(ssp->ss_ioctl_mp);

	if (ssp->ss_comp != NULL) {
		incompfree(ssp->ss_comp);
	}
	if (ssp->ss_ifstats != NULL) {
		if (ifstats_detach(ssp->ss_ifstats) != ssp->ss_ifstats) {
			cmn_err(CE_WARN,
				"slip_close: couldn't find correct ifstats structure");
		}
		kmem_free(ssp->ss_ifstats, sizeof(struct ifstats));
	}
	LOCK_DEALLOC(ssp->ss_lock);
	kmem_free(ssp, sizeof(slip_state_t));
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	return(0);
}

/*
 * STATIC int
 * slip_rput(queue_t *q, mblk_t *mp)
 *	read put procedure
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC int
slip_rput(queue_t *q, mblk_t *mp)
{
	slip_state_t	*ssp;
	mblk_t	*tmp_mp;
	pl_t	pl;

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR) {
			flushq(q, FLUSHDATA);
		}
		putnext(q, mp);
		break;

	case M_DATA:
		putq(q, mp);
		break;

	case M_HANGUP:
		ssp = (slip_state_t *)(q->q_ptr);
		pl = LOCK(ssp->ss_lock, plstr);
		if (ssp->ss_ioctl_mp != NULL) {
			tmp_mp = ssp->ss_ioctl_mp;
			ssp->ss_ioctl_mp = NULL;
			UNLOCK(ssp->ss_lock, pl);
			tmp_mp->b_datap->db_type = M_IOCACK;
			putnext(q, tmp_mp);
		} else {
			ssp->ss_flags |= SS_HANGUP;
			UNLOCK(ssp->ss_lock, pl);
		}
		freemsg(mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
	return(0);
}

/*
 * STATIC int
 * slip_rsrv(queue_t *q)
 *	read service procedure
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC int
slip_rsrv(queue_t *q)
{
	mblk_t	*mp;
	slip_state_t	*ssp;

	ssp = (slip_state_t *)q->q_ptr;
	while (mp = getq(q)) {
		/*
		 * Since we eventually will call putnext without holding
		 * any locks, checking SS_BOUND can only be used
		 * as a hint and ss_lock is not required here.
		 */
		if (!(ssp->ss_flags & SS_BOUND)) {
			freemsg(mp);
			continue;
		}
		if (!canputnext(q)) {
			putbq(q, mp);
			break;
		}
		slip_in(q, mp);
	}
	return(0);
}

/*
 * STATIC void
 * slip_in(queue_t *q, mblk_t *mp)
 *	take what the serial line driver gives us and packetize it;
 *	once a complete packet is formed, hand it to IP.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC void
slip_in(queue_t *q, mblk_t *mp)
{
	mblk_t	*mp_in, *tmp;
	slip_state_t	*ssp;		/* slip state */
	unsigned char	c;
	int		len;
	pl_t 		pl;

	ssp = (slip_state_t *)q->q_ptr;
	pl = LOCK(ssp->ss_lock, plstr);
	mp_in = ssp->ss_mp;

	if (!mp_in) {
		if (!(ssp->ss_mp = mp_in = allocb(RECV_SPACE, BPRI_MED))) {
			UNLOCK(ssp->ss_lock, pl);
			freemsg(mp);
			return;
		}
		mp_in->b_wptr = mp_in->b_rptr += MAX_HDR;
	}

	while (mp) {
		while (mp->b_rptr < mp->b_wptr) {
			if (ssp->ss_flags & SS_ERROR) {
				if (*mp->b_rptr++ == (unsigned char)FRAME_END) {
					ssp->ss_flags &= ~SS_ERROR;
				}
				continue;
			}
			switch (c = *mp->b_rptr++) {
			case TRANS_FRAME_ESCAPE:
				if (ssp->ss_flags & SS_ESCAPE) {
					c = FRAME_ESCAPE;
				}
				break;
			case TRANS_FRAME_END:
				if (ssp->ss_flags & SS_ESCAPE) {
					c = FRAME_END;
				}
				break;
			case FRAME_ESCAPE:
				ssp->ss_flags |= SS_ESCAPE;
				continue;
			case FRAME_END:
				/* if packet is too small, drop silently */
				len = mp_in->b_wptr - mp_in->b_rptr;
				if (len < MINSLIP) {
					mp_in->b_wptr = mp_in->b_rptr;
					ssp->ss_flags &= ~SS_ESCAPE;
					continue;
				}
#if defined(TCPCOMPRESSION)
				/*
				 * if the IP version field != the current IP
				 * version, then see if compression is being
				 * used.
				 */
				if ((c = (*mp_in->b_rptr & 0xf0)) != (IPVERSION << 4)) {
					if (c & 0x80) {
						c = TYPE_COMPRESSED_TCP;
					}
					else if (c == TYPE_UNCOMPRESSED_TCP) {
						*mp_in->b_rptr &= 0x4f;
					}
					/*
					 * We've got something that's not an IP
					 * packet. If compression is enabled,
					 * try to decompress it. Otherwise, if
					 * `auto-enable' compression is on and
					 * it's a reasonable packet, decompress
					 * it and then enable compression.
					 * Otherwise, drop it.
					 */
					if (ssp->ss_flags & SS_COMPRESS) {
						len = in_uncompress_tcp(&mp_in->b_rptr, len,
						 	(u_int)c, ssp->ss_comp, &mp_in->b_wptr);
						if (len <= 0) {
							ssp->ss_ifstats->ifs_ierrors++;
							ssp->ss_ifstats->ifindiscards++;

							mp_in->b_wptr = mp_in->b_rptr = mp_in->b_datap->db_base + MAX_HDR;
							ssp->ss_flags &= ~SS_ESCAPE;
						 	continue;
						}
					} else if ((ssp->ss_flags & SS_AUTOCOMPRESS) &&
					    c == TYPE_UNCOMPRESSED_TCP && len >= 40) {
						len = in_uncompress_tcp(&mp_in->b_rptr, len,
									(u_int)c, ssp->ss_comp, &mp_in->b_wptr);
						ssp->ss_flags |= SS_COMPRESS;
						if (len <= 0) {
							ssp->ss_ifstats->ifs_ierrors++;
							ssp->ss_ifstats->ifindiscards++;
							mp_in->b_wptr = mp_in->b_rptr = mp_in->b_datap->db_base + MAX_HDR;
							ssp->ss_flags &= ~SS_ESCAPE;
						 	continue;
						}
					} else {
						ssp->ss_ifstats->ifs_ierrors++;
						ssp->ss_ifstats->ifindiscards++;
						mp_in->b_wptr = mp_in->b_rptr;
						ssp->ss_flags &= ~SS_ESCAPE;
					 	continue;
					}
					
				}
#endif
				if (!(tmp = slip_alloc_unitdata())) {
					ssp->ss_ifstats->ifs_ierrors++;
					ssp->ss_ifstats->ifindiscards++;
					mp_in->b_wptr = mp_in->b_rptr;
					ssp->ss_flags &= ~SS_ESCAPE;
					continue;
				}
				tmp->b_cont = mp_in;
				ssp->ss_mp = NULL;
				UNLOCK(ssp->ss_lock, pl);
				ssp->ss_ifstats->ifs_ipackets++;
				ssp->ss_ifstats->ifinucastpkts++;
				ssp->ss_ifstats->ifinoctets += msgdsize(mp_in);
				putnext(q, tmp);
				(void)LOCK(ssp->ss_lock, plstr);
				if (!(ssp->ss_mp = mp_in =
				      allocb(RECV_SPACE, BPRI_MED))) {
					ssp->ss_flags |= SS_ERROR;
					ssp->ss_flags &= ~SS_ESCAPE;
					UNLOCK(ssp->ss_lock, pl);
					ssp->ss_ifstats->ifindiscards++;
					ssp->ss_ifstats->ifs_ierrors++;
					freemsg(mp);
					return;
				}
				mp_in->b_wptr = mp_in->b_rptr += MAX_HDR;
				continue;
			}	/* end switch */
			if ((mp_in->b_wptr - mp_in->b_rptr) >= SS_MTU) {
				mp_in->b_wptr = mp_in->b_rptr;
				ssp->ss_flags |= SS_ERROR;
				ssp->ss_flags &= ~SS_ESCAPE;
				continue;
			}
			*mp_in->b_wptr++ = c;
			ssp->ss_flags &= ~SS_ESCAPE;
		}
		tmp = mp;
		mp = mp->b_cont;
		freeb(tmp);
	}
	UNLOCK(ssp->ss_lock, pl);
	return;
}

/*
 * STATIC int
 * slip_wput(queue_t *q, mblk_t *mp)
 *	write put procedure. process everything except DL_UNITDATA_REQ 
 *	messages, which are queued for slip_wsrv().
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC int
slip_wput(queue_t *q, mblk_t *mp)
{
	union DL_primitives	*p;

	switch (mp->b_datap->db_type) {
	case M_IOCTL:
		slip_ioctl(q, mp);
		break;
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHDATA);
		}
		putnext(q, mp);
		break;
	case M_PROTO:
	case M_PCPROTO:
		/* LINTED pointer alignment */
		p = (union DL_primitives *)mp->b_rptr;
		switch (p->dl_primitive) {
		case DL_INFO_REQ:
			slip_info(q, mp);
			break;
		case DL_BIND_REQ:
			slip_bind(q, mp);
			break;
		case DL_UNBIND_REQ:
			slip_unbind(q, mp);
			break;
		case DL_UNITDATA_REQ:
			putq(q, mp);
			break;
		default:
			freemsg(mp);
			break;
		}
		break;
	default:
		freemsg(mp);
		break;
	}
	return(0);
}

/*
 * STATIC int
 * slip_wsrv(queue_t *q)
 *	write service procedure
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC int
slip_wsrv(queue_t *q)
{
	mblk_t		*tmp;
	mblk_t		*mp;
	slip_state_t	*ssp;
	struct ip	*ip;
	pl_t 		pl;

	ssp = (slip_state_t *)q->q_ptr;

	while ((mp = getq(q)) != NULL) {
		if (!canputnext(q)) {
			putbq(q, mp);
			return(0);
		}
		pl = LOCK(ssp->ss_lock, plstr);
		if (!ssp->ss_flags & SS_BOUND) {
			UNLOCK(ssp->ss_lock, pl);
			slip_dlpi_uderror(q, mp, DL_OUTSTATE);
			return(0);
		}
		tmp = mp;
		if ((mp = mp->b_cont) == NULL) {
			/* hmmm - nothing to send? */
			UNLOCK(ssp->ss_lock, pl);
			freeb(tmp);
			continue;
		}
		freeb(tmp);
		/* LINTED pointer alignment */
		ip = (struct ip *)mp->b_rptr;
#if defined(TCPCOMPRESSION)
		if (ip->ip_p == IPPROTO_TCP) {
			int p = ((int *)ip)[ip->ip_hl];
			/* 
			 * RFC 1349 says treat TOS as an int,
			 * not a bit mask.
			 */
			if (IPTOS(ip->ip_tos) == 
			    IPTOS_LOWDELAY ||
			    INTERACTIVE(p & 0xffff) || 
			    INTERACTIVE(p >> 16)) {
				p = 1;
			}
			else {
				p = 0;
			}
			if (ssp->ss_flags & SS_COMPRESS) {
				/* LINTED pointer alignment */
				p = in_compress_tcp(mp, (struct ip *)mp->b_rptr,
					ssp->ss_comp, p);
				*(u_char *)mp->b_rptr |= p;
			}
		}
		else if ((ssp->ss_flags & SS_NOICMP) &&
		    ip->ip_p == IPPROTO_ICMP)
#else
		if ((ssp->ss_flags & SS_NOICMP) &&
		    ip->ip_p == IPPROTO_ICMP)
#endif
		{
			/* drop silently */
			UNLOCK(ssp->ss_lock, pl);
			freemsg(mp);
			continue;
		}
		UNLOCK(ssp->ss_lock, pl);
		slip_out(q, mp);
	}
	return(0);
}

/*
 * STATIC void
 * slip_info(queue_t *q, mblk_t *mp)
 *	process a dlpi info req from above.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC void
slip_info(queue_t *q, mblk_t *mp)
{
	dl_info_ack_t	*ap;
	mblk_t		*tmp;
	slip_state_t	*ssp;
	pl_t		pl;

	ssp = (slip_state_t *)q->q_ptr;

	if (mp->b_datap->db_ref != 1 ||
	    mp->b_datap->db_lim - mp->b_datap->db_base < DL_INFO_ACK_SIZE) {
		if ((tmp = allocb(DL_INFO_ACK_SIZE, BPRI_MED)) == NULL) {
			slip_dlpi_error(q, mp, DL_INFO_REQ, DL_SYSERR, ENOMEM);
			return;
		}
		freemsg(mp);
		mp = tmp;
	}
	else {
		mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
	}

	/* LINTED pointer alignment */
	ap = (dl_info_ack_t *)mp->b_wptr;
	mp->b_wptr += DL_INFO_ACK_SIZE;
	mp->b_datap->db_type = M_PCPROTO;

	pl = LOCK(ssp->ss_lock, plstr);
	ap->dl_max_sdu = ssp->ss_mtu;
	ap->dl_current_state = ssp->ss_flags & SS_BOUND ? DL_IDLE : DL_UNBOUND;
	UNLOCK(ssp->ss_lock, pl);
	ap->dl_primitive = DL_INFO_ACK;
	ap->dl_min_sdu = 1;
	ap->dl_addr_length = 0;
	ap->dl_mac_type = DL_CHAR;	/* close enough for government work */
	ap->dl_reserved = 0;
	ap->dl_service_mode = DL_CLDLS;
	ap->dl_qos_length = 0;
	ap->dl_qos_offset = 0;
	ap->dl_qos_range_length = 0;
	ap->dl_qos_range_offset = 0;
	ap->dl_provider_style = DL_STYLE1;
	ap->dl_addr_offset = DL_INFO_ACK_SIZE;
	ap->dl_growth = 0;

	qreply(q, mp);
}

/*
 * STATIC int
 * slip_bind(queue_t *q, mblk_t *mp)
 *	process a dlpi bind request from above and ack it.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC void
slip_bind(queue_t *q, mblk_t *mp)
{
	dl_bind_req_t	*rp;
	dl_bind_ack_t	*ap;
	slip_state_t	*ssp;
	mblk_t		*tmp;
	unsigned long	sap;
	pl_t		pl;

	ssp = (slip_state_t *)q->q_ptr;
	pl = LOCK(ssp->ss_lock, plstr);
	if (ssp->ss_flags & SS_BOUND) {
		UNLOCK(ssp->ss_lock, pl);
		slip_dlpi_error(q, mp, DL_BIND_REQ, DL_OUTSTATE, 0);
		return;
	}

	/* LINTED pointer alignment */
	rp = (dl_bind_req_t *)mp->b_rptr;

	if ((sap = rp->dl_sap) != IP_SAP) {
		UNLOCK(ssp->ss_lock, pl);
		slip_dlpi_error(q, mp, DL_BIND_REQ, DL_UNSUPPORTED, 0);
		return;
	}

	if (mp->b_datap->db_ref != 1 ||
	    mp->b_datap->db_lim - mp->b_datap->db_base < DL_BIND_ACK_SIZE) {
		if ((tmp = allocb(DL_BIND_ACK_SIZE, BPRI_MED)) == NULL) {
			UNLOCK(ssp->ss_lock, pl);
			slip_dlpi_error(q, mp, DL_BIND_REQ, DL_SYSERR, ENOSR);
			return;
		}
		freemsg(mp);
		mp = tmp;
	}
	else {
		mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
	}

	ssp->ss_flags |= SS_BOUND;
	UNLOCK(ssp->ss_lock, pl);

	mp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	ap = (dl_bind_ack_t *)mp->b_wptr;
	mp->b_wptr += DL_BIND_ACK_SIZE;
	ap->dl_primitive = DL_BIND_ACK;
	ap->dl_sap = sap;
	ap->dl_addr_length = 0;
	ap->dl_addr_offset = DL_BIND_ACK_SIZE;
	ap->dl_max_conind = 0;

	qreply(q, mp);
	return;
}

/*
 * STATIC void
 * slip_unbind(queue_t *q, mblk_t *mp)
 *	process a dlpi unbind request and ack it.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC void
slip_unbind(queue_t *q, mblk_t *mp)
{
	slip_state_t	*ssp;
	dl_ok_ack_t	*ap;
	mblk_t		*tmp;
	pl_t		pl;

	ssp = (slip_state_t *)q->q_ptr;
	pl = LOCK(ssp->ss_lock, plstr);
	if (ssp->ss_flags & SS_BOUND == 0) {
		UNLOCK(ssp->ss_lock, pl);
		slip_dlpi_error(q, mp, DL_UNBIND_REQ, DL_OUTSTATE, 0);
		return;
	}


	if (mp->b_datap->db_ref != 1 ||
	    mp->b_datap->db_lim - mp->b_datap->db_base < DL_OK_ACK_SIZE) {
		if ((tmp = allocb(DL_OK_ACK_SIZE, BPRI_MED)) == NULL) {
			UNLOCK(ssp->ss_lock, pl);
			slip_dlpi_error(q, mp, DL_UNBIND_REQ, DL_SYSERR, ENOSR);
			return;
		}
		freemsg(mp);
		mp = tmp;
	}
	else {
		mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
	}
	ssp->ss_flags &= ~SS_BOUND;
	UNLOCK(ssp->ss_lock, pl);

	/* LINTED pointer alignment */
	ap = (dl_ok_ack_t *)mp->b_wptr;
	mp->b_wptr += DL_OK_ACK_SIZE;
	mp->b_datap->db_type = M_PCPROTO;

	ap->dl_primitive = DL_OK_ACK;
	ap->dl_correct_primitive = DL_UNBIND_REQ;

	qreply(q, mp);
	return;
}

/*
 * STATIC void
 * slip_out(queue_t *q, mblk_t *mp)
 * 	takes an IP packet, sans dlpi header, and escapes all
 *	necessary octets and inserts framing octets at the head and
 *	tail of the packet.  After all of that, send it to the serial
 *	line module to be transmitted.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC void
slip_out(queue_t *q, mblk_t *mp)
{
	mblk_t	*mpp, *mp_save, *mp_tail;
	slip_state_t	*ssp;
	register unsigned char	*cp;
	register unsigned int	num_esc, pkt_size;

	ssp = (slip_state_t *)q->q_ptr;
	num_esc = pkt_size = 0;
	for (mpp = mp; mpp != NULL; mpp = mpp->b_cont) {
		mp_tail = mpp;
		for (cp = mpp->b_rptr; cp < mpp->b_wptr; cp++) {
			switch (*cp) {
			case FRAME_ESCAPE:
			case FRAME_END:
				num_esc++;
			default:
				pkt_size++;
				break;
			}
		}
	}
	ASSERT(mp_tail != NULL);

	if (pkt_size > ssp->ss_mtu) {
		ssp->ss_ifstats->ifoutdiscards++;
		ssp->ss_ifstats->ifs_oerrors++;
		freemsg(mp);
		return;
	}
	
	if (num_esc) {
		if (slip_pp_frame_end)
			mpp = allocb(pkt_size + num_esc + 2, BPRI_MED);
		else
			mpp = allocb(pkt_size + num_esc + 1, BPRI_MED);
		if (mpp == NULL) {
			ssp->ss_ifstats->ifoutdiscards++;
			ssp->ss_ifstats->ifs_oerrors++;
			freemsg(mp);
			return;
		}

		/*
		 * It is recommended that you prepend a FRAME_END, so we do
		 */
		if (slip_pp_frame_end)
			*mpp->b_wptr++ = FRAME_END;
		mp_save = mp;
		while (mp != NULL) {
			for (; mp->b_rptr < mp->b_wptr; mp->b_rptr++) {
				switch (*mp->b_rptr) {
				case FRAME_ESCAPE:
					*mpp->b_wptr++ = FRAME_ESCAPE;
					*mpp->b_wptr++ = TRANS_FRAME_ESCAPE;
					break;
				case FRAME_END:
					*mpp->b_wptr++ = FRAME_ESCAPE;
					*mpp->b_wptr++ = TRANS_FRAME_END;
					break;
				default:
					*mpp->b_wptr++ = *mp->b_rptr;
					break;
				}
			}
			mp = mp->b_cont;
		}
		freemsg(mp_save);
		*mpp->b_wptr++ = FRAME_END;
		mp = mpp;
	}
	else {
		if ((mpp = allocb(1, BPRI_HI)) == NULL) {
			ssp->ss_ifstats->ifoutdiscards++;
			ssp->ss_ifstats->ifs_oerrors++;
			freemsg(mp);
			return;
		}
		mp_tail->b_cont = mpp;
		*mpp->b_wptr++ = FRAME_END;

		/*
		 * It is recommended that you prepend a FRAME_END, so we do
		 */
		if (slip_pp_frame_end && (mpp = allocb(1, BPRI_HI))) {
			*mpp->b_wptr++ = FRAME_END;
			mpp->b_cont = mp;
			mp = mpp;
		}
	}
	ssp->ss_ifstats->ifoutoctets += msgdsize(mp);
	ssp->ss_ifstats->ifoutucastpkts++;
	ssp->ss_ifstats->ifs_opackets++;
	putnext(q, mp);

	return;
}

/*
 * STATIC void
 * slip_ioctl(queue_t *q, mblk_t *mp)
 *	process ioctls.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC void
slip_ioctl(queue_t *q, mblk_t *mp)
{
	struct iocblk	*icp;
	slip_state_t	*ssp;
	struct ifreq	*ifr;
	char 		*cp1, *cp2, *tailp;
	int		oldmtu;
	pl_t		pl;

	ssp = (slip_state_t *)q->q_ptr;

	if ((mp->b_wptr - mp->b_rptr) < sizeof(struct iocblk)) {
		mp->b_datap->db_type = M_IOCNAK;
		qreply(q, mp);
		return;
	}

	/* LINTED pointer alignment */
	icp = (struct iocblk *)mp->b_rptr;

	if (icp->ioc_count == TRANSPARENT) {
		putnext(q, mp);
		return;
	}
	switch ((unsigned int)icp->ioc_cmd) {

	case S_SETSPEED:
		if (mp->b_cont == NULL ||
		    (mp->b_cont->b_wptr - mp->b_cont->b_rptr) < sizeof(int)) {
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			break;
		}
		mp->b_datap->db_type = M_IOCACK;
		/* LINTED pointer alignment */
		ssp->ss_ifstats->ifspeed = *(int *)mp->b_cont->b_rptr;
		qreply(q, mp);
		break;

	case S_MTU:
		if ((mp->b_cont == NULL) ||
		    (mp->b_cont->b_wptr - mp->b_cont->b_rptr) < sizeof(int)) {
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			break;
		}
		mp->b_datap->db_type = M_IOCACK;
		pl = LOCK(ssp->ss_lock, plstr);
		oldmtu = ssp->ss_mtu;
		/* LINTED pointer alignment */
		ssp->ss_mtu = *(int *)mp->b_cont->b_rptr;
		ssp->ss_ifstats->ifs_mtu = ssp->ss_mtu;
		if (oldmtu < ssp->ss_mtu) {
			mblk_t *bp;

			if (!(bp = allocb(RECV_SPACE, BPRI_MED))) {
				mp->b_datap->db_type = M_IOCNAK;
				ssp->ss_mtu = oldmtu;
				ssp->ss_ifstats->ifs_mtu = ssp->ss_mtu;
			}
			else {
				if (ssp->ss_mp) {
					freemsg(ssp->ss_mp);
				}
				ssp->ss_mp = bp;
			}
		}
		UNLOCK(ssp->ss_lock, pl);
		qreply(q, mp);
		break;

	case SIOCLOWER:
		pl = LOCK(ssp->ss_lock, plstr);
		if (ssp->ss_flags & SS_HANGUP) {
			ssp->ss_flags &= ~SS_HANGUP;
			UNLOCK(ssp->ss_lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
		} else {
			ssp->ss_ioctl_mp = mp;
			UNLOCK(ssp->ss_lock, pl);
		}
		break;

	case S_COMPRESSON:
		pl = LOCK(ssp->ss_lock, plstr);
		ssp->ss_flags |= SS_COMPRESS;
		UNLOCK(ssp->ss_lock, pl);
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		break;

	case S_COMPRESSOFF:
		pl = LOCK(ssp->ss_lock, plstr);
		ssp->ss_flags &= ~SS_COMPRESS;
		UNLOCK(ssp->ss_lock, pl);
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		break;

	case S_COMPRESSAON:
		pl = LOCK(ssp->ss_lock, plstr);
		ssp->ss_flags |= SS_AUTOCOMPRESS;
		UNLOCK(ssp->ss_lock, pl);
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		break;

	case S_COMPRESSAOFF:
		pl = LOCK(ssp->ss_lock, plstr);
		ssp->ss_flags &= ~SS_AUTOCOMPRESS;
		UNLOCK(ssp->ss_lock, pl);
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		break;
	case S_ICMP:
		pl = LOCK(ssp->ss_lock, plstr);
		ssp->ss_flags &= ~SS_NOICMP;
		UNLOCK(ssp->ss_lock, pl);
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		break;

	case S_NOICMP:
		pl = LOCK(ssp->ss_lock, plstr);
		ssp->ss_flags |= SS_NOICMP;
		UNLOCK(ssp->ss_lock, pl);
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		break;

	case SIOCSIFFLAGS:
		/* LINTED pointer alignment */
		ifr = (struct ifreq *)mp->b_cont->b_rptr;
		ifr->ifr_flags |= IFF_POINTOPOINT;
                ((struct iocblk_in *) icp)->ioc_ifflags = ifr->ifr_flags;
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
                break;

	case SIOCGIFFLAGS:
		/* LINTED pointer alignment */
		ifr = (struct ifreq *)mp->b_cont->b_rptr;
		ifr->ifr_flags |= IFF_POINTOPOINT;
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
                break;

	/* ack these */
	case SIOCSIFNAME:
		if ((mp->b_cont == NULL)) {
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			break;
		}
		/* LINTED pointer alignment */
		ifr = (struct ifreq *)mp->b_cont->b_rptr;
		for (tailp = ifr->ifr_name; *tailp; tailp++)
			/* null */ ;
		pl = LOCK(ssp->ss_lock, plstr);
		bcopy(ifr->ifr_name, ssp->ss_ifname, tailp - ifr->ifr_name + 2);
		cp1 = ssp->ss_ifname;
		while (*cp1 && !(*cp1 >= '0' && *cp1 <= '9')) {
			cp1++;
		}
		if (*cp1) {
			cp2 = cp1;
			ssp->ss_ifstats->ifs_unit = 0;
			while (*cp2 &&
				(*cp2 >= '0' && *cp2 <= '9')) {
				ssp->ss_ifstats->ifs_unit =
					ssp->ss_ifstats->ifs_unit * 10 + 
					(short)(*cp2++ - '0');
			}
		}
		*cp1 = '\0';
		UNLOCK(ssp->ss_lock, pl);
		/* FALLTHROUGH */

	case SIOCSIFADDR:
	case SIOCGIFMETRIC:
	case SIOCSIFMETRIC:
	case SIOCGIFADDR:
	case SIOCGIFNETMASK:
	case SIOCSIFNETMASK:
	case SIOCGIFBRDADDR:
	case SIOCSIFBRDADDR:
	case SIOCGIFDSTADDR:
	case SIOCSIFDSTADDR:
	case SIOCIFDETACH:
	case IF_UNITSEL:
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
	return;
}

/*
 * STATIC mblk_t *
 * slip_alloc_unitdata(void)
 *	allocate a unitdata_ind to be prepended to
 *	the IP packet we will send up stream.
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC mblk_t *
slip_alloc_unitdata(void)
{
	mblk_t	*mp;
	dl_unitdata_ind_t	*dp;

	if ((mp = allocb(DL_UNITDATA_IND_SIZE, BPRI_HI)) == NULL) {
		return NULL;
	}

	/* LINTED pointer alignment */
	dp = (dl_unitdata_ind_t *)mp->b_wptr;
	mp->b_wptr += DL_UNITDATA_IND_SIZE;
	mp->b_datap->db_type = M_PROTO;

	dp->dl_primitive = DL_UNITDATA_IND;
	dp->dl_dest_addr_length = 0;
	dp->dl_dest_addr_offset = DL_UNITDATA_IND_SIZE;
	dp->dl_src_addr_length = 0;
	dp->dl_src_addr_offset = DL_UNITDATA_IND_SIZE;

	return mp;
}

/*
 * STATIC void
 * slip_dlpi_error(queue_t *q, mblk_t  *mp, unsigned long dl_pr, 
 * 		unsigned long dl_er, unsigned long unix_er)
 *	send back an error ack
 *
 * Arguments:
 *	dl_pr	- dlpi primitive
 *	dl_er	- dlpi error
 *	unix_er	- unix error
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC void
slip_dlpi_error(queue_t *q, mblk_t  *mp, unsigned long dl_pr, 
		unsigned long dl_er, unsigned long unix_er)
{
	dl_error_ack_t	*ap;

	if (mp->b_datap->db_ref != 1 ||
	    mp->b_datap->db_lim - mp->b_datap->db_base < DL_ERROR_ACK_SIZE) {
		freemsg(mp);
		if (!(mp = allocb(DL_ERROR_ACK_SIZE, BPRI_MED))) {
			return;
		}
	}
	else {
		mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
	}

	mp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	ap = (dl_error_ack_t *)mp->b_wptr;
	mp->b_wptr += DL_ERROR_ACK_SIZE;

	ap->dl_primitive = DL_ERROR_ACK;
	ap->dl_error_primitive = dl_pr;
	ap->dl_errno = dl_er;
	ap->dl_unix_errno = unix_er;

	qreply(q, mp);
	return;
}

/*
 * STATIC void
 * slip_dlpi_uderror(queue_t *q, mblk_t *mp, unsigned long dl_er)
 *	process a unitdata error message.
 *
 * Calling/Exit State:
 *	No locks held.
 *
 */
STATIC void
slip_dlpi_uderror(queue_t *q, mblk_t *mp, unsigned long dl_er)
{
	dl_uderror_ind_t	*ep;

	if (mp->b_datap->db_ref != 1 ||
	    mp->b_datap->db_lim - mp->b_datap->db_base < DL_UDERROR_IND_SIZE) {
		freemsg(mp);
		if (!(mp = allocb(DL_UDERROR_IND_SIZE, BPRI_MED))) {
			return;
		}
	}
	else {
		mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
	}

	mp->b_datap->db_type = M_PCPROTO;
	/* LINTED pointer alignment */
	ep = (dl_uderror_ind_t *)mp->b_wptr;
	mp->b_wptr += DL_UDERROR_IND_SIZE;

	ep->dl_primitive = DL_UDERROR_IND;
	ep->dl_dest_addr_length = 0;
	ep->dl_dest_addr_offset = 0;
	ep->dl_errno = dl_er;

	qreply(q, mp);
	return;
}
