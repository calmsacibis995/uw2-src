/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/res_send.c	1.2"
#ident	"$Header: $"

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
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
** Modified for and distributed with 'dig' version 2.0 from
** University of Southern California Information Sciences Institute
** (USC-ISI). 9/1/90
*/


#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_send.c	6.19 (Berkeley) 3/7/88";
#endif /* LIBC_SCCS and not lint */

/*
 * Send query to name server and wait for reply.
 */

#include "hfiles.h"
#include PARAMH
#include <sys/param.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include NAMESERH
#ifndef T_TXT
#define T_TXT 16
#endif /* T_TXT */
#include RESOLVH
#include "qtime.h"
#include "pflag.h"
extern int errno;
extern int eecode;

static int s = -1;	/* socket used for communications */
static struct sockaddr no_addr;
static int old_conn;
  
#define RES_DEBUG2      0x80000000

#ifndef FD_SET
#define	NFDBITS		32
#define	FD_SETSIZE	32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

#define KEEPOPEN (RES_USEVC|RES_STAYOPEN)

res_send(buf, buflen, answer, anslen)
	char *buf;
	int buflen;
	char *answer;
	int anslen;
{
	register int n;
	int retry, v_circuit, resplen, ns;
	int gotsomewhere = 0, connected = 0;
	u_short id, len;
	int conn;
	char *cp;
	fd_set dsmask;
	struct timeval timeout, ftime, htime;
	HEADER *hp = (HEADER *) buf;
	HEADER *anhp = (HEADER *) answer;
	int terrno = ETIMEDOUT;
	char junk[512];
        int retval = 0;

#ifdef DEBUG
	if (pfcode & PRF_QUERY) {
		p_query(buf);
	}
#endif /* DEBUG */
	if (!(_res.options & RES_INIT))
		if (res_init() == -1) {
			return(-99);
		}
	v_circuit = (_res.options & RES_USEVC) || buflen > PACKETSZ;
	id = ntohs(hp->id); hp->id = htons(id-1);
	conn = (_res.options & RES_STAYOPEN);
	if ((s>=0) && (old_conn) && !(conn)) {
	  (void) close(s);
	  s = (-1);
	}
	old_conn = conn;

	/*
	 * Send request, RETRY times, or until successful
	 */
	for (retry = _res.retry; retry > 0; retry--) {
	   for (ns = 0; ns < _res.nscount; ns++) {
	     hp->id = ntohs(hp->id);
	     hp->id = htons(hp->id + 1);
#ifdef DEBUG
		if (_res.options & RES_DEBUG2)
		  printf(";; Querying server (# %d) address = %s\n", ns+1,
			      inet_ntoa(_res.nsaddr_list[ns].sin_addr));
#endif /* DEBUG */
		if (v_circuit) {
			int truncated = 0;

			/*
			 * Use virtual circuit.
			 */

(void) gettimeofday(&htime,NULL);
savetime(hp->id,&htime);

			if (s < 0) {
				s = socket(AF_INET, SOCK_STREAM, 0);
				if (s < 0) {
					terrno = errno;
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
					    perror(";; socket failed");
#endif /* DEBUG */
					continue;
				}
				if (connect(s, &(_res.nsaddr_list[ns]),
				   sizeof(struct sockaddr)) < 0) {
					terrno = errno;
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
					    perror(";; connect failed");
#endif /* DEBUG */
					(void) close(s);
					s = -1;
					continue;
				}
			}
			/*
			 * Send length & message
			 */
			len = htons((u_short)buflen);
			if (write(s,(caddr_t)&len,sizeof(len)) != sizeof(len)) {
				terrno = errno;
#ifdef DEBUG
                                if (_res.options & RES_DEBUG)
                                        perror(";; write failed");
#endif /* DEBUG */
                                (void) close(s);
                                s = -1;
                                continue;

			}
			if (write(s, buf, buflen) != buflen) {
#ifdef DEBUG
                                if (_res.options & RES_DEBUG)
                                        perror(";; write failed");
#endif /* DEBUG */
                                (void) close(s);
                                s = -1;
                                continue;
			}
#ifdef DEBUG
				if (_res.options & RES_DEBUG2) {
			  printf(";; id = %d - sending now: ",ntohs(hp->id));
				  prnttime(&htime);
				  printf("\n");
				}
#endif /* DEBUG */
			/*
			 * Receive length & response
			 */
			cp = answer;
			len = sizeof(short);
			while (len != 0 &&
			    (n = read(s, (char *)cp, (int)len)) > 0) {
				cp += n;
				len -= n;
			}

(void) gettimeofday(&ftime, NULL);

			if (n <= 0) {
				terrno = errno;
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror(";; read failed");
#endif /* DEBUG */
				(void) close(s);
				s = -1;
				continue;
			}
			cp = answer;
			if ((resplen = ntohs(*(u_short *)cp)) > anslen) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
				    fprintf(stderr, ";; response truncated\n");
#endif /* DEBUG */
				len = anslen;
				truncated = 1;
			} else
				len = resplen;
			while (len != 0 &&
			   (n = read(s, (char *)cp, (int)len)) > 0) {
				cp += n;
				len -= n;
			}

			if (n <= 0) {
				terrno = errno;
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror(";; read failed");
#endif /* DEBUG */
				(void) close(s);
				s = -1;
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
					if ((n = read(s, junk, n)) > 0)
						len -= n;
					else
						break;
				      }
			      }
		} else {
			/*
			 * Use datagrams.
			 */
			if (s < 0)
				s = socket(AF_INET, SOCK_DGRAM, 0);
#if	BSD >= 43
			if (_res.nscount == 1 || retry == _res.retry) {
				/*
				 * Don't use connect if we might
				 * still receive a response
				 * from another server.
				 */
				if (connected == 0) {
					if (connect(s, &_res.nsaddr_list[ns],
					    sizeof(struct sockaddr)) < 0) {
#ifdef DEBUG
						if (_res.options & RES_DEBUG)
							perror(";; connect");
#endif /* DEBUG */
						continue;
					}
					connected = 1;
				}
				if (send(s, buf, buflen, 0) != buflen) {
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
						perror(";; send");
#endif /* DEBUG */
					continue;
				}
			} else
#endif /* BSD */

			if (sendto(s, buf, buflen, 0, &_res.nsaddr_list[ns],
			    sizeof(struct sockaddr)) != buflen) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror(";; sendto");
#endif /* DEBUG */
				continue;
			      }

(void) gettimeofday(&htime,NULL);
savetime(hp->id,&htime);

			/*
			 * Wait for reply
			 */

#ifdef DEBUG
				if (_res.options & RES_DEBUG2) {
			  printf(";; id = %d - sending now: ",ntohs(hp->id));
				  prnttime(&htime);
				  printf("\n");
				}
#endif /* DEBUG */

			timeout.tv_sec = (_res.retrans << (_res.retry - retry))
				/ _res.nscount;
			if (timeout.tv_sec <= 0)
				timeout.tv_sec = 1;
			timeout.tv_usec = 0;
wait:
			FD_ZERO(&dsmask);
			FD_SET(s, &dsmask);
			n = select(s+1, &dsmask, (fd_set *)NULL,
				(fd_set *)NULL, &timeout);
			if (n < 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror(";; select");
#endif /* DEBUG */
				continue;
			}
			if (n == 0) {
				/*
				 * timeout
				 */
#ifdef DEBUG
				if (_res.options & RES_DEBUG2) {
					printf(";; timeout at: ");
					(void) gettimeofday(&ftime, NULL);
					prnttime(&ftime);
					printf("\n");
				      }
#endif /* DEBUG */
				/*
				 * Disconnect if we want to listen
				 * for responses from more than one server.
				 */
				if (_res.nscount > 1 && connected) {
					(void) connect(s, &no_addr,
					    sizeof(no_addr));
					connected = 0;
				}
				gotsomewhere = 1;
				continue;
			      }

(void) gettimeofday(&ftime,NULL);

			if ((resplen = recv(s, answer, anslen, 0)) <= 0) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror(";; recvfrom");
#endif /* DEBUG */
				continue;
			}
			gotsomewhere = 1;

			if (id > anhp->id) {
				/*
				 * response from old query, ignore it
				 */
#ifdef DEBUG
			  if (_res.options & RES_DEBUG) {
			    printf(";****; Old Answer id: %d\n",anhp->id);
				}
#endif /* DEBUG */
				goto wait;
			}
			if (!(_res.options & RES_IGNTC) && anhp->tc) {
				/*
				 * get rest of answer
				 */
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					printf(";; truncated answer\n");
#endif /* DEBUG */
				(void) close(s);
				s = -1;
				/*
				 * retry decremented on continue
				 * to desired starting value
				 */
				retry = _res.retry + 1;
				v_circuit = 1;
				continue;
			}
 		}

	     difftv(&ftime,findtime(anhp->id),&hqtime);
	     hqcount = ntohs(hp->id) - _res.id + 1;
	     hxcount = ntohs(hp->id) - ntohs(anhp->id);

	     _res.id = ntohs(hp->id);
             eecode = retval = anhp->rcode;
	     if (retval)
	       retval = 0 - retval;

#ifdef DEBUG
		if (pfcode & PRF_REPLY) {
			p_query(answer);
		}
#endif /* DEBUG */
		/*
		 * We are going to assume that the first server is preferred
		 * over the rest (i.e. it is on the local machine) and only
		 * keep that one open.
		 */
		if ((_res.options & KEEPOPEN) == KEEPOPEN && ns == 0) {
			return (resplen);
		} else {
			(void) close(s);
			s = -1;
			return (resplen);
		}
	   }
	}
	if (s >= 0) {
		(void) close(s);
		s = -1;
	}
	if (v_circuit == 0) {
		if (gotsomewhere == 0) {
                        eecode = 6;
			errno = ECONNREFUSED;
		      } else {
			errno = ETIMEDOUT;
			eecode = 7;
		      }
	      }	else {
		errno = terrno;
		eecode = 8;
	      }
	return (-99);
      }

/*
 * This routine is for closing the socket if a virtual circuit is used and
 * the program wants to close it.  This provides support for endhostent()
 * which expects to close the socket.
 *
 * This routine is not expected to be user visible.
 */
_res_close()
{
	if (s != -1) {
		(void) close(s);
		s = -1;
	}
}
