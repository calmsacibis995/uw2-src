/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:bridge/mem.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)mem.c	6.3	LCC);	/* Modified: 23:21:58 7/12/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <lmf.h>

#include "common.h"
#include "const.h"
#include "log.h"


char *
memory(amount)
register int	amount;
{
    register char	
	*newmem;		/* Newly allocated memory	*/

    if ((newmem = malloc((unsigned) amount)) == NULL)
	fatal(lmf_format_string((char *) NULL, 0,
		lmf_get_message("MEM1","memory: Can't get %1 bytes\n"),
		"%d", amount));

    return newmem;
}



char *
morememory(ptr, amount)
char
	*ptr;
register int
	amount;
{
register char	
	*newmem;		/* Newly allocated memory	*/

	if ((newmem = realloc(ptr, (unsigned) amount)) == NULL)
		fatal(lmf_format_string((char *) NULL, 0, 
			lmf_get_message("MEM2",
			"memory: Can't resize to %1\n"), "%d", amount));

	return newmem;
}

/* strdup(), but with a fatal error on out-of-memory. */
char *
savestr(s)
char *s;
{
	register char *newmem;
	if(newmem = memory(strlen(s)+1)) strcpy(newmem, s);
	return newmem;
}
