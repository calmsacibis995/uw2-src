/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/netstat/route.c	1.2.9.5"
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

/*
 *
 * Copyright 1987, 1988 Lachman Associates, Incorporated (LAI) All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */


#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stream.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <net/route.h>
#include <netinet/ip_str.h>

#include <netdb.h>
#include <stropts.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <errno.h>

extern int      kmem;
extern int      nflag;
extern char    *routename(), *netname(), *ns_print(), *plural();

#define satosin(sa)	((struct sockaddr_in *)(sa))

/*
 * Definitions for showing gateway flags. 
 */
struct bits {
	short           b_mask;
	char            b_val;
}               bits[] = {
	{
		                RTF_UP, 'U'
	}              ,
	{
		                RTF_GATEWAY, 'G'
	}              ,
	{
		                RTF_HOST, 'H'
	}              ,
	{
		                RTF_DYNAMIC, 'D'
	}              ,
	{
		                0
	}
};

/*
 * Print routing tables. 
 */
routepr(hostaddr, netaddr, hashsizeaddr)
	off_t           hostaddr, netaddr, hashsizeaddr;
{
	struct rtentry  rt;
	mblk_t          mb, *m;
	register struct bits *p;
	char            name[16], *flags;
	mblk_t        **routehash;
	struct ip_provider prov;
	int             hashsize;
	int             i, doinghost = 1;

	if (hostaddr == 0) {
		fprintf(stderr, "rthost: symbol not in namelist\n");
		return;
	}
	if (netaddr == 0) {
		fprintf(stderr, "rtnet: symbol not in namelist\n");
		return;
	}
	if (hashsizeaddr == 0) {
		fprintf(stderr, "rthashsize: symbol not in namelist\n");
		return;
	}
	readmem(hashsizeaddr, 1, 0, &hashsize, sizeof(hashsize), "hashsize");
	routehash = (mblk_t **) malloc(hashsize * sizeof(mblk_t *));
	readmem(hostaddr, 1, 0, routehash, hashsize * sizeof(mblk_t *),
		"routehash");
	printf("Routing tables\n");
	printf("%-16.16s %-18.18s %-6.6s %6.6s%8.8s  %s\n",
	       "Destination", "Gateway",
	       "Flags", "Refs", "Use", "Interface");
again:
	for (i = 0; i < hashsize; i++) {
		if (routehash[i] == 0)
			continue;
		m = routehash[i];
		while (m) {
			struct in_addr  in;

			readmem(m, 1, 0, &mb, sizeof(mb), "mblock");
			readmem(mb.b_rptr, 1, 0, &rt, sizeof(rt), "rtentry");
			in = satosin(&rt.rt_dst)->sin_addr;
			if (rt.rt_prov)
				readmem(rt.rt_prov, 1, 0, &prov, sizeof(prov),
					"provider");
			printf("%-16.16s ",
			       (in.s_addr == 0) ? "default" :
			       (rt.rt_flags & RTF_HOST) ?
			       routename(in) :
			       netname(in, 0));
			in = satosin(&rt.rt_gateway)->sin_addr;
			printf("%-18.18s ", routename(in));
			for (flags = name, p = bits; p->b_mask; p++)
				if (p->b_mask & rt.rt_flags)
					*flags++ = p->b_val;
			*flags = '\0';
			printf("%-6.6s %-6d %-8d ", name,
			       rt.rt_refcnt, rt.rt_use);
			if (rt.rt_prov == 0) {
				putchar('\n');
				m = mb.b_cont;
				continue;
			}
			readmem(rt.rt_prov, 1, 0, &prov, sizeof(prov),
				"provider");
			printf("%s\n", prov.name);
			m = mb.b_cont;
		}
	}
	if (doinghost) {
		readmem(netaddr, 1, 0, routehash, hashsize * sizeof(mblk_t *),
			"routehash");
		doinghost = 0;
		goto again;
	}
	free(routehash);
}

/*
 * Same as routepr except it uses ioctls to get info from kernel
 */
routepr_mp()
{
	int	ip;
	struct	strioctl ioc;
	int	rt_num, len;
	struct rtrecord  *rec;
	char   *buffer;
	struct in_addr  in;
	register struct bits *p;
	char     name[16], *flags;

	if ((ip = open("/dev/ip", O_RDONLY)) < 0) {
		perror("open /dev/ip failed");
		return;
	}

	/* get number of rtentries in route tables */
	ioc.ic_cmd = SIOCGRTSIZ;
	ioc.ic_timout = 0;
	ioc.ic_len = 0;
	ioc.ic_dp = 0;
	rt_num = ioctl(ip, I_STR, (caddr_t) &ioc);

	if (rt_num < 0) {
		perror("SIOCGRTSIZ failed");
		close(ip);
		return;
	}
	if (rt_num == 0) {
		printf("route tables empty\n");
		close(ip);
		return;
	}

	rt_num += 3;	/* allocate some extra space */

	if ((buffer = (char*) calloc(rt_num,sizeof(struct rtrecord))) == NULL) {
		perror("cannot allocate rtrecords");
		close(ip);
		return;
	}

	/* get all rtentries */
	ioc.ic_cmd = SIOCGRTTAB;
	ioc.ic_timout = 0;
	ioc.ic_len = rt_num * sizeof(struct rtrecord);
	ioc.ic_dp = buffer;
	if ((rt_num = ioctl(ip, I_STR, (caddr_t) &ioc)) < 0) {
		perror("SIOCGRTTAB failed");
		free(buffer);
		close(ip);
		return;
	}

	printf("Routing tables\n");
	printf("%-20.20s %-20.20s %-8.8s %-6.6s %-10.10s %s\n",
	       "Destination", "Gateway",
	       "Flags", "Refcnt", "Use", "Interface");

	rec = (struct rtrecord *) buffer;
	for ( ; rt_num ; rt_num--, rec++ ) {		
		in = satosin(&rec->rt_dst)->sin_addr;
		printf("%-20.20s ", (in.s_addr == 0) ? "default" :
		       		(rec->rt_flags & RTF_HOST) ?
				 routename(in) :
				 netname(in, 0));
		in = satosin(&rec->rt_gateway)->sin_addr;
		printf("%-20.20s ", routename(in));
		for (flags = name, p = bits; p->b_mask; p++)
			if (p->b_mask & rec->rt_flags)
				*flags++ = p->b_val;
		*flags = '\0';
		printf("%-8.8s %-6d %-10d ", name, rec->rt_refcnt, rec->rt_use);
		printf("%s\n", rec->rt_prov);
		
	}
	free(buffer);
}

