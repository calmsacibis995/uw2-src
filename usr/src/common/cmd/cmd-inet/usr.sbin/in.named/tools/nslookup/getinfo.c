/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/tools/nslookup/getinfo.c	1.2.9.2"
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
static char sccsid[] = "@(#)getinfo.c	5.22 (Berkeley) 6/1/90";
#endif /* not lint */

/*
 *******************************************************************************
 *
 *  getinfo.c --
 *
 *	Routines to create requests to name servers
 *	and interpret the answers.
 *
 *	Adapted from 4.3BSD BIND gethostnamadr.c
 *
 *******************************************************************************
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include "res.h"

#define bcopy(a, b, n)	memcpy(b, a, n)

extern char *_res_resultcodes[];
extern char *res_skip();

#define	MAXALIASES	35
#define MAXADDRS	35
#define MAXDOMAINS	35
#define MAXSERVERS	10

static char *addr_list[MAXADDRS + 1];

static char *host_aliases[MAXALIASES];
static int   host_aliases_len[MAXALIASES];
static char  hostbuf[BUFSIZ+1];

typedef struct {
    char *name;
    char *domain[MAXDOMAINS];
    int   numDomains;
    char *address[MAXADDRS];
    int   numAddresses;
} ServerTable;

ServerTable server[MAXSERVERS];

typedef union {
    HEADER qb1;
    char qb2[PACKETSZ*4];
} querybuf;

typedef union {
    long al;
    char ac;
} align;

#define GetShort(cp)	_getshort(cp); cp += sizeof(unsigned short);


/*
 *******************************************************************************
 *
 *  GetAnswer --
 *
 *	Interprets an answer packet and retrieves the following
 *	information:
 *
 *  Results:
 *      SUCCESS         the info was retrieved.
 *      NO_INFO         the packet did not contain an answer.
 *	NONAUTH		non-authoritative information was found.
 *      ERROR           the answer was malformed.
 *      Other errors    returned in the packet header.
 *
 *******************************************************************************
 */

