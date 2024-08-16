/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:sharedObjects/dns/domain.c	1.3"
#define	DOMAINOBJ

#if PACKETSZ > 4096
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	4096
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<malloc.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include	<arpa/nameser.h>
#include	<mail/link.h>
#include	<mail/table.h>
#include	<mail/tree.h>
#include	<mail/hosts.h>

#include	"conn.h"

typedef struct recordTypes_s
    {
    unsigned
	rt_a	:1,
	rt_cname:1,
	rt_ns	:1;
    }	recordTypes_t;

typedef union
    {
    HEADER
	hdr;
    u_char
	buf[MAXPACKET];
    }	querybuf;

typedef struct domainEntry_s
    {
    void
	*de_nsList,
	*de_privateData;

    char
	*de_name,
	**de_nsNameList;

    unsigned
	de_nameserverPos,
	de_level,
	de_soaCount,
	de_nsCount,
	de_freeLock:1;

    treeList_t
	*de_treeList;

    conn_t
	*de_nameserver;

    table_t
	*de_cname,
	*de_names;
    }	domainEntry_t;
  
#include	"domain.h"

static table_t
    *DomainTable = NULL;

static int
    DebugLevel = 0;

int
    stricmp(char *str1, char *str2)
        {
	int
	    result;

	if(DebugLevel > 4)
	    {
	    fprintf
		(
		stderr,
		"stricmp(%s, %s) Entered.\n",
		str1,
		str2
		);
	    }

	while(!(result = toupper(*str1) - toupper(*str2++)) && *str1++ != '\0');

	if(DebugLevel > 4)
	    {
	    fprintf
		(
		stderr,
		"stricmp() = %d Exited.\n",
		result
		);
	    }

	return(result);
	}

void
    freeElement(void *element)
	{
	void
	    *owner;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"freeElement(0x%x) Entered.\n",
		(int) element
		);
	    }

	if(element == NULL)
	    {
	    }
	else
	    {
	    if((owner = linkOwner(element)) != NULL) free(owner);
	    linkFree(element);
	    }

	if(DebugLevel > 4)
	    {
	    (void) fprintf(stderr, "freeElement() Exited.\n");
	    }
	}

