/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_IN_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_IN_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/in_f.h	1.4"
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
 * Constants and structures defined by the internet system,
 * Per RFC 790, September 1981.
 */

/*
 * Internet address
 */
struct in_addr {
	union {
		struct { unsigned char s_b1, s_b2, s_b3, s_b4; } S_un_b;
		struct { unsigned short s_w1, s_w2; } S_un_w;
		unsigned long S_addr;
	} S_un;
#define s_addr	S_un.S_addr
};

/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 */
#define IN_CLASSA(i)		(((long)(i) & 0x80000000) == 0)
#define IN_CLASSA_NET		0xff000000U
#define IN_CLASSA_NSHIFT	24
#define IN_CLASSA_HOST		0x00ffffff
#define IN_CLASSA_MAX		128

#define IN_CLASSB(i)		(((long)(i) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET		0xffff0000U
#define IN_CLASSB_NSHIFT	16
#define IN_CLASSB_HOST		0x0000ffff
#define IN_CLASSB_MAX		65536

#define IN_CLASSC(i)		(((long)(i) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET		0xffffff00U
#define IN_CLASSC_NSHIFT	8
#define IN_CLASSC_HOST		0x000000ff

#define IN_CLASSD(i)		(((long)(i) & 0xf0000000) == 0xe0000000)
#define IN_MULTICAST(i)		IN_CLASSD(i)

#define IN_EXPERIMENTAL(i)	(((long)(i) & 0xe0000000) == 0xe0000000)
#define IN_BADCLASS(i)		(((long)(i) & 0xf0000000) == 0xf0000000)

#define INADDR_ANY		(unsigned long)0x00000000
#define INADDR_LOOPBACK		(unsigned long)0x7F000001
#define INADDR_BROADCAST	(unsigned long)0xffffffff /* must be masked */
#define INADDR_NONE		(unsigned long)0xffffffff

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_IN_F_H */
