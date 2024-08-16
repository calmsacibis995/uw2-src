/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/res_debug.c	1.2"
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

/*
** Modified for and distributed with 'dig' version 2.0 from 
** University of Southern California Information Sciences Institute
** (USC-ISI). 9/1/90
*/


#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_debug.c	5.22 (Berkeley) 3/7/88";
#endif /* LIBC_SCCS and not lint */

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

#ifndef RES_DEBUG
#define RES_DEBUG2      0x80000000
#endif

#include "hfiles.h"

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include NAMESERH
#ifndef T_TXT
#define T_TXT 16
#endif /* T_TXT */
#include RESOLVH
#include "pflag.h"
#include NETDBH


extern char *p_cdname(), *p_rr(), *p_type(), *p_class();
extern char *inet_ntoa();

char *_res_opcodes[] = {
	"QUERY",
	"IQUERY",
	"CQUERYM",
	"CQUERYU",
	"4",
	"5",
	"6",
	"7",
	"8",
	"UPDATEA",
	"UPDATED",

	"UPDATEDA",
	"UPDATEM",
	"UPDATEMA",
	"ZONEINIT",
	"ZONEREF",
};

char *_res_resultcodes[] = {
	"NOERROR",
	"FORMERR",
	"SERVFAIL",
	"NXDOMAIN",
	"NOTIMP",
	"REFUSED",
	"6",
	"7",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"NOCHANGE",
};


static char wksbuf[16];

char * dewks(wks)
   int wks;
{
    switch (wks) {
    case 5: return("rje");
    case 7: return("echo");
    case 9: return("discard");
    case 11: return("systat");
    case 13: return("daytime");
    case 15: return("netstat");
    case 17: return("qotd");
    case 19: return("chargen");
    case 20: return("ftp-data");
    case 21: return("ftp");
    case 23: return("telnet");
    case 25: return("smtp");
    case 37: return("time");
    case 39: return("rlp");
    case 42: return("name");
    case 43: return("whois");
    case 53: return("domain");
    case 57: return("apts");
    case 59: return("apfs");
    case 67: return("bootps");
    case 68: return("bootpc");
    case 69: return("tftp");
    case 77: return("rje");
    case 79: return("finger");
    case 87: return("link");
    case 95: return("supdup");
    case 100: return("newacct");
    case 101: return("hostnames");
    case 102: return("iso-tsap");
    case 103: return("x400");
    case 104: return("x400-snd");
    case 105: return("csnet-ns");
    case 109: return("pop-2");
    case 111: return("sunrpc");
    case 113: return("auth");
    case 115: return("sftp");
    case 117: return("uucp-path");
    case 119: return("nntp");
    case 121: return("erpc");
    case 123: return("ntp");
    case 133: return("statsrv");
    case 136: return("profile");
    case 144: return("NeWS");
    case 161: return("snmp");
    case 162: return("snmp-trap");
    case 170: return("print-srv");
    default: (void) sprintf(wksbuf, "%d", wks);
             return(wksbuf);
    }
  }

char * deproto(protonum)
     int protonum;
{

switch (protonum) {

 case 1: return("icmp");
 case 2: return("igmp");
 case 3: return("ggp");
 case 5: return("st");
 case 6: return("tcp");
 case 7: return("ucl");
 case 8: return("egp");
 case 9: return("igp");
 case 11: return("nvp-II");
 case 12: return("pup");
 case 16: return("chaos");
 case 17: return("udp");
 default: (void) sprintf(wksbuf, "%d", protonum);
   return(wksbuf);
}
}

p_query(msg)
	char *msg;
{
#ifdef DEBUG
	fp_query(msg,stdout);
#endif
}

char *
do_rrset(msg,cp,cnt,pflag,file,hs)
     int cnt, pflag;
     char *cp,*msg, *hs;
     FILE *file;
{
  int n;
  char *pp;
  char *t1, *t2, *list[100],**tt;
  int sflag;
	/*
	 * Print  answer records
	 */
  sflag = (pfcode & pflag);
  if (n = ntohs(cnt)) {
    *list=NULL;
    if ((sflag) && (pfcode & PRF_HEAD1))
      fprintf(file,hs);
    while (--n >= 0) {
      pp = (char *) malloc(512);
      *pp=0;
      cp = p_rr(cp, msg, pp);
      if ((cp-msg) > PACKETSZ)
	return (NULL);

      if (sflag) {
	if (pfcode & PRF_SORT) {
	  for (tt=list, t1=pp, t2=NULL;
	       ((*tt != NULL) && (strcmp(*tt,t1) < 1));) {
	    tt++;
	  }
	  while (t1 != NULL) {
	    t2 = *tt;
	    *tt++ = t1;
	    t1 = t2;
	  }
	  *tt = t1;
	}
	else {
	  fprintf(file,"%s",pp);
	  free(pp);
	}
      } else
	free(pp);
    }

    if (pfcode & pflag) {
      if (pfcode & PRF_SORT) {
	tt=list;
	while (*tt != NULL) {
	  fprintf(file,"%s",*tt);
	  free(*tt++);
	}
      }
      if (pfcode & PRF_HEADX)
	fprintf(file,"\n");
    }
  }
  return(cp);
}


