/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/rawip/rawip_cb.c	1.6"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	STREAMware TCP/IP Release 1.0
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *		  All rights reserved.
 *
 */

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <mem/kmem.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_pcb.h>
#include <net/inet/in_var.h>
#include <net/inet/insrem.h>
#include <net/inet/route/route.h>
#include <net/inet/protosw.h>
#include <net/inet/rawip/rawip_hier.h>
#include <net/socket.h>
#include <net/socketvar.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

#include <io/ddi.h>		/* must come last */

extern rwlock_t *prov_rwlck;

/*
 * Routines to manage the raw protocol control blocks.
 *
 * TODO:
 *	hash lookups by protocol family/protocol + address family
 *	take care of unique address problems per AF?
 *	redo address binding to allow wildcards
 */

/*
 * int rip_bind(struct inpcb *inp, struct sockaddr_in *addr)
 *
 * Calling/Exit State:
 *	Called from rip_state().
 *	inp->inp_rwlck is held in exclusive mode on entry.
 *
 */
int
rip_bind(struct inpcb *inp, struct sockaddr_in *addr)
{
/* BEGIN DUBIOUS */
	pl_t	pl;

	/*
	 * Should we verify address not already in use?
	 * Some say yes, others no.
	 */
	if (addr->sin_family == AF_INET || addr->sin_family == AF_INET_BSWAP) {
		pl = RW_RDLOCK(prov_rwlck, plstr);
		if (addr->sin_addr.s_addr && !prov_withaddr(addr->sin_addr)) {
			RW_UNLOCK(prov_rwlck, pl);
			return EADDRNOTAVAIL;
		}
		RW_UNLOCK(prov_rwlck, pl);
	} else
		return EAFNOSUPPORT;
/* END DUBIOUS */

	inp->inp_laddr = addr->sin_addr;
	inp->inp_family = addr->sin_family;

	return 0;
}

/*
 * int rip_connaddr(struct inpcb *inp, struct sockaddr_in *addr)
 *	Associate a peer's address with a raw connection block.
 *
 * Calling/Exit State:
 *	Called from ripuwsrv() and rip_state().
 *	inp->inp_rwlck is held in exclusive mode on entry.
 */
int
rip_connaddr(struct inpcb *inp, struct sockaddr_in *addr)
{
	if (addr->sin_family != AF_INET && addr->sin_family != AF_INET_BSWAP)
		return EAFNOSUPPORT;

	inp->inp_faddr = addr->sin_addr;
	inp->inp_family = addr->sin_family;

	return 0;
}
