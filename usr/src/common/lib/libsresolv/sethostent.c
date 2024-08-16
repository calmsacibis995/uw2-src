/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresolv:common/lib/libsresolv/sethostent.c	1.1.1.4"
#ident  "$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <stdio.h>
#include <sys/types.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#include "res.h"
#include "libres_mt.h"

#pragma weak sethostent=_rs_sethostent
#pragma weak endhostent=_rs_endhostent
#pragma weak sethostfile=_rs_sethostfile

extern struct state *get_rs__res();

_rs_sethostent(stayopen)
	int stayopen;
{
	struct state *rp;

	/* Get thread-specific data */
	if ((rp = get_rs__res()) == NULL)
		return (-1);

	if (stayopen)
		rp->options |= RES_STAYOPEN | RES_USEVC;
}

_rs_endhostent()
{
	struct state *rp;

	/* Get thread-specific data */
	if ((rp = get_rs__res()) == NULL)
		return (-1);

	rp->options &= ~(RES_STAYOPEN | RES_USEVC);
	_rs__res_close();
}

_rs_sethostfile(name)
char *name;
{
#ifdef lint
name = name;
#endif
}
