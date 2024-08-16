/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/snmpio.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1989, 1990 INTERACTIVE Systems Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef lint
static char SNMPID[] = "@(#)snmpio.c	6.3 INTERACTIVE SNMP source";
#endif /* lint */

/*
 * Copyright 1987, 1988, 1989 Jeffrey D. Case and Kenneth W. Key (SNMP Research)
 */

/*
 *   snmpio.c : contains routines which are commonly called by SNMP
 *		applications for network input and output.
 */

#include "paths.h"
#include <snmp/snmpio.h>

/* initialize_io does initialization and opens udp connection */
void
initialize_io(program_name, name)
	char *program_name;
	char *name;
{

#ifdef SYSV
	struct servent *SimpleServ;
#endif

	/*  first, copy program name to the save area for use with error messages */
	strncpy(imagename, program_name, Min(sizeof (imagename) - 1,
					     strlen(program_name)));
	/* make sure terminated properly in case it was long */
	imagename[sizeof (imagename) - 1] = '\0';

#ifdef BSD
	/*  set up timer for timeout */
	(void)signal(SIGALRM, time_out);

	/*  now, set up UDP connection */
	if ((SimpleServ = getservbyname("snmp", "udp")) == NULL) {
		LIB_ERROR1("%s:  add    snmp    161/udp  to /etc/services.\n",
			   imagename);
		exit(-1);
	}
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		LIB_ERROR1("%s:  unable to connect to socket.\n", imagename);
		exit(-1);
	}
	sin.sin_addr.s_addr = inet_addr(name);
	if (sin.sin_addr.s_addr == -1) {
		hp = gethostbyname(name);
		if (hp)
			bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);
		else {
			LIB_ERROR1("%s:  host unknown.\n", name);
			exit(-1);
		}
	}
	sin.sin_family = AF_INET;
	sin.sin_port = SimpleServ->s_port;
#endif

#ifdef SYSV
	/*  set up timer for timeout */
	(void)sigset(SIGALRM, time_out);

	/*  now, set up UDP connection */
	if ((SimpleServ = getservbyname("snmp", "udp")) == NULL) {
		LIB_ERROR1("%s:  add    snmp    161/udp  to /etc/services.\n",
			   imagename);
		exit(-1);
	}
	if ((fd = t_open(_PATH_UDP, O_RDWR, (struct t_info *)0)) < 0) {
		LIB_ERROR1("%s:  unable to connect to transport endpoint.\n",
			   imagename);
		exit(-1);
	}
	if (t_bind(fd, (struct t_bind *)0, (struct t_bind *)0) < 0) {
		LIB_ERROR1("%s:  unable to bind transport endpoint.", imagename);
		exit(-1);
	}
	sin.sin_addr.s_addr = inet_addr(name);
	if (sin.sin_addr.s_addr == -1) {
		hp = gethostbyname(name);
		if (hp)
			memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
		else {
			LIB_ERROR1("%s:  host unknown.\n", name);
			exit(-1);
		}
	}
	sin.sin_family = htons(AF_INET);
	sin.sin_port = SimpleServ->s_port;
#endif
}

int
send_request(socket, auth_pointer)
	int socket;
	AuthHeader *auth_pointer;
{
	long packet_len;
	unsigned char out_pkt[2048];

#ifdef BSD

	packet_len = auth_pointer->packlet->length;
	bcopy(auth_pointer->packlet->octet_ptr, out_pkt, packet_len);

	/* for debug
	 * print_packet_out(auth_pointer->packlet->octet_ptr, packet_len);
	 */

	if (sendto(socket, out_pkt, packet_len, 0, &sin, sizeof (sin)) < 0) {
		LIB_ERROR1("%s:  send.\n", imagename);
		return (FALSE);
	}
	_snmpstat.outpkts++;
	return (TRUE);
#endif

#ifdef SYSV
	struct t_unitdata unitdata;

	packet_len = auth_pointer->packlet->length;
	memcpy(out_pkt, auth_pointer->packlet->octet_ptr, packet_len);

	unitdata.addr.buf = (char *)&sin;
	unitdata.addr.len = sizeof (sin);
	unitdata.opt.len = 0;
	unitdata.udata.buf = (char *)out_pkt;
	unitdata.udata.len = packet_len;

	/* for debug
	 * print_packet_out(auth_pointer->packlet->octet_ptr, packet_len);
	 */

	if (t_sndudata(socket, &unitdata) < 0) {
		LIB_ERROR1("%s:  send.\n", imagename);
		return (FALSE);
	}
	_snmpstat.outpkts++;
	return (TRUE);
#endif
}

int
get_response()
{
#ifdef BSD
	int fromlen = sizeof (from);
#endif
#ifdef SYSV
	struct t_unitdata unitdata;
	int flags;
	extern int t_errno;
#endif

	received_state = WAITING;
#ifdef BSD
	alarm(seconds);
	packet_len = (long)recvfrom(fd, packet, sizeof (packet), 0, &from, &fromlen);
	alarm(0);
	if (packet_len <= 0)
		return (TIMEOUT);
	_snmpstat.inpkts++;
	return (RECEIVED);
#endif

#ifdef SYSV
	unitdata.addr.buf = (char *)&from;
	unitdata.addr.maxlen = sizeof (from);
	unitdata.opt.maxlen = 0;
	unitdata.udata.buf = (char *)packet;
	unitdata.udata.maxlen = sizeof (packet);

	alarm(seconds);
	packet_len = t_rcvudata(fd, &unitdata, &flags);
	alarm(0);
	if (packet_len < 0) {
		return (TIMEOUT);
	}
	packet_len = unitdata.udata.len;
	_snmpstat.inpkts++;
	return (RECEIVED);
#endif
}

void
close_up()
{
#ifdef BSD
	close(fd);
#endif

#ifdef SYSV
	t_close(fd);
#endif
}

/*
 *  Handle timing out
 */
#ifdef SYSV
void
#else
int
#endif
time_out()
{
	received_state = TIMEOUT;
#ifndef SYSV
	return (0);
#endif
}

long
make_req_id()
{
	long ltime;

	time(&ltime);
	return (long)(ltime & 0x7fff);
}
