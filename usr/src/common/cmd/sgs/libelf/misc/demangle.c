/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:misc/demangle.c	1.1.3.6"
/*******************************************************************************
C++ source for the C++ Language System, Release 3.0.  This product
is a new release of the original cfront developed in the computer
science research center of AT&T Bell Laboratories.

Copyright (c) 1993  UNIX System Laboratories, Inc.
Copyright (c) 1991, 1992 AT&T and UNIX System Laboratories, Inc.
Copyright (c) 1984, 1989, 1990 AT&T.  All Rights Reserved.

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE of AT&T and UNIX System
Laboratories, Inc.  The copyright notice above does not evidence
any actual or intended publication of such source code.

*******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct DEMARG DEMARG;
typedef struct DEMCL DEMCL;
typedef struct DEM DEM;

enum DEM_TYPE {
	DEM_NONE,		/* placeholder */
	DEM_STI,		/* static construction function */
	DEM_STD,		/* static destruction function */
	DEM_VTBL,		/* virtual table */
	DEM_PTBL,		/* ptbl vector */
	DEM_FUNC,		/* function */
	DEM_MFUNC,		/* member function */
	DEM_SMFUNC,		/* static member function */
	DEM_CMFUNC,		/* const member function */
	DEM_CVMFUNC,		/* const volatile member function */
	DEM_VMFUNC,		/* volatile member function */
	DEM_OMFUNC,		/* conversion operator member function */
	DEM_CTOR,		/* constructor */
	DEM_DTOR,		/* destructor */
	DEM_DATA,		/* data */
	DEM_MDATA,		/* member data */
	DEM_LOCAL,		/* local variable */
	DEM_CTYPE,		/* class type */
	DEM_TTYPE		/* template class type */
};

enum TI_TYPE {
	TI_NONE,
	TI_CBI,			/* Can Be Instantiated */
	TI_DNI,			/* Do Not Instantiate */
	TI_TIR,			/* Template Instantiation Request */
	TI_TID,			/* Type Identifier */
	TI_T,			/* Type info */
};

struct DEMARG {
	char* mods;		/* modifiers and declarators (page 123 in */
				/* ARM), e.g. "CP" */

	long* arr;		/* dimension if mod[i] == 'A' else NULL */

	DEMARG* func;		/* list of arguments if base == 'F' */
				/* else NULL */

	DEMARG* ret;		/* return type if base == 'F' else NULL */

	DEMCL* clname;		/* class/enum name if base == "C" */

	DEMCL** mname;		/* class name if mod[i] == "M" */
				/* in argument list (pointers to members) */

	DEMARG* next;		/* next argument or NULL */

	char* lit;		/* literal value for PT arguments */
				/* e.g. "59" in A<59> */

	char base;		/* base type of argument, */
				/* 'C' for class/enum types */
};

struct DEMCL {
	char* name;		/* name of class or enum without PT args */
				/* e.g. "Vector" */

	DEMARG* clargs;		/* arguments to class, NULL if not PT */

	char* rname;		/* raw class name with __pt__ if PT */
				/* e.g. "A__pt__2_i" */

	DEMCL* next;		/* next class name, NULL if not nested */
};

struct DEM {
	char* f;		/* function or data name;  NULL if type name */
				/* see page 125 of ARM for predefined list */

	char* vtname;		/* if != NULL name of source file for vtbl */

	DEMARG* fargs;		/* arguments of function name if __opargs__ */
				/* else NULL */

	DEMCL* cl;		/* name of relevant class or enum or NULL */
				/* used also for type-name-only input */

	DEMARG* args;		/* args to function, NULL if data or type */

	enum DEM_TYPE type;	/* type of name that was demangled */

	short slev;		/* scope level for local variables or -1 */

	char sc;		/* storage class type 'S' or 'C' or 'V' or: */
				/* i -> __sti   d --> __std */
				/* b -> __ptbl_vec   x -> const volatile */
	char ti_type;		/* template instantiation control variable */
};


/************************* CUSTOMIZATION SECTION *************************/

#if vax || tahoe || (sun && !(svr4 || i386)) || pyr
#define CLIP_UNDER			/* ignore first "_" on names */
#endif

#define SP_ALIGN 0x4			/* alignment of dynamic space blocks */

/*#define DEM_MAIN*/			/* set if want standalone program */

/************************************************************************/

/* fast string compare */

#define STRCMP(s, t) ((s)[0] != (t)[0] || strcmp((s), (t)) != 0)

/* Variables for getting space for internal data structure. */

static char* spbase;
static int splen;

/* Variables and routines for scanning input.  cc is the current
character, while base[0] is the lookahead character and baselen the
max length of input. */

static char cc;
static const char* base;
static int baselen;
#define gc() {cc = baselen >= 1 ? *base++ : 0, baselen--;}

static int waserror = 0;
#define ERROR() {waserror = 1; return NULL;}

/************************* UTILITIES *************************/

/* Routines for dynamic strings */

struct Dynspace {
	char* ptr;
	int len;
	int maxlen;
};
typedef struct Dynspace Dynspace;

static char* dummy = "";

static void space_init(Dynspace* p)
{
	p->ptr = dummy;
	p->len = 0;
	p->maxlen = 0;
}

static void space_delete(Dynspace* p)
{
	if (p->ptr != dummy)
		free(p->ptr);
	space_init(p);
}

static int space_addchar(Dynspace* p, char c)
{
	char* s;

	if (p->len + 1 >= p->maxlen) {
		p->maxlen = (p->maxlen ? p->maxlen * 2 : 64);
		s = malloc(p->maxlen);
		if (!s)
			return -1;
		if (p->ptr != dummy) {
			strcpy(s, p->ptr);
			free(p->ptr);
		}
		p->ptr = s;
	}

	p->ptr[p->len++] = c;
	p->ptr[p->len] = 0;

	return 0;
}

static int space_cat1(Dynspace* p, char* s)
{
	if (!s)
		return -1;

	while (*s) {
		if (space_addchar(p, *s))
			return -1;
		s++;
	}

	return 0;
}

static int space_cat2(Dynspace* p1, Dynspace* p2)
{
	return space_cat1(p1, p2->ptr);
}

/* get space for internal data structure and align it */

static char* gs(int s)
{
	char* p;

	if (s < 1)
		return 0;

	/* align space on SP_ALIGN boundary */

	while ((unsigned long)spbase & (SP_ALIGN - 1))
		spbase++, splen--;

	p = spbase;
	spbase += s;
	splen -= s;

	if (splen < 0)
		return 0;

	return p;
}

/* copy a string */

static char* copy(const char* s)
{
	char* p;

	if (s == NULL)
		return 0;

	p = gs(strlen(s) + 1);
	if (!p)
		return 0;
	strcpy(p, s);
	return p;
}

/************************* DEMANGLER *************************/


