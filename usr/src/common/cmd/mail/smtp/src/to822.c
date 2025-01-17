/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/to822.c	1.7.3.2"
#ident "@(#)to822.c	1.11 'attmail mail(1) command'"
#include "libmail.h"
#include "smtp.h"
#include "smtp_decl.h"
#include "v9regexp.h"
#include "xmail.h"
#include "header.h"
#include "aux.h"
#include "addrformat.h"
#include "s5sysexits.h"

/* imports */
header hdrs[] = {
	HEADER("Date:"),
	HEADER("From:"),
	HEADER("To:"),
	HEADER("Message-ID:"),
	HEADER("MTS-Message-ID:"),
	HEADER("")
};
#define datehdr hdrs[0]
#define fromhdr hdrs[1]
#define tohdr hdrs[2]
#define msghdr hdrs[3]
#define mtshdr hdrs[4]

/* imported */
extern int getheader proto((char *(*fgetsp) proto((char*, int, FILE*)), FILE *fp));
extern int printheaders proto((int (*fputsp) proto((const char*, FILE*)), FILE *fp, int originalfrom));
extern void printbodies proto((int (*fputtsp) proto((const char*, FILE*)), FILE *fp));
extern string *getaddr proto((char*));
extern int cistrncmp proto((char *, char *, int));
extern char *message_id proto((void));
extern const char *progname;

/* predeclared */
static char *convertdate proto((char*));
static int indomain proto((char*,char*));

static char CTIME_RE[] =	/* R.E. to recognize a un*x ctime(3) date */
	"[ \t]*[A-Z][a-z][a-z][ \t]+[A-Z][a-z][a-z][ \t]+[0-9]+[ \t]+[0-9:]+[ \t]+[A-Z]+[ \t]+[0-9]+.*";
	
/* exported */
int extrafrom = 0;
extern int debug;

extern int
to822(fputsp, in, out, sender, domain, rcvr)
	int (*fputsp)();
	FILE *in, *out;		/* file to output to */
	char *sender;		/* name of sender (already in 822 format) */
	char *domain;
	char *rcvr;
{
	string *buf=s_new();
	string *from=s_new();
	string *date=s_new();
	string *remfrom=s_new();
	string *faddr;
	static regexp *pp=NULL;
	int originalfrom=0;
	regsubexp subexp[10];
	char xbuf[4096];
	int n;
	char *cp;

	/* get UNIX from line */
	if(fgets(xbuf, sizeof xbuf, in)==NULL) {
		pfmt(stderr, MM_ERROR, ":24:empty file.\n");
		return -1;
	}

	/* first line had better be a from */
	if (pp==NULL)
		pp = regcomp(REMFROMRE);
	if (regexec(pp, xbuf, subexp, 10)) {
		append_match(subexp, from, REMSENDERMATCH);
		append_match(subexp, date, REMDATEMATCH);
		append_match(subexp, remfrom, REMSYSMATCH);
	} else {
		pfmt(stderr, MM_ERROR, ":25:From_ line missing or malformed.\n");
		return -1;
	}

	/* get any pre-existing RFC822 header lines */
	if (getheader(fgets, in) < 0) {
		pfmt(stderr, MM_ERROR, ":26:error reading input.\n");
		return -1;
	}

	/*
	 *  Output new message.  If there is an original From: line with
	 *  an address in the requested domain, leave it alone.  Otherwise
	 *  generate a From: line and turn any existing one into
	 *  Original-From:
	 */
	if (fromhdr.line != NULL) {
		faddr = getaddr(HCONTENT(fromhdr));
		if(domain!=NULL && !indomain(s_to_c(faddr), domain)){
			sprintf(xbuf, "%s %s\n", fromhdr.name, sender);
			(*fputsp)(xbuf, out);
			originalfrom = 1;
		}
	} else {
		sprintf(xbuf, "%s %s\n", fromhdr.name, sender);
		(*fputsp)(xbuf, out);
	}
	if (datehdr.line == NULL){
		sprintf(xbuf, "%s %s\n", datehdr.name, convertdate(s_to_c(date)));
		(*fputsp)(xbuf, out);
	} else {
		/* Make sure date is in right format to convert */
		static regexp *date_re = NULL;
		regsubexp se[1];
		char *olddate;

		if (date_re == NULL)
			date_re = regcomp(CTIME_RE);
		olddate = s_to_c(datehdr.line) + datehdr.size;
		if (regexec(date_re, olddate, se, 1)) {
			char *rfc822date = convertdate(olddate);
			s_free(datehdr.line);
			s_restart(datehdr.line);
			s_append(datehdr.line, datehdr.name);
			s_append(datehdr.line, " ");
			s_append(datehdr.line, rfc822date);
			s_append(datehdr.line, "\n");
		}
	}
	if (tohdr.line == NULL && rcvr != NULL){
		sprintf(xbuf, "%s %s\n", tohdr.name, rcvr);
		(*fputsp)(xbuf, out);
	}
	if (msghdr.line == NULL) {
		if (mtshdr.line == NULL)
			sprintf(xbuf, "Message-ID: %s\n", message_id());
		else
			strcpy(xbuf, &(s_to_c(mtshdr.line)[4]));
		(*fputsp)(xbuf, out);
	}
		
	printheaders(fputsp, out, originalfrom);
	(*fputsp)("\n", out);
	printbodies(fputsp, out);
	n = 0;
	while (fgets(xbuf, sizeof(xbuf), in)!=NULL) {
		(*fputsp)(xbuf, out);
		n += strlen(xbuf)+1;
		if (n>1024) {
			if (debug)
				fprintf(stderr, ".");
			n -= 1024;
			setalarm(5 * 60, ":40:timer (%d sec) expired: sending mail data.\n", EX_TEMPFAIL);
		}
	}
	s_free(from);
	s_free(date);

	if (ferror(in))
		return -1;

	return 0;
}

/* juggle date fields
	Unix ctime: Mon Oct 19 19:47:25 EDT 1987
	822 standard: [Mon, ] 19 Oct 87 19:47:25 EDT
		(sometimes the , is omitted; sometimes (Mon) at end)
 */
static char *
convertdate(date)
char *date;
{
	static string *ndate=NULL;
	char *field[6];
	char *sp;
	int i;

	sp = date;
	/* parse the date into fields */
	for (i = 0; i < 6; i++) {		
		while (isspace(*sp) || *sp == ',' || *sp == '-')
			*sp++ = '\0';
		field[i] = sp;
		while (!isspace(*sp) && *sp != ',' && *sp != '-' && *sp != '\0')
			sp++;
	}
	*sp = '\0';
	if (strlen(field[5])==4)
		field[5] += 2;

	/* shuffle the fields into internet format */
	if (ndate==NULL)
		ndate = s_new();
	s_restart(ndate);
	s_append(ndate, field[0]);
	s_append(ndate, ", ");
	s_append(ndate, field[2]);
	s_append(ndate, " ");
	s_append(ndate, field[1]);
	s_append(ndate, " ");
	s_append(ndate, field[5]);
	s_append(ndate, " ");
	s_append(ndate, field[3]);
	s_append(ndate, " ");
	s_append(ndate, field[4]);
	return s_to_c(ndate);
}

/*
 *  return true if addr belongs to domain
 */
static int
indomain(addr, domain)
	char *addr;
	char *domain;
{
	int n, m;
	int punct;

	n = strlen(domain);
	m = strlen(addr);
	if(m<n)
		return 0;
	punct = *(addr+m-n);
	if(punct!='@' && punct!='.')
		return 0;
	return(cistrncmp(addr+m-n+1, domain+1, n-1)==0);
}
