/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/rtfcns.c	1.17"

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "rtinc.h"
#include <fcntl.h>
#include <limits.h>

#define ERRSIZE 512	/* size of buffer for error messages */

static int _rtstrncmp ARGS((CONST char *, CONST char *, int));

typedef struct Space	Space;
struct	Space
{
	Space	*s_next;
	size_t	s_size;
	char	*s_ptr;
};

static Space	*space;


/* utility routines for run-time linker
 * some are duplicated here from libc (with different names)
 * to avoid name space collissions
 */

/* null function used as place where a debugger can set a breakpoint */
void _r_debug_state()
{
	struct link_map *lm; 
	struct rt_private_map *lm1;

	if (_rt_event != 0 && _r_debug.r_state == RT_CONSISTENT){
#ifdef DEBUG
	 	DPRINTF(LIST,
			(2,"\nThe link map's state is %d; _rt_event is 0x%x\n", 
				_r_debug.r_state, _rt_event));
	 for (lm=_r_debug.r_map; lm; lm = lm->l_next)
	 	DPRINTF(LIST,
			(2,"\t%s is in the link map\n", lm->l_name));
#endif
		(*_rt_event)((unsigned long)&_r_debug);
	}
	return;
}

/* call initialization routines
 * recursive function - follow siblings first, then children,
 * then current node
 */
static void
do_init(lptr, flist)
mlist *lptr, **flist;
{
	struct rt_private_map	*lm = lptr->l_map;

	if (lptr->l_next)
		do_init(lptr->l_next, flist);
	if (!TEST_FLAG(lm, RT_NEEDED_SEEN)){
		SET_FLAG(lm, RT_NEEDED_SEEN);
		if (NEEDED(lm))
			do_init(NEEDED(lm), flist);
	}
	if (!TEST_FLAG(lm, RT_INIT_CALLED)) {
		mlist	*mptr;
		void (*itmp)();

		if ((mptr = (mlist *)_rtmalloc(sizeof(mlist))) == 0) 
			return;
		SET_FLAG(lm, RT_INIT_CALLED);
		mptr->l_map = lm;
		mptr->l_next = *flist;
		*flist = mptr;
		itmp = INIT(lm);
		if (itmp)
			(*itmp)();
	}
}

/* upper level init - for a.out or dlopen'd lib
 * calls recursive function
 */
void
_rt_call_init(lm, flist)
struct rt_private_map *lm; 
mlist **flist;
{

	struct rt_private_map *lm1;

	for ( lm1 = _ld_loaded; lm1; lm1 = (struct rt_private_map *) NEXT(lm1) )
		CLEAR_FLAG(lm1, RT_NEEDED_SEEN);

	SET_FLAG(lm, RT_NEEDED_SEEN); 		

	if (NEEDED(lm))
		do_init(NEEDED(lm), flist);

	if (NAME(lm) == 0)
		/* not for a.out */
		return;

	if (!TEST_FLAG(lm, RT_INIT_CALLED)) {
		mlist	*mptr;
		void (*itmp)();

		if ((mptr = (mlist *)_rtmalloc(sizeof(mlist))) == 0) 
			return;
		SET_FLAG(lm, RT_INIT_CALLED);
		mptr->l_map = lm;
		mptr->l_next = *flist;
		*flist = mptr;
		itmp = INIT(lm);
		if (itmp)
			(*itmp)();
	}
}

/* function called by atexit(3C) - goes through link_map
 * and invokes each shared object's _fini function (skips a.out)
 */
void 
_rt_do_exit()
{
	/* call fini routines for objects loaded on startup */
	_rt_process_fini(_rt_fini_list, 1);

	/* call dlopen fini routines */
	_rt_dl_do_exit();
}

