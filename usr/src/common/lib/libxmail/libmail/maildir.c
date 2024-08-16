/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/maildir.c	1.4"
#include	<stdio.h>
#include	<unistd.h>
#include	<malloc.h>
#include	<string.h>
#include	<dlfcn.h>
#include	<sys/types.h>
#include	<sys/file.h>
#include	<mail/link.h>

#define	MAILDIR_ACCESS	1
#define	MAILDIR_NOMEM	2
#define	MAILDIR_OPEN	3
#define	MAILDIR_NOSYM	4

#define MAILDIR_CONF	"/etc/mail/lookupLibs"

static struct translator
    {
    int
	(*tr_aliases)(),	/* _maildir_aliases	*/
	(*tr_openList)();
    char
	*(*tr_revAliases)();	/* _maildir_revAliases	*/
    int
	tr_fd;		/* library descriptor	*/
    char
	*tr_listName,
	*tr_name;	/* Full path		*/
    void
	*(*tr_newListData)(),
	(*tr_freeListData)(),
	(*tr_closeList)(),
	*tr_link;
    };

extern int
    errno;

int _mailerror;

int
    stricmp(char *, char *);

void
    freeElement(void *);

/*
 * load_xlate is a routine that will attempt to dynamically link in the
 * file specified by the network configuration structure.
 */
static struct translator
    *load_xlate(char *name)
	{
	struct translator
	    *result = NULL;

	/* do a sanity check on the file ... */
	if (access(name, 00) != 0)
	    {
	    _mailerror = MAILDIR_ACCESS;
	    }
	else if
	    (
		(
		result = (struct translator *) malloc(sizeof (struct translator))
		) == NULL
	    )
	    {
	    _mailerror = MAILDIR_NOMEM;
	    }
	else if((result->tr_link = linkNew(result)) == NULL)
	    {
	    _mailerror = MAILDIR_NOMEM;
	    free((char *)result);
	    result = NULL;
	    }
	else if((result->tr_name = strdup(name)) == NULL)
	    {
	    _mailerror = MAILDIR_NOMEM;
	    linkFree(result->tr_link);
	    free((char *)result);
	    result = NULL;
	    }
	else if((result->tr_fd = _dlopen(name, RTLD_LAZY)) == 0)
	    {
	    _mailerror = MAILDIR_OPEN;
	    linkFree(result->tr_link);
	    free((char *)result->tr_name);
	    free((char *)result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->tr_aliases = (int (*)())_dlsym
		    (
		    result->tr_fd,
		    "_maildir_alias"
		    )
		) == NULL
	    )
	    {
	    _mailerror = MAILDIR_NOSYM;
	    _dlclose(result->tr_fd);
	    linkFree(result->tr_link);
	    free((char *)result->tr_name);
	    free((char *)result);
	    result = NULL;
	    }
	else if
	    (
		(
		result->tr_revAliases = (char *(*)())_dlsym
		    (
		    result->tr_fd,
		    "_maildir_revAlias"
		    )
		) == NULL
	    )
	    {
	    _mailerror = MAILDIR_NOSYM;
	    _dlclose(result->tr_fd);
	    linkFree(result->tr_link);
	    free((char *)result->tr_name);
	    free((char *)result);
	    result = NULL;
	    }
	else
	    {
	    if
		(
		    (
		    result->tr_openList = (int (*)())_dlsym
			(
			result->tr_fd,
			"_maildir_openList"
			)
		    ) == NULL
		)
		{
		result->tr_openList = NULL;
		}

	    if
		(
		    (
		    result->tr_closeList = (void (*)())_dlsym
			(
			result->tr_fd,
			"_maildir_closeList"
			)
		    ) == NULL
		)
		{
		result->tr_closeList = NULL;
		}

	    if
		(
		    (
		    result->tr_newListData = (void *(*)())_dlsym
			(
			result->tr_fd,
			"_maildir_newListData"
			)
		    ) == NULL
		)
		{
		result->tr_newListData = NULL;
		}

	    if
		(
		    (
		    result->tr_freeListData = (void (*)())_dlsym
			(
			result->tr_fd,
			"_maildir_freeListData"
			)
		    ) == NULL
		)
		{
		result->tr_freeListData = NULL;
		}

	    result->tr_listName = (char *)_dlsym
		(
		result->tr_fd,
		"_maildir_listName"
		);
	    }

	return (result);
	}

