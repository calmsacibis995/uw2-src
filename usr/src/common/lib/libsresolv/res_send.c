/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresolv:common/lib/libsresolv/res_send.c	1.1.1.7"
#ident  "$Header: $"

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
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

/*
 * Send query to name server and wait for reply.
 */

#include <sys/byteorder.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <xti.h>
#include <sys/tihdr.h>
#include <stropts.h>
#include <poll.h>
/* The following #undef's are needed because of xti.h */
#undef T_NULL
#undef T_UNSPEC
#include <arpa/nameser.h>
#include <resolv.h>
#include "res.h"
#include "libres_mt.h"

#pragma weak res_send=_rs_res_send
#pragma weak _res_close=_rs__res_close

extern struct state *get_rs__res();

static int s = -1;	/* TLI endpoint used for communications */

static int *
get_rs_s()
{
#ifdef _REENTRANT
        struct _rs_tsd *key_tbl;
	int *sp;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&s);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rs_tsd *)
		  _mt_get_thr_specific_storage(_rs_key, _RS_KEYTBL_SIZE);
	if (key_tbl == NULL) return ((int *)NULL);
	if (key_tbl->s_p == NULL) 
		if ((sp = (int *)(key_tbl->s_p = calloc(1, sizeof(int))))
		    != NULL)
			*sp = -1;
	return ((int *)key_tbl->s_p);
