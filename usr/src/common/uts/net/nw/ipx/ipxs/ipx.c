/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/ipxs/ipx.c	1.22"
#ident	"$Id: ipx.c,v 1.39.2.1 1994/11/08 20:49:02 vtag Exp $"

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
#include <net/nw/ipx/ipxs/ipx.h>
#else
#include "ipx.h"
#endif

/*
 * Forward References for STREAMS
 */
int		ipxuwput(queue_t *q, mblk_t *mp);
int		ipxclose(queue_t *q, int flag, cred_t *credp);
int		ipxopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp);

/*
 * Forward References for General routines and Ipx Functions
 */
FSTATIC	void	IpxUpperFlush(queue_t *, mblk_t *);
FSTATIC	mblk_t *IpxSumPacket(mblk_t *, int);
FSTATIC	void	IpxPropagatePkt(queue_t *, mblk_t *);
FSTATIC	void	IpxDeallocateSocket( ipxSocket_t *);
FSTATIC	int		IpxDeallocateDevice( int);
FSTATIC	void	IpxRemoveFromDeviceList( ipxSocket_t *);

#define	SAPsocket GETINT16(SAP_SOCKET)
#define	RIPsocket GETINT16(RIP_SOCKET)

/*
 *	statistics
 */
static	IpxSocketStats_t	ipxStats = { 0 };	/* Ipx statistics structure */

static	ipxInitialized = 0;					/* Set when initializion complete */
static	ipxMinMaxSDU = IPX_MAX_PACKET_DATA;	/* Min of all lans MaxSDU */

/*
 *	Socket Multiplexor Hash Table
 */
FSTATIC	ipxHash_t	SocketHashTable[256] = {NULL};	/* socket # hash table */
		LKINFO_DECL( ipxHashLkinfo, "IpxHashLock", 0);

/*
 *	Queue Lock Lock Info
 */
		LKINFO_DECL( ipxQueueLkinfo, "IpxQueueLock", 0);
/*
 *	Device Lock
 *		List of devices with no socket bound
 *		Also locks device bit map
 */
FSTATIC lock_t		*ipxDeviceLock = NULL;
FSTATIC ipxSocket_t	*ipxDeviceList = NULL;	
		LKINFO_DECL( ipxDeviceLkinfo, "IpxDeviceLock", 0);

static uint32		ipxInternalNet = 0;		/* Internal Net,Node from lipmx */
static uint8		ipxInternalNode[IPX_NODE_SIZE] = {0};

static int			hiWaterMark = IPX_R_HI_WATER;
static int			loWaterMark = IPX_R_LO_WATER;

/* Keep track of when it's time to dealloc structs */
static int32	ClosesPending = 0;			/* Number of open devices */

/*
 * Forward References for IOCTLs
 */
FSTATIC void	IpxIocAck(queue_t *, mblk_t *, uint);
FSTATIC void	IpxIocNegAck(queue_t *, mblk_t *, int);
FSTATIC void	IpxIocBindSocket(queue_t *, mblk_t *, int, int);
FSTATIC void	IpxIocUnbindSocket(queue_t *, mblk_t *);
FSTATIC void	IpxIocSetWater(queue_t *, mblk_t *);
FSTATIC void	IpxIocStats(queue_t *q, mblk_t *mp);
FSTATIC void	IpxIocInitialize( queue_t *q, mblk_t *mp);

/*
 * Forward References for TPI Interface Functions
 */
FSTATIC void    IpxTpiUDataReq(queue_t *, mblk_t *);
FSTATIC void    IpxTpiUDataInd(queue_t *, mblk_t *);
FSTATIC void    IpxTpiOptReq(queue_t *, mblk_t *);
#ifndef NW_TLI
FSTATIC void 	IpxTpiTAddrReq(queue_t *, mblk_t *);
#endif
FSTATIC void    IpxTpiInfoReq(queue_t *, mblk_t *);
FSTATIC void	IpxTpiAddrReq(queue_t *, mblk_t *);
FSTATIC void    IpxTpiBindReq(queue_t *, mblk_t *, int);
FSTATIC void    IpxTpiUnbindReq(queue_t *);

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
	lipmxlrput, NULL, NULL, NULL, NULL, &ipx_rinfo, NULL
};

static struct qinit lwinit = {
	NULL, lipmxlwsrv, NULL, NULL, NULL, &ipx_winfo, NULL
};

struct streamtab ipxinfo = {
	&urinit, &uwinit, &lrinit, &lwinit
};

#ifdef DEBUG
#undef SOCKETHASH
/*
 * uint16 SOCKETHASH(uint16 *sock)
 *	DEBUG FUNCTION to check prototyping for SOCKETHASH.  A macro  when
 *	DEBUG not set
 *
 * Calling/Exit State:
 *	Locks may be held accross calls to this function
 */
uint16
SOCKETHASH( uint16 *sock)
{
	uint8 *p;
	p = (uint8 *)sock;
	return((p[0] + p[1]) & HASH_MASK);
}
#endif /* DEBUG */
/*
 * int ipxinit(void)
 *	Called during driver load.  Allocates locks and initialized data structures
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 *	Lock structures are allocated for the Device Lock and the Hash Locks
 */
int
ipxinit(void)
{
	uint32 devSize;
	int		i;

	NTR_ENTER(0, 0,0,0,0,0);

	/*
	 *+ Inform that driver is loaded
	 */
#ifdef DEBUG
	cmn_err(CE_CONT, "%s %s%s: %s %s\n",
						IPXSSTR, IPXSVER, IPXSVERM, __DATE__, __TIME__);
#else
	cmn_err(CE_CONT, "%s %s%s\n", IPXSSTR, IPXSVER, IPXSVERM);
#endif /* DEBUG */

	ipxStats.IpxMajorVersion = (uint8)IPX_MAJOR;
	ipxStats.IpxMinorVersion = (uint8)IPX_MINOR;
	ipxStats.IpxRevision[0] = (char)IPX_REV1;
	ipxStats.IpxRevision[1] = (char)IPX_REV2;

	/*
	 *	Allocate device lock
	 */
	ipxDeviceLock = LOCK_ALLOC( DEVICE_LOCK, plstr, &ipxDeviceLkinfo, KM_SLEEP);
	if( ipxDeviceLock == NULL) {
		return( NTR_LEAVE(EAGAIN));
	}

	/*
	 *	Allocate hash locks
	 */
	for( i=0; i<=HASH_MASK; i++) {
		SocketHashTable[i].hlock =
			LOCK_ALLOC( HASH_LOCK, plstr, &ipxHashLkinfo, KM_SLEEP);
		if( SocketHashTable[i].hlock == NULL) {
			int j;
			LOCK_DEALLOC( ipxDeviceLock);
			ipxDeviceLock = NULL;
			for( j=0; j<i; j++) {
				LOCK_DEALLOC( SocketHashTable[j].hlock);
				SocketHashTable[j].hlock = NULL;
			}
			return( NTR_LEAVE(EAGAIN));
		}
	}

	/*
	 *	Initialize Device Bit Array
	 */
	devSize = (ipxMaxDevices + 7) / 8;
	devSize = ((devSize + 3)/4) * 4;
	bzero( (char *)ipxDevices, devSize);

	for( i = 0; i <= ipxMaxDevices; i++) {
		IpxDeallocateDevice(i);
	}
	/*
	 *	Initialize lipmx
	 */
	lipmxinit();
	/*
	 *	Inform lipmx it is running under ipx
	 */
	LipmxSetUnderIpx();
	
	return(NTR_LEAVE(0));
}

/*
 * int ipxfini(void)
 *	Called during driver unload.  Deallocates locks
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 *	Lock structures are deallocated for the Device Lock and the Hash Locks
 */
int
ipxfini(void)
{
	int	i;

	NTR_ENTER(0, 0,0,0,0,0);

	/*
	 *	Decallocate the device lock
	 */
	LOCK_DEALLOC( ipxDeviceLock);
	ipxDeviceLock = NULL;

	/*
	 *	Decallocate the hash locks
	 */
	for( i=0; i<=HASH_MASK; i++) {
		LOCK_DEALLOC( SocketHashTable[i].hlock);
		SocketHashTable[i].hlock = NULL;
	}
	
	return(NTR_LEAVE(0));
}


/*
 * int IsPriv( queue_t *wrq)
 *	Function to allow ipx/lipmx to determine if a function is root
 *	Returns non-zero if privileged
 *
 * Calling/Exit State:
 *	No locks acquired or released by this function.
 *	Locks may be held across calls to this function.
 */
int
IsPriv( queue_t *wrq)
{
	ipxPDS_t	*pds;
	pds = (ipxPDS_t *)&(RD(wrq)->q_ptr);
	return( pds->priv & PRIV_USER);
}

/*
 * int IsControl( queue_t *wrq)
 *	Function to allow ipx/lipmx to determine if a device is the control device
 *	Returns non-zero if the control device
 *
 * Calling/Exit State:
 *	No locks acquired or released by this function.
 *	Locks may be held across calls to this function.
 */
int
IsControl( queue_t *wrq)
{
	ipxSocket_t	*sock;
	sock = (ipxSocket_t *)wrq->q_ptr;
	return( sock->device == 0xFFFF);
}

/*
 * void IpxSetInternalNetNode(uint32 *lanp, uint8 *nodep)
 *	Function to allow ipx/lipmx to determine if a device is the control device
 *	Returns non-zero if the control device
 *
 * Calling/Exit State:
 *	No locks acquired or released by this function.
 *	Locks may be held across calls to this function.
 */
void
IpxSetInternalNetNode(uint32 *lanp, uint8 *nodep)
{
	NTR_ENTER( 2, lanp, nodep, 0, 0, 0);
	/*
	 *	Get Internal Net and Node, when we get this ioctl, lipmx
	 *	is set up.
	 */
	IPXCOPYNET( lanp, &ipxInternalNet);
	IPXCOPYNODE( nodep, ipxInternalNode);
	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxSetInternalNetNode: Set internal net/node to 0x%X/0x%X%X",
		GETINT32(ipxInternalNet),PGETINT32(ipxInternalNode),
		PGETINT16(&ipxInternalNode[4])));
	NTR_VLEAVE();
	return;
}

/*
 * void IpxSendHangup( int lvl)
 *  Sends a hangup func Sends hangup to all open devices except the control
 * device.  This function is called by ipxclose only.
 *
 * Calling/Exit State:
 *	DEVICE_LOCK set on entry and no locks on exit
 *	Function acquires/releases QUEUE, and HASH locks
 */
void
IpxSendHangup( pl_t lvl)
{
	uint32	noNet = 0;
	uint8	noNode[IPX_NODE_SIZE] = {0};
	uint16 idx;
	queue_t	*rdq;
	ipxPDS_t *pds;
	ipxSocket_t *sock, *tsock;

	NTR_ENTER(1, lvl, 0, 0, 0, 0);

	/*
	 *	Loop through hash table, find a queue, and release
	 *	all sockets bound to the queue
	 */
	for(idx=0; idx < 256; idx++) {
		while( SocketHashTable[idx].hlink != NULL) {
			sock = SocketHashTable[idx].hlink;
			rdq = sock->qptr;
			pds = (ipxPDS_t *)&(rdq->q_ptr);

			NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
				"IpxSendHangup: pds @ 0x%X, socket 0x%X, priv 0x%X",
				pds, GETINT16(pds->socket), pds->priv));
			
			/*
			 *	Release all sockets bound to this queue
			 */
			while( pds->socket) {
				sock = (ipxSocket_t *)WR(rdq)->q_ptr;
				NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
					"IpxSendHangup: sock @ 0x%X, sock->socket 0x%X, pds->socket 0x%X",
					sock, GETINT16(sock->socket), GETINT16(pds->socket)));
				/*
				 *	Check for permanent socket structure that has been unbound
				 */
				while( sock->socket == 0) {
					sock = sock->qlink;
					NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
						"IpxSendHangup: next sock struct @ 0x%X, socket 0x%X",
						sock, GETINT16(sock->socket)));
					ASSERT( sock != NULL);
				}

				/* Check for bad socket qlink */
				ASSERT( sock != NULL);
				ASSERT( pds->socket == sock->socket);
				ASSERT( sock->qptr == rdq);
				if( sock->socket == RIPsocket) {
					LipmxRouteRIP( DISABLE_RIP);/* stop routing RIP */
				}
				IpxDeallocateSocket( sock);
			}	
		}
	}
	/*
	 *	All sockets are now deallocated, and all permanent socket structures
	 *	are on the ipxDeviceList.
	 *
	 *	Increment the qcount to force the list to stay constant until we
	 *	are done with it.
	 */
	sock = ipxDeviceList;
	while( sock != NULL) {
		rdq = sock->qptr;
		if( IsControl( WR(rdq))) {
			sock = sock->hlink;
			continue;
		}
		/*
		 *	Don't need hash lock here, because it isn't linked to hash table
		 *	This ensures it remains on the DeviceList until we are done with it.
		 */
		ATOMIC_INT_INCR( &sock->qcount);
		sock = sock->hlink;
	}
	UNLOCK( ipxDeviceLock, lvl);
	/*
	 *	Send hangup to all devices not bound to a socket
	 *	Do not send hangup to the control device
	 */
	sock = ipxDeviceList;
	while( sock != NULL) {
		rdq = sock->qptr;
		if( IsControl( WR(rdq))) {
			sock = sock->hlink;
			continue;
		}
		NWSLOG((IPXID,1,PNW_EXIT_ROUTINE,SL_TRACE,
			"IpxSendHangup: putctl(M_HANGUP) to device %d",
			 sock->device));
		lvl = putnextctl(rdq, M_HANGUP);
		tsock = sock;
		sock = sock->hlink;
		ATOMIC_INT_DECR( &tsock->qcount);
#ifdef DEBUG
		if( lvl == 0) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"IpxSendHangup : putctl(M_HANGUP) FAILED !!"));
			/*
			 *+ Cannot send hangup
			 */
			cmn_err(CE_WARN, "IpxSendHangup : putctl(M_HANGUP) FAILED !!");
		}
