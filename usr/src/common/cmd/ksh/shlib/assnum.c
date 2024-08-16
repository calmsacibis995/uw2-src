/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ksh:shlib/assnum.c	1.2.6.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ksh/shlib/assnum.c,v 1.1 91/02/28 17:41:43 ccs Exp $"

/*
 *   ASSNUM.C
 *
 *   Programmer:  D. G. Korn
 *
 *        Owner:  D. A. Lambeth
 *
 *         Date:  April 17, 1980
 *
 *
 *   NAM_LONGPUT (NP, NUM)
 *
 *        Assign the long integer NUM to NP.  NP should have
 *        the L_FLAG and N_INTGER attributes.
 *
 *
 *   See Also:  nam_putval(III), nam_free(III), nam_strval(III), ltos(III)
 */

#include	"name.h"

#ifdef FLOAT
#   define ltos 	etos
#endif /* FLOAT */
extern char *ltos();

/*
 *   ASSLONG (NP, NUM)
 *
 *        struct namnod *NP;
 *
 *        int NUM;
 *
 *   Assign the value NUM to the namnod given by NP.  All 
 *   appropriate conversions are made.
 */

void nam_longput(np,num)
register struct namnod *np;
#ifdef FLOAT
   double num;
#else
   long num;
#endif /* FLOAT */
{
	register union Namval *up = &np->value.namval;
	if (nam_istype (np, N_INTGER))
	{
		if (nam_istype (np, N_ARRAY))
			up = &(array_find(np,A_ASSIGN)->namval);
#ifdef NAME_SCOPE
		if (nam_istype (np, N_CWRITE))
			np = nam_copy(np,1);
#endif	/* NAME_SCOPE */
        	if (nam_istype (np, N_INDIRECT))
			up = up->up;
		nam_offtype(np,~N_IMPORT);
        	if (nam_istype (np, N_BLTNOD))
			(*up->fp->f_ap)((unsigned)num);
#ifdef FLOAT
		else if (nam_istype (np, N_DOUBLE))
		{
			if(up->dp==0)
				up->dp = new_of(double,0);
			*(up->dp) = num;
		}
#endif /* FLOAT */
		else
		{
			if(up->lp==0)
				up->lp = new_of(long,0);
			*(up->lp) = num;
			if(np->value.namsz == 0)
				np->value.namsz = sh_lastbase;
		}
	}
	else
		nam_putval(np,ltos(num,10));
}
