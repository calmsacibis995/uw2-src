/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/converse.c	1.6.4.2"
#ident "@(#)converse.c	1.14 'attmail mail(1) command'"
/*
 * Do the necessary commands for a smtp transfer.  Start by waiting for the
 * connection to open, then send HELO, MAIL, RCPT, and DATA.  Check the
 * reply codes and give up if needed.
 * 
 * This code modified from the MIT UNIX TCP implementation:
 * Copyright 1984 Massachusetts Institute of Technology
 * 
 * Permission to use, copy, modify, and distribute this file
 * for any purpose and without fee is hereby granted, provided
 * that this copyright and permission notice appear on all copies
 * and supporting documentation, the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * program without specific prior permission, and notice be given
 * in supporting documentation that copying and distribution is
 * by permission of M.I.T.  M.I.T. makes no representations about
 * the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 */

#include "libmail.h"
#include "smtp.h"
#include "xmail.h"
#include "smtp_decl.h"
#include "miscerrs.h"
#include "s5sysexits.h"

#define TRUE 1
#define FALSE 0
#define MINUTES * 60
#define HOURS * 60 MINUTES
#define DAYS * 24 HOURS

char	*timemsg;
int	timelim;
int	timeerror;
long 	nbytes;
extern int debug;
FILE	*fi;
FILE	*fo;

extern int batching;
extern int debug;
extern int catch_bombs;
extern const char *progname;
extern void bad_input_file proto((void));
extern void csend proto((char *buf));

void converse(unixformat, from, rcpts, domain, sfi, sfo, mlfd)
char	*from;				/* from address */
namelist *rcpts;			/* to addresses */
char	*domain;
FILE	*sfi;				/* smtp input */
FILE	*sfo;				/* smtp output */
FILE	*mlfd;				/* mail file descriptor */
{
	extern char *helohost;
	extern jmp_buf bomb_jmp;
	char buf[MAXSTR];
	namelist *np;
	int mailfailed;
 	extern int kludgepause;

 	fi = sfi;  fo = sfo;

	(void) signal(SIGALRM, death);

	setalarm(5 MINUTES, ":37:timer (%d sec) expired: initial handshake.\n", EX_TEMPFAIL);
	expect(220, sfi, sfo, EX_PROTOCOL);			/* expect a service ready msg */

 	/*
 	 * This pause is needed when an smtp call gets redialed
 	 * (the research version of the Datakit splice.)  Because of
 	 * a race condition inherent in both features, -inet- must
 	 * not be too eager to send HELO until the connection is set up
 	 * completely.  Unfortunately, the other guy's 220 response
 	 * (above) can arrive before the redialed connection
 	 * is completely processed locally.  Hence, the brief delay.
 	 * We can't fix this without changing (read ``fixing'') Datakit
 	 * everywhere.  This kludge is slightly easier.
 	 */
 	if (kludgepause)
 		sleep(kludgepause);

	(void) sprintf(buf, "HELO %s\n", helohost);
	csend(buf);
	expect(250, sfi, sfo, EX_PROTOCOL);			/* expect an OK */

	while (from != NULL) {
		mailfailed = 0;	/* Set if longjmp() called */
		if (batching)
			catch_bombs = 1;
		if (setjmp(bomb_jmp) == 0) {
			(void) strcpy(buf, "MAIL FROM:<");
			(void) strcat(buf, from);
			(void) strcat(buf, ">\n");
			csend(buf);
			setalarm(10 MINUTES, ":38:timer (%d sec) expired: response to MAIL FROM/RCPT TO.\n", EX_TEMPFAIL);
			expect(250, sfi, sfo, EX_PROTOCOL);	/* expect OK */

			for (np = rcpts; np != NULL; np = np->next) {
				(void) strcpy(buf, "RCPT TO:<");
				(void) strcat(buf, np->name);
				(void) strcat(buf, ">\n");
				csend(buf);
				expect(250, sfi, sfo, EX_NOUSER);	/* expect OK */
				}
			setalarm(10 MINUTES, ":39:timer (%d sec) expired: response to DATA.\n", EX_TEMPFAIL);
			csend("DATA\n");
			expect(354, sfi, sfo, EX_PROTOCOL);
			setalarm(10 MINUTES, ":40:timer (%d sec) expired: sending mail data.\n", EX_TEMPFAIL);
			do_data(unixformat, mlfd, sfo, from, rcpts, domain);
			setalarm(1 HOURS, ":41:timer (%d sec) expired: expecting delivery ack.\n", EX_TEMPFAIL);
			expect(250, sfi, sfo, EX_PROTOCOL);
		} else
			mailfailed = 1;	/* bomb() was called */

		from = NULL;
		catch_bombs = 0;
		if (batching) {
			char *junk;
			int ijunk;

			/* Handle current file */
			setalarm(0, "", 0);
			/* Don't remove this file if bomb() was called */
			if (mailfailed == 0)
				rm_file();

			/* Send RSET */
			setalarm(5 MINUTES, ":42:timer (%d sec) expired: response to RSET.\n", EX_TEMPFAIL);
			csend("RSET\n");
			expect(250, sfi, sfo, EX_PROTOCOL);

			/* Setup new control file */
			donext(&unixformat, &from, &rcpts, &domain, mlfd, &junk, &junk, &ijunk);
		}
	}

	setalarm(5 MINUTES, "response to QUIT", 0);
	csend("QUIT\n");
	setalarm(5 MINUTES, ":43:timer (%d sec) expired: response to QUIT.\n", 0);
	expect(221, sfi, sfo, 0);			/* who cares? (Some do)*/
	exit(0);	/* ???? added in '93 */
}

