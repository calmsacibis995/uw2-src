/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_ALP_ALP_H	/* wrapper symbol for kernel use */
#define _IO_ALP_ALP_H	/* subject to change without notice */

#ident	"@(#)kern:io/alp/alp.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if _KERNEL_HEADERS

#include <util/types.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <util/types.h>

#else

/* source code compatibility */
#include <sys/types.h>

#endif

#ifndef ALP_QUERY	/* possible source dependency */

#if defined(_KERNEL) || defined(_KMEMUSER)

struct queue;
struct msgb;

typedef struct algo {
	int al_flag;		/* 0 = in-core, 1 = user-level */
	struct queue *al_rq;	/* queues for user-level algorithms */
	struct queue *al_wq;	/* queues for user-level algorithms */
	struct msgb *(*al_func)(struct msgb *, caddr_t); /* interface routine */
	caddr_t (*al_open)(int, caddr_t);	/* open/close routine */
	uchar_t *al_name;	/* name */
	uchar_t *al_expl;	/* explanation */
	struct algo *al_next;	/* next in chain */
} algo_t;

#endif  /* _KERNEL || _KMEMUSER */

#if defined(_KERNEL)

int            alp_register(algo_t *);
struct msgb *(*alp_con(uchar_t *, caddr_t *))(struct msgb *, caddr_t);
struct msgb   *alp_discon(uchar_t *, caddr_t);
algo_t        *alp_query(uchar_t *);

#define OPEN_PROCESS	1
#define CLOSE_PROCESS	0

#endif /* _KERNEL */

#define IALP	('&'<<8|128)

#define ALP_QUERY	(IALP|1)	/* query ioctl */

/*
 * query return value
 */

typedef struct alp_q {
	int a_seq;		/* sequence number */
	int a_flag;		/* flag */
	uchar_t a_name[16];	/* algorithm name */
	uchar_t a_expl[64];	/* explanation field */
} alp_q_t;

#endif	/* ALP_HEADER */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_ALP_ALP_H */
