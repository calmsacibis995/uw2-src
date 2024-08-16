/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/dig.c	1.2"
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
 /*******************************************************************
 **      DiG -- Domain Information Groper                          **
 **                                                                **
 **        dig.c - Version 2.0 (9/1/90)                            **
 **                                                                **
 **        Developed by: Steve Hotz & Paul Mockapetris             **
 **        USC Information Sciences Institute (USC-ISI)            **
 **        Marina del Rey, California                              **
 **        1989                                                    **
 **                                                                **
 **        dig.c -                                                 **
 **           Version 2.0 (9/1/90)                                 **
 **               o renamed difftime() difftv() to avoid           **
 **                 clash with ANSI C                              **
 **               o fixed incorrect # args to strcmp,gettimeofday  **
 **               o incorrect length specified to strncmp          **
 **               o fixed broken -sticky -envsa -envset functions  **
 **               o print options/flags redefined & modified       **
 **                                                                **
 **           Version 2.0.beta (5/9/90)                            **
 **               o output format - helpful to `doc`               **
 **               o minor cleanup                                  **
 **               o release to beta testers                        **
 **                                                                **
 **           Version 1.1.beta (10/26/89)                          **
 **               o hanging zone transer (when REFUSED) fixed      **
 **               o trailing dot added to domain names in RDATA    **
 **               o ISI internal                                   **
 **                                                                **
 **           Version 1.0.tmp  (8/27/89)                           **
 **               o Error in prnttime() fixed                      **
 **               o no longer dumps core on large pkts             **
 **               o zone transfer (axfr) added                     **
 **               o -x added for inverse queries                   **
 **                               (i.e. "dig -x 128.9.0.32")       **
 **               o give address of default server                 **
 **               o accept broadcast to server @255.255.255.255    **
 **                                                                **
 **           Version 1.0  (3/27/89)                               **
 **               o original release                               **
 **                                                                **
 **     DiG is Public Domain, and may be used for any purpose as   **
 **     long as this notice is not removed.                        **
 ****                                                            ****
 ****   NOTE: Version 2.0.beta is not for public distribution    ****
 ****                                                            ****
 *******************************************************************/


#define VERSION 20
#define VSTRING "2.0"

#include "hfiles.h"

#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include NAMESERH
#include RESOLVH
#include NETDBH
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <ctype.h> 
#include <errno.h>
#include "pflag.h"
#ifndef T_TXT
#define T_TXT 16
#endif /*  T_TXT */
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include "res.h"
#include "qtime.h"

int pfcode = PRF_DEF;
int eecode = 0;
extern char *inet_ntoa();
extern struct state _res;

FILE  *qfp;
int sockFD;

#define MAXDATA		256   
#define SAVEENV "DiG.env"

char *defsrv, *srvmsg;
char defbuf[40] = "default -- ";
char srvbuf[60];

#define UC(b)   (((int)b)&0xff)

 /*
 ** Take arguments appearing in simple string (from file)
 ** place in char**.
 */
stackarg(y,l)
     char *l;
     char **y;
{
  int done=0;
  while (!done) {
    switch (*l) {
    case '\t':
    case ' ':  l++;    break;
#if !defined(M_UNIX)
    case NULL:
#else
    case ((char)NULL):
#endif
    case '\n': done++; *y = NULL; break;
    default:   *y++=l;  while (!isspace(*l)) l++;
      if (*l == '\n') done++; *l++ = '\0'; *y = NULL;
}}}


