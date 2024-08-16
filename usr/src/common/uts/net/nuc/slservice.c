/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/slservice.c	1.15"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/slservice.c,v 2.53.2.3 1995/01/05 17:54:53 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: slservice.c
 *	ABSTRACT: Routines for managing generic service calls and forward
 *			  to specific service routines through macro.
 *
 *
 *	Some other things that need to be added:
 *	
 *	Functions declared in this module:
 *	Public functions:
 *	NWslInitService
 *	NWslCreateService
 *	NWslFreeService
 *	NWslGetService
 *	NWslScanService
 *
 */ 

#ifdef _KERNEL_HEADERS
#include <util/param.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/tiuser.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/nucerror.h>
#include <util/cmn_err.h>
#include <net/nuc/nwspiswitch.h>
#include <net/nuc/nwspi.h>

#include <net/nuc/requester.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/ncpiopack.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>
#include <kdrivers.h>
#include <nuctool.h>
#include <sys/nwctypes.h>
#include <slstruct.h>
#include <sys/nucerror.h>
#include <nwspiswitch.h>
#include <nwspi.h>

#include <requester.h>
#include <sys/nwmp.h>
#include <ncpconst.h>
#include <sistructs.h>
#include <ncpiopack.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask NTRM_spil

/*
 *	List head to the generic service structures
 */
void_t			*serviceList;
extern sleep_t	*serviceListSleepLock;
extern sleep_t	*spiTaskListSleepLock;

/*
 *	Forward references
 */
enum NUC_DIAG NWslFindServiceByAddress_l();

/*
 * BEGIN_MANUAL_ENTRY(NWslInitService.3k)
 * NAME
 *		NWslInitService - Initialize the service list
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslInitService()
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *
 * DESCRIPTION
 *		Initialize the SPI service object and it's associated data structure
 *
 * NOTES
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		nucUpperLock
 *	LOCKS HELD WHEN RETURNED:
 *		nucUpperLock
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslInitService()
{
	enum NUC_DIAG ccode;

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	ccode =  NWtlInitSLList ( (SLList **)&serviceList );
	SLEEP_UNLOCK (serviceListSleepLock); 
	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWslCreateService.3k)
 * NAME
 *		NWslCreateService - Create a new service
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslCreateService( serviceName, servPtr )
 *		char		*serviceName;
 *		void_t		**servPtr;
 *
 * INPUT
 *		char		*serviceName;
 *
 * OUTPUT
 *		void_t		**servPtr;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_SERVICE_EXISTS
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *
 * DESCRIPTION
 *		Creates a new service instance node and sets the service name
 *		Adds the newly created node to the list of currently known
 *		services.
 *
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslCreateService (struct netbuf *serviceAddress, SPI_SERVICE_T **servPtr )
{
	enum NUC_DIAG	ccode = SUCCESS;

	/*
	 *	Do a list lookup via NWslFindServiceByAddress
	 *	to ensure that the service is not already in the
	 *	list
	 */
	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	if (NWslFindServiceByAddress_l (serviceAddress, servPtr ) == SUCCESS) {
		SLEEP_UNLOCK (serviceListSleepLock);
		return(SPI_SERVICE_EXISTS);
	}
	SLEEP_UNLOCK (serviceListSleepLock);

	*servPtr = (SPI_SERVICE_T *)kmem_zalloc(sizeof(SPI_SERVICE_T), KM_SLEEP);
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM: NWslCreateService: alloc SPI_SERVICE_T * at 0x%x, size = 0x%x",
                *servPtr, sizeof(SPI_SERVICE_T), 0 );
#endif

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
    ccode = NWtlInitSLList((SLList **)&((*servPtr)->taskList) );
	SLEEP_UNLOCK (spiTaskListSleepLock);

	if ( ccode != SUCCESS ) {
		kmem_free ( (void_t *)(*servPtr), sizeof(SPI_SERVICE_T));	
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWslCreateService: free SPI_SERVICE_T * at 0x%x, size = 0x%x",
                *servPtr, sizeof(SPI_SERVICE_T), 0 );
