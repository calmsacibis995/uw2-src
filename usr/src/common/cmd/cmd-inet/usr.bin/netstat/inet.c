/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/netstat/inet.c	1.1.9.4"
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

#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stream.h>
#include <sys/protosw.h>

#include <netinet/in.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <netdb.h>

#include <sys/sockio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stropts.h>

struct inpcb    inpcb;
struct tcpcb    tcpcb;
extern int      kmem;
extern int      Aflag;
extern int      aflag;
extern int      nflag;
extern char	*plural();

static int      first = 1;
char           *inetname();

/*
 * Print a summary of connections related to an Internet protocol.  For TCP,
 * also give state of connection. Listening processes (aflag) are suppressed
 * unless the -a (all) flag is specified. 
 */

protopr_mp(name)
	char           *name;
{

	int	fd, pcbnum, istcp;
	struct	strioctl  ioc;
	char	devname[30], *buffer;
	register struct	pcbrecord *pcb;

	(void)sprintf(devname,"/dev/%s",name);
	if ((fd = open(devname, O_RDONLY)) < 0) {
		fprintf(stderr, "can't open %s\n",devname);
		return;
	}

	ioc.ic_cmd = SIOCGPCBSIZ;
	ioc.ic_timout = 0;
	ioc.ic_len = 0;
	ioc.ic_dp = 0;
	pcbnum = ioctl(fd, I_STR, (caddr_t) &ioc);

	if (pcbnum < 0) {
		perror("SIOCGPCBSIZ failed");
		close(fd);
		return;
	}
	if (pcbnum == 0) {
		close(fd);
		return;
	}
	pcbnum += 3;	/* allocate some extra space */

	if ((buffer = (caddr_t)calloc(pcbnum,sizeof(struct pcbrecord)))==NULL){
		perror("cannot allocate pcbrecord");
		close(fd);
		return;	
	}
	
	ioc.ic_cmd = SIOCGPCB;
	ioc.ic_timout = 0;
	ioc.ic_len = pcbnum * sizeof(struct pcbrecord);
	ioc.ic_dp = buffer;
	if ((pcbnum = ioctl(fd, I_STR, (caddr_t) &ioc)) < 0) {
		perror("cannot get internet pcb");
		close(fd);
		free(buffer);
		return;
	}

	istcp = strcmp(name, "tcp") == 0; 
	if (pcbnum == 0)
		return;

	for (pcb = (struct pcbrecord*)buffer; pcbnum ; pcbnum--,pcb++) {
		if (!aflag && inet_lnaof(pcb->inp_laddr) == INADDR_ANY) {
			continue;
		}
		if (first) {
			printf("Active Internet connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
			    "%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
			     "%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
			       "Proto", "Recv-Q", "Send-Q",
			     "Local Address", "Foreign Address", "(state)");
			first = 0;
		}
		if (Aflag)
			printf("%-8x ", pcb->inp_addr);
		printf("%-5.5s %6d %6d ", name, istcp ? tcpcb.t_iqsize : 0,
		       istcp ? tcpcb.t_outqsize : 0);
		inetioctlprint(pcb->inp_laddr, pcb->inp_lport, name);
		inetioctlprint(pcb->inp_faddr, pcb->inp_fport, name);
		if (istcp) {
			if (pcb->t_state < 0 || pcb->t_state >= TCP_NSTATES)
				printf(" %d", pcb->t_state);
			else
				printf(" %s", tcpstates[pcb->t_state]);
		}
		putchar('\n');
	}
}


