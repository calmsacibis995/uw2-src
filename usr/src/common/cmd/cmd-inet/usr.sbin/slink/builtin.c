/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/slink/builtin.c	1.3.7.5"
#ident  "$Header: $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

/*
 *
 *	Copyright 1987, 1988 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
#include <fcntl.h>
#include <sys/types.h>
#include <stropts.h>
#include <sys/stream.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <net/strioc.h>
/* #ifdef uts */
#include <sys/dlpi.h>
#include <sys/dlpi_ether.h>
/* #endif */
#include <stdio.h>
#include "defs.h"


extern int	pflag;
extern int	uflag;
struct val      val_none = {V_NONE};
static int	last_link_muxid = -1, last_link_fd = -1;

/*
 * num - convert string to integer.  Returns 1 if ok, 0 otherwise. Result is
 * stored in *res.
 */
int
num(str, res)
	char           *str;
	int            *res;
{
	int             val;
	char           *p;

	val = strtol(str, &p, 10);
	if (*p || p == str)
		return 0;
	else {
		*res = val;
		return 1;
	}
}

struct val     *
Open(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	static struct val rval = {V_FD};

	if ((rval.u.val = open(argv[0].u.sval, O_RDWR)) < 0)
		xerr(fi, c, E_SYS, "open \"%s\"", argv[0].u.sval);
	return &rval;
}

static int      open_argtypes[] = {V_STR};
static struct bfunc open_info = {
	Open, 1, 1, open_argtypes
};


ulong
conv_str_to_sap(char *sap_str, int *conv_error)
{
	int	sap;
	size_t	len;
	char	*resp;

	len = strlen(sap_str);
	sap = strtoul(sap_str, &resp, 0);

	if (((size_t)sap_str + len) == (size_t)resp)
		*conv_error = 0;
	else
		*conv_error = 1;

	return sap;
}

