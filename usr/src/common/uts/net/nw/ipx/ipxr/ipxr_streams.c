/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/ipxr/ipxr_streams.c	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: ipxr_streams.c,v 1.5 1994/04/04 18:26:15 vtag Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#ifdef _KERNEL_HEADERS
#include <net/nw/ipx/lipmx/lipmx.h>
#include <net/nw/ipx/lipmx/lipmx_ioctls.h>
#include <net/nw/ipx/lipmx/norouter.h>

#include <util/sysmacros.h>
#include <proc/cred.h>
#include <io/ddi.h>
#else
#include "lipmx.h"
#include "lipmx_ioctls.h"
#include "norouter.h"
#endif /* _KERNEL_HEADERS */

/* bit-array for cloneopen minor #'s */
static int		minors = 0;

/* Set if control opened */
static queue_t	*Urq = NULL;

extern lock_t			*sapQLock;
extern queue_t  		*sapQ;		/* q for sapping when !underIpx */
extern atomic_int_t		sapQcount;	/* count of putnext calls active for sapQ */
LKINFO_DECL(sapQLkinfo, "SapQueueLock", 0);

/*
 *	bit-masks for cloneopen minor #'s
 */
#define MAX_MINORS		(sizeof(q->q_ptr) * 8)
#define PRIV_MASK		1
#define MINOR_MASK		(~(uint32)1)

/*
 * Forward References for STREAMS
 */
int		ipxopen();
int		ipxclose();
int		ipxuwput();
int		ipxlrput();
int		ipxlwsrv();

/*
 * STREAMS Declarations:
 */
static struct module_info ipx_rinfo = {
	M_IPXID, "ipx", 0, INFPSZ, IPX_R_HI_WATER, IPX_R_LO_WATER
};

static struct module_info ipx_winfo = {
	M_IPXID, "ipx", 0, INFPSZ, IPX_W_HI_WATER, IPX_W_LO_WATER
};

static struct qinit urinit = {
	NULL, NULL, ipxopen, ipxclose, NULL, &ipx_rinfo, NULL
};

static struct qinit uwinit = {
	ipxuwput, NULL, NULL, NULL, NULL, &ipx_winfo, NULL
};

static struct qinit lrinit = {
	ipxlrput, NULL, NULL, NULL, NULL, &ipx_rinfo, NULL
};

static struct qinit lwinit = {
	NULL, ipxlwsrv, NULL, NULL, NULL, &ipx_winfo, NULL
};

extern struct streamtab ipxinfo = {
	&urinit, &uwinit, &lrinit, &lwinit
};

int
ipxinit()
{ 
	NTR_ENTER(0, 0,0,0,0,0);
    sapQLock = LOCK_ALLOC(IPXRSAP_LOCK, plstr, &sapQLkinfo, KM_SLEEP);
	if(sapQLock == NULL) {
        return( NTR_LEAVE(EAGAIN));
	}
	ATOMIC_INT_INIT(&sapQcount, 0);
	lipmxinit();
	return(NTR_LEAVE(0));
}

/*
 * int ipxfini(void)
 *  Called during driver unload.  Deallocates locks
 *
 * Calling/Exit State:
 *  No locks set on entry or exit.
 *  Lock structures are deallocated.
 */
int
ipxfini(void)
{

	NTR_ENTER(0, 0,0,0,0,0);

	/*
	**  Decallocate the locks
	*/	
	LOCK_DEALLOC(sapQLock);
	sapQLock = NULL;
 
	return(NTR_LEAVE(0));
}

int
LipmxTestUQ( void)
{
	NTR_ENTER(0, 0, 0,0,0,0);
	if(Urq)
		return(NTR_LEAVE(TRUE));
	return(NTR_LEAVE(FALSE));
}

int
LipmxSetUQ(queue_t *q)
{
	NTR_ENTER(1, q, 0,0,0,0);
	if(Urq)
		return(NTR_LEAVE(FALSE));
	Urq = q;
	return(NTR_LEAVE(TRUE));
}

void
LipmxClearUQ( void)
{
	NTR_ENTER(0, 0,0,0,0,0);
#ifdef DEBUG
	if(!Urq)
		printf("LipmxClearUQ: Urq being cleared when already NULL !!\n");
#endif /* DEBUG */

	Urq = (queue_t *)NULL;
	NTR_VLEAVE();
	return;
}

int
IsPriv( queue_t *q) {
	return( (int)(q->q_ptr) & PRIV_MASK);
}

int
IsControl( queue_t *q) {
	return( (int)q->q_ptr != PRIV_MASK);
}

void
LipmxClearSapQ(queue_t *q)
{
	pl_t	lvl;

	NTR_ENTER(1, q, 0,0,0,0);

	lvl = LOCK(sapQLock, plstr);

	if(sapQ && (q == sapQ)) {
#ifdef DEBUG
		printf("LipmxClearSapQ: sapQ closed\n");
#endif /* DEBUG */
		NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
				"LipmxClearSapQ : sapQ closed"));
		while(ATOMIC_INT_READ(&sapQcount)) {
			delay(1);
		}
		sapQ = (queue_t *)NULL;
	}
	UNLOCK(sapQLock, lvl);
	NTR_VLEAVE();
	return;
}

void
IpxRouteDataToSocket( mblk_t *mp)
{
	NWSLOG((LIPMXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
		"dummy-IpxRouteDataToSocket: Free message, message to internal lan and no internal lan"));
#ifdef DEBUG
		printf("dummy-IpxRouteDataToSocket: Free message, message to internal lan and no internal lan\n");
#endif
	freemsg(mp);
	return;
}

