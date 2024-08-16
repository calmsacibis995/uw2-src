/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_TCP_TCP_KERN_H	/* wrapper symbol for kernel use */
#define _NET_INET_TCP_TCP_KERN_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/tcp/tcp_kern.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/tcp/tcp.h>		/* REQUIRED */
#include <net/inet/tcp/tcp_timer.h>	/* REQUIRED */
#include <net/inet/tcp/tcp_timer.h>	/* REQUIRED */
#include <net/socket.h>			/* REQUIRED */
#include <net/tihdr.h>			/* REQUIRED */
#include <util/ksynch.h>		/* REQUIRED */
#include <util/types.h>			/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <netinet/tcp.h>		/* REQUIRED */
#include <netinet/tcp_timer.h>		/* REQUIRED */
#include <netinet/tcp_timer.h>		/* REQUIRED */
#include <sys/ksynch.h>			/* REQUIRED */
#include <sys/socket.h>			/* REQUIRED */
#include <sys/stream.h>			/* REQUIRED */
#include <sys/tihdr.h>			/* REQUIRED */
#include <sys/types.h>			/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Kernel variables for TCP.
 */

/*
 * TCP control block, one per tcp; fields:
 */
struct tcpcb {
	struct tcpiphdr *seg_next;	/* sequencing queue */
	short t_state;			/* state of this connection */
	short t_timer[TCPT_NTIMERS];	/* tcp timers */
	short t_rxtshift;		/* log(2) of rexmt exp. backoff */
	short t_rxtcur;			/* current retransmit value */
	short t_dupacks;		/* consecutive dup acks recd */
	unsigned short t_maxseg;	/* maximum segment size */
	char t_force;			/* 1 if forcing out a byte */
	unsigned short t_flags;
#define TF_ACKNOW	0x0001		/* ack peer immediately */
#define TF_DELACK	0x0002		/* ack, but try to delay it */
#define TF_NODELAY	0x0004		/* don't delay packets to coalesce */
#define TF_NOOPT	0x0008		/* don't use tcp options */
#define TF_SENTFIN	0x0010		/* have sent FIN */
#define TF_IOLOCK	0x0020
#define TF_NEEDIN	0x0040
#define TF_NEEDOUT	0x0080
#define TF_NEEDTIMER	0x0100
#define TF_INCLOSE	0x0200		/* Upper half going away */
#define TF_FLOWCTL	0x0400		/* flow control is in effect */
#define TF_INFLUSHQ	0x0800		/* an M_FLUSH is in progress */
	struct	tcpiphdr *t_template;	/* skeletal packet for transmit */
	mblk_t	*t_tmplhdr;		/* template back-pointer for dealloc */
	struct	inpcb *t_inpcb;		/* back pointer to internet pcb */
/*
 * The following fields are used as in the protocol specification.
 * See RFC783, Dec. 1981, page 21.
 */
/* send sequence variables */
	tcp_seq	snd_una;		/* send unacknowledged */
	tcp_seq	snd_nxt;		/* send next */
	tcp_seq	snd_up;			/* send urgent pointer */
	tcp_seq	snd_wl1;		/* window update seg seq number */
	tcp_seq	snd_wl2;		/* window update seg ack number */
	tcp_seq	iss;			/* initial send sequence number */
	unsigned long snd_wnd;		/* send window */
/* receive sequence variables */
	unsigned long rcv_wnd;		/* receive window */
	tcp_seq	rcv_nxt;		/* receive next */
	tcp_seq	rcv_up;			/* receive urgent pointer */
	tcp_seq	irs;			/* initial receive sequence number */
/*
 * Additional variables for this implementation.
 */
/* receive variables */
	tcp_seq	rcv_adv;		/* advertised window */
/* retransmit variables */
	tcp_seq	snd_max;		/* highest sequence number sent
					 * used to recognize retransmits
					 */
	unsigned long	t_maxwin;	/* max window size to use */
/* congestion control (for slow start, source quench, retransmit after loss) */
	unsigned long	snd_cwnd;	/* congestion-controlled window */
	unsigned long	snd_ssthresh;	/* snd_cwnd size threshhold for
					 * for slow start exponential to
					 * linear switch
					 */
/*
 * transmit timing stuff.
 * srtt and rttvar are stored as fixed point; for convenience in smoothing,
 * srtt has 3 bits to the right of the binary point, rttvar has 2.
 * "Variance" is actually smoothed difference.
 */
	short	t_idle;			/* inactivity time */
	short	t_rtt;			/* round trip time */
	tcp_seq	t_rtseq;		/* sequence number being timed */
	short	t_srtt;			/* smoothed round-trip time */
	short	t_rttvar;		/* variance in round-trip time */
	short	t_rttmin;		/* minimum rtt allowed */
	unsigned short max_rcvd;	/* most peer has sent into window */
	unsigned short	max_sndwnd;	/* largest window peer has offered */
/* out-of-band data */
	char	t_oobflags;		/* have some */
	char	t_iobc;			/* input character */
#define TCPOOB_HAVEDATA	0x01
#define TCPOOB_HADDATA	0x02
	struct	tcpcb *t_head;		/* back pointer to accept tcpcb */
	struct	tcpcb *t_q0;		/* queue of partial connections */
	struct	tcpcb *t_q;		/* queue of incoming connections */
	short	t_q0len;		/* partials on t_q0 */
	short	t_qlen;			/* number of connections on t_q */
	unsigned long	t_qlimit;	/* max number queued connections */
	/*
	 * We maintain message blocks on a "private" queue.  The only time
	 * a message is put on our "real" queue is to flow control the
	 * upper stream.  We maintain t_outqsize (the number of bytes on the
	 * "private" queue and t_outqfirst/t_outqlast which maintains
	 * the queue of message blocks.  We only queue M_DATA and M_PROTO
	 * messages (and do not handle priority bands.
	 */
	ulong_t	t_outqsize;		/* amount of data on output queue */
	mblk_t	*t_outqfirst;		/* beginning of queued data */
	mblk_t	*t_outqlast;		/* end of queued data */
	ulong	t_outqhiwat;		/* hi-water mark */
	ulong	t_outqlowat;		/* lo-water mark */
	/*
	 * here we save mblks that arrive before the connection is accepted
	 * by the user and those received when the user's queue is full.
	 * we also save mblks that still need to be sent when the
	 * connection is closed by the user.
	 */
	mblk_t	*t_qfirst;		/* beginning of queued data */
	mblk_t	*t_qlast;		/* end of queued data */
	mblk_t	*t_inqfirst;		/* beginning of pending input */
	mblk_t	*t_inqlast;		/* end of pending input */
	mblk_t	*t_ordrel;		/* template for ordrel */
	int	t_iqsize;		/* amount of data on input queue */
	int	t_iqurp;		/* offset of urgent byte on input q */
	short	t_linger;		/* linger flag (compatibility only) */
	unsigned char	t_onepacket;	/* onepacket mode in effect */
	unsigned short	t_spsize;	/* short packet size */
	int	t_spthresh;		/* short packet threshold */
	int	t_spcount;		/* count of consec. short packets */
};

