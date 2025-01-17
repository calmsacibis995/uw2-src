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

#ident	"@(#)nfs.cmds:automount/bindresvport.c	1.1.1.4"
#ident	"$Header: $"

#include <stdio.h>
#include <rpc/rpc.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <rpc/nettype.h>

#define STARTPORT 600
#define ENDPORT (IPPORT_RESERVED - 1)
#define NPORTS	(ENDPORT - STARTPORT + 1)

/*
 * The argument is a client handle for a UDP connection.
 * Unbind its transport endpoint from the existing port
 * and rebind it to a reserved port.
 */
bindresvport(cl)
	CLIENT *cl;
{
	int fd;
	int res;
	short port;
	struct sockaddr_in *sin;
	struct t_bind *tbind, *tres;
	int i;
	struct netconfig *nconf;
	extern int right_pid;

	/* make sure it's a UDP connection */
	nconf = getnetconfigent(cl->cl_netid);
	if (nconf == NULL)
		return (-1);
        if ((nconf->nc_semantics != NC_TPI_CLTS) ||
            strcmp(nconf->nc_protofmly, NC_INET) ||
            strcmp(nconf->nc_proto, NC_UDP)) {
		freenetconfigent(nconf);
		return (0);	/* not udp - don't need resv port */
	}
	freenetconfigent(nconf);

	if (!clnt_control(cl, CLGET_FD, (caddr_t)&fd)) {
		return (-1);
	}

	/* If fd is already bound - unbind it */
	if (t_getstate(fd) != T_UNBND) {
		if (t_unbind(fd) < 0) {
			return (-1);
		}
	}

	tbind = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
	if (tbind == NULL)
		return (-1);

	tres = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
	if (tres == NULL) {
		(void) t_free((char *) tbind, T_BIND);
		return (-1);
	}

	(void) memset((char *) tbind->addr.buf, 0, tbind->addr.len);
	/* warning: this sockaddr_in is truncated to 8 bytes */
	sin = (struct sockaddr_in *) tbind->addr.buf;
	sin->sin_family = AF_INET;

	tbind->qlen = 0;
	tbind->addr.len = tbind->addr.maxlen;

	/* Need to find a reserved port in the interval
	 * STARTPORT - ENDPORT.  Choose a random starting
	 * place in the interval based on the process pid
	 * and sequentially search the ports for one
	 * that is available.
	 */
	port = (right_pid % NPORTS) + STARTPORT;

	for (i = 0; i < NPORTS; i++) {
		sin->sin_port = htons(port++);
		if (port > ENDPORT)
			port = STARTPORT;
		res = t_bind(fd, tbind, tres);
		if (res == 0) {
			if (memcmp(tbind->addr.buf, tres->addr.buf,
				   (int) tres->addr.len) == 0)
				break;
			else {
				if (t_unbind(fd) < 0)
					break;
			}
		}
		else {
			if (get_t_errno() != TADDRBUSY)
				break;
		}
	}

	(void) t_free((char *) tbind, T_BIND);
	(void) t_free((char *) tres,  T_BIND);
	return (res);
}
