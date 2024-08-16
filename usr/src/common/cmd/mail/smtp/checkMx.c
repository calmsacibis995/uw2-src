/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/checkMx.c	1.1"

#include	<unistd.h>
#include	<string.h>

#if PACKETSZ > 4096
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	4096
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h> 
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <malloc.h>
#include <sys/types.h>
#include	<sys/utsname.h>
#include <time.h>
#include	<mail/link.h>

#include	"smtpType.h"
#include	"smtp.h"

typedef union
    {
    HEADER
	hdr;
    u_char
	buf[MAXPACKET];
    }	querybuf;

typedef struct mxRecord_s
    {
    int
	mx_preference;
    
    char
	*mx_systemName;

    void
	*mx_link;
    }	mxRecord_t;

extern int
    stricmp();

static int
    SearchCount = 0;

static int
    mxRecordCmp(mxRecord_t *rec1_p, mxRecord_t *rec2_p)
	{
	return(rec1_p->mx_preference - rec2_p->mx_preference);
	}

static void
    mxRecordFree(mxRecord_t *rec_p)
	{
	if(rec_p != NULL)
	    {
	    if(rec_p->mx_link != NULL) linkFree(rec_p->mx_link);
	    if(rec_p->mx_systemName != NULL) free(rec_p->mx_systemName);
	    free(rec_p);
	    }
	}

static mxRecord_t
    *mxRecordNew(int preference, char *systemName)
	{
	mxRecord_t
	    *result;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"mxRecordNew(%d, %s) Entered.\n",
		preference,
		systemName
		);
	    }
	
	if((result = (mxRecord_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    }
	else if((result->mx_systemName = strdup(systemName)) == NULL)
	    {
	    mxRecordFree(result);
	    result = NULL;
	    }
	else if((result->mx_link = linkNew(result)) == NULL)
	    {
	    mxRecordFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->mx_preference = preference;
	    }
	
	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"mxRecordNew() = 0x%x Exiting.\n",
		(int) result
		);
	    }
	
	return(result);
	}

static void
    getanswer(querybuf *answer, int anslen, void *list)
	{
	register HEADER
	    *hp;
	register u_char
	    *cp;
	register int
	    n;
	u_char
	    *eom;
	char
	    nameBuff[256],
	    *bp,
	    *chartype;

	unsigned int
	    ancount,
	    qdcount;

	int
	    type,
	    class,
	    buflen,
	    getclass = C_ANY;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"getanswer(0x%x, %d, 0x%x) Entered.\n",
		(int) answer,
		anslen,
		(int) list
		);
	    }

	eom = answer->buf + anslen;
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
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

	while (--ancount >= 0 && cp < eom)
	    {
	    bp = nameBuff;
	    buflen = sizeof(nameBuff);
	    if ((n = dn_expand((char *)answer->buf, eom, cp, bp, buflen)) < 0) break;
	    cp += n;
	    type = _getshort(cp);
	    cp += sizeof(u_short);
	    class = _getshort(cp);
	    cp += sizeof(u_short) + sizeof(u_long);
	    n = _getshort(cp);
	    cp += sizeof(u_short);
switch(type)
{
default:
chartype="UNKOWN";
break;

case T_MX:
chartype="T_MX";
break;

case T_MG:
chartype="T_MG";
break;

case T_MR:
chartype="T_MR";
break;

case	T_MB:
chartype="T_MB";
break;

case T_PTR:
chartype="T_PTR";
break;

case T_CNAME:
chartype="T_CNAME";
break;

case	T_MINFO:
chartype="T_MINFO";
break;
}

	    if(DebugLevel > 5) (void) fprintf(stderr, "\ttype = %s.\n", chartype);

	    switch(type)
		{
		default:
		    {
		    cp += n;
		    break;
		    }
		
		case	T_MX:
		    {
		    mxRecord_t
			*mxRecord_p;

		    int
			preference;

		    preference = _getshort(cp);
		    cp += sizeof(u_short);
		    n -= sizeof(u_short);
		    if((dn_expand((char *)answer->buf, eom, cp, bp, buflen)) < 0) break;
		    if((mxRecord_p = mxRecordNew(preference, bp)) == NULL)
			{
			}
		    else
			{
			linkAddSorted
			    (
			    list,
			    mxRecord_p->mx_link,
			    mxRecordCmp
			    );
			}

		    cp += n;
		    break;
		    }
		}
	    }

	if(DebugLevel > 4) (void) fprintf(stderr, "getanswer() Exited.\n");
	}

int
    checkMxRecords(char *systemName)
	{
	mxRecord_t
	    *curRec_p;

	int
	    localPreference,
	    result = 0,
	    n;
	
	char
	    *domain,
	    systemNameBuff[256],
	    mySystemName[256];

	void
	    *list,
	    *curLink_p;

	querybuf
	    buf;

	struct utsname
	    utsname_s;

	if(DebugLevel > 4)
	    {
	    (void) fprintf
		(
		stderr,
		"checkMxRecords(%s) Entered.\n",
		systemName
		);
	    }

	if(!(_res.options & RES_INIT))
	    {
	    res_init();
	    }
	
	_res.options |= RES_DNSRCH;
	_res.options |= RES_USEVC;
	_res.options |= RES_STAYOPEN;
	/*_res.options |= RES_DEBUG;*/

	SearchCount++;

	(void)strcpy(systemNameBuff, systemName);
	(void)strcat(systemNameBuff, ".");
	systemName = systemNameBuff;

	if((domain = maildomain()) == NULL)
	    {
	    /* ERROR */
	    }
	else if(uname(&utsname_s) < 0)
	    {
	    /* ERROR */
	    }
	else if((list = linkNew(NULL)) == NULL)
	    {
	    }
	else if
	    (
		(
		n = res_search
		    (
		    systemName,
		    C_IN,
		    T_MX,
		    buf.buf,
		    sizeof(buf.buf)
		    )
		) > 0
	    )
	    {
	    getanswer(&buf, n, list);
	    (void) strcpy(mySystemName, utsname_s.nodename);
	    (void) strcat(mySystemName, domain);
	    for
		(
		localPreference = -1,
		    curLink_p = linkNext(list);
		(curRec_p = (mxRecord_t *)linkOwner(curLink_p)) != NULL;
		curLink_p = linkNext(curLink_p)
		)
		{
		if(!stricmp(curRec_p->mx_systemName, mySystemName))
		    {
		    localPreference = curRec_p->mx_preference;
		    break;
		    }
		}

	    while
		(
		!result &&
		    (
		    (curRec_p = (mxRecord_t *)linkOwner(linkNext(list))) != NULL
		    )
		)
		{
		if(DebugLevel > 7)
		    {
		    (void) fprintf
			(
			stderr,
			"sys = %s, pref = %d.\n",
			curRec_p->mx_systemName,
			curRec_p->mx_preference
			);
		    }

		if(localPreference < curRec_p->mx_preference)
		    {
		    result = 1;
		    }

		mxRecordFree(curRec_p);
		}

	    linkFree(list);
	    }

	if(DebugLevel > 4)
	    {
	    (void) fprintf(stderr, "checkMxRecords() = %d Exited.\n", result);
	    }

	return(result);
	}