#ifdef TLI_PRIMS
char *tli_primitives[] =
{
	"CONNECT",	"ACCEPT",	"DISCONNECT",	"DATA",
	"EX_DATA",	"INFORMATION",	"BIND",		"UNBIND",
	"UNITDATA",	"OPTIONS",	"ORDERLY RELEASE",
};
#endif /* TLI_PRIMS */
#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#define INPTOTCP(ip)	((struct tcpcb *)(ip)->inp_ppcb)
#define TCPTOINP(tp)	((struct inpcb *)(((struct tcpcb *)tp)->t_inpcb))
#define TCPTOQ(tp)	((tp)->t_inpcb->inp_q)
#define QTOTCP(q)	((struct tcpcb *)((struct inpcb *)(q)->q_ptr)->inp_ppcb)

/*
 * The smoothed round-trip time and estimated variance
 * are stored as fixed point numbers scaled by the values below.
 * For convenience, these scales are also used in smoothing the average
 * (smoothed = (1/scale)sample + ((scale-1)/scale)smoothed).
 * With these scales, srtt has 3 bits to the right of the binary point,
 * and thus an "ALPHA" of 0.875.  rttvar has 2 bits to the right of the
 * binary point, and is smoothed with an ALPHA of 0.75.
 */
#define TCP_RTT_SCALE		8	/* multiplier for srtt; 3 bits frac. */
#define TCP_RTT_SHIFT		3	/* shift for srtt; 3 bits frac. */
#define TCP_RTTVAR_SCALE	4	/* multiplier for rttvar; 2 bits */
#define TCP_RTTVAR_SHIFT	2	/* multiplier for rttvar; 2 bits */

/*
 * The initial retransmission should happen at rtt + 4 * rttvar.
 * Because of the way we do the smoothing, srtt and rttvar will each
 * average +1/2 tick of bias.	 When we compute the retransmit timer,
 * we want 1/2 tick of rounding and 1 extra tick because of +-1/2 tick
 * uncertainty in the firing of the timer.	 The bias will give us
 * exactly the 1.5 tick we need.  But, because the bias is
 * statistical, we have to test that we don't drop below the minimum
 * feasible timer (which is 2 ticks).  This macro assumes that the
 * value of TCP_RTTVAR_SCALE is the same as the multiplier for rttvar.
 */
