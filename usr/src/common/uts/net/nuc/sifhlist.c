/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:net/nuc/sifhlist.c	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nuc/sifhlist.c,v 2.51.2.2 1994/12/21 02:49:06 ram Exp $"

/*
 *  Netware Unix Client
 *
 *	MODULE: sifhlist.c
 *	ABSTRACT: NCP Task structure file handle list manipulation structure 
 */ 

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
#include <net/nuc/nuc_prototypes.h>

#include <io/ddi.h>

/*
 * BEGIN_MANUAL_ENTRY(NCPsilFreeServer.3k)
 * NAME
 *
 * SYNOPSIS
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
/*
ccode_t
NCPsilFreeVolume( volumeListPtr, volumeNumber )
void_t	*volumeListPtr;
int32	volumeNumber;
{
	return(SUCCESS);
}
*/