void
_rt_process_fini(list, about_to_exit)
mlist *list;
int about_to_exit;
{

	for (; list; list = list->l_next) {
		struct rt_private_map *lm;
		lm = list->l_map;
		if (!TEST_FLAG(lm, RT_FINI_CALLED) &&
			(about_to_exit || (COUNT(lm) == 0))) {
			/* don't call fini routine if already called
			 * or if we are not doing exit processing
			 * and the object's reference count hasn't
			 * reached 0 (for dlclose)
			 */
			void (*fptr)();

			SET_FLAG(lm, RT_FINI_CALLED);
			fptr = FINI(lm);
			if (fptr)
				(*fptr)();
		}	
	}
}

/* internal version of strlen */

int _rtstrlen(s)
register CONST char *s;
{
	register CONST char *s0 = s + 1;

	while (*s++ != '\0')
		;
	return (s - s0);
}

void
_rtmkspace(p, sz)	/* add space to free list */
	char	*p;
	size_t	sz;
{
	register Space		*sp;
	register unsigned long	n = (unsigned long)p;
	register size_t		j;

	if (sz < 2 * sizeof(Space) + 64)
		return;
	j = sizeof(double) - n % sizeof(double);
	sp = (Space *)(p + j);
	sp->s_size = sz - (j + sizeof(Space));
	sp->s_next = space;
	sp->s_ptr = (char *)(sp + 1);
	space = sp;
	return;
}

	
/* Local heap allocator.  Very simple, does not support storage freeing. */
VOID *_rtmalloc(nb)
	register unsigned nb;
{
	register Space	*sp = space,
			*back = 0;
	register char	*tp;
	int fdzero_opened = 0;

	DPRINTF(LIST, (2, "rtld: _rtmalloc(0x%x)\n", nb));
	/* we always round to double-word boundary */
	nb = DROUND(nb);
	while (sp)
	{
		if (sp->s_size >= nb)
		{
			tp = sp->s_ptr;
			if ((sp->s_size -= nb) >= 64)
			{
				sp->s_ptr += nb;
			}
			else
			{
				if (back == 0)
					space = sp->s_next;
				else
					back->s_next = sp->s_next;
			}
			return tp;
		}
		back = sp;
		sp = sp->s_next;
	}
	/* map in at least a page of anonymous memory */
	/* open /dev/zero if not open, for reading anonymous memory */
	if (_devzero_fd == -1) {
		if(_rt_opendevzero() < 0){
			return 0 ;
		}
		fdzero_opened = 1;
	}
	if ((tp = _rtmmap(0, PROUND(nb), (PROT_READ|PROT_WRITE),
	    MAP_PRIVATE, _devzero_fd, 0)) == (caddr_t)-1) {
		_rt_lasterr("%s: %s: can't malloc space",(char*) _rt_name,_proc_name);
		if (fdzero_opened) {
			/* close /dev/zero */
			(void)_rtclose(_devzero_fd);
			_devzero_fd = -1;
		}

		return 0;
	}
	sp = (Space *)tp;
	tp += sizeof(*sp);
	sp->s_next = space;
	space = sp;
	sp->s_ptr = tp + nb;
	sp->s_size = PROUND(nb) - nb - sizeof(*sp);

	if (fdzero_opened) {
		/* close /dev/zero */
		(void)_rtclose(_devzero_fd);
		_devzero_fd = -1;
	}

	return tp;
}

