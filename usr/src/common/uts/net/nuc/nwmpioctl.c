/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/nwmpioctl.c	1.25"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/nwmpioctl.c,v 2.54.2.4 1995/02/12 23:37:32 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: nwmpioctl.c
 *			  ioctl library
 *
 *		Functions in this module:
 */ 

#ifdef _KERNEL_HEADERS
#include <mem/kmem.h>
#include <net/tiuser.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/requester.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/nwmpdev.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/ncpconst.h>
#include <net/nuc/ncpiopack.h>
#include <net/nuc/sistructs.h>
#include <net/nuc/spimacro.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>

#else _KERNEL_HEADERS

#include <sys/tiuser.h>
#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>
#include <sys/gtscommon.h>
#include <sys/requester.h>
#include <sys/nwmp.h>
#include <sys/nwmpdev.h>
#include <sys/nucerror.h>
#include <sys/slstruct.h>
#include <sys/nwportable.h>
#include <sys/nwctypes.h>
#include <sys/spilcommon.h>
#include <sys/slstruct.h>
#include <sys/ncpconst.h>
#include <sys/ncpiopack.h>
#include <sys/sistructs.h>
#include <sys/spimacro.h>
#include <sys/nuc_prototypes.h>

#endif _KERNEL_HEADERS

#define	NTR_ModMask	NTRM_nwmp
/* Stolen directly from lib code */
#define MAX_DN_CHARS	256
#define MAX_DN_BYTES	(2*(MAX_DN_CHARS+1))

/*
 *	variables needed by nwmpInternalCreateTask
 */
int spilInternalTaskAuthenticationSemaphore = -1;
int spilInternalTaskAuthenticationServerSemaphore = -1;
struct netbuf spilInternalTaskAuthenticationAddress;
sv_t *spilInternalTaskAuthenticationAddressSV;
char spilInternalTaskAuthenticationBuffer[MAX_ADDRESS_SIZE];
int spilInternalTaskAuthenticationQueue;
sv_t *spilInternalTaskAuthenticationQueueSV;
uint32 spilInternalTaskAuthenticationUid;
uint32 spilInternalTaskAuthenticationGid;
uint32 spilInternalTaskAuthenticationPid;	/* SLIME */
int16 spilInternalTaskAuthenticationXautoFlags;

extern lock_t	*nucLock;
extern sleep_t	*spiTaskListSleepLock;
extern int32	spiState;

/* Forward */
DNListEntry_t *getDNBPtr( uint32 userID );


/*
 * BEGIN_MANUAL_ENTRY(nwmpOpenService.3k)
 * NAME
 *		nwmpOpenService
 *
 * SYNOPSIS
 *		int
 *		nwmpOpenService( uargs, uid, gid )
 *		struct	openServiceTaskReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		struct	openServiceTaskReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *		SPI_USER_MEMORY_FAULT
 *
 * DESCRIPTION
 *		Opens a service instance
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
int
nwmpOpenServiceTask (comargs_t *comargs, struct openServiceTaskReq *uargs)
{
	int 	ccode;
	struct netbuf address;
	char	addressBuffer[MAX_ADDRESS_SIZE];

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	if ( uargs->address.len > MAX_ADDRESS_SIZE )
		return( NTR_LEAVE( SPI_NAME_TOO_LONG ));

	comargs->address = &address;
	comargs->address->maxlen = MAX_ADDRESS_SIZE;
	comargs->address->len = uargs->address.len;
	comargs->address->buf = addressBuffer;

	if (copyin( uargs->address.buf, addressBuffer, uargs->address.len ))
		return( NTR_LEAVE( SPI_USER_MEMORY_FAULT ));

	ccode = NWsiOpenService( comargs, uargs->mode, uargs->flags );
	if (ccode == SPI_NO_SUCH_SERVICE) {
		ccode = NWsiCreateService( comargs, SPROTO_NCP, NOVELL_IPX, 0);
		if (ccode == SUCCESS)
			ccode = NWsiOpenService( comargs, uargs->mode, uargs->flags );
	}

	return( NTR_LEAVE( ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(nwmpCloseServiceTask.3k)
 * NAME
 *		nwmpCloseServiceTask - Format arguments for calling
 *							   CloseService SPIL call
 *
 * SYNOPSIS
 *		int
 *		nwmpCloseServiceTask( uargs, uid, gid )
 *		struct closeServiceTaskReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		struct closeServiceTaskReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_USER_MEMORY_FAULT
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *
 * DESCRIPTION
 *
 * NOTES
 *
 * SEE ALSO
 *		nwmpioctl(3k), nwmpOpenServicTask(3k)
 *
 * END_MANUAL_ENTRY
 */
int
nwmpCloseServiceTask (comargs_t *comargs, uint flags)
{
	int ccode;

	NTR_ENTER(2, comargs, flags, 0, 0, 0);

	ccode = NWsiCloseService( comargs, flags );

	return( NTR_LEAVE( ccode ));
}

/*
 * BEGIN_MANUAL_ENTRY(nwmpRegisterRaw.3k)
 * NAME
 *		nwmpRegisterRaw
 *
 * SYNOPSIS
 *		int
 *		nwmpRegisterRaw( rargs, uid, gid )
 *		struct regRawReq *rargs;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_USER_MEMORY_FAULT
 *		SPI_CLIENT_RESOURCE_SHORTAGE
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
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
int
nwmpRegisterRaw (comargs_t *comargs, struct regRawReq *rargs,
				void_t *token, uint32 *connectionReference )
{
	int ccode;

	NTR_ENTER(4, comargs, rargs, token, connectionReference, 0);

	if ( rargs->address.len > MAX_ADDRESS_SIZE )
		return( NTR_LEAVE( SPI_NAME_TOO_LONG ));

	if (copyin( rargs->address.buf, comargs->address->buf,
			rargs->address.len )) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto cleanup;
	}
	comargs->address->len = rargs->address.len;

	ccode = NWsiRegisterRaw( comargs, token, connectionReference);
	if (ccode == SPI_NO_SUCH_SERVICE) {
		ccode = NWsiCreateService( comargs, SPROTO_NCP, NOVELL_IPX, 0);
		if (ccode == SUCCESS)
			ccode = NWsiRegisterRaw( comargs, token, connectionReference);
	}

cleanup:

	if (ccode) {
		token = (void_t *)0;
	}

	/*
	 *	The error code will be mapped in DKI when the 
	 *	call returns.  This will allow the general process to
	 *	be OS independent.
	 */

	return( NTR_LEAVE( ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(nwmpRawRequest.3k)
 * NAME
 *		nwmpRawRequest
 *
 * SYNOPSIS
 *		int
 *		nwmpRawRequest( rarg, uid, gid )
 *		struct rawReq *rarg;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		struct rawReq *rarg;
 *		int32	uid;
 *		int32	gid;
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
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
int
nwmpRawRequest (comargs_t *comargs, struct rawReq *rarg, void_t *token)
{
	int			ccode;

	NTR_ENTER(3, comargs, rarg, token, 0, 0);

	ccode = NWsiRaw( comargs, token, rarg->header, 
				rarg->sendHdrLen, &rarg->recvHdrLen, IOMEM_USER,
				rarg->data, rarg->sendDataLen, 
				&rarg->recvDataLen, IOMEM_USER );

	return( NTR_LEAVE( ccode));
}


/*
 * BEGIN_MANUAL_ENTRY(nwmpScanTask.3k)
 * NAME
 *		nwmpScanTask
 *
 * SYNOPSIS
 *		int
 *		nwmpScanTask( args, uid, gid )
 *		struct scanTaskReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		struct scanTaskReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * OUTPUT
 *		int		args;
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
int
nwmpScanTask (comargs_t *comargs, struct scanTaskReq *uargs)
{
	int ccode;
	struct	netbuf address;
	char	addressBuffer[MAX_ADDRESS_SIZE];
	nwcred_t credStruct;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	if ( uargs->address.len > MAX_ADDRESS_SIZE )
		return( NTR_LEAVE( SPI_NAME_TOO_LONG ));

	comargs->address = &address;
	comargs->address->maxlen = MAX_ADDRESS_SIZE;
	comargs->address->len = uargs->address.len;
	comargs->address->buf = addressBuffer;
	comargs->credential = &credStruct;
	credStruct.flags = NWC_OPEN_PUBLIC;

	if (copyin( uargs->address.buf, addressBuffer, uargs->address.len ))
		return( NTR_LEAVE( SPI_USER_MEMORY_FAULT));

	/*
	 *	If the special case is set (user ID is -1)
	 *	this is the beginning of the scan. Start at the
	 *	beginning of the task list
	 */

	NWtlSetCredUserID( &credStruct, uargs->userID );
	NWtlSetCredGroupID( &credStruct, uargs->groupID );

	ccode = NWsiScanServiceTasks( comargs, &(uargs->mode) );

	NWtlGetCredUserID( &credStruct, (uint32 *)&(uargs->userID));
	NWtlGetCredGroupID( &credStruct, (uint32 *)&(uargs->groupID));

	return( NTR_LEAVE( ccode) );
}

int
nwmpGetServerContext (comargs_t *comargs, struct getServerContextReq *uargs)
{
	int ccode;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	ccode = NWsiGetServerContext( comargs, uargs );

	return( NTR_LEAVE( ccode));
}

int
nwmpLicenseConn (comargs_t *comargs, uint32 flags)
{
	int ccode;

	NTR_ENTER(4, comargs, flags, 0, 0, 0);

	ccode = NWsiLicenseConn( comargs, flags );

	return(NTR_LEAVE(ccode));
}

int
nwmpMakeConnectionPermanent (comargs_t *comargs)
{
	int ccode;

	NTR_ENTER(1, comargs, 0, 0, 0, 0);

	ccode = NWsiMakeConnectionPermanent( comargs );
	
	return(NTR_LEAVE(ccode));
}

int
nwmpGetConnInfo (comargs_t *comargs, struct getConnInfoReq *uargs,
				uint32 handleLicense )
{
	char *connInfo;
	int ccode;

	NTR_ENTER(3, comargs, uargs, handleLicense, 0, 0);

	/* perhaps there is a better sanity check than 1000 */
	if (uargs->uInfoLen > 1000) {
		return(NTR_LEAVE(SPI_USER_MEMORY_FAULT));
	}

	connInfo = (char *)kmem_alloc (uargs->uInfoLen, KM_SLEEP);
#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM: nwmpGetConnInfo: alloc connInfo * at 0x%x, size = 0x%x",
			connInfo, uargs->uInfoLen, 0 );
#endif

	if (copyin((caddr_t)uargs->buffer, connInfo, uargs->uInfoLen)) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto done;
	}

	ccode = NWsiGetConnInfo( comargs, uargs->uInfoLevel, uargs->uInfoLen,
				connInfo, handleLicense );

	if (ccode == SUCCESS) {
		if (copyout(connInfo, (caddr_t)uargs->buffer, uargs->uInfoLen)) {
			ccode = SPI_USER_MEMORY_FAULT;
			goto done;
		}
	}

