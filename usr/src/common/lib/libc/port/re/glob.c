/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:re/glob.c	1.4"

#include "synonyms.h"
#include <stddef.h>
#include <stdlib.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <glob.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "re.h"

#ifdef __STDC__
	#pragma weak glob = _glob
	#pragma weak globfree = _globfree
#endif

#ifndef offsetof
#   define offsetof(t, m) (&((t *)0)->m) /* usually works */
#endif

static const char *
#ifdef __STDC__
scan(const char **ptr, int flags)
#else
scan(ptr, flags)const char **ptr; int flags;
#endif
{
	const char *p = *ptr;
	const char *s = 0;

	/*
	* Partition the pathname pattern into at most three substrings:
	*  1. An optional initial component pattern--
	*	from the incoming *ptr to the outgoing *ptr
	*  2. An optional invariant string that follows the pattern--
	*	from the outgoing *ptr to the returned pointer
	*  3. An optional unexamined string--
	*	from the returned pointer
	*/
	for (;;)
	{
		switch (*p)
		{
		case '\\':
			if (flags & GLOB_NOESCAPE)
				break;
			goto special; /* simplest to take quote as special */
		case '(':
			if ((flags & GLOB_EXTENDED) == 0)
				break;
			goto special;
		case '@':
		case '!':
		case '+':
			if ((flags & GLOB_EXTENDED) == 0 || p[1] != '(')
				break;
			/*FALLTHROUGH*/
		case '[':
		case '?':
		case '*':
		special:;
			if (s != 0)
				return s;
			while (*++p != '/')
			{
				if (*p == '\0')
				{
					*ptr = p;
					return p;
				}
			}
			*ptr = p;
		case '/':
			while (*++p == '/')
				;
			s = p;
			continue;
		case '\0':
			return p;
		}
		p++;
	}
}

typedef struct gl_str	Str;
struct gl_str
{
	Str	*next;
	char	str[1];	/* grows as necessary */
};

static void
#ifdef __STDC__
delstrs(Str *sp)
#else
delstrs(sp)Str *sp;
#endif
{
	Str *spn;

	while (sp != 0)
	{
		spn = sp->next;
		free((void *)sp);
		sp = spn;
	}
}

static Str *
#ifdef __STDC__
newstr(Str *sp, ...)
#else
newstr(sp, p, va_alist)Str *sp; va_dcl /*no ;*/
#endif
{
	const char *s;
	size_t len;
	va_list ap;
	Str *new;
	char *p;

	/*
	* First pass: determine the space needed.
	*/
#ifdef __STDC__
	va_start(ap, sp);
#else
	va_start(ap);
#endif
	len = 0;
	while ((s = va_arg(ap, const char *)) != 0)
		len += va_arg(ap, size_t);
	va_end(ap);
	if ((new = (Str *)malloc(offsetof(Str, str) + len + 1)) == 0)
	{
		delstrs(sp);
		return 0;
	}
	new->next = sp;
	/*
	* Second pass: copy in the strings.
	*/
#ifdef __STDC__
	va_start(ap, sp);
#else
	va_start(ap);
#endif
	p = new->str;
	while ((s = va_arg(ap, const char *)) != 0)
	{
		len = va_arg(ap, size_t);
		(void)memcpy((void *)p, (const void *)s, len);
		p += len;
	}
	va_end(ap);
	*p = '\0';
	return new;
}

struct ord
{
	struct lc_collate *col;
	int (*cmp)(struct lc_collate *, const char *, const char *);
};

