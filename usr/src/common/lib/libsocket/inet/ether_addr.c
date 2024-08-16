/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)libsocket:common/lib/libsocket/inet/ether_addr.c	1.1.8.11"
#ident  "$Header: $"

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

/*
 * All routines necessary to deal with the file /etc/ethers.  The file
 * contains mappings from 48 bit ethernet addresses to their corresponding
 * hosts name.  The addresses have an ascii representation of the form
 * "x:x:x:x:x:x" where x is a hex number between 0x00 and 0xff;  the
 * bytes are always in network order.
 */

#include "../socketabi.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <stdlib.h>
#include <netdb.h>
#include <ns.h>
#include "../libsock_mt.h"

static const char *ethers = "/etc/ethers";

/*
 * Parses a line from /etc/ethers into its components.  The line has the form
 * 8:0:20:1:17:c8	krypton
 * where the first part is a 48 bit ethernet addrerss and the second is
 * the corresponding hosts name.
 * Returns zero if successful, non-zero otherwise.
 */
ether_line(s, e, hostname)
	char *s;		/* the string to be parsed */
	ether_addr_t e;	/* ethernet address struct to be filled in */
	char *hostname;		/* hosts name to be set */
{
	register int i;
	unsigned int t[6];
	
	i = sscanf(s, " %x:%x:%x:%x:%x:%x %s",
	    &t[0], &t[1], &t[2], &t[3], &t[4], &t[5], hostname);
	if (i != 7) {
		return (7 - i);
	}
	for (i = 0; i < 6; i++)
		e[i] = (u_char) t[i];
	return (0);
}

/*
 * Converts a 48 bit ethernet number to its string representation.
 */
char *
ether_ntoa(e)
	ether_addr_t e;
{
	char *s;
	static char *ss;

#ifdef _REENTRANT
        struct _s_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		s = ss;
	}
	else {
		key_tbl = (struct _s_tsd *)
			  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
		if (key_tbl == NULL) return (NULL);
		s = key_tbl->ether_s_p;
	}
#else
	s = ss;
#endif /* _REENTRANT */

	if (s == 0) {
		s = (char *)malloc((unsigned) 18);
		if (s == 0)
			return (NULL);
#ifdef _REENTRANT
		if (FIRST_OR_NO_THREAD) {
			ss = s;
		} else {
			key_tbl->ether_s_p = s;
		}
#else
		ss = s;
#endif /* _REENTRANT */
	}
	s[0] = '\000';
	(void) sprintf(s, "%x:%x:%x:%x:%x:%x",
		e[0], e[1], e[2], e[3], e[4], e[5]);

	return (s);
}

#ifdef _REENTRANT

void
_free_s_ether_s(p)
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
 * Converts a ethernet address representation back into its 48 bits.
 */
ether_addr_t *
ether_aton(s)
	char *s;
{
	static ether_addr_t e;
	ether_addr_t *ep;
	register int i;
	unsigned int t[6];
#ifdef _REENTRANT
        struct _s_tsd *key_tbl;

	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL) 
		return (NULL);
	if ((key_tbl->ether_e_p == NULL)
 	 && ((key_tbl->ether_e_p = calloc(1, sizeof (ether_addr_t)))
	     == NULL))
		return (NULL);
	ep = key_tbl->ether_e_p;
#else
	ep = &e;
#endif /* _REENTRANT */
	
	i = sscanf(s, " %x:%x:%x:%x:%x:%x",
	    &t[0], &t[1], &t[2], &t[3], &t[4], &t[5]);
	if (i != 6)
	    return ((ether_addr_t *)NULL);
	for (i = 0; i < 6; i++)
		(*ep)[i] = (u_char) t[i];
	return (ep);
}

#ifdef _REENTRANT

void
_free_s_ether_e(p)
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
 * Given a host's name, this routine returns its 48 bit ethernet address.
 * Returns zero if successful, non-zero otherwise.
 */
ether_hostton(host, e)
	char *host;		/* function input */
	ether_addr_t e;	/* function output */
{
	NS_GETNAME2(ETHDB, host, e, int);

	return(_ether_hostton(host, e));
}

_ether_hostton(host, e)
	char *host;		/* function input */
	ether_addr_t e;	/* function output */
{
	char currenthost[256];
	char buf[512];
	char *val = buf;
	register int reason;
	FILE *f;
	void *ns;

		if ((f = fopen(ethers, "r")) == NULL) {
			return (-1);
		}
		reason = -1;
		while (fscanf(f, "%[^\n] ", val) == 1) {
			if ((ether_line(val, e, currenthost) == 0) &&
			    (strcmp(currenthost, host) == 0)) {
				reason = 0;
				break;
			}
		}
		(void) fclose(f);
		return (reason);
}

/*
 * Given a 48 bit ethernet address, this routine return its host name.
 * Returns zero if successful, non-zero otherwise.
 */
ether_ntohost(host, e)
	char *host;		/* function output */
	ether_addr_t e;	/* function input */
{
	NS_GETNUM2(ETHDB, host, e, int);

	return(_ether_ntohost(host, e));
}

_ether_ntohost(host, e)
	char *host;		/* function output */
	ether_addr_t e;	/* function input */
{
	ether_addr_t currente;
	char buf[512];
	char *val = buf;
	register int reason;
	FILE *f;
	
	if ((f = fopen(ethers, "r")) == NULL) {
		return (-1);
	}
	reason = -1;
	while (fscanf(f, "%[^\n] ", val) == 1) {
		if ((ether_line(val, currente, host) == 0) &&
		    (memcmp((char*) e, (char*) currente, sizeof(currente)) == 0)) {
			reason = 0;
			break;
		}
	}
	(void) fclose(f);
	return (reason);
}
