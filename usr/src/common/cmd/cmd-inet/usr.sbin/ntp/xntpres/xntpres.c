/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/xntpres/xntpres.c	1.2"
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
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * xntpres - process configuration entries which require use of the resolver
 *
 * This is meant to be run by xntpd on the fly.  It is not guaranteed
 * to work properly if run by hand.  This is actually a quick hack to
 * stave off violence from people who hate using numbers in the
 * configuration file (at least I hope the rest of the daemon is
 * better than this).  Also might provide some ideas about how one
 * might go about autoconfiguring an NTP distribution network.
 *
 * Usage is:
 *   xntpres [-d] [-r] keyid keyfile configuration_data
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>

#include "ntp_fp.h"
#include "ntp.h"
#include "ntp_request.h"

#define	STREQ(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)

#ifndef FD_SET
#define	NFDBITS		32
#define	FD_SETSIZE	32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

/*
 * Each item we are to resolve and configure gets one of these
 * structures defined for it.
 */
struct conf_entry {
	struct conf_entry *ce_next;
	char *ce_name;			/* name we are trying to resolve */
	struct conf_peer ce_config;	/* configuration info for peer */
};
#define	ce_peeraddr	ce_config.peeraddr
#define	ce_hmode	ce_config.hmode
#define	ce_version	ce_config.version
#define	ce_flags	ce_config.flags
#define	ce_keyid	ce_config.keyid

/*
 * confentries is a pointer to the list of configuration entries
 * we have left to do.
 */
struct conf_entry *confentries = NULL;

/*
 * We take an interrupt every thirty seconds, at which time we decrement
 * config_timer and resolve_timer.  The former is set to 2, so we retry
 * unsucessful reconfigurations every minute.  The latter is set to
 * an exponentially increasing value which starts at 2 and increases to
 * 32.  When this expires we retry failed name resolutions.
 *
 * We sleep SLEEPTIME seconds before doing anything, to give the server
 * time to arrange itself.
 */
#define	MINRESOLVE	2
#define	MAXRESOLVE	32
#define	CONFIG_TIME	2
#define	ALARM_TIME	30

#define	SLEEPTIME	2

int config_timer = 0;
int resolve_timer = 0;

int resolve_value;	/* next value of resolve timer */

/*
 * Big hack attack
 */
#define	LOCALHOST	0x7f000001	/* 127.0.0.1, in hex, of course */
#define	SKEWTIME	0x08000000	/* 0.03125 seconds as a l_fp fraction */

/*
 * Select time out.  Set to 2 seconds.  The server is on the local machine,
 * after all.
 */
#define	TIMEOUT_SEC	2
#define	TIMEOUT_USEC	0


/*
 * Input processing.  The data on each line in the configuration file
 * is supposed to consist of entries in the following order
 */
#define	TOK_HOSTNAME	0
#define	TOK_HMODE	1
#define	TOK_VERSION	2
#define	TOK_FLAGS	3
#define	TOK_KEYID	4
#define	NUMTOK		5

#define	MAXLINESIZE	512


/*
 * File descriptor for ntp request code.
 */
int sockfd = -1;

/*
 * Misc. data from argument processing
 */
int removefile = 0;	/* remove configuration file when done */

u_long req_keyid;	/* request keyid */
char *keyfile;		/* file where keys are kept */
char *conffile;		/* name of the file with configuration info */

char *progname;
int debug = 0;
extern char *Version;
extern int errno;

/*
 * main - parse arguments and handle options
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	int c;
	int errflg = 0;
	char *cp;
	FILE *in;
	extern int optind;
	extern char *optarg;
	void bong();
	void readconf();
	void doconfigure();
	void checkparent();

	progname = argv[0];

	/*
	 * Better get syslog open early since stderr messages are likely
	 * ending up in the twilight zone
	 */
	cp = rindex(argv[0], '/');
	if (cp == 0)
		cp = argv[0];
	else
		cp++;

#ifndef	LOG_DAEMON
	openlog(cp, LOG_PID);
#else

#ifndef	LOG_NTP
#define	LOG_NTP	LOG_DAEMON
#endif
	openlog(cp, LOG_PID | LOG_NDELAY, LOG_NTP);
#ifdef	DEBUG
	if (debug)
		setlogmask(LOG_UPTO(LOG_DEBUG));
	else
#endif	/* DEBUG */
		setlogmask(LOG_UPTO(LOG_INFO));
