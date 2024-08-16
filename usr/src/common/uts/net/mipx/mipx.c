/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/mipx/mipx.c	1.15"
/*
 *        Copyright  Univel Inc. 1991
 *        (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *        No part of this file may be duplicated, revised, translated, localized
 *        or modified in any manner or compiled, linked or uploaded or
 *        downloaded to or from any computer system without the prior written
 *        consent of Univel, Inc.
 *
 *
 *   Merge IPX driver
 *
 *  MODULE:	mipx.c
 *
 *  ABSTRACT:	STREAMS mux for merge IPX  support.
 *
 */

/*
 * 
 *	Description:
 *			STREAMS Driver for routing merge requests
 *
 *	Author		:	Srikant Mukherjee
 *
 */

#ifdef	_KERNEL_HEADERS
#include	<net/mipx/mipx.h>
#else
#include	"mipx.h"
#endif
/*
 * Forward References for STREAMS
 */
int 		mipxopen();
int		mipxclose();
int		mipxuwput();
int		mipxlrput();
int		mipxlrsrv();
/*
 * Forward References for IOCTLs
 */
void		MipxLink();
void		MipxUnlink();
void		MipxSetNet();
void		MipxRelDos();
mipxData_t mipxData[MIPX_DATA_SIZE];
/*
 *	OTHER FORWARD DECLARATIONS
 */
mipxData_t	*MipxGetDataEntry();
void		MipxIocAck();
void		MipxIocNegAck();
void		MipxDropPacket();
void		MipxSetSocket();
void		Mipxclear();
void		MipxSendImReady();
int		Mipx_outgoing_wdog();
int		Mipx_incoming_wdog();
int		Mipx_wdog_rsp();
void		MipxSendEndOfJob();
void		MipxSendDisconnect();
void		MipxReXmitDisconnect();

/*
 * STREAMS Declarations:
 */
static struct module_info mipx_winfo = {
		MIPXID, "mipx", 0, INFPSZ, MIPX_HI_WATER, MIPX_LO_WATER 
		};

static struct module_info rinfo = {
		MIPXID, "mipx", 0, INFPSZ, MIPX_HI_WATER, MIPX_LO_WATER 
		};

static struct qinit urinit = {
		NULL, NULL, mipxopen, mipxclose, NULL, &rinfo, NULL
		};

static struct qinit uwinit = {
		mipxuwput, NULL, NULL, NULL, NULL, &mipx_winfo, NULL
		};

static struct qinit lrinit = {
		mipxlrput, mipxlrsrv, NULL, NULL, NULL, &rinfo, NULL
		};

static struct qinit lwinit = {
		NULL, NULL, NULL, NULL, NULL, &mipx_winfo, NULL
		};

struct streamtab mipxinfo = {
		&urinit, &uwinit, &lrinit, &lwinit
		};


int mipxdevflag = 0;


/*
 * Loadable module stuff
 */

static lock_t	*mdlck;
static int	NumDOSProcs;
static int	Ipx_Heavy = FALSE;
#define DRVNAME	"mipx - Loadable Merge IPX Driver"

extern	void	mod_drvattach();
extern	void	mod_drvdetach();
STATIC	int	mipx_load(), mipx_unload();
void	mipxinit();
MOD_DRV_WRAPPER(mipx, mipx_load, mipx_unload, NULL, DRVNAME);


/*
 * Wrapper functions.
 */

STATIC int
mipx_load(void)
{
    mod_drvattach(&mipx_attach_info);
    mipxinit();
    return(SUCCESS);
}

STATIC int
mipx_unload(void)
{

    int i;

    for (i=0; i < MIPX_DATA_SIZE; i++)
	if (mipxData[i].timeoutID != 0)
		untimeout(mipxData[i].timeoutID);
    mod_drvdetach(&mipx_attach_info);
    return(SUCCESS);
}

