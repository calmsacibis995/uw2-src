/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/binder.c	1.17"


/* function binding routine - invoked on the first call
 * to a function through the procedure linkage table;
 * passes first through an assembly language interface
 *
 *
 * Takes the address of the PLT entry where the call originated,
 * the offset into the relocation table of the associated
 * relocation entry and the address of the struct rt_private_map * for the entry
 *
 * Returns the address of the function referenced after
 * re-writing the PLT entry to invoke the function
 * directly
 * 
 * On error, causes process to terminate with a SIGKILL
 */

#include "rtinc.h"
#include <signal.h>

 unsigned long 
 _binder(lm, reloc)
 struct rt_private_map *lm;
 unsigned long reloc;
 {
	struct rt_private_map *nlm, *first_lm;
	char *symname;
	Elf32_Rel *rptr;
	Elf32_Sym *sym, *nsym;
	unsigned long value;
	unsigned long *got_addr;

	DPRINTF((LIST|DRELOC),(2, "rtld: _binder(0x%x, 0x%x)\n", reloc, lm));

	if (!lm) {
		_rtfprintf(2, "%s: %s: unidentifiable procedure reference\n",(char*) _rt_name,_proc_name);
		(void)_rtkill(_rtgetpid(), SIGKILL);
	}
	
	/* use relocation entry to get symbol table entry and symbol name */
	first_lm = TEST_FLAG(lm, RT_SYMBOLIC) ? lm : 0;
	rptr = (Elf32_Rel *)((char *)JMPREL(lm) + reloc);
	sym = (Elf32_Sym *)((unsigned long)SYMTAB(lm) + 
		(ELF32_R_SYM(rptr->r_info) * SYMENT(lm)));
	symname = (char *)(STRTAB(lm) + sym->st_name);

	STDLOCK(&_rtld_lock); 

	/* find definition for symbol */
	if ((nsym = _lookup(symname, first_lm, _ld_loaded, lm,
		&nlm, LOOKUP_NORM)) ==
		(Elf32_Sym *)0) {
		_rtfprintf(2, "%s: %s: symbol not found: %s\n",(char*) _rt_name,_proc_name,symname);
		(void)_rtkill(_rtgetpid(), SIGKILL);
	}
	
	STDUNLOCK(&_rtld_lock); 

	/* get definition address and rebuild PLT entry */
	value = nsym->st_value;
	if (NAME(nlm))
		value += ADDR(nlm);

	DPRINTF(DRELOC,(2, "rtld: relocating function %s to 0x%x\n",
		symname, value));

	got_addr = (unsigned long *)rptr->r_offset;
	if (NAME(lm))
		got_addr = (unsigned long *)((unsigned long)got_addr + ADDR(lm));

	*got_addr = value;
	DPRINTF(DRELOC,(2, "got_addr = 0x%x, *got_addr = 0x%x\n", got_addr, *got_addr));
	if (lm != nlm) 
	{
		/* add to list of referenced objects */
		if (!_rt_add_ref(lm, nlm)) 
		{
			_rtfprintf(2, "%s: %s: %s\n", _rt_name,
				_proc_name, _rt_error);
			(void)_rtkill(_rtgetpid(), SIGKILL);
		}
	}
	return(value);
}