done:
	kmem_free (connInfo, uargs->uInfoLen);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM_FREE: nwmpGetConnInfo: free connInfo * at 0x%x, size = 0x%x",
		connInfo, uargs->uInfoLen, 0 );
#endif

	return(NTR_LEAVE(ccode));
}

int
nwmpSetConnInfo (comargs_t *comargs, struct setConnInfoReq *uargs)
{
	char *connInfo;
	int ccode;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	/* perhaps there is a better sanity check than 1000 */
	if (uargs->uInfoLen > 1000) {
		return(NTR_LEAVE(SPI_USER_MEMORY_FAULT));
	}

	connInfo = (char *)kmem_alloc (uargs->uInfoLen, KM_SLEEP);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
			"NUCMEM: nwmpSetConnInfo: alloc connInfo * at 0x%x, size = 0x%x",
			connInfo, uargs->uInfoLen, 0 );
#endif

	if (copyin((caddr_t)uargs->buffer, connInfo, uargs->uInfoLen)) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto done;
	}

	ccode = NWsiSetConnInfo( comargs, uargs->uInfoLevel, uargs->uInfoLen,
				connInfo );

done:
	kmem_free (connInfo, uargs->uInfoLen);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM_FREE: nwmpSetConnInfo: free connInfo * at 0x%x, size = 0x%x",
		connInfo, uargs->uInfoLen, 0 );
#endif

	return(NTR_LEAVE(ccode));
}

int
nwmpScanConnInfo (comargs_t *comargs, struct scanConnInfoReq *uargs)
{
	char *connInfo;
	char *scanInfo;
	int ccode;
	struct netbuf address;
	char addressBuffer[MAX_ADDRESS_SIZE];

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	if ( uargs->address.len > MAX_ADDRESS_SIZE )
		return( NTR_LEAVE( SPI_NAME_TOO_LONG ));

	comargs->address = &address;
	comargs->address->maxlen = MAX_ADDRESS_SIZE;
	comargs->address->len = uargs->address.len;
	comargs->address->buf = addressBuffer;

	if (copyin( uargs->address.buf, addressBuffer, uargs->address.len ))
		return( NTR_LEAVE( SPI_USER_MEMORY_FAULT));

	/* perhaps there is a better sanity check than 1000 */
	if (uargs->uInfoLen > 1000) {
		return(NTR_LEAVE(SPI_USER_MEMORY_FAULT));
	}

	connInfo = (char *)kmem_alloc (uargs->uInfoLen, KM_SLEEP);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: nwmpScanConnInfo: alloc connInfo * at 0x%x, size = 0x%x",
		connInfo, uargs->uInfoLen, 0 );
#endif

	if (uargs->uScanInfoLevel == NWC_CONN_INFO_RETURN_ALL) {
		scanInfo = (char *)NULL;
	} else {
		scanInfo = (char *)kmem_alloc (uargs->uScanInfoLen, KM_SLEEP);
#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM: nwmpScanConnInfo: alloc scanInfo * at 0x%x, size = 0x%x",
			scanInfo, uargs->uScanInfoLen, 0 );
#endif

		if (copyin((caddr_t)uargs->pScanConnInfo,
					   scanInfo, uargs->uScanInfoLen)) {
			ccode = SPI_USER_MEMORY_FAULT;
			goto done;
		}
	}

	if (copyin((caddr_t)uargs->buffer, connInfo, uargs->uInfoLen)) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto done;
	}

	/* just pass uargs */
	ccode = NWsiScanConnInfo( comargs, &uargs->luConnectionReference,
		uargs->uScanInfoLevel, scanInfo, uargs->uScanInfoLen, uargs->uScanFlags,
		uargs->uInfoLevel, uargs->uInfoLen, connInfo);

	if (ccode == SUCCESS) {
		if (copyout(connInfo, (caddr_t)uargs->buffer, uargs->uInfoLen)) {
			ccode = SPI_USER_MEMORY_FAULT;
			goto done;
		}
	}

done:
	kmem_free (connInfo, uargs->uInfoLen);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM_FREE: nwmpScanConnInfo: free connInfo * at 0x%x, size = 0x%x",
		connInfo, uargs->uInfoLen, 0 );
#endif

	if (scanInfo) {
		kmem_free (scanInfo, uargs->uScanInfoLen);

#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: nwmpScanConnInfo: free scanInfo * at 0x%x, size=0x%x",
			scanInfo, uargs->uScanInfoLen, 0 );