/*
 *  escape '.'s at the beginning of lines and turn newlines into
 *  \r\n's.
 */
static char smlastchar;

smfputs(str, fp)
	char *str;
	FILE *fp;
{
	register char *cp;

	/*
	 *  escape a leading dot
	 */
	if(smlastchar=='\n' && str[0]=='.') {
		fputc('.', fp);
		nbytes++;
	}

	/*
	 *  output the line
	 */
	for(cp=str; *cp; cp++){
		if(*cp=='\n') {
			nbytes++;
			putc('\r', fp);
		}
		nbytes++;
		putc(*cp, fp);
	}
	if(cp!=str)
		smlastchar = *(cp-1);
}


/*
 * Send the data from the specified mail file out on the current smtp
 * connection.  Do the appropriate netascii conversion and hidden '.'
 * padding.  Send the <CRLF>.<CRLF> at completion.
 */
void do_data(unixformat, sfi, sfo, from, rcpts, domain)
	register FILE *sfi;		/* mail file descriptor */
	register FILE *sfo;		/* smtp files */
	char *from;
	namelist *rcpts;
	char *domain;
{
	static string *rcvr;
	char buf[4096];
	namelist *p;
	long nchars;

	/*
	 *  turn rcpts into a , list of receivers
	 */
	rcvr = s_reset(rcvr);
	for(p = rcpts; p; p = p->next){
		s_append(rcvr, p->name);
		if(p->next)
			s_append(rcvr, ", ");
	}

	/*
	 *  send data to output
	 */
	setalarm(5 MINUTES, ":44:timer (%d sec) expired: start sending mail data.\n", EX_TEMPFAIL);
	nbytes = 0;
	smlastchar = '\n';
	if(unixformat){
		nchars = 0;
		while(fgets(buf, sizeof(buf), sfi)!=NULL) {
			smfputs(buf, sfo);
			nchars += strlen(buf)+1;
			if (nchars>1024) {
				nchars -= 1024;
				setalarm(5 MINUTES, ":40:timer (%d sec) expired: sending mail data.\n", EX_TEMPFAIL);
			}
		}
	} else {
		if(to822(smfputs, sfi, sfo, from, domain, s_to_c(rcvr))<0){
			(void) pfmt(stderr, MM_ERROR, ":78:bad input file to %s\n", s_to_c(rcvr));
			Syslog(LOG_INFO, "bad input file to %s\n", s_to_c(rcvr));
			bad_input_file();
			/* Unfortunately, there is no recovering from this */
			/* as SMTP does not provide a way to abort a message */
			/* After the DATA command has been issued */
			catch_bombs = 0;
			bomb(E_DATAERR);
		}
	}

	/*
	 *  terminate the DATA command with \r\n.\r\n
	 */
	if(smlastchar != '\n'){
		fputs("\r\n", sfo);
		nbytes += 2;
	}
	fputs(".\r\n", sfo);
	nbytes += 3;
	Syslog(LOG_INFO, "%s  sent %d bytes to  %s\n",
 		from, nbytes, s_to_c(rcvr));

	/*
	 *  see if we screwed up
	 */
	setalarm(30 MINUTES, ":46:timer (%d sec) expired: finishing data.\n", EX_TEMPFAIL);
	fflush(sfo);
	if (ferror(sfo)) {
		pfmt(stderr, MM_ERROR, ":47:write error in smtp: %s\n", Strerror(errno));
		Syslog(LOG_INFO, "write error finishing data to %s",
			s_to_c(rcvr));
		bomb(E_IOERR);
	}
}

