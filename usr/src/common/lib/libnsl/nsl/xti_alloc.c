/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/xti_alloc.c	1.1"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "_import.h"

extern struct _ti_user *_t_checkfd();

char *
_xti_alloc(fd, struct_type, fields)
int fd;
int struct_type;
int fields;
{
	struct strioctl strioc;
	struct T_info_ack info;
	sigset_t		oldmask;
	sigset_t		newmask;
	union structptrs {
		char	*caddr;
		struct t_bind *bind;
		struct t_call *call;
		struct t_discon *dis;
		struct t_optmgmt *opt;
		struct t_unitdata *udata;
		struct t_uderr *uderr;
		struct t_info *info;
	} p;
	unsigned dsize;
	struct _ti_user *tiptr;
	int	ignore_unsupported_fields = 0;	/* set if fields == T_ALL */

	if(struct_type == T_INFO) {
		if ((p.info = (struct t_info *)
			calloc(1, (unsigned)sizeof(struct t_info))) == NULL)
				goto out;
		return((char *)p.info);
	}

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(NULL);
	}
	MUTEX_UNLOCK(&tiptr->lock);
	
	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);

	/*
	 * Get size info for T_ADDR, T_OPT, and T_UDATA fields
	 */
	info.PRIM_type = T_INFO_REQ;
	strioc.ic_cmd = TI_GETINFO;
	strioc.ic_timout = -1;
	strioc.ic_len = sizeof(struct T_info_req);
	strioc.ic_dp = (char *)&info;
	if (ioctl(fd, I_STR, &strioc) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		set_t_errno(TSYSERR);
		return(NULL);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	if (strioc.ic_len != sizeof(struct T_info_ack)) {
		errno = EIO;
		set_t_errno(TSYSERR);
		return(NULL);
	}
	

	/*
	 * Malloc appropriate structure and the specified
	 * fields within each structure.  Initialize the
	 * 'buf' and 'maxlen' fields of each.
	 *
	 * Fields not supported by the transport provider contain
	 * a -2 value for size in the info struct (obtained above).
	 * If the caller requests these fields, return an error
	 * and set  t_errno=TSYSERR,  errno=EINVAL.
	 *
	 * One exception:
	 * If fields == T_ALL, allocate only what the transport
	 * provider supports, and ignore what it does not, but
	 * don't return an error.
	 */

	if (fields == T_ALL)
		ignore_unsupported_fields++;

	switch (struct_type) {

	case T_BIND:
		if ((p.bind = (struct t_bind *)
			calloc(1, (unsigned)sizeof(struct t_bind))) == NULL)
				goto out;
		if (fields & T_ADDR) {
			if (_alloc_buf(&p.bind->addr, info.ADDR_size) < 0)
				goto out;
		}
		return((char *)p.bind);

	case T_CALL:
		/*
		 * TP_NOTSUPPORTED is defined as -2 in sys/tihdr.h but
		 * only if _KERNEL or _KMEMUSER is #defined, so hard-code.
		 */
		if ( !ignore_unsupported_fields &&
		    (fields & T_UDATA) && (info.CDATA_size == -2)) {
                        set_t_errno(TSYSERR);
			errno = EINVAL;
                        return(NULL);
                }
		if (info.SERV_type == T_CLTS) {
                        set_t_errno(TNOSTRUCTYPE);
                        return(NULL);
                }

		if ((p.call = (struct t_call *)
			calloc(1, (unsigned)sizeof(struct t_call))) == NULL)
				goto out;
		if (fields & T_ADDR) {
			if (_alloc_buf(&p.call->addr, info.ADDR_size) < 0)
				goto out;
		}
		if (fields & T_OPT) {
			if (_alloc_buf(&p.call->opt, info.OPT_size) < 0)
				goto out;
		}
		if ((fields & T_UDATA) && (info.CDATA_size != -2)) {
			dsize = _t_max(info.CDATA_size, info.DDATA_size);
			if (_alloc_buf(&p.call->udata, dsize) < 0)
				goto out;
		}
		return((char *)p.call);

	case T_OPTMGMT:
		if ((p.opt = (struct t_optmgmt *)
			calloc(1, (unsigned)sizeof(struct t_optmgmt))) == NULL)
				goto out;
		if (fields & T_OPT){
			if (_alloc_buf(&p.opt->opt, info.OPT_size) < 0)
				goto out;
		}
		return((char *)p.opt);

	case T_DIS:
		if ( !ignore_unsupported_fields &&
		    (fields & T_UDATA) && (info.DDATA_size == -2)) {
                        set_t_errno(TSYSERR);
			errno = EINVAL;
                        return(NULL);
                }
		if (info.SERV_type == T_CLTS) {
                        set_t_errno(TNOSTRUCTYPE);
                        return(NULL);
                }

		if ((p.dis = (struct t_discon *)
			calloc(1, (unsigned)sizeof(struct t_discon))) == NULL)
				goto out;
		if ((fields & T_UDATA) && (info.DDATA_size != -2)) {
			if (_alloc_buf(&p.dis->udata, info.DDATA_size) < 0)
				goto out;
		}
		return((char *)p.dis);

	case T_UNITDATA:
		 if (info.SERV_type != T_CLTS) {
                        set_t_errno(TNOSTRUCTYPE);
                        return(NULL);
                }

		if ((p.udata = (struct t_unitdata *)
			calloc(1, (unsigned)sizeof(struct t_unitdata))) == NULL)
				goto out;
		if (fields & T_ADDR){
			if (_alloc_buf(&p.udata->addr, info.ADDR_size) < 0)
				goto out;
		}
		if (fields & T_OPT){
			if (_alloc_buf(&p.udata->opt, info.OPT_size) < 0)
				goto out;
		}
		if (fields & T_UDATA){
			if (_alloc_buf(&p.udata->udata, info.TSDU_size) < 0)
				goto out;
		}
		return((char *)p.udata);

	case T_UDERROR:
		if (info.SERV_type != T_CLTS) {
                        set_t_errno(TNOSTRUCTYPE);
                        return(NULL);
                }

		if ((p.uderr = (struct t_uderr *)
			calloc(1, (unsigned)sizeof(struct t_uderr))) == NULL)
				goto out;
		if (fields & T_ADDR){
			if (_alloc_buf(&p.uderr->addr, info.ADDR_size) < 0)
				goto out;
		}
		if (fields & T_OPT){
			if (_alloc_buf(&p.uderr->opt, info.OPT_size) < 0)
				goto out;
		}
		return((char *)p.uderr);

	default:
		set_t_errno(TNOSTRUCTYPE);
		return(NULL);
	}

	/*
	 * Clean up. Set errno to ENOMEM if
	 * memory could not be allocated.
	 */
out:
	if (p.caddr)
		t_free(p.caddr, struct_type);

	set_t_errno(TSYSERR);
	errno = ENOMEM;
	return(NULL);
}

_alloc_buf(buf, n)
struct netbuf *buf;
{
	switch(n)
	{
		case -1:
			if ((buf->buf = calloc(1, 1024)) == NULL)
				return(-1);
			else buf->maxlen = 1024;
			break;

		case 0:
		case -2:
			buf->buf = NULL;
			buf->maxlen = 0;
			break;

		default:
			if ((buf->buf = calloc(1, n)) == NULL)
				return(-1);
			else buf->maxlen = n;
			break;
	}
	return(0);
}
