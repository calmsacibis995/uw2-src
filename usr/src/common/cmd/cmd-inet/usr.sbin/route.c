/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/route.c	1.3.10.4"
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
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
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
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)route.c	5.10 (Berkeley)	9/19/88
 */

/*
 * Portions of this source code were provided by UniSoft
 * under a development agreement with AT&T and Motorola.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <sys/stream.h>
#include <net/route.h>
#include <sys/ksym.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <stropts.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>

#define bcopy(f,t,l)	memcpy(t,f,l)

struct rtentry  route;
int             s;
int             forcehost, forcenet, doflush, nflag=0;
struct sockaddr_in sin = {AF_INET};
struct in_addr  inet_makeaddr();
char           *malloc();

main(argc, argv)
	int             argc;
	char           *argv[];
{
	int c;
	extern int optind;

	setlocale(LC_ALL,"");
	setcat("uxroute");
	s = socket(AF_INET, SOCK_RAW, 0);
	if (s < 0) {
		perror("route: socket");
		exit(1);
	}

	while ((c = getopt(argc, argv, "nf")) != -1) {
		switch (c) {
		case 'f':
			doflush++;
			break;
		case 'n':
			nflag++;
			break;
		default:
			goto usage;
		}
	}

	if (doflush)
		flushroutes();
	if (optind < argc) {
		if (strcmp(argv[optind], "add") == 0)
			newroute(argc - optind, &argv[optind]);
		else if (strcmp(argv[optind], "delete") == 0)
			newroute(argc - optind, &argv[optind]);
		else if (strcmp(argv[optind], "change") == 0)
			changeroute(argc - optind - 1, argv[optind+1]);
		else {
			goto usage;
		}
	}

	exit(0);
usage:
	pfmt(stdout,MM_ACTION,":1:usage: route [ -n ] [ -f ] [ %s destination gateway metric ]\n","add");
	pfmt(stdout,MM_NOSTD, ":2:      route [ -n ] [ -f ] [ %s destination gateway ]\n","delete");
	exit(1);
}

/*
 * Purge all entries in the routing tables not associated with network
 * interfaces. 
 */

#ifndef vax
#define RTHOST	"rthost"
#define RTNET	"rtnet"
#define RTHASHSIZE	"rthashsize"
#else
#define RTHOST	"_rthost"
#define RTNET	"_rtnet"
#define RTHASHSIZE	"_rthashsize"
#endif

					/* Current routing support only
					 * allows for internet addresses
					 * in the routing table.  When 
struct sockaddr_in sin = { AF_INET };	 * this is corrected, code which
					 * uses this variable should be
					 * changed.
					 */
flushroutes()
{
	struct rtentry  rt;

	if (rtioctl(s, SIOCFLUSHRT, &rt) < 0) {
		perror(gettxt("uxroute:3", "flushroutes failed"));
		exit(1);
	} else {
		pfmt(stdout,MM_INFO, ":4:flushroutes done\n");
		return;
	}
}

char           *
routename(sa)
	struct sockaddr *sa;
{
	register char  *cp;
	static char     line[50];
	struct hostent *hp;
	static char     domain[MAXHOSTNAMELEN + 1];
	static int      first = 1;
	char           *strchr();

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = strchr(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	switch (sa->sa_family) {

	case AF_INET:
		{
			struct in_addr  in;
			in = ((struct sockaddr_in *) sa)->sin_addr;

			cp = 0;
			if (in.s_addr == INADDR_ANY)
				cp = "default";
			if (cp == 0 && !nflag) {
				hp = gethostbyaddr(&in, sizeof(struct in_addr),
						   AF_INET);
				if (hp) {
					if ((cp = strchr(hp->h_name, '.')) &&
					    !strcmp(cp + 1, domain))
						*cp = 0;
					cp = hp->h_name;
				}
			}
			if (cp)
				strcpy(line, cp);
			else {
#define C(x)	((x) & 0xff)
				in.s_addr = ntohl(in.s_addr);
				(void)sprintf(line, "%u.%u.%u.%u",
					C(in.s_addr >> 24), C(in.s_addr >> 16),
					C(in.s_addr >> 8), C(in.s_addr));
			}
			break;
		}

	default:
		{
			u_short        *s = (u_short *) sa->sa_data;

			(void)sprintf(line, "af %d: %x %x %x %x %x %x %x",
				sa->sa_family, s[0], s[1], s[2], s[3], s[4],
				s[5], s[6]);
			break;
		}
	}
	return (line);
}

/*
 * Return the name of the network whose address is given. The address is
 * assumed to be that of a net or subnet, not a host. 
 */
char           *
netname(sa)
	struct sockaddr *sa;
{
	char           *cp = 0;
	static char     line[50];
	struct netent  *np = 0;
	u_long          net, mask;
	register u_long i;
	int             subnetshift;

