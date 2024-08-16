/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/struct.c	1.5"

#undef printf

#include <stdio.h>
#include "xksh.h" /* which includes sys/types.h */
#include <string.h>
#include <ctype.h>
#include "strparse.h"

int Prfailpoint = 0;

# define max(a,b) 		((a)<(b) ? (b) : (a))

const static char use[] = "0x%x";
const static char use2[] = "%s=0x%x";

static VOID *Hashnams;

static struct strhead **Dynmem = NULL;
static int Ndynmem = 0;
static int Sdynmem = 0;

struct structlist {
	char *prefix;
	int id;
	int size;
	struct strhead **mem;
};

struct structlist *Structlist = NULL;
int Nstructlist;

struct strhead *
list_ffind(valarray, ppval, desc, nvals, assocp)
struct valtype *valarray;
VAL **ppval;
char *desc;
int nvals;
struct assoc_field *assocp;
{
	int i, len;
	VAL *pval;
	struct strhead *strp;

	len = strcspn(desc, "[.");
	for (i = 0; i < nvals; i++) {
		if (valarray[i].type && (strlen(valarray[i].name) == len) && (strncmp(desc, valarray[i].name, len) == 0)) {
			if (desc[len]) {
				pval = &valarray[i].val;
				if (strp = ffind(valarray[i].type, desc + len, &pval, assocp)) {
					*ppval = pval;
					return(strp);
				}
			}
			else {
				*ppval = &valarray[i].val;
				return(valarray[i].type);
			}
		}
	}
	for (i = 0; i < nvals; i++) {
		if ((valarray[i].val == NULL) && ((valarray[i].type->type == TYPE_POINTER) || (valarray[i].type->type == TYPE_DYNARRAY)))
			continue;
		pval = &valarray[i].val;
		if (strp = ffind(valarray[i].type, desc, &pval, assocp)) {
			*ppval = pval;
			return(strp);
		}
	}
	return(NULL);
}

struct strhead *
ffind(strp, fld, pval, assocp)
struct strhead *strp;
char *fld;
VAL *pval;
struct assoc_field *assocp;
{
	char *p, *q, op, *holdname;
	VAL val;
	unsigned int len, sub;

	val = *pval;
	if (!fld || !(*fld))
		return(strp);
	q = fld;
	while (strp && q && *q) {
		p = q;
		if (*q == '.') {
			q++;
			continue;
		}
		if (*q == '[') {
			strp = reduce(strp);
			switch(strp->type) {
			case TYPE_ARRAY:
			case TYPE_DYNARRAY:
			case TYPE_POINTER:
				break;
			default:
				return(NULL);
			}
			q++;
			new_par_int(&q, &sub);
			if (*q != ']')
				return(NULL);
			val = *((VAL *) val);
			strp = reduce(((struct arraystr *) strp)->typeptr);
			val += sub * strp->size;
			q++;
			continue;
		}
		if ((len = strcspn(p, "[.")) < strlen(p)) {
			q = p + len;
			op = *q;
			*q = '\0';
		}
		else
			q = NULL;
		strp = reduce(fld_find(strp, p, &val, assocp, 0));
		if (q) {
			if (op == '.')
				*q++ = op;
			else
				*q = op;
		}
	}
	*pval = val;
	return(strp);
}

/*
** Because type specifiers use pointers to other type specifiers
** we can't free a deallocated type specifier.  At some point,
** reference counts could be used to solve this problem, but since
** we don't expect people to redefine structures very often (there
** is even an option to say "don't redo if present") the memory
** waste is acceptable
*/

static
growmem()
{
	if (!(Dynmem = (struct strhead **) realloc(Dynmem, (Sdynmem + 20) * sizeof(struct strhead *))))
		return(SH_FAIL);
	chg_structlist(Dynmem, DYNMEM_ID);
	memset(((char *) Dynmem) + Sdynmem * sizeof(struct strhead *), '\0', 20 * sizeof(struct strhead *));
	Sdynmem += 20;
}

