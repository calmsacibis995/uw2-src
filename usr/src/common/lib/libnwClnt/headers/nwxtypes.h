/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:headers/nwxtypes.h	1.2"
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
#ifndef	_NWTYPES_HEADER_
#define	_NWTYPES_HEADER_

#define NWMAX_TRANS_ADDR_LENGTH			30

typedef struct tagNWTRAN {
    nuint    transType;
    nuint    transLen;
    nuint8   transBuf[NWMAX_TRANS_ADDR_LENGTH];
} NWTRAN;

typedef struct tagNWU_FRAGMENT {
    pnuint8	fragAddress;
    nuint	fragSize;
} NWU_FRAGMENT, N_FAR *pNWU_FRAGMENT;


#endif
