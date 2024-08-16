/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:sharedObjects/file/file.c	1.4"
#include	<string.h>
#include	<sys/stat.h>
#include	<ctype.h>
#include	<stdio.h>
#include	<errno.h>
#include	<malloc.h>
#include	<dirent.h>
#include	<limits.h>
#include	<mail/link.h>

#define	CONFIGFILE	"/etc/mail/namefiles"

extern void
    freeElement();

extern int
    stricmp(),
    _mailerror;

int
    _maildir_alias(void *list, char *address)
	{
	char
	    buffer[256],
	    *fileName,
	    *name;

	int
	    result = 0;

	FILE
	    *fp_config,
	    *fp_alias;

	if(list == NULL)
	    {
	    }
	else if(address == NULL)
	    {
	    }
	else if(strchr(address, '@') != NULL)
	    {
	    }
	else if((fp_config = fopen(CONFIGFILE, "r")) == NULL)
	    {
	    }
	else
	    {
	    struct stat
		statbuff;

	    while(fgets(buffer, sizeof(buffer), fp_config) != NULL)
		{
		fileName = strtok(buffer, " \t\n");
		if(*fileName == '#')
		    {
		    }
		else if(stat(fileName, &statbuff))
		    {
		    }
		else if(statbuff.st_mode & S_IFDIR)
		    {
		    char
			*nameBuff;
		    
		    if
			(
			    (
			    nameBuff = malloc
				(strlen(fileName) + strlen(address) + 2)
			    ) == NULL
			)
			{
			}
		    else if(strcpy(nameBuff, fileName) == NULL)
			{
			}
		    else if(strcat(nameBuff, "/") == NULL)
			{
			}
		    else if(strcat(nameBuff, address) == NULL)
			{
			}
		    else if((fp_alias = fopen(nameBuff, "r")) == NULL)
			{
			}
		    else
			{
			while(fgets(buffer, sizeof(buffer), fp_alias) != NULL)
			    {
			    for
				(
				name = strtok(buffer, " \t:,\n");
				name != NULL;
				name = strtok(NULL, " \t:,\n")
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
			    }
			
			(void) fclose(fp_alias);
			}
		    }
		else if((fp_alias = fopen(fileName, "r")) == NULL)
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
		}

	    (void) fclose(fp_config);
	    }

	return(result);
	}

char
    *_maildir_revAlias(char *address)
	{
	return(NULL);
	}

#if	0
static int
    openDirList(void *aliasList_p, char *dirName)
	{
	int
	    result = 0;
	
	DIR
	    *dp;
	
	struct dirent
	    *dirent_p;

	void
	    *alias_p;

	struct stat
	    statbuff;

	char
	    orgDirName[PATH_MAX];

	if(getcwd(orgDirName, sizeof(orgDirName)) == NULL)
	    {
	    /* ERROR */
	    }
	else if((dp = opendir(dirName)) == NULL)
	    {
	    perror(dirName);
	    }
	else if(chdir(dirName))
	    {
	    perror(dirName);
	    closedir(dp);
	    }
	else
	    {
	    while((dirent_p = readdir(dp)) != NULL)
		{
		if(*dirent_p->d_name == '.')
		    {
		    /* Hidden file, ignore */
		    }
		else if(stat(dirent_p->d_name, &statbuff))
		    {
		    perror(dirent_p->d_name);
		    }
		else if(statbuff.st_mode & S_IFDIR)
		    {
		    /* Directory, ignore */
		    }
		else if
		    (
			(
			alias_p = aliasNew
			    (
			    dirent_p->d_name,
			    aliasList_p
			    )
			) == NULL
		    )
		    {
		    /* ERROR No Memory */
		    }
		else
		    {
		    result = 1;
		    }
		}
	    
	    chdir(orgDirName);
	    closedir(dp);
	    }

	aliasListCallbackDo(aliasList_p, result);
	return(result);
	}

static int
    openFileList(void *aliasList_p, char *fileName)
	{
	int
	    result = 0;
	
	void
	    *alias_p;

	char
	    *name,
	    *address,
	    buffer[256];

	FILE
	    *fp_alias;
	
	if((fp_alias = fopen(fileName, "r")) == NULL)
	    {
	    perror(fileName);
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

char
    _maildir_listName[] = "System Files";

int
    _maildir_openList(void *aliasList_p, void *localData_p)
	{
	int
	    result = 0;

	char
	    buffer[256],
	    *curData_p,
	    *fileName;

	FILE
	    *fp_config;

	if((fp_config = fopen(CONFIGFILE, "r")) == NULL)
	    {
	    }
	else
	    {
	    struct stat
		statbuff;

	    while(fgets(buffer, sizeof(buffer), fp_config) != NULL)
		{
		fileName = strtok(buffer, " \t\n");
		if(*fileName == '#')
		    {
		    }
		else if(stat(fileName, &statbuff))
		    {
		    }
		else if
		    (
		    aliasListNew
			(
			fileName,
			aliasList_p,
			curData_p = strdup(fileName),
			free,
			(statbuff.st_mode & S_IFDIR)? openDirList: openFileList,
			NULL
			) == NULL
		    )
		    {
		    free(curData_p);
		    }
		else
		    {
		    result = 1;
		    }
		}

	    (void) fclose(fp_config);
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
