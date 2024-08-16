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
#ident	"@(#)fmli:sys/varappend.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"var_arrays.h"

/*
 * add another element onto the end of a v_array
 */
struct v_array *
array_append(array, element)
struct v_array	array[];
char	*element;
{
	register struct v_array	*ptr;

	ptr = v_header(array_grow(array, 1));
	if (element != NULL)
		memcpy(ptr_to_ele(ptr, ptr->tot_used - 1), element, ptr->ele_size);
	return v_body(ptr);
}
