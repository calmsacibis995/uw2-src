/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/dppool.c	1.18"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/dppool.c,v 2.56.2.2 1994/12/21 02:46:12 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncpppool.c
 *	ABSTRACT:  Manage pool of NCP packets on a per channel basis
 *
 *	Functions declared in this module:
 *	Public functions:
 *		NCPdplAllocatePool
 *		NCPdplFreePool
 *		NCPdplGetFreePacket
 *		NCPdplFreePacket
 *		NCPdplEnablePacket
 *	Private functions:
 */ 

#ifdef _KERNEL_HEADERS
#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <util/cmn_err.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/nwctypes.h>	/* formerly included by ncpiopack.h */
#include <net/nuc/nucmachine.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/slstruct.h>	/* needed for prototypes in spilcommon.h */
#include <net/nw/nwportable.h>	/* needed for prototypes in spilcommon.h */
#include <net/nuc/spilcommon.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/ncpconst.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nucerror.h>
#include <net/nw/ntr.h>
#include <util/debug.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>
#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>	/* formerly included by ncpiopack.h */
#include <sys/nucmachine.h>	/* formerly included by ncpinclude.h */
#include <sys/slstruct.h>	/* needed for prototypes in spilcommon.h */
#include <sys/nwportable.h>	/* needed for prototypes in spilcommon.h */
#include <sys/spilcommon.h>	/* formerly included by ncpinclude.h */
#include <sys/ncpconst.h>
#include <sys/ncpiopack.h>
#include <sys/nucerror.h>
#include <sys/debug.h>
#include <sys/nuc_prototypes.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_ncp

/*
 *	ANSI Function prototypes
 */
#if defined(__STDC__)
#endif

/*
 * BEGIN_MANUAL_ENTRY(NCPdplAllocatePool.4k)
 * NAME
 *		NCPdplAllocatePool - Allocate iopacket Pool for a channel.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPdplAllocatePool( numEntry, oppPtr )
 *		int32	numEntry;
 *		void_t	**oppPtr;
 *
 * INPUT
 *		numEntry	- Number of packets in the pool
 *
 * OUTPUT
 *		oppPtr		- Opaque pointer to the pool structure
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Allocates a number of NCP iopacket structures to be maintained as
 *		a pool to allow multiple contexts to be formatting packets while
 *		the wire is busy.
 *
 * NOTES
 *		Allocates a number of packets for the pool based upon a tuneable
 *		parameter in the nwncp kernel package.  
 *
 * SEE ALSO
 *		NCPdplFreePool(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPdplAllocatePool (int32 numEntry, void_t **oppPtr )
{
	int	ccode;
	ppool_t	*pPtr;
	int i;
	iopacket_t	*packet, *ptmp;

	NTR_ENTER(2, numEntry, oppPtr, 0, 0, 0);

	*oppPtr = (void_t *)kmem_alloc (sizeof(ppool_t), KM_SLEEP); 

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: NCPdplAllocatePool: alloc ppool_t * at 0x%x, size = 0x%x",
		*oppPtr, sizeof(ppool_t), 0 );
#endif NUCMEM_DEBUG

	pPtr = (ppool_t *)*oppPtr;
	pPtr->head = (iopacket_t *)NULL;
	pPtr->numEntry = numEntry;

	/*
	 *	Create the semaphore used for synchronizing entry
	 *	onto the queue. This will ensure the pool list doesn't
	 *	get trashed.
	 */
	ccode = NWtlCreateAndSetSemaphore((int *)(&(pPtr->pSemaphore)), numEntry);
	if (ccode != SUCCESS) {
		kmem_free ((char *)(*oppPtr), sizeof(ppool_t));

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: NCPdplAllocPool: free ppool_t * at 0x%x, size = 0x%x",
			*oppPtr, sizeof(ppool_t), 0 );
#endif NUCMEM_DEBUG

		return(NTR_LEAVE (-1));
	}

	/*
	 *	Allocate the buffers now...
	 */
	for (i = 0; i < numEntry; i++) {
		packet = (iopacket_t *)kmem_alloc (sizeof(iopacket_t), KM_SLEEP);

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM: NCPdplAllocatePool : alloc packet * at 0x%x, size = 0x%x",
			packet, sizeof(iopacket_t), 0 );
#endif NUCMEM_DEBUG

		/*
		 *	Setup the links between the nodes
		 */
		ptmp = pPtr->head;
		pPtr->head = packet;
		packet->next = ptmp;
		packet->pSemaphore = pPtr->pSemaphore;
		packet->status = IOP_FREE;
	}

	/*
	 *	In the event of an error during allocation, free up
	 *	everything that has been allocated during this process.
	 */
	if ( ccode ) {
		packet = pPtr->head;
		for (; i >= 0; i--) {
			ptmp = packet->next;
			if ( packet != (iopacket_t *)NULL )
				kmem_free ((char *)packet, sizeof(iopacket_t));

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: NCPdplAllocPool: free packet * at 0x%x, size = 0x%x",
			packet, sizeof(iopacket_t), 0 );
#endif NUCMEM_DEBUG

			packet = ptmp;
		}
		NWtlDestroySemaphore( pPtr->pSemaphore );
		kmem_free ( (char *)pPtr, sizeof(ppool_t));

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: NCPdplAllocPool: free ppool_t * at 0x%x, size = 0x%x",
			pPtr, sizeof(ppool_t), 0 );
#endif NUCMEM_DEBUG

	}
	
	return(NTR_LEAVE (ccode));	
}