void
do_bind(struct finst *fi, struct cmd *c, int fd, char *sap1, char *sap2)
{
	struct strbuf	ctlbuf;
	dl_bind_req_t	*bind_reqp;
	dl_bind_ack_t	*bind_ackp;
	dl_subs_bind_req_t	*subs_bind_reqp;
	dl_subs_bind_ack_t	*subs_bind_ackp;
	dl_error_ack_t	*error_ackp;
	union DL_primitives	dl_prim;
	struct snap_sap	*snsp;
	ulong	tmp_sap;
	int	flags;
	int	conv_error;

	if (uflag)
		return;

	tmp_sap = conv_str_to_sap(sap1, &conv_error);
	if (conv_error != 0)
		xerr(fi, c, E_FSYS, "illegal SAP value");

	bind_reqp = (dl_bind_req_t *)&dl_prim;
	bind_reqp->dl_primitive = DL_BIND_REQ;
	bind_reqp->dl_sap = tmp_sap;
	bind_reqp->dl_max_conind = 0;
	bind_reqp->dl_service_mode = DL_CLDLS;
	bind_reqp->dl_conn_mgmt = 0;
	bind_reqp->dl_xidtest_flg = 0;
	ctlbuf.len = sizeof(dl_bind_req_t);
	ctlbuf.buf = (char *)bind_reqp;
	if (putmsg(fd, &ctlbuf, NULL, 0) < 0)
		xerr(fi, c, E_FSYS, "putmsg(DL_BIND_REQ)");

	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&dl_prim;
	flags = 0;
	if (getmsg(fd, &ctlbuf, NULL, &flags) < 0)
		xerr(fi, c, E_FSYS, "getmsg(DL_BIND_ACK)");

	switch (dl_prim.dl_primitive) {
	case DL_BIND_ACK:
		if (ctlbuf.len < sizeof(dl_bind_ack_t))
			xerr(fi, c, E_FSYS, "DL_BIND_ACK error");
		break;

	case DL_ERROR_ACK:
		if (ctlbuf.len < sizeof(dl_error_ack_t))
			xerr(fi, c, E_FSYS, "DL_ERROR_ACK/DL_BIND_ACK error");

		error_ackp = (dl_error_ack_t *)&dl_prim;
		switch (error_ackp->dl_errno) {
		case DL_SYSERR:
			xerr(fi, c, E_FSYS, "DL_SYSERR/DL_BIND_ACK (%d)",
				error_ackp->dl_unix_errno);

		default:
			xerr(fi, c, E_FSYS, "DL_ERROR_ACK/DL_BIND_ACK (%d)",
				error_ackp->dl_errno);
		}

	default:
		xerr(fi, c, E_FSYS, "expected DL_BIND_ACK got %d",
			dl_prim.dl_primitive);
	}

	if (sap2 == NULL)	/* no subs bind necessary */
		return;

	if ((subs_bind_reqp = (dl_subs_bind_req_t *)malloc(
			sizeof(dl_subs_bind_req_t) + sizeof(struct snap_sap)))
			== NULL)
		xerr(fi, c, E_FSYS, "DL_SUBS_BIND_REQ malloc");

	tmp_sap = conv_str_to_sap(sap2, &conv_error);
	if (conv_error != 0)
		xerr(fi, c, E_FSYS, "illegal subs SAP value");

	subs_bind_reqp->dl_primitive = DL_SUBS_BIND_REQ;
	subs_bind_reqp->dl_subs_sap_offset = sizeof(dl_subs_bind_req_t);
	subs_bind_reqp->dl_subs_sap_length = sizeof(struct snap_sap);
	subs_bind_reqp->dl_subs_bind_class = 0;
	snsp = (struct snap_sap *)((char *)subs_bind_reqp
		+ sizeof(dl_subs_bind_req_t));
	snsp->snap_global = 0;
	snsp->snap_local = tmp_sap;
	ctlbuf.len = sizeof(dl_subs_bind_req_t) + sizeof(struct snap_sap);
	ctlbuf.buf = (char *)subs_bind_reqp;

	if (putmsg(fd, &ctlbuf, NULL, RS_HIPRI) < 0)
		xerr(fi, c, E_FSYS, "putmsg(DL_SUBS_BIND_REQ)");

	free(subs_bind_reqp);

	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *)&dl_prim;
	flags = 0;
	if (getmsg(fd, &ctlbuf, NULL, &flags) < 0)
		xerr(fi, c, E_FSYS, "getmsg(DL_SUBS_BIND_ACK)");

	switch (dl_prim.dl_primitive) {
	case DL_SUBS_BIND_ACK:
		if (ctlbuf.len < sizeof(dl_subs_bind_ack_t))
			xerr(fi, c, E_FSYS, "DL_SUBS_BIND_ACK error");
		break;

	case DL_ERROR_ACK:
		if (ctlbuf.len < sizeof(dl_error_ack_t))
			xerr(fi, c, E_FSYS,
				"DL_ERROR_ACK/DL_SUBS_BIND_ACK error");

		error_ackp = (dl_error_ack_t *)&dl_prim;
		switch (error_ackp->dl_errno) {
		case DL_SYSERR:
			xerr(fi, c, E_FSYS,
				"DL_SYSERR/DL_SUBS_BIND_ACK error %d",
				error_ackp->dl_unix_errno);

		default:
			xerr(fi, c, E_FSYS,
				"DL_ERROR_ACK/DL_SUBS_BIND_ACK error %d",
				error_ackp->dl_errno);
		}

	default:
		xerr(fi, c, E_FSYS, "expected DL_SUBS_BIND_ACK got %d",
			dl_prim.dl_primitive);
	}

	return;
}

struct val     *
Bind(struct finst *fi, struct cmd *c, int argc, struct val *argv)
{

	if (argc == 3)
		do_bind(fi, c, argv[0].u.val, argv[1].u.sval, argv[2].u.sval);
	else	/* argc == 2 */
		do_bind(fi, c, argv[0].u.val, argv[1].u.sval, NULL);

	return &val_none;
}

static int      bind_argtypes[] = { V_FD, V_STR, V_STR };
static struct bfunc bind_info = {
	Bind, 2, 3, bind_argtypes
};

int
find_driver(int	fd, const char *driver_name)
{
	struct str_list	strl;
	int	module_cnt;
	int	cnt;
	int	rval = 0;

	if ((module_cnt = ioctl(fd, I_LIST, NULL)) < 0)
		return -1;

	if ((strl.sl_modlist = (struct str_mlist *)calloc((size_t)module_cnt,
			sizeof(struct str_mlist))) == NULL)
		return -2;

	strl.sl_nmods = module_cnt;
	if (ioctl(fd, I_LIST, &strl) < 0)
		return -3;

	for (cnt = 0; cnt < module_cnt; cnt++) {
		if (strcmp(driver_name, strl.sl_modlist[cnt].l_name) == 0) {
			rval = 1;
			break;
		}
	}

	free(strl.sl_modlist);
	return rval;
}

