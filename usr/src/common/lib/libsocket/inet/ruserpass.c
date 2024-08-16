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

#ident	"@(#)libsocket:common/lib/libsocket/inet/ruserpass.c	1.2.9.6"
#ident	"$Header: $"

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

#include "../socketabi.h"
#include <stdio.h>
#include <unistd.h>
#include <pfmt.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <netdb.h>
#include "../libsock_mt.h"

#define	index	strchr

char	*index(), *getenv(), *getpass(), *getlogin();

_ruserpass(host, aname, apass)
	char *host, **aname, **apass;
{

	if (*aname == 0 || *apass == 0)
		rnetrc(host, aname, apass);
	if (*aname == 0) {
		char *myname = getlogin();
		*aname = malloc(16);
		printf(gettxt("uxnsl:193", "Name (%s:%s): "),
		    host, myname);
		fflush(stdout);
		if (read(2, *aname, 16) <= 0)
			exit(1);
		if ((*aname)[0] == '\n')
			*aname = myname;
		else
			if (index(*aname, '\n'))
				*index(*aname, '\n') = 0;
	}
	if (*aname && *apass == 0) {
		printf(gettxt("uxnsl:194", "Password (%s:%s): "),
		    host, *aname);
		fflush(stdout);
		*apass = getpass("");
	}
}

#define	DEFAULT	1
#define	LOGIN	2
#define	PASSWD	3
#define	ACCOUNT 4
#define MACDEF  5
#define	ID	10
#define	MACHINE	11

char	*strcpy();
static struct tokinfo {
	FILE *cfile;
	char tokval[100];
} tokinfo;

static const struct toktab {
		char *tokstr;
		int tval;
} toktab[] = {
	"default",	DEFAULT,
	"login",	LOGIN,
	"password",	PASSWD,
	"account",	ACCOUNT,
	"machine",	MACHINE,
	"macdef",	MACDEF,
	0,		0
};

static struct tokinfo *
get_s_tokinfo()
{

#ifdef _REENTRANT
        struct _s_tsd *key_tbl;
	struct tokinfo *tip;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&tokinfo);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return ((struct tokinfo *)NULL);
	if (key_tbl->token_info_p == NULL) {
		tip = (struct tokinfo *)calloc(1, sizeof(struct tokinfo));
		if (tip == NULL)
			return ((struct tokinfo *)NULL);
		key_tbl->token_info_p = tip;
		tip->cfile = NULL;
	}
	
	return ((struct tokinfo *)key_tbl->token_info_p);
#else /* !_REENTRANT */
	return (&tokinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_token_info(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

static
rnetrc(host, aname, apass)
	char *host, **aname, **apass;
{
	char *hdir, buf[BUFSIZ], *tmp;
	int t, i, c;
	struct stat stb;
	struct tokinfo *tip;

	/* Get thread-specific data */
	if ((tip = get_s_tokinfo()) == NULL)
		return (0);


	hdir = getenv("HOME");
	if (hdir == NULL)
		hdir = ".";
	(void) sprintf(buf, "%s/.netrc", hdir);
	tip->cfile = fopen(buf, "r");
	if (tip->cfile == NULL) {
		if (errno != ENOENT)
			perror(buf);
		return(0);
	}
next:
	while ((t = token())) switch(t) {

	case DEFAULT:
		(void) token();
		continue;

	case MACHINE:
		if (token() != ID || strcmp(host, tip->tokval))
			continue;
		while ((t = token()) && t != MACHINE) switch(t) {

		case LOGIN:
			if (token())
				if (*aname == 0) { 
					*aname 
					  = malloc((unsigned)
						   strlen(tip->tokval) + 1);
					(void) strcpy(*aname, tip->tokval);
				} else {
					if (strcmp(*aname, tip->tokval))
						goto next;
				}
			break;
		case PASSWD:
			if (fstat(fileno(tip->cfile), &stb) >= 0
			    && (stb.st_mode & 077) != 0) {
				pfmt(stderr, MM_ERROR,
			"uxnsl:195:Error - .netrc file not correct mode.\n");
				pfmt(stderr, MM_ACTION,
			"uxnsl:196:Remove password or correct mode.\n");
				return(-1);
			}
			if (token() && *apass == 0) {
				*apass = malloc((unsigned)
						strlen(tip->tokval) + 1);
				(void) strcpy(*apass, tip->tokval);
			}
			break;
		case ACCOUNT:
		case MACDEF:
			(void) token();
			break;
		default:
			pfmt(stderr, MM_ERROR,
			  "uxnsl:197:Unknown .netrc keyword %s\n", tip->tokval);
			break;
		}
		goto done;
	}
done:
	(void) fclose(tip->cfile);
	return(0);
}

static
token()
{
	char *cp;
	int c;
	struct toktab *t;
	struct tokinfo *tip;

	/* Get thread-specific data */
	if ((tip = get_s_tokinfo()) == NULL)
		return (0);

	if (feof(tip->cfile))
		return (0);
	while ((c = getc(tip->cfile)) != EOF &&
	    (c == '\n' || c == '\t' || c == ' ' || c == ','))
		continue;
	if (c == EOF)
		return (0);
	cp = tip->tokval;
	if (c == '"') {
		while ((c = getc(tip->cfile)) != EOF && c != '"') {
			if (c == '\\')
				c = getc(tip->cfile);
			*cp++ = c;
		}
	} else {
		*cp++ = c;
		while ((c = getc(tip->cfile)) != EOF
		    && c != '\n' && c != '\t' && c != ' ' && c != ',') {
			if (c == '\\')
				c = getc(tip->cfile);
			*cp++ = c;
		}
	}
	*cp = 0;
	if (tip->tokval[0] == 0)
		return (0);
	for (t = (struct toktab *)&toktab[0]; t->tokstr; t++)
		if (!strcmp(t->tokstr, tip->tokval))
			return (t->tval);
	return (ID);
}