main(argc, argv)
     int argc;
     char **argv;
{
	struct hostent *hp;
	short port = htons(NAMESERVER_PORT);
	/*
	 * This mess is here because of alignment
	 * restrictions on certain machines.
	 */
	long al_pkt[PACKETSZ/sizeof(long)];
	long al_ans[PACKETSZ/sizeof(long)];
	char *packet = (char *)al_pkt;
	char *answer = (char *)al_ans;
	/* end mess */
	int n;
	char doping[90];
	char pingstr[50];
	char *afile;
	unsigned long tmpaddr;
        char revaddr[10][10];
        int addri, addrj;
	char *addrc;

	struct timeval exectime, tv1,tv2,tv3;
	char curhost[30];

	char *srv;
	int anyflag = 0;
	int sticky = 0;
	int tmp; 
	int qtype = 1, qclass = 1;
	int addrflag = 0;
	int zone = 0;
        int bytes_out, bytes_in;

	char cmd[256];
	char domain[128];
        char msg[120], *msgptr;
	char **vtmp;
	char *args[30];
	char **ax;
	char **ay;
	int once = 1, dofile=0; /* batch -vs- interactive control */
	char fileq[100];
	char *qptr;
	int  fp;
	int wait=0;
	int envset=0, envsave=0;
	int Xpfcode, Tpfcode, Toptions;
	char *Xres, *Tres;
	char *pp;


	res_init();
	gethostname(curhost,30);
	defsrv = strcat(defbuf, inet_ntoa(_res.nsaddr.sin_addr));
	_res.options |= RES_DEBUG;
	Xres = (char *) malloc(sizeof(_res)+1);
	Tres = (char *) malloc(sizeof(_res)+1);
	bcopy(&_res,Xres,sizeof(struct state));
	Xpfcode=pfcode;

 /*
 ** If LOCALDEF in environment, should point to file
 ** containing local favourite defaults.  Also look for file
 ** DiG.env (i.e. SAVEENV) in local directory.
 */

	if ((((afile= (char *) getenv("LOCALDEF")) != (char *) NULL) &&
	     ((fp=open(afile,O_RDONLY)) > 0)) ||
	    ((fp = open(SAVEENV,O_RDONLY)) > 0)) {
	  read(fp,Xres,sizeof(struct state));
	  read(fp,&Xpfcode,sizeof(int));
	  close(fp);
	  bcopy(Xres,&_res,sizeof(struct state));
	  pfcode = Xpfcode;
	}
 /*
 **   check for batch-mode DiG 
 */
	vtmp = argv; ax=args;
	while (*vtmp != NULL) {
	  if (strcmp(*vtmp,"-f") == 0) {
	    dofile++; once=0;
	    if ((qfp = fopen(*++vtmp,"r")) == NULL) {
	      fflush(stdout);
	      perror("file open");
	      fflush(stderr);
	      exit(10);
	    }
	  } else {
	    *ax++ = *vtmp;
	  }
	  vtmp++;
	}

	_res.id = 1;
	gettimeofday(&tv1,NULL);

 /*
 **  Main section: once if cmd-line query
 **                while !EOF if batch mode
 */
	*fileq='\0';
	while ((dofile && (fgets(fileq,100,qfp) !=NULL)) || 
	       ((!dofile) && (once--))) 
	  {
	    if ((*fileq=='\n') || (*fileq=='#') || (*fileq==';'))
	      continue; /* ignore blank lines & comments */

/*
 * "sticky" requests that before current parsing args
 * return to current "working" environment (X******)
 */
	    if (sticky) {
	      bcopy(Xres,&_res,sizeof(struct state));
	      pfcode = Xpfcode;
	    }

/* concat cmd-line and file args */
	    ay=ax;
	    qptr=fileq;
	    stackarg(ay,qptr);

	    /* defaults */
	    qtype=qclass=1;
	    zone = 0;
	    *pingstr=0;
	    srv=NULL;

	    sprintf(cmd,"\n; <<>> DiG %s <<>> ",VSTRING);
	    argv = args;
/*
 * More cmd-line options than anyone should ever have to
 * deal with ....
 */
	    while (*(++argv) != NULL) { 
	      strcat(cmd,*argv); strcat(cmd," ");
	      if (**argv == '@') {
		srv = (*argv+1);
		continue;
	      }
	      if (**argv == '%')
		continue;
	      if (**argv == '+') {
		SetOption(*argv+1);
		continue;
	      }
	 
	      if (strncmp(*argv,"-nost",5) == 0) {
		sticky=0;; continue;
	      } else if (strncmp(*argv,"-st",3) == 0) {
		sticky++; continue;
	      } else if (strncmp(*argv,"-envsa",6) == 0) {
		envsave++; continue;
	      } else if (strncmp(*argv,"-envse",6) == 0) {
		envset++; continue;
	      }

         if (**argv == '-') {
	   switch (argv[0][1]) { 
	   case 'T': wait = atoi(*++argv);
	     break;
	   case 'c': 
	     if ((tmp = atoi(*++argv)) || *argv[0]=='0') {
	       qclass = tmp;
	     } else if (tmp = StringToClass(*argv,0)) {
	       qclass = tmp;
	     } else {
	       printf("; invalid class specified\n");
	     }
	     break;
	   case 't': 
	     if ((tmp = atoi(*++argv)) || *argv[0]=='0') {
	       qtype = tmp;
	     } else if (tmp = StringToClass(*argv,0)) {
	       qtype = tmp;
	     } else {
	       printf("; invalid type specified\n");
	     }
	     break;
	   case 'x':
	     if (qtype == T_A) qtype = T_ANY;
	     addrc = *++argv;
	     addri=addrj=0;
	     while (*addrc) {
	       if (*addrc == '.') {
		 revaddr[addri][addrj++] = '.';
		 revaddr[addri][addrj] = (char) 0;
		 addri++; addrj=0;
	       } else {
		 revaddr[addri][addrj++] = *addrc;
	       }
	       addrc++;
	     }
	     if (*(addrc-1) == '.') {
	       addri--;
	     } else {
	       revaddr[addri][addrj++] = '.';
	       revaddr[addri][addrj] = (char) 0;
	     }
	     *domain = (char) 0;
	     for (addrj=addri; addrj>=0; addrj--)
	       strcat(domain,revaddr[addrj]);
	     strcat(domain,"in-addr.arpa.");
/* old code -- some bugs
	     tmpaddr = inet_addr(*++argv);
	     pp = (char *) &tmpaddr;
	     sprintf(domain,"%d.%d.%d.%d.in-addr.arpa",
		     UC(pp[3]), UC(pp[2]), UC(pp[1]), UC(pp[0]));
*/        
	     break;
	   case 'p': port = htons(atoi(*++argv)); break;
	   case 'P':
	     if (argv[0][2] != '\0')
	       strcpy(pingstr,&argv[0][2]);
	     else
	       strcpy(pingstr,"ping -q");
	     break;
	   } /* switch - */
	   continue;
	 } /* if '-'   */

	      if ((tmp = StringToType(*argv,-1)) != -1) { 
		if ((T_ANY == tmp) && anyflag++) {  
		  qclass = C_ANY; 	
		  continue; 
		}
		if (T_AXFR == tmp) {
		  pfcode = PRF_ZONE;
		  zone++;
		} else {
		  qtype = tmp; 
		}
	      }
	      else if ((tmp = StringToClass(*argv,-1)) != -1) { 
		qclass = tmp; 
	      }	 else {
		bzero(domain,128);
		sprintf(domain,"%s",*argv);
	      }
	    } /* while argv remains */
if (pfcode & 0x80000)
  printf("; pflag: %x res: %x\n", pfcode, _res.options);
	  
/*
 * Current env. (after this parse) is to become the
 * new "working environmnet. Used in conj. with sticky.
 */
	    if (envset) {
	      bcopy(&_res,Xres,sizeof(struct state));
	      Xpfcode=pfcode;
	      envset=0;
	    }

/*
 * Current env. (after this parse) is to become the
 * new default saved environmnet. Save in user specified
 * file if exists else is SAVEENV (== "DiG.env").
 */
	    if (envsave) {
	      if ((((afile= (char *) getenv("LOCALDEF")) != (char *) NULL) &&
		   ((fp=open(afile,O_WRONLY|O_CREAT|O_TRUNC,
			     S_IREAD|S_IWRITE)) > 0)) ||
		  ((fp = open(SAVEENV,O_WRONLY|O_CREAT|O_TRUNC,
			       S_IREAD|S_IWRITE)) > 0)) {
		write(fp,&_res,sizeof(struct state));
		write(fp,&pfcode,sizeof(int));
		close(fp);
	      }
	      envsave=0;
	    }

	    if (pfcode & PRF_CMD)
	      printf("%s\n",cmd);

      addrflag=anyflag=0;

/*
 * Find address of server to query. If not dot-notation, then
 * try to resolve domain-name (if so, save and turn off print 
 * options, this domain-query is not the one we want. Restore
 * user options when done.
 * Things get a bit wierd since we need to use resolver to be
 * able to "put the resolver to work".
 */

   srvbuf[0]=0;
   srvmsg=defsrv;
   if (srv != NULL) {
     tmpaddr = inet_addr(srv);
     if ((tmpaddr != (unsigned)-1) || 
	 (strncmp("255.255.255.255",srv,15) == 0)) {
       _res.nscount = 1;
       _res.nsaddr.sin_addr.s_addr = tmpaddr;
       srvmsg = strcat(srvbuf, srv);
     } else {
      Tpfcode=pfcode;
      pfcode=0;
      bcopy(&_res,Tres,sizeof(_res));
      Toptions = _res.options;
      _res.options = RES_DEFAULT;
      res_init();
      hp = gethostbyname(srv);
      pfcode=Tpfcode;
      if (hp == NULL) {
	fflush(stdout);
	fprintf(stderr,
		"; Bad server: %s -- using default server and timer opts\n",
		srv);
	fflush(stderr);
        srvmsg = defsrv;
	srv = NULL;
	_res.options = Toptions;
      } else {
	bcopy(Tres,&_res,sizeof(_res));
	bcopy(hp->h_addr, &_res.nsaddr_list[0].sin_addr, hp->h_length);
	_res.nscount = 1;
        srvmsg = strcat(srvbuf,srv);
        strcat(srvbuf, "  ");
        strcat(srvmsg,inet_ntoa(_res.nsaddr.sin_addr));
      }
    }
     _res.id += _res.retry;
   }

       _res.id += _res.retry;
/*       _res.options |= RES_DEBUG;  */
       _res.nsaddr.sin_family = AF_INET;
       _res.nsaddr.sin_port = port;

       if (zone) {
	 do_zone(domain,qtype);
	 if (pfcode & PRF_STATS) {
	   gettimeofday(&exectime,NULL);
	   printf(";; FROM: %s to SERVER: %s\n",curhost,srvmsg);
	   printf(";; WHEN: %s",ctime(&(exectime.tv_sec)));
	 }
	  
	 fflush(stdout);
	 continue;
       }

       bytes_out = n = res_mkquery(QUERY, domain, qclass, qtype,
                         (char *)0, 0, NULL, packet, PACKETSZ);
       if (n < 0) {
	 fflush(stderr);
	 printf(";; res_mkquery: buffer too small\n\n");
	 continue;
       }
       eecode = 0;
       if ((bytes_in = n = res_send(packet, n, answer, PACKETSZ)) < 0) {
	 fflush(stdout);
         n = 0 - n;
         msg[0]=0;
         strcat(msg,";; res_send to server ");
         strcat(msg,srvmsg);
	 perror(msg);
	 fflush(stderr);

#ifndef RESLOCAL
/*
 * resolver does not currently calculate elapsed time
 * if there is an error in res_send()
 */
/*
	 if (pfcode & PRF_STATS) {
	   printf(";; Error time: "); prnttime(&hqtime); putchar('\n');
	 }
*/
#endif
	 if (!dofile) {
            if (eecode)
	      exit(eecode);
	    else
	      exit(9);
	  }
       }
#ifndef RESLOCAL
       else {
	 if (pfcode & PRF_STATS) {
	   printf(";; Sent %d pkts, answer found in time: ",hqcount);
	   prnttime(&hqtime);
	   if (hxcount)
	     printf(" sent %d too many pkts",hxcount);
	   putchar('\n');
	 }
       }
#endif

       if (pfcode & PRF_STATS) {
	 gettimeofday(&exectime,NULL);
	 gethostname(curhost,30);
	 printf(";; FROM: %s to SERVER: %s\n",curhost,srvmsg);
	 printf(";; WHEN: %s",ctime(&(exectime.tv_sec)));
         printf(";; MSG SIZE  sent: %d  rcvd: %d\n", bytes_out, bytes_in);
       }
	  
       fflush(stdout);
/*
 *   Argh ... not particularly elegant. Should put in *real* ping code.
 *   Would necessitate root priviledges for icmp port though!
 */
       if (*pingstr) {
         sprintf(doping,"%s -s %s 56 3 | tail -3",pingstr,
		 (srv==NULL)?(defsrv+10):srv);
	 system(doping);
       }
       putchar('\n');

/*
 * Fairly crude method and low overhead method of keeping two
 * batches started at different sites somewhat synchronized.
 */
       gettimeofday(&tv2, NULL);
       tv1.tv_sec += wait;
       difftv(&tv1,&tv2,&tv3);
       if (tv3.tv_sec > 0)
	 sleep(tv3.tv_sec);
	  }
	return(eecode);
}

