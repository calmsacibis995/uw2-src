/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/silserver.c	1.17"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/silserver.c,v 2.56.2.2 1994/12/21 02:49:11 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE: silserver.c
 *	ABSTRACT: Server object manipulation routines for both external
 *			  and internal use.
 *
 *	Functions declared in this module:
 *	Public functions:
 *		NCPsilInitService
 *		NCPsilAllocServer
 *		NCPsilFreeServer
 *		NCPsilSetServerVersion
 *		NCPsilSetServerTransportMask
 *		NCPsilGetServerTransportMask
 *		NCPsilGetServerAddress
 *		NCPsilSetServerAddress
 *		NCPsilGetServerBlockSize
 *		NCPsilSetServerBlockSize
 *	Private functions:
 *	DEBUG functions:
 *		ValidateServerTag
 */ 

#ifdef _KERNEL_HEADERS
#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <util/cmn_err.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/nwctypes.h>	/* formerly included by sistructs.h */
#include <net/nuc/ncpconst.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/nucmachine.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/sistructs.h> 
#include <net/nuc/nucerror.h>
#include <util/debug.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>
#else /* ndef _KERNEL_HEADERS */
#include <sys/tiuser.h>
#include <kdrivers.h>
#include <sys/nuctool.h>
#include <sys/nwctypes.h>	/* formerly included by sistructs.h */
#include <sys/ncpconst.h>	/* formerly included by ncpinclude.h */
#include <sys/nucmachine.h>	/* formerly included by ncpinclude.h */
#include <sys/slstruct.h>
#include <sys/nwportable.h>
#include <sys/nwctypes.h>
#include <sys/spilcommon.h>	/* formerly included by ncpinclude.h */
#include <sys/sistructs.h> 
#include <sys/nucerror.h>
#include <sys/debug.h>

#endif /* ndef _KERNEL_HEADERS */

/*
 *	Forward declarations
 */
#ifdef NUC_DEBUG
ccode_t NCPsilValidateServerTag();
#endif

#define NTR_ModMask NTRM_ncp

/*
 * BEGIN_MANUAL_ENTRY(NCPsilAllocServer.3k)
 * NAME
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilAllocServer( serverName, serverHandle )
 *		char			*serverName;
 *		ncp_server_t	**serverHandle;
 *
 * INPUT
 *		char			*serverName;
 *
 * OUTPUT
 *		ncp_server_t	**serverHandle;
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Allocate an NCP service Structure
 *
 * SEE ALSO
 *		NCPsilFreeServer(3k)
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsilAllocServer (ncp_server_t **serverHandle)
{
	ccode_t	ccode = SUCCESS;

	/*
	 *	Get the handle that's already initialized
	 */
	*serverHandle = 
		(ncp_server_t *)kmem_zalloc (sizeof(ncp_server_t), KM_SLEEP);

#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NCPsilAllocServer: alloc ncp_server_t * at 0x%x, size = 0x%x",
                *serverHandle, sizeof(ncp_server_t), 0 );
#endif NUCMEM_DEBUG


#if ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	(*serverHandle)->tag[0] = 'S';
	(*serverHandle)->tag[1] = 'H';
#endif ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))

	return(ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsilFreeServer.3k)
 * NAME
 *		NCPsilFreeServer - Free server structure
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilFreeServer( serverHandle )
 *		ncp_server_t	*serverHandle;
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *		Free up the service protocol specific information on the
 *		server specified
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsilFreeServer (ncp_server_t *serverHandle)
{
#ifdef NUC_DEBUG
	NCPsilValidateServerTag( serverHandle );
#endif
	if (serverHandle->address) {
		kmem_free ( (char *)serverHandle->address->buf, MAX_ADDRESS_SIZE);
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NCPsilFreeServer: free netbuf buffer * at 0x%x, size = 0x%x",
                serverHandle->address->buf, MAX_ADDRESS_SIZE, 0 );
#endif
		kmem_free ( (char *)serverHandle->address, sizeof (struct netbuf));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NCPsilFreeServer: free netbuf * at 0x%x, size = 0x%x",
                serverHandle->address, sizeof(struct netbuf), 0 );
#endif
	}
	kmem_free ( (char *)serverHandle, sizeof(ncp_server_t));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NCPsilFreeServer: free ncp_server_t * at 0x%x, size = 0x%x",
                serverHandle, sizeof(ncp_server_t), 0 );
#endif
	return(SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsilSetServerVersion.4k)
 * NAME
 *		NCPsilSetServerVersion -	Set the server's version index
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilSetServerVersion( serverHandle, majorVersion, minorVersion,
 *								nameSpaceSupported )
 *		ncp_server_t	*serverHandle;
 *		uint8			majorVersion;
 *		uint8			minorVersion;
 *
 * INPUT
 *		ncp_server_t	*serverHandle;
 *		uint8			majorVersion;
 *		uint8			minorVersion;
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		Determines the jump table index based upon the major
 *		minor numbers the file server returns.
 *
 * SEE ALSO
 *		NCPsilGetServerVersion(3k)
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsilSetServerVersion (ncp_server_t *serverHandle,
						uint8 majorVersion, uint8 minorVersion)
{
	ccode_t	ccode = SUCCESS;
#ifdef NUC_DEBUG
	NCPsilValidateServerTag( serverHandle );
#endif

	serverHandle->majorVersion = majorVersion;
	serverHandle->minorVersion = minorVersion;

	return(ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsilSetServerAddress.3k)
 * NAME
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilSetServerAddress( serverHandle, address, addressLength )
 *		ncp_server_t	*serverHandle;
 *		uint8			*address;
 *		int32			addressLength;
 *
 * INPUT
 *
 * OUTPUT
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * SEE ALSO
 *
 * NOTES
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsilSetServerAddress (ncp_server_t *serverHandle, struct netbuf *address)
{

#ifdef NUC_DEBUG
	NCPsilValidateServerTag( serverHandle );
#endif
	if (address == NULL)
		return(FAILURE);

	serverHandle->address = (struct netbuf *)kmem_alloc (sizeof(struct netbuf),
								KM_SLEEP);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NCPsilSetServerAddress: alloc netbuf * at 0x%x, size = 0x%x",
                serverHandle->address, sizeof(struct netbuf), 0 );
#endif NUCMEM_DEBUG

	serverHandle->address->buf = (char *)kmem_alloc (MAX_ADDRESS_SIZE, 
		KM_SLEEP);
#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NCPsilSetServerAddress: alloc netbuf buf * at 0x%x, size = 0x%x",
                serverHandle->address->buf, MAX_ADDRESS_SIZE, 0 );
#endif NUCMEM_DEBUG

	bcopy(address->buf, serverHandle->address->buf, address->len);
	serverHandle->address->len = address->len;
	serverHandle->address->maxlen = address->maxlen;
	return (SUCCESS);
}


#ifdef NUC_DEBUG
ccode_t
NCPsilValidateServerTag (ncp_server_t *serverHandle)
{
	NTR_ASSERT( (serverHandle->tag[0] == 'S') &&
			(serverHandle->tag[1] == 'H') );
}
#endif
