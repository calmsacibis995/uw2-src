/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/dlfcns.c	1.23"

#include "rtinc.h"

/* return pointer to string describing last occurring error
 * the notion of the last occurring error is cleared
 */

#ifdef __USLC__
#pragma weak dlopen = _dlopen
#pragma weak dlclose = _dlclose
#pragma weak dlsym = _dlsym
#pragma weak dlerror = _dlerror
#endif

#ifdef __USLC__
char *
_dlerror()
#else
char *
dlerror()
#endif
{
	char * etmp;

	DPRINTF(LIST,(2,"rtld: dlerror()\n"));
	etmp = _rt_error;
	_rt_error = (char *)0;

	return(etmp);
}

/* open a shared object - uses rtld to map the object into
 * the process' address space - maintains list of
 * known objects; on success, returns a pointer to the structure
 * containing information about the newly added object;
 * on failure, returns a null pointer
 */

static DLLIB *dl_head;
static DLLIB *dl_tail;

static int dl_delete		ARGS((struct rt_private_map *lm));

#ifdef __USLC__
VOID *
_dlopen(pathname, mode)
#else
VOID *
dlopen(pathname, mode)
#endif
char *pathname;
int mode;
{
	struct rt_private_map *lm, *first_map = 0;
	DLLIB *dlptr = 0;
	Elf32_Dyn *retval;
	Elf32_Dyn interface[4];
	static unsigned long dlgroup;

	/* dlgroups specify NEEDED dependencies - group 0
	 * is reserved for everything loaded at startup
	 * plus any object loaded with RTLD_GLOBAL
	 */
	DPRINTF(LIST,(2,"rtld: dlopen(%s, %d)\n",pathname?pathname:(CONST char *)"0",mode));

	if ( ((mode & ~RTLD_GLOBAL) != RTLD_LAZY) &&
	     ((mode & ~RTLD_GLOBAL) != RTLD_NOW ) ) {
		_rt_lasterr("%s: %s: illegal mode to dlopen: 0x%x",(char*) _rt_name,_proc_name,mode);
		return 0;
	}

	STDLOCK(&_rtld_lock); 
	

	if (!dl_head) {  
		/* set up DLLIB structure for main */
		/* mark each object already on struct rt_private_map list as
		 * non-deletable so we do not remove those
		 * objects mapped in on startup
		 */
		for (lm = _ld_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
			SET_FLAG(lm, RT_NODELETE);
		}
		SET_FLAG(_rtld_map, RT_NODELETE);
		if ((dl_head = (DLLIB *)_rtmalloc(sizeof(DLLIB))) == 0) 
			goto out;
		dl_head->dl_group = 0;
		dl_head->dl_object = _ld_loaded;
		dl_tail = dl_head;
		dlgroup = 1;
	}

	if (pathname == 0) {
		dlptr = dl_head;
		dlptr->dl_refcnt++;
		goto out;
	}

	/* map in object if not already mapped */
	interface[0].d_tag = DT_FPATH;
	interface[0].d_un.d_ptr = (Elf32_Addr)pathname;
	interface[1].d_tag = DT_MODE;
	interface[1].d_un.d_val = mode;
	interface[2].d_tag = DT_GROUP;
	interface[2].d_un.d_val = dlgroup;
	interface[3].d_tag = DT_NULL;

	if (_rtld(interface, &retval) != 0) 
		goto out;

	/* find pointer to struct rt_private_map of 
	 * shared object in Elf32_Dyn
	 * structure returned by rtld
	 */
	while(retval->d_tag != DT_NULL) {
		if (retval->d_tag == DT_MAP)
			break;
		retval++;
	}
	if (retval->d_tag == DT_NULL) {
		_rt_lasterr("%s: %s: interface error: bad return value to dlopen",(char*) _rt_name,_proc_name);
		goto out;
	}

	lm = (struct rt_private_map *)(retval->d_un.d_ptr);

	for (dlptr = dl_head; dlptr; 
		dlptr = dlptr->dl_next) {
		if (dlptr->dl_object == lm)
			break;
	}

	/* if we already have a dllib structure for this object,
	 * update it; otherwise create a new one
	 */
	 if (!dlptr) {
		if ((dlptr = (DLLIB *)_rtmalloc(sizeof(DLLIB))) == 0)
			goto out;
		dlptr->dl_object = lm;
		dl_tail->dl_next = dlptr;
		dl_tail = dlptr;
		dlptr->dl_group = dlgroup;
		/* save the first struct rt_private_map to be passed to
		 * _rt_call_init.
		 * The invocation of the _init routines for each object
	  	 * loaded will take place after the lock is released. 
		 * This is to prevent a deadlock that may occur when 
		 * a _init routine results in a call to _binder, 
		 * which also has a lock.
		 */
		first_map = lm;
		if (!_rt_hasgroup(dlgroup, lm))
			/* if lm was loaded as the result of a
			 * different dlopen call, or on startup,
			 * group members need to be set
			 */
			_rt_setgroup(dlgroup, lm, ((mode & RTLD_GLOBAL) != 0));
	}

	if (dlptr->dl_refcnt == 0) {
		dlptr->dl_group = dlgroup; /* set to new group */
		if (!_rt_hasgroup(dlgroup, lm))
			/* if lm was loaded as the result of a
			 * different dlopen call, or on startup,
			 * group members need to be set with new group
			 */
			_rt_setgroup(dlgroup, lm, ((mode & RTLD_GLOBAL) != 0));

		/* first open, increment reference count of 
		 * each object on list 
		 */
		for (lm = _ld_loaded; lm; lm = 
			(struct rt_private_map *)NEXT(lm)) {
			if (_rt_hasgroup(dlgroup, lm))
				COUNT(lm) += 1;
		}
		dlgroup++;
	}

	dlptr->dl_refcnt++;
out:
	/* close /dev/zero */
        if (_devzero_fd != -1) {
                (void)_rtclose(_devzero_fd);
                _devzero_fd = -1;
        }
	STDUNLOCK(&_rtld_lock);

	if (first_map)
		_rt_call_init(first_map, &dlptr->dl_fini);

	return (VOID *)dlptr;
}

