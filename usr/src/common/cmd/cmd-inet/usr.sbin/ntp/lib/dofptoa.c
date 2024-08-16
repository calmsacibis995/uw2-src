/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/dofptoa.c	1.2"
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
 * dofptoa - do the grunge work to convert an fp number to ascii
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_fp.h"
#include "lib_strbuf.h"


char *
dofptoa(fpv, neg, ndec, msec)
	u_fp fpv;
	int neg;
	int ndec;
	int msec;
{
	register u_char *cp, *cpend;
	register u_long val;
	register short dec;
	u_char cbuf[12];
	u_char *cpdec;
	char *buf;
	char *bp;

	/*
	 * Get a string buffer before starting
	 */
	LIB_GETBUF(buf);

	/*
	 * Zero out the buffer
	 */
	memset((char *)cbuf, '\0', sizeof cbuf);

	/*
	 * Set the pointers to point at the first
	 * decimal place.  Get a local copy of the value.
	 */
	cp = cpend = &cbuf[5];
	val = fpv;

	/*
	 * If we have to, decode the integral part
	 */
	if (!(val & 0xffff0000))
		cp--;
	else {
		register u_short sv = (u_short)(val >> 16);
		register u_short tmp;
		register u_short ten = 10;

		do {
			tmp = sv;
			sv /= ten;
			*(--cp) = tmp - ((sv<<3) + (sv<<1));
		} while (sv != 0);
	}

	/*
	 * Figure out how much of the fraction to do
	 */
	if (msec) {
		dec = ndec + 3;
		if (dec < 3)
			dec = 3;
		cpdec = &cbuf[8];
	} else {
		dec = ndec;
		cpdec = cpend;
	}

	if (dec > 6)
		dec = 6;
	
	if (dec > 0) {
		do {
			val &= 0xffff;
			val = (val << 3) + (val << 1);
			*cpend++ = (u_char)(val >> 16);
		} while (--dec > 0);
	}

	if (val & 0x8000) {
		register u_char *tp;
		/*
		 * Round it. Ick.
		 */
		tp = cpend;
		*(--tp) += 1;
		while (*tp >= 10) {
			*tp = 0;
			*(--tp) += 1;
		}
	}

	/*
	 * Remove leading zeroes if necessary
	 */
	while (cp < (cpdec -1) && *cp == 0)
		cp++;
	
	/*
	 * Copy it into the buffer, asciizing as we go.
	 */
	bp = buf;
	if (neg)
		*bp++ = '-';
	
	while (cp < cpend) {
		if (cp == cpdec)
			*bp++ = '.';
		*bp++ = (char)(*cp++ + '0');
	}
	*bp = '\0';
	return buf;
}
