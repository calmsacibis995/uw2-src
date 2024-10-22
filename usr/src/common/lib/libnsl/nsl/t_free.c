/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_free.c	1.2.6.2"
#ident	"$Header: $"

#include <sys/xti.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "_import.h"
#include "nsl_mt.h"

#ifdef t_free
#undef t_free
#endif

#pragma weak _xti_free = t_free

t_free(ptr, struct_type)
char *ptr;
int struct_type;
{
	union structptrs {
		struct t_bind *bind;
		struct t_call *call;
		struct t_discon *dis;
		struct t_optmgmt *opt;
		struct t_unitdata *udata;
		struct t_uderr *uderr;
	} p;

	/*
	 * Free all the buffers associated with the appropriate
	 * fields of each structure.
	 */

	switch (struct_type) {

	case T_BIND:
		p.bind = (struct t_bind *)ptr;
		if (p.bind->addr.buf != NULL)
			free(p.bind->addr.buf);
		break;

	case T_CALL:
		p.call = (struct t_call *)ptr;
		if (p.call->addr.buf != NULL)
			free(p.call->addr.buf);
		if (p.call->opt.buf != NULL)
			free(p.call->opt.buf);
		if (p.call->udata.buf != NULL)
			free(p.call->udata.buf);
		break;

	case T_OPTMGMT:
		p.opt = (struct t_optmgmt *)ptr;
		if (p.opt->opt.buf != NULL)
			free(p.opt->opt.buf);
		break;

	case T_DIS:
		p.dis = (struct t_discon *)ptr;
		if (p.dis->udata.buf != NULL)
			free(p.dis->udata.buf);
		break;

	case T_UNITDATA:
		p.udata = (struct t_unitdata *)ptr;
		if (p.udata->addr.buf != NULL)
			free(p.udata->addr.buf);
		if (p.udata->opt.buf != NULL)
			free(p.udata->opt.buf);
		if (p.udata->udata.buf != NULL)
			free(p.udata->udata.buf);
		break;

	case T_UDERROR:
		p.uderr = (struct t_uderr *)ptr;
		if (p.uderr->addr.buf != NULL)
			free(p.uderr->addr.buf);
		if (p.uderr->opt.buf != NULL)
			free(p.uderr->opt.buf);
		break;

	case T_INFO:
		break;

	default:
		set_t_errno(TNOSTRUCTYPE);
		return(-1);
	}
	
	free((void *)ptr);
	return(0);
}
