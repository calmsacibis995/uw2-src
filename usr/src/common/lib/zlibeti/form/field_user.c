/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:form/field_user.c	1.1"

#include "utility.h"

	/**********************
	*  set_field_userptr  *
	**********************/

int set_field_userptr (f, userptr)
FIELD * f;
char * userptr;
{
	Field (f) -> usrptr = userptr;
	return E_OK;
}

char * field_userptr (f)
FIELD * f;
{
	return Field (f) -> usrptr;
}

