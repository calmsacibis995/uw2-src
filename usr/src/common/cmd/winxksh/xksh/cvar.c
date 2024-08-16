/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/cvar.c	1.5"

#include <stdio.h>
#include "xksh.h" /* which includes sys/types.h */
#include <sys/param.h>
#include <string.h>
#include <varargs.h>
#include <search.h>
#include <ctype.h>
#include "strparse.h"

static struct valtype *Varray = NULL;
static VOID **Vscopes = NULL;
static unsigned long Nvars = 0;
static unsigned long Svars = 0;

const static char *Alphanum = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const static char *Exprchar = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.[]";

cexprlen(expr)
char *expr;
{
	return(strspn(expr, Exprchar));
}

cvar_get(expr, valp)
char *expr;
struct valtype *valp;
{
	char *p;
	char hold;
	VAL *tmp;

	if (!Varray)
		return(SH_FAIL);
	if (strncmp(expr, "p0x", 3) == 0)
		return(SH_FAIL);
	p = expr + cexprlen(expr);
	hold = *p;
	*p = '\0';
	valp->type = list_ffind(Varray, &tmp, expr, Nvars, NULL);
	*p = hold;
	if (!valp->type)
		return(SH_FAIL);
	if (valp->type->type == TYPE_ARRAY)
		valp->val = (VAL) tmp;
	else
		valp->val = *tmp;
	return(SH_SUCC);
}

do_cprint(argc, argv)
int argc;
char **argv;
{
	int i = 1;
	struct valtype valt;
	struct easyio ez, *ezp;
	struct buf ezbuf;

	if (strcmp(argv[0], "-?") == 0)
		XK_USAGE(argv[0]);
	if (strcmp(argv[i], "-v" ) == 0) {
		i++;
		ez.type = EZ_STR;
		ez.flags = 0;
		ez.fd_or_buf.buf = &ezbuf;
		ezbuf.buf = NULL;
		ezp = &ez;
		outprintf(&ez, "%s=", argv[i++]);
	}
	else
		ezp = NULL;
	for ( ; argv[i]; i++) {
		if (cvar_get(argv[i], &valt) == SH_FAIL) {
			altprintf((const char *) "Cannot find %s\n", argv[i]);
			XK_USAGE(argv[0]);
		}
		if (new_print(valt.type, (valt.type->type == TYPE_ARRAY) ? valt.val : (VAL) &valt.val, ezp, NULL) == SH_FAIL) {
			altprintf((const char *) "Cannot print %s\n", argv[i]);
			XK_USAGE(argv[0]);
		}
		if (ezp) {
			if (argv[i+1])
				outputc(ezp, ' ');
		}
		else
			outputc(ezp, '\n');
	}
	if (ezp) {
		outputc(ezp, '\0');
		env_set(ezbuf.buf);
	}
	return(SH_SUCC);
}

do_cdecl(argc, argv)
int argc;
char **argv;
{
	char always_ptr, buf[BUFSIZ], *type, *p;
	struct strhead *strp;
	int spot;
	unsigned int i;
	VOID *scope = (VOID *) curlev();

	always_ptr = 0;
	for (i = 1; argv[i] && (argv[i][0] == '-'); i++) {
		switch(argv[i][1]) {
		case 'p':
			always_ptr = 1;
			break;
		case 'g':
			scope = NULL;
			break;
		case '?':
			XK_USAGE(argv[0]);
		}
	}
	type = argv[i++];
	USENORM(); /* parse_decl must be permanent */
	if (!type || !(strp = parse_decl(type, 1)) || !argv[i]) {
		if (!type || !argv[i])
			altfprintf(2, (const char *) "Insufficient arguments to %s\n", argv[0]);
		else
			altfprintf(2, (const char *) "Cannot parse %s\n", type);
		XK_USAGE(argv[0]);
	}
	if ((strp->size > sizeof(VAL)) || (always_ptr && (strp->type != TYPE_DYNARRAY) && (strp->type != TYPE_POINTER)))
		strp = makeptr(strp);
	if (Svars < (Nvars + argc - i - 1)) {
		Svars += 10;
		Varray = (struct valtype *) realloc(Varray, sizeof(struct valtype) * Svars);
		Vscopes = (VOID **) realloc(Vscopes, sizeof(VOID *) * Svars);
	}
	for ( ; argv[i]; i++) {
		if (p = strchr(argv[i], '='))
			*p = '\0';
		for (spot = 0; spot < Nvars; spot++) {
			/* if new declaration is global and we find a variable that
			**    matches the name, then we must override it
			** if new declaration is a tighter scope, then it is a new
			**    variable, so it is NOT a match.  But we need to swap
			**    it so that it comes first.
			*/
			if (strcmp(Varray[spot].name, argv[i]) == 0) {
				if (scope && (Vscopes[spot] != scope)) {
					memmove(Varray + spot + 1, Varray + spot, (Nvars - spot) * sizeof(struct valtype));
					memmove(Vscopes + spot + 1, Vscopes + spot, (Nvars - spot) * sizeof(VOID *));
					Varray[spot].name = strdup(argv[i]);
					Varray[spot].val = NULL;
					Nvars++;
				}
				break;
			}
		}
		if (spot == Nvars) {
			Nvars++;
			Varray[spot].name = strdup(argv[i]);
			Varray[spot].val = NULL;
		}
		else
			new_free(Varray[spot].type, &Varray[spot].val, NULL);
		Vscopes[spot] = scope;
		Varray[spot].type = strp;
		if (p) {
			char *hold = p;

			*p++ = '=';
			Cmode = 1;
			if (new_parse(Varray[spot].type, &p, &Varray[spot].val, NULL) == SH_FAIL) {
				altfprintf(2, (const char *) "Cannot figure out %s for %s\n", hold + 1, type);
				XK_USAGE(argv[0]);
			}
			Cmode = 0;
		}
		else
			Varray[spot].val = 0;
	}
	return(SH_SUCC);
}

do_cset(argc, argv)
int argc;
char **argv;
{
	int j, ret;

	if (strcmp(argv[0], "-?") == 0)
		XK_USAGE(argv[0]);
	Cmode = 1;
	for (j = 1; argv[j]; j++)
		if ((ret = fld_set(Varray, argv[j], Nvars)) == SH_FAIL)
			break;
	Cmode = 0;
	if (ret == SH_FAIL) {
		altfputs(2, "Cannot set values");
		XK_USAGE(argv[0]);
	}
	return(SH_SUCC);
}

cvar_unscope(scope)
VOID *scope;
{
	register int i;

	for (i = 0; i < Nvars; ) {
		if (Vscopes[i] == scope) {
			/*int j;*/

			new_free(Varray[i].type, &Varray[i].val, NULL);
			Nvars--;
			memmove(Varray + i, Varray + i + 1, (Nvars - i) * sizeof(struct valtype));
			memmove(Vscopes + i, Vscopes + i + 1, (Nvars - i) * sizeof(VOID *));
			/*for (j = i; j < Nvars; j++) {*/
				/*Varray[j] = Varray[j + 1];*/
				/*Vscopes[j] = Vscopes[j + 1];*/
			/*}*/
		}
		else
			i++;
	}
}

cvar_init()
{
	Svars = 10;
	Varray = (struct valtype *) malloc(sizeof(struct valtype) * Svars);
	Vscopes = (VOID **) malloc(sizeof(VOID *) * Svars);
}
