/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/siauth.c	1.23"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/siauth.c,v 2.54.2.7 1995/02/13 00:43:38 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: siauth.c 
 *	ABSTRACT: Combines all the NCP libraries and packet modules
 *			  to perform the operation requested.
 *
 *	Functions declared in this module:
 *	
 *	Public functions:
 *	NCPsiInitNCP
 *	NCPsiFreeNCP
 *	NCPsiCreateService
 *	NCPsiDestoryService
 *	NCPsiCreateTask
 *	NCPsiDestroyTask
 *	NCPsiAuthenticateTask
 *	NCPsiRawNCP
 *	NCPsiRegisterRawNCP
 *	NCPsiRelinquishRawToken
 *	NCPsiReturnSSBUBufferCount
 *	NCPsiGetUserID
 *	NCPsiGetBroadcastMessage
 *
 */ 
#ifdef _KERNEL_HEADERS
#include <net/tiuser.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/param.h>

#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/gtsendpoint.h>
#include <net/nuc/gtsconf.h>
#include <net/nuc/gtsmacros.h>
#include <net/nuc/requester.h>
#include <net/nuc/nwmp.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>
#include <sys/time.h>

#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>
#include <sys/ncpconst.h>
#include <sys/nucmachine.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <sys/spilcommon.h>
#include <sys/nucerror.h>
#include <sys/sistructs.h>
#include <sys/ncpiopack.h>
#include <sys/slstruct.h>
#include <sys/gtscommon.h>
#include <sys/gtsendpoint.h>
#include <sys/gtsconf.h>
#include <sys/gtsmacros.h>
#include <sys/requester.h>
#include <sys/nwmp.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask	NTRM_ncp

#ifdef FS_ONLY
ccode_t NCPsiCalculateTimeZoneDifference();
#endif /* FS_ONLY */

/*
 *	ANSI prototypes
 */
#if defined(__STDC__)
#endif

/*
 *	Conf is where all the configuration parameters are externed in from
 *	the space.c module of nwncp driver
 */
#ifdef _KERNEL_HEADERS
#include <net/nuc/nwncpconf.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/nwncpconf.h>
#endif /* ndef _KERNEL_HEADERS */


#define INACTIVE 0
#define ACTIVE   1

void_t *NCPbroadcastMessageQueue;
sv_t *NCPbroadcastMessageQueueSV;
extern lock_t *nucLock;
extern rwlock_t *nucUpperLock;
extern int32  spiState;

/*
 * BEGIN_MANUAL_ENTRY(NCPsiInit.4k)
 * NAME
 *		NCPsiInit - Init routine for the NCP package
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiInitNCP()
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *
 * DESCRIPTION
 *		Performs initialization of the NCP service protocol module
 *
 * NOTES
 *		Initialization routine for the NCP module.  This function
 *		is called by the SPI to initialize all Service protocol modules
 *		under it.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		nucUpperLock
 *
 *	LOCKS HELD WHEN RETURNED:
 *		nucUpperLock
 *
 *
 * SEE ALSO
 *		NCPsiFreeNCP(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiInitNCP()
{

	NTR_ENTER( 0, 0, 0, 0, 0, 0 );

	/*
	 *	Initialize the NCP layer by setting up the memory
	 *	allocation region
	 */
	if ( ncpConf.memSize == 0 )
		ncpConf.memSize = ncpConf.defMemSize;

	if ((NCPbroadcastMessageQueueSV = SV_ALLOC (KM_NOSLEEP)) == NULL) {
			cmn_err(CE_PANIC, "NCPsiInitNCP: NCPbroadcastMessageQueueSV alloc failed");
			return (NTR_LEAVE (SPI_CLIENT_RESOURCE_SHORTAGE));
	}
#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NCPsiInitNCP: alloc sv_t * at 0x%x, size = 0x%x",
		NCPbroadcastMessageQueueSV, sizeof(sv_t), 0 );
