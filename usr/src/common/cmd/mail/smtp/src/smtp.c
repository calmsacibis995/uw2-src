/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtp.c	1.7.3.3"
#ident "@(#)smtp.c	1.15 'attmail mail(1) command'"
/*
 * smtp -- client, send mail to remote smtp server
 * TODO:
 *	allow partial delivery to multiple recipients when only some
 *		fail (maybe)
 * 	send stuff from cmds.h instead of hard-coded here
 */

static char USAGE1[] =
	":10:usage: %s [-D] [-N] [-u] [-d domain] [-H helohost] sender targethost recip ...\n";
static char USAGE2[] =
	":11:usage: %s [-D] [-N] ctlfiles ...\n";
static char USAGE3[] =
	":12:%s: invalid control file\n";

#include "libmail.h"
#include "smtp.h"
#include "addrformat.h"
#include "miscerrs.h"
#include "smtp_decl.h"
#include "aux.h"
#include "s5sysexits.h"
#include "xmail.h"
#ifdef SVR4_1
#include <mac.h>
#endif

#ifndef DIALER
#define DIALER		"tcp"
#endif

#ifndef SERVNAME
#define SERVNAME	"smtp"		/* service we wanna talk to */
#endif

extern char *full_domain_name proto((char *domain));

const char *progname;
int debug = 0;
char *helohost;
char *usage_msg;
int no_nameserver = 0;
int kludgepause = 0;

extern int ipcdebug;
static char errstr[128];

/*
 * main - parse arguments and handle options
 */
main(argc, argv)
int argc;
char *argv[];
{
	register int c;
	int errflg = 0;
	int unixformat = 0;
	int filter = 0;
	char *domain = 0;
	char *sender = 0;
	char *host = 0;
	char *addr = 0;
	char *p;
	namelist *recips;
	FILE *sfi, *sfo;
	string *replyaddr=s_new();
	string *hh;

	extern int optind;

	umask(2);

#ifdef SVR4_1
	(void) setcat("uxsmtp");
	(void) setlocale(LC_ALL, "");
	(void) mldmode(MLD_VIRT);
#endif
	signal(SIGCLD, SIG_DFL); /* Allow us to wait on children */
	/*
	 *  Check if we have been invoked as "smtpbatch".
	 *  If so, read arguments from the first control file in the list.
	 */
	p = strrchr(argv[0], '/');
	p = (p == NULL) ? argv[0] : (p+1);
	progname = p;
	Openlog("smtp", LOG_PID, LOG_SMTP);
	setlogmask(LOG_UPTO(DEFAULT_LOG_LEVEL));
	if (strcmp(p, "smtpbatch") == 0) {
#ifdef SVR4_1
		(void) setlabel("UX:smtpbatch");
#endif
		while ((c = getopt(argc, argv, "DN")) != EOF) {
			switch (c) {
			case 'D':
				debug = 1;
				break;
			case 'N':
				no_nameserver = 1;
				break;
			default:
				(void) pfmt(stderr, MM_ACTION, USAGE2, progname);
				bomb(E_USAGE);
			}
		}
		usage_msg = USAGE3;
		if ((argc - optind) <= 0) {
			(void) pfmt(stderr, MM_ACTION, USAGE2, progname);
			bomb(E_USAGE);
		}
		init_batch(argc-optind+1, &argv[optind-1]);
		donext(&unixformat, &sender, &recips, &domain,
		       stdin, &addr, &host, &filter);
	} else {
#ifdef SVR4_1
		(void) setlabel("UX:smtp");
#endif
		/* Process standard argument list */
		usage_msg = USAGE1;
		setupargs(argc, argv, &unixformat, &sender,
			  &recips, &domain, &addr, &host, &filter);
	}


	/*
	 * run as a filter
	 */
	if ( filter ) {
		do_data(unixformat, stdin, stdout, sender, recips, domain);
		exit(0);
	}

	if (sender) {
		/*
		*  open connection
		*/
		setup(addr ? addr : host, &sfi, &sfo);

		/*
		 *  hold the conversation
		 */
		converse(unixformat, sender, recips, domain, sfi, sfo, stdin);
		/* converse terminates with the appropriate exit code */
	}

	exit(0);
}

