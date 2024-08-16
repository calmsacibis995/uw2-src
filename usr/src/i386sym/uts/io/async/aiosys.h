/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_AIOSYS_H
#define _IO_AIOSYS_H

#ident	"@(#)kern-i386sym:io/async/aiosys.h	1.5"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef	_KERNEL_HEADERS
#include <proc/proc.h>
#else /* !_KERNEL_HEADERS */
#include <sys/proc.h>
#endif /* _KERNEL_HEADERS */
/*
 * For i386sym, async I/O driver is not supported, but this header
 * file is needed for the libthread/asyncio to build
 */

#define 	AIOMEMLOCK      1
#define 	AIORW		2
#define 	AIOREAD         3
#define 	AIOWRITE        4
#define 	AIOPOLL         5
#define		AIOLISTIO	6
#define		AIOGETTUNE	7	/* Retr val of all kernel tuneables */

/* Max number of elements in a poll request. */
#define		NSTATUS		32

/* Limit on size of a single async I/O. */
#define		AIO_SIZE_LIMIT 	PAGESIZE
#define 	AIO_MAP_SIZE	((AIO_SIZE_LIMIT >> PAGESHIFT) + 1)


typedef struct aiostatus {
	void		*ast_cbp;	/* User provided cbp */
	int		ast_errno;	/* Error returned by the I/O */
	ssize_t		ast_count;	/* Size of I/O completed */
} aiostatus_t;

typedef struct aioresult {
	int		ar_total;
	uint_t		ar_timeout;
	aiostatus_t	ar_stat[NSTATUS];
} aioresult_t; 

typedef struct asyncmlock {
	vaddr_t	am_vaddr;		/* Addr of user provided buffer */
	size_t	am_size;		/* Size of the user buffers */
} asyncmlock_t;

typedef struct aiojob {
	int		aj_fd;		/* File descriptor from user */
	vaddr_t		aj_buf;	/* User buffer for I/O */
	size_t		aj_cnt;		/* Number of bytes for I/O */
	off_t		aj_offset;	/* Offset into the raw slice */
	uint_t		aj_flag;	/* Flag variable for general use*/
	uint_t		aj_cmd;		/* Read or write */
	void		*aj_cbp;	/* Ptr to the libctl structure */
	int		aj_errno;	/* Errno for list I/O */
} aiojob_t;

typedef struct aiolistio {
	uint_t		al_mode;	/* LIOWAIT versus LIONOWAIT */
	uint_t		al_nent;	/* Number of entries in the list */
	uint_t		al_flag;	/* Async notifcation ? */
	aiojob_t	al_jobs[1];	/* List of jobs */
} aiolistio_t;
#define LIOWAIT		1
#define LIONOWAIT 	2

typedef struct aio_tune {
	uint_t 	at_listio_max;	/* Max size of any LIST I/O */
	uint_t	at_max;		/* Max outstanding I/O per process */
	uint_t	at_num_ctlblks;	/* Total no of configured ctl blocks */
} aio_tune_t;

#define	A_ASYNCNOTIFY		0x01	

#ifdef _KERNEL

extern void aio_intersect(struct as *as, vaddr_t base, size_t size);
extern void aio_as_free(struct as *as);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_AIOSYS_H */
