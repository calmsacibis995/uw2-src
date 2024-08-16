/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/siltask.c	1.18"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/siltask.c,v 2.55.2.3 1995/01/05 17:54:45 hashem Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE: sitask.c
 *	ABSTRACT: Dependent task structure manipulation
 *
 *	Functions declared in this module:
 *	Public functions:
 *		NCPsilAllocTask
 *		NCPsilFreeTask
 *		NCPsilSetTaskDiagnosticLevel
 *		NCPsilGetTaskDiagnosticInfo
 *		NCPsilSetTaskChannel
 *		NCPsilGetTaskChannel
 *		NCPsilSetTaskCredentials
 *		NCPsilGetTaskServer
 *		NCPsilSetTaskAuthInfo
 *		NCPsilGetTaskAuthInfo
 *	Private functions:
 *	NUC_DEBUG functions:
 * 		ValidateTask
 *
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
#include <net/nuc/ncpiopack.h>
#include <net/nuc/nuc_prototypes.h>
#include <io/ddi.h>
#include <net/nw/ntr.h>
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
#include <sys/ncpiopack.h>
#include <sys/debug.h>

#endif /* ndef _KERNEL_HEADERS */

#define NTR_ModMask NTRM_ncp

/*
 *	Forward declarations
 */
#ifdef NUC_DEBUG
ccode_t NCPsilValidateTask();
#endif

/*
 * BEGIN_MANUAL_ENTRY(NCPsilAllocTask.3k)
 * NAME
 *
 * SYNOPSIS
 *		ccode_t
 *		NCPsilAllocTask( serverPtr, taskPtr )
 *		void_t		*serverPtr;
 *		ncp_task_t	**taskPtr;
 *
 * INPUT
 *		void_t		*serverPtr;
 *
 * OUTPUT
 *		ncp_task_t	**taskPtr;
 *
 * RETURN VALUES
 *
 * DESCRIPTION
 *
 * NOTES
 *		Allocate a task structure from the NCP memory pool.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NCPsilAllocTask( void_t		*serverPtr, ncp_task_t	**taskPtr )
{

	*taskPtr = (ncp_task_t *)kmem_zalloc ( sizeof(ncp_task_t), KM_SLEEP );
#ifdef NUCMEM_DEBUG
	NTR_PRINTF("NUCMEM: NCPsilAllocTask: alloc ncp_task_t * at 0x%x, size = 0x%x",
                *taskPtr, sizeof(ncp_task_t), 0 );
#endif NUCMEM_DEBUG

	
	(*taskPtr)->serverPtr = serverPtr;
#if ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	(*taskPtr)->tag[0] = 'T';
	(*taskPtr)->tag[1] = 'H';
#endif ((defined DEBUG) || (defined NUC_DEBUG) || (defined DEBUG_TRACE))
	return(SUCCESS);
}

#ifdef NUC_DEBUG
ccode_t
NCPsilValidateTask( ncp_task_t *taskPtr )
{
	NTR_ASSERT( (taskPtr->tag[0] == 'T') &&
			(taskPtr->tag[1] == 'H') );
}
#endif
