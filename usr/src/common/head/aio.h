/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.usr:aio.h	1.7"
#ident	"$Header: $"

#ifndef _AIO_H
#define _AIO_H
#include <time.h>  
#include <siginfo.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * asynchronous I/O control block for use with aioread/aiowrite
 */

typedef volatile struct aiocb aiocb_t;
struct	aiocb {
	int		aio_fildes;	/* file descriptor 		*/
	volatile void*	aio_buf;	/* data buffer 			*/
	size_t		aio_nbytes;	/* number of bytes of data 	*/
	off_t		aio_offset;	/* file offset 			*/
	int		aio_reqprio;	/* request priority offset 	*/
	struct sigevent	aio_sigevent;	/* signal number and offset 	*/
	int		aio_lio_opcode;	/* listio operation		*/
	ssize_t		aio__return;	/* operation result value	*/
	int		aio__error;	/* operation error code		*/
	int		aio_flags;	/* flags			*/
	void		*aio__next;	/* GP pointer for implementation */
	int		aio_pad[1];	/* expansion padding		*/
} ;

/*
 * return values for aio_cancel()
 */
#define	AIO_CANCELED	(0)	/* requested operation(s) were canceled	*/
#define	AIO_ALLDONE	(1)	/* all operations have completed */
#define	AIO_NOTCANCELED	(2)	/* at least one of the requested operation(s) is not canceled */

/*
 * mode values for lio_listio()
 */
#define	LIO_NOWAIT	(0)	/* don't wait for listio completion */
#define	LIO_WAIT	(1)	/* wait for listio completion */

/*
 * listio operation codes
 */
#define	LIO_NOP		(0)	/* ignore this aiocb */
#define	LIO_READ	(1)	/* treat as aio_read() */
#define	LIO_WRITE	(2)	/* treat as aio_write() */

/*
 * values for the aio_flag field
 */
#define AIO_RAW		(1)   /* request is to a raw slice of hard disk */

/*
 * function prototypes for asynchronous I/O interfaces
 */
#if defined(__STDC__)

extern int	aio_read(struct aiocb *);
extern int	aio_write(struct aiocb *);
extern int	lio_listio(int, struct aiocb **, int, struct sigevent *);
extern int	aio_error(const struct aiocb *);
extern int	aio_return(struct aiocb *);
extern int	aio_cancel(int, struct aiocb *);
extern int	aio_suspend(const struct aiocb **, int, const struct timespec *);
extern int	aio_fsync(int, struct aiocb *);

#else /* __STDC__ */

extern int	aio_read();
extern int	aio_write();
extern int	lio_listio();
extern int	aio_error();
extern int	aio_return();
extern int	aio_cancel();
extern int	aio_suspend();
extern int	aio_fsync();

#endif /* __STDC__ */

#if defined(__cplusplus)
}
#endif

#endif /* _AIO_H */
