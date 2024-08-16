/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/ma.c	1.4.4.3"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * this file contains functions that deal with memory allocation and stacks
 */

#include <libthread.h>
#include <sys/mman.h>
#include <fcntl.h>

int	_thr_alloc_stack(size_t, caddr_t *);
void	_thr_free_stack(caddr_t, int);
int	_thr_alloc_chunk(caddr_t, int, caddr_t *);
void	_thr_free_chunk(caddr_t, int);

/*
 * used to cache the file descriptor returned from open of /dev/zero,
 * which is used to create zero filled pages.
 */
int _thr_devzero = 0;

/*
 * _thr_defaultstkcache\fP is a linked list of default-sized stacks
 * maintained by the library.  When creating a thread with a default-sized
 *  stack, the stack is obtained from this list if the list is not empty.
 * When a thread is reaped, if its stack was a default-sized stack
 * provided by the library, the stack will be added to this stack cache
 * if the number of stacks in the cache does not exceed a defined limit.
 * Stacks are recycled in this way to help minimize
 * the number of system calls needed when creating threads.
 */
struct stkcache {
	int size;
	char *next;
} _thr_defaultstkcache;
lwp_mutex_t _thr_stkcachelock = {0};



/*
 * _thr_alloc_stack(size_t size, caddr_t *sp)
 *	allocate a stack with redzone. stacks of default size are
 *	cached and allocated in increments greater than 1.
 *
 * Parameter/Calling State:
 *	On entry, no locks are held but signal handlers are disabled.
 *
 *	size - size of stack, 0 means default size
 *	sp  - pointer to the location of the stack, null means anywhere is ok.
 *
 *	During processing, _thr_stkcachelock is acquired. 

 * Return Values/Exit State:
 *	returns 1 on success, 0 on failure
 */

int
_thr_alloc_stack(size_t size, caddr_t *sp)
{
	register int i;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(size >= THR_MIN_STACK);

	if (size == DEFAULTSTACK) {
		_lwp_mutex_lock(&_thr_stkcachelock);
		while (_thr_defaultstkcache.next == NULL) {
			_lwp_mutex_unlock(&_thr_stkcachelock);
			/* add redzone */
			size += PAGESIZE;
			if (!_thr_alloc_chunk(0, DEFAULTSTACKINCR*size, sp))
				return (0);
			for(i = 0; i < DEFAULTSTACKINCR; i++) {
				/*
			 	 * invalidate the top stack page.
			 	 */
				if (mprotect(*sp, PAGESIZE, PROT_NONE)) {
					return (0);
				}
				_thr_free_stack(*sp + PAGESIZE, DEFAULTSTACK);
				*sp += (DEFAULTSTACK + PAGESIZE);
			}		
			_lwp_mutex_lock(&_thr_stkcachelock);
		}
		*sp = _thr_defaultstkcache.next;
		_thr_defaultstkcache.next = (caddr_t)(**((long **)sp));
		_thr_defaultstkcache.size -= 1;
		_lwp_mutex_unlock(&_thr_stkcachelock);
		return(1);
	} else {
		/* add redzone */
		size += PAGESIZE;
		if (!_thr_alloc_chunk(0, size, sp))
			return (0);
		/*
		 * invalidate the top stack page.
		 */
		if (mprotect(*sp, PAGESIZE, PROT_NONE)) {
			return (0);
		}
		*sp += PAGESIZE;
		return (1);
	}	
}



/*
 * _thr_free_stack(caddr_t addr, int size)
 *	free up stack space. stacks of default size are cached until some
 *	high water mark and then they are also freed.
 *
 * Parameter/Calling State:
 *	On entry, no locks are held but signal handlers are disabled.
 *
 *	addr - address of the location to be freed
 *	size - amount of space to be freed
 *
 *	During processing, _thr_stkcachelock is acquired if a default size
 *	stack is being returned.
 *
 * Return Values/Exit State:
 *	no return value
 */

void
_thr_free_stack(caddr_t addr, int size)
{
	int rval;
	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(size >= THR_MIN_STACK);
	ASSERT(addr != NULL);

	if (size == DEFAULTSTACK) {
		_lwp_mutex_lock(&_thr_stkcachelock);
		if (_thr_defaultstkcache.size < MAXSTACKS) {
			*(long *)(addr) = (long)_thr_defaultstkcache.next;
			_thr_defaultstkcache.next = addr; 
			_thr_defaultstkcache.size += 1;
			_lwp_mutex_unlock(&_thr_stkcachelock);
			return;
		}
		_lwp_mutex_unlock(&_thr_stkcachelock);
	}
	/* include one page for redzone */
	rval = munmap(addr - PAGESIZE, (unsigned int)size + PAGESIZE);
	ASSERT(rval == 0);
}



/*
 * _thr_alloc_chunk(caddr_t at, int size, caddr_t *cp)
 *	allocate a chunk of /dev/zero memory.
 *
 * Parameter/Calling State:
 *	No locks are held on entry.
 *
 *	at - is always zero in the thread library
 *	size  -  size of the chunk to be allocated
 *	cp - will point to the location of the allocated memory
 *
 *	No locks are acquried during processing.
 *
 * Return Values/Exit State:
 *	returns 1 on success, 0 on failure.
 */

int
_thr_alloc_chunk(caddr_t at, int size, caddr_t *cp)
{
	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(size > 0);

	if (_thr_devzero == 0 &&
	   (_thr_devzero = open("/dev/zero", O_RDWR)) == -1) {
		return (0);
	}
	if ((*cp = mmap(at, (unsigned int)size, PROT_READ|PROT_WRITE|PROT_EXEC, 
			MAP_PRIVATE, _thr_devzero, 0)) == (caddr_t) -1) {
		return (0);
	}
	ASSERT(cp != NULL);
	return(1);
}



/*
 * _thr_free_chunk(caddr_t addr, int size)
 *	free a chunk of allocated /dev/zero memory.
 *
 * Parameter/Calling State:
 *	No locks are held on entry.
 *
 *	addr -  address of the memory to be freed
 *	size -  amount of memory to be freed
 *
 *	No locks are acquired during processing.
 *
 * Return Values/Exit State:
 *	returns no value
 */

void
_thr_free_chunk(caddr_t addr, int size)
{
	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(size > 0);
	ASSERT(addr != NULL);

	(void) munmap(addr, (unsigned int)size); 
}
