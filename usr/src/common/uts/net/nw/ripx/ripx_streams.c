/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/ripx_streams.c	1.9"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: ripx_streams.c,v 1.11 1994/09/14 16:42:23 meb Exp $"

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
/*
 *	ripx_streams.c
 *
 *  This module contains the streams driver entry points for
 *  open, close, and put, and the streams driver data.  The qinit
 *  srv routine (ripxchain) lives in rip.c, does not participate
 *  in any way in streams msg handling, and is used only for
 *  its timeout/qenable characteristics to manage periodic RIP
 *  updates.  The uwput procedure handles only IOCTLs from
 *  user-land, with none of the unsolicited traffic handled
 *  by most put routines.  The only unsolicited data seen by
 *  ripx comes from from lipmx across the Replaceable-Router
 *  Interface into rip.c, and so is not seen by this module.
 *  Ripx thus constitutes a user configurable/queryable driver,
 *  providing loadable kernel calls via a a strictly defined
 *  "side-door" interface to a more classically streams-ish driver
 *  (lipmx).  Ripx does not receive its unsolicited messages via
 *  the normal streams putnext mechanisms (no M_DATA handling).
 *
 * Lock usage - Code/structures protected:
 *  All entries and exits in this module are free of locks.
 *  We are called from streams context, and need only to
 *  protect our internal structures and code.  The only such
 *  protection needed here is to keep the minor and control
 *  device information sane and consistent in light of the
 *  fact that (while only one open/close at a time can occur
 *  on a given stream (q)) multiple, concurrent opens/closes
 *  can be happening across different streams.  The put examines
 *  only mblk_t* data to determine which ioctl.c function to call
 *  to handle the request, and does not examine/manipulate any
 *  ripx-internal data.
 */
#ifdef _KERNEL_HEADERS
#include <net/nw/ripx/rip.h>
#include <net/nw/ripx/ripx_streams.h>
#include <net/nw/ripx/ripx_ioctls.h>
#include <net/nw/ripx/rrrip.h>
#include <util/sysmacros.h>
#include <proc/cred.h>
#else
#include "rip.h"
#include "ripx_streams.h"
#include "ripx_ioctls.h"
#include "rrrip.h"
#include <sys/sysmacros.h>
#include <sys/cred.h>
#endif /* _KERNEL_HEADERS */

/*
 *	SapQ is used to send info about dead nets to sap
 */
static queue_t		*SapQ = NULL;
static atomic_int_t	 SapQCount;
static lock_t		*SapQLock;
       LKINFO_DECL( SapQInfo, "SapQLock", 0);

extern 	mblk_t	*lanKeys;
uint8   ripControlClosing;
FSTATIC int		RipSetSapQ(queue_t *);
FSTATIC void	RipClearSapQ(queue_t *);

/* bit-array for cloneopen minor #'s
 * These 2 structures must be kept sane during concurrent opens/closes across
 * multiple streams.
 */
		lock_t	*ripxDeviceLock = NULL;
static	uint32	Minors = 0;
static	struct	queue *Device0 = NULL;
       LKINFO_DECL( ripxDeviceInfo, "ripxDeviceLock", 0);

		int ripxopen(queue_t *, dev_t *, int, int, cred_t *);
		int ripxclose(queue_t *, int, cred_t *);
FSTATIC	int ripxuwput(queue_t *, mblk_t *);

/*
 * STREAMS Declarations:
 */
static struct module_info ripx_info = {
	M_RIPXID, "ripx", 0, INFPSZ, 0, 0
};

static struct qinit urinit = {
	NULL, NULL, ripxopen, ripxclose, NULL, &ripx_info, NULL
};

static struct qinit uwinit = {
	ripxuwput, ripxchain, NULL, NULL, NULL, &ripx_info, NULL
};

static struct qinit lrinit = {
	NULL, NULL, NULL, NULL, NULL, &ripx_info, NULL
};

static struct qinit lwinit = {
	NULL, NULL, NULL, NULL, NULL, &ripx_info, NULL
};

struct streamtab RIPX_INFO = {
	&urinit, &uwinit, &lrinit, &lwinit
};

/*
 * int ripxopen(queue_t *q, dev_t *devp, int flag, int sflag,
 *					cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 *
 * Lock usage - Code/structures protected:
 *  Device0, Minors
 */
