/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/pppstat.c	1.2"
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



#include <stdio.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ppp.h>

#include "pppd.h"

char *program_name;

int	pid, s ;
#define TIMEOUT  4 

/* SIGNAL Processing
 */

/*
 * sig_alm - SIGALM signal handler
 */
sig_alm()
{
	printf("No response from pppd, try later\n");
	syslog(LOG_INFO, "sig_alm");
	close(s);
	exit(0);
}

main (argc,argv)
	int	argc;
	char	*argv[];
{
	msg_t msg;
	int	c, port;
	int	rval,totread;
	struct	ppp_stat stat;
	char *p;

	program_name = strrchr(argv[0],'/');
	program_name = program_name ? program_name + 1 : argv[0];


#if defined(LOG_DAEMON)
	openlog(program_name, LOG_PID|LOG_CONS|LOG_NDELAY, LOG_DAEMON);
#else
	openlog(program_name, LOG_PID);
#endif
	sigset(SIGALRM, sig_alm);

	memset((char *)&msg, 0, sizeof(msg));
	msg.m_type = MSTAT;

	s = ppp_sockinit();
	/* Send a MSTAT message to pppd */
	rval = write(s, (char *)&msg, sizeof(msg));
	if (rval < 0) {
		syslog(LOG_INFO, "write to socket failed: %m");
		printf("%s write to socket fail\n",program_name);
		exit(1);
	}

	/* Prepare to receive a pppstat message from pppd 
	 * We quit after TIMEOUT sec.
	 */	
	totread =0;
	p = (char *)& stat;
	memset(p, 0 , sizeof(stat));

	alarm(TIMEOUT); 
	do {
		if((rval = read(s,p, sizeof(stat) - totread)) <0){
			syslog(LOG_ERR,"read socket fail: %m");
			printf("%s read socket fail\n",program_name);
			close(s);
			exit(1);
		}
		p += rval;
		totread += rval;
	} while (totread < sizeof(stat));

	close(s);

	/*
 	* Dump PPP statistics structure. 
 	*/
	printf("%s:\n", "ppp");
  
	printf("\t%lu outbound connection requests\n",stat.out_req);
	printf("\t%lu inbound connection requests\n",stat.in_req);
	printf("\t%lu connections established\n",stat.estab_con);
	printf("\t%lu connections closed\n",stat.closed_con);
	printf("\t%lu password authentication failures\n",stat.fail_pap);
	printf("\t%lu packets sent\n",stat.opkts);
	printf("\t%lu received packets with bad FCS\n",stat.fcs);
	printf("\t%lu received packets with bad address\n",stat.addr);
	printf("\t%lu received packets with bad control\n",stat.ctrl);
	printf("\t%lu received packets with bad protocol\n",stat.proto);
	printf("\t%lu correct packets received\n",stat.ipkts);
	printf("\t\t%lu packets with bad id field\n",stat.id);
	printf("\t\t%lu loopback packets\n",stat.loopback);
	printf("\t%lu state table errors\n",stat.stattbl);

}
