/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/clnt_perror.c	1.3.10.4"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * clnt_perror.c
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdir.h>
#include <rpc/types.h>
#include "trace.h"
#include <rpc/auth.h>
#include <sys/xti.h>
#include <rpc/clnt.h>
#include "rpc_mt.h"

#undef rpc_createerr	/* Need automatic to store get_rpc_createerr() return */

extern int t_nerr;

const char __nsl_dom[]  = "SYS_NETRPC";

#define ERRMSG_SIZE 256

static char *
__buf()
{
	static char *buf;
#ifdef _REENTRANT
	struct _rpc_tsd *key_tbl;
#endif /* _REENTRANT */

        trace1(TR___buf, 0);
#ifdef _REENTRANT

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		if (buf == NULL)
			buf = (char *)malloc(ERRMSG_SIZE);
        	trace1(TR___buf, 1);
		return (buf);
	}

	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rpc_tsd *)
		  _mt_get_thr_specific_storage(__rpc_key, RPC_KEYTBL_SIZE);
	if (key_tbl == NULL) {
		trace1(TR___buf, 1);
                return(NULL);
	}
	if (key_tbl->clnt_error_p == NULL) 
		key_tbl->clnt_error_p = (void *)malloc(ERRMSG_SIZE);
	trace1(TR___buf, 1);
	return ((char *)key_tbl->clnt_error_p);
#else
	if (buf == NULL)
		buf = (char *)malloc(ERRMSG_SIZE);
        trace1(TR___buf, 1);
	return (buf);
#endif /* _REENTRANT */

}

#ifdef _REENTRANT