do_struct(argc, argv)
int argc;
char **argv;
{
	struct structstr *mem;
	struct fieldstr *fldp;
	int i, j, argstart, redo;
	char *name, *fname;
	char *p;

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
	for (i = 0; i < Ndynmem; i++) {
		/* All types in here will have names */
		if (strcmp(name, Dynmem[i]->name) == 0)
			break;
	}
	if (i < Ndynmem)  {
		if (!redo)
			return(SH_SUCC);
	}
	else if (Sdynmem - Ndynmem == 1)
		growmem();
	if (!(mem = (struct structstr *) calloc(1, sizeof(struct structstr))))
		return(SH_FAIL);
	if (!(fldp = (struct fieldstr *) calloc((argc - argstart + 1), sizeof(struct fieldstr))))
		return(SH_FAIL);
	if (i < Ndynmem)
		xkhash_override(Hashnams, name, mem);
	else
		Ndynmem++;
	Dynmem[i] = (struct strhead *) mem;
	mem->type = TYPE_STRUCT;
	mem->name = strdup(name);
	mem->fields = (struct fieldstr *) fldp;
	USENORM(); /* Make sure allocations are permanent */
	for (j = argstart; argv[j]; j++)
		fldp[j - argstart].assoc_field_num = -1;
	for (j = argstart; argv[j]; j++) {
		if (p = strchr(argv[j], ':')) {
			fname = malloc(p - argv[j] + 1);
			strncpy(fname, argv[j], p - argv[j]);
			fname[p - argv[j]] = '\0';
			if (!(fldp[j - argstart].typeptr = parse_decl(p + 1, 0))) {
				altprintf((const char *) "Cannot parse %s\n", p + 1);
				XK_USAGE(argv[0]);
			}
			if (p = strchr(p + 1, ':')) {
				int k;

				p++;
				for (k = argstart; argv[k]; k++)
					if ((strncmp(argv[k], p, strlen(p)) == 0) && !isalnum(argv[k][strlen(p)]))
						break;
				if ((k == j) || !argv[k]) {
					altprintf((const char *) "Cannot find or illegal choice field %s\n", p + 1);
					XK_USAGE(argv[0]);
				}
				fldp[k - argstart].assoc_field_num = j - argstart;
			}
		}
		else {
			fname = strdup(argv[j]);
			fldp[j - argstart].typeptr = T_ulong;
		}
		fldp[j - argstart].name = fname;
		fldp[j - argstart].offset = mem->size;
		mem->size += fldp[j - argstart].typeptr->size;
	}
	return(SH_SUCC);
}

do_union(argc, argv)
int argc;
char **argv;
{
	struct unionstr *mem;
	struct union_fieldstr *fldp;
	int i, j, argstart, redo = 1;
	char *name, *fname;
	char external = 0;
	char *tag = NULL;
	char *p;

	for (argstart = 1; argv[argstart][0] == '-'; argstart++) {
		switch (argv[argstart][1]) {
		case 'R':
			redo = 0;
			break;
		case 'e':
			external = 1;
			break;
		case 't':
			tag = argv[++argstart];
			break;
		default:
			XK_USAGE(argv[0]);
		}
	}
	if (!argv[argstart] || !argv[argstart + 1]) {
		altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		XK_USAGE(argv[0]);
	}
	name = argv[argstart++];
	for (i = 0; i < Ndynmem; i++) {
		/* All types in here will have names */
		if (strcmp(name, Dynmem[i]->name) == 0)
			break;
	}
	if (i < Ndynmem)  {
		if (!redo)
			return(SH_SUCC);
	}
	else if (Sdynmem - Ndynmem == 1)
		growmem();
	if (!(mem = (struct unionstr *) calloc(1, sizeof(struct unionstr))))
		return(SH_FAIL);
	if (!(fldp = (struct union_fieldstr *) calloc((argc - argstart + 1), sizeof(struct union_fieldstr))))
		return(SH_FAIL);
	if (i < Ndynmem)
		xkhash_override(Hashnams, name, mem);
	else
		Ndynmem++;
	Dynmem[i] = (struct strhead *) mem;
	mem->type = TYPE_UNION;
	mem->name = strdup(name);
	mem->fields = (struct union_fieldstr *) fldp;
	USENORM(); /* Make sure allocations are permanent */
	for (j = argstart; argv[j]; j++) {
		if (p = strchr(argv[j], ':')) {
			fname = malloc(p - argv[j] + 1);
			strncpy(fname, argv[j], p - argv[j]);
			fname[p - argv[j]] = '\0';
			if (!(fldp[j - argstart].typeptr = parse_decl(p + 1, 0))) {
				altprintf((const char *) "Cannot parse %s\n", p + 1);
				XK_USAGE(argv[0]);
			}
			if (p = strchr(p + 1, ':')) {
				p++;
				new_par_int(&p, &fldp[j - argstart].tag);
			}
		}
		else {
			fname = strdup(argv[j]);
			fldp[j - argstart].typeptr = T_ulong;
		}
		fldp[j - argstart].name = fname;
		mem->size = max(mem->size, fldp[j - argstart].typeptr->size);
	}
	if (external)
		mem->flags |= F_EXTERNAL_TAG;
	else if (tag) {
		struct strhead *tmp;

		if (!(tmp = ffind(mem, tag, &mem->choice_offset, NULL)))
			return(SH_FAIL);
	}
	else
		mem->choice_size = sizeof(ulong);
	return(SH_SUCC);
}