	switch (sa->sa_family) {

	case AF_INET:
		{
			struct in_addr  in;
			in = ((struct sockaddr_in *) sa)->sin_addr;

			i = in.s_addr = ntohl(in.s_addr);
			if (in.s_addr == 0)
				cp = "default";
			else if (!nflag) {
				if (IN_CLASSA(i)) {
					mask = IN_CLASSA_NET;
					subnetshift = 8;
				} else if (IN_CLASSB(i)) {
					mask = IN_CLASSB_NET;
					subnetshift = 8;
				} else {
					mask = IN_CLASSC_NET;
					subnetshift = 4;
				}
				/*
				 * If there are more bits than the standard
				 * mask would suggest, subnets must be in
				 * use. Guess at the subnet mask, assuming
				 * reasonable width subnet fields. 
				 */
				while (in.s_addr & ~mask)
					mask = (long) mask >> subnetshift;
				net = in.s_addr & mask;
				while ((mask & 1) == 0)
					mask >>= 1, net >>= 1;
				np = getnetbyaddr(net, AF_INET);
				if (np)
					cp = np->n_name;
			}
			if (cp)
				strcpy(line, cp);
			else if ((in.s_addr & 0xffffff) == 0)
				(void)sprintf(line, "%u", C(in.s_addr >> 24));
			else if ((in.s_addr & 0xffff) == 0)
				(void)sprintf(line, "%u.%u",
					C(in.s_addr >> 24), C(in.s_addr >> 16));
			else if ((in.s_addr & 0xff) == 0)
				(void)sprintf(line, "%u.%u.%u",
					C(in.s_addr >> 24), C(in.s_addr >> 16),
					C(in.s_addr >> 8));
			else
				(void)sprintf(line, "%u.%u.%u.%u",
					C(in.s_addr >> 24), C(in.s_addr >> 16),
					C(in.s_addr >> 8), C(in.s_addr));
			break;
		}

	default:
		{
			u_short        *s = (u_short *) sa->sa_data;

			(void) sprintf(line, "af %d: %x %x %x %x %x %x %x",
				sa->sa_family, s[0], s[1], s[2], s[3], s[4],
				s[5], s[6]);
			break;
		}
	}
	return (line);
}

newroute(argc, argv)
	int             argc;
	char           *argv[];
{
	char           *cmd, *dest, *gateway;
	unsigned char  *message;
	int             ishost, metric = 0, ret, attempts, oerrno;
	struct hostent *hp;
	extern int      errno;

	cmd = argv[0];
	if ((strcmp(argv[1], "host")) == 0) {
		forcehost++;
		argc--, argv++;
	} else if ((strcmp(argv[1], "net")) == 0) {
		forcenet++;
		argc--, argv++;
	}
	if (*cmd == 'a') {
		if (argc != 4) {
			pfmt(stdout,MM_ACTION, ":5:usage: %s destination gateway metric\n", cmd);
			pfmt(stdout,MM_NOSTD, ":6:(metric of 0 if gateway is this host)\n");
			return;
		}
		metric = atoi(argv[3]);
	} else {
		if (argc < 3) {
			pfmt(stdout,MM_ACTION, ":7:usage: %s destination gateway\n", cmd);
			return;
		}
	}
	ishost = getaddr(argv[1], &(route.rt_dst), &hp, &dest, forcenet);
	if (forcehost)
		ishost = 1;
	if (forcenet)
		ishost = 0;
	(void) getaddr(argv[2], &(route.rt_gateway), &hp, &gateway, 0);
	route.rt_flags = RTF_UP;
	if (ishost)
		route.rt_flags |= RTF_HOST;
	if (metric > 0)
		route.rt_flags |= RTF_GATEWAY;
	route.rt_proto = RTP_LOCAL;
	for (attempts = 1;; attempts++) {
		errno = 0;
		if ((ret = ioctl(s, *cmd == 'a' ? SIOCADDRT : SIOCDELRT,
				 (caddr_t) & route)) == 0)
			break;
		if (errno != ENETUNREACH && errno != ESRCH)
			break;
		if (hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0], &route.rt_gateway,
			      hp->h_length);
		} else
			break;
	}
	oerrno = errno;
	if(*cmd == 'a') {
		if(ishost)
			message = gettxt("uxroute:8", "add host %s: gateway %s flags 0x%x");
		else
			message = gettxt("uxroute:9", "add net %s: gateway %s flags 0x%x");
	}
	else {
		if(ishost)
			message = gettxt("uxroute:10", "delete host %s: gateway %s flags 0x%x");
		else
			message = gettxt("uxroute:11", "delete net %s: gateway %s flags 0x%x");
	}
	printf(message, 
		strcmp(dest,"default") == 0 ? gettxt("uxroute:12", "\"default\"") : dest,
		strcmp(gateway,"default") == 0 ? gettxt("uxroute:12", "\"default\"") : gateway,
		route.rt_flags);
		
	if (attempts > 1 && ret == 0)
		printf(" (%s)", inet_ntoa(route.rt_gateway));
	if (ret == 0)
		printf("\n");
	else {
		printf(": ");
		fflush(stdout);
		errno = oerrno;
		error("");
	}
}

