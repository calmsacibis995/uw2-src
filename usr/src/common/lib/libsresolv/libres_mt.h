/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresolv:common/lib/libsresolv/libres_mt.h	1.3"
#ident  "$Header: $"

/*
 * libres_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

#include <thread.h>
#include <synch.h>

struct _rs_tsd {
	void	*hostinfo_p;
	void	*nbuf_p;
	void	*_res_p;
	void	*abuf_p;
	void	*s_p;
	void	*servinfo_p;
};

#define _RS_KEYTBL_SIZE		( sizeof(struct _rs_tsd) / sizeof(void *) )

#ifdef _STDC__

extern void _free_rs_keytbl(void *);

extern void _free_rs_hostinfo(void *);
extern void _free_rs_nbuf(void *);
extern void _free_rs__res(void *);
extern void _free_rs_abuf(void *);
extern void _free_rs_s(void *);
extern void _free_rs_servinfo(void *);

#else /* ! __STDC__ */

extern void _free_rs_keytbl();

extern void _free_rs_hostinfo();
extern void _free_rs_nbuf();
extern void _free_rs__res();
extern void _free_rs_abuf();
extern void _free_rs_s();
extern void _free_rs_servinfo();

#endif /* __STDC__ */

extern THREAD_KEY_T _rs_key;

#endif /* _REENTRANT */
