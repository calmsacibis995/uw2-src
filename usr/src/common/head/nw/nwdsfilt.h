/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsfilt.h	1.4"
#ifndef  _NWDSFILT_HEADER_
#define  _NWDSFILT_HEADER_

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

#ifndef __NWDSTYPE_H
#ifdef N_PLAT_UNIX
#include <nw/nwdstype.h>
#else
#include <nwdstype.h>
#endif
#endif

#ifndef  _NWDSDC_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsdc.h>
#else
#include <nwdsdc.h>
#endif
#endif

#ifndef _NWDSBUFT_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsbuft.h>
#else
#include <nwdsbuft.h>
#endif
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else
#include <npackon.h>
#endif

typedef struct _filter_node
{
   struct _filter_node  N_FAR *parent;
   struct _filter_node  N_FAR *left;
   struct _filter_node  N_FAR *right;
   nptr                       value;
   nuint32                    syntax;
   nuint16                    token;
} Filter_Node_T, N_FAR *pFilter_Node_T;

#define FTOK_END     0
#define FTOK_OR      1
#define FTOK_AND     2
#define FTOK_NOT     3
#define FTOK_LPAREN  4
#define FTOK_RPAREN  5
#define FTOK_AVAL    6
#define FTOK_EQ      7
#define FTOK_GE      8
#define FTOK_LE      9
#define FTOK_APPROX  10
#define FTOK_ANAME   14
#define FTOK_PRESENT 15
#define FTOK_RDN     16
#define FTOK_BASECLS 17
#define FTOK_MODTIME 18
#define FTOK_VALTIME 19

#define FBIT_END     (1L << FTOK_END)
#define FBIT_OR      (1L << FTOK_OR)
#define FBIT_AND     (1L << FTOK_AND)
#define FBIT_NOT     (1L << FTOK_NOT)
#define FBIT_LPAREN  (1L << FTOK_LPAREN)
#define FBIT_RPAREN  (1L << FTOK_RPAREN)
#define FBIT_AVAL    (1L << FTOK_AVAL)
#define FBIT_EQ      (1L << FTOK_EQ)
#define FBIT_GE      (1L << FTOK_GE)
#define FBIT_LE      (1L << FTOK_LE)
#define FBIT_APPROX  (1L << FTOK_APPROX)
#define FBIT_ANAME   (1L << FTOK_ANAME)
#define FBIT_PRESENT (1L << FTOK_PRESENT)
#define FBIT_RDN     (1L << FTOK_RDN)
#define FBIT_BASECLS (1L << FTOK_BASECLS)
#define FBIT_MODTIME (1L << FTOK_MODTIME)
#define FBIT_VALTIME (1L << FTOK_VALTIME)

#define FBIT_OPERAND (FBIT_LPAREN | FBIT_NOT | FBIT_PRESENT | FBIT_RDN \
         | FBIT_BASECLS | FBIT_ANAME | FBIT_MODTIME | FBIT_VALTIME)
#define FBIT_RELOP   (FBIT_EQ | FBIT_GE | FBIT_LE | FBIT_APPROX)
#define FBIT_BOOLOP  (FBIT_AND | FBIT_OR)

typedef struct
{
   pFilter_Node_T fn;
   nuint16        level;
   nuint32        expect;
} Filter_Cursor_T, N_FAR *pFilter_Cursor_T, N_FAR * N_FAR *ppFilter_Cursor_T;

#define FTAG_ITEM    0
#define FTAG_OR      1
#define FTAG_AND     2
#define FTAG_NOT     3


#ifdef __cplusplus
   extern "C" {
#endif

NWDSCCODE N_API NWDSAddFilterToken
(
   pFilter_Cursor_T  cur,
   nuint16           tok,
   nptr              val,
   nuint32           syntax
);

NWDSCCODE N_API NWDSAllocFilter
(
   ppFilter_Cursor_T cur
);

void N_API NWDSFreeFilter
(
   pFilter_Cursor_T  cur,
   void (N_FAR N_CDECL *freeVal)(nuint32 syntax, nptr val)
);

NWDSCCODE N_API NWDSPutFilter
(
   NWDSContextHandle       context,
   pBuf_T                  buf,
   pFilter_Cursor_T        cur,
   void (N_FAR N_CDECL *freeVal)(nuint32 syntax, nptr val)
);

NWDSCCODE N_API NWDSDelFilterToken
(
   pFilter_Cursor_T  cur,
   void (N_FAR N_CDECL *freeVal)(nuint32 syntax, nptr val)
);

#ifdef __cplusplus
   }
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else
#include <npackoff.h>
#endif

#endif                           /* #ifndef _NWDSFILT_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsfilt.h,v 1.6 1994/06/08 23:32:50 rebekah Exp $
*/
