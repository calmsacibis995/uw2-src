/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/getcwd.c	1.1.1.3"
#include	"sccs.h"
SCCSID(@(#)getcwd.c	6.5	LCC);	/* Modified: 14:09:57 1/7/92 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <string.h>
#include <sys/types.h>

#include "pci_types.h"
#include "dossvr.h"
#include "xdir.h"



/*
 *  pci_getcwd() -- parses input string and stores current working directory
 */

void
pci_getcwd(ptr, cwd)
register char *ptr;			/* Points to CHDIR input string */
register char *cwd;                     /* Points to place to write the cwd */
{
    register char 
	*component,
	*tptr;

    if ((*ptr == '\0') || (strcmp(ptr, "/") == 0)) {
	strcpy(cwd, "/");
	return;
    }

    if (*ptr == '/')
	strcpy(cwd, "");

/*
 * Parse the input string with '/' as a token and maintain 
 * a string which represents our location within the tree.
 */

    while (component = strtok(ptr, "/")) {
    /* Reset pointer to NULL */
	ptr = 0;

    /* Case 1: append string to cwd */
	if (component[0] != '.') {
	    if (strcmp(cwd, "/") == 0)
		strcat(cwd, component);
	    else {
	        strcat(cwd, "/");
		strcat(cwd, component);
	    }
	/* String must be null terminated! */
	    if (strlen(component) >= (unsigned int)(MAX_FN_COMP))
		cwd[strlen(cwd)+1] = '\0';
	}

    /* Case 2: string refers to parent directory */
	else if ((strcmp(component, "..")) == 0) {
	    tptr = strrchr(cwd, '/');
	    *tptr = 0;
	    if (strcmp(cwd, "") == 0)
		strcpy(cwd, "/");
	}
    }
    if (*cwd == '\0')
	strcpy(cwd, "/");
}
