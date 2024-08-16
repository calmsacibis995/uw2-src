/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/define.c	1.2"
#undef printf

#include <stdio.h>
#include "xksh.h" /* which includes sys/types.h */
#include <sys/param.h>
#include <string.h>
#include <varargs.h>
#include <search.h>
#include <ctype.h>

static struct symarray *Dyndef = NULL;
static int Ndyndef = 0;
static int Sdyndef = 0;

const static char use[] = "0x%x";
const static char use2[] = "%s=0x%x";

struct deflist {
	char *prefix;
	int size;
	struct symarray *defs;
};

struct deflist *Deflist = NULL;
int Ndeflist;

static
growdef()
{
	int i;

	if (!(Dyndef = (struct symarray *) realloc(Dyndef, (Sdyndef + 20) * sizeof(struct symarray))))
		return(SH_FAIL);
	Deflist->defs = Dyndef;
	memset(((char *) Dyndef) + Sdyndef * sizeof(struct symarray), '\0', 20 * sizeof(struct symarray));
	Sdyndef += 20;
}

do_define(argc, argv)
int argc;
char **argv;
{
	int i, argstart, redo;
	char *name;
	struct symarray *found, dummy;

	if (C_PAIR(argv[1], '-', 'R')) {
		redo = 0;
		argstart = 2;
	}
	else {
		argstart = 1;
		redo = 1;
	}
	if (!argv[argstart] || !argv[argstart + 1]) {
		altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		XK_USAGE(argv[0]);
	}
	name = argv[argstart++];
	dummy.str = name;
	found = (struct symarray *) bsearch((char *) &dummy, Dyndef, Ndyndef, sizeof(struct symarray), symcomp);
	if (found) {
		if (!redo)
			return(SH_SUCC);
		i = found - Dyndef;
	}
	else {
		if (Sdyndef == Ndyndef)
			growdef();
		Ndyndef++;
		if (Ndyndef > 1)
			for (i = Ndyndef - 1; i > 0; i--) {
				if (strcmp(name, Dyndef[i - 1].str) >= 0)
					break;
				Dyndef[i] = Dyndef[i - 1];
			}
		else
			i = 0;
		Dyndef[i].str = strdup(name);
		Deflist->size++;
	}
	name = argv[argstart];
	RIF(new_par_int(&name, &Dyndef[i].addr));
	return(SH_SUCC);
}

fdef(str, val)
char *str;
ulong *val;
{
	struct symarray *found, dummy;
	int i;

	dummy.str = str;
	if (!Deflist)
		return(0);
	for (i = 0; i < Ndeflist; i++) {
		if (Deflist[i].defs) {
			if (Deflist[i].size < 0)
				found = (struct symarray *) lfind((char *) &dummy, Deflist[i].defs, (unsigned int *) &Deflist[i].size, sizeof(struct symarray), symcomp);
			else
				found = (struct symarray *) bsearch((char *) &dummy, Deflist[i].defs, Deflist[i].size, sizeof(struct symarray), symcomp);
			if (found != NULL) {
				*val = found->addr;
				return(1);
			}
		}
	}
	return(0);
}

do_deflist(argc, argv)
int argc;
char **argv;
{
	int i, j;
	char *prefix = NULL;
	struct symarray *defptr;

	for (i = 1; argv[i]; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j]; j++) {
				switch(argv[i][j]) {
				case 'p':
					if (argv[i][j + 1]) {
						prefix = argv[i] + j;
						j += strlen(prefix) - 2;
					}
					else {
						prefix = argv[++i];
						j = strlen(prefix) - 1;
					}
				}
			}
		}
		else {
			if ((defptr = (struct symarray *) getaddr(argv[i])) == NULL) {
				altfprintf(2, "Cannot find %s\n", argv[i]);
				XK_USAGE(argv[0]);
			}
		}
	}
	for (i = 0; i < Ndeflist; i++)
		if ((Deflist[i].defs == defptr) && (!prefix || (strcmp(Deflist[i].prefix, prefix) == 0)))
			return(SH_SUCC);
	return(add_deflist(defptr, prefix));
}

static
add_deflist(defptr, prefix)
struct symarray *defptr;
char *prefix;
{
	int i;

	if (!Deflist)
		Deflist = (struct deflist *) malloc((Ndeflist + 1) * sizeof(struct deflist));
	else
		Deflist = (struct deflist *) realloc(Deflist, (Ndeflist + 1) * sizeof(struct deflist));
	if (!Deflist)
		return(SH_FAIL);
	Deflist[Ndeflist].defs = defptr;
	Deflist[Ndeflist].prefix = strdup(prefix);
	if (!defptr[0].str)
		Deflist[Ndeflist].size = 0;
	else {
		for (i = 1; defptr[i].str && defptr[i].str[0]; i++)
			if (symcomp((VOID *) (defptr + i), (VOID *) (defptr + i - 1)) < 0)
				break;
		if (!(defptr[i].str && defptr[i].str[0]))
			Deflist[Ndeflist].size = i;
		else
			Deflist[Ndeflist].size = -1;
	}
	Ndeflist++;
	return(SH_SUCC);
}

do_finddef(argc, argv)
int argc;
char **argv;
{
	ulong found;
	struct symarray dummy;

	if (!argv[1]) {
		altprintf((const char *) "Must give argument to finddef\n");
		XK_USAGE(argv[0]);
	}
	if (fdef(argv[1], &found)) {
		if (argv[2]) {
			char buf[50];

			sprintf(buf, use2, argv[2], found);
			env_set(buf);
		}
		else {
			sprintf(xk_ret_buffer, use, found);
			xk_ret_buf = xk_ret_buffer;
		}
		return(SH_SUCC);
	}
	altfprintf(2, (const char *) "Cannot find %s\n", argv[1]);
	XK_USAGE(argv[0]);
}

def_init()
{
	extern struct symarray basedefs[];

	if (!(Dyndef = (struct symarray *) malloc(20 * sizeof(struct symarray)))) {
		altfputs(2, "Insufficient memory\n");
		exit(1);
	}
	Dyndef[0].str = NULL;
	Sdyndef = 20;
	Ndyndef = 0;
	add_deflist(Dyndef, "dynamic");
	add_deflist(basedefs, "base");
}
