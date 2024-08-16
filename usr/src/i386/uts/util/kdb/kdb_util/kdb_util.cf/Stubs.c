/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/kdb/kdb_util/kdb_util.cf/Stubs.c	1.6"
#ident	"$Header: $"

#ifndef NODEBUGGER

#include <sys/ksynch.h>
#include <sys/types.h>

extern fspin_t debug_count_mutex;

void kdb_init(void)
{
	FSPIN_INIT(&debug_count_mutex);
}

#ifndef MODSTUB
void kdb_printf() {}
boolean_t kdb_check_aborted() { return B_FALSE; }
#endif

#endif /* NODEBUGGER */
