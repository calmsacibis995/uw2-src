/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:form/field_back.c	1.2"

#include "utility.h"

	/*******************
	*  set_field_back  *
	*******************/

int set_field_back (f, back)
FIELD * f;
chtype back;
{
	f = Field (f);

	if ((back & (chtype) A_ATTRIBUTES) != back)
		return E_BAD_ARGUMENT;

	if (Back (f) != back)
	{
		Back (f) = back;
		return _sync_attrs (f);
	}
	return E_OK;
}

chtype field_back (f)
FIELD * f;
{
	return Back (Field (f));
}

