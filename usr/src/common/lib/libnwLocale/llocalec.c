/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:llocalec.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/llocalec.c,v 1.1 1994/09/26 17:20:47 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LLOCALEC.C
  ==
  ==    routine         : NWLlocaleconv
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 21 June 1989
  ==
  ==	comments	: Get locale information.  It is only necessary for
  ==			  a program to call this function if the library
  ==			  routines do not do what it needs.
  ==
  ==    modifications   :
  ==
  ==	dependencies	: _NWGetCountryInfo
  ==
  ========================================================================*/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

LCONV N_FAR * N_API NWLlocaleconv(
   LCONV N_FAR *LlconvPtr)
{
   extern LCONV __lconvInfo;

   _Llocaleconv();
   *LlconvPtr = __lconvInfo;    /* Copy global structure to user copy */

   return LlconvPtr;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/llocalec.c,v 1.1 1994/09/26 17:20:47 rebekah Exp $
*/
