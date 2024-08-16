/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/rtld.c	1.28"

/* main run-time linking routines 
 *
 * _rtld may be called either from _rt_setup, to get
 * the process going at startup, or from _dlopen, to
 * load a shared object into memory during execution.
 *
 * We read the interface structure: if we have been passed the path
 * name of some file, we load that file into memory and continue
 * further processing beginning with that file; else we begin processing
 * with the current end of the rt_private_map list.
 *
 * For each shared object listed in the first object's needed list,
 * we open that object and load it into memory.  We then run down
 * the list of loaded objects and perform any needed relocations on each.
 * Finally, we adjust all memory segment protections, invoke the _init
 * routine for any loaded object that has one, and return a pointer
 * to the map structure for the first object we loaded
 *
 * If the environment flag LD_TRACE_LOADED_OBJECTS is set, we load
 * all objects, as above, print out the path name of each, and then exit.
 * If LD_WARN is also set, we also perform relocations, printing out a
 * diagnostic for any unresolved symbol.
 */

#include "rtinc.h"


static struct rt_private_map * so_loaded ARGS((struct namelist *));

static int mapped_rtld;
static Elf32_Dyn lreturn[2];  /* structure used to return values to caller */

#define STREQ(s1,s2)	(!_rtstrcmp(s1,s2))
#define RTNAME		(NAME(_rtld_map))


