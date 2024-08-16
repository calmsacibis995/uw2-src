/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pathrouter/defs.h	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)defs.h	1.2 'attmail mail(1) command'"
/*
**
**  Defs.h:  header file for pathrouter (was rmail/smail.)
**
**
*/

#ifndef VERSION
#define	VERSION	"SVR4 pathrouter."
#endif

#include "../mail/libmail.h"

/*
 * Mail that would otherwise be undeliverable will be passed to the
 * aliased SMARTHOST for potential delivery.
 *
 * Be sure that the host you specify in your pathalias input knows that you're
 * using it as a relay, or you might upset somebody when they find out some
 * other way.  If you're using 'foovax' as your relay, and below you have
 * #define SMARTHOST "smart-host", then the pathalias alias would be:
 *
 *	smart-host = foovax
 */

#define SMARTHOST  "smart-host"	/* pathalias alias for relay host */

/*
**	PATHS is name of pathalias file.  This is mandatory.
*/

/* for SVR4 use /etc/uucp/paths, for SVR3 use /usr/lib/uucp/paths */

#ifndef PATHS
#ifdef SVR3
#define PATHS	"/usr/lib/uucp/paths"	/* location of the path database */
#else
#define PATHS	"/etc/uucp/paths"	/* location of the path database */
#endif
#endif

/*
** PLEASE DON'T TOUCH THE REST
*/

#define HANDLE	ALL
#define ROUTING REROUTE
#define UNAME 				/* use uname() */

#define SMLBUF	512	/* small buffer (handle one item) */
#define BIGBUF	4096	/* handle lots of items */
 
#define MAXPATH	32	/* number of elements in ! path */
#define MAXDOMS	16	/* number of subdomains in . domain */
#define MAXARGS	500	/* number of arguments */

#define DEBUG 		if (debug==YES) (void) printf
#define ADVISE 		if (debug!=NO) (void) printf
#define error(stat,msg,a)	{ (void) fprintf(stderr, msg, a); exit(stat); }
#define lower(c) 		( Isupper(c) ? _tolower(c) : c )

enum eform {	/* format of addresses */
	ERROR, 		/* bad or invalidated format */
	LOCAL, 		/* just a local name */
	DOMAIN, 	/* user@domain or domain!user */
	UUCP,		/* host!address */
	ROUTE,		/* intermediate form - to be routed */
	SENT		/* sent to a mailer on a previous pass */
};

enum ehandle { 	/* what addresses can we handle? (don't kick to LMAIL) */
	ALL,		/* UUCP and DOMAIN addresses */
	JUSTUUCP,	/* UUCP only; set by -l  */
	NONE		/* all mail is LOCAL; set by -L */
};

enum erouting {	/* when to route A!B!C!D */
	JUSTDOMAIN,	/* route A if A is a domain */
	ALWAYS,		/* route A always; set by -r */
	REROUTE		/* route C, B, or A (whichever works); set by -R */
};

enum edebug {	/* debug modes */
	NO,		/* normal deliver */
	VERBOSE,	/* talk alot */
	YES		/* talk and don't deliver */
};


#include "sysexits.h"
#define	index	strchr
#define	rindex	strrchr

extern int blook ARGS((FILE *file, char *key, char *buf));
extern void build ARGS((char *domain, char *user, enum eform form, char *result));
extern int getpath ARGS((char *key, char *path, int *cost));
extern int isuucp ARGS((char *));
extern void map ARGS((int argc, char **argv, char *hostv[], char *userv[], enum eform formv[], int costv[]));
enum eform parse ARGS((char *address, char *domain, char *user));
enum eform resolve ARGS((char *address, char *domain, char *user, int *cost));
extern char *sform ARGS((enum eform form));
extern int ssplit ARGS((char *buf, int c, char **ptr));