static void
    rcvNs
	(
	conn_t *conn_p,
	domainEntry_t *domainEntry_p,
	querybuf *answer,
	int anslen
	)
	{
	register HEADER
	    *hp;
	register u_char
	    *cp;
	register int
	    n;

	domainEntry_t
	    *tmpDomain;

	recordTypes_t
	    *newType,
	    *tmpType;

	u_char
	    *eom;

	void
	    *curHost_p,
	    *newLink_p;

	char
	    nameBuff[256],
	    *recordName,
	    *firstDot,
	    *newName,
	    *bp;

	int
	    type,
	    buflen,
	    rcode,
	    ancount,
	    qdcount;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"rcvNs(0x%x, 0x%x, 0x%x, %d) Entered.\n",
		(int) conn_p,
		(int) domainEntry_p,
		(int) answer,
		anslen
		);
	    }

	if(anslen >= sizeof(HEADER))
	    {
	    eom = answer->buf + anslen;
	    /*
	     * find first satisfactory answer
	     */
	    hp = &answer->hdr;
	    rcode = hp->rcode;
	    ancount = ntohs((int) hp->ancount);
	    qdcount = ntohs((int) hp->qdcount);
	    cp = answer->buf + sizeof(HEADER);
	    if (qdcount)
		{
		cp += dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
		    {
		    cp += dn_skipname(cp, eom) + QFIXEDSZ;
		    }
		}
	    }
	else if(anslen == 0)
	    {
	    treeListCallbackDo(domainEntry_p->de_treeList, 1);
	    ancount = 0;
	    cp = answer->buf;
	    eom = cp;
	    }
	else
	    {
	    ancount = 0;
	    cp = answer->buf;
	    eom = cp;
	    }

	if(rcode != 0)
	    {
	    if(domainEntry_p->de_nameserver != NULL)
		{
		connectionFree(domainEntry_p->de_nameserver);
		domainEntry_p->de_nameserver = 0;
		}

	    domainOpen(domainEntry_p->de_treeList, domainEntry_p);
	    }
	else while (--ancount >= 0 && cp < eom)
	    {
	    newType = NULL;
	    bp = nameBuff;
	    buflen = sizeof(nameBuff);
	    if ((n = dn_expand((char *)answer->buf, eom, cp, bp, buflen)) < 0) break;
	    recordName = bp;
	    bp += n;
	    buflen -= n;
	    cp += n;

	    if(DebugLevel > 7)
		{
		(void) fprintf
		    (
		    stderr,
		    "\trecordName = %s.\n",
		    recordName
		    );
		}

	    if((firstDot = strchr(recordName, '.')) == NULL)
		{
		tmpDomain = NULL;
		tmpType = NULL;
		}
	    else if((tmpDomain = domainEntryNew(firstDot + 1, NULL)) != NULL)
		{
		tmpType = (recordTypes_t *)tableGetValueByNoCaseString
		    (
		    tmpDomain->de_names,
		    recordName
		    );
		}

	    type = _getshort(cp);
	    cp += sizeof(u_short);
	    cp += sizeof(u_short) + sizeof(u_long);
	    n = _getshort(cp);
	    cp += sizeof(u_short);
	    if(DebugLevel > 6)
		{
		char
		    *chartype;

		switch(type)
		    {
		    default:	chartype="UNKOWN"; break;
		    case	T_A: chartype = "T_A"; break;
		    case	T_NS: chartype = "T_NS"; break;
		    case	T_MD: chartype = "T_MD"; break;
		    case	T_MF: chartype = "T_MF"; break;
		    case	T_CNAME: chartype = "T_CNAME"; break;
		    case	T_SOA: chartype = "T_SOA"; break;
		    case	T_MB: chartype = "T_MB"; break;
		    case	T_MG: chartype = "T_MG"; break;
		    case	T_MR: chartype = "T_MR"; break;
		    case	T_NULL: chartype = "T_NULL"; break;
		    case	T_WKS: chartype = "T_WKS"; break;
		    case	T_PTR: chartype = "T_PTR"; break;
		    case	T_HINFO: chartype = "T_HINFO"; break;
		    case	T_MINFO: chartype = "T_MINFO"; break;
		    case	T_MX: chartype = "T_MX"; break;
		    case	T_TXT: chartype = "T_TXT"; break;
		    case	T_UINFO: chartype = "T_UINFO"; break;
		    case	T_UID: chartype = "T_UID"; break;
		    case	T_GID: chartype = "T_GID"; break;
		    case	T_UNSPEC: chartype = "T_UNSPEC"; break;
		    }
		
		fprintf(stderr, "\ttype = %s.\n", chartype);
		}

	    switch(type)
		{
		default:
		    {
		    if(DebugLevel > 8)
			{
			fprintf
			    (
			    stderr,
			    "\tUnknown type = %d.\n",
			    type
			    );
			}

		    cp += n;
		    break;
		    }

		case	T_A:
		    {
		    domainEntry_t
			*otherDomain;

		    unsigned long
			address;
		    
		    char
			*dotPos = NULL,
			*otherName,
			addrBuff[20];

		    if(tmpDomain != NULL && (tmpType == NULL || !tmpType->rt_a))
			{
			if(tmpType != NULL)
			    {
			    tmpType->rt_a = 1;
			    }
			else if
			    (
				(
				newType = (recordTypes_t *)calloc
				    (
				    sizeof(*newType),
				    1
				    )
				) != NULL
			    )
			    {
			    newType->rt_a = 1;
			    }

			while
			    (
				(
				otherName = (char *)tableGetValueByNoCaseString
				    (
				    tmpDomain->de_cname,
				    recordName
				    )
				) != NULL
			    )
			    {
			    int
				deletedFlag;

			    if(DebugLevel > 8)
				{
				(void) fprintf
				    (
				    stderr,
				    "\tstartLoop: table = 0x%x, otherName = 0x%x:%s\n",
				    (int) tmpDomain->de_cname,
				    (int) otherName,
				    otherName
				    );
				}

			    if((dotPos = strchr(otherName, '.')) == NULL)
				{
				}
			    else if
			        (
				    (
				    otherDomain = domainEntryNew
					(
					dotPos + 1,
					NULL
					)
				    ) == NULL
				)
				{
				}

			    if((curHost_p = hostNew(otherName, "", NULL, NULL)) == NULL)
				{
				/* ERROR No Memory */
				}
			    else
				{
				if(dotPos != NULL)
				    {
				    *dotPos = '\0';
				    }

				if
				    (
				    nodeNew
					(
					otherName,
					otherDomain->de_treeList,
					curHost_p,
					hostFree,
					NULL,
					NULL
					) == NULL
				    )
				    {
				    hostFree(curHost_p);
				    }

				if(dotPos != NULL)
				    {
				    *dotPos = '.';
				    }
				}

			    deletedFlag = tableDeleteEntryByValue
				(
				tmpDomain->de_cname,
				(void *)otherName
				);


			    if(DebugLevel > 8)
				{
				(void) fprintf
				    (
				    stderr,
				    "\tendLoop: table = 0x%x, otherName = 0x%x:%s %s\n",
				    (int) tmpDomain->de_cname,
				    (int) otherName,
				    otherName,
				    (deletedFlag)? "Deleted": "Not Deleted"
				    );
				}
			    }

			address = _getlong(cp);
			(void) sprintf
			    (
			    addrBuff,
			    "%d.%d.%d.%d",
			    (int) address>>24,
			    (int) (address>>16)&0xff,
			    (int) (address>>8)&0xff,
			    (int) address&0xff
			    );

			if((curHost_p = hostNew(recordName, addrBuff, NULL, NULL)) == NULL)
			    {
			    /* ERROR No Memory */
			    }
			else
			    {
			    if(firstDot != NULL)
				{
				*firstDot = '\0';
				}

			    if
				(
				nodeNew
				    (
				    recordName,
				    tmpDomain->de_treeList,
				    curHost_p,
				    hostFree,
				    NULL,
				    NULL
				    ) == NULL
				)
				{
				hostFree(curHost_p);
				}

			    if(firstDot != NULL)
				{
				*firstDot = '.';
				}
			    }
			}

		    cp += n;
		    break;
		    }

		case	T_CNAME:
		    {
		    /*
			Salt the name away and add it iff an A record comes in.
		    */
		    recordTypes_t
			*otherType;

		    domainEntry_t
			*otherDomain;

		    char
			*dotPos,
			*newName;

		    if(tmpDomain == NULL)
			{
			}
		    else
			{
			if(tmpType != NULL)
			    {
			    tmpType->rt_cname = 1;
			    }
			else if
			    (
				(
				newType = (recordTypes_t *)calloc(sizeof(*tmpType), 1)
				) != NULL
			    )
			    {
			    newType->rt_cname = 1;
			    }

			if(dn_expand((char *)answer->buf, eom, cp, bp, buflen) < 0)
			    {
			    }
			else
			    {
			    if((dotPos = strchr(bp, '.')) == NULL)
				{
				}
			    else if
				(
				    (
				    otherDomain = domainEntryNew
					(
					dotPos + 1,
					NULL
					)
				    ) == NULL
				)
				{
				}
			    else if
				(
				    (
				    otherType = (recordTypes_t *)tableGetValueByNoCaseString
					(
					otherDomain->de_names,
					bp
					)
				    ) == NULL
				)
				{
				if((newName = strdup(recordName)) == NULL)
				    {
				    }
				else
				    {
				    tableAddEntry(otherDomain->de_cname, bp, newName, free);
				    }
				}
			    else if(!otherType->rt_a)
				{
				if((newName = strdup(recordName)) == NULL)
				    {
				    }
				else
				    {
				    tableAddEntry(otherDomain->de_cname, bp, newName, free);
				    }
				}
			    else if((curHost_p = hostNew(recordName, "", NULL, NULL)) == NULL)
				{
				/* ERROR No Memory */
				}
			    else
				{
				if(firstDot != NULL) *firstDot = '\0';
				if
				    (
				    nodeNew
					(
					recordName,
					tmpDomain->de_treeList,
					curHost_p,
					hostFree,
					NULL,
					NULL
					) == NULL
				    )
				    {
				    }
				if(firstDot != NULL) *firstDot = '.';
				}
			    }

			}

		    cp += n;
		    break;
		    }

		/*
		    The needed datum in the NS and SOA records happens
		to be in the same place.
		*/
		case	T_SOA:
		    {
		    tmpDomain = domainEntryNew(recordName, NULL);

		    if(DebugLevel > 8)
			{
			(void) fprintf
			    (
			    stderr,
			    "\tbefore domainEntry = 0x%x soa count = %d.\n",
			    (int) domainEntry_p,
			    domainEntry_p->de_soaCount
			    );
			}

		    if(tmpDomain != domainEntry_p)
			{
			}
		    else if(++(domainEntry_p->de_soaCount) >= 2)
			{
			treeListCallbackDo(domainEntry_p->de_treeList, 1);
			break;
			}

		    if(DebugLevel > 8)
			{
			(void) fprintf
			    (
			    stderr,
			    "\tafter domainEntry = 0x%x soa count = %d.\n",
			    (int) domainEntry_p,
			    domainEntry_p->de_soaCount
			    );
			}

		    /* Deliberate fall through */
		    }

		case	T_NS:
		    {
		    if((tmpDomain = domainEntryNew(recordName, NULL)) == NULL)
			{
			}
		    else
			{
			if
			    (
				(
				tmpType = (recordTypes_t *)tableGetValueByNoCaseString
				    (
				    tmpDomain->de_names,
				    recordName
				    )
				) != NULL
			    )
			    {
			    tmpType->rt_ns = 1;
			    }
			else if
			    (
				(
				newType = (recordTypes_t *)calloc
				    (
				    sizeof(*tmpType),
				    1
				    )
				) != NULL
			    )
			    {
			    newType->rt_ns = 1;
			    }

			if
			    (
			    dn_expand
				(
				(char *)answer->buf,
				eom,
				cp,
				bp,
				buflen
				) < 0
			    )
			    {
			    }
			else if((newName = strdup(bp)) == NULL)
			    {
			    }
			else if((newLink_p = linkNew(newName)) == NULL)
			    {
			    free(newName);
			    }
			else if
			    (
			    !linkAddSortedUnique
				(
				tmpDomain->de_nsList,
				newLink_p,
				stricmp,
				freeElement
				)
			    )
			    {
			    tmpDomain->de_nsCount++;

			    if(DebugLevel > 5)
				{
				(void) fprintf
				    (
				    stderr,
				    "\tnameserver %s added to 0x%x.\n",
				    bp,
				    (int) tmpDomain
				    );
				}
			    }
			}

		    cp += n;
		    break;
		    }
		}

	    if(newType != NULL)
		{
		tableAddEntry(tmpDomain->de_names, recordName, newType, free);
		newType = NULL;
		}
	    }

	if(DebugLevel > 4)
	    {
	    (void) fprintf(stderr, "rcvNs() Exited.\n");
	    }
	}

