/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:common/newehdr.c	1.5"


#ifdef __STDC__
	#pragma weak	elf32_newehdr = _elf32_newehdr
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"
#include "error.h"


Elf32_Ehdr *
elf32_newehdr(elf)
	register Elf	*elf;
{
	register
	Elf32_Ehdr	*eh;

	if (elf == 0)
		return 0;

	/*	If reading file, return its hdr
	 */

	if (elf->ed_myflags & EDF_READ)
	{
		if ((eh = elf32_getehdr(elf)) != 0)
			elf->ed_ehflags |= ELF_F_DIRTY;
		return eh;
	}

	/*	Writing file
	 */

	if (elf->ed_class == ELFCLASSNONE)
		elf->ed_class = ELFCLASS32;
	else if (elf->ed_class != ELFCLASS32)
	{
		_elf_err = EREQ_CLASS;
		return 0;
	}
	if ((eh = elf32_getehdr(elf)) != 0)	/* this cooks if necessary */
	{
		elf->ed_ehflags |= ELF_F_DIRTY;
		return eh;
	}
	if ((eh = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr))) == 0)
	{
		_elf_err = EMEM_EHDR;
		return 0;	
	}
	*eh = _elf32_ehdr_init;
	elf->ed_myflags |= EDF_EHALLOC;
	elf->ed_ehflags |= ELF_F_DIRTY;
	return elf->ed_ehdr = eh;
}