typedef struct {
	enum TI_TYPE type;
	char *prefix;
} ti_prefixes_t;

static ti_prefixes_t ti_prefixes[] = {
	{ TI_CBI,	"__CBI__" },
	{ TI_DNI,	"__DNI__" },
	{ TI_TIR,	"__TIR__" },
	{ TI_TID,	"__TID__" },
	{ TI_T,		"____T__" },
	0
};

static DEMARG* getarglist();
static int dem_printcl(DEMCL*, Dynspace*);
static char *get_hoisted_name();

/* get a class name; assumes that first character indicating a class
is present */

static DEMCL* getclass()
{
	int n;
	Dynspace nbuf;
	int i;
	int j;
	int iter;
	DEMCL* p;
	DEMCL* clhead;
	DEMCL* curr;
	DEMARG* ap;
	char *hoisted_name;
	const char* base_save;
	int baselen_save;
	char cc_save;

	space_init(&nbuf);
	iter = 1;
	clhead = NULL;
	curr = NULL;

	/* fix for ambiguity in encoding, either "Q" or "<len>Q" */

	i = 0;
	if (isdigit(base[0])) {
		i = 1;
		if (isdigit(base[1]))
			i = 2;
	}
	if (isdigit(cc) && base[i] == 'Q' && isdigit(base[i + 1]) &&
	    base[i + 2] == '_') {
		gc();
		if (i)
			gc();
		if (i == 2)
			gc();
	}

	/* might be nested class */

	if (cc == 'Q') {
		gc();
		if (!isdigit(cc))
			goto errexit;
		iter = cc - '0';
		if (iter < 1)
			goto errexit;
		gc();
		if (cc != '_')
			goto errexit;
		gc();
	}

	/* grab number of classes expected */

	while (iter-- > 0) {

		/* get a class */

		if (!isdigit(cc))
			goto errexit;
		n = cc - '0';
		gc();
		if (isdigit(cc)) {
			n = n * 10 + cc - '0';
			gc();
		}
		if (isdigit(cc)) {	/* assumes max length of 3 digits */
			n = n * 10 + cc - '0';
			gc();
		}
		if (n < 1)
			goto errexit;
		space_init(&nbuf);
		for (i = 0; i < n; i++) {
			if (!isalnum(cc) && cc != '_')
				goto errexit;
			if (space_addchar(&nbuf, cc))
				goto errexit;
			gc();
		}
		p = (DEMCL*)gs(sizeof(DEMCL));
		if (!p)
			goto errexit;
		p->rname = copy(nbuf.ptr);
		if (!p->rname)
			goto errexit;
		p->clargs = NULL;

		/* might be a template class with __pt */

		for (j = 0; j < i; j++) {
			if (nbuf.ptr[j] == '_' && nbuf.ptr[j + 1] == '_' &&
			    nbuf.ptr[j + 2] == 'p' && nbuf.ptr[j + 3] == 't')
				break;
		}
		if (j == 0)
			goto errexit;
		if (j == i) {
			p->name = copy(nbuf.ptr);
			if (!p->name)
				goto errexit;
		}
		else {
			if (nbuf.ptr[j + 4] != '_' || nbuf.ptr[j + 5] != '_')
				goto errexit;

			/* found __pt__ -- template class */

			nbuf.ptr[j] = 0;
			p->name = copy(nbuf.ptr);
			if (!p->name)
				goto errexit;
			j += 6;
			if (!isdigit(nbuf.ptr[j]))
				goto errexit;
			n = nbuf.ptr[j] - '0';
			j++;
			if (isdigit(nbuf.ptr[j])) {
				n = n * 10 + nbuf.ptr[j] - '0';
				j++;
			}
			if (isdigit(nbuf.ptr[j])) { /* assumes max 3 digits */
				n = n * 10 + nbuf.ptr[j] - '0';
				j++;
			}
			if (n < 2)
				goto errexit;
			if (nbuf.ptr[j] != '_')
				goto errexit;
			j++;
			n--;
			if (!nbuf.ptr[j])
				goto errexit;

			/* get arguments for template class */

			{
				/* save input context */

				base_save = base;
				baselen_save = baselen;
				cc_save = cc;
				base = nbuf.ptr + j;
				baselen = n;
				gc();
				if ((ap = getarglist()) == NULL || cc)
					goto errexit;

				/* restore input context */

				cc = cc_save;
				baselen = baselen_save;
				base = base_save;
				p->clargs = ap;
			}
		}

		/* may have entity promoted out of a function */
		/* have to parse the function name, looking for something  of the */
		/* form name__function__Lnn_nn, to get the simple type name */
		base_save = base;
		baselen_save = baselen;
		cc_save = cc;
		base = p->name;
		baselen = strlen(base);
		gc();
		if ((hoisted_name = get_hoisted_name()) != 0) {
			p->name = hoisted_name;
		}
		/* restore input context */
		cc = cc_save;
		baselen = baselen_save;
		base = base_save;

		p->next = NULL;

		/* link in to list */

		if (clhead != NULL) {
			curr->next = p;
			curr = p;
		}
		else {
			clhead = p;
			curr = clhead;
		}

		space_delete(&nbuf);
	}

	return clhead;

errexit:

	space_delete(&nbuf);
	ERROR();
}

/* copy an argument */

static DEMARG* arg_copy(DEMARG* p)
{
	DEMARG* p2;

	if (p == NULL)
		return 0;

	p2 = (DEMARG*)gs(sizeof(DEMARG));
	if (!p2)
		return 0;
	p2->mods = p->mods;		/* maybe do struct copy here? */
	p2->base = p->base;
	p2->arr = p->arr;
	p2->func = p->func;
	p2->clname = p->clname;
	p2->mname = p->mname;
	p2->lit = p->lit;
	p2->ret = p->ret;
	p2->next = NULL;

	return p2;
}

/* get an argument */

