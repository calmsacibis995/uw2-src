/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresolv:common/lib/libsresolv/res_init.c	1.1.1.7"
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

#include "res.h"
#include <sys/byteorder.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <errno.h>
#include "libres_mt.h"

#pragma weak res_init=_rs_res_init

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

struct state *
get_rs__res()
{

#ifdef _REENTRANT
        struct _rs_tsd *key_tbl;
	struct state *rp;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&_res);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rs_tsd *)
		  _mt_get_thr_specific_storage(_rs_key, _RS_KEYTBL_SIZE);
	if (key_tbl == NULL) return (struct state *)NULL;
	if (key_tbl->_res_p == NULL) {
		if ((rp = (struct state *) (key_tbl->_res_p
					    = calloc(1, sizeof(struct state))))
		    != NULL)
			rp->retrans = RES_TIMEOUT;
			rp->retry   = 4;
			rp->options = RES_DEFAULT;
			rp->nscount = 1;
	}
	return (key_tbl->_res_p);
#else /* !_REENTRANT */
	return (&_res);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rs__res(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

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
_rs_res_init()
{
    register FILE *fp;
    register char *cp, **pp;
    char buf[BUFSIZ];
    extern u_long inet_addr();
    extern char *index();
    extern char *strcpy(), *strncpy();
    extern char *getenv();
    int n = 0;    /* number of nameserver records read from file */
    struct state *rp;
	
    /* Get thread-specific data */
    if ((rp = get_rs__res()) == NULL)
    	return (-1);

    rp->nsaddr.sin_addr.s_addr =  htonl(INADDR_ANY);
    rp->nsaddr.sin_family = AF_INET;
    rp->nsaddr.sin_port = htons(NAMESERVER_PORT);
    rp->nscount = 1;
      /*
       * for the benefit of hidden YP domains, we use the same procedure
       * as sendmail: convert leading + to dot, then drop to first dot
       */
    getdomainname( buf, BUFSIZ);
    if (buf[0] == '+')
	buf[0] = '.';
    cp = index(buf, '.');
    if (cp == NULL)
    	strcpy(rp->defdname, buf);
    else 
    	strcpy(rp->defdname, cp+1);

    if ((fp = _fopen(CONFFILE, "r")) != NULL) {
        /* read the config file */
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            /* read default domain name */
            if (!strncmp(buf, "domain", sizeof("domain") - 1)) {
                cp = buf + sizeof("domain") - 1;
                while (*cp == ' ' || *cp == '\t')
                    cp++;
                if (*cp == '\0')
                    continue;
                (void)strncpy(rp->defdname, cp, sizeof(rp->defdname));
                rp->defdname[sizeof(rp->defdname) - 1] = '\0';
                if ((cp = index(rp->defdname, '\n')) != NULL)
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
                rp->nsaddr_list[n].sin_addr.s_addr = inet_addr(cp);
                if (rp->nsaddr_list[n].sin_addr.s_addr == (unsigned)-1) 
                    rp->nsaddr_list[n].sin_addr.s_addr = INADDR_ANY;
                rp->nsaddr_list[n].sin_family = AF_INET;
                rp->nsaddr_list[n].sin_port = htons(NAMESERVER_PORT);
                if ( ++n >= MAXNS) { 
                    n = MAXNS;
#ifdef DEBUG
                    if ( rp->options & RES_DEBUG )
                        printf("MAXNS reached, reading resolv.conf\n");
#endif DEBUG
                }
                continue;
            }
        }
        if ( n > 1 ) 
            rp->nscount = n;
        (void) fclose(fp);
    }
    if (rp->defdname[0] == 0) {
        if (gethostname(buf, sizeof(rp->defdname)) == 0 &&
           (cp = index(buf, '.')))
             (void)strcpy(rp->defdname, cp + 1);
    }

    /* Allow user to override the local domain definition */
    if ((cp = getenv("LOCALDOMAIN")) != NULL)
        (void)strncpy(rp->defdname, cp, sizeof(rp->defdname));

    /* find components of local domain that might be searched */
    pp = rp->dnsrch;
    *pp++ = rp->defdname;
    for (cp = rp->defdname, n = 0; *cp; cp++)
	if (*cp == '.')
	    n++;
    cp = rp->defdname;
    for (; n >= LOCALDOMAINPARTS && pp < rp->dnsrch + MAXDNSRCH; n--) {
	cp = index(cp, '.');
	*pp++ = ++cp;
    }
    rp->options |= RES_INIT;
    return(0);
}
