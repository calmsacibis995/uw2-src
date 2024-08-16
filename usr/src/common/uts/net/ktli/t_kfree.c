/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kfree.c	1.7"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */


/*
 *	t_kfree.c, kernel TLI function to free the specified
 *	kernel tli data structure.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <proc/user.h>
#include <io/stream.h>
#include <io/ioctl.h>
#include <fs/file.h>
#include <io/stropts.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <svc/errno.h>
#include <net/ktli/t_kuser.h>
#include <mem/kmem.h>

/*
 * t_kfree(tiptr, ptr, struct_type)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	This routine frees memory previously allocated by
 *	t_kalloc().
 *
 *	Returns 0 on success or positive error code.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	ptr			# pointer to allocated struct
 *	struct_type		# type of struct to free
 *	
 */
/*ARGSUSED*/
int
t_kfree(TIUSER *tiptr, char *ptr, int struct_type)
{
	union structptrs {
		struct t_bind		*bind;
		struct t_call		*call;
		struct t_discon		*dis;
		struct t_optmgmt	*opt;
		struct t_kunitdata	*udata;
		struct t_uderr		*uderr;
	} p;
	int				error;

	error = 0;

	/*
	 * free all the buffers associated with the appropriate
	 * fields of each structure.
	 */

	switch (struct_type) {

		case T_BIND:
			/* LINTED pointer alignment */
			p.bind = (struct t_bind *)ptr;
			if (p.bind->addr.buf != NULL)
				kmem_free(p.bind->addr.buf,
					(u_int)p.bind->addr.maxlen);
			kmem_free(ptr, (u_int)sizeof(struct t_bind));
			break;
	
		case T_CALL:
			/* LINTED pointer alignment */
			p.call = (struct t_call *)ptr;
			if (p.call->addr.buf != NULL)
				kmem_free(p.call->addr.buf,
					(u_int)p.call->addr.maxlen);
			if (p.call->opt.buf != NULL)
				kmem_free(p.call->opt.buf,
					(u_int)p.call->opt.maxlen);
			if (p.call->udata.buf != NULL)
				kmem_free(p.call->udata.buf,
					(u_int)p.call->udata.maxlen);
			kmem_free(ptr, (u_int)sizeof(struct t_call));
			break;
	
		case T_OPTMGMT:
			/* LINTED pointer alignment */
			p.opt = (struct t_optmgmt *)ptr;
			if (p.opt->opt.buf != NULL)
				kmem_free(p.opt->opt.buf,
					(u_int)p.opt->opt.maxlen);
			kmem_free(ptr, (u_int)sizeof(struct t_optmgmt));
			break;
	
		case T_DIS:
			/* LINTED pointer alignment */
			p.dis = (struct t_discon *)ptr;
			if (p.dis->udata.buf != NULL)
				kmem_free(p.dis->udata.buf,
					(u_int)p.dis->udata.maxlen);
			kmem_free(ptr, (u_int)sizeof(struct t_discon));
			break;
	
		case T_UNITDATA:
			/* LINTED pointer alignment */
			p.udata = (struct t_kunitdata *)ptr;
	
			if (p.udata->udata.udata_mp) {

				KTLILOG(0x8, "t_kfree: freeing mblk_t %x, ",
						p.udata->udata.udata_mp);
				KTLILOG(0x8, "ref %d\n",
				  p.udata->udata.udata_mp->b_datap->db_ref);

				freemsg(p.udata->udata.udata_mp);
			}

			if (p.udata->opt.buf != NULL)
				kmem_free(p.udata->opt.buf,
					(u_int)p.udata->opt.maxlen);
			if (p.udata->addr.buf != NULL) {

				KTLILOG(0x8, "t_kfree: freeing address %x, ",
						p.udata->addr.buf);
				KTLILOG(0x8, "len %d\n", p.udata->addr.maxlen);

				kmem_free(p.udata->addr.buf,
					(u_int)p.udata->addr.maxlen);
			}

			KTLILOG(0x8, "t_kfree: freeing t_kunitdata\n", 0);

			kmem_free(ptr, (u_int)sizeof(struct t_kunitdata));
			break;
	
		case T_UDERROR:
			/* LINTED pointer alignment */
			p.uderr = (struct t_uderr *)ptr;
			if (p.uderr->addr.buf != NULL)
				kmem_free(p.uderr->addr.buf,
					(u_int)p.uderr->addr.maxlen);
			if (p.uderr->opt.buf != NULL)
				kmem_free(p.uderr->opt.buf,
					(u_int)p.uderr->opt.maxlen);
			kmem_free(ptr, (u_int)sizeof(struct t_uderr));
			break;
	
		case T_INFO:
			break;
	
		default:
			error = EINVAL;
			break;
	}

	return error;
}