#else /* !_REENTRANT */
	return (&s);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rs_s(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */
  
#define KEEPOPEN (RES_USEVC|RES_STAYOPEN)

_rs_res_send(buf, buflen, answer, anslen)
	char *buf;
	int buflen;
	char *answer;
	int anslen;
{
	register int n;
	int try, v_circuit, resplen, ns, tflags;
	int gotsomewhere = 0;
	int connected = 0;
	int connreset = 0;
	u_short id, len;
	char *cp;
	struct pollfd dspoll;
	int timeout;
	HEADER *hp = (HEADER *) buf;
	HEADER *anhp = (HEADER *) answer;
	struct iovec iov[2];
	int terrno = ETIMEDOUT;
	char junk[512];
	struct t_call sc;
	struct t_unitdata ud;
	struct t_info ti;
	struct t_uderr te;
	struct state *rp;
	int *sp;

	/* Get thread-specific data */
	if ((rp = get_rs__res()) == NULL)
		return (-1);
	if ((sp = get_rs_s()) == NULL)
		return (-1);

#ifdef DEBUG
	if (rp->options & RES_DEBUG) {
		printf("_rs_res_send()\n");
		_rs_p_query(buf);
	}
#endif DEBUG
	bzero((char *) &sc, sizeof(sc));
	bzero((char *) &ud, sizeof(ud));
	bzero((char *) &ti, sizeof(ti));
	bzero((char *) &te, sizeof(te));

	if (!(rp->options & RES_INIT))
		if (_rs_res_init() == -1) {
			return(-1);
		}
	v_circuit = (rp->options & RES_USEVC) || buflen > PACKETSZ;
	id = hp->id;
	/*
	 * Send request, RETRY times, or until successful
	 */
	for (try = rp->retry; try > 0; try--) {
	   for (ns = 0; ns < rp->nscount; ns++) {
#ifdef DEBUG
		if (rp->options & RES_DEBUG)
			printf("Querying server (# %d) address = %s\n", ns+1,
			      inet_ntoa(rp->nsaddr_list[ns].sin_addr));
#endif DEBUG
		sc.addr.buf = (char *) &(rp->nsaddr_list[ns]);
		sc.addr.maxlen = sizeof(struct sockaddr);
		sc.addr.len = sizeof(struct sockaddr);
	usevc:
		if (v_circuit) {
			int truncated = 0;

			/*
			 * Use virtual circuit;
			 * at most one attempt per server.
			 */
			try = rp->retry;
			if (*sp < 0) {
				*sp = t_open(_RS_TCP_DEV, O_RDWR, &ti);
				if (*sp < 0) {
					terrno = errno;
#ifdef DEBUG
					if (rp->options & RES_DEBUG)
					    t_error("t_open(vc) failed");
#endif DEBUG
					continue;
				}

				if ((t_bind(*sp, NULL, NULL) < 0) ||
				    (t_connect(*sp, &sc, NULL) < 0)) {
					terrno = errno;
#ifdef DEBUG
					if (rp->options & RES_DEBUG)
					    t_error("t_bind/t_connect(vc) failed");
#endif DEBUG
					(void) t_close(*sp);
					*sp = -1;
					continue;
				}
			}
			/*
			 * Send length & message
			 */
			len = htons((u_short)buflen);
			if ((t_snd(*sp, (char *)&len, sizeof(len), 0)
			    != sizeof(len))
			 || (t_snd(*sp, buf, buflen, 0) != buflen)) {
				terrno = errno;
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					t_error("t_snd failed");
#endif DEBUG
				(void) t_close(*sp);
				*sp = -1;
				continue;
			}
			/*
			 * Receive length & response
			 */
			cp = answer;
			len = sizeof(short);
			while (len != 0 &&
			    (n = t_rcv(*sp, (char *)cp, (int)len, &tflags)) 
			    > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
				terrno = errno;
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					t_error("t_rcv(vc) failed");
#endif DEBUG
				(void) t_close(*sp);
				*sp = -1;
				/*
				 * A long running process might get its TCP
				 * connection reset if the remote server was
				 * restarted.  Requery the server instead of
				 * trying a new one.  When there is only one
				 * server, this means that a query might work
				 * instead of failing.  We only allow one reset
				 * per query to prevent looping.
				 */
				if (terrno == ECONNRESET && !connreset) {
					connreset = 1;
					ns--;
				}
				continue;
			}
			cp = answer;
			if ((resplen = ntohs(*(u_short *)cp)) > anslen) {
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					fprintf(stderr, "response truncated\n");
#endif DEBUG
				len = anslen;
				truncated = 1;
			} else
				len = resplen;
			while (len != 0
			    && (n = t_rcv(*sp, (char *)cp, (int)len, &tflags))
			       > 0) {
				cp += n;
				len -= n;
			}
			if (n <= 0) {
				terrno = errno;
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					t_error("t_rcv failed");
#endif DEBUG
				(void) t_close(*sp);
				*sp = -1;
				continue;
			}
			if (truncated) {
				/*
				 * Flush rest of answer
				 * so connection stays in synch.
				 */
				anhp->tc = 1;
				len = resplen - anslen;
				while (len != 0) {
					n = (len > sizeof(junk) ?
					    sizeof(junk) : len);
					if ((n = t_rcv(*sp, junk, n, &tflags))
					    > 0)
						len -= n;
					else
						break;
				}
			}
		}
		else {
			/* Use datagram provider (UDP) */
			if (*sp < 0) {
				*sp = t_open(_RS_UDP_DEV, O_RDWR, &ti);
				if (*sp < 0) {
					terrno = errno;
#ifdef DEBUG
					if (rp->options & RES_DEBUG)
						t_error("t_open(udp) failed");
#endif DEBUG
					continue;
				}
				if (t_bind(*sp, NULL, NULL) < 0) {
					terrno = errno;
#ifdef DEBUG
					if (rp->options & RES_DEBUG)
						t_error("t_bind failed");
#endif DEBUG
					(void) t_close(*sp);
					*sp = -1;
					continue;
				}
			}
			if (connected && rp->nscount > 1) {
				/*
				 * Disconnect any existing "connection"
				 * before attempting to "connect"
				 * to another server.
				 */
				if (T_disconnect(*sp) < 0) {
#ifdef DEBUG
					if (rp->options & RES_DEBUG)
						t_error("T_disconnect");
#endif DEBUG
					continue;
				}
				connected = 0;
			}
			if (!connected) {
				if (T_connect(*sp, &rp->nsaddr_list[ns],
					      sizeof(struct sockaddr)) < 0) {
#ifdef DEBUG
					if (rp->options & RES_DEBUG)
						t_error("T_connect");
#endif DEBUG
					continue;
				}
				connected = 1;
			}
			ud.addr.buf = NULL;
			ud.addr.len = 0;
			ud.udata.buf = buf;
			ud.udata.len = buflen;

			if (t_sndudata(*sp, &ud) < 0) {
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					t_error("t_sndudata");
#endif DEBUG
				continue;
			}

			/*
			 * Wait for reply
			 */
			timeout = (rp->retrans << (rp->retry - try));
			if (try > 0)
				timeout /= rp->nscount;
			if (timeout <= 0)
				timeout = 1;
			timeout *= 1000;
wait:
			dspoll.fd = *sp;
			dspoll.events = POLLIN | POLLPRI;
			n = poll(&dspoll, (size_t) 1, timeout);
			if (n < 0) {
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					perror("poll");
#endif DEBUG
				continue;
			}
			if (n == 0) {
				/*
				 * timeout
				 */
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					printf("timeout %d %d %d %d\n", timeout,
					       rp->retrans, try,
					       rp->nscount);
#endif DEBUG
				gotsomewhere = 1;
				continue;
			}
			resplen = -1;
			ud.addr.maxlen = sizeof(struct sockaddr_in);
			ud.addr.buf = junk;
			ud.udata.maxlen = anslen;
			ud.udata.buf = answer;
			if (t_rcvudata(*sp, &ud, &tflags) == 0)
				resplen = ud.udata.len;

			if (resplen <= 0) {
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					t_error("t_rcvudata");
#endif DEBUG
				if (errno == ECONNREFUSED) {
					if (rp->nscount > 1 && connected) {
						T_disconnect(*sp);
						connected = 0;
					}
					errno = 0;
				}
				/* clear any error indication at endpoint */
				switch (get_t_errno()) {
				case TLOOK:	
					set_t_errno(0);
					te.addr.maxlen 
					   = sizeof(struct sockaddr_in);
					te.addr.buf = junk;
					if (t_rcvuderr(*sp, &te) == 0)
						terrno = te.error;
					break;
				default:		/* try, try, again */
					terrno = errno;
					set_t_errno(0);
					break;
				}
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					printf("t_rcvudata:errno %d\n",terrno);
#endif DEBUG
				continue;
			}
			gotsomewhere = 1;
			if (id != anhp->id) {
				/*
				 * response from old query, ignore it
				 */
#ifdef DEBUG
				if (rp->options & RES_DEBUG) {
					printf("old answer:\n");
					_rs_p_query(answer);
				}
#endif DEBUG
				goto wait;
			}
			if (!(rp->options & RES_IGNTC) && anhp->tc) {
				/*
				 * get rest of answer;
				 * use TCP with same server.
				 */
#ifdef DEBUG
				if (rp->options & RES_DEBUG)
					printf("truncated answer\n");
#endif DEBUG
				(void) t_close(*sp);
				*sp = -1;
				v_circuit = 1;
				goto usevc;
			}
		}
#ifdef DEBUG
		if (rp->options & RES_DEBUG) {
			printf("got answer:\n");
			_rs_p_query(answer);
		}
#endif DEBUG
		/*
		 * If using virtual circuits, we assume that the first server
		 * is preferred * over the rest (i.e. it is on the local
		 * machine) and only keep that one open.
		 * If we have temporarily opened a virtual circuit,
		 * or if we haven't been asked to keep a socket open,
		 * close the socket.
		 */
		if ((v_circuit &&
		     ((rp->options & RES_USEVC) == 0 || ns != 0))
		 || (rp->options & RES_STAYOPEN) == 0) {
			(void) t_close(*sp);
			*sp = -1;
		}
		return (resplen);
	   }
	}
	if (*sp >= 0) {
		(void) t_close(*sp);
		*sp = -1;
	}
	if (v_circuit == 0)
		if (gotsomewhere == 0)
			errno = ECONNREFUSED;	/* no nameservers found */
		else
			errno = ETIMEDOUT;	/* no answer obtained */
	else
		errno = terrno;
	return (-1);
}

/*
 * This routine is for closing the TLI ep if a virtual circuit is used and
 * the program wants to close it.  This provides support for _rs_endhostent()
 * which expects to close the TLI ep.
 * This routine is for closing the socket if a virtual circuit is used and
 * the program wants to close it.  This provides support for _rs_endhostent()
 * which expects to close the socket.
 *
 * This routine is not expected to be user visible.
 */
_rs__res_close()
{
	int *sp;

	/* Get thread-specific data */
	if ((sp = get_rs_s()) == NULL)
		return (-1);
	if (*sp != -1) {
		(void) t_close(*sp);
		*sp = -1;
	}
}

/*
 * This is a hack to _rs_connect the resolver, even though TLI won't let us.
 * UDP is not so fussy.
 */
static int
T_connect(s, addr, len)
	int             s;
	struct sockaddr *addr;
	int             len;
{
	struct T_conn_req *creq;
	int   blen = sizeof(union T_primitives) + sizeof(struct sockaddr);
	union T_primitives *tprim;
	char           *p;
	int             r;
	int             flags;
	struct strbuf   ctl;

	p = (char *) malloc(blen);
	if (!p) {
		set_t_errno(TSYSERR);
		errno = ENOMEM;
		return -1;
	}
	creq = (struct T_conn_req *) p;
	creq->PRIM_type = T_CONN_REQ;
	creq->DEST_length = len;
	creq->DEST_offset = sizeof(struct T_conn_req);
	memcpy(p + sizeof(struct T_conn_req), addr, len);

	ctl.maxlen = blen;
	ctl.len = sizeof(struct T_conn_req) + sizeof(struct sockaddr);
	ctl.buf = p;

	flags = 0;
	r = putmsg(s, &ctl, (struct strbuf *) 0, flags);
	if (r < 0) {
		set_t_errno(TSYSERR);
		r = -1;
		goto out;
	}
	/* get the ok_ack */
	ctl.len = 0;
	flags = RS_HIPRI;
	r = getmsg(s, &ctl, (struct strbuf *) 0, &flags);
	if (r < 0) {
		set_t_errno(TSYSERR);
		r = -1;
		goto out;
	}
	if (ctl.len < sizeof(long)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		r = -1;
		goto out;
	}
	tprim = (union T_primitives *) p;
	switch (tprim->type) {
	case T_OK_ACK:
		set_t_errno(0);
		errno = 0;
		r = 0;
		break;
	case T_ERROR_ACK:
		set_t_errno(tprim->error_ack.TLI_error);
		errno = tprim->error_ack.UNIX_error;
		r = -1;
		goto out;
	default:
		set_t_errno(TSYSERR);
		errno = EPROTO;
		r = -1;
		goto out;
	}
	ctl.len = 0;
	flags = 0;
	/* get the conn_con */
	r = getmsg(s, &ctl, (struct strbuf *) 0, &flags);
	if (r < 0) {
	}
	if (ctl.len < sizeof(long)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		r = -1;
		goto out;
	}
	tprim = (union T_primitives *) p;
	switch (tprim->type) {
	case T_CONN_CON:
		set_t_errno(0);
		errno = 0;
		r = 0;
		break;
	case T_ERROR_ACK:
		set_t_errno(tprim->error_ack.TLI_error);
		errno = tprim->error_ack.UNIX_error;
		r = -1;
		goto out;
	default:
		set_t_errno(TSYSERR);
		errno = EPROTO;
		r = -1;
		goto out;
	}
out:
	free(p);
	return r;
}

/*
 * A new connection must be established for each nameserver.
 * This could be done by using a separate file descriptor for each host,
 * but it's easier to disconnect (in the UDP sense) from one nameserver
 * before connecting to the next.
 */

static int
T_disconnect(s)
int s;
{
	int			r;
	struct strbuf		ctl;
	union T_primitives	tprim;
	int			flags;

	/* Blow away anything that's in our way. */
	if (ioctl(s, I_FLUSH, FLUSHRW) < 0) {
		set_t_errno(TSYSERR);
		return(-1);
	}

	/* XXX Hold SIGPOLL, as in t_snddis.c? */

	/* Use tprim struct T_discon_req */
	tprim.discon_req.PRIM_type = T_DISCON_REQ;
	tprim.discon_req.SEQ_number = -1;

	ctl.maxlen = sizeof(struct T_discon_req);
	ctl.len = sizeof(struct T_discon_req);
	ctl.buf = (caddr_t)&tprim.discon_req;

	r = putmsg(s, &ctl, (struct strbuf *)NULL, 0);
	if (r < 0) {
		set_t_errno(TSYSERR);
		r = -1;
		goto out2;
	}

	/* Get the T_OK_ACK */
	ctl.len = 0;
	flags = RS_HIPRI;
	r = getmsg(s, &ctl, (struct strbuf *) 0, &flags);
	if (r < 0) {
		set_t_errno(TSYSERR);
		r = -1;
		goto out2;
	}

	if (ctl.len < sizeof(long)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		r = -1;
		goto out2;
	}

	/* Use tprim long */
	switch (tprim.type) {
	case T_OK_ACK:
		set_t_errno(0);
		errno = 0;
		r = 0;
		break;
	case T_ERROR_ACK:
		/* Use tprim struct T_error_ack */
		set_t_errno(tprim.error_ack.TLI_error);
		errno = tprim.error_ack.UNIX_error;
		r = -1;
		goto out2;
	default:
		set_t_errno(TSYSERR);
		errno = EPROTO;
		r = -1;
		goto out2;
	}
out2:
	return r;
}
