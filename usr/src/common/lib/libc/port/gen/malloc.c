/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/malloc.c	1.44"
/**********************************************************************
	Memory management: malloc(), realloc(), free(),
				mallinfo(), memalign()

	The following #-parameters may be redefined:
	SEGMENTED: if defined, memory requests are assumed to be
		non-contiguous across calls of GETCORE's.
	GETCORE: a function to get more core memory. If not SEGMENTED,
		GETCORE(0) is assumed to return the next available
		address. Default is 'sbrk'.
	ERRCORE: the error code as returned by GETCORE.
		Default is ((char *)(-1)).
	CORESIZE: a desired unit (measured in bytes) to be used
		with GETCORE. Default is (1024*ALIGN).

	This algorithm is based on a  best fit strategy with lists of
	free elts maintained in a self-adjusting binary tree. Each list
	contains all elts of the same size. The tree is ordered by size.
	For results on self-adjusting trees, see the paper:
		Self-Adjusting Binary Trees,
		DD Sleator & RE Tarjan, JACM 1985.

	The header of a block contains the size of the data part in bytes.
	Since the size of a block is 0%4, the low two bits of the header
	are free and used as follows:

		BIT0:	1 for busy (block is in use), 0 for free.
		BIT1:	if the block is busy, this bit is 1 if the
			preceding block in contiguous memory is free.
			Otherwise, it is always 0.
**********************************************************************/

#ifdef __STDC__
	#pragma weak mallinfo = _mallinfo
	#pragma weak memalign = _memalign
#endif

#include "synonyms.h"
#include "stdlock.h"
#include "mallint.h"
#include <errno.h>
#include <unistd.h>

static TREE	*Root,		/* root of the free tree */
		*Bottom,	/* the last free chunk in the arena */
		*_morecore();	/* function to get more core */

static char	*Arena;		/* start of malloc arena */
static char	*Baddr;		/* current high address of the arena */
static char	*Lfree;		/* last freed block with data intact */

static void	t_delete();
static void	t_splay();
static void	realfree();
static void	cleanfree();

#define FREESIZE	(1<<5) /* size for preserving free blocks until next malloc */
#define FREEMASK	FREESIZE-1

static VOID	*flist[FREESIZE]; /* list of blocks to be freed on next malloc */
static int	freeidx;	  /* index of free blocks in flist % FREESIZE */
static int	freecount;	  /* number blox pending cleanfree in flist */

#ifdef _REENTRANT
static StdLock	__malloc_lock;
#endif

#ifdef _REENTRANT
static VOID	*_real_malloc(size_t);
static void	_real_free(VOID *);
#define REAL_MALLOC	_real_malloc
#define REAL_FREE	_real_free
#else
#define REAL_MALLOC     malloc
#define REAL_FREE	free
#endif

/* number of small blocks to get at one time */
#define MINNPS	((MINSIZE - WORDSIZE) / (sizeof(WORD) + WORDSIZE))
#define NPS 	((sizeof(WORD)*8) < MINNPS ? MINNPS : sizeof(WORD)*8)

static TREE	*List[MINSIZE/sizeof(WORD)-1]; /* lists of small blocks */
static TREE	*Clist[MINSIZE/sizeof(WORD)-1]; /* container list of small blocks */
static int 
#ifdef __STDC__
freesmall(void)
#else
freesmall()
#endif
{
int i, j, k, ret = 0;
TREE **ocp = 0;
TREE *cp, *tp, *list_top;

	for (i = 0; i < MINSIZE/sizeof(WORD)-1; i++) {
		size_t sz;
		ocp = &Clist[i];
		cp = Clist[i];
		list_top = List[i] = 0;
		if (cp == 0)
			continue;
		sz = BLOCKSIZE(FIRST(cp));
		do {
			k = j = NPS;
			tp = FIRST(cp);
			do {
				if (ISBIT0(SIZE(tp)) == 0){	/* free smblk */
					k--;
					AFTER(tp) = list_top;
					list_top = tp;
				}
				tp = (TREE *)((char *)tp + sz);
			} while (--j > 0);

			if ( k == 0){
				*ocp = CHAIN(cp);
				realfree(cp);
				list_top = List[i];
				ret++;
			}
			else 
				ocp = &CHAIN(cp);
				List[i] = list_top; /*
						     * connect to last
						     * in-use container
						     */
		} while ((cp = *ocp) != 0);
	}
	return ret;
}

