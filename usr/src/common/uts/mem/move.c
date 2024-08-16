/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/move.c	1.8"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <io/uio.h>
#include <mem/faultcatch.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * int
 * uiomove_catch(void *kernbuf, long n, uio_rw_t rw, uio_t *uio,
 *		 uint_t catch_flags)
 *	Copy data to/from a kernel buffer for I/O.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  kernbuf	pointer to kernel data buffer
 *	  n		max number of bytes to copy
 *	  rw		read/write flag: UIO_READ or UIO_WRITE
 *	  uio		"user" I/O descriptor
 *	  catch_flags	CATCH_FAULTS flags; see faultcatch.h
 *	Returns:
 *	  0 if successful, else errno
 *
 * Description:
 *	Copies MIN(n, uio->uio_resid) bytes between kernbuf and the
 *	locations pointed to by uio (which may be either in user or kernel
 *	address space, depending on uio->uio_segflg).  If rw is UIO_READ,
 *	kernbuf is the source; if rw is UIO_WRITE, kernbuf is the destination.
 *
 *	The pointers and counts in the uio structure are updated to reflect
 *	the bytes which were transfered (which may be fewer than requested,
 *	if an error occurred).
 *
 *	This interface includes protection against kernel page fault errors.
 *	A fault error in any pageable kernel address will cause a non-zero
 *	errno to be returned.  A fault error on a user address will cause
 *	EFAULT to be returned.
 */
int
uiomove_catch(void *kernbuf, long n, uio_rw_t rw, uio_t *uio,
	      uint_t catch_flags)
{
	iovec_t *iov;
	size_t cnt;
	int error = 0;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = MIN((long)iov->iov_len, n);
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		switch (uio->uio_segflg) {

		case UIO_USERSPACE:
		case UIO_USERISPACE:
			error = (rw == UIO_READ
				? ucopyout(kernbuf, iov->iov_base, cnt,
					   catch_flags)
				: ucopyin(iov->iov_base, kernbuf, cnt,
					  catch_flags));
			break;

		case UIO_SYSSPACE:
			CATCH_FAULTS(catch_flags) {
				if (rw == UIO_READ)
					bcopy(kernbuf, iov->iov_base, cnt);
				else
					bcopy(iov->iov_base, kernbuf, cnt);
			}
			error = END_CATCH();
			break;

		default:
			error = EINVAL;
			break;
		}
		if (error)
			return error;
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		kernbuf = (char *)kernbuf + cnt;
		n -= cnt;
	}
	return 0;
}



/*
 * int
 * ureadc(int val, uio_t *uiop)
 *	Copy a single character into a "user" I/O buffer.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  val		character to copy to "user" I/O buffer
 *	  uio		"user" I/O descriptor
 *	Returns:
 *	  0 if successful, else EFAULT
 *
 * Description:
 *	Writes a one-byte value, val, to the first location pointed to
 *	by uio (which may be either in user or kernel address space,
 *	depending on uio->uio_segflg).
 *
 *	The pointers and counts in the uio structure are updated to reflect
 *	the bytes which were transfered (which may be fewer than requested,
 *	if an error occurred).
 *
 *	A fault error on the user address will cause EFAULT to be returned.
 */
int
ureadc(int val, uio_t *uiop)
{
	iovec_t *iovp;
	char c;

	/*
	 * first determine if uio is valid.  uiop should be 
	 * non-NULL and the resid count > 0.
	 */
	if (!(uiop && uiop->uio_resid > 0)) 
		return EFAULT;

	/*
	 * scan through iovecs until one is found that is non-empty.
	 * Return EFAULT if none found.
	 */
	while (uiop->uio_iovcnt > 0) {
		iovp = uiop->uio_iov;
		if (iovp->iov_len <= 0) {
			uiop->uio_iovcnt--;
			uiop->uio_iov++;
		} else
			break;
	}

	if (uiop->uio_iovcnt <= 0)
		return EFAULT;

	/*
	 * Transfer character to uio space.
	 */

	c = (char)val;

	switch (uiop->uio_segflg) {

	case UIO_USERISPACE:
	case UIO_USERSPACE:
		if (ucopyout(&c, iovp->iov_base, sizeof(char), 0))
			return EFAULT;
		break;

	case UIO_SYSSPACE: /* can do direct copy since kernel-kernel */
		*iovp->iov_base = c;
		break;

	default:
		return EFAULT; /* invalid segflg value */
	}

	/*
	 * bump up/down iovec and uio members to reflect transfer.
	 */
	iovp->iov_base++;
	iovp->iov_len--;
	uiop->uio_resid--;
	uiop->uio_offset++;
	return 0; /* success */
}


