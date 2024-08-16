/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:sharedObjects/dns/domain.h	1.1"
#if	!defined(DOMAIN_H)
#define	DOMAIN_H

#include	<mail/tree.h>

#if	!defined(DOMAINOBJ)
typedef void domainEntry_t;
#endif

domainEntry_t
    *domainEntryNew(char *domainName, treeList_t *treeList_p);

int
    domainOpen(treeList_t *treeList_p, domainEntry_t *domainEntry_p);

void
    domainEntryFree(domainEntry_t *domainEntry_p),
    domainClose(treeList_t *treeList_p, domainEntry_t *domainEntry_p),
    domainInit(int debugLevel);

#endif