#define TCP_REXMTVAL(tp) \
	(((tp)->t_srtt >> TCP_RTT_SHIFT) + (tp)->t_rttvar)

/*
 * Definitions for return values less than zero for the T_info_ack fields
 * TSDU_size, ETSDU_size, CDATA_size, DDATA_size and ADDR_size.
 */
#define TP_UNLIMITED	-1	/* No maximum limit imposed on value */
#define TP_NOTSUPPORTED	-2	/* Capability not supported by transport */

/*
 * Definitions for return values less than zero for the T_info_ack field
 * OPT_size, which has different semantics from those immediately above.
 */
#define TP_READONLY	-2	/* Options are read-only */
#define TP_NOOPTIONS	-3	/* Transport does not support options */

#define TCP_TSDU_SIZE	0
#define TCP_ETSDU_SIZE	-1
#define TCP_TIDU_SIZE	(16 * 1024)

#define IPPROTO_TCP_MAXSZ	32

/*
 * TCP_OPT_SIZE is based on the maximum size of all level SOL_SOCKET,
 * IPPROTO_TCP, and IPPROTO_IP options (including option header
 * overhead).
 */

#define TCP_OPT_SIZE (SOL_SOCKET_MAXSZ + IPPROTO_TCP_MAXSZ + IPPROTO_IP_MAXSZ)

extern void tcp_state(queue_t *, mblk_t *);
extern int tcp_options(queue_t *, struct T_optmgmt_req *, struct opthdr *,
		       mblk_t *);
extern void tcp_ctloutput(queue_t *, mblk_t *);
extern int tcp_attach(queue_t *);
extern struct tcpcb *tcp_disconnect(struct tcpcb *);
extern struct tcpcb *tcp_usrclosed(struct tcpcb *);
extern void tpqinsque(struct tcpcb *, struct tcpcb *, int);
extern int tpqremque(struct tcpcb *, struct tcpcb *, int);
extern void inpisconnected(struct inpcb *);
extern int inpordrelind(struct inpcb *);
extern void inpisdisconnected(struct inpcb *, int);
extern void tcp_errdiscon(struct inpcb *, int);
extern void tcp_uderr(mblk_t *);
extern void tcp_ghost(struct tcpcb *);
extern void tcp_trace(short, short, struct tcpcb *, struct tcpiphdr *, int);

#if DEBUG
extern void inpdump(struct inpcb *);
extern void stcpdump(queue_t *);
extern void tcbdump(void);
extern void ninpdump(struct inpcb *, int);
#endif /* DEBUG */

extern int tcp_reass(queue_t *, struct tcpcb *, struct tcpiphdr *, mblk_t *);
extern void sendup(struct tcpcb *, mblk_t *, struct tcpiphdr *, queue_t *);
extern mblk_t *headerize(mblk_t *);
extern int tcp_passoobup(mblk_t *, queue_t *, int);
extern void tcp_linput(mblk_t *);
extern struct tcpcb *tcp_uinput(struct tcpcb *);
extern void tcp_dooptions(struct tcpcb *, mblk_t *, struct tcpiphdr *);
extern void tcp_xmit_timer(struct tcpcb *);
extern int tcp_mss(struct tcpcb *);

extern struct tcpcb *tcp_output(struct tcpcb *);
extern void tcp_setpersist(struct tcpcb *);

extern struct tcpiphdr *tcp_template(struct tcpcb *);
extern void tcp_respond(mblk_t *, struct tcpcb *, struct tcpiphdr *, tcp_seq,
			tcp_seq, int);
extern void	tcp_init_tcpcb(struct tcpcb *, struct inpcb *);
extern struct tcpcb *tcp_newtcpcb(struct inpcb *);
extern struct tcpcb *tcp_drop(struct tcpcb *, int);
extern struct tcpcb *tcp_close(struct tcpcb *, int);
extern void tcp_freespc(struct tcpcb *);
extern void tcp_ctlinput(mblk_t *);
extern void tcp_enqdata(struct tcpcb *, mblk_t *, int);
extern void tcp_calldeq(queue_t *);
extern int tcp_deqdata(queue_t *);
extern void tcp_io(struct tcpcb *, int, mblk_t *);
extern void tcp_abortincon(struct inpcb *);
extern void tcp_discon(struct inpcb *, int, int, short);
extern int tcpopen(queue_t *, dev_t *, int, int, cred_t *);
extern void tcp_qdrop(struct tcpcb *, int);

extern struct inpcb tcb;		/* head of queue of active tcpcb's */
extern struct tcpstat tcpstat;		/* tcp statistics */

#define tcptoinp(tp) ((struct inpcb *)(((struct tcpcb *)tp)->t_inpcb))

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_TCP_TCP_KERN_H */