struct translator
    *readConfig()
	{
	static void
	    *result = NULL;

	struct translator
	    *cur_p;

	FILE
	    *fp = NULL;
	
	char
	    *filename,
	    buffer[256];

	if(result != NULL)
	    {
	    }
	else if((result = linkNew(NULL)) == NULL)
	    {
	    }
	else if((fp = fopen(MAILDIR_CONF, "r")) == NULL)
	    {
	    perror(MAILDIR_CONF);
	    }
	else while(fgets(buffer, sizeof(buffer), fp) != NULL)
	    {
	    if((filename = strtok(buffer, "\n")) == NULL)
		{
		}
	    else if((cur_p = load_xlate(filename)) != NULL)
		{
		(void) linkAppend(result, cur_p->tr_link);
		}
	    }

	if(fp != NULL) (void) fclose(fp);

	return(result);
	}

void
    *maildir_alias(char *name, int fullyResolv)
	{
	void
	    *result = NULL,
	    *input,
	    *oldList,
	    *oldLink_p,
	    *curLink_p;

	char		
	    *oldName;

	int
	    delete;

	struct translator
	    *t;


	if((input = linkNew(NULL)) == NULL)
	    {
	    }
	else if((curLink_p = linkNew(strdup(name))) == NULL)
	    {
	    linkFree(input);
	    }
	else if((oldList = linkNew(NULL)) == NULL)
	    {
	    free(linkOwner(curLink_p));
	    linkFree(curLink_p);
	    linkFree(input);
	    }
	else if(linkAppend(input, curLink_p) == NULL)
	    {
	    linkFree(oldList);
	    free(linkOwner(curLink_p));
	    linkFree(curLink_p);
	    linkFree(input);
	    }
	else if((result = linkNew(NULL)) == NULL)
	    {
	    linkFree(oldList);
	    free(linkOwner(curLink_p));
	    linkFree(curLink_p);
	    linkFree(input);
	    }
	else if(fullyResolv)
	    {
	    while((curLink_p = linkNext(input)) != input)
		{
		(void) linkRemove(curLink_p);
		name = linkOwner(curLink_p);

		for
		    (
		    oldLink_p = linkNext(oldList);
		    (oldName = (char *)linkOwner(oldLink_p)) != NULL;
		    oldLink_p = linkNext(oldLink_p)
		    )
		    {
		    if(!stricmp(name, oldName))
			{
			linkFree(curLink_p);
			free(name);
			break;
			}
		    }

		if(oldName != NULL)
		    {
		    /* There was an alias loop & the name has been discarded. */
		    }
		else
		    {
		    for
			(
			t = (struct translator*)linkOwner(linkNext(readConfig())),
			    delete = 0;
			t != NULL && !delete;
			t = (struct translator*)linkOwner(linkNext(t->tr_link))
			)
			{
			/*fprintf(stderr, "resolving %s with %s.", name, t->tr_name);*/
			if((t->tr_aliases)(input, name))
			    {
			    /*fprintf(stderr, "\tmatch");*/
			    delete = 1;
			    }

			/*fprintf(stderr, "\n");*/
			}

		    if(delete)
			{
			(void) linkAppend(oldList, curLink_p);
			}
		    else
			{
			(void) linkAddSortedUnique(result, curLink_p, stricmp, freeElement);
			}
		    }
		}

	    for
		(
		oldLink_p = linkNext(oldList);
		(oldName = (char *)linkOwner(oldLink_p)) != NULL;
		oldLink_p = linkNext(oldList)
		)
		{
		free(oldName);
		linkFree(oldLink_p);
		}

	    linkFree(oldList);
	    linkFree(input);
	    }
	else
	    {
	    while((curLink_p = linkNext(input)) != input)
		{
		(void) linkRemove(curLink_p);
		name = linkOwner(curLink_p);

		for
		    (
		    t = (struct translator*)linkOwner(linkNext(readConfig())),
			delete = 0;
		    t != NULL && !delete;
		    t = (struct translator*)linkOwner(linkNext(t->tr_link))
		    )
		    {
		    /*fprintf(stderr, "resolving %s with %s.", name, t->tr_name);*/
		    if((t->tr_aliases)(result, name))
			{
			/*fprintf(stderr, "\tmatch");*/
			delete = 1;
			}

		    /*fprintf(stderr, "\n");*/
		    }

		if(delete)
		    {
		    free(name);
		    linkFree(curLink_p);
		    }
		else
		    {
		    (void) linkAddSortedUnique(result, curLink_p, stricmp, freeElement);
		    }
		}

	    linkFree(oldList);
	    linkFree(input);
	    }

	return (result);	/* No one works */
	}