static char	ip_sap_str[] = "0x800";
static char	arp_sap_str[] = "0x806";

struct val     *
Link(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	static struct val rval = {V_MUXID};
	int	result;
	char	*sapstr = NULL;

	if (!uflag) {
		/*
		 * For compatibility with earlier versions of ip and arp
		 * which did their own DL_UNITDATA_REQs, we must determine
		 * if we are about to link this interface under arp or ip
		 * and "do the right thing".
		 */
		if ((result = find_driver(argv[0].u.val, "ip")) == 1)
			sapstr = ip_sap_str;
		else if (result == 0) {
			if ((result = find_driver(argv[0].u.val, "arp")) == 1)
				sapstr = arp_sap_str;
			else if (result < 0)
				xerr(fi, c, E_FSYS, "link/find of arp");
		} else
			xerr(fi, c, E_FSYS, "link/find of ip");
		if (sapstr != NULL)
			do_bind(fi, c, argv[1].u.val, sapstr, NULL);
		if ((rval.u.val = ioctl(argv[0].u.val,
				(pflag ? I_PLINK : I_LINK), argv[1].u.val)) < 0)
			xerr(fi, c, E_FSYS, "link");
		last_link_muxid = rval.u.val;
		last_link_fd    = argv[0].u.val;
	} else {
		/*
		 * unlink
		 * ignore return -- since the multiplexor id is lost,
		 * we can only do MUXID_ALL, and we may do it multiple
		 * times for one driver.
		 */
		(void) ioctl(argv[0].u.val, I_PUNLINK, MUXID_ALL);
		rval.u.val = 0;
	}
	close(argv[1].u.val);
	return &rval;
}

static int      link_argtypes[] = {V_FD, V_FD};
static struct bfunc link_info = {
	Link, 2, 2, link_argtypes
};

struct val     *
New_link(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	static struct val rval = {V_MUXID};
	int	result;
	ushort_t	sap_val = 0;

	if (!uflag) {
		if ((rval.u.val = ioctl(argv[0].u.val,
		(pflag ? I_PLINK : I_LINK), argv[1].u.val)) < 0)
			xerr(fi, c, E_FSYS, "new_link");
		last_link_muxid = rval.u.val;
		last_link_fd    = argv[0].u.val;
	} else {
		/*
		 * unlink
		 * ignore return -- since the multiplexor id is lost,
		 * we can only do MUXID_ALL, and we may do it multiple
		 * times for one driver.
		 */
		(void) ioctl(argv[0].u.val, I_PUNLINK, MUXID_ALL);
		rval.u.val = 0;
	}
	close(argv[1].u.val);
	return &rval;
}

static int      new_link_argtypes[] = {V_FD, V_FD};
static struct bfunc new_link_info = {
	New_link, 2, 2, new_link_argtypes
};

struct val     *
Push(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	if (uflag)
		return &val_none;
	if (ioctl(argv[0].u.val, I_PUSH, argv[1].u.sval) < 0)
		xerr(fi, c, E_SYS, "push \"%s\"", argv[1].u.sval);
	return &val_none;
}

static int      push_argtypes[] = {V_FD, V_STR};
static struct bfunc push_info = {
	Push, 2, 2, push_argtypes
};

