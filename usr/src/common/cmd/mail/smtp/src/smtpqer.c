/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtpqer.c	1.10.3.3"
#ident "@(#)smtpqer.c	1.27 'attmail mail(1) command'"
/*
 *  Name:
 *	smtpqer - Modified for use as a System V mail(1) surrogate
 *	for the SMTP transport.
 *
 *  Usage (with OLD_MAIL):
 *	/usr/lib/mail/surrcmd/smtpqer -O [-B] [-D] [-N] [-n] [-u] [-a toaddr]
 *		[-d domain] [-H helohost] from_address to_address
 *  Usage (w/o OLD_MAIL):
 *	/usr/lib/mail/surrcmd/smtpqer [-B] [-D] [-N] [-n] [-u] [-a toaddr]
 *		[-d domain] [-H helohost] from_address to_host to ...
 */

#ifdef TLI
#include <netconfig.h>
#include <netdir.h>
#include <tiuser.h>
#else
#include <netdb.h>
#endif
#include "libmail.h"
#include "smtp.h"
#include "mail.h"
#include "xmail.h"
#ifdef SVR4_1
#include <mac.h>
#endif

extern int gotodir proto((char *));
extern void makedata proto((char *, char *, char *));
extern void makectl proto((int, char *, char *, char *, char *, char *, char *));
extern void smtpsched proto((char *, char *, int));
extern void smtplog proto((char *));
extern int mkdatafile proto((char *));
extern void copymsg proto((FILE *, FILE *));
extern int mkctlfile proto((char, char *, char *));
extern int mxnetdir proto((struct netconfig *, struct nd_hostserv *, struct nd_addrlist **));

#define OLD_USAGE ":73:usage: smtpqer -O [-B] [-D] [-N] [-n] [-u] [-a toaddr] [-d domain] [-H helohost] from to\n"
#define NEW_USAGE ":74:usage: smtpqer [-B] [-D] [-N] [-n] [-u] [-a toaddr] [-d domain] [-H helohost] from tohost to...\n"

#define DIRDOMLEVEL 2
#define SPOOLNAMSIZ 12	/* keep spool dir names no longer (12 allows L.name on v9) */

/* globals */
char datafile[1024];
const char *progname = "smtpqer";	/* Needed for logging */
int debug = 0;
int exit_ok		= 0;	/* Mail handled by this surrogate */
int exit_continue	= 1;	/* Continue with next surrogate */
int no_nameserver = 0;
int allowbinary = 0;
char *mimeconvert = 0;
char *	usage = NEW_USAGE;

void bomb proto((char *msg, ...));

extern char spoolsubdir[];

/* interrupt handling */
SIGRETURN
catchhup(notused)
int notused;
{
	bomb(":79:interrupted: HUP\n");
	/*NOTREACHED*/
}

/* interrupt handling */
SIGRETURN
catchint(s)
	int s;
{
	bomb(":80:interrupted: INT\n");
}

#ifdef TLI
int validhost(host)
char *host;
{
	struct nd_hostserv	ndh;
	struct netconfig *	ncp;
	struct nd_addrlist *	nap;
	void *			handle;

	ndh.h_host = host;
	ndh.h_serv = "smtp";

	if ((handle = setnetpath()) != NULL) {
		while ((ncp = getnetpath(handle)) != NULL) {
			if (ncp->nc_semantics == NC_TPI_CLTS)
				continue;
			if (mxnetdir(ncp, &ndh, &nap) != 0)
				continue;
			endnetpath(handle);
			return 1;
		}
	
		endnetpath(handle);
	}
	return 0;
}
#endif