#endif
		*servPtr = (SPI_SERVICE_T *)NULL;
		return(ccode);
	}

	return(ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWslFreeService.3k)
 * NAME
 *		NWslFreeService - Free a previously allocated service
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslFreeService( serviceName )
 *		char			*serviceName;
 *
 * INPUT
 *		char			*serviceName;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_NO_SUCH_SERVICE
 *
 * DESCRIPTION
 *		Free up the service structure and destroy any associated data
 *		structures.  
 *
 * NOTES
 *		This function assumes that the task list for the service has
 *		already been purged of nodes.
 *
 * SEE ALSO
 *		NWslCreateService(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslFreeService (SPI_SERVICE_T *service )
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_SERVICE_T	*servicePtr;

	if (ccode = NWslFindServiceByAddress_l( service->address, &servicePtr )) {
		return(ccode);
	}

	/*
	 *	We'll assume that the current list pointer is on this node
	 *	as we just searched for it.  There should be no other contexts
	 *	messing with the list at this time.
	 */
	NWtlDeleteNodeSLList( serviceList );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	/*
	 *	Clean up any structures associated with this node
	 */
	NWtlDestroySLList(servicePtr->taskList);
	SLEEP_UNLOCK (spiTaskListSleepLock);

	if (servicePtr->address) {
		kmem_free ( (char *)servicePtr->address->buf, MAX_ADDRESS_SIZE);
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWslFreeService: free netbuf buffer * at 0x%x, size = 0x%x",
                servicePtr->address->buf, MAX_ADDRESS_SIZE, 0 );
#endif
		kmem_free ( (char *)servicePtr->address, sizeof (struct netbuf));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWslFreeService: free netbuf  * at 0x%x, size = 0x%x",
                servicePtr->address, sizeof (struct netbuf), 0 );
#endif
	}

	kmem_free ( (char *)servicePtr, sizeof(SPI_SERVICE_T));	
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWslFreeService: free SPI_SERVICE_T  * at 0x%x, size = 0x%x",
                servicePtr, sizeof (SPI_SERVICE_T), 0 );
#endif

	return(ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NWslGetService.3k)
 * NAME
 *		NWslGetService
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslGetService( serviceName, servPtr )
 *		char	*serviceName;
 *		void_t	**servPtr;
 *
 * INPUT
 *		char			*serviceName;
 *
 * OUTPUT
 *		void_t	**servPtr
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_NO_SUCH_SERVICE
 *
 * DESCRIPTION
 *		Given a name of a service, returns the object pointer to the 
 *		service structure.
 *
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		nucUpperLock
 *
 *	LOCKS HELD WHEN RETURNED:
 *		nucUpperLock
 *
 *
 * SEE ALSO
 *		NWslFindServiceByName(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslGetService (struct netbuf *serviceAddress, SPI_SERVICE_T **servPtr )
{
	enum NUC_DIAG	ccode = SUCCESS;

	*servPtr = (SPI_SERVICE_T *)NULL;

	/*
	 *	Search for the requested service by address.  Return success and the
	 *	SPI_SERVICE_T pointer if found.
	 */
	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	ccode = NWslFindServiceByAddress_l ( serviceAddress, servPtr );
	SLEEP_UNLOCK (serviceListSleepLock);
	if (ccode == SUCCESS)
		return(SUCCESS);
	else
		return(FAILURE);

}

/*
 * BEGIN_MANUAL_ENTRY(NWslScanService.3k)
 * NAME
 *		NWslScanService - Scan Service list
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslScanService( lastName, mask, servPtr )
 *		char	**lastName;
 *		uint32	*mask;
 *		SPI_SERVICE_T	**servPtr;
 *
 * INPUT
 *		char	**lastName;
 *		uint32	*mask;
 *
 * OUTPUT
 *		SPI_SERVICE_T	**servPtr;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_NO_MORE_SERVICE
 *
 * DESCRIPTION
 *		Provides an iterative method of scannnig the service data 
 *		structure for specific service nodes, or for listing all. 
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWslScanService (struct netbuf **lastServiceAddress, uint32 *mask,
				SPI_SERVICE_T **servPtr )
{
	enum NUC_DIAG	ccode = SUCCESS;

    NTR_ENTER(3, lastServiceAddress, mask, servPtr, 0, 0);
#ifdef DEBUG
	NTR_ASSERT(SLEEP_LOCKOWNED(serviceListSleepLock));
#endif
	/*
	 *	If name is null, return the first entry
	 */
	if ( *lastServiceAddress == NULL ) {
		NWtlRewindSLList( serviceList );
		if (NWtlGetContentsSLList( serviceList, (void_t *)servPtr )) {
			return( NTR_LEAVE(SPI_NO_MORE_SERVICE));
		}
		*lastServiceAddress = (*servPtr)->address;
	}
	else {
		/*
		 *	Find the previous one, advance the list pointer
		 *	then extract the new current entry
		 */
		if (NWslFindServiceByAddress_l ( *lastServiceAddress, servPtr ) ==
				SUCCESS) {
			if (NWtlNextNodeSLList( serviceList )) {
				return( NTR_LEAVE (SPI_NO_MORE_SERVICE));
			}

			if (NWtlGetContentsSLList(serviceList, (void_t *)servPtr))
				ccode = SPI_NO_MORE_SERVICE;
			else
				*lastServiceAddress = (*servPtr)->address;
		}
		else
			ccode = SPI_NO_MORE_SERVICE;
	}


	return( NTR_LEAVE(ccode) );
}


