/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/docall.c	1.4"

#include <stdio.h>
#include "xksh.h" /* which includes sys/types.h */
#include <sys/param.h>
#include <string.h>
#include <varargs.h>
#include <search.h>
#include <ctype.h>
#include "strparse.h"

static long get_prdebug(), set_prdebug();

const static char use[] = "0x%x";
const static char use2[] = "%s=0x%x";

int Xk_errno = 0;

int Xkdebug = 0;

#define CONSTIT(X) ((char *) ((const char *) (X)))

struct valtype Call_valarray[MAX_CALL_ARGS] = {
	{ CONSTIT("1"), NULL, NULL },
	{ CONSTIT("2"), NULL, NULL },
	{ CONSTIT("3"), NULL, NULL },
	{ CONSTIT("4"), NULL, NULL },
	{ CONSTIT("5"), NULL, NULL },
	{ CONSTIT("6"), NULL, NULL },
	{ CONSTIT("7"), NULL, NULL },
	{ CONSTIT("8"), NULL, NULL },
	{ CONSTIT("9"), NULL, NULL },
	{ CONSTIT("10"), NULL, NULL },
	{ CONSTIT("11"), NULL, NULL },
	{ CONSTIT("12"), NULL, NULL },
	{ CONSTIT("13"), NULL, NULL },
	{ CONSTIT("14"), NULL, NULL },
	{ CONSTIT("15"), NULL, NULL }
};

char xk_ret_buffer[100];
char *xk_ret_buf = xk_ret_buffer;
char xk_retd_buffer[100];
char *xk_retd_buf = xk_retd_buffer;
struct Bfunction xk_prdebug = { get_prdebug, set_prdebug };

one_fld_print(strp, val, fld, pout)
struct strhead *strp;
VAL val;
char *fld;
struct easyio *pout;
{
	struct assoc_field assoc;

	if (!C_PAIR(fld, '.', '\0'))
		if (!(strp = ffind(strp, fld, &val, &assoc)))
			return(SH_FAIL);
	return(new_print(strp, &val, pout, &assoc));
}

fld_print(type, val, targvar, always_ptr, prargs)
char *type;
VAL val;
char *targvar;
int always_ptr;
char **prargs;
{
	struct strhead *strp;
	struct easyio ez, *ezp;
	int i;
	struct buf ezbuf;
	int ret = SH_SUCC;

	if (!type || !(strp = parse_decl(type, 1)))
		return(SH_FAIL);
	if ((strp->size > sizeof(VAL)) || (always_ptr && (strp->type != TYPE_DYNARRAY) && (strp->type != TYPE_POINTER)))
		strp = makeptr(strp);
	if (targvar) {
		ez.type = EZ_STR;
		ez.flags = 0;
		ez.fd_or_buf.buf = &ezbuf;
		ezbuf.buf = NULL;
		ezp = &ez;
		outprintf(&ez, "%s=", targvar);
	}
	else
		ezp = NULL;
	if (!prargs) {
		if (new_print(strp, &val, ezp, NULL) == SH_FAIL)
			return(SH_FAIL);
		if (!targvar)
			outputc(ezp, '\n');
	}
	else {
		if (!prargs[0])
			return(SH_FAIL);
		for (i = 0; prargs[i]; i++) {
			if (prargs[1])
				outprintf(ezp, prargs[i]);
			if (one_fld_print(strp, val, prargs[i], ezp) == SH_FAIL)
				ret = SH_FAIL;
			outputc(ezp, targvar ? ' ' : '\n');
		}
	}
	if (targvar)
		env_set(ezbuf.buf);
	return(ret);
}

do_field_get(argc, argv)
int argc;
char **argv;
{
	char *p, *bufstart;
	VAL val;
	char *fld, *type, *ptr, *ptr2, **pptr2;
	int i;
	char *targvar = NULL;
	char fail = 0, always_ptr;

	USESTAK();
	always_ptr = 0;
	for (i = 1; argv[i][0] == '-'; i++) {
		switch(argv[i][1]) {
		case 'p':
			always_ptr = 1;
			break;
		case 'v':
			targvar = argv[++i];
			break;
		case '?':
			XK_USAGE(argv[0]);
		}
	}
	type = argv[i++];
	if (!isdigit(argv[i][0]))
		always_ptr = 1;
	val = (VAL) getaddr(argv[i++]);
	if (fld_print(type, val, targvar, always_ptr, argv + i) == SH_FAIL) {
		altfprintf(2, (const char *) "Cannot print %s with value 0x%x\n", type, val);
		XK_USAGE(argv[0]);
	}
}

