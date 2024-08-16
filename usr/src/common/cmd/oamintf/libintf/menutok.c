/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)oamintf:common/cmd/oamintf/libintf/menutok.c	1.1.6.2"
#ident  "$Header: menutok.c 2.0 91/07/12 $"
#include <string.h>
#include <ctype.h>
#include "intf.h"

char *
menutok(str)
char	*str;
{
	static char *save = NULL;
	char	*head;

	if(str == NULL)
		str = save;

	/* eat leading white space */
	while(isspace(*str))
		str++;

	head = str;

	/* find ending token, if any */
	while((*str != NULL) && (*str != '\n') && (*str != TAB_CHAR))
		str++;

	if(*str != NULL) {
		*str = NULL;
		save = str+1;
	} else
		save = str;
	return((head == str) ? NULL : head);
}

