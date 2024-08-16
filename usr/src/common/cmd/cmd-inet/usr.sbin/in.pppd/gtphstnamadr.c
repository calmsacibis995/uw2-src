/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static char TCPID[] = "@(#)gtphstnamadr.c	1.2 STREAMWare for Unixware 2.0 source";
#endif /* lint */
/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/gtphstnamadr.c	1.4"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
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
#include <netdb.h>
#include <sys/types.h>
#include <sys/file.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/conf.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ppp.h>

#ifdef SYSV
#define bcmp(s1, s2, len)	memcmp(s1, s2, len)
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)
#endif /* SYSV */

#define	MAXALIASES	35

static struct ppphostent ppphost;
static char ppphostbuf[BUFSIZ+1];
static char *ppphost_addrs[2];

int h_errno;

/*
 * The following is shared with getppphostent.c
 */
extern	char *_ppphost_file;
int	_ppphost_stayopen;	/* set by setppphostent(), cleared by endppphostent() */

struct ppphostent *
getppphostbyname(nam)
	register char *nam;
{
	register struct ppphostent *hp;
	register char **cp;

	setppphostent(1);
	while (hp = getppphostent()) {
		if (hp->ppp_h_ent == NULL)
			continue;
		if (hp->ppp_h_ent->h_addrtype == AF_INET)
			continue;
		if (strcmp(hp->ppp_h_ent->h_name, nam) == 0)
			break;
		for (cp = hp->ppp_h_ent->h_aliases; cp != 0 && *cp != 0; cp++)
			if (strcmp(*cp, nam) == 0)
				goto found;
	}
found:
	endppphostent();
	return (hp);
}

struct ppphostent *
getppphostbyaddr(addr, length, type)
	char *addr;
	register int length;
	register int type;
{
	register struct ppphostent *hp;
	
	setppphostent(1);
	while (hp = getppphostent()) {
		if (hp->ppp_h_ent->h_addrtype == type
			&& hp->ppp_h_ent->h_length == length
		    && bcmp(hp->ppp_h_ent->h_addr, addr, length) == 0){
			break;
		}
	}
	endppphostent();
	if ( hp == NULL){
		h_errno = HOST_NOT_FOUND;
	}
	return (hp);
}
#ifdef EASY
struct ppphostent *
getppphostbyname(nam)
char *nam;
{
	return ((struct ppphostent *) 0);
}

struct ppphostent *
getppphostbyaddr(addr, length, type)
char *addr;
int length;
int type;
{
	return ((struct ppphostent *) 0);
}
#endif

