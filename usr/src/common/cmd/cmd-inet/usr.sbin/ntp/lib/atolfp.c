/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/atolfp.c	1.2"
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
 * atolfp - convert an ascii string to an l_fp number
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_fp.h"

/*
 * Powers of 10
 */
static u_long ten_to_the_n[10] = {
		   0,
		  10,
		 100,
		1000,
	       10000,
	      100000,
	     1000000,
	    10000000,
	   100000000,
	  1000000000,
};


int
atolfp(str, lfp)
	char *str;
	l_fp *lfp;
{
	register char *cp;
	register u_long dec_i;
	register u_long dec_f;
	char *ind;
	int ndec;
	int isneg;
	static char *digits = "0123456789";

	isneg = 0;
	dec_i = dec_f = 0;
	ndec = 0;
	cp = str;

	/*
	 * We understand numbers of the form:
	 *
	 * [spaces][-][digits][.][digits][spaces|\n|\0]
	 */
	while (isspace(*cp))
		cp++;
	
	if (*cp == '-') {
		cp++;
		isneg = 1;
	}

	if (*cp != '.' && !isdigit(*cp))
		return 0;

	while (*cp != '\0' && (ind = strchr(digits, *cp)) != NULL) {
		dec_i = (dec_i << 3) + (dec_i << 1);	/* multiply by 10 */
		dec_i += (ind - digits);
		cp++;
	}

	if (*cp != '\0' && !isspace(*cp)) {
		if (*cp++ != '.')
			return 0;
	
		while (ndec < 9 && *cp != '\0'
		    && (ind = strchr(digits, *cp)) != NULL) {
			ndec++;
			dec_f = (dec_f << 3) + (dec_f << 1);	/* *10 */
			dec_f += (ind - digits);
			cp++;
		}

		while (isdigit(*cp))
			cp++;
		
		if (*cp != '\0' && !isspace(*cp))
			return 0;
	}

	if (ndec > 0) {
		register u_long tmp;
		register u_long bit;
		register u_long ten_fact;

		ten_fact = ten_to_the_n[ndec];

		tmp = 0;
		bit = 0x80000000;
		while (bit != 0) {
			dec_f <<= 1;
			if (dec_f >= ten_fact) {
				tmp |= bit;
				dec_f -= ten_fact;
			}
			bit >>= 1;
		}
		if ((dec_f << 1) > ten_fact)
			tmp++;
		dec_f = tmp;
	}

	if (isneg)
		M_NEG(dec_i, dec_f);
	
	lfp->l_ui = dec_i;
	lfp->l_uf = dec_f;
	return 1;
}