do_typedef(argc, argv)
int argc;
char **argv;
{
	struct typedefstr *mem;
	int i, redo;
	char *name, *decl;

	if (C_PAIR(argv[1], '-', 'R')) {
		redo = 0;
		name = argv[3];
		decl = argv[2];
	}
	else {
		name = argv[2];
		decl = argv[1];
		redo = 1;
	}
	if (!name || !decl) {
		altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		XK_USAGE(argv[0]);
	}
	for (i = 0; i < Ndynmem; i++)
		if (strcmp(name, Dynmem[i]->name) == 0)
			break;
	if (i < Ndynmem) {
		if (!redo)
			return(SH_SUCC);
	}
	else if (Sdynmem - Ndynmem == 1)
		growmem();
	if (!(mem = (struct typedefstr *) calloc(1, sizeof(struct typedefstr))))
		return(SH_FAIL);
	if (i < Ndynmem)
		xkhash_override(Hashnams, name, mem);
	else
		Ndynmem++;
	Dynmem[i] = (struct strhead *) mem;
	mem->type = TYPE_TYPEDEF;
	mem->typeptr = parse_decl(decl, 0);
	mem->size = mem->typeptr->size;
	mem->name = strdup(name);
	return(SH_SUCC);
}

static char *
endtok(start)
char *start;
{
	while(*start && !isspace(*start) && (*start != ':'))
		start++;
	return(start);
}

