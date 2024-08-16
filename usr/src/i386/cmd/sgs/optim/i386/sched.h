/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/sched.h	1.8"
#ifndef SCHED_H
#define SCHED_H

#include "optim.h"
#include "database.h"

#define NONE     0    /*three ways an instruction can go*/
#define	ALONE	5
#define ON_X	6
#define ON_J	7
#define LOST_CYCLE 8

     /* five types of dependency  */
#define NO_DEP  4
#define ANTI   	0
#define MAY		1
#define DEP		2
#define AGI		3

#define  dependent depend[DEP]
#define  anti_dep  depend[ANTI]
#define  agi_dep  depend[AGI]
#define  may_dep depend[MAY]

#define different	0 /*three possible relations between memories*/
#define same		1
#define unsafe		2

#define NREGS	 8    /* eight registers  */
#define ASIZE	1000  /* Size of memory allocate module array */
#define MAX_LABEL_LENGTH	100 /* assumed length of an operand */


#define tmalloc(type) (type *) malloc(sizeof(type))
#define isprefix(p)   (p->op == REP || p->op == REPZ || p->op == REPNZ\
                       || p->op == LOCK || p->op == ESC)
#define ismovs(p)	(p->op == SMOVB || p->op == SMOVW || p->op ==\
				  SMOVL)
#define ispp(p)  (p->op == PUSHL || p->op == PUSHW || p->op == POPL\
				 || p->op == POPW)
#define isshift(p)	((p->op) >= RCLB && (p->op) <= SHRL)
#define isior(op) ((op) != NULL && ((*op) == '$' || (*op) == '%'))
#define absdiff(x,y) (x < y ? y-x : x-y)
#define MAX(x,y) (x < y ? y : x )
#define MIN(x,y) (x > y ? y : x )
#define touchMEM(p)		((p->uses | p->sets) & MEM)
#define ENTRY(b)	(b->marked & 0xf)
#define exit(b)		((b->marked >> 8) & 0xf)
#define set_entry(b,m)	{ (b)->marked &= 0xfffffff0; (b)->marked |= (m); }


typedef struct clm {       /* a candidate list element */
        struct clm *next;
    	struct clm *back;
        NODE *member;
	    } clistmem;

typedef struct xlist { /* linked list of arrays for memory allocation*/
		struct xlist *next;
		tdependent   *a;
		} alist;

extern unsigned int muses(), msets(), indexing();
extern int get_exe_time();
extern int decompose();
extern int seems_same();
extern void check_fp();

#endif