/*ARGSUSED*/
int
ripxopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp)
{   dev_t   maj, min;
	uint32	priv;
	dev_t   dev;
	int		lvl;

	NTR_ENTER(5, q, devp, flag, sflag, credp);

	maj = getmajor(*devp);
	min = getminor(*devp);

	/*
	**	Check for root user
	*/
	priv = 0;
	if (drv_priv(credp) == 0)
		priv = PRIV_MASK;
	NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter ripxopen, Major device 0x%x, root flag %d", maj, priv));

	/*
	**	Allow only 31 clone devices
	**	use q_ptr as bit mask whose index is minor #
	**	device 0 bit denotes root user
	*/
	q->q_ptr = NULL;
	lvl = LOCK( ripxDeviceLock, plstr);
	if (sflag == CLONEOPEN) {
		/* protect against cloneopens if no ripx0 open */
		if (Device0 == NULL) {
			UNLOCK( ripxDeviceLock, lvl);
			return(NTR_LEAVE(ENXIO));
		}
		for (dev=1; dev < MAX_MINORS; dev++) {
			if (!(Minors & (uint32)(1 << dev))) {
				/* found free slot in bit array */
				/* Set bit for this device in q_ptr and Minors */
				q->q_ptr = (char *)((uint32)(1 << dev) | priv);
				Minors |= 1 << dev;
				OTHERQ(q)->q_ptr = q->q_ptr;
				break;
			}
		}
		if (q->q_ptr == NULL) {
			NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"ripxopen: Cloneopen fail : Minors bit array = 0x%X", Minors));
			UNLOCK( ripxDeviceLock, lvl);
			return(NTR_LEAVE(ENXIO));
		}

		NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"ripxopen: Minor device 0x%X, Minors bit mask 0x%X, q_ptr 0x%X",
			dev, Minors, (uint32)(q->q_ptr)));
		UNLOCK( ripxDeviceLock, lvl);
		*devp = makedevice(maj, dev);
		qprocson( q);
		return(NTR_LEAVE(0));
	}
	/* We're only here if this is the control device */
	if (sflag) { /* clone or mod open */
		UNLOCK( ripxDeviceLock, lvl);
		return(NTR_LEAVE(ENXIO));
	}

	if (Minors) {	/* clones still open from last start */
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"ripxopen: Attemp to open dev 0 while open clones 0x%X",
				 Minors));
		UNLOCK( ripxDeviceLock, lvl);
		return(NTR_LEAVE(EAGAIN));
	}

	/* ripx0 must be opened by root */
	q->q_ptr = (char *)priv;
	OTHERQ(q)->q_ptr = q->q_ptr;
	if (!IS_PRIV(q)) {
		UNLOCK( ripxDeviceLock, lvl);
		return(NTR_LEAVE(EPERM));
	}
	/* protect against multiple ripx0 opens */
	if (Device0 != NULL) {
		UNLOCK( ripxDeviceLock, lvl);
		return(NTR_LEAVE(EAGAIN));
	}
	Device0 = q;

	ripControlClosing = 0;

	NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"ripxopen: Open Control device 0x%X, q_ptr 0x%X, Minors 0x%X",
		min, (uint32)(q->q_ptr), Minors));

	UNLOCK( ripxDeviceLock, lvl);

	dev = min;

	if (ripStats.StartTime == 0) {
		drv_getparm( TIME, (void *)&ripStats.StartTime);
	}

	qprocson( q);

	return(NTR_LEAVE(0));
}

/*
 * int ripxuwput(queue_t *q, mblk_t *mp)
 *
 *  The upper write put procedure is responsible for handling all
 *  IOCTL calls coming downstream.  If an IOCTL is not recognized
 *  it is dropped.  Otherwise, the proper screening function in
 *  ioctls.c is called.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 *
 * Lock usage - Code/structures protected:
 *  None.
 */
