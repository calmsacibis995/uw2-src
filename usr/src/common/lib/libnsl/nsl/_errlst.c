/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/_errlst.c	1.3.6.8"
#ident	"$Header: $"

#include <stdlib.h>
#include <errno.h>
#include <sys/xti.h>
#include "nsl_mt.h"

/*
 * transport errno
 */
#undef t_errno

#ifdef _REENTRANT
extern THREAD_KEY_T _nsl_key;
#endif /* _REENTRANT */

int t_errno = 0; 

/* 
 * This array of messages is kept here for application backwards
 * compatibility.  It may go away in a future release.
 *
 * These are NOT the internationalized messages.  For
 * internationalized messages, use t_strerror() and t_error().
 */

const char *t_errlist[] = {
	"No Error",					/*  0 */
	"incorrect addr format",		  	/*  1 */
	"incorrect option format",			/*  2 */
	"incorrect permissions",			/*  3 */
	"illegal transport fd", 			/*  4 */
	"couldn't allocate addr",  			/*  5 */
	"out of state",					/*  6 */
	"bad call sequence number",			/*  7 */
	"system error",					/*  8 */
	"event requires attention",			/*  9 */
	"illegal amount of data",			/* 10 */
	"buffer not large enough",			/* 11 */
	"flow control",					/* 12 */
	"no data",					/* 13 */
	"discon_ind not found on queue",		/* 14 */
	"unitdata error not found",			/* 15 */
	"bad flags",					/* 16 */
	"no ord rel found on queue",			/* 17 */
	"primitive/action not supported",		/* 18 */
	"state is in process of changing",              /* 19 */
	"unsupported struct-type requested",		/* 20 */
	"invalid transport provider name",		/* 21 */
	"qlen is zero",					/* 22 */
	"address in use",				/* 23 */
	"outstanding connection indications",		/* 24 */
	"transport provider mismatch",			/* 25 */
	"resfd specified to accept w/qlen >0",		/* 26 */
	"resfd not bound to same addr as fd",		/* 27 */
	"incoming connection queue full",		/* 28 */
	"XTI protocol error",				/* 29 */
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	""
};

int
get_t_errno()
{
#ifdef _REENTRANT
	struct _nsl_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) return (t_errno);

	key_tbl = (struct _nsl_tsd *)
		  _mt_get_thr_specific_storage(_nsl_key, NSL_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->t_errno_p != NULL)
		return(*(int *)key_tbl->t_errno_p);
	errno = ENOMEM;
	return (TSYSERR);
#else
	return (t_errno);
#endif /* _REENTRANT */
}

int
set_t_errno(errcode)
	int errcode;
{
#ifdef _REENTRANT
	struct _nsl_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		t_errno = errcode;
		return 0;
	}
	key_tbl = (struct _nsl_tsd *)
		  _mt_get_thr_specific_storage(_nsl_key, NSL_KEYTBL_SIZE);
	if (key_tbl == NULL) return -1;
	if (key_tbl->t_errno_p == NULL) 
		key_tbl->t_errno_p = calloc(1, sizeof(int));
	if (key_tbl->t_errno_p == NULL)
		return -1;
	*(int *)key_tbl->t_errno_p = errcode;
#else
	t_errno = errcode;
#endif /* _REENTRANT */
	return 0;
}

int *
_t_errno()
{
#ifdef _REENTRANT
	struct _nsl_tsd *key_tbl;
	static int __t_errno;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD)
		return (&t_errno);

	key_tbl = (struct _nsl_tsd *)
		  _mt_get_thr_specific_storage(_nsl_key, NSL_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->t_errno_p != NULL)
		return((int *)key_tbl->t_errno_p);
	errno = ENOMEM;
	__t_errno = TSYSERR;
	return (&__t_errno);
#else
	return (&t_errno);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_nsl_t_errno(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */
