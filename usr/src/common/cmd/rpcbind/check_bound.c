/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)rpcbind:check_bound.c	1.7.10.6"
#ident  "$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

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
 */
/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/*
 * check_bound.c
 * Checks to see whether the program is still bound to the
 * claimed address and returns the univeral merged address
 *
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <netconfig.h>
#include <netdir.h>
#include <sys/syslog.h>
#include <stdlib.h>
#include <xti.h>
#include <unistd.h>
#include "rpcbind.h"

struct fdlist {
	int fd;
	struct netconfig *nconf;
	struct fdlist *next;
	int check_binding;
};

static struct fdlist *fdhead;	/* Link list of the check fd's */
static struct fdlist *fdtail;
static char *nullstring = "";

/*
 * Returns 1 if the given address is bound for the given addr & transport
 * For all error cases, we assume that the address is bound
 * Returns 0 for success.
 */
static bool_t
check_bound(fdl, uaddr)
	struct fdlist *fdl;	/* My FD list */
	char *uaddr;		/* the universal address */
{
	int fd;
	struct netbuf *na;
	struct t_bind taddr, *baddr;
	int ans;

	if (fdl->check_binding == FALSE)
		return (TRUE);

	na = uaddr2taddr(fdl->nconf, uaddr);
	if (!na)
		return (TRUE); /* punt, should never happen */

	fd = fdl->fd;
	taddr.addr = *na;
	taddr.qlen = 1;
	baddr = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
	if (baddr == NULL) {
		netdir_free((char *)na, ND_ADDR);
		return (TRUE);
	}
	if (t_bind(fd, &taddr, baddr) != 0) {
		netdir_free((char *)na, ND_ADDR);
		(void) t_free((char *)baddr, T_BIND);
		return (TRUE);
	}
	ans = memcmp(taddr.addr.buf, baddr->addr.buf, baddr->addr.len);
	netdir_free((char *)na, ND_ADDR);
	(void) t_free((char *)baddr, T_BIND);
	if (t_unbind(fd) != 0) {
		/* Bad fd. Purge this fd */
		(void) t_close(fd);
		fdl->fd = t_open(fdl->nconf->nc_device, O_RDWR, NULL);
		if (fdl->fd == -1)
			fdl->check_binding = FALSE;
	}
	return (ans == 0 ? FALSE : TRUE);
}

/*
 * Keep open one more file descriptor for this transport, which
 * will be used to determine whether the given service is up
 * or not by trying to bind to the registered address.
 * We are ignoring errors here. It trashes taddr and baddr;
 * but that perhaps should not matter.
 *
 * We check for the following conditions:
 *	1. Is it possible for t_bind to fail in the case where
 *		we bind to an already bound address and have any
 *		other error number besides TNOADDR.
 *	2. If a address is specified in bind addr, can I bind to
 *		the same address.
 *	3. If NULL is specified in bind addr, can I bind to the
 *		address to which the fd finally got bound.
 */
int
add_bndlist(nconf, taddr, baddr)
        struct netconfig *nconf;
        struct t_bind *taddr, *baddr;
{
        struct fdlist *fdl;
        struct netconfig *newnconf;
        struct t_info tinfo;
        struct t_bind tmpaddr;
        int fd;

        newnconf = getnetconfigent(nconf->nc_netid);
        if (newnconf == NULL)
                return (-1);

        /*
         * get a new fdl and install it in the list
         */
        fdl = (struct fdlist *)
		malloc((u_int)sizeof (struct fdlist));
        if (fdl == NULL) {
                freenetconfigent(newnconf);
		(void)strcpy(syslogmsgp,
			     gettxt(":56", "No memory!"));
                syslog(LOG_ERR, syslogmsg);
                return (-1);
        }
        fdl->nconf = newnconf;
        fdl->next = NULL;
        if (fdhead == NULL) {
                fdhead = fdl;
                fdtail = fdl;
        } else {
                fdtail->next = fdl;
                fdtail = fdl;
        }
        fdl->check_binding = FALSE;

        /*
         * open the descriptor
         */
        if ((fdl->fd = t_open(nconf->nc_device, O_RDWR, &tinfo)) < 0) {
                /*
                 * Note that we have neither dequeued this entry 
		 * nor freed the netconfig structure.
                 */
		(void)strcpy(syslogmsgp, 
			  gettxt(":30", "Could not open connection on %s: %s"));
		syslog(LOG_ERR, syslogmsg, 
		       nconf->nc_netid, t_strerror(t_errno));
                return (-1);
        }

        /*
         * Set the qlen only for cots transports
         */
        switch (tinfo.servtype) {
                case T_COTS:
                case T_COTS_ORD:
                        taddr->qlen = 1;
                        break;
                case T_CLTS:
                        taddr->qlen = 0;
                        break;
                default:
                        goto error;
        }

        if (t_bind(fdl->fd, taddr, baddr) != 0) {
                if ((t_errno == TNOADDR)
		 || (t_errno == TADDRBUSY)) {
                        /*
                         * got expected error
                         */
                        fdl->check_binding = TRUE;
                        return (0);
                }
		(void)strcpy(syslogmsgp,
			     gettxt(":16", "Could not bind on %s: %s"));
		syslog(LOG_ERR, syslogmsg, 
		       nconf->nc_netid, t_strerror(t_errno));
                goto not_bound;
        }

        t_unbind(fdl->fd);

