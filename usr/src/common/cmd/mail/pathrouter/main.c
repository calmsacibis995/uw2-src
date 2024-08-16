/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pathrouter/main.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)main.c	1.2 'attmail mail(1) command'"
/*
**  pathrouter for SVR4 Mailers
**  Based on routing part of smail 2.6
**  This does not invoke the mailer. This allows the administator
**  to setup a routing database in /etc/uucp/paths
**  
**  Modifications November 1992 AJJ
**
**  Based upon code from:
**  rmail/smail - UUCP mailer with automatic routing.
**
**
*/

/* static char 	*sccsid="@(#)main.c	2.6 (smail) 5/24/88"; */

/*
**
**  usage:  	pathrouter [-p] [-d] [-T pathfile] address...
**
**  options:
**		-d 		debug - verbose 
**		-p 		batch mode , outputs oldname newname
**		-T pathfile	use alternate paths file
*/

#include	"defs.h"

/* other externals */

enum edebug debug     = NO;	/* set by -d option		*/
enum ehandle handle   = HANDLE;	/* which mail we can handle, see defs.h	*/
enum erouting routing = ROUTING;/* to route or not to route, see defs.h */

/* hostname -- the CLUSTER name from /etc/mail/mailcnfg if set, else the uname
 * domain -- the DOMAIN name from /etc/mail/mailcnfg if set
 * hostdomain -- the full domain address hostname+domain
 * localhost -- the uname, if CLUSTER is set different to uname, this is
 *     used to identify local mail.
 */
char hostname[SMLBUF]   = "";		
char hostdomain[SMLBUF] = "";	
char localhost[SMLBUF] = "";	
static char domain[SMLBUF] = "";	

char *pathfile  = PATHS;

int  queuecost  = 100;	/* not used by present system */

int  getcost    = 1;

/*
**
**  pathrouter: 
**
**  After processing command line options and finding our host and domain 
**  names, print resolved addresses to stdout. If in batch mode print pairs
**  of original and resolved address.
**
*/

main(argc, argv)
int argc;
char *argv[];
{
	char *hostv[MAXARGS];		/* UUCP neighbor 		*/
	char *userv[MAXARGS];		/* address given to host 	*/
	int  costv[MAXARGS];		/* cost of resolved route	*/
	enum eform formv[MAXARGS];	/* invalid, local, or uucp 	*/
	int c;
	int  batchmode  = 0;		/* set by -p			*/
	int nargc;
	int i;
	char abuf[SMLBUF];

	char **nargv;
	char *optstr = "dpT:";

/*
**  Process command line arguments
*/
	while ((c = getopt(argc, argv, optstr)) != EOF) {
		switch ( c ) {
		case 'd': debug      	= YES; 		break;
		case 'p': batchmode     = YES; 		break;
		case 'T': pathfile     = optarg; 		break;
		default:
			error( EX_USAGE, "usage: %s [flags] address...\n", argv[0] );
			break;
		}
	}
	if ( argc <= optind ) {
		error( EX_USAGE, "usage: %s [flags] address...\n", argv[0] );
	}

/*
**  Get our default hostname and hostdomain.
*/
	(void) strcpy(domain,maildomain()); /* mail domain name */
	(void) strcpy(hostname,mailsystem(0));  /* mail cluster name */
	(void) strcpy(localhost,mailsystem(1)); /* note the uname */

	if (hostname == '\0')  /* if cluster not set, use uname */
		(void) strcpy(hostname,localhost);

	(void) strcpy(hostdomain,hostname);
	(void) strcat(hostdomain,domain);


	nargc = argc - optind;

/*
** Do aliasing and fullname resolution
*/
	nargv=&argv[optind];
/*
**  Map argv addresses to <host, user, form, cost>.
*/
	map(nargc, nargv, hostv, userv, formv, costv);
/*
**  print the mapped addresses and exit.
*/

	for(i=nargc-1; i >= 0; i--) {
		if(formv[i] == ERROR) {
			(void) strcpy(abuf, nargv[i]);
		} else {
			build(hostv[i], userv[i], formv[i], abuf);
			/*
			 * if batchmode output oldname newname 
			 * for each address argument
			 */
			if (batchmode) {
				(void)fprintf(stdout,"%s %s\n", nargv[i], abuf);
				(void)strcpy(abuf,"");
			}
		}
		if (!batchmode) {
			(void) fputs(abuf, stdout); 
			if(i != 0) (void) putchar(' ');
		}
	}
	if (!batchmode)
		(void) putchar('\n');
	return(0);
}
