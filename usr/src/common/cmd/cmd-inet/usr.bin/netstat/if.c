/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/netstat/if.c	1.1.10.4"
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

#include <sys/types.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MACIOC(x)       	(('M' << 8) | (x))
#define MACIOC_GETSTAT		MACIOC(7)

#include <net/if.h>
#include <sys/stream.h>
#include <netinet/in.h>
#include <net/route.h>
#include <netinet/in_var.h>
#include <netinet/ip_str.h>
#include <stdio.h>

#define	MAXIF	20

struct iftot {
	char            ift_name[IFNAMSIZ];	/* interface name */
	int             ift_ip;	/* input packets */
	int             ift_ie;	/* input errors */
	int             ift_op;	/* output packets */
	int             ift_oe;	/* output errors */
	int             ift_co;	/* collisions */
} iftot[MAXIF];

struct ifrecord  ifrecord[MAXIF];

extern int      kmem;
extern int      iflag;
extern char    *interface;
extern char    *routename(), *netname();

/*
 * Print a description of the network interfaces. 
 * Get info using ioctls.
 */

intpr_mp(interval, dummy)
	int             interval;
	off_t           dummy;
{
	struct strioctl ioc;
	struct ifreq	*ifreq;
	register struct ifrecord *rec;
	register struct in_ifaddr *ifa;
	struct in_addr  in;
	struct sockaddr_in *sin;
	register char	*cp;
	register int	n;
	int		fd, i, j, count;

	if (interval) {
		sidewaysintpr(interval, dummy);
		return;
	}

	if ((fd = open("/dev/ip", O_RDONLY)) < 0) {
		perror("open /dev/ip failed");
		return;
	}

	ifreq = (struct ifreq*) ifrecord;
	if (interface)
	    strncpy(ifreq->ifr_name, interface, IFNAMSIZ);	/* ??? */
	else
	    ifreq->ifr_name[0] = 0;
	count = interface ? 1 : MAXIF-2;
	ioc.ic_cmd = interface ? SIOCGIFSTATS : SIOCGIFSTATS_ALL;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(struct ifrecord) * count;
	ioc.ic_dp = (caddr_t) ifrecord;

	if ((count = ioctl(fd, I_STR, (caddr_t) &ioc)) < 0) {
		if (interface && errno == EINVAL)
			fprintf(stderr, "invalid interface <%s>\n", interface);
		else
			perror("SIOCGIFSTATS or SIOCGIFSTATS_ALL failed");
		close(fd);
		return;
	}
	close(fd);
	printf("%-7.7s %-5.5s %-11.11s %-15.15s %-7.7s %-5.5s %-7.7s %-5.5s",
	       "Name", "Mtu", "Network", "Address", "Ipkts", "Ierrs",
	       "Opkts", "Oerrs");
	printf(" %-6.6s", "Collis");
	putchar('\n');

	for (rec=ifrecord, i=0 ; i<count ; i++,rec++) { 
		char nametmp[80];

		if (rec->ifs_name[0] == 0)
		{
		    continue;
		}
		sprintf(nametmp, "%.6s%d", rec->ifs_name, rec->ifs_unit); 
		printf("%-7s", nametmp); 
		printf("%c ",rec->ifs_active ? ' ' : '*');
		printf("%-5d ", rec->ifs_mtu);
		if ((j = rec->ifs_addrcnt) == 0) {
			printf("%-11.11s ", "none");
			printf("%-15.15s ", "none");
		}
		for ( ifa=rec->ifs_addrs ; j ; j--,ifa++) {
			switch (ifa->ia_addr.sa_family) {
			case AF_UNSPEC:
				printf("%-11.11s ", "none");
				printf("%-15.15s ", "none");
				break;
			case AF_INET:
				sin = (struct sockaddr_in *) ifa;
				in.s_addr = htonl(ifa->ia_subnet);
				printf("%-11.11s ",
				       netname(in, ifa->ia_subnetmask));
				printf("%-15.15s ", routename(sin->sin_addr));
				break;
			default:
				printf("af%2d: ", ifa->ia_addr.sa_family);
				for (cp = (char *) ifa +
				     sizeof(struct sockaddr) - 1;
				     cp >= ifa->ia_addr.sa_data; --cp)
					if (*cp != 0)
						break;
				/* cp points to the last nonblank char;
                                   n is the number of non blank char */ 
				n = cp - ifa->ia_addr.sa_data + 1;
				cp = ifa->ia_addr.sa_data;
				if (n <= 6)
					while (--n)
						printf("%02d.", *cp++ & 0xff);
				else
					while (--n)
						printf("%02d", *cp++ & 0xff);
				printf("%02d    ", *cp & 0xff);
				break;
			}
		}
		printf("%-7d %-5d %-7d %-5d %-6d\n",
		       rec->ifs_ipackets, rec->ifs_ierrors,
		       rec->ifs_opackets, rec->ifs_oerrors,
		       rec->ifs_collisions);
	}
}


/*
 * Print a description of the network interfaces. 
 */
