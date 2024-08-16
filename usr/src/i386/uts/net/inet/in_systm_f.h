/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IN_SYSTM_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_IN_SYSTM_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/in_systm_f.h	1.4"
#ident	"$Header: $"

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

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/stream.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * On the 386 family processors structures needn't be aligned
 * to any particular boundry
 */

#ifdef DEBUG

/*
 * integral_pointer_t is a non pointer type that can hold  any
 * pointer type.
 */
typedef unsigned long		integral_pointer_t;

extern mblk_t *realignbp(mblk_t *);

#define BPTOSTRUCTPTR(bp, align) \
	(void *)(((integral_pointer_t)(bp)->b_rptr & ((align)-1)) ? \
		 realignbp((bp)) : (bp))->b_rptr

/*
 * Alignment restrictions for various structures passed in mblks
 */

#define _ALIGNOF_IFREQ			(1)
#define _ALIGNOF_ARPREQ			(1)
#define _ALIGNOF_ARPHDR			(1)
#define _ALIGNOF_ETHER_ARP		(1)
#define _ALIGNOF_IOCBLK_IN		(1)
#define _ALIGNOF_SOCKADDR_IN		(1)
#define _ALIGNOF_STRUCTPTR		(1)
#define _ALIGNOF_STRUCTPTR		(1)
#define _ALIGNOF_IOCBLK			(1)
#define _ALIGNOF_DL_PRIMITIVES		(1)
#define _ALIGNOF_T_PRIMITIVES		(1)
#define _ALIGNOF_N_PRIMITIVES		(1)
#define _ALIGNOF_DL_ERROR_ACK		(1)
#define _ALIGNOF_T_ERROR_ACK		(1)
#define _ALIGNOF_DL_OK_ACK		(1)
#define _ALIGNOF_T_OK_ACK		(1)
#define _ALIGNOF_DL_INFO_ACK		(1)
#define _ALIGNOF_DL_BIND_ACK		(1)
#define _ALIGNOF_DL_BIND_REQ		(1)
#define _ALIGNOF_N_BIND_REQ		(1)
#define _ALIGNOF_N_UNBIND_REQ		(1)
#define _ALIGNOF_DL_UNBIND_REQ		(1)
#define _ALIGNOF_DL_UNITDATA_REQ	(1)
#define _ALIGNOF_DL_UNITDATA_IND	(1)
#define _ALIGNOF_T_UNITDATA_IND		(1)
#define _ALIGNOF_T_UDERROR_IND		(1)
#define _ALIGNOF_N_UDERROR_IND		(1)
#define _ALIGNOF_LINKBLK		(1)
#define _ALIGNOF_STROPTIONS		(1)
#define _ALIGNOF_T_CONN_CON		(1)
#define _ALIGNOF_T_OPTMGMT_REQ		(1)
#define _ALIGNOF_IFRECORD		(1)
#define _ALIGNOF_IP			(1)
#define _ALIGNOF_IPSTAT			(1)
#define _ALIGNOF_ICMP			(1)
#define _ALIGNOF_ICMPSTAT		(1)
#define _ALIGNOF_IP_UNITDATA_REQ	(1)
#define _ALIGNOF_IP_UNITDATA_IND	(1)
#define _ALIGNOF_IP_CTLMSG		(1)
#define _ALIGNOF_IPASFRAG		(1)
#define _ALIGNOF_IPOPTION		(1)
#define _ALIGNOF_RTSTAT			(1)
#define _ALIGNOF_RTENTRY		(1)
#define _ALIGNOF_RTE			(1)
#define _ALIGNOF_IOCQP			(1)
#define _ALIGNOF_UDPIPHDR		(1)
#define _ALIGNOF_UDPSTAT		(1)

#else /* DEBUG */

#define BPTOSTRUCTPTR(bp, x) (void *)(bp)->b_rptr

#endif /* DEBUG */

/*
 * Network types.
 *
 * Internally the system keeps counters in the headers with bytes
 * in such and order that instructions on the local machine will work
 * on them.  These counters are modified into a standard networking
 * byte order (high-ender) before transmission at each protocol level.
 * The n_ types represent the types with the bytes in network order.
 */

typedef unsigned short n_short;		/* short as received from the net */
typedef unsigned long n_long;		/* long as received from the net */
typedef	unsigned long n_time;		/* ms since 00:00 GMT, byte rev */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IN_SYSTM_F_H */
