/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kalloc.c	1.9"
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
 *	t_kalloc.c, kernel TLI function to allocate memory for the
 *	various TLI primitives.
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
#include <proc/signal.h>
#include <net/ktli/t_kuser.h>
#include <mem/kmem.h>

extern	int	max(uint, uint);
static	void	_alloc_buf();

/*
 * t_kalloc(tiptr, struct_type, fields, ptr)
 *	Allocate memory for TLI primitives.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *	On success, ptr is set the structure required.
 *
 * Description:
 *	This routine allocates memory for various TLI
 *	primitives.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	struct_type		# type of struct to allocate
 *	fields			# fields to allocate within struct
 *	ptr			# pointer to allocated struct
 *	
 */
int
t_kalloc(TIUSER *tiptr, int struct_type, int fields, char **ptr)
{
	union structptrs {
		char	*caddr;
		struct t_bind *bind;
		struct t_call *call;
		struct t_discon *dis;
		struct t_optmgmt *opt;
		struct t_kunitdata *udata;
		struct t_uderr *uderr;
		struct t_info *info;
	}p;
	unsigned	dsize;

	if (ptr == NULL)
		return EINVAL;

	/*
	 * allocate appropriate structure and the specified
	 * fields within each structure. Initialize the
	 * 'buf' and 'maxlen' fields of each.
	 */
	switch (struct_type) {

		case T_BIND:
			p.bind = (struct t_bind *)
				kmem_zalloc((u_int)sizeof(struct t_bind),
							KM_SLEEP);
			if (fields & T_ADDR) {
				_alloc_buf(&p.bind->addr, tiptr->tp_info.addr);
			}
			*ptr = (char *)p.bind;
			return 0;

		case T_CALL:
			p.call = (struct t_call *)
				kmem_zalloc((u_int)sizeof(struct t_call),
							KM_SLEEP);
			if (fields & T_ADDR) {
				_alloc_buf(&p.call->addr, tiptr->tp_info.addr);
			}
			if (fields & T_OPT) {
				_alloc_buf(&p.call->opt,
						tiptr->tp_info.options);
			}
			if (fields & T_UDATA) {
				dsize = (int)max(tiptr->tp_info.connect,
						tiptr->tp_info.discon);
				_alloc_buf(&p.call->opt, dsize);
			}
			*ptr = (char *)p.call;
			return 0;

		case T_OPTMGMT:
			p.opt = (struct t_optmgmt *)
				kmem_zalloc((u_int)sizeof(struct t_optmgmt),
							KM_SLEEP);
			if (fields & T_OPT){
				_alloc_buf(&p.opt->opt,
						tiptr->tp_info.options);
			}
			*ptr = (char *)p.opt;
			return 0;

		case T_DIS:
			p.dis = (struct t_discon *)
				kmem_zalloc((u_int)sizeof(struct t_discon),
							KM_SLEEP);
			if (fields & T_UDATA){
				_alloc_buf(&p.dis->udata,
						tiptr->tp_info.discon);
			}
			*ptr = (char *)p.dis;
			return 0;
	
		case T_UNITDATA:
			p.udata = (struct t_kunitdata *)
				kmem_zalloc((u_int)sizeof(struct t_kunitdata),
							KM_SLEEP);
			if (fields & T_ADDR){
				_alloc_buf(&p.udata->addr,
						tiptr->tp_info.addr);
			}
			else	p.udata->addr.maxlen = p.udata->addr.len = 0;
	
			if (fields & T_OPT){
				_alloc_buf(&p.udata->opt,
						tiptr->tp_info.options);
			}
			else	p.udata->opt.maxlen = p.udata->opt.len = 0;
	
			if (fields & T_UDATA){
				p.udata->udata.udata_mp = NULL;
				p.udata->udata.buf = NULL;
				p.udata->udata.maxlen = tiptr->tp_info.tsdu;
				p.udata->udata.len = 0;
			}
			else {
				p.udata->udata.maxlen = p.udata->udata.len = 0;
			}
			*ptr = (char *)p.udata;
			return 0;
	
		case T_UDERROR:
			p.uderr = (struct t_uderr *)
				kmem_zalloc((u_int)sizeof(struct t_uderr),
							KM_SLEEP);
			if (fields & T_ADDR){
				_alloc_buf(&p.uderr->addr,
						tiptr->tp_info.addr);
			}
			if (fields & T_OPT){
				_alloc_buf(&p.uderr->opt,
						tiptr->tp_info.options);
			}
			*ptr = (char *)p.uderr;
			return 0;
	
		case T_INFO:
			p.info = (struct t_info *)
				kmem_zalloc((u_int)sizeof(struct t_info),
							KM_SLEEP);
			*ptr = (char *)p.info;
			return 0;
	
		default:
			return EINVAL;
	}
}
	
/*
 * _alloc_buf(buf, n)
 *	Allocate a buffer of given size.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	No return value.
 *
 * Description:
 *	This routine allocates memory of given size.
 *
 * Parameters:
 *
 *	buf			# pointer to allocated memory
 *	n			# amount of memory to allocate
 *	
 */
static void
_alloc_buf(struct netbuf *buf, long n)
{
	switch(n) {
		case -1L:
			buf->buf = (char *)kmem_zalloc((u_int)1024, KM_SLEEP);
			buf->maxlen = 1024;
			buf->len = 0;
			break;

		case 0L :

		case -2L:
			buf->buf = NULL;
			buf->maxlen = 0;
			buf->len = 0;
			break;

		default:
			buf->buf = (char *)kmem_zalloc((u_int)n, KM_SLEEP);
			buf->maxlen = n;
			buf->len = 0;
			break;
	}
}
