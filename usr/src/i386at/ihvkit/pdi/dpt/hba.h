/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:pdi/dpt/hba.h	1.2"
#ifndef _IO_HBA_HBA_H		/* wrapper symbol for kernel use */
#define _IO_HBA_HBA_H		/* subject to change without notice */

#ident	"@(#)ihvkit:pdi/dpt/hba.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* 
 * hba.h
 *
 * Description:
 *	This file contains defines useful for maintaining source
 *	compatibility for ongoing releases of PDI.  The MACROS 
 *	provided here hide the differences made to the PDI interface 
 *	and differences between a multi-threaded vs. 
 *	non-multithreading driver.
 *
 *	The use of these MACROS and version defines, make it possible
 *	to keep a single copy of the source for a SVR4.2/UnixWare driver 
 *	and a SVR4.2MP driver.
 *
 * How to use hba.h:
 *	To use hba.h, define HBA_PREFIX in your driver.h file:
 *	For example:
 *
 *		#define HBA_PREFIX dpt
 *
 *	and order the include files in the driver as follows:
 *
 *		#include	all other includes
 *		#include	your_driver.h
 *		#include	<sys/hba.h>
 *		#include	<sys/ddi.h>
 *		#include	<sys/ddi_386at.h>  (optional)
 *
 * The following block of comments further describe the version
 * definitions, and extentions/enhancements made to PDI, making
 * it necessary for these MACROS.
 *
 *------------------------------------
 *
 *  1) Source Compatibiliity
 *	The version differences summarized below affect all SVR4.2 and
 *	UnixWare 1.1 driver source moving to SVR4.2MP.  It is recommended 
 *	that a single shared source of the driver be used for all
 *	releases, with appropriate #if PDI_VERSION directives applied.
 *
 *	The following MACROS hide the sleepflag differences:
 *		HBACLOSE, HBAFREEBLK, HBAGETBLK, HBAGETINFO, HBAICMD,
 *		HBAIOCTL, HBAOPEN, HBASEND, HBAXLAT
 *
 *		SDI_GET, SDI_GETBLK, SDI_TRANSLATE
 *
 *	The following MACROS hide the hbaxlat function declaration
 *	differences:
 *		HBAXLAT_DECL, HBAXLAT_RETURN
 *
 *	There are various other defines/MACROS that help hide the
 *	difference between a multithreaded driver and non-multithreaded
 *	driver.
 *
 *  2) PDI_VERSION is a #define made available to HBA drivers through sdi.h
 *	that should be used to conditionally compile code for extending 
 *	driver capability in ongoing releases.  The preferred usage is,
 *	for example:
 *
 *		#if PDI_VERSION >= PDI_SVR42MP
 *
 *	which means SVR4.2MP or later (where PDI_SVR42MP is defined to be 3).
 *
 *	The value of PDI_VERSION is:
 *		1	SVR4.2/UnixWare 1.1 Driver
 *		2	SVR4.2 DTMT (Maintainence Release) Driver
 *		3	SVR4.2MP Driver
 *
 *	A PDI_VERSION 1 driver conforms to the HBA interface 
 *	defined in the SVR4.2 Portable Driver Interface
 *	(PDI) Guide.  A PDI_VERSION 3 driver conforms to 
 *	the SVR4.2MP PDI Guide, which has the following 
 *	extentions to the HBA interface:
 *
 *	- sleep flag argument extension to certain HBA and SDI
 *	  interface routines. (More on this below.)
 *	- breakup control block extension added to hbagetinfo.
 *	- DMAable memory allocated with kmem_alloc_physcontig
 *
 *	NOTE: Since SVR4.2 DTMT was not an official release, the
 *	      differences associated with this version is not included.
 *
 *  3) Sleep flag
 *	The SDI and HBA interfaces were extended in SVR4.2MP to
 *	use a flag argument for those routines that may need to
 *	sleep.  Since certain HBA/SDI routines need to be called at
 *	interrupt level, a sleep flag was necessary to indicate
 *	whether a sleep could be done.  
 *	The HBA routines that have the additional flag argument
 *	include: getblk(), icmd(), send(), and xlat().
 *	The SDI routines that have the additional flag argument
 *	include: sdi_get(), sdi_getblk(), sdi_icmd(), sdi_send,
 *	and sdi_translate().
 *  
 *  4) Breakup 
 *	The SVR4.2MP DDI/DKI has a new improved breakup interface,
 *	buf_breakup().  A PDI_VERSION 3 driver provides the target
 *	driver with the breakup parameters through the SVR4.2MP
 *	extention defined for the HBA getinfo routine.
 *	Pass-thru has also to be made dual level to use buf_breakup()
 *	with the driver's taylor made breakup parameters.
 *
 *	The breakup code differences should be ifdef:
 *
 *		#if (PDI_VERSION >= PDI_SVR42MP)
 *			... new breakup code ...
 *		#else /* !(PDI_VERSION >= PDI_SVR42MP) */ /*
 *			... old breakup code ...
 *		#endif /* !(PDI_VERSION >= PDI_SVR42MP) */ /*
 *
 *  5) Memory allocation
 *	
 *	HBA drivers may need to allocate physically contiguous DMAable
 *	memory.  Past versions of the DDI/DKI were not explicit
 *	about how drivers were to get DMAable memory.  The lack
 *	of documentation and the lack of a DDI/DKI interface for
 *	allocating physically contiguous DMAable memory lead driver
 *	writers to the conclusion that kmem_alloc was the correct
 *	interface.
 *	SVR4.0, SVR4.2, and UnixWare 1.1 kmem_alloc'ed memory is
 *	in fact DMAable, but only guaranteed to be physically
 *	contiguous on allocations of 1 page or less.  This worked
 *	not by design but because of the way KMA was implemented.
 *
 *	SVR4.2MP kmem_alloc'ed memory is not guaranteed to be either
 *	DMAable or physically contiguous.  This is by design.  A
 *	new DDI routine, kmem_alloc_physcontig, is provided for
 *	drivers that need DMAable memory.
 *	
 *	Drivers that currently use kmem_alloc() to allocate DMA memory
 *	must convert to use kmem_alloc_physcontig() for SVR4.2MP.
 *	HBA drivers that are DDI compliant are binary compatible 
 *	with SVR4.2MP, and use kmem_alloc() for DMA memory will 
 *	still work with the PHYSTOKVMEM tunable set to 1 
 *	(See description below.)
 *
 */
