/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/extra.h	1.2"


#define BASE_EXTRA_FUNCS \
int do_cdecl(); \
int do_cprint(); \
int do_cset(); \
int do_libload(); \
int do_libunload(); \
int do_sizeof(); \
int do_findsym(); \
int do_field_get(); \
int do_finddef(); \
int do_deflist(); \
int do_define(); \
int do_structlist(); \
int do_field_comp(); \
int do_deref(); \
int do_call(); \
int do_struct(); \
int do_union(); \
int do_typedef(); \
int do_symbolic(); \

#define BASE_EXTRA_TABLE \
	{ "call", VALPTR(do_call), N_BLTIN|BLT_FSUB }, \
	{ "cdecl", VALPTR(do_cdecl), N_BLTIN|BLT_FSUB }, \
	{ "cprint", VALPTR(do_cprint), N_BLTIN|BLT_FSUB }, \
	{ "cset", VALPTR(do_cset), N_BLTIN|BLT_FSUB }, \
	{ "field_comp", VALPTR(do_field_comp), N_BLTIN|BLT_FSUB }, \
	{ "field_get", VALPTR(do_field_get), N_BLTIN|BLT_FSUB }, \
	{ "libload", VALPTR(do_libload), N_BLTIN|BLT_FSUB }, \
	{ "libunload", VALPTR(do_libunload), N_BLTIN|BLT_FSUB }, \
	{ "sizeof", VALPTR(do_sizeof), N_BLTIN|BLT_FSUB }, \
	{ "findsym", VALPTR(do_findsym), N_BLTIN|BLT_FSUB }, \
	{ "finddef", VALPTR(do_finddef), N_BLTIN|BLT_FSUB }, \
	{ "deflist", VALPTR(do_deflist), N_BLTIN|BLT_FSUB }, \
	{ "define", VALPTR(do_define), N_BLTIN|BLT_FSUB }, \
	{ "structlist", VALPTR(do_structlist), N_BLTIN|BLT_FSUB }, \
	{ "deref", VALPTR(do_deref), N_BLTIN|BLT_FSUB }, \
	{ "struct", VALPTR(do_struct), N_BLTIN|BLT_FSUB }, \
	{ "union", VALPTR(do_union), N_BLTIN|BLT_FSUB }, \
	{ "typedef", VALPTR(do_typedef), N_BLTIN|BLT_FSUB }, \
	{ "symbolic", VALPTR(do_symbolic), N_BLTIN|BLT_FSUB }, \

#define BASE_EXTRA_VAR_DECL \
	extern char *xk_retd_buf; \
	extern char *xk_ret_buf; \
	extern struct Bfunction xk_prdebug; \

#define BASE_EXTRA_VAR \
	"RET",	(char*)(&xk_ret_buf),	N_FREE|N_INDIRECT|N_BLTNOD|N_RDONLY, \
	"_RETX",	(char*)(&xk_ret_buf),	N_FREE|N_INDIRECT|N_BLTNOD|N_RDONLY, \
	"_RETD",	(char*)(&xk_retd_buf),	N_FREE|N_INDIRECT|N_BLTNOD|N_RDONLY, \
	"PRDEBUG",	(char*)(&xk_prdebug),	N_FREE|N_INTGER|N_BLTNOD, \

#define BASE_EXTRA_ALIAS \
	"args",		"setargs \"$@\"",	N_FREE|N_EXPORT,\
	"ccall",	"call -c",			N_FREE|N_EXPORT,\

#define EXTRA_FUNCS BASE_EXTRA_FUNCS
#define EXTRA_TABLE BASE_EXTRA_TABLE
#define EXTRA_VAR_DECL BASE_EXTRA_VAR_DECL
#define EXTRA_VAR BASE_EXTRA_VAR
#define EXTRA_ALIAS BASE_EXTRA_ALIAS
