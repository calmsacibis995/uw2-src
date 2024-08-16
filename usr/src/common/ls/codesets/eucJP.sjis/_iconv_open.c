/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)langsup:common/ls/codesets/eucJP.sjis/_iconv_open.c	1.1"
#include <stdlib.h>
#include <dlfcn.h>
#include "_iconv_defs.h"

#define SJIS_STR	"sjis"
#define EUC_STR		"eucJP"
#define SJ2EUC		sjis_to_euc
#define EUC2SJ		euc_to_sjis
#define CLOSE_FN	_iconv_close

iconv_t
_iconv_open(void *handle, const char *tocode, const char *fromcode)
{
	iconv_t retval;
	size_t (*conv)(iconv_t, const char **, size_t *, char **, size_t *);
	
	if (strcmp(tocode, SJIS_STR) == 0 && strcmp(fromcode, EUC_STR) == 0) {
		conv = &EUC2SJ;
	} else
	if (strcmp(tocode, EUC_STR) == 0 && strcmp(fromcode, SJIS_STR) == 0) {
		conv = &SJ2EUC;
	} else {
		errno = EINVAL;
		return (iconv_t) -1;
	}
	if (conv == NULL) {
		errno = EINVAL;
		return (iconv_t) -1;
	}

	if ((retval = (iconv_t) malloc(sizeof(struct _t_iconv))) == NULL) {
		errno = ENOMEM;
		return (iconv_t) -1;
	}
	retval->conv = conv;
	retval->close = &CLOSE_FN;
	if (retval->close == NULL) {
		free(retval);
		errno = EINVAL;
		return (iconv_t) -1;
	}
	retval->handle=handle;
	return retval;
}