intpr(interval, ifnetaddr, provptr, lptr)
	int             interval;
	off_t           ifnetaddr;
	off_t		provptr;
	off_t		lptr;
{
	struct ifstats  ifstats;
	union {
		struct ifaddr   ifa;
		struct in_ifaddr in;
	}               ifaddr;
	int		provcnt = 0;
	int		tprov = 0;
	off_t		lastprov;
	char            name[16];
	struct	ip_provider	ifp;

	if (provptr == (off_t)0) {
		fprintf(stderr, "provider: symbol not defined\n");
		return;
	}

	if (ifnetaddr == 0) {
		fprintf(stderr, "ifstats: symbol not defined\n");
		return;
	}
	if (interval) {
		sidewaysintpr(interval, ifnetaddr);
		return;
	}

	printf("%-7.7s %-5.5s %-11.11s %-15.15s %-7.7s %-5.5s %-7.7s %-5.5s",
	       "Name", "Mtu", "Network", "Address", "Ipkts", "Ierrs",
	       "Opkts", "Oerrs");
	printf(" %-7s", "Collis");
	putchar('\n');

	/*
	 * lastprov -> last provider, not past the end, so add 1
	 */
	readmem(lptr, 1, 0, (char *)&lastprov, sizeof(lastprov), "lastprov");
	tprov = lastprov - provptr;
	tprov /= sizeof(struct ip_provider);
	tprov++;
	while (provcnt < tprov) {
		struct sockaddr_in *sin;
		register char  *cp;
		int             n;
		struct in_addr  in, inet_makeaddr();

		readmem(provptr + (sizeof(struct ip_provider) * provcnt++), 1, 0, (char *)&ifp, 
			sizeof(ifp), "ip_provider");

		if (!ifp.qbot)
			continue;
		ifp.name[15] = '\0';
		cp = strchr(ifp.name,'\0');
		if (cp != &ifp.name[15] && ((ifp.if_flags & IFF_UP) == 0)) {
			*cp++ = '*';
			*cp = '\0';
		}

		if ((strcmp (interface,ifp.name) == 0) || iflag)
			printf("%-7s %-5d ", ifp.name,
		       	ifp.if_maxtu);
		else {
			continue;
		}
		switch (ifp.ia.ia_ifa.ifa_addr.sa_family) {
			case AF_UNSPEC:
				printf("%-11.11s ", "none");
				printf("%-15.15s ", "none");
				break;
			case AF_INET:
				sin = (struct sockaddr_in *) & ifp.ia.ia_ifa.ifa_addr;
				in.s_addr = htonl(ifp.ia.ia_subnet);
				printf("%-11.11s ",
				       netname(in,
					       ifp.ia.ia_subnetmask));
				printf("%-15.15s ", routename(sin->sin_addr));
				break;
			default:
				printf("af%2d: ", ifp.ia.ia_ifa.ifa_addr.sa_family);
				for (cp = (char *) &ifp.ia.ia_ifa.ifa_addr +
				     sizeof(struct sockaddr) - 1;
				     cp >= ifp.ia.ia_ifa.ifa_addr.sa_data; --cp)
					if (*cp != 0)
						break;
				n = cp - (char *) ifp.ia.ia_ifa.ifa_addr.sa_data + 1;
				cp = (char *) ifp.ia.ia_ifa.ifa_addr.sa_data;
				if (n <= 6)
					while (--n)
						printf("%02d.", *cp++ & 0xff);
				else
					while (--n)
						printf("%02d", *cp++ & 0xff);
				printf("%02d    ", *cp & 0xff);
				break;
			}
		if (ifp.ia.ia_ifa.ifa_ifs) {
			readmem(ifp.ia.ia_ifa.ifa_ifs, 1, 0, (char *)&ifstats, sizeof(ifstats), "ifstats element");
			printf("%-7d %-5d %-7d %-5d %-6d",
			       ifstats.ifs_ipackets, ifstats.ifs_ierrors,
			       ifstats.ifs_opackets, ifstats.ifs_oerrors,
			       ifstats.ifs_collisions);
		} else {
			int fd;
			char devname[32];
			struct strioctl strioc;

			strcpy(devname,"/dev/");
			strcat(devname,ifp.name);
			fd = open(devname, O_RDONLY);
			if (fd >= 0) {
				strioc.ic_len = sizeof(struct ifstats);
				strioc.ic_timout = 0;
				strioc.ic_dp = (char *) &ifstats;
				strioc.ic_cmd = MACIOC_GETSTAT;
				if (ioctl(fd, I_STR, &strioc) < 0) {
					printf("No Statistics Available");
				}
				else {
					printf("%-7d %-5d %-7d %-5d %-6d",
						ifstats.ifs_ipackets, 
						ifstats.ifs_ierrors,
						ifstats.ifs_opackets,
						ifstats.ifs_oerrors,
						ifstats.ifs_collisions);
				}
				close(fd);
			}
			else
				printf("No Statistics Available");
		}
		putchar('\n');
	}
}

/*
 * Print a running summary of interface statistics. Repeat display every
 * interval seconds, showing statistics collected over that interval.  First
 * line printed at top of screen is always cumulative. 
 */

