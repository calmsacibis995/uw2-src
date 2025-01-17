/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/res_init.c	1.2"
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
static char sccsid[] = "@(#)res_init.c	6.8 (Berkeley) 3/7/88";
#endif /* LIBC_SCCS and not lint */

#include "hfiles.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include NAMESERH
#ifndef T_TXT
#define T_TXT 16
#endif /* T_TXT */
#include RESOLVH

/*
 * Resolver configuration file. Contains the address of the
 * inital name server to query and the default domain for
 * non fully qualified domain names.
 */

#ifndef	CONFFILE
#define	CONFFILE	"/etc/resolv.conf"
#endif

/*
 * Resolver state default settings
 */

struct state _res = {
    RES_TIMEOUT,               	/* retransmition time interval */
    4,                         	/* number of times to retransmit */
    RES_DEFAULT,		/* options flags */
    1,                         	/* number of name servers */
};

/*
 * Set up default settings.  If the configuration file exist, the values
 * there will have precedence.  Otherwise, the server address is set to
 * INADDR_ANY and the default domain name comes from the gethostname().
 *
 * The configuration file should only be used if you want to redefine your
 * domain or run without a server on your machine.
 *
 * Return 0 if completes successfully, -1 on error
 */
res_init()
{
    register FILE *fp;
    register char *cp, **pp;
    char buf[BUFSIZ];
    char *afile;
    extern u_long inet_addr();
    extern char *index();
    extern char *strcpy(), *strncpy();
    extern char *getenv();
    int n = 0;    /* number of nameserver records read from file */

    _res.nsaddr.sin_addr.s_addr = INADDR_ANY;
    _res.nsaddr.sin_family = AF_INET;
    _res.nsaddr.sin_port = htons(NAMESERVER_PORT);
    _res.nscount = 1;
    _res.defdname[0] = '\0';

    if ((((afile=getenv("LOCALRES")) != NULL) && 
	 ((fp = fopen(afile,"r")) != NULL)) ||
	((fp = fopen(CONFFILE, "r")) != NULL)) {
        /* read the config file */
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            /* read default domain name */
            if (!strncmp(buf, "domain", sizeof("domain") - 1)) {
                cp = buf + sizeof("domain") - 1;
                while (*cp == ' ' || *cp == '\t')
                    cp++;
                if (*cp == '\0')
                    continue;
                (void)strncpy(_res.defdname, cp, sizeof(_res.defdname));
                _res.defdname[sizeof(_res.defdname) - 1] = '\0';
                if ((cp = index(_res.defdname, '\n')) != NULL)
                    *cp = '\0';
                continue;
            }
            /* read nameservers to query */
            if (!strncmp(buf, "nameserver", 
               sizeof("nameserver") - 1) && (n < MAXNS)) {
                cp = buf + sizeof("nameserver") - 1;
                while (*cp == ' ' || *cp == '\t')
                    cp++;
                if (*cp == '\0')
                    continue;
                _res.nsaddr_list[n].sin_addr.s_addr = inet_addr(cp);
                if (_res.nsaddr_list[n].sin_addr.s_addr == (unsigned)-1) 
                    _res.nsaddr_list[n].sin_addr.s_addr = INADDR_ANY;
                _res.nsaddr_list[n].sin_family = AF_INET;
                _res.nsaddr_list[n].sin_port = htons(NAMESERVER_PORT);
                if ( ++n >= MAXNS) { 
                    n = MAXNS;
#ifdef DEBUG
                    if ( _res.options & RES_DEBUG )
                        printf(";; ** MAXNS reached, reading resolv.conf\n");
#endif /* DEBUG */
                }
                continue;
            }
	  }
        if ( n > 1 ) 
            _res.nscount = n;
        (void) fclose(fp);
      }
    if (_res.defdname[0] == 0) {
        if (gethostname(buf, sizeof(_res.defdname)) == 0 &&
           (cp = index(buf, '.')))
             (void)strcpy(_res.defdname, cp + 1);
    }

    /* Allow user to override the local domain definition */
    if ((cp = getenv("LOCALDOMAIN")) != NULL)
        (void)strncpy(_res.defdname, cp, sizeof(_res.defdname));

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
    _res.options |= RES_INIT;
    return(0);
}
