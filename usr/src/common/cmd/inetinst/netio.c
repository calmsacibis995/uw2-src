/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:netio.c	1.4"

/*
 *  netio.c
 *
 *  This is the routines that manage the TLI connection to an
 *  install server.
 */

#include "inetinst.h"
#include <nl_types.h>
#include <tiuser.h>
#include <stropts.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netdb.h>
#include <netconfig.h>
#include <netdir.h>

/* 
 *  inetinst_connect()
 *  Establishes a TLI connection from a Network Install Client to a
 *  Network Install Server.
 *	INPUT:	host name and network type (ANY_NET, tcp, spx)
 *      OUTPUT: none
 *	ACTION: using network selection routines, determine address of 
 *		inetinst service on requested host on requested network
 */
int
inetinst_connect(char *host, char *network)
{
	int			fd;
	struct t_call		*sndcall;
	struct sockaddr_in	serv_addr;
	extern int		t_errno;
	void			*handlep;
	struct netconfig	*netconfigp, usenetconfig;
	struct	nd_hostserv	nd_hostserv;
	struct nd_addrlist	*nd_addrlistp;
	struct netbuf		*netbufp, usenetbuf;
	char			*service;
	int			i;
	int			netfound=0;
int ret;
	char logbuf[IBUF_SIZE];

	/*
	 *  Set up for Network Selection by specifying host and service
	 */
	nd_hostserv.h_host = host;
	nd_hostserv.h_serv = INETD_SERVICE;

	/*
	 *  Initialize the handle to check for the requested service
	 *  on the requested host via Netswork Selection routines.
	 */
	if ((handlep = setnetpath()) == NULL) {
		return(IERR_BADNET);
	}

	/*
	 *  Try to find the address of the requested service
	 */
	while ((netconfigp = getnetpath(handlep)) != NULL) {
		if(netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) !=0) {
			continue;
		}

		netbufp = nd_addrlistp->n_addrs;

		/*
		 *  Stash the netconfig and netbuf structs for later
		 */
		memcpy (&usenetconfig, netconfigp, sizeof(struct netconfig));
		memcpy (&usenetbuf, netbufp, sizeof(struct netbuf));
		
		/*
		 *  If no network was specified, the first one we find
		 *  will do.
		 */
		if ( !strcmp(network, ANY_NET) ) {
			sprintf(logbuf, catgets(mCat, MSG_SET, C_FOUND_NET, M_FOUND_NET), netconfigp->nc_netid);
			log(logbuf);
			netfound=1;
			break;
		}

		/*
		 *  If a network was specified, then we need to make sure
		 *  that the network we did find is the requested one.
		 */
		if ( !strcmp(network, netconfigp->nc_netid) ) {
			sprintf(logbuf, catgets(mCat, MSG_SET, C_FOUND_NET, M_FOUND_NET), netconfigp->nc_netid);
			log(logbuf);
			netfound=1;
			break;
		}
	}
	endnetpath(handlep);

	/*
	 *  If the user explicitly rewquested a particular network and
	 *  we couldn't find it, exit after logging the situation.
	if (strcmp(network,ANY_NET) && strcmp(network,netconfigp->nc_netid)) {
		sprintf(logbuf, catgets(mCat, MSG_SET, C_ERR_FOUND_NET, M_ERR_FOUND_NET), network);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}
	 */
	/*
	 *  If we didn't find the source host on our network, then
	 *  bail out here.
	 */
	if (netfound == 0) {
		sprintf(logbuf, catgets(mCat, MSG_SET, C_COND_BADHOST, M_COND_BADHOST));
		log(logbuf);
		clean_exit(IERR_BADHOST);
	}

	/*
	 *  Now open up the interface for the service we found
	 */
	if ((fd = t_open(usenetconfig.nc_device , O_RDWR, NULL)) < 0) {
		sprintf(logbuf,catgets(mCat, MSG_SET, C_ERR_TOPEN, M_ERR_TOPEN),
		  usenetconfig.nc_device, t_errno, errno);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}

	if (t_bind(fd, NULL, NULL) < 0) {
		sprintf(logbuf,catgets(mCat, MSG_SET, C_ERR_TBIND, M_ERR_TBIND),
		  netconfigp->nc_netid, t_errno);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}

	if ((sndcall = (struct t_call *)t_alloc(fd, T_CALL, T_ADDR))
		== NULL) {
		sprintf(logbuf,catgets(mCat, MSG_SET, C_ERR_TALLOC, M_ERR_TALLOC), "t_call", t_errno);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}

	/*
	 *  Using the info we got from the Network Selection routines,
	 *  populate address information for TLI.
	 */
	netbufp = nd_addrlistp->n_addrs;
	sndcall->addr.maxlen = netbufp->maxlen;
	sndcall->addr.len = netbufp->len;
	sndcall->addr.buf = netbufp->buf;

	if ((ret = t_connect(fd, sndcall, NULL)) < 0) {
		sprintf(logbuf,catgets(mCat, MSG_SET, C_ERR_TCONNECT, M_ERR_TCONNECT), t_errno);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}

	/*
	 *  Pop the current Stream header so we can push the Read/Write
	 *  interface onto the TLI Stream.
	 */
	if (ioctl(fd, I_POP, 0) < 0) {
		sprintf(logbuf,catgets(mCat, MSG_SET, C_ERR_IPOP, M_ERR_IPOP), errno);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}
	
	if (ioctl(fd, I_PUSH, "tirdwr") < 0) {
		sprintf(logbuf,catgets(mCat, MSG_SET, C_ERR_IPUSH, M_ERR_IPUSH), "tirdwr", errno);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}
	
	/*
	 *  Now make stdin and stdout use the TLI connection. 
	 *  We'll maintain the use of stderr to complain if we need to.
	 */
	if (dup2(fd,0) < 0) {
		sprintf(logbuf,catgets(mCat, MSG_SET, C_ERR_DUP2, M_ERR_DUP2), "stdin", errno);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}
	if (dup2(fd,1) < 0) {
		sprintf(logbuf,catgets(mCat, MSG_SET, C_ERR_DUP2, M_ERR_DUP2), "stdout", errno);
		log(logbuf);
		clean_exit(IERR_BADNET);
	}

	return(0);
}

/*
 *  Routine to do fputs() to network connection.
 *	INPUT	buffer for output, and (FILE *) for where it should go.
 *		(file ptr usually will be stdout)
 *	OUTPUT	buffer on (FILE *) stream.
 *	ACTION	fputs the buffer to the (FILE *) stream, and then
 *		fflush the (FILE *) stream.
 */
void
netputs(char *string, FILE *netfile)
{
	fputs(string, netfile);
	fflush(netfile);
}

/*
 *  Chat exchange on the net
 *	INPUT	string to send out, pointer to place to return
 *		string, verbose flag
 *	OUTPUT	string is sent on net
 *	ACTION	if verbose flag is set, all net traffic is logged.
 *		Log must have been opened with log_init() already.
 */
void
netsendrcv(char *send, int verbose)
{
	char logbuf[IBUF_SIZE];	

	netputs(send, stdout);
	if (verbose) {
		sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_SENT, M_LOG_SENT), send);
		log(logbuf);
	}
	if (fgets(send, IBUF_SIZE, stdin) == NULL) {
		sprintf(logbuf, catgets(mCat, MSG_SET, C_COND_BADNET, M_COND_BADNET));
		log(logbuf);
		clean_exit(IERR_BADNET);
	}
	if (verbose) {
		sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_RCVD, M_LOG_RCVD), send);
		log(logbuf);
	}
	return;
}
