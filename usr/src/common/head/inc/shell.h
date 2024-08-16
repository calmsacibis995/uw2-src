/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/shell.h	1.2"
#ifndef	_SHELL_HEADER_
#define	_SHELL_HEADER_

/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

N_EXTERN_LIBRARY( NWDSCCODE )
_NWCNDSGetConnectionSlot
(
	uint8          connectionType,  
	Net_Address_T	N_FAR *net,
	NWCONN_HANDLE	N_FAR *conn
);

N_EXTERN_LIBRARY( NWDSCCODE )
_NDSGetConnectionInfo(
	NWCONN_HANDLE	   conn,							
	uint8  N_FAR      *connectionStatus,		  		
	uint32 N_FAR      *connectionType,					
	uint8  N_FAR      *serverFlags, 			
	Net_Address_T N_FAR  *net
);

N_EXTERN_LIBRARY( NWDSCCODE )
_NDSGetConnectionSlot
(
	uint8          connectionType,  
	Net_Address_T	N_FAR *net,
	NWCONN_HANDLE	N_FAR *conn
);

#endif									/* #ifndef _SHELL_HEADER_ */
