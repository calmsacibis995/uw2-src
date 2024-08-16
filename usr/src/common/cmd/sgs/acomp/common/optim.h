/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/optim.h	52.6"
/* optim.h */

/* Declarations for tree-opimization routines. */

extern ND1 * op_optim();
extern ND1 * op_init();
extern FP_LDOUBLE op_xtofp();
extern FP_LDOUBLE op_xtodp();


/* Prefix to recognize for special built-in functions.
** All such functions are presumed to begin with the
** same prefix.
*/
#ifndef	BUILTIN_PREFIX
#define	BUILTIN_PREFIX "__std_hdr_"
#endif