/*ARGSUSED*/
changeroute(argc, argv)
	int             argc;
	char           *argv[];
{
	pfmt(stdout,MM_ACTION, ":13:%s command not supported\n","change");
}

error(cmd)
	char           *cmd;
{
	extern int errno;

	switch (errno) {
	case ESRCH:
		pfmt(stderr,MM_ERROR,":14:not in table\n");
		break;
	case EBUSY:
		pfmt(stderr,MM_ERROR,":15:entry in use\n");
		break;
	case ENOBUFS:
		pfmt(stderr,MM_ERROR,":16:routing table overflow\n");
		break;
	default:
		perror(cmd);
	}
}

char           *
savestr(s)
	char           *s;
{
	char           *sav;

	sav = malloc(strlen(s) + 1);
	if (sav == NULL) {
		pfmt(stderr,MM_ERROR,":17:out of memory\n");
		exit(1);
	}
	strcpy(sav, s);
	return (sav);
}

/*
 * Interpret an argument as a network address of some kind, returning 1 if a
 * host address, 0 if a network address. 
 */
getaddr(s, sin, hpp, name, isnet)
	char           *s;
	struct sockaddr_in *sin;
	struct hostent **hpp;
	char          **name;
	int             isnet;
{
	struct hostent *hp;
	struct netent  *np;
	u_long          val;

	*hpp = 0;
	if (strcmp(s, "default") == 0) {
		sin->sin_family = AF_INET;
		sin->sin_addr = inet_makeaddr(0, INADDR_ANY);
		*name = "default";
		return (0);
	}
	sin->sin_family = AF_INET;
	if (isnet == 0) {
		val = inet_addr(s);
		if (val != -1) {
			sin->sin_addr.s_addr = val;
			*name = s;
			return (inet_lnaof(sin->sin_addr) != INADDR_ANY);
		}
	}
	val = inet_network(s);
	if (val != -1) {
		sin->sin_addr = inet_makeaddr(val, INADDR_ANY);
		*name = s;
		return (0);
	}
	np = getnetbyname(s);
	if (np) {
		sin->sin_family = np->n_addrtype;
		sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
		*name = savestr(np->n_name);
		return (0);
	}
	hp = gethostbyname(s);
	if (hp) {
		*hpp = hp;
		sin->sin_family = hp->h_addrtype;
		bcopy(hp->h_addr, &sin->sin_addr, hp->h_length);
		*name = savestr(hp->h_name);
		return (1);
	}
	pfmt(stderr,MM_ERROR,":18:%s: bad value\n", s);
	exit(1);
}

rtioctl(s, cmd, arg)
	int s;
	int cmd;
	char *arg;
{
#ifdef SYSV
	struct strioctl ioc;

	ioc.ic_cmd = cmd;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(struct rtentry);
	ioc.ic_dp = arg;
	return (ioctl(s, I_STR, (char *) &ioc));
#else
	return (ioctl(s, cmd, arg));
#endif SYSV
}