ulong
strprint(va_alist)
va_dcl
{
	va_list ap;
	char *arg;
	char *targvar = NULL;
	char *p;
	char *type;
	VAL val;
	char always_ptr;
	int nonames = 0;
	int ret;

	va_start(ap);
	always_ptr = 0;
	while ((arg = (char *) va_arg(ap, ulong)) && (arg[0] == '-')) {
		int i;

		for (i = 1; arg[i]; i++) {
			switch (arg[i]) {
			case 'v':
				targvar = va_arg(ap, char *);
				i = strlen(arg) - 1;
				break;
			case 'p':
				always_ptr = 1;
				break;
			case 'N':
				nonames = 1;
			}
		}
	}
	type = arg;
	val = (VAL) va_arg(ap, ulong);
	va_end(ap);
	if (!type) {
		altfprintf(2, (const char *) "Insufficient arguments to strprint\n");
		return(SH_FAIL);
	}
	Pr_tmpnonames = nonames;
	ret = fld_print(type, val, targvar, always_ptr, NULL);
	Pr_tmpnonames = 0;
	if (ret == SH_FAIL)
		altfprintf(2, (const char *) "Cannot print %s with value 0x%x\n", type, val);
	return(ret);
}

static
allprint(valarray)
struct valtype *valarray;
{
	int i;
	char *type;

	for (i = 0; valarray[i].type; i++) {
		switch(valarray[i].type->type) {
		case TYPE_INT:
		case TYPE_STRUCT:
		case TYPE_TYPEDEF:
		case TYPE_UNION:
			type = valarray[i].type->name;
			break;
		case TYPE_POINTER:
			type = (char *) ((const char *) "Pointer");
			break;
		case TYPE_ARRAY:
			type = (char *) ((const char *) "Array");
			break;
		case TYPE_DYNARRAY:
			type = (char *) ((const char *) "Dynamic Array(pointer to array)");
		}
		altprintf((const char *) "Argument %d (type %s):\n\t", i + 1, type);
		new_print(valarray[i].type, &valarray[i].val, NULL, NULL);
		altprintf((const char *) "\n");
	}
}

static
pp_usage()
{
	altfprintf(2, (const char *) "Please enter p(rint), s(end) or field=val\n");
}

static
call_postprompt(valarray)
struct valtype *valarray;
{
	char buf[BUFSIZ];

	for ( ; ; ) {
		myprompt((const char *) "Postprompt: ");
		buf[0] = 'q';
		buf[1] = '\0';
		if (!altgets(buf) || (xk_Strncmp(buf, (const char *) "q", 2) == 0)) {
			altfprintf(2, (const char *) "Warning: command will not be executed\n");
			return(0);
		}
		else if (xk_Strncmp(buf, (const char *) "p", 2) == 0)
			allprint(valarray);
		else if (xk_Strncmp(buf, (const char *) "s", 2) == 0)
			return(1);
		else if (!strchr(buf, '=') || (fld_set(valarray, buf, MAX_CALL_ARGS) == SH_FAIL))
			pp_usage();
	}
}

#define ZERORET		0
#define NONZERO		1
#define NONNEGATIVE	2
#define VOIDRET		3

/* In shell, 0 is success so, ZERORET means direct return, NONZERO means
** return the opposite of its truth value and NONNEGATIVE means return
** true if the value IS negative (since FALSE is success)
*/
#define CALL_RETURN(RET) return(SET_RET(RET), (\
			(ret_type == VOIDRET) ? 0 :\
			((ret_type == ZERORET) ? (RET) :\
			((ret_type == NONZERO) ? !(RET) :\
			((RET) < 0)))))
#define EARLY_RETURN(RET) return(SET_RET(RET))
#define SET_RET(RET) (((int) sprintf(xk_ret_buffer, use, (RET))), ((int) sprintf(xk_retd_buffer, "%d", (RET))), (int) (xk_retd_buf = xk_retd_buffer), (int) (xk_ret_buf = xk_ret_buffer), RET)