/*
 * int
 * uwritec(uio_t *uiop)
 *	Copy a single character from a "user" I/O buffer.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  uio		"user" I/O descriptor
 *	Returns:
 *	  0 if successful, else EFAULT
 *
 * Description:
 *	Reads, and returns, a one-byte value from the first location pointed
 *	to by uio (which may be either in user or kernel address space,
 *	depending on uio->uio_segflg).
 *
 *	The pointers and counts in the uio structure are updated to reflect
 *	the bytes which were transfered (which may be fewer than requested,
 *	if an error occurred).
 *
 *	A fault error on the user address, or other error, will cause -1
 *	to be returned.
 */
int
uwritec(uio_t *uiop)
{
	iovec_t *iovp;
	char c;

	/*
	 * verify we were passed a valid uio structure.
	 * (1) non-NULL uiop, (2) positive resid count
	 * (3) there is an iovec with positive length 
	 */

	if (!(uiop && uiop->uio_resid > 0)) 
		return -1;

	while (uiop->uio_iovcnt > 0) {
		iovp = uiop->uio_iov;
		if (iovp->iov_len <= 0) {
			uiop->uio_iovcnt--;
			uiop->uio_iov++;
		} else
			break;
	}

	if (uiop->uio_iovcnt <= 0)
		return -1;

	/*
	 * Get the character from the uio address space.
	 */
	switch (uiop->uio_segflg) {

	case UIO_USERISPACE:
	case UIO_USERSPACE:
		if (ucopyin(iovp->iov_base, &c, sizeof(char), 0))
			return -1;
		break;

	case UIO_SYSSPACE:
		c = *iovp->iov_base;
		break;

	default:
		return -1; /* invalid segflg */
	}

	/*
	 * Adjust fields of iovec and uio appropriately.
	 */
	iovp->iov_base++;
	iovp->iov_len--;
	uiop->uio_resid--;
	uiop->uio_offset++;
	return (uchar_t)c; /* success */
}

/*
 * viod
 * uioupdate(uio_t uiop, long n)
 * 	update uio_resid.
 *
 * Calling/Exit State:
 * 	Update "uio" to reflect that "n" bytes of data were
 * 	(or were not) moved.  The state of the iovecs are not
 * 	updated and are not consistent with uio_resid.  Positive
 * 	"n" means n bytes were copied.  Negative "n" means "uncopy"
 * 	n bytes.
 */
void
uioupdate(register uio_t *uiop, register long n)
{
        if ((n > 0) && (n > uiop->uio_resid)) {
                n = uiop->uio_resid;
	}
        uiop->uio_resid -= n;
        uiop->uio_offset += n;
}

/*
 * void
 * uioskip(uio_t *uiop, long n)
 *	Skip over the next n bytes of *uiop, without copying them anywhere.
 *
 * Calling/Exit State:
 *	The uiop structure is adjusted as if uiomove were called for n bytes,
 *	but no data is copied.
 */
void
uioskip(uio_t *uiop, long n)
{
	if (n > uiop->uio_resid)
		return;
	while (n != 0) {
		iovec_t *iovp = uiop->uio_iov;
		size_t niovb = MIN((long)iovp->iov_len, n);

		if (niovb == 0) {
			uiop->uio_iov++;
			uiop->uio_iovcnt--;
			continue;
		}	
		iovp->iov_base += niovb;
		uiop->uio_offset += niovb;
		iovp->iov_len -= niovb;
		uiop->uio_resid -= niovb;
		n -= niovb;
	}
}