#endif

	}

	return(NTR_LEAVE(ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(nwmpScanServices.3k)
 * NAME
 *		nwmpScanServices
 *
 * SYNOPSIS
 *		int
 *		nwmpScanServices( uargs, uid, gid )
 *		struct scanServiceReq *uargs;
 *		int		args;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		struct scanServiceReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * OUTPUT
 *		int		args;
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
int
nwmpScanServices (struct scanServiceReq *uargs)
{
	int ccode;
	struct	netbuf address;
	char	addressBuffer[MAX_ADDRESS_SIZE];
	struct serviceInfoStruct serviceInfo;

	NTR_ENTER(1, uargs, 0, 0, 0, 0);

	if ( uargs->address.len > MAX_ADDRESS_SIZE )
		return( NTR_LEAVE( SPI_NAME_TOO_LONG ));

	address.maxlen = MAX_ADDRESS_SIZE;
	address.len = uargs->address.len;
	address.buf = addressBuffer;

	if (copyin( uargs->address.buf, addressBuffer, uargs->address.len )) {
		ccode = SPI_USER_MEMORY_FAULT;
		return( NTR_LEAVE(ccode));
	}
	
	/*
	 *	If the service address length is zero, the user wants
	 *	to rewind and start at the beginning of the service
	 *	list.  NWsiScanServices will start at the beginning of the list 
	 */
	if ( address.len == 0)
		serviceInfo.address = NULL;
	else
		serviceInfo.address = &address;

	/*
	 *	Return only those services that match uargs->serviceFlags
	 *	if uargs->serviceFlags is nonzero. 
	 */
	while ((ccode = NWsiScanServices(&serviceInfo)) == SUCCESS) {
		if (uargs->serviceFlags == 0)
			break;
		if ((serviceInfo.serviceFlags & uargs->serviceFlags) == 
			uargs->serviceFlags)
			break;
	}

	if (ccode == SUCCESS) {
		uargs->serviceProtocol = serviceInfo.serviceProtocol;
		uargs->transportProtocol = serviceInfo.transportProtocol; 
		uargs->serviceFlags = serviceInfo.serviceFlags;
		if (serviceInfo.address->len > uargs->address.maxlen ) {
			ccode = SPI_USER_MEMORY_FAULT;
			return( NTR_LEAVE(ccode));
		}
		if (copyout( serviceInfo.address->buf, uargs->address.buf, 
			serviceInfo.address->len) ) {
			ccode = SPI_USER_MEMORY_FAULT;
			return( NTR_LEAVE(ccode));
		}
		uargs->address.len = serviceInfo.address->len;
	}

	return( NTR_LEAVE(ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(nwmpScanServicesByUser.3k)
 * NAME
 *		nwmpScanServicesByUser
 *
 * SYNOPSIS
 *		int
 *		nwmpScanServicesByUser( uargs, uid, gid )
 *		struct scanServicesByUserReq *uargs;
 *		int		args;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		struct scanServicesByUserReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * OUTPUT
 *		int		args;
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		nwmpScanServicesByUser fills a user-supplied buffer with struct
 *		scanServiceByUserStruct which contains a service name and the name
 *		by which the calling process is authenticated on that service.
 *		A structure is filled in for each service authenticated by the user.
 *
 * NOTES
 *
 * SEE ALSO
 *		nwmpReturnSSBUBufferSize
 *		
 *
 * END_MANUAL_ENTRY
 */
enum NUC_DIAG
nwmpScanServicesByUser (comargs_t *comargs,
						struct scanServicesByUserReq *uargs)
{
	enum NUC_DIAG ccode;
	struct scanServicesByUserStruct *buffer;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	/*
	 *	Get a buffer equal in size to the one the user wants me to fill up
	 *  with struct scanServicesByUserStruct entries.
	 */
	buffer = (struct scanServicesByUserStruct*)
		kmem_alloc (uargs->serviceBufferLength, KM_SLEEP);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM:nwmpScanServicesByUser:scanServicesByUserStruct 0x%x,size=0x%x",
		buffer, uargs->serviceBufferLength, 0 );
#endif

	/*
	 *	call the SPIL layer with the Scan Services by User request
	 */	
	ccode = NWsiScanServicesByUser( comargs, buffer,
		uargs->serviceBufferLength, &uargs->serviceBufferUsed );

	/*
	 *	If all went well, copy the populated buffer back to user space
	 */
	if (ccode == SUCCESS || ccode == SPI_MORE_ENTRIES_EXIST ) {
		if (copyout((caddr_t)buffer,
						(caddr_t)uargs->serviceBuffer,
						uargs->serviceBufferUsed)) {
			ccode = SPI_USER_MEMORY_FAULT;
		}
	}

	kmem_free ( buffer, uargs->serviceBufferLength);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM_FREE:nwmpScanServicesByUser: free scanServicesByUserStruct 0x%x, size=0x%x",
		buffer, uargs->serviceBufferLength, 0 );
#endif

	return( NTR_LEAVE(ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(nwmpAuthenticateTask.3k)
 *
 * NAME
 *
 * SYNOPSIS
 *
 * INPUT
 *		Nothing
 *
 * OUTPUT
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
int
nwmpAuthenticateTask (comargs_t *comargs, struct  authTaskReq *uargs)
{
	int ccode;
	uint8 	*authKey = NULL;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	authKey = (uint8 *)kmem_zalloc (SPI_MAX_PASSWORD_LENGTH, KM_SLEEP);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: nwmpAuthenticateTask: alloc authKey * at 0x%x, size = 0x%x",
		authKey, SPI_MAX_PASSWORD_LENGTH, 0 );
#endif

	
	/*
	 *	Get the authKey from the uargs
	 */
	if ( uargs->authKeyLength ) {
		if (copyin( (caddr_t)uargs->authKey,
						(caddr_t)authKey, uargs->authKeyLength )) {
#ifdef NUC_DEBUG
			NWMP_CMN_ERR(CE_CONT,"NWMP:Auth Task CopyIn of authKey failure\n");
#endif NUC_DEBUG
			ccode = SPI_USER_MEMORY_FAULT;
			goto freeAllStorage;
		}
	}

	ccode = NWsiAuthenticate( comargs, uargs, authKey );

/*
 *	Free all storage acquired by this function.
 */
freeAllStorage:
	if( authKey != NULL )
		kmem_free ( authKey, SPI_MAX_PASSWORD_LENGTH);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM_FREE: nwmpAuthenticateTask: free authKey 0x%x, size = 0x%x",
		authKey, SPI_MAX_PASSWORD_LENGTH, 0 );
#endif NUCMEM_DEBUG

	return( NTR_LEAVE( ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(nwmpSetPrimaryService.3k)
 * NAME
 *		nwmpSetPrimaryService
 *
 * SYNOPSIS
 *		int
 *		nwmpSetPrimaryService( uargs, uid, gid )
 *		struct	setPrimaryServiceReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		struct	setPrimaryServiceReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *		SPI_USER_MEMORY_FAULT
 *		SPI_NO_SUCH_SERVICE
 *		SPI_NO_SUCH_TASK
 *
 * DESCRIPTION
 *		Designates the specified service as the Primary or default service.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
int
nwmpSetPrimaryService (comargs_t *comargs)
{
	int 	ccode;

	NTR_ENTER(1, comargs, 0, 0, 0, 0);

	ccode = NWsiSetPrimaryService(comargs);

	return(NTR_LEAVE(ccode));
}

/*
 * BEGIN_MANUAL_ENTRY(nwmpGetServiceMessage.3k)
 * NAME
 *		nwmpGetServiceMessage
 *
 * SYNOPSIS
 *		int
 *		nwmpGetServiceMessage( uargs, uid, gid )
 *		struct	getServiceMessageReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * INPUT
 *		struct	getServiceMessageReq *uargs;
 *		int32	uid;
 *		int32	gid;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *		SPI_USER_MEMORY_FAULT
 *		SPI_NO_SUCH_SERVICE
 *		SPI_NO_SUCH_TASK
 *
 * DESCRIPTION
 *		Retrieves messages from services that have indicated that they are
 *		holding messages for the user.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
int
nwmpGetServiceMessage (struct  getServiceMessageReq *uargs)
{
	int 	ccode;
	NWSI_MESSAGE_T	*spilMessage;

	NTR_ENTER(1, uargs, 0, 0, 0, 0);

	spilMessage = (NWSI_MESSAGE_T *)kmem_alloc (sizeof(NWSI_MESSAGE_T), 
		KM_SLEEP);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: nwmpGetServiceMessage: alloc NWSI_MESSAGE_T 0x%x, size=0x%x",
		spilMessage, sizeof(NWSI_MESSAGE_T), 0 );
#endif

	spilMessage->serviceProtocol = uargs->serviceProtocol;
	spilMessage->spilTaskPtr = NULL;
	spilMessage->spilServiceAddress.maxlen = MAX_ADDRESS_SIZE;
	spilMessage->spilServiceAddress.buf = spilMessage->buffer;

	ccode = NWsiGetServiceMessage( spilMessage );

	if ( ccode != SUCCESS ) 
		goto freeAllStorage;

	uargs->uid = spilMessage->uid;
	uargs->gid = spilMessage->gid;
	uargs->messageLength = spilMessage->messageLength;

	if (copyout(spilMessage->messageText, uargs->messageText, 
		spilMessage->messageLength)) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto freeAllStorage;
	}

	if (spilMessage->spilServiceAddress.len >
			uargs->spilServiceAddress.maxlen) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto freeAllStorage;
	}

	if (copyout(spilMessage->spilServiceAddress.buf,
			uargs->spilServiceAddress.buf,
			spilMessage->spilServiceAddress.len)) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto freeAllStorage;
	}
	uargs->spilServiceAddress.len = spilMessage->spilServiceAddress.len;

	if (copyout(spilMessage->sprotoServiceName, uargs->sprotoServiceName, 
		strlen(spilMessage->sprotoServiceName))) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto freeAllStorage;
	}

	if (copyout(spilMessage->sprotoUserName, uargs->sprotoUserName, 
		strlen(spilMessage->sprotoUserName))) {
		ccode = SPI_USER_MEMORY_FAULT;
		goto freeAllStorage;
	}

freeAllStorage:

	/*
	 *	It is given that the service protocol routine called by 
	 *	NWsiGetServiceMessage will call NWslSetTaskInUse
	 *	with the SPI_TASK_T was serviced.  NWslSetTaskNotInUse must now
	 *	be called to release the task.
	 */
	if ( spilMessage->spilTaskPtr != NULL )
		NWslSetTaskNotInUse_l ( spilMessage->spilTaskPtr );

	kmem_free ( spilMessage, sizeof(NWSI_MESSAGE_T));
#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM_FREE:nwmpGetServiceMessage:free NWSI_MESSAGE_T 0x%x, size=0x%x",
		spilMessage, sizeof(NWSI_MESSAGE_T), 0 );
#endif

	return( NTR_LEAVE( ccode ) );
}


/*
 * BEGIN_MANUAL_ENTRY(nwmpInternalCreateTask.3k)
 * NAME
 *		nwmpInternalCreateTask
 *
 * SYNOPSIS
 *		int
 *		nwmpInternalCreateTask( uargs )
 *		struct reqCreateServiceReq	*uargs;
 *
 * INPUT
 *		struct reqCreateServiceReq	*uargs;
 *
 * OUTPUT
 *		struct reqCreateServiceReq	*uargs;
 *
 * RETURN VALUES
 *		SUCCESS
 *		SPI_CLIENT_RESOURCE_SHORTAGE
 *		SPI_USER_MEMORY_FAULT
 *		SPI_NO_SUCH_SERVICE
 *		SPI_NO_SUCH_TASK
 *
 * DESCRIPTION
 *		This function is called by a user-space program via nwmpioctl that 
 *		expects the address of a service for whom a task is to be authenticated.
 *		The user-space program sleeps on spilInternalTaskAuthenticationQueue
 *		until awakened by another task.  When awakened, the address of the 
 *		service needing an authenticated task will be found in 
 *		spilInternalTaskAuthenticationAddress and the credentials of the 
 *		requestor will be found in spilInternalTaskAuthentication{Uid,Gid}.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
int
nwmpInternalCreateTask ( struct	reqCreateTaskReq	*uargs )
{
	pl_t		pl;
	boolean_t	bool;

	NTR_ENTER(1, uargs, 0, 0, 0, 0);

	/*
	 *	If the authentication semaphore has not yet been created, create it,
	 */
	if (spilInternalTaskAuthenticationSemaphore == -1) {
		if (NWtlCreateAndSetSemaphore(
				&spilInternalTaskAuthenticationSemaphore, 1)) {
			spilInternalTaskAuthenticationSemaphore = -1;
			return(NTR_LEAVE(SPI_CLIENT_RESOURCE_SHORTAGE));
		}
	}

	/* wrong place to initialize but it will work for now */
	spilInternalTaskAuthenticationAddress.buf =
		spilInternalTaskAuthenticationBuffer;
	spilInternalTaskAuthenticationAddress.maxlen = MAX_ADDRESS_SIZE;

	pl = LOCK (nucLock, plstr);

	SV_BROADCAST (spilInternalTaskAuthenticationAddressSV, 0);

	/*
	 *	Sleep until needed
	 */

	if ((bool = SV_WAIT_SIG (spilInternalTaskAuthenticationQueueSV,
					 primed, nucLock)) != B_TRUE) {
		spilInternalTaskAuthenticationSemaphore = -1;
		return(NTR_LEAVE(SPI_INTERRUPTED));
	}

/*****************************************
	SV_WAIT (spilInternalTaskAuthenticationQueueSV, primed, nucLock);
*****************************************/

	pl = LOCK (nucLock, plstr);
	if (spiState == SPI_LAYER_INACTIVE) {
		UNLOCK (nucLock, pl);
		return(NTR_LEAVE(SPI_INTERRUPTED));
	}
	UNLOCK (nucLock, pl);
	

	/*
	 *	This function has been awaken by a task needing an authentication.
	 *	Copy the service name and the tasks' credentials to user-space and 
	 *	return control to the user space caller.  The task needing the 
	 *	authentication is now sleeping on spilInternalTaskAuthenticationName.
	 */
	if (spilInternalTaskAuthenticationAddress.len > uargs->address.maxlen)
		return(NTR_LEAVE(SPI_USER_MEMORY_FAULT));
	if (copyout(spilInternalTaskAuthenticationAddress.buf, 
		uargs->address.buf, spilInternalTaskAuthenticationAddress.len))
			return(NTR_LEAVE(SPI_USER_MEMORY_FAULT));
	uargs->address.len = spilInternalTaskAuthenticationAddress.len;
	uargs->uid = spilInternalTaskAuthenticationUid;
	uargs->gid = spilInternalTaskAuthenticationGid;
	uargs->pid = spilInternalTaskAuthenticationPid;	/* SLIME */
	uargs->xautoFlags = spilInternalTaskAuthenticationXautoFlags;

	return( NTR_LEAVE( SUCCESS ) );
}

/*
 *		
 *	LOCKS EXPECTED TO BE HELD WHEN CALLED:
 *		none
 *		
 *	LOCKS HELD WHEN RETURNED:
 *		none
 *
 */
int
nwmpMakeSignatureDecision (comargs_t *comargs)
{
	int 		ccode = SUCCESS;
	SPI_SERVICE_T	*gService;
	SPI_TASK_T	*gTask;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	if ( NWslGetService( comargs->address, &gService ) )
		return( NTR_LEAVE( SPI_NO_SUCH_SERVICE ) );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
	if (GetTask_l( gService->taskList, comargs->credential, &gTask, FALSE )) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
		ccode = SPI_NO_SUCH_TASK;
		return( NTR_LEAVE( ccode ) );
	}

	SLEEP_UNLOCK (spiTaskListSleepLock);
	SLEEP_LOCK (gTask->spiTaskSleepLock, NUCSPITASKSLEEPPRI);

	/* Additional info returned. */
	if (gTask->mode & SPI_TASK_AUTHENTICATED) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		ccode = SPI_ACCESS_DENIED;
		return( NTR_LEAVE( ccode ) );
	}

    CHECK_CONN_REFERENCE( ccode, comargs->connectionReference,
        ((ncp_channel_t *)((ncp_task_t *)gTask->protoTaskPtr)->channelPtr)->connectionReference);
	
	if ( ccode ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		return( NTR_LEAVE( ccode ) );
	}

	if( ((ncp_channel_t *)((ncp_task_t *)gTask->protoTaskPtr)->channelPtr)->securityFlags & SECURITY_MIS_MATCH ) {
		SLEEP_UNLOCK (gTask->spiTaskSleepLock);
		ccode = SPI_TASK_HOLDING_RESOURCES;
		return( NTR_LEAVE( ccode ) );
	}

	SLEEP_UNLOCK (gTask->spiTaskSleepLock);

	return( NTR_LEAVE( ccode ) );
}

/* return user's security flags */
int
nwmpGetSecurityFlags (comargs_t *comargs, struct getSecurityFlagsReq *uargs)
{
	int 						ccode = SUCCESS;
	extern uint32				defaultSecurityLevel;
	extern uint32				signatureRequiredLevel;
    extern initListEntry_t		*initList;
    initListEntry_t            	*initPtr;


	uargs->defaultLevel = defaultSecurityLevel;
	uargs->baseLevel = signatureRequiredLevel;

	/* See if init entry exists for this uid. */
	if( initList != NULL ) {
    	initPtr = initList;
    	do {
        	if( initPtr->uid == comargs->credential->userID ) {
				/* We've found an existing ILE for this uid. */
				uargs->userLevel = initPtr->securityFlags &
					SET_USER_PREFERENCES_BITS;
    			return( NTR_LEAVE( ccode ) );
        	}
       	 	initPtr = initPtr->initForwardPtr;
    	} while( initPtr != NULL );
	}

	/* init entry doesn't exist, so allocate it and associated items. */
	uargs->userLevel = 0xFF;

	return( NTR_LEAVE( ccode ) );

}

/* return user's checksum flags */
int
nwmpGetChecksumFlags (comargs_t *comargs, struct getSecurityFlagsReq *uargs)
{
    int                         ccode = SUCCESS;
    extern uint32               defaultChecksumLevel;
    extern uint32               checksumRequiredLevel;
    extern initListEntry_t      *initList;
    initListEntry_t             *initPtr;


    uargs->defaultLevel = defaultChecksumLevel;
    uargs->baseLevel = checksumRequiredLevel;

    /* See if init entry exists for this uid. */
    if( initList != NULL ) {
        initPtr = initList;
        do {
            if( initPtr->uid == comargs->credential->userID ) {
                /* We've found an existing ILE for this uid. */
                uargs->userLevel = (initPtr->securityFlags >> 2) &
					SET_USER_PREFERENCES_BITS;
                return( NTR_LEAVE( ccode ) );
            }
            initPtr = initPtr->initForwardPtr;
        } while( initPtr != NULL );
    }

    /* init entry doesn't exist, so allocate it and associated items. */
    uargs->userLevel = 0xFF;

    return( NTR_LEAVE( ccode ) );

}


int
nwmpCheckConn (comargs_t *comargs)
{
    int             ccode = SUCCESS;
    SPI_SERVICE_T   *gService;
    SPI_TASK_T      *gTask;

	NTR_ENTER(1, comargs, 0, 0, 0, 0);

    if ( NWslGetService( comargs->address, &gService ) )
        return( NTR_LEAVE( SPI_NO_SUCH_SERVICE ) );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
    if (GetTask_l ( gService->taskList, comargs->credential, &gTask, FALSE )) {
        ccode = SPI_NO_SUCH_TASK;
    }
	SLEEP_UNLOCK (spiTaskListSleepLock);

    if( !ccode ) {
        CHECK_CONN_REFERENCE( ccode, comargs->connectionReference,
            ((ncp_channel_t *)((ncp_task_t *)gTask->protoTaskPtr)->channelPtr)->connectionReference);
    }

    return( NTR_LEAVE( ccode ) );

}

/* set the monitored connection reference */
int
nwmpSetMonitoredConn (comargs_t *comargs)
{
    int                     ccode=SUCCESS;
	SPI_SERVICE_T			*gService;
	SPI_TASK_T				*gTask;
	extern initListEntry_t	*initList;
	initListEntry_t			*initPtr;

	NTR_ENTER(1, comargs, 0, 0, 0, 0);

    if ( NWslGetService( comargs->address, &gService ) )
        return( NTR_LEAVE( SPI_NO_SUCH_SERVICE ) );

	SLEEP_LOCK (spiTaskListSleepLock, NUCSPITASKSLEEPPRI);
    if (GetTask_l ( gService->taskList, comargs->credential, &gTask, FALSE )) {
		SLEEP_UNLOCK (spiTaskListSleepLock);
        ccode = SPI_NO_SUCH_TASK;
    	return( NTR_LEAVE( ccode ) );
    }
	SLEEP_UNLOCK (spiTaskListSleepLock);
	
    CHECK_CONN_REFERENCE( ccode, comargs->connectionReference,
        ((ncp_channel_t *)((ncp_task_t *)gTask->protoTaskPtr)->channelPtr)->connectionReference);
	
	if( ccode )
    	return( NTR_LEAVE( ccode ) );


	/* find init entry */
	if( initList != 0 ) {
		/* See if list entry exists for this uid */
		initPtr = initList;
		do {
			if( initPtr->uid == comargs->credential->userID ) {
				initPtr->monitoredConn = comargs->connectionReference;
    			return( NTR_LEAVE( ccode ) );
			}
			initPtr = initPtr->initForwardPtr;
		} while( initPtr != 0 );
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
	} else {
		/* No ILE for this uid */
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
	}

    return( NTR_LEAVE( ccode ) );
	
}

/* get the monitored connection reference */
int
nwmpGetMonitoredConn (comargs_t *comargs, struct monConnReq *uargs)
{
    int                     ccode=SUCCESS;
    extern initListEntry_t  *initList;
    initListEntry_t         *initPtr;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);


    /* find init entry */
    if( initList != 0 ) {
        /* See if list entry exists for this uid */
        initPtr = initList;
        do {
            if( initPtr->uid == comargs->credential->userID ) {
                uargs->connReference = initPtr->monitoredConn;
    			return( NTR_LEAVE( ccode ) );
            }
            initPtr = initPtr->initForwardPtr;
        } while( initPtr != 0 );
    }
	/* No ILE for this uid 
	 * OR
	 * This error is used because we map it into NWERR_NOT_INITIALIZED
	 * error upstairs.
	 */

	ccode = SPI_INACTIVE;	

    return( NTR_LEAVE( ccode ) );

}

/* Create a Tagged Data Store */
int
nwmpAllocTDS (comargs_t *comargs, struct TDSReq *uargs)
{
    int             				ccode=SUCCESS;
	TDSListEntry_t					*TDSPtr;
	TDSListEntry_t					*TDSPtr1;
	extern TDSListEntry_t			*TDSList;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	/* Check to see if we have any TDS which match 
		uid/tagid allocated */
	TDSPtr = TDSList;
	while( TDSPtr != 0 ) {
		if( TDSPtr->uid == comargs->credential->userID ) {
			if( TDSPtr->tag == uargs->tag ) {
				/* set error to an already allocated type setting */
				/* This error is used because we map it into NWERR_TAG_IN_USE
					error upstairs.
				*/
				ccode = SPI_FILE_ALREADY_EXISTS;	
    			return( NTR_LEAVE( ccode ) );
			}
		}
		TDSPtr = TDSPtr->TDSForwardPtr;
	}

	/* Allocate a new TDS Entry and TDS */
    TDSPtr1 = (TDSListEntry_t *)kmem_zalloc(sizeof(TDSListEntry_t), KM_SLEEP);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: nwmpAllocTDS: alloc TDSListEntry_t * at 0x%x, size = 0x%x",
		TDSPtr1, sizeof(TDSListEntry_t), 0 );
#endif

	if( uargs->maxSize > MAX_TDS_SIZE )
		uargs->maxSize = MAX_TDS_SIZE;
    TDSPtr1->bufPtr = (uint8 *)kmem_zalloc ( uargs->maxSize, KM_SLEEP );
#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE: nwmpAllocTDS: free TDSListEntry_t 0x%x, size=0x%x",
			TDSPtr1, sizeof(TDSListEntry_t), 0 );
