/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_NET_KTLI_T_KUSER_H	/* wrapper symbol for kernel use */
#define	_NET_KTLI_T_KUSER_H	/* subject to change without notice */

#ident	"@(#)kern:net/ktli/t_kuser.h	1.15"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */


/*
 *	t_kuser.h, definitions for kernel TLI
 *
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/tiuser.h>			/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/stream.h>			/* REQUIRED */
#include <sys/tiuser.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * these make life a lot easier
 */
#define		TCONNREQSZ	sizeof(struct T_conn_req)
#define		TCONNRESSZ	sizeof(struct T_conn_res)
#define		TDISCONREQSZ	sizeof(struct T_discon_req)
#define		TDATAREQSZ	sizeof(struct T_data_req)
#define		TEXDATAREQSZ	sizeof(struct T_exdata_req)
#define		TINFOREQSZ	sizeof(struct T_info_req)
#define		TBINDREQSZ	sizeof(struct T_bind_req)
#define		TUNBINDREQSZ	sizeof(struct T_unbind_req)
#define		TUNITDATAREQSZ	sizeof(struct T_unitdata_req)
#define		TOPTMGMTREQSZ	sizeof(struct T_optmgmt_req)
#define		TORDRELREQSZ	sizeof(struct T_ordrel_req)
#define		TCONNINDSZ	sizeof(struct T_conn_ind)
#define		TCONNCONSZ	sizeof(struct T_conn_con)
#define		TDISCONINDSZ	sizeof(struct T_discon_ind)
#define		TDATAINDSZ	sizeof(struct T_data_ind)
#define		TEXDATAINDSZ	sizeof(struct T_exdata_ind)
#define		TINFOACKSZ	sizeof(struct T_info_ack)
#define		TBINDACKSZ	sizeof(struct T_bind_ack)
#define		TERRORACKSZ	sizeof(struct T_error_ack)
#define		TOKACKSZ	sizeof(struct T_ok_ack)
#define		TUNITDATAINDSZ	sizeof(struct T_unitdata_ind)
#define		TUDERRORINDSZ	sizeof(struct T_uderror_ind)
#define		TOPTMGMTACKSZ	sizeof(struct T_optmgmt_ack)
#define		TORDRELINDSZ	sizeof(struct T_ordrel_ind)
#define		TPRIMITIVES	sizeof(struct T_primitives)

#ifdef	_KERNEL

/*
 * pl value for locks acquired in ktli
 */
#define	PLKTLI		PLSTR

/*
 * priority hint for sleeping while trying to get
 * a sleep lock
 */
#define	PRITLI		PRIMED

/*
 * TIUSER structure, endpoint for communication via kernel TLI.
 * Just cantains the transport provider information and an
 * open file pointer. Note this structure will need to be
 * expanded to handle data related to connection orientated
 * transports. 
 *
 * This structure does not need any locks to protect its fields
 * as it is not shared between threads. The flags are shared between
 * base kernel in t_kspoll() and the routine ktli_poll() which
 * is dispatched by the clock interrupt. However, these two
 * can never be running at the same time. See t_kspoll() for
 * more detail.
 */
typedef struct tiuser {
	struct	file	*tp_fp;		/* open file pointer */
	struct	t_info	 tp_info;	/* transport provider info. */
	int		 tp_flags;	/* flags defined below */
} TIUSER;

/*
 * this makes life easier
 */
#define		TIUSERSZ	sizeof(TIUSER)

/*
 * a kernel network buffer
 */
struct knetbuf {
	mblk_t		*udata_mp;	/* current receive streams block */
	unsigned int	maxlen;
	unsigned int	len;
	char		*buf;
};

/*
 * a unit of data for kernel TLI
 */
struct t_kunitdata {
	struct netbuf addr;
	struct netbuf opt;
	struct knetbuf udata;
};

/*
 * flags for TIUSER
 */
#define		TIME_UP		0x01

/*
 * signal handling in t_kspoll()
 */
#define	POLL_SIG_IGNORE		0x01
#define	POLL_SIG_CATCH		0x02

#ifdef	__STDC__

extern	int	t_kalloc(TIUSER *, int, int, char **);
extern	int	t_kbind(TIUSER *, struct t_bind *, struct t_bind *);
extern	int	t_kclose(TIUSER *, int);
extern	int	t_kconnect(TIUSER *, struct t_call *, struct t_call *);
extern	int	t_kfree(TIUSER *, char *, int);
extern	int	t_kgetstate(TIUSER *, int *);
extern	int	t_kopen(struct file *, dev_t, int, TIUSER **, struct cred *);
extern	int	t_krcvudata(TIUSER *, struct t_kunitdata *, int *, int *);
extern	int	t_ksndudata(TIUSER *, struct t_kunitdata *, frtn_t *);
extern	int	t_kspoll(TIUSER *, int, int, int *);
extern	int	t_kunbind(TIUSER *);
extern	int	tli_send(TIUSER *, mblk_t *, int);
extern	int	tli_recv(TIUSER *, mblk_t **, int);
extern	int	get_ok_ack(TIUSER *, int, int);

#endif

/*
 * support for kernel TLI debugging
 */

#ifdef DEBUG

extern	int	ktlilog;
extern	int	ktli_log();

#define		KTLILOG(A, B, C) ((void)((ktlilog) && ktli_log((A), (B), (C))))

#else

#define		KTLILOG(A, B, C)

#endif /* DEBUG */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_KTLI_T_KUSER_H */