protopr(off, name, coredump)
	off_t           off;
	char           *name;
	int		coredump;
{
	struct inpcb    cb;
	register struct inpcb *prev, *next;
	int             istcp;

	if (!coredump) {
		protopr_mp(name);
		return;
	}
	if (off == 0) {
		return;
	}
	istcp = strcmp(name, "tcp") == 0;
	readmem(off, 1, 0, (char *)&cb, sizeof(struct inpcb), "inpcb");
	inpcb = cb;
	prev = (struct inpcb *) off;
	while (inpcb.inp_next != (struct inpcb *) off) {

		next = inpcb.inp_next;
		readmem(next, 1, 0, (char *)&inpcb, sizeof(inpcb), "inpcb");
		if (inpcb.inp_prev != prev) {
			fprintf(stderr, 
				"corrupt control block chain\n");
			break;
		}
		if (!aflag &&
		    inet_lnaof(inpcb.inp_laddr) == INADDR_ANY) {
			prev = next;
			continue;
		}
		if (istcp) {
			readmem(inpcb.inp_ppcb, 1, 0, (char *)&tcpcb,
				sizeof(tcpcb), "tcpcb");
		}
		if (first) {
			printf("Active Internet connections");
			if (aflag)
				printf(" (including servers)");
			putchar('\n');
			if (Aflag)
				printf("%-8.8s ", "PCB");
			printf(Aflag ?
			    "%-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n" :
			     "%-5.5s %-6.6s %-6.6s  %-22.22s %-22.22s %s\n",
			       "Proto", "Recv-Q", "Send-Q",
			     "Local Address", "Foreign Address", "(state)");
			first = 0;
		}
		if (Aflag)
			printf("%-8x ", next);
		printf("%-5.5s %6d %6d ", name, istcp ? tcpcb.t_iqsize : 0,
		       istcp ? tcpcb.t_outqsize : 0);
		inetprint(&inpcb.inp_laddr, inpcb.inp_lport, name);
		inetprint(&inpcb.inp_faddr, inpcb.inp_fport, name);
		if (istcp) {
			if (tcpcb.t_state < 0 || tcpcb.t_state >= TCP_NSTATES)
				printf(" %d", tcpcb.t_state);
			else
				printf(" %s", tcpstates[tcpcb.t_state]);
		}
		putchar('\n');
		prev = next;
	}
}

/*
 * Dump TCP statistics structure. 
 */
 
tcp_stats(off, name, coredump)
	off_t           off;
	char           *name;
	int		coredump;
{
	struct tcpstat  tcpstat;
	int	fd;
	struct	strioctl ioc;

	if (!coredump) {
		if ((fd = open("/dev/tcp", O_RDONLY)) < 0) {
			perror("tcp_stats: open /dev/tcp failed");
			return;
		}

		ioc.ic_cmd = SIOCGTCPSTATS;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof (struct tcpstat);
		ioc.ic_dp = (caddr_t) &tcpstat;
		if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
			perror("tcp_stats: SIOCGTCPSTATS failed");
			close(fd);
			return;
		}
		close(fd);
	} else {
		if (off == 0) {
			return;
		}
		readmem(off, 1, 0, &tcpstat, sizeof(tcpstat), "tcpstat");
	}
	printf("%s:\n", name);

#define	p(f, m)		printf(m, tcpstat.f, plural(tcpstat.f))
#define	p2(f1, f2, m)	printf(m, tcpstat.f1, plural(tcpstat.f1), tcpstat.f2, plural(tcpstat.f2))
  
	p(tcps_sndtotal, "\t%d packet%s sent\n");
	p2(tcps_sndpack,tcps_sndbyte,
		"\t\t%d data packet%s (%d byte%s)\n");
	p2(tcps_sndrexmitpack, tcps_sndrexmitbyte,
		"\t\t%d data packet%s (%d byte%s) retransmitted\n");
	p2(tcps_sndacks, tcps_delack,
		"\t\t%d ack-only packet%s (%d delayed)\n");
	p(tcps_sndurg, "\t\t%d URG only packet%s\n");
	p(tcps_sndprobe, "\t\t%d window probe packet%s\n");
	p(tcps_sndwinup, "\t\t%d window update packet%s\n");
	p(tcps_sndctrl, "\t\t%d control packet%s\n");
	p(tcps_sndrsts, "\t\t\t%d reset%s\n");

	p(tcps_rcvtotal, "\t%d packet%s received\n");
	p2(tcps_rcvackpack, tcps_rcvackbyte, "\t\t%d ack%s (for %d byte%s)\n");
	p(tcps_rcvdupack, "\t\t%d duplicate ack%s\n");
	p(tcps_rcvacktoomuch, "\t\t%d ack%s for unsent data\n");
	p2(tcps_rcvpack, tcps_rcvbyte,
		"\t\t%d packet%s (%d byte%s) received in-sequence\n");
	p2(tcps_rcvduppack, tcps_rcvdupbyte,
		"\t\t%d completely duplicate packet%s (%d byte%s)\n");
	p2(tcps_rcvpartduppack, tcps_rcvpartdupbyte,
		"\t\t%d packet%s with some dup. data (%d byte%s duped)\n");
	p2(tcps_rcvoopack, tcps_rcvoobyte,
		"\t\t%d out-of-order packet%s (%d byte%s)\n");
	p2(tcps_rcvpackafterwin, tcps_rcvbyteafterwin,
		"\t\t%d packet%s (%d byte%s) of data after window\n");
	p(tcps_rcvwinprobe, "\t\t%d window probe%s\n");
	p(tcps_rcvwinupd, "\t\t%d window update packet%s\n");
	p(tcps_rcvafterclose, "\t\t%d packet%s received after close\n");
	p(tcps_rcvbadsum, "\t\t%d discarded for bad checksum%s\n");
	p(tcps_rcvbadoff, "\t\t%d discarded for bad header offset field%s\n");
	p(tcps_rcvshort, "\t\t%d discarded because packet too short\n");
	p(tcps_inerrors,
		"\t\t%d system error%s encountered during processing\n");

	p(tcps_connattempt, "\t%d connection request%s\n");
	p(tcps_accepts, "\t%d connection accept%s\n");
	p(tcps_connects, "\t%d connection%s established (including accepts)\n");
	p2(tcps_closed, tcps_drops,
		"\t%d connection%s closed (including %d drop%s)\n");
	p(tcps_conndrops, "\t%d embryonic connection%s dropped\n");
	p(tcps_attemptfails, "\t%d failed connect and accept request%s\n");
	p(tcps_estabresets, "\t%d reset%s received while established\n");
	p2(tcps_rttupdated, tcps_segstimed,
		"\t%d segment%s updated rtt (of %d attempt%s)\n");

	p(tcps_rexmttimeo, "\t%d retransmit timeout%s\n");
	p(tcps_timeoutdrop, "\t\t%d connection%s dropped by rexmit timeout\n");

	p(tcps_persisttimeo, "\t%d persist timeout%s\n");

	p(tcps_keeptimeo, "\t%d keepalive timeout%s\n");
	p(tcps_keepprobe, "\t\t%d keepalive probe%s sent\n");
	p(tcps_keepdrops, "\t\t%d connection%s dropped by keepalive\n");

	p(tcps_linger, "\t%d connection%s lingered\n");
	p(tcps_lingerexp, "\t\t%d linger timer%s expired\n");
	p(tcps_lingercan, "\t\t%d linger timer%s cancelled\n");
	p(tcps_lingerabort, "\t\t%d linger timer%s aborted by signal\n");