static void
    gotConnectionCallback(domainEntry_t *domainEntry_p, conn_t *conn_p)
	{
	/*
	    We may have a connection to the nameserver.
	Now we need to do a zone transfer.
	*/
	int
	    n;

	char
	    queryBuff[1024];

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"gotConnectionCallback(0x%x, 0x%x) Entered.\n",
		(int) domainEntry_p,
		(int) conn_p
		);
	    }

	domainEntry_p->de_nameserver = conn_p;

	if(conn_p == NULL)
	    {
	    domainOpen(domainEntry_p->de_treeList, domainEntry_p);
	    }
	else if
	    (
	    n = res_mkquery
		(
		QUERY,
		domainEntry_p->de_name,
		C_IN,
		T_AXFR,
		NULL,
		0,
		NULL,
		queryBuff,
		sizeof(queryBuff)
		)
	    )
	    {
	    unsigned short
		wireCount;

	    domainEntry_p->de_soaCount = 0;
	    wireCount = htons(n);
	    connectionSend
		(
		conn_p,
		(char *)&wireCount,
		2,
		rcvNs
		);

	    connectionSend
		(
		conn_p,
		queryBuff,
		n,
		rcvNs
		);
	    }
	else
	    {
	    treeListCallbackDo(domainEntry_p->de_treeList, 0);
	    }

	if(DebugLevel > 4)
	    {
	    (void) fprintf(stderr, "gotConnectionCallback() Exited.\n");
	    }
	}