static DEMARG* getarg(int acmax, DEMARG** arg_cache, int* ncount)
{
	char mods[1000];
	long arrdim[1000];
	DEMCL* clist[1000];
	int mc;
	int type;
	static DEMARG* p;
	DEMCL* clp;
	long n;
	DEMARG* farg;
	DEMARG* fret;
	Dynspace litbuf;
	int lp;
	int foundx;
	int arrp;
	int i;
	int wasm;
	int waslm;
	int clc;
	int ic;

	space_init(&litbuf);

	/* might be stuff remaining from Nnn -- see getarglist() */

	if (ncount != NULL && *ncount > 0) {
		DEMARG* x;
		(*ncount)--;
		x = arg_copy(p);
		if (!x)
			goto errexit;
		space_delete(&litbuf);
		return x;
	}

	mc = 0;
	type = 0;
	clp = NULL;
	farg = NULL;
	fret = NULL;
	lp = 0;
	foundx = 0;
	arrp = 0;
	wasm = 0;
	clc = 0;

	/* get type */

	while (!type) {
		switch (cc) {

			/* modifiers and declarators */

			case 'X':
				gc();
				foundx = 1;
				break;
			case 'U':
			case 'C':
			case 'V':
			case 'S':
			case 'P':
			case 'R':
				if (cc == 'C' && base[0] == 'V')
					cc = 'x'; /* const volatile */
				mods[mc++] = cc;
				if (cc == 'x')
					gc();
				gc();
				break;

			/* fundamental types */

			case 'v':
			case 'c':
			case 's':
			case 'i':
			case 'l':
			case 'f':
			case 'd':
			case 'r':
			case 'e':
				type = cc;
				gc();
				break;

			/* arrays */

			case 'A':
				mods[mc++] = cc;
				gc();
				if (!isdigit(cc))
					goto errexit;
				n = cc - '0';
				gc();
				while (isdigit(cc)) {
					n = n * 10 + cc - '0';
					gc();
				}
				if (cc != '_')
					goto errexit;
				gc();
				arrdim[arrp++] = n;
				break;

			/* functions */

			case 'F':
				type = cc;
				gc();
				if ((farg = getarglist()) == NULL)
					goto errexit;
				if (cc != '_')
					goto errexit;
				gc();
				if ((fret = getarg(-1, (DEMARG**)0, (int*)0)) == NULL)
					goto errexit;
				break;

			/* pointers to member */

			case 'M':
				mods[mc++] = cc;
				wasm = 1;
				gc();
				if ((clist[clc++] = getclass()) == NULL)
					goto errexit;
				break;

			/* repeat previous argument */

			case 'T':
				gc();
tcase:
				if (!isdigit(cc))
					goto errexit;
				n = cc - '0';
				gc();
#if 0
/* commented out because of ambiguity in cfront mangling */
				if (isdigit(cc)) {
					n = n * 10 + cc - '0';
					gc();
				}
#endif
				if (n < 1)
					goto errexit;
				if (arg_cache == NULL || n - 1 > acmax)
					goto errexit;
				p = arg_copy(arg_cache[n - 1]);
				if (!p)
					goto errexit;
				space_delete(&litbuf);
				return p;

			/* repeat previous argument N times */

			case 'N':
				gc();
				if (!isdigit(cc))
					goto errexit;
				if (ncount == NULL)
					goto errexit;
				*ncount = cc - '0' - 1;
				if (*ncount < 0)
					goto errexit;
				gc();
				goto tcase;

			/* class, struct, union, enum */

			case '1': case '2': case '3': case '4': case '5':
			case '6': case '7': case '8': case '9': case 'Q':
				if ((clp = getclass()) == NULL)
					goto errexit;
				type = 'C';
				break;

			default:
				space_delete(&litbuf);
				return NULL;
		}
	}

	/* template literals */

	if (type && foundx) {
		n = 0;
		waslm = 0;

		/* pointer to member */

		if (cc == 'L' && base[0] == 'M') {
			gc();
			gc();
			while (cc != '_' && cc)
				gc();
			if (!cc)
				goto errexit;
			gc();
			while (cc != '_' && cc)
				gc();
			if (!cc)
				goto errexit;
			gc();
			n = cc - '0';
			gc();
			if (isdigit(cc)) {
				n = n * 10 + cc - '0';
				gc();
			}
			if (isdigit(cc)) {
				n = n * 10 + cc - '0';
				gc();
			}
			waslm = 1;
		}

		/* other literal */

		else if (cc == 'L') {
			gc();
			if (!isdigit(cc))
				goto errexit;
			n = cc - '0';
			gc();
			if (isdigit(cc) && base[0] == '_') {
				n = n * 10 + cc - '0';
				gc();
				gc();
			}
			if (cc == 'n') {
				gc();
				n--;
				if (space_addchar(&litbuf, '-'))
					goto errexit;	
				lp++;
			}
		}
		else if (cc == '0') {
			n = 1;
		}
		else if (isdigit(cc)) {
			n = cc - '0';
			gc();
			if (isdigit(cc)) {
				n = n * 10 + cc - '0';
				gc();
			}
		}
		else {
			goto errexit;
		}
		if (!n && waslm) {
			if (space_addchar(&litbuf, '0'))
				goto errexit;
			lp = 1;
		}
		else {
			ic = -1;
			while (n-- > 0) {
				if (!isalnum(cc) && cc != '_')
					goto errexit;
				if (space_addchar(&litbuf, cc))
					goto errexit;
				lp++;
				gc();
				if (n > 0 && lp >= 2 &&
				    litbuf.ptr[lp - 1] == '_' && litbuf.ptr[lp - 2] == '_') {
					if ((clist[ic = clc++] = getclass()) == NULL)
						goto errexit;
					litbuf.ptr[lp - 1] = 0;
					litbuf.ptr[lp - 2] = 0;
					lp -= 2;
					break;
				}	
			}
			if ((wasm && waslm) || ic >= 0) {
				Dynspace buf2;
				Dynspace t;
				int i, len;
				space_init(&buf2);
				if (dem_printcl(clist[ic >= 0 ? ic : 0], &buf2) == -1) {
					space_delete(&buf2);
					space_delete(&litbuf);
					return NULL;
				}
				len = strlen(buf2.ptr);
				space_init(&t);
				for (i = 0; i < len; i++) {
					if (space_addchar(&t, buf2.ptr[i])) {
						space_delete(&buf2);
						goto errexit;
					}
				}
				space_delete(&buf2);
				if (space_addchar(&t, ':') || space_addchar(&t, ':'))
					goto errexit;
				for (i = 0; i < litbuf.len; i++)
					if (space_addchar(&t, litbuf.ptr[i]))
						goto errexit;
				free(litbuf.ptr);
				litbuf = t;
				lp = litbuf.len;
			}
		}
	}

	/* put together argument */

	mods[mc] = 0;
	if (litbuf.ptr != dummy)
		litbuf.ptr[lp] = 0;
	p = (DEMARG*)gs(sizeof(DEMARG));
	if (!p)
		goto errexit;
	if (mc) {
		p->mods = copy(mods);
		if (!p->mods)
			goto errexit;
	}
	else {
		p->mods = NULL;
	}
	if (lp) {
		p->lit = copy(litbuf.ptr);
		if (!p->lit)
			goto errexit;
	}
	else {
		p->lit = NULL;
	}
	if (arrp > 0) {
		p->arr = (long*)gs(sizeof(long) * arrp);
		if (!p->arr)
			goto errexit;
		for (i = 0; i < arrp; i++)
			p->arr[i] = arrdim[i];
	}
	else {
		p->arr = NULL;
	}
	p->base = type;
	p->func = farg;
	p->ret = fret;
	p->clname = clp;
	if (clc > 0) {
		p->mname = (DEMCL**)gs(sizeof(DEMCL*) * (clc + 1));
		if (!p->mname)
			goto errexit;
		for (i = 0; i < clc; i++)
			p->mname[i] = clist[i];
		p->mname[clc] = NULL;
	}
	else {
		p->mname = NULL;
	}
	p->next = NULL;

	space_delete(&litbuf);

	return p;

errexit:

	space_delete(&litbuf);
	ERROR();
}

