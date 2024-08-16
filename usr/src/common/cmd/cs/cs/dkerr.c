/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/dkerr.c	1.1.1.4"
#ident	"$Header: $"

/*
 *	Convert an error number from the Common Control into a string
 */
/*#ifndef DIAL
	static char	SCCSID[] = "dkerr.c	2.6+BNU DKHOST 87/03/05";
#endif */
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include <unistd.h>
#include "dk.h"
#include "sysexits.h"


#define DIALERCODE	11

GLOBAL int	dk_verbose = 1;	/* Print error messages on stderr if 1 */
GLOBAL int	dk_errno = 0;	/* Saved error number from iocb.req_error */

static char	generalmsg[32];

GLOBAL char *
dkerr(err)
{

    /* dialer messages */
    if (((err & 0377) == DIALERCODE) && (err & 0xff00)) {
	switch(err>>8) {
	case	0:
		return ("");

	case	1:		/* code 1 - phone # missing */
		return (gettxt(":39", "Please supply a valid phone number"));
		
	case	2:		/* code 2 - bad port */
		return (gettxt(":40", "No response from auto-dialer. Try again"));

	case	3:		/* code 3 - dial failure */
		return (gettxt(":41", "Auto dialer failed to initiate call. Try again"));

	case	4:		/* code 4 - bad telephone line */
		return (gettxt(":42", "No initial dial tone detected"));

	case	5:		/* code 5 - no sec. dial tone */
		return (gettxt(":43", "No secondary dial tone detected"));

	case	6:		/* code 6 - busy signal detected */
		return (gettxt(":44", "Dialed number is busy"));

	case	7:		/* code 7 - auto-dialer didn't get ans. */
		return (gettxt(":45", "No answer from dialed number"));

	case	8:		/* code 8 - no carrier tone det. */
		return (gettxt(":46", "No carrier tone was detected"));

	case	9:		/* code 9 - auto dialer didn't complete */
		return (gettxt(":47", "Could not complete your call. Try again."));

	case	10:		/*code 10 - bad number*/
		return (gettxt(":48", "Wrong number"));

	default:
		sprintf(generalmsg,
		    gettxt(":49", "Invalid dialer error code: %d (0x%x)"), err, err);
		return (generalmsg);
	}
    }

    /* datakit messages */
    if (err >= 0 && err <= 99) {
	switch(err) {
	case 0:			/* code 0 - Something is Wrong */
		return(gettxt(":50", "Call Failed"));

	case 1:			/* code 1 - busy */
		return(gettxt(":51", "All channels busy"));

	case 2:			/* code 2 - trunk down */
		return(gettxt(":52", "Remote node not answering"));

	case 3:			/* code 3 - termporary no dest */
		return(gettxt(":53", "Server not answering"));

	case 4:			/* code 4 - permonent no dest (INTERT) */
		return(gettxt(":54", "Non-assigned number"));

	case 5:			/* code 5 - System Overload (REORT) */
		return(gettxt(":55", "All trunk channels busy"));

	case 6:			/* code 6 - already exists */
		return(gettxt(":56", "Server already exists"));

	case 7:			/* code 7 - denied by remote server */
		return(gettxt(":57", "Access denied"));

	default:	
		break;
	}
    }


    /* finally - some datakit host messages */
    switch (err) {
    case 130: /* Code 130 */
	return(gettxt(":58", "Dkserver: Can't open line: See System Administrator"));

    case 133: /* Code 133 */
	return(gettxt(":59", "Dkserver: Dksrvtab not readable: See System Administrator"));

    case 134:
	return(gettxt(":60", "Dkserver: Can't chroot: See System Administrator"));

    default:
	/*
	 * Any error code not caught above in (this function)
	 * will be trapped here:
	 */
	sprintf(generalmsg, gettxt(":61", "Error code %d"), err) ;
	return(generalmsg);
    }
}

GLOBAL int
dkerrmap(dkcode)
{
	if (dkcode < 0)
		return(-dkcode);

	switch(dkcode){
	case 0:
	case 1:
	case 2:
	case 3:
	case 5:
		return(EX_TEMPFAIL);

	case 4:
		return(EX_NOHOST);

	case 6:
		return(EX_CANTCREAT);

	case 7:
		return(EX_NOPERM);

	default:
		return(EX_DATAERR);
	}
}
