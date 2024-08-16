/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/ti_opts.c	1.2.7.3"
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
 * ti_opts.c
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "trace.h"
#include <rpc/rpc.h>
#include <sys/ticlts.h>
#include <sys/ticots.h>
#include <sys/ticotsord.h>
#include <sys/syslog.h>

/*
 * This routine is typically called on the server side if the server
 * wants to know the caller uid.  Called typically by rpcbind.  It
 * depends upon the t_optmgmt to return the uid value.
 */
int
_rpc_negotiate_uid(fd)
	int fd;
{
	struct t_optmgmt options;
	struct t_info	tinfo;

	trace2(TR___rpc_negotiate_uid, 0, fd);
	if (t_getinfo(fd, &tinfo) == -1) {
		syslog(LOG_ERR,
		    "rpc_negotiate_uid (t_getinfo):  t_errno %d errno %d",
		    t_errno, errno);
		trace2(TR___rpc_negotiate_uid, 1, fd);
		return (-1);
	}
	switch ((int) tinfo.servtype) {
	case T_COTS: {
		struct {
			struct tco_opt_hdr	uid_hdr;
			struct tco_opt_setid	uid_opt;
		} send_in;

		send_in.uid_hdr.hdr_thisopt_off = sizeof (struct tco_opt_hdr);
		send_in.uid_hdr.hdr_nexthdr_off = TCO_OPT_NOHDR;
		send_in.uid_opt.setid_type = TCO_OPT_SETID;
		send_in.uid_opt.setid_flg = TCO_IDFLG_UID;
		options.opt.maxlen = sizeof (send_in);
		options.opt.len = sizeof (send_in);
		options.opt.buf = (char *) &send_in;
		send_in.uid_hdr.hdr_thisopt_off = sizeof (struct tco_opt_hdr);
		send_in.uid_hdr.hdr_nexthdr_off = TCO_OPT_NOHDR;
		send_in.uid_opt.setid_type = TCO_OPT_SETID;
		send_in.uid_opt.setid_flg = TCO_IDFLG_UID;
		options.opt.maxlen = sizeof (send_in);
		options.opt.len = sizeof (send_in);
		options.opt.buf = (char *) &send_in;
	}
		break;

	case T_COTS_ORD: {
		struct {
			struct tcoo_opt_hdr	uid_hdr;
			struct tcoo_opt_setid	uid_opt;
		} send_in;

		send_in.uid_hdr.hdr_thisopt_off = sizeof (struct tcoo_opt_hdr);
		send_in.uid_hdr.hdr_nexthdr_off = TCOO_OPT_NOHDR;
		send_in.uid_opt.setid_type = TCOO_OPT_SETID;
		send_in.uid_opt.setid_flg = TCOO_IDFLG_UID;
		options.opt.maxlen = sizeof (send_in);
		options.opt.len = sizeof (send_in);
		options.opt.buf = (char *) &send_in;
		send_in.uid_hdr.hdr_thisopt_off = sizeof (struct tcoo_opt_hdr);
		send_in.uid_hdr.hdr_nexthdr_off = TCOO_OPT_NOHDR;
		send_in.uid_opt.setid_type = TCOO_OPT_SETID;
		send_in.uid_opt.setid_flg = TCOO_IDFLG_UID;
		options.opt.maxlen = sizeof (send_in);
		options.opt.len = sizeof (send_in);
		options.opt.buf = (char *) &send_in;
	}
		break;

	case T_CLTS: {
		struct {
			struct tcl_opt_hdr	uid_hdr;
			struct tcl_opt_setid	uid_opt;
		} send_in;

		send_in.uid_hdr.hdr_thisopt_off = sizeof (struct tcl_opt_hdr);
		send_in.uid_hdr.hdr_nexthdr_off = TCL_OPT_NOHDR;
		send_in.uid_opt.setid_type = TCL_OPT_SETID;
		send_in.uid_opt.setid_flg = TCL_IDFLG_UID;
		options.opt.maxlen = sizeof (send_in);
		options.opt.len = sizeof (send_in);
		options.opt.buf = (char *) &send_in;
		send_in.uid_hdr.hdr_thisopt_off = sizeof (struct tcl_opt_hdr);
		send_in.uid_hdr.hdr_nexthdr_off = TCL_OPT_NOHDR;
		send_in.uid_opt.setid_type = TCL_OPT_SETID;
		send_in.uid_opt.setid_flg = TCL_IDFLG_UID;
		options.opt.maxlen = sizeof (send_in);
		options.opt.len = sizeof (send_in);
		options.opt.buf = (char *) &send_in;
	}
		break;
	}

	options.flags = T_NEGOTIATE;
	if (t_optmgmt(fd, &options, &options) == -1) {
		syslog(LOG_ERR,
		    "rpc_negotiate_uid (t_optmgmt): t_errno %d errno %d",
		    t_errno, errno);
		trace2(TR___rpc_negotiate_uid, 1, fd);
		return (-1);
	}

	trace2(TR___rpc_negotiate_uid, 1, fd);
	return (0);
}

