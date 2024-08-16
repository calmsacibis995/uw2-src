/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:sharedObjects/extract/extract.c	1.4"
#include	<string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include	<sys/mman.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<mail/link.h>
#include	<mail/table.h>

#define	EXTRACT_FILE	"/var/spool/mailq/smf_ns.xrt"
#define	MAP_FILE	"/etc/mail/smfmap"
#define	MAILCNFG	"/etc/mail/mailcnfg"
#define	SMFCNFG	"/etc/mail/smfcnfg"

typedef struct extractEntry_s
    {
    char
	ee_fullName[66],
	ee_longSmfName[256],
	ee_shortSmfName[18],
	ee_department[20],
	ee_title[20],
	ee_phone[28],
	ee_extra[104];
    }	extractEntry_t;

extern void
    freeElement();

extern char
    *getenv();

extern int
    stricmp(),
    _mailerror;

extractEntry_t
    *LastExtractEntry_p = NULL;

table_t
    *getDomainMap()
	{
	static table_t
	    *result = NULL;
	
	FILE
	    *fp_map = NULL;

	char
	    buffer[1024],
	    *domain_p,
	    *workgroup_p;

	if(result != NULL)
	    {
	    }
	else if((result = tableNew()) == NULL)
	    {
	    /* ERROR No Memory */
	    }
	else if((fp_map = fopen(MAP_FILE, "r")) == NULL)
	    {
	    perror(MAP_FILE);
	    }
	else while(fgets(buffer, sizeof(buffer), fp_map) != NULL)
	    {
	    if((domain_p = strtok(buffer, "\r\n\t ")) == NULL)
		{
		/* Blank line */
		}
	    else if(*domain_p == '#')
		{
		/* Comment */
		}
	    else if((workgroup_p = strtok(NULL, "\r\n\t ")) == NULL)
		{
		/* ERROR Bad Syntax */
		}
	    else if((workgroup_p = strdup(workgroup_p)) == NULL)
		{
		/* ERROR No Memory */
		}
	    else
		{
		tableAddEntry(result, domain_p, workgroup_p, free);
		}
	    }
	
	if(fp_map != NULL) fclose(fp_map);

	return(result);
	}

extractEntry_t
    *getExtractPtr()
	{
	static extractEntry_t
	    *extractFile_p = NULL;

	int
	    fd;
	
	struct stat
	    statBuff;

	if(extractFile_p != NULL)
	    {
	    }
	else if((fd = open(EXTRACT_FILE, O_RDONLY)) < 0)
	    {
	    perror(EXTRACT_FILE);
	    }
	else if(fstat(fd, &statBuff))
	    {
	    perror(EXTRACT_FILE);
	    close(fd);
	    }
	else if
	    (
		(
		extractFile_p = (extractEntry_t *)mmap
		    (
		    NULL,
		    statBuff.st_size,
		    PROT_READ,
		    MAP_SHARED,
		    fd,
		    0
		    )
		) == NULL
	    )
	    {
	    perror(EXTRACT_FILE);
	    close(fd);
	    }
	else
	    {
	    LastExtractEntry_p =
		extractFile_p + statBuff.st_size/sizeof(*extractFile_p);

	    close(fd);
	    }
	    
	return(extractFile_p);
	}

char
    *getWorkgroupName(char *address)
	{
	static char
	    *defaultWorkgroup = NULL;
	
	FILE
	    *fp_config;

	char
	    *result,
	    *token,
	    *domain_p,
	    buffer[1024];

	if((domain_p = strchr(address, '@')) != NULL)
	    {
	    *domain_p++ = '\0';
	    result = (char *)tableGetValueByNoCaseString
		(
		getDomainMap(),
		domain_p
		);
	    }
	else if(defaultWorkgroup != NULL)
	    {
	    result = defaultWorkgroup;
	    }
	else if((fp_config = fopen(SMFCNFG, "r")) == NULL)
	    {
	    perror(SMFCNFG);
	    }
	else while(fgets(buffer, sizeof(buffer), fp_config) != NULL)
	    {
	    if((token = strtok(buffer, " \t=\r\n")) == NULL)
		{
		/* empty line */
		}
	    else if(strcmp(token, "WORKGROUP"))
		{
		/* Not workgroup */
		}
	    else if((token = strtok(NULL, " \t=\"\r\n")) == NULL)
		{
		/* Bad format, no workgroup */
		}
	    else
		{
		defaultWorkgroup = result = strdup(token);
		break;
		}
	    }

	return(result);
	}

char
    *getGatewayName()
	{
	static char
	    *result = NULL;
	
	FILE
	    *fp_config;

	char
	    *token,
	    buffer[1024];

	if(result != NULL)
	    {
	    }
	else if((fp_config = fopen(MAILCNFG, "r")) == NULL)
	    {
	    }
	else while(fgets(buffer, sizeof(buffer), fp_config) != NULL)
	    {
	    if((token = strtok(buffer, " \t=\r\n")) == NULL)
		{
		/* empty line */
		}
	    else if(strcmp(token, "%g"))
		{
		/* Not gateway */
		}
	    else if((token = strtok(NULL, " \t=\r\n")) == NULL)
		{
		/* Bad format, no gateway */
		}
	    else
		{
		result = strdup(token);
		break;
		}
	    }

	return(result);
	}

int
    _maildir_alias(void *list, char *address)
	{
	extractEntry_t
	    *extract_p,
	    *curEntry_p;

	char
	    buffer[256],
	    name[512],
	    *mungedAddress,
	    *underscore_p,
	    *gatewayName,
	    *workgroupName,
	    *workgroup,
	    *extractPath;

	int
	    result = 0;

	if(list == NULL)
	    {
	    }
	else if(address == NULL)
	    {
	    }
	else if((mungedAddress = strdup(address)) == NULL)
	    {
	    }
	else if((workgroupName = getWorkgroupName(mungedAddress)) == NULL)
	    {
	    }
	else if((gatewayName = getGatewayName()) == NULL)
	    {
	    }
	else if((extract_p = getExtractPtr()) == NULL)
	    {
	    }
	else
	    {
	    while((underscore_p = strchr(mungedAddress, '_')) != NULL)
		{
		*underscore_p = ' ';
		}

	    for
		(
		curEntry_p = extract_p;
		curEntry_p < LastExtractEntry_p;
		curEntry_p++
		)
		{
		if
		    (
		    striatcmp(mungedAddress, curEntry_p->ee_shortSmfName)
			&& striatcmp(mungedAddress, curEntry_p->ee_longSmfName)
		    )
		    {
		    /* No Match */
		    }
		else if
		    (
			(
			workgroup = strchr(curEntry_p->ee_shortSmfName, '@')
			)  == NULL ||
		    stricmp(workgroup + 1, workgroupName)
		    )
		    {
		    /* No Match */
		    }
		else
		    {
		    (void) strcpy(name, curEntry_p->ee_shortSmfName);
		    (void) strcat(name, ".");
		    (void) strcat(name, gatewayName);

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

	    free(mungedAddress);
	    }

	return(result);
	}

char
    *_maildir_revAlias(char *address)
	{
	return(NULL);
	}