/*
 ***************************************************************************
 *
 *  SetOption -- 
 *
 *	This routine is used to change the state information
 *	that affect the lookups. The command format is
 *	   set keyword[=value]
 *	Most keywords can be abbreviated. Parsing is very simplistic--
 *	A value must not be separated from its keyword by white space.
 *
 *      Andrew Cherenson        CS298-26  Fall 1985
 *
 ***************************************************************************
 */

/*
** Modified for 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

int
SetOption(string)
    char *string;
{
    char 	option[NAME_LEN];
    char 	type[NAME_LEN];
    char 	*ptr;
    int 	i;

    i = sscanf(string, " %s", option);
    if (i != 1) {
	fprintf(stderr, ";*** Invalid option: %s\n",  option);
	return(ERROR);
    } 
   
    if (strncmp(option, "aa", 2) == 0) {	/* aaonly */
	    _res.options |= RES_AAONLY;
	} else if (strncmp(option, "noaa", 4) == 0) {
	    _res.options &= ~RES_AAONLY;
	} else if (strncmp(option, "deb", 3) == 0) {	/* debug */
	    _res.options |= RES_DEBUG;
	} else if (strncmp(option, "nodeb", 5) == 0) {
	    _res.options &= ~(RES_DEBUG | RES_DEBUG2);
	} else if (strncmp(option, "ko", 2) == 0) {	/* keepopen */
	    _res.options |= (RES_STAYOPEN | RES_USEVC);
	} else if (strncmp(option, "noko", 4) == 0) {
	    _res.options &= ~RES_STAYOPEN;
	} else if (strncmp(option, "d2", 2) == 0) {	/* d2 (more debug) */
	    _res.options |= (RES_DEBUG | RES_DEBUG2);
	} else if (strncmp(option, "nod2", 4) == 0) {
	    _res.options &= ~RES_DEBUG2;
	} else if (strncmp(option, "def", 3) == 0) {	/* defname */
	    _res.options |= RES_DEFNAMES;
	} else if (strncmp(option, "nodef", 5) == 0) {
	    _res.options &= ~RES_DEFNAMES;
	} else if (strncmp(option, "sea", 3) == 0) {	/* search list */
	    _res.options |= RES_DNSRCH;
	} else if (strncmp(option, "nosea", 5) == 0) {
	    _res.options &= ~RES_DNSRCH;
	} else if (strncmp(option, "do", 2) == 0) {	/* domain */
	    ptr = index(option, '=');
	    if (ptr != NULL) {
		sscanf(++ptr, "%s", _res.defdname);
		res_re_init();
	    }
	  } else if (strncmp(option, "ti", 2) == 0) {      /* timeout */
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      sscanf(++ptr, "%d", &_res.retrans);
	    }

	  } else if (strncmp(option, "ret", 3) == 0) {    /* retry */
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      sscanf(++ptr, "%d", &_res.retry);
	    }

	} else if (strncmp(option, "i", 1) == 0) {	/* ignore */
	    _res.options |= RES_IGNTC;
	} else if (strncmp(option, "noi", 3) == 0) {
	    _res.options &= ~RES_IGNTC;
	} else if (strncmp(option, "pr", 2) == 0) {	/* primary */
	    _res.options |= RES_PRIMARY;
	} else if (strncmp(option, "nop", 3) == 0) {
	    _res.options &= ~RES_PRIMARY;
	} else if (strncmp(option, "rec", 3) == 0) {	/* recurse */
	    _res.options |= RES_RECURSE;
	} else if (strncmp(option, "norec", 5) == 0) {
	    _res.options &= ~RES_RECURSE;
	} else if (strncmp(option, "v", 1) == 0) {	/* vc */
	    _res.options |= RES_USEVC;
	} else if (strncmp(option, "nov", 3) == 0) {
	    _res.options &= ~RES_USEVC;
	} else if (strncmp(option, "pfset", 5) == 0) {
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      pfcode = xstrtonum(++ptr);
	    }
	} else if (strncmp(option, "pfand", 5) == 0) {
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      pfcode = pfcode & xstrtonum(++ptr);
	    }
	} else if (strncmp(option, "pfor", 4) == 0) {
	    ptr = index(option, '=');
	    if (ptr != NULL) {
	      pfcode = pfcode | xstrtonum(++ptr);
	    }
	} else if (strncmp(option, "pfmin", 5) == 0) {
	      pfcode = PRF_MIN;
	} else if (strncmp(option, "pfdef", 5) == 0) {
	      pfcode = PRF_DEF;
	} else if (strncmp(option, "an", 2) == 0) {  /* answer section */
	      pfcode |= PRF_ANS;
	} else if (strncmp(option, "noan", 4) == 0) {
	      pfcode &= ~PRF_ANS;
	} else if (strncmp(option, "qu", 2) == 0) {  /* question section */
	      pfcode |= PRF_QUES;
	} else if (strncmp(option, "noqu", 4) == 0) {  
	      pfcode &= ~PRF_QUES;
	} else if (strncmp(option, "au", 2) == 0) {  /* authority section */
	      pfcode |= PRF_AUTH;
	} else if (strncmp(option, "noau", 4) == 0) {  
	      pfcode &= ~PRF_AUTH;
	} else if (strncmp(option, "ad", 2) == 0) {  /* addition section */
	      pfcode |= PRF_ADD;
	} else if (strncmp(option, "noad", 4) == 0) {  
	      pfcode &= ~PRF_ADD;
	} else if (strncmp(option, "tt", 2) == 0) {  /* TTL & ID */
	      pfcode |= PRF_TTLID;
	} else if (strncmp(option, "nott", 4) == 0) {  
	      pfcode &= ~PRF_TTLID;
	} else if (strncmp(option, "he", 2) == 0) {  /* head flags stats */
	      pfcode |= PRF_HEAD2;
	} else if (strncmp(option, "nohe", 4) == 0) {  
	      pfcode &= ~PRF_HEAD2;
	} else if (strncmp(option, "H", 1) == 0) {  /* header all */
	      pfcode |= PRF_HEADX;
	} else if (strncmp(option, "noH", 3) == 0) {  
	      pfcode &= ~(PRF_HEADX);
	} else if (strncmp(option, "qr", 2) == 0) {  /* query */
	      pfcode |= PRF_QUERY;
	} else if (strncmp(option, "noqr", 4) == 0) {  
	      pfcode &= ~PRF_QUERY;
	} else if (strncmp(option, "rep", 3) == 0) {  /* reply */
	      pfcode |= PRF_REPLY;
	} else if (strncmp(option, "norep", 5) == 0) {  
	      pfcode &= ~PRF_REPLY;
	} else if (strncmp(option, "cm", 2) == 0) {  /* command line */
	      pfcode |= PRF_CMD;
	} else if (strncmp(option, "nocm", 4) == 0) {  
	      pfcode &= ~PRF_CMD;
	} else if (strncmp(option, "cl", 2) == 0) {  /* class mnemonic */
	      pfcode |= PRF_CLASS;
	} else if (strncmp(option, "nocl", 4) == 0) {  
	      pfcode &= ~PRF_CLASS;
	} else if (strncmp(option, "st", 2) == 0) {  /* stats*/
	      pfcode |= PRF_STATS;
	} else if (strncmp(option, "nost", 4) == 0) {  
	      pfcode &= ~PRF_STATS;
	} else if (strncmp(option, "sor", 3) == 0) {  /* sort */
	      pfcode |= PRF_SORT;
	} else if (strncmp(option, "nosor", 5) == 0) {  
	      pfcode &= ~PRF_SORT;
	} else {
	    fprintf(stderr, "; *** Invalid option: %s\n",  option);
	    return(ERROR);
	  }
    return(SUCCESS);
}