do_call(argc, argv)
int argc;
char **argv;
{
	int (*func)();
	char *p;
	char dorun, promptflag;
	unsigned char usestak, ret_type;
	register int i, j, ret;

	if (strcmp(argv[0], "-?") == 0)
		XK_USAGE(argv[0]);
	Cmode = 0;
	promptflag = 0;
	usestak = 1;
	ret_type = ZERORET;
	dorun = 1;
	if (!argv[1]) {
		altfprintf(2, (const char *) "No function to call\n");
		EARLY_RETURN(1);
	}
	for (j = 1; argv[j][0] == '-'; j++) {
		for (i = 1; argv[j][i]; i++) {
			switch(argv[j][i]) {
			case 'c':
				Cmode = 1;
				break;
			case 'F':
				/* Do not free, which means don't use the stack */
				usestak = 0;
				break;
			case 'r':
				/* reverse sense of return value */
				ret_type = NONZERO;
				break;
			case 'n':
				/* Non-negative return value is okay */
				ret_type = NONNEGATIVE;
				break;
			case 'v':
				/* Void return value, always return SUCCESS */
				ret_type = VOIDRET;
				break;
			default:
				altfprintf(2, (const char *) "Unrecognized flag %c\n", argv[j][1]);
				EARLY_RETURN(1);
			}
		}
	}
	if (!argv[j]) {
		altfprintf(2, (const char *) "No function to call\n");
		CALL_RETURN(1);
	}
	func = (int (*)()) fsym(argv[j], -1);
	if (!func && ((argv[j][0] != '0') || (UPP(argv[j][1]) != 'X') || !(func = (int (*)()) strtoul(argv[j], &p, 16)) || *p)) {
		altfprintf(2, (const char *) "No function to call %s\n", argv[j]);
		CALL_RETURN(1);
	}
	j++;
	for (i = 0; (i < MAX_CALL_ARGS) && argv[j]; j++, i++) {
		char *valstr;

		if (C_PAIR(argv[j], '+', '?')) {
			promptflag = 1;
			continue;
		}
		else if (C_PAIR(argv[j], '+', '+')) {
			j++;
			break;
		}
		if (argv[j][0] == '@') {
			if (!(valstr = strchr(argv[j] + 1, ':'))) {
				dorun = 0;
				ret = -1;
				break;
			}
			USESTAK();
			*valstr = '\0';
			Call_valarray[i].type = parse_decl(argv[j] + 1, 1);
			*valstr = ':';
			if (!Call_valarray[i].type) {
				dorun = 0;
				ret = -1;
				break;
			}
			if (!usestak)
				USENORM();
			valstr++;
			if (Call_valarray[i].type->size > sizeof(VAL))
				Call_valarray[i].type = makeptr(Call_valarray[i].type);
			if (new_parse(Call_valarray[i].type, &valstr, &Call_valarray[i].val, NULL) == SH_FAIL) {
				valstr = strchr(argv[j] + 1, ':');
				*valstr = '\0';
				altfprintf(2, (const char *) "Cannot figure out %s for %s\n", valstr + 1, argv[j] + 1);
				*valstr = ':';
				dorun = 0;
				ret = -1;
				break;
			}
		}
		else if (isdigit(argv[j][0])) {
			char *p;

			p = argv[j];
			Call_valarray[i].type = T_ulong;
			new_par_int(&p, &Call_valarray[i].val);
		}
		else if (strcmp(argv[j], (const char *) "NULL") == 0) {
			Call_valarray[i].type = T_ulong;
			Call_valarray[i].val = (VAL) NULL;
		}
		else if (Cmode) {
			if (isalpha(argv[j][0])) {
				if (cvar_get(argv[j], Call_valarray + i) == SH_FAIL) {
					altfprintf(2, (const char *) "No such variable: %s\n", argv[j]);
					dorun = 0;
					ret = -1;
					break;
				}
			}
			else {
				if (argv[j][0] == '!')
					Call_valarray[i].val = (VAL) (argv[j] + 1);
				else
					Call_valarray[i].val = (VAL) (argv[j]);
				Call_valarray[i].type = T_string_t;
			}
		}
		else {
			Call_valarray[i].type = T_string_t;
			Call_valarray[i].val = (VAL) (argv[j]);
		}
	}
	if (!usestak)
		USENORM();
	else
		USESTAK();
	for ( ; i < MAX_CALL_ARGS; i++) {
		Call_valarray[i].type = T_ulong;
		Call_valarray[i].val = (VAL) NULL;
	}
	if (dorun) {
		extern int errno;

		/* Process special arguments */
		while (argv[j]) {
			fld_set(Call_valarray, argv[j], i);
			j++;
		}
		if (!promptflag || call_postprompt(Call_valarray))
			ret = (*func)(Call_valarray[0].val, Call_valarray[1].val,
			    Call_valarray[2].val, Call_valarray[3].val,
				Call_valarray[4].val, Call_valarray[5].val,
				Call_valarray[6].val, Call_valarray[7].val,
				Call_valarray[8].val, Call_valarray[9].val,
				Call_valarray[10].val, Call_valarray[11].val,
				Call_valarray[12].val, Call_valarray[13].val,
				Call_valarray[14].val, Call_valarray[15].val,
				Call_valarray[16].val, Call_valarray[17].val,
				Call_valarray[18].val, Call_valarray[19].val,
				Call_valarray[20].val, Call_valarray[21].val,
				Call_valarray[22].val, Call_valarray[23].val,
				Call_valarray[24].val);
		else
			ret = 0;
		Xk_errno = errno;
	}
#ifdef COMMENTED_OUT
	for (i = 0; i < MAX_CALL_ARGS; i++) {
		/* There is no recourse for failure */
		XK_FREE(tblarray + i, pargs + i, 0, 0, all_tbl_find);
	}
#endif /* COMMENTED_OUT */
	CALL_RETURN(ret);
}

