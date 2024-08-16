/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_SYSMACROS_H	/* wrapper symbol for kernel use */
#define _UTIL_SYSMACROS_H	/* subject to change without notice */

#ident	"@(#)kern:util/sysmacros.h	1.18"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Various system macros.
 */

#ifdef _KERNEL_HEADERS

#include <util/param.h>		/* SVR4.0COMPAT */
#include <util/sysmacros_f.h>	/* PORTABILITY */
#include <util/types.h>		/* REQUIRED */
#include <mem/immu.h>		/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/param.h>		/* SVR4.0COMPAT */
#include <sys/sysmacros_f.h>	/* PORTABILITY */
#include <sys/types.h>		/* REQUIRED */
#include <sys/immu.h>		/* PORTABILITY */

#else

#include <sys/param.h>		/* SVR4.0COMPAT */
#include <sys/sysmacros_f.h>	/* PORTABILITY */
#include <sys/immu.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

/*
 * Some macros for units conversion
 */

/*
 * Disk blocks (sectors) and bytes.
 */
#define	dtob(DD)	((DD) << SCTRSHFT)
#define	btod(BB)	(((BB) + NBPSCTR - 1) >> SCTRSHFT)
#define	btodt(BB)	((BB) >> SCTRSHFT)

/*
 * Disk blocks (sectors) and pages.
 */
#define	ptod(PP)	((PP) << DPPSHFT)
#define	dtop(DD)	(((DD) + NDPP - 1) >> DPPSHFT)
#define dtopt(DD)	((DD) >> DPPSHFT)

/* common macros */

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) < (b) ? (b) : (a))

#define SNEXT(p)	(char *)((int)(p) + sizeof (short))
#define INEXT(p)	(char *)((int)(p) + sizeof (int))
#define LNEXT(p)	(char *)((int)(p) + sizeof (long))

/* make a device number */
			
#define _MAKEDEVICE(major,minor)	\
		(dev_t)(((major_t)(major) << L_BITSMINOR) | \
			((minor_t)(minor) & L_MAXMIN))
#define makedevice(major,minor)		_MAKEDEVICE(major, minor)


#ifdef _KERNEL

/* make an old device number */

#define	o_makedev(major,minor)	\
		(o_dev_t)(((major) << O_BITSMINOR) | ((minor) & O_MAXMIN))

/* convert to old dev format */

#define _cmpdev(dev)	\
		o_makedev(_GETMAJOR(dev), _GETMINOR(dev))
#define cmpdev_fits(dev)	\
		(_GETMAJOR(dev) <= O_MAXMAJ && _GETMINOR(dev) <= O_MAXMIN)
#define cmpdev(dev)	\
		(cmpdev_fits(dev) ? _cmpdev(dev) : make_cmpdev(dev))

/* convert to new dev format */

#define expdev(dev) 	\
		_MAKEDEVICE((((dev_t)(dev) >> O_BITSMINOR) & O_MAXMAJ), \
			    ((dev_t)(dev) & O_MAXMIN))

#ifdef __STDC__
extern o_dev_t make_cmpdev(dev_t);
extern dev_t udev_getrdev(dev_t);
#endif

/*
 *  Evaluate to true if the process is an RFS server.
 */
#define	RF_SERVER()	(u.u_procp->p_sysid != 0)

#endif /* _KERNEL */


/*
 * Macros for counting and rounding.
 */
#define howmany(x, y)	(((x)+((y)-1))/(y))
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))

#if defined(__cplusplus)
#define offsetof(x,y)	((int)__INTADDR__(&((x *)0)->y))
#else
#define offsetof(x,y)	((int)&((x *)0)->y)
#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_SYSMACROS_H */
