/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_BYTEORDER_F_H	/* wrapper symbol for kernel use */
#define _NET_INET_BYTEORDER_F_H	/* subject to change without notice */

#ident	"@(#)kern-i386:net/inet/byteorder_f.h	1.6"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

#define BYTE_ORDER	LITTLE_ENDIAN

#if defined(__cplusplus)
inline unsigned long
ntohl(unsigned long nl) {
	return (((nl<<24)&0xFF000000) +
		((nl<<8)&0xFF0000) +
		((nl>>8)&0xFF00) +
		((nl>>24)&0xFF));
}

inline unsigned long
htonl(unsigned long hl) {
	return (((hl<<24)&0xFF000000) +
		((hl<<8)&0xFF0000) +
		((hl>>8)&0xFF00) +
		((hl>>24)&0xFF));
}

inline unsigned short
ntohs(unsigned short ns) {
	return (((ns<<8)&0xFF00) +
		((ns>>8)&0xFF));
}

inline unsigned short
htons(unsigned short hs) {
	return (((hs<<8)&0xFF00) +
		((hs>>8)&0xFF));
}

inline unsigned short
bswaps(unsigned short us) {
	return (((us<<8)&0xFF00) +
		((us>>8)&0xFF));
}

#else

/*
 * The following macro will swap bytes in a short.
 * Warning: this macro expects 16-bit shorts and 8-bit chars
 */

#define bswaps(us)	(((unsigned short)((us) & 0xff) << 8) | \
			((unsigned short)((us) & ~0xff) >> 8))

/*
 * Macros for conversion between host and internet network byte order.
 *
 */

/*
 *	unsigned long htonl(hl)
 *	long hl;
 *	reverses the byte order of 'long hl'
 */

#define htonl(hl) __htonl(hl)
#ifdef __STDC__
__asm unsigned long __htonl(hl)
#else
asm unsigned long __htonl(hl)
#endif
{
%mem	hl;
	movl	hl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned long ntohl(nl)
 *	unsigned long nl;
 *	reverses the byte order of 'unsigned long nl'
 */

#define ntohl(nl) __ntohl(nl)
#ifdef __STDC__
__asm unsigned long __ntohl(nl)
#else
asm unsigned long __ntohl(nl)
#endif
{
%mem	nl;
	movl	nl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned short htons(hs)
 *	short hs;
 *
 *	reverses the byte order in hs.
 */

#define htons(hs) __htons(hs)
#ifdef __STDC__
__asm unsigned short __htons(hs)
#else
asm unsigned short __htons(hs)
#endif
{
%mem	hs;
	movl	hs, %eax
	xchgb	%ah, %al
	clc
}

/*
 *	unsigned short ntohs(ns)
 *	unsigned short ns;
 *
 *	reverses the bytes in ns.
 */

#define ntohs(ns) __ntohs(ns)
#ifdef __STDC__
__asm unsigned short __ntohs(ns)
#else
asm unsigned short __ntohs(ns)
#endif
{
%mem	ns;
	movl	ns, %eax
	xchgb	%ah, %al
	clc
}

#endif /* __cplusplus */

#endif /* _NET_INET_BYTEORDER__F_H */
