/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:common/kind.c	1.4"


#ifdef __STDC__
	#pragma weak	elf_kind = _elf_kind
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"


Elf_Kind
elf_kind(elf)
	Elf	*elf;
{
	if (elf == 0)
		return ELF_K_NONE;
	return elf->ed_kind;
}
