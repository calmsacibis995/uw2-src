/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/cr1/cr1_mt.h	1.1.2.2"
#ident  "$Header: $"

/*
 * cr1_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

extern THREAD_KEY_T _cr1_key;

struct _cr1_tsd {
	void	*key_p;
};

#define CR1_KEYTBL_SIZE	( sizeof (struct _cr1_tsd) / sizeof (void *) )

extern void _free_cr1_keytbl(void *);
extern void _free_cr1_key(void *);

#endif /* _REENTRANT */
