/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/slhandle.c	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/slhandle.c,v 2.55.2.3 1995/01/05 17:54:50 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	  MODULE: slhandle.c
 *	  ABSTRACT: SPIL resource handle library for managing generic
 *	  resource handles that are handed to the consumer as a
 *	  resource tag.
 *
 *	  Each generic resource handle consists of at least 2 components:
 *
 *		Local Resource Handle - Local structure for the SPIL layer
 *		Service Protocol Res handle - Handle allocated by the
 *									  underlying service protocol
 *									  module.
 *	
 *
 *	NOTE: Need to complete man pages for 1-line functions.
 *
 *	Functions defined in this module:
 *
 *	NWslAllocHandle
 *	NWslFreeHandle
 *	NWslSetHandleSProtoResHandle
 *	NWslGetHandleSProtoResHandle
 *	NWslSetHandleLocalResHandle
 *	NWslGetHandleLocalResHandle
 *	NWslGetHandleStamp
 *	NWslValidateStamp
 *	NWslGetHandleStampType
 *
 */ 

#include <net/nuc/nuctool.h>
#include <net/nuc/nwctypes.h>
#include <net/tiuser.h>
#include <net/nuc/slstruct.h>
#include <net/nw/nwportable.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/slstruct.h>
#include <net/nuc/nucerror.h>
#include <util/cmn_err.h>
#include <net/nuc/nwspiswitch.h>
#include <net/nuc/nwspi.h>
#include <net/nw/ntr.h>
#include <net/nuc/nuc_prototypes.h>

#include <util/debug.h>
#include <util/ksynch.h>
#include <mem/kmem.h>

#include <io/ddi.h>

#define NTR_ModMask NTRM_spil

/*
 * BEGIN_MANUAL_ENTRY(NWslAllocHandle.3k)
 * NAME
 *		NWslAllocHandle
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslAllocHandle( handle )
 *		void_t			*handle;
 *
 * INPUT
 *		void_t			*handle;
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
enum NUC_DIAG
NWslAllocHandle (SPI_HANDLE_T	**handle, uint32			stamp)
{
	*handle = 
		(SPI_HANDLE_T *)kmem_zalloc (sizeof(SPI_HANDLE_T), KM_SLEEP);

	/*
	 *	Stamp the handle's type for sanity checking at the interface
	 */
	(*handle)->stamp = stamp;

	return(SUCCESS);
}

/*
 * BEGIN_MANUAL_ENTRY(NWslFreeHandle.3k)
 * NAME
 *		NWslFreeHandle
 *
 * SYNOPSIS
 *		enum NUC_DIAG
 *		NWslFreeHandle( handle )
 *		void_t			*handle;
 *
 * INPUT
 *		void_t			*handle;
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
enum NUC_DIAG
NWslFreeHandle (SPI_HANDLE_T	*handle)
{
	NTR_ASSERT(handle != (SPI_HANDLE_T *)NULL);
	kmem_free (handle, sizeof(SPI_HANDLE_T));
	handle = NULL;
	return(SUCCESS);
}


void_t
NWslGetHandleStampType ( int32	objectType, int32	*stampType)
{
	switch (objectType) {
		case NS_DIRECTORY:
		case NS_ROOT:
			*stampType = SPI_DHANDLE_STAMP;
			break;

		case NS_FILE:
		case NS_SYMBOLIC_LINK:
			*stampType = SPI_FHANDLE_STAMP;
			break;

		default:
			cmn_err (CE_PANIC, "NWslGetHandleStampType: bad objectType\n");
			break;
	}
}
