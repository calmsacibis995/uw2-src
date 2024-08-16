/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/symbolic.c	1.2"

#include <stdio.h>
#include "xksh.h" /* which includes sys/types.h */
#include <string.h>
#include <ctype.h>
#include "strparse.h"

struct symlist *Symlist = NULL;
int Nsymlist;

/*
** Notice that the types are defined by their pointer
*/
struct symlist *
fsymbolic(ptr)
VOID *ptr;
{
	int i;

	for (i = 0; i < Nsymlist; i++)
		if (Symlist[i].ptr == ptr)
			return(Symlist + i);
	return(NULL);
}

do_symbolic(argc, argv)
int argc;
char **argv;
{
	int i, nsyms, isflag;
	ulong j;
	struct symarray syms[50];
	char *p;
	char *type;
	VOID *ptr;

	nsyms = 0;
	isflag = 0;
	for (i = 1; argv[i]; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j]; j++) {
				switch(argv[i][j]) {
				case 'm':
					isflag = 1;
					break;
				case 't':
					if (argv[i][j + 1])
						type = argv[i] + j + 1;
					else
						type = argv[++i];
					j = strlen(argv[i]) - 1;
					break;
				}
			}
		}
		else {
			syms[nsyms++].str = argv[i];
			if (nsyms == 50)
				break;
		}
	}
	if (!nsyms) {
		altprintf((const char *) "Symbolics must be given\n");
		XK_USAGE(argv[0]);
	}
	if (p = strchr(type, '.')) {
		*p = '\0';
		ptr = (VOID *) all_tbl_search(type, 0);
		*p = '.';
		if (!ptr) {
			altprintf((const char *) "Cannot find %s\n", type);
			XK_USAGE(argv[0]);
		}
		if ((ptr = (VOID *) fld_find(ptr, p + 1, NULL, NULL, 1)) == NULL) {
			altprintf((const char *) "Cannot find %s\n", type);
			XK_USAGE(argv[0]);
		}
	}
	else if ((ptr = (VOID *) all_tbl_search(type, 0)) == NULL) {
		altprintf((const char *) "Cannot find %s\n", type);
		XK_USAGE(argv[0]);
	}

	for (i = 0; i < nsyms; i++) {
		if (!fdef(syms[i].str, &j)) {
			altprintf((const char *) "Cannot find %s\n", syms[i].str);
			XK_USAGE(argv[0]);
		}
		syms[i].str = strdup(syms[i].str);
		syms[i].addr = j;
	}
	if (*((char *) ptr) & F_READONLY) {
		altprintf((const char *) "Cannot associate symbolics with built-in types\n");
		XK_USAGE(argv[0]);
	}
	add_symbolic(isflag, ptr, syms, nsyms);
	return(SH_SUCC);
}

add_symbolic(isflag, ptr, syms, nsyms)
int isflag;
VOID *ptr;
struct symarray *syms;
int nsyms;
{
	struct symlist *symptr;

	if ((symptr = fsymbolic(ptr)) == NULL) {
		if (!Symlist)
			Symlist = (struct symlist *) malloc((Nsymlist + 1) * sizeof(struct symlist));
		else
			Symlist = (struct symlist *) realloc(Symlist, (Nsymlist + 1) * sizeof(struct symlist));
		if (!Symlist)
			return(SH_FAIL);
		symptr = Symlist + Nsymlist;
		Nsymlist++;
	}
	else
		free(symptr->syms);
	*((char *) ptr) |= F_SYMBOLIC;
	symptr->ptr = ptr;
	symptr->nsyms = nsyms;
	symptr->isflag = isflag;
	symptr->syms = (struct symarray *) malloc(nsyms * sizeof(struct symarray));
	memcpy(symptr->syms, syms, nsyms * sizeof(struct symarray));
}
