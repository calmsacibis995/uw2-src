/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*#ident	"@(#)r5xgc:gram.y	1.1"	*/
/*
** grammar for xgc syntax
*/

%{
#define YYDEBUG 1

#include <stdio.h>
#include <X11/X.h>
#include "constants.h"

extern int yylineno;
extern FILE *yyin;

extern void GC_change_function();
extern void GC_change_foreground();
extern void GC_change_background();
extern void GC_change_linewidth();
extern void GC_change_linestyle();
extern void GC_change_capstyle();
extern void GC_change_joinstyle();
extern void GC_change_fillstyle();
extern void GC_change_fillrule();
extern void GC_change_arcmode();
extern void GC_change_dashlist();
extern void GC_change_planemask();
extern void change_test();
extern void change_percent();
extern void run_test();
%}

%union
{
  int num;
  char *ptr;
};

%token <ptr> STRING
%token <num> NUMBER
%token <num> RUN
%token <num> FUNCTION FUNCTIONTYPE
%token <num> TEST TESTTYPE
%token <num> LINESTYLE LINESTYLETYPE
%token <num> CAPSTYLE CAPSTYLETYPE
%token <num> JOINSTYLE JOINSTYLETYPE
%token <num> ROUND SOLID
%token <num> FILLSTYLE FILLSTYLETYPE
%token <num> FILLRULE FILLRULETYPE
%token <num> ARCMODE ARCMODETYPE
%token <num> FOREGROUND BACKGROUND LINEWIDTH PLANEMASK DASHLIST PERCENT
%token <num> FONT

%%

all		: stmts
		;

stmts		: /* empty */
		| stmts '\n'
		| stmts stmt '\n'
		;

stmt		: error
		| RUN 
	{ run_test(); }  
		| TEST TESTTYPE 
	{ change_test ($2, TRUE); }
		| FUNCTION FUNCTIONTYPE 
	{ GC_change_function ($2, TRUE); }
		| LINESTYLE LINESTYLETYPE 
	{ GC_change_linestyle ($2, TRUE); }
		| LINESTYLE SOLID
	{ GC_change_linestyle (LineSolid, TRUE); }
		| CAPSTYLE CAPSTYLETYPE 
	{ GC_change_capstyle ($2, TRUE); }
		| CAPSTYLE ROUND 
	{ GC_change_capstyle (CapRound, TRUE); }
		| JOINSTYLE JOINSTYLETYPE 
	{ GC_change_joinstyle ($2, TRUE); }
		| JOINSTYLE ROUND 
	{ GC_change_joinstyle (JoinRound, TRUE); }
		| FILLSTYLE FILLSTYLETYPE
	{ GC_change_fillstyle ($2, TRUE); }
		| FILLSTYLE SOLID
	{ GC_change_fillstyle (FillSolid, TRUE); }
		| FILLRULE FILLRULETYPE
	{ GC_change_fillrule ($2, TRUE); }
		| ARCMODE ARCMODETYPE
	{ GC_change_arcmode ($2, TRUE); }
		| FOREGROUND NUMBER
	{ GC_change_foreground ($2, TRUE); }
		| BACKGROUND NUMBER
	{ GC_change_background ($2, TRUE); }
		| LINEWIDTH NUMBER
	{ GC_change_linewidth ($2, TRUE); }
		| PLANEMASK NUMBER
	{ GC_change_planemask ($2, TRUE); }
		| DASHLIST NUMBER
	{ GC_change_dashlist ($2, TRUE); }
		| FONT STRING
	{ GC_change_font ($2, TRUE); }
		| PERCENT NUMBER
	{ change_percent ($2, TRUE); }
		;

%%
yyerror(s)
     char *s;
{
  fprintf(stderr, "xgc: syntax error, line %d\n", yylineno);
}
