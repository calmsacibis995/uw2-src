/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ypcmd:getlist.c	1.3.7.3"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include "ypsym.h"

extern void free();
extern char *strdup();

/*
 * Add a name to the list
 */
static listofnames *
newname(str)
char *str;
{
	listofnames *it;
	char *copy;

	if (str==NULL) return(NULL);
	copy=strdup(str);
	if (copy==NULL) return(NULL);
	it=(listofnames *) malloc(sizeof(listofnames));
	if (it==NULL){
		free(copy);
		return(NULL);
	}
	it->name=copy;
	it->nextname=NULL;
	return(it);
}

/*
 * Assemble the list of names
 */
listofnames *
names (filename, count)
int count;
char *filename;
{
	listofnames *list;
	listofnames *end;
	listofnames *nname;
	FILE *fyle;
	char line[256];
	char name[256];

	fyle=fopen(filename,"r");
	if (fyle==NULL) {
		return(NULL);
	}
	list=NULL;
	while (fgets(line,sizeof(line),fyle)) {
		if (line[0]=='#') continue;
		if (line[0]=='\0') continue;
		if (line[0]=='\n') continue;
		nname = newname(line);
		if (nname) {
			if (list==NULL) {
					list = nname;
					end = nname;
			} else {
				end->nextname = nname;
				end = nname;
			}
			count++;
		} else
			pfmt(stderr, MM_STD, ":1:file %s bad malloc %s\n",filename,name);
	}
	fclose(fyle);
	return(list);
}

void
free_listofnames(list)
listofnames *list;
{
	listofnames *next=(listofnames *)NULL;

	for(; list;list=next)
		{
		next=list->nextname;
		if (list->name) free(list->name);
		free((char *)list);	
		}
}


#ifdef MAIN
main(argc,argv)
char **argv;
{
	listofnames *list;

        (void)setlocale(LC_ALL,"");
        (void)setcat("uxypserv");
        (void)setlabel("UX:ypserv");
 
	list=names(argv[1]);
#ifdef DEBUG
	print_listofnames(list);
#endif
	free_listofnames(list);
#ifdef DEBUG
	pfmt(stderr, MM_STD | MM_INFO, ":2:Done\n");
#endif
}
#endif

#ifdef DEBUG
void
print_listofnames(list)
listofnames *list;
{
	if (list==NULL) pfmt(stdout, MM_NOSTD, ":3:NULL\n");
	for(; list;list=list->nextname)
		pfmt(stdout, MM_NOSTD, ":4:%s\n",list->name);
}
#endif