#endif
	}
	IPXCOPYNET(&noNet, &ipxInternalNet);
	IPXCOPYNODE(noNode, ipxInternalNode);

	NTR_VLEAVE();
	return;
}

/*
 * uint16 IpxAllocateDevice( void)
 *	Finds and unused device number in the device structure.
 *	Called from ipxopen.
 *
 * Calling/Exit State:
 *	DEVICE_LOCK set on entry and exit
 *	Other locks may be held across calls to this function.
 */
FSTATIC uint16
IpxAllocateDevice( void)
{
	uint16 dev, devidx, devno;

	NTR_ENTER(0, 0, 0, 0, 0, 0);

	for( devidx = 0; (int)devidx < (int)((ipxMaxDevices + 31)/32); devidx++) {
		if( *(ipxDevices + devidx) == 0)
			continue;
        for (dev = 0; dev < 32; dev++) {
            if(*(ipxDevices + devidx) & (uint32)(1 << dev)) {
                /*
				 *	found free slot in bit array, Clear bit for this device
				 */
				devno = (devidx * 32) + dev;
                *(ipxDevices + devidx) &= ~(uint32)(1 << dev);
                return( NTR_LEAVE(devno));
            }
        }
	}
	/*
	 *	All out of devices
	 */
	return( NTR_LEAVE(0xFFFF));
}

/*
 * int IpxDeallocateDevice( int devno)
 *	Releases a device number to the device structure.
 *	Called from ipxclose.
 *
 * Calling/Exit State:
 *	DEVICE_LOCK set on entry and exit
 *	Other locks may be held across calls to this function.
 */
FSTATIC int
IpxDeallocateDevice( int devno)
{
	int		dev, devidx;
	uint32	bit;

	NTR_ENTER(1, devno, 0, 0, 0, 0);
	/*
	 *	Set bit for this device, device number is available
	 */
	devidx = devno / 32;
	dev = devno % 32;
	bit = 1 << dev;
	if( *(ipxDevices + devidx) & bit) {
		/* Already available, error */
		return(NTR_LEAVE(0));
	}
	*(ipxDevices + devidx) |= (uint32)(1 << dev);

	return(NTR_LEAVE(1));
}

/*
 * void ipxAddToDeviceList( ipxSocket_t *sock)
 *	Adds the permanent socket structure for a queue to the list of
 *	devices that have no sockets bound.
 *	Other locks may be held across calls to this function.
 *
 * Calling/Exit State:
 *	DEVICE_LOCK set on entry and exit
 */
FSTATIC void
ipxAddToDeviceList( ipxSocket_t *sock)
{
	NTR_ENTER(1, sock, 0, 0, 0, 0);
	sock->hlink = ipxDeviceList;
	ipxDeviceList = sock;
	
	NTR_VLEAVE();
	return;
}

/*
 * void IpxRemoveFromDeviceList( ipxSocket_t *sock)
 *	Removes permanent socket struct for the queue from the list
 *	of devices not bound to a socket.
 *	Other locks may be held across calls to this function.
 *
 * Calling/Exit State:
 *	DEVICE_LOCK set on entry and exit
 */
FSTATIC void
IpxRemoveFromDeviceList( ipxSocket_t *sock)
{
	ipxSocket_t *psock;
	
	NTR_ENTER(1, sock, 0, 0, 0, 0);

	/* Make sure devicelist is not null */
	ASSERT( ipxDeviceList != NULL);

	/*
	 *	Make sure we are not in hangup, don't remove from list if
	 *	SendHangup is doing a putnextctl
	 */
#ifndef NW_UP
	while( ATOMIC_INT_READ( &sock->qcount)) {
		drv_usecwait(10);
	}
#endif
	/*
	 *	Remove from device list
	 */
	if( ipxDeviceList == sock) {
		ipxDeviceList = sock->hlink;
	} else {
		psock = ipxDeviceList;
		while( psock->hlink != 0) {
			if( psock->hlink == sock) {
				/*	Found it, unlink it */
				psock->hlink = sock->hlink;
				psock = NULL;	/* Flag that we found it */
				break;
			}
			psock = psock->hlink;
		}
		/* Socket struct not in list, cannot deallocate */
		ASSERT( psock == NULL);

	}
	NTR_VLEAVE();
	return;
}


/*
 * ipxSocket_t * IpxQueueFindSocket( queue_t *rdq, uint16 socketNetOrder)
 *	Find a socket linked to a queue of a stream.  A socket number of
 *	identifies means socket struct with no socket bound (i.e. available
 *	socket structure).
 *
 * Calling/Exit State:
 *	QUEUE_LOCK set on entry and exit.
 *	Other locks may be held across calls to this function.
 */
FSTATIC ipxSocket_t *
IpxQueueFindSocket( queue_t *rdq, uint16 socketNetOrder)
{
	ipxSocket_t *sock;

	NTR_ENTER(2, rdq, socketNetOrder, 0, 0, 0);

	sock = (ipxSocket_t *)WR(rdq)->q_ptr;
	while( sock) {
		if( sock->socket == socketNetOrder) {
			return((ipxSocket_t *)NTR_LEAVE(sock));
		}
		sock = sock->qlink;
	}

	return((ipxSocket_t *)NTR_LEAVE(NULL));
}

/*
 * ipxSocket_t * IpxHashFindSocket( uint16 socketNetOrder)
 *	If a socket is bound, return its socket structure pointer, otherwise NULL.
 *	Socket is located via the hash table, and must be specified in net order.
 *
 * Calling/Exit State:
 *	Entry with HASH_LOCK lock not set, exits with HASH_LOCK set.
 *	If socket not found, exits with no lock set.
 *	Other locks may be held across calls to this function.
 */
FSTATIC ipxSocket_t *
IpxHashFindSocket( uint16 socketNetOrder, pl_t *lvl)
{
	ipxSocket_t *sock;
	int	hash;

	NTR_ENTER(2, socketNetOrder, *lvl, 0, 0, 0);

	if( socketNetOrder == 0) {
		return( (ipxSocket_t *)NTR_LEAVE(NULL));
	}

	hash = SOCKETHASH( &socketNetOrder);
	*lvl = LOCK( SocketHashTable[hash].hlock, plstr);
	sock = SocketHashTable[ hash].hlink;

	while( sock != NULL) {
		if( sock->socket == socketNetOrder) {
			return((ipxSocket_t *)NTR_LEAVE(sock));
		}
		sock = sock->hlink;
	}
	UNLOCK( SocketHashTable[hash].hlock, *lvl);
	return((ipxSocket_t *)NTR_LEAVE(NULL));
}


/*
 * uint16 IpxFindFirstSocket( queue_t *rdq)
 *	Return the first socket number bound to the given queue
 *
 * Calling/Exit State:
 *	Entry and exit with QUEUE_LOCK set.
 *	Other locks may be held across calls to this function.
 */
FSTATIC uint16
IpxFindFirstSocket( queue_t *rdq)
{
	ipxSocket_t	*qsock;

	NTR_ENTER(1, rdq, 0, 0, 0, 0);
	/*
	 *	Find the first socket number bound
	 */
	qsock = (ipxSocket_t *)WR(rdq)->q_ptr;
	while( qsock) {
		if( qsock->socket != 0) {
			return(NTR_LEAVE(qsock->socket));
		}
		qsock = qsock->qlink;
	}
	return(NTR_LEAVE(0));
}

/*
 * uint16 IpxAllocateSocket( uint16 socketNetOrder, queue_t *rdq, long *tpiStatus, ipxSocket_t **sockbuf, int tli_Flag)
 *	Create a socket structure for the specified socket
 *	Use permanent socket struct if unused and remove from ipxDeviceList
 *		otherwise request memory from the kernel.
 *	Link into appropriate hash index
 *	Link into queue list	
 *	Called from IpxTpiBindReq, and IpxIocBindSocket.
 *
 * Calling/Exit State:
 *	DEVICE_LOCK set on entry and exit.
 *	Acquires/Releases QUEUE_LOCK and HASH_LOCK.
 */
FSTATIC uint16
IpxAllocateSocket(
	uint16	 socketNetOrder,	/* requested socket number */
	queue_t	 *rdq,				/* read queue */
	long	 *tpiStatus,		/* Return tpi status */
	ipxSocket_t	 **sockbuf,		/* pre-allocated socket structure */
	int		  tli_flag)			/* Set if tli_bind request */
{
	int     i;
    static  uint16	socketMachOrder = IPX_MIN_EPHEMERAL_SOCKET;
    uint16	sMachOrder;
	uint16			idx;
	ipxSocket_t		*sock, *hsock, *qsock;
	ipxPDS_t		*pds;
	pl_t			qlvl, hlvl;

    NTR_ENTER(4, socketNetOrder,rdq,tpiStatus,tli_flag,0);

	qsock = (ipxSocket_t *)(WR(rdq)->q_ptr);
	qlvl = LOCK( qsock->qlock, plstr);

	if( socketNetOrder != 0) {
		sMachOrder = GETINT16(socketNetOrder);
		if( (sMachOrder < IPX_MIN_EPHEMERAL_SOCKET)
				|| (sMachOrder > IPX_MAX_EPHEMERAL_SOCKET)) {
			if( !IsPriv( WR(rdq))) {
				/* Non ephemeral socket, user not privilged, fail*/
				UNLOCK( qsock->qlock, qlvl);
				NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
					"IpxAllocateSocket: non ephemeral socket 0x%X, not root",
						sMachOrder));
				*tpiStatus = TACCES;
				return( NTR_LEAVE(0));
			}
		}
		if( (sock = IpxHashFindSocket( socketNetOrder, &hlvl)) != NULL) {
			if( sock->qptr == rdq) {
				/*	Socket already allocated, by same stream */
				if( tli_flag) {
					*tpiStatus = -TNOADDR;
				} else {
					*tpiStatus = -TADDRBUSY;
				}
				sock->ref++;
				NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
					"IpxAllocateSocket: Socket 0x%X already owned by this q",
					sMachOrder));
				UNLOCK( SocketHashTable[SOCKETHASH(&sock->socket)].hlock, hlvl);
				UNLOCK( qsock->qlock, qlvl);
				return( NTR_LEAVE(socketNetOrder));
			} else {
				/*	Socket already allocated some other stream, fail the call */
				if( tli_flag) {
					*tpiStatus = TNOADDR;
				} else {
					*tpiStatus = TADDRBUSY;
				}
				NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
					"IpxAllocateSocket: Socket 0x%X already allocated",
					sMachOrder));
				UNLOCK( SocketHashTable[SOCKETHASH(&sock->socket)].hlock, hlvl);
				UNLOCK( qsock->qlock, qlvl);
				return( NTR_LEAVE(0));
			}

		}
	} else {
		for( i=0; i < IPX_EPHEMERAL_SOCKET_RANGE; i++) {
			if (socketMachOrder > IPX_MAX_EPHEMERAL_SOCKET)
				socketMachOrder = IPX_MIN_EPHEMERAL_SOCKET;
			socketNetOrder = GETINT16(socketMachOrder);
			if( (sock = IpxHashFindSocket(socketNetOrder, &hlvl)) == NULL) {
				/* Found an unused socket */
				NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
					"IpxAllocateSocket: Allocating socket 0x%X",
						GETINT16(socketNetOrder)));
				socketMachOrder++;	/* Prime socket number for next call */
				break;
			}
			UNLOCK( SocketHashTable[SOCKETHASH(&sock->socket)].hlock, hlvl);
			socketMachOrder++;
		}
		if( i >= IPX_EPHEMERAL_SOCKET_RANGE) {
			UNLOCK( qsock->qlock, qlvl);
				NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
					"IpxAllocateSocket: Ephemeral sockets exhausted"));
				/*
				 *+ We have run out of sockets
				 */
				cmn_err(CE_WARN,
					"IpxAllocateSocket: Ephemeral sockets exhausted");
			*tpiStatus = TNOADDR;
			/*	No ephemeral sockets in available */
			return( NTR_LEAVE(0));
		}
	}


	pds = (ipxPDS_t *)&(rdq->q_ptr);
	if( (sock = IpxQueueFindSocket( rdq, 0)) != NULL) {
		/*
		 *	We found an unused socket struct we can use to bind this socket
		 *	If it is the permanent socket structure,
		 *	fill in RD->q_ptr->socket with updated socket value
		 */
		if( pds->socket == 0) {
			IpxRemoveFromDeviceList( sock);
		}
		sock->socket = socketNetOrder;
		pds->socket = IpxFindFirstSocket( rdq);
	} else {
		/*
		 *	Use allocated  socket struct to bind this socket
		 *	It is linked as second in list.  We don't need to update
		 *	pds->socket because we only get here if the first socket
		 *	struct is already bound to a socket, and pds->socket has that num
		 */

        sock = *sockbuf;
        *sockbuf = NULL;	/* Tell caller we used this structure */

		/*
		 *	Link struct as 2nd entry into list of sockets bound to this queue
		 */
		sock->socket = socketNetOrder;
		sock->qlink = qsock->qlink;
		qsock->qlink = sock;
		sock->device = 0;
		ATOMIC_INT_INIT( &sock->qcount, 0);
	}

	/*
	 *	Socket structure allocated, fill it in
	 */
	sock->qptr = rdq;
	sock->hlink = NULL;
	sock->ref = 1;

	/*
	 *	Link structure into end of hash table list for this socket
	 */
	idx = SOCKETHASH( &socketNetOrder);
	hlvl = LOCK( SocketHashTable[idx].hlock, plstr);
	if( SocketHashTable[idx].hlink == NULL) {
		SocketHashTable[idx].hlink = sock; 
	} else {
		hsock = SocketHashTable[idx].hlink;
		while( hsock->hlink) {
			hsock = hsock->hlink;
		}
		hsock->hlink = sock;
	}
	UNLOCK( SocketHashTable[idx].hlock, hlvl);
	UNLOCK( qsock->qlock, qlvl);

	*tpiStatus = 0;
	return( NTR_LEAVE(socketNetOrder));
}

