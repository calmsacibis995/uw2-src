/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresolv:common/lib/libsresolv/res_mkquery.c	1.1.1.4"
#ident  "$Header: $"

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

#include <sys/byteorder.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "res.h"

#pragma weak res_mkquery=_rs_res_mkquery
struct state *
get_rs__res();

/*
 * Form all types of queries.
 * Returns the size of the result or -1.
 */
_rs_res_mkquery(op, dname, class, type, data, datalen, newrr, buf, buflen)
	int op;			/* opcode of query */
	char *dname;		/* domain name */
	int class, type;	/* class and type of query */
	char *data;		/* resource record data */
	int datalen;		/* length of data */
	struct rrec *newrr;	/* new rr for modify or append */
	char *buf;		/* buffer to put query */
	int buflen;		/* size of buffer */
{
	register HEADER *hp;
	register char *cp;
	register int n;
	char dnbuf[MAXDNAME];
	char *dnptrs[10], **dpp, **lastdnptr;
	extern char *index();
	struct state *rp;

	
	/* Get thread-specific data */
	if ((rp = get_rs__res()) == NULL)
    		return (-1);

#ifdef DEBUG
	if (rp->options & RES_DEBUG)
		printf("_rs_res_mkquery(%d, %s, %d, %d)\n",
		       op, dname, class, type);
#endif /* DEBUG */
	/*
	 * Initialize header fields.
	 */
	bzero(buf, sizeof(HEADER));
	hp = (HEADER *) buf;
	hp->id = htons(++rp->id);
	hp->opcode = op;
	hp->pr = (rp->options & RES_PRIMARY) != 0;
	hp->rd = (rp->options & RES_RECURSE) != 0;
	hp->rcode = NOERROR;
	cp = buf + sizeof(HEADER);
	buflen -= sizeof(HEADER);
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof(dnptrs)/sizeof(dnptrs[0]);
	/*
	 * perform opcode specific processing
	 */
	switch (op) {
	case QUERY:
		buflen -= QFIXEDSZ;
		if ((n = _rs_dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		_rs_putshort(type, cp);
		cp += sizeof(u_short);
		_rs_putshort(class, cp);
		cp += sizeof(u_short);
		hp->qdcount = htons(1);
		if (op == QUERY || data == NULL)
			break;
		/*
		 * Make an additional record for completion domain.
		 */
		buflen -= RRFIXEDSZ;
		if ((n = _rs_dn_comp(data, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		_rs_putshort(T_NULL, cp);
		cp += sizeof(u_short);
		_rs_putshort(class, cp);
		cp += sizeof(u_short);
		_rs_putlong(0, cp);
		cp += sizeof(u_long);
		_rs_putshort(0, cp);
		cp += sizeof(u_short);
		hp->arcount = htons(1);
		break;

	case IQUERY:
		/*
		 * Initialize answer section
		 */
		if (buflen < 1 + RRFIXEDSZ + datalen)
			return (-1);
		*cp++ = '\0';	/* no domain name */
		_rs_putshort(type, cp);
		cp += sizeof(u_short);
		_rs_putshort(class, cp);
		cp += sizeof(u_short);
		_rs_putlong(0, cp);
		cp += sizeof(u_long);
		_rs_putshort(datalen, cp);
		cp += sizeof(u_short);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		hp->ancount = htons(1);
		break;

#ifdef ALLOW_UPDATES
	/*
	 * For UPDATEM/UPDATEMA, do UPDATED/UPDATEDA followed by UPDATEA
	 * (Record to be modified is followed by its replacement in msg.)
	 */
	case UPDATEM:
	case UPDATEMA:

	case UPDATED:
		/*
		 * The res code for UPDATED and UPDATEDA is the same; user
		 * calls them differently: specifies data for UPDATED; server
		 * ignores data if specified for UPDATEDA.
		 */
	case UPDATEDA:
		buflen -= RRFIXEDSZ + datalen;
		if ((n = _rs_dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		_rs_putshort(type, cp);
                cp += sizeof(u_short);
                _rs_putshort(class, cp);
                cp += sizeof(u_short);
		_rs_putlong(0, cp);
		cp += sizeof(u_long);
		_rs_putshort(datalen, cp);
                cp += sizeof(u_short);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		if ( (op == UPDATED) || (op == UPDATEDA) ) {
			hp->ancount = htons(0);
			break;
		}
		/* Else UPDATEM/UPDATEMA, so drop into code for UPDATEA */

	case UPDATEA:	/* Add new resource record */
		buflen -= RRFIXEDSZ + datalen;
		if ((n = _rs_dn_comp(dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		_rs_putshort(newrr->r_type, cp);
                cp += sizeof(u_short);
                _rs_putshort(newrr->r_class, cp);
                cp += sizeof(u_short);
		_rs_putlong(0, cp);
		cp += sizeof(u_long);
		_rs_putshort(newrr->r_size, cp);
                cp += sizeof(u_short);
		if (newrr->r_size) {
			bcopy(newrr->r_data, cp, newrr->r_size);
			cp += newrr->r_size;
		}
		hp->ancount = htons(0);
		break;

#endif ALLOW_UPDATES
	}
	return (cp - buf);
}
