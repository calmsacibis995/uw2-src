/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getwidth.c	1.2"
#include "synonyms.h"
#include <widec.h>
#include "wcharm.h"
#include <sys/euc.h>
#include <ctype.h>

#ifdef __STDC__
	#pragma weak getwidth = _getwidth
#endif

void getwidth(eucstruct)
eucwidth_t *eucstruct;
{
	eucstruct->_eucw1 = eucw1;
	eucstruct->_eucw2 = eucw2;
	eucstruct->_eucw3 = eucw3;
	eucstruct->_multibyte = multibyte;
	eucstruct->_pcw = sizeof(wchar_t);
	eucstruct->_scrw1 = scrw1;
	eucstruct->_scrw2 = scrw2;
	eucstruct->_scrw3 = scrw3;
}