/* get list of arguments */

static DEMARG* getarglist()
{
	DEMARG* p;
	DEMARG* head;
	DEMARG* curr;
	DEMARG** arg_cache;
	int acmax;
	int ncount;
	int ncount_max;
	int i;
	DEMARG** p2;

	head = NULL;
	curr = NULL;

	acmax = -1;
	ncount = 0;
	ncount_max = 0;
	arg_cache = NULL;

	/* this routine sets up the arg cache which is used by T and
	N formats */

	for (;;) {

		/* get the argument */

		p = getarg(acmax, arg_cache, &ncount);

		/* arguments are picked off until there are no more */

		if (p == NULL) {
			if (arg_cache)
				free(arg_cache);
			if (waserror)
				return NULL;
			return head;
		}

		/* cache argument for Tn and Nnn */

		if (acmax + 2 > ncount_max) {
			ncount_max = (ncount_max ? ncount_max * 2 : 16);
			p2 = (DEMARG**)malloc(sizeof(DEMARG*) * ncount_max);
			if (!p2)
				return NULL;
			if (arg_cache) {
				for (i = 0; i <= acmax; i++)
					p2[i] = arg_cache[i];
				free(arg_cache);
			}
			arg_cache = p2;
		}
		arg_cache[++acmax] = p;
		if (curr == NULL) {
			head = p;
			curr = head;
		}
		else {
			curr->next = p;
			curr = p;
		}
	}
}

/* get_function saves the name of the entity (function of data) in p->f,
 * and the function arguments, if it is indeed a function, in p->args.
 * check_args is 0 if the function is called from dem, where the name
 * may be a function or object name.  check_args is set to 1 when get_function
 * is called from get_hoisted_name, where it must check that is really
 * has a valid function name
 */
static int get_function(DEM *p, int check_args)
{
	long n;
	const char* ob;
	Dynspace nbuf;

	/* get function name */

	space_init(&nbuf);
	while (isalnum(cc) || cc == '_') {
		if (space_addchar(&nbuf, cc))
			goto errexit;
		if (!base[0] ||
		    (base[0] == '_' && base[1] == '_' && base[2] != '_')) {
			gc();
			break;
		}
		gc();

		/* conversion operators */

		if (!STRCMP(nbuf.ptr, "__op")) {
			ob = base - 1;
			if ((p->fargs = getarg(-1, (DEMARG**)0, (int*)0)) == NULL)
				goto errexit;
			while (ob < base - 1) {
				if (space_addchar(&nbuf, *ob++))
					goto errexit;
			}
			break;
		}
	}
	if (!isalpha(nbuf.ptr[0]) && nbuf.ptr[0] != '_')
		goto errexit;

	/* pick off delimiter */

	if (cc == '_' && base[0] == '_') {
		gc();
		gc();
		if (!cc)
			goto errexit;
	}

	/* get class name */

	if (isdigit(cc) || cc == 'Q') {
		if ((p->cl = getclass()) == NULL)
			goto errexit;
	}

	/* a function template */

	else if (cc == 'p' && !strncmp(base, "t__F", 4)) {
		gc();
		gc();
		gc();
		gc();
		gc();
		if (!isdigit(cc))
			goto errexit;
		n = cc - '0';
		gc();
		if (isdigit(cc)) {
			n = n * 10 + cc - '0';
			gc();
		}
		if (isdigit(cc)) {
			n = n * 10 + cc - '0';
			gc();
		}
		if (n < 1)
			goto errexit;
		while (n-- > 0) {
			if (!isalnum(cc) && cc != '_')
				goto errexit;
			gc();
		}
		if (cc != '_' || base[0] != '_')
			goto errexit;
		gc();
		gc();
	}

	if (!STRCMP(nbuf.ptr, "__vtbl")) {
		if (cc == '_' && base[0] == '_' && base[1]) {
			p->vtname = copy(base + 1);
			if (!p->vtname)
				goto errexit;
		}
	}

	/* const/static member functions */

	if ((cc == 'C' || cc == 'S' || cc == 'V') && base[0] == 'F') {
		p->sc = cc;
		gc();
	}
	if (cc == 'C' && base[0] == 'V' && base[1] == 'F') {
		p->sc = 'x';
		gc();
		gc();
	}

	/* get arg list for function */
	if (cc == 'F') {
		gc();
		if ((p->args = getarglist()) == NULL || waserror)
			goto errexit;
	}
	else if (check_args) {
		/* no argument list, not a valid function name for a hoisted type */
		goto errexit;
	}

	if ((p->f = copy(nbuf.ptr)) == NULL)
		goto errexit;

	space_delete(&nbuf);
	return 1;

errexit:
	space_delete(&nbuf);
	return 0;
}

/* may have entity promoted out of a function -  name__function__Lnn_nn
 * get_hoisted_name returns the simple (without __function__Lnn_nn)
 * name of the entity
 */
static char *get_hoisted_name()
{
	const char *t = base;
	const char *save = base - 1;
	char save_cc = cc;

	/* find 2 consecutive underscores */
	while (t[0] && !(t[0] == '_' && t[1] == '_'))
		t++;
	if (t[0] == '_' && t[1] == '_' && t[2]) {
		DEM *p2;

		base = t + 2;
		baselen = strlen(base);
		gc();

		p2 = (DEM *)gs(sizeof(DEM));
		if (!p2)
			return 0;

		/* check for a valid function name with arguments */
		if (get_function(p2, 1)) {
			const char *t2 = base;
			size_t len = t - save;
			char *name;

			/* check for __Lnn_nn */
			if (cc != '_' || *t2++ != '_'
				|| *t2++ != 'L' || !isdigit(*t2++))
				return 0;
			while (isdigit(*t2))
				t2++;
			if (*t2++ != '_')
				return 0;
			while (*t2) {
				if (!isdigit(*t2++))
					return 0;
			}

			if ((name = gs(len+1)) == 0)
				return 0;
			strncpy(name, save, len);
			name[len] = '\0';
			return name;
		}
	}
	return 0;
}

/* entry point for demangling */

