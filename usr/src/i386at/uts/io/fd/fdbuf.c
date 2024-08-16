/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/fd/fdbuf.c	1.7"
#ident	"$Header: $"

#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
#include <fs/buf.h>
#include <util/sysmacros.h>
#include <svc/errno.h>

#include <util/cmn_err.h>
#include <io/dma.h>
#include <io/i8237A.h>
#include <io/cram/cram.h>
#include <io/fd/fd.h>

#include <io/ddi.h>
#include <io/ddi_i386at.h>

void fdbufinit(void);
int fdbufgrow(unsigned , int );
extern	void	outb(int, uchar_t);

struct  fdbufstruct     fdbufstruct;
struct	fdstate		fd[NUMDRV];

int	fdloaded = 0;

bcb_t *fdbcb;

/*
 * void
 * fdbufinit(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
fdbufinit(void)
{
	long	bus_type;

        /*
         * If fdloaded is 1 it means that fdinit() has initialized
         * the device, so don't reset the controller.
         */
        if(!fdloaded)   {
		drv_gethardware( IOBUS_TYPE, &bus_type );

		if ( bus_type == BUS_MCA ) {
			outb(FDCTRL, ENAB_MCA_INT);
		}
		else {
			outb(FDCTRL, ENABINT);
		}
	}

	fdbcb = bcb_alloc(KM_NOSLEEP);
	fdbcb->bcb_addrtypes = BA_KVIRT;
	fdbcb->bcb_max_xfer = ptob(1);
	fdbcb->bcb_granularity = NBPSCTR;
	fdbcb->bcb_physreqp = physreq_alloc(KM_NOSLEEP);
	if (fdbcb->bcb_physreqp == NULL)
		return;
	dma_physreq(DMA_CH2, DMA_PATH_8, fdbcb->bcb_physreqp);
	if (!physreq_prep(fdbcb->bcb_physreqp, KM_NOSLEEP)) {
		physreq_free(fdbcb->bcb_physreqp);
		return;
	}

	fdbufgrow(FDMEMSIZE, KM_NOSLEEP);
}

/*
 * int
 * fdbufgrow(unsigned bytesize, int sleepflag)
 *
 * Calling/Exit State:
 *	None.
 */
int
fdbufgrow(unsigned bytesize, int sleepflag)
{
	/* Allocate a buffer to hold  bytesize  bytes  without */
	/* crossing a dma boundary.                                  */

	/* We can't handle more than 64k, since this has to cross  a */
	/* dma boundary.                                             */
	if ( bytesize > 0x10000 || bytesize == 0 )
	    return (EINVAL);

	/* We already have enough mem. */
	if (bytesize <= fdbufstruct.fbs_size)
	    return (0);

	if ( (int)fdbufstruct.fbs_size > 0 ) {
	    kmem_free(fdbufstruct.fbs_addr, fdbufstruct.fbs_size);
	    fdbufstruct.fbs_size = 0;
	    fdbufstruct.fbs_addr = (caddr_t)NULL;
	}

	/* Get the contiguous pages */
	fdbufstruct.fbs_addr = kmem_alloc_physreq(bytesize,
						  fdbcb->bcb_physreqp,
						  sleepflag);
	if (fdbufstruct.fbs_addr == NULL)
	    return (EAGAIN);
	fdbufstruct.fbs_size = bytesize;
	return (0);
}
