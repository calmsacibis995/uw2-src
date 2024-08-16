/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/nsl_mt.h	1.2.1.4"
#ident	"$Header: $"

/*
 * nsl_mt.h
 */

#include <mt.h>

extern int get_t_errno();
extern int set_t_errno();

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

struct _nsl_tsd {
	void	*t_errno_p;
	char	*unk_err_str_p;
};

#define NSL_KEYTBL_SIZE		( sizeof(struct _nsl_tsd) / sizeof(void *) )

extern thread_key_t _nsl_key;
extern RWLOCK_T _nsl_lock;

extern void _free_nsl_keytbl(void *);
extern void _free_nsl_t_errno(void *);
extern void _free_nsl_unk_err_str(void *);

#endif /* _REENTRANT */