void
    domainClose(treeList_t *treeList_p, domainEntry_t *domainEntry_p)
	{
	}

int
    domainOpen(treeList_t *treeList_p, domainEntry_t *domainEntry_p)
	{
	querybuf
	    buf;

	int
	    result = 0,
	    n;

	char
	    **curName_p;
	
	void
	    *curLink_p;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"domainOpen(0x%x, 0x%x) Entered.\n",
		(int) treeList_p,
		(int) domainEntry_p
		);
	    }

	if(domainEntry_p == NULL)
	    {
	    }
	else if(domainEntry_p->de_nsCount)
	    {
	    /* We already got the nameservers. */
	    }
	else if
	    (
		(
		n = res_search
		    (
		    domainEntry_p->de_name,
		    C_IN,
		    T_NS,
		    buf.buf,
		    sizeof(buf)
		    )
		) >= 0
	    )
	    {
	    rcvNs(NULL, NULL, &buf, n);
	    }

	if(domainEntry_p == NULL)
	    {
	    treeListCallbackDo(treeList_p, 0);
	    result = 1;
	    }
	else if(domainEntry_p->de_level < 2)
	    {
	    /* The root nameservers do not support zone transfers. */
	    treeListCallbackDo(treeList_p, 0);
	    result = 1;
	    }
	else if(domainEntry_p->de_nameserver != NULL)
	    {
	    /* Do got nameserver connection callback here */
	    gotConnectionCallback(domainEntry_p, domainEntry_p->de_nameserver);
	    }
	else if(domainEntry_p->de_nsNameList != NULL)
	    {
	    /* Pick one and start the host connection get. */
	    if(domainEntry_p->de_nameserverPos >= domainEntry_p->de_nsCount)
		{
		treeListCallbackDo(treeList_p, 0);
		result = 1;
		}
	    else
		{
		connectionGetHost
		    (
		    /*domainEntry_p->de_nsNameList[rand()%domainEntry_p->de_nsCount],*/
		    domainEntry_p->de_nsNameList[domainEntry_p->de_nameserverPos++],
		    domainEntry_p,
		    gotConnectionCallback
		    );
		}
	    }
	else if
	    (
		(
		domainEntry_p->de_nsNameList = (char **)calloc
		    (
		    sizeof(*domainEntry_p->de_nsNameList),
		    domainEntry_p->de_nsCount + 1
		    )
		) == NULL
	    )
	    {
	    /* No Memory */
	    treeListCallbackDo(treeList_p, 0);
	    result = 1;
	    }
	else
	    {
	    if(DebugLevel > 8)
		{
		(void) fprintf
		    (
		    stderr,
		    "\tcreating nameserver list for 0x%x.\n",
		    (int) domainEntry_p
		    );
		}
	    for
		(
		curName_p = domainEntry_p->de_nsNameList,
		    curLink_p = linkLast(domainEntry_p->de_nsList);
		(*curName_p = (char *)linkOwner(curLink_p)) != NULL;
		curName_p++,
		    linkFree(curLink_p),
		    curLink_p = linkLast(domainEntry_p->de_nsList)
		)
		{
		if(DebugLevel > 8)
		    {
		    fprintf
			(
			stderr,
			"\tAdding %s to nameserver list.\n",
			*curName_p
			);
		    }
		}

	    /* pick one and start the host connection get. */
	    if(domainEntry_p->de_nameserverPos >= domainEntry_p->de_nsCount)
		{
		treeListCallbackDo(treeList_p, 0);
		result = 1;
		}
	    else
		{
		connectionGetHost
		    (
		    /*domainEntry_p->de_nsNameList[rand()%domainEntry_p->de_nsCount],*/
		    domainEntry_p->de_nsNameList[domainEntry_p->de_nameserverPos++],
		    domainEntry_p,
		    gotConnectionCallback
		    );
		}
	    }

	if(DebugLevel > 4) (void) fprintf(stderr, "domainOpen() = 0 Exited.\n");

	return(result);
	}

