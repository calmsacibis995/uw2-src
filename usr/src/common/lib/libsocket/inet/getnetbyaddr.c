/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/inet/getnetbyaddr.c	1.1.9.6"
#ident  "$Header: $"

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

#include <netdb.h>
#include <ns.h>

extern int _get_s_net_stayopen();
extern struct netent *_getnetent();

struct netent *
_getnetbyaddr(net, type)
	register long	net;
	register int	type;
{
	register struct netent *p;
	int net_stayopen;

	net_stayopen = _get_s_net_stayopen();
	_setnetent(net_stayopen);
	while (p = _getnetent())
		if (p->n_addrtype == type && p->n_net == net)
			break;
	if (!net_stayopen)
		_endnetent();
	return (p);
}

struct netent *
getnetbyaddr(net, type)
	register long	net;
	register int	type;
{
	NS_GETNUM2(NETDB, net, type, struct netent *);

	return (_getnetbyaddr(net, type));
}