static int
GetAnswer(nsAddrPtr, queryType, msg, msglen, iquery, hostPtr, isServer)
    struct in_addr	*nsAddrPtr;
    char		*msg;
    int			queryType;
    int			msglen;
    Boolean		iquery;
    register HostInfo	*hostPtr;
    Boolean		isServer;
{
    register HEADER	*headerPtr;
    register char	*cp;
    querybuf		answer;
    char		*eom, *bp, **aliasPtr;
    char		**addrPtr;
    char		*namePtr;
    char		*dnamePtr;
    int			type, class;
    int			qdcount, ancount, arcount, nscount, buflen;
    int			origClass;
    int			numAliases = 0;
    int			numAddresses = 0;
    int			n, i, j;
    int			len;
    int			dlen;
    int			status;
    int			numServers;
    Boolean		haveAnswer;
    Boolean		printedAnswers = FALSE;


    /*
     *  If the hostPtr was used before, free up the calloc'd areas.
     */
    FreeHostInfoPtr(hostPtr);

    status = SendRequest(nsAddrPtr, msg, msglen, (char *) &answer,
			    sizeof(answer), &n);

    if (status != SUCCESS) {
	    if (_res.options & RES_DEBUG2)
		    printf("SendRequest failed\n");
	    return (status);
    }
    eom = (char *) &answer + n;

    headerPtr = (HEADER *) &answer;

    if (headerPtr->rcode != NOERROR) {
	return (headerPtr->rcode);
    }

    qdcount = ntohs(headerPtr->qdcount);
    ancount = ntohs(headerPtr->ancount);
    arcount = ntohs(headerPtr->arcount);
    nscount = ntohs(headerPtr->nscount);

    /*
     * If there are no answer, n.s. or additional records
     * then return with an error.
     */
    if (ancount == 0 && nscount == 0 && arcount == 0) {
	return (NO_INFO);
    }


    bp	   = hostbuf;
    buflen = sizeof(hostbuf);
    cp	   = (char *) &answer + sizeof(HEADER);

    /* Skip over question section. */
    while (qdcount-- > 0) {
	cp += dn_skipname(cp, eom) + QFIXEDSZ;
    }

    aliasPtr	= host_aliases;
    addrPtr	= addr_list;
    haveAnswer	= FALSE;

    /*
     * Scan through the answer resource records.
     * Answers for address query types are saved.
     * Other query type answers are just printed.
     */
    if (ancount != 0) {
	if (!isServer && !headerPtr->aa) {
	    printf("Non-authoritative answer:\n");
	}

	if (queryType != T_A && !(iquery && queryType == T_PTR)) {
	    while (--ancount >= 0 && cp < eom) {
		if ((cp = Print_rr(cp, (char *)&answer, eom, stdout)) == NULL) {
		    return(ERROR);
		}
	    }
	    printedAnswers = TRUE;
	} else {
	    while (--ancount >= 0 && cp < eom) {
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0) {
		    return(ERROR);
		}
		cp   += n;
		type  = GetShort(cp);
		class = GetShort(cp);
		cp   += sizeof(u_long);	/* skip TTL */
		dlen  = GetShort(cp);
		if (type == T_CNAME) {
		    /*
		     * Found an alias.
		     */
		    cp += dlen;
		    if (aliasPtr >= &host_aliases[MAXALIASES-1]) {
			continue;
		    }
		    *aliasPtr++ = bp;
		    n = strlen(bp) + 1;
		    host_aliases_len[numAliases] = n;
		    numAliases++;
		    bp += n;
		    buflen -= n;
		    continue;
		} else if (type == T_PTR) {
		    /*
		     *  Found a "pointer" to the real name.
		     */
		    if((n=dn_expand((char *)&answer, eom, cp, bp,buflen)) < 0) {
			cp += n;
			continue;
		    }
		    cp += n;
		    len = strlen(bp) + 1;
		    hostPtr->name = Calloc(1, len);
		    bcopy(bp, hostPtr->name, len);
		    haveAnswer = TRUE;
		    break;
		} else if (type != T_A) {
		    cp += dlen;
		    continue;
		}
		if (haveAnswer) {
		    /*
		     * If we've already got 1 address, we aren't interested
		     * in addresses with a different length or class.
		     */
		    if (dlen != hostPtr->addrLen) {
			cp += dlen;
			continue;
		    }
		    if (class != origClass) {
			cp += dlen;
			continue;
		    }
		} else {
		    /*
		     * First address: record its length and class so we
		     * only save additonal ones with the same attributes.
		     */
		    hostPtr->addrLen = dlen;
		    origClass = class;
		    hostPtr->addrType = (class == C_IN) ? AF_INET : AF_UNSPEC;
		    len = strlen(bp) + 1;
		    hostPtr->name = Calloc(1, len);
		    bcopy(bp, hostPtr->name, len);
		}
		bp += (((u_long)bp) % sizeof(align));

		if (bp + dlen >= &hostbuf[sizeof(hostbuf)]) {
		    if (_res.options & RES_DEBUG) {
			printf("Size (%d) too big\n", dlen);
		    }
		    break;
		}
		bcopy(cp, *addrPtr++ = bp, dlen);
		bp +=dlen;
		cp += dlen;
		numAddresses++;
		haveAnswer = TRUE;
	    }
	}
    }

    if ((queryType == T_A || queryType == T_PTR) && haveAnswer) {

	/*
	 *  Go through the alias and address lists and return them
	 *  in the hostPtr variable.
	 */

	if (numAliases > 0) {
	    hostPtr->aliases = (char **) Calloc(1 + numAliases, sizeof(char *));
	    for (i = 0; i < numAliases; i++) {
		hostPtr->aliases[i] = Calloc(1, host_aliases_len[i]);
		bcopy(host_aliases[i], hostPtr->aliases[i],host_aliases_len[i]);
	    }
	    hostPtr->aliases[i] = NULL;
	}
	if (numAddresses > 0) {
	    hostPtr->addrList = (char **)Calloc(1+numAddresses, sizeof(char *));
	    for (i = 0; i < numAddresses; i++) {
		hostPtr->addrList[i] = Calloc(1, hostPtr->addrLen);
		bcopy(addr_list[i], hostPtr->addrList[i], hostPtr->addrLen);
	    }
	    hostPtr->addrList[i] = NULL;
	}
#ifdef verbose
	if (headerPtr->aa || nscount == 0) {
	    hostPtr->servers = NULL;
	    return (SUCCESS);
	}
#else
	hostPtr->servers = NULL;
	return (SUCCESS);
#endif
    }

    /*
     * At this point, for the T_A query type, only empty answers remain.
     * For other query types, additional information might be found
     * in the additional resource records part.
     */

    if (!headerPtr->aa && (queryType != T_A) && (nscount > 0 || arcount > 0)) {
	if (printedAnswers) {
	    putchar('\n');
	}
	printf("Authoritative answers can be found from:\n");
    }

    cp = res_skip((char *) &answer, 2, eom);

    numServers = 0;
    if (queryType != T_A) {
	/*
	 * If we don't need to save the record, just print it.
	 */
	while (--nscount >= 0 && cp < eom) {
	    if ((cp = Print_rr(cp, (char *) &answer, eom, stdout)) == NULL) {
		return(ERROR);
	    }
	}
    } else {
	while (--nscount >= 0 && cp < eom) {
	    /*
	     *  Go through the NS records and retrieve the names of hosts
	     *  that serve the requested domain.
	     */

	    if ((n = dn_expand((char *) &answer, eom, cp, bp, buflen)) < 0) {
		return(ERROR);
	    }
	    cp += n;
	    len = strlen(bp) + 1;
	    dnamePtr = Calloc(1, len);   /* domain name */
	    bcopy(bp, dnamePtr, len);

	    type  = GetShort(cp);
	    class = GetShort(cp);
	    cp   += sizeof(u_long);	/* skip TTL */
	    dlen  = GetShort(cp);

	    if (type != T_NS) {
		cp += dlen;
	    } else {
		Boolean	found;

		if ((n = dn_expand((char *) &answer, eom, cp, bp, buflen)) < 0){
		    return(ERROR);
		}
		cp += n;
		len = strlen(bp) + 1;
		namePtr = Calloc(1, len); /* server host name */
		bcopy(bp, namePtr, len);

		/*
		 * Store the information keyed by the server host name.
		 */
		found = FALSE;
		for (j = 0; j < numServers; j++) {
		    if (strcmp(namePtr, server[j].name) == 0) {
			found = TRUE;
			free(namePtr);
			break;
		    }
		}
		if (found) {
		    server[j].numDomains++;
		    if (server[j].numDomains <= MAXDOMAINS) {
			server[j].domain[server[j].numDomains-1] = dnamePtr;
		    }
		} else {
		    if (numServers >= MAXSERVERS) {
			break;
		    }
		    server[numServers].name = namePtr;
		    server[numServers].domain[0] = dnamePtr;
		    server[numServers].numDomains = 1;
		    server[numServers].numAddresses = 0;
		    numServers++;
		}
	    }
	}
    }

    /*
     * Additional resource records contain addresses of servers.
     */
    cp = res_skip((char *) &answer, 3, eom);

    if (queryType != T_A) {
	/*
	 * If we don't need to save the record, just print it.
	 */
	while (--arcount >= 0 && cp < eom) {
	    if ((cp = Print_rr(cp, (char *) &answer, eom, stdout)) == NULL) {
		return(ERROR);
	    }
	}
    } else {
	while (--arcount >= 0 && cp < eom) {
	    if ((n = dn_expand((char *) &answer, eom, cp, bp, buflen)) < 0) {
		break;
	    }
	    cp   += n;
	    type  = GetShort(cp);
	    class = GetShort(cp);
	    cp   += sizeof(u_long);	/* skip TTL */
	    dlen  = GetShort(cp);

	    if (type != T_A)  {
		cp += dlen;
		continue;
	    } else {
		for (j = 0; j < numServers; j++) {
		    if (strcmp(bp, server[j].name) == 0) {
			server[j].numAddresses++;
			if (server[j].numAddresses <= MAXADDRS) {
			    server[j].address[server[j].numAddresses-1] = 
				    				Calloc(1,dlen);
			    bcopy(cp,
			      server[j].address[server[j].numAddresses-1],dlen);
			    break;
			}
		    }
		}
		cp += dlen;
	    }
	}
    }

    /*
     * If we are returning name server info, transfer it to the hostPtr.
     */
    if (numServers > 0) {
	hostPtr->servers = (ServerInfo **)
				Calloc(numServers+1, sizeof(ServerInfo *));

	for (i = 0; i < numServers; i++) {
	    hostPtr->servers[i] = (ServerInfo *) Calloc(1, sizeof(ServerInfo));
	    hostPtr->servers[i]->name = server[i].name;


	    hostPtr->servers[i]->domains = (char **)
				Calloc(server[i].numDomains+1,sizeof(char *));
	    for (j = 0; j < server[i].numDomains; j++) {
		hostPtr->servers[i]->domains[j] = server[i].domain[j];
	    }
	    hostPtr->servers[i]->domains[j] = NULL;


	    hostPtr->servers[i]->addrList = (char **)
				Calloc(server[i].numAddresses+1,sizeof(char *));
	    for (j = 0; j < server[i].numAddresses; j++) {
		hostPtr->servers[i]->addrList[j] = server[i].address[j];
	    }
	    hostPtr->servers[i]->addrList[j] = NULL;

	}
	hostPtr->servers[i] = NULL;
    }

    switch (queryType) {
	case T_A:
		return NONAUTH;
	case T_PTR:
		if (iquery)
			return NO_INFO;
		/* fall through */
	default:
		return SUCCESS;
    }
}

