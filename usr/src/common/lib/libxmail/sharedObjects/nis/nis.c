/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:sharedObjects/nis/nis.c	1.4"
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<errno.h>
#include	<malloc.h>
#include	<rpcsvc/ypclnt.h>
#include	<rpcsvc/yp_prot.h>
#include	<mail/link.h>

typedef struct localData_s
    {
    void
	*ld_aliasList;
    
    int
	ld_result;
    }	localData_t;

extern void
    freeElement();

extern char
    *getenv();

extern int
    stricmp(),
    _mailerror;

int
    _maildir_alias(void *list, char *address)
	{
	char
	    *name,
	    *defaultDomain,
	    *outputBuffer;

	int
	    outputLength,
	    error,
	    result = 0;

	yp_get_default_domain(&defaultDomain);

	if(list == NULL)
	    {
	    }
	else if(address == NULL)
	    {
	    }
	else if(strchr(address, '@') != NULL)
	    {
	    }
	else
	    {
	    error = yp_match
		(
		defaultDomain,
		"mail.aliases",
		address,
		strlen(address),
		&outputBuffer,
		&outputLength
		);
	    
	    if(error == YPERR_KEY)
		{
		error = yp_match
		    (
		    defaultDomain,
		    "mail.aliases",
		    address,
		    strlen(address) + 1,
		    &outputBuffer,
		    &outputLength
		    );
		}
	    
	    if(!error)
		{
		for
		    (
		    name = strtok(outputBuffer, ", \t\n");
		    name != NULL;
		    name = strtok(NULL, ", \t\n")
		    )
		    {
		    if
			(
			linkAddSortedUnique
			    (
			    list,
			    linkNew(strdup(name)),
			    stricmp,
			    freeElement
			    )
			)
			{
			}
		    else
			{
			result++;
			}
		    }

		free(outputBuffer);
		}
	    }

	return(result);
	}

char
    *_maildir_revAlias(char *address)
	{
	char
	    *name,
	    *defaultDomain,
	    *outputBuffer,
	    *result;

	int
	    outputLength;

	yp_get_default_domain(&defaultDomain);

	if(address == NULL)
	    {
	    }
	else if(strchr(address, '@') != NULL)
	    {
	    }
	else if
	    (
	    yp_match
		(
		defaultDomain,
		"mail.aliases",
		address,
		strlen(address) + 1,
		&outputBuffer,
		&outputLength
		) != 0
	    )
	    {
	    }
	else 
	    {
	    if((result = strtok(outputBuffer, " \t\n")) != NULL)
		{
		result = strdup(result);
		}

	    free(outputBuffer);
	    }

	return(result);
	}

#if	0
char
    _maildir_listName[] = "Network Information Services";

static int
    doAlias
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
	    *alias_p;

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

		/* Make the alias */
		if
		    (
			(
			alias_p = aliasNew(name, localData_p->ld_aliasList)
			) == NULL
		    )
		    {
		    result = 1;
		    /* ERROR No Memory */
		    }
		else
		    {
		    result = 0;
		    for
			(
			address = strtok(inval, " \t\r\n");
			address != NULL;
			address = strtok(NULL, " \t\r\n")
			)
			{
			aliasElementNew(alias_p, address);
			}

		    localData_p->ld_result++;
		    }

		break;
		}
	    }

	return(result);
	}

int
    _maildir_openList(void *aliasList_p, void *data)
	{
	localData_t
	    localData;

	char
	    *defaultDomain;

	struct ypall_callback
	    callback;

	localData.ld_result = 0;
	localData.ld_aliasList = aliasList_p;

	callback.foreach = doAlias;
	callback.data = (char *)&localData;

	yp_get_default_domain(&defaultDomain);

	yp_all(defaultDomain, "alias.byname", &callback);

	aliasListCallbackDo(aliasList_p, localData.ld_result);
	return(localData.ld_result);
	}

void
    _maildir_closeList(void *aliasList_p, void *localData_p)
	{
	}

void
    *_maildir_newListData(void *info)
	{
	return(NULL);
	}

void
    _maildir_freeListData(void *localData_p)
	{
	}
	
#endif
