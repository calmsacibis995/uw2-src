/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/nullzero/nullzero.c	1.9"
#ident	"$Header: $"

#include <io/conf.h>
#include <io/poll.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/seg_dz.h>
#include <mem/seg_vn.h>
#include <proc/cred.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <io/ddi.h>

extern void map_addr(vaddr_t *, uint_t, off_t, int);
STATIC int nzrw(dev_t, struct uio *, struct cred *, enum uio_rw);

#define	M_NULL		0	/* /dev/null - EOF & Rathole */
#define	M_ZERO		1	/* /dev/zero - source of private memory */
#define M_DZERO		2	/* /dev/dzero - deferred zero mappings */

/* initialized security flags, public device, no Mac checks,
 * for data transfer and set MP flag.
 */

int nzdevflag = (D_MP | D_INITPUB | D_NOSPECMACDATA); 

/*
 * int nzopen(dev_t *devp, int flag, int type, struct cred *cr)
 *	Open routine for nullzero. /dev/null and /dev/zero behave the
 *	same regardless of how opened. So this function does nothing.
 * 
 * Calling/Exit State:
 *	Standard args to all driver open()/close() entry points.
 */

/* ARGSUSED */
int
nzopen(dev_t *devp, int flag, int type, struct cred *cr)
{
	if (getminor(*devp) > M_DZERO)
		return ENODEV;
	return 0;
}

/*
 * int nzclose(dev_t devp, int flag, struct cred *cr)
 *	Open routine for nullzero. /dev/null and /dev/zero behave the
 *	same regardless of how closed. So this function does nothing.
 * 
 * Calling/Exit State:
 *	Standard args to all driver open()/close() entry points.
 */

/* ARGSUSED */
int
nzclose(dev_t dev, int flag, struct cred *cr)
{
	return 0;
}

/*
 * int nzioctl(dev_t dev,int cmd, int arg, int flag, struct cred *cr,
 * 		int *rvalp)
 * 	There are no ioctl operations for dev/null , de/zero.
 *
 * Calling/Exit State:
 *	Standard arguments to all driver ioctl entry points.
 */

/* ARGSUSED */
int
nzioctl(dev_t dev, int cmd, int arg, int flag, struct cred *cr, int *rvalp)
{
	return ENODEV;
}

/*
 * int nzread(dev_t dev,  struct uio *uiop, struct cred *cr)
 * 	This function just calls nzrw function with UIO_READ
 *	argument.
 *
 * Calling/Exit State:
 *	Standard arguments to all driver read entry points.
 */

int
nzread(dev_t dev, struct uio *uiop, struct cred *cr)
{
	return (nzrw(dev, uiop, cr, UIO_READ));
}

/*
 * int nzwrite(dev_t dev,  struct uio *uiop, struct cred *cr)
 * 	This function just calls nzrw function with UIO_WRITE
 *	argument.
 *
 * Calling/Exit State:
 *	Standard arguments to all driver write entry points.
 */

int
nzwrite(dev_t dev, struct uio *uiop, struct cred *cr)
{
	return (nzrw(dev, uiop, cr, UIO_WRITE));
}

/*
 * When reading the M_ZERO device, we simply copyout the zeroes
 * array in NZEROES sized chunks to the user's address.
 *
 */
#define NZEROES		0x100
static char zeroes[NZEROES];

/*
 * int nzrw(dev_t dev,  struct uio *uiop, struct cred *cr,
 *		enum uio_rw rw)
 *	This function fills up the user buffer with zeroes if
 *	reading from /dev/zero or if writing to /dev/zero
 *	just advances the  uio offset. If reading from /dev/null
 *	it returns immediately or if writing to /dev/null just
 *	advances the uio offset.
 *
 * Calling/Exit State:
 * 	This function is called by nzread and nzwrite with
 *	UIO_WRITE or UIO_READ argument. 
 */
