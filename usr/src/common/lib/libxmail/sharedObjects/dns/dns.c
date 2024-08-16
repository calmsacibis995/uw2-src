/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:sharedObjects/dns/dns.c	1.5"
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
#include <time.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <fcntl.h>
#include <net/if.h>
#include <stropts.h>
#include <signal.h>
#include	<mail/link.h>

typedef union
    {
    HEADER
	hdr;
    u_char
	buf[MAXPACKET];
    }	querybuf;

extern void
    freeElement();

extern int
    stricmp(),
    _mailerror;

static int
    CnameOk = 0,
    SearchCount = 0,
    AlarmHit = 0;

static void
    doAlarm(int stuff)
	{
	AlarmHit = 1;
	}

static int
    ipUp()
	{
	int
	    fd_ip;

	struct ifreq
	    ifr[2];

        struct strioctl
	    ioc;

	static enum
	    {
	    ip_unknown = 0,
	    ip_up,
	    ip_down,
	    }	ip_state;

	ioc.ic_cmd = SIOCGIFCONF;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(ifr);
	ioc.ic_dp = (void *)ifr;

	if(ip_state != ip_unknown)
	    {
	    }
	else if((fd_ip = open("/dev/ip", O_RDONLY)) < 0)
	    {
	    ip_state = ip_down;
	    }
	else if(ioctl(fd_ip, I_STR, (caddr_t) &ioc) < 0)
	    {
	    ip_state = ip_down;
	    close(fd_ip);
	    }
	else
	    {
	    ip_state = (ioc.ic_len/sizeof(struct ifreq) != 0)? ip_up: ip_down;
	    close(fd_ip);
	    }
	
	return(ip_state == ip_up);
	}

static int
    getanswer(querybuf *answer, int anslen, void *list, char *name, int minfoOk, int cnameOk)
	{
	register HEADER
	    *hp;
	register u_char
	    *cp;
	register int
	    n;
	int
	    result = 0;

	u_char
	    *eom;
	char
	    nameBuff[256],
	    /*
	    *chartype,
	    */
	    *bp;

	int
	    type,
	    buflen,
	    ancount,
	    qdcount;

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
	    if
		(
		    (
		    n = dn_expand
			(
			(unsigned char *)answer->buf,
			eom,
			cp,
			(unsigned char *)bp,
			buflen
			)
		    ) < 0
		)
		{
		break;
		}

	    cp += n;
	    type = _getshort(cp);
	    cp += sizeof(u_short);
	    cp += sizeof(u_short) + sizeof(u_long);
	    n = _getshort(cp);
	    cp += sizeof(u_short);
	    /*
	    switch(type)
		{
		default:
		    {
		    chartype="UNKOWN";
		    break;
		    }

		case T_MG:
		    {
		    chartype="T_MG";
		    break;
		    }

		case T_MR:
		    {
		    chartype="T_MR";
		    break;
		    }

		case	T_MB:
		    {
		    chartype="T_MB";
		    break;
		    }

		case T_PTR:
		    {
		    chartype="T_PTR";
		    break;
		    }

		case T_CNAME:
		    {
		    chartype="T_CNAME";
		    break;
		    }

		case	T_MINFO:
		    {
		    chartype="T_MINFO";
		    break;
		    }
		}
	    */

	    switch(type)
		{
		default:
		    {
		    cp += n;
		    break;
		    }
		
		case	T_MB:
		    {
		    (void) strncpy(bp, name, buflen);
		    (void) strncat(bp, "@", buflen);
		    bp += strlen(bp);
		    buflen -= strlen(bp);
		    if((dn_expand((unsigned char *)answer->buf, eom, cp, (unsigned char *)bp, buflen)) < 0) break;
		    if
			(
			linkAddSortedUnique
			    (
			    list,
			    linkNew(strdup(nameBuff)),
			    stricmp,
			    freeElement
			    )
			)
			{
			}
		    else
			{
			result++;
			}

		    cp += n;
		    break;
		    }
		
		case	T_CNAME:
		    {
		    if(!cnameOk)
			{
			cp += n;
			}
		    else if((dn_expand((unsigned char *)answer->buf, eom, cp, (unsigned char *)bp, buflen)) < 0)
			{
			}
		    else
			{
			*strchr(nameBuff, '.') = '@';
			CnameOk++;
			result += _maildir_alias(list, nameBuff);
			CnameOk--;
			cp += n;
			}

		    break;
		    }

		case	T_MG:
		case	T_MR:
		case	T_PTR:
		    {
		    if((dn_expand((unsigned char *)answer->buf, eom, cp, (unsigned char *)bp, buflen)) < 0) break;
		    *strchr(nameBuff, '.') = '@';

		    if
			(
			linkAddSortedUnique
			    (
			    list,
			    linkNew(strdup(nameBuff)),
			    stricmp,
			    freeElement
			    )
			)
			{
			}
		    else
			{
			result++;
			}

		    cp += n;
		    break;
		    }
		
		case	T_MINFO:
		    {
		    if(!minfoOk)
			{
			}
		    else if
			(
			    (
			    dn_expand((unsigned char *)answer->buf, eom, cp, (unsigned char *)bp, buflen)
			    ) < 0
			)
			{
			}
		    else
			{
			*strchr(nameBuff, '.') = '@';
			if
			    (
			    linkAddSortedUnique
				(
				list,
				linkNew(strdup(nameBuff)),
				stricmp,
				freeElement
				)
			    )
			    {
			    }
			else
			    {
			    result++;
			    }
			}

		    cp += n;
		    break;
		    }
		}
	    }

	return(result);
	}

