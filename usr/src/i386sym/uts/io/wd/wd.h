/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_WD_H	/* wrapper symbol for kernel use */
#define	_IO_WD_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/wd/wd.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * wd.h
 *      SCSI disk driver for SSM definitions
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <proc/cred.h>	/* REQUIRED for external interface declarations */
#include <io/uio.h>	/* REQUIRED for external interface declarations */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <sys/cred.h>	/* REQUIRED for external interface declarations */
#include <sys/uio.h>	/* REQUIRED for external interface declarations */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

/*
 * The following are used by the interrupt handler to
 * compute which disk unit and which of its CBs are
 * referred to by the interrupt vector argument.  Basically:
 *	unit = (vector - wd_base_vector) >> WD_LEVEL_SHIFT;
 *	CB# for that unit = (vector - wd_base_vector) & WD_LEVEL_CB;
 */
#define WD_LEVEL_SHIFT	0x01		/* Divide by #cb's per device to 
					 * get units index into wd_info */
#define WD_LEVEL_CB	0x01		/* Mask for unit's CB index */

/*
 * The following structure is used to note the logical 
 * partitioning of the physical disk into its minor devices.  
 * When p_start < 0, the slice is not available for use.
 */
struct	partt {
	daddr_t	p_start;		/* Slice's start sector on disk */
	long	p_size;			/* # of sectors in this slice */
	int	p_flags;		/* partition open state */
	int	p_nopen;		/* # active open()s for which there
					 * will be a corresponding close(). */
} ;
/*
 * Information structure for a SCSI disk on a SSM. There is one 
 * per physcal device unit.  Each usable unit has two message
 * blocks (CBs) for communicating requests with the SSM SCSI.
 * Each CB is assigned an in-core table (iovects) for mapping 
 * out scatter-gather, DMA transfers for the SSM to perform.
 */
typedef struct {
	int	wd_alive;		/* Is device alive? */ 
	int	wd_nopen;		/* # active open()s for which there
					 * will be a corresponding close().
					 * Corresponds to # of subunit closes
					 * that are expected. */
	struct	cb_desc *wd_cbdp;	/* Description command blocks I own */
	ushort	wd_iovcount;		/* Number of iovects per cb */
	int	wd_flags;		/* Device state flags */
	unchar	wd_devno;		/* Logical SCSI_DEVNO(targ, lun) */
	unchar	wd_ctlr;		/* SSM controller number */
	int	wd_inuse;		/* # of cbs currently in use */
	daddr_t wd_capacity;		/* # usable sectors on physical disk */
	buf_t	*wd_bufh;		/* Pending I/O request queue header;
					 * sorted for seek optimization */
	lock_t 	*wd_lock;		/* Mutex access to this structure */
	sleep_t *wd_sleep;		/* Mutex open(), close(), and size() */
	struct	partt wd_p[V_NUMPAR];	/* Unit's partition layout */
	void	*wd_stats;		/* Pointer to disk stats */
} wd_t;

/*
 * Defines for info struct state flags
 */
#define WD_IS_ALIVE	0x01		/* Board passed probing */
#define WD_EXCLUSIVE	0x02		/* Disk in exclusive access mode */
#define WD_RAW		0x04		/* Unpartitioned disk access */

/*
 * The following flags are used for tracking
 * opens for which there will be a corresponding
 * close.  There is only one close for all opens
 * opens to a minor device's character interface.  
 * Ditto for its block interface.  Although there
 * is one for each layered driver open call.
 *
 * Track these flags and the total number of opens
 * for which there will be a close on a per minor
 * device basis.  
 */
#define WD_CHR		0x08		/* Opened via the BLOCK interface */
#define WD_BLK		0x10		/* Opened via the CHR interface */

/*
 * SCSI disk sizes
 */
#define WD_MAX_XFER		256	/* SCSI xfer size limit, in sectors */

/* public function declarations */
void	wdinit(void);
int	wdopen(dev_t *, int, int, cred_t *);
int	wdclose(dev_t, int, int, cred_t *);
int	wdread(dev_t, uio_t *, cred_t *);
int	wdwrite(dev_t, uio_t *, cred_t *);
void    wdprint(dev_t, char *);
int	wdstrategy(buf_t *);
void	wdintr(int);
int	wdsize(dev_t);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_WD_H */