/*
 * Fake a reinitialization when the domain is changed.
 */
res_re_init()
{
    register char *cp, **pp;
    int n;

    /* find components of local domain that might be searched */
    pp = _res.dnsrch;
    *pp++ = _res.defdname;
    for (cp = _res.defdname, n = 0; *cp; cp++)
	if (*cp == '.')
	    n++;
    cp = _res.defdname;
    for (; n >= LOCALDOMAINPARTS && pp < _res.dnsrch + MAXDNSRCH; n--) {
	cp = index(cp, '.');
	*pp++ = ++cp;
    }
    *pp = 0;
    _res.options |= RES_INIT;
}



/*
** Written for 'dig' version 1.0 at University of Southern
** California Information Sciences Institute (USC-ISI). 3/27/89
*/
/*
 * convert char string (decimal, octal, or hex) to integer
 */
int xstrtonum(p)
char *p;
{
int v = 0;
int i;
int b = 10;
int flag = 0;
while (*p != 0) {
  if (!flag++)
    if (*p == '0') {
      b = 8; p++;
      continue;
    }
  if (isupper(*p))
      *p=tolower(*p);
  if (*p == 'x') {
    b = 16; p++;
    continue;
  }
  if (isdigit(*p)) {
    i = *p - '0';
  } else if (isxdigit(*p)) {
    i = *p - 'a' + 10;
  } else {
    fprintf(stderr,"; *** Bad char in numeric string -- ignored\n");
    i = -1;
  }
  if (i >= b) {
    fprintf(stderr,"; *** Bad char in numeric string -- ignored\n");
    i = -1;
  }
  if (i >= 0)
    v = v * b + i;
p++;
}
return(v);
}

