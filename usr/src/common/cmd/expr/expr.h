/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)expr:expr.h	1.1"
#ident  "$Header: $"

/*
 * 	Headerfile to expr
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
/* Enhanced Application Compatibility */
# define SUBSTR 275
# define LENGTH 276
# define INDEX 277

#define EQL(x,y) !strcmp(x,y)
