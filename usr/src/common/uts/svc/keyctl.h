/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_KEYCTL_H   /* wrapper symbol for kernel use */
#define _SVC_KEYCTL_H   /* subject to change without notice */

#ident	"@(#)kern:svc/keyctl.h	1.4"
#ident  "$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* 
 * Definition of current actions supported by the keyctl() system call
 */
#define K_SETLIMIT	0x00010000
#define K_GETLIMIT	0x00020000
#define K_INCRUSE	0x00030000
#define K_DECRUSE	0x00040000
#define K_PINCRUSE	0x00050000
#define	K_VALIDATE	0x00060000

/*
 * Definition of resources currently controlled via the keyctl() system call
 * 
 */
#define K_USER		0x00000000
#define K_PROC		0x00000001

#define K_ACTIONMASK	0xFFFF0000
#define K_RESOURCEMASK	0x0000FFFF

#define K_GETPROCLIMIT	(K_GETLIMIT | K_PROC)
#define K_GETUSERLIMIT	(K_GETLIMIT | K_USER)
#define K_SETPROCLIMIT	(K_SETLIMIT | K_PROC)
#define K_SETUSERLIMIT	(K_SETLIMIT | K_USER)
#define K_INCRCURNUSER	(K_INCRUSE  | K_USER)
#define K_PINCRCURNUSER	(K_PINCRUSE | K_USER)
#define K_DECRCURNUSER	(K_DECRUSE  | K_USER)

#define	MAXSKEYS	256
#define K_UNLIMITED	0xEFFF

#define TYPELEN	15
#define SNLEN	10
#define STRLEN	16
#define SKLEN	8

struct k_skey {
	uchar_t	sernum[STRLEN];	/* serial number */
	uchar_t	serkey[STRLEN];	/* serial key */
};

typedef struct k_skey k_skey_t;

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_KEYCTL_H */