/*
 * Print the contents of a query.
 * This is intended to be primarily a debugging routine.
 */
fp_query(msg,file)
	char *msg;
	FILE *file;
{
	register char *cp;
	register HEADER *hp;
	register int n;
	char zfile[256], tfile[100];

	/*
	 * Print header fields.
	 */
	hp = (HEADER *)msg;
	cp = msg + sizeof(HEADER);
	if ((pfcode & PRF_HEADX) || hp->rcode) {
	  fprintf(file,";; ->>HEADER<<- ");
	  fprintf(file,"opcode: %s ", _res_opcodes[hp->opcode]);
	  fprintf(file,", status: %s", _res_resultcodes[hp->rcode]);
/*	  if (pfcode & PRF_TTLID)  XXXX */
	    fprintf(file,", id: %d", ntohs(hp->id));
	  putc('\n',file);
	}
	if (pfcode & PRF_HEAD2) {
	  fprintf(file,";; flags:");
	  if (hp->qr)
	    fprintf(file," qr");
	  if (hp->aa)
	    fprintf(file," aa");
	  if (hp->tc)
	    fprintf(file," tc");
	  if (hp->rd)
	    fprintf(file," rd");
	  if (hp->ra)
	    fprintf(file," ra");
	  if (hp->pr)
	    fprintf(file," pr");
	  if (_res.options & RES_DEBUG2)
	    fprintf(file," res_opts: %x", _res.options);
	  putc(' ',file);
	}
	if (pfcode & PRF_HEAD1) {
	  fprintf(file,"; Ques: %d, ", ntohs(hp->qdcount));
	  fprintf(file,"Ans: %d, ", ntohs(hp->ancount));
	  fprintf(file,"Auth: %d, ", ntohs(hp->nscount));
	  fprintf(file,"Addit: %d", ntohs(hp->arcount));
	}
	if (pfcode & (PRF_HEADX | PRF_HEAD2 | PRF_HEAD1))
	  putc('\n',file);
	/*
	 * Print question records.
	 */
	if (n = ntohs(hp->qdcount)) {
	  if (pfcode & PRF_QUES)
	    fprintf(file,";; QUESTIONS: \n");
	  while (--n >= 0) {
	    sprintf(zfile,";;\t");
	    cp = p_cdname(cp, msg, zfile);
	    if (cp == NULL)
	      return;
	    sprintf(tfile,", type = %s", p_type(_getshort(cp)));
	    strcat(zfile,tfile);
	    cp += sizeof(u_short);
	    sprintf(tfile,", class = %s\n", p_class(_getshort(cp)));
	    strcat(zfile,tfile);
	    cp += sizeof(u_short);
	    if (pfcode & PRF_QUES)
	      fprintf(file,"%s",zfile);
	  }
	  if (pfcode & PRF_QUES)
	      fprintf(file,"\n");
	}

	/*
	 * Print authoritative answer records
	 */

if ((cp = do_rrset(msg,cp,hp->ancount,PRF_ANS,file,";; ANSWERS:\n")) == NULL)
  return;

	/*
	 * print name server records
	 */

if ((cp = do_rrset(msg,cp,hp->nscount,PRF_AUTH,file,
		   ";; AUTHORITY RECORDS:\n")) == NULL)
  return;

	/*
	 * print additional records
	 */

if ((cp = do_rrset(msg,cp,hp->arcount,PRF_ADD,file,
		   ";; ADDITIONAL RECORDS:\n")) == NULL)
  return;

} /* end function */

      

char *
p_cdname(cp, msg, p)
	char *cp, *msg;
        char *p;
{
#ifdef DEBUG
	char name[MAXDNAME];
	int n;

    if ((n = dn_expand(msg, msg + 512, cp, name, sizeof(name))) < 0)
		return (NULL);
	if (name[0] == '\0') {
		name[0] = '.';
		name[1] = '\0';
	}
	strcat(p,name);
	return (cp + n);
#endif
}



