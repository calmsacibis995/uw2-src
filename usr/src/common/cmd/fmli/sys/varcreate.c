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
#ident	"@(#)fmli:sys/varcreate.c	1.3.3.3"

#include	<stdio.h>
#include 	"terror.h"	/* dmd s15 */
#include	"wish.h"
#include	"var_arrays.h"


/*
 * create a new v_array with space in it for "num" elements of size "size"
 * (but the array is empty - must use array_grow or array append to fill it)
 */
struct v_array *
array_create(size, num)
unsigned	size;
unsigned	num;
{
	register unsigned	realsize;
	register unsigned	initstep;
	register struct v_array	*ptr;

	realsize = size * num + sizeof(struct v_array);
	initstep = num / 10;
	if (initstep < 2)
		initstep = 2;
	if ((ptr = (struct v_array *)malloc(realsize)) == NULL)
		fatal(NOMEM, nil);	/* dmd s15 */
	ptr->tot_used = 0;
	ptr->tot_left = num;
	ptr->ele_size = size;
	ptr->step_size = initstep;
	return v_body(ptr);
}