void
mipxinit()
{
	int	device;


	NumDOSProcs = 0;
	Ipx_Heavy = FALSE;
	for(device = 0; device < MIPX_DATA_SIZE; device++) {
		mipxData[device].ipx_qptr   = NULL;
		mipxData[device].mpip_qptr  = NULL;
		mipxData[device].prev   = NULL;
		mipxData[device].next   = NULL;
		mipxData[device].cinfo   = NULL;
		mipxData[device].l_index = 0;
		mipxData[device].timeoutID = 0;
		mipxData[device].state = 0;
		mipxData[device].dsock.sock[0] = 0;
		mipxData[device].dsock.sock[1] = 0;
		mipxData[device].dnet.net[0] = 0;
		mipxData[device].dnet.net[1] = 0;
		mipxData[device].dnet.net[2] = 0;
		mipxData[device].dnet.net[3] = 0;
		mipxData[device].count = 0;
		mipxData[device].ncpseqnum = 0;
	}

}

int 
mipxopen(q, devp, flag, sflag, credp)
queue_t	*q;		/* pointer to read queue */
dev_t	*devp;	/* major/minor device number -- zero for modules */
int		flag;	/* file open flags -- zero for modules */
int	 	sflag;	/* stream open flags */
cred_t	*credp;
{
	int dev;
	mipxData_t *mdp;	

	NumDOSProcs++;
	if (sflag == CLONEOPEN)
		mdp = MipxGetDataEntry(&dev);

	if (mdp == NULL)
		return ENXIO;
	mipxData[dev].ipx_qptr = q; /*a little bit of a misnomer but this is 
				    the only case where it point to a non-ipx 
				    stream*/
	mipxData[dev].prev = NULL;
	mipxData[dev].next = NULL;
	mipxData[dev].mpip_qptr = NULL;
	mipxData[dev].count = 0;
	mipxData[dev].timeoutID = 0;
	q->q_ptr = (caddr_t) &mipxData[dev];
	WR(q)->q_ptr = (caddr_t) &mipxData[dev];
	*devp = makedevice(getmajor(*devp), dev);
	return SUCCESS;
}

/*
 * The Upper Write Put procedure is responsible for handling all
 * IOCTL calls coming downstream.  If an IOCTL is not recognized
 * it is dropped.  
 *
 */
int 
mipxuwput(q, mp)
queue_t	*q;			/* pointer to the write queue */
mblk_t	*mp;		/* message pointer */
{
	register struct iocblk	*iocp;
	mipxData_t *mdp;


	switch(MTYPE(mp)) {
	case M_IOCTL:
		iocp = (struct iocblk *)mp->b_rptr;
		switch (iocp->ioc_cmd) {
		case I_LINK:
		case I_PLINK:
			MipxLink(q, mp, iocp);
			break;
		case I_UNLINK:
		case I_PUNLINK:
			MipxUnlink(q, mp, iocp);
			break;
		case MIPX_SET_NET:
		/*This is the case where this is running on a ipx heavy stack*/
			MipxSetNet(q, mp, iocp);
			break;
		case MIPX_REL_DOS:
			MipxSendImReady((mipxData_t *)q->q_ptr);
			MipxIocAck(q, mp, 0, 0, iocp);
			break;
		default:
			MipxIocNegAck(q, mp, 0, 0, iocp);
			break;
		}
		break;

	default:
		freemsg(mp);
		break;

	}

	return (SUCCESS);
}

int 
mipxlrsrv(q)
queue_t	*q;			/* pointer to the read queue */
{
	int ret;
	mblk_t	*mp;		/* message pointer */

	while (mp = getq(q)) {
		ret = MipxRouteData(q, mp);
		if ( ret < 0 ) {
			putbq(q, mp);
			break;
		}
	}
	return;
}