/*
*******************************************************************************
*
*  GetHostInfo --
*
*	Retrieves host name, address and alias information
*	for a domain.
*
*	Algorithm from res_search().
*
*  Results:
*	ERROR		- res_mkquery failed.
*	+ return values from GetAnswer()
*
*******************************************************************************
*/

int
GetHostInfoByName(nsAddrPtr, queryClass, queryType, name, hostPtr, isServer)
    struct in_addr	*nsAddrPtr;
    int			queryClass;
    int			queryType;
    char		*name;
    HostInfo		*hostPtr;
    Boolean		isServer;
{
    int			n;
    register int	result;
    register char	*cp, **domain;
    extern char		*hostalias();
    Boolean		got_nodata = FALSE;
    unsigned long	ina;

    /* Catch explicit addresses */
    if ((queryType == T_A) && IsAddr(name, &ina)) {
	hostPtr->name = Calloc(strlen(name)+3, 1);
	(void)sprintf(hostPtr->name,"[%s]",name);
	hostPtr->aliases = NULL;
	hostPtr->servers = NULL;
	hostPtr->addrType = AF_INET;
	hostPtr->addrLen = sizeof(struct in_addr);
	hostPtr->addrList = (char **)Calloc(2, sizeof(char *));
	hostPtr->addrList[0] = Calloc(sizeof(long), sizeof(char));
	bcopy((char *)&ina, hostPtr->addrList[0], sizeof(ina));
	hostPtr->addrList[1] = NULL;
	return(SUCCESS);
    }

    result = NXDOMAIN;
    for (cp = name, n = 0; *cp; cp++)
	    if (*cp == '.')
		    n++;
    if (n == 0 && (cp = hostalias(name))) {
	    printf("Aliased to \"%s\"\n\n", cp);
	    return (GetHostDomain(nsAddrPtr, queryClass, queryType,
		    cp, (char *)NULL, hostPtr, isServer));
    }
    /*
     * We do at least one level of search if
     *	- there is no dot and RES_DEFNAME is set, or
     *	- there is at least one dot, there is no trailing dot,
     *	  and RES_DNSRCH is set.
     */
    if ((n == 0 && _res.options & RES_DEFNAMES) ||
       (n != 0 && *--cp != '.' && _res.options & RES_DNSRCH))
	 for (domain = _res.dnsrch; *domain; domain++) {
	    result = GetHostDomain(nsAddrPtr, queryClass, queryType,
		    name, *domain, hostPtr, isServer);
	    /*
	     * If no server present, give up.
	     * If name isn't found in this domain,
	     * keep trying higher domains in the search list
	     * (if that's enabled).
	     * On a NO_INFO error, keep trying, otherwise
	     * a wildcard entry of another type could keep us
	     * from finding this entry higher in the domain.
	     * If we get some other error (negative answer or
	     * server failure), then stop searching up,
	     * but try the input name below in case it's fully-qualified.
	     */
	    if (result == SUCCESS || result == NO_RESPONSE)
		    return result;
	    if (result == NO_INFO)
		    got_nodata++;
	    if ((result != NXDOMAIN && result != NO_INFO) ||
		(_res.options & RES_DNSRCH) == 0)
		    break;
    }
    /*
     * If the search/default failed, try the name as fully-qualified,
     * but only if it contained at least one dot (even trailing).
     * This is purely a heuristic; we assume that any reasonable query
     * about a top-level domain (for servers, SOA, etc) will not use
     * res_search.
     */
    if (n && (result = GetHostDomain(nsAddrPtr, queryClass, queryType,
		    name, (char *)NULL, hostPtr, isServer)) == SUCCESS)
	    return result;
    if (got_nodata)
	result = NO_INFO;
    return (result);
}

