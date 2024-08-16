/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdstype.h	1.4"
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
#define __NWDSTYPE_H

#ifdef N_PLAT_NLM
   #define NWFAR
   #define NWPASCAL
#endif

#ifndef N_PLAT_MAC

typedef unsigned long uint32;
typedef signed long int32;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned char uint8;
typedef signed char int8;

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
#include <nw/nwcaldef.h>
#else
#include <nwcaldef.h>
#endif
#endif

#ifndef NWDSCCODE
#define NWDSCCODE    int
#endif

#ifndef NWUNSIGNED
#define NWUNSIGNED unsigned
#endif

#else    /* for the Macintosh... */
#define NWDSCCODE    long

#ifndef NWFAR
#define NWFAR        far
#endif

#ifndef NWPASCAL
#define NWPASCAL     pascal
#endif

#ifndef N_API
#define N_API        NWFAR NWPASCAL
#endif

#define NWUNSIGNED unsigned long
#ifndef uchar
# define uchar unsigned char
#endif
#ifndef ushort
# define ushort   unsigned short
#endif
#ifndef ulong
# define ulong unsigned long
#endif

#ifndef uint8
# define uint8 unsigned char
#endif
#ifndef uint16
# define uint16   unsigned short
#endif
#ifndef uint32
# define uint32   unsigned long
#endif

#ifndef int8
# define int8  char
#endif
#ifndef int16
# define int16 short
#endif
#ifndef int32
# define int32 long
#endif

#include "NWMacTyp.h"
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdstype.h,v 1.6 1994/06/08 23:32:55 rebekah Exp $
*/
