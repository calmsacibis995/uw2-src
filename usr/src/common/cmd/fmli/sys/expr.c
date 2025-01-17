/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)fmli:sys/expr.c	1.10.3.4"

#include	<stdio.h>
#include	<string.h>
/*
 * FMLI begin 
 */
#include	<setjmp.h>
#include 	"terror.h"	/* dmd s15 */
#include	"wish.h"
/* #include	"message.h" */
#include	"eval.h"

static jmp_buf Jumpenv;
/*
 * FMLI end
 */

# define A_STRING 258
# define NOARG 259
# define OR 260
# define AND 261
# define EQ 262
# define LT 263
# define GT 264
# define GEQ 265
# define LEQ 266
# define NEQ 267
# define ADD 268
# define SUBT 269
# define MULT 270
# define DIV 271
# define REM 272
# define MCH 273
# define MATCH 274

/* size of subexpression array */
#define MSIZE	512
#ifdef error
#  undef error
#endif
#define error(c)	errxx()
#define EQL(x,y) !strcmp(x,y)

#define ERROR(c)	errxx()
#include  <stdlib.h>
#include  <regexpr.h>


long atol();
static char *ltoa();
void exit();
static char	**Av;
static char *buf;
static int	Ac;
static int	Argi;
static int noarg;
static int paren;
static IOSTRUCT *Errstr;	/* abs s14 */
static char *expbuf;
/*	
 *	Array used to store subexpressions in regular expressions
 *	Only one subexpression allowed per regular expression currently 
 */
static char Mstring[1][MSIZE];

static char *operator[] = { 
	"|", "&", "+", "-", "*", "/", "%", ":",
	"=", "==", "<", "<=", ">", ">=", "!=",
	"match", "\0" };
static int op[] = { 
	OR, AND, ADD,  SUBT, MULT, DIV, REM, MCH,
	EQ, EQ, LT, LEQ, GT, GEQ, NEQ,
	MATCH };
static int pri[] = {
	1,2,3,3,3,3,3,3,4,4,5,5,5,6,7};


static int
yylex() 
{
	register char *p;
	register i;

	if(Argi >= Ac) return NOARG;

	p = Av[Argi];

	if((*p == '(' || *p == ')') && p[1] == '\0' )
		return (int)*p;
	for(i = 0; *operator[i]; ++i)
		if(EQL(operator[i], p))
			return op[i];


	return A_STRING;
}