#endif	/* LOG_DAEMON */

	syslog(LOG_INFO, Version);

	while ((c = getopt(argc, argv, "dr")) != EOF)
		switch (c) {
		case 'd':
			++debug;
			break;
		case 'r':
			++removefile;
			break;
		default:
			errflg++;
			break;
		}
	if (errflg || (optind + 3) != argc) {
		(void) fprintf(stderr,
		    "usage: %s [-d] [-r] keyid keyfile conffile\n", progname);
		syslog(LOG_ERR, "exiting due to usage error");
		exit(2);
	}

	if (!atouint(argv[optind], &req_keyid)) {
		syslog(LOG_ERR, "undecodeable keyid %s", argv[optind]);
		exit(1);
	}

	keyfile = argv[optind+1];
	conffile = argv[optind+2];

	/*
	 * Make sure we have the key we need
	 */
	if (!authreadkeys(keyfile))
		exit(1);
	if (!authhavekey(req_keyid)) {
		syslog(LOG_ERR, "request keyid %lu not found in %s",
		    req_keyid, keyfile);
		exit(1);
	}

	/*
	 * Read the configuration info
	 */
	if ((in = fopen(conffile, "r")) == NULL) {
		syslog(LOG_ERR, "can't open configuration file %s: %m",
		    conffile);
		exit(1);
	}
	readconf(in, conffile);
	(void) fclose(in);
	if (removefile)
		(void) unlink(conffile);

	/*
	 * Sleep a little to make sure the server is completely up
	 */
	sleep(SLEEPTIME);

	/*
	 * Make a first cut at resolving the bunch
	 */
	doconfigure(1);
	if (confentries == NULL)
		exit(0);		/* done that quick */
	
	/*
	 * Here we've got some problem children.  Set up the timer
	 * and wait for it.
	 */
	resolve_value = resolve_timer = MINRESOLVE;
	config_timer = CONFIG_TIME;
	(void) sigset(SIGALRM, bong);
	alarm(ALARM_TIME);

	for (;;) {
		if (confentries == NULL)
			exit(0);
		checkparent();
		if (resolve_timer == 0) {
			if (resolve_value < MAXRESOLVE)
				resolve_value <<= 1;
			resolve_timer = resolve_value;
			config_timer = CONFIG_TIME;
			doconfigure(1);
			continue;
		} else if (config_timer == 0) {
			config_timer = CONFIG_TIME;
			doconfigure(0);
			continue;
		}
		/*
		 * There is a race in here.  Is okay, though, since
		 * all it does is delay things by 30 seconds.
		 */
		(void) pause();
	}
}


/*
 * bong - service and reschedule an alarm() interrupt
 */
void
bong()
{
	if (config_timer > 0)
		config_timer--;
	if (resolve_timer > 0)
		resolve_timer--;
	alarm(ALARM_TIME);
}


/*
 * checkparent - see if our parent process is still running
 */
void
checkparent()
{
	/*
	 * If our parent (the server) has died we will have been
	 * inherited by init.  If so, exit.
	 */
	if (getppid() == 1) {
		syslog(LOG_INFO, "parent died before we finished, exiting");
		exit(0);
	}
}


/*
 * removeentry - we are done with an entry, remove it from the list
 */
void
removeentry(entry)
	struct conf_entry *entry;
{
	register struct conf_entry *ce;

	ce = confentries;
	if (ce == entry) {
		confentries = ce->ce_next;
		return;
	}

	while (ce != NULL) {
		if (ce->ce_next == entry) {
			ce->ce_next = entry->ce_next;
			return;
		}
		ce = ce->ce_next;
	}
}


/*
 * addentry - add an entry to the configuration list
 */
void
addentry(name, mode, version, flags, keyid)
	char *name;
	int mode;
	int version;
	int flags;
	u_long keyid;
{
	register char *cp;
	register struct conf_entry *ce;
	int len;
	extern char *emalloc();

	len = strlen(name) + 1;
	cp = emalloc((unsigned)len);
	bcopy(name, cp, len);

	ce = (struct conf_entry *)emalloc(sizeof(struct conf_entry));
	ce->ce_name = cp;
	ce->ce_peeraddr = 0;
	ce->ce_hmode = (u_char)mode;
	ce->ce_version = (u_char)version;
	ce->ce_flags = (u_char)flags;
	ce->ce_keyid = htonl(keyid);
	ce->ce_next = NULL;

	if (confentries == NULL) {
		confentries = ce;
	} else {
		register struct conf_entry *cep;

		for (cep = confentries; cep->ce_next != NULL;
		    cep = cep->ce_next)
			/* nothing */;
		cep->ce_next = ce;
	}
}


