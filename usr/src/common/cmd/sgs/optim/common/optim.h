/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:common/optim.h	1.35"
#ifndef OPTIM_H
#define OPTIM_H

/*	machine independent include file for code improver */

#include <stdio.h>
#include "defs.h"
extern int atoi(); /* from <stdlib.h> */

/* booleans */

typedef int boolean;
#define false	0
#define true	1

/* predefined "opcodes" for various nodes */

#define GHOST	0		/* temporary, to prevent linking node in */
#define TAIL	0		/* end of text list */
#define MISC	1		/* miscellaneous instruction */
#define FILTER	2		/* nodes to be filtered before optim */

extern void ldanal();

typedef struct sdependent {           /*linked list of dependent*/
	struct sdependent *next;      /*nodes, usefull for      */
	struct node *depends;         /*schedule.               */
	} tdependent;

/* structure of each text node */

typedef struct node {
	struct node *forw;	/* forward link */
	struct node *back;	/* backward link */
	tdependent  *depend[4]; /* dependent instructions */
	unsigned int sets;
	unsigned int uses;
	unsigned int idxs;
	unsigned int idus;
	int         dependents; /* # of dependent sons */
	int         chain_length; /* longest depending path */
	int         usage; /*used for marking at loops.c and sched.c*/
	int         nparents; /* on how many instructions it depends*/
	int         pairtype;  
	int			zero_op1;
	int			ebp_offset;
	int 		esp_offset;
	char *ops[6];	/* opcode or label and operand field strings */
	IDTYPE uniqid;		/* unique identification for this node */
	unsigned short op;	/* operation code */
	unsigned
	    nlive:32,	/* registers used by instruction */
	    ndead:32;	/* registers set but not used by instruction */
	unsigned
	    nrlive:32,	/* registers used by instruction */
	    nrdead:32;	/* registers set but not used by instruction */
	USERTYPE userdata;	/* user-defined data for this node */
	int extra;		/* used in enter-leave removal, if non-negative
				    contains the stack size on execution for this instruction */
	int extra2;    /*used together with extra to count changes in index
					register values. */
	int sasm;		/* is instruction a safe asm */
} NODE;


/* values for the extra field above */
#define REMOVE -1
#define NO_REMOVE -2
#define TMPSRET -3

#define opcode	ops[0]
#define op1	ops[1]
#define op2	ops[2]
#define op3	ops[3]
#define op4	ops[4]
#define op5	ops[5]
#define op6	ops[6]

/* block of text */

typedef struct block {
	struct block *next;	/* pointer to textually next block */
	struct block *prev;	/* pointer to textually previous block */
	struct block *nextl;	/* pointer to next executed block if no br */
	struct block *nextr;	/* pointer to next executed block if br */
	struct block *ltest;	/* for loop termination tests */
	NODE *firstn;		/* first text node of block */
	NODE *lastn;		/* last text node of block */
	short index;		/* block index for debugging purposes */
	short length;		/* number of instructions in block */
	short indeg;		/* number of text references */
	short indeg2;		/* number of text references, compare to indeg */
	short marked;		/* marker for various things */
	int	entry_depth;
	int exit_depth;
} BLOCK;

/* structure of non-branch text reference node */

typedef struct ref {
	char *lab;		/* label referenced */
	struct ref *nextref;	/* link to next ref */
	char *switch_table_name;
	BLOCK *switch_entry;
} REF;

typedef struct switch_tbl {
	REF *first_ref;	/* point to first label of switch in the ref list */
	REF *last_ref;	/* point to last label of switch in the ref list */
	char *switch_table_name; 
	struct switch_tbl *next_switch;
} SWITCH_TBL;

/* externals */

extern NODE n0;			/* header node of text list */
extern NODE ntail;		/* trailer node of text list */
extern NODE *lastnode;		/* pointer to last node on text list */
extern BLOCK b0;
extern REF r0;			/* header node of reference list */
extern REF *lastref;		/* pointer to last label reference */
extern SWITCH_TBL sw0;  /* header of switch table list */
extern int pic_flag;
extern int ieee_flag;
extern int dflag;		/* suppress movw 2 2 movb's */
extern int lflag;		/* print loop labels to stderr */
extern int ninst;		/* total # of instructions */

extern NODE *Saveop();
extern boolean same(), sameaddr();
extern char *getspace(), *xalloc();
extern void addref(), fatal(), init(), optim(), prtext(), xfree();
extern void start_switch_tbl(), end_switch_tbl();
extern void bldgr();
extern void set_refs_to_blocks();
extern void indeg(), indeg2();
extern int same_inst();

/* user-supplied functions or macros */

extern char *getp();
extern char *newlab();

#define saveop(opn, str, len, op) \
	    (void) Saveop((opn), (str), (unsigned)(len), (unsigned short)(op))
#define addtail(p)		/* superfluous */
#define appinst()		/* superfluous */
#define appmisc(str, len)	saveop(0, (str), (len), MISC)
#define appfl(str, len)		saveop(0, (str), (len), FILTER)
#define applbl(str, len) \
	(setlab(Saveop(0, (str), (unsigned)(len),MISC)), --ninst)
#define ALLN(p)			p = n0.forw; p != &ntail; p = p->forw
#define PRINTF			(void) printf
#define FPRINTF			(void) fprintf
#define SPRINTF			(void) sprintf
#define PUTCHAR(c)		(void) putchar(c)
#define DELNODE(p)		((p)->back->forw = (p)->forw, \
				    (p)->forw->back = (p)->back)
#define APPNODE(p, q)		PUTNODE((p), (q), back, forw)
#define INSNODE(p, q)		PUTNODE((p), (q), forw, back)
#define PUTNODE(p, q, f, b)	((p)->f = (q), (p)->b = (q)->b, \
				    (q)->b->f = (p) , \
				    (q)->b = (p))
#define GETSTR(type)		(type *) getspace(sizeof(type))
#define COPY(str, len)	((len) != 0 ? \
	(char *)memcpy(getspace(len), str, (int)(len)) : str)

#endif
