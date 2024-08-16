/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/common/niserror.c	1.1"
#ident  "$Header: $"
#include <stdio.h>
#include <netdb.h>
#include "nis.h"
#ifdef _REENTRANT
#include "nis_mt.h"
#endif

int nis_errno;

#ifdef _REENTRANT
void
_free_nis_errno(p)
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
get_niserror()
{
#ifdef _REENTRANT
	struct _nis_tsd *key_tbl;

	/*
	 * This is the case of the initial thread
	 */
	if (FIRST_OR_NO_THREAD)
		return(nis_errno);

	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _nis_tsd *)
		_mt_get_thr_specific_storage(_nis_key,_NIS_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->nis_errno_p != NULL)
		return(*(int *)key_tbl->nis_errno_p);
	return(NO_ERRORMEM);
#else
	return(nis_errno);
#endif

}

int
set_niserror(err)
int err;
{
#ifdef _REENTRANT
	struct _nis_tsd *key_tbl;

	/*
	 * This is the case of the initial thread
	 */
	if (FIRST_OR_NO_THREAD){
		nis_errno = err;
		return(0);
	}
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _nis_tsd *)
		_mt_get_thr_specific_storage(_nis_key,_NIS_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return(-1);
	if (key_tbl->nis_errno_p == NULL)
		key_tbl->nis_errno_p = (void *)calloc(1, sizeof(int));
	if (key_tbl->nis_errno_p == NULL)
		return(-1);
	*(int *)key_tbl->nis_errno_p = err;
#else
	nis_errno = err;
#endif
}