/********************************************************
 * Print resource record fields in human readable form.
 ********************************************************/
char *
p_rr(cp, msg,p)
	char *cp, *msg, *p;
{
#ifdef DEBUG
	int type, class, dlen, n, c, xx,lcnt;
	struct in_addr inaddr;
	char *cp1;
	unsigned long tmpttl;
        struct servent *service;
	struct protoent *protocol;
	char file[130];

	if ((cp = p_cdname(cp, msg, p)) == NULL)
		return (NULL);			/* compression error */

	if (*(p+strlen(p)-1) != '.')
	  strcat(p,".");

	type = _getshort(cp);
	cp += sizeof(u_short);
	class = _getshort(cp);
	cp += sizeof(u_short);
	tmpttl = _getlong(cp);
	cp += sizeof(u_long);

	if (pfcode & PRF_TTLID) {
	  sprintf(file,"\t%d",tmpttl);
	  strcat(p,file);
	}

	if (pfcode & PRF_CLASS)
	  sprintf(file,"\t%s\t%s",p_class(class), p_type(type));
	else
	  sprintf(file,"\t%s", p_type(type));
	strcat(p, file);

	dlen = _getshort(cp);
	cp += sizeof(u_short);
	cp1 = cp;
	/*
	 * Print type specific data, if appropriate
	 */
	switch (type) {
	case T_A:
		switch (class) {
		case C_IN:
			bcopy(cp, (char *)&inaddr, sizeof(inaddr));
			if (dlen == 4) {
			        sprintf(file,"\t%s",inet_ntoa(inaddr));
				strcat(p,file);
				cp += dlen;
			} else if (dlen == 7) {
				strcat(p,inet_ntoa(inaddr));
				sprintf(file,";; proto: %d", cp[4]);
				strcat(p,file);
				sprintf(file,", port: %d",
					(cp[5] << 8) + cp[6]);
				strcat(p,file);
				cp += dlen;
			}
			break;
		default:
			cp += dlen;
		}
		break;
	case T_CNAME:
	case T_MB:
#ifdef OLDRR
	case T_MD:
	case T_MF:
#endif /* OLDRR */
	case T_MG:
	case T_MR:
	case T_NS:
	case T_PTR:
		strcat(p,"\t");
		cp = p_cdname(cp, msg, p);
		strcat(p,".");
		break;

	case T_HINFO:
		if (n = *cp++) {
			sprintf(file,"\t%.*s ", n, cp);
			strcat(p,file);
			cp += n;
		}
		if (n = *cp++) {
			sprintf(file,"%.*s", n, cp);
			strcat(p,file);
			cp += n;
		}
		break;

	case T_UINFO:
		sprintf(file,"\t%s", cp);
		strcat(p,file);
		cp += dlen;
		break;

	case T_TXT:
		xx=0;
		while (xx++<dlen) {
		  if (n = *cp++) {
		    sprintf(file,"\t%.*s",n,cp);
		    strcat(p,file);
		    cp += (n-1);
		    xx += n;
		    if (*cp++ != '\n')
		      strcat(p,"\n");
		  }
		}
		strcat(p,"\n");
		break;

	case T_SOA:
		strcat(p,"\t");
		cp = p_cdname(cp, msg, p);
		strcat(p,".  ");
		cp = p_cdname(cp, msg, p);
		sprintf(file,". (\n\t\t\t%ld\t;serial\n", _getlong(cp));
		strcat(p,file);
		cp += sizeof(u_long);
		sprintf(file,"\t\t\t%ld\t;refresh\n", _getlong(cp));
		strcat(p,file);
		cp += sizeof(u_long);
		sprintf(file,"\t\t\t%ld\t;retry\n", _getlong(cp));
		strcat(p,file);
		cp += sizeof(u_long);
		sprintf(file,"\t\t\t%ld\t;expire\n", _getlong(cp));
		strcat(p,file);
		cp += sizeof(u_long);
		sprintf(file,"\t\t\t%ld )\t;minim\n", _getlong(cp));
		strcat(p,file);
		cp += sizeof(u_long);
		break;

	case T_MX:
		sprintf(file,"\t%ld ",_getshort(cp));
		strcat(p,file);
		cp += sizeof(u_short);
		cp = p_cdname(cp, msg, p);
		strcat(p,".");
		break;

	case T_MINFO:
		strcat(p,"\t");
		cp = p_cdname(cp, msg, p);
		strcat(p,". ");
		cp = p_cdname(cp, msg, p);
		strcat(p,". ");
		break;


	case T_UID:
	case T_GID:
		if (dlen == 4) {
			sprintf(file,"\t%ld", _getlong(cp));
			strcat(p,file);
			cp += sizeof(int);
		}
		break;

	case T_WKS:
		if (dlen < sizeof(u_long) + 1)
			break;
		bcopy(cp, (char *)&inaddr, sizeof(inaddr));
		cp += sizeof(u_long);

/* Although more portable, quite slow ...

		if ((protocol = getprotobynumber((int) *cp)) != NULL) {
		  sprintf(file,"\t %s %s (\n", inet_ntoa(inaddr),
			  protocol->p_name);
		} else {
		  sprintf(file,"\t%s %d (\n", inet_ntoa(inaddr), *cp);
		}
*/
		sprintf(file, "\t %s %s (\n", inet_ntoa(inaddr),
			deproto((int) *cp));

		cp++;
		strcat(p,file);
		n = 0;
		strcat(p,"\t\t\t");
                lcnt = 5;
		while (cp < cp1 + dlen) {
			c = *cp++;
			do {
			  if (c & 0200) {

/* Although more portable, quite slow ... even using set/endservent()

			     if ((service = getservbyport(n,NULL)) != NULL) {
			       sprintf(file," %s", service->s_name);
			     } else {
			       sprintf(file, " %d", n);
			     }
*/
			    sprintf(file," %s", dewks(n));
			    strcat(p,file);
			     if (--lcnt == 0) {
			       lcnt=5;
			       strcat(p,"\n\t\t\t");
			     }
			   }
			  c <<= 1;
			} while (++n & 07);
		      }
		strcat(p," )");
		break;

#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		{
			int NumBytes = 8;
			char *DataPtr;
			int i;

			if (dlen < NumBytes) NumBytes = dlen;
			sprintf(file, "\tFirst %d bytes of hex data:",
				NumBytes);
			strcat(p,file);
		for (i = 0, DataPtr = cp; i < NumBytes; i++, DataPtr++) {
		  sprintf(file, " %x", *DataPtr);
		  strcat(p,file);
		}
			strcat(p,"\n");
			cp += dlen;
		}
		break;
#endif /* ALLOW_T_UNSPEC */

	default:
		strcat(p,"\t???\n");
		cp += dlen;
	}
