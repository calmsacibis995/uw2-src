/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:form/field_init.c	1.1"

#include "utility.h"

	/*******************
	*  set_field_init  *
	*******************/

int set_field_init (f, func)
FORM * f;
PTF_void func;
{
	Form (f) -> fieldinit = func;
	return E_OK;
}

PTF_void field_init (f)
FORM * f;
{
	return Form (f) -> fieldinit;
}