/*
This routine is all that this whole kernel module is about. This routes
between the mpip stream and and the streams associated with the various 
open sockets.
*/
int 
mipxlrput(q, mp)
queue_t	*q;			/* pointer to the read queue */
mblk_t	*mp;		/* message pointer */
{


	mipxData_t *mdp;
	short	found;
	ipxConnInfo_t	*cptr;
	pl_t	oldprio;

	found = 0;
	switch (MTYPE(mp)) {
	case M_ERROR:
	case M_HANGUP:
		oldprio = splhi();
		mdp = q->q_ptr;
		freemsg(mp);
		if (q == mdp->ipx_qptr) {
			mdp->ipx_qptr = NULL;
			splx(oldprio);
			break;
		}
		while (mdp != NULL)  {
			if (mdp->cinfo != NULL) {
			   if (mdp->ipx_qptr != NULL) {
				found++;
				mdp->count = 1;
				MipxSendEndOfJob(mdp);
				MipxSendDisconnect(mdp);
			   } 
			   else {
			   	cptr = mdp->cinfo;
			    	mdp->cinfo = NULL;
			    	kmem_free(cptr, sizeof(*mdp->cinfo));
			   }
			}
			mdp->mpip_qptr = NULL;
			mdp = mdp->next;
		}
		mdp = q->q_ptr; /* works only because this is the mpip q
				   hence the q_pptr points to the first mdp
				   whose ipx_qptr points to the controlling 
				   stream 
				*/
		if (found)
			mdp->timeoutID = timeout(MipxReXmitDisconnect, (caddr_t)mdp, 100);
		else {
                	if (mdp->ipx_qptr != NULL) {
                        	mp = allocb(1, BPRI_MED);
                        	if (mp != NULL) {
                                	mp->b_datap->db_type = M_DATA;
                                	*mp->b_wptr++ = (unsigned char)IPX_DIE;
                                	putnext(mdp->ipx_qptr, mp);
                        	}
                	}
        	}
		splx(oldprio);
		break;

	case M_DATA:
		if (MipxRouteData(q, mp) < 0)
			putq(q, mp);
		break;

	default:
		freemsg(mp);
		break;
	}

	return(SUCCESS);
}

void
MipxReXmitDisconnect(mdp)
mipxData_t *mdp;
{
	mipxData_t *tmpmdp;
	unsigned char	found;
	mblk_t *mp;
	ipxConnInfo_t	*cptr;

	found =0;
	tmpmdp = mdp;
	tmpmdp->timeoutID = 0;
	while (tmpmdp != NULL)  {
		if (tmpmdp->cinfo != NULL) {
			tmpmdp->ncpseqnum--;
			MipxSendEndOfJob(tmpmdp);
			MipxSendDisconnect(tmpmdp);
			tmpmdp->count++;
			found++;
			if ((tmpmdp->ipx_qptr == NULL) || (tmpmdp->count == 20)) {
			    cptr = tmpmdp->cinfo;
			    tmpmdp->cinfo = NULL;
			    kmem_free(cptr, sizeof(*tmpmdp->cinfo));
		       	    found--;
			}
		}
		tmpmdp = tmpmdp->next;
	}
	if (found)
	     mdp->timeoutID = timeout(MipxReXmitDisconnect, (caddr_t)mdp, 100);
	else {
		if (mdp->ipx_qptr != NULL) {
			mp = allocb(1, BPRI_MED);
			if (mp != NULL) {
				mp->b_datap->db_type = M_DATA;
				*mp->b_wptr++ = (unsigned char)IPX_DIE;
				putnext(mdp->ipx_qptr, mp);
			}
		}
	}
}
int
mipxclose(q, flag, credp)
queue_t	*q;				/* pointer to the read queue */
int		flag;			/* file open flags - zero for modules */
cred_t	*credp;
{
/*
	int	device;
	if (--NumDOSProcs == 0) {
	   for(device = 0; device < MIPX_DATA_SIZE; device++) {
		mipxData[device].ipx_qptr   = NULL;
		mipxData[device].mpip_qptr  = NULL;
		mipxData[device].prev   = NULL;
		mipxData[device].next   = NULL;
		mipxData[device].l_index = 0;
		mipxData[device].state = 0;
		mipxData[device].dsock.sock[0] = 0;
		mipxData[device].dsock.sock[1] = 0;
		mipxData[device].dnet.net[0] = 0;
		mipxData[device].dnet.net[1] = 0;
		mipxData[device].dnet.net[2] = 0;
		mipxData[device].dnet.net[3] = 0;
		mipxData[device].count = 0;
		}
	}
*/
	return SUCCESS;

}

void
MipxLink(q,mp,iocp)
queue_t			*q;			/* pointer to the write queue */
mblk_t			*mp;		/* message pointer */
struct iocblk	*iocp;
{
	mipxData_t  *mdp, *cur;
	struct linkblk 	*linkp;


	if (!mp->b_cont) {
		MipxIocNegAck(q,mp,0,0,iocp);
		return;
	}

	if(DATA_SIZE(mp->b_cont) != sizeof(struct linkblk)) {
		MipxIocNegAck(q,mp,0,0,iocp);
		return;
	}