/*
 * int
 * uiomvuio(uio_t *ruio, uio_t *wuio)
 *	Copy data between two "user" I/O buffers.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  ruio		source "user" I/O descriptor
 *	  wuio		destination "user" I/O descriptor
 *	Returns:
 *	  0 on success or a non-zero errno on failure
 *
 * Description:
 *	Copies MIN(ruio->uio_resid, wuio->uio_resid) bytes from addresses
 *	described by ruio to those described by wuio.  Both uio structures
 *	are updated to reflect the move.
 *
 *	At least one of the I/O buffers must be in kernel space (UIO_SYSSPACE).
 */
int
uiomvuio(uio_t *ruio, uio_t *wuio)
{
	iovec_t *riov;
	iovec_t *wiov;
	int n;
	size_t cnt;
	boolean_t kerncp;
	int err;

	n = MIN(ruio->uio_resid, wuio->uio_resid);
	kerncp = (ruio->uio_segflg == UIO_SYSSPACE &&
		  wuio->uio_segflg == UIO_SYSSPACE);

	riov = ruio->uio_iov;
	wiov = wuio->uio_iov;
	while (n) {
		while (!wiov->iov_len) {
			wiov = ++wuio->uio_iov;
			wuio->uio_iovcnt--;
		}
		while (!riov->iov_len) {
			riov = ++ruio->uio_iov;
			ruio->uio_iovcnt--;
		}
		cnt = MIN(wiov->iov_len, MIN(riov->iov_len, n));

		if (kerncp)
			bcopy(riov->iov_base, wiov->iov_base, cnt);
		else {
		    if (ruio->uio_segflg == UIO_SYSSPACE)
			err = ucopyout(riov->iov_base, wiov->iov_base, cnt, 0);
		    else
			err = ucopyin(riov->iov_base, wiov->iov_base, cnt, 0);
		    if (err)
			return err;
		}

		riov->iov_base += cnt;
		riov->iov_len -= cnt;
		ruio->uio_resid -= cnt;
		ruio->uio_offset += cnt;
		wiov->iov_base += cnt;
		wiov->iov_len -= cnt;
		wuio->uio_resid -= cnt;
		wuio->uio_offset += cnt;
		n -= cnt;
	}
	return 0;
}

/*
 * void
 * ovbcopy(void *from, void *to, size_t count)
 *	Overlapping bcopy (source and target may overlap arbitrarily).
 *
 * Calling/Exit State:
 *	Parameters:
 *	  from		pointer to source buffer
 *	  to		pointer to destination buffer
 *	  count		number of bytes to copy
 */
void
ovbcopy(void *from, void *to, size_t count)
{
	register char *src_cp = from;
	register char *dst_cp = to;
	register int diff;

	if ((diff = src_cp - dst_cp) < 0)
		diff = -diff;
	if (src_cp < dst_cp && count > diff) {
		do {
			count--;
			*(dst_cp + count) = *(src_cp + count);
		} while (count);
	} else if (src_cp != dst_cp) {
		while (count--)
			*dst_cp++ = *src_cp++;
	}
}

/*
 * int
 * copystr(const char *from, char *to, size_t max, size_t *np)
 *	Copy a string from a kernel address into a kernel buffer.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  from		pointer to source null-terminated string
 *	  to		pointer to kernel destination buffer
 *	  max		size of destination buffer, including room for null
 *	  np		out arg, if non-NULL, set to string length, inc. null
 *	Returns:
 *	  0 if successful, else errno (ENAMETOOLONG if string is too long).
 *	  If not successful, *np is not changed.
 */
int
copystr(const char *from, char *to, size_t max, size_t *np)
{
	int len;

	if ((len = strcpy_max(to, from, max)) == -1)
		return ENAMETOOLONG;
	if (np != NULL)
		*np = len + 1;	/* include null byte */
	return 0;
}

/*
 * int
 * kzero(void *dst, size_t cnt)
 *	Zero a kernel buffer with protection against kernel page fault errors.
 *
 * Calling/Exit State:
 *	A fault error in any pageable kernel address will cause a non-zero
 *	errno to be returned.
 */
int
kzero(void *dst, size_t cnt)
{
	CATCH_FAULTS(CATCH_KERNEL_FAULTS)
		bzero(dst, cnt);
	return END_CATCH();
}