#endif

	return(NTR_LEAVE( SUCCESS ));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsiFreeNCP.3k)
 * NAME
 *		NCPsiFreeNCP()
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiFreeNCP()
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *
 * DESCRIPTION
 *		Shuts down the NCP layer
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
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiFreeNCP()
{
	pl_t		pl;
	ncp_task_t	*taskPtr;

	NTR_ENTER( 0, 0, 0, 0, 0, 0 );

	/*
	 *	Awaken any consumers of broadcast messages and inform them we are
	 *	terminating.
	 */
	pl = LOCK (nucLock, plstr);
	taskPtr = (ncp_task_t *) NCPbroadcastMessageQueue;
	NCPbroadcastMessageQueue = (void_t *)-1;
	UNLOCK(nucLock, pl);
	SV_BROADCAST (NCPbroadcastMessageQueueSV, 0);
	if (taskPtr && taskPtr != (void_t *)-1 && taskPtr->spilTaskPtr) {
		NWslSetTaskNotInUse_l(taskPtr->spilTaskPtr);
	}
	NTR_LEAVE( SUCCESS );
	return(SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY(NCPsiCreateService.4k)
 * NAME
 *		NCPsiCreateService - Allocate and initialize an NCP server structure 
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiCreateService( serverName, credStruct, transProto, 
 *							address, addressLength, servicePtr )
 *		char			*serverName;
 *		void_t			*credStruct;
 *		int32			transProto;
 *		uint8			*address;
 *		int32			addressLength;
 *		ncp_server_t 	**servicePtr;
 *
 * INPUT
 *		void_t			*credStruct;
 *		char			*serverName;
 *		int32			transProto;
 *		uint8			*address;
 *		int32			addressLength;
 *
 * OUTPUT
 *		ncp_server_t	**servicePtr;
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Allocates a server structure, and calls the server over the
 *		wire to find out what it is, and get volume information
 *
 * NOTES
 *		Checks to see if the server is a version capable of supporting
 *		Name spaces.  Does not actually determine what the name space
 *		is supported.
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 *
 * SEE ALSO
 *		NCPsiDestroyService(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiCreateService (	struct	netbuf	*serverAddress,
						void_t			*credStruct,
						int32			transProto,
						ncp_server_t	**servicePtr )
{
	int				ccode;
	ncp_channel_t	*channelPtr = 0;
	version_t		majorVersion;
	version_t		minorVersion;
	uint16			buffSize;

	NTR_ENTER(4, serverAddress, credStruct, transProto, servicePtr, 0 );

	ccode = NCPsilAllocServer( servicePtr );
	if (ccode) {
		NTR_LEAVE( FAILURE );
		return(FAILURE);
	}

	NCPsilSetServerAddress( *servicePtr, serverAddress );

#ifdef NUC_DEBUG
    NCPsilValidateServerTag( *servicePtr );
#endif NUC_DEBUG

    (*servicePtr)->transportMask = transProto;

	ccode = NCPdplAllocChannel(credStruct, transProto, serverAddress, 
		(*servicePtr)->blockSize, NULL, NULL, &channelPtr);
	if (ccode) {

#ifdef NUC_DEBUG
		NCP_CMN_ERR (CE_CONT,
			"NCPsi:CreateService; AllocChannel  cc = %X \n",ccode);
#endif NUC_DEBUG

		NCPsilFreeServer( *servicePtr );
		NTR_LEAVE( ccode );
		return(ccode);
	}

	/*
	 *	Establish the connection to the server.
	 */

	ccode = NCPspCreateConnection_l ( channelPtr );
	if (ccode) {

#ifdef NUC_DEBUG
		NCP_CMN_ERR (CE_CONT,
			"NCPsi:CreateService; Create Connection cc=%X \n",ccode);
#endif NUC_DEBUG

		goto error;
	}

	/*
	 *	Ping the server to determine block size the file system
	 *	can use for transfers
	 *	Note: This request hits the wire.
	 */
	ccode = NCPspNegotiateBufferSize_l ( channelPtr, &buffSize );
	if (ccode) {

#ifdef NUC_DEBUG
		NCP_CMN_ERR (CE_CONT,
			"NCPsi:CreateService; Negotiate size failed! cc=%X \n", ccode);
#endif NUC_DEBUG

		goto error;
	}

#ifdef NUC_DEBUG
    NCPsilValidateServerTag( *servicePtr );
#endif NUC_DEBUG

    (*servicePtr)->blockSize = buffSize;

	/*
	 *	Get the server's version information
	 *	Note: This request hits the wire.
	 */
	ccode = NCPspGetServerVersion_l ( channelPtr, &majorVersion, &minorVersion,
		(*servicePtr)->serverName );
	if (ccode == SUCCESS) {
		if (NCPsilSetServerVersion(*servicePtr, majorVersion, minorVersion)) {

#ifdef NUC_DEBUG
			NCP_CMN_ERR (CE_CONT, "NCPsi; Bad server version passed\n");
#endif NUC_DEBUG

			ccode = FAILURE;
			goto error;
		}
	}
	/*
	 *	Now that the information has been gathered, free the 
	 *	transport resources as they're not needed from this point on
	 *	Note: This request hits the wire.
	 */
	NCPspDestroyServiceConnection_l ( channelPtr );

	NCPdplFreeChannel_l ( channelPtr );

	NTR_LEAVE( ccode );

	return(ccode);

error:

	/*
	 *	In the event of catastrophic error, free up all resources
	 */
	NCPspDestroyServiceConnection_l ( channelPtr );

	NCPsilFreeServer( *servicePtr );

	NCPdplFreeChannel_l ( channelPtr );

	NTR_LEAVE( ccode );

	return(ccode);

}

/*
 * BEGIN_MANUAL_ENTRY(NCPsiCreateTask.4k)
 * NAME
 *		NCPsiCreateTask -	SPI Interface to NCP module for creating a
 *							task to a service. 
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiCreateTask( serverPtr, credStruct, mode, taskPtr )
 *		ncp_server_t	*serverPtr;
 *		void_t	*credStruct;
 *		bmask_t	mode;
 *		void_t	*spilTaskPtr;	
 *		void_t	**taskPtr;	
 *
 * INPUT
 *		ncp_server_t	*serverPtr;
 *		void_t	*credStruct;
 *		bmask_t	mode;
 *		void_t	*spilTaskPtr;	
 *
 * OUTPUT
 *		void_t	**taskPtr;	
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		Create a connection instance to a given server for a given 
 *		credential structure.
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
 *		NCPsiDestroyTask(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiCreateTask (ncp_server_t *serverPtr, void_t *credStruct, bmask_t mode,
					  void_t *spilTaskPtr, ncp_task_t **taskPtr )
{
	ccode_t			ccode;
	ncp_channel_t	*channelPtr = NULL;
	uint32			transProto; 
	uint32			uid;
	void_t			*authPtr = NULL, *sprotoCredStruct = NULL;
	uint16			serverPacketSize;

	NTR_ENTER(5, serverPtr, credStruct, mode, spilTaskPtr, taskPtr );

	/*
	 *	Allocate and initialize a task structure, 
	 */
	if (ccode = NCPsilAllocTask( serverPtr, taskPtr )) {
		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto error;
	}

#ifdef NUC_DEBUG
    NCPsilValidateServerTag( serverPtr );
#endif NUC_DEBUG

    transProto = ((ncp_server_t *)serverPtr)->transportMask;
	
	/*
	 *  Allocate an NCP authentication structure to contain the
	 *  NetWare userid (password data will be added later)
	 */
	if (ccode = NCPsilAllocAuth( (ncp_auth_t **)&authPtr ))
		goto error;
	
#ifdef NUC_DEBUG
    NCPsilValidateTask( *taskPtr );
#endif NUC_DEBUG

    (*taskPtr)->authInfoPtr = authPtr;
  
#ifdef NUC_DEBUG
    NCPsilValidateTask( *taskPtr );
#endif NUC_DEBUG

	if ( NWtlDupCredStruct( credStruct, (nwcred_t **)&sprotoCredStruct ) ) {

		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
		goto error;
	}

#ifdef NUC_DEBUG
    NCPsilValidateTask( *taskPtr );
#endif NUC_DEBUG

    (*taskPtr)->credStructPtr = sprotoCredStruct;

	(*taskPtr)->spilTaskPtr = spilTaskPtr;

	/*
	 *	Establish the connection to the server.
	 */
	if (ccode = NCPdplAllocChannel(credStruct, transProto, serverPtr->address, 
			serverPtr->blockSize, *taskPtr, spilTaskPtr, &channelPtr))
		goto error;

    (*taskPtr)->channelPtr = channelPtr;

	if (ccode = NCPspCreateConnection_l ( channelPtr ))  {

		goto error;
	}

	if( ccode = negotiateSecurityLevels_l ( credStruct, channelPtr ) ) {

		goto error;
	}

	serverPacketSize = MAX_NEGOTIATED_BUFFER_SIZE;
  	ccode = NCPspNegotiateBufferSize_l ( channelPtr, &serverPacketSize );
  	if (ccode) {

		goto error;
  	}

#ifdef NUC_DEBUG
    NCPsilValidateServerTag( serverPtr );
#endif NUC_DEBUG

    serverPtr->blockSize = serverPacketSize;

	if (serverPtr->majorVersion > 2){
		channelPtr->burstInfo = NULL;
		if( doPacketBurst ){
			NCPspInitiatePacketBurstConnection_l ( channelPtr ); 
		}
	}

#ifdef FS_ONLY
	if (ccode = NCPsiCalculateTimeZoneDifference_l ( channelPtr ))  {

		goto error;
	}
#endif /* FS_ONLY */

	if( serverPtr->majorVersion >= 4 )
		channelPtr->ndsState |= NWC_NDS_CAPABLE;

	SLEEP_UNLOCK (channelPtr->connSleepLock);

#ifdef NUC_DEBUG
		NCP_CMN_ERR (CE_CONT, "NCPsiCreateTask:  NCP task at 0x%X\n", *taskPtr);
#endif NUC_DEBUG

	NWtlGetCredUserID((nwcred_t *)credStruct, &uid);
	inc_req_ref_cnt(uid);

	NTR_LEAVE((uint_t) *taskPtr);

	return( SUCCESS );

error:
	
	if ( authPtr != NULL )
		NCPsilFreeAuth( authPtr );

	if ( *taskPtr != NULL ) {

#ifdef NUC_DEBUG
    	NCPsilValidateTask( *taskPtr );
#endif NUC_DEBUG

    	kmem_free ( *taskPtr, sizeof(ncp_task_t));

#ifdef NUCMEM_DEBUG
        NTR_PRINTF (
			"NUCMEM_FREE: NCPsiCreateTask: free task * at 0x%x, size = 0x%x",
			*taskPtr, sizeof(ncp_task_t), 0 );
#endif NUCMEM_DEBUG
	}
		
	if ( channelPtr != NULL )
		NCPdplFreeChannel_l ( channelPtr );
		
	if ( sprotoCredStruct != NULL )
		NWtlFreeCredStruct ( sprotoCredStruct );

	NTR_LEAVE( ccode );

	return( ccode );
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsiDestroyTask.4k)
 * NAME
 *		NCPsiDestroyTask -	Free all resources assocated with a service
 *							task.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiDestroyTask( taskPtr )
 *		void_t	*taskPtr;
 *
 * INPUT
 *		void_t	*taskPtr;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		Tear down the connection for a task and free all associated 
 *		resources.  If the associated SPI_TASK_T indicates that there are
 *		resources still allocated to this task, simply call down to GTS
 *		and terminate all resources below but preserve the NCP resources
 *		until SPI_TASK_T resouces are zero.
 *
 * NOTES
 *		Should clean up all resources associated with a task.
 *
 * SEE ALSO
 *		NCPsiCreateTask(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiDestroyTask (	ncp_task_t *taskPtr )
{
	ncp_channel_t	*channelPtr;
	void_t			*authPtr;
	SPI_TASK_T		*gTask;
	int32			diagnostic;
	uint32			uid;

	NTR_ENTER(1, taskPtr, 0, 0, 0, 0 );

#ifdef NUC_DEBUG
    NCPsilValidateTask( taskPtr );
#endif NUC_DEBUG

    channelPtr = taskPtr->channelPtr;

	SLEEP_LOCK (channelPtr->connSleepLock, NUCCONNSLEEPPRI);

	/*
	 *	If this task has SPIL resources still allocated, call down to
	 *	Generic Transport to free all transport resources for this channel
	 *	but retain NCP resources until SPIL resources are freed.  Then
	 *	return SUCCESS to the caller.  This routine will be reinvoked by
	 *	the SPIL layer for each SPIL resource to be freed and when all
	 *	SPIL resources have been freed, the NCP resources will be freed
	 *	as well.
	 *	
	 *	Note:  DisConnectFromPeer and CloseTransportEndPoint are also
	 *	called by NCPdplFreeChannel.
	 */
	gTask = (SPI_TASK_T *)(taskPtr->spilTaskPtr);
	if (gTask->resourceCount) {
		if (channelPtr->transportHandle) {
			GTS_DISCONNECT( channelPtr->transportHandle, 
				&diagnostic);
			GTS_CLOSE( channelPtr->transportHandle, 
				&diagnostic);
			channelPtr->transportHandle = NULL;
		}

		SLEEP_UNLOCK (channelPtr->connSleepLock);

		return(NTR_LEAVE( SUCCESS ));
	}
		

	/*
	 *	Tear down the connection information:
	 */
    NWtlGetCredUserID(taskPtr->credStructPtr, &uid);
    close_monitored_conn(uid, channelPtr->connectionReference);
	NCPspDestroyServiceConnection_l ( channelPtr );

#ifdef NUC_DEBUG
    NCPsilValidateTask( taskPtr );
#endif NUC_DEBUG

    authPtr = taskPtr->authInfoPtr;
  
	NWtlFreeCredStruct ( taskPtr->credStructPtr );
	NCPsilFreeAuth( authPtr );
	NCPdplFreeChannel_l ( channelPtr );

#ifdef NUC_DEBUG
    NCPsilValidateTask( taskPtr );
#endif NUC_DEBUG

    kmem_free ( (char *)taskPtr, sizeof(ncp_task_t));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF (
			"NUCMEM_FREE: NCPsiDestroyTask: free task * at 0x%x, size = 0x%x",
			taskPtr, sizeof(ncp_task_t), 0 );
#endif NUCMEM_DEBUG
  
#ifdef NUC_DEBUG
		NCP_CMN_ERR (CE_CONT,
			"NCPsiDestroyTask:  NCP task at 0x%X freed\n", taskPtr);
#endif NUC_DEBUG

	return(NTR_LEAVE( SUCCESS ));
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsiAuthenticateTask.4k)
 * NAME
 *		NCPsiAuthenticateTask -	Authenticate a previously task to the
 *								service it's connected to.
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiAuthenticateTask( taskPtr, userName, authKey, authKeyLen )
 *		ncp_task_t	*taskPtr;
 *		char	*userName;
 *		uint8	*authKey;
 *		uint8	authKeyLen;
 *
 * INPUT
 *		ncp_task_t	*taskPtr;
 *		char	*userName;
 *		uint8	*authKey;
 *		uint8	authKeyLen;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		Given a handle to a previously create service task, a username, 
 *		and password, authenticate this connection instance.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiAuthenticateTask (	ncp_task_t	*taskPtr,
						uint32		authType,
						uint32		objectID,
						uint8		*authKey,
						uint8		authKeyLen )
{
	ncp_auth_t	*authPtr;
	uint32		securityFlags;
	md4_t		*md4 = NULL;

	NTR_ENTER(5, taskPtr, authType, objectID, authKey, authKeyLen );

	/*
	 *	Move in the NetWare user ID and login key
	 */
	authPtr = (ncp_auth_t *)taskPtr->authInfoPtr;
	authPtr->objectID = objectID;

	if (((ncp_channel_t *)(taskPtr->channelPtr))->spiTaskPtr != NULL) {
		md4 = ((SPI_TASK_T *)
			(((ncp_channel_t *)(taskPtr->channelPtr))->spiTaskPtr))->md4;
	} else {
		return( NTR_LEAVE( FAILURE ) );
	}
	securityFlags = ((ncp_channel_t *)(taskPtr->channelPtr))->securityFlags;

	if( securityFlags & SECURITY_MIS_MATCH ) {
		return( NTR_LEAVE( FAILURE ) );
	}

	if (securityFlags & NWC_SECURITY_LEVEL_SIGN_HEADERS) {

		if (md4 == NULL) {
			((SPI_TASK_T *)
				(((ncp_channel_t *)(taskPtr->channelPtr))->spiTaskPtr))->md4 =
					(md4_t *)kmem_alloc (sizeof(md4_t), KM_SLEEP);

			md4 = ((SPI_TASK_T *)
				(((ncp_channel_t *)(taskPtr->channelPtr))->spiTaskPtr))->md4;

    	    NTR_PRINTF (
				"NCPsiAuthenticateTask: Allocate md4 = 0x%x", md4, 0, 0 );

#ifdef NUCMEM_DEBUG
			NTR_PRINTF (
				"NUCMEM: NCPsiAuthenticateTask: alloc md4_t x%x, size = 0x%x",
				md4, sizeof(md4_t), 0 );
#endif NUCMEM_DEBUG

		}

		if( authType == NWC_AUTH_STATE_BINDERY ) {
			generateSessionKey(&authKey[0], &authKey[16], md4->sessionKey);
		} else {
			bcopy( (caddr_t)authKey,
					   (caddr_t)md4->sessionKey, SESSION_KEY_LENGTH );
		}
		InitMD4State(md4->currentMessageDigest);
		InitMD4State(md4->previousMessageDigest);
	}
	taskPtr->isAuthenticated = TRUE;
	((ncp_channel_t *)taskPtr->channelPtr)->authenticationState = authType;

	NTR_LEAVE( SUCCESS );

	return(SUCCESS);

}


/*
 * BEGIN_MANUAL_ENTRY(NCPsiRawNCP.3k)
 * NAME
 *		NCPsiRawNCP -	Raw NCP request 
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiRawNCP( taskPtr, ncpHeader, sHdrLen, rHdrLen, ncpData, sDatLen, rDatLen)
 *		void_t	*taskPtr;
 *		void_t	*ncpHeader;
 *		int32	sHdrLen, rHdrLen;
 *		void_t	*ncpData;
 *		int32	sDatLen, rDatLen;
 *
 * INPUT
 *		void_t	*taskPtr;
 *		void_t	*ncpHeader;
 *		int32	sHdrLen, rHdrLen;
 *		void_t	*ncpData;
 *		int32	sDatLen, rDatLen;
 *
 * OUTPUT
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *	Takes a RAW ncp down from user land and sends it out.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiRawNCP (	void_t	*taskPtr,
				void_t	*token, 
				void_t	*ncpHeader,
				int32	sHdrLen,
				int32	*rHdrLen,
				int32	hkResFlag, 
				void_t	*ncpData,
				int32	sDatLen,
				int32	*rDatLen,
				int32	dkResFlag )
{
	ccode_t	ccode = SUCCESS;
	void_t	*channelPtr;

	NTR_ENTER(5, taskPtr, token, ncpHeader, sHdrLen, sDatLen );

	/*
	 *	Get the I/O channel used for transmission and receipt
	 */
#ifdef NUC_DEBUG
    NCPsilValidateTask( taskPtr );
#endif NUC_DEBUG

    channelPtr = ((ncp_task_t *)taskPtr)->channelPtr;
  
	ccode = NCPspRawNCP_l ( channelPtr, token, 
						ncpHeader, sHdrLen, rHdrLen, hkResFlag,
						ncpData, sDatLen, rDatLen, dkResFlag );

	NTR_LEAVE( ccode );

	return(ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsiRegisterRawNCP.3k)
 * NAME
 *		NCPsiRegisterRawNCP
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsiRegisterRawNCP( taskPtr, token )
 *		void_t		*taskPtr;
 *		void_t		*token;
 *
 * INPUT
 *		void_t		*taskPtr;
 *
 * OUTPUT
 *		void_t		*token;
 *
 * RETURN VALUES
 *		SUCCESS	- Task assigned to this token
 *		FAILURE	- No more available tasks
 *
 * DESCRIPTION
 *		Gets the channel pointer for this task and allocates a task from
 *		the channel's bitmask.  If the mask is full, this call will fail
 *		otherwise, a task number is returned in token.
 *
 * NOTES
 *		
 *	LOCKS THAT MAY BE HELD WHEN CALLED:
 *		nucUpperLock
 *
 *	LOCKS HELD WHEN RETURNED:
 *		nucUpperLock
 *
 *
 * SEE ALSO
 *		NCPsiRelinquishRawToken(3k)
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiRegisterRawNCP (void_t *taskPtr, void_t *token,
							uint32 *connectionReference )
{
	void_t	*channelPtr;
	ccode_t	ccode;

	NTR_ENTER(3, taskPtr, token, connectionReference, 0, 0 );

#ifdef NUC_DEBUG
    NCPsilValidateTask( taskPtr );
#endif NUC_DEBUG

    channelPtr = ((ncp_task_t *)taskPtr)->channelPtr;
  
	ccode = NCPdplGetFreeChannelTaskNumber_l (channelPtr, token,
				connectionReference );

	NTR_LEAVE( ccode );

	return(ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsiRelinquishRawToken.3k)
 * NAME
 *		NCPsiRelinquishRawToken - Return the token to NCP
 *
 * SYNOPSIS
 *		NCPsiRelinquishRawToken( taskPtr, token )
 *		void_t		*taskPtr;
 *		void_t		*token;
 *
 * INPUT
 *		void_t		*taskPtr;
 *		void_t		*token;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		Causes the task number previously allocated in RegisterRawNCP
 *		to be made available to other tasks, and sends and EndOfJob
 *		NCP to the target server.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiRelinquishRawToken (void_t *taskPtr, void_t *token )
{
	void_t	*channelPtr;

	NTR_ENTER(2, taskPtr, token, 0, 0, 0 );

#ifdef NUC_DEBUG
    NCPsilValidateTask( taskPtr );
#endif NUC_DEBUG

    channelPtr = ((ncp_task_t *)taskPtr)->channelPtr;
  
	NCPspEndOfTask_l ( channelPtr, token );
	NCPdplClearChannelTaskNumber_l ( channelPtr, token );

	NTR_LEAVE( SUCCESS );

	return(SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsiGetBroadcastMessage.3k)
 * NAME
 *		NCPsiGetBroadcaseMessage - Sleeps until awaken by an asynchronous
 *			event handler, then retrieves a broadcast message from the server
 *			and return it to the caller.
 *
 * SYNOPSIS
 *		NCPsiGetBroadcastMessage( message )
 *		NWSI_MESSAGE_T	*message;
 *
 * INPUT
 *		NWSI_MESSAGE_T	*message;
 *
 * OUTPUT
 *		NWSI_MESSAGE_T	*message;
 *
 * RETURN VALUES
 *		SPI_SUCCESS
 *		SPI_FAILURE
 *
 * DESCRIPTION
 *		This function sleeps at a priority allowing a signal to cause a
 *		wakeup.  If this function was awaken by a signal, it returns
 *		SPI_INTERRUPTED.  Otherwise a brodcast message is retrieved from
 *		the server and returned to the caller.
 *
 * NOTES
 *		This function sleeps on &NCPbroadcastMessageQueue until awaken.
 *		Currently NCPbroadcastMessageQueue is a pointer to an ncp_task_t
 *		from which a message should be retrieved.  The ncp_task_t pointer
 *		is returned to the caller.
 *
 * SEE ALSO
 *		NCPspMessageSocketEventHandler
 *		NCPspGetBroadcastMessage
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsiGetBroadcastMessage (NWSI_MESSAGE_T *message )
{
	ncp_task_t		*taskPtr;
	ncp_auth_t		*authPtr;
	ccode_t			ccode = SUCCESS;
	pl_t			pl;
	
	NTR_ENTER(1, message, 0, 0, 0, 0 );

	pl = LOCK(nucLock, plstr);
	for (;;) {

		/*
		 * If the queue contains -1, a shutdown is in progress.
		 * Terminate this thread with failure.  If there is a pending
		 * message, return it.  Otherwise, sleep waiting for a message.
		 * If the queue contains a NULL entry after returning from
		 * sleep, there are other consumers of broadcast messages
		 * waiting on this  queue.  Terminate this thread if another is
		 * active.
		 *
		 * TODO:  queue of length one?
		 */
		if (spiState == SPI_LAYER_INACTIVE) {
			ccode = SPI_INTERRUPTED;
			break;
		}
		if (NCPbroadcastMessageQueue == (void_t *)-1) {
			ccode = FAILURE;
			break;
		}
		if (NCPbroadcastMessageQueue) {

			/*
			 * We have a message queued.  Get the message from the
			 * queue and clear it for the next message.
			 */
			taskPtr = (ncp_task_t *) NCPbroadcastMessageQueue;
			NCPbroadcastMessageQueue = NULL;
			UNLOCK(nucLock, pl);
			message->spilTaskPtr = taskPtr->spilTaskPtr;
			authPtr = taskPtr->authInfoPtr;
			SLEEP_LOCK (((ncp_channel_t *)(taskPtr->channelPtr))->connSleepLock,
						NUCCONNSLEEPPRI);
			ccode = NCPspGetBroadcastMessage_l ( taskPtr->channelPtr, message );
			SLEEP_UNLOCK (((ncp_channel_t *)(taskPtr->channelPtr))->connSleepLock);
			if (ccode == NWD_GTS_NO_MESSAGE) {

				/* Give up broadcast queue reference. */

				NWslSetTaskNotInUse_l(message->spilTaskPtr);
				message->spilTaskPtr = NULL;
				pl = LOCK (nucLock, plstr);
				continue;
			}
			pl = LOCK (nucLock, plstr);
			break;
		}
		if (SV_WAIT_SIG(NCPbroadcastMessageQueueSV, primed, nucLock)
				!= B_TRUE) {
			pl = LOCK (nucLock, plstr);
			ccode = SPI_INTERRUPTED;
			break;
		}
		pl = LOCK (nucLock, plstr);
		if (!NCPbroadcastMessageQueue) {
			ccode = FAILURE;
			break;
		}
	}
	UNLOCK(nucLock, pl);
	NTR_LEAVE(ccode);
	return(ccode);
}

void
generateSessionKey (unsigned char *encryptedPassword, unsigned char *k,
						  unsigned char *sessionKey)
{
	uint32 messageDigest[4];
	uint8 block[64];

	bzero((caddr_t)&block[0], 64);
	bcopy((caddr_t)encryptedPassword, (caddr_t)&block[0], 16);
	bcopy((caddr_t)k, (caddr_t)&block[16], 8);
	bcopy((caddr_t)"Authorized NetWare Client", (caddr_t)&block[24], 25);
	InitMD4State(messageDigest);
	BuildMessageDigest(messageDigest, block);
	bcopy((caddr_t)&messageDigest[0], (caddr_t)sessionKey, 8);
}

ccode_t
securityMatch_l (ncp_channel_t *channelPtr, uint8 serverSecurityLevel )
{

	NTR_ENTER(2, channelPtr, serverSecurityLevel, 0, 0, 0 );

	NTR_ASSERT (channelPtr->spiTaskPtr != NULL);

	channelPtr->securityFlags = 0;
	((SPI_TASK_T *)(channelPtr->spiTaskPtr))->ipxchecksum = 0;

	if( serverSecurityLevel & SIGNATURE_REQUESTED_BIT ) {
		channelPtr->securityFlags |= NWC_SECURITY_SIGNING_IN_USE;
		channelPtr->securityFlags |= NWC_SECURITY_LEVEL_SIGN_HEADERS;
	}
#ifdef LATER
	else {
		if( ((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4 ) {
			kmem_free (((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4,
					sizeof(md4_t));
#ifdef NUCMEM_DEBUG
		NTR_PRINTF(
			"NUCMEM_FREE: securityMatch_l: free md4_t * at 0x%x, size = 0x%x",
			((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4, sizeof(md4_t), 0 );
#endif
			((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4 = NULL;
		}
	}
#endif LATER

	if( serverSecurityLevel & CHECKSUMMING_REQUESTED_BIT ) {
		/* NOTE: We need to start ipx checksums IMMEDIATELY so
		   the appropriate supporting code must be inserted here. */
		channelPtr->securityFlags |= NWC_SECURITY_LEVEL_CHECKSUM;
		((SPI_TASK_T *)(channelPtr->spiTaskPtr))->ipxchecksum =
				NWC_SECURITY_LEVEL_CHECKSUM;
	}

#ifdef LATER
	if( serverSecurityLevel & COMPLETE_SIGNATURE_REQUESTED_BIT ) {
		channelPtr->securityFlags |= NWC_SECURITY_SIGNING_INUSE;
		channelPtr->securityFlags |= NWC_SECURITY_LEVEL_SIGN_ALL;
	}
	if( serverSecurityLevel & ENCRYPTION_REQUESTED_BIT ) {
		channelPtr->securityFlags |= NWC_SECURITY_SIGNING_INUSE;
		channelPtr->securityFlags |= NWC_SECURITY_LEVEL_ENCRYPT;
	}
	/* Not sure what LIP support we would need now */
#endif LATER

	NTR_LEAVE( SUCCESS );
	return (SUCCESS);

}

ccode_t
securityMisMatch_l (uint32 userSecurityLevel, uint8 serverSecurityLevel,
						ncp_channel_t *channelPtr )
{
	ccode_t		ccode=SUCCESS;

	NTR_ENTER(3, userSecurityLevel, serverSecurityLevel, channelPtr, 0, 0 );

	NTR_ASSERT (channelPtr->spiTaskPtr != NULL);

	/* there appears to be server weirdness which can cause netware
	 *	servers to change signature state after the first
	 *	packet 97 exchange.  (This behavior only bites us on primary 
	 *	connections.)  We will try to work around for now 
	 *	by inserting the following code.
	 */
	if( ((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4 ) {
		kmem_free ( ((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4,
				sizeof(md4_t));

		NTR_PRINTF (
			"securityMisMatch_l: free md4 0x%x",
			((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4, 0, 0 );

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: securityMisMatch_l: free md4 * at 0x%x, size = 0x%x",
			((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4, sizeof(md4_t), 0 );
#endif

		((SPI_TASK_T *)(channelPtr->spiTaskPtr))->md4 = NULL;
	}

	/*
	 * The server wants something different, lets see
	 * if we can accomodate it.
	 */

	/*
	 * Eager to please, our NW servers start checksumming after the first
	 * packet 97, irregardless of any other mismatched options.
	 */
	if( serverSecurityLevel & CHECKSUMMING_REQUESTED_BIT ) {
		if( (userSecurityLevel & DO_IPX_CHECKSUM_BITS) != 0 ) {
			channelPtr->securityFlags |= NWC_SECURITY_LEVEL_CHECKSUM;
			((SPI_TASK_T *)(channelPtr->spiTaskPtr))->ipxchecksum =
					NWC_SECURITY_LEVEL_CHECKSUM;
		}
	} 

	switch( userSecurityLevel & DO_IPX_CHECKSUM_BITS ) {
		case 0x00: 	/* client will not do checksums */
		{
			if( serverSecurityLevel & CHECKSUMMING_REQUESTED_BIT ) {
				channelPtr->securityFlags |= SECURITY_MIS_MATCH;
				ccode = SPI_SERVER_FAILURE;
      			goto error;
			} 
			break;
		}
		case 0x0c:	/* client insists on checksums */
		{
			if( !(serverSecurityLevel & CHECKSUMMING_REQUESTED_BIT) ) {
				channelPtr->securityFlags |= SECURITY_MIS_MATCH;
				ccode = SPI_ACCESS_DENIED;
      			goto error;
			}
			break;
		}
		default:
		{
      		break;
		}
	}

	switch( userSecurityLevel & DO_HEADER_SIGNATURE_BITS ) {
		case 0x00: 	/* client will not do signatures */
		{
			if( serverSecurityLevel & SIGNATURE_REQUESTED_BIT ) {
				channelPtr->securityFlags |= SECURITY_MIS_MATCH;
				ccode = SPI_ACCESS_DENIED;
      			goto error;
			} 
			break;
		}
		case 0x03:		/* client insists on signatures */
		{
			if( !(serverSecurityLevel & SIGNATURE_REQUESTED_BIT) ) {
				channelPtr->securityFlags |= SECURITY_MIS_MATCH;
				ccode = SPI_ACCESS_DENIED;
      			goto error;
			}
			break;
		}
		default:
		{
      		break;
		}
	}

#ifdef LATER
	switch( userSecurityLevel & DO_COMPLETE_SIGNATURE_BITS ) {
		case 0x00: 	/* client will not do complete signatures */
		{
			if( serverSecurityLevel & COMPLETE_SIGNATURE_REQUESTED_BIT ) {
				ccode = SPI_ACCESS_DENIED;
      			goto error;
			} 
			break;
		}
		case 0x30:	/* client insists on complete signatures */
		{
			if( !(serverSecurityLevel & COMPLETE_SIGNATURE_REQUESTED_BIT) ) {
				ccode = SPI_ACCESS_DENIED;
      			goto error;
			}
			break;
		}
		default:
		{
      		break;
		}
	}

	switch( userSecurityLevel & DO_ENCRYPTION_BITS ) {
		case 0x00: 	/* client will not do encryption */
		{
			if( serverSecurityLevel & ENCRYPTION_REQUESTED_BIT ) {
				ccode = SPI_ACCESS_DENIED;
      			goto error;
			} 
			break;
		}
		case 0xc0:	/* client insists on encryption */
		{
			if( !(serverSecurityLevel & ENCRYPTION_REQUESTED_BIT) ) {
				ccode = SPI_ACCESS_DENIED;
      			goto error;
			}
			break;
		}
		default:
		{
      		break;
		}
	}
#else LATER
	if( serverSecurityLevel & COMPLETE_SIGNATURE_REQUESTED_BIT ) {
		channelPtr->securityFlags |= SECURITY_MIS_MATCH;
		ccode = SPI_ACCESS_DENIED;
   		goto error;
	} 
	if( serverSecurityLevel & ENCRYPTION_REQUESTED_BIT ) {
		channelPtr->securityFlags |= SECURITY_MIS_MATCH;
		ccode = SPI_ACCESS_DENIED;
    	goto error;
	}
#endif LATER

	/* LIP is okay if the negotiated packet size is not larger than MAX */
        if( (!(serverSecurityLevel & LARGE_INTERNET_PACKET_BIT))  &&
			(channelPtr->negotiatedBufferSize > MAX_NEGOTIATED_BUFFER_SIZE) ) {
		/* the server wants large internet packets */
		channelPtr->securityFlags |= SECURITY_MIS_MATCH;
		ccode = SPI_ACCESS_DENIED;
	}

error:
	NTR_LEAVE( ccode );
	return( ccode );

}

ccode_t
NCPsiLicenseTask (ncp_task_t *taskPtr, uint32 flags)
{
	ncp_channel_t	*channelPtr;
	ccode_t 			ccode;

#ifdef NUC_DEBUG
    NCPsilValidateTask( taskPtr );
#endif NUC_DEBUG

    channelPtr = taskPtr->channelPtr;
 
	ccode = NCPspLicenseConnection_l ( channelPtr, flags );

	return( ccode );
}

ccode_t
negotiateSecurityLevels_l  (void_t *credStruct, ncp_channel_t *channelPtr ) 
{
	ccode_t					ccode=SUCCESS;
	extern initListEntry_t  *initList;
	initListEntry_t 		*initPtr;
	uint32					userSecurityLevel;
	uint16					requestedPacketSize;
	uint16					serverPacketSize;
	uint8					serverSecurityLevel=0;
	uint8					requestedSecurityLevel;
	uint8					tempSigLevel = 0, tempChecksumLevel = 0;
	extern uint32			defaultChecksumLevel;
	extern uint32			checksumRequiredLevel;
	extern uint32			defaultSecurityLevel;
	extern uint32			signatureRequiredLevel;


	/*
	 *	NCP 97 stuff: With this NCP we determine signature level
	 *	to use, if ipx checksums are required, and if the server is
	 *	NDS capable. Note: for now we ALWAYS indicate that LIP is
	 *	not wanted.
	 *
     *  The design of this subroutine, allows a "suggestion" to be 
	 *	passed in from user when they call an initialization
	 *	routine. 
     */

    
	/*
	 *	Only allow header signature & ipx bits for now
	 */
	if( signatureRequiredLevel == 0xFF ) {
		userSecurityLevel = defaultSecurityLevel;
	} else {
		userSecurityLevel = signatureRequiredLevel;
	}

	if( checksumRequiredLevel == 0xFF ) {
		userSecurityLevel |= (defaultChecksumLevel << 2);
	} else {
		userSecurityLevel |= (checksumRequiredLevel << 2);
	}


	userSecurityLevel = userSecurityLevel & 
		(DO_HEADER_SIGNATURE_BITS | DO_IPX_CHECKSUM_BITS);


#ifdef LATER
	requestedPacketSize = defaultPacketSize;
#else LATER
	requestedPacketSize = MAX_NEGOTIATED_BUFFER_SIZE;
#endif	LATER
	
    if( initList != 0 ) {
        /* See if list entry exists for this uid */
        initPtr = initList;
        do {
            if( initPtr->uid == ((nwcred_t *)credStruct)->userID ) {
				/* check if signatures levels have been increased lately */
				tempSigLevel = initPtr->securityFlags &
							SET_USER_PREFERENCES_BITS;
				tempChecksumLevel = (initPtr->securityFlags >> 2) &
							SET_USER_PREFERENCES_BITS;
				if( tempSigLevel < signatureRequiredLevel &&
					signatureRequiredLevel != 0xFF ) {
                	userSecurityLevel = signatureRequiredLevel;
				} else {
					userSecurityLevel = tempSigLevel;
				}

				if( tempChecksumLevel < checksumRequiredLevel &&
					checksumRequiredLevel != 0xFF ) {
                	userSecurityLevel |= checksumRequiredLevel << 2;
				} else {
					userSecurityLevel |= tempChecksumLevel << 2;
				}
#ifdef LATER
                requestedPacketSize = initPtr->packetSize;
#endif	LATER
                break;
            }
            initPtr = initPtr->initForwardPtr;
        } while( initPtr != 0 );
    }

	if( userSecurityLevel & TRY_FOR_HEADER_SIGNATURES ) {
		serverSecurityLevel |= SIGNATURE_REQUESTED_BIT;
	}
	
	/* for now, just disable LIP */
	serverSecurityLevel |= LARGE_INTERNET_PACKET_BIT;

	if( userSecurityLevel & TRY_FOR_IPX_CHECKSUMS ) {
		serverSecurityLevel |= CHECKSUMMING_REQUESTED_BIT;
	}

#ifdef NWSN_DEBUG
	NCP_CMN_ERR (CE_CONT,
		"DEBUG: negotiateSecurityLevels(): userSecurityLevel = 0x%x\n", 
		userSecurityLevel );
	NCP_CMN_ERR (CE_CONT,
		"DEBUG: negotiateSecurityLevels(): serverSecurityLevel = 0x%x\n", 
		serverSecurityLevel );
#endif NWSN_DEBUG


#ifdef LATER
	if( userSecurityLevel & DO_COMPLETE_SIGNATURE_BITS ) {
		serverSecurityLevel |= COMPLETE_SIGNATURE_REQUESTED_BIT;
	}
	if( userSecurityLevel & DO_ENCRYPTION_BITS ) {
		serverSecurityLevel |= ENCRYPTION_REQUESTED_BIT;
	}
#endif LATER

#ifdef LATER
	serverPacketSize = requestedPacketSize;
#else LATER
	serverPacketSize = MAX_NEGOTIATED_BUFFER_SIZE;
#endif LATER
	requestedSecurityLevel = serverSecurityLevel;
	
	ccode = NCPspMaxPacketSize_l ( channelPtr, &serverPacketSize,
		&serverSecurityLevel );

	if( ccode == FAILURE ) {
		/*
		 *	Server doesn't support NCP 97
		 */
		serverSecurityLevel = 0x80;
		ccode = securityMisMatch_l ( userSecurityLevel, 
				serverSecurityLevel, channelPtr );
		if( ccode == SPI_SERVER_FAILURE ) {
			ccode = SPI_ACCESS_DENIED;
		} else {
			ccode = SUCCESS;
		}
	} else if ( !ccode ) {
		/*
		 * Check to see what the server wants
		 */
		if( requestedSecurityLevel == serverSecurityLevel ) {
			/*
			 *	Score! a match on the first try!
			 */
			ccode = securityMatch_l ( channelPtr, serverSecurityLevel);
		} else {
			/*
			 *	the server wants something different, lets see
			 *	if we can accomodate it.
			 */
			if( (ccode = securityMisMatch_l ( userSecurityLevel, 
				serverSecurityLevel, channelPtr )) ) {
				if( ccode == SPI_ACCESS_DENIED )
					ccode = SUCCESS;
			} else {

				/*
				 *	We have to resend the NCP 97
				 */
				requestedSecurityLevel = serverSecurityLevel;
				ccode = NCPspMaxPacketSize_l ( channelPtr, &serverPacketSize,
					&serverSecurityLevel );
	
				if( ccode == FAILURE ) {
					/*
					 *	Server doesn't support NCP 97
					 */
					serverSecurityLevel = 0x80;
					ccode = SUCCESS;
				}
				if( !ccode ) {
					if( requestedSecurityLevel == serverSecurityLevel ) {
						ccode = securityMatch_l ( channelPtr, 
							serverSecurityLevel);
					} else {
						ccode = SPI_ACCESS_DENIED;
					}
				}
			}
		}
	}

	return( ccode );

}
