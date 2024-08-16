/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/gtphostent.c	1.7"
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
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/syslog.h>
#include <sys/conf.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ppp.h>

#ifdef SYSV
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)
#endif /* SYSV */

/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14

static FILE *ppphostf = NULL;
static FILE *authf = NULL;
static char line[BUFSIZ+1];
static char line1[BUFSIZ+1];
static char ppphostaddr[MAXADDRSIZE];
static struct ppphostent ppphost;
static struct hostent hostentry;
static unsigned long saddr=0;
static char *addr=(char *)&saddr;

/*
 * The following is shared with getppphostnamadr.c
 */
char	*_ppphost_file = "/etc/inet/ppphosts";
int	_ppphost_stayopen;
char	*_auth_file = "/etc/inet/pppauth";

char *any();
void papgetpwd();

char *pppfgets();

setppphostent(f)
	int f;
{
	if (ppphostf != NULL)
		rewind(ppphostf);
	_ppphost_stayopen |= f;
}

endppphostent()
{
	if (ppphostf) {
		fclose(ppphostf);
		ppphostf = NULL;
	}
	_ppphost_stayopen = 0;
}

struct ppphostent *
getppphostent()
{
	char *p;
	char *ps,*ptr,*ptr1;
	register char *cp;
	unsigned long l;
	struct hostent *hostptr;
	int	len;
	char	*res;

	if (ppphostf == NULL && (ppphostf = fopen(_ppphost_file, "r" )) == NULL)
		return (NULL);
again:
	/*
	 * Allow room for trailing space (see below).
	 */
	if ((p = pppfgets(line, BUFSIZ - 1, ppphostf)) == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	p = strtok(p, "#\n");
	if (p == NULL)
		goto again;
	/*
	 * Put a space at the end of the line
	 * (for the case where pap is the last option on the line).
	 */
	strcat(p, " ");
	ppphost.ppp_h_ent = (struct hostent *)0;
	ps = strtok(p, " \t");
	l = inet_addr(ps);

	if(*ps == '*'){
		if(*(ps+1)){
			hostentry.h_name = ps+1;
			hostentry.h_aliases = NULL;
			hostentry.h_addrtype = -AF_INET;
			saddr = 0;
                        hostentry.h_addr_list = &addr;
			ppphost.ppp_h_ent = &hostentry;
		}
	}
	else {
	/* THIS STUFF IS INTERNET SPECIFIC */
         if (l != INADDR_NONE) {
                hostentry.h_name = 0;
                hostentry.h_aliases = 0;
                hostentry.h_addrtype = AF_INET;
                saddr = l;
                hostentry.h_length = sizeof(saddr);
                hostentry.h_addr_list = &addr;
                ppphost.ppp_h_ent = &hostentry;
         } else {
                ppphost.ppp_h_ent = gethostbyname(ps);
                if (ppphost.ppp_h_ent == 0) {
                        ppp_syslog(LOG_WARNING, "getppphostent: can't get IP address for %s", ps);
                        return &ppphost;
                }
         }
	}

	ppphost.tty_line = strtok(NULL, " \t");
	ppphost.uucp_system = strtok(NULL, " \t\n");
	for(ptr=ppphost.uucp_system;*ptr!='\0';ptr++);
	ptr++;

	if((ptr1=strstr(ptr,"remote="))!=NULL && ppphost.ppp_h_ent==&hostentry){
                l = inet_addr(ptr1+7);
		if (l != INADDR_NONE) {
			saddr = l;
		} else {	
                        hostptr = gethostbyname(ptr1+7);
                        hostptr->h_name = hostentry.h_name;
                        hostptr->h_aliases = hostentry.h_aliases;
                        hostptr->h_addrtype = hostentry.h_addrtype;
                        ppphost.ppp_h_ent = hostptr;
                }
        }

	if((ptr1=strstr(ptr,"idle="))==NULL)
		ppphost.ppp_cnf.inactv_tmout = 0;
	else
		ppphost.ppp_cnf.inactv_tmout = atoi(ptr1+5);

	if((ptr1=strstr(ptr," tmout="))==NULL
			&& (ptr1=strstr(ptr,"	tmout="))==NULL)
		ppphost.ppp_cnf.restart_tm = 0;
	else
		ppphost.ppp_cnf.restart_tm = atoi(ptr1+6);
	
	if((ptr1=strstr(ptr,"conf="))==NULL)
		ppphost.ppp_cnf.max_cnf = 0;
	else
		ppphost.ppp_cnf.max_cnf = atoi(ptr1+5);

	if((ptr1=strstr(ptr,"term="))==NULL)
		ppphost.ppp_cnf.max_trm = 0;
	else
		ppphost.ppp_cnf.max_trm = atoi(ptr1+5);
	
	if((ptr1=strstr(ptr,"nak="))==NULL)
		ppphost.ppp_cnf.max_failure = 0;
	else
		ppphost.ppp_cnf.max_failure = atoi(ptr1+4);
	
	if((ptr1=strstr(ptr,"mru="))==NULL)
		ppphost.ppp_cnf.mru = 0;
	else
		ppphost.ppp_cnf.mru = atoi(ptr1+4);
	
	if((ptr1=strstr(ptr,"accm="))==NULL)
		ppphost.ppp_cnf.accm = 0;
	else
		ppphost.ppp_cnf.accm = strtoul((ptr1+5),(char **)NULL,16);
	
	if(strstr(ptr,"pap ")==NULL && strstr(ptr, "pap	")==NULL)
		ppphost.ppp_cnf.pap = 0;
	else
		ppphost.ppp_cnf.pap = 1;
	
	if(strstr(ptr,"nomgc")==NULL)
		ppphost.ppp_cnf.mgc = 1;
	else
		ppphost.ppp_cnf.mgc = 0;
	
	if(strstr(ptr,"protcomp")==NULL)
		ppphost.ppp_cnf.protcomp = 0;
	else
		ppphost.ppp_cnf.protcomp = 1;

	if(strstr(ptr,"accomp")==NULL)
		ppphost.ppp_cnf.accomp = 0;
	else
		ppphost.ppp_cnf.accomp = 1;

	if(strstr(ptr,"ipaddr")==NULL)
		ppphost.ppp_cnf.ipaddress = 0;
	else
		ppphost.ppp_cnf.ipaddress = 1;

	if(strstr(ptr,"rfc1172addr")==NULL)
		ppphost.ppp_cnf.newaddress = 1;
	else
		ppphost.ppp_cnf.newaddress = 0;

	if(strstr(ptr,"VJ")==NULL)
		ppphost.ppp_cnf.vjcomp = 0;
	else
		ppphost.ppp_cnf.vjcomp = 1;

	if(strstr(ptr,"old")==NULL)
		ppphost.ppp_cnf.old_ppp = 0;
	else
		ppphost.ppp_cnf.old_ppp = 1;

	if((ptr1=strstr(ptr,"paptmout="))==NULL)
		ppphost.ppp_cnf.pap_tmout = 0;
	else
		ppphost.ppp_cnf.pap_tmout = atoi(ptr1+9);

	if((ptr1=strstr(ptr,"debug="))==NULL)
		ppphost.ppp_cnf.debug = 0;
	else {
		/* let strtol() figure out which base the user used */
		ppphost.ppp_cnf.debug = (int)strtol(ptr1 + 6, &res, 0);
		if (res == (ptr1 + 6)) {
			ppp_syslog(LOG_WARNING,
				"getppphostent: invalid debug level");
			ppphost.ppp_cnf.debug = 0;
		}
		if (ppphost.ppp_cnf.debug > 15 || ppphost.ppp_cnf.debug < 0) {
			ppp_syslog(LOG_WARNING,
				"getppphostent: debug level:%d out of range",
				ppphost.ppp_cnf.debug);
			ppphost.ppp_cnf.debug = 0;
		}
	}
			
	
	ppphost.ppp_cnf.pid_pwd.PID[0] = '*'; 
	papgetpwd(ppphost.ppp_cnf.pid_pwd.PID,ppphost.ppp_cnf.pid_pwd.PWD); 

	return (&ppphost);
}

setppphostfile(file)
	char *file;
{
	_ppphost_file = file;
}


void
papgetpwd(pid,pwd)
char pid[];
char pwd[];
{
	char *p,*ps;

	pwd[0]='\0';

	if((authf= fopen(_auth_file,"r"))==NULL) 
		return;

again: if((p=fgets(line1,BUFSIZ,authf)) == NULL){
		fclose(authf); 
		return ;
	}
	if(*p == '#')
		goto again;
	p =strtok(p,"#\n");
	if( p== NULL)
		goto again;

	ps = strtok(p," \t");
	if(pid[0] == '*'){
		if(*ps == '*'){
			strcpy(pid,ps+1);
			ps = strtok(NULL," \t");
			strcpy(pwd,ps);
			fclose(authf); 
		} else
			goto again;
	}
	else {
		if(strcmp(ps,pid)==0){
			ps = strtok(NULL," \t");
			strcpy(pwd,ps);
			fclose(authf); 
		} else
			goto again;
	}
			
}

char *
pppfgets(line,n,fd)
char *line;
int n;
FILE *fd;
{
	char tmp[BUFSIZ];
	int i,len=0;

	while(len<n){
		if(fgets(tmp,BUFSIZ,fd)==NULL)
			return(NULL);
		for(i=0;tmp[i]!='\0';i++);
		bcopy(tmp,&line[len],i+1);
		len += i-1;
		if(tmp[i-2]!='\\')
			return(line);
		else
			line[len-1]=' ';
	}
	return(NULL);
}		