#ifdef _REENTRANT
VOID *  malloc(size)
size_t		size;
{

	register VOID *retptr;

	STDLOCK(&__malloc_lock);
	retptr =  REAL_MALLOC(size);
	PRINT(buf, "malloc size = %d addr = 0x%p\n", size, retptr)
	STDUNLOCK(&__malloc_lock);
	return retptr;
}

VOID *	REAL_MALLOC(size)
#else
VOID *  malloc(size)
#endif
size_t		size;
	{
	reg size_t	n;
	reg TREE	*tp, *sp;
	reg size_t	o_bit1;

	/**/ COUNT(nmalloc);

	/* make sure that size is 0 mod WORDSIZE */

	ROUND(size);

	/* small blocks */
	if(size < MINSIZE)
		goto _smalloc;

	/* see if the last free block can be used */
	if (Lfree)
		{
		sp = BLOCK(Lfree);
		n = SIZE(sp);
		CLRBITS01(n);
                if (n >= size)
                        {
                        freeidx = (freeidx + FREESIZE - 1) & FREEMASK; /* one ba
ck */
                        flist[freeidx] = NULL;
                        Lfree = flist[(freeidx + FREESIZE - 1) & FREEMASK];
                        freecount--;
                        if (n == size)
                                return DATA(sp);
                        else
                                {
                                /* got a big enough piece */
                                o_bit1 = SIZE(sp) & BIT1;
                                SIZE(sp) = n;
                                goto leftover;
                                }
                        }

		}
	o_bit1 = 0;

	 /*perform free of fast queue, hopefully we'll put on
	  *the tree an element of the proper size and save the
	  *call to _morecore
	  */
  	if (freecount > 0)
		cleanfree(NULL);

	/* search for an elt of the right size */
	sp = NULL;
	n  = 0;

	if(Root)
		{
		tp = Root;

		while(1)
			{
			/* branch left */
			if(SIZE(tp) >= size)
				{
				if(n == 0 || n >= SIZE(tp))
					{
					sp = tp;
					n = SIZE(tp);
					}
				if(LEFT(tp))
					tp = LEFT(tp);
				else	break;
				}
			else	{ /* branch right */
				if(RIGHT(tp))
					tp = RIGHT(tp);
				else	break;
				}
			}

		if(sp)	{
			t_delete(sp);
			}
		else if(tp != Root)
			{
			/* make the searched-to element the root */
			t_splay(tp);
			Root = tp;
			}
		}

	/* if found none fitted in the tree */
	if(!sp)	{
		if(Bottom && size <= SIZE(Bottom))
			sp = Bottom;
		else if((sp = _morecore(size)) == NULL) { /* no more memory */
			if (freesmall() == 0)
				return NULL;
			else
				return REAL_MALLOC(size);
		}
	}

	/* tell the forward neighbor that we're busy */
	CLRBIT1(SIZE(NEXT(sp)));

	/**/ ASSERT(ISBIT0(SIZE(NEXT(sp))));


leftover:
	/* if the leftover is enough for a new free piece */
	if((n = (SIZE(sp) - size)) >= MINSIZE + WORDSIZE)
		{
		n -= WORDSIZE;
		SIZE(sp) = size;
		tp = NEXT(sp);
		SIZE(tp) = n|BIT0;
		realfree(DATA(tp));
		}
	else if(BOTTOM(sp))
		Bottom = NULL;

	/* return the allocated space */
	SIZE(sp) |= BIT0 | o_bit1;
	return DATA(sp);

/*
**	Allocation of small blocks
*/

_smalloc:;
	/**/ ASSERT(size%sizeof(WORD) == 0);
	/* want to return a unique pointer on malloc(0) */
	if(size == 0)
		size = sizeof(WORD);

	/* list to use */
	n = size/sizeof(WORD) - 1;

	if(List[n] == NULL)
		{
		reg int j;

		if (freecount > 0)
		{
			cleanfree(0);
			goto _smalloc;
		}

/**/ ASSERT((size+WORDSIZE)*NPS >= MINSIZE);
		/* get NPS of these block types */
		if((sp = (TREE *) REAL_MALLOC(((size+WORDSIZE)*NPS) + WORDSIZE)) == NULL)
			return NULL;

		/* set up Container list */
		CHAIN(sp) = Clist[n];
		Clist[n] = sp;

		List[n] = sp = FIRST(sp); /* real start of data */

		/* make them into a link list */
		for(j = 0; j < NPS; ++j)
			{
			tp = sp;
			SIZE(tp) = size;
			sp = NEXT(tp);
			AFTER(tp) = sp;
			}
		AFTER(tp) = NULL;
		}

	/* allocate from the head of the queue */
	tp = List[n];
	List[n] = AFTER(tp);
	SETBIT0(SIZE(tp));
	return DATA(tp);
	}


