/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:nextchar.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/nextchar.c,v 1.4 1994/09/26 17:21:14 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==	file		: NextChar.c
  ==
  ==	routine 	: NWNextChar
  ==
  ==	author		: Joe Ivie
  ==
  ==	date		: 22 December 1993
  ==
  ==	comments	: This function is Double-Byte sensitive
  ==
  ==    modifications   :
  ==
  ==	dependencies	:
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

char N_FAR * N_API NWNextChar(
   char N_FAR *str)
{
	 return((str += (NWCharType((unsigned char) *str) == NWSINGLE_BYTE) ? 1 : 2));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/nextchar.c,v 1.4 1994/09/26 17:21:14 rebekah Exp $
*/
