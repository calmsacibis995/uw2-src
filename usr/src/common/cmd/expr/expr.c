/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)expr:expr.c	1.21.4.4"
#ident "$Header: expr.c 1.2 91/08/13 $"

#include  <stdlib.h>
#include  <regex.h>
#include  <locale.h>
#include  <pfmt.h>
#include  <string.h>
#include  <stdio.h>
#include  <limits.h>
#include  "expr.h"

long atol();
char *ltoa(), *strcpy();
char	*expres();
void 	exit();
char	**Av;
int	Ac;
int	Argi;
int noarg;
int paren;

const char badsyn[] = ":233:Syntax error\n";
const char zerodiv[] = ":234:Division by zero\n";

static int
get2num(r1, r2, v1, v2)
char *r1, *r2;
long *v1, *v2;
{
	char *ep;

	if((*v1 = strtol(r1, &ep, 10)) == 0 && ep == r1)
		return 0;
	if((*v2 = strtol(r2, &ep, 10)) == 0 && ep == r2)
		return 0;
	return 1;
}

char *rel(oper, r1, r2) register char *r1, *r2; 
{
	long i1, i2;

	if(get2num(r1, r2, &i1, &i2))
		i1 -= i2;
	else
		i1 = strcoll(r1, r2);
	switch(oper) {
	case EQ: 
		i1 = i1==0; 
		break;
	case GT: 
		i1 = i1>0; 
		break;
	case GEQ: 
		i1 = i1>=0; 
		break;
	case LT: 
		i1 = i1<0; 
		break;
	case LEQ: 
		i1 = i1<=0; 
		break;
	case NEQ: 
		i1 = i1!=0; 
		break;
	}
	return i1? "1": "0";
}

char *arith(oper, r1, r2) char *r1, *r2; 
{
	long i1, i2;
	register char *rv;

	if(!get2num(r1, r2, &i1, &i2))
		yyerror(":235:Non-numeric argument\n");

	switch(oper) {
	case ADD: 
		i1 = i1 + i2; 
		break;
	case SUBT: 
		i1 = i1 - i2; 
		break;
	case MULT: 
		i1 = i1 * i2; 
		break;
	case DIV: 
		if (i2 == 0)
			yyerror(zerodiv);
		i1 = i1 / i2; 
		break;
	case REM: 
		if (i2 == 0)
			yyerror(zerodiv);
		i1 = i1 % i2; 
		break;
	}
	return ltoa(i1);
}
char *conj(oper, r1, r2) char *r1, *r2; 
{
	register char *rv;

	switch(oper) {

	case OR:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			rv = r2;
		else
			rv = r1;
		break;
	case AND:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			rv = "0";
		else if(EQL(r2, "0")
		    || EQL(r2, ""))
			rv = "0";
		else
			rv = r1;
		break;
	}
	return rv;
}

char *match(s, p)
char *s, *p;
{
	static regex_t re;
	static int gotre;
	regmatch_t rm[2];
	char msg[256];
	size_t n;
	char *q;
	int err;

	if(*p == '\0') {
		if(!gotre)
			yyerror(":199:No remembered search string"); /*no \n*/
	} else {
		if(gotre) {
			gotre = 0;
			regfree(&re);
		}
		if(*p != '^') {
			q = malloc(1 + strlen(p) + 1);
			q[0] = '^';
			strcpy(&q[1], p);
			p = q;
		}
		if((err = regcomp(&re, p, REG_OLDBRE)) != 0) {
	badre:;
			regerror(err, &re, msg, sizeof(msg));
			pfmt(stderr, MM_ERROR, ":1230:RE error: %s\n", msg);
			exit(2);
		}
		if(re.re_nsub > 1)
			yyerror(":236:Too many `\\(' s\n");
		gotre = 1;
	}
	if((err = regexec(&re, s, (size_t)2, &rm[0], 0)) == REG_NOMATCH) {
		if(re.re_nsub == 1)
			return "";
		return "0";
	}
	if(err != 0)
		goto badre;
	if(re.re_nsub == 1) {
		n = rm[1].rm_eo - rm[1].rm_so;
		q = malloc(n + 1);
		memcpy(q, &s[rm[1].rm_so], n);
		q[n] = '\0';
		return q;
	}
	return ltoa((long)rm[0].rm_eo);	/* rm[0].rm_so must be 0 */
}

yyerror(s)
char *s;
{
	pfmt(stderr, MM_ERROR, s);
	if(strchr(s, '\n') == 0)
		putc('\n', stderr);
	exit(2);
}

char *ltoa(l)
long l;
{
	char num[1 + (sizeof(long) * CHAR_BIT + 2) / 3 + 1];

	(void)sprintf(num, "%ld", l);
	return strdup(num);
}

main(argc, argv) char **argv; 
{
	char *ans;

	Ac = argc;
	Argi = 1;
	noarg = 0;
	paren = 0;
	Av = argv;
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:expr");

	ans = expres(0,1);
	if(Ac != Argi || paren != 0) {
		yyerror(badsyn);
	}
	(void) write(1, ans, (unsigned) strlen(ans));
	(void) write(1, "\n", 1);
	exit((!strcmp(ans, "0") || !ans[0])? 1: 0);
	/* NOTREACHED */
}