sidewaysintpr(interval, dummy)
	int             interval;
	off_t           dummy;
{
	struct strioctl ioc;
	struct ifreq	*ifreq;
	register struct ifrecord *rec;
	int		fd, i, line = 0, count, realcount;
	struct iftot    *sum = iftot + MAXIF - 1;
	struct iftot	*total = sum - 1;
	register struct iftot	*ip, *interesting;
	char		name[30];
	int		foundmax = 0;
	unsigned long	curmaxpkts = 0;
	int		curmaxif = -1;

	if ((fd = open("/dev/ip", O_RDONLY)) < 0) {
		perror("open /dev/ip failed");
		return;
	}

	for (;;) { 
		ifreq = (struct ifreq*) ifrecord;
		if (interface)
		    strncpy(ifreq->ifr_name, interface, IFNAMSIZ);
		else
		    ifreq->ifr_name[0] = 0;
		count = interface ? 1 : MAXIF-2;
		ioc.ic_cmd = interface ? SIOCGIFSTATS : SIOCGIFSTATS_ALL;
		ioc.ic_len = sizeof(struct ifrecord) * count;
		ioc.ic_timout = 0;
		ioc.ic_dp = (caddr_t) ifrecord;
		count = ioctl(fd, I_STR, (caddr_t) &ioc);
		if (count < 0) {
			perror("SIOCGIFSTATS or SIOCGIFSTATS_ALL failed");
			close(fd);
			return;
		}
		if (count == 0) {
			fprintf(stderr, "no interface attached\n");
			close(fd);
			return;
		}

		for (realcount=0, i=0, rec=ifrecord ; i < count ; i++, rec++) {
			if (rec->ifs_name) {
			    unsigned long sum;
			    realcount++;
			    sprintf(name,"%s%d",ifrecord[i].ifs_name,
					    ifrecord[i].ifs_unit);	
			    /* Set name to if with most traffic! */
			    if ((foundmax == 0)
			    &&  (((sum = ifrecord[i].ifs_ipackets +
					ifrecord[i].ifs_ipackets))
				       > curmaxpkts)
			    ) {
				curmaxpkts = sum;
				curmaxif = i;
			    }
			}
		}
		foundmax = 1;

		if (line == 0) { /* banner */
			printf("    input  %-8.8s   output       ", 
				interface ? interface : name);
			if (realcount > 1)
				printf("   input  (Total)    output       ");
			putchar('\n');
			printf("%-7.7s %-5.5s %-7.7s %-5.5s %-5.5s ",
	       			"packets", "errs", "packets", "errs", "colls");
			if (realcount > 1)
				printf("%-7.7s %-5.5s %-7.7s %-5.5s %-5.5s ",
		       			"packets", "errs", "packets", "errs", "colls");
			putchar('\n');
			fflush(stdout);
			for (ip=iftot, i=0 ; i < MAXIF ; i++,ip++) {
				ip->ift_ip = 0;
				ip->ift_ie = 0;
				ip->ift_op = 0;
				ip->ift_oe = 0;
				ip->ift_co = 0;
			}	
		}

		sum->ift_ip = 0;
		sum->ift_ie = 0;
		sum->ift_op = 0;
		sum->ift_oe = 0;
		sum->ift_co = 0;

		for (i=0, rec=ifrecord, ip=iftot ; i < count ; i++,rec++,ip++) {
			if (rec->ifs_name == 0)
			{
			    continue;
			}
			sprintf(name,"%s%d",rec->ifs_name,rec->ifs_unit);	
			if ((interface && strncmp(name, interface, IFNAMSIZ) == 0) ||
			    (!interface && i==curmaxif) ) {
				printf("%-7d %-5d %-7d %-5d %-5d ",
			       		rec->ifs_ipackets - ip->ift_ip,
			       		rec->ifs_ierrors - ip->ift_ie,
			       		rec->ifs_opackets - ip->ift_op,
			       		rec->ifs_oerrors - ip->ift_oe,
			       		rec->ifs_collisions - ip->ift_co);
			}
			ip->ift_ip = rec->ifs_ipackets;
			ip->ift_ie = rec->ifs_ierrors;
			ip->ift_op = rec->ifs_opackets;
			ip->ift_oe = rec->ifs_oerrors;
			ip->ift_co = rec->ifs_collisions;
			sum->ift_ip += ip->ift_ip;
			sum->ift_ie += ip->ift_ie;
			sum->ift_op += ip->ift_op;
			sum->ift_oe += ip->ift_oe;
			sum->ift_co += ip->ift_co;
		}
		if (realcount > 1) {
			printf("%-7d %-5d %-7d %-5d %-5d\n",
		       		sum->ift_ip - total->ift_ip,
		       		sum->ift_ie - total->ift_ie,
		       		sum->ift_op - total->ift_op,
		       		sum->ift_oe - total->ift_oe,
		       		sum->ift_co - total->ift_co);
		}
		else {
			putchar('\n');
		}
		*total = *sum;
		fflush(stdout);
		line++;
		if (interval)
			sleep(interval);
		if (line == 21)
			line = 0;
	}
	/*NOTREACHED*/
}