/*
 * This returns the uid of the caller.  It assumes that the optbuf information
 * is stored at xprt->xp_p2.
 */
int
_rpc_get_local_uid(trans, uid_out)
	SVCXPRT *trans;
	uid_t *uid_out;
{
	struct netbuf *abuf;
	struct t_info tinfo;

	trace1(TR___rpc_get_local_uid, 0);
	abuf = (struct netbuf *) trans->xp_p2;
	if (abuf == (struct netbuf *) 0) {
#ifdef RPC_DEBUG
		syslog(LOG_ERR, "rpc_get_local_uid:  null xp_p2");
#endif
		trace1(TR___rpc_get_local_uid, 1);
		return (-1);
	}

	if (t_getinfo(trans->xp_fd, &tinfo) == -1) {
		syslog(LOG_ERR,
		    "rpc_negotiate_uid (t_getinfo): t_errno %d errno %d",
		    t_errno, errno);
		trace1(TR___rpc_get_local_uid, 1);
		return (-1);
	}

	switch ((int) tinfo.servtype) {
	case T_COTS: {
		struct myopts {
			struct tco_opt_hdr uid_hdr;
			struct tco_opt_uid uid_opt;
		}  *opt_out;

		if (abuf->len != sizeof (*opt_out)) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
			"rpc_get_local_uid (T_COTS): len %d is wrong, want %d",
				abuf->len, sizeof (*opt_out));
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		if (abuf->buf == 0) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
			"rpc_get_local_uid (T_COTS): null data");
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		opt_out = (struct myopts *) abuf->buf;
		if (opt_out->uid_opt.uid_type != TCO_OPT_UID) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
			"rpc_get_local_uid (T_COTS): type %d is wrong, want %d",
				opt_out->uid_opt.uid_type, TCO_OPT_UID);
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		*uid_out = opt_out->uid_opt.uid_val;
	}
		break;

	case T_COTS_ORD: {
		struct myopts {
			struct tcoo_opt_hdr uid_hdr;
			struct tcoo_opt_uid uid_opt;
		}  *opt_out;

		if (abuf->len != sizeof (*opt_out)) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
		"rpc_get_local_uid (T_COTS_ORD): len %d is wrong, want %d",
				abuf->len, sizeof (*opt_out));
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		if (abuf->buf == 0) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
			"rpc_get_local_uid (T_COTS_ORD): null data");
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		opt_out = (struct myopts *) abuf->buf;
		if (opt_out->uid_opt.uid_type != TCOO_OPT_UID) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
		"rpc_get_local_uid (T_COTS_ORD):  type %d is wrong, want %d",
				opt_out->uid_opt.uid_type, TCOO_OPT_UID);
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		*uid_out = opt_out->uid_opt.uid_val;
	}
		break;

	case T_CLTS: {
		struct myopts {
			struct tcl_opt_hdr uid_hdr;
			struct tcl_opt_uid uid_opt;
		}  *opt_out;

		if (abuf->len != sizeof (*opt_out)) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
			"rpc_get_local_uid (T_CLTS): len %d is wrong, want %d",
				abuf->len, sizeof (*opt_out));
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		if (abuf->buf == 0) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
				"rpc_get_local_uid (T_CLTS): null data");
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		opt_out = (struct myopts *) abuf->buf;
		if (opt_out->uid_opt.uid_type != TCL_OPT_UID) {
#ifdef RPC_DEBUG
			syslog(LOG_ERR,
			"rpc_get_local_uid (T_CLTS): type %d is wrong, want %d",
				opt_out->uid_opt.uid_type, TCL_OPT_UID);
#endif
			trace1(TR___rpc_get_local_uid, 1);
			return (-1);
		}
		*uid_out = opt_out->uid_opt.uid_val;
	}
		break;

	}

	trace1(TR___rpc_get_local_uid, 1);
	return (0);
}