	linkp = (struct linkblk *) mp->b_cont->b_rptr;

	mdp = q->q_ptr;
	if (mdp->mpip_qptr == NULL) {
		mdp->count = 0;
		mdp->mpip_qptr = RD(linkp->l_qbot);
		mdp->l_index = linkp->l_index;
		mdp->timeoutID = 0;
		mdp->mpip_qptr->q_ptr = (caddr_t) mdp;
		WR(mdp->mpip_qptr)->q_ptr = (caddr_t) mdp;
		mdp->next = NULL;
		mdp->prev = NULL;
		mdp->state = MIPX_CNTL_STR;
		MipxIocAck(q,mp,0,0,iocp);/* mpip driver linked; send an ACK */
		return;
	}
	mdp->count++;
	if ( mdp->count == 9 )
		MipxSendImReady(mdp);
	/*These are linking of ipx streams*/
	mdp = MipxGetDataEntry(0);
	if (mdp != NULL) {
		mdp->ipx_qptr	= RD(linkp->l_qbot);
		mdp->mpip_qptr	= ((mipxData_t *)q->q_ptr)->mpip_qptr;
		mdp->l_index     = linkp->l_index;
		mdp->timeoutID = 0;
		mdp->state = MIPX_UNUSED;
		mdp->ipx_qptr->q_ptr = (caddr_t) mdp;
		WR(mdp->ipx_qptr)->q_ptr = (caddr_t) mdp;
		mdp->dnet = ((mipxData_t *)(q->q_ptr))->dnet;
		for (cur = ((mipxData_t *)q->q_ptr); cur->next != NULL; cur = cur->next );
		cur->next = mdp;
		mdp->next = NULL;
		mdp->prev = cur;
		MipxIocAck(q,mp,0,0,iocp); 
	} else MipxIocNegAck(q,mp,0,0,iocp);
	return;
}

void
MipxUnlink(q,mp,iocp)
queue_t			*q;			/* pointer to the write queue */
mblk_t			*mp;		/* message pointer */
struct iocblk	*iocp;
{
	struct linkblk 	*linkp;
	mipxData_t	*mdp, *mdpdest;


	linkp = (struct linkblk *) mp->b_cont->b_rptr;
	mdp = (mipxData_t *) (q->q_ptr);
	while ((mdp != NULL) && (mdp->l_index != linkp->l_index)) 
		mdp = mdp->next;
	if (mdp == NULL) {
		MipxIocNegAck(q,mp,0,0,iocp);
		cmn_err(CE_NOTE, "mipx: Error in Unlink");
		return;
	}
	mdp->ipx_qptr = NULL;
	mdp->mpip_qptr = NULL;
	if (mdp->prev != NULL)
		mdp->prev->next = mdp->next;
	if (mdp->next != NULL)
		mdp->next->prev = mdp->prev;
	mdp->prev = NULL;
	mdp->next = NULL;
	mdp->l_index = 0;
	mdp->timeoutID = 0;
	mdp->state = 0;
	mdp->dsock.sock[0] = 0;
	mdp->dsock.sock[1] = 0;
	mdp->dnet.net[0] = 0;
	mdp->dnet.net[1] = 0;
	mdp->dnet.net[2] = 0;
	mdp->dnet.net[3] = 0;
	mdp->count = 0;
	mdp->ncpseqnum = 0;
	MipxIocAck(q,mp,0,0,iocp);
	return;
}

void
MipxSetNet(q, mp, iocp)
queue_t			*q;			/* pointer to the write queue */
mblk_t			*mp;		/* message pointer */
struct iocblk	*iocp;
{
	struct linkblk 	*linkp;
	mipxData_t	*mdp, *mdpdest;
	queue_t		*qbot;


	if (!mp->b_cont) {
		MipxIocNegAck(q,mp,0,0,iocp);
		return;
	}

	if (DATA_SIZE(mp->b_cont) < IPX_NET_SIZE) {
		MipxIocNegAck(q,mp,0,0,iocp);
		return;
	}
	Ipx_Heavy = TRUE;
	bcopy((char *) (((mipxData_t *) (q->q_ptr))->dnet.net), mp->b_cont->b_rptr, IPX_NET_SIZE);
	MipxIocAck(q,mp,0,0,iocp);
	return;
}

