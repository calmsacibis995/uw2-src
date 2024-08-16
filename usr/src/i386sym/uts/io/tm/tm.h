/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_TM_H	/* wrapper symbol for kernel use */
#define	_IO_TM_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/tm/tm.h	1.2"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * tm.h
 *	SSM/Streamer tape driver structure definitions.
 */

#ifdef _KERNEL_HEADERS

#include <io/scsi.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/scsi.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/* 
 * Standard Tape ioctl commands.  
 * XXX - these should come from <sys/tape.h> once
 * 	 its delivered.
 */
#define T_BASE		('[' << 8)
#define T_RETENSION	(T_BASE | 001)	/* Retension the medium		*/
#define T_RWD		(T_BASE | 002)	/* Rewind medium 		*/
#define T_ERASE		(T_BASE | 003)	/* Erase entire medium		*/
#define T_WRFILEM	(T_BASE | 004)	/* Write filemarks		*/
#define T_SFF		(T_BASE | 007)	/* Space forward filemarks 	*/
#define T_SBF		(T_BASE | 010)	/* Space forward blocks 	*/
#define T_LOAD		(T_BASE | 011)	/* Load medium, position to BOT */
#define T_UNLOAD	(T_BASE | 012)	/* Unload medium		*/
#define	T_RDBLKLEN	(T_BASE | 016)	/* Read block size		*/
#define	T_WRBLKLEN	(T_BASE | 017)	/* Set block size		*/
#define	T_PREVMV	(T_BASE | 020)	/* Prevent media removal	*/
#define	T_ALLOMV	(T_BASE | 021)	/* Allow media removal		*/
#define T_SFB		(T_BASE | 022)	/* Space backward filemarks 	*/
#define T_SBB 		(T_BASE | 023)	/* Space backward blocks 	*/
#define T_EOD		(T_BASE | 024)	/* Space to end of recorded data */
#define T_STD		(T_BASE | 030)	/* Set recording density 	*/
#define	T_VERBOSE	(T_BASE | 031)	/* tm-driver verbosity		*/
#define	T_RST		(T_BASE | 032)	/* Reset the device		*/

        /* Settable tm-driver verbosity flags */
#define TMF_PRSENSE		0x10	/* Report all SCSI request-sense data */
#define TMF_RSENSE		0x20	/* Report sense data for READs */
#define TMF_WSENSE		0x40	/* Report sense data for WRITEs */
#define TMF_SSENSE		0x80	/* Report sense data for repositions */

/* Block length limit data for T_RDBLKLEN and T_WRBLKLEN */
struct blklen {
        unsigned res1:8;           /* Reserved                  */
        unsigned max_blen:24;      /* maximum block length      */
        unsigned min_blen:16;      /* minimum block length      */
};

/*
 * The following defines the format of a minor device 
 * number for a tape driver unit consisting of a mode 
 * byte and a unit number byte.  
 * 
 * The mode byte contains flags for selecting driver
 * modes, such as no-rewind-on-close, and a density
 * selector field.  Each drive class (i.e. 1/4" 
 * vs. 1/2") may define its own densities.  Drivers
 * may support only a subset of these modes.
 * 
 * For efficient access and flexibility a byte is 
 * reserved for the unit number.  Each driver shall
 * determine the maximum number of units it supports.
 */
#define MT_MINOR(unit,flags,density) ((unit) << 8 | (flags) | (density)) 
					/* Build a tape device minor number */

#define MT_UNIT(x) ((x) >> 8 & 0xff)	/* Extract unit number from minor */
#define MT_FLAGS(x) ((x) & 0xf0)	/* Extract mode flags from minor */
#define MT_DENSITY(x) ((x) & 0x0f)	/* Extract mode density from minor */

/* QIC density selectors */
#define MTD_NC		0x0		/* No change - use last setting */
#define MTD_Q11		0x1		/* Use QIC-11 */
#define MTD_Q24		0x2		/* Use QIC-24 */
#define MTD_Q120	0x3		/* Use QIC-120 */
#define MTD_Q150	0x4		/* Use QIC-150 */
#define MTD_Q525	0x5		/* Use QIC-525 */
#define MTD_DEF		0x6		/* Use the drive standard default */

/* Mode flags */
#define MTF_AUTORET	0x40		/* Retension medium upon 1st access */
#define MTF_NOREWFLAG	0x80		/* Suppress rewind on close */

#define MT_RETEN(dev)	((minor(dev) & MTF_AUTORET) != 0) 
					/* Determine from the device's device
					 * number if it should be retensioned 
					 * upon first access.  */

#define MT_REWIND(dev)	((minor(dev) & MTF_NOREWFLAG) == 0) 
					/* Determine from the device's device
					 * number if it should be rewound 
					 * when closed.  */

#if defined(_KERNEL) || defined(_KMEMUSER)
/*
 * Structure for grouping global xs-driver data
 * together.  Consists of binary reconfiguration
 * parameters via the driver's Space.c file.
 */
typedef const struct tm_bin_conf {
	major_t	c_major;		/* Major device # for tm devices */
	uint 	bin;			/* SLIC bin for tm-interrupts */
} tm_bconf_t;

extern tm_bconf_t tm_global;            /* Initialized in tm-driver's Space.c */

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_TM_H */
