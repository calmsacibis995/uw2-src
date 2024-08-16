/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:uninset.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uninset.c,v 1.1 1994/09/26 17:21:41 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNINSET.C												*
 *																			*
 * Date Created:	October 15, 1991										*
 *																			*
 * Version:			1.00													*
 *																			*
 * Programmers:		Lloyd Honomichl											*
 *																			*
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.			*
 *																			*
 * No part of this file may be duplicated, revised, translated, localized	*
 * or modified in any manner or compiled, linked or uploaded or downloaded	*
 * to or from any computer system without the prior written consent of 		*
 * Novell, Inc.																*
 *																			*
 ****************************************************************************/

/*
	Unicode version of strnset function

	Example:

		uninset(s, c, n);

	Description:

		Sets not more than n characters in the array pointed to by s to the
		character specified by c.  Stops when it reaches the terminating null
		character, or reaches the length specified.

	Returns:
		
		Returns a pointer to s.
*/

/****************************************************************************/

#ifndef MACINTOSH

#include "ntypes.h"

#include "unicode.h"


unicode N_FAR * N_API uninset
(

	unicode N_FAR *s,				/* String to be modified					*/
	unicode	 c,					/* Character the string will be filled with	*/
	size_t	 length)			   /* Maximum length to be filled				*/

{

	int		 i;					/* Loop variable							*/

	/*
		Fill the string with the specified character
	*/
	for ( i = 0 ; i < (int)length && s[i] ; i++ )
		s[i] = c;

	/*
		Return pointer to s
	*/
	return s;
}

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uninset.c,v 1.1 1994/09/26 17:21:41 rebekah Exp $
*/