void
MipxIocNegAck(q,mp,err,count,iocp)
queue_t			*q;			/* pointer to the write queue */
mblk_t			*mp;		/* message pointer */
int				err;
long			count;
struct iocblk 	*iocp;
{


	MTYPE(mp) = M_IOCNAK;
	iocp->ioc_error = err;
	iocp->ioc_count = count;
	qreply(q, mp);

}

void
MipxIocAck(q,mp,err,count,iocp)
queue_t			*q;			/* pointer to the write queue */
mblk_t			*mp;		/* message pointer */
int				err;
long			count;
struct iocblk 	*iocp;
{


	MTYPE(mp) = M_IOCACK;
	iocp->ioc_error = err;
	iocp->ioc_count = count;
	qreply(q, mp);

}


int
MipxRouteData(q,mp)
queue_t	*q;
mblk_t	*mp;		/* message pointer */
{
	ipxHdr_t	*ipxHeader;
	ipxNcpReq_t	*ncpReq;
	ipxConnInfo_t	*cptr;
	unsigned char  destSocket[IPX_SOCK_SIZE]; 	/* net Order */
	mipxData_t	*mdp;
	mblk_t		*bp;
	pl_t		oldpri;


	if (mp->b_cont != NULL) {
		bp = msgpullup(mp, -1);
		freemsg(mp);
	} else bp = mp;
	if (bp == NULL)
		return SUCCESS;
	ipxHeader = (ipxHdr_t *)bp->b_rptr;
	ncpReq = (ipxNcpReq_t *)bp->b_rptr;
	
	mdp = q->q_ptr;
	if (q == mdp->mpip_qptr) {
		if (ipxHeader->chksum == 0x0000) { 
					/*Checksums are set top 0xffff in the
					     DOS side for the reqular IPX 
					     packets. A checksum of zero means
					     this is either an open or a close*/
		return(MipxSetStreamOpts(q, bp));
	}
	   while ((mdp != NULL) && !(IPXCMPSOCK(ipxHeader->src.sock, mdp->dsock.sock))) 
		mdp = mdp->next;
	   if (mdp == NULL) {
		MipxDropPacket(bp);
		return SUCCESS;
		}
	   if	((mdp->ipx_qptr != NULL) && (canput(WR(mdp->ipx_qptr)->q_next))) {
	   	if ( MIPX_OUTGOING_WDOG(ipxHeader, bp))
			mdp->state &= ~MIPX_WDOG_PENDING;
		if ((Ipx_Heavy == TRUE) && IPX_NET_NULL(ipxHeader->dest.net)) {
			ipxHeader->dest.net[0] = mdp->dnet.net[0];
			ipxHeader->dest.net[1] = mdp->dnet.net[1];
			ipxHeader->dest.net[2] = mdp->dnet.net[2];
			ipxHeader->dest.net[3] = mdp->dnet.net[3];
		}
		ipxHeader->tc = IPX_TRANSPORT_CONTROL;
		if (ipxHeader->pt == IPX_NOVELL_PACKET_TYPE) 
			mdp->ncpseqnum = (mdp->ncpseqnum + 1) & 0xff;
		putnext(WR(mdp->ipx_qptr), bp);
		return(SUCCESS);
	   } else  return (FAIL);
	} else {
	/*
	This case is for when the dos session is dead; mpip_qptr is set to
	NULL when the HANGUP is received from the merge process
	 */
	if (mdp->mpip_qptr == NULL) {
	      oldpri = splhi();
	      if ((MIPX_DISCONNECT_RSP(ncpReq, mdp)) && (mdp->cinfo != NULL)) {
			cptr = mdp->cinfo;
			mdp->cinfo = NULL;
			kmem_free(cptr, sizeof(ipxConnInfo_t));
		}
		freemsg(bp);
		splx(oldpri);
		return(SUCCESS);
	}
	if(canput(WR(mdp->mpip_qptr)->q_next)) {
		if (MIPX_INCOMING_WDOG(ipxHeader, bp)) {
			if (mdp->state & (MIPX_WDOG_PENDING | MIPX_USED))
				return (Mipx_wdog_rsp(bp, mdp));
			else mdp->state |= MIPX_WDOG_PENDING;
		}
		if (MIPX_CONNECTION_RSP(ncpReq)) {
			mdp->ncpseqnum = 0;
			if (mdp->cinfo == NULL)
				mdp->cinfo = kmem_alloc(sizeof(ipxConnInfo_t), KM_NOSLEEP);
			if (mdp->cinfo == NULL) {
				MipxDropPacket(bp);
				return (SUCCESS);
			}
			mdp->cinfo->connNum = ((ipxNcpReq_t *)(bp->b_rptr))->connNum;
			mdp->cinfo->ServerAddr = ipxHeader->src;
		}
		ipxHeader->dest.sock[0] = mdp->dsock.sock[0];
		ipxHeader->dest.sock[1] = mdp->dsock.sock[1];
		ipxHeader->tc = IPX_TRANSPORT_CONTROL;
		putnext(WR(mdp->mpip_qptr), bp);
		return(SUCCESS);
	} else  return (FAIL);
    }
}