static int dem(const char* s, DEM* p, char* buf, size_t maxlen)
{
	Dynspace nbuf;
	Dynspace buf2;
	long n;
	const char* t;
	const char* t2;
	char* t3;
	char* t4;
	int flag;
	int cuflag;
	int found_ln_prefix;
	enum DEM_TYPE dt;
	char *hoisted_name;

	space_init(&nbuf);
	space_init(&buf2);

	if (s == NULL || !*s || p == NULL || buf == NULL || maxlen < 1)
		goto errexit;

	cuflag = 0;
	found_ln_prefix = 0;

	/* cuflag is needed because not all names have _ before them
	even on machines where _ is added;  e.g. internal names */

#ifdef CLIP_UNDER
	if (*s == '_')
		s++, cuflag = 1;
#endif
	if (!*s)
		goto errexit;

	/* set up space and input buffer management */

	spbase = buf;
	splen = maxlen;
	waserror = 0;

	p->fargs = NULL;
	p->cl = NULL;
	p->sc = 0;
	p->args = NULL;
	p->f = NULL;
	p->vtname = NULL;
	p->slev = -1;
	p->type = DEM_NONE;
	p->ti_type = TI_NONE;

	/* handle __nn_nn_ prefix */

	if (cuflag)
		s--;
	if (s[0] == '_' && s[1] == '_' && isdigit(s[2])) {
		t = s + 2;
		while (isdigit(*t))
			t++;
		if (t[0] == '_' && isdigit(t[1])) {
			t++;
			while (isdigit(*t))
				t++;
			if (t[0] == '_' && t[1]) {
				s = t + 1;
				found_ln_prefix = 1;
			}
		}
	}
	if (cuflag && !found_ln_prefix)
		s++;

	/* special case local variables */

	if (cuflag)
		s--;
	if (!found_ln_prefix && s[0] == '_' && s[1] == '_' && isdigit(s[2])) {
		t = s + 2;
		n = 0;
		while (isdigit(*t)) {
			n = n * 10 + *t - '0';
			t++;
		}
		if (*t) {
			p->f = copy(t);
			if (!p->f)
				goto errexit;
			p->slev = n;
			goto done2;
		}
	}
	if (cuflag)
		s++;

	/* special case sti/std/ptbl */

	if (s[0] == '_' && s[1] == '_' &&
	    (!strncmp(s, "__sti__", 7) ||
	    !strncmp(s, "__std__", 7) ||
	    !strncmp(s, "__ptbl_vec__", 12))) {
		p->sc = s[4];
		t = (s[2] == 's' ? s + 7 : s + 12);
		while (*t == '_')
			t++;
		t4 = copy(t);
		if (!t4)
			goto errexit;
		t3 = t4;

		while (t3[0] &&
		       !( (t3[0] == '_' && t3[1] == 'c' && t3[2] == '_') ||
			  (t3[0] == '_' && t3[1] == 'C' && t3[2] == '_') ||
			  (t3[0] == '_' && t3[1] == 'c' && t3[2] == 'p' && t3[3] == 'p' && t3[4] == '_') ||
			  (t3[0] == '_' && t3[1] == 'C' && t3[2] == 'P' && t3[3] == 'P' && t3[4] == '_') ||
			  (t3[0] == '_' && t3[1] == 'c' && t3[2] == 'x' && t3[3] == 'x' && t3[4] == '_') ||
			  (t3[0] == '_' && t3[1] == 'C' && t3[2] == 'X' && t3[3] == 'X' && t3[4] == '_')))
			t3++;

		*t3 = 0;
		p->f = copy(t4);
		if (!p->f)
			goto errexit;
		cc = 0;
		goto done2;
	}

	/* special case template instantiation control variables */
	if (s[0] == '_' && s[1] == '_') {
		ti_prefixes_t *ti_ptr;
		for (ti_ptr = ti_prefixes; ti_ptr->prefix; ti_ptr++) {
			if (strncmp(s, ti_ptr->prefix, strlen(ti_ptr->prefix)) == 0) {
				s += strlen(ti_ptr->prefix);
				p->ti_type = (char)ti_ptr->type;
				break;
			}
		}
	}

	/* special case type names */

	if (cuflag)
		s--;
	t = s;
	flag = 0;
	while (t[0] && (t[0] != '_' || t == s || t[-1] != '_'))
		t++;

	/* __pt__ -- template class */

	if (t[0] == '_' && t[1] == 'p' && t[2] == 't' &&
	    t[3] == '_' && t[4] == '_')
		flag = 1;
	if (t[0] == '_' && t[1] == '_' && t[2] == 'p' && t[3] == 't' &&
	    t[4] == '_' && t[5] == '_')
		flag = 1;
	if (!flag) {
		t = s;

		/* nested */

		if ((t[0] == '_' && t[1] == '_' && t[2] == 'Q' &&
		    isdigit(t[3]) && t[4] == '_'))
			flag = 2;
	}
	if (flag) {
		waserror = 0;
		if (flag == 1) {

			/* put length on if template class */

			char buf[10];
			int i, len;
			sprintf(buf, "%d", strlen(s));
			len = strlen(buf);
			for (i = 0; i < len; i++)
				if (space_addchar(&buf2, buf[i]))
					goto errexit;
			len = strlen(s);
			for (i = 0; i < len; i++)
				if (space_addchar(&buf2, s[i]))
					goto errexit;
			base = buf2.ptr;
		}
		else {
			base = s + 2;
		}
		baselen = strlen(base);
		gc();
		if ((p->cl = getclass()) == NULL)
			goto errexit;
		cc = 0;
		goto done2;
	}


	if (cuflag)
		s++;

	base = s;
	baselen = strlen(base);
	gc();
	waserror = 0;

	/* may have entity promoted out of a function - name__function__Lscope */
	if (cc != '_' && (hoisted_name = get_hoisted_name()) != 0) {
		DEMCL *clp = (DEMCL *)gs(sizeof(DEMCL));

		clp->next = 0;
		clp->rname = 0;
		clp->clargs = 0;
		clp->name = hoisted_name;
		p->cl = clp;
		goto done2;
	}

	/* reset the context that was altered by the failed call to get_hoisted_name */
	base = s;
	baselen = strlen(base);
	gc();
	waserror = 0;
	get_function(p, 0);

	/* there should not be any characters left, except for a few cases
	 * of stuff that is ignored in vtbl names
	 */
	if ((cc && p->f && STRCMP(p->f, "__vtbl") != 0) || waserror)
		goto errexit;

done2:

	/* figure out type we got */

	dt = DEM_NONE;
	if (p->sc) {
		switch (p->sc) {
			case 'i':
				dt = DEM_STI;
				break;
			case 'd':
				dt = DEM_STD;
				break;
			case 'b':
				dt = DEM_PTBL;
				break;
			case 'C':
				dt = DEM_CMFUNC;
				break;
			case 'x':
				dt = DEM_CVMFUNC;
				break;
			case 'S':
				dt = DEM_SMFUNC;
				break;
			case 'V':
				dt = DEM_VMFUNC;
				break;
			default:
				goto errexit;
				break;
		}
	}
	else if (p->args != NULL) {
		if (p->fargs != NULL) {
			dt = DEM_OMFUNC;
		}
		else if (p->cl != NULL) {
			t3 = p->f;
			if (t3[0] == '_' && t3[1] == '_') {
				if (t3[2] == 'c' && t3[3] == 't' && !t3[4])
					dt = DEM_CTOR;
				else if (t3[2] == 'd' && t3[3] == 't' &&
				    !t3[4])
					dt = DEM_DTOR;
				else
					dt = DEM_MFUNC;
			}
			else {
				dt = DEM_MFUNC;
			}
		}
		else {
			dt = DEM_FUNC;
		}
	}
	else if (p->f == NULL && p->cl != NULL) {
		if (p->cl->clargs != NULL)
			dt = DEM_TTYPE;
		else
			dt = DEM_CTYPE;
	}
	else if (found_ln_prefix || p->slev != -1) {
		dt = DEM_LOCAL;
	}
	else if (p->f != NULL) {
		if (p->cl != NULL) {
			t3 = p->f;
			if (t3[0] == '_' && t3[1] == '_' && t3[2] == 'v' &&
			    t3[3] == 't' && t3[4] == 'b' && t3[5] == 'l' &&
			    !t3[6])
				dt = DEM_VTBL;
			else
				dt = DEM_MDATA;
		}
		else {
			dt = DEM_DATA;
		}
	}

	if (dt == DEM_NONE)
		goto errexit;

	p->type = dt;

	space_delete(&nbuf);
	space_delete(&buf2);

	return 0;

errexit:

	space_delete(&nbuf);
	space_delete(&buf2);

	return (splen < 0 ? -2 : -1);
}