#undef p
#undef p2
}

/*
 * Dump UDP statistics structure. 
 */
udp_stats(off, name, coredump)
	off_t           off;
	char           *name;
	int		coredump;
{
	struct udpstat  udpstat;
	int	fd;
	struct	strioctl ioc;

	if (!coredump) {

		if ((fd = open("/dev/udp", O_RDONLY)) < 0) {
			perror("udp_stats: open /dev/udp failed");
			return;
		}

		ioc.ic_cmd = SIOCGUDPSTATS;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof udpstat;
		ioc.ic_dp = (caddr_t) &udpstat;
		if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
			perror("udp_stats: SIOCGUDPSTATS failed");
			close(fd);
			return;
		}
		close(fd);
	} else {
		if (off == 0) {
			return;
		}
		readmem(off, 1, 0, &udpstat, sizeof(udpstat), "udpstat");
	}

	printf("%s:\n\t%u incomplete header%s\n", name,
	       udpstat.udps_hdrops, plural(udpstat.udps_hdrops));
	printf("\t%u bad data length field%s\n",
	       udpstat.udps_badlen, plural(udpstat.udps_badlen));
	printf("\t%u bad checksum%s\n",
	       udpstat.udps_badsum, plural(udpstat.udps_badsum));
/*
	printf("\t%u full socket%s\n",
	       udpstat.udps_fullsock, plural(udpstat.udps_fullsock));
 */
	printf("\t%u bad port%s\n",
	       udpstat.udps_noports, plural(udpstat.udps_noports));
	printf("\t%u input packet%s delivered\n",
	       udpstat.udps_indelivers, plural(udpstat.udps_indelivers));
	printf("\t%u system error%s during input\n",
	       udpstat.udps_inerrors, plural(udpstat.udps_inerrors));
	printf("\t%u packet%s sent\n",
	       udpstat.udps_outtotal, plural(udpstat.udps_outtotal));
}

/*
 * Dump IP statistics structure. 
 */
