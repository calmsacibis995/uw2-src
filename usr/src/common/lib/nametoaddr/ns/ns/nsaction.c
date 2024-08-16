/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/ns/nsaction.c	1.1"
#ident  "$Header: $"
#include <stdio.h>
#include <netdb.h>
#ifdef _REENTRANT
#include "ns_mt.h"
#endif

int _ns_action;

#ifdef _REENTRANT
void
_free_ns_action(p)
    void *p;
{
    if (FIRST_OR_NO_THREAD)
        return;
    if (p != NULL)
        free(p);
    return;
}
#endif /* _REENTRANT */

int
get_nsaction()
{
#ifdef _REENTRANT
	struct _ns_tsd *key_tbl;

	/*
	 * This is the case of the initial thread
	 */
	if (FIRST_OR_NO_THREAD)
		return(_ns_action);

	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _ns_tsd *)
		_mt_get_thr_specific_storage(_ns_key,_NS_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->ns_action_p != NULL)
		return(*(int *)key_tbl->ns_action_p);
	return(NO_ERRORMEM);
#else
	return(_ns_action);
#endif

}

int
set_nsaction(code)
int code;
{
#ifdef _REENTRANT
	struct _ns_tsd *key_tbl;

	/*
	 * This is the case of the initial thread
	 */
	if (FIRST_OR_NO_THREAD){
		_ns_action = code;
		return(0);
	}
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _ns_tsd *)
		_mt_get_thr_specific_storage(_ns_key,_NS_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return(-1);
	if (key_tbl->ns_action_p == NULL)
		key_tbl->ns_action_p = (void *)calloc(1, sizeof(int));
	if (key_tbl->ns_action_p == NULL)
		return(-1);
	*(int *)key_tbl->ns_action_p = code;
#else
	_ns_action = code;
#endif
}

