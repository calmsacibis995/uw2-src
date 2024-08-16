/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/basetbl.c	1.2"

#include <stdio.h>
#include "xksh.h"
#include "strparse.h"

/*
 * Declare all strings in one place to avoid duplication
 */
const static char STR_uint[] = "uint";
const static char STR_intp[] = "intp";
const static char STR_int[] = "int";
const static char STR_length[] = "length";
const static char STR_choice[] = "choice";
const static char STR_ulong[] = "ulong";
const static char STR_longp[] = "longp";
const static char STR_long[] = "long";
const static char STR_llength[] = "llength";
const static char STR_lchoice[] = "lchoice";
const static char STR_ushort[] = "ushort";
const static char STR_short[] = "short";
const static char STR_slength[] = "slength";
const static char STR_schoice[] = "schoice";
const static char STR_unchar[] = "unchar";
const static char STR_char[] = "char";
const static char STR_cchoice[] = "cchoice";
const static char STR_string[] = "string";
const static char STR_string_t[] = "string_t";
const static char STR_buffer[] = "buffer";

#define INTSTR static const struct intstr
#define PTRSTR static const struct ptrstr
#define DYNASTR static const struct arraystr
#define TYPDSTR static const struct typedefstr
typedef struct strhead * STRP;

INTSTR t_uint = { F_READONLY, TYPE_INT, sizeof(uint), (char *) STR_uint };
INTSTR t_int = { F_READONLY, TYPE_INT, sizeof(int), (char *) STR_int };
INTSTR t_length = { F_READONLY|F_LENGTH_FIELD, TYPE_INT, sizeof(int), (char *) STR_length };
INTSTR t_choice = { F_READONLY|F_CHOICE_FIELD, TYPE_INT, sizeof(int), (char *) STR_choice };
PTRSTR t_pint = { F_READONLY, TYPE_POINTER, sizeof(int *), (STRP) &t_int };
TYPDSTR t_intp = { F_READONLY, TYPE_TYPEDEF, sizeof(int *), (char *) STR_intp, (STRP) &t_pint };

INTSTR t_ulong = { F_READONLY, TYPE_INT, sizeof(ulong), (char *) STR_ulong };
INTSTR t_long = { F_READONLY, TYPE_INT, sizeof(long), (char *) STR_long };
INTSTR t_llength = { F_READONLY|F_LENGTH_FIELD, TYPE_INT, sizeof(long), (char *) STR_llength };
INTSTR t_lchoice = { F_READONLY|F_CHOICE_FIELD, TYPE_INT, sizeof(long), (char *) STR_lchoice };
PTRSTR t_plong = { F_READONLY, TYPE_POINTER, sizeof(long *), (STRP) &t_long };
TYPDSTR t_longp = { F_READONLY, TYPE_TYPEDEF, sizeof(long *), (char *) STR_longp, (STRP) &t_plong };

INTSTR t_ushort = { F_READONLY, TYPE_INT, sizeof(ushort), (char *) STR_ushort };
INTSTR t_short = { F_READONLY, TYPE_INT, sizeof(short), (char *) STR_short };
INTSTR t_slength = { F_READONLY|F_LENGTH_FIELD, TYPE_INT, sizeof(short), (char *) STR_slength };
INTSTR t_schoice = { F_READONLY|F_CHOICE_FIELD, TYPE_INT, sizeof(short), (char *) STR_schoice };

INTSTR t_unchar = { F_READONLY, TYPE_INT, sizeof(unchar), (char *) STR_unchar };
INTSTR t_char = { F_READONLY, TYPE_INT, sizeof(char), (char *) STR_char };
INTSTR t_cchoice = { F_READONLY|F_CHOICE_FIELD, TYPE_INT, sizeof(char), (char *) STR_cchoice };

DYNASTR t_str = { F_READONLY, TYPE_DYNARRAY, sizeof(char *), (STRP) &t_char };
TYPDSTR t_string = { F_READONLY, TYPE_TYPEDEF, sizeof(char *), (char *) STR_string, (STRP) &t_str };
TYPDSTR t_string_t = { F_READONLY, TYPE_TYPEDEF, sizeof(char *), (char *) STR_string_t, (STRP) &t_str };
DYNASTR t_buf = { F_READONLY, TYPE_DYNARRAY, sizeof(char *), (STRP) &t_char };
TYPDSTR t_buffer = { F_READONLY, TYPE_TYPEDEF, sizeof(char *), (char *) STR_buffer, (STRP) &t_buf };

STRP T_ulong = (STRP) &t_ulong;
STRP T_uint = (STRP) &t_uint;
STRP T_ushort = (STRP) &t_ushort;
STRP T_unchar = (STRP) &t_unchar;
STRP T_length = (STRP) &t_length;
STRP T_llength = (STRP) &t_llength;
STRP T_slength = (STRP) &t_slength;
STRP T_string_t = (STRP) &t_string_t;

const STRP basemems[] = {
	(STRP) &t_buffer,
	(STRP) &t_char,
	(STRP) &t_cchoice,
	(STRP) &t_choice,
	(STRP) &t_int,
	(STRP) &t_intp,
	(STRP) &t_lchoice,
	(STRP) &t_length,
	(STRP) &t_llength,
	(STRP) &t_long,
	(STRP) &t_longp,
	(STRP) &t_schoice,
	(STRP) &t_short,
	(STRP) &t_slength,
	(STRP) &t_string,
	(STRP) &t_string_t,
	(STRP) &t_uint,
	(STRP) &t_ulong,
	(STRP) &t_unchar,
	(STRP) &t_ushort,
	NULL
};

const struct symarray basedefs[] = {
	{ "PRDECIMAL", PRDECIMAL },
	{ "PRHEX", PRHEX },
	{ "PRMIXED", PRMIXED },
	{ "PRMIXED_SYMBOLIC", PRMIXED_SYMBOLIC },
	{ "PRNAMES", PRNAMES },
	{ "PRSYMBOLIC", PRSYMBOLIC },
	{ NULL, 0 }
};
