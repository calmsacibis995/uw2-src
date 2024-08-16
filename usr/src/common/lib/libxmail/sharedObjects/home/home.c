/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:sharedObjects/home/home.c	1.4"
#include	<string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include	<mail/link.h>

#define	ALIASNAME	"/lib/names"

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
	    buffer[256],
	    *name,
	    *homePath,
	    *aliasPath;

	int
	    result = 0;

	FILE
	    *fp_alias;

	if(list == NULL)
	    {
	    }
	else if(address == NULL)
	    {
	    }
	else if((homePath = getenv("HOME")) == NULL)
	    {
	    }
	else if((aliasPath = malloc(strlen(homePath) + 1 + strlen(ALIASNAME))) == NULL)
	    {
	    }
	else if(strcat(strcpy(aliasPath, homePath), ALIASNAME) == NULL)
	    {
	    }
	else if(strchr(address, '@') != NULL)
	    {
	    }
	else if((fp_alias = fopen(aliasPath, "r")) == NULL)
	    {
	    }
	else
	    {
	    char
		*lastChar,
		*newBuff;

	    int
		ignore,
		continuation,
		nextCont;

	    for
		(
		ignore = continuation = 0;
		fgets( buffer, sizeof(buffer), fp_alias) != NULL;
		continuation = nextCont,
		    ignore = ignore && continuation
		)
		{
		if((newBuff = strtok(buffer, "\r\n")) == NULL)
		    {
		    nextCont = 0;
		    }
		else
		    {
		    lastChar = newBuff + strlen(newBuff) - 1;
		    if(nextCont = (*lastChar == '\\'))
			{
			*lastChar = '\0';
			}

		    if(continuation)
			{
			}
		    else
			{
			name = strtok(newBuff, " \t\n,:");
			newBuff = NULL;
			if(name == NULL)
			    {
			    ignore = 1;
			    }
			else if(*name == '#')
			    {
			    ignore = 1;
			    }
			else if(stricmp(name, address))
			    {
			    ignore = 1;
			    }
			}

		    if(ignore)
			{
			}
		    else while(name = strtok(newBuff, " \t:,\n"))
			{
			newBuff = NULL;
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
		    }
		}
	    
	    (void) fclose(fp_alias);
	    }

	return(result);
	}

char
    *_maildir_revAlias(char *address)
	{
	return(NULL);
	}

#if	0
char
    _maildir_listName[] = "Home Directory File";

int
    _maildir_openList(void *aliasList_p, void *localData_p)
	{
	int
	    result = 0;

	void
	    *alias_p;

	char
	    buffer[256],
	    *address,
	    *homePath,
	    *aliasPath,
	    *name;

	FILE
	    *fp_alias;

	if((homePath = getenv("HOME")) == NULL)
	    {
	    }
	else if((aliasPath = malloc(strlen(homePath) + 1 + strlen(ALIASNAME))) == NULL)
	    {
	    }
	else if(strcat(strcpy(aliasPath, homePath), ALIASNAME) == NULL)
	    {
	    }
	else if((fp_alias = fopen(aliasPath, "r")) == NULL)
	    {
	    }
	else
	    {
	    while(fgets( buffer, sizeof(buffer), fp_alias) != NULL)
		{
		name = strtok(buffer, " \t\n,:");
		if(*name == '#')
		    {
		    }
		else if
		    (
		    (alias_p = aliasNew(name, aliasList_p)) == NULL
		    )
		    {
		    /* ERROR No Memory */
		    }
		else
		    {
		    result = 1;
		    while(address = strtok(NULL, " \t:,\n"))
			{
			aliasElementNew(alias_p, address);
			}
		    }
		}
	    
	    (void) fclose(fp_alias);
	    }

	aliasListCallbackDo(aliasList_p, result);
	return(result);
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
