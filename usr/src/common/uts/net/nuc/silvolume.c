/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/silvolume.c	1.14"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/silvolume.c,v 2.55.2.2 1994/12/21 02:49:22 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE: silvolume.c
 *	ABSTRACT: NCP Server structure volume list manipulation structure 
 *
 *	Functions declared in this module:
 *	Public functions:
 *		NCPsilAllocVolume
 *		NCPsilFreeVolume
 *		NCPsilSetVolumeNameSpaceOps
 *		NCPsilGetVolumeNameSpaceOps
 *		NCPsilSetVolumeNumber
 *		NCPsilGetVolumeNumber
 *	
 *	Private functions:
 *	NUC_DEBUG functions:
 *		ValidateVolume
 */ 
#include <net/tiuser.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>	/* formerly included by sistructs.h */
#include <util/cmn_err.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/ncpconst.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/nucmachine.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>	/* formerly included by ncpinclude.h */
#include <net/nuc/sistructs.h> 
#include <net/nuc/nucerror.h>
#include <util/debug.h>
#include <net/nuc/ncpiopack.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

#define NTR_ModMask NTRM_ncp

/*
 *	Forward references
 */
#ifdef NUC_DEBUG
ccode_t NCPsilValidateVolume();
#endif


/*
 * BEGIN_MANUAL_ENTRY(NCPsilAllocVolume.3k)
 * NAME
 *		NCPsilAllocVolume - Allocate a new NCP volume structure
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilAllocVolume( volumeHandle )
 *		void_t	**volumeHandle;
 *
 * INPUT
 *		volumeHandle double pointer
 *
 * OUTPUT
 *		pointer to structure allocated from memory pool.
 *
 * RETURN VALUES
 *		SUCCESS
 *		FAILURE
 *		SPI_MEMORY_EXHAUSTED
 *
 * DESCRIPTION
 *		Allocates a volume handle structure from the NCP memory
 *		pool and returns the pointer to the caller.
 *
 * SEE ALSO
 *		NCPsilFreeVolume(3k)
 *
 * NOTES
 *		Assumes the pointer is valid.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsilAllocVolume (	ncp_volume_t	**volumeHandle )
{
	ccode_t	ccode = SUCCESS;

	*volumeHandle = 
		(void_t *)kmem_zalloc (sizeof(ncp_volume_t), KM_SLEEP);


#if ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	{
		(*volumeHandle)->tag[0] = 'V';
		(*volumeHandle)->tag[1] = 'H';
	}
#endif ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))

	return (ccode);
}

/*
 * BEGIN_MANUAL_ENTRY(NCPsilFreeVolume.3k)
 * NAME
 *		NCPsilFreeVolume - Free a previously allocated volume structure
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilFreeVolume( volumeHandle )
 *		void_t	*volumeHandle;
 *
 * INPUT
 *		volumeHandle -previously allocated volume handle
 *
 * OUTPUT
 *		Nothing
 *
 * RETURN VALUES
 *		SUCCESS
 *
 * DESCRIPTION
 *		Frees volume handle to the pool.
 *
 * SEE ALSO
 *		NCPsilAllocVolume(3k)
 *
 * NOTES
 *		Assumes the handle is a valid pointer.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsilFreeVolume (	ncp_volume_t	*volumeHandle )
{
#ifdef NUC_DEBUG
	NCPsilValidateVolume (volumeHandle);
#endif
	kmem_free (volumeHandle, sizeof (ncp_volume_t));
	return (SUCCESS);
}

ccode_t
NCPsilGetVolumeNumber (	ncp_volume_t	*volumeHandle,
			int32		*volumeNumber )
{
#ifdef NUC_DEBUG
	NCPsilValidateVolume (volumeHandle);
#endif
	*volumeNumber = volumeHandle->number;
	return(SUCCESS);
}

#ifdef NUC_DEBUG
ccode_t
NCPsilValidateVolume (	ncp_volume_t	*volumeHandle )
{
	NTR_ASSERT( (volumeHandle->tag[0] == 'V') &&
			(volumeHandle->tag[1] == 'H') );
}
#endif
