/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/pushpop.c	1.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <limits.h>
#ifdef __STDC__
#   include <stddef.h>
#   include <stdlib.h>
#else
#   define offsetof(type, mem)	((size_t)&(((type *)0)->mem))
    extern char *malloc();
    extern void free();
#endif
#include "stdiom.h"

#define PBLEN	(MB_LEN_MAX > 8 ? MB_LEN_MAX : 8)

struct layer	/* saved underlying FILE, BFILE and new buffer */
{
	Uchar	*begptr;
	Uchar	*endptr;
	Uchar	*_ptr;
	int	eflags;
	int	_cnt;
	Uchar	buf[PBLEN];
};

int
#ifdef __STDC__
_pushbuf(register BFILE *bp, int ch)	/* add new buffer layer */
#else
_pushbuf(bp, ch)register BFILE *bp; int ch;
#endif
{
	register struct layer *p;
	register FILE *fp = (FILE *)bp->file._base;

	if ((p = (struct layer *)malloc(sizeof(struct layer))) == 0)
		return EOF;
	p->begptr = bp->begptr;
	p->endptr = bp->endptr;
	p->eflags = bp->eflags;
	p->_ptr = fp->_ptr;
	p->_cnt = fp->_cnt;
	bp->begptr = p->buf;
	bp->endptr = &p->buf[sizeof(p->buf)];
	bp->eflags |= IO_PUSHED;
	fp->_ptr = &p->buf[sizeof(p->buf) - 1];
	*fp->_ptr = ch;
	fp->_cnt = 1;
	return ch;
}

void
#ifdef __STDC__
_popbuf(register BFILE *bp)	/* remove empty buffer layer */
#else
_popbuf(bp)register BFILE *bp;
#endif
{
	register struct layer *p;
	register FILE *fp = (FILE *)bp->file._base;

	p = (struct layer *)(bp->begptr - offsetof(struct layer, buf));
	bp->begptr = p->begptr;
	bp->endptr = p->endptr;
	bp->eflags = p->eflags;
	fp->_ptr = p->_ptr;
	fp->_cnt = p->_cnt;
	free((void *)p);
}

int
#ifdef __STDC__
_hidecnt(register BFILE *bp)	/* total of all hidden fp->_cnt's */
#else
_hidecnt(bp)register BFILE *bp;
#endif
{
	register struct layer *p;
	register int sum = 0;

	p = (struct layer *)(bp->begptr - offsetof(struct layer, buf));
	if (p->_cnt > 0)
		sum = p->_cnt;
	while (p->eflags & IO_PUSHED)
	{
		p = (struct layer *)(p->begptr - offsetof(struct layer, buf));
		if (p->_cnt > 0)
			sum += p->_cnt;
	}
	return sum;
}
