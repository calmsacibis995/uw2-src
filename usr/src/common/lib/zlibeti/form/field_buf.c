/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libeti:form/field_buf.c	1.5"

#include "utility.h"
#include <stdio.h>

	/*********************
	*  set_field_buffer  *
	*********************/

int set_field_buffer (f, n, v)
FIELD * f;
int n;
char * v;
{
	char * p;
	char * x;
	int s;
	int err = 0;
	int	len;
	int	size;

	if (! f || ! v || n < 0 || n > f -> nbuf)
		return E_BAD_ARGUMENT;

	len = strlen( v );
	size = BufSize( f );

	if ( Status( f, GROWABLE ) && len > size )
		if ( !_grow_field( f, (len - size - 1)/GrowSize(f) + 1 ))
			return E_SYSTEM_ERROR;

	x = Buffer (f, n);
	s = BufSize (f);
	/* String buffer default string set
	   (to multibyte character maximum or NULL charcter) */
	p = memccpy (x, v, '\0', s);

	if (p)
		(void)memset (p - 1, ' ', s - (p - x) + 1);

	if (n == 0)
	{
		if (_sync_field (f) != E_OK)
			++err;
		if (_sync_linked (f) != E_OK)
			++err;
	}
	return err ? E_SYSTEM_ERROR : E_OK;
}

char * field_buffer (f, n)
FIELD * f;
int n;
{
/*
	field_buffer may not be accurate on the current field unless
	called from within the check validation function or the
	form/field init/term functions.

	field_buffer is always accurate on validated fields.
*/
	if (f && n >= 0 && n <= f -> nbuf)
		return Buffer (f, n);
	else
		return (char *) 0;
}