/*
 * findhostaddr - resolve a host name into an address
 *
 * The routine sticks the address into the entry's ce_peeraddr if it
 * gets one.  It returns 1 for "success" and 0 for an uncorrectable
 * failure.  Note that "success" includes try again errors.  You can
 * tell that you got a try again since ce_peeraddr will still be zero.
 */
int
findhostaddr(entry)
	struct conf_entry *entry;
{
	struct hostent *hp;

	checkparent();		/* make sure our guy is still running */

	hp = gethostbyname(entry->ce_name);

	if (hp == NULL) {
#ifndef NODNS
		/*
		 * If the resolver is in use, see if the failure is
		 * temporary.  If so, return success.
		 */
		extern int h_errno;

		if (h_errno == TRY_AGAIN)
			return 1;
#endif
		return 0;
	}

	/*
	 * Use the first address.  We don't have any way to
	 * tell preferences and older gethostbyname() implementations
	 * only return one.
	 */
	(void) bcopy(hp->h_addr, (char *)&(entry->ce_peeraddr),
	    sizeof(struct in_addr));
	return 1;
}


/*
 * openntp - open a socket to the ntp server
 */
void
openntp()
{
	struct sockaddr_in saddr;

	if (sockfd >= 0)
		return;
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		syslog(LOG_ERR, "socket() failed: %m");
		exit(1);
	}

	bzero((char *)&saddr, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(NTP_PORT);		/* trash */
	saddr.sin_addr.s_addr = htonl(LOCALHOST);	/* garbage */


	/*
	 * Make the socket non-blocking.  We'll wait with select()
	 */
	if (fcntl(sockfd, F_SETFL, O_NDELAY) == -1) {
		syslog(LOG_ERR, "fcntl(O_NDELAY) failed: %m");
		exit(1);
	}

	if (connect(sockfd, (char *)&saddr, sizeof(saddr)) == -1) {
		syslog(LOG_ERR, "connect() failed: %m");
		exit(1);
	}
}


/*
 * request - send a configuration request to the server, wait for a response
 */
int
request(conf)
	struct conf_peer *conf;
{
	fd_set fdset;
	struct timeval tvout;
	struct req_pkt reqpkt;
	l_fp ts;
	int n;

	checkparent();		/* make sure our guy is still running */

	if (sockfd < 0)
		openntp();
	
	/*
	 * Try to clear out any previously received traffic so it
	 * doesn't fool us.  Note the socket is nonblocking.
	 */
	while (read(sockfd, (char *)&reqpkt, REQ_LEN_MAC) > 0)
		/* nothing */;

	/*
	 * Make up a request packet with the configuration info
	 */
	bzero((char *)&reqpkt, sizeof(reqpkt));

	reqpkt.rm_vn_mode = RM_VN_MODE(0, 0);
	reqpkt.auth_seq = AUTH_SEQ(1, 0);	/* authenticated, no seq */
	reqpkt.implementation = IMPL_XNTPD;	/* local implementation */
	reqpkt.request = REQ_CONFIG;		/* configure a new peer */
	reqpkt.err_nitems = ERR_NITEMS(0, 1);	/* one item */
	reqpkt.mbz_itemsize = MBZ_ITEMSIZE(sizeof(struct conf_peer));
	bcopy((char *)conf, reqpkt.data, sizeof(struct conf_peer));
	reqpkt.keyid = htonl(req_keyid);

	auth1crypt(req_keyid, (u_long *)&reqpkt, REQ_LEN_NOMAC);
	gettstamp(&ts);
	M_ADDUF(ts.l_ui, ts.l_uf, SKEWTIME);
	HTONL_FP(&ts, &reqpkt.tstamp);
	auth2crypt(req_keyid, (u_long *)&reqpkt, REQ_LEN_NOMAC);

	/*
	 * Done.  Send it.
	 */
	n = write(sockfd, (char *)&reqpkt, REQ_LEN_MAC);
	if (n < 0) {
		syslog(LOG_ERR, "send to NTP server failed: %m");
		return 0;	/* maybe should exit */
	}