/*
 * BEGIN_MANUAL_ENTRY(NCPdplFreePool.4k)
 * NAME
 *		NCPdplFreePool - Free previously allocated iopacket pool
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPdplFreePool( pPtr )
 *		void_t	*pPtr;
 *
 * INPUT
 *		pPtr	- Pointer to a previously allocated pool
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Frees all resources allocated during the AllocPool call
 *	
 * NOTES
 *
 * SEE ALSO
 *		NWtlAllocmem(3k), NWtlFreemem(3k), NWtlCreateSemaphore(3k),
 *		NWtlDestroySemaphore(3k), NCPdplAllocPool(3k)	
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPdplFreePool (	ppool_t		*poolPtr )
{
	int			i;
	iopacket_t	*pe, *pn;

	pe = poolPtr->head;

	/*
	 *	Traverse the list of packets in the pool, freeing all.
	 */
	for (i = 0; i < poolPtr->numEntry; i++) {
		if ( pe != (iopacket_t *)NULL ) {
			pn = pe->next;
			kmem_free ( (char *)pe, sizeof (iopacket_t));

#ifdef NUCMEM_DEBUG
			NTR_PRINTF(
				"NUCMEM_FREE: NCPdplFreePool: free packet * at 0x%x, size=0x%x",
				pe, sizeof(iopacket_t), 0 );
#endif NUCMEM_DEBUG

			pe = pn;
		}
		else
			break;	
	}

	NWtlDestroySemaphore( poolPtr->pSemaphore );
	kmem_free ( (char *)poolPtr, sizeof(ppool_t));

#ifdef NUCMEM_DEBUG
	NTR_PRINTF(
		"NUCMEM_FREE: NCPdplFreePool: free ppool_t * at 0x%x, size = 0x%x",
		poolPtr, sizeof(ppool_t), 0 );
#endif NUCMEM_DEBUG

	return(SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPdplGetFreePacket.3k)
 * NAME
 *		NCPdplGetFreePacket - Get available packet from pool
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPdplGetFreePacket( channel, packet )
 *		void_t		*channel;
 *		iopacket_t	**packet;
 *
 * INPUT
 *		channel	-	Channel structure that will yield the pool pointer
 *
 * OUTPUT
 *		packet	-	Pointer to packet structure that can be used by the
 *					caller to pre-format an NCP request.
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Performs a P operation on the pool semaphore in order to serialize
 *		contexts asking for a packet.  When the P completes, it is guaranteed
 *		that a free packet exists, and that the context can grab it.
 *		
 *
 * NOTES
 *		This function blocks the caller until a packet comes free.
 *		It is assumed that this function will never be called from the
 *		callout list, or by an interrupt handler.
 *
 * SEE ALSO
 *		NCPdplFreePacket(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPdplGetFreePacket_l (	ncp_channel_t	*channel,
						iopacket_t		**packet )
{
	ppool_t			*pool;
	extern	int32	spilInitialized;

	NTR_ENTER(2, channel, packet, 0, 0, 0);

	/*
	 *	Get the pool handle from the channel structure
	 */
	NTR_ASSERT ((channel->tag[0] == 'C') && (channel->tag[1] == 'P'));
	pool = (ppool_t *)channel->packetPool;

	/*
	 *	Lock the pool semaphore to get one
	 */
	NWtlPSemaphore( pool->pSemaphore );

	/* See if while we were asleep we started to close down */
	if( !spilInitialized ) {
		return( NTR_LEAVE(SPI_INACTIVE) );
	}

	*packet = pool->head;

	/*
	 *	Scan the pool for the free packet
	 */
	while (	((*packet)->next != (iopacket_t *)NULL) && 
			((*packet)->status != IOP_FREE))
		*packet = (*packet)->next;		

	/*
	 *	If the packet returned after the scan is not of free
	 *	status, the pool and semaphore are out of sync
	 *	as the semaphore's initial value should be the same as the number
	 *	of packets in the pool.
	 */
	NTR_ASSERT((*packet)->status == IOP_FREE);

	(*packet)->status = IOP_INUSE;

	NTR_TRACE( NVLTT_NCP_GetPacket, *packet, 0, 0, 0 );
	 
	return( NTR_LEAVE( SUCCESS ) );	
}

/*
 * BEGIN_MANUAL_ENTRY(NCPdplFreePacket.3k)
 * NAME
 *		NCPdplFreePacket - Return the packet to the pool free list
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPdplFreePacket( packet )
 *		iopacket_t	*packet;
 *
 * INPUT
 *		packet	- Pointer to packet structure
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *
 * DESCRIPTION
 *		Given a packet pointer aquired via the GetFreePacket function,
 *		places the packet back into a free state and V's the pool semaphore
 *		thus waking up any contexts waiting to get a free packet.
 *
 * NOTES
 *		Assumed to not be called by callout list or interrupt handler
 *
 * SEE ALSO
 *		NCPdplGetFreePacket(3K)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPdplFreePacket (	iopacket_t	*packet )
{
	NTR_ENTER(1, packet, 0, 0, 0, 0);

	packet->status = IOP_FREE;
	/*
	 *	bump the semaphore so if there are any threads blocked on it,
	 *	they'll be scheduled, the first of which will get this one.
	 */
	NWtlVSemaphore_l ( packet->pSemaphore ); 

	NTR_TRACE( NVLTT_NCP_FreePacket, packet, 0, 0, 0 );

	return( NTR_LEAVE( SUCCESS ) );
}

