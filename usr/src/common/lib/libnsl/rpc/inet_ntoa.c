/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/inet_ntoa.c	1.3.7.4"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
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
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * inet_ntoa.c
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */
#include <sys/types.h>
#include <rpc/rpc.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "trace.h"
#include "rpc_mt.h"

#define INETADDRSIZE 18

/*
 * The returned value of inet_ntoa() are maintained
 * by per-thread base. But a static buffer (b) is used
 * under single-threaded or !_REENTRANT.
 */
char *
inet_ntoa(in)
	struct in_addr in;
{
	static char b[INETADDRSIZE];
	char *buf;
	register char *p;
#ifdef _REENTRANT
        struct _rpc_tsd *key_tbl;
#endif /* _REENTRANT */

        trace1(TR_inet_ntoa, 0);
#ifdef _REENTRANT
	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		buf = b;
	}

	/*
	 * This is the case of threads other than the first.
	 */
	else {
		key_tbl
		   = (struct _rpc_tsd *)
		     _mt_get_thr_specific_storage(__rpc_key, RPC_KEYTBL_SIZE);
		if (key_tbl == NULL) {
			trace1(TR_inet_ntoa, 1);
                	return(NULL);
		}
        	if (key_tbl->inet_p == NULL) 
        		key_tbl->inet_p = (void *)malloc(INETADDRSIZE);
		buf = (char *)key_tbl->inet_p;
		if (buf == NULL) {
			trace1(TR_inet_ntoa, 1);
			return(buf);
		}
	}
#else
	buf = b;
#endif /* _REENTRANT */

#define	UC(b)	(((int)b)&0xff)
	p = (char *)&in;
	sprintf(buf, "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
        trace1(TR_inet_ntoa, 1);
	return (buf);
}

#ifdef _REENTRANT

void
_free_rpc_inet(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

/*
 * Internet address interpretation routine.
 * All the network library routines call this
 * routine to interpret entries in the data bases
 * which are expected to be an address.
 * The value returned is in network order.
 */
u_long
inet_addr(cp)
	register char *cp;
{
	register u_long val, base, n;
	register char c;
	u_long parts[4], *pp = parts;

        trace1(TR_inet_addr, 0);
again:
	/*
	 * Collect number up to ``.''.
	 * Values are specified as for C:
	 * 0x=hex, 0=octal, other=decimal.
	 */
	val = 0; base = 10;
	if (*cp == '0') {
		if (*++cp == 'x' || *cp == 'X')
			base = 16, cp++;
		else
			base = 8;
	}
	while (c = *cp) {
		if (isdigit(c)) {
			if ((c - '0') >= base)
			    break;
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	if (*cp == '.') {
		/*
		 * Internet format:
		 *	a.b.c.d
		 *	a.b.c	(with c treated as 16-bits)
		 *	a.b	(with b treated as 24 bits)
		 */
		if (pp >= parts + 4) {
			trace1(TR_inet_addr, 1);
			return (-1);
		}
		*pp++ = val, cp++;
		goto again;
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && !isspace(*cp)) {
		trace1(TR_inet_addr, 1);
		return (-1);
	}
	*pp++ = val;
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts;
	switch (n) {

	case 1:				/* a -- 32 bits */
		val = parts[0];
		break;

	case 2:				/* a.b -- 8.24 bits */
		val = (parts[0] << 24) | (parts[1] & 0xffffff);
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
			(parts[2] & 0xffff);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
		      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
		break;

	default:
		trace1(TR_inet_addr, 1);
		return (-1);
	}
	val = htonl(val);
	trace1(TR_inet_addr, 1);
	return (val);
}


/*
 * Return the network number from an internet
 * address; handles class a/b/c network #'s.
 */
inet_netof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	int val;

	trace1(TR_inet_netof, 0);
	if (IN_CLASSA(i)) {
		val = ((i)&IN_CLASSA_NET) >> IN_CLASSA_NSHIFT;
	} else if (IN_CLASSB(i)) {
		val = ((i)&IN_CLASSB_NET) >> IN_CLASSB_NSHIFT;
	} else {
		val = ((i)&IN_CLASSC_NET) >> IN_CLASSC_NSHIFT;
	}
	trace1(TR_inet_netof, 1);
	return (val);
}
