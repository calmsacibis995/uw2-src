/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwaccess.h	1.1"
/*--------------------------------------------------------------------------
   Copyright (c) 1993 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#if ! defined( NWACCESS_H )
#define NWACCESS_H

#ifndef NTYPES_H
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include "ntypes.h"
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#define N_SYS_USER          0
#define N_SYS_NETWARE       1
#define N_SYS_UCS           2
#define N_SYS_MAX_COUNT     20

#define N_SYS_NAME_MAX_LEN  31

typedef struct tag_NWAccess
{
   nuint32 luConn;
} NWAccess, N_FAR *pNWAccess;

#define NWCSetConnP(_access_, _conn_) ((_access_)->luConn = (_conn_))

#define NWCGetConnP(_access_)         ((_access_)->luConn)

#define NWCSetConn(_access_, _conn_)  ((_access_).luConn = (_conn_))

#define NWCGetConn(_access_)          ((_access_).luConn)

#define NWCDeclareAccess(_access_) NWAccess _access_;

/* NOTE: NWClient contains functions which will fill an NWAccess structure */

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwaccess.h,v 1.1 1994/09/26 17:11:44 rebekah Exp $
*/