/*
 * void IpxDeallocateSocket( ipxSocket_t *sock)
 *	Remove socket struct from hash and queue list
 *	if is the permanent socket structure, and the last socket unbound,
 *	return to the FreeDevice list, otherwise to kmemfree.
 *
 * Calling/Exit State:
 *	DEVICE_LOCK set on entry and exit.
 *	Acquires/Releases QUEUE_LOCK and HASH_LOCK.
 */
FSTATIC void
IpxDeallocateSocket( ipxSocket_t *sock)
{
	uint16		hidx;
	ipxPDS_t	*pds;
	pl_t		hlvl, qlvl;
	ipxSocket_t *tsock, *qsock;
	queue_t		*rdq;

	NTR_ENTER(1, sock, 0, 0, 0, 0);

	hidx = SOCKETHASH( &sock->socket);
	rdq = sock->qptr;

	/*	Socket struct not allocated, error */
	ASSERT( sock->socket != 0);

	/* Socket struct not in hash table, cannot deallocate */
	ASSERT( SocketHashTable[hidx].hlink != NULL);

	hlvl = LOCK( SocketHashTable[hidx].hlock, plstr);
#ifndef NW_UP
	while( ATOMIC_INT_READ( &sock->qcount)) {
		drv_usecwait(10);
	}
#endif

	/*
	**	Don't deallocate socket structure until reference count is zero
	*/
	if( --(sock->ref) != 0) {
		UNLOCK( SocketHashTable[hidx].hlock, hlvl);
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxDeallocateSocket:Socket struct reference count %d", sock->ref));
		NTR_VLEAVE();
		return;
	}

	/*
	 *	Remove from hash list
	 */
	if( SocketHashTable[hidx].hlink == sock) {
		SocketHashTable[hidx].hlink = sock->hlink;
	} else {
		tsock = SocketHashTable[hidx].hlink;
		while( tsock->hlink != NULL) {
			if( tsock->hlink == sock) {
				/*	Found it, unlink */
				tsock->hlink = sock->hlink;
				tsock = NULL;
				break;
			}
			tsock = tsock->hlink;
		}
		/* Socket struct for not in hash list, cannot deallocate */
		ASSERT( tsock == NULL);

	}
	UNLOCK( SocketHashTable[hidx].hlock, hlvl);

	/*
     *  If this is the permanent socket struct for this queue, don't free
     *  just zero the socket number.
	 *
     *  The device number in the permanent socket structure is always
     *  non zero, and is zero in all other socket structures.
	 */
	pds = (ipxPDS_t *)&(sock->qptr->q_ptr);
	qsock = (ipxSocket_t *)WR(sock->qptr)->q_ptr;

	qlvl = LOCK( qsock->qlock, plstr);

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxDeallocateSocket: sock @ 0x%x, tsock @  0x%X",
		sock, tsock));

	tsock = qsock;
	if( qsock == sock) {
		/*
		 *	This is the permanent socket struct, just zero socket num
		 */
		NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
			"IpxDeallocateSocket: Permanent structure 0x%x, socket 0x%X unbound",
			sock, GETINT16(sock->socket)));
		sock->socket = 0;
	} else {
		while( tsock->qlink != NULL) {
			if( tsock->qlink == sock) {
#ifdef DEBUG
				tsock = NULL;
#endif
				break;
			}
			tsock = tsock->qlink;
		}
		/* Socket struct not in queue list */
		ASSERT( tsock == 0);

        /*
        **  We can only do kmem_free on AIX if in user context, and since SPX
        **  can do an unbind request from a packet on the wire, we just
        **  mark unused here.  We do if for everyone do avoid extra ifdefs
        */
        NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
            "IpxDeallocateSocket:  0x%X Socket 0x%X socket cleared",
            sock, GETINT16(sock->socket)));
        sock->socket = 0;
	}

	/*
	 *	Update socket number in pds->socket
	 */
	if( (pds->socket = IpxFindFirstSocket( rdq)) == 0) {
		/*
		 *	No more sockets bound, put back into device list
		 */
		ipxAddToDeviceList( (ipxSocket_t *)(WR(rdq)->q_ptr));
	}
	UNLOCK( qsock->qlock, qlvl);

	NTR_VLEAVE();
	return;
}

/*
 * int ipxopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
/*ARGSUSED*/
int
ipxopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp)
{	dev_t		 maj;
	ipxPDS_t 	*pds;
	uint8		 priv;
	dev_t   	 dev;
	ipxSocket_t *sock;
	pl_t		 lvl;

	NTR_ENTER(5, q, devp, flag, sflag, credp);
	maj = getmajor(*devp);

	NWSLOG((IPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter ipxopen, Major device = 0x%X", maj));

	priv = 0;		/* Assume not privileged */
	if(drv_priv(credp) == 0)
		priv = PRIV_USER;	/* Set privileged */


	if( (sflag != CLONEOPEN) && sflag) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"ipxopen: not clone open sflag = 0x%X",sflag));
		return(NTR_LEAVE(ENXIO));
	}

	/*
	 *	Allocate permanent socket structure for this queue
	 */
	if( (sock =
			(ipxSocket_t *)kmem_alloc( sizeof(ipxSocket_t),KM_SLEEP)) == NULL) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"ipxopen: unable to allocate socket structure"));
		return(NTR_LEAVE(EPERM));
	}

	NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
		"ipxopen: KMEM: Allocated socket structure at 0x%X, size %d", 
			sock, sizeof(ipxSocket_t)));
	
	lvl = LOCK( ipxDeviceLock, plstr);

	if( (dev = IpxAllocateDevice()) == 0xFFFF) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"ipxopen: all devices in use"));
		UNLOCK( ipxDeviceLock, lvl);
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"ipxopen: KMEM: Free socket structure at 0x%X, size %d", 
				sock, sizeof(ipxSocket_t)));
		kmem_free( sock, sizeof(ipxSocket_t));
		return(NTR_LEAVE(EAGAIN));
	}

	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"ipxopen: Open device = %d",dev));

	if( sflag == CLONEOPEN) {
		/* This is a clone device */
		if( ipxInitialized == 0) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"ipxopen: clone device open not allowed"));
			IpxDeallocateDevice(dev);
			UNLOCK( ipxDeviceLock, lvl);
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"ipxopen: KMEM: Free socket structure at 0x%X, size %d", 
					sock, sizeof(ipxSocket_t)));
			kmem_free( sock, sizeof(ipxSocket_t));
			return(NTR_LEAVE(EAGAIN));
		}
	} else {
		/*
		 *	Set up 0 device
		 */
		if( dev != 0) {
			NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
				"ipxopen: multiple opens on ipx0"));
			IpxDeallocateDevice(dev);
			UNLOCK( ipxDeviceLock, lvl);
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"ipxopen: KMEM: Free socket structure at 0x%X, size %d", 
					sock, sizeof(ipxSocket_t)));
			kmem_free( sock, sizeof(ipxSocket_t));
			return(NTR_LEAVE(EAGAIN));
		}
		if( ClosesPending != 0) {
			NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
				"ipxopen: %d devices still open, don't allow device0 to open",
					ClosesPending));
			IpxDeallocateDevice(dev);
			UNLOCK( ipxDeviceLock, lvl);
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"ipxopen: KMEM: Free socket structure at 0x%X, size %d", 
					sock, sizeof(ipxSocket_t)));
			kmem_free( sock, sizeof(ipxSocket_t));
			return(NTR_LEAVE(EAGAIN));
		}
		if(!priv) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"ipxopen: non-root attempt to open control dev"));
			IpxDeallocateDevice(dev);
			UNLOCK( ipxDeviceLock, lvl);
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"ipxopen: KMEM: Free socket structure at 0x%X, size %d", 
					sock, sizeof(ipxSocket_t)));
			kmem_free( sock, sizeof(ipxSocket_t));
			return(NTR_LEAVE(EPERM));
		}
	}

	ClosesPending++;

	/*
	 *	Initialize private data
	 *		Read q_ptr has pds structure
	 *		Write q_ptr has pointer to permanent socket structure
	 */
	sock->qptr = q;
	sock->socket = 0;

	/*
	 *	Device must be non zero in permanent socket structure for this queue
	 */
	if( dev == 0) {
		sock->device = 0xFFFF;
	} else {
		sock->device = (uint16)dev;
	}
	sock->qlink = NULL;

	pds = (ipxPDS_t *)&(q->q_ptr);
	pds->priv = priv;
	pds->pad = 0;
	pds->socket = 0;
	WR(q)->q_ptr = (char *)sock;

	sock->qlock = LOCK_ALLOC( QUEUE_LOCK, plstr, &ipxQueueLkinfo, KM_NOSLEEP);
	if( sock->qlock == NULL) {
		ClosesPending--;
		NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
			"ipxopen: failure to obtain lock structure for queue"));
		IpxDeallocateDevice(dev);
		UNLOCK( ipxDeviceLock, lvl);
		return(NTR_LEAVE(EAGAIN));
	}
	ATOMIC_INT_INIT( &sock->qcount, 0);
	ipxAddToDeviceList( sock);
	UNLOCK( ipxDeviceLock, lvl);
	qprocson( q);

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"ipxopen: pds @ 0x%X, priv 0x%X, socket 0x%X",
		pds, pds->priv, GETINT16(pds->socket)));
	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"ipxopen: open device 0x%X RD 0x%X, WR 0x%X",
		(uint32)(WR(q)->q_ptr), q, WR(q)));

	NWSLOG((IPXID,0,PNW_EXIT_ROUTINE,SL_TRACE, "Exit ipxopen, open devices %d",
		ClosesPending));
	*devp = makedevice(maj, dev);
	return(NTR_LEAVE(0));
}