struct strhead *
parse_decl(decl, fail_undef)
char *decl;
int fail_undef;
{
	struct strhead *strp;
	char *p, *end;
	char hold;
	int flag = 0, done;

	end = decl;
	do {
		p = end;
		XK_SKIPWHITE(&p);
		end = endtok(p);
		hold = *end;
		*end = '\0';
		done = ((strcmp(p, (const char *) "struct") != 0) &&
			(strcmp(p, (const char *) "const") != 0) &&
			(strcmp(p, (const char *) "unsigned") != 0) &&
			(strcmp(p, (const char *) "signed") != 0) &&
			(strcmp(p, (const char *) "union") != 0));
		*end = hold;
	} while (!done && hold);
	if (!p[0]) {
		if (fail_undef) {
			altprintf((const char *) "Cannot parse %s\n", decl);
			return(NULL);
		}
		altprintf((const char *) "Cannot parse %s, using ulong\n", decl);
		return(T_ulong);
	}
	hold = *end;
	*end = '\0';
	strp = all_tbl_search(p, flag|NOHASH);
	*end = hold;
	if (!strp) {
		if (fail_undef) {
			altprintf((const char *) "Cannot parse %s\n", decl);
			return(NULL);
		}
		altprintf((const char *) "Cannot parse %s, using ulong\n", decl);
		return(T_ulong);
	}
	XK_SKIPWHITE(&end);
	for (p = end; *p; p++, xk_skipwhite(&p)) {
		switch(*p) {
		case ':':
			return(strp);
		case '[':
			{
				ulong nelem = 0;
				struct arraystr *ptype;

				p++;
				new_par_int(&p, &nelem);
				XK_SKIPWHITE(&p);
				if (*p != ']') {
					altprintf((const char *) "Cannot find ]\n");
					return(NULL);
				}
				if (!(ptype = (struct arraystr *) (*Mall_func)(sizeof(struct arraystr))))
					return(NULL);
				if (nelem) {
					ptype->type = TYPE_ARRAY;
					ptype->size = nelem * strp->size;
					ptype->flags = 0;
				}
				else {
					ptype->type = TYPE_DYNARRAY;
					ptype->size = sizeof(VOID *);
					ptype->flags = 0;
				}
				ptype->typeptr = strp;
				strp = (struct strhead *) ptype;
				break;
			}
		case '*':
			strp = makeptr(strp);
			break;
		default:
			altprintf((const char *) "Unrecognized character %c\n", *p);
			return(NULL);
		}
	}
	return(strp);
}

do_structlist(argc, argv)
int argc;
char **argv;
{
	int i, j, id = 0;
	char *prefix = NULL;
	struct strhead **memptr;

	for (i = 1; argv[i]; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j]; j++) {
				switch(argv[i][j]) {
				case 'i':
					if (argv[i][j + 1])
						fdef(argv[i] + j + 1, &id);
					else
						fdef(argv[++i], &id);
					j = strlen(argv[i]) - 1;
					break;
				case 'p':
					if (argv[i][j + 1])
						prefix = argv[i] + j + 1;
					else
						prefix = argv[++i];
					j = strlen(prefix) - 1;
					break;
				default:
					altprintf((const char *) "Illegal option to structlist: %s", argv[i]);
					XK_USAGE(argv[0]);
				}
			}
		}
		else {
			if ((memptr = (struct strhead **) getaddr(argv[i])) == NULL) {
				altfprintf(2, "Cannot find %s\n", argv[i]);
				XK_USAGE(argv[0]);
			}
		}
	}
	for (i = 0; i < Nstructlist; i++)
		if ((Structlist[i].mem == memptr) && (!prefix || (strcmp(Structlist[i].prefix, prefix) == 0)) && (!id || (Structlist[i].id == id)))
			return(SH_SUCC);
	add_structlist(memptr, prefix, id);
}

static
chg_structlist(memptr, id)
struct strhead **memptr;
int id;
{
	int i;

	for (i = 0; i < Nstructlist; i++)
		if (Structlist[i].id == id) {
			Structlist[i].mem = memptr;
			return;
		}
}

add_structlist(memptr, prefix, id)
struct strhead **memptr;
char *prefix;
int id;
{
	int i;

	if (!Structlist)
		Structlist = (struct structlist *) malloc((Nstructlist + 1) * sizeof(struct structlist));
	else
		Structlist = (struct structlist *) realloc(Structlist, (Nstructlist + 1) * sizeof(struct structlist));
	if (!Structlist)
		return(SH_FAIL);
	Structlist[Nstructlist].mem = memptr;
	Structlist[Nstructlist].id = id;
	Structlist[Nstructlist].prefix = strdup(prefix);
	if (memptr[0]) {
		for (i = 1; memptr[i]; i++) {
			switch(memptr[i]->type) {
			case TYPE_ARRAY:
			case TYPE_DYNARRAY:
			case TYPE_POINTER:
				altprintf((const char *) "All entries in a structlist must have a name field\n");
				XK_USAGE((const char *) "structlist");
			}
			if (strcmp(memptr[i]->name, memptr[i - 1]->name) < 0)
				break;
		}
		if (!memptr[i])
			Structlist[Nstructlist].size = i - 1;
		else
			Structlist[Nstructlist].size = -1;
	}
	else
		Structlist[Nstructlist].size = 0;
	Nstructlist++;
	return(SH_SUCC);
}

