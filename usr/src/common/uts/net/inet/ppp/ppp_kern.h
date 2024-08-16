/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_PPP_PPP_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_PPP_PPP_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/ppp/ppp_kern.h	1.12"
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

#ifdef _KERNEL

#include <net/inet/ppp/ppp.h>		/* REQUIRED */
#include <net/inet/ppp/pppcnf.h>	/* REQUIRED */

/* hdlc packet fields */
#define CHK_OPT			(CNF_NAK - 2)
#define FMT_OPT			(CNF_NAK - 1)
#define FMT_OPT_W_CNF_NAK	CNF_NAK
#define FMT_OPT_W_CNF_REJ	CNF_REJ
#define APPEND			(CNF_REJ + 1)
#define CNF_REJ			4   /* Configure reject */
#define CHK_LCP_ID		(USHRT_MAX - ((ushort)((ushort)USHRT_MAX << 1) >>1))
#define FRMSG			((ushort)CHK_LCP_ID >>1)
#define HDLC_ADDR		0xFF
#define HDLC_UI_CTRL		0x03
#define OSI_PROTO		0x0023
#define OSICP_PROTO		0x8023
#define IP_PROTO		0x0021
#define PPP_VJC_COMP		0x002d		/* VJ Comp TCP/IP 1 */
#define PPP_VJC_UNCOMP		0x002f		/* VJ Comp TCP/IP 2 */
#define IPCP_PROTO		0x8021
#define LCP_PROTO		0xc021
#define PAP_PROTO		0xc023
#define ICP_PROTO		0xfeff /*largest value: high order even, low order odd*/
#define N_PPP_NCP	1
#define N_PPP_PROTO	4
#define ICP_LAYER	3
#define LCP_LAYER	2
#define NCP_LAYER	1
#define NTW_LAYER	0

/* ppp hdlc packet structures */

struct	hdlc_pkt_hdr_s {	/* hdlc packet header for ppp */
	unchar	hdlc_addr;
	unchar	hdlc_ctrl;
	ushort	hdlc_proto;
};
#define HPH_LN	sizeof(struct hdlc_pkt_hdr_s)

struct	lcp_pkt_hdr_s {		/* link control protocol packet header */
	unchar	lcp_code;
	unchar	lcp_id;
	ushort	lcp_len;
};
#define LPH_LN	sizeof(struct lcp_pkt_hdr_s)

struct hdlc_e_pkt_s {
	struct hdlc_pkt_hdr_s hdr;	/* offset = 0 bytes */
	struct lcp_pkt_hdr_s lcp;	/* offset = 4 bytes */
};
#define HEP_LN sizeof(struct hdlc_e_pkt_s)

struct cnf_req_s {   /* Configure request */
	struct co_s co;
};

struct cnf_ack_s {   /* Configure Acknowledge */
	struct co_s co;
};

struct cnf_nak_s {   /* Configure No Acknowledge */
	struct co_s co;
};

struct cnf_rej_s {   /* Configure reject */
	struct co_s co;
};

struct trm_req_ack_s {   /* Terminate request/acknowledge */
	unchar data[2];
};

struct cod_rej_s {   /* Code Reject -- Invalid code */
	struct	lcp_pkt_hdr_s lcp;
	unchar data[2];
};

struct prot_rej_s {   /* Protocol Reject */
	ushort rej_proto;
	struct	lcp_pkt_hdr_s lcp;
	unchar data[2];
};

struct echo_req_rpl_s {   /* Echo reply/request */
	ulong magic_number;
	unchar data[2];
};

struct dscd_req_s {   /* Discard Request */
	ulong magic_number;
	unchar data[2];
};

