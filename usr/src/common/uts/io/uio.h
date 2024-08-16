/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_UIO_H	/* wrapper symbol for kernel use */
#define _IO_UIO_H	/* subject to change without notice */

#ident	"@(#)kern:io/uio.h	1.16"
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
 * I/O parameter information.  A uio structure describes the I/O which
 * is to be performed by an operation.  Typically the data movement will
 * be performed by a routine such as uiomove(), which updates the uio
 * structure to reflect what was done.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#else

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

typedef struct iovec {
	caddr_t	iov_base;
	int	iov_len;
} iovec_t;

#if defined(_KERNEL) || defined(_KMEMUSER)

typedef struct uio {
	iovec_t	*uio_iov;	/* pointer to array of iovecs */
	int	uio_iovcnt;	/* number of iovecs */
	off_t	uio_offset;	/* file offset */
	short	uio_segflg;	/* address space (kernel or user) */
	short	uio_fmode;	/* file mode flags */
	daddr_t	uio_limit;	/* u-limit (maximum "block" offset) */
	int	uio_resid;	/* residual count */
} uio_t;

/*
 * I/O direction.
 */
typedef enum uio_rw { UIO_READ, UIO_WRITE } uio_rw_t;

/*
 * Segment flag values.
 */
typedef enum uio_seg { UIO_USERSPACE, UIO_SYSSPACE, UIO_USERISPACE } uio_seg_t;

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#ifdef __STDC__
extern int uiomove_catch(void *kernbuf, long n, uio_rw_t rw, uio_t *uiop,
			 uint_t catch_flags);
extern int ureadc(int val, uio_t *uiop);
extern int uwritec(uio_t *uiop);
extern int uiomvuio(uio_t *ruiop, uio_t *wuiop);
extern void uioskip(uio_t *uiop, long n);
extern void uioupdate(uio_t *uiop, long n);
#else
extern int ureadc();
extern int uwritec();
extern int uiomvuio();
extern void uioskip();
extern void uioupdate();
#endif


#if !defined(_DDI)

#define _UIOMOVE(kernbuf, n, rw, uiop) \
		uiomove_catch(kernbuf, n, rw, uiop, CATCH_KERNEL_FAULTS)
#define uiomove(kernbuf, n, rw, uiop)	_UIOMOVE(kernbuf, n, rw, uiop)

#endif /* !_DDI */


#else /* ! _KERNEL  */

#ifdef __STDC__
extern ssize_t readv(int, const struct iovec *, int);
extern ssize_t writev(int, const struct iovec *, int);
#else
extern int readv();
extern int writev();
#endif /* __STDC__  */


#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_UIO_H */
