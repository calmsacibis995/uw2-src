/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifndef _NET_INET_IN_SYSTM_H	/* wrapper symbol for kernel use */
#define _NET_INET_IN_SYSTM_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/in_systm.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

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

/*
 * Miscellaneous internetwork definitions for kernel.
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/in_systm_f.h>	/* PORTABILITY */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL)

#include <netinet/in_systm_f.h>		/* PORTABILITY */
#include <sys/stream.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

#define CHECKSIZE(bp, size) \
	if (((bp) = reallocb((bp), (size), 0)) == NULL) { return; }

#define BPTOIOCBLK(bp) \
	((struct iocblk *)BPTOSTRUCTPTR((bp), _ALIGNOF_IOCBLK))
#define BPTODL_PRIMITIVES(bp) \
	((union DL_primitives *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_PRIMITIVES))
#define BPTOT_PRIMITIVES(bp) \
	((union T_primitives *)BPTOSTRUCTPTR((bp), _ALIGNOF_T_PRIMITIVES))
#define BPTON_PRIMITIVES(bp) \
	((union N_primitives *)BPTOSTRUCTPTR((bp), _ALIGNOF_N_PRIMITIVES))
#define BPTODL_ERROR_ACK(bp) \
	((dl_error_ack_t *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_ERROR_ACK))
#define BPTOT_ERROR_ACK(bp) \
	((struct T_error_ack *)BPTOSTRUCTPTR((bp), _ALIGNOF_T_ERROR_ACK))
#define BPTODL_OK_ACK(bp) \
	((dl_ok_ack_t *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_OK_ACK))
#define BPTOT_OK_ACK(bp) \
	((struct T_ok_ack *)BPTOSTRUCTPTR((bp), _ALIGNOF_T_OK_ACK))
#define BPTODL_INFO_ACK(bp) \
	((dl_info_ack_t *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_INFO_ACK))
#define BPTODL_BIND_ACK(bp) \
	((dl_bind_ack_t *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_BIND_ACK))
#define BPTODL_BIND_REQ(bp) \
	((dl_bind_req_t *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_BIND_REQ))
#define BPTON_BIND_REQ(bp) \
	((struct N_bind_req *)BPTOSTRUCTPTR((bp), _ALIGNOF_N_BIND_REQ))
#define BPTON_UNBIND_REQ(bp) \
	((struct N_unbind_req *)BPTOSTRUCTPTR((bp), _ALIGNOF_N_UNBIND_REQ))
#define BPTODL_UNBIND_REQ(bp) \
	((dl_unbind_req_t *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_UNBIND_REQ))
#define BPTODL_UNITDATA_REQ(bp) \
	((dl_unitdata_req_t *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_UNITDATA_REQ))
#define BPTODL_UNITDATA_IND(bp) \
	((dl_unitdata_ind_t *)BPTOSTRUCTPTR((bp), _ALIGNOF_DL_UNITDATA_IND))
#define BPTOT_UNITDATA_IND(bp) \
	((struct T_unitdata_ind *)BPTOSTRUCTPTR((bp), _ALIGNOF_T_UNITDATA_IND))
#define BPTOT_UDERROR_IND(bp) \
	((struct T_uderror_ind *)BPTOSTRUCTPTR((bp), _ALIGNOF_T_UDERROR_IND))
#define BPTON_UDERROR_IND(bp) \
	((struct N_uderror_ind *)BPTOSTRUCTPTR((bp), _ALIGNOF_N_UDERROR_IND))
#define BPTOLINKBLK(bp) \
	((struct linkblk *)BPTOSTRUCTPTR((bp), _ALIGNOF_LINKBLK))
#define BPTOSTROPTIONS(bp) \
	((struct stroptions *)BPTOSTRUCTPTR((bp), _ALIGNOF_STROPTIONS))
#define BPTOT_CONN_CON(bp) \
	((struct T_conn_con *)BPTOSTRUCTPTR((bp), _ALIGNOF_T_CONN_CON))
#define BPTOT_OPTMGMT_REQ(bp) \
	((struct T_optmgmt_req *)BPTOSTRUCTPTR((bp), _ALIGNOF_T_OPTMGMT_REQ))

#else /* _KERNEL */

#include <netinet/in_systm_f.h>		/* PORTABILITY */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _NET_INET_IN_SYSTM_H */