/*
 * Perform a query on the concatenation of name and domain,
 * removing a trailing dot from name if domain is NULL.
 */
GetHostDomain(nsAddrPtr, queryClass, queryType, name, domain, hostPtr, isServer)
    struct in_addr	*nsAddrPtr;
    int			queryClass;
    int			queryType;
    char		*name, *domain;
    HostInfo		*hostPtr;
    Boolean		isServer;
{
    querybuf buf;
    char nbuf[2*MAXDNAME+2];
    char *longname = nbuf;
    int n;

    if (domain == NULL) {
	    /*
	     * Check for trailing '.';
	     * copy without '.' if present.
	     */
	    n = strlen(name) - 1;
	    if (name[n] == '.' && n < sizeof(nbuf) - 1) {
		    bcopy(name, nbuf, n);
		    nbuf[n] = '\0';
	    } else
		    longname = name;
    } else {
	    (void)sprintf(nbuf, "%.*s.%.*s",
		    MAXDNAME, name, MAXDNAME, domain);
	    longname = nbuf;
    }
    n = res_mkquery(QUERY, longname, queryClass, queryType,
		    (char *)0, 0, (char *)0, (char *) &buf, sizeof(buf));
    if (n < 0) {
	if (_res.options & RES_DEBUG) {
	    printf("Res_mkquery failed\n");
	}
	return (ERROR);
    }

    n = GetAnswer(nsAddrPtr, queryType, (char *)&buf, n, 0, hostPtr, isServer);

    /*
     * GetAnswer didn't find a name, so set it to the specified one.
     */
    if (n == NONAUTH) {
	if (hostPtr->name == NULL) {
	    int len = strlen(longname) + 1;
	    hostPtr->name = Calloc(len, sizeof(char));
	    bcopy(longname, hostPtr->name, len);
	}
    }
    return(n);
}