/*
 * PDI_VERSION is defined as follows for the versions:
 *	UnixWare 1.1	1
 *	SVR4.2 DTMT	undefined
 *	SVR4.2 MP	3
 * The following defines PDI_VERSION for DTMT, and causes an
 * error for all others.
 */
#ifndef PDI_VERSION
#ifdef PDI_SVR42
#define PDI_VERSION	2
#else	/* !PDI_SVR42 */
#error Something other than UnixWare 1.1, SVR4.2 DTMT, and SVR4.2 MP
#endif /* PDI_SVR42 */
#endif /* !PDI_VERSION */

/*
 * HBA_IDATA()
 * Macro to be called in hbastart routine (prior to call to
 * sdi_gethbano() and sdi_register()). This Macro insures that
 * the correct version number is initialized in the idata structure.
 */
#define HBA_IDATA(X) \
	{ \
	int i; \
	for(i=0; i<X; i++) \
		glue(HBA_PREFIX, idata)[i].version_num = PDI_VERSION; \
	}
			
/*
 * Useful defines for comparison with PDI_VERSION
 */
#define PDI_UNIXWARE11	1
#define PDI_SVR42_DTMT	2
#define PDI_SVR42MP	3

/*
 * Includes for the following MACROS
 */
#ifdef _KERNEL_HEADERS

#include <fs/buf.h>			/* REQUIRED */
#include <io/target/scsi.h>		/* REQUIRED */
#include <proc/cred.h>			/* REQUIRED */
#include <proc/proc.h>			/* REQUIRED */
#if (PDI_VERSION >= PDI_SVR42MP)
#include <util/engine.h>		/* REQUIRED */
#include <util/ksynch.h>		/* REQUIRED */
#endif	/* (PDI_VERSION >= PDI_SVR42MP) */

#elif defined(_KERNEL) || defined(_KMEMUSER)
#include <sys/buf.h>			/* REQUIRED */
#include <sys/scsi.h>			/* REQUIRED */
#include <sys/cred.h>			/* REQUIRED */
#include <sys/proc.h>			/* REQUIRED */
#if (PDI_VERSION >= PDI_SVR42MP)
#include <sys/engine.h>			/* REQUIRED */
#include <sys/ksynch.h>			/* REQUIRED */
#endif	/* (PDI_VERSION >= PDI_SVR42MP) */

#endif /* _KERNEL || _KMEMUSER */

/*
 *
 * Drivers that are multi-threaded, but compiled for UnixWare 1.1
 * continue to work with the following MACROS. 
 * (These are already defined in ddi.h for >= SVR4.2MP)
 * (These are already defined in sdi_comm.h for == SVR4.2 DTMT)
 *
 */
#if (PDI_VERSION < PDI_SVR42_DTMT)

