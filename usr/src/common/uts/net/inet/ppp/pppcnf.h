/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_PPP_PPPCNF_H	/* wrapper symbol for kernel use */
#define _NET_INET_PPP_PPPCNF_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/ppp/pppcnf.h	1.5"
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
 * System V STREAMS TCP - Release 3.0 
 *
 * Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI) 
 * All Rights Reserved. 
 *
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code. 
 *
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates. 
 *
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies. 
 */


#ifdef _KERNEL_HEADERS

#include <net/inet/byteorder.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/byteorder.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(INKERNEL)

#define EXACT_LN 0
#define MIN_LN 1
#define MAX_LN 2

typedef struct cnf_opt_tbl_s {
	unchar min_max_ex_ln;
	unchar opt_ln;
	ushort (*conf)();
} cnf_opt_tbl_t;

struct co_icp_shr_dtp_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	unchar	*shr_dtp;
};
#define CO_ICP_SHR_DTP_LN (sizeof(struct co_icp_shr_dtp_s))

#pragma pack(1)
struct co_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
};
#define CO_LN (sizeof(struct co_s))
#pragma pack()

#pragma pack(1)
struct co_lcp_max_rcv_un_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	ushort	max_rcv_un;
};
#define CO_LCP_MAX_RCV_UN_LN (sizeof(struct co_lcp_max_rcv_un_s))
#pragma pack()

#pragma pack(1)
struct co_lcp_asy_ctl_mp_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	ulong	asy_ctl_mp;
};
#define CO_LCP_ASY_CTL_MP_LN (sizeof(struct co_lcp_asy_ctl_mp_s))
#pragma pack()

#pragma pack(1)
struct co_lcp_auth_tp_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	ushort	auth_tp;
};
#define CO_LCP_AUTH_TP_LN (sizeof(struct co_lcp_auth_tp_s))
#pragma pack()

#pragma pack(1)
struct co_lcp_mgc_no_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	ulong	mgc_no;
};
#define CO_LCP_MGC_NO_LN (sizeof(struct co_lcp_mgc_no_s))
#pragma pack()

#pragma pack(1)
struct co_lcp_fcs_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	unchar	fcs_opt;
};
#define CO_LCP_FCS_LN (sizeof(struct co_lcp_fcs_s))
#pragma pack()

#pragma pack(1)
struct co_lcp_lnk_qual_mn_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	ulong	lnk_qual_mn;
};
#define CO_LCP_LNK_QUAL_MN_LN (sizeof(struct co_lcp_lnk_qual_mn_s))
#pragma pack()

#define co_lcp_prot_fld_cmp_s co_s
#define CO_LCP_PROT_FLD_CMP_LN CO_LN

#define co_lcp_addr_fld_cmp_s co_s
#define CO_LCP_ADDR_FLD_CMP_LN CO_LN

#pragma pack(1)
struct co_lcp_padding_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	unchar	max_pad;
};
#define CO_LCP_PADDING_LN (sizeof(struct co_lcp_padding_s))
#pragma pack()

#pragma pack(1)
struct co_lcp_callback_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	unchar	operation;
};
#define CO_LCP_CALLBACK_LN (sizeof(struct co_lcp_callback_s))
#pragma pack()

#pragma pack(1)
struct co_lcp_frames_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
};
#define CO_LCP_FRAMES_LN (sizeof(struct co_lcp_frames_s))
#pragma pack()

#pragma pack(1)
struct co_ipcp_cmp_tp_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	ushort	cmp_tp;
};
#define CO_IPCP_CMP_TP_LN (sizeof(struct co_ipcp_cmp_tp_s))
#pragma pack()

#pragma pack(1)
struct co_vj_cmp_s {
	unchar	vjc_msi;	/* Max-Slot-ID */
	unchar	vjc_csi;	/* Comp-Slot-ID */
};
#define	CO_VJ_CMP_LN (sizeof(struct co_vj_cmp_s))
#pragma pack()

/* valid value(s) for cmp_tp */
#define VJC_COMP_TCPIP	0x002D
#define BAD_VJC_COMP_TCPIP	0x0037	/* bogus value from RFC1172 */

#pragma pack(1)
struct co_ipcp_ipaddrs_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	ulong	src_ipaddr;
	ulong	dst_ipaddr;
};
#define CO_IPCP_IPADDRS_LN (sizeof(struct co_ipcp_ipaddrs_s))
#pragma pack()

#pragma pack(1)
struct co_ipcp_ipaddr_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	ulong	ipaddr;
};
#define CO_IPCP_IPADDR_LN (sizeof(struct co_ipcp_ipaddr_s))
#pragma pack()

struct co_icp_phn_no_s {
	unchar	cnf_tp;
	unchar	cnf_ln;
	unchar	phone_no[20];
};

#pragma pack(1)
union	cnf_opt_u {
	struct co_s co;
	struct co_ipcp_ipaddrs_s co_ipcp_ipaddrs;
	struct co_ipcp_cmp_tp_s co_ipcp_cmp_tp;
	struct co_ipcp_ipaddr_s co_ipcp_ipaddr; 
};
#define COU_LN	sizeof(struct cnf_opt_s)
#pragma pack()

#endif	/* defined(_KERNEL) || defined(INKERNEL) */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_PPP_PPPCNF_H */