/*
**	realloc().
**	If the block size is increasing, we try forward merging first.
**	This is not best-fit but it avoids some data recopying.
*/
VOID *	realloc(old,size)
VOID		*old;
size_t 		size;
	{
	reg TREE	*tp, *np;
	reg size_t	ts;
	reg char	*new;

	VOID 		*retptr = NULL;

	/**/ COUNT(nrealloc);

	/* pointer to the block */
	if(old == NULL)
	{
		PRINT(buf, "realloc size = %d addr = 0x%p\n", size,old)
		return malloc(size);
	}
	
	STDLOCK(&__malloc_lock);

	/* perform free's of space since last malloc */
	if (freecount > 0)
		cleanfree(old);

	/* make sure that size is 0 mod WORDSIZE */
	ROUND(size);

	tp = BLOCK(old);
	ts = SIZE(tp);

	/* if the block was freed, data has been destroyed. */
	if(!ISBIT0(ts)) {
		goto unlock;	/* return retptr = NULL as initialized */
	}

	/* nothing to do */
	CLRBITS01(SIZE(tp));
	if(size == SIZE(tp))
		{
		SIZE(tp) = ts;
		retptr = old;
		goto unlock;
		}

	/* special cases involving small blocks */
	if(size < MINSIZE || SIZE(tp) < MINSIZE)
		goto call_malloc;

	/* block is increasing in size, try merging the next block */
	if(size > SIZE(tp))
		{
		np = NEXT(tp);
		if(!ISBIT0(SIZE(np)))
			{
			/**/ ASSERT(SIZE(np) >= MINSIZE);
			/**/ ASSERT(!ISBIT1(SIZE(np)));
			SIZE(tp) += SIZE(np)+WORDSIZE;
			if(np != Bottom)
				t_delete(np);
			else	Bottom = NULL;
			CLRBIT1(SIZE(NEXT(np)));
			}

#ifndef SEGMENTED
		/* not enough & at TRUE end of memory, try extending core */
		if(size > SIZE(tp) && BOTTOM(tp) && GETCORE(0) == Baddr)
		{
			Bottom = tp;
			if((tp = _morecore(size)) == NULL)
			{
				tp = Bottom;
				Bottom = 0;
			}
		}
#endif /*!SEGMENTED*/
		}

	/* got enough space to use */
	if(size <= SIZE(tp))
		{
		reg size_t n;

chop_big:;
		if((n = (SIZE(tp) - size)) >= MINSIZE + WORDSIZE)
			{
			n -= WORDSIZE;
			SIZE(tp) = size;
			np = NEXT(tp);
			SIZE(np) = n|BIT0;
			realfree(DATA(np));
			}
		else if(BOTTOM(tp))
			Bottom = NULL;

		/* the previous block may be free */
		SETOLD01(SIZE(tp), ts);
		retptr = old;
		goto unlock;
		}

	/* call malloc to get a new block */
call_malloc:;
		SETOLD01(SIZE(tp), ts);
		if((new = REAL_MALLOC(size)) != NULL)
			{
			CLRBITS01(ts);
			if(ts > size)
				ts = size;
			MEMCOPY(new, old, ts);
			REAL_FREE(old);
			retptr = new;
			goto unlock;
			}

		/*
		** Attempt special case recovery allocations since malloc() failed:
		**
		** 1. size <= SIZE(tp) < MINSIZE
		**	Simply return the existing block
		** 2. SIZE(tp) < size < MINSIZE
		**	malloc() may have failed to allocate the chunk of
		**	small blocks. Try asking for MINSIZE bytes.
		** 3. size < MINSIZE <= SIZE(tp)
		**	malloc() may have failed as with 2.  Change to
		**	MINSIZE allocation which is taken from the beginning
		**	of the current block.
		** 4. MINSIZE <= SIZE(tp) < size
		**	If the previous block is free and the combination of
		**	these two blocks has at least size bytes, then merge
		**	the two blocks copying the existing contents backwards.
		*/

		CLRBITS01(SIZE(tp));
		if(SIZE(tp) < MINSIZE)
			{
			if(size < SIZE(tp))		/* case 1. */
				{
				SETOLD01(SIZE(tp), ts);
				retptr = old;
				goto unlock;
				}
			else if(size < MINSIZE)		/* case 2. */
				{
				size = MINSIZE;
				goto call_malloc;
				}
			}
		else if(size < MINSIZE)			/* case 3. */
			{
			size = MINSIZE;
			goto chop_big;
			}
		else if(ISBIT1(ts) && (SIZE(np=LAST(tp))+SIZE(tp)+WORDSIZE) >= size)
			{
			/**/ ASSERT(!ISBIT0(SIZE(np)));
			t_delete(np);
			SIZE(np) += SIZE(tp) + WORDSIZE;
			/*
			** Since the copy may overlap, use memmove() if available.
			** Otherwise, copy by hand.
			*/
#ifdef __STDC__
			(void)memmove(DATA(np), old, SIZE(tp));
#else
			{
			reg WORD *src = (WORD *)old;
			reg WORD *dst = (WORD *)DATA(np);
			reg size_t  n = SIZE(tp) / WORDSIZE;

			do	{
				*dst++ = *src++;
				} while (--n > 0);
			}
#endif
			old = DATA(np);
			tp = np;
			CLRBIT1(ts);
			goto chop_big;
			}
		SETOLD01(SIZE(tp), ts);
		
		/* return NULL */
unlock:
		PRINT(buf, "realloc size = %d addr = 0x%p\n", size,retptr)
		STDUNLOCK(&__malloc_lock);
		return retptr;
	}



