/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/xksh.h	1.2"

#ifndef XKSH_H
#define XKSH_H

#ifndef SYMS_ONLY

#include <sh_config.h>
#include "xksh_config.h"

#define SH_FAIL 1
#define SH_SUCC 0

#define PRSYMBOLIC			1
#define PRMIXED				2
#define PRDECIMAL			4
#define PRHEX				8
#define PRMIXED_SYMBOLIC	16
#define PRNAMES				32

#define UPP(CH) (islower(CH) ? toupper(CH) : (CH))
#define C_PAIR(STR, CH1, CH2) (((STR)[0] == (CH1)) && ((STR)[1] == (CH2)))
#define XK_USAGE(X) return(xk_usage(X), SH_FAIL);
#define UPPER(C) (islower(C) ? toupper(C) : (C))
#define isvarchar(C) (isalnum(C) || ((C) == '_'))
#define USESTAK() do { Mall_func = stakalloc; Free_func = xkstakfree; Grow_func = stakbgrow; } while(0)
#define USENORM() do { Mall_func = malloc; Free_func = free; Grow_func = bgrow; } while(0)

/* In the future, this will require following pointers, unless we
** can always trace back types to typedefs.  For example, ulong is
** a typedef, but it is simple because it is really just a long.
*/
#define IS_SIMPLE(TBL) ((TBL)->flags & F_SIMPLE)

#ifndef N_DEFAULT /* From name.h */
/* Stolen out of include/name.h, the problems of including things
** out of the ksh code is major.  Hence, the copy rather than the
** include.
*/

struct Bfunction {
	long	(*f_vp)();		/* value function */
	long	(*f_ap)();		/* assignment function */
};

#endif /* N_DEFAULT: From name.h */

#define ALLDATA		INT_MAX

#define BIGBUFSIZ (10 * BUFSIZ)

#define IN_BAND		1
#define OUT_BAND	2
#define NEW_PRIM	4

struct fd {
	int vfd;
	int flags;
	char mode;
	struct strbuf *lastrcv;
	int rcvcount;
	int sndcount;
	int uflags;
};

struct vfd {
	int fd;
};

extern struct fd *Fds;
extern struct vfd *Vfds;

/*struct libdesc {*/
	/*char *name;*/
	/*VOID *handle;*/
/*};*/
struct libstruct {
	char *prefix;
	/*int nlibs;*/
	/*struct libdesc *libs;*/
	char *name;
	VOID *handle;
};

extern char xk_retd_buffer[];
extern char *xk_retd_buf;

extern char xk_ret_buffer[];
extern char *xk_ret_buf;

#ifndef OSI_LIB_CODE
#define PARPEEK(b, s) (((b)[0][0] == s[0]) ? 1 :  0 )
#define PAREXPECT(b, s) (((b)[0][0] == s[0]) ? 0 : -1 )
#define OFFSET(T, M) ((int)(&((T)NULL)->M))

typedef char *string_t;

char *malloc();
char *realloc();
char *bgrow();
void free();
extern char *(*Mall_func)();
extern char *(*Grow_func)();
extern void (*Free_func)();
char *stakalloc();
char *stakbgrow();
void xkstakfree();
char *strdup();
VOID *getaddr();

#define SUCCESS	0
#define FAIL	(-1)

#define TRUE	1
#define FALSE	0

/* The following macro, RIF, stands for Return If Fail.  Practically
 * every line of encode/decode functions need to do this, so it aids
 * in readability.
 */
#define RIF(X) do { if ((X) == FAIL) return(FAIL); } while(0)

#endif /* not OSI_LIB_CODE */

#define DYNMEM_ID		(1)
#define BASE_ID			(2)

#define ALTPUTS(STR) do { p_setout(1); p_str((STR), '\n'); } while(0)

#ifndef NULL
#define NULL	(0)
#endif

#ifdef SPRINTF_RET_LEN
#define lsprintf sprintf
#endif

struct strhead *all_tbl_find();
struct strhead *list_ffind();
struct strhead *all_tbl_search();
struct strhead *fld_find();
struct strhead *ffind();
struct strhead *reduce();
struct strhead *parse_decl();
struct strhead *makeptr();

#define MAX_CALL_ARGS 25

extern struct strhead *T_ulong, *T_string_t;
extern int Pr_tmpnonames;
extern int _Delim;
extern int Xk_errno;
extern int Cmode;
int symcomp();

struct symlist {
	VOID *ptr;
	int isflag;
	int nsyms;
	struct symarray *syms;
};
#define NOHASH		1
#define TYPEONLY	2
#define STRUCTONLY	4

#define XK_SKIPWHITE(PBUF) while (isspace(**(PBUF))) (*PBUF)++

#endif /* not SYMS_ONLY */

struct symarray {
	const char *str;
	unsigned long addr;
};

#endif /* XKSH_H */