do_sizeof(argc, argv)
int argc;
char **argv;
{
	struct strhead *strp;
	int i;

	i = 1;
	if (!argv[i]) {
		altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		XK_USAGE(argv[0]);
	}
	if ((strp = all_tbl_search(argv[i], 0)) == NULL) {
		altprintf((const char *) "Cannot find %s\n", argv[i]);
		XK_USAGE(argv[0]);
	}
	if (argv[++i]) {
		char buf[50];

		sprintf(buf, use2, argv[i], strp->size);
		env_set(buf);
	}
	else {
		sprintf(xk_ret_buffer, use, strp->size);
		xk_ret_buf = xk_ret_buffer;
	}
	return(SH_SUCC);
}

struct strhead *
all_tbl_search(name, flag)
char *name;
int flag;
{
	register int i;
	VOID *found;

	if (found = (VOID *) xkhash_find(Hashnams, name))
		return((struct strhead *) found);
	else {
		register int j;
		register struct strhead **subtbl;

		for (i = 0; i < Nstructlist; i++) {
			if (subtbl = Structlist[i].mem) {
				for (j = 0; subtbl[j]; j++) {
					if (strcmp(name, subtbl[j]->name) == 0) {
						if (!(flag & NOHASH))
							xkhash_add(Hashnams, name, subtbl[j]);
						return(subtbl[j]);
					}
				}
			}
		}
	}
	return(NULL);
}
/*
** 
**
**
*/

struct strhead *
fld_find(strp, fld, pval, assocp, fld_ok)
struct strhead *strp;
char *fld;
VAL *pval;
struct assoc_field *assocp;
int fld_ok;
{
	int i;

	switch(strp->type) {
	case TYPE_ARRAY:
		return(fld_find(((struct arraystr *) strp)->typeptr, fld, pval, assocp, fld_ok));
	case TYPE_DYNARRAY:
	case TYPE_POINTER:
		{
			if (pval) {
				if (!*pval)
					return(NULL);
				*((VAL *) pval) = **((VAL **) pval);
			}
			return(fld_find(((struct ptrstr *) strp)->typeptr, fld, pval, assocp, fld_ok));
		}
	}
	/* We have something with a name */
	if (strcmp(strp->name, fld) == 0)
		return(strp);
	switch(strp->type) {
	case TYPE_TYPEDEF:
		return(fld_find(((struct typedefstr *) strp)->typeptr, fld, pval, assocp, fld_ok));
	case TYPE_INT:
		return(NULL);
	case TYPE_UNION:
		{
			struct union_fieldstr *fldp = ((struct unionstr *) strp)->fields;
			int i;
			struct strhead *ret;

			for (i = 0; fldp[i].name; i++) {
				if (strcmp(fldp[i].name, fld) == 0)
					return(fld_ok ? (struct strhead *) (fldp + i) : fldp[i].typeptr);
			}
			for (i = 0; fldp[i].name; i++) {
				if (ret = fld_find(fldp[i].typeptr, fld, pval, assocp, fld_ok))
					return(ret);
			}
			return(NULL);
		}
	case TYPE_STRUCT:
		{
			struct fieldstr *fldp = ((struct structstr *) strp)->fields;
			int i;
			struct strhead *ret;

			for (i = 0; fldp[i].name; i++) {
				if (strcmp(fldp[i].name, fld) == 0) {
					if (pval)
						*pval += fldp[i].offset;
					if (assocp) {
						if (fldp[i].assoc_field_num >= 0) {
							assocp->type = fldp[fldp[i].assoc_field_num].typeptr;
							assocp->pval = (VAL *) ((unchar *) pval) + fldp[fldp[i].assoc_field_num].offset;
						}
					}
					return(fld_ok ? (struct strhead *) (fldp + i) : fldp[i].typeptr);
				}
			}
			for (i = 0; fldp[i].name; i++) {
				if (pval) {
					/*
					** If there is a pointer it must not be NULL if
					** the field is a pointer field
					*/
					if (!*pval) {
						switch(fldp[i].typeptr->type) {
						case TYPE_POINTER:
						case TYPE_DYNARRAY:
							continue;
						}
					}
					*pval += fldp[i].offset;
				}
				if (ret = fld_find(fldp[i].typeptr, fld, pval, assocp, fld_ok))
					return(ret);
				if (pval)
					*pval -= fldp[i].offset;
			}
			return(NULL);
		}
	}
}