/************************* PRINT AN UNMANGLED NAME *************************/

static int dem_printarglist(DEMARG*, Dynspace*, int);
static int dem_printarg(DEMARG*, Dynspace*, int);

/* format a class name */
static int dem_printcl(DEMCL* p, Dynspace* buf)
{
	int i;
	Dynspace buf2;

	space_init(&buf2);

	if (p == NULL || buf == NULL)
		goto errexit;

	i = 0;
	while (p != NULL) {
		i++;

		/* handle nested */

		if (i > 1)
			if (space_cat1(buf, "::"))
				goto errexit;
		if (space_cat1(buf, p->name))
			goto errexit;

		/* template class */

		if (p->clargs != NULL) {
			if (buf->ptr[strlen(buf->ptr) - 1] == '<')
				if (space_cat1(buf, " "))
					goto errexit;
			if (space_cat1(buf, "<"))
				goto errexit;
			space_delete(&buf2);
			if (dem_printarglist(p->clargs, &buf2, 0) == -1)
				goto errexit;
			if (space_cat2(buf, &buf2))
				goto errexit;
			if (buf->ptr[strlen(buf->ptr) - 1] == '>')
				if (space_cat1(buf, " "))
					goto errexit;
			if (space_cat1(buf, ">"))
				goto errexit;
		}
		p = p->next;
	}

	space_delete(&buf2);
	return 0;

errexit:

	space_delete(&buf2);
	return -1;
}

/* format an argument list */
static int dem_printarglist(DEMARG* p, Dynspace* buf, int sv)
{
	int i;
	Dynspace buf2;

	space_init(&buf2);

	if (p == NULL || buf == NULL || sv < 0 || sv > 1)
		goto errexit;

	/* special case single "..." argument */

	if (p->base == 'v' && p->mods == NULL && p->next != NULL &&
	    p->next->base == 'e' && p->next->next == NULL) {
		if (space_cat1(buf, "..."))
			goto errexit;
		space_delete(&buf2);
		return 0;
	}

	/* special case single "void" argument */

	if (p->base == 'v' && p->mods == NULL) {
		if (space_cat1(buf, "void"))
			goto errexit;
		space_delete(&buf2);
		return 0;
	}

	i = 0;
	while (p != NULL) {
		i++;
		if (i > 1)
			if (space_cat1(buf, p->base == 'e' ? " " : ","))
				goto errexit;
		space_delete(&buf2);
		if (dem_printarg(p, &buf2, sv) == -1)
			goto errexit;
		if (space_cat2(buf, &buf2))
			goto errexit;
		p = p->next;
	}

	space_delete(&buf2);
	return 0;

errexit:

	space_delete(&buf2);
	return -1;
}

