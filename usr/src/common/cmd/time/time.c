/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)time:time.c	1.6.1.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/time/time.c,v 1.1 91/02/28 20:14:08 ccs Exp $"
/*
**	Time a command
*/

#include	<stdio.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<time.h>
#include	<sys/time.h>
#include	<sys/times.h>
#include	<sys/param.h>		/* HZ defined here */
#include	<unistd.h>
#include	<locale.h>
#include	<pfmt.h>

int	pflag = 0;			/* 1 = -p flag specified */

static char posix_var[] = "POSIX2";
static int posix;

main(argc, argv)
char **argv;
{
	struct tms buffer;
	register pid_t p;
	extern	errno;
	extern	char	*sys_errlist[];
	int	status;
	clock_t	before, after;
	int	optsw;		/* switch for while of getopt() */

	(void)setlocale(LC_ALL, "");
        (void)setcat("uxue");
        (void)setlabel("UX:time");
	tzset();

	if (getenv(posix_var) != 0)	{
		posix = 1;
	} else	{
		posix = 0;
	}

	if(argc<=1)
		exit(0);
	while ((optsw = getopt(argc, argv, "p")) != EOF) {
		switch(optsw) {
		case 'p':
			pflag = 1;
			break;
		default:
			pfmt(stderr, MM_ACTION,
			    ":327:Usage:\ttime [-p] command [argument...]\n");
			exit(1);
			/* NOTREACHED */
		}
	}
	before = times(&buffer);
	
	p = fork();
	if(p == (pid_t)-1) {
		pfmt(stderr, MM_ERROR,
			":322:Cannot fork -- try again.\n");
		exit(2);
	}
	if(p == (pid_t)0) {
		int rc;
		execvp(argv[optind], &argv[optind]);
		if (errno == ENOENT) {
			rc = 127;
		} else {
			rc = 126;
		}
	        pfmt(stderr, MM_ERROR, ":6:%s: %s\n",
			sys_errlist[errno], argv[optind]);
		exit(rc);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while(wait(&status) != p);
	if((status & 0377) != '\0')
		pfmt(stderr, MM_ERROR,
			":323:Command terminated abnormally.\n");
	after = times(&buffer);
	fprintf(stderr,"\n");
	if (pflag || posix) {
		printpt(gettxt(":324", "real"), (after-before));
		printpt(gettxt(":325", "user"),
			buffer.tms_utime + buffer.tms_cutime);
		printpt(gettxt(":326", "sys "),
			buffer.tms_stime + buffer.tms_cstime);
	} else {
		printt(gettxt(":324", "real"), (after-before));
		printt(gettxt(":325", "user"),
			buffer.tms_utime + buffer.tms_cutime);
		printt(gettxt(":326", "sys "),
			buffer.tms_stime + buffer.tms_cstime);
	}
	exit(status >> 8);
}

/* number of digits following the radix character */
#define PREC(X) ((((X) > 0) && ((X) < 101)) ? 2 : 3)

printpt(s, a)
char *s;
clock_t a;
{

	int	clk_tck = sysconf(_SC_CLK_TCK);
	int	minutes = (int)(a / (60 * clk_tck));
	float	seconds = ((float)a - (minutes * 60 * clk_tck)) / clk_tck;

	
	fprintf(stderr,s);
	fprintf(stderr, gettxt(":328", " %dm%.*fs\n"), 
			minutes, PREC(clk_tck), seconds);
}
	
/*
The following use of HZ/10 will work correctly only if HZ is a multiple
of 10.  However the only values for HZ now in use are 100 for the 3B
and 60 for other machines.
*/
char quant[] = { HZ/10, 10, 10, 6, 10, 6, 10, 10, 10 };
char *pad  = "000      ";
char *sep  = "\0\0.\0:\0:\0\0";
char *nsep = "\0\0.\0 \0 \0\0";

printt(s, a)
char *s;
clock_t a;
{
	register i;
	char	digit[9];
	char	c;
	int	nonzero;

	for(i=0; i<9; i++) {
		digit[i] = a % quant[i];
		a /= (clock_t)quant[i];
	}
	fprintf(stderr,s);
	nonzero = 0;
	while(--i>0) {
		c = digit[i]!=0 ? digit[i]+'0':
		    nonzero ? '0':
		    pad[i];
		if (c != '\0')
			putc (c, stderr);
		nonzero |= digit[i];
		c = nonzero?sep[i]:nsep[i];
		if (c != '\0')
			putc (c, stderr);
	}
	fprintf(stderr,"\n");
}