struct val     *
Sifname(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strioctl iocb;
	struct ifreq    ifr;
	int    unlinkret;

	if (uflag)
		return &val_none;
	strcpy(ifr.ifr_name, argv[2].u.sval);
	ifr.ifr_metric = argv[1].u.val;
	iocb.ic_cmd = SIOCSIFNAME;
	iocb.ic_timout = 15;
	iocb.ic_len = sizeof(ifr);
	iocb.ic_dp = (char *) &ifr;
	if (ioctl(argv[0].u.val, I_STR, &iocb) < 0) {

		/* This failure case used to be more difficult to
		 * reach.  Now we avoid the case where the
		 * /etc/init.d/inetinit start script
		 *	(calls initialize -U)
		 * is run without an intervening /etc/init.d/inetinit
		 * stop, since this caused the following. 
		 * 
		 * The stack will return -1 for SIOCSIFNAME calls for
		 * defined interfaces that already have qbot provider
		 * streams defined, since is is by default bound to ip
		 * address 0.0.0.0.  Here we must unlink that partial
		 * provider, since we customarily link before the
		 * SIOCSIFNAME. 
		 */
		if ((last_link_fd    == argv[0].u.val) &&
		    (last_link_muxid != -1) &&
		    (last_link_fd    != -1)) {
			if (verbose) {
				/* rather than build slink interpreter data
				 * structures, we'll fake the printout
				 * here.  Must remain in sync with docmd()
				 * and docmd() and showval()
				 */
				fprintf(stderr,
					"corrective unlink <FD %d> <%sLINK %d>\n",
					argv[0].u.val,
					(pflag ? "P" : ""),
					last_link_muxid);
			}
			unlinkret = ioctl(argv[0].u.val,
				(pflag ? I_PUNLINK : I_UNLINK),
				last_link_muxid);
			if (verbose) {
				fprintf(stderr,
					"corrective unlink <%sLINK %d>\n",
					(pflag ? "P" : ""),
					unlinkret);
			}
		}

		xerr(fi, c, E_SYS, "sifname");
	}
	return &val_none;
}

static int      sifname_argtypes[] = {V_FD, V_MUXID, V_STR};
static struct bfunc sifname_info = {
	Sifname, 3, 3, sifname_argtypes
};

struct val     *
Unitsel(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strioctl iocb;
	int             unit;

	if (uflag)
		return &val_none;
	if (!(num(argv[1].u.sval, &unit)))
		xerr(fi, c, 0, "unitsel: bad unit number specification");
	iocb.ic_cmd = IF_UNITSEL;
	iocb.ic_timout = 0;
	iocb.ic_len = sizeof(int);
	iocb.ic_dp = (char *) &unit;
	if (ioctl(argv[0].u.val, I_STR, &iocb) < 0)
		xerr(fi, c, E_SYS, "unitsel");
	return &val_none;
}

static int      unitsel_argtypes[] = {V_FD, V_STR};
static struct bfunc unitsel_info = {
	Unitsel, 2, 2, unitsel_argtypes
};

struct val     *
Initqp(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	/* the order of these must agree with the IQP_XX defines */
	static char    *qname[] = {"rq", "wq", "hdrq", "muxrq", "muxwq"};
	static int      vtval[] = {IQP_LOWAT, IQP_HIWAT};
	static char    *vname[] = {"lowat", "hiwat"};
	struct iocqp    iocqp[IQP_NQTYPES * IQP_NVTYPES];
	int             niocqp;
	char           *dev;
	int             fd;
	struct strioctl iocb;
	int             i, qtype, vtype, val;

	if (uflag)
		return &val_none;
	dev = argv[0].u.sval;
	niocqp = 0;
	for (i = 1; i < argc;) {
		for (qtype = 0; qtype < IQP_NQTYPES; qtype++) {
			if (strcmp(argv[i].u.sval, qname[qtype]) == 0)
				break;
		}
		if (qtype == IQP_NQTYPES) {
			xerr(fi, c, 0, "initqp: bad queue type \"%s\"",
			     argv[i].u.sval);
		}
		i++;
		if (i + IQP_NVTYPES > argc) {
			xerr(fi, c, 0, "initqp: incomplete specification for %s\n",
			     qname[qtype]);
		}
		for (vtype = 0; vtype < IQP_NVTYPES; vtype++, i++) {
			if (num(argv[i].u.sval, &val)) {
				if (val < 0 || val > 65535) {
					xerr(fi, c, 0, "initqp: %s %s out of range",
					     qname[qtype], vname[vtype]);
				}
				iocqp[niocqp].iqp_type = qtype | vtval[vtype];
				iocqp[niocqp++].iqp_value = val;
			} else if (strcmp(argv[i].u.sval, "-")) {
				xerr(fi, c, 0, "initqp: illegal value for %s %s",
				     qname[qtype], vname[vtype]);
			}
		}
	}
	if ((fd = open(dev, O_RDWR)) < 0)
		xerr(fi, c, E_SYS, "initqp: open \"%s\"", dev);
	iocb.ic_cmd = INITQPARMS;
	iocb.ic_timout = 0;
	iocb.ic_len = niocqp * sizeof(struct iocqp);
	iocb.ic_dp = (char *) iocqp;
	if (ioctl(fd, I_STR, &iocb) < 0)
		xerr(fi, c, E_SYS, "initqp: ioctl INITQPARMS");
	close(fd);
	return &val_none;
}