/* internal version of strncmp */
static int _rtstrncmp(s1, s2, n)
register CONST char *s1, *s2;
register int n;
{
	n++;
	if (s1 == s2)
		return(0);
	while (--n != 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return((n == 0) ? 0 : *s1 - *(s2-1));
}

/* internal getenv routine - only a few strings are relevant */
CONST char *
_readenv(envp, bmode)
CONST char **envp;
int *bmode;
{
	register CONST char *s1;
	int i;
	CONST char *envdirs = 0;

	if (envp == (CONST char **)0)
		return((char *)0);
	while (*envp != (CONST char *)0) {
		s1 = *envp++;
		if (*s1++ != 'L' || *s1++ != 'D' || *s1++ != '_' )
			continue;

#ifdef DEBUG
		if (_rtstrncmp( s1, "DEBUG=", 6 ) == 0) {
			i = (int)(s1[6] - '0');
			if (i > 0 && i <= MAXLEVEL)
				_debugflag = i;
		}
#endif

		i = sizeof("TRACE_LOADED_OBJECTS") - 1;
		if (_rtstrncmp( s1, "TRACE_LOADED_OBJECTS", i) == 0) {
			s1 += i;
			if ( *s1 == '\0' || *s1 == '=' )
				_rt_tracing = 1;
		}

		if (_rtstrncmp( s1, "WARN", 4) == 0) {
			s1 += 4;
			if ( s1[0] == '=' && s1[1]  != '\0' )
				_rt_warn = 1;
		}

		i = sizeof("LIBRARY_PATH") - 1;
		if (_rtstrncmp( s1, "LIBRARY_PATH", i ) == 0) {
			s1 += i;
			if ( s1[0] == '=' && s1[1] != '\0' )
				envdirs = ++s1;
		}

		i = sizeof("BIND_NOW") - 1;
		if (_rtstrncmp( s1, "BIND_NOW", i ) == 0) {
			s1 += i;
			if ( s1[0] == '=' && s1[1] != '\0' )
				*bmode = RTLD_NOW;
		}
	}

	/* LD_WARN is meaningful only if tracing */
	if (!_rt_tracing)
		_rt_warn = 0;

	return envdirs;
}

/* internal version of strcpy */
char *_rtstrcpy(s1, s2)
register char *s1;
register CONST char *s2;
{
	char *os1 = s1;

	while (*s1++ = *s2++)
		;
	return(os1);
}

/* _rtstr3cpy - concatenate 3 strings */
char *_rtstr3cpy(s1, s2, s3, s4)
register char *s1;
register CONST char *s2, *s3, *s4;
{
	char *os1 = s1;

	while (*s1++ = *s2++)
		;
	--s1;
	while (*s1++ = *s3++)
		;
	--s1;
	while (*s1++ = *s4++)
		;
	return(os1);
}


/*  Local "fprintf"  facilities.  */

static char *printn ARGS((int, unsigned long, int, char *, register char *, char *));
static void doprf ARGS((int , CONST char *, va_list , char *));
static void _rtstatwrite ARGS((int fd, char *buf, int len));


/*VARARGS2*/
#ifdef __STDC__
void _rtfprintf(int fd, CONST char *fmt, ...)
#else
void _rtfprintf(fd, fmt, va_alist)
int fd;
char *fmt;
va_dcl
#endif
{
	va_list adx;
	char linebuf[ERRSIZE];


#ifdef __STDC__
	va_start(adx, fmt);
#else
	va_start(adx);
#endif

	doprf(fd, fmt, adx, linebuf);
	va_end(adx);
}

/* error recording function - we write the error string to
 * a static buffer and set a global pointer to point to the string
 */

/*VARARGS1*/
#ifdef __STDC__
void _rt_lasterr(CONST char *fmt, ...)
#else
void _rt_lasterr(fmt, va_alist)
char *fmt;
va_dcl
#endif
{
	va_list adx;
	static char errptr[ERRSIZE];
#ifdef __STDC__
	va_start(adx, fmt);
#else
	va_start(adx);
#endif

	doprf(-1, fmt, adx, errptr);
	va_end(adx);
	_rt_error = errptr;
}

static void doprf(fd, fmt, adx, linebuf)
int fd;
CONST char *fmt;
char *linebuf;
va_list adx;
{
	register char c;		
	register char *lbp, *s;
	int i, b, num;	

#define	PUTCHAR(c)	{\
			if (lbp >= &linebuf[ERRSIZE]) {\
				_rtstatwrite(fd, linebuf, lbp - linebuf);\
				lbp = linebuf;\
			}\
			*lbp++ = (c);\
			}

	lbp = linebuf;
	while ((c = *fmt++) != '\0') {
		if (c != '%') {
			if (c == '\n')
				PUTCHAR('\r');
			PUTCHAR(c);
		}
		else {
			c = *fmt++;
			num = 0;
			switch (c) {
		
			case 'x': 
			case 'X':
				b = 16;
				num = 1;
				break;
			case 'd': 
			case 'D':
			case 'u':
				b = 10;
				num = 1;
				break;
			case 'o': 
			case 'O':
				b = 8;
				num = 1;
				break;
			case 'c':
				b = va_arg(adx, int);
				for (i = 24; i >= 0; i -= 8)
					if ((c = ((b >> i) & 0x7f)) != 0) {
						if (c == '\n')
							PUTCHAR('\r');
						PUTCHAR(c);
					}
				break;
			case 's':
				s = va_arg(adx, char*);
				while ((c = *s++) != 0) {
					if (c == '\n')
						PUTCHAR('\r');
					PUTCHAR(c);
				}
				break;
			case '%':
				PUTCHAR('%');
				break;
			}
			if (num) {
				lbp = printn(fd, va_arg(adx, unsigned long), b,
					linebuf, lbp, &linebuf[ERRSIZE]);
			}
			
		}
	}
	(void)_rtstatwrite(fd, linebuf, lbp - linebuf);
}

