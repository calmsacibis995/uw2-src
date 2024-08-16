/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_error.c	1.2.5.6"
#ident	"$Header: $"

#include <stdio.h>
#include <unistd.h>
#include <xti.h>
#include <errno.h>
#include "_import.h"

#ifdef t_error
#undef t_error
#endif

#pragma weak _xti_error = t_error

int
t_error(s)
char	*s;
{

	int terrno = get_t_errno();

	/*
	 * Special case: t_errno == 0.
	 * As per X/Open specs, it may be: "0: unknown error"
	 * We decided to keep "No Error", for now.
	 */
	if (terrno == 0) {
		if(strlen(s) == 0) {
			fprintf(stderr, "%s\n", 
			    gettxt("uxnsl:31", "No Error"));
		} else {
			fprintf(stderr, "%s: %s\n", s,
			    gettxt("uxnsl:31", "No Error"));
		}
		return(0);
	}


	/*
	 * the following 4 cases cover all formats
	 */

	if(strlen(s) == 0) {
	/* no user string  */
		if(terrno == TSYSERR) {
			fprintf(stderr, "%s: %s\n",
			    t_strerror(terrno), strerror(errno));
		} else {
			fprintf(stderr, "%s\n", t_strerror(terrno));
		}
	} else {
	/* user string */
		if(terrno == TSYSERR) {
			fprintf(stderr, "%s: %s: %s\n",
			    s, t_strerror(terrno), strerror(errno));
		} else {
			fprintf(stderr, "%s: %s\n", s, t_strerror(terrno));
		}
	}
	
	return(0);
}