/*
 * int ipxuwput(queue_t *q, mblk_t *mp)
 * The Upper Write Put procedure is responsible for handling all
 * IOCTL  and M_CTL messages coming downstream.  If not  recognized,
 * the message is forwarded to lipmx by calling "lipmxuwput"
 *
 * Outgoing datagrams are also handled here:
 *
 * If the TPI interface is used, the messages are M_PROTO or M_PCPROTO types.
 * A socket must be bound for the message to be forwarded.
 *
 * If the getmsg/putmsg interface is used, the message comes
 * downstream with only an M_DATA message (no control portion) and
 * must include the IPX HEADER information.
 * A socket must be bound for the message to be forwarded.
 *
 * The upper put procedure forwards the datagram to LIPMX for
 * routing and dispatch.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
int
ipxuwput(queue_t *q, mblk_t *mp)
{
register	struct iocblk 	*iocp;
	int		*cmdp;
	int		 tli_flag = 0;	/* Set if NON XTI BIND req, affects error codes */

	NTR_ENTER(2, q, mp, 0,0,0);

	NWSLOG((IPXID,0,PNW_ENTER_ROUTINE,SL_TRACE, "Enter ipxuwput"));

	switch(MTYPE(mp)) {
	case M_DATA:
		ipxStats.IpxOutData++;
		NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"ipxuwput: M_DATA size %d",DATA_SIZE(mp)));
		if(DATA_SIZE(mp) < IPX_HDR_SIZE) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
					"ipxuwput: Data < IPX_HEADER_SIZE 0x%X, dropped",
					DATA_SIZE(mp)));
			ipxStats.IpxOutBadSize++;
			freemsg(mp);
			return(NTR_LEAVE(-1));
		}
		ipxStats.IpxOutToSwitch++;
		IpxPropagatePkt(q, mp);
		break;

	case M_PROTO:
		NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"ipxuwput: M_PROTO size %d",DATA_SIZE(mp)));

		if (DATA_SIZE(mp) < sizeof(long)) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"ipxuwput: M_PROTO bad data size 0x%X, dropped",DATA_SIZE(mp)));
			freemsg(mp);
			break;
		}

		/*
		 *	Get Primitive TYPE, from user data
		 */
		cmdp = (int *)mp->b_rptr;
		switch( (int)*cmdp) {
			case T_UNITDATA_REQ :
				ipxStats.IpxTLIOutData++;
				IpxTpiUDataReq(q,mp);
				break;
			case O_T_BIND_REQ :		/* Non XTI Bind Request */
				tli_flag = 1;
				/*FALLTHRU*/
			case T_BIND_REQ :		/* XTI Bind Request */
				ipxStats.IpxTLIBind++;
				IpxTpiBindReq(q,mp, tli_flag);
				break;
			case T_OPTMGMT_REQ :
				ipxStats.IpxTLIOptMgt++;
				IpxTpiOptReq(q,mp);
				break;
			case T_UNBIND_REQ :
				freemsg(mp);
				IpxTpiUnbindReq(q);
				break;
#ifndef NW_TLI
			case T_ADDR_REQ:
				IpxTpiTAddrReq(q, mp);
				break;
#endif
			default:
				ipxStats.IpxTLIUnknown++;
				NWSLOG((IPXID,0,PNW_SWITCH_DEFAULT,SL_TRACE,
					"ipxuwput: M_PROTO unknown tpi type %d, dropped",*cmdp));
				freemsg(mp);
				break;
		}
		break;

	case M_IOCTL:
		NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"ipxuwput: M_IOCTL size %d ",DATA_SIZE(mp)));
		iocp = (struct iocblk *)mp->b_rptr;
		switch (iocp->ioc_cmd) {
			case IPX_SET_WATER:
				ipxStats.IpxIoctlSetWater++;
				IpxIocSetWater(q,mp);
				break;
			case IPX_SET_SOCKET:
				ipxStats.IpxIoctlBindSocket++;
				IpxIocBindSocket(q,mp,SET_SOCKET, 0);
				break;
			case IPX_BIND_SOCKET:
				ipxStats.IpxIoctlBindSocket++;
				IpxIocBindSocket(q,mp,BIND_SOCKET, 0);
				break;
			case IPX_UNBIND_SOCKET:
				ipxStats.IpxIoctlUnbindSocket++;
				IpxIocUnbindSocket(q,mp);
				break;
			case IPX_INITIALIZE:
				IpxIocInitialize(q,mp);
				break;
			case IPX_STATS:
				ipxStats.IpxIoctlStats++;
				IpxIocStats(q,mp);
				break;
			default:
				ipxStats.IpxIoctlUnknown++;
				NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
					"ipxuwput: M_IOCTL sending IOCTL cmd 0x%X to lipmx",
					iocp->ioc_cmd));
				lipmxuwput(q, mp);
				break;
		}
		break;

	case M_CTL:
	{
		NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"ipxuwput: M_CTL size %d",DATA_SIZE(mp)));
		if(DATA_SIZE(mp) < sizeof(int)) {
			NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
					"ipxuwput: M_CTL bad size, dropped"));
			freemsg(mp);
			break;
		}

		cmdp = (int *)mp->b_rptr;
		switch ( (int)*cmdp) {
			case IPX_O_BIND_SOCKET:	/* Non XTI Bind Socket from SPX */
				tli_flag = 1;
				/*FALLTHRU*/
			case IPX_BIND_SOCKET:		/* XTI Bind Socket from SPX */
				NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
					"ipxuwput: M_CTL IPX_BIND_SOCKET"));
				ipxStats.IpxIoctlBindSocket++;
				IpxIocBindSocket(q, mp, KNL_SOCKET, tli_flag);
				break;

			default:
				NWSLOG((IPXID,0,PNW_SWITCH_DEFAULT,SL_TRACE,
					"ipxuwput: M_CTL unknown type 0x%X, send to lipmx",*cmdp));
				lipmxuwput(q, mp);
				break;
		}
		break;
	}

	case M_FLUSH:
		NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"ipxuwput: M_FLUSH size %d",DATA_SIZE(mp)));
		IpxUpperFlush(q,mp);
		break;

	case M_PCPROTO:
		NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"ipxuwput: M_PCPROTO size %d",DATA_SIZE(mp)));

		if (DATA_SIZE(mp) < sizeof(long)) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"ipxuwput: M_PCPROTO bad data size 0x%X, dropped",
				DATA_SIZE(mp)));
			freemsg(mp);
			break;
		}
		/*
		 *	Get Primitive TYPE, from user data
		 */
		cmdp = (int *)mp->b_rptr;
		switch( (int)*cmdp) {
			case T_INFO_REQ:
				IpxTpiInfoReq(q,mp);
				break;
			default:
				NWSLOG((IPXID,0,PNW_SWITCH_DEFAULT,SL_TRACE,
					"ipxuwput: Dropping M_PCPROTO tpi type 0x%X",*cmdp));
				freemsg(mp);
				break;
		}
		break;

	default:
		NWSLOG((IPXID,0,PNW_DROP_PACKET,SL_TRACE,
				"ipxuwput: Unknown MTYPE 0x%X, send to lipmx",MTYPE(mp)));
		lipmxuwput(q, mp);
		break;

	}
	NWSLOG((IPXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
		"Exit ipx upper write put procedure"));

	return(NTR_LEAVE(0));
}

/*
 * int ipxclose(queue_t *q, int flag, cred_t *credp)
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
/*ARGSUSED*/
int
ipxclose(queue_t *q, int flag, cred_t *credp)
{
	ipxPDS_t	*pds;
	dev_t		 dev;
	ipxSocket_t	*tsock, *sock, *qsock;
	pl_t		 lvl;

	NTR_ENTER(3, q, flag, credp,0,0);

	NWSLOG((IPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"ipxclose: ENTER q 0x%X flags %d ",q, flag));

	qprocsoff( q);

	lvl = LOCK( ipxDeviceLock, plstr);

	pds = (ipxPDS_t *)&(q->q_ptr);
	qsock = (ipxSocket_t *)WR(q)->q_ptr;
	dev = qsock->device;
	/*
	 *	Set device number for controlling device
	 */
	if( dev == 0xFFFF)
		dev = 0;

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"ipxclose: pds @ 0x%X, priv 0x%X, sock 0x%X",
		pds, pds->priv, GETINT16(pds->socket)));

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"ipxclose: closing device 0x%X, queue 0x%X", dev, q));

	/*
	 *	Release all bound sockets
	 */
	while( pds->socket ) {
		NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
			"ipxclose: unbinding socket 0x%X", GETINT16(pds->socket)));
		sock = IpxQueueFindSocket( q, pds->socket);
		/* make sure we find struct for socket */
		ASSERT( sock != NULL);

		if( sock->socket == RIPsocket) {
			LipmxRouteRIP( DISABLE_RIP);	/* Tell lipmx stop routing RIP */
		}
		IpxDeallocateSocket( sock);
		ipxStats.IpxBoundSockets--;
	}

	LOCK_DEALLOC( qsock->qlock);
	/*
	 *	Remove permanent socket struct from unbound device list
	 */
	IpxRemoveFromDeviceList( qsock);
	ClosesPending--;

	/*
	 *	Release device number
	 */
	IpxDeallocateDevice( dev);

	if( ClosesPending == 0) {
		lipmxCleanUp();	/* Tell lipmx to clean up structures */
	}

	if( dev == 0) {
		/*
		 *	If controlling device close, hangup all other devices
		 *	Allow now more opens except control device
		 */
		NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
			"ipxclose: closing control device"));
		ipxInitialized = 0;
		IpxSendHangup(lvl); 	/* Hangup unlocks Device Lock */
	} else {
		UNLOCK( ipxDeviceLock, lvl);
	}

    /*
     *  Since we can only free in user context (ioctl/open/close) on AIX,
     *  and since unbind requests can come from the wire to spx, we don't
     *  free on unbind, therefore, free those buffers here
     */
    sock = qsock->qlink;
    while( sock) {
        tsock = sock;
        sock = sock->qlink;
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"ipxclose: KMEM: Free socket structure at 0x%X, size %d", 
				tsock, sizeof(ipxSocket_t)));
        kmem_free( tsock, sizeof(ipxSocket_t));
    }

	/*
	 *	Free the permanent socket structure
	 */
	NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
		"ipxclose: KMEM: Free permanent socket structure at 0x%X, size %d", 
			qsock, sizeof(ipxSocket_t)));
	kmem_free( qsock, sizeof(ipxSocket_t));

	NWSLOG((IPXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
		"ipxclose: EXIT q 0x%X flags %d, open devices %d",
			q, flag, ClosesPending));

	return(NTR_LEAVE(0));
}

/*
 * void IpxRouteDataToSocket(mblk_t *mp)
 *	Send data received from lipmx to an upper queue based on IPX socket number.
 *	This is the primary code path.
 *	Called only by lipmx.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 *	Acquires the HASH_LOCK
 */
void
IpxRouteDataToSocket(mblk_t *mp)
{	uint16		destSock;
	uint16		chksum;
	queue_t		*urdq;		/* upper queue of destSock */
	ipxHdr_t	*ipxHeader;
	ipxSocket_t	*sock;
	ipxPDS_t	*pds;
	pl_t		 lvl;
	int			 hash;

	NTR_ENTER(1, mp, 0,0,0,0);

	NWSLOG((IPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"Enter IpxRouteDataToSocket"));

	ipxStats.IpxInData++;

	/*
	 * Route this packet to a socket
	 *
	 * A message that enters this routine from Lipmx has had a
	 * Link Layer Header in the first message block (of type
	 * M_PROTO) stripped by the lipmx and then the Data in
	 * the message block passed on up to us as an M_DATA.
	 * Size of mp is guarenteed at least IPX_HDR_SIZE
	 */
	ipxHeader = (ipxHdr_t *)mp->b_rptr;

	IPXCOPYSOCK(ipxHeader->dest.sock,&destSock);

    /* discover urdq associated with destination socket */

	/*
	 *	This is the similar to IpxHashFindSocket, but done inline
	 *	to avoid function call overhead
	 */
	hash = SOCKETHASH( &destSock);
	lvl = LOCK( SocketHashTable[ hash].hlock, plstr);
	sock = SocketHashTable[hash].hlink;
    while( sock) {
        if( sock->socket == destSock)
            break;
		sock = sock->hlink;
    }

    if( sock == 0) {
		UNLOCK( SocketHashTable[hash].hlock, lvl);
        ipxStats.IpxSocketNotBound++;
        NWSLOG((IPXID,0,PNW_DROP_PACKET,SL_TRACE,
			"IpxRouteDataToSocket: no urdq for socket 0x%X, dropped",
			GETINT16(destSock)));
        freemsg(mp);
        NTR_VLEAVE();
        return;
    }
	urdq = sock->qptr;
	ATOMIC_INT_INCR( &sock->qcount);
	UNLOCK( SocketHashTable[hash].hlock, lvl);

	NWSLOG((IPXID,0,PNW_ALL,SL_TRACE,
		"IpxRouteDataToSocket: Routing to socket 0x%X, q 0x%X",
		GETINT16(destSock), urdq));

	if( canputnext(urdq)) {
		
		chksum = (uint16)(PGETINT16(&(ipxHeader->chksum)));
		if( chksum != IPX_CHKSUM) {
			NWSLOG((IPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"IpxRouteDataToSocket: verify checksum 0x%X", chksum));
			if( (mp = IpxSumPacket(mp, CHKSUM_VERIFY)) == NULL) {
				ATOMIC_INT_DECR( &sock->qcount);
				ipxStats.IpxSumFail++;
				NWSLOG((IPXID,0,PNW_DROP_PACKET,SL_TRACE,
					"IpxRouteDataToSocket: IpxSumPacket() failed, dropped"));
				NTR_VLEAVE();
				return;
			}
		}

		pds = (ipxPDS_t *)&(urdq->q_ptr);

		NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
			"IpxRouteDataToSocket: pds @ 0x%X, sock 0x%X, priv 0x%X",
			pds, GETINT16(pds->socket), pds->priv));

		if( pds->priv & TLI_SOCKET) {
			IpxTpiUDataInd(urdq,mp);
			ATOMIC_INT_DECR( &sock->qcount);
			ipxStats.IpxDataToSocket++;
		} else {
			ipxStats.IpxRouted++;
			NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
				"IpxRouteDataToSocket: not TPI, doing putnext()"));
			putnext(urdq,mp);
			ATOMIC_INT_DECR( &sock->qcount);
			ipxStats.IpxDataToSocket++;
		}
	} else {
		NWSLOG((IPXID,0,PNW_DROP_PACKET,SL_TRACE,
				"IpxRouteDataToSocket: can't canput, dropped"));
		ipxStats.IpxBusySocket++;
		ATOMIC_INT_DECR( &sock->qcount);
		freemsg(mp);
	}
	NTR_VLEAVE();
	return;
}

/*
 * void IpxTpiErrorAck(queue_t *q, long prim, long tpiError, long unixError)
 *	Send error acknowledgment response to TPI requests.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 */