#endif

	/* Set TDS info */
	TDSPtr1->uid = comargs->credential->userID;
	TDSPtr1->tag = uargs->tag;
	TDSPtr1->maxSize = uargs->maxSize;
	TDSPtr1->flags = uargs->flags;		/* not currently used */

	/* Add new TDS Entry to the list */
	if( TDSList == NULL ) {
		TDSList = TDSPtr1;
	} else {
		TDSPtr = TDSList;
		while( TDSPtr->TDSForwardPtr != NULL ) {
			TDSPtr = TDSPtr->TDSForwardPtr;
		}
		TDSPtr->TDSForwardPtr = TDSPtr1;
	}

    return( NTR_LEAVE( ccode ) );

}


/* Free a Tagged Data Store */
int
nwmpFreeTDS (comargs_t *comargs, struct TDSReq *uargs)
{
    int                             ccode=SUCCESS;
    TDSListEntry_t                  *TDSPtr;
    TDSListEntry_t                  *TDSPtrToLast1;
	extern TDSListEntry_t			*TDSList;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	/* see if any TDS exists */
	if( TDSList == NULL ) {
		/* No TDS allocated */
		/* This error is used because we map it into NWERR_INVALID_TAG
			error upstairs.
		*/
		ccode = SPI_CANT_OPEN_DIRECTORY;	
    	return( NTR_LEAVE( ccode ) );
	}

	/* see if our TDS exists */
	/* This error is used because we map it into NWERR_INVALID_TAG
		error upstairs.
	*/
	ccode = SPI_CANT_OPEN_DIRECTORY;	
	TDSPtrToLast1=NULL;
	TDSPtr = TDSList;
	do {
		if( TDSPtr->uid == comargs->credential->userID ) {
			if( TDSPtr->tag == uargs->tag ) {
				/* we found a TDS to free! */
				if( TDSPtrToLast1 == NULL ) {
					TDSList = TDSPtr->TDSForwardPtr;
				} else {
					TDSPtrToLast1->TDSForwardPtr = TDSPtr->TDSForwardPtr;
				}
				kmem_free ( TDSPtr->bufPtr, uargs->maxSize);
#ifdef NUCMEM_DEBUG
				NTR_PRINTF (
					"NUCMEM_FREE: nwmpFreeTDS: free TDS_buffer 0x%x, size=0x%x",
					TDSPtr->bufPtr, uargs->maxSize, 0 );
#endif
				kmem_free ( TDSPtr, sizeof(TDSListEntry_t));
#ifdef NUCMEM_DEBUG
				NTR_PRINTF (
					"NUCMEM_FREE: nwmpFreeTDS: free TDSListEntry_t 0x%x, size=0x%x",
					TDSPtr, sizeof(TDSListEntry_t), 0 );
#endif
				ccode = SUCCESS;
				break;
			}
		}
		TDSPtrToLast1 = TDSPtr;
    	TDSPtr = TDSPtr->TDSForwardPtr;
    } while( TDSPtr != NULL ); 

    return( NTR_LEAVE( ccode ) );

}
/* Get info concerning a Tagged Data Store */
int
nwmpGetTDS (comargs_t *comargs, struct TDSReq *uargs)
{
    int                             ccode=SUCCESS;
    TDSListEntry_t                  *TDSPtr;
	extern TDSListEntry_t			*TDSList;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

    /* see if any TDS exists */
    if( TDSList == NULL ) {
        /* No TDS allocated */
		/* This error is used because we map it into NWERR_INVALID_TAG
			error upstairs.
		*/
		ccode = SPI_CANT_OPEN_DIRECTORY;	
    	return( NTR_LEAVE( ccode ) );
    }

    /* see if our TDS exists */
	/* This error is used because we map it into NWERR_INVALID_TAG
		error upstairs.
	*/
	ccode = SPI_CANT_OPEN_DIRECTORY;	
    TDSPtr = TDSList;
    do {
        if( TDSPtr->uid == comargs->credential->userID ) {
            if( TDSPtr->tag == uargs->tag ) {
                /* we found the TDS we're looking for */
				uargs->maxSize = TDSPtr->maxSize;
				uargs->dataSize = TDSPtr->dataSize;
				uargs->flags = TDSPtr->flags;
				ccode = SUCCESS;
                break;
            }
        }
        TDSPtr = TDSPtr->TDSForwardPtr;
    } while( TDSPtr != NULL );

    return( NTR_LEAVE( ccode ) );

}