int _rtld(interface, rt_ret)
Elf32_Dyn *interface;
Elf32_Dyn **rt_ret;
{
	Elf32_Dyn *inttmp[DT_MAXNEGTAGS+1];
	Elf32_Dyn *lneed;
	register struct rt_private_map *lm, *first_loaded;
	char  *pname, *name;
	register int i; 
	int group;
	unsigned int mode;
	unsigned int is_global;

	DPRINTF(LIST,(2, "rtld: _rtld(0x%x, 0x%x)\n",
		(unsigned long)interface, 
		(unsigned long)rt_ret));

	/* create array of pointers to the interface structures
	 * array[tag] points to interface with d_tag == tag
	 */
	for (i= 0; i <= DT_MAXNEGTAGS; i++)
		inttmp[i] = (Elf32_Dyn *)0;
	while (interface->d_tag != DT_NULL)  {
		int k;
		if (interface->d_tag > 0) {
			_rt_lasterr("%s: %s: internal interface error",(char*) _rt_name,_proc_name);
			return(1);
		}
		k = -interface->d_tag;
		inttmp[k] = interface++;
	}

	/* inform debuggers that we are adding to the rt_private_map */
	_r_debug.r_state = RT_ADD;
	_r_debug_state();

	/* dlopen group - 0 for startup files */
	if (inttmp[- DT_GROUP])
		group = inttmp[- DT_GROUP]->d_un.d_val;
	else
		group = 0;

	/* determine binding mode  - default is lazy binding */
	
	if (inttmp[- DT_MODE])
		mode = inttmp[- DT_MODE]->d_un.d_val;
	else 
		mode = RTLD_LAZY;

	is_global = ((mode & RTLD_GLOBAL) != 0);

	/* if interface contains pathname, map in that file */

	if (inttmp[- DT_FPATH]) {
		struct namelist *fnames;
		int  fileCnt, index;
		int  anyLoaded;	/* true if at least one new file is loaded */

		pname = (char *)(inttmp[- DT_FPATH]->d_un.d_ptr);

		/* null pointer an invalid file name */
		if (pname == (char *)0) {
			_rt_lasterr("%s: %s: internal interface error: null pathname specified",(char*) _rt_name,_proc_name);
			return(1);
		}
		anyLoaded = 0;
		if (STREQ(pname,RTNAME)) {
			if (!mapped_rtld) {
				NEXT(_ld_tail) = (struct link_map *)_rtld_map;
				PREV(_rtld_map) = (struct link_map *)_ld_tail;
				_ld_tail = _rtld_map;
				anyLoaded++;
				mapped_rtld = 1;
		 	   	_rt_addset(group, _ld_tail, is_global);
			}
			else
				lm = _rtld_map;
		} else {
			fileCnt = _so_find(pname, &fnames);

			if (fileCnt <= 0) 
				return 1;

			for(index=0; index<fileCnt; index++, fnames++) {
				if ((lm = so_loaded(fnames)) == _rtld_map) {
					if (!mapped_rtld) {
						NEXT(_ld_tail) = (struct link_map *)_rtld_map;
						PREV(_rtld_map) = (struct link_map *)_ld_tail;
						_ld_tail = _rtld_map;
						mapped_rtld = 1;
					}
				} else if (lm == 0) {

					/* map the object in and put
					 * it in the link map
					 */

					struct rt_private_map *lm2;

					lm2 = _map_so(fnames->n_fd, fnames->n_name);
					if (lm2==(struct rt_private_map *)0) {
						return 1;
					}
					INO(lm2) = fnames->n_ino;
					DEV(lm2) = fnames->n_dev;
					NEXT(_ld_tail) = (struct link_map *)lm2;
					PREV(lm2) = (struct link_map *)_ld_tail;
					_ld_tail = lm2;
				}
				else
					continue;
				if (anyLoaded == 0) 
					first_loaded = _ld_tail;
				anyLoaded++;
				_rt_addset(group, _ld_tail, is_global);
			}
		}
		if (anyLoaded == 0) {
			/* no files were loaded;
			 * set up return value
		    	 * struct rt_private_map of loaded object
		    	 */
			lreturn[0].d_tag = DT_MAP;
			lreturn[0].d_un.d_ptr = (Elf32_Addr)lm;
			lreturn[1].d_tag = DT_NULL;
			*rt_ret = &(lreturn[0]);
			return 0;
		}
	} else {
		first_loaded = _ld_tail;
		_rt_addset(group, _ld_tail, is_global);
	}

	/* map in all shared objects needed; start with needed
	 * section of first_loaded and map all those in;
	 * then go through needed sections of all objects just mapped,
	 * etc. result is a breadth first ordering of all needed objects
	 */


	for (lm = first_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		/* process each shared object on needed list */
		for (lneed = (Elf32_Dyn *)DYN(lm); 
			lneed->d_tag!=DT_NULL; lneed++) {
			struct namelist *fnames;
			int  fileCnt, index;
			struct rt_private_map  *lm1;

			if (lneed->d_tag != DT_NEEDED) {
				continue;
		   	}
			name = (char *)STRTAB(lm) + lneed->d_un.d_val;
			if (STREQ(name,RTNAME)) {
				if (!mapped_rtld) {
					NEXT(_ld_tail) = 
					(struct link_map *)_rtld_map;
					PREV(_rtld_map) = 
					(struct link_map *)_ld_tail;
					_ld_tail = _rtld_map;
					mapped_rtld = 1;
				   	if (!_rt_add_needed(lm, 
						_ld_tail)) {
						_rt_cleanup(first_loaded);
						return 1;
			   		}
		 	   		_rt_addset(group, _ld_tail, is_global);
				} else {
			   		if (!_rt_add_needed(lm, _rtld_map)) {
						_rt_cleanup(first_loaded);
						return 1;
			   		}
					_rt_setgroup(group, _rtld_map, is_global);
				}
				continue;
			}
			fileCnt = _so_find(name, &fnames);
			if (fileCnt <= 0) 
				return 1;

			for(index=0; index < fileCnt; index++, fnames++) {
				if (((lm1 = so_loaded(fnames)) == 0) ||
					((lm1 == _rtld_map) && !mapped_rtld))
				{
					if (lm1 == _rtld_map) {
						NEXT(_ld_tail) = (struct link_map *)_rtld_map;
						PREV(_rtld_map) = (struct link_map *)_ld_tail;
						_ld_tail = _rtld_map;
						mapped_rtld = 1;
					} else {
					/* map the object in 
					 * and put it in the
					 * link map
					 */
						lm1 = _map_so(fnames->n_fd, 
							fnames->n_name);
						if (lm1 == (struct rt_private_map *)0) {
							_rt_cleanup(first_loaded);
							return 1;
						}
						INO(lm1) = fnames->n_ino;
						DEV(lm1) = fnames->n_dev;
						NEXT(_ld_tail) = (struct link_map *)lm1;
						PREV(lm1) = (struct link_map *)_ld_tail;
						_ld_tail = lm1;
					}
					if (!_rt_add_needed(lm, _ld_tail)) {
						_rt_cleanup(first_loaded);
						return 1;
					}
					_rt_addset(group, _ld_tail, is_global);
				} else {
					if (!_rt_add_needed(lm, lm1)) {
						_rt_cleanup(first_loaded);
						return 1;
					}
					_rt_setgroup(group, lm1, is_global);
				}
			} /* for(index;;) */
		} /* end for needed */
	} 
	/* if tracing print out names of all objects just added - skip a.out 
	 * we get here only on initial startup - not from dlopen
	 */
	if (_rt_tracing) {
		for (lm = (struct rt_private_map *)NEXT(_ld_loaded); lm; lm = (struct rt_private_map *)NEXT(lm)) {
			_rtfprintf(1, "%s: %s: file loaded: %s\n",(char*) _rt_name,_proc_name,NAME(lm));
		}
		/* if _rt_warn not set, exit */
		if (!_rt_warn)
			_rtexit(0);
	}

	/* for each object just added (except ld.so), call relocate 
	 * to relocate symbols
	 */

	for (lm = first_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		struct rt_private_map *lm2;
	 /*
	 * If the binding mode was set at ld time with -Bbind_now
	 * (if the .dynamic section has DT_BIND_NOW set), then RTLD_NOW 
	 * should be the mode and it should take precedence over all others
	 */
				
		if (!_relocate(lm, 
			( TEST_FLAG(lm, RT_BIND_NOW))? RTLD_NOW: mode))
				if (!_rt_warn) {
					_rt_cleanup(first_loaded);
					return(1);
				}
			/* go through entire list of objects; for
			 * those whose referenced bit has been
			 * set by relocate(), add it to lm's reference
			 * list; then clear bit
			 */
			for (lm2 = _ld_loaded; lm2; lm2 =
				(struct rt_private_map *)NEXT(lm2)) {
				if (TEST_FLAG(lm2, RT_REFERENCED)) {
					if (!_rt_add_ref(lm, lm2)) {
						_rt_cleanup(first_loaded);
						return 1;
					}
					CLEAR_FLAG(lm2, RT_REFERENCED);
				}
			}


	}

	/* perform special copy type relocations */
	for ( ; _rt_copy_entries; _rt_copy_entries = _rt_copy_entries->r_next)
		_rt_memcpy(_rt_copy_entries->r_to, _rt_copy_entries->r_from, _rt_copy_entries->r_size);
		

	/* close /dev/zero */
	if(_devzero_fd != -1){
		(void)_rtclose(_devzero_fd);
		_devzero_fd = -1;
	}

	/* if tracing, exit */
	if (_rt_tracing)
		_rtexit(0);

	/* set correct protections for each segment mapped */
	for (lm = first_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		if (!TEST_FLAG(lm, RT_TEXTREL) || lm == _rtld_map)
			continue;
		if (!_set_protect(lm, 0)) {
			_rt_cleanup(first_loaded);
			return(1);
		}
	}

	/* tell debuggers that the rt_private_map is now in a consistent state */
	_r_debug.r_state = RT_CONSISTENT;
	_r_debug_state();

	/* set up return value  - pointer to first object loaded */
	lreturn[0].d_tag = DT_MAP;
	lreturn[0].d_un.d_ptr = (Elf32_Addr)first_loaded;
	lreturn[1].d_tag = DT_NULL;
	*rt_ret = &(lreturn[0]);
	
	return(0);
}