int
ripxuwput(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	switch(MTYPE(mp)) {
	case M_IOCTL:
		NWSLOG((RIPXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"ripxuwput: M_IOCTL size %d, q_ptr = 0x%x",
			DATA_SIZE(mp), (uint32)(q->q_ptr)));
		switch (((struct iocblk *)mp->b_rptr)->ioc_cmd) {
		/*
		**	Initialization functions
		*/
		case RIPX_SET_SAPQ:
			ripStats.RipxIoctlSetSapQ++;
			RIPISetSapQ(q,mp);
			break;
		case RIPX_INITIALIZE:
			ripStats.RipxIoctlInitialize++;
			RIPInitRouter(q,mp);
			break;
		case RIPX_START_ROUTER:
			RIPStartRouter(q,mp);
			break;
		case RIPX_SET_ROUTER_TYPE:
			RIPSetRouterType(q,mp);
			break;
		/*
		**	Info functions
		*/
		case RIPX_GET_ROUTE_TABLE_HASH_SIZE:
			ripStats.RipxIoctlGetHashSize++;
			RipIGetRouteTableHashSize(q,mp);
			break;
		case RIPX_GET_HASH_TABLE_STATS:
			ripStats.RipxIoctlGetHashStats++;
			RipIGetHashTableStats(q,mp);
			break;
		case RIPX_DUMP_HASH_TABLE:
			ripStats.RipxIoctlDumpHashTable++;
			RipIDumpHashTable(q,mp);
			break;
		/*
		** Replaceable Router IOCTL Functions
		*/
		case RIPX_GET_ROUTER_TABLE:
			ripStats.RipxIoctlGetRouterTable++;
			RIPIGetRouterTable(q,mp);
			break;
		case RIPX_GET_NET_INFO:
			ripStats.RipxIoctlGetNetInfo++;
			RIPIGetNetInfo(q,mp);
			break;
		case RIPX_CHECK_SAP_SOURCE:
			ripStats.RipxIoctlCheckSapSource++;
			RIPICheckSapSource(q,mp);
			break;
		case RIPX_RESET_ROUTER:
			ripStats.RipxIoctlResetRouter++;
			RIPIResetRouter(q,mp);
			break;
		case RIPX_DOWN_ROUTER:
			ripStats.RipxIoctlDownRouter++;
			RIPIDownRouter(q,mp);
			break;
		case RIPX_STATS:
			ripStats.RipxIoctlStats++;
			RIPIInfo(q,mp);
			break;
		default:
			ripStats.RipxIoctlUnknown++;
			RipIBogusIoctl(q,mp);
			break;
		}
		break;

	default:
		NWSLOG((RIPXID,0,PNW_DROP_PACKET,SL_TRACE,
				"ripxuwput: freeing unknown MTYPE %d",MTYPE(mp)));
		freemsg(mp);
		break;

	}

	return(NTR_LEAVE(0));
}

/*
 * int ripxclose(queue_t *q, int flag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 *
 * Lock usage - Code/structures protected:
 *  Device0, Minors
 */
/*ARGSUSED*/
int
ripxclose(queue_t *q, int flag, cred_t *credp)
{
	int	lvl;
	mblk_t	*tmpPtr;

	NTR_ENTER(2, q, flag, 0,0,0);
	NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"ripxclose: ENTER q 0x%X flags %d ", q, flag));


	/* We are using q_ptr to hold the bit-array mask
	 * of the minor device number. 0 means control device,
	 * else index of non-zero bit is minor #.
	 */
	if (q == Device0) {
		/* control device is closing - cleanup */
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"ripxclose: q->q_ptr 0x%x is NULL",(uint32)(q->q_ptr)));

		/*
		**	Deregister before we set device lock
		*/
		RipDeregisterRouter();

		ripControlClosing = 1;

		RipUntimeout();

		qprocsoff( q);

		RIPfinish();

		lvl = LOCK( ripxDeviceLock, plstr);
		/* 
		 * Free all rrLanKey structures allocated in RIPgrantLanKey,
		 * they are linked together using b_next.
		 */
		while (lanKeys != NULL) {
			tmpPtr = lanKeys;
			lanKeys = lanKeys->b_next;
			freemsg(tmpPtr);
		}
		Device0 = NULL;
		ripx_free_heap();
	} else {
		/* clear the bit for this minor number
		 * in the Minors bit array
		 */
		qprocsoff( q);
		lvl = LOCK( ripxDeviceLock, plstr);
		Minors &= ~((uint32)(q->q_ptr) & MINOR_MASK);
	}

	NWSLOG((RIPXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
		"ripxclose: EXIT q 0x%X, q_ptr 0x%X, Minors 0x%X flags %d",
			q, (uint32)(q->q_ptr), Minors, flag));
	RipClearSapQ(q);
	UNLOCK( ripxDeviceLock, lvl);

	return(NTR_LEAVE(0));
}

/*
 * void NotifySapNetDead(uint32 deadNet)
 *
 * When RIP kill a net out of the RIT, this function is called to
 * notify SAP in a timely manner.  This saves SAP the embarassment
 * of advertising services on networks that are know longer reachable.
 * RUNS ONLY IN SERVICE ROUTINE CONTEXT.
 *
 * Calling/Exit State:
 * |S| No locking needed
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
NotifySapNetDead(uint32 deadNet)
{	mblk_t	*deadMp;
	uint32	*deadNum;
	int		 lvl;

	NTR_ENTER(1, deadNet, 0,0,0,0);

	if (SapQ != NULL) {
		lvl = LOCK( SapQLock, plstr);
		if( SapQ == NULL) {
			UNLOCK( SapQLock, lvl);
		} else {
			if ((deadMp = allocb(sizeof(uint32), BPRI_LO)) == NULL) {
				NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
					"NotifySapNetDead: allocb size %d failed",
					sizeof(uint32)));
				UNLOCK( SapQLock, lvl);
			} else {
				ATOMIC_INT_INCR( &SapQCount);
				UNLOCK( SapQLock, lvl);
				MTYPE(deadMp) = M_DATA;
				deadNum = (uint32 *)deadMp->b_rptr;
				deadMp->b_wptr += sizeof(uint32);
				*deadNum = deadNet;
				putnext(SapQ, deadMp);
				ATOMIC_INT_DECR( &SapQCount);
			}
		}
	}
	NTR_VLEAVE();
	return;
}

/*
 * void RIPISetSapQ(queue_t *q, mblk_t *mp)
 *
 * For proper RIP/SAP interaction, SAP needs to be quickly
 * informed when a network is deleted from the RIT (so that
 * it doesn't advertise services available on networks that
 * are no longer alive).  SAP will therefore open /dev/ripx
 * and notify us that this stream should receive such notifications.
 *
 * Calling/Exit State:
 *  No locks held on exit or entry.
 *
 * Lock usage - Code/structures protected:
 *  SapQLock
 */