/* Read from a Tagged Data Store */
int
nwmpReadTDS (comargs_t *comargs, struct TDSRWReq *uargs)
{
    int                             ccode=SUCCESS;
    TDSListEntry_t                  *TDSPtr;
    extern TDSListEntry_t           *TDSList;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

    /* see if any TDS exists */
    if( TDSList == NULL ) {
        /* No TDS allocated */
		/* This error is used because we map it into NWERR_INVALID_TAG
			error upstairs.
		*/
		ccode = SPI_CANT_OPEN_DIRECTORY;	
    	return( NTR_LEAVE( ccode ) );
    }

    /* see if our TDS exists */
	/* This error is used because we map it into NWERR_INVALID_TAG
		error upstairs.
	*/
	ccode = SPI_CANT_OPEN_DIRECTORY;	
    TDSPtr = TDSList;
    do {
        if( TDSPtr->uid == comargs->credential->userID ) {
            if( TDSPtr->tag == uargs->tag ) {
                /* we found the TDS we're looking for */
				
				if( (uint16)(uargs->length + uargs->offset) > 
					(uint16) TDSPtr->maxSize ) {
					/* Too many bytes to read */
					if( uargs->offset >= TDSPtr->maxSize ) {
						/* Leave this error here for now */
						ccode = SPI_USER_MEMORY_FAULT;
						break;
					} else {
						/* Readjust size to read what's there */
						uargs->length = TDSPtr->maxSize - uargs->offset;
					}
				} 

				if(copyout((caddr_t)(TDSPtr->bufPtr + uargs->offset),
							   uargs->buffer,
							   uargs->length) ) {
					ccode = SPI_USER_MEMORY_FAULT;
				} else {
                	ccode = SUCCESS;
				}
                break;
            }
        }
        TDSPtr = TDSPtr->TDSForwardPtr;
    } while( TDSPtr != NULL );
 
    return( NTR_LEAVE( ccode ) );

}

