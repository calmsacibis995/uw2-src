/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libxchoose:rcmd.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libxchoose/rcmd.c,v 1.2 1994/03/23 21:18:43 jodi Exp $"

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "rcmd.c	5.24 (Berkeley) 2/24/91";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include	"util.h"
extern char *gettxt();

rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;						/* host to connect to */
	u_short rport;						/* port number to connect to */
	const char *locuser, *remuser, *cmd;
	int *fd2p;
{
	int s, pid;
	long oldmask;
	char c;
	fd_set reads;
	int						i,lport;
	struct nd_hostserv 		nd_hostserv;
	struct nd_hostservlist	*nd_hostservlist;
	struct netconfig 		*netconfigp = NULL;
	struct nd_addrlist 		*nd_addrlistp = NULL;
	struct netbuf  			*netbufp = NULL;
	struct t_call			*callptr;
	struct servent 			*sp;
	int						hostNameFound;
	static void   			*handlep = NULL;

	if((handlep = setnetpath()) == NULL)
	{
		/*
		 * Print our error string and the reason why setnetpath
		 * failed.
		 */
		nc_perror(gettxt("libxchoosemsgs:4", "rcmd: Cannot rewind network selection path"));
		return(-1);
	}
	/*
	 * Find the service name
	 */
	sp = getservbyport(rport, NULL);
	if (sp == NULL) 
	{
		(void) fprintf(stderr, gettxt("libxchoosemsgs:5", "rcmd: unknown port.\n"));
		return (-1);
	}
	
	oldmask = sigblock(sigmask(SIGURG));
	/*
	 * --  Find the transport provider for this service
	 */
	nd_hostserv.h_host = *ahost;
	nd_hostserv.h_serv = sp->s_name;
	s = -1;
    
	while((s < 0 ) && (netconfigp = getnetpath(handlep)) != NULL )
	{
		if (netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0) 
		{
			hostNameFound = TRUE;
			netbufp = nd_addrlistp->n_addrs;
            for(i=0; i< nd_addrlistp->n_cnt; i++)
            {
				/*
	 			* Connect to the server on first address accepted
	 			*/
				if ((s=BindReservedAndConnect(netconfigp,netbufp)) <= 0 )
				{
					netbufp++;
					continue;
				}
				break;
			}
		}
	}

	(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
	/*
	 * 	Unable to connect.
	 */
	if ( s <= 0 )
	{
		sigsetmask(oldmask);
		/*
	 	* 	Unable to connect or host not found.
	 	*/
		if ( hostNameFound == TRUE )
			(void)fprintf(stderr,gettxt("libxchoosemsgs:6", "rcmd: Not able to connect to %s.\n"),*ahost);
		else
			(void)fprintf(stderr,gettxt("libxchoosemsgs:7", "rcmd: Host address unknown for %s.\n"),*ahost);
		if ( nd_addrlistp != NULL )
			(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
		if ( netconfigp != NULL )
			freenetconfigent(netconfigp);
		return(-1);
	}
	if (ioctl(s, I_POP, "timod") < 0) {
		fprintf(stderr, gettxt("libxchoosemsgs:8", "rcmd: Streams I_POP failed errno = %d.\n"),errno);
		freenetconfigent(netconfigp);
		t_close(s);
		return (-1);
	}
	if (ioctl(s, I_PUSH, "tirdwr") < 0) {
		fprintf(stderr, gettxt("libxchoosemsgs:9", "rcmd: Streams I_PUSH failed errno = %d.\n"),errno);
		freenetconfigent(netconfigp);
		t_close(s);
		return (-1);
	}
	pid = getpid();
	fcntl(s, F_SETOWN, pid);
	
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 ;
		/*
		 *	Bind to a reserved port for connection acceptance.
		 *	Fill in the lport variable so we can pass it to
		 *	the remote process.
		 */
		if ((s2=BindRSAndListen(netconfigp,&lport,*ahost)) <= 0 )
		{
			goto bad;
		}
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num)+1) != strlen(num)+1) {
			perror(gettxt("libxchoosemsgs:10", "rcmd: write: setting up stderr"));
			(void) close(s2);
			goto bad;
		}
		FD_ZERO(&reads);
		FD_SET(s, &reads);
		FD_SET(s2, &reads);
		errno = 0;
		/*
		 *	Wait for the connection request
		 *	or some error reply.
		 */
		if (select(32, &reads, 0, 0, 0) < 1 ||
		    !FD_ISSET(s2, &reads)) {
			if (errno != 0)
				perror(gettxt("libxchoosemsgs:11", "rcmd: select: setting up stderr"));
			else
			    fprintf(stderr,
				gettxt("libxchoosemsgs:12", "rcmd: select: protocol failure in circuit setup.\n"));
			(void) close(s2);
			goto bad;
		}
		/*
		 *	Accept a connection on this transport endpoint
		 */
		if ( AcceptConnection(s2,&callptr) < 0 )
		{
			fprintf(stderr,gettxt("libxchoosemsgs:13", "rcmd: AcceptConnection on stderr failed.\n"));
			lport = 0;
			goto bad;
		}
		/*
		 *	Check that the peer is on a reserverd port
		 */
if ( strcmp(netconfigp->nc_netid,"tcp") != 0 ) /* byte order problem */
											   /* MR to usl in process */
		if (netdir_options(netconfigp,ND_CHECK_RESERVEDPORT,
											0,(char *)&callptr->addr) != 0)
		{
			perror(gettxt("libxchoosemsgs:14", "rcmd: Peer not on reserved port"));
			goto bad;
		}
		if (ioctl(s2, I_POP, "timod") < 0) {
			perror(gettxt("libxchoosemsgs:15", "rcmd: failed to pop timod\n"));
			goto bad;
		}
		if (ioctl(s2, I_PUSH, "tirdwr") < 0) {
			perror(gettxt("libxchoosemsgs:16", "rcmd: failed to push tirdwr\n"));
			goto bad;
		}
		*fd2p = s2;
	}

	(void) write(s, locuser, strlen(locuser)+1);
	(void) write(s, remuser, strlen(remuser)+1);
	(void) write(s, cmd, strlen(cmd)+1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad2;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}
	freenetconfigent(netconfigp);
	sigsetmask(oldmask);
	return (s);
bad2:
	if (lport)
		(void) close(*fd2p);
bad:
	freenetconfigent(netconfigp);
	(void) close(s);
	sigsetmask(oldmask);
	return (-1);
}