void
MipxDropPacket(mp)
mblk_t	*mp;		/* message pointer */
{
	if (mp)
		freemsg(mp);
	return;
}


/*
 *This handles the open and close requests by the merge process. On an open
 *it will grab  one of the free streams pointed to by the mipxdata
 *On a close, the stream is marked  as free. On a die, messsage has
 *to be sent to the appropriate controlling stream. We know that a die request
 *came from the mpip driver and now simply look for its controlling stream and 
 *send an M_DATA upstream for the admin process to retrieve. At this time we
 *also send out an End Of Job and and a Disconnect Request and set a timeout.
 *This timeout handles the retransmit of the two packets shoud that be needed
 *through the routine MipxReXmitDisconnect().
*/
int
MipxSetStreamOpts(q,mp)
queue_t	*q;		/* pointer to read queue */
mblk_t *mp;
{

	int	i;
	mblk_t  *opreq;
	struct iocblk *iocp;
	mipxData_t *p, *mdp, *cur;

	switch ((unsigned char) *(mp->b_rptr+2)) {
	case IPX_OPENSOCK:
		mdp = ((mipxData_t *)q->q_ptr);
		if (mdp->count <= 0) {
			cmn_err(CE_NOTE, "Linking incomplete");
			return (FAIL);
		}
		mdp = mdp->next;
		while ((mdp != NULL) && (mdp->state != MIPX_UNUSED))
			mdp = mdp->next;
		if (mdp == NULL) {
			MipxDropPacket(mp);
			return;
		}
		mdp->state = MIPX_USED;
		mdp->dsock.sock[0] = *((unsigned char *)mp->b_rptr + 4);
		mdp->dsock.sock[1] = *((unsigned char *)mp->b_rptr + 5);
/*
		cmn_err(CE_NOTE, " In OPEN DOS sock = %x %x ", mdp->dsock.sock[0], mdp->dsock.sock[1]);
*/
		mdp = ((mipxData_t *)q->q_ptr);
		if ( --mdp->count < 2 ) {
/*
*	This is called when the driver has run out of free streams to ipx.
*	This sends up a messsage via the mpip driver's controlling stream.
*	The mpip driver is identified by the fact that this is called only
*	in the event of an ipxopen request which comes down an mpip 
*	driver only.
*/
			*mp->b_rptr = IPX_GETMORESTREAMS;
			MTYPE(mp) = M_DATA;
			mp->b_wptr = mp->b_rptr + 1;
			cmn_err(CE_NOTE, " Hey, I'm asking for more");
        		putnext(mdp->ipx_qptr, mp);
/*
			The ipx_qptr on the first element of the list is 
			pointing to the controlling stream
*/
		} else freemsg(mp);
		break;
	case IPX_CLOSESOCK:
		for (mdp = q->q_ptr; mdp != NULL; mdp = mdp->next) {
			if (IPXCMPSOCK(((unsigned char *)mp->b_rptr +4), mdp->dsock.sock))
				break;
		}
		mdp->state = MIPX_UNUSED;
		mdp = q->q_ptr;
		mdp->count++;
		freemsg(mp);
		break;
	case IPX_DIE:
		opreq = allocb(sizeof(short), BPRI_MED);
		MTYPE(opreq) = M_DATA;
		*opreq->b_wptr++ = (unsigned char)IPX_DIE;
		putnext(((mipxData_t *)(q->q_ptr))->ipx_qptr, opreq);
		break;
	default:
		freemsg(mp);
	}
	return (SUCCESS);
}
mipxData_t
*MipxGetDataEntry(slot)
int *slot;
{

	int i;


	for (i=0; i < MIPX_DATA_SIZE; i++)
		if ((mipxData[i].ipx_qptr == NULL) && (mipxData[i].mpip_qptr == NULL)){
			if (slot != NULL) *slot = i;
			return (&mipxData[i]);
			}
	return ((mipxData_t *)NULL);
}