int _Prdebug;

static long
get_prdebug()
{
	return(_Prdebug);
}

static long
set_prdebug(n)
long n;
{
	_Prdebug = n;
}

do_field_comp(argc, argv)
int argc;
char **argv;
{
	struct easyio ez1, ez2;
	struct buf ez1buf, ez2buf;
	struct strhead *strp;
	char *valstr;
	VAL val1, val2;
	unsigned int i;
	unsigned char always_ptr;
	char *type;

	if (strcmp(argv[0], "-?") == 0)
		XK_USAGE(argv[0]);
	ez1.type = ez2.type = EZ_STR;
	ez1.flags = ez2.flags = 0;
	ez1.fd_or_buf.buf = &ez1buf;
	ez2.fd_or_buf.buf = &ez2buf;
	ez1buf.buf = ez2buf.buf = NULL;
	USESTAK();
	i = 1;
	if (C_PAIR(argv[i], '-', 'p')) {
		i++;
		always_ptr = 1;
	}
	else
		always_ptr = 0;
	type = argv[i++];
	if (!isdigit(argv[i][0]))
		always_ptr = 1;
	val1 = (VAL) getaddr(argv[i++]);
	if (!type || !(strp = parse_decl(type, 1))) {
		altfprintf(2, (const char *) "Cannot parse %s\n", type);
		XK_USAGE(argv[0]);
	}
	if ((strp->size > sizeof(VAL)) || (always_ptr && (strp->type != TYPE_DYNARRAY) && (strp->type != TYPE_POINTER)))
		strp = makeptr(strp);
	for ( ; argv[i]; i++) {
		char *tmpstr;

		if (tmpstr = valstr = strchr(argv[i], '='))
			*valstr++ = '\0';
		else
			valstr = argv[i];
		ez1buf.curspot = 0;
		ez2buf.curspot = 0;
		if ((one_fld_print(strp, val1, argv[i], &ez1) == SH_FAIL) || !ez1buf.curspot) {
			altfprintf(2, (const char *) "Failed to print: ");
			ALTPUTS(argv[i]);
			if (tmpstr != argv[i])
				*tmpstr = '=';
			XK_USAGE(argv[0]);
		}
		if (new_parse(strp, &valstr, &val2, NULL) == SH_FAIL) {
			altfprintf(2, (const char *) "Failed to parse: ");
			ALTPUTS(tmpstr + 1);
			if (tmpstr != argv[i])
				*tmpstr = '=';
			XK_USAGE(argv[0]);
		}
		if ((one_fld_print(strp, val2, argv[i], &ez2) == SH_FAIL) || !ez2buf.curspot) {
			altfprintf(2, (const char *) "Failed to print: ");
			if (tmpstr != argv[i])
				*tmpstr = '=';
			ALTPUTS(argv[i]);
			XK_USAGE(argv[0]);
		}
		if (tmpstr != argv[i])
			*tmpstr = '=';
		if ((ez1buf.curspot != ez2buf.curspot) || strncmp((char *) ez2buf.buf, (char *) ez1buf.buf, ez1buf.curspot)) {
			if (env_get((const char *) "PRCOMPARE"))
				altfprintf(2, (const char *) "Comparision failed: field %s\nActual:  %s\nCompare: %s\n", argv[i], ez1buf.buf, ez2buf.buf);
			return(SH_FAIL);
		}
	}
	return(SH_SUCC);
}

call_init()
{
	def_init();
	struct_init();
}

static
myprompt(prompt)
char *prompt;
{
	p_flush();
	p_setout(2);
	p_str(prompt, 0);
}