/* Write to a Tagged Data Store */
int
nwmpWriteTDS (comargs_t *comargs, struct TDSRWReq *uargs)
{
    int                             ccode=SUCCESS;
    TDSListEntry_t                  *TDSPtr;
    extern TDSListEntry_t           *TDSList;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

    /* see if any TDS exists */
    if( TDSList == NULL ) {
        /* No TDS allocated */
		/* This error is used because we map it into NWERR_INVALID_TAG
			error upstairs.
		*/
		ccode = SPI_CANT_OPEN_DIRECTORY;	
    	return( NTR_LEAVE( ccode ) );
    }

    /* see if our TDS exists */
	/* This error is used because we map it into NWERR_INVALID_TAG
		error upstairs.
	*/
	ccode = SPI_CANT_OPEN_DIRECTORY;	
    TDSPtr = TDSList;
    do {
        if( TDSPtr->uid == comargs->credential->userID ) {
            if( TDSPtr->tag == uargs->tag ) {
                /* we found the TDS we're looking for */

                if( (uint16)(uargs->length + uargs->offset) > 
					(uint16)TDSPtr->maxSize ) {
                    /* Too many bytes to write but do what we can */
					/* This error is used because we map it into 
						NWERR_WRITE_TRUNCATED error upstairs.
					*/
					ccode = SPI_BAD_BYTE_RANGE;	
					if( TDSPtr->maxSize > uargs->offset ) {
						uargs->length = TDSPtr->maxSize - uargs->offset;
                    	(void)copyin( uargs->buffer, 
									  (caddr_t)(TDSPtr->bufPtr + uargs->offset),
									  uargs->length );
						TDSPtr->dataSize = TDSPtr->maxSize;
					} else {
						uargs->length = 0;
					}
                } else {
	                if(copyin( uargs->buffer,
								   (caddr_t)(TDSPtr->bufPtr + uargs->offset),
								   uargs->length) ) {
   	                 	ccode = SPI_USER_MEMORY_FAULT;
   	             	} else {
						if( TDSPtr->dataSize < uargs->offset + uargs->length )
							TDSPtr->dataSize = uargs->offset + uargs->length;
   	                 	ccode = SUCCESS;
   	             	}
				}
                break;
            }
        }
        TDSPtr = TDSPtr->TDSForwardPtr;
    } while( TDSPtr != NULL );

    return( NTR_LEAVE( ccode ) );

}


/* Write to a Distinguished Name Buffer */
int
nwmpWriteDN (comargs_t *comargs, struct gbufReq *uargs)
{
    int                             ccode=SUCCESS;
    DNListEntry_t 		           *DNPtr;
    extern DNListEntry_t            *DNList;
	pl_t							pl;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	pl = LOCK (nucLock, plstr);

    /* see if any DNBufs exists */
    if( DNList == NULL ) {
        /* No DNBufs allocated */
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
		UNLOCK (nucLock, pl);

    	return( NTR_LEAVE( ccode ) );
    }

    /* see if our DNBuf exists */
	/* This error is used because we map it into NWERR_NOT_INITIALIZED
		error upstairs.
	*/
	ccode = SPI_INACTIVE;	
    DNPtr = DNList;
    do {
        if( DNPtr->uid == comargs->credential->userID ) {
            /* we found the DN we're looking for */

            if( uargs->bufLength > MAX_DN_BYTES ) {
                /* Too many bytes to write
				 * This error is used because we map it into
				 * NWERR_WRITE_TRUNCATED error upstairs.
				 */
				ccode = SPI_BAD_BYTE_RANGE;	
                break;
            }

            if(copyin( (caddr_t)uargs->buffer,
						   (caddr_t)DNPtr->bufPtr,
						   uargs->bufLength) ) {
                ccode = SPI_USER_MEMORY_FAULT;
            } else {
				DNPtr->bufLength = uargs->bufLength;
                ccode = SUCCESS;
            }
            break;
        }
        DNPtr = DNPtr->DNForwardPtr;
    } while( DNPtr != NULL );

	UNLOCK (nucLock, pl);

    return( NTR_LEAVE( ccode ) );

}


/* Read from a Distinguished Name Buffer */
int
nwmpReadDN (comargs_t *comargs, struct gbufReq *uargs)
{
    int                            ccode=SUCCESS;
    DNListEntry_t                  *DNPtr;
    extern DNListEntry_t           *DNList;
	pl_t							pl;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	pl = LOCK (nucLock, plstr);

    /* see if any DNBuf exists */
    if( DNList == NULL ) {
        /* No DNBuf allocated */
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
		UNLOCK (nucLock, pl);

    	return( NTR_LEAVE( ccode ) );
    }

    /* see if our DNBuf exists */
	/* This error is used because we map it into NWERR_NOT_INITIALIZED
		error upstairs.
	*/
	ccode = SPI_INACTIVE;	
    DNPtr = DNList;
    do {
        if( DNPtr->uid == comargs->credential->userID ) {
            /* we found the DN we're looking for */

            if( uargs->bufLength > MAX_DN_BYTES ) {
                /* Too many bytes to read so readjust size to read 
					what's there */
                uargs->bufLength = MAX_DN_BYTES;
            }

            if(copyout((caddr_t)DNPtr->bufPtr,
						   (caddr_t)uargs->buffer,
						   uargs->bufLength) ) {
                ccode = SPI_USER_MEMORY_FAULT;
            } else {
                ccode = SUCCESS;
            }
            break;
        }
        DNPtr = DNPtr->DNForwardPtr;
    } while( DNPtr != NULL );

	UNLOCK (nucLock, pl);

    return( NTR_LEAVE( ccode ) );

}

/* Read the preferred tree name */
int
nwmpGetPreferredTree (comargs_t *comargs, struct gbufReq *uargs)
{
    int                     ccode=SUCCESS;
	extern initListEntry_t	*initList;
	initListEntry_t			*initPtr;
	pl_t					pl;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	pl = LOCK (nucLock, plstr);

	/* find init entry to get preferred tree name */
	if( initList != 0 ) {
		/* See if list entry exists for this uid */
		initPtr = initList;
		do {
			if( initPtr->uid == comargs->credential->userID ) {
				if( initPtr->prefTreeNameLen > uargs->bufLength ) {
					/* informational */
					/* ccode = 0x02; */	/* ??? What do we tell the user? */
				} else {
					uargs->bufLength = initPtr->prefTreeNameLen;
				}
				if(copyout((caddr_t)initPtr->preferredTree,
							   (caddr_t)uargs->buffer,
							   uargs->bufLength )) {
                	ccode = SPI_USER_MEMORY_FAULT;
            	} 

				UNLOCK (nucLock, pl);

    			return( NTR_LEAVE( ccode ) );
			}
			initPtr = initPtr->initForwardPtr;
		} while( initPtr != 0 );
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
		uargs->bufLength = 0;
	} else {
		/* No ILE for this uid */
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
		uargs->bufLength = 0;
	}
	
	UNLOCK (nucLock, pl);

    return( NTR_LEAVE( ccode ) );

}

