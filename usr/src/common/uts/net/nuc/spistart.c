/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/spistart.c	1.35"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/spistart.c,v 2.55.2.12 1995/02/13 16:54:59 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE: spigeneral.c
 *	ABSTRACT: SPI routines called by the management portal (NWMP).
 *			  and it's associated libraries.
 *
 *	Functions declared in this module:
 *
 *	NWsiStartSPI
 *	NWsiStopSPI
 *	NWsiInitSPI
 *	NWsiFreeSPI
 *	NWsiCreateService
 *	NWsiDeleteService
 *	NWsiOpenService
 *	NWsiCloseService
 *	NWsiAuthenticate
 *	NWsiScanServices
 *	NWsiScanServiceTasks
 *	NWsiRegisterRaw
 *	NWsiRelinquishRawToken
 *	NWsiRaw
 *	NWsiSetPrimaryService
 *	NWsiGetServiceMessage
 *
 */ 

#ifdef _KERNEL_HEADERS
#include <net/tiuser.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/param.h>

#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/nwspiswitch.h>
#include <net/nuc/nwspi.h>
#include <net/nuc/spimacro.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/requester.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/nwtypes.h>
#include <net/nuc/ncpiopack.h>
#include <net/nw/ntr.h>
#include <net/nuc/ipxengtune.h>
#include <net/nuc/nuc_hier.h>
#include <net/nuc/nuc_prototypes.h>


#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>

#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>
#include <sys/nwspiswitch.h>
#include <sys/nwspi.h>
#include <sys/spimacro.h>
#include <sys/nucerror.h>
#include <sys/nwmp.h>
#include <sys/slstruct.h>
#include <sys/nwportable.h>
#include <sys/nwctypes.h>
#include <sys/spilcommon.h>
#include <sys/slstruct.h>
#include <sys/requester.h>
#include <sys/ncpconst.h>
#include <sys/sistructs.h>
#include <sys/nwtypes.h>
#include <sys/ncpiopack.h>


#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_spil

#define ENABLED (int)-1
#define DISABLED (int)0

/*
 *	External data structures from config file
 */
extern SPI_SWITCH_T	SPISwitch[];

extern lock_t		*nucLock;
extern lock_t		*criticalSectLock;
extern rwlock_t		*nucUpperLock;
extern sv_t			*spilInternalTaskAuthenticationQueueSV;
extern sv_t			*spilInternalTaskAuthenticationAddressSV;
extern sleep_t		*serviceListSleepLock;
extern sleep_t		*spiTaskListSleepLock;

/*
 *	External data structures from space/tune file
 */
extern long spiState;

/*
 *	SPI Global variables
 */
extern void_t	*serviceList;
extern void_t	*NWslTaskFreeList;
extern size_t	*NWslTaskFreeListLength;

/*
 *	Forward references
 */
enum NUC_DIAG NWsiInitSPI();
enum NUC_DIAG NWsiFreeSPI();
enum NUC_DIAG NWsiSetPrimaryService();
enum NUC_DIAG NWsiCloseServiceWithTaskPtr_l();
enum NUC_DIAG NWsiRegisterRaw();

/*
 * BEGIN_MANUAL_ENTRY(NWsiStartSPI.3k)
 * NAME
 *		NWsiStartSPI - Enable the Service Protocol Interface Layer.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiStartSPI()
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		Nothing
 *
 * DESCRIPTION
 *		Enable SPI layer
 *
 * NOTES
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
void_t
NWsiStartSPI()
{
	pl_t            pl;
	NTR_ENTER(0, 0, 0, 0, 0, 0);

	NWsiInitSPI();
	pl = RW_WRLOCK (nucUpperLock, plstr);
	spiState = SPI_LAYER_ACTIVE;
	RW_UNLOCK (nucUpperLock, pl);
	NTR_LEAVE( 0 );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiStopSPI.3k)
 * NAME
 *		NWsiStopSPI - Disable the Service Protocol Interface Layer.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiStopSPI()
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		Nothing
 *
 * DESCRIPTION
 *		Disable the spi layer from allowing any calls down to the lower
 *		layers.   This allows orderly teardown of the stack.
 *		
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiStopSPI()
{
	SPI_SERVICE_T   *servPtr;
	SPI_TASK_T	*taskHandle;
	extern void_t	*serviceList;
	enum NUC_DIAG	ccode;
	pl_t            pl;
	pl_t		s;
	int 		i, j;
	extern struct ipxEngTuneStruct ipxEngTune;
	extern ipxClient_t clientList[];
	extern sv_t *NCPbroadcastMessageQueueSV;
	extern sv_t *spilInternalTaskAuthenticationAddressSV;
	extern sv_t *spilInternalTaskAuthenticationQueueSV;


	NTR_ENTER(0, 0, 0, 0, 0, 0);

	pl = RW_WRLOCK (nucUpperLock, plstr);
	if( spiState == SPI_LAYER_ACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		NWsiFreeSPI();
		pl = RW_WRLOCK (nucUpperLock, plstr);
		spiState = SPI_LAYER_INACTIVE;
	}
	RW_UNLOCK (nucUpperLock, pl);


	/* destroy serviceList and all entries */
	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	ccode = NWtlRewindSLList( serviceList );
	(void)NCPsiFreeNCP();	/* Keep async events away from bcast queue. */
	while((ccode = NWtlGetContentsSLList( serviceList, (void_t *)&servPtr ))
				== SUCCESS ){
		SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
		ccode = NWtlRewindSLList( servPtr->taskList );
		while((ccode = NWtlGetContentsSLList(servPtr->taskList,
					(void_t *)&taskHandle)) == SUCCESS){
			SLEEP_LOCK (taskHandle->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	   		taskHandle->mode |= SPI_TASK_DELETED;

			/*
			 * It is safe to dispose of the task when either all
			 * references are gone, or there is only one remaining
			 * reference, and it is from an async event handler
			 * registration.  NCPsiDestroyTask destroys the
			 * latter.
			 */
			pl = LOCK (taskHandle->spiTaskLock, plstr);
			while ((taskHandle->useCount > 0 &&
						!(taskHandle->mode & SPI_ASYNC_REGISTERED)) ||
					taskHandle->useCount > 1) {
#ifdef NUC_DEBUG
	cmn_err (CE_WARN,
		"NWsiStopSPI to snooze for SPI_TASK_T at 0x%X, useCount = %d, "
			"mode = 0x%x\n",
			taskHandle, taskHandle->useCount, taskHandle->mode);
#endif
				SLEEP_UNLOCK (taskHandle->spiTaskSleepLock);
				SV_WAIT (taskHandle->spiTaskSV, primed,
					taskHandle->spiTaskLock);
				SLEEP_LOCK (taskHandle->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
				LOCK (taskHandle->spiTaskLock, plstr);
			}
			UNLOCK (taskHandle->spiTaskLock, pl);
			ccode = NCPsiDestroyTask( taskHandle->protoTaskPtr );
			ccode = NWslFreeTask( taskHandle );
			ccode = NWtlDeleteNodeSLList( servPtr->taskList );
		}
		SLEEP_UNLOCK (spiTaskListSleepLock);
		ccode = NCPsilFreeServer( servPtr->protoServicePtr );

		ccode = NWslFreeService( servPtr );

	}

	ccode = NWtlDestroySLList( serviceList );

	serviceList = NULL;
	SLEEP_UNLOCK (serviceListSleepLock);

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	NWtlRewindSLList(NWslTaskFreeList);
	NWslCleanTaskFreeList(B_TRUE);
	(void)NWtlDestroySLList(NWslTaskFreeList);
	NWslTaskFreeList = NULL;
	SLEEP_UNLOCK (spiTaskListSleepLock);

	if( spilInternalTaskAuthenticationQueueSV ) {
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWsiStopSPI: free sv_t * at 0x%x, size = 0x%x",
                spilInternalTaskAuthenticationQueueSV, sizeof(sv_t), 0 );
#endif

		pl = LOCK(nucLock, plstr);
		if (SV_BLKD (spilInternalTaskAuthenticationQueueSV)) {
			UNLOCK(nucLock, pl);
			SV_BROADCAST (spilInternalTaskAuthenticationQueueSV, 0);
		} else {
			UNLOCK(nucLock, pl);
		}

		SV_DEALLOC( spilInternalTaskAuthenticationQueueSV );
		spilInternalTaskAuthenticationQueueSV = NULL;
	}

	if( spilInternalTaskAuthenticationAddressSV ) {
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWsiStopSPI: free sv_t * at 0x%x, size = 0x%x",
                spilInternalTaskAuthenticationAddressSV, sizeof(sv_t), 0 );
#endif

		pl = LOCK(nucLock, plstr);
		if (SV_BLKD (spilInternalTaskAuthenticationAddressSV)) {
			UNLOCK(nucLock, pl);
			SV_BROADCAST (spilInternalTaskAuthenticationAddressSV, 0);
		} else {
			UNLOCK(nucLock, pl);
		}
		SV_DEALLOC( spilInternalTaskAuthenticationAddressSV );
		spilInternalTaskAuthenticationAddressSV = NULL;
	}

	if( NCPbroadcastMessageQueueSV ) {
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWsiStopSPI: free sv_t * at 0x%x, size = 0x%x",
                NCPbroadcastMessageQueueSV, sizeof(sv_t), 0 );
