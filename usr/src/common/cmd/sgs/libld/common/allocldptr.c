/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libld:common/allocldptr.c	1.7"
#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#endif
#include "filehdr.h"
#include "ldfcn.h"
#include "lddef.h"
#include "synsyms.h"

LDLIST	*_ldhead = NULL;


LDFILE *
allocldptr()
{
	extern LDLIST *_ldhead;
	LDLIST *ldptr, *ldindx;
	static int last_fnum_ = 0;

	if ((ldptr = (LDLIST *)calloc(1, LDLSZ)) == NULL)
		return (NULL);
	ldptr->ld_next = NULL;
	if (_ldhead == NULL)
		_ldhead = ldptr;
	else
	{
		for (ldindx = _ldhead;
			ldindx->ld_next != NULL; ldindx = ldindx->ld_next)
		{
		}
		ldindx->ld_next = ldptr;
	}
	ldptr->ld_item._fnum_ = ++last_fnum_;
	return ((LDFILE *)ldptr);
}