/*
** Modified for and distributed with 'dig' version 2.0 from
** University of Southern California Information Sciences Institute
** (USC-ISI). 9/1/90
*/

/*
 ****************************************************************************
 *
 *  subr.c --
 *
 *	Miscellaneous subroutines for the name server 
 *	lookup program.
 *  
 *	Copyright (c) 1985 
 *  	Andrew Cherenson
 *  	CS298-26  Fall 1985
 *
 ****************************************************************************
*/

/*
 *****************************************************************************
 *
 *  DecodeError --
 *
 *	Converts an error code into a character string.
 *
 ****************************************************************************
*/


/*
char *
DecodeError(result)
    int result;
{
	switch(result) {
	    case NOERROR: 	return("Success"); break;
	    case FORMERR:	return("Format error"); break;
	    case SERVFAIL:	return("Server failed"); break;
	    case NXDOMAIN:	return("Non-existent domain"); break;
	    case NOTIMP:	return("Not implemented"); break;
	    case REFUSED:	return("Query refused"); break;
	    case NOCHANGE:	return("No change"); break;
	    case NO_INFO: 	return("No information"); break;
	    case ERROR: 	return("Unspecified error"); break;
	    case TIME_OUT: 	return("Timed out"); break;
	    case NONAUTH: 	return("Non-authoritative answer"); break;
	    default: 		break;
	}
	return("BAD ERROR VALUE"); 
      }


*/



