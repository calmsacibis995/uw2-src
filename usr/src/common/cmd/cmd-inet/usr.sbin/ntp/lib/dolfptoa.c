/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/dolfptoa.c	1.2"
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
 * dolfptoa - do the grunge work of converting an l_fp number to decimal
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_fp.h"
#include "lib_strbuf.h"

char *
dolfptoa(fpi, fpv, neg, ndec, msec)
	u_long fpi;
	u_long fpv;
	int neg;
	int ndec;
	int msec;
{
	register u_char *cp, *cpend;
	register u_long work_i;
	register int dec;
	u_char cbuf[24];
	u_char *cpdec;
	char *buf;
	char *bp;

	/*
	 * Get a string buffer before starting
	 */
	LIB_GETBUF(buf);

	/*
	 * Zero the character buffer
	 */
	memset(cbuf, '\0', sizeof(cbuf));

	/*
	 * Work on the integral part.  This is biased by what I know
	 * compiles fairly well for a 68000.
	 */
	cp = cpend = &cbuf[10];
	work_i = fpi;
	if (work_i & 0xffff0000) {
		register u_long lten = 10;
		register u_long ltmp;

		do {
			ltmp = work_i;
			work_i /= lten;
			ltmp -= (work_i<<3) + (work_i<<1);
			*--cp = (u_char)ltmp;
		} while (work_i & 0xffff0000);
	}
	if (work_i != 0) {
		register u_short sten = 10;
		register u_short stmp;
		register u_short swork = (u_short)work_i;

		do {
			stmp = swork;
			swork /= sten;
			stmp -= (swork<<3) + (swork<<1);
			*--cp = (u_char)stmp;
		} while (swork != 0);
	}

	/*
	 * Done that, now deal with the problem of the fraction.  First
	 * determine the number of decimal places.
	 */
	if (msec) {
		dec = ndec + 3;
		if (dec < 3)
			dec = 3;
		cpdec = &cbuf[13];
	} else {
		dec = ndec;
		if (dec < 0)
			dec = 0;
		cpdec = &cbuf[10];
	}
	if (dec > 12)
		dec = 12;
	
	/*
	 * If there's a fraction to deal with, do so.
	 */
	if (fpv != 0) {
		register u_long work_f;

		work_f = fpv;
		while (dec > 0) {
			register u_long tmp_i;
			register u_long tmp_f;

			dec--;
			/*
			 * The scheme here is to multiply the
			 * fraction (0.1234...) by ten.  This moves
			 * a junk of BCD into the units part.
			 * record that and iterate.
			 */
			work_i = 0;
			M_LSHIFT(work_i, work_f);
			tmp_i = work_i;
			tmp_f = work_f;
			M_LSHIFT(work_i, work_f);
			M_LSHIFT(work_i, work_f);
			M_ADD(work_i, work_f, tmp_i, tmp_f);
			*cpend++ = (u_char)work_i;
			if (work_f == 0)
				break;
		}

		/*
		 * Rounding is rotten
		 */
		if (work_f & 0x80000000) {
			register u_char *tp = cpend;

			*(--tp) += 1;
			while (*tp >= 10) {
				*tp = 0;
				*(--tp) += 1;
			};
			if (tp < cp)
				cp = tp;
		}
	}
	cpend += dec;


	/*
	 * We've now got the fraction in cbuf[], with cp pointing at
	 * the first character, cpend pointing past the last, and
	 * cpdec pointing at the first character past the decimal.
	 * Remove leading zeros, then format the number into the
	 * buffer.
	 */
	while (cp < cpdec) {
		if (*cp != 0)
			break;
		cp++;
	}
	if (cp == cpdec)
		--cp;

	bp = buf;
	if (neg)
		*bp++ = '-';
	while (cp < cpend) {
		if (cp == cpdec)
			*bp++ = '.';
		*bp++ = (char)(*cp++ + '0');	/* ascii dependent? */
	}
	*bp = '\0';

	/*
	 * Done!
	 */
	return buf;
}
