/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/symintUtil.c	1.2.1.3"

#include "hidelibc.h"		/* uses "_" to hide libc functions */
#include "hidelibelf.h"		/* uses "_" to hide libelf functions */

/*
*	file: symintUtil.c
*	desc: utilities for symint code
*	date: 11/08/88
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "debug.h"

#if defined(__STDC__)
extern void *malloc(size_t);
extern void *realloc(void *, size_t);
extern void *calloc(size_t, size_t);
#else
extern char *malloc();
extern char *realloc();
extern char *calloc();
#endif

/*
*	_lprof_Malloc, _lprof_Calloc and _lprof_Realloc are used to 
*	monitor the allocation
*	of memory.  If failure occurs, we detect it and exit.
*/

void *
_lprof_Calloc(item_count, item_size)
uint item_count;
uint item_size;
{
	register void *p;

	if ((p = calloc(item_count, item_size)) == NULL) {
		DEBUG(printf("- size=%d, count=%d\n", item_size, item_count));
		_err_exit(gettxt("uxcds:1368","calloc: Out of space"));
	}
	return (p);
}

void *
_lprof_Realloc(pointer, size)
void *pointer;
uint size;
{
	register void *p;

	if ((p = realloc(pointer, size)) == NULL) {
		_err_exit(gettxt("uxcds:1369","realloc: Out of space"));
	}
	return (p);
}

void *
_lprof_Malloc(item_count, item_size)
int item_count;
int item_size;
{
        register char *p;

        if ((p = malloc((size_t)item_count * (size_t)item_size)) == NULL)  
                _err_exit("malloc: Out of space\n");
        return (p);
}