int
    _maildir_alias(void *list, char *address)
	{
	char
	    *name,
	    *fullName,
	    *substring,
	    *endstring,
	    *atSign;

	int
	    /*
	    error,
	    */
	    done,
	    result = 0,
	    oldAlarm,
	    (*oldSig)(),
	    n;

	querybuf
	    buf;

	oldAlarm = alarm(0);
	oldSig = sigset(SIGALRM, doAlarm);

	if(!ipUp())
	    {
	    }
	else if(list == NULL)
	    {
	    }
	else if(address == NULL)
	    {
	    }
	else
	    {
	    if(!(_res.options & RES_INIT))
		{
		res_init();
		}
	    
	    _res.options |= RES_DNSRCH;
	    /*_res.options |= RES_USEVC;*/
	    /*_res.options |= RES_STAYOPEN;*/
	    /*_res.options |= RES_DEBUG;*/

	    sethostent(1);
	    fullName = strdup(address);
	    atSign = strchr(fullName, '@');
	    if(atSign != NULL)
		{
		*atSign = '.';
		}

	    do
		{
		SearchCount++;
		AlarmHit = 0;
		alarm(60);
		if
		    (
			(
			n = res_search
			    (
			    fullName,
			    C_IN,
			    T_MAILB,
			    buf.buf,
			    sizeof(buf)
			    )
			) >= 0
		    )
		    {
		    name = strtok(fullName, ".");
		    result += getanswer(&buf, n, list, name, 0, CnameOk);
		    done = 1;
		    }
		else if((substring = strstr(fullName, "-request")) != NULL)
		    {
		    endstring = strdup(substring + strlen("-request"));
		    (void) strcpy(substring, endstring);
		    free(endstring);
		    AlarmHit = 0;
		    alarm(60);
		    if
			(
			    (
			    n = res_search
				(
				fullName,
				C_IN,
				T_MINFO,
				buf.buf,
				sizeof(buf)
				)
			    ) >= 0
			)
			{
			name = strtok(fullName, ".");
			result += getanswer(&buf, n, list, name, 1, CnameOk);
			}

		    done = 1;
		    }
/*
		else if((error = h_errno) == TRY_AGAIN)
		    {
		    done = 0;
		    perror("dns search");
		    printf("SearchCount = %d herrno = %d.\n", SearchCount, error);
		    sleep(10);
		    }
*/
		else
		    {
		    done = 1;
		    }
		
		alarm(0);
		} while(!done && !AlarmHit);

	    free(fullName);
	    }

	alarm(oldAlarm);
	sigset(SIGALRM, oldSig);
	return(result);
	}

char
    *_maildir_revAlias(char *address)
	{
	int
	    oldAlarm,
	    (*oldSig)(),
	    *curLink_p,
	    *list;

	char
	    *fullName,
	    *name,
	    *atSign,
	    *result = NULL;

	int
	    n;

	querybuf
	    buf;

	oldAlarm = alarm(0);
	oldSig = sigset(SIGALRM, doAlarm);
	if(!ipUp())
	    {
	    }
	else if(address == NULL)
	    {
	    }
	else
	    {
	    fullName = strdup(address);
	    atSign = strchr(fullName, '@');
	    if(atSign != NULL)
		{
		*atSign = '.';
		}

	    if(!(_res.options & RES_INIT))
		{
		res_init();
		}
	    
	    _res.options |= RES_DNSRCH;
	    _res.options |= RES_USEVC;
	    _res.options |= RES_STAYOPEN;
	    /*_res.options |= RES_DEBUG;*/

	    AlarmHit = 0;
	    alarm(60);
	    if
		(
		    (
		    n = res_search
			(
			fullName,
			C_IN,
			T_PTR,
			buf.buf,
			sizeof(buf)
			)
		    ) < 0
		)
		{
		}
	    else if((list = linkNew(NULL)) == NULL)
		{
		}
	    else
		{
		name = strtok(fullName, ".");
		(void) getanswer(&buf, n, list, name, 0, 0);
		result = (char *)linkOwner(curLink_p = linkNext(list));
		linkFree(curLink_p);
		while((curLink_p = linkNext(list)) != list)
		    {
		    free(linkOwner(curLink_p));
		    linkFree(curLink_p);
		    }

		linkFree(list);
		}

	    free(fullName);
	    }

	alarm(oldAlarm);
	sigset(SIGALRM, oldSig);
	return(result);
	}