/*
**	realfree().
**	Coalescing of adjacent free blocks is done first.
**	Then, the new free block is leaf-inserted into the free tree
**	without splaying. This strategy does not guarantee the amortized
**	O(nlogn) behaviour for the insert/delete/find set of operations
**	on the tree. In practice, however, free is much more infrequent
**	than malloc/realloc and the tree searches performed by these
**	functions adequately keep the tree in balance.
*/
static void	realfree(old)
VOID		*old;
	{
	reg TREE	*tp, *sp, *np;
	reg size_t	ts, size;

	/**/ COUNT(nfree);

	/* pointer to the block */
	tp = BLOCK(old);
	ts = SIZE(tp);
	if(!ISBIT0(ts))
		return;
	CLRBITS01(SIZE(tp));

	/* small block, put it in the right linked list */
	if(SIZE(tp) < MINSIZE)
		{
		/**/ ASSERT(SIZE(tp)/sizeof(WORD) >= 1);
		ts = SIZE(tp)/sizeof(WORD) - 1;
		AFTER(tp) = List[ts];
		List[ts] = tp;
		return;
		}

	/* see if coalescing with next block is warranted */
	np = NEXT(tp);
	if(!ISBIT0(SIZE(np)))
		{
		if(np != Bottom)
			t_delete(np);
		SIZE(tp) += SIZE(np)+WORDSIZE;
		}

	/* the same with the preceding block */
	if(ISBIT1(ts))
		{
		np = LAST(tp);
		/**/ ASSERT(!ISBIT0(SIZE(np)));
		/**/ ASSERT(np != Bottom);
		t_delete(np);
		SIZE(np) += SIZE(tp)+WORDSIZE;
		tp = np;
		}

	/* initialize tree info */
	PARENT(tp) = LEFT(tp) = RIGHT(tp) = LINKFOR(tp) = NULL;

	/* the last word of the block contains self's address */
	*(SELFP(tp)) = tp;

	/* set bottom block, or insert in the free tree */
	if(BOTTOM(tp))
		Bottom = tp;
	else	{
		/* search for the place to insert */
		if(Root)
			{
			size = SIZE(tp);
			np = Root;
			while(1)
				{
				if(SIZE(np) > size)
					{
					if(LEFT(np))
						np = LEFT(np);
					else	{
						LEFT(np) = tp;
						PARENT(tp) = np;
						break;
						}
					}
				else if(SIZE(np) < size)
					{
					if(RIGHT(np))
						np = RIGHT(np);
					else	{
						RIGHT(np) = tp;
						PARENT(tp) = np;
						break;
						}
					}
				else	{
					if((sp = PARENT(np)) != NULL)
						{
						if(np == LEFT(sp))
							LEFT(sp) = tp;
						else	RIGHT(sp) = tp;
						PARENT(tp) = sp;
						}
					else	Root = tp;

					/* insert to head of list */
					if((sp = LEFT(np)) != NULL)
						PARENT(sp) = tp;
					LEFT(tp) = sp;

					if((sp = RIGHT(np)) != NULL)
						PARENT(sp) = tp;
					RIGHT(tp) = sp;

					/* doubly link list */
					LINKFOR(tp) = np;
					LINKBAK(np) = tp;
					SETNOTREE(np);

					break;
					}
				}
			}
		else	Root = tp;
		}

	/* tell next block that this one is free */
	SETBIT1(SIZE(NEXT(tp)));

	/**/ ASSERT(ISBIT0(SIZE(NEXT(tp))));

	return;
	}