#endif
		while (SV_BLKD (NCPbroadcastMessageQueueSV)) {
			SV_BROADCAST (NCPbroadcastMessageQueueSV, 0);
		}
		SV_DEALLOC( NCPbroadcastMessageQueueSV );
		NCPbroadcastMessageQueueSV = NULL;
	}

	s = LOCK(criticalSectLock, NUCPLHI);
	for (i = 0; i < ipxEngTune.maxClients; i++) {
		for( j=0; j<clientList[i].numTasks; j++ ) {
			if( clientList[i].taskList[j].callOutID ) {
				clientList[i].taskList[j].state |= IPX_TASK_TIMEDOUT;
				untimeout( clientList[i].taskList[j].callOutID );
				clientList[i].taskList[j].callOutID = 0;
				NWtlVPsuedoSema(clientList[i].taskList[j].syncSemaphore);
			}
		}
	}
	UNLOCK(criticalSectLock, s);

	/* Refuse to wait long */
	ipxEngTune.timeoutQuantumLimit = 600;

	return( NTR_LEAVE( SUCCESS ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiInitSPI.3k)
 * NAME
 *		NWsiInitSPI - Initialize the Service Protocol Interface Layer.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiInitSPI()
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		Nothing
 *
 * DESCRIPTION
 *		Initialize all parameters needed in the layer
 *
 * NOTES
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiInitSPI()
{
	int				i;
	enum NUC_DIAG	ccode;
	pl_t			pl;

	NTR_ENTER(0, 0, 0, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if ( spiState == SPI_LAYER_ACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_LAYER_ACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	ccode = NWslInitService();

	pl = RW_WRLOCK (nucUpperLock, plstr);

	for (i = 0; i < NUM_SPROTO; i++) {
		if (SPISwitch[i].mode == ENABLED)
			ccode = SPI_INIT(SPISwitch[i].spi_ops);
	}

	RW_UNLOCK (nucUpperLock, pl);

	if ((spilInternalTaskAuthenticationAddressSV = SV_ALLOC (KM_NOSLEEP))
				== NULL) {
			cmn_err(CE_PANIC, "NWsiInitSPI: spilInternalTaskAuthenticationAddressSV alloc failed");
			return (NTR_LEAVE (SPI_GENERAL_FAILURE));
	}
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM: NWsiInitSPI: alloc sv_t * at 0x%x, size = 0x%x",
                spilInternalTaskAuthenticationAddressSV, sizeof(sv_t), 0 );
#endif

	if ((spilInternalTaskAuthenticationQueueSV = SV_ALLOC (KM_SLEEP))
				== NULL) {
			cmn_err(CE_PANIC, "NWsiInitSPI: spilInternalTaskAuthenticationQueueSV alloc failed");
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWsiInitSPI: free sv_t * at 0x%x, size = 0x%x",
                spilInternalTaskAuthenticationAddressSV, sizeof(sv_t), 0 );
#endif
			SV_DEALLOC( spilInternalTaskAuthenticationAddressSV );
			return (NTR_LEAVE (SPI_GENERAL_FAILURE));
	}
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM: NWsiInitSPI: alloc sv_t * at 0x%x, size = 0x%x",
                spilInternalTaskAuthenticationQueueSV, sizeof(sv_t), 0 );
#endif
	
	return( NTR_LEAVE( ccode ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiFreeSPI.3k)
 * NAME
 *		NWsiFreeSPI -	Free data structures allocated for the SPI
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiFreeSPI()
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		Nothing
 *
 * DESCRIPTION
 *
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiFreeSPI()
{
	register int	i;
	pl_t			pl;

	NTR_ENTER(0, 0, 0, 0, 0, 0);

	pl = RW_WRLOCK (nucUpperLock, plstr);

	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	for (i = 0; i < NUM_SPROTO; i++) {
		if (SPISwitch[i].mode == ENABLED)
			SPI_FREE(SPISwitch[i].spi_ops);
	}

	RW_UNLOCK (nucUpperLock, pl);

	return( NTR_LEAVE( SUCCESS ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiCreateService.3k)
 * NAME
 *		NWsiCreateService - Create a service instance
 *
 * SYNOPSIS
 *		module: spigeneral.c
 *		enum NUC_DIAG
 *		NWsiCreateService( serviceName, credStruct, spDomain, 
 *							tpDomain, address, addressLength)
 *		char	*serviceName;
 *		void_t	*credStruct;
 *		int32	spDomain;
 *		int32	tpDomain;
 *		void_t	*address;
 *		int32	addressLength;
 *		
 *
 * INPUT
 *		serviceName	- Name service will be known as
 *		credStruct	- Credentials structure
 *		spDomain	- Service protocol type
 *		tpDomain	- Transport protocol type
 *		address		- Transport address of target endpoint
 *		addressLength - Length in bytes of the address 
 *		serviceFlags - attributes of this service
 *			SERVICE_IS_LOCAL - service resides on the local LAN
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SPI_SUCCESS			- Request successful
 *		SPI_SERVICE_EXISTS	- Name currently being used 
 *		SPI_FAILURE (-1)		- Memory allocation failure/misc error
 *		
 *
 * DESCRIPTION
 *		Registers a serviceName in the SPI layer to be used as an
 *		access point for clients wishing to authenticate and request
 *		services from.
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
 *		NWsiDestroyService(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiCreateService (comargs_t *comargs, int32 spDomain,
	int32 tpDomain, uint32 serviceFlags)
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_SERVICE_T	*gService, *gService1;
	void_t		*pService;
	SPI_OPS_T	*ops;
	pl_t			pl;

	NTR_ENTER(4, comargs, spDomain, tpDomain, serviceFlags, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	/*
	**	Make sure SPI is running before allowing this to happen
	*/
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	/*
	 *	Check the specified service protocol to ensure that
	 *	it is enabled in this kernel
	 */
	if ( SPISwitch[spDomain].mode != ENABLED ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INVALID_SPROTO ) );
	}

	ops = SPISwitch[spDomain].spi_ops;

	RW_UNLOCK (nucUpperLock, pl);

	/*
	 *	Allocate memory for this service structure, and assign
	 *	the operations structure.
	 */
	ccode = NWslCreateService( comargs->address, &gService );
	if (ccode)
		return( NTR_LEAVE( ccode ) );

	/*
	 *	Call the specific service routine now to do it's work
	 */
	ccode = SPI_SCREATE( ops, comargs->address,
		comargs->credential, tpDomain, &pService );

	if ( ccode == SUCCESS ) {

		/* Unable to lock SPI_SCREATE so we'll check again to see
			if someone else has added this service to the list while we
			were sleeping.
		*/
		SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
		if( NWslFindServiceByAddress_l ( comargs->address, &gService1 ) 
			== SUCCESS ) {
			SLEEP_UNLOCK (serviceListSleepLock);
      		NTR_PRINTF("NWsiCreateService: Rechecking address and found one!!!",
                0, 0, 0 );
			kmem_free ( gService, sizeof (SPI_SERVICE_T));
#ifdef NUCMEM_DEBUG
        	NTR_PRINTF("NUCMEM_FREE: NWsiCreateService: free SPI_SERVICE_T * at 0x%x, size = 0x%x",
                gService, sizeof(SPI_SERVICE_T), 0 );
#endif
			return( NTR_LEAVE( SPI_SERVICE_EXISTS ) );
		}
		SLEEP_UNLOCK (serviceListSleepLock);

		pl = RW_WRLOCK (nucUpperLock, plstr);

		NWslSetServiceProtoHandle( gService, pService );
		NWslSetServiceOps( gService, (void_t *)ops );
		gService->transportProtocol = tpDomain;
		gService->serviceProtocol = spDomain;
		/* copy netbuf */
		if ((ccode = NWslSetServiceAddress(gService, comargs->address))
					!= SUCCESS) {
			RW_UNLOCK (nucUpperLock, pl);
			kmem_free ( gService, sizeof (SPI_SERVICE_T));
#ifdef NUCMEM_DEBUG
        	NTR_PRINTF("NUCMEM_FREE: NWsiCreateService: free SPI_SERVICE_T * at 0x%x, size = 0x%x",
                gService, sizeof(SPI_SERVICE_T), 0 );
#endif
			return( NTR_LEAVE( ccode ) );
		}
		gService->flags = serviceFlags;
		RW_UNLOCK (nucUpperLock, pl);
		SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
		if ((ccode = NWtlAddToSLList(serviceList, gService)) != SUCCESS) {
			NWslFreeService( gService );
		}
		SLEEP_UNLOCK (serviceListSleepLock);
#ifdef NUC_DEBUG
		cmn_err (CE_WARN, 
			"NWsiCreateService: SPI_SERVICE_T at 0x%X\n", gService );
#endif
	} else {
		kmem_free ( gService, sizeof (SPI_SERVICE_T));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NWsiCreateService: free SPI_SERVICE_T * at 0x%x, size = 0x%x",
                gService, sizeof(SPI_SERVICE_T), 0 );
#endif
	}

	return( NTR_LEAVE( ccode ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiOpenService.3k)
 * NAME
 *		NWsiOpenService - Open a service protocol instance (create a task)
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiOpenService( credPtr, serviceName, mode )
 *		void_t	*credPtr;
 *		char	*serviceName;
 *		bmask_t	mode;
 *
 * INPUT
 *		void_t	*credPtr;
 *		char	*serviceName;
 *		bmask_t	mode;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		Create a task instance for this client for this service.
 *
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiOpenService (comargs_t *comargs, bmask_t mode, uint32 flags )
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_OPS_T	*ops;
	SPI_SERVICE_T	*gService;
	void_t		*pService;
	SPI_TASK_T	*gTask;
	ncp_task_t	*pTask;
	void_t		*taskList;
	SPI_TASK_T	*tmpTask;
	pl_t		pl;

	NTR_ENTER(3, comargs, mode, flags, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Make sure SPI is running before allowing this to happen
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	if (NWslGetService( comargs->address, &gService ))
		return( NTR_LEAVE(  SPI_NO_SUCH_SERVICE  ) );

	NWslGetServiceTaskList( gService, &taskList );

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (ccode = NWslCreateTask( taskList, comargs->credential, &gTask )) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		SLEEP_UNLOCK (serviceListSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	/*
	 * Grab the list lock now to avoid hierarchy problems later.  It would
	 * appear that the task cannot yet be taken out of use by another
	 * thread of control, but we are being cautious about the implications
	 * of registering gTask with an async event handler in SPI_SOPEN.
	 */
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if ( NWslSetTaskInUse_l ( gTask ) ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		SLEEP_UNLOCK (spiTaskListSleepLock);
 		SLEEP_UNLOCK (serviceListSleepLock);
       return( NTR_LEAVE( SPI_TASK_DRAINING ) );
	}

	NWslSetTaskService( gTask, gService );
	NWslGetServiceOps( gService, &ops );
	NWslGetServiceProtoHandle( gService, &pService );

	ccode = SPI_SOPEN(ops, pService, comargs->credential, mode, gTask, &pTask);
	if (ccode != SUCCESS) {
		/*
		 *	Problems in the sproto
		 */
		NWslFreeTask( gTask );
		SLEEP_UNLOCK (spiTaskListSleepLock);
 		SLEEP_UNLOCK (serviceListSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	/*
	 * TODO:  this is racy.  We register the gTask with an async
	 * event handler in SPI_OPEN, then finish setting up the
	 * task.  Unfortunately, channelPtr comes back from SPI_OPEN,
	 * so the fix is nontrivial.
	 */
	NWslSetTaskProtoHandle( gTask, pTask );
	if (flags & NWC_OPEN_PUBLIC) {
		((ncp_channel_t *)pTask->channelPtr)->uPublicState = NWC_CONN_PUBLIC;
	} else  if (flags & NWC_OPEN_PRIVATE) {
		((ncp_channel_t *)pTask->channelPtr)->uPublicState = NWC_CONN_PRIVATE;
	}
	NWslSetTaskMode( gTask, SPI_TASK_CONNECTED );
	NWtlAddToSLList( gService->taskList, gTask );

	if ((comargs->credential->flags & NWC_OPEN_PUBLIC) &&
			getPrimaryService(comargs, &tmpTask, gTask, TRUE) == FAILURE) {
		gTask->mode |= SPI_PRIMARY_SERVICE;
	}
	gTask->mode |= SPI_ASYNC_REGISTERED;

	/*
	 * The SPIL task has been successfully created.  Note that we leave it
	 * held by the registered async event handler.
	 */
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);
	SLEEP_UNLOCK (spiTaskListSleepLock);
 	SLEEP_UNLOCK (serviceListSleepLock);

	NTR_LEAVE((uint_t) gTask);

	return( SUCCESS );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiCloseService.3k)
 * NAME
 *		NWsiCloseService - Close a service instance (destroy a task)
 *
 * SYNOPSIS
 *		enum NUC_DIAG 
 *		NWsiCloseService( credPtr, serviceName )
 *		void_t	*credPtr;
 *		char	*serviceName;
 *
 * INPUT
 *		void_t	*credPtr;
 *		char	*serviceName;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_FAILURE
 *		SPI_INACTIVE
 *		SPI_NO_SUCH_SERVICE
 *
 * DESCRIPTION
 *
 * NOTES
 *		This function will free the SPI_TASK_T and call the Service Protocol
 *		to perform any required cleanup necessary for this task if the 
 *		SPIL resource count is zero.  
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG 
NWsiCloseService (comargs_t *comargs, uint32 flags)
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	SPI_TASK_T	*newPrimaryTask = NULL;
	ncp_task_t	*protoHandle;
	ncp_channel_t	*channel;
	extern uint32 connectionReference;
	pl_t			pl;
	
	NTR_ENTER(2, comargs, flags, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		return(NTR_LEAVE(SPI_INACTIVE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	if (NWslGetService(comargs->address, &gService)) {
		return(NTR_LEAVE(SPI_NO_SUCH_SERVICE));
	}

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l (gService->taskList, comargs->credential, &gTask, FALSE))	{
		SLEEP_UNLOCK (spiTaskListSleepLock);
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);

	NWslGetTaskProtoHandle(gTask, &protoHandle);

	CHECK_CONN_REFERENCE(ccode, comargs->connectionReference,
		((ncp_channel_t *)protoHandle->channelPtr)->connectionReference);

	if (ccode) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		SLEEP_UNLOCK (spiTaskListSleepLock);
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(ccode));
	}

	/*
	 * Note that we do not have to set gTask in Use, since it is held by
	 * the registered async event handler.
	 */

	if (gTask->mode & SPI_PRIMARY_SERVICE) {
		getPrimaryService(comargs, &newPrimaryTask, gTask, TRUE);
		if (newPrimaryTask == NULL && !flags) {
			/*
			 *  If there's not another connection that can be
			 *  made into the primary connection, then don't delete
			 *  the current primary connection.
			 *
			 *  It should be as if the connection entry were deleted
			 *  and re-created so quickly that no process should ever
			 *  find the primary connection missing.
			 */

			channel = (ncp_channel_t *)protoHandle->channelPtr;
			SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);
			NCPspLogout_l (channel);

			gTask->mode &= ~(SPI_TASK_AUTHENTICATED);

			channel->connectionReference = connectionReference++;
			channel->referenceCount = 0;
			channel->licenseState = NWC_NOT_LICENSED;
			channel->authenticationState = NWC_AUTH_STATE_NONE;
			SLEEP_UNLOCK (channel->connSleepLock);
			SLEEP_UNLOCK (gTask->spiTaskSleepLock);
			SLEEP_UNLOCK (spiTaskListSleepLock);
			SLEEP_UNLOCK (serviceListSleepLock);
			return( NTR_LEAVE( SUCCESS ) );
		}
	}

	/*
	 * Here we give up the reference of the async event handler.  This is
	 * safe, because we hold the list lock.
	 */
	gTask->mode &= ~SPI_ASYNC_REGISTERED;
	NWslSetTaskNotInUse_l (gTask);
	ccode = NWsiCloseServiceWithTaskPtr_l (gTask);
	if (!ccode && newPrimaryTask) {
		newPrimaryTask->mode |= SPI_PRIMARY_SERVICE;
	}
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);
	SLEEP_UNLOCK (spiTaskListSleepLock);
	SLEEP_UNLOCK (serviceListSleepLock);

	return(NTR_LEAVE(ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiCloseServiceWithTaskPtr.3k)
 * NAME
 *		NWsiCloseServiceWithTaskPtr - Close a service instance (destroy a task)
 *
 * SYNOPSIS
 *		enum NUC_DIAG 
 *		NWsiCloseServiceWithTaskPtr( credPtr, serviceName )
 *		void_t	*credPtr;
 *		char	*serviceName;
 *
 * INPUT
 *		void_t	*credPtr;
 *		char	*serviceName;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_FAILURE
 *		SPI_INACTIVE
 *		SPI_NO_SUCH_SERVICE
 *
 * DESCRIPTION
 *
 * NOTES
 *		This function will free the SPI_TASK_T and call the Service Protocol
 *		to perform any required cleanup necessary for this task if the 
 *		SPIL resource count is zero.  If the resource count is not zero
 *		the SPROTO layer is called anyway but the SPI_TASK_T remains 
 *		allocated in support of future calls to NWsiCloseNode to free the
 *		associated resources.  The SPI_TASK_T will be freed when the 
 *		resource count goes to zero.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		spiTaskSleepLock
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		spiTaskSleepLock
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG 
NWsiCloseServiceWithTaskPtr_l (SPI_TASK_T *gTask)
{
	SPI_SERVICE_T	*gService;
	void_t		*pHandle;
	SPI_OPS_T	*ops;
	SPI_TASK_T	*tempTaskPtr;
	pl_t			pl;
	
	NTR_ENTER(2, gTask, opl, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	
	/*
	 *	Make sure SPI is running before allowing this to happen
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	/*
	 *	Unlink this task's structures from this service provider.  Ignore
	 *	the fact that the task may have been previously unlinked.
	 */
	gService = gTask->spiServicePtr;
	NWtlRewindSLList( gService->taskList );
	while (NWtlGetContentsSLList (gService->taskList,
				(void_t *)&tempTaskPtr ) == 0 ) {
		if ( tempTaskPtr == gTask ) {
			NWtlDeleteNodeSLList( gService->taskList );
			break;
		}
		if ( NWtlNextNodeSLList( gService->taskList ) != SUCCESS )
			break;
	}


	/*
	 * It is safe to dispose of the task when either all references are
	 * gone, or there is only one remaining reference, and it is from an
	 * async event handler registration.  SPI_SCLOSE destroys the latter.
	 * Note that we assume that useCount can be updated without taking the
	 * spiTaskListSleepLock, which we hold.
	 */
	gTask->mode |= SPI_TASK_DELETED;
	pl = LOCK (gTask->spiTaskLock, plstr);
	while ((gTask->useCount > 0 && !(gTask->mode & SPI_ASYNC_REGISTERED)) ||
			gTask->useCount > 1) {
#ifdef NUC_DEBUG
	cmn_err (CE_WARN,
		"NWsiCloseService w/ptr to snooze for SPI_TASK_T at 0x%X, useCount = %d, "
			"mode = 0x%x\n", gTask, gTask->useCount, gTask->mode);
#endif
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		SV_WAIT (gTask->spiTaskSV, primed, gTask->spiTaskLock);
		SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
		LOCK (gTask->spiTaskLock, plstr);
	}
	UNLOCK (gTask->spiTaskLock, pl);


	/*
	 *	Call the Service Protocol Layer to free any resources that can
	 *	be freed at this time.  This may happen several times and the
	 *	diagnostic from SPI_SCLOSE, if any will be ignored.
	 */
	NWslGetServiceOps( gService, &ops );
	NWslGetTaskProtoHandle( gTask, &pHandle );
	SPI_SCLOSE( ops, pHandle ); 
	gTask->mode &= ~SPI_ASYNC_REGISTERED;

	/*
	 *	If resources are still in use by this SPI_TASK_T, insure that
	 *	the mode is set to SPI_TASK_DRAINING and return SUCCESS.
	 *	This function will be called again by NWsiCloseNode after all
	 *	resources are freed to free the SPI_TASK_T structure storage.
	 *	If a task goes into SPI_TASK_DRAINING mode becuase of an
	 *	error and the task has no resources allocated, this function 
	 *	will be called by the GetTask search function to clean up.
	 */

	if ( gTask->resourceCount ) {
		gTask->mode |= (SPI_TASK_DRAINING);
		return( NTR_LEAVE( SUCCESS ) );
	}


#ifdef NUC_DEBUG
	cmn_err (CE_WARN, 
		"NWsiCloseService w/ptr will now smoke SPI_TASK_T at 0x%X\n", gTask );
#endif

	(void)NWtlSeekToEndSLList(NWslTaskFreeList);
	gTask->freeTime = lbolt;
	(void)NWtlAddToSLList(NWslTaskFreeList, gTask);
	NWslTaskFreeListLength += 1;

	/*
	 *	If this is the last task for this SPI_SERVICE_T, NWsiDeleteService
	 *	will delete the SPI_SERVICE_T.  This is done in case a server is 
	 *	brought down and back up with a different address.
	 */

	return (NTR_LEAVE (SUCCESS));
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiAuthenticate.3k)
 * NAME
 *		NWsiAuthenticate -	Authenticate a previously connected task.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiAuthenticate( credPtr, serverName, userName, authenticationKey )
 *		void_t	*credPtr;
 *		char	*serverName;
 *		char	*userName;
 *		char	*authenticationKey;
 *
 * INPUT
 *		void_t	*credPtr;
 *		char	*serverName;
 *		char	*userName;
 *		char	*authenticationKey;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_FAILURE
 *		SPI_INACTIVE
 *		SPI_NO_SUCH_SERVICE
 *		SPI_NO_SUCH_TASK
 *		SPI_AUTHENTICATION_FAILURE
 *
 * DESCRIPTION
 *		Call the service specific authentication function for validating
 *		service access.
 *
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiAuthenticate (comargs_t *comargs, struct authTaskReq *uargs,
	uint8 *authKey)
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_TASK_T	*gTask;
	SPI_SERVICE_T	*gService;
	SPI_OPS_T	*ops;
	ncp_task_t	*pHandle;
	ncp_channel_t	*channel;
	pl_t			pl;

	NTR_ENTER(3, comargs, uargs, authKey, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);
	
	/*
	 *	Make sure SPI is running before allowing this to happen
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	if (NWslGetService( comargs->address, &gService ))
		return( NTR_LEAVE(  SPI_NO_SUCH_SERVICE  ) );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l ( gService->taskList, comargs->credential, &gTask, FALSE )) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return( NTR_LEAVE(  SPI_NO_SUCH_TASK  ) );
	}
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	SLEEP_UNLOCK (spiTaskListSleepLock);

	NWslGetTaskProtoHandle( gTask, &pHandle );

	CHECK_CONN_REFERENCE( ccode, comargs->connectionReference,
		((ncp_channel_t *)pHandle->channelPtr)->connectionReference);

	if( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if ( !(gTask->mode & SPI_TASK_CONNECTED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( SPI_NO_SUCH_TASK ) );
	}

	if ( NWslSetTaskInUse_l ( gTask ) ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE(  SPI_TASK_DRAINING  ) );
	}

	NWslGetServiceOps( gService, &ops );
	ccode = SPI_AUTH( ops, pHandle, uargs->authType, uargs->objID, 
		authKey, uargs->authKeyLength );	

	if (ccode != SUCCESS) {
		gTask->mode |= SPI_TASK_DRAINING;
		NWslSetTaskNotInUse_l ( gTask );
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return (NTR_LEAVE (SPI_SERVER_UNAVAILABLE));
	}

	gTask->mode |= SPI_TASK_AUTHENTICATED;

	channel = pHandle->channelPtr;

	SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);

	if (channel->licenseCount) {
		if (((ncp_server_t *)(gService->protoServicePtr))->majorVersion >= 4) {
			(void) SPI_LICENSE( ops, pHandle, 1 );
		}
		channel->licenseState = NWC_CONNECTION_LICENSED;
	}

	SLEEP_UNLOCK (channel->connSleepLock);

	NWslSetTaskNotInUse_l ( gTask );
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return( NTR_LEAVE( ccode ) );
}

/*
* BEGIN_MANUAL_ENTRY(NWsiScanServices.3k)
* NAME
*		NWsiScanServices -	Scan registered services
*
* SYNOPSIS
*		enum NUC_DIAG
*		NWsiScanServices( serviceInfo )
*		struct serviceInfoStruct	*serviceInfo;
*
* INPUT
*		struct serviceInfoStruct	*serviceInfo;
*
* OUTPUT
*		struct serviceInfoStruct	*serviceInfo;
*
* RETURN VALUES
*		SPI_FAILURE
*		SPI_INACTIVE
*		SPI_NO_MORE_SERVICE
*
* DESCRIPTION
*		Allow the iterative search for all Services registered in
*		spil
*
* NOTES
*		Currently provides no pattern matching, or specific search
*		capability
*		
*	LOCKS EXPECTED TO BE HELD WHEN CALLED:
*		none
*		
*	LOCKS HELD WHEN RETURNED:
*		none
*
* SEE ALSO
*
* END_MANUAL_ENTRY
*/
enum NUC_DIAG
NWsiScanServices (struct serviceInfoStruct *serviceInfo)
{
	SPI_SERVICE_T	*servPtr;
	pl_t			pl;

	NTR_ENTER(1, serviceInfo, 0, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Make sure SPI is running before allowing this to happen
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	/*
	 *	If address is null, return the first entry
	 */
	if (serviceInfo->address == NULL) {
		NWtlRewindSLList( serviceList );
		if (NWtlGetContentsSLList( serviceList, (void_t *)&servPtr )) {
			SLEEP_UNLOCK (serviceListSleepLock);
			return(NTR_LEAVE(SPI_NO_MORE_SERVICE));
		}
		serviceInfo->serviceProtocol = servPtr->serviceProtocol;
		serviceInfo->transportProtocol = servPtr->transportProtocol;
		serviceInfo->address = servPtr->address;
		serviceInfo->serviceFlags = servPtr->flags;
		SLEEP_UNLOCK (serviceListSleepLock);
		NTR_LEAVE((uint_t) servPtr);
		return(SUCCESS);
	} else {
		/*
		 *	Find the previous one, advance the list pointer
		 *	then extract the new current entry
		 */
		if (NWslFindServiceByAddress_l (serviceInfo->address, &servPtr)
					== SUCCESS) {
			if (NWtlNextNodeSLList( serviceList )) {
				SLEEP_UNLOCK (serviceListSleepLock);
				return(NTR_LEAVE(SPI_NO_MORE_SERVICE));
			}

			if (NWtlGetContentsSLList(serviceList, (void_t *)&servPtr)) {
				SLEEP_UNLOCK (serviceListSleepLock);
				return(NTR_LEAVE(SPI_NO_MORE_SERVICE));
			}
			else {
				serviceInfo->serviceProtocol = servPtr->serviceProtocol;
				serviceInfo->transportProtocol = servPtr->transportProtocol;
				serviceInfo->address = servPtr->address;
				serviceInfo->serviceFlags = servPtr->flags;
				SLEEP_UNLOCK (serviceListSleepLock);
				NTR_LEAVE((uint_t) servPtr);
				return(SUCCESS);
			}
		} else {
			SLEEP_UNLOCK (serviceListSleepLock);
			return(NTR_LEAVE(SPI_NO_MORE_SERVICE));
		}
	}

}


/*
 * BEGIN_MANUAL_ENTRY(NWsiScanServiceTasks.3k)
 * NAME
 *		NWsiScanServiceTasks -	Scan tasks connected to a service
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiScanServiceTasks( serviceName, credPtr, mode )
 *		char	*serviceName;
 *		void_t	*credPtr;
 *		bmask_t	*mode;
 *
 * INPUT
 *		char	*serviceName;
 *		void_t	*credPtr;
 *		bmask_t	*mode;
 *
 * OUTPUT
 *		void_t	*credPtr;
 *		bmask_t	*mode;
 *
 * RETURN VALUES
 *		SPI_SUCCESS			- Successful request
 *		SPI_INACTIVE		- SPIL is currently disabled
 *		SPI_NO_SUCH_SERVICE	- Service name passed was invalid
 *		SPI_NO_MORE_TASK	- Last task in the list was encountered
 *
 * DESCRIPTION
 *		Iteratively scans the task list for a specific service to list
 *		all credential structures associated with this service.
 *
 * NOTES
 *		Currently performs a sequential search without pattern match
 *		capability.  This capability may be added later if the need is
 *		exhibited.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiScanServiceTasks (comargs_t *comargs, bmask_t *mode)
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	void_t			*taskList;
	SPI_TASK_T		*taskHandle;
	pl_t			pl;

	NTR_ENTER(2, comargs, mode, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Make sure SPI is running before allowing this to happen
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	if ((NWslFindServiceByAddress_l (comargs->address, &gService ))
				== SUCCESS ) {
		SLEEP_UNLOCK (serviceListSleepLock);
		NWslGetServiceTaskList( gService, &taskList );
		ccode = NWslScanTasks( taskList, comargs->credential, mode, &taskHandle );
	} else {
		SLEEP_UNLOCK (serviceListSleepLock);
		ccode = SPI_NO_SUCH_SERVICE;
	}

	return( NTR_LEAVE( ccode ) );
}

/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
enum NUC_DIAG
NWsiGetServerContext (comargs_t *comargs, struct getServerContextReq *context )
{
	enum NUC_DIAG ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	ncp_task_t	*pTask;
	pl_t		pl;

	NTR_ENTER(2, comargs, context, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		return(NTR_LEAVE(SPI_INACTIVE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	if (NWslFindServiceByAddress_l (comargs->address, &gService)) {
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_SERVICE));
	}
	SLEEP_UNLOCK (serviceListSleepLock);

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l (gService->taskList, comargs->credential, &gTask, FALSE))	{
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_CONNECTED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}

	NWslGetTaskProtoHandle( gTask, &pTask );

	CHECK_CONN_REFERENCE( ccode, comargs->connectionReference, 
		((ncp_channel_t *)pTask->channelPtr)->connectionReference);

	if( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_TASK_DRAINING));
	}

	NWslGetServerContext( gService, context );
	NWslSetTaskNotInUse_l ( gTask );
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return(NTR_LEAVE(ccode));
}

/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
enum NUC_DIAG
NWsiLicenseConn (comargs_t *comargs, uint32 flags)
{
	enum NUC_DIAG ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	ncp_task_t	*protoHandle;
	ncp_channel_t	*channel;
	SPI_OPS_T	*ops;
	pl_t			pl;

	NTR_ENTER(2, comargs, flags, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		return(NTR_LEAVE(SPI_INACTIVE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	if (NWslFindServiceByAddress_l (comargs->address, &gService)) {
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_SERVICE));
	}
	SLEEP_UNLOCK (serviceListSleepLock);

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l (gService->taskList, comargs->credential, &gTask, FALSE))	{
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_CONNECTED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}

	NWslGetTaskProtoHandle( gTask, &protoHandle );

	CHECK_CONN_REFERENCE( ccode, comargs->connectionReference, 
		((ncp_channel_t *)protoHandle->channelPtr)->connectionReference);

	if( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_TASK_DRAINING));
	}

	NWslGetServiceOps( gService, &ops );

	channel = protoHandle->channelPtr;

	SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);

	if (channel->authenticationState == NWC_AUTH_STATE_NONE) {
		if (flags) {
			channel->licenseCount++;
		} else {
			channel->licenseCount--;
		}
		/* should I check for negative license counts ? */
	} else {
		if (flags) {
			if (channel->licenseCount == 0) {
				if (((ncp_server_t *)(gService->protoServicePtr))->majorVersion >= 4) {
					(void) SPI_LICENSE( ops, protoHandle, flags );
				}
				channel->licenseCount++;
				channel->licenseState = NWC_CONNECTION_LICENSED;
			} else {
				channel->licenseCount++;
			}
		} else {
			if (channel->licenseCount == 1) {
				if (((ncp_server_t *)(gService->protoServicePtr))->majorVersion >= 4) {
					(void) SPI_LICENSE( ops, protoHandle, flags );
				}
				channel->licenseCount--;
				if (channel->licenseCount < 0) {
					channel->licenseCount = 0;
				}
				if (channel->licenseCount == 0) {
					channel->licenseState = NWC_NOT_LICENSED;
				}
			} else {
				channel->licenseCount--;
				if (channel->licenseCount < 0) {
					channel->licenseCount = 0;
				}
				if (channel->licenseCount == 0) {
					channel->licenseState = NWC_NOT_LICENSED;
				}
			}
		}
	}

	SLEEP_UNLOCK (channel->connSleepLock);

	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return(NTR_LEAVE(ccode));
}

/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
enum NUC_DIAG
NWsiMakeConnectionPermanent (comargs_t *comargs)
{
	enum NUC_DIAG ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	ncp_task_t	*pTask;
	pl_t		pl;

	NTR_ENTER(1, comargs, 0, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		return(NTR_LEAVE(SPI_INACTIVE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	if (NWslFindServiceByAddress_l (comargs->address, &gService)) {
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_SERVICE));
	}
	SLEEP_UNLOCK (serviceListSleepLock);

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l (gService->taskList, comargs->credential, &gTask, FALSE))	{
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_CONNECTED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}

	NWslGetTaskProtoHandle( gTask, &pTask );

	CHECK_CONN_REFERENCE( ccode, comargs->connectionReference, 
		((ncp_channel_t *)pTask->channelPtr)->connectionReference);

	if( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_TASK_DRAINING));
	}

	gTask->mode |= SPI_TASK_PERMANENT;
	NWslSetTaskNotInUse_l ( gTask );
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return(NTR_LEAVE(ccode));
}

/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
enum NUC_DIAG
NWsiGetConnInfo (comargs_t *comargs, uint32 uInfoLevel, uint32 uInfoLen,
	char *buffer, uint32 handleLicense)
{
	enum NUC_DIAG ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	uint32		pluConnectionReference;
	ncp_task_t	*pTask;
	pl_t		pl;

	NTR_ENTER( 5, comargs, uInfoLevel, uInfoLen, buffer, handleLicense );

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		return(NTR_LEAVE(SPI_INACTIVE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	if (NWslFindServiceByAddress_l (comargs->address, &gService)) {
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_SERVICE));
	}
	SLEEP_UNLOCK (serviceListSleepLock);

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l (gService->taskList, comargs->credential, &gTask, FALSE))	{
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_CONNECTED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}

	NWslGetTaskProtoHandle( gTask, &pTask );

	CHECK_CONN_REFERENCE( ccode, comargs->connectionReference, 
		((ncp_channel_t *)pTask->channelPtr)->connectionReference);

	if( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_TASK_DRAINING));
	}

	ccode = NWslGetConnInfo(gTask, &pluConnectionReference,  uInfoLevel,
		uInfoLen, buffer, handleLicense);

	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return(NTR_LEAVE(ccode));
}

