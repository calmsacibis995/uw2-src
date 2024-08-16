/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/list.c	1.2"
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
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */


/**************************************************************************
 **                                                                      **
 **  Modified for & distributed with DiG - Version 1.1.beta              **
 **                                                                      **
 **  Distributed with 'dig' version 2.0 release from USC-ISI (9/1/90).   **
 **                                                                      **
 *************************************************************************/

#ifndef lint
static char sccsid[] = "@(#)list.c	5.10 (Berkeley) 2/17/88";
#endif /* not lint */

#include "hfiles.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include NETDBH
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include NAMESERH
#include RESOLVH
#include "res.h"

/*
 *  Imported from res_debug.c
 */
extern char *_res_resultcodes[];

typedef union {
    HEADER qb1;
    char qb2[PACKETSZ];
} querybuf;

extern u_long 		inet_addr();
extern HostInfo 	*defaultPtr;
extern HostInfo 	curHostInfo;
extern int 		curHostValid;
extern int              sockFD;

#define HASH_SIZE 50

int
do_zone(domain,qtype)
     char *domain;
     int  qtype;
{
	querybuf 		buf;
	struct sockaddr_in 	sin;
	HEADER 			*headerPtr;
	int 			msglen;
	int 			amtToRead;
	int 			numRead;
	int 			soacnt = 0;
	u_short 		len;
	char 			*cp, *nmp;
	char 			dname[2][NAME_LEN];
	FILE*                   filePtr;
	enum {
	    NO_ERRORS, 
	    ERR_READING_LEN, 
	    ERR_READING_MSG,
	    ERR_PRINTING,
	} error = NO_ERRORS;

	msglen = res_mkquery(QUERY, domain, C_IN, T_AXFR,
				(char *)0, 0, (char *)0, 
				(char *) &buf, sizeof(buf));
	if (msglen < 0) {
	    if (_res.options & RES_DEBUG) {
	      (void) fprintf(stderr,
			     "; ***zone transfer: res_mkquery failed\n");
	    }
	    return (ERROR);
	}

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family	= AF_INET;
	sin.sin_port	= _res.nsaddr.sin_port;

	sin.sin_addr =  _res.nsaddr.sin_addr; /* .s_addr; */

	if ((sockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	    perror("ListHosts");
	    return(ERROR);
	}
	if (connect(sockFD, &sin, sizeof(sin)) < 0) {
	    perror("ListHosts");
	    (void) close(sockFD);
	    sockFD = -1;
	    return(ERROR);
	}	

        len = htons(msglen);

        if (write(sockFD, (char *)&len, sizeof(len)) != sizeof(len) ||
            write(sockFD, (char *) &buf, msglen) != msglen) {
		perror("ListHosts");
		(void) close(sockFD);
		sockFD = -1;
		return(ERROR);
	}

	filePtr = stdout;

	while (1) {

	    cp = (char *) &buf;
	    amtToRead = sizeof(u_short);
	    while((amtToRead > 0) && 
		  ((numRead = read(sockFD, cp, amtToRead)) > 0)) {
		cp 	  += numRead;
		amtToRead -= numRead;
	    }
	    if (numRead <= 0) {
		error = ERR_READING_LEN;
		break;
	    }	

	    if ((len = htons(*(u_short *)&buf)) == 0) {
		break;	/* nothing left to read */
	    }

	    amtToRead = len;
	    cp = (char *) &buf;
	    while((amtToRead > 0) &&
		  ((numRead = read(sockFD, cp, amtToRead)) > 0)) {
		cp += numRead;
		amtToRead -= numRead;
	    }
	    if (numRead <= 0) {
		error = ERR_READING_MSG;
		break;
	    }

	    cp = buf.qb2 + sizeof(HEADER);
	    if (ntohs(buf.qb1.qdcount) > 0)
		cp += dn_skipname(cp, buf.qb2 + len) + QFIXEDSZ;
	    nmp = cp;
	    cp += dn_skipname(cp, (u_char *)&buf + len);

	    if ((_getshort(cp) == T_SOA)) {
		dn_expand(buf.qb2, buf.qb2 + len, nmp, dname[soacnt],
			sizeof(dname[0]));
	        if (soacnt) {
		    if (strcmp(dname[0], dname[1]) == 0) {
		      (void) printf(";  Matching SOA found\n");
		      break;
		    }
		  } else {
		    fp_query((char *) &buf, filePtr);
		    soacnt++;
		  }
	      } else
		fp_query((char *) &buf, filePtr);

	    headerPtr = (HEADER *) &buf;
	    if (headerPtr->rcode != NOERROR) break;
	  }

	(void) close(sockFD);
	sockFD = -1;

	switch (error) {
	case NO_ERRORS:
	  return (SUCCESS);
	case ERR_READING_LEN:
	  return(ERROR);
	case ERR_PRINTING:
	  (void) fprintf(stderr,
			 "; *** Error during listing of %s:\n",	domain);
	  return(ERROR);
	case ERR_READING_MSG:
	  headerPtr = (HEADER *) &buf;
	  (void) fprintf(stderr,
			 "; ListHosts: error receiving zone transfer:\n");
	  (void) fprintf(stderr,
	      "; result: %s, answers = %d, authority = %d, additional = %d\n",
		  _res_resultcodes[headerPtr->rcode], 
		  ntohs(headerPtr->ancount), ntohs(headerPtr->nscount), 
		  ntohs(headerPtr->arcount));
	  return(ERROR);
	default:
	  return(ERROR);
	}
      }


