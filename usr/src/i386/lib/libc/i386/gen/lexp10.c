/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libc-i386:gen/lexp10.c	1.2"

#ifdef __STDC__
	#pragma weak lexp10 = _lexp10
#endif
#include	"synonyms.h"
#include	<sys/types.h>
#include	<sys/dl.h>

dl_t
lexp10(exp)
dl_t	exp;
{
	dl_t	result;

	result = lone;

	while(exp.dl_hop != 0  ||  exp.dl_lop != 0){
		result = lmul(result, lten);
		exp    = lsub(exp, lone);
	}

	return(result);
}
