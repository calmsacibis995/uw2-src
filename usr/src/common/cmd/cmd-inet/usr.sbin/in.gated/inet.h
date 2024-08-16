/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/inet.h	1.2"
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
 *      System V STREAMS TCP - Release 4.0
 *
 *      Copyright 1990 Interactive Systems Corporation,(ISC)
 *      All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * $Header: /users/jch/src/gated/src/RCS/inet.h,v 2.0 90/04/16 16:52:44 jch Exp $
 */

/********************************************************************************
*										*
*	GateD, Release 2							*
*										*
*	Copyright (c) 1990 by Cornell University				*
*	    All rights reserved.						*
*										*
*	    Royalty-free licenses to redistribute GateD Release 2 in		*
*	    whole or in part may be obtained by writing to:			*
*										*
*	    Center for Theory and Simulation in Science and Engineering		*
*	    Cornell University							*
*	    Ithaca, NY 14853-5201.						*
*										*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
*										*
*	GateD is based on Kirton's EGP, UC Berkeley's routing daemon		*
*	(routed), and DCN's HELLO routing Protocol.  Development of Release	*
*	2 has been supported by the National Science Foundation.		*
*										*
*	The following acknowledgements and thanks apply:			*
*										*
*	    Mark Fedor (fedor@psi.com) for the development and maintenance	*
*	    up to release 1.3.1 and his continuing advice.			*
*										*
*********************************************************************************
*      Portions of this software may fall under the following			*
*      copyrights: 								*
*										*
*	Copyright (c) 1988 Regents of the University of California.		*
*	All rights reserved.							*
*										*
*	Redistribution and use in source and binary forms are permitted		*
*	provided that the above copyright notice and this paragraph are		*
*	duplicated in all such forms and that any documentation,		*
*	advertising materials, and other materials related to such		*
*	distribution and use acknowledge that the software was developed	*
*	by the University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote products derived	*
*	from this software without specific prior written permission.		*
*	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
********************************************************************************/


/* macros to select internet address given pointer to a struct sockaddr */

/* clear and init a sockaddr_in */
#ifdef	RTM_ADD
#define	sockclear_in(x)	memset((caddr_t)(x), (char) 0, sizeof(struct sockaddr_in)); \
    			((struct sockaddr_in *)(x))->sin_family = AF_INET; \
    			((struct sockaddr_in *)(x))->sin_len = sizeof (struct sockaddr_in);
#else				/* RTM_ADD */
#define	sockclear_in(x)	memset((caddr_t)(x), (char) 0, sizeof(struct sockaddr_in)); \
    			((struct sockaddr_in *)(x))->sin_family = AF_INET;
#endif				/* RTM_ADD */
#define	socktype_in(x)	((struct sockaddr_in *) (x))

/* additional definitions to netinet/in.h */

#ifndef IPPROTO_EGP
#define IPPROTO_EGP 8
#endif

/* definitions from C-gateway */

#define	AMSK	0200			/* Mask values used to decide on which */
#define	AVAL	0000			/* class of address we have */
#define	BMSK	0300
#define	BVAL	0200
#define	CMSK	0340			/* the associated macros take an arg */
#define	CVAL	0300			/* of the form in_addr.i_aaddr.i_anet */

#define	in_isa(x)	(((x) & AMSK) == AVAL)
#define	in_isb(x)	(((x) & BMSK) == BVAL)
#define	in_isc(x)	(((x) & CMSK) == CVAL)

#define CLAA 1
#define CLAB 2
#define CLAC 3

/* definitions from routed/defs.h */

#define equal(a1, a2) (memcmp((caddr_t)(a1), (caddr_t)(a2), socksize(a1)) == 0)
#define	equal_in(a1, a2) (a1.s_addr == a2.s_addr)

/* Calculate the natural netmask for a given network */
#define	gd_inet_netmask(net) (IN_CLASSA(net) ? IN_CLASSA_NET :\
			 (IN_CLASSB(net) ? IN_CLASSB_NET :\
			  (IN_CLASSC(net) ? IN_CLASSC_NET : 0)))

/* Test if the address is a host */
#define	gd_inet_ishost(sin)	(gd_inet_lnaof(socktype_in(sin)->sin_addr) != 0)

/* Compare two sockaddr_in's for an address match */
#define	gd_inet_netmatch(sin1, sin2)	(gd_inet_netof((sin1)->sin_addr) == gd_inet_netof((sin2)->sin_addr))

#ifdef	USE_PROTOTYPES
extern struct in_addr gd_inet_makeaddr(u_long net, int host, int subnetsAllowed);
extern u_long gd_inet_netof(struct in_addr in);
extern u_long gd_inet_wholenetof(struct in_addr in);
extern u_long gd_inet_lnaof(struct in_addr in);
extern int gd_inet_class(u_char * net);
extern int gd_inet_checkhost(struct sockaddr_in * sin);
extern u_long gd_inet_hash(sockaddr_un * sin);
extern char *inet_ntoa(struct in_addr in_addr);
extern u_short gd_inet_cksum(struct iovec v[], int nv, int len);

#else				/* USE_PROTOTYPES */
extern u_long gd_inet_hash();
extern u_long gd_inet_wholenetof();
extern u_long gd_inet_netof();
extern u_long gd_inet_lnaof();
extern int gd_inet_checkhost();
extern int gd_inet_class();
extern u_short gd_inet_cksum();
extern struct in_addr gd_inet_makeaddr();
extern char *inet_ntoa();

#endif				/* USE_PROTOTYPES */
