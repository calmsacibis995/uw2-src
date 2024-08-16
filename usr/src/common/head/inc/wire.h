/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/wire.h	1.4"
#ifndef __WIRE_H
#define __WIRE_H
/***************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written consent of
 * Novell, Inc.
 *
 ***************************************************************************/
#ifndef  _NWDSATTR_HEADER_
#include <nwdsattr.h>
#endif


#define BUFFER_PAD 3

/* NWClient DS prototypes */
int NWCWGetInt32(ppnstr8 cur, pnstr8 limit, pnuint32 i);
int NWCWPutInt32(ppnstr8 cur, pnstr8 limit, nuint32 i);
int NWCWPutAlign32(ppnstr8 cur, pnstr8 limit);
int NWCWGetAlign32(ppnstr8 cur, pnstr8 limit);
int NWCWPutData(ppnstr8 cur, pnstr8 limit, nuint32 size, nptr data);
int NWCWGetData(ppnstr8 cur, pnstr8 limit, pnuint32 size, ppnstr8 data);
int NWCWPutString(ppnstr8 cur, pnstr8 limit, punicode str);
int NWCWGetString(ppnstr8 cur, pnstr8 limit, ppunicode str);

/* NWNet DS Prototypes */
NWDSCCODE WGetBoolean(ppnstr8 cur, pnstr8 limit, pnuint8 boolean);
NWDSCCODE WPutBoolean(ppnstr8 cur, pnstr8 limit, nuint8 boolean);
NWDSCCODE WGetInt8(ppnstr8 cur, pnstr8 limit, pnuint8 i);
NWDSCCODE WPutInt8(ppnstr8 cur, pnstr8 limit, nuint8 i);
NWDSCCODE WGetInt16(ppnstr8 cur, pnstr8 limit, pnuint16 i);
NWDSCCODE WPutInt16(ppnstr8 cur, pnstr8 limit, nuint16 i);
NWDSCCODE WGetInt32(ppnstr8 cur, pnstr8 limit, pnuint32 i);
NWDSCCODE WPutInt32(ppnstr8 cur, pnstr8 limit, nuint32 i);
NWDSCCODE WSkipInt8(ppnstr8 cur, pnstr8 limit, ppnstr8 ip);
NWDSCCODE WSkipInt16(ppnstr8 cur, pnstr8 limit, ppnstr8 ip);
NWDSCCODE WSkipInt32(ppnstr8 cur, pnstr8 limit, ppnstr8 ip);
NWDSCCODE WGetAlign32(ppnstr8 cur, pnstr8 limit);
NWDSCCODE WPutAlign32(ppnstr8 cur, pnstr8 limit);
NWDSCCODE WPutData(ppnstr8 cur, pnstr8 limit, nuint32 size, nptr data);
NWDSCCODE WGetData(ppnstr8 cur, pnstr8 limit, pnuint32 size, ppnstr8 data);
NWDSCCODE WPutString(ppnstr8 cur, pnstr8 limit, punicode str);
NWDSCCODE WGetString(ppnstr8 cur, pnstr8 limit, ppunicode str);
NWDSCCODE WGetTimeStamp(ppnstr8 cur, pnstr8 limit, pTimeStamp_T timeStamp);
NWDSCCODE WPutTimeStamp(ppnstr8 cur, pnstr8 limit, pTimeStamp_T timeStamp);

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/wire.h,v 1.4 1994/06/08 23:35:55 rebekah Exp $
*/
