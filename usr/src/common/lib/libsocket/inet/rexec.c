/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/inet/rexec.c	1.2.8.7"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <sys/byteorder.h>

#define	bcopy(a,b,c)	memcpy((b),(a),(c))
char	*getpass(), *getlogin();

rexec(ahost, rport, name, pass, cmd, fd2p)
	char **ahost;
	int rport;
	char *name, *pass, *cmd;
	int *fd2p;
{
	int s, timo = 1, s3;
	struct sockaddr_in sin, sin2, from;
	char c;
	u_short port;
	struct hostent *hp;

	hp = gethostbyname(*ahost);
	if (hp == 0) {
		herror(*ahost);
		return (-1);
	}
	*ahost = hp->h_name;
	_ruserpass(hp->h_name, &name, &pass);
retry:
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("rexec: socket");
		return (-1);
	}
	sin.sin_family = hp->h_addrtype;
	sin.sin_port = htons(rport);
	bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		if (errno == ECONNREFUSED && timo <= 16) {
			(void) close(s);
			sleep(timo);
			timo *= 2;
			goto retry;
		}
		perror(hp->h_name);
		(void) close(s);
		return (-1);
	}
	if (fd2p == 0) {
		(void) write(s, "", 1);
		port = 0;
	} else {
		char num[8];
		int s2;
		size_t sin2len;
		
		s2 = socket(AF_INET, SOCK_STREAM, 0);
		if (s2 < 0) {
			(void) close(s);
			return (-1);
		}
		listen(s2, 1);
		sin2len = sizeof (sin2);
		if (getsockname(s2, (struct sockaddr *)&sin2, &sin2len) < 0 ||
		    sin2len != sizeof (sin2)) {
			perror("getsockname");
			(void) close(s2);
			goto bad;
		}
		port = ntohs((u_short)sin2.sin_port);
		(void) sprintf(num, "%u", port);
		(void) write(s, num, strlen(num)+1);
		{ size_t len = sizeof (from);
		  s3 = accept(s2, (struct sockaddr *)&from, &len);
		  close(s2);
		  if (s3 < 0) {
			perror("accept");
			port = 0;
			goto bad;
		  }
		}
		*fd2p = s3;
	}
	(void) write(s, name, strlen(name) + 1);
	/* should public key encypt the password here */
	(void) write(s, pass, strlen(pass) + 1);
	(void) write(s, cmd, strlen(cmd) + 1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad;
	}
	return (s);
bad:
	if (port)
		(void) close(*fd2p);
	(void) close(s);
	return (-1);
}