void
    domainEntryFree(domainEntry_t *domainEntry_p)
	{
	char
	    **curName_p,
	    *curName;

	void
	    *curLink_p;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"domainEntryFree(0x%x) Entered.\n",
		(int) domainEntry_p
		);
	    }

	if(domainEntry_p == NULL)
	    {
	    }
	else if(domainEntry_p->de_freeLock)
	    {
	    }
	else if(tableDeleteEntryByValue(DomainTable, (void *)domainEntry_p))
	    {
	    }
	else
	    {
	    domainEntry_p->de_freeLock = 1;
	    if(domainEntry_p->de_name != NULL) free(domainEntry_p->de_name);
	    if(domainEntry_p->de_cname != NULL) tableFree(domainEntry_p->de_cname);
	    if(domainEntry_p->de_names != NULL) tableFree(domainEntry_p->de_names);
	    if(domainEntry_p->de_nameserver != NULL) connectionFree(domainEntry_p->de_nameserver);
	    if(domainEntry_p->de_nsList != NULL)
		{
		while
		    (
			(
			curName = (char *)linkOwner
			    (
			    curLink_p = linkNext(domainEntry_p->de_nsList)
			    )
			) != NULL
		    )
		    {
		    free(curName);
		    linkFree(curLink_p);
		    }
		
		linkFree(domainEntry_p->de_nsList);
		}

	    if(domainEntry_p->de_nsNameList != NULL)
		{
		for
		    (
		    curName_p = domainEntry_p->de_nsNameList;
		    *curName_p != NULL;
		    curName_p++
		    )
		    {
		    free(*curName_p);
		    }
		
		free(domainEntry_p->de_nsNameList);
		}

	    free(domainEntry_p);
	    }

	if(DebugLevel > 4) (void)fprintf(stderr, "domainEntryFree() Exited.\n");
	}