/* Set the preferred tree name */
int
nwmpSetPreferredTree (comargs_t *comargs, struct gbufReq *uargs)
{
	int						i;
    int                     ccode = SUCCESS;
	extern initListEntry_t	*initList;
	initListEntry_t			*initPtr;
	pl_t					pl;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	pl = LOCK (nucLock, plstr);

	/* find init entry to get preferred tree name */
	if( initList != 0 ) {
		/* See if list entry exists for this uid */
		initPtr = initList;
		do {
			if( initPtr->uid == comargs->credential->userID ) {
#ifdef CHECK_FOR_2_1
				if( MAX_DS_TREE_NAME_LEN < uargs->bufLength ) {
#else
				if( initPtr->prefTreeNameLen < uargs->bufLength ) {
#endif
					/* ??? What do we tell the user */
					/* This error is used because we map it into 
						NWERR_INVALID_PARAMETERS error upstairs.
					*/
					UNLOCK (nucLock, pl);
					ccode = SPI_GENERAL_FAILURE;	

    				return( NTR_LEAVE( ccode ) );
				} 
				if(copyin((caddr_t)uargs->buffer,
							  (caddr_t)initPtr->preferredTree,
							  uargs->bufLength )) {
					UNLOCK (nucLock, pl);
                	ccode = SPI_USER_MEMORY_FAULT;

    				return( NTR_LEAVE( ccode ) );
            	} 
#ifdef CHECK_FOR_2_1
				initPtr->prefTreeNameLen = uargs->bufLength;
#else
				for (i = uargs->bufLength - 1; i < MAX_DS_TREE_NAME_LEN; i++ )
					initPtr->preferredTree[i] = '_';
#endif

				UNLOCK (nucLock, pl);

    			return( NTR_LEAVE( ccode ) );
			}
			initPtr = initPtr->initForwardPtr;
		} while( initPtr != 0 );
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
	} else {
		/* No ILE for this uid */
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
	}
	
	UNLOCK (nucLock, pl);

    return( NTR_LEAVE( ccode ) );

}

/* Read the NLS Path */
int
nwmpReadNLSPath (comargs_t *comargs, struct gbufReq *uargs)
{
    int                     ccode=SUCCESS;
    extern initListEntry_t  *initList;
    initListEntry_t         *initPtr;
	pl_t					pl;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	pl = LOCK (nucLock, plstr);

    /* find init entry to get preferred server name */
    if( initList != 0 ) {
        /* See if list entry exists for this uid */
        initPtr = initList;
        do {
            if( initPtr->uid == comargs->credential->userID ) {
                if( initPtr->NLSPathLength > uargs->bufLength ) {
                    /* informational */
                    /* ccode = 0x02; */   /* ??? What do we tell the user? */
                } else {
                    uargs->bufLength = initPtr->NLSPathLength;
                }
                if(copyout((caddr_t)initPtr->NLSPath,
							   (caddr_t)uargs->buffer,
							   uargs->bufLength )) {
                    ccode = SPI_USER_MEMORY_FAULT;
                }

				UNLOCK (nucLock, pl);

    			return( NTR_LEAVE( ccode ) );
            }
            initPtr = initPtr->initForwardPtr;
        } while( initPtr != 0 );
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
        uargs->bufLength = 0;
    } else {
        /* No ILE for this uid */
		/* This error is used because we map it into NWERR_NOT_INITIALIZED
			error upstairs.
		*/
		ccode = SPI_INACTIVE;	
        uargs->bufLength = 0;
    }

	UNLOCK (nucLock, pl);

    return( NTR_LEAVE( ccode ) );

}

/*	Initialize certain requester values for a given uid 
 *	This initialization routine can be called as many times as
 *	required.
*/
int
nwmpInitRequester (comargs_t *comargs, struct initReq *uargs)
{
	int								i;
    int                             ccode = SUCCESS;
    initListEntry_t            		*initPtr;
    initListEntry_t            		*lastInitPtr=0;
    extern initListEntry_t          *initList;
	extern uint32					defaultChecksumLevel;
	extern uint32					checksumRequiredLevel;
	extern uint32					defaultSecurityLevel;
	extern uint32					signatureRequiredLevel;
    DNListEntry_t              		*DNPtr;
	pl_t					pl;

	NTR_ENTER(2, comargs, uargs, 0, 0, 0);

	pl = LOCK (nucLock, plstr);

	/*	Check for an initList entry for this uid. If it exists, 
	 *	overwrite it. If it doesn't exist, create one & 
	 *	initialize it before placing it on the chain. If any parameters
	 *	aren't set, we'll use defaults if the ILE doesn't exist or
	 *	just leave what's there if they do exist.
	 */

	/* See if init entry exists for this uid. */
	lastInitPtr = NULL;
	if( initList != NULL ) {
    	initPtr = initList;
    	do {
        	if( initPtr->uid == comargs->credential->userID ) {
				/* We've found an existing ILE for this uid. */
				UNLOCK (nucLock, pl);
           		ccode = changeExistingILEValues( uargs, initPtr, comargs );
    			return( NTR_LEAVE( ccode ) );
        	}
        	lastInitPtr = initPtr;
       	 	initPtr = initPtr->initForwardPtr;
    	} while( initPtr != NULL );
	}

	UNLOCK (nucLock, pl);

	/* init entry doesn't exist, so allocate it and associated items. */
	initPtr = (initListEntry_t *)kmem_zalloc ( sizeof(initListEntry_t), 
		KM_NOSLEEP );
    if (initPtr == NULL) {
        ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
    	return( NTR_LEAVE( ccode ) );
    }

	pl = LOCK (nucLock, plstr);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF (
		"NUCMEM: nwmpInitRequester: alloc initListEntry_t 0x%x, size=0x%x",
		initPtr, sizeof(initListEntry_t), 0 );
#endif

	/* ID this ILE with user's uid */	
	initPtr->uid = comargs->credential->userID;

    /* Now let's set the user's security preferences */
    /* At some point we're supposed to support header signing,
        complete signatures, encryption, and ipx checksums. At this
        time we're only supporting header signatures.
    */

	if( signatureRequiredLevel == 0xFF ) {
		if( (uargs->securityLevel & 0xFF) == 0xFF ) {
    		initPtr->securityFlags = defaultSecurityLevel & 
				SET_USER_PREFERENCES_BITS;
		} else {
    		initPtr->securityFlags = (( defaultSecurityLevel > 
				(uargs->securityLevel & SET_USER_PREFERENCES_BITS )) ? 
				defaultSecurityLevel : 
				(uargs->securityLevel & SET_USER_PREFERENCES_BITS ));
		}
	} else {
		if( (uargs->securityLevel & 0xFF) == 0xFF ) {
    		initPtr->securityFlags = signatureRequiredLevel;
		} else {
    		initPtr->securityFlags = (( signatureRequiredLevel > 
				(uargs->securityLevel & SET_USER_PREFERENCES_BITS )) ? 
				signatureRequiredLevel : 
				(uargs->securityLevel & SET_USER_PREFERENCES_BITS ));
		}
	}


	if( checksumRequiredLevel == 0xFF ) {
        if( (uargs->checksumLevel & 0xFF) == 0xFF ) {
            initPtr->securityFlags |= (( defaultChecksumLevel &
                SET_USER_PREFERENCES_BITS) << 2);
        } else {
    		initPtr->securityFlags |= ((( defaultChecksumLevel > 
				(uargs->checksumLevel & SET_USER_PREFERENCES_BITS )) ? 
				defaultChecksumLevel : 
				(uargs->checksumLevel & SET_USER_PREFERENCES_BITS )) << 2);
        }
    } else {
        if( (uargs->checksumLevel & 0xFF) == 0xFF ) {
            initPtr->securityFlags |= (checksumRequiredLevel << 2);
        } else {
    		initPtr->securityFlags |= ((( checksumRequiredLevel > 
				(uargs->checksumLevel & SET_USER_PREFERENCES_BITS )) ? 
				checksumRequiredLevel : 
				(uargs->checksumLevel & SET_USER_PREFERENCES_BITS )) << 2);
        }
    }

	UNLOCK (nucLock, pl);

#ifdef NWSN_DEBUG
	NWMP_CMN_ERR (CE_CONT,
		"DEBUG: nwmpInitRequester(): initPtr->securityFlags = 0x%x\n", 
		initPtr->securityFlags );
#endif


#ifdef SUPPORT_2_1
	/* Max packet size to negotiate. If both end point support a larger
		packet size than the intermediate routers we're toast. We can't
		support this item being variable until we support LIP.
	*/

	/* Max NCP retries and Max NCP timeout may go here in 2.1 */
#endif /* SUPPORT_2_1 */

	/* See if we need to allocate memory for preferred tree name */
	/* allocate some memory */
	initPtr->preferredTree = (uint8 *)kmem_alloc (MAX_DS_TREE_NAME_LEN,
    	KM_SLEEP );
#ifdef NUCMEM_DEBUG
		NTR_PRINTF (
			"NUCMEM_FREE:nwmpInitRequester:free initListEntry_t 0x%x, size=0x%x",
			initPtr, sizeof(initListEntry_t), 0 );
#endif
		if( uargs->prefTreeNameLen > 0 ) { 
			if( uargs->prefTreeNameLen <= MAX_DS_TREE_NAME_LEN ) {
				if (copyin((caddr_t)uargs->preferredTree,
							   (caddr_t)initPtr->preferredTree,
							   uargs->prefTreeNameLen)) {
					ccode = SPI_USER_MEMORY_FAULT;
           			kmem_free ( initPtr->preferredTree, MAX_DS_TREE_NAME_LEN);
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: nwmpInitRequester: free preferredTree * at 0x%x, size = 0x%x",
                initPtr->preferredTree, MAX_DS_TREE_NAME_LEN, 0 );
#endif
					kmem_free ( initPtr, sizeof(initListEntry_t));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: nwmpInitRequester: free initListEntry_t * at 0x%x, size = 0x%x",
                initPtr, sizeof(initListEntry_t), 0 );
#endif
           			ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
    				return( NTR_LEAVE( ccode ) );
				} else {
					pl = LOCK (nucLock, plstr);

					if( uargs->prefTreeNameLen > 0 ) {
						for( i=uargs->prefTreeNameLen; i<MAX_DS_TREE_NAME_LEN;
								i++ ) {
							initPtr->preferredTree[i] = '_';
						}
						/* set length field */
						initPtr->prefTreeNameLen = MAX_DS_TREE_NAME_LEN;
					} else {
						initPtr->prefTreeNameLen = 0;
					}

					UNLOCK (nucLock, pl);
				}
			} else {
				/* Name is too large */
				/* This error is used because we map it into 
					NWERR_INVALID_PARAMETERS error upstairs.
					Set uargs length for dianostic return.
				*/
				ccode = SPI_GENERAL_FAILURE;	
				initPtr->prefTreeNameLen = 0;
				uargs->prefTreeNameLen = 0;
			}
		} else {
			/* No preferred tree set */
			pl = LOCK (nucLock, plstr);
			initPtr->prefTreeNameLen = 0;
			UNLOCK (nucLock, pl);
		}



	/* NLS Path -- DS, It is currently unclear whether we will be keeping
	   this information in the kernel. We'll leave this code here until
	   the decision has been made. */

    /* allocate some memory */
    initPtr->NLSPath = (uint8 *)kmem_zalloc ( MAXPATHLEN, KM_SLEEP );
#ifdef NUCMEM_DEBUG
			NTR_PRINTF (
				"NUCMEM_FREE: nwmpInitRequester: free preferredTree 0x%x, size=0x%x",
				initPtr->preferredTree, MAX_DS_TREE_NAME_LEN, 0 );
