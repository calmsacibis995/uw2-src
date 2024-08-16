/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/basename.c	1.1.1.2"
#ident  "$Header: basename.c 1.1 91/07/03 $"

#include	"MS-DOS.h"

#include	<stdio.h>
 
char	*
basename(input)
char	*input;
{
	char	*c_ptr;

	if ((c_ptr = strrchr(input + 1, '/')) == NULL)
		c_ptr = *input == '/' ? input + 1 : input;
	else
		c_ptr++;

	return(c_ptr);
}
