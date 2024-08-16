/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:sharedObjects/system/system.c	1.3"
#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<dirent.h>
#include	<sys/types.h>
#include	<sys/file.h>
#include	<mail/link.h>
#include	<mail/hosts.h>
#include	<mail/tree.h>

#define	HOSTS_FILE	"/etc/hosts"

char
    _tree_listName[] = "System Files";

int
    _tree_openList(void *treeList_p, void *localData_p)
	{
	int
	    result = 0;

	char
	    buffer[256],
	    *address,
	    *name;

	void
	    *curHost_p;

	FILE
	    *fp_hosts;

	if((fp_hosts = fopen(HOSTS_FILE, "r")) == NULL)
	    {
	    }
	else
	    {
	    while(fgets(buffer, sizeof(buffer), fp_hosts) != NULL)
		{
		if((address = strtok(buffer, " \t\r\n")) == NULL)
		    {
		    /* Blank line */
		    }
		else if(*address == '#' || *address == ';')
		    {
		    /* COMMENT */
		    }
		else if((name = strtok(NULL, " \t\r\n;#")) == NULL)
		    {
		    /* Blank line */
		    }
		else if((curHost_p = hostNew(name, address, NULL, NULL)) == NULL)
		    {
		    /* ERROR No Memory */
		    }
		else if
		    (
		    nodeNew
			(
			name,
			treeList_p,
			curHost_p,
			hostFree,
			NULL,
			NULL
			) == NULL
		    )
		    {
		    }
		else
		    {
		    result = 1;
		    }
		}

	    (void) fclose(fp_hosts);
	    }

	treeListCallbackDo(treeList_p, result);
	return(result);
	}

void
    _tree_closeList(void *treeList_p, void *localData_p)
	{
	}

void
    *_tree_newListData(treeList_t *treeList_p)
	{
	return(NULL);
	}

void
    _tree_freeListData(void *localData_p)
	{
	}
	