/*
 * Expect a reply message with the specified code.  If the specified code
 * is received return TRUE; otherwise print the error message out on the
 * standard output and give up.  Note that the reply can be a multiline
 * message.
 */
void expect(code, sfi, sfo, error)
int	code;
FILE	*sfi, *sfo;
int	error;
{
	int retcd;
	char cmdbuf[BUFSIZ];
	char cbuf[6144];

	cbuf[0] = '\0';
	/* get whole reply */
more:
	while (tgets(cmdbuf, sizeof cmdbuf, sfi) > 0) {
		cmdbuf[131] = '\n';
		cmdbuf[132] = '\0';		/* not too long */
		Syslog(LOG_DEBUG, "---> %s", cmdbuf);
		if (cmdbuf[3] != '-')	/* continuation line? */
			break;		/* no, last line */
		else if (strlen(cmdbuf) + strlen(cbuf) + 4 < sizeof(cbuf)) {
			strcat(cbuf, "<<< ");
			strcat(cbuf, cmdbuf);
		}
	}
	if (sscanf(cmdbuf, "%d", &retcd) != 1 ){
		int l=strlen(cmdbuf)-1;
		if (l>=0 && cmdbuf[l]=='\n')
			cmdbuf[l]='\0';
		(void) fprintf(stderr, "non-numeric command reply (%s)\n", cmdbuf);
		if (error)
			bomb(EX_PROTOCOL);
		else
			return;
	}
	if (retcd == code) {
		return;
	}
	if (retcd/100 == code/100) {
		return;
	}
	Syslog(LOG_NOTICE, "Failed, expecting %d, got %d\n", code, retcd);
	/* print the error line */
	(void) pfmt(stderr, MM_ERROR, ":79:%s<<< %s", cbuf, cmdbuf);
	csend ("QUIT\n");
	if (error)
		if (retcd/100 == 4)
			bomb(EX_TEMPFAIL); /* 4xx make temp fails, no matter what*/
		else
			bomb(error);	/* map smtp errors to mailsys errors */
	return;
}

void setalarm(limit, message, error)
	char *message;
{
	timelim = limit;
	timemsg = message;
	timeerror = error;
	alarm(limit);
}

/* Maximum time to live elapsed.  Die right now. */
SIGRETURN
death(unused)
int unused;
{
	Syslog(LOG_NOTICE, "Timer (%d sec) expired: %s.\n", timelim, timemsg);
	(void) pfmt(stderr, MM_INFO, timemsg, timelim);
	exit(1);
}

SIGRETURN
ignore_signal(){}

void csend(buf)
char *buf;
{
	Syslog(LOG_DEBUG, "<--- %s", buf);
	tputs(buf, fo);
}