	/*
	 * Wait for a response.  A weakness of the mode 7 protocol used
	 * is that there is no way to associate a response with a
	 * particular request, i.e. the response to this configuration
	 * request is indistinguishable from that to any other.  I should
	 * fix this some day.  In any event, the time out is fairly
	 * pessimistic to make sure that if an answer is coming back
	 * at all, we get it.
	 */
	for (;;) {
		FD_ZERO(&fdset);
		FD_SET(sockfd, &fdset);
		tvout.tv_sec = TIMEOUT_SEC;
		tvout.tv_usec = TIMEOUT_USEC;

		n = select(sockfd + 1, &fdset, (fd_set *)0,
		    (fd_set *)0, &tvout);

		if (n <= 0) {
			if (n < 0)
				syslog(LOG_ERR, "select() fails: %m");
			return 0;
		}

		n = read(sockfd, (char *)&reqpkt, REQ_LEN_MAC);
		if (n <= 0) {
			if (n < 0) {
				syslog(LOG_ERR, "read() fails: %m");
				return 0;
			}
			continue;
		}

		/*
		 * Got one.  Check through to make sure it is what
		 * we expect.
		 */
		if (n < RESP_HEADER_SIZE) {
			syslog(LOG_ERR, "received runt response (%d octets)",
			    n);
			continue;
		}

		if (!ISRESPONSE(reqpkt.rm_vn_mode)) {
#ifdef DEBUG
			if (debug > 1)
				printf("received non-response packet\n");
#endif
			continue;
		}

		if (ISMORE(reqpkt.rm_vn_mode)) {
#ifdef DEBUG
			if (debug > 1)
				printf("received fragmented packet\n");
#endif
			continue;
		}

		if (INFO_VERSION(reqpkt.rm_vn_mode) != NTP_VERSION
		    || INFO_MODE(reqpkt.rm_vn_mode) != MODE_PRIVATE) {
#ifdef DEBUG
			if (debug > 1)
				printf("version (%d) or mode (%d) incorrect\n",
				    INFO_VERSION(reqpkt.rm_vn_mode),
				    INFO_MODE(reqpkt.rm_vn_mode));
#endif
			continue;
		}

		if (INFO_SEQ(reqpkt.auth_seq) != 0) {
#ifdef DEBUG
			if (debug > 1)
				printf("nonzero sequence number (%d)\n",
				    INFO_SEQ(reqpkt.auth_seq));
#endif
			continue;
		}

		if (reqpkt.implementation != IMPL_XNTPD ||
		    reqpkt.request != REQ_CONFIG) {
#ifdef DEBUG
			if (debug > 1)
				printf(
			    "implementation (%d) or request (%d) incorrect\n",
				    reqpkt.implementation, reqpkt.request);
#endif
			continue;
		}

		if (INFO_NITEMS(reqpkt.err_nitems) != 0 ||
		    INFO_MBZ(reqpkt.mbz_itemsize) != 0 ||
		    INFO_ITEMSIZE(reqpkt.mbz_itemsize != 0)) {
#ifdef DEBUG
			if (debug > 1)
				printf(
			    "nitems (%d) mbz (%d) or itemsize (%d) nonzero\n",
				    INFO_NITEMS(reqpkt.err_nitems),
				    INFO_MBZ(reqpkt.mbz_itemsize),
				    INFO_ITEMSIZE(reqpkt.mbz_itemsize));
#endif
			continue;
		}

		n = INFO_ERR(reqpkt.err_nitems);
		switch (n) {
		case INFO_OKAY:
			/* success */
			return 1;
		
		case INFO_ERR_IMPL:
			syslog(LOG_ERR,
			    "server reports implementation mismatch!!");
			return 0;
		
		case INFO_ERR_REQ:
			syslog(LOG_ERR,
			    "server claims configuration request is unknown");
			return 0;
		
		case INFO_ERR_FMT:
			syslog(LOG_ERR,
			    "server indicates a format error occured(!!)");
			return 0;

		case INFO_ERR_NODATA:
			syslog(LOG_ERR,
		"server indicates no data available (shouldn't happen)");
			return 0;
		
		case INFO_ERR_AUTH:
			syslog(LOG_ERR,
			    "server returns a permission denied error");
			return 0;

		default:
			syslog(LOG_ERR,
			    "server returns unknown error code %d", n);
			return 0;
		}
	}
}


