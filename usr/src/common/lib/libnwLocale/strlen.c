/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:strlen.c	1.2"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/strlen.c,v 1.3 1994/09/26 17:21:23 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Library name:  NWLOCALE.LIB                                              *
 *	                                                                         *
 * Filename:      STRLEN.C                                                  *
 *                                                                          *
 * Date Created:  November 1992                                             *
 *                                                                          *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.          *
 *                                                                          *
 * No part of this file may be duplicated, revised, translated, localized   *
 * or modified in any manner or compiled, linked or uploaded or downloaded	 *
 * to or from any computer system without the prior written consent of      *
 * Novell, Inc.                                                             *
 *                                                                          *
 ****************************************************************************/

#define NWL_EXCLUDE_FILE 1
#define NWL_EXCLUDE_TIME 1

#include "ntypes.h"
#include "nwlocale.h"
#include "locifunc.h"

int N_API NWstrlen(char N_FAR *str)
{
   int len = 0;

   while (*str++)
      len++;

   return len;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/strlen.c,v 1.3 1994/09/26 17:21:23 rebekah Exp $
*/
