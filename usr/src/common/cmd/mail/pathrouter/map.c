/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pathrouter/map.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)map.c	1.2 'attmail mail(1) command'"
/* static char 	*sccsid="@(#)map.c	2.6 (smail) 5/24/88"; */

#include	"defs.h"

extern int queuecost;

/*
**
**  map(): map addresses into <host, user, form, cost> sets.
**
**  Calls resolve() for each address of argv.  The result is hostv and 
**  userv arrays (pointing into buffers userz and hostz), and formv array.
**
*/

void
map(argc, argv, hostv, userv, formv, costv)
int argc;				/* address count 		*/
char **argv;				/* address vector 		*/
char *hostv[];				/* remote host vector 		*/
char *userv[];				/* user name vector 		*/
enum eform formv[];			/* address format vector 	*/
int costv[];				/* cost vector 			*/
{
	int i, cost;
	char *c;
	static char userbuf[BIGBUF], *userz;
	static char hostbuf[BIGBUF], *hostz;

	userz = userbuf;
	hostz = hostbuf;

	for( i=0; i<argc; i++ ) {
		cost = queuecost+1;		/* default is queueing */
		userv[i] = userz;		/* put results here */
		hostv[i] = hostz;
		if ( **argv == '(' ) {		/* strip () */
			++*argv;
			c = index( *argv, ')' );
			if (c)
				*c = '\0';
		}
						/* here it comes! */
		formv[i] = resolve(*argv++, hostz, userz, &cost);
		costv[i] = cost;
		userz += strlen( userz ) + 1;	/* skip past \0 */
		hostz += strlen( hostz ) + 1;
	}
	return;
}