/*
 * nexttoken - return the next token from a line
 */
char *
nexttoken(lptr)
	char **lptr;
{
	register char *cp;
	register char *tstart;

	cp = *lptr;

	/*
	 * Skip leading white space
	 */
	while (*cp == ' ' || *cp == '\t')
		cp++;
	
	/*
	 * If this is the end of the line, return nothing.
	 */
	if (*cp == '\n' || *cp == '\0') {
		*lptr = cp;
		return NULL;
	}
	
	/*
	 * Must be the start of a token.  Record the pointer and look
	 * for the end.
	 */
	tstart = cp++;
	while (*cp != ' ' && *cp != '\t' && *cp != '\n' && *cp != '\0')
		cp++;
	
	/*
	 * Terminate the token with a \0.  If this isn't the end of the
	 * line, space to the next character.
	 */
	if (*cp == '\n' || *cp == '\0')
		*cp = '\0';
	else
		*cp++ = '\0';

	*lptr = cp;
	return tstart;
}


/*
 * readconf - read the configuration information out of the file we
 *	      were passed.  Note that since the file is supposed to be
 *	      machine generated, we bail out at the first sign of trouble.
 */
void
readconf(fp, name)
	FILE *fp;
	char *name;
{
	register int i;
	char *token[NUMTOK];
	u_long intval[NUMTOK];
	int flags;
	char buf[MAXLINESIZE];
	char *bp;

	while (fgets(buf, MAXLINESIZE, fp) != NULL) {

		bp = buf;
		for (i = 0; i < NUMTOK; i++) {
			if ((token[i] = nexttoken(&bp)) == NULL) {
				syslog(LOG_ERR,
				    "tokenizing error in file `%s', quitting",
				    name);
				exit(1);
			}
		}

		for (i = 1; i < NUMTOK; i++) {
			if (!atouint(token[i], &intval[i])) {
				syslog(LOG_ERR,
		 "format error for integer token `%s', file `%s', quitting",
				    token[i], name);
				exit(1);
			}
		}

		if (intval[TOK_HMODE] != MODE_ACTIVE &&
		    intval[TOK_HMODE] != MODE_CLIENT &&
		    intval[TOK_HMODE] != MODE_BROADCAST) {
			syslog(LOG_ERR, "invalid mode (%d) in file %s",
			    intval[TOK_HMODE], name);
			exit(1);
		}

		if (intval[TOK_VERSION] != NTP_VERSION &&
		    intval[TOK_VERSION] != NTP_OLDVERSION) {
			syslog(LOG_ERR, "invalid version (%d) in file %s",
			    intval[TOK_VERSION], name);
			exit(1);
		}

		if ((intval[TOK_FLAGS] & ~(FLAG_AUTHENABLE|FLAG_MINPOLL))
		    != 0) {
			syslog(LOG_ERR, "invalid flags (%d) in file %s",
			    intval[TOK_FLAGS], name);
			exit(1);
		}

		flags = 0;
		if (intval[TOK_FLAGS] & FLAG_AUTHENABLE)
			flags |= CONF_FLAG_AUTHENABLE;
		if (intval[TOK_FLAGS] & FLAG_MINPOLL)
			flags |= CONF_FLAG_MINPOLL;
		
		/*
		 * This is as good as we can check it.  Add it in.
		 */
		addentry(token[TOK_HOSTNAME], (int)intval[TOK_HMODE],
		    (int)intval[TOK_VERSION], flags, intval[TOK_KEYID]);
	}
}


/*
 * doconfigure - attempt to resolve names and configure the server
 */
void
doconfigure(dores)
	int dores;
{
	register struct conf_entry *ce;
	register struct conf_entry *ceremove;

	ce = confentries;
	while (ce != NULL) {
		if (dores && ce->ce_peeraddr == 0) {
			if (!findhostaddr(ce)) {
				syslog(LOG_ERR,
				    "couldn't resolve `%s', giving up on it",
				    ce->ce_name);
				ceremove = ce;
				ce = ceremove->ce_next;
				removeentry(ceremove);
				continue;
			}
		}

		if (ce->ce_peeraddr != 0) {
			if (request(&ce->ce_config)) {
				ceremove = ce;
				ce = ceremove->ce_next;
				removeentry(ceremove);
				continue;
			}
		}
		ce = ce->ce_next;
	}
}
