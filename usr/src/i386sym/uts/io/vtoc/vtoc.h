/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SYS_VTOC_H	/* wrapper symbol for kernel use */
#define	_SYS_VTOC_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/vtoc/vtoc.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * VTOC driver data structures and definitions.
 */

#define V_NUMPAR 		255		/* max number of partitions */
#define V_VTOCSEC		16		/* where VTOC starts */
#define	V_SIZE			8192		/* VTOC size */
#define VTYPCHR			14		/* max length of type name */

/*
 * structure of a single partition description in the VTOC
 */
struct partition	{
	daddr_t p_start;	/* start sector no of partition*/
	long	p_size;		/* # of blocks in partition*/
	int	p_type;		/* type of this partition */
	short	p_bsize;	/* block size in bytes */
	short	p_fsize;	/* frag size in bytes */
};

/* Partition types */
#define V_NOPART		0x00		/* empty slot */
#define	V_RAW			0x01		/* regular partition */
#define	V_BOOT			0x02		/* bootstrap */
#define V_RESERVED		0x03		/* reserved disk area */
#define	V_FW			0x04		/* firware */
#define V_DIAG			0x05		/* SCAN dumps */

/*
 * on-disk VTOC
 */
struct vtoc {
	ulong	v_sanity;	/* magic number */
	ushort	v_version;
	ulong	v_cksum;	/* checksum after v_cksum has been zeroed */
	long	v_size;		/* # of bytes in this vtoc */
	ushort	v_nparts;	/* number of partitions */
	int	v_secsize;	/* sector size in bytes */
	int	v_ntracks;	/* # tracks/cylinder */
	int	v_nsectors;	/* # sectors/track */
	int	v_ncylinders;	/* # cylinders */
	int	v_rpm;		/* revolutions/minute */
	int	v_capacity;	/* # of sectors per disk */
	int	v_nseccyl;	/* # sectors/cylinder */
	long	v_reserved[8];
	char	v_disktype[VTYPCHR]; /* type of disk this is */
	struct partition v_part[V_NUMPAR];
};

#define VTOC_SANE		0x061160	/* Indicates a sane VTOC */
#define V_VERSION_1		0x01		/* layout version number */

/* vtoc driver ioctl() commands */
#define VIOC			('V'<<8)
#define V_READ			(VIOC|1)	/* Read VTOC */
#define V_WRITE			(VIOC|2)	/* Write VTOC */
#define V_BUILD			(VIOC|3)	/* Build new device */
#define V_DESTROY		(VIOC|4)	/* Destroy device */
#define V_WRITEPROTECT		(VIOC|5)	/* Write protect device */
#define V_WRITEENABLE		(VIOC|6)	/* Write enable device */
#ifdef DEBUG
#define V_IOCTL			(VIOC|255) 	/* Pass ioctl to physdev */
#endif /* DEBUG */

/* Do partition structs a and b overlap? */

#define V_OVLAP(A, B)	   	((A)->p_start < (B)->p_start + (B)->p_size && \
				 (B)->p_start < (A)->p_start + (A)->p_size)

/*
 * Special minor numbers for disks
 *
 * Disks devices use the following semantics for their minor numbers.
 * V_RAW_MINOR is the low byte of the minor number of the unpartitioned
 * disk device.  Thus, or-ing V_RAW_MINOR into a dev_t will result in
 * the dev_t of the unpartitioned device.  At probe time, disk drivers
 * should generate one dev per drive which has been configured, with
 * a minor number equal to the drive number left shifted 8 bits and
 * or-ed with V_RAW_MINOR.
 */

#define V_RAW_MINOR	0x00FF

#define V_PART_MASK	0x00FF	/* part of devt which encodes partition */
#define V_UNIT_MASK	0xFF00	/* part of devt which encodes unit number */

#define VPART(dev)	(minor(dev) & V_PART_MASK)
#define VUNIT(dev)	(minor(dev) >> 8)

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * driver-specific part of a vtoc dev.  We share the same structure
 * between "normal" vtoc devs and the diagnostic vtoc dev, to make
 * memory allocation easier.  The driver_id should be first for
 * conformance to other partitioning drivers.
 */
struct vtoc_private {
	int		vp_driver_id;	/* major number of vtoc driver */
	daddr_t		vp_start;	/* start of physical partition */
	long		vp_size;	/* size of this device */
	struct	dev_handle *vp_physdev;	/* handle on physical driver */
	struct	devops	*vp_devops;	/* copy of devops used by other devs */
	int		vp_flags;	/* see below */
};

/* vp_flags */

#define	VP_WRPROTECT		0x01		/* No raw writes allowed */

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _SYS_VTOC_H */
