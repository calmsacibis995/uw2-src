/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/varchkapnd.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"var_arrays.h"

/*
 * like array_append, but creates array if NULL
 */
struct v_array *
array_check_append(size, array, element)
unsigned size;
struct v_array	array[];
char	*element;
{
	if (array == NULL)
		array = array_create(size, 8);
	return array_append(array, element);
}
