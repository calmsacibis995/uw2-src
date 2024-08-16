/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:unipcpy.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unipcpy.c,v 1.1 1994/09/26 17:21:44 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNIPCPY.C												*
 *																			*
 * Date Created:	October 8, 1991											*
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
	Unicode version of stpcpy function

	Example:

		unipcpy(s1, s2);

	Description:

		Copies the unicode string pointed to by s2 (including the terminating
		null character) into the array pointed to by s2.  Same as unicpy
		except for the return value.

	Returns:
		
		Returns the value of s1 + strlen(s2).
*/

/****************************************************************************/

#ifndef MACINTOSH

#include "ntypes.h"

#include "unicode.h"

/****************************************************************************/

unicode N_FAR * N_API unipcpy(

	unicode N_FAR *s1,		  	/* Buffer characters are copied to			*/
	unicode N_FAR *s2)		  	/* Buffer characters are copied from		*/

{

	int		 i = 0;				/* Loop counter								*/

	/*
		Copy until the null
	*/
	do
	{
		s1[i++] = *s2;
	} while (*s2++);

	/*
		Return adjusted value of s1
	*/
	return (s1 + i - 1);
}

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/unipcpy.c,v 1.1 1994/09/26 17:21:44 rebekah Exp $
*/
