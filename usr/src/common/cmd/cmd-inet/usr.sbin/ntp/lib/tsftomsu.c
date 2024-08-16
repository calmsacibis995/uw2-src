/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/tsftomsu.c	1.2"
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
 * tsftomsu - convert from a time stamp fraction to milliseconds
 */
#include <sys/types.h>
#include "ntp_fp.h"

int
tsftomsu(tsf, round)
	u_long tsf;
	int round;
{
	register u_long val_ui, val_uf;
	register u_long tmp_ui, tmp_uf;
	register int i;

	/*
	 * Essentially, multiply by 10 three times in l_fp form.
	 * The integral part is the milliseconds.
	 */
	val_ui = 0;
	val_uf = tsf;
	for (i = 3; i > 0; i--) {
		M_LSHIFT(val_ui, val_uf);
		tmp_ui = val_ui;
		tmp_uf = val_uf;
		M_LSHIFT(val_ui, val_uf);
		M_LSHIFT(val_ui, val_uf);
		M_ADD(val_ui, val_uf, tmp_ui, tmp_uf);
	}

	/*
	 * Round the value if need be, then return it.
	 */
	if (round && (val_uf & 0x80000000))
		val_ui++;
	return (int)val_ui;
}