static int      initqp_argtypes[] = {
	V_STR, V_STR, V_STR, V_STR, V_STR, V_STR, V_STR, V_STR,
	V_STR, V_STR, V_STR, V_STR, V_STR, V_STR, V_STR, V_STR
};
static struct bfunc initqp_info = {
	Initqp, 4, 16, initqp_argtypes
};

/* #ifdef uts */
struct val     *
Dlattach(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	struct strbuf   ctlbuf;
	dl_attach_req_t att_req;
	dl_error_ack_t *error_ack;
	union DL_primitives	dl_prim;
	int             flags = 0;
	int             fd, unit;

	if (uflag)
		return &val_none;
	if (!(num(argv[1].u.sval, &unit)))
		xerr(fi, c, 0, "unitsel: bad unit number specification");
	fd = argv[0].u.val;
	att_req.dl_primitive = DL_ATTACH_REQ;
	att_req.dl_ppa = unit;
	ctlbuf.len = sizeof(dl_attach_req_t);
	ctlbuf.buf = (char *) &att_req;
	if (putmsg(fd, &ctlbuf, NULL, 0) < 0)
		xerr(fi, c, E_SYS, "dlattach: putmsg");
	ctlbuf.maxlen = sizeof(union DL_primitives);
	ctlbuf.len = 0;
	ctlbuf.buf = (char *) &dl_prim;
	if (getmsg(fd, &ctlbuf, NULL, &flags) < 0)
		xerr(fi, c, E_SYS, "dlattach: getmsg");
	switch (dl_prim.dl_primitive) {
	case DL_OK_ACK:
		if (ctlbuf.len < sizeof(dl_ok_ack_t) ||
		    ((dl_ok_ack_t *) & dl_prim)->dl_correct_primitive
		    != DL_ATTACH_REQ)
			xerr(fi, c, 0, "dlattach: protocol error");
		else
			return &val_none;

	case DL_ERROR_ACK:
		if (ctlbuf.len < sizeof(dl_error_ack_t))
			xerr(fi, c, 0, "dlattach: protocol error");
		else {
			error_ack = (dl_error_ack_t *) & dl_prim;
			switch (error_ack->dl_errno) {
			case DL_BADPPA:
				xerr(fi, c, 0, "dlattach: bad PPA");

			case DL_ACCESS:
				xerr(fi, c, 0, "dlattach: access error");

			case DL_SYSERR:
				xerr(fi, c, 0, "dlattach: system error %d",
				     error_ack->dl_unix_errno);

			default:
				xerr(fi, c, 0, "dlattach: protocol error");
			}
		}

	default:
		xerr(fi, c, 0, "dlattach: protocol error");
	}
	/* NOTREACHED */
}

static int      dlattach_argtypes[] = {V_FD, V_STR};
static struct bfunc dlattach_info = {
	Dlattach, 2, 2, dlattach_argtypes
};
/* #endif */

struct val     *
Strcat(fi, c, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	int             argc;
	struct val     *argv;
{
	static struct val rval = {V_STR};
	int             len;
	char           *newstr;

	len = strlen(argv[0].u.sval) + strlen(argv[1].u.sval) + 1;
	newstr = xmalloc(len);
	strcpy(newstr, argv[0].u.sval);
	strcat(newstr, argv[1].u.sval);
	rval.u.sval = newstr;
	return &rval;
}

static int      strcat_argtypes[] = {V_STR, V_STR};
static struct bfunc strcat_info = {
	Strcat, 2, 2, strcat_argtypes
};

binit()
{
	deffunc("return", F_RETURN);
	deffunc("open", F_BUILTIN, &open_info);
	deffunc("bind", F_BUILTIN, &bind_info);
	deffunc("link", F_BUILTIN, &link_info);
	deffunc("new_link", F_BUILTIN, &new_link_info);
	deffunc("push", F_BUILTIN, &push_info);
	deffunc("sifname", F_BUILTIN, &sifname_info);
	deffunc("unitsel", F_BUILTIN, &unitsel_info);
/* #ifdef uts */
	deffunc("dlattach", F_BUILTIN, &dlattach_info);
/* #endif */
	deffunc("strcat", F_BUILTIN, &strcat_info);
	deffunc("initqp", F_BUILTIN, &initqp_info);
}