struct_init()
{
	extern struct strhead *basemems[];

	Hashnams = (VOID *) gettree(50);
	if (!(Dynmem = (struct strhead **) malloc(20 * sizeof(struct strhead *)))) {
		altfputs(2, "Insufficient memory\n");
		exit(1);
	}
	Dynmem[0] = NULL;
	Sdynmem = 20;
	Ndynmem = 0;
	add_structlist(basemems, "base", BASE_ID);
	add_structlist(Dynmem, "dynamic", DYNMEM_ID);
}

struct strhead *
makeptr(strp)
struct strhead *strp;
{
	struct ptrstr *ptype;

	if (!(ptype = (struct ptrstr *) (*Mall_func)(sizeof(struct ptrstr))))
		return(NULL);
	ptype->type = TYPE_POINTER;
	ptype->size = sizeof(VOID *);
	ptype->flags = 0;
	ptype->typeptr = strp;
	return((struct strhead *) ptype);
}

struct strhead *
reduce(strp)
struct strhead *strp;
{
	while (strp && (strp->type == TYPE_TYPEDEF))
		strp = ((struct typedefstr *) strp)->typeptr;
	return(strp);
}

fld_set(valarray, desc, nvals)
struct valtype *valarray;
char *desc;
int nvals;
{
	VAL *pval;
	struct strhead *strp;
	char *valstr;
	char op;
	char field[80];
	struct assoc_field assoc;

	assoc.pval = NULL;
	assoc.type = NULL;
	if ((valstr = strchr(desc, '=')) == NULL)
		return(SH_FAIL);
	if (ispunct(valstr[-1]) && (valstr[-1] != ']')) {
		op = valstr[-1];
		strncpy(field, desc, valstr - desc - 1);
		field[valstr - desc - 1] = '\0';
		valstr++;
	}
	else {
		op = '\0';
		strncpy(field, desc, valstr - desc);
		field[valstr - desc] = '\0';
		valstr++;
	}
	if (!(strp = reduce(list_ffind(valarray, &pval, field, nvals, &assoc))))
		return(SH_FAIL);

	if (!op) {
		char *tmpstr = valstr;

		/* Leave this as type pointer if it is one.  Make the user
		** say p[0] if they want to change the contents of the pointer.
		*/
		if (new_parse(strp, &valstr, pval, &assoc) == SH_FAIL)
			altprintf((const char *) "Cannot set value: %s\n", tmpstr);
	}
	else {
		unsigned long newval, intval;

		while ((strp->type == TYPE_POINTER) && *pval) {
			pval = (VAL *) *pval;
			strp = reduce(((struct ptrstr *) strp)->typeptr);
		}
		if (strp->type != TYPE_INT) {
			altprintf((const char *) "Operations cannot be run on unset or complex types\n");
			return(SH_FAIL);
		}
		new_par_int(&valstr, &newval);
		get_int(strp->size, pval, &intval);
		switch(op) {
		case '+':
			intval += newval;
			break;
		case '-':
			intval -= newval;
			break;
		case '*':
			intval *= newval;
			break;
		case '/':
			intval /= newval;
			break;
		case '%':
			intval %= newval;
			break;
		case '&':
			intval &= newval;
			break;
		case '|':
			intval |= newval;
			break;
		case '^':
			intval ^= newval;
			break;
		}
		set_int(strp->size, pval, intval);
	}
	return(SH_SUCC);
}

xk_skipwhite(pbuf)
char **pbuf;
{
	XK_SKIPWHITE(pbuf);
}
