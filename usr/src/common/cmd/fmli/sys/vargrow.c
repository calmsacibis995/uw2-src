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
#ident	"@(#)fmli:sys/vargrow.c	1.3.3.3"

#include	<stdio.h>
#include 	"terror.h"	/* dmd s15 */
#include	"wish.h"
#include	"var_arrays.h"


/*
 * make the v_array bigger by one element, mallocing as needed
 */
struct v_array *
array_grow(array, step)
struct v_array	array[];
unsigned	step;
{
	register struct v_array	*ptr;
	register unsigned	delta;

	ptr = v_header(array);
	if (step > ptr->tot_left) {
		delta = ptr->step_size;
		if (delta < step)
			delta = step;
		if ((ptr = (struct v_array *)realloc(ptr, sizeof(struct v_array) + (ptr->tot_used + ptr->tot_left + delta) * ptr->ele_size)) == NULL)
			fatal(NOMEM, nil);	/* dmd s15 */
		ptr->tot_left += delta;
	}
	ptr->tot_used += step;
	ptr->tot_left -= step;
	return v_body(ptr);
}