static Str *
#ifdef __STDC__
mergesort(struct ord *op, Str *sp, size_t len)
#else
mergesort(op, sp, len)struct ord *op; Str *sp; size_t len;
#endif
{
	Str head, *mid, *nxt, *ptr;
	size_t lcnt, rcnt;

	/*
	* Advance mid to just before the midpoint of the list
	* and cut the list there.
	*/
	mid = sp;
	rcnt = lcnt = len >> 1;	/* guaranteed: != 0 */
	while (--rcnt != 0)
		mid = mid->next;
	rcnt = len - lcnt;
	nxt = mid->next;
	mid->next = 0;
	mid = nxt;
	/*
	* Sort the respective half lists, only calling recurring
	* if there are at least two elements in the list.
	*/
	if (lcnt > 1)
		sp = mergesort(op, sp, lcnt);
	if (rcnt > 1)
		mid = mergesort(op, mid, rcnt);
	/*
	* Merge the two sorted half lists together.
	*/
	for (ptr = &head;; ptr = nxt)
	{
		if (sp == 0)
		{
			ptr->next = mid;
			break;
		}
		if (mid == 0)
		{
			ptr->next = sp;
			break;
		}
		if ((op->cmp != 0 ? (*op->cmp)(op->col, sp->str, mid->str)
			: strcmp(sp->str, mid->str)) > 0)
		{
			ptr->next = nxt = mid;
			mid = mid->next;
		}
		else
		{
			ptr->next = nxt = sp;
			sp = sp->next;
		}
	}
	return head.next;
}

static const char special[] = "/*@|.";
#define DIRMARK		(&special[0])
#define EXECMARK	(&special[1])
#define SLNKMARK	(&special[2])
#define FIFOMARK	(&special[3])
#define DOTSTR		(&special[4])

