/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_BOOTINFO_H	/* wrapper symbol for kernel use */
#define _SVC_BOOTINFO_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:svc/bootinfo.h	1.25"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <util/param.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *	Definition of bootinfo structure.  This is used to pass
 *	information between the bootstrap and the kernel.
 */

#define BKI_MAGIC	0xff1234ff

#define B_MAXARGS	15		/* max. number of boot args */
#define B_STRSIZ	100		/* max length of boot arg. string */
#define B_PATHSIZ	30		/* max length of file path string */
#define B_MAXMEMR	5		/* max memrng entry count */
#define B_MAXMEMA	15		/* max memavail entry count */
#define B_MAXMEMU	15		/* max memused entry count */

#define NBT_LOADABLES	9		/* number loadable binary files */
#define SBF_DIRSIZ	8

struct unusedmem {
	paddr_t		base;
	long		extent;
	ushort_t	flags;
	struct unusedmem  *next;
};

struct bootmem {
	paddr_t		base;
	long		extent;
	ushort_t	flags;
};
	
struct krdata {
	paddr_t	paddr;
	vaddr_t	vaddr;
	long	type;
	size_t	size;
};

struct bootinfo {
	ushort_t	bootflags;	/* miscellaneous flags */
	ushort_t	machflags;	/* machine-type flags */

	struct hdparams { 		/* hard disk parameters */
		ushort	hdp_ncyl;	/* # cylinders (0 = no disk) */
		unchar	hdp_nhead;	/* # heads */
		unchar	hdp_nsect;	/* # sectors per track */
		ushort	hdp_precomp;	/* write precomp cyl */
		ushort	hdp_lz;		/* landing zone */
	} hdparams[2];			/* hard disk parameters */

	int	memavailcnt;
	struct	bootmem	memavail[B_MAXMEMA];

	int	memusedcnt;
	struct	bootmem	memused[B_MAXMEMU];

	int	bargc;			    /* count of boot arguments */
	char	bargv[B_MAXARGS][B_STRSIZ]; /* argument strings */

	char	id[5];			/* Contents of F000:E000 */

	char	pad1[1];

	ushort_t	dma_offset;	/* # megabytes to add to
					 *   tune.t_dmalimit and t_dmabase
					 */
	struct krdata	kd[3];		/* loaded raw data descriptors */

};

/* flags for struct mem flags */

#define B_MEM_NODMA	0x01
#define B_MEM_KTEXT	0x02
#define B_MEM_KDATA	0x04
#define B_MEM_KRDATA	0x08
#define B_MEM_MEMFSROOT	0x10
#define B_MEM_BASE	0x100
#define B_MEM_EXPANS	0x200
#define B_MEM_SHADOW	0x400
#define B_MEM_TREV	0x1000	/* Test Memory in high to low order	*/
#define B_MEM_FORCE	0x2000	/* Ignore CMOS and continue probing memory */
#define B_MEM_SKIPCHK 	0x4000	/* use CMOS extent for memory range */
#define B_MEM_BOOTSTRAP	0x8000	/* Used internally by bootstrap */

/* Types for loadable kernel raw data files */
#define MEMFSROOT_META	0
#define MEMFSROOT_FS	1
#define RM_DATABASE	2

/* Flag definitions for bootflags */

#define BF_FLOPPY	0x01		/* booted from floppy */

/* Flag definitions for machflags */

#define MEM_CACHE	0x01	/* This machine has cache memory */
#define CPQ_CACHE	0x02	/* Uses Compaq style cache workaround to
				   support dual port memory on I/O boards,
				   no caching above 0x80000000 */
#define MEM_SHADOW	0x04	/* Machine supports shadow RAM */
#define MC_BUS		0x10	/* Machine has micro-channel bus */
#define AT_BUS		0x20	/* Machine has ISA bus */
#define EISA_IO_BUS	0x40	/* Machine has EISA bus */

#define BOOTINFO_LOC	((paddr_t)0x1000)
#define STARTSTK	((paddr_t)0x1000)

#ifdef _KERNEL

extern struct bootinfo bootinfo;
extern struct unusedmem *memNOTused;
#ifndef NO_RDMA
extern struct unusedmem *memNOTusedNDMA;
#endif /* NO_RDMA */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_BOOTINFO_H */