/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
enum NUC_DIAG
NWsiSetConnInfo (comargs_t *comargs, uint32 uInfoLevel, uint32 uInfoLen,
	char *buffer)
{
	enum NUC_DIAG ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	ncp_task_t	*pTask;
	pl_t		pl;

	NTR_ENTER(4, comargs, uInfoLevel, uInfoLen, buffer, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		return(NTR_LEAVE(SPI_INACTIVE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	if (NWslFindServiceByAddress_l (comargs->address, &gService)) {
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_SERVICE));
	}
	SLEEP_UNLOCK (serviceListSleepLock);

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l (gService->taskList, comargs->credential, &gTask, FALSE))	{
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);

	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
	if (!(gTask->mode & SPI_TASK_CONNECTED)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_TASK));
	}

	NWslGetTaskProtoHandle( gTask, &pTask );

	CHECK_CONN_REFERENCE( ccode, comargs->connectionReference, 
		((ncp_channel_t *)pTask->channelPtr)->connectionReference);

	if( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if (NWslSetTaskInUse_l (gTask)) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return(NTR_LEAVE(SPI_TASK_DRAINING));
	}

	ccode = NWslSetConnInfo(gTask, uInfoLevel, uInfoLen, buffer);

	NWslSetTaskNotInUse_l (gTask);
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return(NTR_LEAVE(ccode));
}

