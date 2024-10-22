/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:common/getbase.c	1.6"


#ifdef __STDC__
	#pragma weak	elf_getbase = _elf_getbase
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"


off_t
elf_getbase(elf)
	Elf	*elf;
{
	if (elf == 0)
		return -1;
	return elf->ed_baseoff;
}
