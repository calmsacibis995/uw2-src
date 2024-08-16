/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:sys/lwpself.c	1.1"
#include <thread.h>

extern __lwp_desc_t **__lwp_priv_datap;

lwpid_t
_lwp_self(void)
{
	if (__lwp_priv_datap == 0)
		__thr_init();

	if ((*__lwp_priv_datap)->lwp_id == 0) {
		/* not yet initialized */
		(*__lwp_priv_datap)->lwp_id = __lwp_self();
	}
	return ((*__lwp_priv_datap)->lwp_id);
}