typedef struct hdlc_pkt_s {
	struct hdlc_pkt_hdr_s hdr;	/* offset = 0 bytes */
	struct lcp_pkt_hdr_s lcp;	/* offset = 4 bytes */
	union {
		unchar data[2];				/* offset = 8 bytes */
		struct cnf_req_s cnf_req;   /* Configure request */
		struct cnf_ack_s cnf_ack;   /* Configure Acknowledge */
		struct cnf_nak_s cnf_nak;   /* Configure No Acknowledge */
		struct cnf_rej_s cnf_rej;   /* Configure reject */
		struct trm_req_ack_s trm_req_ack;   /* Terminate request/acknowledge */
		struct cod_rej_s cod_rej;   /* Code Reject -- Invalid code */
		struct prot_rej_s prot_rej;   /* Protocol Reject */
		struct echo_req_rpl_s echo_req_rpl;   /* Echo reply/request */
		struct dscd_req_s dscd_req;   /* Discard Request */
	}data;
}hdlc_pkt_t;

#define PROT_FLD_CMP	0x1 	/* this removes one byte from the header */
#define ADR_CTL_CMP	0x2 	/* this removes two bytes from the header */

#if defined(TCPCOMPRESSION)
#define	TCP_IP_HDR_CMP	0x4	/* do Van Jacobson TCP/IP header compression */
#define	VJC_CSI		0x8	/* Compress the Slot ID */
#endif	/* defined(TCPCOMPRESSION) */

#define IPADDRESS	0x10	/* do IP address option */
#define NEWADDR		0x20	/* Use new ip-address: option 3 instead of 1 */
#define PAP		0x40	/* do passwd authentication */
#define MGC		0x80	/* do magic number */
#define MRU		0x100	/* do MRU negotiation */
#define ACCM		0x200	/* do ACCM negotiation */

typedef struct ppp_asyh_shr_s {
	ushort	flgs;
	ushort	mru;
	ulong	accm;
	ulong	neg_accm;
	ushort	auth_proto;
	ulong	auth_state;
	ulong	mgc_no;
	int	debug;
} ppp_asyh_shr_t;

typedef struct ppp_asyh_lr_s {
	lock_t		*lr_lck;	/* lock protecting this structure */
	int		lr_refcnt;	/* reference count */
	ppp_asyh_shr_t	local;		/* shared data for local side */
	ppp_asyh_shr_t	remote;		/* shared data for remote side */
} ppp_asyh_lr_t;

/* per physical (usually tty) unit structure */
typedef struct ppp_ppc_s {
	lock_t	*lp_lck;		/* lock protecting this structure */
	int	lp_refcnt;		/* do not remove/deallocate if > 0 */
	queue_t *ppp_lwrq;		/* write queue of lower stream */
	int	l_index;		/* link index */
	ushort	protocol;
	unchar	prot_grp_i;		/* protocol group index */
	unchar	prot_i;			/* protocol index */
	unchar	ppp_state[N_PPP_PROTO];	/* state table */
	unchar  pend_open[N_PPP_PROTO]; /* pending open table */
	unchar	ppp_open_type;		/* open type */
	unchar	perm_ppp;		/* dedicated PPP link */
	unchar	lcp_id;			/* current id field value */
	unchar	lst_lcp_id;		/* last id filed value */	
	unchar	lst_auth_id;		/* last id sent in PAP request */	
	unchar	max_cnf_retries;	/* Max-Configure */
	unchar	max_trm_retries;	/* Max-Termination */
	unchar	max_nak;		/* Max-cnf NAK */
	unchar	restart_cnt;		/* restart counter */
	unchar	loopback_cnt;		/* loopback counter */
	unchar	bad_cnf_retries;	
	unchar	conf_ack_timeout;	/* Timeout value */
	int	old_ppp;		/* remote side using old_ppp */
	unchar	cnf_ack_pend;		/* wait for configure ack */
	unchar	trm_ack_pend;		/* wait for terminate ack */
	unchar	pap_timeout;		/* PAP timeout value */
	ushort	prs_tm_wo_cnf_ack; /* present time w/o cnf_ack (in seconds) */
	ushort	auth_tm_wo_cnf_ack; /* present time w/o auth_ack (in seconds) */
	ushort	auth_tm_wo_cnf_req; /* present time w/o auth_req (in seconds) */
	unchar	prs_tm_wo_data;	/* present time w/o data (in minutes) */
	unchar	max_tm_wo_data;	/* maximum time w/o data (min) before close*/
	ppp_asyh_lr_t	*lp_lrp;	/* data shared with hdlc */
	struct ppp_ppc_s	*lp_next;
	struct ppp_ppc_s	*lp_prev;
	struct ppp_dlpi_sp_s *ppp_dlpi_sp; /* output consumer for this unit */
	struct ppp_ppc_s *ppp_nxt_sp_ppc;  /* next physical unit for this output consumer */
	struct ppp_ppc_s *ppp_nxt_tp_ppc;  /* next physical unit of this type */
	struct	ifstats ppc_stats;	/* netstat stuff */

#if defined(TCPCOMPRESSION)
	struct	incompress *ppp_comp;	/* tcp compression struct */
#endif	/* defined(TCPCOMPRESSION) */

	struct	pswdauth pid_pwd;	/* pap struct */
	char	ppc_ifname[IFNAMSIZ];
} ppp_ppc_t;

