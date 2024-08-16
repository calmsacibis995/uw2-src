/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libxchoose:rexec.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libxchoose/rexec.c,v 1.3 1994/03/23 21:18:46 jodi Exp $"

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "rexec.c	5.5 (Berkeley) 6/27/88";
#endif /* LIBC_SCCS and not lint */

#include <ctype.h>
#include <sys/types.h>
/*#include <sys/socket.h>*/

#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>

#include <sys/stat.h>

#include <string.h>

extern int errno;

#include	"util.h"
extern char *gettxt();
static ruserpass(char *host, char **aname, char **apass);
static token(void);

rexec(ahost, rport, name, pass, cmd, fd2p)
	char **ahost;
	int rport;
	char *name, *pass, *cmd;
	int *fd2p;
{
	int s;
	char c;

	int						i,lport;
	int  					pid;
	fd_set					reads;
	struct nd_hostserv 		nd_hostserv;
	struct netconfig 		*netconfigp = NULL;
	struct nd_addrlist 		*nd_addrlistp = NULL;
	struct netbuf  			*netbufp = NULL;
	struct t_call			*callptr;
	static void   			*handlep = NULL;
	char					num[8];
	int						hostNameFound;

	if((handlep = setnetpath()) == NULL)
	{
		/*
		 * Print our error string and the reason why setnetpath
		 * failed.
		 */
		nc_perror(gettxt("libxchoosemsgs:17", "rexec: Cannot rewind network selection path"));
		return(-1);
	}
	/*
	 * Find the transport provider for this service
	 * Note:USL has a byte order problem. Need to use 2 to be 0x200 (512).
	 *      Probably they need to do htons.
	 */
	nd_hostserv.h_host = *ahost;
	if ( rport == 512 || rport == 0 || rport == 2 )
	{
		/*
		 * All transports that support the exec utility
		 * on all registered ports.
		 */
		nd_hostserv.h_serv = "exec";
	}
	else
	{
		/*
		 * All transports that support the exec utility
		 * on a single specific port.
		 */
		(void) sprintf(num, "%d", rport);
		nd_hostserv.h_serv = num;
	}
	s = -1;
    
	while((s < 0 ) && (netconfigp = getnetpath(handlep)) != NULL )
	{
		if (netdir_getbyname(netconfigp, &nd_hostserv, &nd_addrlistp) == 0) 
		{
			hostNameFound = TRUE;
			netbufp = nd_addrlistp->n_addrs;
            for(i=0; i< nd_addrlistp->n_cnt; i++)
            {
				/*
	 			* Connect to the server on first address accepted
	 			*/
				if ((s=BindAndConnect(netconfigp,netbufp)) <= 0 )
				{
					netbufp++;
					continue;
				}
				break;
			}
		}
	}
	if ( s < 0 )
	{
		/*
	 	* 	Unable to connect or host not found.
	 	*/
		if ( hostNameFound == TRUE )
			(void)fprintf(stderr,gettxt("libxchoosemsgs:18", "rexec: Not able to connect to %s.\n"),*ahost);
		else
			(void)fprintf(stderr,gettxt("libxchoosemsgs:19", "rexec: Host address unknown for %s.\n"),*ahost);
		if ( nd_addrlistp != NULL )
			(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
		if ( netconfigp != NULL )
			freenetconfigent(netconfigp);
		return(-1);
	}
	if (ioctl(s, I_POP, "timod") < 0) {
		fprintf(stderr, gettxt("libxchoosemsgs:20", "rexec: Streams I_POP failed errno = %d.\n"),errno);
		freenetconfigent(netconfigp);
		t_close(s);
		return (-1);
	}
	if (ioctl(s, I_PUSH, "tirdwr") < 0) {
		fprintf(stderr, gettxt("libxchoosemsgs:21", "rexec: Streams I_PUSH failed errno = %d.\n"),errno);
		freenetconfigent(netconfigp);
		t_close(s);
		return (-1);
	}
	pid = getpid();
	fcntl(s, F_SETOWN, pid);
	/*
	 * Set the user name pass word up if not already.
	 * Reads .netrc file in users home directory.
	 */
	ruserpass(*ahost, &name, &pass);
	
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 ;
		/*
		 *	Bind to a port for connection acceptance.
		 *	Fill in the lport variable so we can pass it to
		 *	the remote process.
		 */
		if ((s2=BindAndListen(netconfigp,&lport)) <= 0 )
		{
			goto bad;
		}
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num)+1) != strlen(num)+1) {
			perror(gettxt("libxchoosemsgs:22", "rexec: write: setting up stderr"));
			(void) close(s2);
			goto bad;
		}
		FD_ZERO(&reads);
		FD_SET(s, &reads);
		FD_SET(s2, &reads);
		errno = 0;
		/*
		 *	Wait for the connection request
		 *	or some error reply.
		 */
		if (select(32, &reads, 0, 0, 0) < 1 ||
		    !FD_ISSET(s2, &reads)) {
			if (errno != 0)
				perror(gettxt("libxchoosemsgs:23", "rexec: select: setting up stderr"));
			else
			    fprintf(stderr,
				gettxt("libxchoosemsgs:24", "rexec: select: protocol failure in circuit setup.\n"));
			(void) close(s2);
			goto bad;
		}
		/*
		 *	Accept a connection on this transport endpoint
		 *  qlen is set to 1. No async event handling here.
		 */
		if ( AcceptConnection(s2,&callptr) < 0 )
		{
			perror(gettxt("libxchoosemsgs:25", "rexec: AcceptConnection for stderr failed.\n"));
			lport = 0;
			goto bad;
		}
		if (ioctl(s2, I_POP, "timod") < 0) {
			perror(gettxt("libxchoosemsgs:26", "rexec: failed to pop timod\n"));
			goto bad;
		}
		if (ioctl(s2, I_PUSH, "tirdwr") < 0) {
			perror(gettxt("libxchoosemsgs:27", "rexec: failed to push tirdwr\n"));
			goto bad;
		}
		*fd2p = s2;
	}
	(void) write(s, name, strlen(name) + 1);
	/* should public key encypt the password here */
	(void) write(s, pass, strlen(pass) + 1);
	(void) write(s, cmd, strlen(cmd) + 1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad;
	}
	return (s);
