/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/crash.h	1.5"
#ident	"$Header: crash.h 1.1 91/07/23 $"

#include "sys/types.h"
#include "setjmp.h"
#include "string.h"
#include "sys/plocal.h"
#include <sys/kcore.h>

/* This file should include only command independent declarations */

#define ARGLEN 40	/* max length of argument */
#define NARGS 25	/* number of arguments to one function */
#define LINESIZE 256	/* size of function input line */

extern FILE *fp, *rp;	/* output file, redirect file pointer */
extern int Virtmode;	/* current address translation mode */
extern int mem;		/* file descriptor for dumpfile */
extern jmp_buf syn;	/* syntax error label */
extern struct var vbuf;	/* tunable variables buffer */
extern char *args[];	/* argument array */
extern int argcnt;	/* number of arguments */
extern int optind;	/* argument index */
extern char *optarg;	/* getopt argument */
extern long getargs();	/* function to get arguments */
extern long strcon();	/* function to convert strings to long */
extern long eval();	/* function to evaluate expressions */
extern struct syment *symsrch();	/* function for symbol search */
extern int tabsize;	/* Size of function table */
extern struct func functab[]; /* Function table */
extern char outfile[];	/* Holds name of output file for redirection */
extern int active;	/* Flag set if crash is examining an active system */
extern struct syment* findsp(); /* when using getksym to get symbol info, keeps
				 * track of manufactured syments */
extern struct plocal l;
extern int Nengine;

typedef	struct cr_mchunk {
	vaddr_t addr;
	struct {
		vaddr_t reg_addr;
		mregion_t mreg;
		off_t image_off;
	} cr_mregdesc[NMREG];
	struct cr_mchunk *next;
} cr_mchunk_t;


/* function definition */
struct func {
	char *name;
	char *syntax;
	int (*call)();
	char *description;
};

extern int debugmode;

#define MAINSTORE 0

#ifdef __STDC__
#define GETDSYM(sym,fatal)	getdsym(&S_##sym,#sym,fatal);
#else
#define	GETDSYM(sym,fatal)	getdsym(&S_sym,"sym",fatal);
#endif


#define UNKNOWN ((ulong_t) -1)

extern void getdsym();
extern char *vnotofsname();
extern int Cur_proc;
extern int Cur_lwp;
extern int Cur_eng;

/*
 * prototypes for exported functions
 */
struct lwp *slot_to_lwp(int, struct proc *);
struct proc *slot_to_proc(int);