int
StringToClass(class, dflt)
    char *class;
    int dflt;
{
	if (strcasecmp(class, "IN") == 0)
		return(C_IN);
	if (strcasecmp(class, "CHAOS") == 0)
		return(C_CHAOS);
	if (strcasecmp(class, "ANY") == 0)
		return(C_ANY);
/*	fprintf(stderr, "; ** unknown query class: %s\n", class); */
	return(dflt);
}

/*
 ***************************************************************************
 *
 *  StringToType --
 *
 *	Converts a string form of a query type name to its 
 *	corresponding integer value.
 *
 *******************************************************************************
 */


int
StringToType(type, dflt)
    char *type;
    int dflt;
{
	if (strcasecmp(type, "A") == 0)
		return(T_A);
	if (strcasecmp(type, "NS") == 0)
		return(T_NS);			/* authoritative server */
	if (strcasecmp(type, "MX") == 0)
		return(T_MX);			/* mail exchanger */
	if (strcasecmp(type, "CNAME") == 0)
		return(T_CNAME);		/* canonical name */
	if (strcasecmp(type, "SOA") == 0)
		return(T_SOA);			/* start of authority zone */
	if (strcasecmp(type, "MB") == 0)
		return(T_MB);			/* mailbox domain name */
	if (strcasecmp(type, "MG") == 0)
		return(T_MG);			/* mail group member */
	if (strcasecmp(type, "MR") == 0)
		return(T_MR);			/* mail rename name */
	if (strcasecmp(type, "WKS") == 0)
		return(T_WKS);			/* well known service */
	if (strcasecmp(type, "PTR") == 0)
		return(T_PTR);			/* domain name pointer */
	if (strcasecmp(type, "HINFO") == 0)
		return(T_HINFO);		/* host information */
	if (strcasecmp(type, "MINFO") == 0)
		return(T_MINFO);		/* mailbox information */
	if (strcasecmp(type, "AXFR") == 0)
		return(T_AXFR);			/* zone transfer */
	if (strcasecmp(type, "MAILB") == 0)
		return(T_MAILB);		/* mail box */
	if (strcasecmp(type, "ANY") == 0)
		return(T_ANY);			/* matches any type */
	if (strcasecmp(type, "TXT") == 0)       
	  return(T_TXT);                        /* text strings */
	if (strcasecmp(type, "UINFO") == 0)
		return(T_UINFO);		/* user info */
	if (strcasecmp(type, "UID") == 0)
		return(T_UID);			/* user id */
	if (strcasecmp(type, "GID") == 0)
		return(T_GID);			/* group id */
/*	fprintf(stderr, "; ** unknown query type: %s\n", type); */
	return(dflt);
}

