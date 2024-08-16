/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/xti_strerror.c	1.7"

#include <unistd.h>
#include <sys/xti.h>
#include <string.h>
#include "_import.h"
#include "nsl_mt.h"

/*
 * t_nerr represents the value of the last XTI error from <sys/xti.h>.
 * It must be updated if new errors and error messages are added.
 */
const int t_nerr = TPROTO;

#ifdef _REENTRANT
extern THREAD_KEY_T _nsl_key;
#endif /* _REENTRANT */

/*
 * The following symbol is here just-in-case.  In principle,
 * it is not needed since t_strerror() was introduced only with
 * XPG4's XTI.
 */
#pragma weak t_strerror = _xti_strerror

char *
_xti_strerror(int errnum)
{
	static char unk_err[64];

/*
 * the following strings must be exactly as specified by Xopen
 * in xti.h (the comments by the t_errno values)
 */
	switch (errnum) {

	case TBADADDR:						/*  1 */
	    return gettxt("uxnsl:1", "incorrect addr format");
	case TBADOPT:						/*  2 */
	    return gettxt("uxnsl:2", "incorrect option format");
	case TACCES:						/*  3 */
	    return gettxt("uxnsl:3", "incorrect permissions");
	case TBADF:						/*  4 */
	    return gettxt("uxnsl:4", "illegal transport fd");
	case TNOADDR:						/*  5 */
	    return gettxt("uxnsl:5", "couldn't allocate addr");
	case TOUTSTATE:						/*  6 */
	    return gettxt("uxnsl:6", "out of state");
	case TBADSEQ:						/*  7 */
	    return gettxt("uxnsl:7", "bad call sequence number");
	case TSYSERR:						/*  8 */
	    return gettxt("uxnsl:8", "system error");
	case TLOOK:						/*  9 */
	    return gettxt("uxnsl:9", "event requires attention");
	case TBADDATA:						/* 10 */
	    return gettxt("uxnsl:10", "illegal amount of data");
	case TBUFOVFLW:						/* 11 */
	    return gettxt("uxnsl:11", "buffer not large enough");
	case TFLOW:						/* 12 */
	    return gettxt("uxnsl:12", "flow control");
	case TNODATA:						/* 13 */
	    return gettxt("uxnsl:13", "no data");
	case TNODIS:						/* 14 */
	    return gettxt("uxnsl:14", "discon_ind not found on queue");
	case TNOUDERR:						/* 15 */
	    return gettxt("uxnsl:15", "unitdata error not found");
	case TBADFLAG:						/* 16 */
	    return gettxt("uxnsl:16", "bad flags");
	case TNOREL:						/* 17 */
	    return gettxt("uxnsl:17", "no ord rel found on queue");
	case TNOTSUPPORT:					/* 18 */
	    return gettxt("uxnsl:18", "primitive/action not supported");
	case TSTATECHNG:					/* 19 */
	    return gettxt("uxnsl:19", "state is in process of changing");
	case TNOSTRUCTYPE:					/* 20 */
	    return gettxt("uxnsl:20", "unsupported struct-type requested");
	case TBADNAME:						/* 21 */
	    return gettxt("uxnsl:21", "invalid transport provider name");
	case TBADQLEN:						/* 22 */
	    return gettxt("uxnsl:22", "qlen is zero");
	case TADDRBUSY:						/* 23 */
	    return gettxt("uxnsl:23", "address in use");
	case TINDOUT:						/* 24 */
	    return gettxt("uxnsl:24", "outstanding connection indications");
	case TPROVMISMATCH:					/* 25 */
	    return gettxt("uxnsl:25", "transport provider mismatch");
	case TRESQLEN:						/* 26 */
	    return gettxt("uxnsl:26", "resfd specified to accept w/qlen >0");
	case TRESADDR:						/* 27 */
	    return gettxt("uxnsl:27", "resfd not bound to same addr as fd");
	case TQFULL:						/* 28 */
	    return gettxt("uxnsl:28", "incoming connection queue full");
	case TPROTO:						/* 29 */
	    return gettxt("uxnsl:29", "XTI protocol error");

	default:
#ifdef _REENTRANT
	    {
		struct _nsl_tsd *key_tbl;

		if (FIRST_OR_NO_THREAD) {
			sprintf (unk_err, gettxt("uxnsl:30", "%d: error unknown"), errnum);
			return(unk_err);
		}

		key_tbl = (struct _nsl_tsd *)
                    _mt_get_thr_specific_storage(_nsl_key, NSL_KEYTBL_SIZE);
		if (key_tbl == NULL) {
			set_t_errno(TSYSERR);
			return "";
		}
		if (key_tbl->unk_err_str_p == NULL)
			key_tbl->unk_err_str_p = (char *)
			    calloc(64, sizeof(char));
		if (key_tbl->unk_err_str_p == NULL) {
			set_t_errno(TSYSERR);
			return "";
		}
        	sprintf(key_tbl->unk_err_str_p, gettxt("uxnsl:30", "%d: error unknown"), errnum);
        	return key_tbl->unk_err_str_p;
	    }
#else
	    sprintf (unk_err, gettxt("uxnsl:30", "%d: error unknown"), errnum);
	    return(unk_err);
#endif

	}
}
