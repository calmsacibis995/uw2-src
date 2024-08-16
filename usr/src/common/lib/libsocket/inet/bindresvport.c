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

#ident	"@(#)libsocket:common/lib/libsocket/inet/bindresvport.c	1.2.8.10"
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

#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/byteorder.h>
#include <stdlib.h>
#include "../libsock_mt.h"

#define bzero(s, len)	memset(s, 0, len)

static int	set_s_port();
static short	get_s_port();

/*
 * Bind a socket to a privileged IP port
 */
bindresvport(sd, sin)
	int sd;
	struct sockaddr_in *sin;
{
	int res;
	short port;
	struct sockaddr_in myaddr;
	int i;

#define STARTPORT 600
#define ENDPORT (IPPORT_RESERVED - 1)
#define NPORTS	(ENDPORT - STARTPORT + 1)

	if (sin == (struct sockaddr_in *)0) {
		sin = &myaddr;
		bzero(sin, sizeof (*sin));
		sin->sin_family = AF_INET;
	} else if (sin->sin_family != AF_INET) {
		errno = EPFNOSUPPORT;
		return (-1);
	}
	port = get_s_port();
	res = -1;
	errno = EADDRINUSE;
	for (i = 0; i < NPORTS && res < 0 && errno == EADDRINUSE; i++) {
		sin->sin_port = htons(port++);
		if (port > ENDPORT) {
			port = STARTPORT;
		}
		res = bind(sd, (struct sockaddr *)sin,
			   sizeof(struct sockaddr_in));
	}
	set_s_port(port);
	return (res);
}

/*
 * Access functions to set and get the initial guess of the next
 * reserved port to which to bind.
 * These functions could use a mutex lock to guard a process-wide
 * guess or they could use thread-specific data to spread out the
 * guesses from the various threads of the process.
 * It seems most efficient for one thread at a time to attempt to
 * bind to a reserved port, rather than for a number of threads to
 * be making many of the same incorrect guesses. 
 * Therefore, we use a process-wide guess that one thread at a time
 * will take as a starting point.  When it succeeds, it will update
 * the process-wide guess and allow other threads to attempt to
 * bind.  We sequence the bind attempts of threads by using a mutex.
 */

static short guess;

static int
set_s_port(port)
	short port;
{
	guess = port;
	MUTEX_UNLOCK(&_s_port_lock);
	return (0);
}

static short
get_s_port()
{
	MUTEX_LOCK(&_s_port_lock);
	if (guess == 0)
		guess = (getpid() % NPORTS) + STARTPORT;
	return (guess);
}