/*
 ****************************************************************************
 *
 *  DecodeType --
 *
 *	Converts a query type to a descriptive name.
 *	(A more verbose form of p_type.)
 *
 *
 ****************************************************************************
*/

static  char nbuf[20];

char *
DecodeType(type)
	int type;
{
	switch (type) {
	case T_A:
		return("address");
	case T_NS:
		return("name server");
	case T_MX:		
		return("mail exchanger");
	case T_CNAME:		
		return("cannonical name");
	case T_SOA:		
		return("start of authority zone");
	case T_MB:		
		return("mailbox domain name");
	case T_MG:		
		return("mail group member");
	case T_MR:		
		return("mail rename name");
	case T_NULL:		
		return("null resource record");
	case T_WKS:		
		return("well known service");
	case T_PTR:		
		return("domain name pointer");
	case T_HINFO:		
		return("host");
	case T_MINFO:		
		return("mailbox (MINFO)");
	case T_AXFR:		
		return("zone transfer");
	case T_MAILB:		
		return("mail box");
	case T_ANY:		
		return("any type");
	case T_UINFO:
		return("user info");
	case T_UID:
		return("user id");
	case T_GID:
		return("group id");
        case T_TXT:
		return("txt");
	default:
		(void) sprintf(nbuf, "%d", type);
		return (nbuf);
	}
}


/*
******************************************************************************
 *
 *  IntrHandler --
 *
 *	This routine is called whenever a control-C is typed. 
 *	It performs three main functions:
 *	 - close an open socket connection.
 *	 - close an open output file (used by LookupHost, et al.)
 *	 - jump back to the main read-eval loop.
 *		
 *	Since a user may type a ^C in the middle of a routine that
 *	is using a socket, the socket would never get closed by the 
 *	routine. To prevent an overflow of the process's open file table,
 *	the socket and output file descriptors are closed by
 *	the interrupt handler.
 *
 *  Side effects:
 *	If sockFD is valid, its socket is closed.
 *	If filePtr is valid, its file is closed.
 *	Flow of control returns to the main() routine.
 *
 ****************************************************************************
*/

/*

int
IntrHandler()
{
    extern jmp_buf env;

    if (sockFD >= 0) {
	close(sockFD);
	sockFD = -1;
    }
    if (filePtr != NULL && filePtr != stdout) {
	fclose(filePtr);
	filePtr = NULL;
    }
    printf("\n");
    longjmp(env, 1);
}
*/

/*
 **************************************************************************
 *
 *  Calloc --
 *
 *      Calls the calloc library routine with the interrupt
 *      signal blocked.  The interrupt signal blocking is done
 *      to prevent malloc from getting confused if a
 *      control-C arrives in the middle of its bookkeeping
 *      routines.  We need to do this because a control-C
 *      causes a return to the main command loop instead
 *      causing the program to die.
 *
 *	This method doesn't prevent the pointer returned
 *	by calloc from getting lost, so it is possible
 *	to get "core leaks".
 *
 *  Results:
 *	(address)	- address of new buffer.
 *	NULL		- calloc failed.
 *
 **************************************************************************
*/

/*
char *
Calloc(num, size)
    unsigned num, size;
{
	char 	*ptr;
	int 	saveMask;
	extern char *calloc();

	saveMask = sigblock(1 << (SIGINT-1));
	ptr = calloc(num, size);
	(void) sigsetmask(saveMask);
	if (ptr == NULL) {
	    fflush(stdout);
	    fprintf(stderr, "; ** Calloc failed\n");
	    fflush(stderr);
	    abort();
	} else {
	    return(ptr);
	}
}
*/