int
Mipx_outgoing_wdog(mp)
mblk_t *mp;
{
	ipxHdr_t *hptr;
	hptr = (ipxHdr_t *)mp->b_rptr;
	if ((hptr->len == 0x2000 ) && (hptr->pt == 0 ) && (*(mp->b_rptr + 31) == 'Y')) {
	return(TRUE);
	}
	return (FALSE);
}

int
Mipx_wdog_rsp(mp, mdp)
mblk_t *mp;
mipxData_t *mdp;
{
   ipxHdr_t *hptr;
   ipxAddr_t	tmp;

   hptr = (ipxHdr_t *)mp->b_rptr;
   *(mp->b_rptr + 31) = 'Y';
   tmp = hptr->dest;
   hptr->dest = hptr->src;
   hptr->src = tmp;
   if(canput(WR(mdp->ipx_qptr)->q_next)) {
	hptr->tc = IPX_TRANSPORT_CONTROL;
	putnext(WR(mdp->ipx_qptr), mp);
    	return(0);
   } else return (-1);
}
void
MipxSendImReady(mdp)
mipxData_t *mdp;
{
	mblk_t *tmp;

	if (mdp->mpip_qptr != NULL) {
		tmp = allocb(1, BPRI_MED);
		if ( tmp != NULL) {
			tmp->b_datap->db_type = M_DATA;
			*tmp->b_wptr++ = (unsigned char)MIPX_IMREADY;
			putnext(WR(mdp->mpip_qptr), tmp);
		}
	}
}

void
MipxSendEndOfJob(mdp)
mipxData_t *mdp;
{
	mblk_t *tmp;
	ipxNcpReq_t *tHdr;

	if (mdp->ipx_qptr == NULL)
		return;
	tmp = allocb(sizeof(ipxNcpReq_t), BPRI_MED);
	if (tmp != NULL) {
		tHdr = (ipxNcpReq_t *) tmp->b_wptr;
		tHdr->chksum = 0xffff;
		tHdr->len = XCHNG(37);
		tHdr->tc = IPX_TRANSPORT_CONTROL;
		tHdr->pt = 0x11;
		tHdr->dest = mdp->cinfo->ServerAddr;
		tHdr->reqType = 0x2222;	
		tHdr->seqNum = (mdp->ncpseqnum++) & 0xff;	
		tHdr->connNum = mdp->cinfo->connNum;	
		tHdr->taskNum= 0;	
		tHdr->reserved = 0;	
		tHdr->reqCode = 24;	
		tmp->b_wptr += 37;
   		if(canput(WR(mdp->ipx_qptr)->q_next))
			putnext(WR(mdp->ipx_qptr), tmp);
		else freemsg(tmp);
	}
}

void
MipxSendDisconnect(mdp)
mipxData_t *mdp;
{
	mblk_t *tmp;
	ipxNcpReq_t *tHdr;

	if (mdp->ipx_qptr == NULL)
		return;
	tmp = allocb(sizeof(ipxNcpReq_t), BPRI_MED);
	if (tmp != NULL) {
		tHdr = (ipxNcpReq_t *) tmp->b_wptr;
		tHdr->chksum = 0xffff;
		tHdr->len = XCHNG(36);
		tHdr->tc = IPX_TRANSPORT_CONTROL;
		tHdr->pt = 0x11;
		tHdr->dest= mdp->cinfo->ServerAddr;
		tHdr->reqType = 0x5555;	
		tHdr->seqNum = mdp->ncpseqnum & 0xff;	
		tHdr->connNum = mdp->cinfo->connNum;	
		tmp->b_wptr += 36;
   		if(canput(WR(mdp->ipx_qptr)->q_next))
			putnext(WR(mdp->ipx_qptr), tmp);
		else freemsg(tmp);
        }
}
