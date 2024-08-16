/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:net/inet/netlib_f.c	1.5"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991 UNIX System Laboratories, Inc.
 *  	          All rights reserved.
 */

#include <io/log/log.h>
#include <io/stream.h>
#include <net/dlpi.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_systm.h>
#include <net/inet/in_var.h>
#include <net/inet/route/route.h>
#include <net/inet/protosw.h>
#include <net/inet/strioc.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <net/sockio.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#include <io/ddi.h>		/* must come last */

/*
 * int makeopt(mblk_t *bp, int level, int name, void *ptr, int len)
 *	Format option in message.  The specified option and value are
 *	copied into the last mblk at b_wptr.  b_wptr is incremented
 *	accordingly.  If there is not enough space in the last mblk, a
 *	new mblk is allocated.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Where to copy option
 *	  level		Protocol level affected
 *	  name		Option to modify
 *	  ptr		Pointer to option value
 *	  len		Length of option value
 *
 *	Locking:
 *	  None.
 *
 *	Possible Returns:
 *	  1	Success
 *	  0	Failure (Unable to allocate mblk).
 */
int
makeopt(mblk_t *bp, int level, int name, void *ptr, int len)
{
	struct opthdr *opt;
	int rlen, tlen;

	for (; bp->b_cont; bp = bp->b_cont)
		;
	rlen = OPTLEN(len);
	tlen = sizeof (struct opthdr) + rlen;
	if ((bp->b_datap->db_lim - bp->b_wptr) < tlen) {
		if (!(bp->b_cont = allocb(max(tlen, 64), BPRI_MED)))
			return 0;
		bp = bp->b_cont;
		bp->b_datap->db_type = M_PROTO;
	}
	/* LINTED pointer alignment */
	opt = (struct opthdr *)bp->b_wptr;
	opt->level = level;
	opt->name = name;
	opt->len = rlen;
	if (rlen)
		bzero(OPTVAL(opt), rlen);
	if (len)
		bcopy(ptr, OPTVAL(opt), len);
	bp->b_wptr += tlen;

	return 1;
}
