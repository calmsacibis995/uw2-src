/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _AIOSYS_H
#define _AIOSYS_H
#ident	"@(#)libthread:common/lib/libthread/asyncio/aiosys.h	1.1.1.3"
#ident  "$Header: $"

extern int      _close(int);

/*
 * asynchronous I/O operation structure, for internal use only.
 * one per user aiocb_t structure, may be linked on per-file-descriptor list
 * or on ready-job list.
 */
typedef	struct aioop aioop_t;
struct	aioop {
	list_t 		a_flist;	/* for a 2-way list */
#define	a_flink		a_flist.flink	/* forward link */
#define	a_blink		a_flist.rlink	/* backward link */
	thread_t	a_tid;		/* id of server thread */
	uchar_t		a_cmd;		/* AIO_READ or AIO_WRITE */
	ushort		a_flag;		/* AIO_CANCEL or AIO_CLOSE */
	aiocb_t		*a_aiocbp;	/* pointer to user's aiocb_t request */
} ;

/*
 * a_cmd field
 */
#define	AIO_READ	(1)
#define	AIO_WRITE	(2)
/*
 * a_flag field; also for 'cmd' arg in cancel
 */
#define	AIO_CANCEL	(1)		/* canceled due to aio_cancel() */
#define	AIO_CLOSE	(2)		/* canceled due to close() */

/*
 * head of per-file-descriptor list, as many as fdt_size.
 * (the list_t member must be the first in both fd_entry_t and aioop_t)
 */
typedef struct fd_entry fd_entry_t;
struct	fd_entry {
	list_t		fd_aiolist;		/* head of per-fd list of aioop_t */
#define	fd_aioflink	fd_aiolist.flink	/* forward link */
#define	fd_aioblink	fd_aiolist.rlink	/* backward link */
	/*
	 * the fd_flag indicates if the file descriptor list is in use, 
	 * empty, waiting for a server, being served, being closed,
	 * or file is opened for sequential or random I/O
	 */
	ushort		fd_flag;		/* see below */
} ;

/*
 * fd_flag: 
 * initially a per-fd list is not F_INUSE. Once accessed by an aioread/aiowrite,
 * it is F_INUSE.
 * Additionally, F_SEQUENTIAL is set if the fd is opened for sequential I/O.
 * The very first request is placed directly on the ready-job
 * list and the F_ONREADYQ is set to indicate that. Then when a server gets
 * to this request, F_ONREADYQ is changed to F_INSVC. F_CLOSING is set when
 * close() is being processed for that file descriptor, and cleared when
 * close() completes.
 *
 */
#define	F_CLOSING	(0x01)		/* reject aioread/aiowrite */
#define	F_ONREADYQ	(0x02)		/* has a job on aio server q */
#define	F_INSVC		(0x04)		/* being served by a server */
#define	F_INUSE		(0x08)		/* in use, list may be empty */
#define F_SEQUENTIAL	(0x010)		/* fd is opened for sequential I/O */
/*
 * file descriptor table structure, one per application
 */
typedef struct fd_table fd_table_t;
struct	fd_table {
	int		fdt_size;	/* number of fd_entry_t in table */
	fd_entry_t	*fdt_array;	/* ptr to actual table of fd_entry_t */
} ;

/*
 * When the fd_table has to be expanded, a new, larger table will be
 * allocated and the old copied into the new. The amount of increment
 * is defined by the following #define.
 */
#define	FDT_INCREMENT	32

/*
 * one (aio) server-thread-control-block per aioserver,
 * it is allocated and passed to aioserver thead on thr_create(),
 * an aioserver frees this structure before calling thr_exit().
 */
typedef struct svrtcb svrtcb_t;
struct	svrtcb {
	svrtcb_t	*st_svrtcbp;	/* to next */
	int		st_fd;		/* set when working on a per-fd queue */
	thread_t	st_tid;		/* thread id of server */
	sigjmp_buf	st_jmpbuf;	/* for sig[set/long]jmp for cancel */
	aioop_t		*st_ap;		/* the job being worked on */
} ;

/*
 * for aio_suspend and lio_listio request since it might block
 */
typedef struct pollreq pollreq_t;
struct pollreq {
	pollreq_t	*p_next;
	aiocb_t		**p_list;	/* list if I/O requests */
	ushort		p_flag;		/* AIO_SUSPEND or AIO_LISTIO	*/
        int             p_size;         /* size of the array */
	int		p_nent;		/* number of elements in the list */
	int		p_runcnt;	/* how many matched */
	struct timespec	p_timeout;	/* time out for this aio_suspend */
	cond_t		p_cond;		/* become available */
};

/*
 * p_flag is set to either AIO_SUSPEND or AIO_LISTIO depending whether
 * request is for aio_suspend() or lio_listio().
 * In case of aio_suspend() the requestor is woken up when any aiocb in 
 * the request has completed; in case of lio_listio() requestor is woken up
 * when all aiocbs in the list completed.  
 */

#define AIO_SUSPEND	(1)
#define AIO_LISTIO	(2)

/*
 * number of elements for various control blocks
 */
#define	MAXAIOOP	(128)
#define	MAXFDQ		(20)
#define	MAXSVRCNT	(30)		/* 1/2 of MAXULWP */
#define	MINSVRCNT	(10)		/* 1/6 of MAXULWP */
#define	MAXSVRTCB	MAXSVRCNT

/*
 * miscellaneous
 */
#define	AIOSLEEP	(20)	/* idle aio servers timeout interval, in seconds */
#define	AIOLONGSLEEP	(86400)	/* 'indefinite' sleep on time-out, in seconds */

#define	NULLV		((void *) NULL)
#define	NULLLIST	((list_t *) NULL)
#define	NULLAIOOP	((aioop_t *) NULL)
#define	NULLAIOCB	((aiocb_t *) NULL)
#define	NULLFDQ		((fd_entry_t *) NULL)
#define	NULLTID		((thread_t) NULL)
#define	NULLSVRTCB	((svrtcb_t *) NULL)
#define	NULLPOLLREQ	((pollreq_t *) NULL)
#define	INVALIDFD	(-1)

/*
 * definition of ASSERT used for debugging.
 */

#ifdef AIODEBUG
#define ASSERT(x)	((void)((x) || aio_assfail(#x, __FILE__, __LINE__)))
#else /* AIODEBUG */
#define ASSERT(x)
#endif /* AIODEBUG */

/*
 * definitions for the support of asynchronous I/O for raw slices of hard disk
 */
#include "sys/aiosys.h"

/*
 * control block for lio_listio() that uses a callback function with LIO_NOWAIT
 */
typedef	struct lioreq lioreq_t;
struct	lioreq {
	lioreq_t 	*lio_next;
	struct sigevent	lio_notify;	/* from lio_listio() sig argument */
	int		lio_total;	/* lio_nent - (no. of ignored) */
	int		lio_cnt;	/* invoke  callback when == lio_total */
};

/*
 * MAXSTATUS: max number of struct aiostatus in one retrieval
 */
#define	MAXSTATUS	(NSTATUS)

#define	NULLLIOREQ	((lioreq_t *) NULL)

#endif /* _AIOSYS_H */