/* Printn prints a number n in base b. */
static char *printn(fd, n, b, linebufp, lbp, linebufend)
int fd, b;
unsigned long n;
char *linebufp, *linebufend;
register char *lbp;
{
	char prbuf[11];			/* Local result accumulator */
	register char *cp;
	CONST char *nstring = "0123456789abcdef";

#undef PUTCHAR
#define	PUTCHAR(c)	{\
			if (lbp >= linebufend) {\
				_rtstatwrite(fd, linebufp, lbp - linebufp);\
				lbp = linebufp;\
			}\
			*lbp++ = (char)(c);\
			}

	if (b == 10 && (int)n < 0) {
		PUTCHAR('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = nstring[n % b];
		n /= b;
	} while (n);
	do {
		PUTCHAR(*--cp);
	} while (cp > prbuf);
	return (lbp);
}

static void _rtstatwrite(fd, buf, len)
int fd, len;
char *buf;
{
	if (fd == -1) {
		*(buf + len) = '\0';
		return;
	}
	(void)_rtwrite(fd, buf, len);
}
int _rt_opendevzero()
{
	if ((_devzero_fd = _rtopen(DEV_ZERO, O_RDONLY)) == -1) {
		_rt_lasterr("%s: %s: can't open %s",
			(char*) _rt_name,_proc_name,(CONST char *)DEV_ZERO);
		return -1;
	}
	return 1;
}

/* returns non-zero if set1 and set2 contain a common member, i.e.
 * the objects they belong to belong to a common group; else returns 0
 */
int 
_rt_ismember(lm1, lm2)
struct rt_private_map *lm1, *lm2;
{
	struct rt_set	*set1, *set2;
	DPRINTF(LIST,(2,"rtld: _rt_ismember(0x%x, 0x%x)\n", lm1, lm2));

	set1 = GRPSET(lm1);
	set2 = GRPSET(lm2);
	
	while(set1 && set2)
	{
		if ((set1->members & set2->members) != 0)
			return 1;
		set1 = set1->next;
		set2 = set2->next;
	}
	return 0;
}

/* returns 1 if lm is a member of group grp, else 0 */
int
_rt_hasgroup(grp, lm)
unsigned long grp;
struct rt_private_map	*lm;
{

	struct	rt_set	*set;
	unsigned long	i;

	DPRINTF(LIST,(2,"rtld: _rt_hasgroup(%d, 0x%x)\n", grp, lm));

	set = GRPSET(lm);
	for(i = grp/LONG_BIT; i != 0; i--)
	{
		set = set->next;
		if (!set)
			return 0;
	}
	return(set->members & ((unsigned long)1 << (grp % LONG_BIT)));
}

/* is lm a member of the global group (group 0 ?) */
int
_rt_isglobal(lm)
struct rt_private_map	*lm;
{
	struct	rt_set	*set;

	DPRINTF(LIST,(2,"rtld: _rt_isglobal(0x%x)\n", lm));
	set = GRPSET(lm);  /* always at least 1 */
	return(set->members & 1);
}