/*
	if (pfcode & PRF_TTLID) {
	  sprintf(file,"\t; %d",tmpttl);
	  strcat(p,file);
	}
*/
	strcat(p,"\n");

	if (cp != cp1 + dlen)
	   fprintf(stdout,";; packet size error (%#x != %#x)\n", cp, cp1+dlen);
	return (cp);
#endif
}

static	char nbuf[20];

/*
 * Return a string for the type
 */
char *
p_type(type)
	int type;
{
	switch (type) {
	case T_A:
		return("A");
	case T_NS:		/* authoritative server */
		return("NS");
#ifdef OLDRR
	case T_MD:		/* mail destination */
		return("MD");
	case T_MF:		/* mail forwarder */
		return("MF");
#endif /* OLDRR */
	case T_CNAME:		/* connonical name */
		return("CNAME");
	case T_SOA:		/* start of authority zone */
		return("SOA");
	case T_MB:		/* mailbox domain name */
		return("MB");
	case T_MG:		/* mail group member */
		return("MG");
	case T_MX:		/* mail routing info */
		return("MX");
	case T_MR:		/* mail rename name */
		return("MR");
	case T_NULL:		/* null resource record */
		return("NULL");
	case T_WKS:		/* well known service */
		return("WKS");
	case T_PTR:		/* domain name pointer */
		return("PTR");
	case T_HINFO:		/* host information */
		return("HINFO");
	case T_MINFO:		/* mailbox information */
		return("MINFO");
	case T_AXFR:		/* zone transfer */
		return("AXFR");
	case T_MAILB:		/* mail box */
		return("MAILB");
	case T_MAILA:		/* mail address */
		return("MAILA");
	case T_ANY:		/* matches any type */
		return("ANY");
        case T_TXT:
		return("TXT");
	case T_UINFO:
		return("UINFO");
	case T_UID:
		return("UID");
	case T_GID:
		return("GID");
#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		return("UNSPEC");
#endif /* ALLOW_T_UNSPEC */
	default:
		(void)sprintf(nbuf, "%d", type);
		return(nbuf);
	}
}

/*
 * Return a mnemonic for class
 */
char *
p_class(class)
	int class;
{

	switch (class) {
	case C_IN:		/* internet class */
		return("IN");
	case C_ANY:		/* matches any class */
		return("ANY");
	default:
		(void)sprintf(nbuf, "%d", class);
		return(nbuf);
	}
}