domainEntry_t
    *domainEntryNew(char *domainName, treeList_t *treeList_p)
	{
	domainEntry_t
	    *result,
	    *parent;
	
	char
	    *dotPos;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"domainEntryNew(%s, 0x%x) Entered.\n",
		domainName,
		(int) treeList_p
		);
	    }

	if((dotPos = strchr(domainName, '.')) == NULL)
	    {
	    dotPos = ".";
	    }

	if
	    (
		(
		result = (domainEntry_t *)tableGetValueByNoCaseString
		    (
		    DomainTable,
		    domainName
		    )
		) != NULL
	    )
	    {
	    /* Got the domain. */
	    }
	else if(treeList_p != NULL)
	    {
	    }
	else if((parent = (domainEntry_t *)domainEntryNew(dotPos + 1, NULL)) == NULL)
	    {
	    /* ERROR No Parent */
	    result = NULL;
	    }
	
	if(result != NULL || (parent == NULL && treeList_p == NULL))
	    {
	    }
	else if((result = (domainEntry_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->de_nsList = linkNew(NULL)) == NULL)
	    {
	    domainEntryFree(result);
	    result = NULL;
	    }
	else if((result->de_cname = tableNew()) == NULL)
	    {
	    domainEntryFree(result);
	    result = NULL;
	    }
	else if((result->de_names = tableNew()) == NULL)
	    {
	    domainEntryFree(result);
	    result = NULL;
	    }
	else if((result->de_name = strdup(domainName)) == NULL)
	    {
	    domainEntryFree(result);
	    result = NULL;
	    }
	else
	    {
	    if(treeList_p == NULL)
		{
		*dotPos = '\0';
		result->de_treeList = treeListNew
		    (
		    domainName,
		    parent->de_treeList,
		    result,
		    domainEntryFree,
		    domainOpen,
		    domainClose,
		    NULL
		    );

		result->de_level = parent->de_level + 1;
		*dotPos = '.';
		}
	    else
		{
		result->de_treeList = treeList_p;
		result->de_level = 0;
		}

	    tableAddEntry(DomainTable, result->de_name, (void *)result, domainEntryFree);
	    }
	
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"domainEntryNew() = 0x%x Exited.\n",
		(int) result
		);
	    }

	return(result);
	}

void
    domainInit(int debugLevel)
	{
	static int
	    init = 0;
	
	DebugLevel = debugLevel;
	if(!(_res.options & RES_INIT))
	    {
	    res_init();
	    }
	
	if(!init)
	    {
	    init = 1;
	    srand(time(NULL));
	    DomainTable = tableNew();
	    }
	}