/* 
**	clean up all attached shared objects in case of error
*/

void _rt_cleanup(lm)
	struct rt_private_map *lm;
{
	Elf32_Phdr *phdr;
	int unmap_later = 0;
	unsigned long phdr_addr, phdr_msize;
	int j;

	if (_rt_nodelete)
		return;

	if (PREV(lm)) {
		NEXT((struct rt_private_map *) PREV(lm)) = 0;
		_ld_tail = (struct rt_private_map *) PREV(lm);
	}

	for (; lm; lm = (struct rt_private_map *) NEXT(lm)) {
		if (TEST_FLAG(lm, RT_NODELETE))
			continue;
		phdr = (Elf32_Phdr *) PHDR(lm);
		for (j = 0; j < (int) PHNUM(lm); j++) {
			unsigned long addr, msize;

			if (phdr->p_type == PT_LOAD) {
				addr = (unsigned long) phdr->p_vaddr +
					ADDR(lm);
				msize = phdr->p_memsz + (addr - PTRUNC(addr));
				if (PTRUNC(addr) <= (unsigned long) phdr &&
					PTRUNC(addr) + msize >=
					(unsigned long) phdr) {
					phdr_addr = addr;
					phdr_msize = msize;
					unmap_later = 1;
				} else {
					(void) _rtmunmap((caddr_t) PTRUNC(addr),
						msize);
				}
			}
			phdr = (Elf32_Phdr *) ((unsigned long) phdr +
				PHSZ(lm));
		}

		/* If the phdr is part of a segment that should be unmapped
		** then we need to wait and unmap it last or we will not be
		** able to read the rest of the entries.
		*/
		if (unmap_later != 0) {
			(void) _rtmunmap((caddr_t) PTRUNC(phdr_addr),
				phdr_msize);
			unmap_later = 0;
		}
	}
}


