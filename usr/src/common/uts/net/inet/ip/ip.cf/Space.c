/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ip/ip.cf/Space.c	1.7"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <netinet/ip_str.h>
#include <netinet/ip_var.h>
#include <sys/protosw.h>

/*
 * ipcnt stores the number of ip minor devices (upper streams) and
 * ip_pcb[] is the array of ip upper stream private data structures.
 * IP_UNITS (from System file).
 */
int	ipcnt = IP_UNITS;
struct ip_pcb	ip_pcb[IP_UNITS];
/*
 * Switch to disable ip checksums.
 * XXX DON'T TRY THIS TRICK AT HOME KIDS!
 * IPCKSUM is a tunable.
 */
boolean_t	ipcksum = IPCKSUM;
/*
 * Switch to determine if host will act as a gateway.
 * IPFORWARDING is a tunable.
 */
int	ipforwarding = IPFORWARDING;
/*
 * Switch to determine if host will generate ICMP redirect messages when
 * forwarding packets to a destination that the sender should be able to
 * reach directly.  This should be enabled if needed to support diskless
 * workstations or if host is acting as a gateway (see IPFORWARDING above).
 * IPSENDREDIRECTS is a tunable.
 */
boolean_t	ipsendredirects = IPSENDREDIRECTS;
/*
 * Time-to-live for fragments in the ip packet reassembly queue.
 */
int	ipq_ttl = IPFRAGTTL / PR_SLOWHZ;
/*
 * DEBUG only.
 */
boolean_t	ipprintfs = B_FALSE;
