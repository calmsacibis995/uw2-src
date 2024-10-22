/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:common/ndxscn.c	1.2"


#ifdef __STDC__
	#pragma weak	elf_ndxscn = _elf_ndxscn
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"


size_t
elf_ndxscn(scn)
	register Elf_Scn	*scn;
{

	if (scn == 0)
		return SHN_UNDEF;
	return scn->s_index;
}