/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
enum NUC_DIAG
NWsiScanConnInfo (comargs_t *comargs, uint32 *pluConnectionReference,
	uint32 uScanInfoLevel, char *pScanConnInfo, uint32 uScanInfoLen,
	uint32 uScanFlags, uint32 uInfoLevel, uint32 uInfoLen,
	char *buffer)
{
	enum NUC_DIAG ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*taskList;
	pl_t		pl;

	NTR_ENTER(5, comargs, pluConnectionReference, uScanInfoLevel, 
		pScanConnInfo, uScanInfoLen);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		return(NTR_LEAVE(SPI_INACTIVE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	if (NWslFindServiceByAddress_l (comargs->address, &gService)) {
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(SPI_NO_SUCH_SERVICE));
	}
	SLEEP_UNLOCK (serviceListSleepLock);

	NWslGetServiceTaskList(gService, &taskList);
	ccode = NWslScanConnInfo( taskList, comargs->credential,
		pluConnectionReference, uScanInfoLevel, pScanConnInfo,
		uScanInfoLen, uScanFlags, uInfoLevel, uInfoLen, buffer );

	return(NTR_LEAVE(ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiScanServicesByUser.3k)
 * NAME
 *		NWsiScanServicesByUser -	Scan tasks connected to a service
 *		and return a 
 *		buffer of populated struct ScanServiceByUserStruct.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiScanServicesByUser( uid, gid, buffer, bufferLength, bufferUsed )
 *		int32	uid;
 *		int32	gid;
 *		struct scanServicesByUserStruct	*buffer;
 *		int32	bufferLength;
 *		int32	*bufferUsed;
 *
 * INPUT
 *		int32	uid;
 *		int32	gid;
 *		struct scanServicesByUserStruct	*buffer;
 *		int32	bufferLength;
 *
 * OUTPUT
 *		struct scanServicesByUserStruct	*buffer;
 *		int32	*bufferUsed;
 *
 * RETURN VALUES
 *		SPI_SUCCESS				- Successful request
 *		SPI_MORE_ENTRIES_EXIST	- More entries were avilable that could fit
 *								  into the buffer provided.
 *
 * DESCRIPTION
 *		Scan the available services looking for authentications for the 
 *		caller.  All that are found are returned in buffer.  The service
 *		Protocol layer is called to retrieve the name by which the service
 *		knows the caller.
 * 
 * NOTES
 *
 * SEE ALSO
 *		NWsiReturnSSBUBufferSize
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiScanServicesByUser (comargs_t *comargs,
	struct scanServicesByUserStruct *buffer, int32	bufferLength,
	int32	*bufferUsed)
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_SERVICE_T	*servicePtr;
	struct netbuf	*serviceAddressPtr;
	void_t		*taskList;
	SPI_TASK_T	*taskPtr;
	void_t		*sprotoTaskPtr;
	char		*userName;
	SPI_OPS_T	*ops;
	uint32		mode;
	pl_t		pl;

	NTR_ENTER(4, comargs, buffer, bufferLength, bufferUsed, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Verify that the SPIL layer has been initialized
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	*bufferUsed = 0;
	serviceAddressPtr = NULL;

	/*
	 *	Scan the list of registered services for tasks belonging to the
	 *	caller.
	 */
	for (;;) {
		/*
		 *	Get the next service in the list.  If no more are found, this is
		 *	not an error:  return SPI_SUCCESS at end of the server list
		 */
		SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
		ccode = NWslScanService( &serviceAddressPtr, &mode, &servicePtr );
		SLEEP_UNLOCK (serviceListSleepLock);
		if ( ccode != SUCCESS ) {
			ccode = SUCCESS;
			goto Done;
		}
		NWslGetServiceTaskList( servicePtr, &taskList );
		
		/*
		 *	Find a task that belongs to the caller.  If one is not found,
		 *	continue with the next service.
		 */
		SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
		ccode = GetTask_l(servicePtr->taskList, comargs->credential, &taskPtr,
							FALSE);
		if ( ccode != SUCCESS )  {
			SLEEP_UNLOCK (spiTaskListSleepLock);
			continue;
		}
		SLEEP_UNLOCK (spiTaskListSleepLock);

		/*
		 *	Copy the service name and user name to the buffer
		 *	provided by the caller
		 */
		if ( *bufferUsed + sizeof( struct scanServicesByUserStruct)
			> bufferLength ) {
			ccode = SPI_MORE_ENTRIES_EXIST;
			goto Done;
		}
		buffer->address.maxlen = MAX_ADDRESS_SIZE;
		buffer->address.len = serviceAddressPtr->len;
		bcopy(serviceAddressPtr->buf, buffer->buffer, serviceAddressPtr->len);

		/*
		 *	Call the Service protocol layer to retrieve the user
		 *	name from the SPROTO-owned authentication structure.
		 */
		NWslGetServiceOps( servicePtr, &ops );
		NWslGetTaskProtoHandle( taskPtr, &sprotoTaskPtr );
		if ( ( ccode = SPI_SSBU_NAME( ops, sprotoTaskPtr, &userName ) ) ) {
			ccode = SPI_GENERAL_FAILURE;
			goto Done;
		}
		strcpy ( buffer->userName, userName );
		buffer->taskMode = taskPtr->mode;
		buffer->resourceCount = taskPtr->resourceCount;
		buffer++;
		*bufferUsed += sizeof( struct scanServicesByUserStruct );
	}

	Done:
		
	return( NTR_LEAVE( ccode ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiRaw.3k)
 * NAME
 *		NWsiRaw -	Raw packet to/from service protocol layer.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiRaw( serviceName, token, credPtr, header, sendHdrLen, recvHdrLen, data, sendDataLen, recvDataLen )
 *		char	*serviceName;
 *		void_t	*token;
 *		void_t	*credPtr;
 *		char	*header;
 *		int32	sendHdrLen, recvHdrLen;
 *		char	*data;
 *		int32	sendDataLen, recvDataLen;
 *
 * INPUT
 *		char	*serviceName;
 *		void_t	*token;
 *		void_t	*credPtr;
 *		char	*header;
 *		int32	sendHdrLen, recvHdrLen;
 *		char	*data;
 *		int32	sendDataLen, recvDataLen;
 *
 * OUTPUT
 *		char	*header;
 *		int32	sendHdrLen, recvHdrLen;
 *		char	*data;
 *		int32	sendDataLen, recvDataLen;
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		This function provides the capability of transmitting service
 *		protocol requests over the same transport endpoint as the file
 *		system.  The purpose of this mechanism is to allow API's or
 *		other IPC mechanisms to format their own requests thay may not
 *		be part of the kernel service protocol suite that the package
 *		must incorporate in order to support the Netware Unix File 
 *		System.  Some examples are:
 *				 Authentication utilities
 *				 Netware permissions modification utilities
 *				 Netware User management utilities
 *				 etc...
 *
 * NOTES
 *		The token from NWsiRegisterRaw must be passed, or the request
 *		will fail.
 *
 * SEE ALSO
 *		NWsiRegisterRaw(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiRaw (comargs_t *comargs, void_t *token, char *header, int32 sendHdrLen, 
	int32 *recvHdrLen, int32 hkResFlag, char *data,
	int32 sendDataLen, int32 *recvDataLen, int32 dkResFlag)
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	SPI_OPS_T	*ops;
	ncp_task_t	*pTask;
	pl_t		pl;

	NTR_ENTER(5, comargs, token, header, sendHdrLen, sendDataLen);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Make sure SPI is running before allowing this to happen
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}
	RW_UNLOCK (nucUpperLock, pl);

	if (NWslGetService( comargs->address, &gService ))
		return( NTR_LEAVE(  SPI_NO_SUCH_SERVICE  ) );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l ( gService->taskList, comargs->credential, &gTask, FALSE )) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return( NTR_LEAVE(  SPI_NO_SUCH_TASK  ) );
	}		
	SLEEP_UNLOCK (spiTaskListSleepLock);
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);

	NWslGetTaskProtoHandle( gTask, &pTask );

	CHECK_CONN_REFERENCE( ccode, comargs->connectionReference, 
		((ncp_channel_t *)pTask->channelPtr)->connectionReference);

	if( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if ( NWslSetTaskInUse_l ( gTask ) ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE(  SPI_TASK_DRAINING  ) );
	}

	NWslGetServiceOps( gService, &ops );

	ccode = SPI_RAW(ops, pTask, token,
				header, sendHdrLen, recvHdrLen, hkResFlag,
				data, sendDataLen, recvDataLen, dkResFlag );

	if (   (ccode == SPI_SERVER_UNAVAILABLE) || (ccode == SPI_BAD_CONNECTION)
		|| (ccode == SPI_NO_CONNECTIONS_AVAILABLE)
		|| (ccode == SPI_SERVER_DOWN)) {
		gTask->mode |= SPI_TASK_DRAINING;
	}

	NWslSetTaskNotInUse_l ( gTask );
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return( NTR_LEAVE( ccode ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiRegisterRaw.3k)
 * NAME
 *		NWsiRegisterRaw
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiRegisterRaw( serviceName, credPtr, token )
 *		char	*serviceName;
 *		void_t	*credPtr;
 *		void_t	*token;
 *
 * INPUT
 *		char	*serviceName;
 *		void_t	*credPtr;
 *
 * OUTPUT
 *		void_t	*token;
 *
 * RETURN VALUES
 *		SPI_INACTIVE	- SPI has not been initialized yet.
 *		SPI_NO_SUCH_SERVICE	- No service has been registered with this name
 *		SPI_NO_SUCH_TASK	- No task exists for this client to this service
 *
 * DESCRIPTION
 *		Register this instance as being capable of sending and receiving
 *		Service protocol requests/replys that are not formatted by the
 *		underlying package.
 *
 * NOTES
 *		This currently calls the NCP suite to allocate an NCP task number
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *		NWsiRaw(3k)
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiRegisterRaw (comargs_t *comargs, void_t *token,
				uint32 *connectionReference)
{
	enum NUC_DIAG	ccode;
	ncp_task_t	*spTask;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	SPI_OPS_T	*ops;
	pl_t		pl;

	NTR_ENTER(3, comargs, token, connectionReference, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Make sure SPI is running before allowing this to happen
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	if (NWslGetService( comargs->address, &gService ))
		return( NTR_LEAVE(  SPI_NO_SUCH_SERVICE  ) );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l ( gService->taskList, comargs->credential, &gTask, FALSE )) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return( NTR_LEAVE(  SPI_NO_SUCH_TASK  ) );
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);

	NWslGetTaskProtoHandle( gTask, &spTask );

	/*
	 *	Indicate that this task is in use
	 */
	if ( NWslSetTaskInUse_l ( gTask ) ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE(  SPI_TASK_DRAINING  ) );
	}

	NWslGetServiceOps( gService, &ops );
	ccode = SPI_REGISTER_RAW( ops, spTask, token, connectionReference );

	if ( ccode == FAILURE ) {
		gTask->mode |= SPI_TASK_DRAINING;
	}

	/*
	 *	Indicate that this task is no longer in use
	 */
	NWslSetTaskNotInUse_l ( gTask );
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return( NTR_LEAVE( ccode ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiRelinquishRawToken.3k)
 * NAME
 *		NWsiRelinquishRawToken
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiRelinquishRawToken( serviceName, credPtr, token )
 *		char	*serviceName;
 *		void_t	*credPtr;
 *		void_t	*token;
 *
 * INPUT
 *		char	*serviceName;
 *		void_t	*credPtr;
 *		void_t	*token;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * NOTES
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiRelinquishRawToken (comargs_t *comargs, void_t *token)
{
	enum NUC_DIAG	ccode = SUCCESS;
	ncp_task_t	*spTask;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	SPI_OPS_T	*ops;
	pl_t		pl;

	NTR_ENTER(2, comargs, token, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Make sure SPI is running before allowing this to happen
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);

	if (NWslGetService( comargs->address, &gService ) )
		return( NTR_LEAVE(  SPI_NO_SUCH_SERVICE  ) );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l ( gService->taskList, comargs->credential, &gTask, FALSE ) ) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return( NTR_LEAVE(  SPI_NO_SUCH_TASK  ) );
	}
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);

	NWslGetTaskProtoHandle( gTask, &spTask );

	/*
	 *  Blow away connection when reference count and
	 *  resource count go to 0 and one of the following are true:
	 *  1. the connection is a private connection
	 *  2. the connection is a public connection that is not permanent
	 *  3. the connection is a public connection that is not authenticated
	 *  4. the connection is a public connection that is not primary.
	 *
	 *  Note: requester uses referenceCount and nucfs uses resourceCount.
	 */
	if ((((ncp_channel_t *)spTask->channelPtr)->referenceCount == 1) &&
		(gTask->resourceCount == 0) &&
		(( comargs->credential->flags & NWC_OPEN_PRIVATE) ||
		!(( gTask->mode & SPI_TASK_PERMANENT ) ||
		  ( gTask->mode & SPI_TASK_AUTHENTICATED ) ||
		  ( gTask->mode & SPI_PRIMARY_SERVICE )))) {

		ccode = NWsiCloseServiceWithTaskPtr_l (gTask);
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		SLEEP_UNLOCK (spiTaskListSleepLock);
		return( NTR_LEAVE( ccode ) );
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);
	CHECK_CONN_REFERENCE( ccode, comargs->connectionReference, 
		((ncp_channel_t *)spTask->channelPtr)->connectionReference);

	if( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if ( NWslSetTaskInUse_l ( gTask ) ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE(  SPI_TASK_DRAINING  ) );
	}

	NWslGetServiceOps( gService, &ops );

	ccode = SPI_RELINQUISH_RAW( ops, spTask, token );

	if ( ccode == SPI_SERVER_UNAVAILABLE ) {
		gTask->mode |= SPI_TASK_DRAINING;
	}

	NWslSetTaskNotInUse_l ( gTask );
	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return( NTR_LEAVE( ccode ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiSetPrimaryService.3k)
 * NAME
 *		NWsiSetPrimaryService -	Scan all tasks for the user and set the one
 *		associated with the requested service as the Primary or default
 *		service.
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiSetPrimaryService( uid, gid, serviceName )
 *		int32	uid;
 *		int32	gid;
 *		char	*serviceName;
 *
 * INPUT
 *		int32	uid;
 *		int32	gid;
 *		char	*serviceName;
 *
 * OUTPUT
 *
 * RETURN VALUES
 *		SPI_SUCCESS				- Successful request
 *
 * DESCRIPTION
 *		Scan the list of services for which the user has created service
 *		tasks.  Select the task associated with the requested server as the
 *		Primary or default service.  Remove this designation from all other
 *		tasks.  If the task cannot be located, leave the original service
 *		as the Primary.  If serviceName is NULL, the caller wishes no
 *		primary service, therefore remove any such designation from all
 *		services.
 *
 * NOTES
 *		serviceAddress is NULL if no service is to be designated as the
 *		primary service.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiSetPrimaryService (comargs_t *comargs )
{
	enum NUC_DIAG	ccode = SUCCESS;
	uint32			mode;
	SPI_SERVICE_T	*servicePtr;
	struct netbuf	*serviceAddressPtr = NULL;
	void_t			*taskList;
	SPI_TASK_T		*taskPtr, *firstTaskPtr = NULL,
					*originalPrimaryTaskPtr = NULL;
	uint32			foundIt = FALSE;
	int32			serviceCount = 0;
	ncp_task_t		*spTask, *ospTask;
	pl_t			pl;

	NTR_ENTER(1, comargs, 0, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Verify that the SPIL layer has been initialized
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}
	RW_UNLOCK (nucUpperLock, pl);

	/*
	 *	Scan the list of registered services for tasks belonging to the
	 *	caller.
	 */
	for (;;) {
		/*
		 *	Get the next service in the list.  If no more are found, 
		 *	exit with SUCCESS.
		 *
		 *	P.S. mode is not actually used by NWslScanService at this time.
		 */
		SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
		ccode = NWslScanService( &serviceAddressPtr, &mode, &servicePtr );
		SLEEP_UNLOCK (serviceListSleepLock);
		if ( ccode != SUCCESS ) {
			ccode = SUCCESS;
			break;
		}
		NWslGetServiceTaskList( servicePtr, &taskList );
		
		/*
		 *	Find a task that belongs to the caller.  If one is not found,
		 *	continue with the next service.
		 */
		SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
		ccode = GetTask_l(servicePtr->taskList, comargs->credential, &taskPtr,
							FALSE);
		SLEEP_UNLOCK (spiTaskListSleepLock);
		if ( ccode != SUCCESS )  {
			continue;
		}
		/*
		 *	Do not permit a non-active service to be marked as the
		 *	primary.  
		 */

		SLEEP_LOCK (taskPtr->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
		if ((taskPtr->mode & SPI_TASK_DRAINING) ) {
			taskPtr->mode &= ~SPI_PRIMARY_SERVICE;
			SLEEP_UNLOCK (taskPtr->spiTaskSleepLock);
			continue;
		}

		/*
		 *	If the name of the new primary service is NULL, the caller
		 *	wishes NO service to be marked as the primary.  Continue to
		 *	the next service if TRUE.
		 */

		/*
		 *	Count the number of tasks found
		 */
		serviceCount++;

		/*
		 *	Save the first authenticated task encountered in case the 
		 *	requested service is not found and there is no current 
		 *	Primary Service
		 */
		if ( firstTaskPtr == NULL )
			firstTaskPtr = taskPtr;

		/*
		 *	If this is the original Primary service, remember where it is
		 *	in case we can't find the new Primary service
		 */
		if ( taskPtr->mode & SPI_PRIMARY_SERVICE )
			originalPrimaryTaskPtr = taskPtr;

		if (comargs->credential->flags & NWC_OPEN_PRIVATE)	{
			/*
			 *  Can't make a private connection the primary connection.
			 */
			SLEEP_UNLOCK (taskPtr->spiTaskSleepLock);
			continue;
		}

		taskPtr->mode &= ~(SPI_PRIMARY_SERVICE);
		NWslGetTaskProtoHandle(taskPtr, &spTask);
		ccode = SUCCESS;
		CHECK_CONN_REFERENCE( ccode, comargs->connectionReference, 
			((ncp_channel_t *)spTask->channelPtr)->connectionReference);

		if (!ccode) {
			taskPtr->mode |= SPI_PRIMARY_SERVICE;

			/*
			 *  remove old primary connection if no one is using it
			 */
			foundIt = TRUE;
		}
		SLEEP_UNLOCK (taskPtr->spiTaskSleepLock);
	}
	
	/*
	 *	If I found the new Primary service or serviceAddress was NULL, all 
	 *	other services had their tasks marked as NOT the Primary service.  
	 *	Return with a smile
	 */
	if ( foundIt || (comargs->address == NULL) ) {
		return( NTR_LEAVE( SUCCESS ) );
	}

	/*
	 *	I didn't find the new Primary service.  If there used to be a Primary
	 *	service, restore its status as the Primary and return
	 *	SPI_NO_SUCH_SERVICE.  If serviceName was null, return SUCCESS.
	 */
	ccode = SPI_NO_SUCH_SERVICE;
	if ( serviceCount == 0 )
		return( NTR_LEAVE( ccode ) );
		
	if ( originalPrimaryTaskPtr != NULL ) {
		originalPrimaryTaskPtr->mode |= SPI_PRIMARY_SERVICE;
		return( NTR_LEAVE( ccode ) );
	}

	/*
	if ( firstTaskPtr != NULL )
		firstTaskPtr->mode |= SPI_PRIMARY_SERVICE;
	*/

	return( NTR_LEAVE( ccode ) );
}

/*
 * BEGIN_MANUAL_ENTRY(NWsiGetServiceMessage.3k)
 * NAME
 *		NWsiGetServiceMessage
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWsiGetServiceMessage( message )
 *		NWSI_MESSAGE_T	*message;
 *
 * INPUT
 *		NWSI_MESSAGE_T	*message;
 *
 * OUTPUT
 *		NWSI_MESSAGE_T	*message;
 *
 * RETURN VALUES
 *		SPI_INACTIVE	- SPI has not been initialized yet.
 *		SPI_NO_SUCH_SERVICE	- No service has been registered with this name
 *		SPI_NO_SUCH_TASK	- No task exists for this client to this service
 *
 * DESCRIPTION
 *
 * NOTES
 *		This function calls the service protocol layer to return a message
 *		from the service when such a message becomes available.  The call 
 *		to SPROTO will sleep until the message is obtained or the service
 *		is no longer available or interrupted by a signal.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
NWsiGetServiceMessage (NWSI_MESSAGE_T  *message)
{
	enum NUC_DIAG	ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	SPI_OPS_T	*ops;
	pl_t		pl;

	NTR_ENTER(1, message, 0, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	/*
	 *	Ensure the SPIL layer has been initialized
	 */
	if ( spiState == SPI_LAYER_INACTIVE ) {
		RW_UNLOCK (nucUpperLock, pl);
		return( NTR_LEAVE( SPI_INACTIVE ) );
	}

	RW_UNLOCK (nucUpperLock, pl);


	/*
	 *	Check the specified service protocol to ensure that
	 *	it is enabled in this kernel
	 */
	if ( SPISwitch[message->serviceProtocol].mode != ENABLED )
		return( NTR_LEAVE( SPI_INVALID_SPROTO ) );

	ops = SPISwitch[message->serviceProtocol].spi_ops;

	ccode = SPI_GET_MESSAGE( ops, message );


	if (ccode != SUCCESS) {
#ifdef NUC_DEBUG
		cmn_err (CE_NOTE, "SPI_GET_MESSAGE failed with cc=%d \n",ccode);
#endif
		if ( ccode == SPI_SERVER_UNAVAILABLE ) {
			gTask = message->spilTaskPtr;
			SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);
			gTask->mode |= SPI_TASK_DRAINING;
			SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		}
		return( NTR_LEAVE( ccode ) );
	}

	/*
	 *	Complete the NWSI_MESSAGE_T structure 
	 */
	gTask = (SPI_TASK_T *)message->spilTaskPtr;

	NWtlGetCredUserID( gTask->credentialsPtr, &(message->uid) );
	NWtlGetCredGroupID( gTask->credentialsPtr, &(message->gid) );
	gService =  gTask->spiServicePtr;
	message->spilServiceAddress.len = gService->address->len;
	bcopy(gService->address->buf, message->buffer, gService->address->len);
	return( NTR_LEAVE( ccode ) );
}

/*		
 * TODO: locking!
 *
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
int
inc_req_ref_cnt( int32 uid )
{
    int                     ccode=SUCCESS;
    extern initListEntry_t  *initList;
    initListEntry_t         *initPtr;


    NTR_ENTER(1, uid, 0, 0, 0, 0);

    /* find init entry to get storage areas */
    if( initList != 0 ) {
        /* See if list entry exists for this uid */
        initPtr = initList;
        do {
            if( initPtr->uid == uid ) {
                initPtr->refCount += 1;
                goto out;
            }
            initPtr = initPtr->initForwardPtr;
        } while( initPtr != 0 );
        ccode = FAILURE;   /* ??? What do we tell the user? */
    } else {
        /* No ILE for this uid */
        ccode = FAILURE;   /* ??? What do we tell the user? */
    }

  out:

    return( NTR_LEAVE( ccode ) );

}