char           *
routename(in)
	struct in_addr  in;
{
	register char  *cp;
	static char     line[MAXHOSTNAMELEN + 1];
	struct hostent *hp;
	static char     domain[MAXHOSTNAMELEN + 1];
	static int      first = 1;
	char           *strchr();

	if (first) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = strchr(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag) {
		hp = gethostbyaddr(&in, sizeof(struct in_addr),
				   AF_INET);
		if (hp) {
			if ((cp = strchr(hp->h_name, '.')) &&
#if 0
			    /*
			     * XXX - strcasecmp will be in the new libsocket
			     */
			    !strcasecmp(cp + 1, domain))
#else
			    !strcmp(cp + 1, domain))
#endif
				*cp = 0;
			cp = hp->h_name;
		}
	}
	if (cp)
		strncpy(line, cp, sizeof(line) - 1);
	else {
#define C(x)	((x) & 0xff)
		in.s_addr = ntohl(in.s_addr);
		(void) sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
	}
	return (line);
}

/*
 * Return the name of the network whose address is given. The address is
 * assumed to be that of a net or subnet, not a host. 
 */
char           *
netname(in, mask)
	struct in_addr  in;
	u_long          mask;
{
	char           *cp = 0;
	static char     line[MAXHOSTNAMELEN - 1];
	struct netent  *np = 0;
	u_long          net;
	register        i;
	int             subnetshift;

	i = ntohl(in.s_addr);
	if (!nflag && i) {
		if (mask == 0) {
			if (IN_CLASSA(i)) {
				mask = IN_CLASSA_NET;
				subnetshift = 8;
			} else if (IN_CLASSB(i)) {
				mask = IN_CLASSB_NET;
				subnetshift = 8;
			} else {
				mask = IN_CLASSC_NET;
				subnetshift = 4;
			}
			/*
			 * If there are more bits than the standard mask
			 * would suggest, subnets must be in use. Guess at
			 * the subnet mask, assuming reasonable width subnet
			 * fields. 
			 */
			while (i & ~mask)
				mask |= (long) mask >> subnetshift;
		}
		net = i & mask;
		while ((mask & 1) == 0)
			mask >>= 1, net >>= 1;
		np = getnetbyaddr(net, AF_INET);
		if (np)
			cp = np->n_name;
	}
	if (cp)
		strcpy(line, cp);
	else if ((i & 0xffffff) == 0)
		sprintf(line, "%u", C(i >> 24));
	else if ((i & 0xffff) == 0)
		sprintf(line, "%u.%u", C(i >> 24), C(i >> 16));
	else if ((i & 0xff) == 0)
		sprintf(line, "%u.%u.%u", C(i >> 24), C(i >> 16), C(i >> 8));
	else
		sprintf(line, "%u.%u.%u.%u", C(i >> 24), C(i >> 16),
			C(i >> 8), C(i));
	return (line);
}

/*
 * Print routing statistics 
 */
rt_stats(off, coredump)
	int	coredump;
	off_t           off;
{
	struct rtstat   rtstat;
	int	ip;
	struct	strioctl ioc;

	if (!coredump) {
		if ((ip = open("/dev/ip",O_RDONLY)) < 0) {
			perror("open /dev/ip failed");
			return;
		}
		ioc.ic_cmd = SIOCGRTSTATS;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof (struct rtstat);
		ioc.ic_dp = (caddr_t) &rtstat;
		if (ioctl(ip, I_STR, (caddr_t) &ioc) < 0) {
			perror("SIOCGRTSTATS failed");
			close(ip);
			return;
		}
		close(ip);
	} else {
		if (off == 0) {
			fprintf(stderr, "rtstat: symbol not in namelist\n");
			return;
		}
		readmem(off, 1, 0, &rtstat, sizeof(rtstat), "rtstat");
	}
	printf("routing:\n");
	printf("\t%u bad routing redirect%s\n",
	       rtstat.rts_badredirect, plural(rtstat.rts_badredirect));
	printf("\t%u dynamically created route%s\n",
	       rtstat.rts_dynamic, plural(rtstat.rts_dynamic));
	printf("\t%u new gateway%s due to redirects\n",
	       rtstat.rts_newgateway, plural(rtstat.rts_newgateway));
	printf("\t%u destination%s found unreachable\n",
	       rtstat.rts_unreach, plural(rtstat.rts_unreach));
	printf("\t%u use%s of a wildcard route\n",
	       rtstat.rts_wildcard, plural(rtstat.rts_wildcard));
}
