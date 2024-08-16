/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/ndsncp.h	1.4"
#ifndef  _NDSNCP_HEADER_
#define  _NDSNCP_HEADER_
/********************************************************************
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised,
 * translated, localized or modified in any manner
 * or compiled, linked or uploaded or downloaded to
 * or from any computer system without the prior
 * written permission of Novell, Inc.
 *
 ********************************************************************/
#ifndef NWMISC_INC
#include <nwmisc.h>
#endif   /* NWMISC_INC */

#ifdef N_PLAT_MAC        /* for the Macintosh */
#include "NWContext.h"
#define NWCONN_HANDLE   NWConnRefNumHandle
#endif   /* MACINTOSH */

/* Prototypes  */

N_EXTERN_LIBRARY( NWDSCCODE )
NDSSetMonitoredConnection
(
   NWCONN_HANDLE  conn
);

#endif   /*_NDSNCP_HEADER_ */

/* ################################################################ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/ndsncp.h,v 1.6 1994/06/08 23:35:31 rebekah Exp $
*/
