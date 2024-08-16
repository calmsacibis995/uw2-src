/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/tools/nslookup/skip.c	1.1.9.2"
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
 * Copyright (c) 1985 Regents of the University of California. All rights
 * reserved.
 * 
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)skip.c	5.9 (Berkeley) 8/3/90";
#endif /* not lint */

/*
 *******************************************************************************
 *
 *  skip.c --
 *
 *	Routines to skip over portions of a query buffer.
 *
 *	Note: this file has been submitted for inclusion in
 *	BIND resolver library. When this has been done, this file
 *	is no longer necessary (assuming there haven't been any
 *	changes).
 *
 *	Adapted from 4.3BSD BIND res_debug.c
 *
 *******************************************************************************
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/nameser.h>

char *res_skip_rr();


/*
 *******************************************************************************
 *
 *  res_skip --
 *
 * 	Skip the contents of a query.
 *
 * 	Interpretation of numFieldsToSkip argument:
 *            res_skip returns pointer to:
 *    	1 ->  start of question records.
 *    	2 ->  start of authoritative answer records.
 *    	3 ->  start of additional records.
 *    	4 ->  first byte after end of additional records.
 *
 *   Results:
 *	(address)	- success operation.
 *  	NULL 		- a resource record had an incorrect format.
 *
 *******************************************************************************
 */

char *
res_skip(msg, numFieldsToSkip, eom)
	char *msg;
	int numFieldsToSkip;
	char *eom;
{
	register char *cp;
	register HEADER *hp;
	register int tmp;
	register int n;

	/*
	 * Skip the header fields.
	 */
	hp = (HEADER *)msg;
	cp = msg + sizeof(HEADER);

	/*
	 * skip question records.
	 */
	if (n = ntohs(hp->qdcount) ) {
		while (--n >= 0 && cp < eom) {
			tmp = dn_skipname(cp, eom);
			if (tmp == -1) return(NULL);
			cp += tmp;
			cp += sizeof(u_short);	/* type 	*/
			cp += sizeof(u_short);	/* class 	*/
		}
	}
	if (--numFieldsToSkip <= 0) return(cp);

	/*
	 * skip authoritative answer records
	 */
	if (n = ntohs(hp->ancount)) {
		while (--n >= 0 && cp < eom) {
			cp = res_skip_rr(cp, eom);
			if (cp == NULL) return(NULL);
		}
	}
	if (--numFieldsToSkip == 0) return(cp);

	/*
	 * skip name server records
	 */
	if (n = ntohs(hp->nscount)) {
		while (--n >= 0 && cp < eom) {
			cp = res_skip_rr(cp, eom);
			if (cp == NULL) return(NULL);
		}
	}
	if (--numFieldsToSkip == 0) return(cp);

	/*
	 * skip additional records
	 */
	if (n = ntohs(hp->arcount)) {
		while (--n >= 0 && cp < eom) {
			cp = res_skip_rr(cp, eom);
			if (cp == NULL) return(NULL);
		}
	}

	return(cp);
}


/*
 *******************************************************************************
 *
 *  res_skip_rr --
 *
 * 	Skip over resource record fields.
 *
 *   Results:
 *	(address)	- success operation.
 *  	NULL 		- a resource record had an incorrect format.
 *******************************************************************************
 */

char *
res_skip_rr(cp, eom)
	char *cp;
	char *eom;
{
	int tmp;
	int dlen;

	if ((tmp = dn_skipname(cp, eom)) == -1)
		return (NULL);			/* compression error */
	cp += tmp;
	if ((cp + RRFIXEDSZ) > eom)
		return (NULL);
	cp += sizeof(u_short);	/* 	type 	*/
	cp += sizeof(u_short);	/* 	class 	*/
	cp += sizeof(u_long);	/* 	ttl 	*/
	dlen = _getshort(cp);
	cp += sizeof(u_short);	/* 	dlen 	*/
	cp += dlen;
	if (cp > eom)
		return (NULL);
	return (cp);
}
