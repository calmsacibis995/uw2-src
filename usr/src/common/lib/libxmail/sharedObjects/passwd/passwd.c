/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:sharedObjects/passwd/passwd.c	1.3"
#include	<string.h>
#include	<ctype.h>
#include	<stdio.h>
#include	<errno.h>
#include	<malloc.h>
#include	<pwd.h>
#include	<mail/link.h>
#include	<mail/table.h>

typedef struct aliasEntry_s
    {
    void
	*ae_link;

    char
	*ae_name,
	*ae_address;
    }	aliasEntry_t;

extern void
    freeElement();

extern int
    stricmp(),
    _mailerror;

static void
    *AliasTable = NULL,
    *AliasList = NULL;
    
static void
    aliasEntryFree(aliasEntry_t *aliasEntry_p)
	{
	if(aliasEntry_p != NULL)
	    {
	    if(aliasEntry_p->ae_link != NULL) linkFree(aliasEntry_p->ae_link);
	    if(aliasEntry_p->ae_name != NULL) free(aliasEntry_p->ae_name);
	    if(aliasEntry_p->ae_address != NULL) free(aliasEntry_p->ae_address);
	    free(aliasEntry_p);
	    }
	}

static aliasEntry_t
    *aliasEntryNew(char *name, char *address)
	{
	aliasEntry_t
	    *result;
	
	if(!stricmp(name, address))
	    {
	    /* Alias loop, throw away.*/
	    result = NULL;
	    }
	else if((result = (aliasEntry_t *)calloc(sizeof(*result), 1)) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else if((result->ae_link = linkNew(result)) == NULL)
	    {
	    /* ERROR No Memory */
	    aliasEntryFree(result);
	    result = NULL;
	    }
	else if((result->ae_name = strdup(name)) == NULL)
	    {
	    /* ERROR No Memory */
	    aliasEntryFree(result);
	    result = NULL;
	    }
	else if((result->ae_address = strdup(address)) == NULL)
	    {
	    /* ERROR No Memory */
	    aliasEntryFree(result);
	    result = NULL;
	    }
	else
	    {
	    linkAppend(AliasList, result->ae_link);
	    }
	
	return(result);
	}

extern char
    *mailsystem(int);

static void
    initAliasTable()
	{
	struct passwd
	    *passwd_p;

	FILE
	    *fp;

	int
	    systemNameLen;

	char
	    *systemName;
	
	systemNameLen = ((systemName = mailsystem(1)) == NULL)?
	    -1: strlen(systemName);

	if(AliasTable != NULL)
	    {
	    }
	else if((fp = fopen("/etc/passwd", "r")) == NULL)
	    {
	    }
	else if((AliasTable = tableNew()) == NULL)
	    {
	    (void) fclose(fp);
	    }
	else if((AliasList = linkNew(NULL)) == NULL)
	    {
	    tableFree(AliasTable);
	    AliasTable = NULL;
	    (void) fclose(fp);
	    }
	else
	    {
	    while((passwd_p = fgetpwent(fp)) != NULL)
		{
		char
		    *ptr,
		    *newName,
		    buffer[256];
		
		(void) strncpy(buffer, passwd_p->pw_comment, sizeof(buffer));
		while((ptr = strchr(buffer, ' ')) != NULL)
		    {
		    *ptr = '_';
		    }
		
		if(systemNameLen < 0)
		    {
		    newName = strdup(passwd_p->pw_name);
		    }
		else if
		    (
			(
			newName = malloc
			    (
			    systemNameLen + 2 + strlen(passwd_p->pw_name)
			    )
			) == NULL
		    )
		    {
		    }
		else
		    {
		    (void) strcpy(newName, passwd_p->pw_name);
		    (void) strcat(newName, "@");
		    (void) strcat(newName, systemName);
		    }

		tableAddEntry
		    (
		    AliasTable,
		    buffer,
		    newName,
		    free
		    );

		tableAddEntry
		    (
		    AliasTable,
		    passwd_p->pw_name,
		    strdup(newName),
		    free
		    );

		switch(passwd_p->pw_uid)
		    {
		    default:
			{
			if(passwd_p->pw_uid > 100)
			    {
			    (void) aliasEntryNew(buffer, passwd_p->pw_name);
			    }

			break;
			}

		    case	60001:
		    case	60002:
			{
			break;
			}
		    }
		}
	    
	    (void) fclose(fp);
	    }
	}

int
    _maildir_alias(void *list, char *address)
	{
	char
	    *alias;

	int
	    result = 0;

	initAliasTable();

	if(list == NULL)
	    {
	    }
	else if(address == NULL)
	    {
	    }
	else if(strchr(address, '@') != NULL)
	    {
	    }
	else if
	    (
		(
		alias = (char *)tableGetValueByNoCaseString(AliasTable, address)
		) == NULL
	    )
	    {
	    }
	else if
	    (
	    linkAddSortedUnique
		(
		list,
		linkNew(strdup(alias)),
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

	return(result);
	}

char
    *_maildir_revAlias(char *address)
	{
	return(NULL);
	}

#if	0
char
    _maildir_listName[] = "Password File";

int
    _maildir_openList(void *aliasList_p, void *localData_p)
	{
	int
	    result = 0;

	aliasEntry_t
	    *curEntry_p;

	void
	    *alias_p;

	initAliasTable();

	for
	    (
	    curEntry_p = (aliasEntry_t *)linkOwner(linkNext(AliasList));
	    curEntry_p != NULL;
	    curEntry_p = (aliasEntry_t *)linkOwner(linkNext(curEntry_p->ae_link))
	    )
	    {
	    if
		(
		    (
		    alias_p = aliasNew
			(
			curEntry_p->ae_name,
			aliasList_p
			)
		    ) != NULL
		)
		{
		aliasElementNew(alias_p, curEntry_p->ae_address);
		result = 1;
		}
	    }

	aliasListCallbackDo(aliasList_p, result);
	return(result);
	}

void
    *_maildir_newListData(void *info)
	{
	return(NULL);
	}
#endif