/* format a single argument */
static int dem_printarg(DEMARG* p, Dynspace* buf, int f)
{
	Dynspace bufc;
	Dynspace bufc2;
	Dynspace farg;
	Dynspace fret;
	Dynspace pref;
	Dynspace scr;
	Dynspace ptrs;
	char* t;
	char* m;
	char* mm;
	int arrindx;
	long dim;
	int i;
	int sv;
	char* s;
	char* trail;
	int clc;

	space_init(&bufc);
	space_init(&bufc2);
	space_init(&farg);
	space_init(&fret);
	space_init(&pref);
	space_init(&scr);
	space_init(&ptrs);

	if (p == NULL || buf == NULL || f < 0 || f > 1)
		goto errexit;

	/* format the underlying type */

	sv = !f;

	switch (p->base) {

		/* fundamental types */

		case 'v':
			t = "void";
			break;
		case 'c':
			t = "char";
			break;
		case 's':
			t = "short";
			break;
		case 'i':
			t = "int";
			break;
		case 'l':
			t = "long";
			break;
		case 'f':
			t = "float";
			break;
		case 'd':
			t = "double";
			break;
		case 'r':
			t = "long double";
			break;
		case 'e':
			t = "...";
			sv = 1;
			break;

		/* functions */

		case 'F':
			if (dem_printarg(p->ret, &fret, 0) == -1)
				goto errexit;
			if (dem_printarglist(p->func, &farg, 0) == -1)
				goto errexit;
			break;

		/* class, struct, union, enum */

		case 'C':
			if (dem_printcl(p->clname, &bufc) == -1)
				goto errexit;
			t = bufc.ptr;
			break;

		default:
			goto errexit;
			break;
	}

	/* handle modifiers and declarators */

	m = p->mods;
	if (m == NULL)
		m = "";

	/* const, volatile, signed, and unsigned */

	mm = m;
	while (*mm) {
		if ((mm[0] == 'C' || mm[0] == 'V' || mm[0] == 'x') && (mm[1] != 'P' && mm[1] != 'R' && mm[1] != 'M') && (mm[1] || p->base != 'F')) {
			if (space_cat1(&pref, mm[0] == 'C' ? "const " : mm[0] == 'V' ? "volatile " : "const volatile "))
				goto errexit;
			break;
		}
		mm++;
	}
	mm = m;
	while (*mm) {
		if (*mm == 'S' || *mm == 'U') {
			if (space_cat1(&pref, *mm == 'S' ? "signed " :"unsigned "))
				goto errexit;
			break;
		}
		mm++;
	}

	/* go through modifier list */

	mm = m;
	arrindx = 0;
	clc = 0;
	while (*mm) {
		space_delete(&scr);
		space_delete(&bufc2);
		if (mm[0] == 'P') {
			if (space_addchar(&scr, '*') || space_cat2(&scr, &ptrs))
				goto errexit;
			space_delete(&ptrs);
			if (space_cat2(&ptrs, &scr))
				goto errexit;
		}
		else if (mm[0] == 'R') {
			if (space_addchar(&scr, '&') || space_cat2(&scr, &ptrs))
				goto errexit;
			space_delete(&ptrs);
			if (space_cat2(&ptrs, &scr))
				goto errexit;
		}
		else if (mm[0] == 'M') {
			if (dem_printcl(p->mname[clc++], &bufc2) == -1)
				goto errexit;
			if (space_cat2(&scr, &bufc2) ||
			    space_cat1(&scr, "::*") ||
			    space_cat2(&scr, &ptrs))
				goto errexit;
			space_delete(&ptrs);
			if (space_cat2(&ptrs, &scr))
				goto errexit;
		}
		else if ((mm[0] == 'C' || mm[0] == 'V' || mm[0] == 'x') && mm[1] == 'P') {
			if (space_cat1(&scr, " *") || 
			    space_cat1(&scr, mm[0] == 'C' ? "const" : mm[0] == 'V' ? "volatile" : "const volatile") ||
			    space_cat1(&scr, isalnum(ptrs.ptr[0]) || ptrs.ptr[0] == '_' ? " " : "") ||
			    space_cat2(&scr, &ptrs))
				goto errexit;
			space_delete(&ptrs);
			if (space_cat2(&ptrs, &scr))
				goto errexit;
			mm++;
		}
		else if ((mm[0] == 'C' || mm[0] == 'V' || mm[0] == 'x') && mm[1] == 'R') {
			if (space_cat1(&scr, " &") || 
			    space_cat1(&scr, mm[0] == 'C' ? "const" : mm[0] == 'V' ? "volatile" : "const volatile") ||
			    space_cat1(&scr, isalnum(ptrs.ptr[0]) || ptrs.ptr[0] == '_' ? " " : "") ||
			    space_cat2(&scr, &ptrs))
				goto errexit;
			space_delete(&ptrs);
			if (space_cat2(&ptrs, &scr))
				goto errexit;
			mm++;
		}
		else if ((mm[0] == 'C' || mm[0] == 'V' || mm[0] == 'x') && mm[1] == 'M') {
			if (dem_printcl(p->mname[clc++], &bufc2) == -1)
				goto errexit;
			if (space_cat2(&scr, &bufc2) ||
			    space_cat1(&scr, "::*") || 
			    space_cat1(&scr, mm[0] == 'C' ? "const" : mm[0] == 'V' ? "volatile" : "const volatile") ||
			    space_cat1(&scr, isalnum(ptrs.ptr[0]) || ptrs.ptr[0] == '_' ? " " : "") ||
			    space_cat2(&scr, &ptrs))
				goto errexit;
			space_delete(&ptrs);
			if (space_cat2(&ptrs, &scr))
				goto errexit;
			mm++;
		}
		else if (mm[0] == 'A') {
			char xx[10];
			dim = p->arr[arrindx++];
			if (dim > 0) {
				sprintf(xx, "%ld", dim);
			} else {
				/* An undimensioned array. */
				xx[0] = '\0';
			}
			s = sv ? "" : "@";
			if (ptrs.ptr == dummy) {
				if (space_cat1(&scr, s) ||
				    space_addchar(&scr, '[') ||
				    space_cat1(&scr, xx) ||
				    space_addchar(&scr, ']'))
					goto errexit;
				sv = 1;
			}
			else if (ptrs.ptr[0] == '(' || ptrs.ptr[0] == '[') {
				if (space_cat2(&scr, &ptrs) ||
				    space_addchar(&scr, '[') ||
				    space_cat1(&scr, xx) ||
				    space_addchar(&scr, ']'))
					goto errexit;
			}
			else {
				if (space_addchar(&scr, '(') ||
				    space_cat2(&scr, &ptrs) ||
				    space_cat1(&scr, s) ||
				    space_addchar(&scr, ')') ||
				    space_addchar(&scr, '[') ||
				    space_cat1(&scr, xx) ||
				    space_addchar(&scr, ']'))
					goto errexit;
				sv = 1;
			}
			space_delete(&ptrs);
			if (space_cat2(&ptrs, &scr))
				goto errexit;
		}
		else if (mm[0] == 'U' || mm[0] == 'C' || mm[0] == 'V' || mm[0] == 'x' || mm[0] == 'S') {
			/* ignore */
		}
		else {
			goto errexit;
		}
		mm++;
	}

	/* put it together */

	s = sv ? "" : "@";
	if (p->base == 'F') {

		/* function argument */

		char z = 0;
		i = 0;
		if (ptrs.ptr[0] == ' ')
			i = 1;
		trail = "";
		if (p->mods != NULL)
			z = p->mods[strlen(p->mods) - 1];
		trail = z == 'C' ? " const" : z == 'V' ? " volatile" : z == 'x' ? " const volatile" : "";
		if (ptrs.ptr[i]) {
			if (space_cat2(buf, &pref) ||
			    space_cat2(buf, &fret) ||
			    space_cat1(buf, " (") ||
			    space_cat1(buf, ptrs.ptr + i) ||
			    space_cat1(buf, s) ||
			    space_cat1(buf, ")(") ||
			    space_cat2(buf, &farg) ||
			    space_addchar(buf, ')') ||
			    space_cat1(buf, trail))
				goto errexit;
		}
		else {
			if (space_cat2(buf, &pref) ||
			    space_cat2(buf, &fret) ||
			    space_addchar(buf, ' ') ||
			    space_cat1(buf, s) ||
			    space_addchar(buf, '(') ||
			    space_cat2(buf, &farg) ||
			    space_addchar(buf, ')') ||
			    space_cat1(buf, trail))
				goto errexit;
		}
	}
	else {
		if (space_cat2(buf, &pref) ||
		    space_cat1(buf, t) ||
		    space_cat1(buf, ptrs.ptr[0] == '(' || isalnum(ptrs.ptr[0]) || ptrs.ptr[0] == '_' ? " " : "") ||
		    space_cat2(buf, &ptrs) ||
		    space_cat1(buf, s))
			goto errexit;
	}
	if (p->lit != NULL) {

		/* literal */

		space_delete(&scr);
		if (isdigit(p->lit[0]) || p->lit[0] == '-') {
			if (space_addchar(&scr, '(') ||
			    space_cat2(&scr, buf) ||
			    space_addchar(&scr, ')') ||
			    space_cat1(&scr, p->lit))
				goto errexit;
		}
		else {
			if (space_addchar(&scr, '&') ||
			    space_cat1(&scr, p->lit))
				goto errexit;
		}
		space_delete(buf);
		if (space_cat2(buf, &scr))
			goto errexit;
	}

	space_delete(&bufc);
	space_delete(&bufc2);
	space_delete(&farg);
	space_delete(&fret);
	space_delete(&pref);
	space_delete(&scr);
	space_delete(&ptrs);
	return 0;

errexit:

	space_delete(&bufc);
	space_delete(&bufc2);
	space_delete(&farg);
	space_delete(&fret);
	space_delete(&pref);
	space_delete(&scr);
	space_delete(&ptrs);
	return -1;
}

