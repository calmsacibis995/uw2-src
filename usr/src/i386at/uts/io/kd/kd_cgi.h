/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_KD_KD_CGI_H	/* wrapper symbol for kernel use */
#define _IO_KD_KD_CGI_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/kd/kd_cgi.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Structure for listing valid adapter I/O addresses
 */
struct portrange {
	ushort_t first;		/* first port */
	ushort_t count;		/* number of valid right after 'first' */
};

#define	BLACK		0x0
#define	BLUE		0x1
#define	GREEN		0x2
#define	CYAN		0x3
#define	RED		0x4
#define	MAGENTA		0x5
#define	BROWN		0x6
#define	WHITE		0x7
#define	GRAY		0x8
#define	LT_BLUE		0x9
#define	LT_GREEN	0xA
#define	LT_CYAN		0xB
#define	LT_RED		0xC
#define	LT_MAGENTA	0xD
#define	YELLOW		0xE
#define	HI_WHITE	0xF

struct cgi_class {
	char   *name;
	char   *text;
	long	base;
	long	size;
	struct portrange *ports;
};

#ifdef _KERNEL

struct channel_info;

extern int cgi_mapclass(struct channel_info *, int, int *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_KD_KD_CGI_H */
