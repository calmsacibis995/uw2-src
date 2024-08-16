/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:form/field_fore.c	1.2"

#include "utility.h"

	/*******************
	*  set_field_fore  *
	*******************/

int set_field_fore (f, fore)
FIELD * f;
chtype fore;
{
	f = Field (f);

	if ((fore & (chtype) A_ATTRIBUTES) != fore)
		return E_BAD_ARGUMENT;

	if (Fore (f) != fore)
	{
		Fore (f) = fore;
		return _sync_attrs (f);
	}
	return E_OK;
}

chtype field_fore (f)
FIELD * f;
{
	return Fore (Field (f));
}

