#ident	"@(#)ucb:common/ucbcmd/sendmail/aux/mconnect.c	1.5"
#ident	"$Header: $"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */



/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * mconnect.c - A program to test out SMTP connections.
 * Usage: mconnect [host]
 *  ... SMTP dialog
 *  ^C or ^D or QUIT
 */

# include <stdio.h>
# include <signal.h>
# include <ctype.h>
# include <sgtty.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
#define bcopy(a,b,c) memcpy(b,a,c)

struct sockaddr_in	SendmailAddress;
struct sgttyb		TtyBuf;

main(argc, argv)
	int argc;
	char **argv;
{
	register int s;
	char *host = NULL;
	int pid;
	int on = 1;
	struct servent *sp;
	int raw = 0;
	char buf[1000];
	extern char *index();
	register FILE *f;
	register struct hostent *hp;
	u_long theaddr;
	extern u_long inet_addr();
	extern finis();

	(void) gtty(0, &TtyBuf);
	(void) signal(SIGINT, (void (*)())finis);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		perror("socket");
		exit(-1);
	}
	(void) setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));

	sp = getservbyname("smtp", "tcp");
	if (sp != NULL)
		SendmailAddress.sin_port = sp->s_port;

	while (--argc > 0)
	{
		register char *p = *++argv;

		if (*p == '-')
		{
			switch (*++p)
			{
			  case 'h':		/* host */
				break;

			  case 'p':		/* port */
				SendmailAddress.sin_port = htons(atoi(*++argv));
				argc--;
				break;

			  case 'r':		/* raw connection */
				raw = 1;
				TtyBuf.sg_flags &= ~CRMOD;
				stty(0, &TtyBuf);
				TtyBuf.sg_flags |= CRMOD;
				break;
			}
		}
		else if (host == NULL)
			host = p;
	}
	if (host == NULL)
		host = "localhost";

	hp = gethostbyname(host);
	if (hp == NULL)
	{
		/* Try for dotted pair or whatever */
		theaddr = inet_addr(host);
		SendmailAddress.sin_addr.s_addr = theaddr;
		if (-1 == theaddr) {
			fprintf(stderr, "mconnect: unknown host %s\r\n", host);
			finis();
		}
	} else {
		bcopy(hp->h_addr, &SendmailAddress.sin_addr, hp->h_length);
	}
	SendmailAddress.sin_family = AF_INET;

	printf("connecting to host %s (%s), port %d\r\n", host,
	       inet_ntoa(SendmailAddress.sin_addr),
	       ntohs(SendmailAddress.sin_port));
	if (connect(s, &SendmailAddress, sizeof SendmailAddress) < 0)
	{
		perror("connect");
		exit(-1);
	}

	/* good connection, fork both sides */
	printf("connection open\n");
	pid = fork();
	if (pid < 0)
	{
		perror("fork");
		exit(-1);
	}
	if (pid == 0)
	{
		/* child -- standard input to sendmail */
		int c;

		f = fdopen(s, "w");
		while ((c = fgetc(stdin)) >= 0)
		{
			if (!raw && c == '\n')
				fputc('\r', f);
			fputc(c, f);
			if (c == '\n')
				fflush(f);
		}
		shutdown(s,1);
		sleep(10);
	}
	else
	{
		/* parent -- sendmail to standard output */
		f = fdopen(s, "r");
		while (fgets(buf, sizeof buf, f) != NULL)
		{
			fputs(buf, stdout);
			fflush(stdout);
		}
	}
	finis();
}

finis()
{
	stty(0, &TtyBuf);
	exit(0);
}
