/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/w4opt.c	1.1.2.12"
/* w4opt.c
**
**	Intel 386 four-instruction window improver
**
**
**
** This module contains improvements for four instruction windows,
** of which there aren't many.
**
*/

/* #include "defs" -- optim.h takes care of this */
#include "optim.h"
#include "optutil.h"
extern int ptm_opts;
extern int jflag;

/* w4opt -- 4-instruction peephole window */

boolean					/* true if changes made */
w4opt(pf,pl)
register NODE * pf;			/* pointer to first inst. in window */
register NODE * pl;			/* pointer to last inst. in window */
{
    register NODE * p2 = pf->forw;	/* point at middle node */
    register NODE * p3 = p2->forw;	/* point at middle node */

    int cop1 = pf->op;			/* op code number of first inst. */
    int cop2 = p2->op;			/* op code number of second */
    int cop3 = p3->op;			/* ... of third */
    int cop4 = pl->op;			/* ... of fourth */

/*
**	This transformation is for the following code, which
**	can be a common sequence from the rcc compiler.
**	The instruction sequence that is sought after is:
**
**		fstl	X(ebp)	\
**		fstp	%st(0)		| fstl	X(ebp)
**		fldl	Y(ebp)		| faddl	Y(ebp)
**		faddl	X(ebp)	/
**
**	All of the operations may be converted, because there
**	are divrl, and subrl instructions non-communitive operators.
*/
/* redundant - never reached because of w2opt.c, treatment moved 
   therefore to w3opt.c */
#if 0
    if (
	    cop1 == FSTL
	&&  cop2 == FSTP
	&&  cop3 == FLDL
	&&  ( cop4 == FADDL || cop4 == FSUBL ||
	      cop4 == FMULL || cop4 == FDIVL )
	&&  strcmp(pf->op1,pl->op1) == 0
	&&  !isvolatile(pf,1)
	&&  strcmp(p2->op1,"%st(0)") == 0
	)
    {
	wchange();			/* change the window */
	lmrgin1(pl,pf,pf);		/* preserve line number info */
	pl->op1 = p3->op1;
	DELNODE(p2);			/* delete the extraneous moves */
	DELNODE(p3);
	if ( cop4 == FSUBL )
		chgop (pl, FSUBRL, "fsubrl");
	if ( cop4 == FDIVL )
		chgop (pl, FDIVRL, "fdivrl");
	return(true);
    }
#endif
/* op= improvement
**
** This improvement looks for a specific class of mov, op, mov
** instructions that may be converted to the shorter op, mov
** instruction sequence.  An example:
**
**	movl	O1,R1
**	op1X	R2,R1		->	op1X R2,O1
**	movl	R1,O1
**	op2X	R1,R3		->	op2X O1,R3
**
**	if R1 is dead after the sequence
**
*/
    if  (!ptm_opts || jflag)
	if ( 
	    cop1 == MOVL
	&&  cop3 == MOVL
	&&  (is2dyadic(pl) || cop4 == PUSHW || cop4 == PUSHL)
	&&  isreg(pf->op2)
	&&  isreg(p2->op1)
	&&  isreg(p2->op2)
	&&  isreg(p3->op1)
	&&	isreg(p2->op2)
	&&  isreg(pl->op1)
	&&  (pl->op2 == NULL || isreg(pl->op2))
	&&  strcmp(pf->op2,p2->op2) == 0
	&&  strcmp(p2->op2,p3->op1) == 0
	&&  strcmp(p3->op1,pl->op1) == 0
	&&  strcmp(pf->op1,p3->op2) == 0
	&&  ! strcmp(p2->op1,pf->op2) == 0
	&&  ((pl->op2 == NULL) || (! strcmp(pl->op2,pf->op2) == 0))
	&&  isdead(pl->op1,pl)
	&&  ! usesreg(pf->op1, pf->op2)
	&&  ! isvolatile(pf,1)		/* non-volatile */
	)
    {
	if (cop2 == ADDB || cop2 == ADDW || cop2 == ADDL ||
	    cop2 == ANDB || cop2 == ANDW || cop2 == ANDL ||
	    cop2 == ORB ||  cop2 == ORW ||  cop2 == ORL ||
	    cop2 == SARB || cop2 == SARW || cop2 == SARL ||
	    cop2 == SHLB || cop2 == SHLW || cop2 == SHLL ||
	    cop2 == SHRB || cop2 == SHRW || cop2 == SHRL ||
	    cop2 == SUBB || cop2 == SUBW || cop2 == SUBL ||
	    cop2 == XORB || cop2 == XORW || cop2 == XORL) 
	{
	wchange();		/* change the window */
	lmrgin1(p3,pl,p2);	/* preserve line number info */
	p2->op2 = pf->op1;
	pl->op1 = pf->op1;	/* Move fields around */
	makedead(pf->op2,p2);
	makelive(pf->op1,p2);
	DELNODE(pf);
	DELNODE(p3);
	return(true);
	}
    }
	/* cmp  jcc cmp jcc improvement
	**
	** cmp reg1,X
	** jcc
	** cmp reg1,X   - delete
	** jmp
	*/
	if (pf->op == CMPL
	 && p3->op == CMPL
	 && iscbr(p2)
	 && isbr(pl)
	 && !strcmp(pf->op1,p3->op1)
	 && !strcmp(pf->op2,p3->op2)
	 && !isvolatile(p3,2)
	 ) {
		wchange();
		DELNODE(p3);
		lexchin(p3,pl);
		return true;
	 }
	 /* made by loop unroll:
	 ** addl r1,r2 -> leal (,r1,4),r2
	 ** addl r1,r2
	 ** addl r1,r2
	 ** addl r1,r2
	 */
	 if (cop1 == ADDL
	  && cop2 == ADDL
	  && cop3 == ADDL
	  && cop4 == ADDL
	  && isreg(pf->op1)
	  && isreg(p2->op1)
	  && isreg(p3->op1)
	  && isreg(pl->op1)
	  && isreg(pf->op2)
	  && isreg(p2->op2)
	  && isreg(p3->op2)
	  && isreg(pl->op2)
	  && !strcmp(pf->op1,p2->op1)
	  && !strcmp(pf->op1,p3->op1)
	  && !strcmp(pf->op1,pl->op1)
	  && !strcmp(pf->op2,p2->op2)
	  && !strcmp(pf->op2,p3->op2)
	  && !strcmp(pf->op2,pl->op2)
          && !samereg(pf->op1,pf->op2)
   	  ) {
		wchange();
		chgop(pf,LEAL,"leal");
		pf->op1 = getspace(NEWSIZE);
		sprintf(pf->op1,"(%s,%s,4)",pf->op2,p2->op1);
		DELNODE(p2);
		DELNODE(p3);
		DELNODE(pl);
		return true;
	  }
    return(false);
}