/* Other header compression stuff */
/* IS_COMP_PROT returns true if p (proto field from hdlc) is one of VJ Compressed packet types */
#define IS_COMP_PROT(p)	((p) == PPP_VJC_COMP || (p) == PPP_VJC_UNCOMP)

/*
 * COMPRESS_TYPE returns the type of the packet that the in_{un}compress_tcp
 * routines use given the hdlc pakcet type.  It is assumed that the proto p
 * is one of PPP_VJC_COMP or PPP_VJC_UNCOMP, so call IS_COMP_PROTO before
 * calling COMPRESS_TYPE.
 */
#define COMPRESS_TYPE(p)	((p) == PPP_VJC_COMP ? TYPE_COMPRESSED_TCP : TYPE_UNCOMPRESSED_TCP)

/*
 * SND_COMPRESSED_TCP/RCV_COMPRESSED_TCP return true (non-zero)
 * if we should send/receive (respectively) compressed TCP packets.
 */
#define SND_COMPRESSED_TCP(p)	((p)->ppp_comp->t_max_states > 0)
#define RCV_COMPRESSED_TCP(p)	((p)->ppp_comp->r_max_states > 0)

struct ppp_ppc_type_s {
	u_short			mi_idnum;
	char			*mi_idname;
	struct ppp_ppc_s	*first_ppc;
};

#define PPCID_REQ_PEND	0x01
/* per service provider (upper stream queue) structure; */
typedef struct ppp_dlpi_sp_s {
	lock_t	*up_lck;		/* lock protecting this structure */
	queue_t	*ppp_rdq;		/* read queue of upper stream */
	minor_t	up_minor;
	uint	ppp_flags;
	ulong	ppp_dl_state;
	ulong	ppp_sap;
	mblk_t	*ud_indp;
	struct ppp_dlpi_sp_s	*up_next;
	struct ppp_dlpi_sp_s	*up_prev;
	struct	ppp_ppc_s *ppp_ppc;	/* physical unit */
	struct	in_ifaddr ia;
	struct	ifstats ppp_stats;	/* netstat stuff */
	char	ppp_ifname[IFNAMSIZ];
} ppp_dlpi_sp_t;

/* (upper stream queue) structure; */
typedef struct ppp_ctrl_s {
	lock_t	*pc_lck;	/* lock protecting this structure */
	queue_t	*pc_rdq;	/* read queue of upper stream */
	mblk_t	*pc_reqbp;	/* PPCID_REQ message */
} ppp_ctrl_t;

/*
 * Head of allocated upper (service providers) and
 * lower (physical units) * private data structures.
 */
typedef struct ppp_prv_head_s {
	lock_t		*ph_lck;	/* lock protecting this structure */
	atomic_int_t	ph_refcnt;	/* list(s) are in use while > 0 */
	boolean_t	ph_inited;	/* driver has been initialized */
	ppp_dlpi_sp_t	*ph_uhead;	/* head of service providers list */
	ppp_ppc_t	*ph_lhead;	/* head of physical units list */
	mblk_t		*ph_hdlcbp;	/* HDLC message header template */
} ppp_prv_head_t;

typedef struct ppp_timers_s {
	lock_t	*pt_lck;
	toid_t	pt_sndrcv_toid;
	toid_t	pt_cnfack_toid;
	toid_t	pt_authack_toid;
	toid_t	pt_authreq_toid;
	long	pt_sectoticks;
} ppp_timers_t;

