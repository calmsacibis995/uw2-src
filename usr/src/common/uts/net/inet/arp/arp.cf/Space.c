/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/arp/arp.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <netinet/if_ether.h>

/*
 * Specify the number of arp hash table buckets.
 * ARPHASHBCKTCNT is a tunable.
 */
int	arptab_nb = ARPHASHBCKTCNT;
/*
 * Specify the number of entries per arp hash table bucket.
 * ARPHASHBCKTSIZE is a tunable.
 */
int	arptab_bsiz = ARPHASHBCKTSIZE;
/*
 * Set the size of the arp hash table.
 */
#define ARPTABSIZE	(ARPHASHBCKTCNT * ARPHASHBCKTSIZE)
int	arptab_size = ARPTABSIZE;
struct arptab	arptab[ARPTABSIZE];