/* add group with id grp to set;
 * if is_global is non-zero, add set 0
 */

void 
_rt_addset(grp, lm, is_global)
unsigned long		grp;
struct rt_private_map	*lm;
unsigned int		is_global;
{
	struct	rt_set	*set;
	unsigned long	i;
	
	DPRINTF(LIST,(2,"rtld: _rt_addset(%d, 0x%x, %d)\n", grp, lm, is_global));

	set = GRPSET(lm);
	if (is_global) /* global group is bit 1 of first set on list */
                set->members |= 1;
	for(i = grp/LONG_BIT; i != 0; i--) {
		/* depends on rtmalloc zeroing memory */
		if (!set->next)
			set->next = (struct rt_set *)
				_rtmalloc(sizeof(struct rt_set));
		set = set->next;
	}
	set->members |= ((unsigned long)1 << (grp % LONG_BIT));
}

/* delete group with id grp from set */
void
_rt_delset(grp, lm)
unsigned long		grp;
struct rt_private_map	*lm;
{
	unsigned long	i;
	struct rt_set	*set;

	DPRINTF(LIST,(2,"rtld: _rt_delset(%d, 0x%x)\n", grp, lm));

	set = GRPSET(lm);
	
	for(i = grp/LONG_BIT; i != 0; i--)
	{
		/* depends on rtmalloc zeroing memory */
		set = set->next;
		if (!set)
			return;
	}
	set->members &= ~((unsigned long)1 << (grp % LONG_BIT));
}

/* add group to entire dependency graph of lm0 */
void
_rt_setgroup(group, lm0, is_global)
unsigned long         	group;
struct rt_private_map 	*lm0;
unsigned  int		is_global;	
{
	mlist *lptr;

	DPRINTF(LIST,(2,"rtld: _rt_setgroup(%d, 0x%x)\n", group, lm0));

	if (_rt_hasgroup(group, lm0))
		/* have already been here */
		return;
	_rt_addset(group, lm0, is_global);
	for(lptr = NEEDED(lm0); lptr; lptr = lptr->l_next) {
		_rt_setgroup(group, lptr->l_map, is_global);
	
	}
}
/* add an entry to lm's reference list and bump refcount for
 * ref; can fail only if malloc fails
 * do not add or bump count if lm already references ref
 */
int
_rt_add_ref(lm, ref)
struct rt_private_map *lm, *ref;
{
	mlist *lptr, *lprev;

	DPRINTF(LIST,(2,"rtld: _rt_add_ref(0x%x, 0x%x)\n", lm, ref));
	lprev = 0;
	for (lptr = REFLIST(lm); lptr; 
		lprev = lptr, lptr = lptr->l_next)  {
		if (ref == lptr->l_map) 
			return 1;
	}
	if ((lptr = (mlist *)_rtmalloc(sizeof(mlist))) == 0) 
		return 0;

	lptr->l_map = ref;
	COUNT(ref) += 1;

	/* append the new list item */
	if (!lprev) {
		/* first item */
		REFLIST(lm) = lptr;
	}
	else {
		lprev->l_next = lptr;
	}
	return 1;
}

/* add an entry to lm's needed list
 * can fail only if malloc fails;
 * this also results in incrementing the reference
 * count of the needed object
 */
int
_rt_add_needed(lm, needed)
struct rt_private_map *lm, *needed;
{
	mlist *lptr, *lprev;

	DPRINTF(LIST,(2,"rtld: _rt_add_needed(0x%x, 0x%x)\n", lm, needed));
	lprev = 0;
	for (lptr = NEEDED(lm); lptr; 
		lprev = lptr, lptr = lptr->l_next)  {
		if (needed == lptr->l_map) 
			return 1;
	}
	if ((lptr = (mlist *)_rtmalloc(sizeof(mlist))) == 0) 
		return 0;

	lptr->l_map = needed;
	COUNT(needed) += 1;

	/* append the new list item */
	if (!lprev) {
		/* first item */
		NEEDED(lm) = lptr;
	} else {
		lprev->l_next = lptr;
	}
	return 1;
}