        /*
         * Set the qlen only for cots transports
         */
        switch (tinfo.servtype) {
                case T_COTS:
                case T_COTS_ORD:
                        tmpaddr.qlen = 1;
                        break;
                case T_CLTS:
                        tmpaddr.qlen = 0;
                        break;
                default:
                        goto error;
        }

        tmpaddr.addr.len = tmpaddr.addr.maxlen = 0;
        tmpaddr.addr.buf = NULL;
        if (t_bind(fdl->fd, &tmpaddr, taddr) != 0) {
		(void)strcpy(syslogmsgp,
			     gettxt(":16", "Could not bind on %s: %s"));
		syslog(LOG_ERR, syslogmsg,
		       nconf->nc_netid, t_strerror(t_errno));
                goto error;
	}
        /*
         * at this point fdl->fd is bound to a transport
	 * chosen address now open a new fd
         */
        if ((fd = t_open(nconf->nc_device, O_RDWR, &tinfo)) < 0) {
		(void)strcpy(syslogmsgp,
			  gettxt(":30", "Could not open connection on %s: %s"));
		syslog(LOG_ERR, syslogmsg, 
		       nconf->nc_netid, t_strerror(t_errno));
                goto error;
	}

        /*
         * now try to bind it to the same address as fdl->fd
	 * is bound to
         */
        if (t_bind(fd, taddr, baddr) != 0) {
                if (t_errno == TADDRBUSY) {
                        t_close(fd);
                        t_unbind(fdl->fd);
                        fdl->check_binding = TRUE;
                        return (0);
                }
		(void)strcpy(syslogmsgp,
			     gettxt(":16", "Could not bind on %s: %s"));
		syslog(LOG_ERR, syslogmsg, 
		       nconf->nc_netid, t_strerror(t_errno));
                goto not_bound;
        } else {
                t_close(fd);
                goto error;
        }

not_bound:
        t_close(fdl->fd);
        fdl->fd = -1;
        return (1);

error:
        t_close(fdl->fd);
        fdl->fd = -1;
        return (-1);
}

bool_t
is_bound(netid, uaddr)
	char *netid;
	char *uaddr;
{
	struct fdlist *fdl;

	for (fdl = fdhead; fdl; fdl = fdl->next)
		if (strcmp(fdl->nconf->nc_netid, netid) == 0)
			break;
	if (fdl == NULL)
		return (TRUE);
	return (check_bound(fdl, uaddr));
}

/*
 * Returns NULL if there was some system error.
 * Returns "" if the address was not bound, i.e the server crashed.
 * Returns the merged address otherwise.
 */
char *
mergeaddr(xprt, netid, uaddr, saddr)
	SVCXPRT *xprt;
	char *netid;
	char *uaddr;
	char *saddr;
{
	struct fdlist *fdl;
	struct nd_mergearg ma;
	int stat;

	for (fdl = fdhead; fdl; fdl = fdl->next)
		if (strcmp(fdl->nconf->nc_netid, netid) == 0)
			break;
	if (fdl == NULL)
		return (NULL);
	if (check_bound(fdl, uaddr) == FALSE)
		/* that server died */
		return (nullstring);
	/*
	 * If saddr is not NULL, the remote client may have included the
	 * address by which it contacted us.  Use that for the "client" uaddr,
	 * otherwise use the info from the SVCXPRT.
	 */
	if (saddr != NULL) {
		ma.c_uaddr = saddr;
	} else {
		ma.c_uaddr = taddr2uaddr(fdl->nconf, svc_getrpccaller(xprt));
		if (ma.c_uaddr == NULL) {
			(void)strcpy(syslogmsgp,
				     gettxt(":45", 
	  "Failed to convert network address to universal address for %s: %s"));
			syslog(LOG_ERR, syslogmsg, 
			       fdl->nconf->nc_netid, netdir_sperror());
			return (NULL);
		}
	}

#ifdef ND_DEBUG
	if (saddr == NULL) {
		fprintf(stderr, "mergeaddr: client uaddr = %s\n", ma.c_uaddr);
	} else {
		fprintf(stderr, "mergeaddr: contact uaddr = %s\n", ma.c_uaddr);
	}
#endif
	ma.s_uaddr = uaddr;
	stat = netdir_options(fdl->nconf, ND_MERGEADDR, 0, (char *)&ma);
	if (saddr == NULL) {
		free(ma.c_uaddr);
	}
	if (stat) {
		(void)strcpy(syslogmsgp,
			 gettxt(":47", "Failed to merge addresses for %s: %s"));
		syslog(LOG_ERR, syslogmsg,  
		       fdl->nconf->nc_netid, netdir_sperror());
		return (NULL);
	}
#ifdef ND_DEBUG
	fprintf(stderr, "mergeaddr: uaddr = %s, merged uaddr = %s\n",
				uaddr, ma.m_uaddr);
#endif
	return (ma.m_uaddr);
}

/*
 * Returns a netconf structure from its internal list.  This
 * structure should not be freed.
 */
struct netconfig *
rpcbind_get_conf(netid)
	char *netid;
{
	struct fdlist *fdl;

	for (fdl = fdhead; fdl; fdl = fdl->next)
		if (strcmp(fdl->nconf->nc_netid, netid) == 0)
			break;
	if (fdl == NULL)
		return (NULL);
	return (fdl->nconf);
}

#ifdef RPCBIND_DEBUG
syslog(a, msg, b, c, d)
	int a;
	char *msg;
	caddr_t b, c, d;
{
	char buf[1024];

	sprintf(buf, msg, b, c, d);
	fprintf(stderr, "Syslog: %s\n", buf);
}
#endif