#define	LKINFO_DECL(l,s,f)		int l
#define	LOCK_ALLOC(h,i,p,f)		((lock_t *)1)
#define	LOCK(lockp, ipl)		spl5s()
#define	UNLOCK(lockp, ipl)		splx(ipl)

#define	_SDI_LOCKED	0x1
#define	_SDI_SLEEPING	0x2
#define	SLEEP_ALLOC(h,i,f)		(int *)kmem_zalloc(sizeof(int), f)
#define	SLEEP_LOCK(lockp, pri)		{ \
				while ( *(lockp) & _SDI_LOCKED ) { \
					*(lockp) |= _SDI_SLEEPING; \
					sleep(lockp, pri); \
				} \
				*(lockp) |= _SDI_LOCKED; }

#define	SLEEP_UNLOCK(lockp)		{ \
				*(lockp) &= ~_SDI_LOCKED; \
				if (*(lockp) & _SDI_SLEEPING) { \
					*(lockp) &= ~_SDI_SLEEPING; \
					wakeup(lockp); \
				} }

#define	RW_WRLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	RW_RDLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	RW_UNLOCK(lockp, ipl)		UNLOCK(lockp, ipl)
#define	TRYLOCK(lockp, ipl)		LOCK(lockp, ipl)
#define	SV_ALLOC(f)			(int *)kmem_zalloc(1, f)

#define	SV_WAIT(svp, pri, lockp)	{ (void)spl0(); (void)sleep(svp, pri); }
#define	SV_BROADCAST(svp, flags)	wakeup(svp)

#define lock_t		int
#define rwlock_t	int
#define sleep_t		int
#define sv_t		int
#define	pl_t		int
#define bcb_t		char
#define	spldisk()	spl5()

#ifndef PLDISK
#define	PLDISK	5
#endif

#define	ITIMEOUT(f, a, t, p)	timeout(f, a, t)

#define F_DMA_24	F_DMA		/* Device supports 24-bit DMA */
#define F_DMA_32	0x010		/* Device supports 32-bit DMA */

#endif	/* (PDI_VERSION < PDI_SVR42MP) */

#if (PDI_VERSION <= PDI_SVR42MP)
/*
 * Additional SCSI defines/structures that should be part of scsi.h
 * (These are defined in scsi.h for > SVR4.2MP)
 */
#define	SS_RDBLKLEN	0X05		/* Read block length limits	   */

/*
 * Block length limit data
 */
struct blklen {
	unsigned res1:8;	   /* Reserved			*/
	unsigned max_blen:24;	   /* maximum block length	*/
	unsigned min_blen:16;	   /* minimum block length	*/
};

#define RDBLKLEN_SZ       6 
#define RDBLKLEN_AD(x)     ((char *)(x))

/*
 * Mode data structure
 */
struct mode {
	unsigned md_len   :8;		/* Sense data length		*/
	unsigned md_media :8;		/* Medium type			*/
	unsigned md_speed :4;		/* Tape speed			*/
	unsigned md_bm    :3;		/* Buffering mode		*/
	unsigned md_wp    :1;		/* Write protected		*/
	unsigned md_bdl   :8;		/* Block descriptor length	*/
	unsigned md_dens  :8;		/* Density code			*/
	unsigned md_nblks :24;		/* Number of blocks		*/
	unsigned md_res   :8;		/* Reserved field		*/
	unsigned md_bsize :24;		/* Block size			*/
};
#endif /* (PDI_VERSION <= PDI_SVR42MP) */

#if (PDI_VERSION >= PDI_SVR42MP)
#define	ITIMEOUT(f, a, t, p) itimeout(f, a, t, p)
#endif	/* (PDI_VERSION >= PDI_SVR42MP) */

/*
 * MACROS for concatenation to generate 'HBA_PREFIX' func name
 * As an example, here's what's going on below:
 *	GETBLK -> glue(PREFIX, getblk) -> xglue(PREFIX, getblk) -> dptgetblk
 */
#define xglue(a, b) a ## b
#define glue(a,b) xglue (a, b)

/*
 *
 * HBA interface MACROS
 *
 * The following MACROS are used for the HBA interface routines
 * When expanded, they provide the correct interface for the
 * given PDI_VERSION.
 * Extentions that were added to SVR4.2MP which are accounted for
 * in these MACROS include:
 *	1) Several routines have one additional argument, sleepflag.
 *	2) xlat has a return value, and the declaration changed from 
 *		void to int
 *
 * These MACROS require that the following define:
 *
 *	#define HBA_PREFIX your_driver_prefix
 *
 * be put in your_driver.h header file, and that the order of
 * include files be:
 *
 *	#include	all other includes
 *	#include	your_driver.h
 *	#include	<sys/hba.h>
 *	#include	<sys/ddi.h>
 *
 */

