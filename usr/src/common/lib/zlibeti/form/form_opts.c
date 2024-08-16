/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libeti:form/form_opts.c	1.2"

#include "utility.h"

	/******************
	*  set_form_opts  *
	******************/

int set_form_opts (f, opts)
FORM * f;
OPTIONS opts;
{
	Form (f) -> opts = opts;
	return E_OK;
}

OPTIONS form_opts (f)
FORM * f;
{
	return Form (f) -> opts;
}

int form_opts_on (f, opts)
FORM * f;
OPTIONS opts;
{
	Form (f) -> opts |= opts;
	return E_OK;
}

int form_opts_off (f, opts)
FORM * f;
OPTIONS opts;
{
	Form (f) -> opts &= ~opts;
	return E_OK;
}
