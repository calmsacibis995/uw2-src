/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)librpcsvc:common/lib/librpcsvc/secretkey.c	1.1.6.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/librpcsvc/secretkey.c,v 1.1 91/02/28 20:58:54 ccs Exp $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
*          All rights reserved.
*/ 
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)secretkey.c 1.4 89/03/24 Copyr 1986 Sun Micro";
#endif

/*
 * secretkey.c
 */

/*
 * Secret key lookup routines
 */
#include <stdio.h>
#include <pwd.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#include <string.h>

/*
 * Get somebody's encrypted secret key from the database, using the given
 * passwd to decrypt it.
 */
getsecretkey(netname, secretkey, passwd)
	char *netname;
	char *secretkey;
	char *passwd;
{
	char lookup[3 * HEXKEYBYTES];
	char *p;

	if (secretkey == NULL)
		return (0);
	if (!getpublicandprivatekey(netname, lookup))
		return (0);
	p = strchr(lookup, ':');
	if (p == NULL) {
		return (0);
	}
	p++;
	if (!xdecrypt(p, passwd)) {
		return (0);
	}
	if (memcmp(p, p + HEXKEYBYTES, KEYCHECKSUMSIZE) != 0) {
		secretkey[0] = 0;
		return (1);
	}
	p[HEXKEYBYTES] = 0;
	/* Assuming that secretkey has enough space */
	(void) strcpy(secretkey, p);
	return (1);
}