#if (PDI_VERSION <= PDI_UNIXWARE11)

#define HBACLOSE(dev, flags, otype, credp) glue(HBA_PREFIX, close)(dev, flags, otype, credp)
#define HBAFREEBLK(hbap) glue(HBA_PREFIX, freeblk)(hbap)
#define HBAGETBLK(sleepflag) glue(HBA_PREFIX, getblk)()
#define HBAGETINFO(addr, getinfop) glue(HBA_PREFIX, getinfo)(addr, getinfop)
#define HBAICMD(hbap, sleepflag) glue(HBA_PREFIX, icmd)(hbap)
#define HBAIOCTL(dev, cmd, arg, mode, credp, rvalp) glue(HBA_PREFIX, ioctl)(dev, cmd, arg, mode, credp, rvalp)
#define HBAOPEN(dev, flags, otype, credp) glue(HBA_PREFIX, open)(dev, flags, otype, credp)
#define HBASEND(hbap, sleepflag) glue(HBA_PREFIX, send)(hbap)
#define HBAXLAT(hbap, flag, procp, sleepflag) glue(HBA_PREFIX, xlat)(hbap, flag, procp)

#define HBAXLAT_DECL void
#define HBAXLAT_RETURN(x) return

#else /* (PDI_VERSION > PDI_UNIXWARE11) */

#define HBACLOSE(dev, flags, otype, credp) glue(HBA_PREFIX, close)(dev, flags, otype, credp)
#define HBAFREEBLK(hbap) glue(HBA_PREFIX, freeblk)(hbap)
#define HBAGETBLK(sleepflag) glue(HBA_PREFIX, getblk)(sleepflag)
#define HBAGETINFO(addr, getinfop) glue(HBA_PREFIX, getinfo)(addr, getinfop)
#define HBAICMD(hbap, sleepflag) glue(HBA_PREFIX, icmd)(hbap, sleepflag)
#define HBAIOCTL(dev, cmd, arg, mode, credp, rvalp) glue(HBA_PREFIX, ioctl)(dev, cmd, arg, mode, credp, rvalp)
#define HBAOPEN(dev, flags, otype, credp) glue(HBA_PREFIX, open)(dev, flags, otype, credp)
#define HBASEND(hbap, sleepflag) glue(HBA_PREFIX, send)(hbap, sleepflag)
#define HBAXLAT(hbap, flag, procp, sleepflag) glue(HBA_PREFIX, xlat)(hbap, flag, procp, sleepflag)

#define HBAXLAT_DECL int
#define HBAXLAT_RETURN(x) return (x)

#endif /* (PDI_VERSION > PDI_UNIXWARE11) */

/*
 *
 * SDI interface MACROS
 *
 * The following MACROS are used for the SDI interface routines
 * When expanded, they provide the correct interface for the
 * given PDI_VERSION.  The interface for SVR4.2MP include
 * a sleepflag to some of the routines.
 *
 * These MACROS require that the following define:
 *
 *	#define HBA_PREFIX your_driver_prefix
 *
 * be put in your_driver.h header file, and that the order of
 * include files be:
 *
 *	#include	all other includes
 *	#include	your_driver.h
 *	#include	<io/hba/hba.h>
 *	#include	<io/ddi.h>
 */
#if (PDI_VERSION <= PDI_UNIXWARE11)

#define SDI_GET(head, sleepflag) sdi_get(head, KM_SLEEP)
#define SDI_GETBLK(sleepflag) sdi_getblk()
#define SDI_TRANSLATE(sp, flag, procp, sleepflag) sdi_translate(sp, flag, procp)

#else /* (PDI_VERSION > PDI_UNIXWARE11) */

#define SDI_GETBLK(sleepflag) sdi_getblk(sleepflag)
#define SDI_TRANSLATE(sp, flag, procp, sleepflag) sdi_translate(sp, flag, procp, sleepflag)

#endif /* (PDI_VERSION > PDI_UNIXWARE11) */


/*
 * Prototypes for HBA driver entry points.
 * All versions supplied.
 */
#if (PDI_VERSION <= PDI_UNIXWARE11)