/*
**	Get more core. Gaps in memory are noted as busy blocks.
*/

static TREE *
_morecore(size_t size)
{
	size_t reqsz;
	char *addr;
	TREE *tp;

	reqsz = ((size + 3 * WORDSIZE - 1) / CORESIZE + 1) * CORESIZE;
	if (reqsz < size) /* overflow */
	{
	err:;
		errno = ENOMEM;
		return 0;
	}
	tp = Bottom;
	/*
	* Keep trying to allocate space, with smaller and smaller chunks.
	*/
	while ((addr = GETCORE(reqsz)) == ERRCORE)
	{
		size_t sz, nsz;

		if (GETCORE(0) != Baddr)
			sz = size + 3 * WORDSIZE;
		else
		{
			sz = size + WORDSIZE; /* reuse gap header */
			if (tp != 0)
				sz -= SIZE(tp); /* reuse Bottom */
			nsz = ((sz - 1) / CORESIZE + 1) * CORESIZE;
			if (nsz < reqsz)
			{
				reqsz = nsz;
				continue;
			}
		}
		if ((nsz = sysconf(_SC_PAGESIZE)) != 0
			&& (nsz = ((sz - 1) / nsz + 1) * nsz) < reqsz)
		{
			reqsz = nsz;
			continue;
		}
		if (sz < reqsz)
		{
			reqsz = sz;
			continue;
		}
		return 0; /* cannot allocate sufficient space */
	}
	if (addr == Baddr) /* contiguous, must be aligned */
	{
		Baddr += reqsz;
		if (tp != 0)
			SIZE(tp) += reqsz; /* full allocation usable */
		else
		{
			tp = (TREE *)(addr - WORDSIZE); /* reuse gap header */
			Bottom = tp;
			SIZE(tp) = reqsz - WORDSIZE; /* new gap header */
		}
	}
	else /* first time or discontigous allocation */
	{
		int offset;

		if ((offset = (size_t)addr % WORDSIZE) != 0) /* misaligned */
		{
			addr += offset;
			reqsz -= offset;
		}
		if (Baddr != 0) /* fill in gap header size */
			SIZE((TREE *)(Baddr - WORDSIZE)) = (addr - Baddr) | BIT0;
		else /* first time */
			Arena = addr;
		Baddr = addr + reqsz;
		if (tp != 0) /* place old Bottom into free tree */
		{
			SETBIT0(SIZE(tp));
			realfree(DATA(tp));
		}
		tp = (TREE *)addr;
		reqsz -= 2 * WORDSIZE; /* block and gap header */
		SIZE(tp) = reqsz;
		if (reqsz < size) /* retry didn't get enough space! */
			goto err;
	}
	SIZE((TREE *)(Baddr - WORDSIZE)) = BIT0; /* mark gap as busy */
	return tp;
}

/*
**	Tree rotation functions (BU: bottom-up, TD: top-down)
*/

