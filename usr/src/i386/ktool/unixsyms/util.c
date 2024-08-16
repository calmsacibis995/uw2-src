/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:i386/ktool/unixsyms/util.c	1.2"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/vmparam.h>

#ifdef __STDC__
int
is_kaddr(vaddr_t addr)
#else
int
is_kaddr(addr)
vaddr_t addr;
#endif
{
	return (KADDR(addr));
}

#ifdef __STDC__
size_t
pagesize(void)
#else
size_t
pagesize()
#endif
{
	return (PAGESIZE);
}

#ifdef __STDC__
size_t
pg_size_align(size_t size)
#else
size_t
pg_size_align(size)
size_t size;
#endif
{
	return (ptob(btopr(size)));
}
