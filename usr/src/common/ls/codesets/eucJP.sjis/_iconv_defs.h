/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)langsup:common/ls/codesets/eucJP.sjis/_iconv_defs.h	1.1"
#include <errno.h>
#include <iconv.h>
#include "iconvm.h"

iconv_t _iconv_open(void *handle, const char *tocode, const char *fromcode);
size_t
euc_to_sjis(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf,
		size_t *outbytesleft);
size_t
sjis_to_euc(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf,
		size_t *outbytesleft);
int _iconv_close(iconv_t cd);

#ifndef NULL
#define NULL 0
#endif