/*
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		serviceListSleepLock
 *
 *	LOCKS HELD WHEN RETURNED:
 *		serviceListSleepLock
 *
 */

enum NUC_DIAG
NWslFindServiceByAddress_l (struct netbuf *address, SPI_SERVICE_T **servicePtr)
{
	register int i;
	int found;
	enum NUC_DIAG	ccode = SPI_NO_SUCH_SERVICE;

	NTR_ENTER(2, address, servicePtr, 0, 0, 0);

	NWtlRewindSLList( serviceList );

	while(!(NWtlGetContentsSLList( serviceList, (void_t *)servicePtr ))) {

		found = TRUE;
		if (address->len != (*servicePtr)->address->len)
			found = FALSE;

		else for (i = 0; i < address->len; i++) {
			if (address->buf[i] != (*servicePtr)->address->buf[i])
				found = FALSE;
		}

		if (found == TRUE) {
			return( NTR_LEAVE(SUCCESS) );
		} else {
			if (NWtlNextNodeSLList(serviceList))
				return( NTR_LEAVE(ccode) ); 
		}
	}

	return( NTR_LEAVE(ccode) );
}

enum NUC_DIAG
NWslSetServiceAddress (SPI_SERVICE_T *servicePtr, struct netbuf *address)
{
	if (address == NULL)
		return(FAILURE);

	servicePtr->address = (struct netbuf *)kmem_alloc (sizeof(struct netbuf),
							KM_NOSLEEP);
	if (servicePtr->address == NULL)
		return(SPI_CLIENT_RESOURCE_SHORTAGE);

#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM: NWslSetServiceAddress: alloc netbuf * at 0x%x, size = 0x%x",
                servicePtr->address, sizeof(struct netbuf), 0 );
#endif

	servicePtr->address->buf = (char *)kmem_alloc (MAX_ADDRESS_SIZE, KM_NOSLEEP);
	if (servicePtr->address->buf == NULL) {
		kmem_free ((void_t *)servicePtr->address, sizeof(struct netbuf));
		servicePtr->address = NULL;
		return(SPI_CLIENT_RESOURCE_SHORTAGE);
	}

#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM: NWslSetServiceAddress: alloc netbuf buf * at 0x%x, size = 0x%x",
                servicePtr->address->buf, MAX_ADDRESS_SIZE, 0 );
#endif

	bcopy(address->buf, servicePtr->address->buf, address->len);
	servicePtr->address->len = address->len;
	servicePtr->address->maxlen = address->maxlen;

	return(SUCCESS);
}

enum NUC_DIAG
NWslGetServerContext (SPI_SERVICE_T *service,
						struct getServerContextReq *context)
{
	context->majorVersion =
		((ncp_server_t *)(service->protoServicePtr))->majorVersion;
	context->minorVersion =
		((ncp_server_t *)(service->protoServicePtr))->minorVersion;

	return (SUCCESS);
}