/*
 *	Send hangup to SAPD, the only process we are interested in
 *	if not under ipx
 */
void
IpxSendHangup( void)
{
	mblk_t	*hup;
	pl_t	lvl;

	NTR_ENTER( 0, 0, 0, 0,0,0);
	lvl = LOCK(sapQLock, plstr);
	if( sapQ) {
		if ((hup = allocb(0,BPRI_MED)) != NULL) {
			MTYPE(hup) = M_HANGUP; 	
			ATOMIC_INT_INCR(&sapQcount);
			UNLOCK(sapQLock, lvl);
			putnext( sapQ, hup );
			ATOMIC_INT_DECR(&sapQcount);
		} else {
			UNLOCK(sapQLock, lvl);
		}
	} else {
		UNLOCK(sapQLock, lvl);
	}
	NTR_VLEAVE();
	return;
}
/* q = pointer to read queue */
/* devp = major/minor device number -- zero for modules */
/* flag = file open flags -- zero for modules */
/* sflag = stream open flags */
/* credp = credentials structure */
int
ipxopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp)
{	dev_t	maj;
	uint32	priv;
	dev_t	dev;

	NTR_ENTER( 5, q, devp, flag, sflag, credp);
	maj = getmajor(*devp);

	/*
	**  Check for root user
	*/
	priv = 0;
	if( drv_priv( credp) == 0)
		priv = PRIV_MASK;
	NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter lipmxopen, Major device 0x%x, root flag %d", maj, priv));

	/* use q_ptr as bit mask whose index is minor #
	 */
	q->q_ptr = 0;
	if (sflag == CLONEOPEN) {
		/* protect against cloneopens if no lipmx0 open */
		if(!LipmxTestUQ())
			return(NTR_LEAVE(ENXIO));
	
		for (dev=1; dev <= MAX_MINORS; dev++) {
			if(!(minors & (uint32)(1 << dev))) {

				/* found free slot in bit array */
				/* Set bit for this device in q_ptr and minors */
				q->q_ptr = (char *)( (uint32)(1 << dev) | priv);
				minors |= 1 << dev;
				OTHERQ(q)->q_ptr = q->q_ptr;
				break;
			}
		}
		if(q->q_ptr) {
#ifdef DEBUG
			printf("Lipmx: CLONEOPEN : minor = 0x%X    minors = 0x%X\n",
				dev, minors);
#endif /* DEBUG */
			NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
				"lipmxopen: Device 0x%X, Minors bit mask 0x%X, q_ptr 0x%X",
				dev, minors, (uint32)(q->q_ptr)));
			*devp = makedevice(maj, dev);
			return(NTR_LEAVE(0));
		} else {
#ifdef DEBUG
			printf("Lipmx: Cloneopen fail : Minors bit array = 0x%Xh\n",
				minors);
#endif
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"lipmxopen: Cloneopen fail : Minors bit array = 0x%Xh", minors));
			return(NTR_LEAVE(EAGAIN));
		}
	} else {
		if(sflag) /* Make sure not modopen */
			return(NTR_LEAVE(ENXIO));
	}

	/* lipmx0 must be opened by root */
	q->q_ptr = (char *)priv;
	OTHERQ(q)->q_ptr = q->q_ptr;
	if( !IsPriv(q))
		return(NTR_LEAVE(EPERM));
	/* Don't allow a double open */
	if(!LipmxSetUQ(q))
		return(NTR_LEAVE(EAGAIN));

	NWSLOG((LIPMXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
		"Exit LIPMX open"));
	return(NTR_LEAVE(0));
}

/*ARGSUSED*/
int
ipxclose(queue_t *q, int flag, cred_t *credp)
{
	NTR_ENTER( 3, q, flag, credp,0,0);

	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"lipmxclose: ENTER q %xh flags %d ",q, flag));

	if (q == NULL) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"lipmxclose: q %xh is NULL",q));
		return((int)NTR_LEAVE(0));
	}

	/* We are using q_ptr to hold the bit-array mask
	 * of the minor device number. 0 means control device,
	 * else index of non-zero bit is minor #.
	 */
	/* control device is closing - cleanup */
	if ( IsControl( q)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"lipmxclose: q->q_ptr %xh is NULL",q->q_ptr));

		LipmxClearUQ();
#ifdef DEBUG
		printf("lipmx: control close : lans now = 0   minors = 0x%X\n", minors);
#endif /* DEBUG */
	} else {
		/* clear the bit for this minor number
		 * in the minors bit array
		 */
		minors &= ~((uint32)(q->q_ptr) & MINOR_MASK);
		LipmxClearSapQ(q);
#ifdef DEBUG
		printf("lipmx : clone close : minors = 0x%X\n", minors);
#endif /* DEBUG */
	}
	
	if( !LipmxTestUQ() && (minors == 0)) {
		/* ALL devices closed */
		lipmxCleanUp();
	}
	NWSLOG((LIPMXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
		"lipmxclose: EXIT q %xh flags %d", q, flag));

	return((int)NTR_LEAVE(0));
}

int
ipxuwput( queue_t *q, mblk_t *mp)
{
	return(lipmxuwput( q, mp));
}
int
ipxlrput( queue_t *q, mblk_t *mp)
{
	return(lipmxlrput( q, mp));
}
int
ipxlwsrv( queue_t *q)
{
	return(lipmxlwsrv( q));
}
/*
 *  The following are to allow lipmx to inform ipx that
 *  the link is successful
 */
void
IpxIncDlLink(void)
{
	return;
}
void
IpxDecDlLink(void)
{
	return;
}

/*ARGSUSED*/
void
IpxSetInternalNetNode(uint32 *netp, uint8 *nodep)
{
	return;
}
