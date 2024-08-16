/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rexec/rx_mt.h	1.1.1.3"
#ident  "$Header: $"

/*
 * rx_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

struct _rx_tsd {
	void	*errno_p;
	void	*cserrno_p;
	void	*dflag_p;
};

#define RX_KEYTBL_SIZE		( sizeof (struct _rx_tsd) / sizeof (void *) )

extern THREAD_KEY_T _rx_key;
extern MUTEX_T _rx_free_lock;
extern RWLOCK_T _rx_conn_lock;

extern void _free_rx_keytbl	(void *);
extern void _free_rx_errno	(void *);
extern void _free_rx_cserrno	(void *);
extern void _free_rx_dflag	(void *);

#endif /* _REENTRANT */
