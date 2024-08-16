/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/genlib.c	1.2"

#undef printf
#include <stdio.h>
#include "sh_config.h" /* which includes sys/types.h */
#include "xksh_config.h"
/*#include <sys/types.h>*/

#ifdef DYNLIB
#include <dlfcn.h>
#endif

#include <string.h>
#include <stropts.h>
#include <search.h>
#include <ctype.h>
#include "xksh.h"

struct libstruct *All_libs;
static int Nlibs;
static struct usage {
	const char *funcname;
	const char *message;
} Xk_usage[] = {
	(const char *) "call", (const char *)  "call [-F] [-n] [-r] func [ arg ] . . . [ ++ ] [ arg-modifier ] . . .",
	(const char *) "cdecl", (const char *)  "cdecl [-p] [-g] type var[=val] . . .",
	(const char *) "cprint", (const char *)  "cprint [-v target-variable] c-variable-expression . . .",
	(const char *) "cset", (const char *)  "cset c-variable-expression=val . . .",
	(const char *) "define", (const char *)  "define [-R] name value",
	(const char *) "deflist", (const char *)  "deflist [ -p prefix ] address",
	(const char *) "deref", (const char *)  "deref [-p] [-l] [-len] address [ shell-variable ]",
	(const char *) "field_comp", (const char *)  "field_comp type address [ criterion ] . . . ",
	(const char *) "field_get", (const char *)  "field_get [ -v var ] type address [ field-name ] . . . ",
	(const char *) "finddef", (const char *)  "finddef defname [ shell-variable ] ",
	(const char *) "findsym", (const char *)  "findsym symbol-name [ shell-variable ] ",
	(const char *) "libload", (const char *)  "libload [-f] [-p|-n name] object-name",
	(const char *) "libunload", (const char *)  "libunload [-n name] object-name",
	(const char *) "sizeof", (const char *)  "sizeof typename [ shell-variable ] ",
	(const char *) "struct", (const char *)  "struct [-R] name fld[:type] . . .",
	(const char *) "structlist", (const char *)  "structlist [ -i id ] [ -p prefix ] address",
	(const char *) "symbolic", (const char *)  "symbolic -m -t type . . . symbolic . . .",
	(const char *) "typedef", (const char *)  "typedef [-R] type-descriptor typename"
};

int
xk_usage(funcname)
char *funcname;
{
	int i;

	for (i = 0; i < sizeof(Xk_usage) / sizeof(struct usage); i++) {
		if (!funcname)
			ALTPUTS(Xk_usage[i].message);
		else if (!funcname || (strcmp(funcname, Xk_usage[i].funcname) == 0)) {
			altfprintf(2, (const char *) "Usage: %s\n", Xk_usage[i].message);
			return(SH_SUCC);
		}
	}
	return(funcname ? SH_FAIL : SH_SUCC);
}

ulong
fsym(str, lib)
char *str;
int lib;
{
#ifdef STATICLIB
	extern struct symarray Symarray[];
	extern int Symsize;
	int symcomp();
	struct symarray dummy;
#endif
#ifdef DYNLIB
	int i;
#endif
	VOID *found;

#ifdef STATICLIB
	dummy.str = str;
	if ((found = (VOID *) bsearch((char *) &dummy, Symarray, Symsize, sizeof(struct symarray), symcomp)) != NULL)
		return(((struct symarray *) found)->addr);
#endif /* STATICLIB */

#ifdef DYNLIB
	for (i = 0; i < Nlibs; i++)
		if ((found = dlsym(All_libs[i].handle, str)) != NULL)
			return((ulong) found);
#endif /* DYNLIB */
	return(NULL);
}

do_libinit()
{
}

static
del_lib(num)
int num;
{
	if (dlclose(All_libs[num].handle) != 0)
		altfputs(2, dlerror());
	free(All_libs[num].name);
	if (All_libs[num].prefix)
		free(All_libs[num].prefix);
	memmove(All_libs + num, All_libs + num + 1, Nlibs - num - 1);
	Nlibs--;
}

do_libunload(argc, argv)
int argc;
char **argv;
{
	int i, j, found;

	for (i = 1; argv[i]; i++) {
		found = 0;
		if (C_PAIR(argv[i], '-', '-n')) {
			char *name;

			if (argv[i][2])
				name = argv[i] + 2;
			else
				name = argv[i++];
			for (j = 0; j < Nlibs; j++)
				if (strcmp(All_libs[j].prefix, name) == 0) {
					found++;
					del_lib(j);
				}
			if (!found) {
				fprintf(stderr, "%s not found\n", name);
				continue;
			}
		}
		else {
			for (j = 0; j < Nlibs; j++)
				if (strcmp(All_libs[j].name, argv[i]) == 0) {
					found++;
					del_lib(j);
				}
			if (!found) {
				fprintf(stderr, "%s not found\n", argv[i]);
				continue;
			}
		}
	}
	return(SH_SUCC);
}