ip_stats(off, name, coredump)
	off_t           off;
	char           *name;
	int		coredump;
{
	struct ipstat   ipstat;
	int	fd;
	struct	strioctl ioc;

	if (!coredump) {
		if ((fd = open("/dev/ip", O_RDONLY)) < 0)  {
			perror("ip_stats: open /dev/ip failed"); 
			return;
		}

		ioc.ic_cmd = SIOCGIPSTATS;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof ipstat;
		ioc.ic_dp = (caddr_t) &ipstat;
		if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
			perror("ip_stats: SIOCGIPSTATS failed");
			close(fd);
			return;
		}
		close(fd);
	} else {
		if (off == 0) {
			return;
		}
		readmem(off, 1, 0, &ipstat, sizeof(ipstat), "ipstat");
	}

	printf("%s:\n\t%u total packets received\n", name,
	       ipstat.ips_total);
	printf("\t%u bad header checksum%s\n",
	       ipstat.ips_badsum, plural(ipstat.ips_badsum));
	printf("\t%u with size smaller than minimum\n", ipstat.ips_tooshort);
	printf("\t%u with data size < data length\n", ipstat.ips_toosmall);
	printf("\t%u with header length < data size\n", ipstat.ips_badhlen);
	printf("\t%u with data length < header length\n", ipstat.ips_badlen);
	printf("\t%u with unknown protocol\n", ipstat.ips_unknownproto);
	printf("\t%u fragment%s received\n",
	       ipstat.ips_fragments, plural(ipstat.ips_fragments));
	printf("\t%u fragment%s dropped (dup or out of space)\n",
	       ipstat.ips_fragdropped, plural(ipstat.ips_fragdropped));
	printf("\t%u fragment%s dropped after timeout\n",
	       ipstat.ips_fragtimeout, plural(ipstat.ips_fragtimeout));
	printf("\t%u packet%s reassembled\n",
	       ipstat.ips_reasms, plural(ipstat.ips_reasms));
	printf("\t%u packet%s forwarded\n",
	       ipstat.ips_forward, plural(ipstat.ips_forward));
	printf("\t%u packet%s not forwardable\n",
	       ipstat.ips_cantforward, plural(ipstat.ips_cantforward));
	printf("\t%u no routes\n", ipstat.ips_noroutes);
	printf("\t%u redirect%s sent\n",
	       ipstat.ips_redirectsent, plural(ipstat.ips_redirectsent));
	printf("\t%u system error%s during input\n",
	       ipstat.ips_inerrors, plural(ipstat.ips_inerrors));
	printf("\t%u packet%s delivered\n",
	       ipstat.ips_indelivers, plural(ipstat.ips_indelivers));
	printf("\t%u total packet%s sent\n",
	       ipstat.ips_outrequests, plural(ipstat.ips_outrequests));
	printf("\t%u system error%s during output\n",
	       ipstat.ips_outerrors, plural(ipstat.ips_outerrors));
	printf("\t%u packet%s fragmented\n",
	       ipstat.ips_pfrags, plural(ipstat.ips_pfrags));
	printf("\t%u packet%s not fragmentable\n",
	       ipstat.ips_fragfails, plural(ipstat.ips_fragfails));
	printf("\t%u fragment%s created\n",
	       ipstat.ips_frags, plural(ipstat.ips_frags));
}

static char    *icmpnames[] = {
			       "echo reply",
			       "#1",
			       "#2",
			       "destination unreachable",
			       "source quench",
			       "routing redirect",
			       "#6",
			       "#7",
			       "echo",
			       "#9",
			       "#10",
			       "time exceeded",
			       "parameter problem",
			       "time stamp",
			       "time stamp reply",
			       "information request",
			       "information request reply",
			       "address mask request",
			       "address mask reply",
};

/*
 * Dump ICMP statistics. 
 */
icmp_stats(off, name, coredump)
	off_t           off;
	char           *name;
	int		coredump;
{
	struct icmpstat icmpstat;
	register int    i, first;

	int	fd;
	struct	strioctl ioc;

	if (!coredump) {
		if ((fd = open("/dev/icmp", O_RDONLY)) < 0) {
			perror("icmp_stats: open /dev/icmp failed");
			return;
		}

		ioc.ic_cmd = SIOCGICMPSTATS;
		ioc.ic_timout = 0;
		ioc.ic_len = sizeof icmpstat;
		ioc.ic_dp = (caddr_t) &icmpstat;
		if (ioctl(fd, I_STR, (caddr_t) &ioc) < 0) {
			perror("icmp_stats: SIOCGICMPSTATS failed");
			close(fd);
			return;
		}
		close(fd);
	} else {
		if (off == 0) {
			return;
		}
		readmem(off, 1, 0, &icmpstat, sizeof(icmpstat), "icmpstat");
	}

	printf("%s:\n\t%u call%s to icmp_error\n", name,
	       icmpstat.icps_error, plural(icmpstat.icps_error));
	printf("\t%u error%s not generated because old message was icmp\n",
	       icmpstat.icps_oldicmp, plural(icmpstat.icps_oldicmp));
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat.icps_outhist[i] != 0) {
			if (first) {
				printf("\tOutput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %u\n", icmpnames[i],
			       icmpstat.icps_outhist[i]);
		}
	printf("\t%u message%s with bad code fields\n",
	       icmpstat.icps_badcode, plural(icmpstat.icps_badcode));
	printf("\t%u message%s < minimum length\n",
	       icmpstat.icps_tooshort, plural(icmpstat.icps_tooshort));
	printf("\t%u bad checksum%s\n",
	       icmpstat.icps_checksum, plural(icmpstat.icps_checksum));
	printf("\t%u message%s with bad length\n",
	       icmpstat.icps_badlen, plural(icmpstat.icps_badlen));
	for (first = 1, i = 0; i < ICMP_MAXTYPE + 1; i++)
		if (icmpstat.icps_inhist[i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %u\n", icmpnames[i],
			       icmpstat.icps_inhist[i]);
		}
	printf("\t%u message response%s generated\n",
	       icmpstat.icps_reflect, plural(icmpstat.icps_reflect));
	printf("\t%u message%s received\n",
	       icmpstat.icps_intotal, plural(icmpstat.icps_intotal));
	printf("\t%u message%s sent\n",
	       icmpstat.icps_outtotal, plural(icmpstat.icps_outtotal));
	printf("\t%u system error%s during output\n",
	       icmpstat.icps_outerrors, plural(icmpstat.icps_outerrors));
}