FSTATIC void
IpxTpiErrorAck(queue_t *q, long prim, long tpiError, long unixError)
{
	mblk_t *mp;
	struct T_error_ack *tErrorAck;

	NTR_ENTER(4, q, prim, tpiError, unixError,0);

	NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
		"IpxTpiErrorAck Error prim %d, tpi 0%d unix 0%d",
		prim, tpiError, unixError));

	if ((mp=allocb(sizeof(struct T_error_ack),BPRI_HI)) == NULL) {
			NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
				"IpxTpiErrorAck: couldnt alloc error"));
			NTR_VLEAVE();
			return;
	}

	tErrorAck = (struct T_error_ack *)mp->b_rptr;
	tErrorAck->PRIM_type = T_ERROR_ACK;
	tErrorAck->ERROR_prim = prim;
	tErrorAck->TLI_error = tpiError;
	tErrorAck->UNIX_error = unixError;
	MTYPE(mp) = M_PCPROTO;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_error_ack);
	qreply(q,mp);

	NTR_VLEAVE();
	return;
}

/*
 * void IpxTpiInfoReq(queue_t *q, mblk_t *mp)
 *	Respond to a TLI info request.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 */
FSTATIC void
IpxTpiInfoReq(queue_t *q, mblk_t *mp)
{	struct T_info_ack *tpiPrimPtr;
	ipxPDS_t *pds;
	mblk_t *nmp;

	NTR_ENTER(2, q, mp, 0,0,0);

	pds = (ipxPDS_t *)&(RD(q)->q_ptr);

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxTpiInfoReq: pds @ 0x%X, sock 0x%X, priv 0x%X",
		pds, GETINT16(pds->socket), pds->priv));

	freemsg(mp);
		
	NWSLOG((IPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"IpxTpiInfoReq: ENTER TInfoReq q 0x%X mp 0x%X", q, mp));

	/*
	 * since the message T_info_req is not big enough to reply with we
	 *	need to allocate memory
	 */

	if ((nmp = allocb(sizeof(struct T_info_ack),BPRI_MED)) == NULL) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_ERROR,
			"IpxTpiInfoReq: TInfoReq alloc >64"));

		IpxTpiErrorAck( q, T_INFO_REQ, TSYSERR, ENOSR);

		NTR_VLEAVE();
		return;
	}

	tpiPrimPtr 	= (struct T_info_ack *)nmp->b_rptr;

	tpiPrimPtr->PRIM_type 	= T_INFO_ACK;
	tpiPrimPtr->TSDU_size 	= ipxMinMaxSDU;
	tpiPrimPtr->ETSDU_size 	= IPX_ETSDU_SIZE;
	tpiPrimPtr->CDATA_size 	= IPX_CDATA_SIZE;
	tpiPrimPtr->DDATA_size 	= IPX_DDATA_SIZE;
	tpiPrimPtr->ADDR_size 	= sizeof(ipxAddr_t);
	tpiPrimPtr->OPT_size 	= IPX_OPT_SIZE;
	tpiPrimPtr->TIDU_size 	= ipxMinMaxSDU;
	tpiPrimPtr->SERV_type 	= T_CLTS;
#ifndef OS_AIX
	tpiPrimPtr->PROVIDER_flag = XPG4_1 | SNDZERO;
#endif
	if( pds->socket == 0) {
		tpiPrimPtr->CURRENT_state = TS_UNBND;
	} else {
		tpiPrimPtr->CURRENT_state = TS_IDLE;
	}

	MTYPE(nmp)=M_PCPROTO;
	nmp->b_wptr=nmp->b_rptr+sizeof(struct T_info_ack);

	qreply(q,nmp);

	NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
	            "IpxTpiInfoReq: EXIT"));

	NTR_VLEAVE();
	return;
}

#ifndef NW_TLI
/*
 * void IpxTpiTAddrReq(queue_t *q, mblk_t *mp)
 *	Respond to TPI request for the end point's address information
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 */
FSTATIC void
IpxTpiTAddrReq(queue_t *q, mblk_t *mp)
{
	struct T_addr_ack *tpiPrimPtr;
	ipxPDS_t *pds;
	mblk_t *nmp;
	ipxAddr_t *ipxAddrPtr;
	int allocsize = sizeof(struct T_addr_ack);

	NTR_ENTER(2, q, mp, 0,0,0);

	pds = (ipxPDS_t *)&(RD(q)->q_ptr);

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxTpiTAddrReq: pds @ 0x%X, sock 0x%X, priv 0x%X",
		pds, GETINT16(pds->socket), pds->priv));

	freemsg(mp);
		
	NWSLOG((IPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"IpxTpiTAddrReq: ENTER TAddrReq q 0x%X mp 0x%X", q, mp));

	/* verify that end point is TLI/XTI based */
	if (!(pds->priv & TLI_SOCKET)) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_ERROR,
			"IpxTpiTAddrReq: TAddrReq end point not a TLI/XTI socket"));
		IpxTpiErrorAck( q, T_ADDR_REQ, TSYSERR, EPROTO);
		NTR_VLEAVE();
		return;
	}

	/*
	 * Since the message T_addr_req is not big enough to reply with we
	 * need to allocate memory for the T_addr_ack.  If end point is bound
	 * (i.e. its TPI state is TS_IDLE) also need to include size of a
	 * ipxAddr_t to return the address information of the local address
	 * Absence or presence of a socket number (i.e. whether or not
	 * pds->socket is zero) correlates to TPI states TS_UNBND and
	 * TS_IDLE respectively.
	 */
	if (pds->socket){
		allocsize += sizeof(ipxAddr_t);
	}
	if ((nmp = allocb(allocsize,BPRI_MED)) == NULL) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_ERROR,
			"IpxTpiTAddrReq: TAddrReq allocb of size=%d failed",
			 allocsize));
		IpxTpiErrorAck( q, T_ADDR_REQ, TSYSERR, ENOSR);
		NTR_VLEAVE();
		return;
	}

	tpiPrimPtr 	= (struct T_addr_ack *)nmp->b_rptr;

	tpiPrimPtr->PRIM_type 	= T_ADDR_ACK;
	/*
	 * Copy local address information if endpoint is bound (i.e. TPI
	 * state is TS_IDLE.
	 */
	if (pds->socket) {
		tpiPrimPtr->LOCADDR_length = sizeof(ipxAddr_t);
		tpiPrimPtr->LOCADDR_offset = sizeof(struct T_addr_ack);
		ipxAddrPtr =
		 (ipxAddr_t *)(nmp->b_rptr + tpiPrimPtr->LOCADDR_offset);
		IPXCOPYNET(&ipxInternalNet, ipxAddrPtr->net);
		IPXCOPYNODE(ipxInternalNode, ipxAddrPtr->node);
		IPXCOPYSOCK(&(pds->socket), ipxAddrPtr->sock);
	} else {
		tpiPrimPtr->LOCADDR_length = 0;
		tpiPrimPtr->LOCADDR_offset = 0;
	}
	/*
	 * Set remote address information to zero for connectionless
	 * endpoints.
	 */
	tpiPrimPtr->REMADDR_length = 0;
	tpiPrimPtr->REMADDR_offset = 0;

	MTYPE(nmp)=M_PCPROTO;
	nmp->b_wptr=nmp->b_rptr + allocsize;

	qreply(q,nmp);

	NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
	            "IpxTpiAddrReq: EXIT"));

	NTR_VLEAVE();
	return;
}
#endif /* NW_TLI */

/*
 * void IpxTpiBindReq(queue_t *q, mblk_t *mp)
 *	Respond to a TLI Bind Socket request.  A single bind to a queue is allowed.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 *	Acquires/Releases DEVICE_LOCK, QUEUE_LOCK, and HASH_LOCK.
 */
FSTATIC void
IpxTpiBindReq(queue_t *q, mblk_t *mp, int tli_flag)
{	struct	T_bind_req 	*tBindReq;
	struct 	T_bind_ack 	*tBindAck;
			ipxPDS_t	*pds;
			uint16 		socketNetOrder;
			uint8 		*socketPtr;
			long		status;
			pl_t		lvl;
			ipxSocket_t	*Asock;
			long		 prim = T_BIND_REQ;

	NTR_ENTER(3, q, mp, tli_flag,0,0);

	if( tli_flag) {
		prim = O_T_BIND_REQ;
	}

	tBindReq = (struct T_bind_req *)mp->b_rptr;
	pds = (ipxPDS_t *)&(RD(q)->q_ptr);

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxTpiBindReq: pds @ 0x%X, sock 0x%X, priv 0x%X",
		pds, GETINT16(pds->socket), pds->priv));

	NWSLOG((IPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"IpxTpiBindReq: ENTER device 0x%X", q->q_ptr));

	if (DATA_SIZE(mp) < sizeof(struct T_bind_req)) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiBindReq: bad msg data size 0x%X",DATA_SIZE(mp)));
		freemsg(mp);
		IpxTpiErrorAck( q, prim, TSYSERR, EBADMSG);
		NTR_VLEAVE();
		return;
	}

	if ( (tBindReq->ADDR_length > 0)
			&& (tBindReq->ADDR_length < sizeof(ipxAddr_t))) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiBindReq: short address size 0x%X", tBindReq->ADDR_length));
		freemsg(mp);
		IpxTpiErrorAck( q, prim, TBADADDR, 0);
		NTR_VLEAVE();
		return;
	}

	if (tBindReq->ADDR_length ==  0) {
		NWSLOG((IPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"IpxTpiBindReq: zero addr length - dynamic"));
		socketNetOrder = 0;
	} else {
		socketPtr = (uint8 *)(mp->b_rptr + tBindReq->ADDR_offset +
			IPX_NET_SIZE + IPX_NODE_SIZE);
		IPXCOPYSOCK(socketPtr,&socketNetOrder);
		NWSLOG((IPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"IpxTpiBindReq: requesting socket 0x%X", GETINT16(socketNetOrder)));
	}

	freemsg(mp);

	/*
	 * Absence or presence of a socket number (i.e. whether or not
	 * pds->socket is zero) correlates to TPI states TS_UNBND and
	 * TS_IDLE respectively.
	 */
	if ( pds->socket != 0) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiBindReq: stream out of state, socket already bound"));
		IpxTpiErrorAck( q, prim, TOUTSTATE, 0);
		NTR_VLEAVE();
		return;
	}

	/*
	 *	Don't allow bind to SAP or RIP
	 *	The app must set the net/node, and TLI cannot do that.
	 */
	if(	(socketNetOrder == SAPsocket) || (socketNetOrder == RIPsocket)) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiBindReq: Attempting to bind SAP or RIP socket"));
		IpxTpiErrorAck( q, prim, TBADADDR, 0);
		NTR_VLEAVE();
		return;
	}

	if( (mp = allocb(sizeof(struct T_bind_ack) + sizeof(ipxAddr_t),
			BPRI_MED)) == NULL) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiBindReq: cannot alloc bindack"));
		IpxTpiErrorAck( q, prim, TSYSERR, ENOSR);
		NTR_VLEAVE();
		return;
	}

    /*
     *  On AIX, we cannot acquire kernel memory if splstr, so get it now.
	 *	to avoid ifdef's we do it the same for everyone
	 */
	if( (Asock = (ipxSocket_t *)
			kmem_alloc( sizeof( ipxSocket_t), KM_NOSLEEP)) == NULL) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxTpiBindReq: Allocate of socket structure failed"));
		/*
		 *+ Cannot allocate socket structure
		 */
		cmn_err(CE_WARN,
			"IpxIpxTpiBindReq: Allocate of socket structure failed");
		NTR_VLEAVE();
		return;
	}
	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxTpiBindReq: KMEM: Pre Allocate of socket structure 0x%X, size %d",
		Asock, sizeof( ipxSocket_t)));

	lvl = LOCK( ipxDeviceLock, plstr);

	if( (socketNetOrder = IpxAllocateSocket(
			socketNetOrder, RD(q), &status, &Asock, tli_flag)) == 0) {
		UNLOCK( ipxDeviceLock, lvl);
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"ipxIpxTpiBindReq: KMEM: Free socket structure at 0x%X, size %d", 
				Asock, sizeof(ipxSocket_t)));
		kmem_free( Asock, sizeof( ipxSocket_t));
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiBindReq: AllocateSocket failed"));
		freemsg(mp);
		IpxTpiErrorAck( q, prim, status, 0);
		NTR_VLEAVE();
		return;
	}

	pds->priv |= TLI_SOCKET;

	ipxStats.IpxBoundSockets++;

	UNLOCK( ipxDeviceLock, lvl);

	/*
	 * If the socket structure wasn't used, release it
	 */
	if( Asock != NULL) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"ipxIpxTpiBindReq: KMEM: Free socket structure at 0x%X, size %d", 
				Asock, sizeof(ipxSocket_t)));
		kmem_free( Asock, sizeof( ipxSocket_t));
	}

	MTYPE(mp) = M_PCPROTO;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_bind_ack) + sizeof(ipxAddr_t);

	tBindAck = (struct T_bind_ack *)mp->b_rptr;
	tBindAck->PRIM_type = T_BIND_ACK;
	tBindAck->ADDR_length = sizeof(ipxAddr_t);
	tBindAck->ADDR_offset = sizeof(struct T_bind_ack);
	tBindAck->CONIND_number = 0;
	IPXCOPYNET(&ipxInternalNet, (mp->b_rptr+sizeof(struct T_bind_ack)));
	IPXCOPYNODE(ipxInternalNode,
		(mp->b_rptr+sizeof(struct T_bind_ack)+IPX_NET_SIZE));
	socketPtr = (uint8 *)( mp->b_rptr + sizeof(struct T_bind_ack)
				+ IPX_NET_SIZE + IPX_NODE_SIZE );
	IPXCOPYSOCK(&socketNetOrder,socketPtr);

	NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
		"IpxTpiBindReq: socket 0x%X allocated", GETINT16(socketNetOrder)));
	qreply(q,mp);

	NTR_VLEAVE();
	return;
} /* IpxTpiBindReq */

