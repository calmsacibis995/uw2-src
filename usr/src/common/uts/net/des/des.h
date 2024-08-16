/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_DES_DES_H	/* wrapper symbol for kernel use */
#define _NET_DES_DES_H	/* subject to change without notice */

#ident	"@(#)kern:net/des/des.h	1.4"
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
 *	des.h, generic DES driver interface
 *
 *	This file should be hardware independent.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */

#endif

/*
 * maximum # of bytes to encrypt
 */
#define	DES_MAXLEN 		65536
/*
 * maximum # of bytes to encrypt quickly 
 */
#define DES_QUICKLEN		16

enum	desdir			{ ENCRYPT, DECRYPT };
enum	desmode			{ CBC, ECB };

/*
 * parameters to ioctl call
 */
struct desparams {
	u_char des_key[8];	/* key (with low bit parity) */
	enum desdir des_dir;	/* direction */
	enum desmode des_mode;	/* mode */
	u_char des_ivec[8];	/* input vector */
	unsigned des_len;	/* number of bytes to crypt */
	union {
		u_char UDES_data[DES_QUICKLEN];
		u_char *UDES_buf;
	} UDES;
#	define des_data UDES.UDES_data	/* direct data here if quick */
#	define des_buf	UDES.UDES_buf	/* otherwise, pointer to data */
};

/*
 * encrypt an arbitrary sized buffer
 */
#define	DESIOCBLOCK		_IOWR('d', 6, struct desparams)

/* 
 * encrypt of small amount of data, quickly
 */
#define DESIOCQUICK		_IOWR('d', 7, struct desparams) 

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_DES_DES_H*/