char
    *maildir_revAlias(char *name)
	{
	char
	    *address,
	    *localPart = NULL,
	    *domain = NULL,
	    *curDomain,
	    *result = NULL;

	struct translator
	    *t;

	/*fprintf(stderr, "maildir_revAlias(%s)\n", name);*/
	if((address = strchr(name, '<')) == NULL)
	    {
	    address = name;
	    }

	/*fprintf(stderr, "\taddress = %s\n", address);*/

	if((name = strdup(address)) == NULL)
	    {
	    }
	else if((address = strtok(name, "<>")) == NULL)
	    {
	    free(name);
	    }
	else if((localPart = strtok(address, "@\r\n")) == NULL)
	    {
	    free(name);
	    }
	else if((localPart = strdup(localPart)) == NULL)
	    {
	    free(name);
	    }
	else
	    {
	    if((domain = strtok(NULL, "@\r\n")) == NULL)
		{
		}
	    else if((curDomain = domain = strdup(domain)) == NULL)
		{
		}
	    else
		{
		while(result == NULL && curDomain != NULL)
		    {
		    strcpy(name, localPart);
		    strcat(name, "@");
		    strcat(name, curDomain);

		    /*fprintf(stderr, "trying %s.\n", name);*/
		    for
			(
			t = (struct translator*)linkOwner(linkNext(readConfig()));
			t != NULL;
			t = (struct translator*)linkOwner(linkNext(t->tr_link))
			)
			{
			if((result = (*(t->tr_revAliases))(name)) != NULL)
			    {
			    break;
			    }
			}

		    curDomain = strchr(curDomain, '.');
		    if(curDomain != NULL) curDomain++;
		    }

		free(domain);
		}

	    if(result != NULL)
		{
		}
	    else for
		(
		t = (struct translator*)linkOwner(linkNext(readConfig()));
		t != NULL;
		t = (struct translator*)linkOwner(linkNext(t->tr_link))
		)
		{
		if((result = (*(t->tr_revAliases))(name)) != NULL)
		    {
		    break;
		    }
		}

	    free(name);
	    free(localPart);
	    }

	return (result);	/* No one works */
	}

#if	0
int
    treeRootOpen(void *parentList_p, void *data)
	{
	void
	    *curData_p;

	struct translator
	    *t;

	for
	    (
	    t = (struct translator*)linkOwner(linkNext(readConfig()));
	    t != NULL;
	    t = (struct translator*)linkOwner(linkNext(t->tr_link))
	    )
	    {
	    if(t->tr_listName == NULL)
		{
		}
	    else if
		(
		aliasListNew
		    (
		    t->tr_listName,
		    parentList_p,
		    curData_p = t->tr_newListData(NULL),
		    t->tr_freeListData,
		    t->tr_openList,
		    t->tr_closeList
		    ) == NULL
		)
		{
		t->tr_freeListData(curData_p);
		}
	    }

	aliasListCallbackDo(parentList_p, 1);
	return(1);
	}
#endif