main(argc, argv)
int argc;
char *argv[];
{
	register int c;
	int errflg=0;
	extern int optind;
	extern char *optarg;
	char *p;
	char *domain=0;
	int unixformat=0;
	int norun=0;
	int batch = 0;
	char *helohost=0;
	char *toaddr=0;
	char *from=0;
	string *to=s_new();
	string *msg = s_new();
	int old_mail = 0;
	register int l;
	char host[256];

	signal(SIGHUP, catchhup);
	signal(SIGINT, catchint);
	signal(SIGCLD, SIG_DFL); /* Allow us to wait on children */
#ifdef SVR4_1
	(void) setcat("uxsmtp");
	(void) setlabel("UX:smtpqer");
	(void) setlocale(LC_ALL, "");
	(void) mldmode(MLD_VIRT);
#endif
	while ((c = getopt(argc, argv, "OBC:DNbnua:d:H:")) != EOF)
		switch (c) {
		case 'B':	batch = 1;		break;
		case 'C':	mimeconvert = optarg;	break;
		case 'D':	debug = 1;		break;
		case 'N':	no_nameserver = 1;	break;
		case 'b':	allowbinary = 1;	break;
		case 'n':	norun = 1;		break;
		case 'u':	unixformat = 1;		break;
		case 'a':	toaddr=optarg;		break;
		case 'd':	domain=optarg;		break;
		case 'H':	helohost=optarg;	break;
		case 'O':	old_mail = 1;
				exit_ok = 99;
				exit_continue = 0;
				usage = OLD_USAGE;	break;
		case '?':
		default:
				errflg++;		break;
		}

	if (old_mail) {

		/* There should be two arguments left (who from and to) */
		if (errflg || (argc - optind) < 2) {
			(void) pfmt(stderr, MM_ACTION, usage);
			exit(exit_continue);
		}
		from=argv[optind++];
		if ((p = strchr(argv[optind], '!')) == NULL)
			exit(exit_continue);	/* means it is not for SMTP */

		/* Get host part of the name */
		l = p - argv[optind];
		strncpy(host, argv[optind], l);
		host[l] = '\0';
		argv[optind] = ++p;
	} else {

		if (errflg || (argc - optind) < 3) {
			(void) pfmt(stderr, MM_ACTION, usage);
			exit(exit_continue);
		}
		from=argv[optind++];
		strcpy(host, argv[optind++]);
	}

	/* Is it a valid SMTP host? */
#ifdef TLI
	if (validhost(host) == 0) {
#else
	if (lookup_mx(host) == 0) {
#endif
		(void) pfmt(stderr, MM_INFO, ":36:unknown host <%s>\n", host);
		exit(exit_continue);	/* means it is not for SMTP */
	}

	/* Set domain name (if not set) */
	if (domain == (char *) 0) {
		(void) xsetenv(MAILCNFG);
		domain = maildomain();
	}

	/* Set the HELO name (if not set) */
	if (helohost == NULL) {
		extern char *full_domain_name();
		helohost = full_domain_name(domain);
	}

	for (; optind < argc; optind++) {
		s_append(to, argv[optind]);
		s_append(to, " ");
	}

	/*
	 *  make spool files:
	 * 	C.xxxxxxxxxxxx	- the control file
	 * 	D.xxxxxxxxxxxx	- the data file
	 */
	if(gotodir(host)<0)
		bomb(":30:cannot create spool directory %s%s\n", SMTPQROOT, spoolsubdir);
	makedata(from, domain, s_to_c(to));
	makectl(unixformat, helohost, domain, from, toaddr, host, s_to_c(to));

	/*
	 *  run the queue for the receiver
	 */
	if(!norun)
		smtpsched("Qsmtpsched", spoolsubdir, batch);

	/* Successfully queued, log it */
	msg = s_xappend(msg, "queued message for host <", host, "> user <", s_to_c(to), ">", (char*)0);
	smtplog(s_to_c(msg));
	if (debug)
		fprintf(stderr, "%s\n", s_to_c(msg));
	exit(exit_ok);	/* succesfully queued */
	/*NOTREACHED*/
}

/*
 *  the data file is pre-converted to rfc822
 */
void makedata(from, domain, to)
	char *from;
	char *domain;
	char *to;
{
	int fd;
	FILE *fp;

	/*
	 *  create data file
	 */
	strcpy(datafile, "D.xxxxxxxxxxxx");
	fd = mkdatafile(datafile);
	if(fd<0)
		bomb(":31:creating spool file\n");
	fp = fdopen(fd, "w");

	/*
	 *  copy data
	 */
	clearerr(fp);
	clearerr(stdin);
	if (mimeconvert && cascmp(mimeconvert, "text") != 0) {
		FILE *infp = popen("/usr/lib/mail/surrcmd/mimeto7bit", "r");
		if (infp)
		    copymsg(infp, fp);
		if (!infp || pclose(infp) != 0) {
			unlink(datafile);
			bomb(":82:Cannot convert mail message to 7-bit\n");
		}
	} else
		copymsg(stdin, fp);
	fflush(fp);

	/*
	 *  make sure it worked
	 */
	if(ferror(fp) || ferror(stdin)){
		unlink(datafile);
		bomb(":32:writing data file\n");
	}
	fclose(fp);
}

int binary(buf, n)
char *buf;
int n;
{
	register char *p;

	/* RFC821 specifies that mail data must be 7-bit ascii */
	for (p = buf; p < &buf[n]; p++)
		if (*p & 0x80)
			return 1;
	return 0;
}

/*
 *  just copy input to output
 */
void copymsg(in, out)
	FILE *in;
	FILE *out;
{
	char buf[4096];
	int n;

	while(n=fread(buf, 1, sizeof(buf), in)) {
		if (!allowbinary && binary(buf, n))
			bomb(":33:Binary contents cannot be sent via SMTP\n");
		if(fwrite(buf, 1, n, out)!=n)
			bomb(":32:writing data file\n");
		}
}

/*
 *  Make a control file.  The two lines contain:
 *	<reply-addr>	<dest>
 * 	-H <hello host> -d <domain> <reply_addr> <dest> <recipients>
 */
void makectl(unixformat, helo, domain, from, daddr, dest, to)
char *helo, *domain, *from, *daddr, *dest, *to;
{
	string *msg = s_new();

	s_append(msg, from);
	s_append(msg, " ");
	s_append(msg, dest);
	s_append(msg, "\n");
	if (no_nameserver)
		s_append(msg, "-N ");
	if (unixformat)
		s_append(msg, "-u ");
	if (domain && *domain) {
		s_append(msg, "-d ");
		s_append(msg, domain);
		s_append(msg, " ");
	}
	if (daddr) {
		s_append(msg, "-a ");
		s_append(msg, daddr);
		s_append(msg, " ");
	}
	s_append(msg, "-H ");
	s_append(msg, helo);
	s_append(msg, " ");
	s_append(msg, from);
	s_append(msg, " ");
	s_append(msg, dest);
	s_append(msg, " ");
	s_append(msg, to);
	s_append(msg, "\n");
	if (mkctlfile('C', datafile, s_to_c(msg)) < 0)
		bomb(":35:creating control file\n");
}

/* VARARGS1 PRINTFLIKE1 */
void
#ifdef __STDC__
bomb(char *msg, ...)
#else
# ifdef lint
bomb(Xmsg, va_alist)
char *Xmsg;
va_dcl
# else
bomb(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
	char *msg;
#endif
	va_list args;

#ifndef __STDC__
# ifdef lint
	msg = Xmsg;
# endif
#endif

#ifdef __STDC__
	va_start(args, msg);
#else
	va_start(args);
	msg = va_arg(args, char *);
#endif
	(void) vpfmt(stderr, MM_ERROR, msg, args);
	va_end(args);
	if (datafile[0]!=0)
		unlink(datafile);
	exit(exit_continue);
	/* NOTREACHED */
}
