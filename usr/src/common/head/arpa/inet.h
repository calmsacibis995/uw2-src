/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:arpa/inet.h	1.1.9.6"
#ident  "$Header:  "

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

#ifndef _ARPA_INET_H
#define _ARPA_INET_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * External definitions for
 * functions in inet(3N)
 */

#ifdef __STDC__

extern int 		inet_lnaof(struct in_addr);
extern int		inet_netof(struct in_addr);
extern unsigned long	inet_addr(char *);
extern char		*inet_ntoa(struct in_addr);
extern struct in_addr	inet_makeaddr(unsigned long, unsigned long);
extern unsigned long	inet_network(char *);

#else /* !__STDC__ */

extern int 		inet_lnaof();
extern int		inet_netof();
extern unsigned long	inet_addr();
extern char		*inet_ntoa();
extern struct in_addr	inet_makeaddr();
extern unsigned long	inet_network();

#endif /* __STDC__ */

#if defined(__cplusplus)
}
#endif

#endif /* ARPA_INET_H */