/*ARGSUSED*/
STATIC int
nzrw(dev_t dev, struct uio *uiop, struct cred *cr, enum uio_rw rw)
{
	register off_t n;
	int error = 0;

        while (error == 0 && uiop->uio_resid != 0) {
		/*
		 * Don't cross page boundaries.  uio_offset could be
		 * negative, so don't just take a simpler MIN.
		 */
                n = MIN(MIN(uiop->uio_resid, ptob(1)),
			ptob(1) - uiop->uio_offset % ptob(1));

		switch (getminor(dev)) {
		case M_DZERO:
		case M_ZERO:
			if (rw == UIO_READ) {
				n = MIN(n, sizeof (zeroes));
				if (uiomove(zeroes, n, rw, uiop))
					error = ENXIO;
				break;
			}
			/*FALLTHRU*/
		case M_NULL:
			if (rw == UIO_WRITE) {
				uiop->uio_offset += uiop->uio_resid;
				uiop->uio_resid = 0;
			}
			return 0;
		}
	}
	return (error);
}


/*
 * int nzsegmap(dev_t dev,  uint_t off, struct as *as, vaddr_t *addrp,
 *		uint_t len, uint_t prot, uint_t maxprot, uint_t flags,
 *		struct cred *cr)
 *	This function is called when /dev/zero or /dev/dzero is mmapped.
 *	It creates seg_vn segment for the address range requested 
 *	through as_map for dev/zero and creates a seg_dz segment for
 *	th address range for dev/dzero.
 *
 * Calling/Exit State:
 *	flags denotes if we have to use the user specified address and
 *	if mapping is MAP_FIXED or MAP_SHARED.
 * 	ENXIO is returned if trying to mmap /dev/null. 
 */
int
nzsegmap(dev_t dev, off_t off, struct as *as, vaddr_t *addrp, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, struct cred *cred)
{
	struct segvn_crargs vn_a;
	struct segdz_crargs dz_a;
	minor_t devnum;

	/* cannot map /dev/null */

	if ((devnum = getminor(dev)) == M_NULL)
		return ENXIO;

	if ((devnum == M_DZERO) && ((flags & MAP_TYPE) != MAP_PRIVATE))
		return EINVAL;

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * No need to worry about vac alignment since this
		 * is a "clone" object that doesn't yet exist.
		 */
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address -
		 * Blow away any previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	switch (devnum) {
	case M_ZERO:
		/*
		 * Use seg_vn segment driver for /dev/zero mapping.
		 * Passing in a NULL amp gives us the "cloning" effect.
		 */
		vn_a.vp = NULL;
		vn_a.offset = 0;
		vn_a.type = (flags & MAP_TYPE);
		vn_a.prot = (uchar_t)prot;
		vn_a.maxprot = (uchar_t)maxprot;
		vn_a.cred = cred;

		return (as_map(as, *addrp, len, segvn_create, &vn_a));
		break;

	case M_DZERO:
		/*
		 * Use seg_dz segment driver for /dev/dz mapping.
		 * Don't concatenate this segment with other dz segments.
		 * Need our own private stack segment.
		 */
		dz_a.flags = SEGDZ_NOCONCAT;
		dz_a.prot = (uchar_t)prot;
		return (as_map(as, *addrp, len, segdz_create, &dz_a));
		break;
	}
}

/*
 * int nzchpoll(dev_t dev, short events, int anyyet, short *reventsp,
 *		struct pollhead **phpp)
 *	This function is the entry point for the poll system call for
 *	dev/null and dev/zero. It returns immediately.
 *
 * Calling/Exit State:
 *	If the events type is POLLIN, POLLRDNORM or POLLOUT , reventsp
 *	is enabled that type. The poll head pointer is set to Null.
 *
 */
/*ARGSUSED*/
int
nzchpoll(dev_t dev, short events, int anyyet, short *reventsp,
		struct pollhead **phpp)
{
	*reventsp = 0;
	if (events & POLLIN)
		*reventsp |= POLLIN;
	if (events & POLLRDNORM)
		*reventsp |= POLLRDNORM;
	if (events & POLLOUT)
		*reventsp |= POLLOUT;
	*phpp = (struct pollhead *) NULL;
	return (0);
}
