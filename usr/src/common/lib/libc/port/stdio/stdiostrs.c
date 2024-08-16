/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/stdiostrs.c	1.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

/* shared constant strings in stdio */

const char	_str_shpath[] = "/sbin/sh";
const char	_str_shname[] = "sh";
const char	_str_sh_arg[] = "-c";
const char	_str_xxxxxx[] = "XXXXXX";
const char	_str_tmpdir[] = P_tmpdir;
const char	_str_devtty[] = "/dev/tty";
const char	_str_uc_hex[] = "0123456789ABCDEF";
const char	_str_lc_hex[] = "0123456789abcdef";
const char	_str_uc_nan[] = "NAN";
const char	_str_lc_nan[] = "nan";
const char	_str_uc_inf[] = "INFINITY";
const char	_str_lc_inf[] = "infinity";
#ifndef NO_NCEG_FPE
const char	_str__inity[] = "iInNiItTyY";
const wchar_t	_wcs_lc_nan[4] = {'n','a','n'};
const wchar_t	_wcs_lc_inf[9] = {'i','n','f','i','n','i','t','y'};
#endif