static char *
rel(oper, r1, r2) register char *r1, *r2; 
{
	long i;

	if(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$"))
		i = atol(r1) - atol(r2);
	else
		i = strcmp(r1, r2);
	switch(oper) {
	case EQ: 
		i = i==0; 
		break;
	case GT: 
		i = i>0; 
		break;
	case GEQ: 
		i = i>=0; 
		break;
	case LT: 
		i = i<0; 
		break;
	case LEQ: 
		i = i<=0; 
		break;
	case NEQ: 
		i = i!=0; 
		break;
	}
	return i? "1": "0";
}

static char *
arith(oper, r1, r2) char *r1, *r2; 
{
	long i1, i2;
	register char *rv;

	if(!(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$")))
		yyerror( gettxt(":239", "Non-numeric argument") );
	i1 = atol(r1);
	i2 = atol(r2);

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
			yyerror( gettxt(":240","Division by zero") );
		i1 = i1 / i2; 
		break;
	case REM: 
		if (i2 == 0)
			yyerror( gettxt(":240","Division by zero") );
		i1 = i1 % i2; 
		break;
	}
	if ((rv = malloc(16)) == NULL)
		fatal(NOMEM, nil);	/* dmd s15 */
	(void) strcpy(rv, ltoa(i1));
	return rv;
}

static char *
conj(oper, r1, r2) 
char *r1, *r2; 
{
	register char *rv;

	switch(oper) {

	case OR:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			if(EQL(r2, "0")
			    || EQL(r2, ""))
				rv = "0";
			else
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

static char *
match(s, p)
char *s, *p;
{
	register char *rv;

	if ((rv=malloc(8)) == NULL)
		fatal(NOMEM, nil);	/* dmd s15 */
	(void) strcpy(rv, ltoa((long)ematch(s, p)));
	if(nbra) {
		(void) free(rv);	/* dmd s15 */
		if ((rv = malloc( strlen(Mstring[0]) + 1)) == NULL)
			fatal(NOMEM, nil);	/* dmd s15 */
		(void) strcpy(rv, Mstring[0]);
	}
	return rv;
}

static int
ematch(s, p)
char *s;
register char *p;
{
	char *nexpbuf;
	register num;
	extern char *braslist[], *braelist[], *loc2;

	nexpbuf = compile(p, (char *)0, (char *)0);
	if(nbra > 1)
		yyerror( gettxt(":241","Too many '\\('s") );
	if(regerrno) {
		if (regerrno != 41 || expbuf == NULL)
			errxx();
	} else {
		if (expbuf)
			free(expbuf);
		expbuf = nexpbuf;
	}
	if(advance(s, expbuf)) {
		if(nbra == 1) {
			p = braslist[0];
			num = braelist[0] - p;
			if ((num > MSIZE - 1) || (num < 0))
				yyerror( gettxt (":242", "string too long\n") );
			(void) strncpy(Mstring[0], p, num);
			Mstring[0][num] = '\0';
		}
		return(loc2-s);
	}
	return(0);
}

static
errxx()
{
	yyerror( gettxt(":243","RE error") );
}

static int
yyerror(s)
char *s;
{
	char tmpbuf[BUFSIZ];

 /*
  * Changed the expr prefix to fmlexpr.         mek
  */
	sprintf(tmpbuf, "fmlexpr: %s\n", s);
	putastr(tmpbuf, Errstr); 		/* abs s14 */

	/*****************
        mess_temp(tmpbuf);
	mess_flash(tmpbuf);
	doupdate();
        ****************** abs s14 */
	
	longjmp(Jumpenv, 1);
	/*NOTREACHED*/
}

static char *
ltoa(l)
long l;
{
	static str[20];
	register char *sp = (char *) &str[18];	/*u370*/
	register i;
	register neg = 0;

	if(l == 0x80000000L)
		return "-2147483648";
	if(l < 0)
		++neg, l = -l;
	str[19] = '\0';
	do {
		i = l % 10;
		*sp-- = '0' + i;
		l /= 10;
	} 
	while(l);
	if(neg)
		*sp-- = '-';
	return ++sp;
}
static char *
expres(prior,par)  int prior, par; 
{
	int ylex, temp, op1;
	char *r1, *ra, *rb;

	ylex = yylex();
	if (ylex >= NOARG && ylex < MATCH) {
		yyerror( gettxt(":244","Syntax error") );
	}
	if (ylex == A_STRING) {
		r1 = Av[Argi++];
		temp = Argi;
	}
	else {
		if (ylex == '(') {
			paren++;
			Argi++;
			r1 = expres(0,Argi);
			Argi--;
		}
	}
lop:
	ylex = yylex();
	if (ylex > NOARG && ylex < MATCH) {
		op1 = ylex;
		Argi++;
		if (pri[op1-OR] <= prior ) 
			return r1;
		else {
			switch(op1) {
			case OR:
			case AND:
				r1 = conj(op1,r1,expres(pri[op1-OR],0));
				break;
			case EQ:
			case LT:
			case GT:
			case LEQ:
			case GEQ:
			case NEQ:
				r1=rel(op1,r1,expres(pri[op1-OR],0));
				break;
			case ADD:
			case SUBT:
			case MULT:
			case DIV:
			case REM:
				r1=arith(op1,r1,expres(pri[op1-OR],0));
				break;
			case MCH:
				r1=match(r1,expres(pri[op1-OR],0));
				break;
			}
			if(noarg == 1) {
				return r1;
			}
			Argi--;
			goto lop;
		}
	}
	ylex = yylex();
	if(ylex == ')') {
		if(par == Argi) {
			yyerror( gettxt(":244","Syntax error") );
		}
		if(par != 0) {
			paren--;
			Argi++;
		}
		Argi++;
		return r1;
	}
	ylex = yylex();
	if(ylex > MCH && ylex <= MATCH) {
		if (Argi == temp) {
			return r1;
		}
		op1 = ylex;
		Argi++;
		switch(op1) {
		case MATCH:
			rb = expres(pri[op1-OR],0);
			ra = expres(pri[op1-OR],0);
		}
		switch(op1) {
		case MATCH: 
			r1 = match(rb,ra); 
			break;
		}
		if(noarg == 1) {
			return r1;
		}
		Argi--;
		goto lop;
	}
	ylex = yylex();
	if (ylex == NOARG) {
		noarg = 1;
	}
	return r1;
}

cmd_expr(argc, argv, instr, outstr, errstr)
int argc;
char **argv; 
IOSTRUCT	*instr;
IOSTRUCT	*outstr;
IOSTRUCT	*errstr;	/* abs s14 */
{
        Errstr = errstr;	/* abs s14 */

	Ac = argc;
	Argi = 1;
	noarg = 0;
	paren = 0;
	Av = argv;
	buf = NULL;

	Mstring[0][0] = '\0';
	expbuf = NULL;

	/*
	 * FMLI: return FAIL if setjmp returns a non-zero value
	 * (i.e., called by longjmp() in yyerror())
	 */
	if (setjmp(Jumpenv) != 0)
		return(FAIL);

	buf = expres(0,1);
	if(Ac != Argi || paren != 0) {
		yyerror( gettxt(":245","syntax error") );
	}

	/*
	 * use FMLI output routines
	 */
	(void) putastr(buf, outstr);
	(void) putastr("\n", outstr);
	if ( expbuf )
	    (void) free (expbuf);
	return((!strcmp(buf, "0") || !buf[0])? FAIL: SUCCESS);
}
