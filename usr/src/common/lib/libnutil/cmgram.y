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

/* #ident	"@(#)libnwutil:common/lib/libnutil/cmgram.y	1.4" */
/* #ident	"$Id: cmgram.y,v 1.4 1994/07/26 17:46:09 vtag Exp $" */

%{
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

#define NWCM_SCHEMA_FRIEND
#include "nwcm.h"

#ifndef NULL
#define	NULL	0
#endif

extern int	ConfigDirty;

int		yyerror(char *);

#ifdef DEBUG
extern char *	NWCMConfigFilePath;
extern int	NWCMConfigFileLineNo;
#endif
extern void	ConvertToUpper(char *);

%}

%union {
	struct cp_s *	token;
	unsigned long	integer;
	int		boolean;
	char *		string;
}

%type	<token>		Token
%type	<integer>	IntegerExpression

%token	<integer>	IntegerConstant
%token	<boolean>	BooleanConstant
%token	<string>	QuotedString
%token	<string>	TokenString

%left '+' '-'
%left '*' '/' '%'
%left Negative

%%

ConfigFile		: /*EMPTY*/
			| ConfigStatement ConfigFile
			;

ConfigStatement		: Token '=' IntegerExpression
			{
				if ($1 == NULL)
					goto ParseIntParamEnd;
				if ($1->type != NWCP_INTEGER) {
#ifdef DEBUG
fprintf(stderr, "%s, line %d: parameter %s not type integer\n", NWCMConfigFilePath, NWCMConfigFileLineNo, $1->name);
#endif
					ConfigDirty++;
					goto ParseIntParamEnd;
				}
				if (*((unsigned long *) $1->cur_val) == $3) {
					goto ParseIntParamEnd;
				}
				if ($1->validation.data) {
					if ($1->validation.data->func) {
						if (!$1->validation.data->func($3)) {
#ifdef DEBUG
fprintf(stderr, "%s, line %d: %d is an invalid value for %s\n", NWCMConfigFilePath, NWCMConfigFileLineNo, $3, $1->name);
#endif
							ConfigDirty++;
							goto ParseIntParamEnd;
						}
					} else if (($3 < $1->validation.data->min)
					    || ($3 > $1->validation.data->max)) {
#ifdef DEBUG
fprintf(stderr, "%s, line %d: %d is out of range for %s\n", NWCMConfigFilePath, NWCMConfigFileLineNo, $3, $1->name);
#endif
						ConfigDirty++;
						goto ParseIntParamEnd;
					}
				}
				*((unsigned long *) $1->cur_val) = $3;
			ParseIntParamEnd:
#ifndef __GNUC__
				/*EMPTY*/;
#endif
			}
			| Token '=' BooleanConstant
			{
				if ($1 == NULL)
					goto ParseBoolParamEnd;
				if ($1->type != NWCP_BOOLEAN) {
#ifdef DEBUG
fprintf(stderr, "%s, line %d: parameter %s not type boolean\n", NWCMConfigFilePath, NWCMConfigFileLineNo, $1->name);
#endif
					ConfigDirty++;
					goto ParseBoolParamEnd;
				}
				if (*((int *) $1->cur_val) == $3)
					goto ParseBoolParamEnd;
				*((int *) $1->cur_val) = $3;
			ParseBoolParamEnd:
#ifndef __GNUC__
				/*EMPTY*/;
#endif
			}
			| Token '=' QuotedString
			{
				if ($1 == NULL)
					goto ParseStringParamEnd;
				if ($1->type != NWCP_STRING) {
#ifdef DEBUG
fprintf(stderr, "%s, line %d: parameter %s not type string\n", NWCMConfigFilePath, NWCMConfigFileLineNo, $1->name);
#endif
					ConfigDirty++;
					goto ParseStringParamEnd;
				}
				if ($1->format == df_uppercase)
					ConvertToUpper($3);
				if (strncmp((char *) $1->cur_val, $3,
				    NWCM_MAX_STRING_SIZE) == 0)
					goto ParseStringParamEnd;
				if ($1->validation.func) {
					if (!$1->validation.func($3)) {
#ifdef DEBUG
fprintf(stderr, "%s, line %d: \"%s\" is an invalid value for %s", NWCMConfigFilePath, NWCMConfigFileLineNo, $3, $1->name);
#endif
						ConfigDirty++;
						goto ParseStringParamEnd;
					}
				}
				(void) strncpy((char *) $1->cur_val, $3,
				    NWCM_MAX_STRING_SIZE);
			ParseStringParamEnd:
#ifndef __GNUC__
				/*EMPTY*/;
#endif
			}
			;

Token			: TokenString
			{
				int	ccode;

				if ($$ = (struct cp_s *)
				    malloc(sizeof(struct cp_s))) {
					if (ccode = _LookUpParameter($1,
					    NWCP_UNDEFINED, $$)) {
#ifdef DEBUG
fprintf(stderr, "%s, line %d: unknown parameter name %s\n", NWCMConfigFilePath, NWCMConfigFileLineNo, $1);
#endif
						ConfigDirty++;
						free((void *) $$);
						$$ = NULL;
					}
				}
			}
			;

IntegerExpression	: '(' IntegerExpression ')'
			{
				$$ = $2;
			}
			| IntegerExpression '*' IntegerExpression
			{
				$$ = $1 * $3;
			}
			| IntegerExpression '/' IntegerExpression
			{
				$$ = $1 / $3;
			}
			| IntegerExpression '%' IntegerExpression
			{
				$$ = $1 % $3;
			}
			| IntegerExpression '+' IntegerExpression
			{
				$$ = $1 + $3;
			}
			| IntegerExpression '-' IntegerExpression
			{
				$$ = $1 - $3;
			}
			| '-' IntegerExpression %prec Negative
			{
				$$ = -$2;
			}
			| IntegerConstant
			;

%%

#ifdef DEBUG

#include <stdio.h>

int
yyerror(char * s)
{
	return fprintf(stderr, "%s, line %d: %s\n", NWCMConfigFilePath,
	    NWCMConfigFileLineNo, s);
}

#else /* DEBUG */

int
yyerror(char * s)
{
	return 0;
}

#endif /* DEBUG */
