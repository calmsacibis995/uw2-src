/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:sharedObjects/nis/nis.c	1.1"
#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<dirent.h>
#include	<rpcsvc/ypclnt.h>
#include	<rpcsvc/yp_prot.h>
#include	<sys/types.h>
#include	<sys/file.h>
#include	<mail/link.h>
#include	<mail/hosts.h>
#include	<mail/tree.h>

typedef struct localData_s
    {
    void
	*ld_treeList;
    
    int
	ld_result;
    }	localData_t;

char
    _tree_listName[] = "Network Information Service";

static int
    doHost
	(
	int instatus,
	char *inkey,
	int inkeylen,
	char *inval,
	int invallen,
	localData_t *localData_p
	)
	{
	int
	    result;

	char
	    *name,
	    *address;

	void
	    *curHost_p;

	switch(instatus)
	    {
	    default:
		{
		result = 1;
		break;
		}

	    case	YP_TRUE:
		{
		/* NULL Terminate the key and value */
		if(inkey[inkeylen - 1] != '\0') inkey[inkeylen] = '\0';
		if(inval[invallen - 1] != '\0') inval[invallen] = '\0';

		/* Strip white space from the key and value */
		name = strtok(inkey, " \t\r\n");
		address = strtok(inval, " \t\r\n");

		/* Make the host */
		if((curHost_p = hostNew(name, address, NULL, NULL)) == NULL)
		    {
		    /* ERROR No Memory */
		    result = 1;
		    }
		else if
		    (
		    nodeNew
			(
			name,
			localData_p->ld_treeList,
			curHost_p,
			hostFree,
			NULL,
			NULL
			) == NULL
		    )
		    {
		    result = 1;
		    }
		else
		    {
		    localData_p->ld_result = 1;
		    result = 0;
		    }

		break;
		}
	    }

	return(result);
	}

#if	0
int
    _tree_openList(void *treeList_p, void *localData_p)
	{
	localData_t
	    localData;

	int
	    outKeyLen,
	    outValLen,
	    rcode;

	char
	    *outKey,
	    *outVal,
	    *defaultDomain;

	localData.ld_result = 0;
	localData.ld_treeList = treeList_p;

	yp_get_default_domain(&defaultDomain);
	if((rcode = yp_bind(defaultDomain)) != 0)
	    {
	    fprintf(stderr, "rcode = %d => %s.\n", rcode, yperr_string(rcode));
	    fflush(stderr);
	    }
	else for
	    (
	    rcode = yp_first
		(
		defaultDomain,
		"hosts.byname",
		&outKey,
		&outKeyLen,
		&outVal,
		&outValLen
		);
	    rcode == 0;
	    rcode = yp_next
		(
		defaultDomain,
		"hosts.byname",
		&outKey,
		&outKeyLen,
		&outVal,
		&outValLen
		)
	    )
	    {
	    doHost
		(
		YP_TRUE,
		outKey,
		outKeyLen,
		outVal,
		outValLen,
		localData_p
		);
	    }

	treeListCallbackDo(treeList_p, localData.ld_result);
	return(localData.ld_result);
	}

#else
int
    _tree_openList(void *treeList_p, void *localData_p)
	{
	localData_t
	    localData;

	char
	    *defaultDomain;

	struct ypall_callback
	    callback;

	localData.ld_result = 0;
	localData.ld_treeList = treeList_p;

	callback.foreach = doHost;
	callback.data = (char *)&localData;

	yp_get_default_domain(&defaultDomain);

	(void) yp_all(defaultDomain, "hosts.byname", &callback);

	treeListCallbackDo(treeList_p, localData.ld_result);
	return(localData.ld_result);
	}
#endif

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
	