int
#ifdef __STDC__
glob(const char *path, int flags, int (*err)(const char *, int), glob_t *gp)
#else
glob(path,flags,err,gp)const char*path;int flags,(*err)();glob_t*gp;
#endif
{
	const char *pat, *fix, *more;
	Str *cur, *new, *nxt;
	size_t fixlen, count;
	int ret, m, fnmflags;
	fnm_t fnmtop;
	char **vp;

	ret = GLOB_NOSPACE;	/* usual failure reason */
	count = 0;
	new = 0;
	cur = 0;
	fnmflags = FNM_PATHNAME | FNM_PERIOD | FNM_COMPONENT;
	if (flags & GLOB_OKAYDOT)
		fnmflags = FNM_PATHNAME | FNM_COMPONENT;
	if (flags & GLOB_NOESCAPE)
		fnmflags |= FNM_NOESCAPE;
	if (flags & GLOB_BADRANGE)
		fnmflags |= FNM_BADRANGE;
	if (flags & GLOB_BKTESCAPE)
		fnmflags |= FNM_BKTESCAPE;
	if (flags & GLOB_EXTENDED)
		fnmflags |= FNM_EXTENDED;
	pat = fix = path;
	more = scan(&fix, flags);
	if (fix != pat) /* path starts with a pattern component */
	{
		cur = newstr((Str *)0, (char *)0); /* empty string */
	}
	else if (*more == '\0') /* path contains no pattern components */
	{
#if 1
		/*
		* The intent of the 1003.2 specification seems to be that
		* even if the path contains no pattern characters, the
		* existance of the filesystem entry must be verified.
		*/
		if ((flags & GLOB_NOCHECK) == 0
			&& access(path, F_OK | EFF_ONLY_OK) != 0)
		{
			ret = GLOB_NOMATCH;
			goto cleanup;
		}
#endif
	copy1:;
		cur = newstr((Str *)0, path, strlen(path), (char *)0);
		if (cur == 0)
			goto cleanup;
		count = 1;
		fixlen = 0;
		pat = 0;	/* flags this special case */
		goto skip;
	}
	else /* start with the fixed portion as the initial directory */
	{
		cur = newstr((Str *)0, fix, more - fix, (char *)0);
		pat = fix = more;
		more = scan(&fix, flags);
	}
	if (cur == 0)
		goto cleanup;
	/*
	* Iterate for each pattern component of the path.
	* Attempt to open the directory, match each of its entries
	* against the current pattern, and add to the list for each
	* entry that matches.
	*/
	for (;;)
	{
		/*
		* Run through all of the "pending directories" and
		* attempt to match each entry against the pattern.
		* Compile the pattern assuming that we'll get to use
		* it below.
		*/
		fixlen = more - fix;
		if ((m = _fnmcomp(&fnmtop, (const unsigned char *)pat,
			fnmflags)) != 0)
		{
			if (m == FNM_BADPAT)
				ret = GLOB_BADPAT;
			goto cleanup;
		}
		fnmflags |= FNM_REUSE;
		do
		{
			size_t curlen = strlen(cur->str);
			DIR *dirp;

			if ((dirp = opendir(curlen == 0 ? DOTSTR : cur->str))
				== 0)
			{
				/*
				* ENOENT means that one of the presumed-to-exist
				* components isn't there.  This is distinct from
				* the other errno values that force a call to
				* the caller's error function.
				*/
				if ((m = errno) != ENOENT)
				{
					if (err != 0 && (*err)(curlen == 0
						? DOTSTR : cur->str, m) != 0
						|| flags & GLOB_ERR)
					{
						ret = GLOB_ABORTED;
						goto cleanup;
					}
				}
			}
			else /* match the entries against the pattern */
			{
				struct dirent *dp;

				while ((dp = readdir(dirp)) != 0)
				{
					if (_fnmexec(&fnmtop,
						(const unsigned char *)
						dp->d_name) == FNM_NOMATCH)
					{
						continue;
					}
					if ((new = newstr(new, cur->str, curlen,
						dp->d_name, strlen(dp->d_name),
						fix, fixlen, (char *)0)) == 0)
					{
						goto cleanup;
					}
					count++;
				}
				(void)closedir(dirp);
			}
			nxt = cur->next;
			free((void *)cur);
		} while ((cur = nxt) != 0);
		/*
		* Have run through all "pending directories" for the
		* current component pattern.  If the new list is
		* empty, get out.  If there is no more to the pathname
		* pattern, get out.  Otherwise, rescan the pathname
		* pattern and continue.
		*/
		if (new == 0)
		{
		check1:;
			if (flags & GLOB_NOCHECK)
				goto copy1;
			ret = GLOB_NOMATCH;
			goto cleanup;
		}
		cur = new;
		new = 0;
		if (*more == '\0')
			break;
		count = 0;
		pat = fix = more;
		more = scan(&fix, flags);
	}
	/*
	* Have generated all possible matching pathnames.
	* However, there can be nonexistent in the list due to
	* invalid appended fixed strings.
	* Also, when marking is requested, all the matching
	* pathnames must be checked for interesting file types.
	*/
skip:;
	if (fixlen != 0 || flags & (GLOB_MARK | GLOB_FULLMARK))
	{
		struct stat stbuf;

		do
		{
			nxt = cur->next;
			if (lstat(cur->str, &stbuf) != 0)
			{
				/*
				* Can't access it.
				* Usually just toss it from the list,
				* but if the string is from a forced
				* single entry list (a copy of the
				* original pattern), keep it.
				*/
				if (--count == 0)
				{
					if (pat == 0) /* was a copy of path */
					{
						count = 1;
						goto move;
					}
					free((void *)cur);
					goto check1;
				}
			}
			else if (flags & (GLOB_MARK | GLOB_FULLMARK))
			{
				/*
				* Make the checks for marks the same as is
				* done in ls(1):  First check for a directory.
				* Then, if other marks are requested, check
				* for a symbolic link, or a file with an
				* executable bit set, or a fifo.
				*/
				if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
				{
					more = DIRMARK;
				}
				else if ((flags & GLOB_FULLMARK) == 0)
				{
					goto move;
				}
				else if ((stbuf.st_mode & S_IFMT) == S_IFLNK)
				{
					more = SLNKMARK;
				}
				else if (stbuf.st_mode 
					& (S_IXUSR | S_IXGRP | S_IXOTH))
				{
					more = EXECMARK;
				}
				else if ((stbuf.st_mode & S_IFMT) == S_IFIFO)
				{
					more = FIFOMARK;
				}
				else /* no mark for this pathname */
				{
					goto move;
				}
				if ((new = newstr(new, cur->str,
					strlen(cur->str), more, (size_t)1,
					(char *)0)) == 0)
				{
					goto cleanup;
				}
			}
			else /* just insert it on the result list */
			{
			move:;
				cur->next = new;
				new = cur;
				continue;
			}
			free((void *)cur);
		} while ((cur = nxt) != 0);
		cur = new;
		new = 0;
	}
	/*
	* The list of strings pointed to by cur is finalized.
	* Sort them if there's more than one and if not bypassed.
	*/
	if ((flags & GLOB_NOSORT) == 0 && count > 1)
	{
		struct lc_collate *col;
		struct ord ordtop;

		ordtop.cmp = 0;
		ordtop.col = col = 0;
		if ((flags & GLOB_NOCOLLATE) == 0)
		{
			if (fnmflags & FNM_REUSE
				&& fnmtop.__fnmflags & FNM_COLLATE)
			{
				ordtop.col = fnmtop.__fnmcol;
				goto setcmp;
			}
			else if ((col = _lc_collate((struct lc_collate *)0))
				!= 0)
			{
				ordtop.col = col;
			setcmp:;
				if (ordtop.col->maintbl == 0) /* CHF_ENCODED */
					ordtop.col = 0;
#ifdef DSHLIB
				else if (ordtop.col->strc != 0)
					ordtop.cmp = ordtop.col->strc;
#endif
				else
					ordtop.cmp = _strqcoll;
				
			}
		}
		cur = mergesort(&ordtop, cur, count);
		if (col != 0)
			(void)_lc_collate(col);
	}
	/*
	* Normalize the caller's data structure by eliminating the
	* special cases for the initial call and reserved slots.
	*/
	if ((flags & GLOB_APPEND) == 0)
	{
		gp->gl_str = 0;
		gp->gl_pathc = 0;
		gp->gl_pathv = 0;
		if ((flags & GLOB_DOOFFS) == 0)
			gp->gl_offs = 0;
	}
	/*
	* Grow the vector of pointers.
	*/
	fixlen = gp->gl_offs + gp->gl_pathc + count + 1;
	if ((vp = (char **)realloc((void *)gp->gl_pathv,
		sizeof(char *) * fixlen)) == 0)
	{
		goto cleanup;
	}
	/*
	* Insert null pointers for the initial call with GLOB_DOOFFS.
	*/
	if (gp->gl_pathv == 0 && (fixlen = gp->gl_offs) != 0)
	{
		do
			vp[--fixlen] = 0;
		while (fixlen != 0);
	}
	gp->gl_pathv = vp;
	vp += gp->gl_offs + gp->gl_pathc;
	gp->gl_pathc += count;
	/*
	* Assign the linked strings to new slots in the vector,
	* noting the last string in the list.
	*/
	nxt = cur;
	do
	{
		*vp++ = nxt->str;
		new = nxt;
	} while ((nxt = nxt->next) != 0);
	*vp = 0;
	/*
	* Link in to the existing chain of pathnames.
	*/
	new->next = gp->gl_str;
	gp->gl_str = cur;
	ret = 0; /* success! */
	/*
	* Clean up the information used by _fnm*()s.
	*/
out:;
	if (fnmflags & FNM_REUSE)
		_fnmfree(&fnmtop);
	return ret;
cleanup:;
	if (cur != 0)
		delstrs(cur);
	if (new != 0)
		delstrs(new);
	goto out;
}

void
#ifdef __STDC__
globfree(glob_t *gp)
#else
globfree(gp)glob_t *gp;
#endif
{
	delstrs(gp->gl_str);
	free((void *)gp->gl_pathv);
}
