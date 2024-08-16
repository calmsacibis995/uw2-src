/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_DEBUG_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_DEBUG_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp_debug.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *		PROPRIETARY NOTICE (Combined)
 *
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *
 *
 *
 *		Copyright Notice
 *
 *  Notice of copyright on this source code product does not indicate
 *  publication.
 *
 *	(c) 1986,1987,1988,1989	 Sun Microsystems, Inc.
 *	(c) 1983,1984,1985,1986,1987,1988,1989	AT&T.
 *		  All rights reserved.
 */

/*
 * System V STREAMS TCP - Release 3.0
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 * All Rights Reserved.
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.	 The copyright above does not evidence any actual or intended
 * publication of this source code.
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.	This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates.
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/in_systm.h>		/* REQUIRED */
#include <net/inet/tcp/tcp_kern.h>	/* REQUIRED */
#include <net/inet/tcp/tcp_var.h>	/* REQUIRED */
#include <net/inet/tcp/tcpip.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <netinet/in_systm.h>		/* REQUIRED */
#include <netinet/tcp_kern.h>		/* REQUIRED */
#include <netinet/tcp_var.h>		/* REQUIRED */
#include <netinet/tcpip.h>		/* REQUIRED */

#elif defined(_KMEMUSER)

#include <netinet/tcp_kern.h>		/* REQUIRED */
#include <netinet/tcp_var.h>		/* REQUIRED */
#include <netinet/tcpip.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

struct tcp_debug {
	n_time		td_time;
	short		td_act;
	short		td_ostate;
	caddr_t		td_tcb;
	struct tcpiphdr td_ti;
	int		td_req;
	struct tcpcb	td_cb;
};

#define TA_INPUT	0
#define TA_OUTPUT	1
#define TA_USER		2
#define TA_RESPOND	3
#define TA_DROP		4
#define TA_TIMER	5

#ifdef TANAMES
char *tanames[] = {
	"input", "output", "user", "respond", "drop", "timer",
};
#endif /* TANAMES */

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL
extern int tcp_ndebug;
extern int tcpconsdebug;
extern int tcpalldebug;
extern int tcp_debx;
extern struct tcp_debug	tcp_debug[];

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_DEBUG_H */