#endif
    	if( uargs->NLSPathLen > 0 ) {
			if( uargs->NLSPathLen <= MAXPATHLEN ) {
           		if (copyin((caddr_t)uargs->NLSPath, (caddr_t)initPtr->NLSPath,
							   uargs->NLSPathLen)) {
               		ccode = SPI_USER_MEMORY_FAULT;
               		if (initPtr->preferredTree)
           				kmem_free (initPtr->preferredTree,MAX_DS_TREE_NAME_LEN);
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: nwmpInitRequester: free preferredTree * at 0x%x, size = 0x%x",
                initPtr->preferredTree, MAX_DS_TREE_NAME_LEN, 0 );
#endif
            		kmem_free ( initPtr->NLSPath, MAXPATHLEN);
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: nwmpInitRequester: free NLSPath * at 0x%x, size = 0x%x",
                initPtr->NLSPath, MAXPATHLEN, 0 );
#endif
					kmem_free ( initPtr, sizeof(initListEntry_t));
#ifdef NUCMEM_DEBUG
					NTR_PRINTF (
						"NUCMEM_FREE: nwmpInitRequester: free initListEntry_t 0x%x, size=0x%x",
						initPtr, sizeof(initListEntry_t), 0 );
#endif
               		ccode = SPI_CLIENT_RESOURCE_SHORTAGE;
    				return( NTR_LEAVE( ccode ) );
				}
            } else {
        		/* User has a path length that's too large */
				/* This error is used because we map it into 
					NWERR_INVALID_PARAMETERS error upstairs.
				*/
				ccode = SPI_GENERAL_FAILURE;	
				uargs->NLSPathLen = 0;
			}
            /* set length field */
            initPtr->NLSPathLength = uargs->NLSPathLen;
        }

	/* Distinguished Name Buffer AKA - default context */
	/* This little guy sits on a list of his own.  */

	if( (DNPtr = getDNBPtr( comargs->credential->userID )) != NULL ) {
       	if( uargs->DNBufferLength > MAX_DN_BYTES ) {
        	/* User has a length that's too large */
			/* This error is used because we map it into 
				NWERR_INVALID_PARAMETERS error upstairs.
			*/
			ccode = SPI_GENERAL_FAILURE;	
           	uargs->DNBufferLength = 0;
       	}
		if(copyin( (caddr_t)uargs->DNBuffer, (caddr_t)DNPtr->bufPtr, 
			uargs->DNBufferLength)) {
			ccode = SPI_USER_MEMORY_FAULT;
		}
		DNPtr->bufLength = uargs->DNBufferLength;
	} else {
		/* Couldn't get a DNB */
		/* This error is used because we map it into 
			NWERR_INVALID_PARAMETERS error upstairs.
		*/
		ccode = SPI_GENERAL_FAILURE;	
	}



	pl = LOCK (nucLock, plstr);
	/* Add init entry to master list */
	if( lastInitPtr == NULL ) {
		initList = initPtr;
	} else {
		lastInitPtr->initForwardPtr = initPtr;
	}
	UNLOCK (nucLock, pl);

    return( NTR_LEAVE( ccode ) );

}

/*	This routine changes ILE values if appropriate.
*/
int
changeExistingILEValues (struct initReq  *uargs, initListEntry_t *initPtr,
	comargs_t *comargs)
{
    int                             ccode=SUCCESS;
	int								i;
	DNListEntry_t                   *DNPtr;
	extern uint32					defaultChecksumLevel;
	extern uint32					checksumRequiredLevel;
	extern uint32					defaultSecurityLevel;
	extern uint32					signatureRequiredLevel;
	pl_t							pl;

	pl = LOCK (nucLock, plstr);

	/*	Check security preferences. This one is different since 0 is 
	 *	a valid value, we have to check for FF indicating that no
	 *	preference was indicated by the user.
	 */
    if( (uargs->securityLevel & 0xFF) != 0xFF ) {
        initPtr->securityFlags &= ~SET_USER_PREFERENCES_BITS;
        if( signatureRequiredLevel == 0xFF ) {
            initPtr->securityFlags |= (( defaultSecurityLevel >
                (uargs->securityLevel & SET_USER_PREFERENCES_BITS )) ?
                defaultSecurityLevel :
                (uargs->securityLevel & SET_USER_PREFERENCES_BITS ));
        } else {
        	initPtr->securityFlags &= ~SET_USER_PREFERENCES_BITS;
            initPtr->securityFlags |= (( signatureRequiredLevel >
                (uargs->securityLevel & SET_USER_PREFERENCES_BITS )) ?
                signatureRequiredLevel :
                (uargs->securityLevel & SET_USER_PREFERENCES_BITS ));
        }
    }

	/*	Check checksum preferences. This one is different since 0 is 
	 *	a valid value, we have to check for FF indicating that no
	 *	preference was indicated by the user.
	 */
	if( (uargs->checksumLevel & 0xFF) != 0xFF ) {
		initPtr->securityFlags &= ~(SET_USER_PREFERENCES_BITS << 2);
		if( checksumRequiredLevel == 0xFF ) {
    		initPtr->securityFlags |= ((( defaultChecksumLevel > 
				(uargs->checksumLevel & SET_USER_PREFERENCES_BITS )) ? 
				defaultChecksumLevel : 
				(uargs->checksumLevel & SET_USER_PREFERENCES_BITS )) << 2);
		} else {
			initPtr->securityFlags &= ~(SET_USER_PREFERENCES_BITS << 2);
    		initPtr->securityFlags |= ((( checksumRequiredLevel > 
				(uargs->checksumLevel & SET_USER_PREFERENCES_BITS )) ? 
				checksumRequiredLevel : 
				(uargs->checksumLevel & SET_USER_PREFERENCES_BITS )) << 2);
		}
	}

	UNLOCK (nucLock, pl);

#ifdef NWSN_DEBUG
	NWMP_CMN_ERR (CE_CONT,
		"DEBUG: changeExistingILEValues(): initPtr->securityFlags = 0x%x\n",
		initPtr->securityFlags );
#endif

#ifdef SUPPORT_2_1
	/* max packet size to negotiate */
#endif /* SUPPORT_2_1 */


	/* preferred tree name */
	if( uargs->prefTreeNameLen != 0 ) {
        if( uargs->prefTreeNameLen > MAX_DS_TREE_NAME_LEN ) {
        	/* User has a length that's too large */
			/* This error is used because we map it into 
				NWERR_INVALID_PARAMETERS error upstairs.
			*/
			ccode = SPI_GENERAL_FAILURE;	
            uargs->prefTreeNameLen = 0;
        } else {
			if (copyin((caddr_t)uargs->preferredTree,
						   (caddr_t)initPtr->preferredTree,
						   uargs->prefTreeNameLen)) {
				ccode = SPI_USER_MEMORY_FAULT;
			}

			pl = LOCK (nucLock, plstr);

			for( i=uargs->prefTreeNameLen; i<MAX_DS_TREE_NAME_LEN; i++ ) {
				initPtr->preferredTree[i] = '_';
			}
			/* set length field */
			initPtr->prefTreeNameLen = MAX_DS_TREE_NAME_LEN;

			UNLOCK (nucLock, pl);
		}
	}

	/* NLS Path */
	if( uargs->NLSPathLen != 0 ) {
        if( uargs->NLSPathLen > MAXPATHLEN ) {
        	/* User has a length that's too large */
			/* This error is used because we map it into 
				NWERR_INVALID_PARAMETERS error upstairs.
			*/
			ccode = SPI_GENERAL_FAILURE;	
            uargs->NLSPathLen = 0;
        } else {
			if(copyin((caddr_t)uargs->NLSPath,
						  (caddr_t)initPtr->NLSPath, uargs->NLSPathLen)) {
				ccode = SPI_USER_MEMORY_FAULT;
			}

			pl = LOCK (nucLock, plstr);
			initPtr->NLSPathLength = uargs->NLSPathLen;
			UNLOCK (nucLock, pl);
		}
	}

	/* DNBuf - Default Context  */
	if( uargs->DNBufferLength != 0 ) {
		/* get existing DNB or allocate one */
		if( (DNPtr = getDNBPtr( comargs->credential->userID )) != NULL ) {
        	if( uargs->DNBufferLength > MAX_DN_BYTES ) {
        		/* User has a length that's too large */
				/* This error is used because we map it into 
					NWERR_INVALID_PARAMETERS error upstairs.
				*/
				ccode = SPI_GENERAL_FAILURE;	
            	uargs->DNBufferLength = 0;
        	} else {
				if(copyin( (caddr_t)uargs->DNBuffer,
							   (caddr_t)DNPtr->bufPtr, 
					uargs->DNBufferLength)) {
					ccode = SPI_USER_MEMORY_FAULT;
				}

				pl = LOCK (nucLock, plstr);
				DNPtr->bufLength = uargs->DNBufferLength;
				UNLOCK (nucLock, pl);
			}
		} else {
			ccode = SPI_USER_MEMORY_FAULT;
		}
	}

	return( ccode );
}

DNListEntry_t *
getDNBPtr( uint32 userID )
{

	DNListEntry_t                   *DNPtr;
    DNListEntry_t                   *lastDNPtr=0;
	int                             DNF=FALSE;
    extern DNListEntry_t            *DNList;
	pl_t							pl;

	pl = LOCK (nucLock, plstr);

    /* See if DNBuf exists for this uid. */
    if( DNList != NULL ) {
        DNPtr = DNList;
        do {
            if( DNPtr->uid == userID ) {
                /* We have pre-existing DNBuf for this uid. Ignore it
                    and continue. */
                DNF = TRUE;
            }
            lastDNPtr = DNPtr;
            DNPtr = DNPtr->DNForwardPtr;
        } while( DNPtr != NULL );
    }

	UNLOCK (nucLock, pl);

    if( DNF == FALSE ) {

        /* No DNBuf, so create it and add it to the DNList */
        DNPtr = (DNListEntry_t *)kmem_zalloc ( sizeof(DNListEntry_t), 
			KM_SLEEP);
    } else {
		DNPtr = lastDNPtr;
	}

	return( DNPtr );

}

int
nwmpGetPrimaryService (comargs_t *comargs, uint32 *reference)
{
	NTR_ENTER(2, comargs, reference, 0, 0, 0);

	return(NTR_LEAVE(NWsiGetPrimaryService(comargs, reference)));
}