/*
*******************************************************************************
*
*  GetHostInfoByAddr --
*
*	Performs an inverse query to find the host name
*	that corresponds to the given address.
*
*  Results:
*	ERROR		- res_mkquery failed.
*	+ return values from GetAnswer()
*
*******************************************************************************
*/

int
GetHostInfoByAddr(nsAddrPtr, address, hostPtr)
    struct in_addr	*nsAddrPtr;
    struct in_addr	*address;
    HostInfo		*hostPtr;
{
    int		n;
    querybuf	buf;
    char	qbuf[MAXDNAME];
    char	*p = (char *) &address->s_addr;

    (void)sprintf(qbuf, "%u.%u.%u.%u.in-addr.arpa",
	    ((unsigned)p[3] & 0xff),
	    ((unsigned)p[2] & 0xff),
	    ((unsigned)p[1] & 0xff),
	    ((unsigned)p[0] & 0xff));
    n = res_mkquery(QUERY, qbuf, C_IN, T_PTR,
	    NULL,  0, NULL, (char *) &buf, sizeof(buf));
    if (n < 0) {
	if (_res.options & RES_DEBUG) {
	    printf("res_mkquery() failed\n");
	}
	return (ERROR);
    }
    n = GetAnswer(nsAddrPtr, T_PTR, (char *) &buf, n, 1, hostPtr, 1);
    if (n == SUCCESS) {
	hostPtr->addrType = AF_INET;
	hostPtr->addrLen = 4;
	hostPtr->addrList = (char **)Calloc(2, sizeof(char *));
	hostPtr->addrList[0] = Calloc(sizeof(long), sizeof(char));
	bcopy((char *)p, hostPtr->addrList[0], sizeof(struct in_addr));
	hostPtr->addrList[1] = NULL;
    }
    return n;
}

