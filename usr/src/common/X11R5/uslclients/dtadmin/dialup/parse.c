/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/parse.c	1.1"
#endif

/*
 * retrieve information from the node.attr file
 */

#include <stdio.h>
#include <string.h>
#include <dtcopy.h>

extern	char	*malloc();
extern  char	*_cleanline();

int
AttrParse(filename, list) 
char		*filename;
stringll_t 	**list;
{

	register 	stringll_t *new;
	stringll_t 	**dst;
	FILE		*fin;
	char		*namep;
	char		*valuep;
	char		buf[BUFSIZ];

	if ((fin = fopen(filename, "r")) == NULL) {
		(void)fprintf(stderr,
			"AttrParse(): open failed on file \"%s\"\n",
				filename);
		return(-1);
	}
	
	dst = list;
	while (fgets(buf, sizeof buf, fin) != NULL) {

		namep = buf;

		if (buf[0] == '#')
			continue;  /* skip comments */

		namep = _cleanline(namep);
		if ( strlen(namep) == 0)
			continue;  /* skip empty lines */

		if ( (valuep = strchr(namep, (int)'=')) == (char *)NULL)
			continue;
		*valuep++ = '\0';

		if ( (new = (stringll_t *)malloc( sizeof( stringll_t ) ))
			== NULL_STRING ) {
			(void)fprintf(stderr,
				"AttrParse(): failed to allocate memory\n");
			(void)fclose(fin);
			return(-1);
		}
		if ( (new -> name = strdup( namep )) == (char *)0 ) {
			(void)fprintf(stderr,
				"AttrParse(): failed to allocate memory\n");
			(void)fclose(fin);
			return(-1);
		}
		if ( (new -> value = strdup( valuep )) == (char *)0 ) {
			(void)fprintf(stderr,
				"AttrParse(): failed to allocate memory\n");
			(void)fclose(fin);
			return(-1);
		}

		new -> next = NULL_STRING;
		*dst = new;
		dst = &(new -> next);
	}
	(void)fclose(fin);
	return(0);
}

char	*
GetAttrValue(list, name)
stringll_t	*list;
char		*name;
{
	stringll_t	*token;
	stringll_t	*save_token;

	save_token = NULL_STRING;
	for(token = list; token != NULL_STRING; token = token->next) {
		if ( strcmp(name, token->name) == 0 && 
				strlen(token->value) )
			save_token = token;
	}

	if (save_token)
		return(save_token->value);
	else
		return((char *)NULL);
}
