/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:form/form_user.c	1.1"

#include "utility.h"

	/*********************
	*  set_form_userptr  *
	*********************/

int set_form_userptr (f, userptr)
FORM * f;
char * userptr;
{
	Form (f) -> usrptr = userptr;
	return E_OK;
}

char * form_userptr (f)
FORM * f;
{
	return Form (f) -> usrptr;
}