bad:
	if (lport)
		(void) close(*fd2p);
	(void) close(s);
	return (-1);
}
/*------------------------------------------------------------------------*/
/*  From here to EOF is .netrc file read stuff.  We are not advertising
/*  this functionality but it has been tested.  Functioanlity has been
/*  limited to login name, password, and machine name.  NO macro,
/*  default or account support, these are silently ignored.
/*------------------------------------------------------------------------*/
static	FILE *cfile;

#define	DEFAULT	1
#define	LOGIN	2
#define	PASSWD	3
#define	ACCOUNT 4
#define MACDEF  5
#define	ID		10
#define	MACH	11

static char tokval[100];

static struct toktab {
        char *tokstr;
        int tval;
} toktab[]= {
        "default",      DEFAULT,
        "login",        LOGIN,
        "password",     PASSWD,
        "passwd",       PASSWD,
        "account",      ACCOUNT,
        "machine",      MACH,
        "macdef",       MACDEF,
        0,              0
};


static
ruserpass(host, aname, apass)
        char *host, **aname, **apass;
{
	char *hdir, buf[BUFSIZ];
    int t;
    struct stat stb;


	/*
	 * Test if already set.
	 */
	if (*aname != 0 && *apass != 0)
		return(0);
	
    hdir = getenv("HOME");
    if (hdir == NULL)
        hdir = ".";
    (void) sprintf(buf, "%s/.netrc", hdir);
    cfile = fopen(buf, "r");
    if (cfile == NULL) {
        if (errno != ENOENT)
            perror(buf);
        return(0);
    }

next:
	while ((t = token())) switch(t) {

	case DEFAULT:				/* IGNORE */
		(void) token();
		continue;

	case MACH:
		if (token() != ID)
			continue;
        /*
         * Allow match only for user's input host name
		 * Case insensitive.
         */
         if (strcmpi(host, tokval) != 0)
			continue;

		while ((t = token()) && t != MACH) switch(t) {

		case LOGIN:
			if (token())
				if (*aname == 0) { 
					*aname = malloc((unsigned) strlen(tokval) + 1);
					(void) strcpy(*aname, tokval);
				} else {
					if (strcmp(*aname, tokval))
						goto next;
				}
			break;
		case PASSWD:					/* Don't care about anonymous */
			if (fstat(fileno(cfile), &stb) >= 0
			    && (stb.st_mode & 077) != 0) {
	fprintf(stderr, gettxt("libxchoosemsgs:28", "rexec: Error - .netrc file not correct mode.\n"));
	fprintf(stderr, gettxt("libxchoosemsgs:29", "rexec: Remove password or correct mode.\n"));
				return(-1);    /* replaces goto bad */
			}
			if (token() && *apass == 0) {
				*apass = malloc((unsigned) strlen(tokval) + 1);
				(void) strcpy(*apass, tokval);
			}
			break;
		case ACCOUNT:				/* IGNORE */
		case MACDEF:				/* IGNORE */
			(void) token();
			break;
		default:
	fprintf(stderr, gettxt("libxchoosemsgs:30", "rexec: Unknown .netrc keyword %s\n"), tokval);
			break;
		}
		goto done;
	}
done:
	(void) fclose(cfile);
	return(0);
}

static
token()
{
	char *cp;
	int c;
	struct toktab *t;

	if (feof(cfile))
		return (0);
	while ((c = getc(cfile)) != EOF &&
	    (c == '\n' || c == '\t' || c == ' ' || c == ','))
		continue;
	if (c == EOF)
		return (0);
	cp = tokval;
	if (c == '"') {
		while ((c = getc(cfile)) != EOF && c != '"') {
			if (c == '\\')
				c = getc(cfile);
			*cp++ = c;
		}
	} else {
		*cp++ = c;
		while ((c = getc(cfile)) != EOF
		    && c != '\n' && c != '\t' && c != ' ' && c != ',') {
			if (c == '\\')
				c = getc(cfile);
			*cp++ = c;
		}
	}
	*cp = 0;
	if (tokval[0] == 0)
		return (0);
	for (t = toktab; t->tokstr; t++)
		if (!strcmp(t->tokstr, tokval))
			return (t->tval);
	return (ID);
}
/*-----------------------------------------------------------------------*/
/*  Routine: strcmpi
/*
/*  Purpose: Case insensitive string compare 
/*
/*-----------------------------------------------------------------------*/
static
strcmpi(const char *string1, const char *string2)
{
	do
	{
		if ( 	(*string1 != (*string2)) &&
				(*string1 != toupper(*string2)) &&
				(*string2 != toupper(*string1)))
		{
			return(*string1 - *string2);
		}
		string2++;
	}
	while(*string1++ != '\0');
	return(0);
}