typedef struct ppp_log_s {
	lock_t	*log_lck;
	queue_t	*log_rdq;
	char	log_buf[LOGSIZE];
} ppp_log_t;
/*
 * Macros for PPP logging
 */
#define PPC_INDX(a)	((a)->l_index)
#define PPC_DB(a)	((a)->lp_lrp->local.debug)
#define PPPLOG(a, b)	(((a) & (b)) == (a))
#define UC(b)		(((int)b) & 0xff)
/*
 * Logging control flags.
 */
#define PPPL_OPTS	0x1	/* Log option negotiation messages */
#define PPPL_STATE	0x2	/* Log state transition messages */
#define PPPL_PPP	0x4	/* Log PPP packet traces */
#define PPPL_ASYH	0x8	/* Log ASYH packet traces */
#define PPPL_ALL	( PPPL_OPTS | PPPL_STATE | PPPL_PPP | PPPL_ASYH )
/*
 * Link states.
 */
#define PPP_STATES      10      /* No. of states in PPP */
#define INITIAL         0       /* Lower layer is unabaiable */
#define STARTING        1       /* Open counterpart of INITIAL */
#define CLOSED          2       /* No connection, lower layer is available */
#define STOPPED         3       /* Open counterpart of CLOSED */
#define CLOSING         4       /* We've sent a Terminate Request */
#define STOPPING        5       /* Open counterpart to CLOSING */
#define REQSENT         6       /* We've sent a Config Request */
#define ACKRCVD         7       /* We've received a Config Ack */
#define ACKSENT         8       /* We've sent a Config Ack */
#define OPENED          9       /* Connection open */


/*
 * Link events.
 */
#define PPP_EVENTS      16      /* No. of events in PPP */
#define UP              0       /* Lower layer is ready */
#define DOWN            1       /* Lower layer is no longer ready */
#define OPEN            2       /* Receive a link open indication */
#define CLOSE           3       /* Receive a link close indication */
#define TO_P            4       /* Expiration of Restart timer with Restart>0 */
#define TO_M            5       /* Expiration of Restart timer with Restart==0*/
#define RCR_P           6       /* Received a good Config Request */
#define RCR_M           7       /* Received a bad Config Request */
#define RCA             8       /* Received a Config Acknowledge */
#define RCN             9       /* Received a Config No Acknowledge */
#define RTR             10      /* Received a Terminate Request */
#define RTA             11      /* Received a Terminate Acknowledge */
#define RUC             12      /* Received a Unknown Code */
#define RXJ_P           13      /* Received a Code Reject with Acceptable Reject
ion */
#define RXJ_M           14      /* Received a Code Reject with Catastrophic Reje
ction*/
#define RXR             15      /* Received an Echo Request(Reply), Discard Req
*/


/* This is for backwards compatiblity */
#define ACTV_OPEN       0       /* active open */
#define PSSV_OPEN       1       /* passive open */


/*
 * Link packet codes.
 */
#define CNF_REQ		1	/* Configure request */
#define CNF_ACK		2	/* Configure Acknowledge */
#define CNF_NAK		3	/* Configure No Acknowledge */
#define CNF_REJ		4	/* Configure reject */
#define TRM_REQ		5	/* Terminate request */
#define TRM_ACK		6	/* Terminate acknowledge */
#define COD_REJ		7	/* Code Reject -- Invalid code */
#define PROT_REJ	8	/* Protocol Reject */
#define ECHO_REQ	9	/* Echo request */
#define ECHO_RPL	10	/* Echo reply */
#define DSCD_REQ	11	/* Discard Request */

/*
 * PAP packet codes.
 */
#define PAP_REQ		1	/* Authentication request */
#define PAP_ACK		2	/* Authentication Acknowledge */
#define PAP_NAK		3	/* Authentication No Acknowledge */

typedef struct ppp_snd_act_tbl_s {
	void	(*func)(ppp_ppc_t *, mblk_t *, unchar);
} ppp_snd_act_tbl_t;

typedef struct ppp_gen_act_tbl_s {
	void	(*func)(ppp_ppc_t *, unchar);
} ppp_gen_act_tbl_t;

