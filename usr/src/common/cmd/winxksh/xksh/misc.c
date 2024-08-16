/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/misc.c	1.2"

#include <stdio.h>
#include "xksh.h" /* which includes sys/types.h */
#include <sys/param.h>
#include <string.h>
#include <varargs.h>
#include <search.h>
#include <ctype.h>

const static char use[] = "0x%x";
const static char use2[] = "%s=0x%x";

void
altperror(s)
char *s;
{
	if (s)
		altfprintf(2, "%s: ", s);
#ifdef HAS_STRERROR
	altfputs(2, strerror(Xk_errno));
#else /* NO strerror() */
#ifdef HAS_SYS_ERRLIST
	{
		extern char *sys_errlist[];
		extern int sys_nerr;

		if (Xk_errno < sys_nerr)
			altfputs(2, sys_errlist[Xk_errno]);
		else
			altfprintf(2, "Unknown Error: %d\n", Xk_errno);
	}
#else /* NO sys_errlist either */
	altfprintf(2, "Unknown Error: %d\n", Xk_errno);
#endif
#endif 
}

int
symcomp(sym1, sym2)
const VOID *sym1, *sym2;
{
	return(strcmp(((struct symarray *) sym1)->str, ((struct symarray *) sym2)->str));
}

#ifndef STRTOUL_AVAILABLE

#define NPTR (nptr ? nptr : &ptr)

unsigned long
strtoul(str, nptr, base)
register const char *str;
char **nptr;
int base;
{
	register int c;
	int neg = 0;

	if (!isalnum(c = *str)) {
		while (isspace(c))
			c = *++str;
		switch (c) {
		case '-':
			neg++;
			/* FALLTHROUGH */
		case '+':
			c = *++str;
		}
	}
	/* If the number is negative, we can just call strtol */
	if (neg)
		return(-strtol(str, nptr, base));
	if (c != '0') {
		if (!base)
			base = 10;
	}
	else if (str[1] == 'x' || str[1] == 'X') {
		if (!base || (base == 16)) {
			str += 2;
			base = 16;
		}
	}
	else {
		if (!base || (base == 8)) {
			base = 8;
			str++;
		}
	}

	if (!str[0]) {
		if (nptr)
			*nptr = (char *) str;
		return(0);
	}
	/*
	** Short buffers are fine, quick, cheap optimization
	*/
	if (!str[1] || !str[2] || !str[3])
		return(strtol(str, nptr, base));
	/*
	** Break the buffer into all of the characters but the last and the
	** last character.  The result is the first part multiplied by the
	** base plus the last part.
	*/
	{
		char *last_spot;
		unsigned long part;
		char *ptr;

		last_spot = (char *) str + strlen(str) - 1;
		c = *last_spot;
		*last_spot = '\0';
		part = strtol(str, NPTR, base);
		*last_spot = c;
		if (*NPTR == last_spot)
			return(part * base + strtol(last_spot, NPTR, base));
		return(part);
	}
}
#endif /* not STRTOUL_AVAILABLE */

VOID *
getaddr(str)
char *str;
{
	if (isdigit(str[0]))
		return((VOID *) strtoul(str, NULL, 0));
	else
		return((VOID *) fsym(str, -1));
}

do_deref(argc, argv)
int argc;
char **argv;
{
	unchar *ptr;
	long i, len = 0;
	short longwise = -1;
	char printit = 0;


	for (i = 1; argv[i][0] == '-'; i++) {
		if (isdigit(argv[i][1])) {
			if (longwise < 0)
				longwise = 0;
			ptr = (unchar *) argv[i] + 1;
			new_par_int(&ptr, &len);
			if (!len) {
				altprintf((const char *) "Invalid length specifier: %s\n", argv[i]);
				XK_USAGE(argv[0]);
			}
		}
		else if (argv[i][1] == 'l')
			longwise = 1;
		else if (argv[i][1] == 'p')
			printit = 1;
	}
	if (longwise < 0)
		longwise = 1;
	if (!len)
		len = sizeof(long);
	if (!argv[i]) {
		altprintf((const char *) "Insufficient arguments to deref\n");
		XK_USAGE(argv[0]);
	}	
	ptr = (unchar *) getaddr(argv[i++]);
	if (ptr) {
		if (argv[i] || printit) {
			char *dbuf, *p;
			int totlen;
			char buf[10 * BUFSIZ];
			int incr;

			if (printit)
				totlen = len + 1 + 1;
			else
				totlen = len + strlen(argv[i]) + 1 + 1;
			dbuf = (char *) (totlen < (10 * BUFSIZ - 1)) ? buf : malloc(totlen);
			if (printit)
				strcpy(dbuf, "0x");
			else
				sprintf(dbuf, "%s=0x", argv[i]);
			p = dbuf + strlen(dbuf);
			incr = longwise ? sizeof(long) : sizeof(char);
			for (i=0; i < len; i += incr, p += 2 * incr)
				sprintf(p, "%*.*x", incr * 2, incr * 2, longwise ? *((ulong *) (ptr + i)) : (unsigned long) (ptr[i]));
			if (printit)
				ALTPUTS(dbuf);
			else
				env_set(dbuf);
			if (dbuf != buf)
				free(dbuf);
		}
		else {
			if (len > sizeof(ulong)) {
				altprintf((const char *) "The length must be less than %d to set RET\n", sizeof(ulong));
				XK_USAGE(argv[0]);
			}
			sprintf(xk_ret_buffer, use, *((ulong *) ptr));
			xk_ret_buf = xk_ret_buffer;
		}
		return(SH_SUCC);
	}
	altfprintf(2, "Cannot find %s\n", argv[--i]);
	XK_USAGE(argv[0]);
}

VOID *
nop(var)
VOID *var;
{
	return(var);
}

VOID *
save_alloc(var)
VOID *var;
{
	return(var);
}
