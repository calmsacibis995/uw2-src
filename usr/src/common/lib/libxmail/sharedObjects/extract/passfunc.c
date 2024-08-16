/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:sharedObjects/extract/passfunc.c	1.2"
#include	<stdio.h>
#include	<malloc.h>
#include	<ctype.h>
#include	<mail/link.h>

int
    striatcmp(char *str1, char *str2)
        {
	int
	    result = 0;

	while(*str1 != '@' && *str2 != '@' && !(result = toupper(*str1) - toupper(*str2++)) && *str1++ != '\0');

	return(result);
	}

int
    stricmp(char *str1, char *str2)
        {
	int
	    result;

	while(!(result = toupper(*str1) - toupper(*str2++)) && *str1++ != '\0');

	return(result);
	}

void
    freeElement(void *element)
	{
	void
	    *owner;

	if(element == NULL)
	    {
	    }
	else
	    {
	    if((owner = linkOwner(element)) != NULL) free(owner);
	    linkFree(element);
	    }
	}