void setupargs(argc, argv, unixformat, sender, recips, domain, addr, host, filter)
int argc;
char **argv;
int *unixformat;
char **sender;
namelist **recips;
char **domain, **addr, **host;
int *filter;
{
	register int c;
	int errflg = 0;
	string *hh;
	string *replyaddr=s_new();
	extern int optind;
	extern char *optarg;

	optind = 1;		/* Reinitialize getopt(3) -- because it is */
	optarg = (char *)0;	/* used several times when batching */

	*unixformat = 0;
	*sender = *domain = *addr = *host = (char *) 0;

	while ((c = getopt(argc, argv, "ga:uDd:H:fL:KN")) != EOF)
		switch (c) {
		case 'a':	*addr = optarg;		break;
		case 'u':	*unixformat = 1;	break;
		case 'D':	debug = 1;		break;
		case 'N':	no_nameserver = 1;	break;
		case 'd':		/* domain name */
				*domain = optarg;	break;
		case 'H':		/* host for HELLO message */
				helohost = optarg;	break;
		case 'f':	*filter++;		break;
		case 'K':	kludgepause++;		break;
		case 'L':	setloglevel(optarg);	break;
		case '?':
		default:
			errflg++;
			break;
		}
	if (errflg || (argc - optind) < 3) {
		(void) pfmt(stderr, MM_ACTION, usage_msg, progname);
		Syslog(LOG_WARNING, "SMTP illegal usage.");
		bomb(EX_USAGE);
	}

	/*
	 *  figure out what to call ourselves
	 */
	if (helohost==NULL)
		helohost=s_to_c(s_copy(full_domain_name((char*)0)));

	/*
	 *  if there is no domain in the helo host name
	 *  and the -d option is specified, domainify
	 *  the helo host
	 */
	if (strchr(helohost, '.') == 0 && *domain) {
		hh = s_copy(helohost);
		s_append(hh, *domain);
		helohost = s_to_c(hh);
	}

	/*
	 *  put our address onto the reply address
	 */
	if (strchr(argv[optind], '!') == 0 || !*domain){
		s_append(replyaddr, helohost);
		s_append(replyaddr, "!");
	}
	s_append(replyaddr, argv[optind]);
	optind++;

	/*
	 *  convert the arguments to 822 form
	 */
	*sender = convertaddr(s_to_c(replyaddr), *domain ? *domain : "", SOURCEROUTE);
	*host = argv[optind++];
	*recips = newname(convertto(argv[optind++], *unixformat, *host));
	for (; optind < argc; optind++)
		*recips = appendname(*recips, convertto(argv[optind], *unixformat, *host));
}

namelist *
newname(s)
	char *s;
{
	namelist *np;

	np = (namelist *) malloc(sizeof(namelist));
	if (np == NULL) {
		Syslog(LOG_WARNING, "could not alloc (newname)");
		bomb(1);
	}
	np->name = s;
	np->next = NULL;
	return np;
}

/* could add at beginning, but let's maintain original order */
namelist *
appendname(nl, s)
	char *s;
	namelist *nl;
{
	register namelist *tl;

	if (nl == NULL)
		bomb(1);	/* shouldn't happen */
	for (tl=nl; tl->next!=NULL; tl=tl->next)
		;
	tl->next = newname(s);
	return nl;
}

/*
 *  convert a destination address to outgoing format
 *
 *	if unixformat, just leave it alone
 *
 *	if not add the destination host name.
 */
char *
convertto(recip, unixformat, desthost)
	char *recip;
	char *desthost;
{
	static string *buf;

	if(unixformat)
		return recip;
	
	buf = s_reset(buf);
	s_append(buf, desthost);
	s_append(buf, "!");
	s_append(buf, recip);
	return convertaddr(s_to_c(buf), "", SOURCEROUTE);
}


#ifdef NOTYET_FROM_UPAS
/*
 * setup -- setup tcp/ip connection to/from server
 */
void setup(host, sfip, sfop)
	char *host;
	FILE **sfip, **sfop;
{
	int s;
	char *path;
	int localerr;

	path = ipcpath(host, DIALER, SERVNAME);
	Syslog(LOG_DEBUG, "Opening connection to %s\n", path);
	if ((s = ipcopen(path, "")) < 0) {
		extern int ipcerrno;
		extern char syserrstr[];

		char errbuf[256];
		sprintf(errbuf, "SMTP connect error to %s", host);
		ipcperror(errbuf);
		Syslog(LOG_INFO, "%s: %s\n", errbuf, syserrstr);
		bomb(ipcerrno);
	}

	if (((*sfip = fdopen(s, "r")) == (FILE *) NULL) ||
	    ((*sfop = fdopen(s, "w")) == (FILE *) NULL)) {
		perror("setup - fdopen");
		Syslog(LOG_INFO, "setup - fdopen");
		bomb(EX_IOERR);
	}
}
#endif /* NOTYET_FROM_UPAS */
