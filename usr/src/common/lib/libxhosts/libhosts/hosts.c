/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libhosts:libhosts/hosts.c	1.1"
#include	<stdio.h>
#include	<malloc.h>
#include	<string.h>
#include	<mail/tree.h>

typedef struct host_s
    {
    char
	*h_name,
	*h_address;
    void
	*h_data,
	(*h_dataFree)();
    }	host_t;

static int
    DebugLevel = 0;

void
    hostFree(host_t *host_p)
	{
	if(host_p != NULL)
	    {
	    if(host_p->h_name != NULL) free(host_p->h_name);
	    if(host_p->h_address != NULL) free(host_p->h_address);
	    if((host_p->h_data != NULL) && (host_p->h_dataFree != NULL))
		{
		host_p->h_dataFree(host_p->h_data);
		}

	    free(host_p);
	    }
	}

host_t
    *hostNew(char *name, char *address, void *data, void (*dataFree)())
	{
	host_t
	    *result;
	
	if(DebugLevel > 2)
	    {
	    (void) fprintf
		(
		stderr,
		"hostNew(%s, %s, 0x%x, 0x%x) Entered.\n",
		(name == NULL)? "NIL": name,
		(address == NULL)? "NIL": address,
		(int) data,
		(int) dataFree
		);
	    }

	if((result = (host_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else if((result->h_name = strdup(name)) == NULL)
	    {
	    /* ERROR No Memory */
	    hostFree(result);
	    result = NULL;
	    }
	else if((result->h_address = strdup(address)) == NULL)
	    {
	    /* ERROR No Memory */
	    hostFree(result);
	    result = NULL;
	    }
	else
	    {
	    result->h_data = data;
	    result->h_dataFree = dataFree;
	    }
	
	if(DebugLevel > 2)
	    {
	    (void) fprintf(stderr, "hostNew() = 0x%x Exited.\n", (int) result);
	    }

	return(result);
	}

char
    *hostName(host_t *host_p)
	{
	return((host_p != NULL)? host_p->h_name: NULL);
	}

char
    *hostAddress(host_t *host_p)
	{
	return((host_p != NULL)? host_p->h_address: NULL);
	}

void
    *hostData(host_t *host_p)
	{
	return((host_p != NULL)? host_p->h_data: NULL);
	}

void
    *hostInit(int debugLevel)
	{
	DebugLevel = debugLevel;
	if(DebugLevel > 0)
	    {
	    fprintf(stderr, "hostInit(%d) Done.\n", DebugLevel);
	    }

	return(treeInit("/usr/lib/trees/hosts", debugLevel));
	}

