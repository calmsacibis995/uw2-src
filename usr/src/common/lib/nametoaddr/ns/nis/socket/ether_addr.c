/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/socket/ether_addr.c	1.2"
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


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include "nis.h"
#include <ns.h>

extern	char	*ether_ntoa();
extern  ether_addr_t *ether_aton();

/*
 * Given a host's name, this routine returns its 48 bit ethernet address.
 * Returns zero if successful, non-zero otherwise.
 */
nis_ether_hostton(host, e)
	char *host;		/* function input */
	ether_addr_t e;	/* function output */
{
	char currenthost[256];
	char *val = NULL;
	int vallen;
	register int err;
	char *map = "ethers.byname";
	char *domain = nis_domain();
	
	err = yp_match(domain, map, host, strlen(host), &val, &vallen);
	if (err){
		set_niserror(err);
		yp_retcode(err);
		return(-1);
	}
	set_nsaction(NS_SUCCESS);
	err = ether_line(val, e, currenthost);
	free(val);  /* yp_match always allocates for us */
	return (err);
}

/*
 * Given a 48 bit ethernet address, this routine return its host name.
 * Returns zero if successful, non-zero otherwise.
 */
nis_ether_ntohost(host, e)
	char *host;		/* function output */
	ether_addr_t e;	/* function input */
{
	ether_addr_t currente;
	char *val=NULL;
	int vallen;
	char *map = "ethers.byaddr";
	char *domain = nis_domain();
	int  err;
	char b[18];

	sprintf(b, "%x:%x:%x:%x:%x:%x", 
			e[0], e[1], e[2], e[3], e[4], e[5]);
	err = yp_match(domain, map, b, strlen(b), &val, &vallen);
	if (err){
		set_niserror(err);
		yp_retcode(err);
		return(-1);
	}
	set_nsaction(NS_SUCCESS);
	err = ether_line(val, &currente, host);
	free(val);  /* yp_match always allocates for us */
	return (err);
}