/*
 * void IpxTpiUDataReq(queue_t *q, mblk_t *mp)
 *	Send application data to lipmx.  Called from ipxuwput.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 */
FSTATIC void
IpxTpiUDataReq(queue_t *q, mblk_t *mp)
{	struct T_unitdata_req	*TUDataReq;
	struct T_uderror_ind	*TUDErrorInd;
	ipxPDS_t				*pds;
	ipxHdr_t				*ipxHdrPtr;
	ipxAddr_t				destAddress;
	long					tError;
	long					msize;
	uint8					ipxPacketType;
	mblk_t					*ErrorMp, *nmp;
	uint16					chksum = IPX_CHKSUM;

	NTR_ENTER(2, q, mp, 0,0,0);

	NWSLOG((IPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"Enter IpxTpiUDataReq"));

	pds = (ipxPDS_t *)&(RD(q)->q_ptr);

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxTpiUDataReq: pds @ 0x%X, sock 0x%X, priv 0x%X",
		pds, GETINT16(pds->socket), pds->priv));

	tError = 0;

	if( pds->socket == 0) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: stream out of state"));
		tError = TOUTSTATE;
		ipxStats.IpxTLIOutBadState++;
		freemsg(mp);
		goto ErrorInd;
	}

	if (DATA_SIZE(mp) < sizeof(struct T_unitdata_req) ) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: bad udatareq hdr size %d",DATA_SIZE(mp)));
		tError = TBADADDR;
		ipxStats.IpxTLIOutBadSize++;
		freemsg(mp);
		goto ErrorInd;
	}

	/*
	 * Verify that size of data does not exceed the TSDU size for IPX
	 * Since number of data octets sent out must be an even number,
	 * when checking data size add one to size if size is not an even
	 * number of octets. 
	 */
	if ((mp->b_cont) && (msize = msgdsize(mp->b_cont))) {
		if( msize & 1)
			msize++;
		if (msize > ipxMinMaxSDU) {
			tError = TBADDATA;
			freemsg(mp);
			goto ErrorInd;
		}
	}


	TUDataReq = (struct T_unitdata_req *)mp->b_rptr;

	if (TUDataReq->DEST_length < sizeof(ipxAddr_t)) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: bad address length %d",
				TUDataReq->DEST_length));
		tError = TBADADDR;
		ipxStats.IpxTLIOutBadAddr++;
		freemsg(mp);
		goto ErrorInd;
	}

	IPXCOPYADDR(mp->b_rptr + TUDataReq->DEST_offset, &destAddress);

	if (TUDataReq->OPT_length > (sizeof(uint8) + sizeof(uint16))) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: bad opt length %d",TUDataReq->OPT_length));
		tError = TBADOPT;
		ipxStats.IpxTLIOutBadOpt++;
		freemsg(mp);
		goto ErrorInd;
	}

	NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: TPI OPT_length = 0x%X", TUDataReq->OPT_length));
	if (TUDataReq->OPT_length == 0) {
		ipxPacketType = 0;
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: TPI gets default pt = 0x%X", ipxPacketType));
	} else {
		ipxPacketType = *(uint8 *)(mp->b_rptr + TUDataReq->OPT_offset);
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: TPI pt gets 0x%X", ipxPacketType));
		if(TUDataReq->OPT_length >= 3) {
			GETALIGN16( mp->b_rptr + TUDataReq->OPT_offset + 1, &chksum);
			NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
				"IpxTpiUDataReq: TPI chksum gets 0x%X", chksum));
		}
	}

	if (DATA_BSIZE(mp) < sizeof(ipxHdr_t)) {
		NWSLOG((IPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"IpxTpiUDataReq: have to alloc ipx header"));
		if ((nmp=allocb(sizeof(ipxHdr_t),BPRI_MED)) == NULL) {
			NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
				"IpxTpiUDataReq: cant alloc header"));
			tError = TNOADDR;
			ipxStats.IpxTLIOutHdrAlloc++;
			freemsg(mp);
			goto ErrorInd;
		} else {
			NWSLOG((IPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"IpxTpiUDataReq: switching b_cont fields"));
			nmp->b_cont = mp->b_cont;
			freeb(mp);
			mp = nmp;
		}
	} else {
		mp->b_rptr = mp->b_datap->db_base;
		NWSLOG((IPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"IpxTpiUDataReq: DATA_SIZE(mp) >= ipxHdr_t"));
	}

	MTYPE(mp) = M_DATA;
	mp->b_wptr = mp->b_rptr + sizeof(ipxHdr_t);

	ipxHdrPtr = (ipxHdr_t *)mp->b_rptr;
	PPUTINT16((uint16)msgdsize(mp), &ipxHdrPtr->len);
	ipxHdrPtr->pt = ipxPacketType;
	IPXCOPYADDR(&destAddress, &ipxHdrPtr->dest);
	ipxHdrPtr->chksum = chksum;
#ifdef NTR_TRACING
	if(chksum != IPX_CHKSUM) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: chksum trigger gets 0x%X",
			PGETINT16(&ipxHdrPtr->chksum)));
	}
#endif

	ipxStats.IpxTLIOutToSwitch++;
	IpxPropagatePkt(q, mp);

	NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"IpxTpiUDataReq: EXIT"));

	NTR_VLEAVE();
	return;

/*NOTREACHED*/	/* Shut lint up */
ErrorInd:
	NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
		"IpxTpiUDataReq: Error %d",tError));

	if ((ErrorMp=allocb(sizeof(struct T_uderror_ind), BPRI_HI))
		== NULL) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"IpxTpiUDataReq: cant alloc >16"));
			NTR_VLEAVE();
			return;
	}

	TUDErrorInd = (struct T_uderror_ind *)ErrorMp->b_rptr;
	TUDErrorInd->PRIM_type = T_UDERROR_IND;
	TUDErrorInd->DEST_length = 0;
	TUDErrorInd->DEST_offset = 0;
	TUDErrorInd->OPT_length = 0;
	TUDErrorInd->OPT_offset = 0;
	TUDErrorInd->ERROR_type = tError;
	ErrorMp->b_wptr= ErrorMp->b_rptr + sizeof(struct T_uderror_ind);
	MTYPE(ErrorMp) = M_PROTO;
	qreply(q,ErrorMp);
	NTR_VLEAVE();
	return;
}

/*
 * void IpxTpiUDataInd(queue_t *urdq, mblk_t *mp)
 *	Send TLI application data to the application.
 *	Called from IpxRouteDataToSocket, which released all locks but has
 *	the socket queue counter incremented.  IpxRouteDataToSocket will
 *	decrement the queue counter when this function returns.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 */
FSTATIC void
IpxTpiUDataInd(queue_t *urdq, mblk_t *mp)
{
	struct T_unitdata_ind *tUDataInd;
	ipxHdr_t *inIpxHdr;
	uint8 ipxPacketType;
	ipxAddr_t ipxSourceAddress;
	char *bp;
	mblk_t	*nmp;

	NTR_ENTER(2, urdq, mp, 0,0,0);

	inIpxHdr = (ipxHdr_t *) mp->b_rptr;

	NWSLOG((IPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"IpxTpiUDataInd ENTER"));
	
	/*
	 *	No check for (DATA_SIZE(mp) < IPX_HDR_SIZE) lipmx did this for us
	 */

	IPXCOPYADDR(&inIpxHdr->src, &ipxSourceAddress);
	ipxPacketType = inIpxHdr->pt;

	/*
	 * since the udata ind has to be a separate message block we have
	 * to allocate and tack it on
	 */

	if ((nmp=allocb(sizeof(struct T_unitdata_ind) + sizeof(ipxAddr_t) +
		sizeof(uint8), BPRI_MED)) == NULL ) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataInd: cant alloc >32"));
		ipxStats.IpxRoutedTLIAlloc++;
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	nmp->b_cont = mp;

	tUDataInd = (struct T_unitdata_ind *)nmp->b_rptr;
	tUDataInd->PRIM_type = T_UNITDATA_IND;
	tUDataInd->SRC_length = sizeof(ipxAddr_t);
	tUDataInd->SRC_offset = sizeof(struct T_unitdata_ind);
	tUDataInd->OPT_length = sizeof(uint8);
	tUDataInd->OPT_offset = sizeof(struct T_unitdata_ind) + sizeof(ipxAddr_t);
	IPXCOPYADDR(&ipxSourceAddress, nmp->b_rptr + sizeof(struct T_unitdata_ind));

	bp = (char *)(nmp->b_rptr + sizeof(struct T_unitdata_ind)
			+ sizeof(ipxAddr_t));
	*bp = ipxPacketType;

	MTYPE(nmp) = M_PROTO;
	nmp->b_wptr = nmp->b_rptr
		+ sizeof(struct T_unitdata_ind) + sizeof(ipxAddr_t) + sizeof(uint8);

	/*
	 *	Don't hand the app the ipx header
	 */
	mp->b_rptr += IPX_HDR_SIZE;

	/*
	 *	Canput already done by RouteDataToSocket
	 */
	ipxStats.IpxRoutedTLI++;
	putnext(urdq,nmp);

	NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"IpxTpiudataInd: EXIT"));

	NTR_VLEAVE();
	return;
}

/*
 * void IpxTpiOptReq(queue_t *q, mblk_t *mp)
 *	Responds to TLI Options request
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
FSTATIC void
IpxTpiOptReq(queue_t *q, mblk_t *mp)
{	struct 	T_optmgmt_req 	*tOptReq;
			ipxAddr_t 		*ipxAddrPtr;
			ipxPDS_t		*pds;
			ipxSocket_t		*sock;

	NTR_ENTER(2, q, mp, 0,0,0);

	NWSLOG((IPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"IpxTpiOptReq ENTER"));

	pds = (ipxPDS_t *)&(RD(q)->q_ptr);
	tOptReq = (struct T_optmgmt_req *)mp->b_rptr;

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxTpiOptReq: pds @ 0x%X, sock 0x%X, priv 0x%X",
		pds, GETINT16(pds->socket), pds->priv));

	if (DATA_SIZE(mp)<sizeof(struct T_optmgmt_req)+sizeof(ipxAddr_t)){
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxTpiOptReq Bad request size 0x%X",DATA_SIZE(mp)));
		IpxTpiErrorAck( q, T_OPTMGMT_REQ, TBADOPT, 0);
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	if( pds->socket == 0) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUDataReq: stream out of state"));
		IpxTpiErrorAck( q, T_OPTMGMT_REQ, TOUTSTATE, 0);
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	ipxAddrPtr = (ipxAddr_t *)(mp->b_rptr + tOptReq->OPT_offset);

	sock = (ipxSocket_t *)q->q_ptr;
	IPXCOPYNET(&ipxInternalNet, ipxAddrPtr->net);
	IPXCOPYNODE(ipxInternalNode, ipxAddrPtr->node);
	IPXCOPYSOCK(&(sock->socket), ipxAddrPtr->sock);

	tOptReq->PRIM_type = T_OPTMGMT_ACK;
	MTYPE(mp) = M_PCPROTO;
	qreply(q,mp);

	NWSLOG((IPXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
			"IpxTpiOptReq EXIT"));

	NTR_VLEAVE();
	return;
}

/*
 * void IpxTpiUnbindReq(queue_t *q)
 *	Respond to a TLI Unbind Socket request.  A single bind to a queue
 *	is allowed.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 *	Acquires/Releases DEVICE_LOCK, QUEUE_LOCK, and HASH_LOCK.
 */
FSTATIC void
IpxTpiUnbindReq(queue_t *q)
{	struct T_ok_ack *okAckPtr;
	ipxPDS_t		*pds;
	mblk_t 			*ackMp;
	ipxSocket_t		*sock;
	pl_t			 lvl;

	NTR_ENTER(1, q, 0,0,0,0);

	NWSLOG((IPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"IpxTpiUnbind ENTER "));

	pds = (ipxPDS_t *)&(RD(q)->q_ptr);

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxTpiUnbindReq: pds @ 0x%X, sock 0x%X, priv 0x%X",
		pds, GETINT16(pds->socket), pds->priv));

	if( !(pds->priv & TLI_SOCKET) || (pds->socket == 0)) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUnbind bad state stream not bound tpi"));
		IpxTpiErrorAck( q, T_UNBIND_REQ, TOUTSTATE, 0);
		NTR_VLEAVE();
		return;
	}

	sock = (ipxSocket_t *)q->q_ptr;

	NWSLOG((IPXID, 0, PNW_DATA_ASCII, SL_TRACE,
		"IpxTpiUnbind: Unbinding TLI socket 0x%X", GETINT16( sock->socket)));

	if ((ackMp=allocb(sizeof(struct T_ok_ack),BPRI_MED)) == NULL) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
			"IpxTpiUnbind no streams resource"));
		IpxTpiErrorAck( q, T_UNBIND_REQ, TSYSERR, ENOSR);
		NTR_VLEAVE();
		return;
	}

	lvl = LOCK( ipxDeviceLock, plstr);

	IpxDeallocateSocket( sock);
	ipxStats.IpxBoundSockets--;

	UNLOCK( ipxDeviceLock, lvl);

	pds->priv &= ~TLI_SOCKET;


	okAckPtr = (struct T_ok_ack *)ackMp->b_rptr;
	okAckPtr->PRIM_type = T_OK_ACK;
	okAckPtr->CORRECT_prim = T_UNBIND_REQ;
	MTYPE(ackMp) = M_PCPROTO;
	ackMp->b_wptr = ackMp->b_rptr + sizeof(struct T_ok_ack);
	qreply(q,ackMp);

	NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"IpxTpiUnbind EXIT "));
	NTR_VLEAVE();
	return;
}