/* function that determines whether a file
 * has already been loaded; if so, returns a pointer to its
 * link structure; else returns a NULL pointer
 * We check inode and device number, rather than name itself.
 */
static struct rt_private_map *
so_loaded(nameitem)
struct namelist *nameitem;
{
	register struct rt_private_map *lm;

	DPRINTF(LIST,(2, "rtld: so_loaded(%s)\n",nameitem->n_name));

	for (lm = (struct rt_private_map *)NEXT(_ld_loaded); lm; lm = (struct rt_private_map *)NEXT(lm)) 
	{
		if ((DEV(lm) == nameitem->n_dev) &&
			(INO(lm) == nameitem->n_ino)) 
		{
			_rtclose(nameitem->n_fd);
			return(lm);
		}
	}
	if ((DEV(_rtld_map) == nameitem->n_dev) &&
		(INO(_rtld_map) == nameitem->n_ino))
	{
		_rtclose(nameitem->n_fd);
		return _rtld_map;
	}
	return((struct rt_private_map *)0);
}

/* symbol lookup routine - takes symbol name, pointer to a
 * rt_private_map to search first, a list of rt_private_maps to search if that fails.
 * If successful, returns pointer to symbol table entry
 * and to rt_private_map of enclosing object. Else returns a null
 * pointer.
 * 
 * If flag argument is 1, we do treat undefined symbols with type
 * function specially in the a.out - if they have a value, even though
 * undefined, we use that value.  This allows us to associate all references
 * to a function's address to a single place in the process: the plt entry
 * for that function in the a.out.  Calls to lookup from plt binding routines
 * do NOT pass a flag value of 1.
 * An object may reference symbols in a second object's symbol table
 * if the second object does not have the refdeny bit set or both
 * objects belong to the same dlopen group or the referenced
 * object belongs to the global group
 */

