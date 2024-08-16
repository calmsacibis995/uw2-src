/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/netselect/netsel_mt.h	1.1.1.3"
#ident  "$Header:$"

/*
 * netsel_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

struct _netsel_tsd {
	void	*ncerror_p;
	void	*linenum_p;
	void	*fieldnum_p;
	void	*retstr_p;
	void	*savep_p;
};

#define NETSEL_KEYTBL_SIZE	( sizeof(struct _netsel_tsd) / sizeof(void *) )

extern THREAD_KEY_T _netselect_key;
extern MUTEX_T _netselect_counter_lock;
extern RWLOCK_T _netselect_list_lock;

extern void _free_netsel_keytbl(void *);
extern void _free_netsel_ncerror(void *);
extern void _free_netsel_linenum(void *);
extern void _free_netsel_fieldnum(void *);
extern void _free_netsel_retstr(void *);
extern void _free_netsel_savep(void *);

#endif /* _REENTRANT */