int			glue(HBA_PREFIX, init)(void);
int			glue(HBA_PREFIX, start)(void);
void			glue(HBA_PREFIX, intr)(unsigned int);
void			glue(HBA_PREFIX, halt)(void);
STATIC long		glue(HBA_PREFIX, send)(register struct hbadata *);
STATIC long		glue(HBA_PREFIX, icmd)(register struct hbadata *);
STATIC struct hbadata  *glue(HBA_PREFIX, getblk)(void);
STATIC long		glue(HBA_PREFIX, freeblk)(register struct hbadata *);
STATIC void		glue(HBA_PREFIX, getinfo)(struct scsi_ad *,struct hbagetinfo *);
STATIC int		glue(HBA_PREFIX, open)(dev_t *, int, int, cred_t *);
STATIC int		glue(HBA_PREFIX, close)(dev_t, int, int, cred_t *);
STATIC int		glue(HBA_PREFIX, ioctl)(dev_t, int, caddr_t, int, cred_t *, int *);
STATIC HBAXLAT_DECL	glue(HBA_PREFIX, xlat)(register struct hbadata *, int, struct proc *);
STATIC int 		glue(HBA_PREFIX, _load)(void);
STATIC int		glue(HBA_PREFIX, _unload)(void);

#else /* (PDI_VERSION > PDI_UNIXWARE11) */

int			glue(HBA_PREFIX, init)(void);
int			glue(HBA_PREFIX, start)(void);
void			glue(HBA_PREFIX, intr)(unsigned int);
void			glue(HBA_PREFIX, halt)(void);
STATIC long		glue(HBA_PREFIX, send)(register struct hbadata *, int);
STATIC long		glue(HBA_PREFIX, icmd)(register struct hbadata *, int);
STATIC struct hbadata  *glue(HBA_PREFIX, getblk)(int);
STATIC long		glue(HBA_PREFIX, freeblk)(register struct hbadata *);
STATIC void		glue(HBA_PREFIX, getinfo)(struct scsi_ad *,struct hbagetinfo *);
STATIC int		glue(HBA_PREFIX, open)(dev_t *, int, int, cred_t *);
STATIC int		glue(HBA_PREFIX, close)(dev_t, int, int, cred_t *);
STATIC int		glue(HBA_PREFIX, ioctl)(dev_t, int, caddr_t, int, cred_t *, int *);
STATIC HBAXLAT_DECL	glue(HBA_PREFIX, xlat)(register struct hbadata *, int, struct proc *, int);
STATIC int 		glue(HBA_PREFIX, _load)(void);
STATIC int		glue(HBA_PREFIX, _unload)(void);

#endif /* (PDI_VERSION > PDI_UNIXWARE11) */


/*
 * SDI prototypes provided for UnixWare 1.1 drivers
 * (These are defined in sdi.h for > UnixWare 1.1)
 */
#if (PDI_VERSION <= PDI_UNIXWARE11)
#if defined(_KERNEL) 

extern int 		sdi_access(struct sdi_edt *, int, struct owner *);
extern void 		sdi_blkio(buf_t *, unsigned int, void (*)());
extern void 		sdi_callback(struct sb *);
extern void 		sdi_clrconfig(struct owner *, int, void (*)());
extern struct owner *	sdi_doconfig(struct dev_cfg[], int, char *, 
				struct drv_majors *, void (*)());
extern void		sdi_errmsg(char *, struct scsi_ad *, struct sb *,
				struct sense *, int, int);
extern struct dev_spec * sdi_findspec(struct sdi_edt *, struct dev_spec *[]);
extern void		sdi_free(struct head *, struct jpool *);
extern long		sdi_freeblk(struct sb *);
extern struct jpool	*sdi_get(struct head *, int);
extern struct sb	*sdi_getblk();
extern void		sdi_getdev(struct scsi_ad *, dev_t *);
extern int		sdi_gethbano(int);
extern int		sdi_register(void *, void *);
extern int		sdi_icmd(struct sb *);
extern int		sdi_send(struct sb *);
extern short		sdi_swap16(uint);
extern int		sdi_swap24(uint);
extern long		sdi_swap32(ulong);
extern struct sdi_edt	*sdi_redt(int, int, int);
extern void		sdi_name(struct scsi_ad *, char *);
extern void		sdi_translate(struct sb *, int, proc_t *);
extern int		sdi_wedt(struct sdi_edt *, int, char *);

extern int		sdi_open(dev_t *, int, int, cred_t *);
extern int		sdi_close(dev_t , int, int, cred_t *);
extern int		sdi_ioctl(dev_t, int, caddr_t, int, cred_t *, int *);

#define		SC_EXHAN	SC_HAN
#define 	SC_EXTCN	SC_TCN
#define		SC_EXLUN	SC_LUN

#endif /* _KERNEL */
#endif /* (PDI_VERSION <= PDI_UNIXWARE11) */

#if defined(__cplusplus)
	}
#endif
#endif /* _IO_HBA_HBA_H */		/* wrapper symbol for kernel use */