/*
 * Pretty print an Internet address (net address + port). If the nflag was
 * specified, use numbers instead of names. 
 */
inetioctlprint(in, port, proto)
	register struct in_addr *in;
	int             port;
	char           *proto;
{
	struct servent *sp = 0;
	char            line[80], *cp, *strchr();
	int             width;

	(void)sprintf(line, "%.*s.", (Aflag && !nflag) ? 12 : 16, inetname(in));
	cp = strchr(line, '\0');
	if (!nflag && port)
		sp = getservbyport(port, proto);
	if (sp || port == 0)
		(void)sprintf(cp, "%.8s", sp ? sp->s_name : "*");
	else
		(void)sprintf(cp, "%d", ntohs((u_short) port));
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}

inetprint(in, port, proto)
	register struct in_addr *in;
	u_short         port;
	char           *proto;
{
	struct servent *sp = 0;
	char            line[1024], *cp, *strchr();
	int             width;

	(void)sprintf(line, "%.*s.", (Aflag && !nflag) ? 12 : 16, inetname(*in));
	cp = strchr(line, '\0');
	if (!nflag && port)
		sp = getservbyport((int)port, proto);
	if (sp || port == 0)
		(void)sprintf(cp, "%.8s", sp ? sp->s_name : "*");
	else
		(void)sprintf(cp, "%u", ntohs((u_short) port) & 0xffff);
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}

/*
 * Construct an Internet address representation. If the nflag has been
 * supplied, give numeric value, otherwise try for symbolic name. 
 */
char           *
inetname(in)
	struct in_addr  in;
{
	register char  *cp;
	struct hostent *hp;
	static char     line[MAXHOSTNAMELEN + 1];
	struct netent  *np;
	static char     domain[MAXHOSTNAMELEN + 1];
	static int      first = 1;
	extern char    *strchr();

	if (first && !nflag) {
		first = 0;
		if (gethostname(domain, MAXHOSTNAMELEN) == 0 &&
		    (cp = strchr(domain, '.')))
			(void) strcpy(domain, cp + 1);
		else
			domain[0] = 0;
	}
	cp = 0;
	if (!nflag && in.s_addr != INADDR_ANY) {
		int             net = inet_netof(in);
		int             lna = inet_lnaof(in);

		if (lna == INADDR_ANY) {
			np = getnetbyaddr(net, AF_INET);
			if (np)
				cp = np->n_name;
		}
		if (cp == 0) {
			hp = gethostbyaddr((char *)&in, sizeof(in), AF_INET);
			if (hp) {
				if ((cp = strchr(hp->h_name, '.')) &&
#if 0
				    /*
				     * XXX - strcasecmp will be in the new
				     * libsocket
				     */
				    !strcasecmp(cp + 1, domain))
#else
				    !strcmp(cp + 1, domain))
#endif
					*cp = 0;
				cp = hp->h_name;
			}
		}
	}
	if (in.s_addr == INADDR_ANY)
		strcpy(line, "*");
	else if (cp)
		strcpy(line, cp);
	else {
		in.s_addr = ntohl(in.s_addr);
#define C(x)	((x) & 0xff)
		(void)sprintf(line, "%u.%u.%u.%u", C(in.s_addr >> 24),
			C(in.s_addr >> 16), C(in.s_addr >> 8), C(in.s_addr));
	}
	return (line);
}
