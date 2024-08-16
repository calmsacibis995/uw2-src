/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/tools/sc_parser.y	1.4"

/*
 * Copyright 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include "sc_parser.h"

#ifndef NULL
#define	NULL	0
#endif
%}

%union {
	unsigned long	integer;
	char *		string;
	int		boolean;
	enum df_e	format;
}

%token			KW_ADD
%token			KW_BOOLEAN
%token			KW_DECIMAL
%token			KW_DIV
%token			KW_HEXADECIMAL
%token			KW_IDENT
%token			KW_INTEGER
%token			KW_LPAREN
%token			KW_MOD
%token			KW_MUL
%token			KW_OCTAL
%token			KW_RPAREN
%token			KW_STRING
%token			KW_SUB
%token			KW_UPPERCASE
%token	<boolean>	VAL_BOOLEAN
%token	<integer>	VAL_INTEGER
%token	<string>	VAL_FUNCTION
%token	<string>	VAL_STRING
%token	<string>	VAL_TOKEN

%type	<integer>	IntegerValue

%type	<format>	IntegerFormat
%type	<format>	StringFormat

%left KW_ADD KW_SUB
%left KW_MUL KW_DIV KW_MOD
%left Negative

%%

ConfigSchema	: /*EMPTY*/
		| ConfigSchema Statement
		;

Statement	: Ident
		| ParamDef
		;

Ident		: KW_IDENT VAL_STRING
		{
			Ident($2);
		}
		;

ParamDef	: IntegerParamDef
		| BooleanParamDef
		| StringParamDef
		;

IntegerParamDef	: VAL_TOKEN VAL_TOKEN VAL_TOKEN VAL_TOKEN IntegerFormat IntegerValue
		{
			IntegerParamDef($1, $2, $3, $4, $5, $6, NO_VALIDATION);
		}
		| VAL_TOKEN VAL_TOKEN VAL_TOKEN VAL_TOKEN IntegerFormat IntegerValue IntegerValue IntegerValue
		{
			IntegerParamDef($1, $2, $3, $4, $5, $6, MIN_MAX_VALIDATION, $7, $8);
		}
		| VAL_TOKEN VAL_TOKEN VAL_TOKEN VAL_TOKEN IntegerFormat IntegerValue VAL_FUNCTION
		{
			IntegerParamDef($1, $2, $3, $4, $5, $6, FUNCTION_VALIDATION, $7);
		}
		;

BooleanParamDef	: VAL_TOKEN VAL_TOKEN VAL_TOKEN VAL_TOKEN KW_BOOLEAN VAL_BOOLEAN
		{
			BooleanParamDef($1, $2, $3, $4, $6, NULL);
		}
		| VAL_TOKEN VAL_TOKEN VAL_TOKEN VAL_TOKEN KW_BOOLEAN VAL_BOOLEAN VAL_FUNCTION
		{
			BooleanParamDef($1, $2, $3, $4, $6, $7);
		}
		;

StringParamDef	: VAL_TOKEN VAL_TOKEN VAL_TOKEN VAL_TOKEN StringFormat VAL_STRING
		{
			StringParamDef($1, $2, $3, $4, $5, $6, NULL);
		}
		| VAL_TOKEN VAL_TOKEN VAL_TOKEN VAL_TOKEN StringFormat VAL_STRING VAL_FUNCTION
		{
			StringParamDef($1, $2, $3, $4, $5, $6, $7);
		}
		;

IntegerFormat	: KW_INTEGER
		{
			$$ = df_normal;
		}
		| KW_OCTAL KW_INTEGER
		{
			$$ = df_octal;
		}
		| KW_DECIMAL KW_INTEGER
		{
			$$ = df_decimal;
		}
		| KW_HEXADECIMAL KW_INTEGER
		{
			$$ = df_hexadecimal;
		}
		;

StringFormat	: KW_STRING
		{
			$$ = df_normal;
		}
		| KW_UPPERCASE KW_STRING
		{
			$$ = df_uppercase;
		}
		;

IntegerValue	: KW_LPAREN IntegerValue KW_RPAREN
		{
			$$ = $2;
		}
		| IntegerValue KW_MUL IntegerValue
		{
			$$ = $1 * $3;
		}
		| IntegerValue KW_DIV IntegerValue
		{
			$$ = $1 / $3;
		}
		| IntegerValue KW_MOD IntegerValue
		{
			$$ = $1 % $3;
		}
		| IntegerValue KW_ADD IntegerValue
		{
			$$ = $1 + $3;
		}
		| IntegerValue KW_SUB IntegerValue
		{
			$$ = $1 - $3;
		}
		| KW_SUB IntegerValue %prec Negative
		{
			$$ = -$2;
		}
		| VAL_INTEGER
		;

%%
