/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/route/route.cf/Space.c	1.4"
#ident	"$Header: $"

#include <config.h>
#include <netinet/ip_str.h>

#if RT_UNITS == 0		/* XXX */
#undef RT_UNITS			/* XXX */
#define RT_UNITS	16	/* XXX */
#endif				/* XXX */

/*
 * Number of DLPI Services providers (IP lower streams) supported.
 * RT_UNITS (from System file).
 */
int	provider_cnt = RT_UNITS;
struct ip_provider	provider[RT_UNITS];
/*
 * Switch to enable large packet transmissions via routers/gateways.
 * RTSUBNETSARELOCAL is a tunable.
 */
boolean_t	subnetsarelocal = RTSUBNETSARELOCAL;