typedef struct ppp_spe_act_tbl_s {
	void	(*func)(ppp_ppc_t *, unchar);
} ppp_spe_act_tbl_t;

typedef struct ppp_state_tbl_s {
	ulong snd_act;
	ulong gen_act;
	ulong spe_act;
	ushort next_state;
} ppp_state_tbl_t;

typedef struct ppp_proto_tbl_s {
	ushort ppp_proto;		/*protocol type placed in hdlc header*/
	unchar ppp_n_cnf_opt;		/* no. of configuration options */
	cnf_opt_tbl_t *ppp_cnf_opt_tbl;	/* table of configuration options */
} ppp_proto_tbl_t;

#define PUT_SRV_TRC	9
#define MDT_MPR_TRC	8
#define IOCTL_TRC	7
#define STATE_TRC	6
#define MSS_DATA_TRC	5
#define EXCPT_TRC	4
#define ERROR_TRC	1

/*
 * Lock hierarchy definitions.
 */
#define PPP_BASE_LCK_HIER	(1)
#define PPP_PH_LCK_HIER		(PPP_BASE_LCK_HIER)
#define PPP_UP_LCK_HIER		(PPP_BASE_LCK_HIER + 2)
#define PPP_LL_LCK_HIER		(PPP_UP_LCK_HIER + 1)
#define PPP_LP_LCK_HIER		(PPP_UP_LCK_HIER + 2)
#define PPP_LR_LCK_HIER		(PPP_LP_LCK_HIER + 2)
#define PPP_PT_LCK_HIER		(32)
#define PPP_PC_LCK_HIER		(32)
#define PPP_LOG_LCK_HIER	(32)

#define IN_LNG(a)	SOCK_INADDR(&(a))->s_addr

/*
 * Function prototypes.
 */
extern void	pppadd_sp_conn(ppp_dlpi_sp_t *, ppp_ppc_t *);
extern ushort	pppcnf_chk_req(unchar, ppp_ppc_t *, mblk_t *);
extern mblk_t	*pppcnf_fmt_req(ppp_ppc_t *, ppp_proto_tbl_t *, mblk_t *);
extern int	pppctrl_close(queue_t *, dev_t *, int, int, cred_t *);
extern void	pppctrl_notify(ppp_ppc_t *);
extern int	pppctrl_open(queue_t *, dev_t *, int, int, cred_t *);
extern int	pppctrl_ursrv(queue_t *);
extern int	pppctrl_uwput(queue_t *, mblk_t *);
extern int	pppctrl_uwsrv(queue_t *);
extern int	pppioctl(queue_t *, mblk_t *);
extern void	ppppap_open(ppp_ppc_t *, int, char *, pl_t);
extern void	ppppap_fail(ppp_ppc_t *, int, pl_t);
extern void	ppppap_recv(ppp_ppc_t *, mblk_t *);
extern void	ppppap_snd_rsp(ppp_ppc_t *, char *, int, unchar);
extern void	ppppap_start(ppp_ppc_t *);
extern ppp_ppc_t	*ppprm_sp_conn(ppp_dlpi_sp_t *, ppp_ppc_t *);
extern void	pppsnd_prot_rej(ppp_ppc_t *, mblk_t *, unchar);
extern void	pppstate(unchar, int, ppp_ppc_t *, mblk_t *);
extern void	ppp_ioc_ack(queue_t *, mblk_t *);
extern void	ppp_ioc_error(queue_t *, mblk_t *, int);
extern void	ppp_layer_up(ppp_ppc_t *, unchar);

extern		ppplog_open(queue_t *, dev_t *, int, int, cred_t *);
extern		ppplog_close(queue_t *, dev_t *, int, int, cred_t *);
extern int	ppplog_ursrv(queue_t *);

extern char	*ppp_hexdata(char *,  mblk_t *);

#ifdef __STDC__
#include <stdarg.h>
extern void	ppplog(int, char *, ...);
#else
#include <varargs.h>
extern void	ppplog();
#endif	/* __STDC__ */

extern struct incompress	*incompalloc(void);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_PPP_PPP_KERN_H */
