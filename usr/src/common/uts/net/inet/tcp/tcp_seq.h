/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_SEQ_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_SEQ_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp_seq.h	1.6"
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

/*
 * Portions of this source code were provided by UniSoft
 * under a development agreement with AT&T and Motorola.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/tcp/tcp.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <netinet/tcp.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * TCP sequence numbers are 32 bit integers operated on with modular
 * arithmetic.	These macros can be used to compare such integers.
 */

#if defined(vax) || defined(m88k)

#define SEQ_LT(a, b)	((int)((a)-(b)) < 0)
#define SEQ_LEQ(a, b)	((int)((a)-(b)) <= 0)
#define SEQ_GT(a, b)	((int)((a)-(b)) > 0)
#define SEQ_GEQ(a, b)	((int)((a)-(b)) >= 0)

#else

#define SEQ_LT(a, b)	(((a)-(b))&0x80000000)
#define SEQ_LEQ(a, b)	(!SEQ_GT(a, b))
#define SEQ_GT(a, b)	SEQ_LT(b, a)
#define SEQ_GEQ(a, b)	(!SEQ_LT(a, b))

#endif

/*
 * Macros to initialize tcp sequence numbers for
 * send and receive from initial send and receive
 * sequence numbers.
 */
#define TCP_RCVSEQINIT(tp)	(tp)->rcv_adv = (tp)->rcv_nxt = (tp)->irs + 1

#define TCP_SNDSEQINIT(tp)	(tp)->snd_una = (tp)->snd_nxt = \
		(tp)->snd_max = (tp)->snd_up = (tp)->iss

#define TCP_ISSINCR	(125 * 1024)	/* increment for tcp_iss each second */

extern tcp_seq tcp_iss;		/* TCP initial send seq # */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_SEQ_H */
