/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/silauth.c	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/silauth.c,v 2.51.2.4 1995/02/12 23:37:56 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: ncpauth.c
 *	ABSTRACT: module for managing the authentication structure
 *
 */ 

#ifdef _KERNEL_HEADERS
#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <net/nw/ntr.h>
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

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask NTRM_ncp

/*
 * BEGIN_MANUAL_ENTRY(NCPsilAllocAuth.3k)
 * NAME
 *		NCPsilAllocAuth
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilAllocAuth( taskPtr, userName )
 *		ncp_auth_t	**authPtr;
 *
 * INPUT
 *		ncp_auth_t	**authPtr;
 *
 * OUTPUT
 *		ncp_auth_t	*authPtr;
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		This function is called to allocate an authorization structure for 
 *		NetWare-authenticated tasks.
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsilFreeAuth
 *
 * END_MANUAL_ENTRY
 */
int
NCPsilAllocAuth( ncp_auth_t	**authPtr )
{

	*authPtr = (ncp_auth_t *)kmem_zalloc ( sizeof(ncp_auth_t), KM_SLEEP );
#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NCPsilAllocAuth: alloc ncp_auth_t * at 0x%x, size = 0x%x",
                *authPtr, sizeof(ncp_auth_t), 0 );
#endif NUCMEM_DEBUG
	return(SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsilFreeAuth.3k)
 * NAME
 *		NCPsilFreeAuth
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilFreeAuth( taskPtr, userName )
 *		ncp_auth_t	*authPtr;
 *
 * INPUT
 *		ncp_auth_t	*authPtr;
 *
 * OUTPUT
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *
 * DESCRIPTION
 *		This function is called to free a previously allocated 
 *		authorization structure.
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsilAllocAuth
 *
 * END_MANUAL_ENTRY
 */
int
NCPsilFreeAuth( ncp_auth_t	*authPtr )
{

	kmem_free ( authPtr, sizeof(ncp_auth_t));
#ifdef NUCMEM_DEBUG
        NTR_PRINTF("NUCMEM_FREE: NCPsilFreeAuth: free ncp_auth_t * at 0x%x, size = 0x%x",
                authPtr, sizeof(ncp_auth_t), 0 );
#endif
	return(SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsilGetUserID.3k)
 * NAME
 *		NCPsilGetUserID
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilGetUserID( taskPtr, userName )
 *		ncp_task_t	*taskPtr;
 *		char		**userName;
 *
 * INPUT
 *		ncp_task_t	*taskPtr;
 *
 * OUTPUT
 *		char		**userName;
 *
 * RETURN VALUES
 *		SUCCESS
 *
 * DESCRIPTION
 *		This function is called by the SPIL layer to return a pointer to the 
 *		NetWare userid used to authenticate this task.  It is done at this
 *		layer because the authentication structure is an SPROTO structure 
 *		pointed to by the SPROTO task structure pointed to by the SPIL-layer
 *		task structure. 
 *
 * NOTES
 *
 * SEE ALSO
 *		NCPsilSetUserID
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsilGetUserID( ncp_task_t	*taskPtr, char		**userName )
{
	ncp_auth_t	*authInfoPtr;

	authInfoPtr = taskPtr->authInfoPtr;
	*userName = authInfoPtr->userID;
	return( SUCCESS );
}