#define LEFT1(x,y)	if((RIGHT(x) = LEFT(y))) PARENT(RIGHT(x)) = x;\
			if((PARENT(y) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(y)) = y;\
				else RIGHT(PARENT(y)) = y;\
			LEFT(y) = x; PARENT(x) = y

#define RIGHT1(x,y)	if((LEFT(x) = RIGHT(y))) PARENT(LEFT(x)) = x;\
			if((PARENT(y) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(y)) = y;\
				else RIGHT(PARENT(y)) = y;\
			RIGHT(y) = x; PARENT(x) = y

#define BULEFT2(x,y,z)	if((RIGHT(x) = LEFT(y))) PARENT(RIGHT(x)) = x;\
			if((RIGHT(y) = LEFT(z))) PARENT(RIGHT(y)) = y;\
			if((PARENT(z) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(z)) = z;\
				else RIGHT(PARENT(z)) = z;\
			LEFT(z) = y; PARENT(y) = z; LEFT(y) = x; PARENT(x) = y

#define BURIGHT2(x,y,z)	if((LEFT(x) = RIGHT(y))) PARENT(LEFT(x)) = x;\
			if((LEFT(y) = RIGHT(z))) PARENT(LEFT(y)) = y;\
			if((PARENT(z) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(z)) = z;\
				else RIGHT(PARENT(z)) = z;\
			RIGHT(z) = y; PARENT(y) = z; RIGHT(y) = x; PARENT(x) = y

#define TDLEFT2(x,y,z)	if((RIGHT(y) = LEFT(z))) PARENT(RIGHT(y)) = y;\
			if((PARENT(z) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(z)) = z;\
				else RIGHT(PARENT(z)) = z;\
			PARENT(x) = z; LEFT(z) = x;

#define TDRIGHT2(x,y,z)	if((LEFT(y) = RIGHT(z))) PARENT(LEFT(y)) = y;\
			if((PARENT(z) = PARENT(x)))\
				if(LEFT(PARENT(x)) == x) LEFT(PARENT(z)) = z;\
				else RIGHT(PARENT(z)) = z;\
			PARENT(x) = z; RIGHT(z) = x;



/*
**	Delete a tree element
*/
static void	t_delete(op)
reg TREE	*op;
	{
	reg TREE	*tp, *sp, *gp;

	/* if this is a non-tree node */
	if(ISNOTREE(op))
		{
		tp = LINKBAK(op);
		if((sp = LINKFOR(op)) != NULL)
			LINKBAK(sp) = tp;
		LINKFOR(tp) = sp;
		return;
		}

	/* make op the root of the tree */
	if(PARENT(op))
		t_splay(op);

	/* if this is the start of a list */
	if((tp = LINKFOR(op)) != NULL)
		{
		PARENT(tp) = NULL;
		if((sp = LEFT(op)) != NULL)
			PARENT(sp) = tp;
		LEFT(tp) = sp;

		if((sp = RIGHT(op)) != NULL)
			PARENT(sp) = tp;
		RIGHT(tp) = sp;

		Root = tp;
		return;
		}

	/* if op has a non-null left subtree */
	if((tp = LEFT(op)) != NULL )
		{
		PARENT(tp) = NULL;

		if(RIGHT(op))
			{
			/* make the right-end of the left subtree its root */
			while((sp = RIGHT(tp)) != NULL)
				{
				if((gp = RIGHT(sp)) != NULL)
					{
					TDLEFT2(tp,sp,gp);
					tp = gp;
					}
				else	{
					LEFT1(tp,sp);
					tp = sp;
					}
				}

			/* hook the right subtree of op to the above elt */
			RIGHT(tp) = RIGHT(op);
			PARENT(RIGHT(tp)) = tp;
			}
		}

	/* no left subtree */
	else if((tp = RIGHT(op)) != NULL)
		PARENT(tp) = NULL;

	Root = tp;
	}



/*
**	Bottom up splaying (simple version).
**	The basic idea is to roughly cut in half the
**	path from Root to tp and make tp the new root.
*/
static void	t_splay(tp)
reg TREE	*tp;
	{
	reg TREE	*pp, *gp;

	/* iterate until tp is the root */
	while((pp = PARENT(tp)) != NULL)
		{
		/* grandparent of tp */
		gp = PARENT(pp);

		/* x is a left child */
		if(LEFT(pp) == tp)	
			{
			if(gp && LEFT(gp) == pp)
				{
				BURIGHT2(gp,pp,tp);
				}
			else	{
				RIGHT1(pp,tp);
				}
			}
		else	{
			/**/ ASSERT(RIGHT(pp) == tp);
			if(gp && RIGHT(gp) == pp)
				{
				BULEFT2(gp,pp,tp);
				}
			else	{
				LEFT1(pp,tp);
				}
			}
		}
	} 



/*
**	free().
**	Performs a delayed free of the block pointed to
**	by old. The pointer to old is saved on a list, flist,
**	until the next malloc or realloc. At that time, all the
**	blocks pointed to in flist are actually freed via
**	realfree(). This allows the contents of free blocks to
**	remain undisturbed until the next malloc or realloc.
*/
#ifdef _REENTRANT
void	free(old)
VOID	*old;
{
	STDLOCK(&__malloc_lock);
	PRINT(buf, "free size = %d addr = 0x%p\n", SIZE(BLOCK(old)), old)
	REAL_FREE(old);
	STDUNLOCK(&__malloc_lock);
}
void
REAL_FREE(old)
VOID	*old;
#else
void    free(old)
VOID    *old;
#endif
	{
	char **flp;

	if(old == NULL)
		return;
					/* guarantee uniqueness */
	flp = (char **)&(flist[freeidx]);
	if(*flp != NULL)		/* list is full */
		{
		if(*flp == old)
			return;
		realfree(*flp);
		freecount--;
		*flp = NULL;
		}
	for (;;)
		{
		if(flp == (char **)&(flist[0]))
			flp = (char **)&flist[FREESIZE];
		if(*--flp == 0)
			break;
		if(*flp == old)
			return;
		}

	flist[freeidx] = Lfree = old;
	freeidx = (freeidx + 1) & FREEMASK; /* one forward */
	freecount++;
	}



/*
**	cleanfree() frees all the blocks pointed to by flist.
**	
**	realloc() should work if it is called with a pointer
**	to a block that was freed since the last call to malloc() or
**	realloc(). If cleanfree() is called from realloc(), ptr
**	is set to the old block and that block should not be
**	freed since it is actually being reallocated.
*/
static void	cleanfree(ptr)
VOID	*ptr;
	{
	reg char	**flp;
        size_t ts;

	flp = (char **)&(flist[freeidx]);
	for (;;)
		{
		if (flp == (char **)&(flist[0]))
			flp = (char **)&(flist[FREESIZE]);
		if (*--flp == NULL)
			break;
		if (*flp != ptr)
			{
			TREE *tp;

			tp = BLOCK(*flp);
        		ts = SIZE(tp);
			if (ISBIT0(ts))
				{
				CLRBITS01(ts);
				if (ts < MINSIZE)
					{
					SIZE(tp) = ts;
					/**/ ASSERT(ts/sizeof(WORD) >= 1);
					ts = ts/sizeof(WORD) - 1;
					AFTER(tp) = List[ts];
					List[ts] = tp;
					}
				else
					realfree(*flp);
				}
			}
		*flp = NULL;
		}
	freeidx = 0;
	Lfree = NULL;
	freecount = 0;
	}

struct mallinfo
#ifdef __STDC__
mallinfo(void)
#else
mallinfo()
#endif
{
int i;
TREE *next;
struct mallinfo minf;
size_t sz;

	memset(&minf, 0, sizeof(struct mallinfo));

	if (Baddr == 0)
		return minf;

	minf.arena = (int) (Baddr - Arena);

	/*  Something in the arena	*/
	next = (TREE *)Arena;
	do {
		sz = BLOCKSIZE(next);
		if (SIZE((TREE *)next) & BIT0)
			minf.uordblks += sz;
		else
			minf.fordblks += sz;
		minf.ordblks++;
	} while ((next = (TREE *)((char *)next + sz)) < (TREE *)Baddr);

	for (i = 0; i < MINSIZE/sizeof(WORD)-1; i++)
	{
		TREE *stp;

		if ((stp = Clist[i]) == 0)
			continue;

		next = FIRST(stp);
		sz = BLOCKSIZE(next);
		for ( ; stp != 0; stp = CHAIN(stp))
		{
			int j = NPS;
			minf.smblks += NPS;
			minf.hblks++;
			minf.hblkhd += sizeof(TREE *) + WORDSIZE;
			do {
				if (SIZE(next) & BIT0)
					minf.usmblks += sz;
				else
					minf.fsmblks += sz;
				next = (TREE *)((char *)next + sz);
			} while (--j > 0);
		}
	}
	return minf;
}

/*
 * memalign(align,nbytes)
 *
 * Description:
 *	Returns a block of specified size on a specified alignment boundary.
 *
 * Algorithm:
 *	Malloc enough to ensure that a block can be aligned correctly.
 *	Find the alignment point and return the fragments
 *	before and after the block.
 *
 * Errors:
 *	Returns NULL and sets errno as follows:
 *	[EINVAL]
 *		if nbytes = 0,
 *		or if alignment is misaligned,
 *	 	or if the heap has been detectably corrupted.
 *	[ENOMEM]
 *		if the requested memory could not be allocated.
 */

VOID *
memalign(align, nbytes)
	size_t	align;
	size_t	nbytes;
{
	size_t	 reqsize;		/* Num of bytes to get from malloc() */
	register TREE	*p;		/* Ptr returned from malloc() */
	register TREE	*blk;		/* For addressing fragment blocks */
	register size_t	blksize;	/* Current (shrinking) block size */
	register TREE	*alignedp;	/* Ptr to properly aligned boundary */
	register TREE	*aligned_blk;	/* The block to be returned */
	register size_t	frag_size;	/* size of fragments fore and aft */
	size_t	 x;			
	VOID *ret;

	/*
	 * check for valid size and alignment parameters
	 */
	if (nbytes == 0 || _misaligned(align) || align == 0) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * Malloc enough memory to guarantee that the result can be
	 * aligned correctly. The worst case is when malloc returns
	 * a block so close to the next alignment boundary that a
	 * fragment of minimum size cannot be created.
	 */
	ROUND(nbytes);

	if (nbytes < MINSIZE)    /* avoid smalloc/small block free */
		nbytes = MINSIZE;

	reqsize = nbytes + align + MINSIZE;

	STDLOCK(&__malloc_lock);

	p = (TREE *) REAL_MALLOC(reqsize);
	if (p == (TREE *) NULL) {
		ret = NULL;
		goto out;
	}

	/*
	 * get size of the entire block (overhead and all)
	 */
	blk = BLOCK(p);			/* back up to get length word */
	blksize = SIZE(blk);
	CLRBITS01(blksize);

	/*
	 * locate the proper alignment boundary within the block.
	 */
	x = _roundup((size_t)p, align);		/* ccom work-around */
	alignedp = (TREE *)x;
	aligned_blk = BLOCK(alignedp);

	/*
	 * Check out the space to the left of the alignment
	 * boundary, and split off a fragment if necessary.
	 */
	frag_size = (size_t)aligned_blk - (size_t)blk;
	if (frag_size != 0) {
		/*
		 * Create a fragment to the left of the aligned block.
		 */
		if ( frag_size < sizeof(TREE) ) {
			/*
			 * Not enough space. So make the split
			 * at the other end of the alignment unit.
			 */
			frag_size += align;
			aligned_blk = _nextblk(aligned_blk,align);
		}
		blksize -= frag_size;
		SIZE(aligned_blk) = blksize | BIT0;
		frag_size -= WORDSIZE;
		SIZE(blk) = frag_size | (ISBIT1(SIZE(blk)) ? BITS01 : BIT0);
		REAL_FREE(DATA(blk));
	}

	/*
	 * Is there a (sufficiently large) fragment to the
	 * right of the aligned block?
	 */
	frag_size = blksize - nbytes;
	if (frag_size > MINSIZE + WORDSIZE) {
		/*
		 * split and free a fragment on the right
		 */
		blksize = ISBIT1(SIZE(aligned_blk));
		SIZE(aligned_blk) = nbytes;
		blk = NEXT(aligned_blk);
		blksize? SETBITS01(SIZE(aligned_blk)) : SETBIT0(SIZE(aligned_blk));
		SIZE(blk) = (frag_size-WORDSIZE) | BIT0;
		REAL_FREE(DATA(blk));
	}
	ret = DATA(aligned_blk);
out:;
	STDUNLOCK(&__malloc_lock);
	return ret;
}