/* takes the name of a symbol and a pointer to a dllib structure;
 * search for the symbol in the shared object specified
 * and in all objects in the specified object's needed list
 * returns the address of the symbol if found; else 0
 */

#ifdef __USLC__
VOID *
_dlsym(handle, name)
#else
char *
dlsym(handle, name)
#endif
char *name;
VOID *handle;
{
	struct rt_private_map *lm, *nlm;
	Elf32_Sym *sym = 0;
	unsigned long addr = 0;
	unsigned long group;

	DPRINTF(LIST,(2,"rtld: dlsym(0x%x, %s)\n",handle,name?name:(CONST char *)"0"));

	if (!name) {
		_rt_lasterr("%s: %s: null symbol name to dlsym",(char*) _rt_name,_proc_name);
		return(0);
	}

	STDLOCK(&_rtld_lock); 

	if (((DLLIB *)handle)->dl_refcnt <= 0) {
		_rt_lasterr("%s: %s: dlsym: attempt to find symbol %s in closed object",(char*) _rt_name,_proc_name,name);

		goto out;
	}

	/* for each object on map list, if that object is in the
	 * handle's group, lookup name in that object's 
	 * symbol table
	 */
	group = ((DLLIB *)handle)->dl_group;
	for (lm = _ld_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		if (!_rt_hasgroup(group, lm))
			continue;
		if ((sym = _lookup(name, lm, 0, 0, &nlm,
			LOOKUP_NORM)) != (Elf32_Sym *)0)
			break;
	}

	if (!sym) {
		_rt_lasterr("%s: %s: dlsym: can't find symbol: %s",(char*) _rt_name,_proc_name,name);
		goto out;
	}
	addr = sym->st_value;
	if (NAME(nlm))
		addr += ADDR(nlm);

out:
	/* close /dev/zero */
        if(_devzero_fd != -1) {
                (void)_rtclose(_devzero_fd);
                _devzero_fd = -1;
        }

	STDUNLOCK(&_rtld_lock); 
	return((VOID *)addr);
}

/* close the shared object associated with handle;
 * reference counts are decremented - we check reference
 * counts of all objects in dlopen group; 
 * When an object's reference count goes to 0, it is unmapped;
 * an object's reference count is incremented for:
 *	dlopen group
 *	each time it is a member of an object's needed list
 *	each object implicitly referencing this object.
 * So, an object cannot be unmapped until:
 *	All dlopen groups of which it is a member are closed;
 *	All objects whose needed lists it belongs to have 0 ref counts
 *	All objects implicitly referencing it have 0 ref counts
 * Returns 0 on success, 1 on failure.
 */

#ifdef __USLC__