/*
 * void IpxIocBindSocket(queue_t *q, mblk_t *mp, int type, int tli_flag)
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	Acquires/Releases DEVICE_LOCK, QUEUE_LOCK, and HASH_LOCK.
 */
FSTATIC void
IpxIocBindSocket(queue_t *q, mblk_t *mp, int type, int tli_flag)
{
	uint16 			socketNetOrder;
	IpxSetSocket_t *reqSocket;
	long			state;
	ipxPDS_t		*pds;
	struct iocblk	*iocp;
	pl_t			lvl;
	ipxSocket_t     *Asock;

	NTR_ENTER(4, q, mp, type,tli_flag,0);

	ipxStats.IpxBind++;
	NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
		"SWITCH M_IOCTL: IpxIocBindSocket: IPX_BIND_SOCKET type %d", type));

	if( (!mp->b_cont) || (DATA_SIZE(mp->b_cont) != sizeof(IpxSetSocket_t))) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocBindSocket: SOCKET wrong size"));
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	pds = (ipxPDS_t *)&(RD(q)->q_ptr);
	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxIocBindSocket: pds @ 0x%X, priv 0x%X, sock = 0x%X",
		pds, pds->priv, GETINT16(pds->socket)));

	/*
	 *	Don't allow multiple binds using SET SOCKET
	 *
	 *	Don't allow mixing of socket bind functions
	 */
	switch( type) {
	case BIND_SOCKET:
		if( pds->priv & (TLI_SOCKET | SET_SOCKET | KNL_SOCKET) ) {
			type = 0xFF;
		}
		break;
	case SET_SOCKET:
		if( pds->socket != 0) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"IpxIocBindSocket: Cannot do multiple SET_SOCKETS"));
			type = 0xFF;
		}
		break;
	case KNL_SOCKET:
		if( pds->priv & (TLI_SOCKET | SET_SOCKET | BIND_SOCKET) ) {
			type = 0xFF;
		}
		break;
#ifdef DEBUG
	default:
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocBindSocket: Illegal type %d", type));
		/*
		 *+ Socket structure is probably waco
		 */
		cmn_err(CE_PANIC, "IpxIocBindSocket: Illegal type 0x%X", type);
		NTR_VLEAVE();
		return;
#endif
	}

	if( type == 0xFF) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocBindSocket: Cannot mix SET/BIND socket functs 0x%X, type 0x%X",
			pds->priv, type));
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	} 

	reqSocket = (IpxSetSocket_t *)mp->b_cont->b_rptr;
	socketNetOrder = GETINT16(reqSocket->socketNum);


    /*
     *  On AIX, we cannot acquire kernel memory if splstr, so get it now.
	 *	to avoid ifdef's we do it the same for everyone
	 */
	if( (Asock = (ipxSocket_t *)
			kmem_alloc( sizeof( ipxSocket_t), KM_NOSLEEP)) == NULL) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocBindReq: Allocate of socket structure failed"));
		/*
		 *+ Could not allocate socket structure
		 */
		cmn_err(CE_WARN,
			"IpxIpxTpiBindReq: Allocate of socket structure failed");
		NTR_VLEAVE();
		return;
	}
	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxIocBindReq: KMEM: Pre Allocate of socket structure 0x%X size %d",
		Asock, sizeof( ipxSocket_t)));

	lvl = LOCK( ipxDeviceLock, plstr);

	if( (socketNetOrder = IpxAllocateSocket(
			socketNetOrder, RD(q), &state, &Asock, tli_flag)) == 0) {
		UNLOCK( ipxDeviceLock, lvl);
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocBindReq: KMEM: Free socket structure at 0x%X, size %d", 
				Asock, sizeof(ipxSocket_t)));
		kmem_free( Asock, sizeof( ipxSocket_t));
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocBindSocket: Allocation of socket failed"));
		if( type != KNL_SOCKET) {
			switch(state) {
				case TACCES: state = EPERM; break;
				case TNOADDR: state = EBUSY; break;
				case TADDRBUSY: state = EBUSY; break;
			}
		}
		IpxIocNegAck(q,mp,state);
		NTR_VLEAVE();
		return;
	}

	/*
	 *	Check to see if this socket already bound by this queue
	 */
	if( state > 0) {
		ipxStats.IpxBoundSockets++;
	}

	UNLOCK( ipxDeviceLock, lvl);

	/*
	 * If socket struct wasn't used, release it
	 */
	if( Asock != NULL) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocBindReq: KMEM: Free socket structure at 0x%X, size %d", 
				Asock, sizeof(ipxSocket_t)));
		kmem_free( Asock, sizeof( ipxSocket_t));
	}

	/* if KNL_SOCKET (SPX) let them know the state.
	 * if state < 0 the socket is already bound, by same stream 
	 */
	if( type == KNL_SOCKET) {
		iocp = (struct iocblk *)mp->b_rptr;
		iocp->ioc_rval = state;	
	}

	if( type == SET_SOCKET) {
		reqSocket->socketNum = socketNetOrder;
	} else {
		reqSocket->socketNum = GETINT16( socketNetOrder);
	}
	pds->priv |= type;

	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"IpxIocBindSocket: Device 0x%X, Socket = 0x%X, Queue = 0x%X",
		(uint32)(q->q_ptr), GETINT16(socketNetOrder), RD(q)));

	if( socketNetOrder == RIPsocket) {
		LipmxRouteRIP( ENABLE_RIP);	/* Tell lipmx to route up RIP packets */
	}

	IpxIocAck(q,mp,IPX_SOCK_SIZE);
	NTR_VLEAVE();
	return;
}

/*
 * void IpxIocUnbindSocket(queue_t *q, mblk_t *mp)
 *	Non TLI Release (Unbind) of a socket (SET, BIND, KNL)
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	Acquires/Releases DEVICE_LOCK, QUEUE_LOCK, and HASH_LOCK.
 */
FSTATIC void
IpxIocUnbindSocket(queue_t *q, mblk_t *mp)
{
	uint16 	socketNetOrder;
	IpxSetSocket_t *reqSocket;
	ipxSocket_t *sock;
	ipxPDS_t	*pds;
	pl_t		 dlvl, hlvl;

	NTR_ENTER(2, q, mp, 0,0,0);

	NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
		"SWITCH M_IOCTL: IpxIocUnbindSocket: IPX_REL_SOCKET"));

	if( (!mp->b_cont) || (DATA_SIZE(mp->b_cont) != sizeof(IpxSetSocket_t))) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocUnbindSocket: SOCKET wrong size"));
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	reqSocket = (IpxSetSocket_t *)mp->b_cont->b_rptr;
	socketNetOrder = GETINT16(reqSocket->socketNum);

	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"IpxIocUnbindSocket: Release socket 0x%X", GETINT16(socketNetOrder)));

	dlvl = LOCK( ipxDeviceLock, plstr);
	
	if( (sock = IpxHashFindSocket( socketNetOrder, &hlvl)) == NULL) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocUnbindSocket: Socket not found"));
		UNLOCK( ipxDeviceLock, dlvl);
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	UNLOCK( SocketHashTable[SOCKETHASH(&sock->socket)].hlock, hlvl);

	/*
	 *	Make sure socket owned by this queue
	 */
	if( sock->qptr != RD(q)) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocUnbindSocket: Stream does not own this socket"));
		UNLOCK( ipxDeviceLock, dlvl);
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	pds = (ipxPDS_t *)&(RD(q)->q_ptr);

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxIocUnbindSocket: pds @ 0x%X, sock 0x%X, priv 0x%X",
		pds, GETINT16(pds->socket), pds->priv));

	if( pds->priv & TLI_SOCKET) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocUnbindSocket: Cannot ioctl Unbind TLI socket"));
		UNLOCK( ipxDeviceLock, dlvl);
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	if( sock->socket == RIPsocket) {
		LipmxRouteRIP( DISABLE_RIP);/* Tell lipmx to stop routing RIP packets */
	}

	IpxDeallocateSocket( sock);
	ipxStats.IpxBoundSockets--;

	UNLOCK( ipxDeviceLock, dlvl);

	/*
	 *	If last socket released, clear SET/BIND flags
	 */
	if( pds->socket == 0) {
		pds->priv &= ~(SET_SOCKET | BIND_SOCKET | KNL_SOCKET);
	}
	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"IpxIocUnbindSocket: Socket release complete"));
	IpxIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void IpxIocSetWater(queue_t *q, mblk_t *mp)
 *	Set Steam head HI and LO water marks.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 */
FSTATIC void
IpxIocSetWater(queue_t *q, mblk_t *mp)
{
	IpxSetWater_t *waterMark;
	mblk_t	*optmp;
	struct stroptions *opts;

	NTR_ENTER(2, q, mp, 0,0,0);

	NWSLOG((IPXID,0,PNW_SWITCH_CASE,SL_TRACE,
		"SWITCH M_IOCTL: IpxIocSetWater: IPX_SET_WATER"));

	if( (!mp->b_cont) || (DATA_SIZE(mp->b_cont) != sizeof(IpxSetWater_t))) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocSetWater: wrong size"));
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	waterMark = (IpxSetWater_t *)mp->b_cont->b_rptr;

	if( waterMark->hiWater == UINT_MAX) {
		waterMark->hiWater = hiWaterMark;
	}
	if( waterMark->loWater == UINT_MAX) {
		waterMark->loWater = loWaterMark;
	}

	if( (waterMark->loWater > loWaterMark) || (waterMark->hiWater == 0)
			|| (waterMark->hiWater > hiWaterMark)) {
		/* only accept water marks bigger than max if privileged */
		if( !IsPriv(q)) {
			NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"IpxIocSetWater: Not privileged"));
			IpxIocNegAck(q,mp,EINVAL);
			NTR_VLEAVE();
			return;
		}
	}

	/*
	 *	Set a high water mark so we don't easily loose packets
	 *	due to flow control at the stream head.  (if canput fails
	 *	ipx drops the packet)
	 */
	if ((optmp = allocb(sizeof(struct stroptions),BPRI_MED)) == NULL) {
		NWSLOG((IPXID, 0, PNW_ERROR, SL_ERROR,
			"IpxIoxSetWater: Unable to allocate M_SETOPTS buf"));
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	opts = (struct stroptions *)optmp->b_rptr;
	optmp->b_wptr = optmp->b_rptr + sizeof(struct stroptions);
	opts->so_flags = SO_HIWAT | SO_LOWAT;
	opts->so_hiwat = waterMark->hiWater;
	opts->so_lowat = waterMark->loWater;
	MTYPE(optmp) = M_SETOPTS;
	qreply(q, optmp);
	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"IpxIocSetWater: HIwater =%d LOwater = %d",
		waterMark->hiWater, waterMark->loWater));
	IpxIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void IpxIocNegAck(queue_t *q, mblk_t *mp, int err)
 *	IOCTL negative acknowledge
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 */
FSTATIC void
IpxIocNegAck(queue_t *q, mblk_t *mp, int err)
{
	struct iocblk *iocp;
	NTR_ENTER(3, q, mp, err, 0, 0);

	NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
		"IpxIocNegAck: Negative Acknowledgement of M_IOCTL, err = %d", err));
	iocp = (struct iocblk *)mp->b_rptr;
	MTYPE(mp) = M_IOCNAK;
	iocp->ioc_error = err;
	iocp->ioc_count = 0;
	qreply(q, mp);
	NTR_VLEAVE();
	return;
}

/*
 * void IpxIocAck(queue_t *q, mblk_t *mp, uint count)
 *	IOCTL positive acknowledge
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 */
FSTATIC void
IpxIocAck(queue_t *q, mblk_t *mp, uint count)
{
	struct iocblk *iocp;
	NTR_ENTER(3, q, mp, count,0,0);

	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"IpxIocAck: Positive Acknowledgement of M_IOCTL, count = %d",
		count));
	iocp = (struct iocblk *)mp->b_rptr;
	MTYPE(mp) = M_IOCACK;
	iocp->ioc_error = 0;
	iocp->ioc_count = count;
	qreply(q, mp);
	NTR_VLEAVE();
	return;
}

/*
 * void IpxUpperFlush(queue_t *q, mblk_t *mp)
 *	Flush upper queues
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 */
FSTATIC void
IpxUpperFlush(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	NWSLOG((IPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"Enter IpxUpperFlush"));

	if (*mp->b_rptr & FLUSHW)
		flushq(q, FLUSHDATA);
	if (*mp->b_rptr & FLUSHR) {
		flushq(RD(q), FLUSHDATA);
		*mp->b_rptr &= ~FLUSHW;
		qreply(q, mp);
	} else {
		freemsg(mp);
	}
	NTR_VLEAVE();
	return;
}