/* overloads reference count decrement in ILE */
/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
int
close_monitored_conn( int32 uid, int32 connReference)
{
    int                     ccode=SUCCESS;
    extern initListEntry_t  *initList;
    initListEntry_t         *initPtr;


    NTR_ENTER(2, uid, connReference, 0, 0, 0);

    /* find init entry to get storage areas */
    if( initList != 0 ) {
        /* See if list entry exists for this uid */
        initPtr = initList;
        do {
            if( initPtr->uid == uid ) {
				if( initPtr->refCount > 0 ) {
					initPtr->refCount -= 1;
				} 
				if( initPtr->monitoredConn == connReference ){
					initPtr->monitoredConn = 0;
				}
				goto out;
            }
            initPtr = initPtr->initForwardPtr;
        } while( initPtr != 0 );
        ccode = FAILURE;   /* ??? What do we tell the user? */
    } else {
        /* No ILE for this uid */
        ccode = FAILURE;   /* ??? What do we tell the user? */
    }

  out:

    return( NTR_LEAVE( ccode ) );

}

/*
 *  Should only be called with comargs->credential flags
 *  set to match public connections.
 */
/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 * TODO:  deadlock-prone when called with skip locked.
 */
ccode_t
getPrimaryService(comargs_t *comargs, SPI_TASK_T **new, SPI_TASK_T *skip,
					int sleepLockInherited)
{
	struct netbuf	*serviceAddressPtr = NULL;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;
	SPI_TASK_T	*tmpTask = NULL;
	ncp_task_t	*protoHandle;
	ncp_channel_t	*channel;
	uint32		n = 0xffffffff;
	uint32		mode;

    NTR_ENTER (4, comargs, new, skip, sleepLockInherited, 0);
#ifdef DEBUG
	NTR_ASSERT(SLEEP_LOCKOWNED(serviceListSleepLock));
	NTR_ASSERT(SLEEP_LOCKOWNED(spiTaskListSleepLock));
#endif

	while (NWslScanService(&serviceAddressPtr, &mode, &gService) == SUCCESS) {
		/*
		 *  There's only one public connection per UID on a server
		 *  task list.  That's why we don't need a while loop.
		 */
		if (GetTask_l (gService->taskList, comargs->credential, &gTask,
						sleepLockInherited) == SUCCESS) {
			if (gTask == skip) {	
				continue;
			}

			/*
			 *  If skip is NULL, then find the primary connection.
			 */
			if (skip == NULL && (gTask->mode & SPI_PRIMARY_SERVICE)) {
				tmpTask = gTask;
				break;
			}

			NWslGetTaskProtoHandle(gTask, &protoHandle);
			channel = (ncp_channel_t *)protoHandle->channelPtr;

			/*
			 *  Public connection with lowest reference number
			 *  becomes new primary connection.
			 */

			SLEEP_LOCK (channel->connSleepLock, NUCCONNSLEEPPRI);

			if (channel->connectionReference < n) {
				n = channel->connectionReference;
				tmpTask = gTask;
			}

			SLEEP_UNLOCK (channel->connSleepLock);
		}
	}
	if (tmpTask) {
		*new = tmpTask;
		return( NTR_LEAVE(SUCCESS) );
	} else
		return( NTR_LEAVE(FAILURE) );
}

