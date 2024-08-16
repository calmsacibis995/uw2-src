/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/sap_dos.h	1.4"
#ident	"$Id: sap_dos.h,v 1.3 1994/05/05 16:21:22 mark Exp $"

/*
/*
 * Copyright 1991, 1992 Novell, Inc. 
 * All Rights Reserved.
 *
 * This work is subject to U.S. and International copyright laws and
 * treaties.  No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 */

#ifndef _NET_NW_SAP_DOS__
#define _NET_NW_SAP_DOS__

#include <sys/types.h>
#include <sys/sap.h>

/*
**	The following are SAP API functions are used by processes accessing
**	the Sap Server on the local machine.
*/
#if defined( __STDC__) || defined(__cplusplus)
#ifdef __cplusplus
extern "C" {
#endif
extern int AdvertiseService( uint16, char *, uint8 * );
extern int ShutdownSAP( void );
extern int QueryServices( uint16, uint16, uint16, SAP_ID_PACKET * );
#ifdef __cplusplus
}
#endif

#else
extern int AdvertiseService();
extern int ShutdownSAP();
extern int QueryServices();
#endif

#endif /* _NET_NW_SAP_DOS__ */
