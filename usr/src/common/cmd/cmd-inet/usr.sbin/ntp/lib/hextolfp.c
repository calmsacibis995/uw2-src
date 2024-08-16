/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/hextolfp.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * hextolfp - convert an ascii hex string to an l_fp number
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_fp.h"

int
hextolfp(str, lfp)
	char *str;
	l_fp *lfp;
{
	register char *cp;
	register char *cpstart;
	register u_long dec_i;
	register u_long dec_f;
	char *ind;
	static char *digits = "0123456789abcdefABCDEF";

	dec_i = dec_f = 0;
	cp = str;

	/*
	 * We understand numbers of the form:
	 *
	 * [spaces]8_hex_digits[.]8_hex_digits[spaces|\n|\0]
	 */
	while (isspace(*cp))
		cp++;
	
	cpstart = cp;
	while (*cp != '\0' && (cp - cpstart) < 8 &&
	    (ind = strchr(digits, *cp)) != NULL) {
		dec_i = dec_i << 4;	/* multiply by 16 */
		dec_i += ((ind - digits) > 15) ? (ind - digits) - 6
		    : (ind - digits);
		cp++;
	}

	if ((cp - cpstart) < 8 || ind == NULL)
		return 0;
	if (*cp == '.')
		cp++;

	cpstart = cp;
	while (*cp != '\0' && (cp - cpstart) < 8 &&
	    (ind = strchr(digits, *cp)) != NULL) {
		dec_f = dec_f << 4;	/* multiply by 16 */
		dec_f += ((ind - digits) > 15) ? (ind - digits) - 6
		    : (ind - digits);
		cp++;
	}

	if ((cp - cpstart) < 8 || ind == NULL)
		return 0;
	
	if (*cp != '\0' && !isspace(*cp))
		return 0;

	lfp->l_ui = dec_i;
	lfp->l_uf = dec_f;
	return 1;
}
