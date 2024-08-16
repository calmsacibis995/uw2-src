/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_MP_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_MP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp_mp.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL)

extern rwlock_t *tcp_addr_rwlck;
extern lock_t *tcp_conn_lck;
extern lock_t *tcp_debug_lck;
extern lock_t *tcp_iss_lck;

#define TCPSTAT_INC(s)		(++tcpstat.s)
#define TCPSTAT_ADD(s, n)	(tcpstat.s += n)

#define TCP_MAXIDLE_READ()	(tcp_maxidle)
#define TCP_MAXIDLE_WRITE(v)	(tcp_maxidle = v)

#define TCP_ISS_READ() (tcp_iss)

extern lkinfo_t tcp_inp_lkinfo;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_MP_H */
