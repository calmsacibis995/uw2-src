/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/netio.c	1.6.4.2"
#ident "@(#)netio.c	1.11 'attmail mail(1) command'"

#include "libmail.h"
#include "smtp.h"
#include "miscerrs.h"
#include "s5sysexits.h"
#ifdef PRIV
# include <priv.h>
#endif

#ifdef NOBOMB
#define bomb exit
#endif

extern void bomb proto((int));

extern const char *progname;

int
tgets(line, size, fi)		/* fgets from TCP */
char *line;
int size;
FILE *fi;
{
	register char *cr, *rp;

	*line = 0;
#ifdef PRIV
	procprivl(SETPRV, DEV_W, (priv_t)0);
	rp = fgets(line, size, fi);
	procprivl(CLRPRV, DEV_W, (priv_t)0);
#else
	rp = fgets(line, size, fi);
#endif
	if (ferror(fi)) {
		pfmt(stderr, MM_ERROR, ":54:error reading from smtp: %s\n", Strerror(errno));
		bomb(EX_IOERR);
	}
	if (feof(fi)) {
		pfmt(stderr, MM_ERROR, ":55:read eof from smtp: %s\n", Strerror(errno));
		bomb(EX_IOERR);
	}
	if (rp==NULL) {
		pfmt(stderr, MM_ERROR, ":55:read eof from smtp: %s\n", Strerror(errno));
		bomb(EX_IOERR);
	}

	/* convert \r\n -> \n */
	cr = line + strlen(line) - 2;
	if (cr >= line && *cr == '\r' && *(cr+1) == '\n') {	/* CRLF there? */
		*cr++ = '\n';
		*cr = 0;
	} else				/* no CRLF present */
		cr += 2;		/* point at NUL byte */

	if (feof(fi)) {
		pfmt(stderr, MM_ERROR, ":55:read eof from smtp: %s\n", Strerror(errno));
		bomb(EX_IOERR);
	}
	return cr - line;
}

int
tputs(line, fo)			/* fputs to TCP */
char *line;
FILE *fo;
{
	char buf[MAXSTR];
	register char *nl;

	(void) strcpy(buf, line);
	/* replace terminating \n by \r\n */
	nl = buf + strlen(buf) - 1;		/* presumably \n */
	if (nl >= buf && *nl=='\n') {		/* if it is ... */
		*nl++ = '\r';
		*nl++ = '\n';
		*nl = 0;
	} /* else
		printf("%s: unterminated line: <%s>\n", progname, buf); */

#ifdef PRIV
	procprivl(SETPRV, DEV_W, (priv_t)0);
	(void) fputs(buf, fo);
	(void) fflush(fo);
	procprivl(CLRPRV, DEV_W, (priv_t)0);
#else
	(void) fputs(buf, fo);
	(void) fflush(fo);
#endif
	if (ferror(fo)) {
		pfmt(stderr, MM_ERROR, ":56:error writing to smtp: %s\n", Strerror(errno));
		bomb(EX_IOERR);
	}
	return 0;
}