Elf32_Sym *_lookup(name, first_lm, lm_list, ref_lm, lm, flag)
CONST char *name;
struct rt_private_map *first_lm;
struct rt_private_map *lm_list;
struct rt_private_map *ref_lm;
struct rt_private_map **lm;
int flag;
{
	register unsigned long hval = 0;
	unsigned long buckets;
	register unsigned long htmp, ndx;
	register struct rt_private_map *nlm;
	Elf32_Sym *sym;
	Elf32_Sym *symtabptr;
	char *strtabptr;
	unsigned long *hashtabptr;

	DPRINTF(LIST,(2, "rtld: _lookup(%s, 0x%x, 0x%x 0x%x 0x%x)\n",name, 
		(unsigned long)first_lm, (unsigned long)lm_list, 
		(unsigned long)ref_lm, (unsigned long)lm));

	/* hash symbol name - use same hash function used by ELF access
	 * library 
	 * the form of the hash table is
	 * |--------------|
	 * | # of buckets |
	 * |--------------|
	 * | # of chains  |
	 * |--------------|
	 * | bucket[]	  |
	 * |   ...	  |
	 * |--------------|
	 * | chain[]	  |
	 * |   ...	  |
	 * |--------------|
	 */
	{
	register CONST char *p;
	register unsigned long g;

	p = name;
	hval = 0;
	while (*p) {
		hval = (hval << 4) + *p++;
		if ((g = (hval & 0xf0000000)) != 0)
			hval ^= g >> 24;
		hval &= ~g;
	}
	}

	if (first_lm) { /*  start search with specified rt_private_map */
		buckets = HASH(first_lm)[0];
		htmp = hval % buckets;
		/* get first symbol on hash chain */
		ndx = HASH(first_lm)[htmp + 2];

		hashtabptr = HASH(first_lm) + 2 + buckets;
		strtabptr = STRTAB(first_lm);
		symtabptr = SYMTAB(first_lm);

		while (ndx) {
			sym = symtabptr + ndx;
			if (_rtstrcmp(strtabptr + sym->st_name, name))  /* names do not match */
				ndx = hashtabptr[ndx];
			else if (sym->st_shndx != SHN_UNDEF) {
					*lm = first_lm;
					return(sym);
				}
			else if (flag == LOOKUP_SPEC && !NAME(first_lm) &&
				sym->st_shndx == SHN_UNDEF &&
				sym->st_value != 0 &&
				(ELF32_ST_TYPE(sym->st_info) == STT_FUNC)) {
					*lm = first_lm;
					return(sym);
				}
			else /* local or undefined - fall
			      * through
			      */
				break; 
		} /* end while */
	} /* first_lm */

	/* go through each rt_private_map and look for symbol */
	for (nlm = lm_list; nlm; nlm = (struct rt_private_map *)NEXT(nlm)) {
		if (nlm != first_lm && 
 			(_rt_ismember(nlm, ref_lm) ||
				_rt_isglobal(nlm))) {
			buckets = HASH(nlm)[0];
			htmp = hval % buckets;
			/* get first symbol on hash chain */
			ndx = HASH(nlm)[htmp + 2];

			hashtabptr = HASH(nlm) + 2 + buckets;
			strtabptr = STRTAB(nlm);
			symtabptr = SYMTAB(nlm);

			while (ndx) {
				sym = symtabptr + ndx;
				if (_rtstrcmp(strtabptr + sym->st_name, name))  /* names do not match */
					ndx = hashtabptr[ndx];
				else if (sym->st_shndx != SHN_UNDEF) {
					*lm = nlm;
					return(sym);
				}
				else if (flag == LOOKUP_SPEC && !NAME(nlm) &&
					sym->st_shndx == SHN_UNDEF &&
					sym->st_value != 0 &&
					(ELF32_ST_TYPE(sym->st_info) == 
						STT_FUNC)) {
						*lm = nlm;
						return(sym);
					}
				else /* local or undefined - fall
				      * through
				      */
					break; 
			} /* end while (ndx) */
		} /* end if access permitted */
	} /* end for */
	/* if here -  no match */
	return((Elf32_Sym *)0);
}
