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


#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/htable/htable.h	1.1.7.4"
#ident  "$Header: htable.h 1.2 91/06/26 $"

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
#include <netinet/in.h>

/*
 * common definitions for htable
 */

struct addr {
	struct	in_addr addr_val;
	struct	addr *addr_link;
};

struct name {
	char	*name_val;
	struct	name *name_link;
};

struct gateway {
	struct	gateway *g_link;
	struct	gateway *g_dst;		/* connected gateway if metric > 0 */
	struct	gateway *g_firstent;	/* first entry for this gateway */
	struct	name	*g_name;
	int	g_net;
	struct in_addr	g_addr;		/* address on g_net */
	int	g_metric;		/* hops to this net */
};

#define	NOADDR			((struct addr *)0)
#define	NONAME			((struct name *)0)

#define	KW_NET		1
#define	KW_GATEWAY	2
#define	KW_HOST		3

struct name *newname();

#ifdef __STDC__
void *malloc();
#else
char *malloc();
#endif	/* __STDC__*/

char *infile;			/* Input file name */
