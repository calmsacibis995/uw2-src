/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:isalnum.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/isalnum.c,v 1.1 1994/09/26 17:20:40 rebekah Exp $"
/*****************************************************************************
* BEGIN_MANUAL_ENTRY ( NWisalnum )
*
* Supported in (marked with X):
* 2.11  2.2   3.0   3.10  3.11  3.2
*  X		 X		 X		 X		 X		 X
*
* SOURCE MODULE  : ISALNUM.C
*
* API NAME       : NWisalnum
*
* INCLUDE        : NWLOCALE.H
*
* SYNTAX         : int N_API NWisalnum( unsigned int ch );
*
* PARAMETERS     : ch -> the character to be tested
*
* RETURN         : nonzero for true, zero for false
*
* DESCRIPTION    : tests for the digits '0' through '9' or the letters
*                  'A' through 'Z' or 'a' through 'z'
*
* MODIFICATIONS  : All DBCS are assumed to be alphanumeric 
*
* END_MANUAL_ENTRY
*****************************************************************************/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#include <ctype.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

int N_API NWisalnum(
   unsigned int ch )
{
    if (NWCharType( (unsigned char) ch ) == NWSINGLE_BYTE)
        return isalnum( (unsigned char) ch );
	 else		/* DBCS just return the char back */
        return ch;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/isalnum.c,v 1.1 1994/09/26 17:20:40 rebekah Exp $
*/