void
RIPISetSapQ(queue_t *q, mblk_t *mp)
{
	int	lvl;

	NTR_ENTER(2, q, mp, 0,0,0);

	/* ioctl root only usage */
	if (!IS_PRIV(q)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPISetSapQ: non root access attempted"));
		RipIocNegAck(q,mp,EPERM);
		NTR_VLEAVE();
		return;
	}

	if (mp->b_cont != NULL) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPISetSapQ: non-zero size data area"));
		RipIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	
	lvl = LOCK( SapQLock, plstr);
	if (SapQ != NULL) {
		UNLOCK( SapQLock, lvl);
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPISetSapQ: attempt to set SapQ more than once"));
		RipIocNegAck(q,mp,EINVAL);
	} else {
		SapQ = RD(q);
		UNLOCK( SapQLock, lvl);
		NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RIPISetSapQ: SapQ set to 0x%X", RD(q)));
		RipIocAck(q,mp,0);
	}

	NTR_VLEAVE();
	return;
}

/*
 * void RipClearSapQ(queue_t *Q)
 *
 * A queue is going away (stream being closed.  If it is
 * the queue that identified itself as the one to which
 * network death notifications should be sent, NULL out
 * that queue_t*.
 *
 * Calling/Exit State:
 *	DeviceLock set on exit and entry
 *
 * Lock usage - Code/structures protected:
 *  SapQLock
 */
FSTATIC void
RipClearSapQ(queue_t *Q)
{
	int		lvl;
	NTR_ENTER(1, Q, 0,0,0,0);

	if (Q == SapQ) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipIClearSapQ: SapQ being NULLed"));
		for( ;; ) {
			lvl = LOCK( SapQLock, plstr);
			if( ATOMIC_INT_READ( &SapQCount) != 0) {
				UNLOCK( SapQLock, lvl);
				delay(1);
				continue;
			} else {
				SapQ = NULL;
				UNLOCK( SapQLock, lvl);
				break;
			}
		}
	}

	NTR_VLEAVE();
	return;
}

/*
 * int ripxAllocStreamsLocks(void)
 *
 * Allocate locks used by streams, namely
 * the minors bit map, and the SapQ.  Called at driver load.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Lock usage
 *  ripxDeviceLock and SapQLock Allocated
 */
int
ripxAllocStreamsLocks( void)
{

	NTR_ENTER(0, 0, 0,0,0,0);
    /*
    **  Allocate device lock
    */
    ripxDeviceLock = LOCK_ALLOC( DEVICE_LOCK, plstr, &ripxDeviceInfo, KM_SLEEP);
    if( ripxDeviceLock == NULL) {
        return( NTR_LEAVE(EAGAIN));
    }

    /*
    **  Allocate SapQ lock
    */
    SapQLock = LOCK_ALLOC( SAPQ_LOCK, plstr, &SapQInfo, KM_SLEEP);
    if( SapQLock == NULL) {
        return( NTR_LEAVE(EAGAIN));
    }
	ATOMIC_INT_INIT( &SapQCount, 0);

	return(NTR_LEAVE(0));
}

/*
 * void ripxDeallocStreamsLocks(void)
 *
 * Deallocate locks used by streams, namely
 * the minors bit map, and the SapQ.  Called at driver unload.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Lock usage 
 *  ripxDeviceLock and SapQLock Deallocated
 */
void
ripxDeallocStreamsLocks( void)
{
	NTR_ENTER(0, 0, 0,0,0,0);
    /*
    **  Deallocate device lock
    */
    LOCK_DEALLOC( ripxDeviceLock);

    /*
    **  Deallocate SapQ lock
    */
    LOCK_DEALLOC( SapQLock);

	NTR_VLEAVE();
	return;
}