/*
*******************************************************************************
*
*  FreeHostInfoPtr --
*
*	Deallocates all the calloc'd areas for a HostInfo variable.
*
*******************************************************************************
*/

void
FreeHostInfoPtr(hostPtr)
    register HostInfo *hostPtr;
{
    int i, j;

    if (hostPtr->name != NULL) {
	free(hostPtr->name);
	hostPtr->name = NULL;
    }

    if (hostPtr->aliases != NULL) {
	i = 0;
	while (hostPtr->aliases[i] != NULL) {
	    free(hostPtr->aliases[i]);
	    i++;
	}
	free((char *)hostPtr->aliases);
	hostPtr->aliases = NULL;
    }

    if (hostPtr->addrList != NULL) {
	i = 0;
	while (hostPtr->addrList[i] != NULL) {
	    free(hostPtr->addrList[i]);
	    i++;
	}
	free((char *)hostPtr->addrList);
	hostPtr->addrList = NULL;
    }

    if (hostPtr->servers != NULL) {
	i = 0;
	while (hostPtr->servers[i] != NULL) {

	    if (hostPtr->servers[i]->name != NULL) {
		free(hostPtr->servers[i]->name);
	    }

	    if (hostPtr->servers[i]->domains != NULL) {
		j = 0;
		while (hostPtr->servers[i]->domains[j] != NULL) {
		    free(hostPtr->servers[i]->domains[j]);
		    j++;
		}
		free((char *)hostPtr->servers[i]->domains);
	    }

	    if (hostPtr->servers[i]->addrList != NULL) {
		j = 0;
		while (hostPtr->servers[i]->addrList[j] != NULL) {
		    free(hostPtr->servers[i]->addrList[j]);
		    j++;
		}
		free((char *)hostPtr->servers[i]->addrList);
	    }
	    free((char *)hostPtr->servers[i]);
	    i++;
	}
	free((char *)hostPtr->servers);
	hostPtr->servers = NULL;
    }
}
