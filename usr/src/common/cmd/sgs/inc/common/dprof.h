/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-inc:common/dprof.h	1.1"
/*
*
*   NB: assume pointers to functions have type unsigned long.
*	assume text pc from system is an integral type
*/

#define FCNTOT		600  	/*  (300 * sizeof(WORD)) */
#define NULL	 	0
#define MON_OUT  	"mon.out"

/* histogram information is kept in unsigned shorts */
typedef unsigned short WORD;


/* Actual structure of call count cell */
typedef struct cnt {
	unsigned long	*fnpc;		/* pointer to _mcount return */
	signed long	mcnt;		/* call count */
	struct _SOentry	*_SOptr;	/* pointer to _SOentry for function */
} Cnt;

/* Actual structure of call count buffer, replaces struct hdr.
 * This structure will hold call counts for all SOs
*/
typedef struct cntb {
	struct cntb	*next;		/* pointer to next call count buffer */
	Cnt		cnts[FCNTOT];	/* 600 call count cells */
} Cntb;

/* SOpath, baseaddr, and endaddr are from rtld information in the
   case of shared objects  */
typedef struct _SOentry {
	char	*SOpath;		/* path name of SO */
	unsigned long	baseaddr;	/* virtual address SO start of text */
	unsigned long	textstart;	/* virtual address SO start of text */
	unsigned long	endaddr;	/* virtual address SO end of text */
	signed int	ccnt;		/* counter of number of functions */
	WORD		*tcnt;		/* pointer to histogram buffer */
	unsigned int	size;		/* size of histogram buffer */
	struct _SOentry	*prev_SO;	/* pointer to previous _SOentry */
	struct _SOentry	*next_SO;	/* pointer to next _SOentry */
} SOentry;





#if defined(__STDC__)
extern void _newmon(int (*)(), int (*)(), WORD *, int);
#else
extern void _newmon();
#endif