/*		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
enum NUC_DIAG
NWsiGetPrimaryService (comargs_t *comargs, uint32 *reference)
{
	SPI_TASK_T	*gTask;
	ncp_task_t	*pTask;
	ncp_channel_t	*channel;
	pl_t			pl;

	NTR_ENTER(2, comargs, reference, 0, 0, 0);

	pl = RW_RDLOCK (nucUpperLock, plstr);

	if (spiState == SPI_LAYER_INACTIVE) {
		RW_UNLOCK (nucUpperLock, pl);
		return(NTR_LEAVE(SPI_INACTIVE));
	}

	RW_UNLOCK (nucUpperLock, pl);

	SLEEP_LOCK (serviceListSleepLock, NUCSPILSLEEPPRI);
	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (getPrimaryService(comargs, &gTask, NULL, FALSE) == FAILURE) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		SLEEP_UNLOCK (serviceListSleepLock);
		return(NTR_LEAVE(FAILURE));
	}
	SLEEP_UNLOCK (spiTaskListSleepLock);
	SLEEP_UNLOCK (serviceListSleepLock);

	NWslGetTaskProtoHandle(gTask, &pTask);
	channel = (ncp_channel_t *)pTask->channelPtr;
	*reference = channel->connectionReference;

	return(NTR_LEAVE(SUCCESS));
}
