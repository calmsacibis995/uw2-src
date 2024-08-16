/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:sharedObjects/dns/dns.c	1.3"
#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<dirent.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include	<resolv.h>
#include	<sys/types.h>
#include	<sys/file.h>
#include	<mail/link.h>
#include	<mail/hosts.h>
#include	<mail/tree.h>

#include	"domain.h"
#include	"conn.h"

char
    _tree_listName[] = "Domain Name Server";

int
    _tree_openList(void *treeList_p, domainEntry_t *domainEntry_p)
	{
	int
	    result;
	
	result = domainOpen(treeList_p, domainEntry_p);

	return(result);
	}

void
    _tree_closeList(void *treeList_p, domainEntry_t *domainEntry_p)
	{
	domainClose(treeList_p, domainEntry_p);
	}

void
    *_tree_newListData(treeList_t *treeList_p)
	{
	domainEntry_t
	    *rootDomain_p;

	connectionInit(0);
	domainInit(0);
	tableSetDebugLevel(0);
	rootDomain_p = domainEntryNew("", treeList_p);
	domainEntryNew(_res.defdname, NULL);
	return(rootDomain_p);
	}

void
    _tree_freeListData(domainEntry_t *domainEntry_p)
	{
	domainEntryFree(domainEntry_p);
	}