void
_free_rpc_clnt_error(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

static char *
auth_errmsg(stat)
	enum auth_stat stat;
{
	trace1(TR_auth_errmsg, 0);
	switch (stat) {
	case AUTH_OK:
		trace1(TR_auth_errmsg, 1);
		return (gettxt("uxnsl:46", "Authentication OK"));
	case AUTH_BADCRED:
		trace1(TR_auth_errmsg, 1);
		return (gettxt("uxnsl:47", "Invalid client credential"));
	case AUTH_REJECTEDCRED:
		trace1(TR_auth_errmsg, 1);
		return (gettxt("uxnsl:48", "Server rejected credential"));
	case AUTH_BADVERF:
		trace1(TR_auth_errmsg, 1);
		return (gettxt("uxnsl:49", "Invalid client verifier"));
	case AUTH_REJECTEDVERF:
		trace1(TR_auth_errmsg, 1);
		return (gettxt("uxnsl:50", "Server rejected verifier"));
	case AUTH_TOOWEAK:
		trace1(TR_auth_errmsg, 1);
		return (gettxt("uxnsl:51", "Client credential too weak"));
	case AUTH_INVALIDRESP:
		trace1(TR_auth_errmsg, 1);
		return (gettxt("uxnsl:52", "Invalid server verifier"));
	case AUTH_FAILED:
		trace1(TR_auth_errmsg, 1);
		return (gettxt("uxnsl:53", "Failed (unspecified error)"));
	}
	trace1(TR_auth_errmsg, 1);
	return (gettxt("uxnsl:54", "Unknown authentication error"));
}

/*
 * Return string reply error info. For use after clnt_call()
 */
char *
clnt_sperror(cl, s)
	CLIENT *cl;
	char *s;
{
	struct rpc_err e;
	void clnt_perrno();
	char *err;
	char *str, *buf;

	trace2(TR_clnt_sperror, 0, cl);
	str = buf =__buf();
	if (str == NULL) {
		trace2(TR_clnt_sperror, 1, cl);
		return (NULL);
	}
	CLNT_GETERR(cl, &e);

	(void) sprintf(str, "%s: ", s);
	str += strlen(str);

	(void) strcpy(str, clnt_sperrno(e.re_status));
	str += strlen(str);

	switch (e.re_status) {
	case RPC_SUCCESS:
	case RPC_CANTENCODEARGS:
	case RPC_CANTDECODERES:
	case RPC_TIMEDOUT:
	case RPC_PROGUNAVAIL:
	case RPC_PROCUNAVAIL:
	case RPC_CANTDECODEARGS:
	case RPC_SYSTEMERROR:
	case RPC_UNKNOWNHOST:
	case RPC_UNKNOWNPROTO:
	case RPC_UNKNOWNADDR:
	case RPC_NOBROADCAST:
	case RPC_RPCBFAILURE:
	case RPC_PROGNOTREGISTERED:
	case RPC_FAILED:
		break;

	case RPC_N2AXLATEFAILURE:
		(void) sprintf(str, "; %s", netdir_sperror());
		str += strlen(str);
		break;

	case RPC_TLIERROR:
		(void) sprintf(str, "; %s", t_strerror(e.re_terrno));
		str += strlen(str);
		if (e.re_errno) {
			(void) sprintf(str, "; %s", strerror(e.re_errno));
			str += strlen(str);
		}
		break;

	case RPC_CANTSEND:
	case RPC_CANTRECV:
		if (e.re_errno) {
			(void) sprintf(str, "; %s",
					strerror(e.re_errno));
			str += strlen(str);
		}
		if (e.re_terrno) {
			(void) sprintf(str, "; %s", t_strerror(e.re_terrno));
			str += strlen(str);
		}
		break;

	case RPC_VERSMISMATCH:
		(void) sprintf(str,
		    gettxt("uxnsl:55",
			"; low version = %lu, high version = %lu"),
		    e.re_vers.low, e.re_vers.high);
		str += strlen(str);
		break;

	case RPC_AUTHERROR:
		err = auth_errmsg(e.re_why);
		(void) sprintf(str, "; why = ");
		str += strlen(str);
		if (err != NULL) {
			(void) sprintf(str, "%s", err);
		} else {
			(void) sprintf(str,
			    gettxt("uxnsl:56",
				"(unknown authentication error - %d)"),
			    (int) e.re_why);
		}
		str += strlen(str);
		break;

	case RPC_PROGVERSMISMATCH:
		(void) sprintf(str,
		    gettxt("uxnsl:55",
			"; low version = %lu, high version = %lu"),
		    e.re_vers.low, e.re_vers.high);
		str += strlen(str);
		break;

	default:	/* unknown */
		(void) sprintf(str, "; s1 = %lu, s2 = %lu",
				e.re_lb.s1, e.re_lb.s2);
		str += strlen(str);
		break;
	}
	trace2(TR_clnt_sperror, 1, cl);
	return (buf);
}

void
clnt_perror(cl, s)
	CLIENT *cl;
	char *s;
{
	char *errmsg;

	trace2(TR_clnt_perror, 0, cl);
	if ((errmsg = clnt_sperror(cl,s)) != NULL)
		(void) fprintf(stderr, "%s\n", errmsg);
	trace2(TR_clnt_perror, 1, cl);
}

void
clnt_perrno(num)
	enum clnt_stat num;
{
	trace1(TR_clnt_perrno, 0);
	(void) fprintf(stderr, "%s\n", clnt_sperrno(num));
	trace1(TR_clnt_perrno, 1);
}

/*
 * Why a client handle could not be created
 */
char *
clnt_spcreateerror(s)
	char *s;
{
	extern int _sys_num_err;
	char *str;
	rpc_createerr_t rpc_createerr = get_rpc_createerr();

	trace1(TR_clnt_spcreateerror, 0);
	str = __buf();
	if (str == NULL) {
		trace1(TR_clnt_spcreateerror, 1);
		return (NULL);
	}
	(void) sprintf(str, "%s: ", s);
	(void) strcat(str, clnt_sperrno(rpc_createerr.cf_stat));

	switch (rpc_createerr.cf_stat) {
	case RPC_N2AXLATEFAILURE:
		(void) strcat(str, " - ");
		(void) strcat(str, netdir_sperror());
		break;

	case RPC_RPCBFAILURE:
		(void) strcat(str, " - ");
		(void) strcat(str,
			clnt_sperrno(rpc_createerr.cf_error.re_status));
		break;

	case RPC_SYSTEMERROR:
		(void) strcat(str, " - ");
		if ((rpc_createerr.cf_error.re_errno > 0) &&
			(rpc_createerr.cf_error.re_errno < _sys_num_err))
			(void) strcat(str,
			    strerror(rpc_createerr.cf_error.re_errno));
		else
			(void) sprintf(&str[strlen(str)],
			    gettxt("uxnsl:57", "System Error %d"),
			    rpc_createerr.cf_error.re_errno);
		break;

	case RPC_TLIERROR:
		(void) strcat(str, " - ");
		if ((rpc_createerr.cf_error.re_terrno > 0) &&
			(rpc_createerr.cf_error.re_terrno < t_nerr)) {
			(void) strcat(str,
				t_strerror(rpc_createerr.cf_error.re_terrno));
			if (rpc_createerr.cf_error.re_terrno == TSYSERR) {
				char *err;
				err = strerror(rpc_createerr.cf_error.re_errno);
				if (err) {
					strcat(str, " (");
					strcat(str, err);
					strcat(str, ")");
				}
			}
		}
		else
			(void) sprintf(&str[strlen(str)],
				gettxt("uxnsl:58", "TLI/XTI Error %d"),
				rpc_createerr.cf_error.re_terrno);
		if (rpc_createerr.cf_error.re_errno > 0) {
			if (rpc_createerr.cf_error.re_errno < _sys_num_err)
				(void) strcat(str,
			    strerror(rpc_createerr.cf_error.re_errno));
			else
				(void) sprintf(&str[strlen(str)],
				    gettxt("uxnsl:57", "System Error %d"),
				    rpc_createerr.cf_error.re_terrno);
		}
		break;

	case RPC_AUTHERROR:
		(void) strcat(str, " - ");
		(void) strcat(str,
			auth_errmsg(rpc_createerr.cf_error.re_why));
		break;
	}
	trace1(TR_clnt_spcreateerror, 1);
	return (str);
}

void
clnt_pcreateerror(s)
	char *s;
{
	char *errmsg;

	trace1(TR_clnt_pcreateerror, 0);
	if ((errmsg = clnt_spcreateerror(s)) != NULL)
		(void) fprintf(stderr, "%s\n", errmsg);
	trace1(TR_clnt_pcreateerror, 1);
}

/*
 * This interface for use by rpc_call() and rpc_broadcast()
 */

char *
clnt_sperrno(stat)
	enum clnt_stat stat;
{
	trace1(TR_clnt_sperrno, 0);
	switch (stat) {
	case RPC_SUCCESS:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:59", "RPC: Success"));
	case RPC_CANTENCODEARGS:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:60", "RPC: Can't encode arguments"));
	case RPC_CANTDECODERES:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:61", "RPC: Can't decode result"));
	case RPC_CANTSEND:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:62", "RPC: Unable to send"));
	case RPC_CANTRECV:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:63", "RPC: Unable to receive"));
	case RPC_TIMEDOUT:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:64", "RPC: Timed out"));
	case RPC_VERSMISMATCH:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:65",
		    "RPC: Incompatible versions of RPC"));
	case RPC_AUTHERROR:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:66", "RPC: Authentication error"));
	case RPC_PROGUNAVAIL:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:67", "RPC: Program unavailable"));
	case RPC_PROGVERSMISMATCH:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:68", "RPC: Program/version mismatch"));
	case RPC_PROCUNAVAIL:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:69", "RPC: Procedure unavailable"));
	case RPC_CANTDECODEARGS:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:70",
		    "RPC: Server can't decode arguments"));
	case RPC_SYSTEMERROR:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:71", "RPC: Remote system error"));
	case RPC_UNKNOWNHOST:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:72", "RPC: Unknown host"));
	case RPC_UNKNOWNPROTO:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:73", "RPC: Unknown protocol"));
	case RPC_RPCBFAILURE:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:74", "RPC: Rpcbind failure"));
	case RPC_N2AXLATEFAILURE:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:75",
		    "RPC: Name to address translation failed"));
	case RPC_NOBROADCAST:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:76", "RPC: Broadcast not supported"));
	case RPC_PROGNOTREGISTERED:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:77", "RPC: Program not registered"));
	case RPC_UNKNOWNADDR:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:78",
		    "RPC: Remote server address unknown"));
	case RPC_TLIERROR:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:79", "RPC: Miscellaneous tli/xti error"));
	case RPC_FAILED:
		trace1(TR_clnt_sperrno, 1);
		return (gettxt("uxnsl:80", "RPC: Failed (unspecified error)"));
	}
	trace1(TR_clnt_sperrno, 1);
	return (gettxt("uxnsl:81", "RPC: (unknown error code)"));
}