/*
 * void IpxIocInitialize(queue_t *q, mblk_t *mp)
 *	Mark IPX Initialized, must come from control device
 *	Sets flag that allows clone devices to open
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 */
FSTATIC void
IpxIocInitialize(queue_t *q, mblk_t *mp)
{
	IpxSetWater_t *waterMark;

	NTR_ENTER(2, q, mp, 0,0,0);

	NWSLOG((IPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"Enter IpxIocInitialize"));

	if( !IsControl( q)) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE, "Not control"));
		IpxIocNegAck(q,mp,EPERM);
		NTR_VLEAVE();
		return;
	}

	if( ipxInitialized != 0) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE, "Already initialized"));
		IpxIocNegAck(q,mp,EAGAIN);
		NTR_VLEAVE();
		return;
	}

	/*
	**	Set Max Hi Water Mark
	*/
	if( (!mp->b_cont) || (DATA_SIZE(mp->b_cont) < sizeof(IpxSetWater_t))) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
			"IpxIocInitialize: wrong size"));
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	waterMark = (IpxSetWater_t *)mp->b_cont->b_rptr;

	if( waterMark->loWater > loWaterMark) {
		loWaterMark = waterMark->loWater;
	}
	if( waterMark->hiWater > hiWaterMark) {
		hiWaterMark = waterMark->hiWater;
	}
	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"IpxInitialize: Max HIwater =%d Max LOwater = %d",
		hiWaterMark, loWaterMark));

	ipxMinMaxSDU = LipmxGetMinMaxSDU();
	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"IpxInitialize: Minimum of MaxSDUs = %d ", ipxMinMaxSDU));

	/*
	**	Indicate we are initialized
	*/
	ipxInitialized = 1;
	IpxIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void IpxIocStats(queue_t *q, mblk_t *mp)
 *	Return statistics to application.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 */
FSTATIC void
IpxIocStats(queue_t *q, mblk_t *mp)
{	IpxSocketStats_t	*sp;

	NTR_ENTER(2, q, mp, 0,0,0);

	NWSLOG((IPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"Enter IpxIocStats"));

	if((mp->b_cont == NULL) || 
		(DATA_SIZE(mp->b_cont) <
			sizeof(IpxSocketStats_t) + sizeof(IpxLanStats_t))) {
		NWSLOG((IPXID,0,PNW_ERROR,SL_TRACE,
				"IpxIocStats: not enough data area"));
		IpxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	sp = (IpxSocketStats_t *)(mp->b_cont->b_rptr + sizeof(IpxLanStats_t));
	*sp = ipxStats;
	lipmxuwput(q,mp);
	NTR_VLEAVE();
	return;
}

/*
 * void IpxPropagatePkt(queue_t *q, mblk_t *mp)
 *	Send a packet from an application to lipmx (to the wire).
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 *	Under one condition (Non Kernel multiple binds) QUEUE_LOCK is acquired.
 */
FSTATIC void
IpxPropagatePkt(queue_t *q, mblk_t *mp)
{
	uint16  		srcSocket;
	ipxPDS_t		*pds;
	ipxSocket_t		*sock, *qsock;
	ipxHdr_t		*ipxHdrPtr;
	pl_t			 lvl;
	
	NTR_ENTER(2, q, mp, 0,0,0);

	pds = (ipxPDS_t *)&(RD(q)->q_ptr);
	ipxHdrPtr = (ipxHdr_t *)mp->b_rptr;
	ipxStats.IpxInSwitch++;

	NWSLOG((IPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"IpxPropagatePkt: pds @ 0x%X, sock 0x%X, priv 0x%X",
		pds, GETINT16(pds->socket), pds->priv));

	/* 
	 * If application did BIND_SOCKET on this stream, we must
	 * verify the socket used, otherwise, set socket for user
	 */
	if( pds->priv & (SET_SOCKET | TLI_SOCKET)) {
		qsock = (ipxSocket_t *)q->q_ptr;
		srcSocket = qsock->socket;
		IPXCOPYSOCK(&srcSocket, ipxHdrPtr->src.sock);
	} else {

		IPXCOPYSOCK(ipxHdrPtr->src.sock, &srcSocket);
		if( ( srcSocket == 0)
				|| ( (pds->priv & (KNL_SOCKET | BIND_SOCKET)) == 0)) {
			/*
			 *	Source socket must be specified for BIND | KNL socket.
			 *	If it is not set, the setream is in error.
			 *	Error the stream if no socket is bound
			 */
			MTYPE(mp) = M_ERROR;
			mp->b_rptr = mp->b_datap->db_base;
			mp->b_wptr = mp->b_rptr+1;
			*(mp->b_rptr) = EPROTO;
			if( srcSocket == 0) {
				NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
					"IpxPropagatePkt: BIND_SOCKET stream, pkt socket==0, shut down stream"));
#ifdef DEBUG
				cmn_err(CE_WARN,
					"IpxPropagatePkt: BIND_SOCKET stream, pkt socket==0, shut down stream");
#endif
			}
			ipxStats.IpxSwitchInvalSocket++;
			qreply( q, mp);
			NTR_VLEAVE();
			return;
		}
#ifndef DEBUG
		/*
		 *	KNL_SOCKET is trusted, we don't check socket number
		 *	unless DEBUG is on
		 */
		if( (pds->priv & BIND_SOCKET) > 0) {
#endif
			/*
			 *	Locate the socket structure for this socket
			 *	Done inline to avoid function call overhead
			 */
			qsock = (ipxSocket_t *)q->q_ptr;
			lvl = LOCK( qsock->qlock, plstr);

			sock = qsock;
			while( sock) {
				if( sock->socket == srcSocket) {
					break;
				}
				sock = sock->qlink;
			}

			UNLOCK( qsock->qlock, lvl);

			if( sock == NULL) {
				/* Not found, drop msg */
				NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
					"IpxPropagatePkt: Error, can't find socket 0x%X, dropped",
					GETINT16(srcSocket)));
				ipxStats.IpxSwitchInvalSocket++;
				freemsg( mp);
				NTR_VLEAVE();
				return;
			}

#ifndef DEBUG
		}
#endif
	}
	/* 
	 * Need to allow certain sockets to use NIC net/node.
	 * RIP/SAP source address has been set to
	 * a connected net/node address.  If RIP goes to user-land, let it.
	 */
	if(			!(srcSocket == SAPsocket)
			&&	!(srcSocket == RIPsocket)) {
		/* fill in the source internal net/node */
		IPXCOPYNET(&ipxInternalNet, ipxHdrPtr->src.net);
		IPXCOPYNODE(ipxInternalNode, ipxHdrPtr->src.node);
	}
	NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"IpxPropagatePkt: source socket is 0x%X",
		PGETINT16(ipxHdrPtr->src.sock)));

	/*
	 * If the packet type (pt) is equal to IPX_WAN_PACKET_TYPE
	 * (interlan broadcast) then the transport control byte is
	 * used to keep track of how many networks have been
	 * traversed.  Since this pkt is just starting out,
	 * tc = 0 == TRANSPORT_CONTROL, so it gets the same value
	 * whether WAN or not.
 	 */
	ipxHdrPtr->tc = TRANSPORT_CONTROL;

	if( PGETINT16( &ipxHdrPtr->chksum) == GETINT16(IPX_CHKSUM_TRIGGER)) {
		if( (mp = IpxSumPacket(mp, CHKSUM_CALC)) == NULL) {
			NWSLOG((IPXID,0,PNW_DROP_PACKET,SL_TRACE,
				"IpxPropagatePacket: IpxSumPacket() failed, dropped"));
			ipxStats.IpxSwitchSumFail++;
			NTR_VLEAVE();
			return;
		}
		ipxStats.IpxSwitchSum++;
	} else {
		PPUTINT16( IPX_CHKSUM, &ipxHdrPtr->chksum);
		NWSLOG((IPXID,0,PNW_DROP_PACKET,SL_TRACE,
			"IpxPropagatePacket: Set IPX chksum to default"));
	}

#ifdef IPX_PAD_EVEN
	{
		mblk_t *msgBlock = mp, *padMsgBlk;
		int msize;
		
		msize = msgdsize( msgBlock);
		if( msize & 1) /* if( msize % 2) */ {
			ipxStats.IpxSwitchEven++;
		   /* find the last message block and add 1 byte */
			while(msgBlock->b_cont != NULL)
				 msgBlock = msgBlock->b_cont;
			if (DATA_REMAINING(msgBlock) > 1) {
				msize++;
				msgBlock->b_wptr++;
				NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
					"IpxPropagatePacket: odd size buf, new size %d", msize));
			} else {
				padMsgBlk = allocb(1,BPRI_MED);
				if( padMsgBlk == NULL) {
					ipxStats.IpxSwitchAllocFail++;
					NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
						"IpxPropagatePkt Alloc failed, dropped"));
					freemsg(mp);
					NTR_VLEAVE();
					return;
				}
				ipxStats.IpxSwitchEvenAlloc++;
				MTYPE( padMsgBlk) = M_DATA;
				*padMsgBlk->b_wptr = 0x00;
				padMsgBlk->b_wptr++;
				msgBlock->b_cont = padMsgBlk;
				NWSLOG((IPXID,0,PNW_ASSIGNMENT,SL_TRACE,
					"ipxuwput: odd size buf, alloc buf new size %d",
					 ++msize));
			}
		}
		NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"IpxPropagatePkt send to lipmx, size = %d", msize));
	}
#else
	NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"IpxPropagatePkt send to lipmx"));
#endif
	ipxStats.IpxSwitchToLan++;
	lipmxuwput(q, mp);
	NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, "IpxPropagatePkt EXIT"));
	NTR_VLEAVE();
	return;
}

/*
 * int IpxSumPacket(mblk_t *mp, int task)
 *	Computes checksum for a data packet.
 *	Who can get checksums calculated (outgoing):
 * 	 1) NCPs
 * 	 2) SPX
 * 	 3) TLI with special _OPT (but not NetBios)
 *	 4) putmsg with IPX_TRIGGER
 *
 *	All incoming packets with chksum != 0xFFFF will
 *	have their chksums verified.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 */
FSTATIC mblk_t *
IpxSumPacket(mblk_t *mp, int task)
{	char		end[2], hops;
	uint16		checksum, i, len;
	uint16		*p;
	uint32		sum;
	ipxHdr_t	*ipxPkt;
	mblk_t		*pmp;

	NTR_ENTER(2, mp, task, 0,0,0);

	ipxPkt = (ipxHdr_t *)mp->b_rptr;
	NWSLOG((IPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"Enter IpxSumPacket: task = %d sum = 0x%X",
			task, PGETINT16(&(ipxPkt->chksum)))); 
	switch(task) {
		default:
			freemsg( mp);
			return( (mblk_t *)NTR_LEAVE(NULL));
			/*NOTREACHED*/
			break;
		case CHKSUM_CALC:
			PPUTINT16(0, &(ipxPkt->chksum));
			/* FALLTHRU */
		case CHKSUM_VERIFY:
			/*
			 *	if we don't set chksum to 0, chksum comes out to be 0
			 */
			break;
	}

	/* Ignore hops in chksum calc */
	hops = ipxPkt->tc;
	ipxPkt->tc = 0;

	len = PGETINT16(&(ipxPkt->len));
	NWSLOG((IPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"IpxSumPacket: len = 0x%X", len));
	if(mp->b_cont) {
		if( (pmp = msgpullup(mp,len)) == NULL) {
			NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
				"IpxSumPacket: cant msgpullup %d",len));
			freemsg( mp);
			return( (mblk_t *)NTR_LEAVE(NULL));
		}
#ifndef NW_UP
		/* Due to the difference in behavior of msgpullup and pullupmsg */
		freemsg( mp);
		mp = pmp;
#endif
	}	

	/* Set up pointer to NEW mp */
	ipxPkt = (ipxHdr_t *)mp->b_rptr;

	/* calculate chksum in Intel byte-order */
	if(len & 1) {	/* handle odd size msg now */
		end[0] = (char)*(mp->b_wptr-1);
		end[1] = 0;
		sum = PREVGETINT16(&end);
	} else {
		sum = 0;
	}

	for( i=len>>1, p=(uint16 *)ipxPkt; i; i--, p++) {
		sum += PREVGETINT16(p);
	}
	sum = (sum & 0xffff) + (sum >> 16); /* add in carrys from for loop */
	sum = (sum & 0xffff) + (sum >> 16); /* add in final carry*/
	checksum = ~(uint16)sum;

	NWSLOG((IPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"IpxSumPacket: checksum = 0x%X", checksum));
	if(task == CHKSUM_VERIFY) {
		if(checksum) {
			NWSLOG((IPXID, 0, PNW_ERROR, SL_TRACE,
					"IpxSumPacket: VERIFY failed"));
			freemsg( mp);
			return( (mblk_t *)NTR_LEAVE(NULL));
		}
	} else
		PREVPUTINT16(checksum, &(ipxPkt->chksum));

	/* Restore hops in header */
	ipxPkt->tc = hops;

	NWSLOG((IPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"EXIT IpxSumPacket")); 
	return( (mblk_t *)NTR_LEAVE((uint32)mp));
}
