/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IN_COMP_H	/* wrapper symbol for kernel use */
#define _NET_INET_IN_COMP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/in_comp.h	1.5"
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
 * Definitions for tcp compression routines.
 *
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	Van Jacobson (van@helios.ee.lbl.gov), Dec 31, 1989:
 *	- Initial distribution.
 */

/*
 *	STREAMware TCP/IP - Release
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
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

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *		  All rights reserved.
 *
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>			/* REQUIRED */
#include <net/inet/ip/ip.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/stream.h>			/* REQUIRED */
#include <netinet/ip.h>			/* REQUIRED */

#else

#include <sys/stream.h>			/* REQUIRED */
#include <netinet/ip.h>			/* REQUIRED */

#endif

#ifdef _KERNEL

#define MAX_STATES	16		/* must be > 2 and < 256 */
#define MIN_STATES	3
#define MAX_HDR		128

/*
 * Compressed packet format:
 *
 * The first octet contains the packet type (top 3 bits), TCP 'push'
 * bit, and flags that indicate which of the 4 TCP sequence numbers
 * have changed (bottom 5 bits).  The next octet is a conversation
 * number that associates a saved IP/TCP header with the compressed
 * packet.  The next two octets are the TCP checksum from the original
 * datagram.	The next 0 to 15 octets are sequence number changes,
 * one change per bit set in the header (there may be no changes and
 * there are two special cases where the receiver implicitly knows
 * what changed -- see below).
 *
 * There are 5 numbers which can change (they are always inserted in
 * the following order): TCP urgent pointer, window, acknowlegement,
 * sequence number and IP ID.  (The urgent pointer is different from
 * the others in that its value is sent, not the change in value.)
 * Since typical use of SLIP links is biased toward small packets (see
 * comments on MTU/MSS below), changes use a variable length coding
 * with one octet for numbers in the range 1 - 255 and 3 octets (0,
 * MSB, LSB) for numbers in the range 256 - 65535 or 0.  (If the
 * change in sequence number or ack is more than 65535, an
 * uncompressed packet is sent.)
 */

/*
 * Packet types (must not conflict with IP protocol version)
 *
 * The top nibble of the first octet is the packet type.  There are
 * three possible types: IP (not proto TCP or tcp with one of the
 * control flags set); uncompressed TCP (a normal IP/TCP packet but
 * with the 8-bit protocol field replaced by an 8-bit connection id --
 * this type of packet syncs the sender & receiver); and compressed
 * TCP (described above).
 *
 * LSB of 4-bit field is TCP "PUSH" bit (a worthless anachronism) and
 * is logically part of the 4-bit "changes" field that follows.	 Top
 * three bits are actual packet type.  For backward compatibility
 * and in the interest of conserving bits, numbers are chosen so the
 * IP protocol version number (4) which normally appears in this nibble
 * means "IP packet".
 */

/* packet types */
#define TYPE_IP				0x40
#define TYPE_UNCOMPRESSED_TCP		0x70
#define TYPE_COMPRESSED_TCP		0x80
#define TYPE_ERROR			0x00

/* Bits in first octet of compressed packet */
#define NEW_C	0x40	/* flag bits for what changed in a packet */
#define NEW_I	0x20
#define NEW_S	0x08
#define NEW_A	0x04
#define NEW_W	0x02
#define NEW_U	0x01

/* reserved, special-case values of above */
#define SPECIAL_I (NEW_S|NEW_W|NEW_U)	/* echoed interactive traffic */
#define SPECIAL_D (NEW_S|NEW_A|NEW_W|NEW_U) /* unidirectional data */
#define SPECIALS_MASK (NEW_S|NEW_A|NEW_W|NEW_U)

#define TCP_PUSH_BIT 0x10

/*
 * "state" data for each active tcp conversation on the wire.  This is
 * basically a copy of the entire IP/TCP header from the last packet
 * we saw from the conversation together with a small identifier
 * the transmit & receive ends of the line use to locate saved header.
 */
struct cstate {
	struct cstate  *cs_next;	/* next most recently used cstate
					 * (xmit only) */
	unsigned short	cs_hlen;	/* size of hdr (receive only) */
	unsigned char	cs_id;		/* connection # associated with this
					 * state */
	unsigned char	cs_filler;
	union {
		char		csu_hdr[MAX_HDR];
		struct ip	csu_ip;	/* IP/TCP hdr from most recent packet*/
	} slcs_u;
};
#define cs_ip slcs_u.csu_ip
#define cs_hdr slcs_u.csu_hdr

/*
 * all the state data for one serial line (we need one of these
 * per line).
 */
struct incompress {
	struct cstate  *last_cs;	/* most recently used tstate */
	unsigned char	last_recv;	/* last rcvd conn. id */
	unsigned char	last_xmit;	/* last sent conn. id */
	unsigned short	flags;
	unsigned short	t_max_states;
	unsigned short	r_max_states;
#ifndef SL_NO_STATS
	int		sls_packets;	/* outbound packets */
	int		sls_compressed;	/* outbound compressed packets */
	int		sls_searches;	/* searches for connection state */
	int		sls_misses;	/* times couldn't find conn. state */
	int		sls_uncompressedin; /* inbound uncompressed packets */
	int		sls_compressedin; /* inbound compressed packets */
	int		sls_errorin;	/* inbound unknown type packets */
	int		sls_tossed;	/* inbound packets tossed because of
					 * error */
#endif
	struct cstate	tstate[MAX_STATES]; /* xmit connection states */
	struct cstate	rstate[MAX_STATES]; /* receive connection states */
};


/* flag values */
#define SLF_TOSS 0x01		/* tossing rcvd frames because of input err */

extern struct incompress *incompalloc(void);
extern void incompfree(struct incompress *);
extern void in_compress_init(struct incompress *, u_short, u_short);
extern unsigned char in_compress_tcp(mblk_t *, struct ip *,
				     struct incompress *, int);
extern int in_uncompress_tcp(unsigned char **, int, unsigned int,
			     struct incompress *, unsigned char **);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IN_COMP_H */