struct Ops {
	char* encode;
	char* name;
};

static const struct Ops ops[] = {
	"__pp",		"operator++",
	"__as",		"operator=",
	"__vc",		"operator[]",
	"__nw",		"operator new",
	"__dl",		"operator delete",
	"__rf",		"operator->",
	"__ml",		"operator*",
	"__mm",		"operator--",
	"__oo",		"operator||",
	"__md",		"operator%",
	"__mi",		"operator-",
	"__rs",		"operator>>",
	"__ne",		"operator!=",
	"__gt",		"operator>",
	"__ge",		"operator>=",
	"__or",		"operator|",
	"__aa",		"operator&&",
	"__nt",		"operator!",
	"__apl",	"operator+=",
	"__amu",	"operator*=",
	"__amd",	"operator%=",
	"__ars",	"operator>>=",
	"__aor",	"operator|=",
	"__cm",		"operator,",
	"__dv",		"operator/",
	"__pl",		"operator+",
	"__ls",		"operator<<",
	"__eq",		"operator==",
	"__lt",		"operator<",
	"__le",		"operator<=",
	"__ad",		"operator&",
	"__er",		"operator^",
	"__co",		"operator~",
	"__ami",	"operator-=",
	"__adv",	"operator/=",
	"__als",	"operator<<=",
	"__aad",	"operator&=",
	"__aer",	"operator^=",
	"__rm",		"operator->*",
	"__cl",		"operator()",
	NULL,		NULL
};

/* format a function name */
static int dem_printfunc(DEM* dp, Dynspace* buf)
{
	int i;
	Dynspace buf2;

	space_init(&buf2);

	if (dp == NULL || buf == NULL)
		goto errexit;

	if (dp->f[0] == '_' && dp->f[1] == '_') {

		/* conversion operators */

		if (!strncmp(dp->f, "__op", 4) && dp->fargs != NULL) {
			if (dem_printarg(dp->fargs, &buf2, 0) == -1)
				goto errexit;
			if (space_cat1(buf, "operator ") ||
			    space_cat2(buf, &buf2))
				goto errexit;
		}

		/* might be overloaded operator */

		else {
			i = 0;
			while (ops[i].encode != NULL && strcmp(ops[i].encode, dp->f))
				i++;
			if (space_cat1(buf, ops[i].encode != NULL ? ops[i].name : dp->f))
				goto errexit;
		}
	}
	else {
		if (space_cat1(buf, dp->f))
			goto errexit;
	}

	space_delete(&buf2);
	return 0;

errexit:

	space_delete(&buf2);
	return -1;
}

/* entry point to formatting functions */
static int dem_print(DEM* p, Dynspace* buf)
{
	Dynspace buf2;
	char* s;
	int t;

	space_init(&buf2);

	if (p == NULL || buf == NULL)
		goto errexit;

	/* template instantiation prefixes */
	if (p->ti_type) {
		if (space_cat1(buf, ti_prefixes[p->ti_type-1].prefix))
			goto errexit;
	}

	/* type names */

	if (p->f == NULL && p->cl != NULL) {
		if (dem_printcl(p->cl, buf) == -1)
			goto errexit;
		space_delete(&buf2);
		return 0;
	}

	/* sti/std */

	if (p->sc == 'i' || p->sc == 'd') {
		if (space_cat1(buf, p->f) || space_cat1(buf, ":__st") ||
		    space_addchar(buf, p->sc))
			goto errexit;
		space_delete(&buf2);
		return 0;
	}
	if (p->sc == 'b') {
		if (space_cat1(buf, p->f) || space_cat1(buf, ":__ptbl_vec"))
			goto errexit;
		space_delete(&buf2);
		return 0;
	}

	/* format class name */

	if (p->cl != NULL) {
		if (dem_printcl(p->cl, &buf2) == -1)
			goto errexit;
		if (space_cat2(buf, &buf2) || space_cat1(buf, "::"))
			goto errexit;
	}

	/* special case constructors and destructors */

	s = buf2.ptr + strlen(buf2.ptr) - 1;
	t = 0;
	while (s >= buf2.ptr) {
		if (*s == '>')
			t++;
		else if (*s == '<')
			t--;
		else if (*s == ':' && !t)
			break;
		s--;
	}
	if (!STRCMP(p->f, "__ct")) {
		if (space_cat1(buf, s + 1))
			goto errexit;
	}
	else if (!STRCMP(p->f, "__dt")) {
		if (space_cat1(buf, "~") || space_cat1(buf, s + 1))
			goto errexit;
	}
	else {
		space_delete(&buf2);
		if (dem_printfunc(p, &buf2) == -1)
			goto errexit;
		if (space_cat2(buf, &buf2))
			goto errexit;
	}

	/* format argument list */

	if (p->args != NULL) {
		if (space_addchar(buf, '('))
			goto errexit;
		space_delete(&buf2);
		if (dem_printarglist(p->args, &buf2, 0) == -1)
			goto errexit;
		if (space_cat2(buf, &buf2) || space_addchar(buf, ')'))
			goto errexit;
	}

	/* const member functions */

	if (p->sc == 'C' || p->sc == 'V' || p->sc == 'x')
		if (space_cat1(buf, p->sc == 'C' ? " const" : p->sc == 'V' ? " volatile": " const volatile"))
			goto errexit;

	space_delete(&buf2);
	return 0;

errexit:

	space_delete(&buf2);
	return -1;
}

/* demangle in --> out */
int demangle(const char* in, char* out, size_t len)
{
	DEM d;
	int n;
	int i;
	char* sbuf;
	Dynspace out2;
	int needed_len;

	space_init(&out2);

	if (in == NULL || !*in || out == NULL || len < 1)
		goto errexit;

	n = 4096;
	for (;;) {
		sbuf = malloc(n);
		if (!sbuf)
			goto errexit;
		i = dem(in, &d, sbuf, n);
		if (i)
			free(sbuf);
		if (i == -1)
			goto errexit;
		if (i != -2)
			break;
		n *= 2;
	}

	i = dem_print(&d, &out2);
	free(sbuf);
	if (i < 0)
		goto errexit;

	needed_len = out2.len + 1;

	if (len >= needed_len)
		strcpy(out, out2.ptr);

	space_delete(&out2);
	return needed_len;

errexit:

	space_delete(&out2);
	return -1;
}
