/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rtime_tli.c	1.2.11.2"
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
 * rtime_tli.c 
 * get time from remote machine
 *
 * gets time, obtaining value from host
 * on the (udp, tcp)/time tli connection. Since timeserver returns
 * with time of day in seconds since Jan 1, 1900, must
 * subtract seconds before Jan 1, 1970 to get
 * what unix uses.
 */
#include <rpc/rpc.h>
#include "trace.h"
#include <errno.h>
#include <sys/poll.h>
#include <rpc/nettype.h>
#include <netdir.h>
#include <stdio.h>

#ifdef RPC_DEBUG
#define	debug(msg)	t_error(msg)
#else
#define	debug(msg)
#endif

#define	NYEARS	(1970 - 1900)
#define	TOFFSET ((u_long)60*60*24*(365*NYEARS + (NYEARS/4)))

extern int errno;

/*
 * This is based upon the internet time server, but it contacts it by
 * using TLI instead of socket.
 */
rtime_tli(host, timep, timeout)
	char *host;
	struct timeval *timep;
	struct timeval *timeout;
{
	unsigned long thetime;
	int flag;
	struct nd_addrlist *nlist = NULL;
	struct nd_hostserv rpcbind_hs;
	struct netconfig *nconf = NULL;
	int foundit = 0;
	int fd = -1;

	trace1(TR_rtime_tli, 0);
	nconf = _rpc_getconfip(timeout == NULL ? "tcp" : "udp");
	if (nconf == (struct netconfig *)NULL)
		goto error;

	if ((fd = t_open(nconf->nc_device, O_RDWR, NULL)) == -1) {
		debug("open");
		goto error;
	}
	if (t_bind(fd, (struct t_bind *) NULL, (struct t_bind *) NULL) < 0) {
		debug("bind");
		goto error;
	}

	/* Get the address of the rpcbind */
	rpcbind_hs.h_host = host;
	rpcbind_hs.h_serv = "time";
	/* Basically get the address of the remote machine on IP */
	if (netdir_getbyname(nconf, &rpcbind_hs, &nlist))
		goto error;

	if (nconf->nc_semantics == NC_TPI_CLTS) {
		struct t_unitdata tu_data;
		struct pollfd pfd;
		int res;
		int msec;

		tu_data.addr = *nlist->n_addrs;
		tu_data.udata.buf = (char *)&thetime;
		tu_data.udata.len = sizeof (thetime);
		tu_data.udata.maxlen = tu_data.udata.len;
		tu_data.opt.len = 0;
		tu_data.opt.maxlen = 0;
		if (t_sndudata(fd, &tu_data) == -1) {
			debug("udp");
			goto error;
		}
		pfd.fd = fd;
		pfd.events = POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND;

		msec = _rpc_timeval_to_msec(timeout);
		do {
			res = poll(&pfd, 1, msec);
		} while (res < 0);
		if ((res <= 0) || (pfd.revents & POLLNVAL))
			goto error;
		if (t_rcvudata(fd, &tu_data, &flag) < 0) {
			debug("udp");
			goto error;
		}
		foundit = 1;
	} else {
		struct t_call sndcall;

		sndcall.addr = *nlist->n_addrs;
		sndcall.opt.len = sndcall.opt.maxlen = 0;
		sndcall.udata.len = sndcall.udata.maxlen = 0;

		if (t_connect(fd, &sndcall, NULL) == -1) {
			debug("tcp");
			goto error;
		}
		if (t_rcv(fd, (char *)&thetime, sizeof (thetime), &flag)
				!= sizeof (thetime)) {
			debug("tcp");
			goto error;
		}
		foundit = 1;
	}

	thetime = ntohl(thetime);
	timep->tv_sec = thetime - TOFFSET;
	timep->tv_usec = 0;

error:
	if (nconf) {
		(void) freenetconfigent(nconf);
		if (fd != -1) {
			(void) t_close(fd);
			if (nlist)
				netdir_free((char *)nlist, ND_ADDRLIST);
		}
	}
	trace1(TR_rtime_tli, 1);
	return (foundit);
}