/*
 ****************************************************************************
 *
 *  PrintHostInfo --
 *
 *	Prints out the HostInfo structure for a host.
 *
 *****************************************************************************
*/

 
/*
void
PrintHostInfo(file, title, hp)
	FILE 	*file;
	char 	*title;
	register HostInfo *hp;
{
	register char 		**cp;
	register ServerInfo 	**sp;
	char 			comma;
	int  			i;

	fprintf(file, "%-7s  %s\n", title, hp->name);

	if (hp->addrList != NULL) {
	    if (hp->addrList[1] != NULL) {
		fprintf(file, "Addresses:");
	    } else {
		fprintf(file, "Address:");
	    }
	    comma = ' ';
	    i = 0;
	    for (cp = hp->addrList; cp && *cp; cp++) {
		i++;
		if (i > 4) {
		    fprintf(file, "\n\t");
		    comma = ' ';
		    i = 0;
		}
	   fprintf(file,"%c %s", comma, inet_ntoa(*(struct in_addr *)*cp));
		comma = ',';
	    }
	}

	if (hp->aliases != NULL) {
	    fprintf(file, "\nAliases:");
	    comma = ' ';
	    i = 10;
	    for (cp = hp->aliases; cp && *cp && **cp; cp++) {
		i += strlen(*cp) + 2;
		if (i > 75) {
		    fprintf(file, "\n\t");
		    comma = ' ';
		    i = 10;
		}
		fprintf(file, "%c %s", comma, *cp);
		comma = ',';
	    }
	}

	if (hp->servers != NULL) {
	    fprintf(file, "Served by:\n");
	    for (sp = hp->servers; *sp != NULL ; sp++) {

		fprintf(file, "- %s\n\t",  (*sp)->name);

		comma = ' ';
		i = 0;
		for (cp = (*sp)->addrList; cp && *cp && **cp; cp++) {
		    i++;
		    if (i > 4) {
			fprintf(file, "\n\t");
			comma = ' ';
			i = 0;
		    }
		    fprintf(file, 
			"%c %s", comma, inet_ntoa(*(struct in_addr *)*cp));
		    comma = ',';
		}
		fprintf(file, "\n\t");

		comma = ' ';
		i = 10;
		for (cp = (*sp)->domains; cp && *cp && **cp; cp++) {
		    i += strlen(*cp) + 2;
		    if (i > 75) {
			fprintf(file, "\n\t");
			comma = ' ';
			i = 10;
		    }
		    fprintf(file, "%c %s", comma, *cp);
		    comma = ',';
		}
		fprintf(file, "\n");
	    }
	}

	fprintf(file, "\n\n");
}
*/

/*
 *****************************************************************************
 *
 *  OpenFile --
 *
 *	Parses a command string for a file name and opens
 *	the file.
 *
 *  Results:
 *	file pointer	- the open was successful.
 *	NULL		- there was an error opening the file or
 *			  the input string was invalid.
 *
 *****************************************************************************
*/



/*
 *  Open an output file if we see '>' or >>'.
 *  Check for overwrite (">") or concatenation (">>").
 */

/*
FILE *
OpenFile(string, file)
    char *string;
    char *file;
{
	char 	*redirect;
	FILE 	*tmpPtr;

	redirect = index(string, '>');
	if (redirect == NULL) {
	    return(NULL);
	}
	if (redirect[1] == '>') {
	    sscanf(redirect, ">> %s", file);
	    tmpPtr = fopen(file, "a+");
	} else {
	    sscanf(redirect, "> %s", file);
	    tmpPtr = fopen(file, "w");
	}

	if (tmpPtr != NULL) {
	    redirect[0] = '\0';
	}

	return(tmpPtr);
}
*/

#ifdef RESLOCAL
/*
** Written with 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

static int qptr = 0;
struct timelist _qtime[HMAXQTIME];
struct timeval hqtime;
u_short hqcount, hxcount;

struct timeval
*findtime(id)
     u_short id;
{
int i;
  for (i=0; i<HMAXQTIME; i++)
    if (_qtime[i].id == id)
      return(&(_qtime[i].time));
  return(NULL);
}


savetime(id,t)
     u_short id;
     struct timeval *t;
{
qptr = ++qptr % HMAXQTIME;
_qtime[qptr].id = id;
_qtime[qptr].time.tv_sec = t->tv_sec;
_qtime[qptr].time.tv_usec = t->tv_usec;
}


struct timeval
*difftv(a,b,tmp)
     struct timeval *a, *b, *tmp;
{
  tmp->tv_sec = a->tv_sec - b->tv_sec;
  if ((tmp->tv_usec = a->tv_usec - b->tv_usec) < 0) {
    tmp->tv_sec--;
    tmp->tv_usec += 1000000;
  }
return(tmp);
}


prnttime(t)
     struct timeval *t;
{
printf("%u msec ",t->tv_sec * 1000 + (t->tv_usec / 1000));
}
#endif /* RESLOCAL */
