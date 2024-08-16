/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_PPP_PPP_H	/* wrapper symbol for kernel use */
#define _NET_INET_PPP_PPP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/ppp/ppp.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
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

#ifdef _KERNEL_HEADERS

#include <net/inet/ppp/pppcnf.h>	 /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEM_USER)

#include <netinet/pppcnf.h>		 /* REQUIRED */

#else

#include <sys/types.h>			/* COMPATIBILITY */
#include <netinet/pppcnf.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * N.B.: For proper interoperability with 4.3 BSD
 * PPPMTU is now a hard limit on input packet size.
 * PPPMTU must be <= CLBYTES - sizeof(struct ifnet *).
 * This interprets to about 800 bytes
 */
/* 
 * PPPMTU is suggested to be a mutiple of 2 + 40 bytes for the IP header
 */
#define	PPPMTU			296
#define	PPP_DEF_MRU		1500

/* define configurable parameters default values */
#define DEF_RESTART_TM          3
#define DEF_MAX_TRM             2
#define DEF_MAX_CNF             10
#define DEF_MAX_FAILURE         10
#define DEF_INACTV_TMOUT        0
#define DEF_MRU                 296
#define DEF_ACCM                0x00000000
#define DEF_PAP_TM		1

#define PERM_CONN 0

#define PPCID_REQ	1
#define PPCID_RSP	2
#define PPCID_NAK	3
#define PPCID_ACK	4
#define PPCID_CLOSE	5
#define PPCID_UP	6
#define PPCID_CNF	7
#define PPCID_PAP	8
#define PPCID_STAT	9
#define PPCID_PERM	10

#define MAX_CONN_PER_PROV 4
#define NO_CONN 0
#define RW_CONN 1
#define R_CONN 2 
#define W_CONN 3

#define IPDEV		"/dev/ip"
#define PPCIDDEV	"/dev/ppcid"
#define PPLOGDEV	"/dev/pplog"
#define PPPDEV		"/dev/ppp"

extern struct ppphostent *getppphostbyname();
extern struct ppphostent *getppphostbyaddr();
extern struct ppphostent *getppphostent();
struct ppp_ppc_inf_ctl_s {
	int		function;
	int		l_index;
};

#define PID_SIZE     64
#define PWD_SIZE     64
#define MSG_SIZE     64
#define LOCAL		2
#define REMOTE		1
#define LOGSIZE		1500

struct	pswdauth {
	char PID[PID_SIZE];
	char PWD[PWD_SIZE];
};

struct ppp_configure {
	int     inactv_tmout;           /* inactivity wait time(minutes) */
	int     restart_tm;             /* restart timer */
	int     max_cnf;                /* max configure retries */
	int     max_trm;                /* max termination retries */
	int     max_failure;            /* max configure-nak retries */
	ushort  mru;                    /* max receive unit */
	ulong   accm;                   /* asyc control character map */
	int     pap;                    /* password authentication */
	int     mgc;                    /* maginc number */
	int     accomp;                 /* addrerss-control field compression */
	int     protcomp;               /* protocol field compression */
	int     ipaddress;              /* ip-address option */
	int     newaddress;             /* rfc1172addr or rfc1332addr */
	int     vjcomp;                 /* vj compression compression */
	int     old_ppp;                /* other side is ISC old ppp */
	int     pap_tmout;              /* PAP timer */
	struct  pswdauth pid_pwd;	/* PID-PSWD pair for PAP */
	int	debug;			/* debug level */
};

struct ppp_stat {
	ulong	out_req;	/* # of outbound connection request */
	ulong	in_req;		/* # of inbound connection request */
	ulong	estab_con;	/* # of connection established */
	ulong	closed_con;	/* # of closed connection */
	ulong	fail_pap;	/* # of PAP failed */
	ulong	opkts;		/* # of output packets */
	ulong	fcs;		/* # of input packets with bad fcs */
	ulong	addr;		/* # of input packets with bad addr */
	ulong	ctrl;		/* # of input packets with bad controls */
	ulong	proto;		/* # of input packets with bad protocol */
	ulong	ipkts;		/* # of input packets */
	ulong	id;		/* # of packets with bad id*/
	ulong	loopback;	/* # of loopback packets */
	ulong	stattbl;	/* # of state table err */
} ;

struct ppphostent {
	struct hostent	*ppp_h_ent;
	char	*tty_line;
	char	*uucp_system;
	struct  ppp_configure ppp_cnf;
};

struct ppp_ppc_inf_dt_s {
	union {
		struct inf_dt1 {
			struct in_ifaddr	di1_ia;
			struct ppp_configure	di1_cnf;
		} s_di1;
		struct inf_dt2 {
			char	di2_ifname[IFNAMSIZ];
			int	di2_ifunit;
		} s_di2;
		struct inf_dt3 {
			char	di3_pid[PID_SIZE];
			char	di3_pwd[PWD_SIZE];
			int	di3_id;
		} s_di3;
		struct inf_dt4 {
			struct	ppp_stat di4_stat;
		} s_di4;
	} u_di;
};

#define di_ia		u_di.s_di1.di1_ia
#define di_ifname	u_di.s_di2.di2_ifname
#define di_ifunit	u_di.s_di2.di2_ifunit
#define di_pid		u_di.s_di3.di3_pid
#define di_pwd		u_di.s_di3.di3_pwd
#define di_id		u_di.s_di3.di3_id
#define di_stat		u_di.s_di4.di4_stat

#define PPP_FCS_ERROR	0x13

/* Ioctl's */
#define PPPIOCTL	('P' << 8)
#define P_SETSPEED	(PPPIOCTL | 0xFE)
#define P_GETLINEDIS	(PPPIOCTL | 0xFD)

/* PPP log message type */
#define PPP_LOG		1
struct ppp_log_ctl_s {
	int	function;
	int	l_index;
};

#define LOGSIZE	1500
struct	ppp_log_dt_s {
	int	arg1;
	int	arg2;
	int	arg3;
	int	arg4;
	char	fmt[LOGSIZE];
};
/*
 * OBSOLETE - These symbols (PPP_HIWAT, CLISTRESERVE and MAX_ENDFRAME)
 * are no longer used but are left here for source compatibility.
 */
#define PPP_HIWAT	1000
#define CLISTRESERVE	1000
#define MAX_ENDFRAME	5

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_PPP_PPP_H */