do_libload(argc, argv)
int argc;
char **argv;
{
	char buf[50];
	VOID (*func)();
	int i, j, k;
	char *prefix = NULL;
	char uprefix[10];
	VOID *handles[10];
	char *libs[10];
	char *ext_ubar;
#ifdef DYNLIB
	int flag = RTLD_NOW;
#endif

	uprefix[0] = '\0';
	ext_ubar = "";
	for (j = 0, i = 1; argv[i]; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'l':
#ifdef DYNLIB
				flag = RTLD_NOW;
#endif
				break;
			case 'f':
#ifdef DYNLIB
				flag = RTLD_NOW;
#endif
				break;
			case 'n':
				{
					int k;

					if (argv[i][2])
						prefix = argv[i] + 2;
					else
						prefix = argv[++i];
					if (prefix[strlen(prefix) - 1] != '_')
						ext_ubar = "_";
					for (k = 0; prefix[k]; k++)
						uprefix[k] = UPP(prefix[k]);
					uprefix[k] = '\0';
					break;
				}
			case 'p':
				{
					int k;

					if (argv[i][2])
						prefix = argv[i] + 2;
					else
						prefix = argv[++i];
					if (prefix[strlen(prefix) - 1] != '_')
						ext_ubar = "_";
					for (k = 0; prefix[k]; k++)
						uprefix[k] = UPP(prefix[k]);
					uprefix[k] = '\0';
					break;
				}
			default:
				altprintf((const char *) "Unknown option keyletter: %s\n", argv[i][1]);
				XK_USAGE(argv[0]);
			}
		}
		else {
			libs[j++] = argv[i];
		}
	}
	if (All_libs) {
		Nlibs += j;
		All_libs = (struct libstruct *) realloc(All_libs, Nlibs * sizeof(struct libstruct));
	}
	else {
		Nlibs = j;
		All_libs = (struct libstruct *) malloc(Nlibs * sizeof(struct libstruct));
	}
	if (!All_libs) {
		altprintf((const char *) "Cannot allocate All_libs, exiting\n");
		exit(1);
	}
	memset(&All_libs[Nlibs - j], '\0', j * sizeof(struct libstruct));
	for (k = Nlibs - j; k < Nlibs; k++) {
		if (prefix)
			All_libs[k].prefix = strdup(prefix);
		All_libs[k].name = strdup(libs[k-(Nlibs-j)]);
#ifdef DYNLIB
		if ((All_libs[Nlibs - 1].handle = dlopen((strcmp(All_libs[k].name, (const char *) "a.out") == 0) ? NULL : All_libs[k].name, flag)) == NULL)
			altprintf((const char *) "Warning: Cannot load library %s: %s\n", All_libs[k].name, dlerror());
#endif
	}
	sprintf(buf, (const char *) "%s%slibinit", prefix, ext_ubar);
	if (func = (VOID (*)()) fsym(buf, -1))
		(*func)();
	return(SH_SUCC);
}

do_findsym(argc, argv)
int argc;
char **argv;
{
	ulong found;
	struct symarray dummy;

	if (!argv[1]) {
		altfprintf(2, (const char *) "Must give argument to findsym\n");
		XK_USAGE(argv[0]);
	}
	if ((found = fsym(argv[1], -1)) != NULL) {
		if (argv[2]) {
			char buf[50];

			sprintf(buf, "%s=0x%lx", argv[2], found);
			env_set(buf);
		}
		else {
			sprintf(xk_ret_buffer, "0x%x", found);
			xk_ret_buf = xk_ret_buffer;
		}
	}
	else {
		altfprintf(2, (const char *) "Cannot find %s\n", argv[1]);
		XK_USAGE(argv[0]);
	}
	return(SH_SUCC);
}

/*
** Keep these alternate functions at the end of the file
** so that the undef's do not cause problems
*/
#undef fflush

altflush(fp)
FILE *fp;
{
	p_flush();
}

char *(*Promptfunc)();

char *
altprompt(str)
char *str;
{
	return(Promptfunc ? (*Promptfunc)(str) : str);
}

altfputs(fd, str)
int fd;
char *str;
{
	p_setout(fd);
	p_str(str, '\n');
}

altputs(str)
char *str;
{
	p_setout(1);
	p_str(str, '\n');
}

#undef printf
altfputchar(fd, c)
int fd;
char c;
{
	p_setout(fd);
	p_char(c);
}

altfprintf(fd, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
int fd;
const char *fmt;
ulong arg1, arg2, arg3, arg4, arg5, arg6, arg7;
{
	char buf[10 * BUFSIZ];

	sprintf(buf, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	p_setout(fd);
	p_str(buf, '\0');
	return(strlen(buf));
}

altprintf(fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
char *fmt;
ulong arg1, arg2, arg3, arg4, arg5, arg6, arg7;
{
	char buf[10 * BUFSIZ];

	sprintf(buf, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	p_setout(1);
	p_str(buf, '\0');
	return(strlen(buf));
}

char *
altgets(buf)
char *buf;
{
	char *p;

	p_setout(2);
	if (!vi_read(0, buf, BUFSIZ))
		return(NULL);
	if (p = strchr(buf, '\n'))
		*p = '\0';
	return(buf);
}

do_init()
{
	char *libs[3];
	static int init = 0;

	if (!init) {
		init = 1;
		libs[0] = "libload";
		libs[1] = "a.out";
		libs[2] = NULL;
		do_libload(2, libs);
		call_init();
		cvar_init();
	}
}

#ifndef SPRINTF_RET_LEN
/*
 * SYSTEM V sprintf() returns the length of the buffer, other versions
 * of the UNIX System don't.  So, if the SPRINTF_RET_LEN flag is not true,
 * then we define an alternate function, lsprintf(), which has the SYSV
 * behavior. Otherwise, lsprintf is #defined in xksh.h to be the
 * same as sprintf.
 */

int
lsprintf(buf, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
char *buf, *fmt;
ulong arg1, arg2, arg3, arg4, arg5, arg6, arg7;
{
	sprintf(buf, fmt, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	return(strlen(buf));
}
#endif