int 
_dlclose(handle)
#else 
int 
dlclose(handle)
#endif
VOID *handle;
{
	struct rt_private_map	*lm, *group_obj;
	int ret = 0;
	int group;

	DPRINTF(LIST,(2,"rtld: dlclose(0x%x)\n",handle));
	STDLOCK(&_rtld_lock); 

	if (((DLLIB *)handle)->dl_refcnt <= 0) {
		_rt_lasterr("%s: %s: dlclose: attempt to close already closed object",(char*) _rt_name,_proc_name);
		ret = 1;
		goto out;
	}

	if ((--(((DLLIB *)handle)->dl_refcnt) != 0) ||
		(NAME(((DLLIB *)handle)->dl_object) == 0))
		goto out;

	/* last close */
	group = ((DLLIB *)handle)->dl_group;

	/* decrement reference counts of all objects associated
	 * with this object
	 */

	for (lm = _ld_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		if (_rt_hasgroup(group, lm)) {
			COUNT(lm) -= 1;
			_rt_delset(group, lm);
			if (COUNT(lm) == 0) {
				mlist	*l2;
				for(l2 = REFLIST(lm); l2; 
					l2 = l2->l_next)
					COUNT(l2->l_map) -= 1;
				for(l2 = NEEDED(lm); l2; 
					l2 = l2->l_next)
					COUNT(l2->l_map) -= 1;
			}
		}
	}

	/* call fini routines of dlgroup */
	_rt_process_fini(((DLLIB *)handle)->dl_fini, 0);

	/* go through map list - for each entry
	 * whose refcount has gone to 0
	 * delete those members 
	 */
	group_obj  = ((DLLIB *)handle)->dl_object;
	for(lm = _ld_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		if ((COUNT(lm) > 0)  || TEST_FLAG(lm, RT_NODELETE))
			continue;
		if (lm == group_obj)
			((DLLIB *)handle)->dl_object = 0;
		if (!dl_delete(lm)) {
			ret = 1;
			goto out;
		}
	}
out:
	/* close /dev/zero */
        if (_devzero_fd != -1) {
                (void)_rtclose(_devzero_fd);
                _devzero_fd = -1;
        }
	STDUNLOCK(&_rtld_lock);
	return(ret);
}


static int 
dl_delete(lm)
struct rt_private_map *lm;
{
	int j;
	Elf32_Phdr *phdr;
	int unmap_later = 0;
	unsigned long phdr_addr, phdr_msize;

	DPRINTF(LIST,(2,"rtld: dl_delete(0x%x)\n",lm));
	/* alert debuggers that link_map list is shrinking */
	_r_debug.r_state = RT_DELETE;
	_r_debug_state();

	if (!TEST_FLAG(lm, RT_FINI_CALLED))
	{
		/* might not have been called if there
		 * were still references to this object
		 * when its dlgroup was closed
		 */
		void (*fptr)();
		fptr = FINI(lm);
		if (fptr)
			(*fptr)();
	}
	/* unmap each segment */
	phdr = (Elf32_Phdr *)PHDR(lm);
	for (j = 0; j < (int)PHNUM(lm); j++) {
		unsigned long addr, msize;
		if (phdr->p_type == PT_LOAD) {
			addr = (unsigned long)phdr->p_vaddr + ADDR(lm);
			msize = phdr->p_memsz + (addr - PTRUNC(addr));
			if ((PTRUNC(addr) <= (unsigned long) phdr) &&
				(PTRUNC(addr) + msize >= 
				(unsigned long) phdr)) {
				/* If the phdr is part of a segment 
				 * that should be unmapped then we 
				 * need to wait and unmap it last or 
				 * we will not be able to read the rest 
				 * of the entries.
				 */
				phdr_addr = addr;
				phdr_msize = msize;
				unmap_later = 1;
			} else {
				if (_rtmunmap((caddr_t) PTRUNC(addr),
					msize) == -1) {
				/*  ??? or should we continue ??? */
					_rt_lasterr("%s: %s: dlclose: failure unmapping %s",(char*) _rt_name,_proc_name,NAME(lm));
					return(0);
				}
			}
		}
		phdr = (Elf32_Phdr *)((unsigned long)phdr + PHSZ(lm));
	} /* end for phdr loop */
	if (unmap_later != 0) {
		if (_rtmunmap((caddr_t) PTRUNC(phdr_addr),
			phdr_msize) == -1) {
			_rt_lasterr("%s: %s: dlclose: failure unmapping %s",(char*) _rt_name,_proc_name,NAME(lm));
			return(0);
		}
	}
	SET_FLAG(lm, RT_NODELETE);
	/* unlink lm from chain 
	 * we never unlink the
	 * 1st item on the chain
	 * (the a.out)
	 */
	NEXT((struct rt_private_map *)PREV(lm)) = NEXT(lm);
	if (!NEXT(lm))
		_ld_tail = (struct rt_private_map *)PREV(lm);
	else
		PREV((struct rt_private_map *)NEXT(lm)) = PREV(lm);
	
	/* alert debuggers that link_map is consistent again */
	_r_debug.r_state = RT_CONSISTENT;
	_r_debug_state();

	return(1);
}

/* call fini functions for all dlgroups still around at exit */
void
_rt_dl_do_exit()
{
	DLLIB	*dllist;
	for(dllist = dl_head; dllist; dllist = dllist->dl_next) {
		_rt_process_fini(dllist->dl_fini, 1);
	}
}
