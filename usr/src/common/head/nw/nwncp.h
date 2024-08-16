/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwncp.h	1.5"
/*--------------------------------------------------------------------------
   Copyright (c) 1993 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWNCP_H
#define NWNCP_H

#ifndef NTYPES_H
#ifdef N_PLAT_UNIX
# include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
# include <ntypes.h>
#endif /* N_PLAT_UNIX */
#endif

#ifndef NWACCESS_H
#ifdef N_PLAT_UNIX
# include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
# include <nwaccess.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#define NCP_AUGMENT  1

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( nint32 )
NWNCPInit(pNWAccess pAccess);

N_EXTERN_LIBRARY( nint32 )
NWNCPTerm(pNWAccess pAccess);

#ifdef __cplusplus
}
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwncp.h,v 1.7 1994/09/26 17:12:23 rebekah Exp $
*/
