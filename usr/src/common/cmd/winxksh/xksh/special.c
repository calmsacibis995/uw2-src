/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/special.c	1.2"

#include "sh_config.h" /* which includes sys/types.h */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "xksh.h"
#include "strparse.h"

int
new_prin_charstr(pout, str, len)
VOID *pout;
unsigned char *str;
int len;
{
	register int i;

	if (str == NULL)
		outprintf(pout, "NULL");
	else {
		for (i = 0; i < len; i++)
			if (!(isprint(str[i]) || isspace(str[i]))) {
				new_prin_hexstr(pout, str, len);
				return;
			}
		outputc(pout, '"');
		for (i = 0; i < len; i++) {
			switch (str[i]) {
			case '"':
				outprintf(pout, "\\\"");
				break;
			case '\n':
				outprintf(pout, "\\n");
				break;
			case '\t':
				outprintf(pout, "\\t");
				break;
			default:
				outputc(pout, str[i]);
			}
		}
		outputc(pout, '"');
	}
}

static int
new_prin_hexstr(pout, str, len)
VOID *pout;
char *str;
int len;
{
	register int i;

	outprintf(pout, "0x");
	for (i = 0; i < len; i++) {
		if (str[i] & 0xf0) {
			outprintf(pout, "%x", (long) str[i]);
		}
		else
			outprintf(pout, "0%x", (long) str[i]);
	}
}

int
new_par_chararr(ppinp, str, maxlen, plen)
char **ppinp;
char *str;
int maxlen;
int *plen;
{
	return(new_par_charstr(ppinp, str, maxlen, plen));
}

/*
 * NOTE: We leave a malloced buffer the same size rather
 * than realloc()'ing it to be the exact size in order
 * to save time and avoid malloc arena fragmentation
 */
int
new_par_charptr(ppinp, pstr, plen)
char **ppinp;
char **pstr;
int *plen;
{
	int buflen;

	XK_SKIPWHITE(ppinp);
	buflen = strlen(*ppinp);
	if (strncmp(*ppinp, "NULL", 4) == 0) {
		*pstr = NULL;
		*plen = -1;
		return(SH_SUCC);
	}
	if (!ispunct(**ppinp))
		buflen /= 2;
	if (!(*pstr = (*Mall_func)(buflen)))
		return(SH_FAIL);
	if (new_par_charstr(ppinp, *pstr, buflen, plen) == SH_FAIL) {
		(*Free_func)(*pstr);
		return(SH_FAIL);
	}
	return(SH_SUCC);
}

int
new_par_charstr(ppinp, str, maxlen, plen)
char **ppinp;
char *str;
int maxlen;
int *plen;
{
	register int i;
	char delim;

	XK_SKIPWHITE(ppinp);
	if (!ispunct(**ppinp)) {
		if ((*ppinp)[0] == '0' && ((*ppinp)[1] == 'x' || (*ppinp)[1] == 'X'))
			(*ppinp) += 2;
		return(new_par_hexstr(ppinp, str, maxlen, plen));
	}
	else
		delim = *((*ppinp)++);
	i = 0;
	while ((i < maxlen) && ((*ppinp)[0] != '\0') && ((*ppinp)[0] != delim)) {
		if ((*ppinp)[0] == '\\') {
			(*ppinp)++;
			switch ((*ppinp)[0]) {
			case 't':
				str[i++] = '\t';
				(*ppinp)++;
				break;
			case 'n':
				str[i++] = '\n';
				(*ppinp)++;
				break;
			case 'r':
				str[i++] = '\r';
				(*ppinp)++;
				break;
			case 'v':
				str[i++] = '\v';
				(*ppinp)++;
				break;
			case 'f':
				str[i++] = '\f';
				(*ppinp)++;
				break;
			case 'b':
				str[i++] = '\b';
				(*ppinp)++;
				break;
			case '0':
				str[i++] = (char)strtol(*ppinp, ppinp, 8);
				break;
			case 's':
				(*ppinp)++;
				break;
			default:
				str[i++] = *(*ppinp)++;
			}
		}
		else
			str[i++] = *(*ppinp)++;
	}
	if (i < maxlen) {
		str[i] = '\0';
		if (**ppinp)
			(*ppinp)++;	/* eat the trailing delim */
	}
	*plen = i;
	return(SH_SUCC);
}

const static char *conv = "0123456789ABCDEF";
#define CONV(C) (strchr(conv, (C)) - conv)

new_par_hexstr(ppinp, str, maxlen, plen)
char **ppinp;
char *str;
int maxlen;
int *plen;
{
	char c1, c2;
	register int i;

	for (i = 0; (i < maxlen); i++) {
		c1 = UPP((*ppinp)[0]);
		c2 = UPP((*ppinp)[1]);
		if (!c1 || !c2 || !strchr(conv, c1) || !strchr(conv, c2))
			break;
		str[i] = CONV(c1)* 16 + CONV(c2);
		(*ppinp) += 2;
	}
	if (i < maxlen)
		str[i] = '\0';
	*plen = i;
	return(SH_SUCC);
}

struct special {
	char *name;
	int (*free)();
	int (*parse)();
	int (*print)();
};

#define SPEC_FREE	0
#define SPEC_PARSE	1
#define SPEC_PRINT	2

static struct special *Special = NULL;
static int Nspecs = 0;

int (*
find_special(type, name))()
int type;
char *name;
{
	int i;

	if (!Special)
		return(NULL);
	for (i = 0; i < Nspecs; i++) {
		if (strcmp(Special[i].name, name) == 0) {
			switch(type) {
			case SPEC_PRINT:
				return(Special[i].print);
			case SPEC_FREE:
				return(Special[i].free);
			case SPEC_PARSE:
				return(Special[i].parse);
			}
		}
	}
	return(NULL);
}

int
set_special(name, free, parse, print)
char *name;
int (*free)();
int (*parse)();
int (*print)();
{
	int i;

	for (i = 0; i < Nspecs; i++)
		if (strcmp(Special[i].name, name) == 0)
			break;
	if (i == Nspecs) {
		if (!Special) {
			Special = (struct special *) malloc(sizeof(struct special));
			Special[0].name = strdup(name);
			Nspecs = 1;
		}
		else {
			Special = (struct special *) realloc(Special, (Nspecs + 1) * sizeof(struct special));
			Special[i].name = strdup(name);
			Nspecs++;
		}
	}
	if (!Special)
		return(FAIL);
	Special[i].free = free;
	Special[i].parse = parse;
	Special[i].print = print;
	return(SUCCESS);
}
